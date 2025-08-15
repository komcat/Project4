#include "ACSControllerManagerStandardized.h"
#include "core/ConfigRegistry.h"
#include <iostream>

// Constructor
ACSControllerManagerStandardized::ACSControllerManagerStandardized(ConfigManager& configManager)
  : DeviceManagerBase("ACS_Controller_Manager"), m_configManager(configManager) {

  std::cout << "ACSControllerManagerStandardized: Created with real ConfigManager" << std::endl;

  // Load devices from configuration
  LoadDevicesFromConfig();
}

// === CORE LIFECYCLE ===
bool ACSControllerManagerStandardized::Initialize() {
  if (m_isInitialized) return true;

  std::cout << "ACSControllerManagerStandardized: Initialize() - TEST STUB with real config" << std::endl;

  // Reload configuration and devices
  LoadDevicesFromConfig();

  m_isInitialized = true;
  return true;
}

bool ACSControllerManagerStandardized::ConnectAll() {
  if (!m_isInitialized) return false;

  std::cout << "ACSControllerManagerStandardized: ConnectAll() - TEST STUB" << std::endl;

  // Mock: Try to connect all devices with different pattern than PI
  for (size_t i = 0; i < m_mockConnectionStates.size(); ++i) {
    if (i < m_mockDeviceNames.size()) {
      m_mockConnectionStates[i] = (i % 3 != 0); // Different pattern than PI
      std::cout << "  Mock device '" << m_mockDeviceNames[i]
        << "': " << (m_mockConnectionStates[i] ? "CONNECTED" : "FAILED") << std::endl;
    }
  }
  return true; // Always succeed in test mode
}

bool ACSControllerManagerStandardized::DisconnectAll() {
  std::cout << "ACSControllerManagerStandardized: DisconnectAll() - TEST STUB" << std::endl;

  // Mock: Disconnect all devices
  for (size_t i = 0; i < m_mockConnectionStates.size(); ++i) {
    m_mockConnectionStates[i] = false;
  }
  return true;
}

// === DEVICE ACCESS ===
ACSController* ACSControllerManagerStandardized::GetDevice(const std::string& deviceName) {
  std::cout << "ACSControllerManagerStandardized: GetDevice('" << deviceName
    << "') - TEST STUB returning nullptr" << std::endl;
  return nullptr; // Return nullptr for test mode (no actual devices)
}

const ACSController* ACSControllerManagerStandardized::GetDevice(const std::string& deviceName) const {
  return nullptr; // Test mode
}

// === DEVICE ENUMERATION ===
int ACSControllerManagerStandardized::GetDeviceCount() const {
  return static_cast<int>(m_mockDeviceNames.size());
}

std::vector<std::string> ACSControllerManagerStandardized::GetDeviceNames() const {
  return m_mockDeviceNames;
}

// === INDIVIDUAL DEVICE CONTROL ===
bool ACSControllerManagerStandardized::ConnectDevice(const std::string& deviceName) {
  std::cout << "ACSControllerManagerStandardized: ConnectDevice('" << deviceName
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

bool ACSControllerManagerStandardized::DisconnectDevice(const std::string& deviceName) {
  std::cout << "ACSControllerManagerStandardized: DisconnectDevice('" << deviceName
    << "') - TEST STUB" << std::endl;

  size_t index = FindDeviceIndex(deviceName);
  if (index < m_mockDeviceNames.size()) {
    m_mockConnectionStates[index] = false;
    std::cout << "  Mock disconnected device: " << deviceName << std::endl;
    return true;
  }

  return false;
}

bool ACSControllerManagerStandardized::IsDeviceConnected(const std::string& deviceName) const {
  size_t index = FindDeviceIndex(deviceName);
  if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
    return m_mockConnectionStates[index];
  }
  return false;
}

// === TESTING UTILITIES ===
void ACSControllerManagerStandardized::SetMockDeviceConnected(const std::string& deviceName, bool connected) {
  size_t index = FindDeviceIndex(deviceName);
  if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
    m_mockConnectionStates[index] = connected;
    std::cout << "Mock: Set " << deviceName << " to "
      << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
  }
}

// === PRIVATE HELPER METHODS ===
size_t ACSControllerManagerStandardized::FindDeviceIndex(const std::string& deviceName) const {
  for (size_t i = 0; i < m_mockDeviceNames.size(); ++i) {
    if (m_mockDeviceNames[i] == deviceName) {
      return i;
    }
  }
  return m_mockDeviceNames.size(); // Return invalid index if not found
}

void ACSControllerManagerStandardized::LoadDevicesFromConfig() {
  std::cout << "ACSControllerManagerStandardized: Loading ACS devices from configuration..." << std::endl;

  // Clear existing mock data
  m_mockDeviceNames.clear();
  m_mockConnectionStates.clear();

  try {
    // Get all motion devices from config
    auto devices = Config::Motion::GetAllDevices();

    // Filter for ACS controller devices
    for (const auto& device : devices) {
      if (device.typeController == "ACS" && device.isEnabled) {
        m_mockDeviceNames.push_back(device.name);
        m_mockConnectionStates.push_back(false); // Start disconnected
        std::cout << "  Found ACS device: " << device.name
          << " @ " << device.ipAddress << ":" << device.port << std::endl;
      }
    }

    std::cout << "ACSControllerManagerStandardized: Loaded " << m_mockDeviceNames.size()
      << " ACS devices from configuration" << std::endl;

  }
  catch (const std::exception& e) {
    std::cout << "ACSControllerManagerStandardized: Error loading from config: " << e.what() << std::endl;

    // Fallback to hardcoded devices if config fails
    m_mockDeviceNames = { "gantry-main", "gantry-secondary" };
    m_mockConnectionStates = { false, false };
    std::cout << "ACSControllerManagerStandardized: Using fallback device list" << std::endl;
  }
}
