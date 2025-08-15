#include "PIControllerManagerStandardized.h"
#include "core/ConfigRegistry.h"
#include <iostream>

// For actual PI hardware communication - you'll need to uncomment these when ready
// #include "devices/motions/PIController.h"  // Your actual PI controller class

// Constructor
PIControllerManagerStandardized::PIControllerManagerStandardized(ConfigManager& configManager, bool hardwareMode)
  : DeviceManagerBase("PI_Controller_Manager"), m_configManager(configManager), m_hardwareMode(hardwareMode) {

  std::cout << "PIControllerManagerStandardized: Created with real ConfigManager"
    << (m_hardwareMode ? " [HARDWARE MODE]" : " [MOCK MODE]") << std::endl;

  // Load devices from configuration
  LoadDevicesFromConfig();
}

// === CORE LIFECYCLE ===
bool PIControllerManagerStandardized::Initialize() {
  if (m_isInitialized) return true;

  std::cout << "PIControllerManagerStandardized: Initialize() - "
    << (m_hardwareMode ? "HARDWARE MODE" : "MOCK MODE") << std::endl;

  // Reload configuration and devices
  LoadDevicesFromConfig();

  m_isInitialized = true;
  return true;
}

bool PIControllerManagerStandardized::ConnectAll() {
  if (!m_isInitialized) return false;

  std::cout << "PIControllerManagerStandardized: ConnectAll() - "
    << (m_hardwareMode ? "HARDWARE MODE" : "MOCK MODE") << std::endl;

  bool allSuccess = true;

  if (m_hardwareMode) {
    // Real hardware connection
    for (const auto& [deviceName, config] : m_deviceConfigs) {
      if (config.isEnabled) {
        if (!ConnectDevice(deviceName)) {
          allSuccess = false;
          std::cout << "  Failed to connect real device: " << deviceName << std::endl;
        }
      }
    }
  }
  else {
    // Mock connection behavior
    for (size_t i = 0; i < m_mockConnectionStates.size(); ++i) {
      if (i < m_mockDeviceNames.size()) {
        m_mockConnectionStates[i] = (i % 2 == 0); // Connect every other device
        std::cout << "  Mock device '" << m_mockDeviceNames[i]
          << "': " << (m_mockConnectionStates[i] ? "CONNECTED" : "FAILED") << std::endl;
      }
    }
  }

  return allSuccess;
}

bool PIControllerManagerStandardized::DisconnectAll() {
  std::cout << "PIControllerManagerStandardized: DisconnectAll() - "
    << (m_hardwareMode ? "HARDWARE MODE" : "MOCK MODE") << std::endl;

  if (m_hardwareMode) {
    // Disconnect all real devices
    for (auto& [deviceName, device] : m_realDevices) {
      if (device) {
        // Call actual device disconnect here
        // device->Disconnect();
        std::cout << "  Disconnected real device: " << deviceName << std::endl;
      }
    }
    m_realDevices.clear();
  }
  else {
    // Mock disconnect all devices
    for (size_t i = 0; i < m_mockConnectionStates.size(); ++i) {
      m_mockConnectionStates[i] = false;
    }
  }

  return true;
}

// === DEVICE ACCESS ===
PIController* PIControllerManagerStandardized::GetDevice(const std::string& deviceName) {
  if (m_hardwareMode) {
    auto it = m_realDevices.find(deviceName);
    if (it != m_realDevices.end()) {
      std::cout << "PIControllerManagerStandardized: GetDevice('" << deviceName
        << "') - returning real device" << std::endl;
      return it->second.get();
    }
    else {
      std::cout << "PIControllerManagerStandardized: GetDevice('" << deviceName
        << "') - real device not found or not connected" << std::endl;
      return nullptr;
    }
  }
  else {
    std::cout << "PIControllerManagerStandardized: GetDevice('" << deviceName
      << "') - MOCK MODE returning nullptr" << std::endl;
    return nullptr;
  }
}

const PIController* PIControllerManagerStandardized::GetDevice(const std::string& deviceName) const {
  if (m_hardwareMode) {
    auto it = m_realDevices.find(deviceName);
    return (it != m_realDevices.end()) ? it->second.get() : nullptr;
  }
  else {
    return nullptr; // Mock mode
  }
}

// === DEVICE ENUMERATION ===
int PIControllerManagerStandardized::GetDeviceCount() const {
  if (m_hardwareMode) {
    return static_cast<int>(m_deviceConfigs.size());
  }
  else {
    return static_cast<int>(m_mockDeviceNames.size());
  }
}

std::vector<std::string> PIControllerManagerStandardized::GetDeviceNames() const {
  if (m_hardwareMode) {
    std::vector<std::string> names;
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
  std::cout << "PIControllerManagerStandardized: ConnectDevice('" << deviceName
    << "') - " << (m_hardwareMode ? "HARDWARE MODE" : "MOCK MODE") << std::endl;

  if (m_hardwareMode) {
    // Check if device already connected
    if (m_realDevices.find(deviceName) != m_realDevices.end()) {
      std::cout << "  Device already connected: " << deviceName << std::endl;
      return true;
    }

    // Try to create and connect real device
    if (CreateRealDevice(deviceName)) {
      std::cout << "  Successfully connected real device: " << deviceName << std::endl;
      return true;
    }
    else {
      std::cout << "  Failed to connect real device: " << deviceName << std::endl;
      return false;
    }
  }
  else {
    // Mock mode
    size_t index = FindDeviceIndex(deviceName);
    if (index < m_mockDeviceNames.size()) {
      m_mockConnectionStates[index] = true;
      std::cout << "  Mock connected device: " << deviceName << std::endl;
      return true;
    }

    std::cout << "  Mock device not found: " << deviceName << std::endl;
    return false;
  }
}

bool PIControllerManagerStandardized::DisconnectDevice(const std::string& deviceName) {
  std::cout << "PIControllerManagerStandardized: DisconnectDevice('" << deviceName
    << "') - " << (m_hardwareMode ? "HARDWARE MODE" : "MOCK MODE") << std::endl;

  if (m_hardwareMode) {
    auto it = m_realDevices.find(deviceName);
    if (it != m_realDevices.end()) {
      // Call actual device disconnect
      // it->second->Disconnect();
      m_realDevices.erase(it);
      std::cout << "  Disconnected real device: " << deviceName << std::endl;
      return true;
    }
    else {
      std::cout << "  Real device not found or not connected: " << deviceName << std::endl;
      return false;
    }
  }
  else {
    // Mock mode
    size_t index = FindDeviceIndex(deviceName);
    if (index < m_mockDeviceNames.size()) {
      m_mockConnectionStates[index] = false;
      std::cout << "  Mock disconnected device: " << deviceName << std::endl;
      return true;
    }
    return false;
  }
}

bool PIControllerManagerStandardized::IsDeviceConnected(const std::string& deviceName) const {
  if (m_hardwareMode) {
    auto it = m_realDevices.find(deviceName);
    if (it != m_realDevices.end()) {
      // Check real device connection status
      // return it->second->IsConnected();
      return true; // For now, assume connected if in map
    }
    return false;
  }
  else {
    // Mock mode
    size_t index = FindDeviceIndex(deviceName);
    if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
      return m_mockConnectionStates[index];
    }
    return false;
  }
}

// === TESTING UTILITIES ===
void PIControllerManagerStandardized::SetMockDeviceConnected(const std::string& deviceName, bool connected) {
  if (!m_hardwareMode) {
    size_t index = FindDeviceIndex(deviceName);
    if (index < m_mockDeviceNames.size() && index < m_mockConnectionStates.size()) {
      m_mockConnectionStates[index] = connected;
      std::cout << "Mock: Set " << deviceName << " to "
        << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
    }
  }
  else {
    std::cout << "SetMockDeviceConnected ignored in hardware mode" << std::endl;
  }
}

// === HARDWARE-SPECIFIC METHODS ===
bool PIControllerManagerStandardized::IsDeviceResponding(const std::string& deviceName) const {
  if (m_hardwareMode) {
    auto it = m_realDevices.find(deviceName);
    if (it != m_realDevices.end()) {
      // Check if device is responding to commands
      // return it->second->Ping() or similar
      return true; // Placeholder
    }
  }
  return false;
}

std::string PIControllerManagerStandardized::GetDeviceInfo(const std::string& deviceName) const {
  auto config = GetDeviceConfig(deviceName);
  if (!config.name.empty()) {
    return "PI Device: " + config.name + " @ " + config.ipAddress + ":" + std::to_string(config.port) +
      " [Axes: " + config.installAxes + "]";
  }
  return "Device not found: " + deviceName;
}

PIControllerManagerStandardized::PIDeviceConfig PIControllerManagerStandardized::GetDeviceConfig(const std::string& deviceName) const {
  auto it = m_deviceConfigs.find(deviceName);
  return (it != m_deviceConfigs.end()) ? it->second : PIDeviceConfig{};
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

  // Clear existing data
  m_mockDeviceNames.clear();
  m_mockConnectionStates.clear();
  m_deviceConfigs.clear();

  try {
    // Get all motion devices from config
    auto devices = Config::Motion::GetAllDevices();

    // Filter for PI controller devices
    for (const auto& device : devices) {
      if (device.typeController == "PI" && device.isEnabled) {
        // Store device configuration
        PIDeviceConfig config;
        config.name = device.name;
        config.ipAddress = device.ipAddress;
        config.port = device.port;
        config.id = device.id;
        config.isEnabled = device.isEnabled;
        config.installAxes = device.installAxes;

        m_deviceConfigs[device.name] = config;

        // Also populate mock data for fallback
        m_mockDeviceNames.push_back(device.name);
        m_mockConnectionStates.push_back(false); // Start disconnected

        std::cout << "  Found PI device: " << device.name
          << " @ " << device.ipAddress << ":" << device.port
          << " [Axes: " << device.installAxes << "]" << std::endl;
      }
    }

    std::cout << "PIControllerManagerStandardized: Loaded " << m_deviceConfigs.size()
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

bool PIControllerManagerStandardized::CreateRealDevice(const std::string& deviceName) {
  auto* config = GetMutableDeviceConfig(deviceName);
  if (!config) {
    std::cout << "  Device config not found: " << deviceName << std::endl;
    return false;
  }

  try {
    // TODO: Uncomment and implement when PIController class is ready
    /*
    auto device = std::make_unique<PIController>();

    // Configure device
    device->SetIPAddress(config->ipAddress);
    device->SetPort(config->port);
    device->SetDeviceID(config->id);

    // Attempt connection
    if (device->Connect()) {
        // Additional setup if needed
        device->SetAxes(config->installAxes);

        // Store the connected device
        m_realDevices[deviceName] = std::move(device);
        return true;
    } else {
        std::cout << "  Failed to connect to PI device at "
                  << config->ipAddress << ":" << config->port << std::endl;
        return false;
    }
    */

    // Placeholder for now - simulate successful connection
    std::cout << "  [PLACEHOLDER] Would connect to PI device: " << deviceName
      << " @ " << config->ipAddress << ":" << config->port << std::endl;

    // Create a placeholder (nullptr for now)
    m_realDevices[deviceName] = nullptr; // TODO: Replace with real device
    return true;

  }
  catch (const std::exception& e) {
    std::cout << "  Exception creating PI device " << deviceName << ": " << e.what() << std::endl;
    return false;
  }
}

void PIControllerManagerStandardized::DestroyRealDevice(const std::string& deviceName) {
  auto it = m_realDevices.find(deviceName);
  if (it != m_realDevices.end()) {
    // TODO: Call device cleanup if needed
    // if (it->second) it->second->Disconnect();
    m_realDevices.erase(it);
    std::cout << "  Destroyed real device: " << deviceName << std::endl;
  }
}

PIControllerManagerStandardized::PIDeviceConfig* PIControllerManagerStandardized::GetMutableDeviceConfig(const std::string& deviceName) {
  auto it = m_deviceConfigs.find(deviceName);
  return (it != m_deviceConfigs.end()) ? &it->second : nullptr;
}