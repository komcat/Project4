#include "PIControllerManagerStandardized.h"
#include "core/ConfigRegistry.h"
#include <iostream>

// Constructor
PIControllerManagerStandardized::PIControllerManagerStandardized(ConfigManager& configManager)
  : DeviceManagerBase("PI_Controller_Manager"), m_configManager(configManager) {

  std::cout << "PIControllerManagerStandardized: Created with real ConfigManager" << std::endl;

  // Load devices from configuration
  LoadDevicesFromConfig();
}

// === CORE LIFECYCLE ===
bool PIControllerManagerStandardized::Initialize() {
  if (m_isInitialized) return true;

  std::cout << "PIControllerManagerStandardized: Initialize() - TEST STUB with real config" << std::endl;

  // Reload configuration and devices
  LoadDevicesFromConfig();

  m_isInitialized = true;
  return true;
}

bool PIControllerManagerStandardized::ConnectAll() {
  if (!m_isInitialized) return false;

  std::cout << "PIControllerManagerStandardized: ConnectAll() - TEST STUB" << std::endl;

  // Mock: Connect some devices successfully
  for (size_t i = 0; i < m_mockConnectionStates.size(); ++i) {
    if (i < m_mockDeviceNames.size()) {
      m_mockConnectionStates[i] = (i % 2 == 0); // Connect every other device
      std::cout << "  Mock device '" << m_mockDeviceNames[i]
        << "': " << (m_mockConnectionStates[i] ? "CONNECTED" : "FAILED") << std::endl;
    }
  }
  return true; // Always succeed in test mode
}

bool PIControllerManagerStandardized::DisconnectAll() {
  std::cout << "PIControllerManagerStandardized: DisconnectAll() - TEST STUB" << std::endl;

  // Mock: Disconnect all devices
  for (size_t i = 0; i < m_mockConnectionStates.size(); ++i) {
    m_mockConnectionStates[i] = false;
  }
  return true;
}

// === DEVICE ACCESS ===
PIController* PIControllerManagerStandardized::GetDevice(const std::string& deviceName) {
  std::cout << "PIControllerManagerStandardized: GetDevice('" << deviceName
    << "') - TEST STUB returning nullptr" << std::endl;
  return nullptr; // Return nullptr for test mode (no actual devices)
}

const PIController* PIControllerManagerStandardized::GetDevice(const std::string& deviceName) const {
  return nullptr; // Test mode
}

// === DEVICE ENUMERATION ===
int PIControllerManagerStandardized::GetDeviceCount() const {
  return static_cast<int>(m_mockDeviceNames.size());
}

std::vector<std::string> PIControllerManagerStandardized::GetDeviceNames() const {
  return m_mockDeviceNames;
}

// === INDIVIDUAL DEVICE CONTROL ===
bool PIControllerManagerStandardized::ConnectDevice(const std::string& deviceName) {
  std::cout << "PIControllerManagerStandardized: ConnectDevice('" << deviceName
    << "') - TEST STUB" << std::endl;

  size_t index = FindDeviceIndex(deviceName);
  if (index < m_mockDeviceNames.size()) {
    m_mockConnectionStates[index] = true;
    std::cout << "  Mock connected device: " << deviceName << std::endl;
    return true;
  }

  std::cout << "  Mock device not found: " << deviceName << std::endl;
  return false;
}

bool PIControllerManagerStandardized::DisconnectDevice(const std::string& deviceName) {
  std::cout << "PIControllerManagerStandardized: DisconnectDevice('" << deviceName
    << "') - TEST STUB" << std::endl;

  size_t index = FindDeviceIndex(deviceName);
  if (index < m_mockDeviceNames.size()) {
    m_mockConnectionStates[index] = false;
    std::cout << "  Mock disconnected device: " << deviceName << std::endl;
    return true;
  }

  return false;
}

bool PIControllerManagerStandardized::IsDeviceConnected(const std::string& deviceName) const {
  size_t index = FindDeviceIndex(deviceName);
  if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
    return m_mockConnectionStates[index];
  }
  return false;
}

// === TESTING UTILITIES ===
void PIControllerManagerStandardized::SetMockDeviceConnected(const std::string& deviceName, bool connected) {
  size_t index = FindDeviceIndex(deviceName);
  if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
    m_mockConnectionStates[index] = connected;
    std::cout << "Mock: Set " << deviceName << " to "
      << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
  }
}

// === PRIVATE HELPER METHODS ===
size_t PIControllerManagerStandardized::FindDeviceIndex(const std::string& deviceName) const {
  for (size_t i = 0; i < m_mockDeviceNames.size(); ++i) {
    if (m_mockDeviceNames[i] == deviceName) {
      return i;
    }
  }
  return m_mockDeviceNames.size(); // Return invalid index if not found
}

void PIControllerManagerStandardized::LoadDevicesFromConfig() {
  std::cout << "PIControllerManagerStandardized: Loading PI devices from configuration..." << std::endl;

  // Clear existing mock data
  m_mockDeviceNames.clear();
  m_mockConnectionStates.clear();

  try {
    // Get all motion devices from config
    auto devices = Config::Motion::GetAllDevices();

    // Filter for PI controller devices
    for (const auto& device : devices) {
      if (device.typeController == "PI" && device.isEnabled) {
        m_mockDeviceNames.push_back(device.name);
        m_mockConnectionStates.push_back(false); // Start disconnected
        std::cout << "  Found PI device: " << device.name
          << " @ " << device.ipAddress << ":" << device.port << std::endl;
      }
    }

    std::cout << "PIControllerManagerStandardized: Loaded " << m_mockDeviceNames.size()
      << " PI devices from configuration" << std::endl;

  }
  catch (const std::exception& e) {
    std::cout << "PIControllerManagerStandardized: Error loading from config: " << e.what() << std::endl;

    // Fallback to hardcoded devices if config fails
    m_mockDeviceNames = { "hex-left", "hex-right", "hex-bottom" };
    m_mockConnectionStates = { false, false, false };
    std::cout << "PIControllerManagerStandardized: Using fallback device list" << std::endl;
  }
}