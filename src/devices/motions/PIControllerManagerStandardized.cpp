#include "PIControllerManagerStandardized.h"
#include "PIController.h"
#include "MotionTypes.h"
#include "core/ConfigRegistry.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

// Constructor
PIControllerManagerStandardized::PIControllerManagerStandardized(ConfigManager& configManager, bool hardwareMode)
  : DeviceManagerBase("PI_Controller_Manager"),
  m_configManager(configManager),
  m_hardwareMode(hardwareMode) {

  std::cout << "PIControllerManagerStandardized: Created "
    << (m_hardwareMode ? "[HARDWARE MODE]" : "[MOCK MODE]") << std::endl;

  // Load device configurations
  LoadDevicesFromConfig();
}

// Destructor - explicit definition needed for unique_ptr with forward declaration
PIControllerManagerStandardized::~PIControllerManagerStandardized() {
  std::cout << "PIControllerManagerStandardized: Shutting down..." << std::endl;

  // Disconnect all devices before destruction
  if (m_isInitialized) {
    DisconnectAll();
  }

  std::cout << "PIControllerManagerStandardized: Shutdown complete" << std::endl;
}

// === CORE LIFECYCLE METHODS ===

bool PIControllerManagerStandardized::Initialize() {
  if (m_isInitialized) {
    return true;
  }

  std::cout << "PIControllerManagerStandardized: Initializing..." << std::endl;

  // Reload configurations
  LoadDevicesFromConfig();

  m_isInitialized = true;
  std::cout << "PIControllerManagerStandardized: Initialization complete" << std::endl;
  return true;
}

bool PIControllerManagerStandardized::ConnectAll() {
  if (!m_isInitialized) {
    std::cout << "PIControllerManagerStandardized: Cannot connect - not initialized" << std::endl;
    return false;
  }

  std::cout << "PIControllerManagerStandardized: ConnectAll() - "
    << (m_hardwareMode ? "HARDWARE MODE" : "MOCK MODE") << std::endl;

  bool allSuccess = true;

  if (m_hardwareMode) {
    // Connect to real hardware
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    for (const auto& [deviceName, config] : m_deviceConfigs) {
      if (config.isEnabled) {
        std::cout << "  Connecting to: " << deviceName << std::endl;
        if (!ConnectDevice(deviceName)) {
          std::cout << "  Failed to connect: " << deviceName << std::endl;
          allSuccess = false;
        }
        else {
          std::cout << "  Successfully connected: " << deviceName << std::endl;
        }
      }
      else {
        std::cout << "  Skipping disabled device: " << deviceName << std::endl;
      }
    }
  }
  else {
    // Mock mode connections
    for (size_t i = 0; i < m_mockDeviceNames.size(); ++i) {
      if (i < m_mockConnectionStates.size()) {
        m_mockConnectionStates[i] = (i % 2 == 0); // Connect every other device
        std::cout << "  Mock device '" << m_mockDeviceNames[i]
          << "': " << (m_mockConnectionStates[i] ? "CONNECTED" : "FAILED") << std::endl;
      }
    }
  }

  std::cout << "PIControllerManagerStandardized: ConnectAll() complete - "
    << (allSuccess ? "SUCCESS" : "PARTIAL FAILURE") << std::endl;
  return allSuccess;
}

bool PIControllerManagerStandardized::DisconnectAll() {
  std::cout << "PIControllerManagerStandardized: DisconnectAll()" << std::endl;

  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);

    // Create a copy of device names to avoid iterator invalidation
    std::vector<std::string> deviceNames;
    for (const auto& [name, device] : m_realDevices) {
      deviceNames.push_back(name);
    }

    // Disconnect each device
    for (const std::string& deviceName : deviceNames) {
      DisconnectDevice(deviceName);
    }
  }
  else {
    // Mock mode disconnection
    std::fill(m_mockConnectionStates.begin(), m_mockConnectionStates.end(), false);
  }

  std::cout << "PIControllerManagerStandardized: DisconnectAll() complete" << std::endl;
  return true;
}

// === DEVICE ACCESS METHODS ===

PIController* PIControllerManagerStandardized::GetDevice(const std::string& deviceName) {
  return GetRealDevice(deviceName);
}

const PIController* PIControllerManagerStandardized::GetDevice(const std::string& deviceName) const {
  return GetRealDevice(deviceName);
}

// === DEVICE ENUMERATION METHODS ===

int PIControllerManagerStandardized::GetDeviceCount() const {
  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return static_cast<int>(m_deviceConfigs.size());
  }
  else {
    return static_cast<int>(m_mockDeviceNames.size());
  }
}

std::vector<std::string> PIControllerManagerStandardized::GetDeviceNames() const {
  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    std::vector<std::string> names;
    names.reserve(m_deviceConfigs.size());
    for (const auto& [name, config] : m_deviceConfigs) {
      names.push_back(name);
    }
    return names;
  }
  else {
    return m_mockDeviceNames;
  }
}

// === INDIVIDUAL DEVICE CONTROL ===

bool PIControllerManagerStandardized::ConnectDevice(const std::string& deviceName) {
  if (m_hardwareMode) {
    // Check if already connected
    if (IsRealDeviceConnected(deviceName)) {
      std::cout << "  Device already connected: " << deviceName << std::endl;
      return true;
    }

    // Try to create and connect real device
    return CreateRealDevice(deviceName);
  }
  else {
    // Mock mode
    size_t index = FindMockDeviceIndex(deviceName);
    if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
      m_mockConnectionStates[index] = true;
      std::cout << "  Mock connected: " << deviceName << std::endl;
      return true;
    }
    return false;
  }
}

bool PIControllerManagerStandardized::DisconnectDevice(const std::string& deviceName) {
  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_realDevices.find(deviceName);
    if (it != m_realDevices.end()) {
      DestroyRealDevice(deviceName);
      return true;
    }
    return false;
  }
  else {
    // Mock mode
    size_t index = FindMockDeviceIndex(deviceName);
    if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
      m_mockConnectionStates[index] = false;
      std::cout << "  Mock disconnected: " << deviceName << std::endl;
      return true;
    }
    return false;
  }
}

bool PIControllerManagerStandardized::IsDeviceConnected(const std::string& deviceName) const {
  return IsRealDeviceConnected(deviceName);
}

// === CONFIGURATION METHODS ===

bool PIControllerManagerStandardized::AddDeviceConfig(const std::string& deviceName, const std::string& ipAddress, int port) {
  if (!IsValidDeviceName(deviceName)) {
    std::cout << "PIControllerManagerStandardized: Invalid device name: " << deviceName << std::endl;
    return false;
  }

  std::lock_guard<std::mutex> lock(m_devicesMutex);

  PIDeviceConfig config(deviceName, ipAddress, port);
  if (!ValidateDeviceConfig(config)) {
    return false;
  }

  m_deviceConfigs[deviceName] = config;
  std::cout << "PIControllerManagerStandardized: Added device config: " << deviceName
    << " @ " << ipAddress << ":" << port << std::endl;
  return true;
}

bool PIControllerManagerStandardized::RemoveDeviceConfig(const std::string& deviceName) {
  std::lock_guard<std::mutex> lock(m_devicesMutex);

  // Disconnect if connected
  auto deviceIt = m_realDevices.find(deviceName);
  if (deviceIt != m_realDevices.end()) {
    DestroyRealDevice(deviceName);
  }

  // Remove config
  auto configIt = m_deviceConfigs.find(deviceName);
  if (configIt != m_deviceConfigs.end()) {
    m_deviceConfigs.erase(configIt);
    std::cout << "PIControllerManagerStandardized: Removed device config: " << deviceName << std::endl;
    return true;
  }
  return false;
}

PIControllerManagerStandardized::PIDeviceConfig PIControllerManagerStandardized::GetDeviceConfig(const std::string& deviceName) const {
  const PIDeviceConfig* config = GetConstDeviceConfig(deviceName);
  return config ? *config : PIDeviceConfig{};
}

std::vector<PIControllerManagerStandardized::PIDeviceConfig> PIControllerManagerStandardized::GetAllDeviceConfigs() const {
  std::lock_guard<std::mutex> lock(m_devicesMutex);
  std::vector<PIDeviceConfig> configs;
  configs.reserve(m_deviceConfigs.size());
  for (const auto& [name, config] : m_deviceConfigs) {
    configs.push_back(config);
  }
  return configs;
}

// === MODE CONTROL ===

void PIControllerManagerStandardized::SetHardwareMode(bool enabled) {
  if (m_hardwareMode != enabled) {
    std::cout << "PIControllerManagerStandardized: Switching to "
      << (enabled ? "HARDWARE" : "MOCK") << " mode" << std::endl;

    // Disconnect all devices before switching modes
    DisconnectAll();
    m_hardwareMode = enabled;
  }
}

// === TESTING/MOCK UTILITIES ===

void PIControllerManagerStandardized::SetMockDeviceConnected(const std::string& deviceName, bool connected) {
  if (m_hardwareMode) {
    std::cout << "PIControllerManagerStandardized: SetMockDeviceConnected ignored in hardware mode" << std::endl;
    return;
  }

  size_t index = FindMockDeviceIndex(deviceName);
  if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
    m_mockConnectionStates[index] = connected;
    std::cout << "Mock: Set " << deviceName << " to "
      << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
  }
}

void PIControllerManagerStandardized::AddMockDevice(const std::string& deviceName) {
  if (std::find(m_mockDeviceNames.begin(), m_mockDeviceNames.end(), deviceName) == m_mockDeviceNames.end()) {
    m_mockDeviceNames.push_back(deviceName);
    m_mockConnectionStates.push_back(false);
    std::cout << "PIControllerManagerStandardized: Added mock device: " << deviceName << std::endl;
  }
}

// === STATUS AND DIAGNOSTICS ===

int PIControllerManagerStandardized::GetConnectedDeviceCount() const {
  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    int count = 0;
    for (const auto& [name, device] : m_realDevices) {
      if (device && device->IsConnected()) {
        count++;
      }
    }
    return count;
  }
  else {
    return static_cast<int>(std::count(m_mockConnectionStates.begin(), m_mockConnectionStates.end(), true));
  }
}

bool PIControllerManagerStandardized::IsDeviceResponding(const std::string& deviceName) const {
  if (m_hardwareMode) {
    const PIController* device = GetRealDevice(deviceName);
    return device && device->IsConnected();
  }
  return IsDeviceConnected(deviceName);
}

std::string PIControllerManagerStandardized::GetDeviceInfo(const std::string& deviceName) const {
  const PIDeviceConfig* config = GetConstDeviceConfig(deviceName);
  if (config) {
    return "PI Device: " + config->name + " @ " + config->ipAddress + ":" +
      std::to_string(config->port) + " [Axes: " + config->installAxes + "]";
  }
  return "Device not found: " + deviceName;
}

void PIControllerManagerStandardized::PrintDeviceStatus() const {
  std::cout << "=== PI Controller Device Status ===" << std::endl;

  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    for (const auto& [deviceName, config] : m_deviceConfigs) {
      bool connected = IsRealDeviceConnected(deviceName);
      std::cout << "  " << deviceName << ": "
        << (config.isEnabled ? "ENABLED" : "DISABLED") << " | "
        << (connected ? "CONNECTED" : "DISCONNECTED");

      if (connected) {
        const PIController* device = GetRealDevice(deviceName);
        if (device) {
          std::cout << " | Controller ID: " << device->GetControllerId();
        }
      }
      std::cout << " | " << config.ipAddress << ":" << config.port << std::endl;
    }
  }
  else {
    for (size_t i = 0; i < m_mockDeviceNames.size(); i++) {
      bool connected = (i < m_mockConnectionStates.size()) ? m_mockConnectionStates[i] : false;
      std::cout << "  " << m_mockDeviceNames[i] << ": MOCK | "
        << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
    }
  }

  std::cout << "Total connected: " << GetConnectedDeviceCount() << std::endl;
  std::cout << "===============================" << std::endl;
}

// === BATCH OPERATIONS ===

bool PIControllerManagerStandardized::HomeAllDevices() {
  std::cout << "PIControllerManagerStandardized: Homing all connected devices..." << std::endl;

  bool allSuccess = true;
  auto connectedDevices = GetConnectedDeviceNames();

  for (const std::string& deviceName : connectedDevices) {
    PIController* device = GetDevice(deviceName);
    if (device && device->IsConnected()) {
      std::cout << "  Homing device: " << deviceName << std::endl;
      if (!device->HomeAll()) {
        std::cout << "  Failed to home device: " << deviceName << std::endl;
        allSuccess = false;
      }
    }
  }

  return allSuccess;
}

bool PIControllerManagerStandardized::StopAllDevices() {
  std::cout << "PIControllerManagerStandardized: Stopping all connected devices..." << std::endl;

  bool allSuccess = true;
  auto connectedDevices = GetConnectedDeviceNames();

  for (const std::string& deviceName : connectedDevices) {
    PIController* device = GetDevice(deviceName);
    if (device && device->IsConnected()) {
      std::cout << "  Stopping device: " << deviceName << std::endl;
      if (!device->StopAllAxes()) {
        std::cout << "  Failed to stop device: " << deviceName << std::endl;
        allSuccess = false;
      }
    }
  }

  return allSuccess;
}

std::vector<std::string> PIControllerManagerStandardized::GetConnectedDeviceNames() const {
  std::vector<std::string> connected;

  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    for (const auto& [name, device] : m_realDevices) {
      if (device && device->IsConnected()) {
        connected.push_back(name);
      }
    }
  }
  else {
    for (size_t i = 0; i < m_mockDeviceNames.size(); i++) {
      if (i < m_mockConnectionStates.size() && m_mockConnectionStates[i]) {
        connected.push_back(m_mockDeviceNames[i]);
      }
    }
  }

  return connected;
}

// === PRIVATE HELPER METHODS ===

void PIControllerManagerStandardized::LoadDevicesFromConfig() {
  std::cout << "PIControllerManagerStandardized: Loading devices from configuration..." << std::endl;

  // Clear existing data
  {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    m_deviceConfigs.clear();
  }
  m_mockDeviceNames.clear();
  m_mockConnectionStates.clear();

  try {
    // Get all motion devices from config
    auto devices = Config::Motion::GetAllDevices();

    int piDeviceCount = 0;
    for (const auto& device : devices) {
      if (device.typeController == "PI") {
        PIDeviceConfig config;
        config.name = device.name;
        config.ipAddress = device.ipAddress;
        config.port = device.port;
        config.id = device.id;
        config.isEnabled = device.isEnabled;
        config.installAxes = device.installAxes;

        {
          std::lock_guard<std::mutex> lock(m_devicesMutex);
          m_deviceConfigs[device.name] = config;
        }

        // Also add to mock data for fallback
        m_mockDeviceNames.push_back(device.name);
        m_mockConnectionStates.push_back(false);

        std::cout << "  Found PI device: " << device.name
          << " @ " << device.ipAddress << ":" << device.port
          << " [Enabled: " << (device.isEnabled ? "Yes" : "No") << "]" << std::endl;
        piDeviceCount++;
      }
    }

    std::cout << "PIControllerManagerStandardized: Loaded " << piDeviceCount
      << " PI devices from configuration" << std::endl;

  }
  catch (const std::exception& e) {
    std::cout << "PIControllerManagerStandardized: Error loading from config: " << e.what() << std::endl;
    CreateDefaultConfigs();
  }
}

void PIControllerManagerStandardized::CreateDefaultConfigs() {
  std::cout << "PIControllerManagerStandardized: Creating default device configurations..." << std::endl;

  // Default PI controller configurations
  std::vector<std::tuple<std::string, std::string, int>> defaultDevices = {
      {"hex-left", "192.168.1.100", 50000},
      {"hex-right", "192.168.1.101", 50000},
      {"hex-bottom", "192.168.1.102", 50000}
  };

  {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    for (const auto& [name, ip, port] : defaultDevices) {
      PIDeviceConfig config(name, ip, port);
      m_deviceConfigs[name] = config;

      m_mockDeviceNames.push_back(name);
      m_mockConnectionStates.push_back(false);

      std::cout << "  Created default config: " << name << " @ " << ip << ":" << port << std::endl;
    }
  }
}

bool PIControllerManagerStandardized::CreateRealDevice(const std::string& deviceName) {
  PIDeviceConfig* config = GetMutableDeviceConfig(deviceName);
  if (!config) {
    std::cout << "  Device config not found: " << deviceName << std::endl;
    return false;
  }

  try {
    std::cout << "  Creating PI device: " << deviceName
      << " @ " << config->ipAddress << ":" << config->port << std::endl;

    // Create PIController instance
    auto device = std::make_unique<PIController>();

    // Create MotionDevice for configuration
    MotionDevice motionDevice = CreateMotionDeviceFromConfig(*config);

    // Configure the device
    if (!device->ConfigureFromDevice(motionDevice)) {
      std::cout << "  Failed to configure PI device: " << deviceName << std::endl;
      return false;
    }

    // Attempt connection
    if (device->Connect(config->ipAddress, config->port)) {
      std::cout << "  Successfully connected PI device: " << deviceName
        << " (Controller ID: " << device->GetControllerId() << ")" << std::endl;

      // Set window title for identification
      device->SetWindowTitle("Controller: " + deviceName);

      // Store the connected device
      {
        std::lock_guard<std::mutex> lock(m_devicesMutex);
        m_realDevices[deviceName] = std::move(device);
        config->isConnected = true;
      }

      return true;
    }
    else {
      std::cout << "  Failed to connect to PI device at "
        << config->ipAddress << ":" << config->port << std::endl;
      return false;
    }

  }
  catch (const std::exception& e) {
    std::cout << "  Exception creating PI device " << deviceName << ": " << e.what() << std::endl;
    return false;
  }
}

void PIControllerManagerStandardized::DestroyRealDevice(const std::string& deviceName) {
  // Note: mutex should already be locked by caller
  auto it = m_realDevices.find(deviceName);
  if (it != m_realDevices.end()) {
    if (it->second) {
      try {
        std::cout << "  Disconnecting PI device: " << deviceName << std::endl;

        // Stop all axes safely
        it->second->StopAllAxes();

        // Give time for operations to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Disconnect from hardware
        it->second->Disconnect();

      }
      catch (const std::exception& e) {
        std::cout << "  Exception disconnecting PI device " << deviceName
          << ": " << e.what() << std::endl;
      }
    }

    // Remove from devices map
    m_realDevices.erase(it);

    // Update config status
    PIDeviceConfig* config = GetMutableDeviceConfig(deviceName);
    if (config) {
      config->isConnected = false;
    }

    std::cout << "  Destroyed real device: " << deviceName << std::endl;
  }
}

PIController* PIControllerManagerStandardized::GetRealDevice(const std::string& deviceName) {
  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_realDevices.find(deviceName);
    return (it != m_realDevices.end()) ? it->second.get() : nullptr;
  }
  return nullptr;
}

const PIController* PIControllerManagerStandardized::GetRealDevice(const std::string& deviceName) const {
  if (m_hardwareMode) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_realDevices.find(deviceName);
    return (it != m_realDevices.end()) ? it->second.get() : nullptr;
  }
  return nullptr;
}

bool PIControllerManagerStandardized::IsRealDeviceConnected(const std::string& deviceName) const {
  if (m_hardwareMode) {
    const PIController* device = GetRealDevice(deviceName);
    return device && device->IsConnected();
  }
  else {
    // Mock mode fallback
    size_t index = FindMockDeviceIndex(deviceName);
    return (index < m_mockConnectionStates.size()) ? m_mockConnectionStates[index] : false;
  }
}

size_t PIControllerManagerStandardized::FindMockDeviceIndex(const std::string& deviceName) const {
  auto it = std::find(m_mockDeviceNames.begin(), m_mockDeviceNames.end(), deviceName);
  return (it != m_mockDeviceNames.end()) ? std::distance(m_mockDeviceNames.begin(), it) : m_mockDeviceNames.size();
}

PIControllerManagerStandardized::PIDeviceConfig* PIControllerManagerStandardized::GetMutableDeviceConfig(const std::string& deviceName) {
  std::lock_guard<std::mutex> lock(m_devicesMutex);
  auto it = m_deviceConfigs.find(deviceName);
  return (it != m_deviceConfigs.end()) ? &it->second : nullptr;
}

const PIControllerManagerStandardized::PIDeviceConfig* PIControllerManagerStandardized::GetConstDeviceConfig(const std::string& deviceName) const {
  std::lock_guard<std::mutex> lock(m_devicesMutex);
  auto it = m_deviceConfigs.find(deviceName);
  return (it != m_deviceConfigs.end()) ? &it->second : nullptr;
}

MotionDevice PIControllerManagerStandardized::CreateMotionDeviceFromConfig(const PIDeviceConfig& config) const {
  MotionDevice motionDevice;
  motionDevice.Name = config.name;
  motionDevice.IpAddress = config.ipAddress;
  motionDevice.Port = config.port;
  motionDevice.InstalledAxes = config.installAxes;
  motionDevice.IsEnabled = config.isEnabled;
  motionDevice.Id = config.id;
  return motionDevice;
}

bool PIControllerManagerStandardized::ValidateDeviceConfig(const PIDeviceConfig& config) const {
  if (config.name.empty()) {
    std::cout << "PIControllerManagerStandardized: Invalid config - empty device name" << std::endl;
    return false;
  }

  if (config.ipAddress.empty()) {
    std::cout << "PIControllerManagerStandardized: Invalid config - empty IP address for " << config.name << std::endl;
    return false;
  }

  if (config.port <= 0 || config.port > 65535) {
    std::cout << "PIControllerManagerStandardized: Invalid config - invalid port " << config.port
      << " for " << config.name << std::endl;
    return false;
  }

  return true;
}

bool PIControllerManagerStandardized::IsValidDeviceName(const std::string& deviceName) const {
  if (deviceName.empty()) {
    return false;
  }

  // Check for invalid characters
  for (char c : deviceName) {
    if (!std::isalnum(c) && c != '-' && c != '_') {
      return false;
    }
  }

  return true;
}