// acs_controller.cpp
#include "ACSController.h"

#include <iostream>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iomanip>  // For std::setprecision

// Constructor - initialize with correct axis identifiers
ACSController::ACSController()
  : m_controllerId(ACSC_INVALID),
  m_port(ACSC_SOCKET_STREAM_PORT) {

  // Initialize atomic variables
  m_isConnected.store(false);
  m_threadRunning.store(false);
  m_terminateThread.store(false);

  std::cout << "ACSController: Initializing controller" << std::endl;

  // Initialize available axes with string identifiers (consistent with PI controller)
  m_availableAxes = { "X", "Y", "Z" };

  // Start communication thread
  StartCommunicationThread();
}

ACSController::~ACSController() {
  std::cout << "ACSController: Shutting down controller" << std::endl;

  // Stop communication thread
  StopCommunicationThread();

  // Disconnect if still connected
  if (m_isConnected) {
    Disconnect();
  }
}

void ACSController::StartCommunicationThread() {
  if (!m_threadRunning) {
    m_threadRunning.store(true);
    m_terminateThread.store(false);
    m_communicationThread = std::thread(&ACSController::CommunicationThreadFunc, this);
    std::cout << "ACSController: Communication thread started" << std::endl;
  }
}

void ACSController::StopCommunicationThread() {
  if (m_threadRunning) {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_terminateThread.store(true);
    }
    m_condVar.notify_all();

    if (m_communicationThread.joinable()) {
      m_communicationThread.join();
    }

    m_threadRunning.store(false);
    std::cout << "ACSController: Communication thread stopped" << std::endl;
  }
}

void ACSController::CommunicationThreadFunc() {
  // Set update interval to 200ms (5 Hz)
  const auto updateInterval = std::chrono::milliseconds(200);

  // Frame counter for less frequent updates
  int frameCounter = 0;

  // Initialization of last update timestamps
  m_lastStatusUpdate = std::chrono::steady_clock::now();
  m_lastPositionUpdate = m_lastStatusUpdate;

  while (!m_terminateThread) {
    auto cycleStartTime = std::chrono::steady_clock::now();

    // Process any pending motor commands first for responsiveness
    {
      std::lock_guard<std::mutex> lock(m_commandMutex);
      for (auto& cmd : m_commandQueue) {
        if (!cmd.executed) {
          // Execute the command
          MoveRelative(cmd.axis, cmd.distance, false);
          cmd.executed = true;
        }
      }

      // Remove executed commands
      m_commandQueue.erase(
        std::remove_if(m_commandQueue.begin(), m_commandQueue.end(),
          [](const MotorCommand& cmd) { return cmd.executed; }),
        m_commandQueue.end());
    }

    // Only update if connected
    if (m_isConnected) {
      frameCounter++;

      // Always update positions
      std::map<std::string, double> positions;
      if (GetPositions(positions)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_axisPositions = positions;
        m_lastPositionUpdate = std::chrono::steady_clock::now();
      }

      // Update other status less frequently (every 3rd frame, ~1.67Hz)
      if (frameCounter % 3 == 0) {
        // Update axis motion status for X, Y, Z only
        for (const auto& axis : { "X", "Y", "Z" }) {
          bool moving = IsMoving(axis);
          std::lock_guard<std::mutex> lock(m_mutex);
          m_axisMoving[axis] = moving;
        }

        // Update servo status for X, Y, Z only
        for (const auto& axis : { "X", "Y", "Z" }) {
          bool enabled;
          if (IsServoEnabled(axis, enabled)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_axisServoEnabled[axis] = enabled;
          }
        }

        m_lastStatusUpdate = std::chrono::steady_clock::now();
      }
    }

    // Calculate how long to sleep to maintain consistent update rate
    auto cycleEndTime = std::chrono::steady_clock::now();
    auto cycleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
      cycleEndTime - cycleStartTime);
    auto sleepTime = updateInterval - cycleDuration;

    // Wait for next update or termination
    std::unique_lock<std::mutex> lock(m_mutex);
    if (sleepTime.count() > 0) {
      m_condVar.wait_for(lock, sleepTime, [this]() { return m_terminateThread.load(); });
    }
    else {
      // No sleep needed if we're already behind schedule, but yield to let other threads run
      lock.unlock();
      std::this_thread::yield();
    }
  }
}

// Helper method to process the command queue
void ACSController::ProcessCommandQueue() {
  std::lock_guard<std::mutex> lock(m_commandMutex);
  for (auto& cmd : m_commandQueue) {
    if (!cmd.executed) {
      // Execute the command
      MoveRelative(cmd.axis, cmd.distance, false);
      cmd.executed = true;
    }
  }

  // Remove executed commands
  m_commandQueue.erase(
    std::remove_if(m_commandQueue.begin(), m_commandQueue.end(),
      [](const MotorCommand& cmd) { return cmd.executed; }),
    m_commandQueue.end());
}

// Helper method to update positions using batch query
void ACSController::UpdatePositions() {
  if (!m_isConnected) return;

  std::map<std::string, double> positions;
  if (GetPositions(positions)) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_axisPositions = positions;
    m_lastPositionUpdate = std::chrono::steady_clock::now();
  }
}

// Helper method to update motor status (moving, servo state)
void ACSController::UpdateMotorStatus() {
  if (!m_isConnected) return;

  auto now = std::chrono::steady_clock::now();

  // Use batch queries if possible, otherwise query each axis
  for (const auto& axis : m_availableAxes) {
    int axisIndex = GetAxisIndex(axis);
    if (axisIndex >= 0) {
      int state = 0;
      if (acsc_GetMotorState(m_controllerId, axisIndex, &state, NULL)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_axisMoving[axis] = (state & ACSC_MST_MOVE) != 0;
        m_axisServoEnabled[axis] = (state & ACSC_MST_ENABLE) != 0;
      }
    }
  }

  m_lastStatusUpdate = now;
}

// Helper to convert string axis identifiers to ACS axis indices
int ACSController::GetAxisIndex(const std::string& axis) {
  if (axis == "X") return ACSC_AXIS_X;
  if (axis == "Y") return ACSC_AXIS_Y;
  if (axis == "Z") return ACSC_AXIS_Z;

  std::cout << "ACSController: WARNING - Unknown axis identifier: " << axis << std::endl;
  return -1;
}

bool ACSController::Connect(const std::string& ipAddress, int port) {
  // Check if already connected
  if (m_isConnected) {
    std::cout << "ACSController: WARNING - Already connected to a controller" << std::endl;
    return true;
  }

  std::cout << "ACSController: Attempting connection to " << ipAddress << ":" << port << std::endl;

  // Store connection parameters
  m_ipAddress = ipAddress;
  m_port = port;

  // Attempt to connect to the controller
  // ACS requires a non-const char buffer for the IP address
  char ipBuffer[64];
  strncpy(ipBuffer, m_ipAddress.c_str(), sizeof(ipBuffer) - 1);
  ipBuffer[sizeof(ipBuffer) - 1] = '\0'; // Ensure null termination

  m_controllerId = acsc_OpenCommEthernet(ipBuffer, m_port);

  if (m_controllerId == ACSC_INVALID) {
    int errorCode = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to connect to controller. Error code: " << errorCode << std::endl;
    return false;
  }

  m_isConnected.store(true);
  std::cout << "ACSController: Successfully connected to " << ipAddress << std::endl;

  // Enable all configured axes
  for (const auto& axis : m_availableAxes) {
    int axisIndex = GetAxisIndex(axis);
    if (axisIndex >= 0) {
      if (acsc_Enable(m_controllerId, axisIndex, NULL)) {
        std::cout << "ACSController: Enabled axis " << axis << std::endl;
      }
      else {
        int error = acsc_GetLastError();
        std::cout << "ACSController: ERROR - Failed to enable axis " << axis << ". Error: " << error << std::endl;
      }
    }
  }

  // Initialize position cache immediately
  std::map<std::string, double> initialPositions;
  if (GetPositions(initialPositions)) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_axisPositions = initialPositions;
    m_lastPositionUpdate = std::chrono::steady_clock::now();

    // Log initial positions for debugging
    if (m_enableDebug) {
      std::stringstream ss;
      ss << "Initial positions: ";
      for (const auto& [axis, pos] : initialPositions) {
        ss << axis << "=" << pos << " ";
      }
      std::cout << "ACSController: " << ss.str() << std::endl;
    }
  }
  else {
    std::cout << "ACSController: WARNING - Failed to initialize position cache after connection" << std::endl;
  }

  return true;
}

// Updated ACSController::Disconnect() method - changed from void to bool

bool ACSController::Disconnect() {
  if (!m_isConnected) {
    std::cout << "ACSController: Already disconnected" << std::endl;
    return true;  // Already disconnected is considered success
  }

  std::cout << "ACSController: Disconnecting from controller" << std::endl;

  bool success = true;

  // Ensure all axes are stopped before disconnecting
  if (!StopAllAxes()) {
    std::cout << "ACSController: WARNING - Failed to stop all axes before disconnect" << std::endl;
    success = false;  // Note the failure but continue with disconnect
  }

  // Close connection
  if (acsc_CloseComm(m_controllerId) == 0) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to close communication. Error code: " << error << std::endl;
    success = false;
  }

  // Always update connection state regardless of close result
  m_isConnected.store(false);
  m_controllerId = ACSC_INVALID;

  if (success) {
    std::cout << "ACSController: Successfully disconnected from controller" << std::endl;
  }
  else {
    std::cout << "ACSController: Disconnected with warnings/errors" << std::endl;
  }

  return success;
}
bool ACSController::MoveToPosition(const std::string& axis, double position, bool blocking) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot move axis - not connected" << std::endl;
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  std::cout << "ACSController: Moving axis " << axis << " to position " << position << std::endl;

  // Set up arrays for acsc_ToPointM
  int axes[2] = { axisIndex, -1 }; // -1 marks the end of the array
  double points[1] = { position };

  // Command the move - using absolute positioning
  if (!acsc_ToPointM(m_controllerId, ACSC_AMF_WAIT, axes, points, NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to move axis. Error code: " << error << std::endl;
    return false;
  }

  // Start the motion
  if (!StartMotion(axis)) {
    return false;
  }

  // If blocking mode, wait for motion to complete
  if (blocking) {
    return WaitForMotionCompletion(axis);
  }

  return true;
}

bool ACSController::MoveRelative(const std::string& axis, double distance, bool blocking) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot move axis - not connected" << std::endl;
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  std::cout << "ACSController: Moving axis " << axis << " relative distance " << distance << std::endl;

  // Log pre-move position if debugging is enabled
  if (m_enableDebug) {
    double currentPos = 0.0;
    if (GetPosition(axis, currentPos)) {
      std::cout << "ACSController: Pre-move position of axis " << axis << " = " << currentPos << std::endl;
    }
  }

  // Set up arrays for acsc_ToPointM with RELATIVE flag
  int axes[2] = { axisIndex, -1 }; // -1 marks the end of the array
  double distances[1] = { distance };

  // Command the relative move
  if (!acsc_ToPointM(m_controllerId, ACSC_AMF_WAIT | ACSC_AMF_RELATIVE, axes, distances, NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to move axis relatively. Error code: " << error << std::endl;
    return false;
  }

  // Start the motion
  if (!StartMotion(axis)) {
    return false;
  }

  // If blocking mode, wait for motion to complete
  if (blocking) {
    return WaitForMotionCompletion(axis);
  }

  return true;
}

bool ACSController::HomeAxis(const std::string& axis) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot home axis - not connected" << std::endl;
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  std::cout << "ACSController: Homing axis " << axis << std::endl;

  // Option 1: Use a direct FaultClear + Home sequence
  if (!acsc_FaultClear(m_controllerId, axisIndex, NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to clear faults for homing. Error code: " << error << std::endl;
    // Continue anyway as the axis might not have faults
  }

  // Wait for homing to complete
  return WaitForMotionCompletion(axis);
}

bool ACSController::StopAxis(const std::string& axis) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot stop axis - not connected" << std::endl;
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  std::cout << "ACSController: Stopping axis " << axis << std::endl;

  // Command the stop
  if (!acsc_Halt(m_controllerId, axisIndex, NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to stop axis. Error code: " << error << std::endl;
    return false;
  }

  return true;
}

bool ACSController::StopAllAxes() {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot stop all axes - not connected" << std::endl;
    return false;
  }

  std::cout << "ACSController: Stopping all axes" << std::endl;

  // Command the stop for all axes
  if (!acsc_KillAll(m_controllerId, NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to stop all axes. Error code: " << error << std::endl;
    return false;
  }

  return true;
}

bool ACSController::IsMoving(const std::string& axis) {
  if (!m_isConnected) {
    return false;
  }

  // Check if we have a recent cached value (less than 200ms old)
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
    now - m_lastStatusUpdate).count();

  if (elapsed < m_statusUpdateInterval) {
    // Use cached value if it exists and is recent
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_axisMoving.find(axis);
    if (it != m_axisMoving.end()) {
      return it->second;
    }
  }

  // If no recent cached value, do direct query
  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  // Get the motion state
  int state = 0;
  if (!acsc_GetMotorState(m_controllerId, axisIndex, &state, NULL)) {
    return false;
  }

  // Check if the axis is moving based on state bits
  bool isMoving = (state & ACSC_MST_MOVE) != 0;

  // Update the cache
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_axisMoving[axis] = isMoving;
    m_lastStatusUpdate = now;
  }

  return isMoving;
}

bool ACSController::GetPosition(const std::string& axis, double& position) {
  if (!m_isConnected) {
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  if (!acsc_GetFPosition(m_controllerId, axisIndex, &position, NULL)) {
    int error = acsc_GetLastError();
    // Only log in debug mode to reduce overhead
    if (m_enableDebug) {
      std::cout << "ACSController: Error getting position for axis " << axis << ": " << error << std::endl;
    }
    return false;
  }

  return true;
}

bool ACSController::GetPositions(std::map<std::string, double>& positions) {
  if (!m_isConnected || m_availableAxes.empty()) {
    return false;
  }

  // Since we now have only X, Y, Z axes, we can optimize this for exactly 3 axes
  // Create arrays for the axes we know we have
  int axisIndices[3] = {
      GetAxisIndex("X"), GetAxisIndex("Y"), GetAxisIndex("Z")
  };
  double posArray[3] = { 0.0 };

  // Query positions individually for each axis
  // This could be optimized with a batch call if the ACS API supports it
  bool success = true;
  for (int i = 0; i < 3; i++) {
    if (axisIndices[i] >= 0) {
      if (!acsc_GetFPosition(m_controllerId, axisIndices[i], &posArray[i], NULL)) {
        success = false;
      }
    }
  }

  if (success) {
    // Fill the map with results
    for (int i = 0; i < 3; i++) {
      if (axisIndices[i] >= 0) {
        positions[m_availableAxes[i]] = posArray[i];
      }
    }
  }

  return success;
}

bool ACSController::EnableServo(const std::string& axis, bool enable) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot change servo state - not connected" << std::endl;
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  std::cout << "ACSController: Setting servo state for axis " << axis << " to "
    << (enable ? "enabled" : "disabled") << std::endl;

  // Enable or disable the servo
  bool result = false;
  if (enable) {
    result = acsc_Enable(m_controllerId, axisIndex, NULL) != 0;
  }
  else {
    result = acsc_Disable(m_controllerId, axisIndex, NULL) != 0;
  }

  if (!result) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to set servo state. Error code: " << error << std::endl;
  }

  return result;
}

bool ACSController::IsServoEnabled(const std::string& axis, bool& enabled) {
  if (!m_isConnected) {
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  // Get the motor state
  int state = 0;
  if (!acsc_GetMotorState(m_controllerId, axisIndex, &state, NULL)) {
    return false;
  }

  // Check if the axis is enabled based on state bits
  enabled = (state & ACSC_MST_ENABLE) != 0;
  return true;
}

bool ACSController::SetVelocity(const std::string& axis, double velocity) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot set velocity - not connected" << std::endl;
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  std::cout << "ACSController: Setting velocity for axis " << axis << " to " << velocity << std::endl;

  // Set the velocity
  if (!acsc_SetVelocity(m_controllerId, axisIndex, velocity, NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to set velocity. Error code: " << error << std::endl;
    return false;
  }

  return true;
}

bool ACSController::GetVelocity(const std::string& axis, double& velocity) {
  if (!m_isConnected) {
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  // Get the velocity
  if (!acsc_GetVelocity(m_controllerId, axisIndex, &velocity, NULL)) {
    return false;
  }

  return true;
}

bool ACSController::WaitForMotionCompletion(const std::string& axis, double timeoutSeconds) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot wait for motion completion - not connected" << std::endl;
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    return false;
  }

  std::cout << "ACSController: Waiting for motion completion on axis " << axis << std::endl;

  // Use system clock for timeout
  auto startTime = std::chrono::steady_clock::now();
  int checkCount = 0;

  while (true) {
    checkCount++;
    bool stillMoving = IsMoving(axis);

    if (!stillMoving) {
      std::cout << "ACSController: Motion completed on axis " << axis << std::endl;
      return true;
    }

    // Check for timeout
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

    if (elapsedSeconds > timeoutSeconds) {
      std::cout << "ACSController: WARNING - Timeout waiting for motion completion on axis " << axis << std::endl;
      return false;
    }

    // Sleep to avoid CPU spikes
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

bool ACSController::ConfigureFromDevice(const MotionDevice& device) {
  if (m_isConnected) {
    std::cout << "ACSController: WARNING - Cannot configure from device while connected" << std::endl;
    return false;
  }

  m_deviceName = device.Name;
  std::cout << "ACSController: Configuring from device: " << device.Name << std::endl;

  // Store the IP address and port from the device configuration
  m_ipAddress = device.IpAddress;
  m_port = device.Port;

  // Define available axes based on device configuration
  m_availableAxes.clear();

  // If InstalledAxes is specified, use it
  if (!device.InstalledAxes.empty()) {
    // Since InstalledAxes may be space-separated (e.g., "X Y Z"),
    // we need to parse it by splitting on spaces

    std::string axisStr = device.InstalledAxes;
    std::string delimiter = " ";
    size_t pos = 0;
    std::string token;

    // Parse the space-separated string
    while ((pos = axisStr.find(delimiter)) != std::string::npos) {
      token = axisStr.substr(0, pos);
      if (!token.empty()) {
        m_availableAxes.push_back(token);
      }
      axisStr.erase(0, pos + delimiter.length());
    }

    // Add the last token if there is one
    if (!axisStr.empty()) {
      m_availableAxes.push_back(axisStr);
    }

    // Log the configured axes
    std::string axesList;
    for (const auto& axis : m_availableAxes) {
      if (!axesList.empty()) axesList += " ";
      axesList += axis;
    }

    std::cout << "ACSController: Configured with specified axes: " << axesList << std::endl;
  }
  // Otherwise, use defaults based on device type (historically ACS controllers use X, Y, Z)
  else {
    m_availableAxes = { "X", "Y", "Z" };
    std::cout << "ACSController: Configured with default gantry axes (X Y Z)" << std::endl;
  }

  return true;
}



bool ACSController::StartMotion(const std::string& axis) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot start motion - not connected" << std::endl;
    return false;
  }

  int axisIndex = GetAxisIndex(axis);
  if (axisIndex < 0) {
    std::cout << "ACSController: ERROR - Invalid axis for starting motion: " << axis << std::endl;
    return false;
  }

  std::cout << "ACSController: Starting motion on axis " << axis << std::endl;

  // Set up axis array for GoM (ending with -1)
  int axes[2] = { axisIndex, -1 };

  // Call acsc_GoM to start the motion
  if (!acsc_GoM(m_controllerId, axes, NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to start motion on axis " << axis
      << ". Error code: " << error << std::endl;
    return false;
  }

  return true;
}

bool ACSController::MoveToPositionMultiAxis(const std::vector<std::string>& axes,
  const std::vector<double>& positions,
  bool blocking) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot move axes - not connected" << std::endl;
    return false;
  }

  if (axes.size() != positions.size() || axes.empty()) {
    std::cout << "ACSController: ERROR - Invalid axes/positions arrays for multi-axis move" << std::endl;
    return false;
  }

  // Log the motion command
  std::stringstream ss;
  ss << "ACSController: Moving multiple axes to positions: ";
  for (size_t i = 0; i < axes.size(); i++) {
    ss << axes[i] << "=" << positions[i] << " ";
  }
  std::cout << ss.str() << std::endl;

  // Convert string axes to ACS axis indices
  std::vector<int> axisIndices;
  for (const auto& axis : axes) {
    int axisIndex = GetAxisIndex(axis);
    if (axisIndex < 0) {
      std::cout << "ACSController: ERROR - Invalid axis: " << axis << std::endl;
      return false;
    }
    axisIndices.push_back(axisIndex);
  }

  // Set up arrays for acsc_ToPointM
  std::vector<int> axesArray(axisIndices.size() + 1);  // +1 for the terminating -1
  for (size_t i = 0; i < axisIndices.size(); i++) {
    axesArray[i] = axisIndices[i];
  }
  axesArray[axisIndices.size()] = -1;  // Mark the end of the array with -1

  // Command the move using acsc_ToPointM
  if (!acsc_ToPointM(m_controllerId, ACSC_AMF_WAIT, axesArray.data(),
    const_cast<double*>(positions.data()), NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to move axes. Error code: " << error << std::endl;
    return false;
  }

  // Start the motion - for multi-axis we'll need to use GoM instead of Go
  if (!acsc_GoM(m_controllerId, axesArray.data(), NULL)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to start motion. Error code: " << error << std::endl;
    return false;
  }

  // If blocking mode, wait for motion to complete on all axes
  if (blocking) {
    bool allCompleted = true;
    for (const auto& axis : axes) {
      if (!WaitForMotionCompletion(axis)) {
        std::cout << "ACSController: ERROR - Timeout waiting for motion completion on axis " << axis << std::endl;
        allCompleted = false;
      }
    }
    return allCompleted;
  }

  return true;
}



bool ACSController::RunBuffer(int bufferNumber, const std::string& labelName) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot run buffer - not connected" << std::endl;
    return false;
  }

  // Validate buffer number (0-63 depending on controller)
  if (bufferNumber < 0 || bufferNumber > 63) {
    std::cout << "ACSController: ERROR - Invalid buffer number " << bufferNumber
      << ". Must be between 0 and 63" << std::endl;
    return false;
  }

  // Prepare label parameter
  char* labelPtr = nullptr;
  char labelBuffer[256] = { 0 };

  if (!labelName.empty()) {
    // Validate label name (must start with underscore or A-Z)
    std::string upperLabel = labelName;
    std::transform(upperLabel.begin(), upperLabel.end(), upperLabel.begin(), ::toupper);

    if (upperLabel[0] != '_' && (upperLabel[0] < 'A' || upperLabel[0] > 'Z')) {
      std::cout << "ACSController: ERROR - Invalid label name '" << labelName
        << "'. Label must start with underscore or letter A-Z" << std::endl;
      return false;
    }

    // Copy to buffer for ACS API
    strncpy(labelBuffer, upperLabel.c_str(), sizeof(labelBuffer) - 1);
    labelBuffer[sizeof(labelBuffer) - 1] = '\0';
    labelPtr = labelBuffer;

    std::cout << "ACSController: Running buffer " << bufferNumber << " from label " << labelName << std::endl;
  }
  else {
    std::cout << "ACSController: Running buffer " << bufferNumber << " from start" << std::endl;
  }

  // Call ACS API function
  if (!acsc_RunBuffer(m_controllerId, bufferNumber, labelPtr, ACSC_SYNCHRONOUS)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to run buffer " << bufferNumber
      << ". Error code: " << error << std::endl;
    return false;
  }

  std::cout << "ACSController: Successfully started buffer " << bufferNumber << std::endl;
  return true;
}

bool ACSController::StopBuffer(int bufferNumber) {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot stop buffer - not connected" << std::endl;
    return false;
  }

  // Validate buffer number (0-63 depending on controller)
  if (bufferNumber < 0 || bufferNumber > 63) {
    std::cout << "ACSController: ERROR - Invalid buffer number " << bufferNumber
      << ". Must be between 0 and 63" << std::endl;
    return false;
  }

  std::cout << "ACSController: Stopping buffer " << bufferNumber << std::endl;

  // Call ACS API function
  if (!acsc_StopBuffer(m_controllerId, bufferNumber, ACSC_SYNCHRONOUS)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to stop buffer " << bufferNumber
      << ". Error code: " << error << std::endl;
    return false;
  }

  std::cout << "ACSController: Successfully stopped buffer " << bufferNumber << std::endl;
  return true;
}

bool ACSController::StopAllBuffers() {
  if (!m_isConnected) {
    std::cout << "ACSController: ERROR - Cannot stop all buffers - not connected" << std::endl;
    return false;
  }

  std::cout << "ACSController: Stopping all buffers" << std::endl;

  // Use ACSC_NONE to stop all buffers
  if (!acsc_StopBuffer(m_controllerId, ACSC_NONE, ACSC_SYNCHRONOUS)) {
    int error = acsc_GetLastError();
    std::cout << "ACSController: ERROR - Failed to stop all buffers. Error code: " << error << std::endl;
    return false;
  }

  std::cout << "ACSController: Successfully stopped all buffers" << std::endl;
  return true;
}




// =====================================================
// ACSController Methods - NO HANDLE CHECK VERSION
// =====================================================

bool ACSController::GetFirmwareVersion(std::string& firmwareVersion) {
  char versionBuffer[256] = { 0 };
  int received = 0;

  // Use acsc_GetFirmwareVersion API
  int result = acsc_GetFirmwareVersion(m_controllerId, versionBuffer, sizeof(versionBuffer), &received, ACSC_IGNORE);

  if (result != 0 && received > 0) {
    firmwareVersion = std::string(versionBuffer, received);
    // Clean up any null terminators or extra whitespace
    firmwareVersion.erase(firmwareVersion.find_last_not_of("\0\r\n\t ") + 1);
    return true;
  }

  return false;
}

bool ACSController::GetSerialNumber(std::string& serialNumber) {
  char serialBuffer[256] = { 0 };
  int received = 0;

  // Use acsc_GetSerialNumber API
  int result = acsc_GetSerialNumber(m_controllerId, serialBuffer, sizeof(serialBuffer), &received, ACSC_IGNORE);

  if (result != 0 && received > 0) {
    serialNumber = std::string(serialBuffer, received);
    // Clean up any null terminators or extra whitespace
    serialNumber.erase(serialNumber.find_last_not_of("\0\r\n\t ") + 1);
    return true;
  }

  return false;
}

bool ACSController::GetDeviceIdentification(std::string& manufacturerInfo) {
  std::string firmware, serial;
  bool hasFirmware = GetFirmwareVersion(firmware);
  bool hasSerial = GetSerialNumber(serial);

  if (!hasFirmware && !hasSerial) {
    return false;
  }

  // Combine firmware and serial information
  manufacturerInfo = "ACS Controller";

  if (hasFirmware) {
    manufacturerInfo += " | Firmware: " + firmware;
  }

  if (hasSerial) {
    manufacturerInfo += " | Serial: " + serial;
  }

  return true;
}