// pi_controller.cpp
#include "PIController.h"


#include <iostream>
#include <chrono>
#include <sstream>
#include <algorithm>

// Modify the constructor to initialize timestamps
// Updated constructor - initialize analog reading
PIController::PIController()
	: m_controllerId(-1),
	m_port(50000),
	m_lastStatusUpdate(std::chrono::steady_clock::now()),
	m_lastPositionUpdate(std::chrono::steady_clock::now()) {

	// Initialize atomic variables
	m_isConnected.store(false);
	m_threadRunning.store(false);
	m_terminateThread.store(false);
	m_enableAnalogReading.store(true);  // Enable analog reading by default

	//m_logger = Logger::GetInstance();
	
	std::cout << "PIController: Initializing controller" << std::endl;
	//// Get global data store instance
	//m_dataStore = GlobalDataStore::GetInstance();

	// Initialize available axes
	m_availableAxes = { "X", "Y", "Z", "U", "V", "W" };

	// Start communication thread
	StartCommunicationThread();
}

PIController::~PIController() {
	
	std::cout << "PIController: Shutting down controller" << std::endl;
	// Stop communication thread
	StopCommunicationThread();

	// Disconnect if still connected
	if (m_isConnected) {
		Disconnect();
	}
}

void PIController::StartCommunicationThread() {
	if (!m_threadRunning) {
		m_threadRunning.store(true);
		m_terminateThread.store(false);
		m_communicationThread = std::thread(&PIController::CommunicationThreadFunc, this);
		
		std::cout << "PIController: Communication thread started" << std::endl;
	}
}

void PIController::StopCommunicationThread() {
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
		
		std::cout << "PIController: Communication thread stopped" << std::endl;
	}
}


// Add this improved version to the CommunicationThreadFunc
// 4. Update CommunicationThreadFunc in pi_controller.cpp
// Updated communication thread - now includes analog reading
void PIController::CommunicationThreadFunc() {
	const auto updateInterval = std::chrono::milliseconds(50);  // 20Hz update rate
	int frameCounter = 0;

	while (!m_terminateThread) {
		if (m_isConnected) {
			frameCounter++;

			// Always update positions
			std::map<std::string, double> positions;
			if (GetPositions(positions)) {
				std::lock_guard<std::mutex> lock(m_mutex);
				m_axisPositions = positions;
			}

			// Update motion status
			{
				const char* allAxes = "X Y Z U V W";
				BOOL isMovingArray[6] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE };

				if (PI_IsMoving(m_controllerId, allAxes, isMovingArray)) {
					std::lock_guard<std::mutex> lock(m_mutex);
					const std::vector<std::string> axisNames = { "X", "Y", "Z", "U", "V", "W" };

					for (int i = 0; i < 6; i++) {
						m_axisMoving[axisNames[i]] = (isMovingArray[i] == TRUE);
					}
				}
			}

			// Update servo status less frequently
			if (frameCounter % 3 == 0) {
				for (const auto& axis : m_availableAxes) {
					bool enabled;
					if (IsServoEnabled(axis, enabled)) {
						std::lock_guard<std::mutex> lock(m_mutex);
						m_axisServoEnabled[axis] = enabled;
					}
				}
			}

			// NEW: Update analog readings every frame (if enabled)
			if (m_enableAnalogReading && frameCounter % 2 == 0) {  // Update analog every other frame (10Hz)
				UpdateAnalogReadings();
			}
		}

		// Wait for next update or termination
		std::unique_lock<std::mutex> lock(m_mutex);
		m_condVar.wait_for(lock, updateInterval, [this]() { return m_terminateThread.load(); });
	}
}

// NEW: Update analog readings in communication thread
void PIController::UpdateAnalogReadings() {
	if (!m_isConnected || !m_enableAnalogReading || m_activeAnalogChannels.empty()) {
		return;
	}

	std::map<int, double> voltages;
	if (GetAnalogVoltages(m_activeAnalogChannels, voltages)) {
		// Update cached values
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_analogVoltages = voltages;
		}

		//// Update global data store
		//if (m_dataStore && !m_deviceName.empty()) {
		//	for (const auto& [channel, voltage] : voltages) {
		//		std::string dataKey = m_deviceName + "-A-" + std::to_wstring(channel);
		//		m_dataStore->SetValue(dataKey, static_cast<float>(voltage));
		//	}
		//}
	}
}

// NEW: Get analog channel count
bool PIController::GetAnalogChannelCount(int& numChannels) {
	if (!m_isConnected) {
		return false;
	}

	if (!PI_qTAC(m_controllerId, &numChannels)) {
		int error = PI_GetError(m_controllerId);
		if (m_debugVerbose) {
			
			std::cout << "PIController: Failed to get analog channel count. Error: " << error << std::endl;
		}
		return false;
	}

	return true;
}

// NEW: Get single analog voltage
bool PIController::GetAnalogVoltage(int channel, double& voltage) {
	if (!m_isConnected) {
		return false;
	}

	int channelId = channel;
	if (!PI_qTAV(m_controllerId, &channelId, &voltage, 1)) {
		int error = PI_GetError(m_controllerId);
		if (m_debugVerbose) {
			
			std::cout << "PIController: Failed to read analog channel " << channel
				<< ". Error: " << error << std::endl;
		}
		return false;
	}

	return true;
}

// NEW: Get multiple analog voltages
bool PIController::GetAnalogVoltages(std::vector<int> channels, std::map<int, double>& voltages) {
	if (!m_isConnected || channels.empty()) {
		return false;
	}

	// Convert vector to array
	std::vector<int> channelIds = channels;
	std::vector<double> values(channels.size(), 0.0);

	if (!PI_qTAV(m_controllerId, channelIds.data(), values.data(), static_cast<int>(channels.size()))) {
		int error = PI_GetError(m_controllerId);
		if (m_debugVerbose) {
			
			std::cout << "PIController: Failed to read analog channels. Error: " << error << std::endl;
		}
		return false;
	}

	// Convert to map
	voltages.clear();
	for (size_t i = 0; i < channels.size(); i++) {
		voltages[channels[i]] = values[i];
	}

	return true;
}

// Also enhance the Connect function to check connection details
// Modified Connect function to initialize status structures

// Updated Connect method - initialize analog channels
bool PIController::Connect(const std::string& ipAddress, int port) {
	if (m_isConnected) {
		
		std::cout << "PIController: Already connected to a controller" << std::endl;
		return true;
	}

	
	std::cout << "PIController: Connecting to controller at " << ipAddress << ":" << port << std::endl;

	m_ipAddress = ipAddress;
	m_port = port;

	// Attempt to connect
	m_controllerId = PI_ConnectTCPIP(m_ipAddress.c_str(), m_port);

	if (m_controllerId < 0) {
		int errorCode = PI_GetInitError();
		
		std::cout << "PIController: Failed to connect to controller at " << ipAddress << ":" << port
			<< ". Error code: " << errorCode << std::endl;
		return false;
	}

	m_isConnected.store(true);
	
	std::cout << "PIController: Successfully connected to controller with ID: " << m_controllerId << std::endl;

	// Initialize the position and status maps
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (const auto& axis : m_availableAxes) {
			m_axisPositions[axis] = 0.0;
			m_axisMoving[axis] = false;
			m_axisServoEnabled[axis] = false;
		}
		m_lastStatusUpdate = std::chrono::steady_clock::now();
		m_lastPositionUpdate = std::chrono::steady_clock::now();
	}

	// Initialize controller
	PI_INI(m_controllerId, NULL);

	// NEW: Initialize analog channels
	InitializeAnalogChannels();

	// Update cached positions and statuses
	std::map<std::string, double> positions;
	if (GetPositions(positions)) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_axisPositions = positions;
	}

	return true;
}

// NEW: Initialize analog channels
void PIController::InitializeAnalogChannels() {
	if (!m_isConnected || !m_enableAnalogReading) {
		return;
	}

	// Get number of analog channels
	if (GetAnalogChannelCount(m_numAnalogChannels)) {
		
		std::cout << "PIController: Found " << m_numAnalogChannels << " analog channels" << std::endl;
		// Initialize analog voltage cache
		std::lock_guard<std::mutex> lock(m_mutex);
		for (int channel : m_activeAnalogChannels) {
			if (channel <= m_numAnalogChannels) {
				m_analogVoltages[channel] = 0.0;
			}
		}
	}
	else {
		
		std::cout << "PIController: Could not determine number of analog channels" << std::endl;
	}
}




void PIController::Disconnect() {
	if (!m_isConnected) {
		return;
	}
	StopCommunicationThread();
	
	std::cout << "PIController: Disconnecting from controller" << std::endl;

	// Ensure all axes are stopped before disconnecting
	StopAllAxes();

	// Close connection
	PI_CloseConnection(m_controllerId);

	m_isConnected.store(false);
	m_controllerId = -1;

	std::cout << "PIController: Disconnected from controller" << std::endl;
}

// MoveToPosition optimized to use cached data for status
bool PIController::MoveToPosition(const std::string& axis, double position, bool blocking) {
	if (!m_isConnected) {
		std::cout << "PIController: Cannot move axis - not connected" << std::endl;
		return false;
	}

	// Only log at debug level to reduce overhead
	if (m_enableDebug) {
		std::cout << "PIController: Moving axis " << axis << " to position " << position << std::endl;
	}

	// Convert single-axis string to char array for PI GCS2 API
	const char* axes = axis.c_str();
	double positions[1] = { position };

	// Command the move
	if (!PI_MOV(m_controllerId, axes, positions)) {
		int error = 0;
		PI_qERR(m_controllerId, &error);
		std::cout << "PIController: Failed to move axis " << axis << " to position " << position << ". Error code: " << error << std::endl;
		return false;
	}

	// Update the cache to reflect we're now moving
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_axisMoving[axis] = true;
	}

	// If blocking mode, wait for motion to complete
	if (blocking) {
		return WaitForMotionCompletion(axis);
	}

	return true;
}

// Add these improved logging sections to the MoveRelative function in pi_controller.cpp

// Update MoveRelative function to use correct identifiers
// 5. Update the MoveRelative function in pi_controller.cpp
bool PIController::MoveRelative(const std::string& axis, double distance, bool blocking) {
	if (!m_isConnected) {
		std::cout << "PIController: Cannot move axis - not connected" << std::endl;
		return false;
	}

	// Only log if verbose is enabled
	if (m_debugVerbose) {

		std::cout << "PIController: START Moving axis " + axis + " relative distance " + std::to_string(distance) << std::endl;
		std::cout << "PIController: Controller ID = " << m_controllerId << ", IsConnected = " << (m_isConnected ? "true" : "false") << std::endl;
	}

	// Use the correct axis identifier directly
	const char* axes = axis.c_str();
	double distances[1] = { distance };

	// Log pre-move position only if verbose debugging is enabled
	if (m_debugVerbose) {
		double currentPos = 0.0;
		if (GetPosition(axis, currentPos)) {
			std::cout << "PIController: Pre-move position of axis " << axis << " = " << currentPos << std::endl;
		}
		else {
			std::cout << "PIController: Failed to get pre-move position of axis " << axis << std::endl;
		}
	}

	// Command the relative move - only log if verbose is enabled
	if (m_debugVerbose) {
		std::cout << "PIController: Sending MVR command with axis=" << axis << ", distance=" << distance << std::endl;
	}

	bool moveResult = PI_MVR(m_controllerId, axes, distances);

	if (!moveResult) {
		int error = 0;
		PI_qERR(m_controllerId, &error);
		std::string errorMsg = "PIController: Failed to move axis relatively. Error code: " + std::to_string(error);

		std::cout << "PIController: Failed to move axis relatively. Error code: " << error << std::endl;

		if (m_debugVerbose) {
			std::cout << errorMsg << std::endl;

			// Add detailed error information
			char errorText[256] = { 0 };
			if (PI_TranslateError(error, errorText, sizeof(errorText))) {
				std::cout << "PIController: Error translation: " << errorText << std::endl;
			}
		}

		return false;
	}

	if (m_debugVerbose) {
		std::cout << "PIController: MVR command sent successfully" << std::endl;
	}

	// *** KEY FIX: IMMEDIATELY UPDATE THE MOVING STATUS AFTER SENDING THE COMMAND ***
	// This ensures that the UI reflects that the axis is moving right away
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_axisMoving[axis] = true;

		if (m_debugVerbose) {
			std::cout << "PIController: Manually set axis " << axis << " movement status to MOVING" << std::endl;
		}
	}

	// If blocking mode, wait for motion to complete
	if (blocking) {
		if (m_debugVerbose) {
			std::cout << "PIController: Waiting for motion to complete..." << std::endl;
		}

		bool waitResult = WaitForMotionCompletion(axis);

		if (m_debugVerbose) {
			std::cout << "PIController: Motion completion wait result: " << (waitResult ? "success" : "failed") << std::endl;
		}

		return waitResult;
	}

	// Get post-move position if verbose debugging is enabled
	if (m_debugVerbose) {
		double currentPos = 0.0;
		if (GetPosition(axis, currentPos)) {
			std::cout << "PIController: Post-move position of axis " << axis << " = " << currentPos << std::endl;
		}


		std::cout << "PIController: FINISHED Moving axis " + axis + " relative distance " + std::to_string(distance) << std::endl;
	}

	return true;
}


bool PIController::HomeAxis(const std::string& axis) {
	if (!m_isConnected) {
		std::cout << "PIController: Cannot home axis - not connected" << std::endl;
		return false;
	}

	std::cout << "PIController: Homing axis " << axis << std::endl;

	// Convert single-axis string to char array for PI GCS2 API
	const char* axes = axis.c_str();

	// Command the homing operation
	if (!PI_FRF(m_controllerId, axes)) {
		int error = 0;
		PI_qERR(m_controllerId, &error);

		std::cout << "PIController: Failed to home axis " << axis << ". Error code: " << error << std::endl;
		return false;
	}

	// Wait for homing to complete
	return WaitForMotionCompletion(axis);
}

bool PIController::StopAxis(const std::string& axis) {
	if (!m_isConnected) {

		std::cout << "PIController: Cannot stop axis - not connected" << std::endl;
		return false;
	}


	std::cout << "PIController: Stopping axis " << axis << std::endl;

	// Convert single-axis string to char array for PI GCS2 API
	const char* axes = axis.c_str();

	// Command the stop
	if (!PI_HLT(m_controllerId, axes)) {
		int error = 0;
		PI_qERR(m_controllerId, &error);
		std::cout << "PIController: Failed to stop axis " << axis << ". Error code: " << error << std::endl;
		return false;
	}

	return true;
}

bool PIController::StopAllAxes() {
	if (!m_isConnected) {
		std::cout << "PIController: Cannot stop all axes - not connected" << std::endl;
		return false;
	}

	std::cout << "PIController: Stopping all axes" << std::endl;

	// Command the stop for all axes
	if (!PI_STP(m_controllerId)) {
		int error = 0;
		PI_qERR(m_controllerId, &error);
		std::cout << "PIController: Failed to stop all axes. Error code: " << error << std::endl;
		return false;
	}

	return true;
}

// IsMoving optimized to use less frequent direct API calls
// Updated IsMoving method in PIController with better detection
// 3. Updated IsMoving method for PIController implementation in pi_controller.cpp
bool PIController::IsMoving(const std::string& axis) {
	if (!m_isConnected) {
		return false;
	}

	// Direct query implementation for more reliable status
	const char* axes = axis.c_str();
	BOOL isMovingArray[1] = { FALSE };

	// Call the PI_IsMoving function - this is the key function that checks motion status
	bool success = PI_IsMoving(m_controllerId, axes, isMovingArray);

	if (success) {
		// Log the actual value returned by the PI API, but only if verbose debugging is enabled
		if (m_debugVerbose) {
			std::cout << "PI_IsMoving API returned for axis " << axis << ": "
				<< (isMovingArray[0] ? "TRUE (moving)" : "FALSE (idle)") << std::endl;
		}

		// Explicitly update the cache
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_axisMoving[axis] = (isMovingArray[0] == TRUE);
		}

		return (isMovingArray[0] == TRUE);
	}
	else {
		// If query fails, report the error, but only if verbose debugging is enabled
		if (m_debugVerbose) {
			int error = PI_GetError(m_controllerId);
			std::cout << "ERROR: IsMoving query failed for axis " << axis
				<< " with error code: " << error << std::endl;
		}

		// Keep existing status if query fails
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_axisMoving.find(axis);
		return (it != m_axisMoving.end() && it->second);
	}
}



// Optimize the position queries by implementing batch query
bool PIController::GetPositions(std::map<std::string, double>& positions) {
	if (!m_isConnected || m_availableAxes.empty()) {
		return false;
	}

	// For C-887, we need space-separated axis names for batch query
	std::string allAxes = "X Y Z U V W";  // Query all six hexapod axes at once

	// Allocate array for positions - 6 axes for hexapod
	double posArray[6] = { 0.0 };

	// Query positions in a single API call
	bool success = PI_qPOS(m_controllerId, allAxes.c_str(), posArray);

	if (success) {
		// Fill the map with results
		positions["X"] = posArray[0];
		positions["Y"] = posArray[1];
		positions["Z"] = posArray[2];
		positions["U"] = posArray[3];
		positions["V"] = posArray[4];
		positions["W"] = posArray[5];

		// Log the positions (occasionally to reduce log spam)
		static int callCount = 0;
		if (++callCount % 100 == 0 && enableDebug) {


			std::cout << "PIController: Positions - X:" << posArray[0]
				<< " Y:" << posArray[1]
				<< " Z:" << posArray[2]
				<< " U:" << posArray[3]
				<< " V:" << posArray[4]
				<< " W:" << posArray[5] << std::endl;
		}
	}

	return success;
}

bool PIController::EnableServo(const std::string& axis, bool enable) {
	if (!m_isConnected) {
		std::cout << "PIController: Cannot change servo state - not connected" << std::endl;
		return false;
	}


	std::cout << "PIController: Setting servo state for axis " << axis
		<< " to " << (enable ? "enabled" : "disabled") << std::endl;

	const char* axes = axis.c_str();
	BOOL states[1] = { enable ? TRUE : FALSE };

	if (!PI_SVO(m_controllerId, axes, states)) {
		int error = 0;
		PI_qERR(m_controllerId, &error);
		std::cout << "PIController: Failed to set servo state for axis " << axis
			<< ". Error code: " << error << std::endl;
		return false;
	}

	return true;
}

// IsServoEnabled optimized to use cached values
bool PIController::IsServoEnabled(const std::string& axis, bool& enabled) {
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
		auto it = m_axisServoEnabled.find(axis);
		if (it != m_axisServoEnabled.end()) {
			enabled = it->second;
			return true;
		}
	}

	// If no recent cached value, do direct query
	const char* axes = axis.c_str();
	BOOL states[1] = { FALSE };

	bool success = PI_qSVO(m_controllerId, axes, states);

	if (success) {
		enabled = (states[0] == TRUE);

		// Update the cache
		std::lock_guard<std::mutex> lock(m_mutex);
		m_axisServoEnabled[axis] = enabled;
		m_lastStatusUpdate = now;
		return true;
	}

	// In case of query error, return false
	return false;
}
bool PIController::SetVelocity(const std::string& axis, double velocity) {
	if (!m_isConnected) {
		std::cout << "PIController: Cannot set velocity - not connected" << std::endl;
		return false;
	}

	std::cout << "PIController: Setting velocity for axis " << axis << " to " << velocity << std::endl;
	const char* axes = axis.c_str();
	double velocities[1] = { velocity };

	if (!PI_VEL(m_controllerId, axes, velocities)) {
		int error = 0;
		PI_qERR(m_controllerId, &error);
		std::cout << "PIController: Failed to set velocity for axis " << axis
			<< ". Error code: " << error << std::endl;
		return false;
	}

	return true;
}

bool PIController::GetVelocity(const std::string& axis, double& velocity) {
	if (!m_isConnected) {
		return false;
	}

	const char* axes = axis.c_str();
	double velocities[1] = { 0.0 };

	if (!PI_qVEL(m_controllerId, axes, velocities)) {
		// Error checking omitted for brevity in status check
		return false;
	}

	velocity = velocities[0];
	return true;
}

// Add logging to WaitForMotionCompletion to track any issues there
// Modified WaitForMotionCompletion to use atomic variables correctly
bool PIController::WaitForMotionCompletion(const std::string& axis, double timeoutSeconds) {
	if (!m_isConnected) {
		std::cout << "PIController: Cannot wait for motion completion - not connected" << std::endl;
		return false;
	}

	// Use system clock for timeout
	auto startTime = std::chrono::steady_clock::now();
	int checkCount = 0;

	while (true) {
		checkCount++;

		// First check if we have recent cached motion status
		bool stillMoving = false;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_axisMoving.find(axis);
			if (it != m_axisMoving.end()) {
				stillMoving = it->second;
			}
		}

		// If cached value says we're not moving OR if we're not sure, double-check directly
		if (!stillMoving) {
			// Double-check with a direct query to be sure
			stillMoving = IsMoving(axis);

			// Update the cache
			std::lock_guard<std::mutex> lock(m_mutex);
			m_axisMoving[axis] = stillMoving;
		}

		if (!stillMoving) {
			if (m_enableDebug) {

				std::cout << "PIController: Motion completed on axis " << axis
					<< " after " << checkCount << " checks" << std::endl;
			}
			return true;
		}

		// Check for timeout
		auto currentTime = std::chrono::steady_clock::now();
		auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

		if (elapsedSeconds > timeoutSeconds) {
			std::string timeoutMsg = "PIController: Timeout waiting for motion completion on axis " + axis;
			std::cout << timeoutMsg << std::endl;
			return false;
		}

		// Log less frequently to reduce overhead
		if (m_enableDebug && checkCount % 20 == 0) {

			std::cout << "PIController: Still waiting for axis " << axis
				<< " to complete motion, elapsed time: " << elapsedSeconds << "s" << std::endl;
		}

		// Sleep to avoid CPU spikes but be responsive
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

//
// Updated ConfigureFromDevice method for PIController to handle space-separated InstalledAxes
//
// Updated ConfigureFromDevice - set device name for data store
bool PIController::ConfigureFromDevice(const MotionDevice& device) {
	if (m_isConnected) {
		std::cout << "PIController: Cannot configure from device while connected" << std::endl;
		return false;
	}

	std::cout << "PIController: Configuring from device: " << device.Name << std::endl;
	// Store device name for data store keys
	m_deviceName = device.Name;

	// Store connection info
	m_ipAddress = device.IpAddress;
	m_port = device.Port;

	// Configure axes
	m_availableAxes.clear();
	if (!device.InstalledAxes.empty()) {
		std::string axisStr = device.InstalledAxes;
		std::string delimiter = " ";
		size_t pos = 0;
		std::string token;

		while ((pos = axisStr.find(delimiter)) != std::string::npos) {
			token = axisStr.substr(0, pos);
			if (!token.empty()) {
				m_availableAxes.push_back(token);
			}
			axisStr.erase(0, pos + delimiter.length());
		}

		if (!axisStr.empty()) {
			m_availableAxes.push_back(axisStr);
		}

		std::string axesList;
		for (const auto& axis : m_availableAxes) {
			if (!axesList.empty()) axesList += " ";
			axesList += axis;
		}
		std::cout << "PIController: Configured with axes: " << axesList << std::endl;
	}
	else {
		m_availableAxes = { "X", "Y", "Z", "U", "V", "W" };
		
		std::cout << "PIController: Using default hexapod axes: X Y Z U V W" << std::endl;
	}

	return true;
}



bool PIController::MoveToNamedPosition(const std::string& deviceName, const std::string& positionName) {

	std::cout << "PIController: Moving to named position " << positionName << " for device " << deviceName << std::endl;
	//TODO
	std::cout << "PIController: MoveToNamedPosition is not implemented yet." << std::endl;
	return true;
}

// Update UI rendering to match the new axis identifiers
// Optimize the individual GetPosition by using cached positions when possible
bool PIController::GetPosition(const std::string& axis, double& position) {
	if (!m_isConnected) {
		return false;
	}

	// First check if we have a recent cached value
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_axisPositions.find(axis);
		if (it != m_axisPositions.end()) {
			position = it->second;
			return true;
		}
	}

	// Fall back to individual query if no cached value exists
	const char* axes = axis.c_str();
	double positions[1] = { 0.0 };

	bool result = PI_qPOS(m_controllerId, axes, positions);

	if (result) {
		position = positions[0];

		// Update the cache with this new value
		std::lock_guard<std::mutex> lock(m_mutex);
		m_axisPositions[axis] = position;
	}

	return result;
}

bool PIController::MoveToPositionAll(double x, double y, double z, double u, double v, double w, bool blocking) {
	if (!m_isConnected) {

		std::cout << "PIController: Cannot move axes - not connected" << std::endl;
		return false;
	}



	std::cout << "PIController: Moving all axes to position X=" << x
		<< ", Y=" << y
		<< ", Z=" << z
		<< ", U=" << u
		<< ", V=" << v
		<< ", W=" << w << std::endl;

	// Define the axes to move
	//require space between axes
	const char* szAxes = "X Y Z U V W";

	// Define the target positions in the same order as the axes
	double pdValueArray[6] = { x, y, z, u, v, w };

	// Call the PI_MOV function directly
	if (!PI_MOV(m_controllerId, szAxes, pdValueArray)) {
		int error = PI_GetError(m_controllerId);
		std::cout << "PIController: Failed to move all axes. Error code: " << error << std::endl;
		return false;
	}

	// If blocking mode, wait for motion to complete on all axes
	if (blocking) {
		bool success = true;
		for (const auto& axis : { "X", "Y", "Z", "U", "V", "W" }) {
			if (!WaitForMotionCompletion(axis)) {
				std::cout << "PIController: Timeout waiting for motion completion on axis " << axis << std::endl;
				success = false;
			}
		}
		return success;
	}

	return true;
}





bool PIController::MoveToPositionMultiAxis(const std::vector<std::string>& axes,
	const std::vector<double>& positions,
	bool blocking) {
	if (!m_isConnected) {

		std::cout << "PIController: Cannot move axes - not connected" << std::endl;
		return false;
	}

	// Validate input arrays
	if (axes.size() != positions.size() || axes.empty()) {

		std::cout << "PIController: Invalid axes/positions arrays for multi-axis move" << std::endl;
		return false;
	}

	// Log the motion command
	std::stringstream ss;
	ss << "PIController: Moving multiple axes to positions: ";
	for (size_t i = 0; i < axes.size(); i++) {
		ss << axes[i] << "=" << positions[i] << " ";
	}
	
	std::cout << ss.str() << std::endl;

	// Create space-separated string of axes (e.g., "X Y Z")
	std::string axesStr;
	for (size_t i = 0; i < axes.size(); i++) {
		axesStr += axes[i];
		if (i < axes.size() - 1) {
			axesStr += " "; // Add space between axes
		}
	}

	// Convert to C-style arrays for PI API
	const char* szAxes = axesStr.c_str();

	// Create a copy of the positions array that can be modified by the PI API
	std::vector<double> posArray = positions;

	// Call the PI_MOV function to move to the specified positions
	if (!PI_MOV(m_controllerId, szAxes, posArray.data())) {
		int error = PI_GetError(m_controllerId);
		
		std::cout << "PIController: Failed to move axes. Error code: " << error << std::endl;
		return false;
	}

	// If blocking, wait for motion to complete on all axes
	if (blocking) {
		bool success = true;
		for (const auto& axis : axes) {
			if (!WaitForMotionCompletion(axis)) {
				std::cout << "PIController: Timeout waiting for motion completion on axis " << axis << std::endl;
				success = false;
			}
		}
		return success;
	}

	return true;
}


// Add to pi_controller.cpp:

/**
 * Starts a scanning procedure to determine the maximum intensity of an analog input signal in a plane.
 * The search consists of two subprocedures:
 * - "Coarse portion" (similar to FSC function)
 * - "Fine portion" (similar to AAP function)
 * The fine portion is only executed when the coarse portion has previously been successfully completed.
 *
 * @param axis1 First axis that defines scanning area (X, Y, or Z). During the coarse portion,
 *              the platform moves in this axis from scanning line to scanning line by the distance given by distance.
 * @param length1 Length of scanning area along axis1 in mm
 * @param axis2 Second axis that defines scanning area (X, Y, or Z). During the coarse portion,
 *              the scanning lines are in this axis.
 * @param length2 Length of scanning area along axis2 in mm
 * @param threshold Intensity threshold of the analog input signal, in V
 * @param distance Distance between the scanning lines in mm, used only during the coarse portion
 * @param alignStep Starting value for the step size in mm, used only during the fine portion
 * @param analogInput Identifier of the analog input signal whose maximum intensity is sought
 * @return TRUE if scan started successfully, FALSE otherwise
 */
bool PIController::FSA(const std::string& axis1, double length1,
	const std::string& axis2, double length2,
	double threshold, double distance,
	double alignStep, int analogInput) {
	// Check if controller is connected
	if (!m_isConnected) {

		std::cout << "PIController: Cannot perform FSA scan - not connected" << std::endl;
		return false;
	}


	std::cout << "PIController: Starting FSA scan" << std::endl;

	// Call the PI GCS2 function
	bool result = PI_FSA(m_controllerId,
		axis1.c_str(), length1,
		axis2.c_str(), length2,
		threshold, distance,
		alignStep, analogInput);

	if (!result) {
		int error = PI_GetError(m_controllerId);

		std::cout << "PIController: FSA scan failed. Error code: " << error << std::endl;
		return false;
	}


	std::cout << "PIController: FSA scan started successfully" << std::endl;
	return true;
}

/**
 * Starts a scanning procedure which scans a specified area ("scanning area") until the analog
 * input signal reaches a specified intensity threshold.
 * The scanning procedure corresponds to the "coarse portion" of the scanning procedure
 * that is started with the FSA function.
 *
 * @param axis1 The axis in which the platform moves from scanning line to scanning line
 *              by the distance given by distance (X, Y, or Z).
 * @param length1 Length of scanning area along axis1 in mm
 * @param axis2 The axis in which the scanning lines are located (X, Y, or Z)
 * @param length2 Length of scanning area along axis2 in mm
 * @param threshold Intensity threshold of the analog input signal, in V
 * @param distance Distance between the scanning lines in mm
 * @param analogInput Identifier of the analog input signal whose maximum intensity is sought
 * @return TRUE if scan started successfully, FALSE otherwise
 */
bool PIController::FSC(const std::string& axis1, double length1,
	const std::string& axis2, double length2,
	double threshold, double distance,
	int analogInput) {
	// Check if controller is connected
	if (!m_isConnected) {

		std::cout << "PIController: Cannot perform FSC scan - not connected" << std::endl;
		return false;
	}


	std::cout << "PIController: Starting FSC scan" << std::endl;

	// Call the PI GCS2 function
	bool result = PI_FSC(m_controllerId,
		axis1.c_str(), length1,
		axis2.c_str(), length2,
		threshold, distance,
		analogInput);

	if (!result) {
		int error = PI_GetError(m_controllerId);

		std::cout << "PIController: FSC scan failed. Error code: " << error << std::endl;
		return false;
	}


	std::cout << "PIController: FSC scan started successfully" << std::endl;
	return true;
}

/**
 * Starts a scanning procedure to determine the global maximum intensity of an analog
 * input signal in a plane. Unlike FSC, this method scans the entire area to find the
 * global maximum rather than stopping when a threshold is reached.
 *
 * @param axis1 The axis in which the platform moves from scanning line to scanning line
 *              by the distance given by distance (X, Y, or Z).
 * @param length1 Length of scanning area along axis1 in mm
 * @param axis2 The axis in which the scanning lines are located (X, Y, or Z)
 * @param length2 Length of scanning area along axis2 in mm
 * @param threshold Intensity threshold of the analog input signal, in V
 * @param distance Distance between the scanning lines in mm
 * @param analogInput Identifier of the analog input signal whose maximum intensity is sought
 * @return TRUE if scan started successfully, FALSE otherwise
 */
bool PIController::FSM(const std::string& axis1, double length1,
	const std::string& axis2, double length2,
	double threshold, double distance,
	int analogInput) {
	// Check if controller is connected
	if (!m_isConnected) {

		std::cout << "PIController: Cannot perform FSM scan - not connected" << std::endl;
		return false;
	}


	std::cout << "PIController: Starting FSM scan" << std::endl;

	// Call the PI GCS2 function
	bool result = PI_FSM(m_controllerId,
		axis1.c_str(), length1,
		axis2.c_str(), length2,
		threshold, distance,
		analogInput);

	if (!result) {
		int error = PI_GetError(m_controllerId);

		std::cout << "PIController: FSM scan failed. Error code: " << error << std::endl;
		return false;
	}


	std::cout << "PIController: FSM scan started successfully" << std::endl;
	return true;
}

// Then implement the method in pi_controller.cpp:
bool PIController::CopyPositionToClipboard() {
	// Create a copy of current positions to avoid locking the mutex for too long
	std::map<std::string, double> positions;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		positions = m_axisPositions;
	}

	// Check if we have any positions to copy
	if (positions.empty()) {
		return false;
	}

	// Extract just the device name without the "Controller:" prefix
	std::string deviceName = m_windowTitle;
	// Remove "Controller: " if present
	size_t prefixPos = deviceName.find("Controller: ");
	if (prefixPos != std::string::npos) {
		deviceName = deviceName.substr(prefixPos + 12); // Length of "Controller: " is 12
	}

	// Format as JSON with proper indentation
	std::stringstream jsonStr;

	// Start JSON object with device name
	jsonStr << "{" << std::endl;
	jsonStr << "  \"device\": \"" << deviceName << "\"," << std::endl;
	jsonStr << "  \"positions\": {" << std::endl;

	// Use iterator to handle the comma placement
	auto it = positions.begin();
	auto end = positions.end();

	while (it != end) {
		// Format with 6 decimal places precision
		jsonStr << "    \"" << it->first << "\": " << std::fixed << std::setprecision(6) << it->second;

		// Add comma if not the last element
		++it;
		if (it != end) {
			jsonStr << ",";
		}
		jsonStr << std::endl;
	}

	jsonStr << "  }" << std::endl;
	jsonStr << "}";


	return true;
}

bool PIController::SetSystemVelocity(double velocity) {
	if (!m_isConnected) {

		std::cout << "PIController: Cannot set system velocity - not connected" << std::endl;
		return false;
	}


	std::cout << "PIController: Setting system velocity to " << velocity << std::endl;

	if (!PI_VLS(m_controllerId, velocity)) {
		int error = 0;
		PI_qERR(m_controllerId, &error);

		std::cout << "PIController: Failed to set system velocity. Error code: " << error << std::endl;
		return false;
	}

	return true;
}

bool PIController::GetSystemVelocity(double& velocity) {
	if (!m_isConnected) {
		return false;
	}

	if (!PI_qVLS(m_controllerId, &velocity)) {
		int error = 0;
		PI_qERR(m_controllerId, &error);

		std::cout << "PIController: Failed to get system velocity. Error code: " << error << std::endl;
		return false;
	}

	return true;
}
// Add these to your pi_controller.cpp file:

bool PIController::Home(const std::string& axis) {
	if (!IsConnected()) {
		std::cerr << "PIController::Home failed: Controller not connected" << std::endl;
		return false;
	}

	if (axis.empty()) {
		std::cerr << "PIController::Home failed: Empty axis name" << std::endl;
		return false;
	}

	// Validate axis exists (if you have this method, otherwise remove this check)
	// if (!IsAxisValid(axis)) {
	//     std::cerr << "PIController::Home failed: Invalid axis " << axis << std::endl;
	//     return false;
	// }

	std::cout << "PIController: Homing axis: " << axis << std::endl;

	// Call PI_GOH with single axis
	BOOL result = PI_GOH(m_controllerId, axis.c_str());

	if (!result) {
		int errorCode = PI_GetError(m_controllerId);
		std::cerr << "PIController::Home failed for axis " << axis
			<< " with PI error: " << errorCode << std::endl;
		return false;
	}

	std::cout << "PIController: Successfully started homing for axis: " << axis << std::endl;
	return true;
}

bool PIController::HomeAll() {
	if (!IsConnected()) {
		std::cerr << "PIController::HomeAll failed: Controller not connected" << std::endl;
		return false;
	}

	std::cout << "PIController: Homing all axes" << std::endl;

	// Call PI_GOH with empty string to home all axes
	BOOL result = PI_GOH(m_controllerId, "");

	if (!result) {
		int errorCode = PI_GetError(m_controllerId);
		std::cerr << "PIController::HomeAll failed with PI error: " << errorCode << std::endl;
		return false;
	}

	std::cout << "PIController: Successfully started homing for all axes" << std::endl;
	return true;
}

bool PIController::HomeAxes(const std::vector<std::string>& axes) {
	if (!IsConnected()) {
		std::cerr << "PIController::HomeAxes failed: Controller not connected" << std::endl;
		return false;
	}

	if (axes.empty()) {
		std::cerr << "PIController::HomeAxes failed: Empty axes list" << std::endl;
		return false;
	}

	// Validate all axes first (if you have IsAxisValid method)
	// for (const std::string& axis : axes) {
	//     if (!IsAxisValid(axis)) {
	//         std::cerr << "PIController::HomeAxes failed: Invalid axis " << axis << std::endl;
	//         return false;
	//     }
	// }

	// Convert axes vector to space-separated string
	std::string axesString = AxesToString(axes);

	std::cout << "PIController: Homing axes: " << axesString << std::endl;

	// Call PI_GOH with axes string
	BOOL result = PI_GOH(m_controllerId, axesString.c_str());

	if (!result) {
		int errorCode = PI_GetError(m_controllerId);
		std::cerr << "PIController::HomeAxes failed for [" << axesString
			<< "] with PI error: " << errorCode << std::endl;
		return false;
	}

	std::cout << "PIController: Successfully started homing for axes: " << axesString << std::endl;
	return true;
}

bool PIController::HomeAxes(const std::string& axesString) {
	if (!IsConnected()) {
		std::cerr << "PIController::HomeAxes failed: Controller not connected" << std::endl;
		return false;
	}

	if (axesString.empty()) {
		std::cerr << "PIController::HomeAxes failed: Empty axes string" << std::endl;
		return false;
	}

	std::cout << "PIController: Homing axes: " << axesString << std::endl;

	// Call PI_GOH with provided axes string
	BOOL result = PI_GOH(m_controllerId, axesString.c_str());

	if (!result) {
		int errorCode = PI_GetError(m_controllerId);
		std::cerr << "PIController::HomeAxes failed for [" << axesString
			<< "] with PI error: " << errorCode << std::endl;
		return false;
	}

	std::cout << "PIController: Successfully started homing for axes: " << axesString << std::endl;
	return true;
}

bool PIController::DefineHome(const std::string& axis) {
	if (!IsConnected()) {
		std::cerr << "PIController::DefineHome failed: Controller not connected" << std::endl;
		return false;
	}

	if (axis.empty()) {
		std::cerr << "PIController::DefineHome failed: Empty axis name" << std::endl;
		return false;
	}

	// Validate axis exists (if you have this method)
	// if (!IsAxisValid(axis)) {
	//     std::cerr << "PIController::DefineHome failed: Invalid axis " << axis << std::endl;
	//     return false;
	// }

	std::cout << "PIController: Defining home position for axis " << axis 		<<  std::endl;

	// Call PI_DFH to define home position
	BOOL result = PI_DFH(m_controllerId, axis.c_str());

	if (!result) {
		int errorCode = PI_GetError(m_controllerId);
		std::cerr << "PIController::DefineHome failed for axis " << axis
			<< " with PI error: " << errorCode << std::endl;
		return false;
	}

	std::cout << "PIController: Successfully defined home position for axis: " << axis << std::endl;
	return true;
}

bool PIController::DefineHomeAll() {
	if (!IsConnected()) {
		std::cerr << "PIController::DefineHomeAll failed: Controller not connected" << std::endl;
		return false;
	}

	std::cout << "PIController: Defining home position for all axes at position: " << std::endl;

	// Get all available axes and define home for each
	auto axes = GetAvailableAxes();
	bool success = true;

	for (const std::string& axis : axes) {
		BOOL result = PI_DFH(m_controllerId, axis.c_str());
		if (!result) {
			int errorCode = PI_GetError(m_controllerId);
			std::cerr << "PIController::DefineHomeAll failed for axis " << axis
				<< " with PI error: " << errorCode << std::endl;
			success = false;
			// Continue with other axes even if one fails
		}
	}

	if (success) {
		std::cout << "PIController: Successfully defined home position for all axes" << std::endl;
	}
	else {
		std::cerr << "PIController::DefineHomeAll partially failed - check output for details" << std::endl;
	}

	return success;
}

std::string PIController::AxesToString(const std::vector<std::string>& axes) const {
	if (axes.empty()) {
		return "";
	}

	std::string result = axes[0];
	for (size_t i = 1; i < axes.size(); ++i) {
		result += " " + axes[i];
	}

	return result;
}