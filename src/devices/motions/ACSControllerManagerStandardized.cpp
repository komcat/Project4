// =====================================================
// ACSControllerManagerStandardized.cpp - UPDATED IMPLEMENTATION
// =====================================================

#include "ACSControllerManagerStandardized.h"
#include "core/ConfigRegistry.h"
#include <iostream>

ACSControllerManagerStandardized::ACSControllerManagerStandardized(ConfigManager& configManager)
  : DeviceManagerBase("ACS_Controller_Manager"), m_configManager(configManager) {
  LoadDevicesFromConfig();
}

// === CORE LIFECYCLE ===
bool ACSControllerManagerStandardized::Initialize() {
  if (m_isInitialized) return true;

  std::cout << "ACSControllerManager: Initialize() - KISS design" << std::endl;

  // Clear existing data
  m_controllers.clear();
  m_deviceNames.clear();

  // Reload config and create controllers
  LoadDevicesFromConfig();

  for (const auto& config : m_deviceConfigs) {
    if (config.isEnabled) {
      std::cout << "Creating ACS controller: " << config.name
        << " @ " << config.ipAddress << ":" << config.port << std::endl;

      // Create controller instance (constructor takes no parameters)
      auto controller = std::make_unique<ACSController>();

      // Configure the controller with device info if needed
      // (Note: ConfigureFromDevice would need MotionDevice struct)

      m_controllers[config.name] = std::move(controller);
      m_deviceNames.push_back(config.name);
    }
  }

  m_isInitialized = true;
  std::cout << "ACSControllerManager: Initialized with " << m_controllers.size() << " devices" << std::endl;
  return true;
}

bool ACSControllerManagerStandardized::ConnectAll() {
  if (!m_isInitialized) return false;

  std::cout << "ACSControllerManager: ConnectAll()" << std::endl;
  bool allSuccess = true;

  for (const auto& [deviceName, controller] : m_controllers) {
    // Find the config for this device to get IP and port
    DeviceConfig* config = FindDeviceConfig(deviceName);
    if (!config) {
      std::cout << "  " << deviceName << ": ❌ FAIL (no config)" << std::endl;
      allSuccess = false;
      continue;
    }

    std::cout << "  Connecting " << deviceName << "... ";
    if (controller->Connect(config->ipAddress, config->port)) {
      std::cout << "✅ OK" << std::endl;
    }
    else {
      std::cout << "❌ FAIL" << std::endl;
      allSuccess = false;
    }
  }

  return allSuccess;
}

bool ACSControllerManagerStandardized::DisconnectAll() {
  std::cout << "ACSControllerManager: DisconnectAll()" << std::endl;
  bool allSuccess = true;

  for (const auto& [deviceName, controller] : m_controllers) {
    std::cout << "  Disconnecting " << deviceName << "... ";
    if (controller->Disconnect()) {
      std::cout << "✅ OK" << std::endl;
    }
    else {
      std::cout << "❌ FAIL" << std::endl;
      allSuccess = false;
    }
  }

  return allSuccess;
}

// === DEVICE ACCESS ===
ACSController* ACSControllerManagerStandardized::GetDevice(const std::string& deviceName) {
  auto it = m_controllers.find(deviceName);
  return (it != m_controllers.end()) ? it->second.get() : nullptr;
}

const ACSController* ACSControllerManagerStandardized::GetDevice(const std::string& deviceName) const {
  auto it = m_controllers.find(deviceName);
  return (it != m_controllers.end()) ? it->second.get() : nullptr;
}

// === DEVICE ENUMERATION ===
int ACSControllerManagerStandardized::GetDeviceCount() const {
  return static_cast<int>(m_deviceNames.size());
}

std::vector<std::string> ACSControllerManagerStandardized::GetDeviceNames() const {
  return m_deviceNames;
}

bool ACSControllerManagerStandardized::HasDevice(const std::string& deviceName) const {
  return m_controllers.find(deviceName) != m_controllers.end();
}

// === INDIVIDUAL DEVICE CONTROL ===
bool ACSControllerManagerStandardized::ConnectDevice(const std::string& deviceName) {
  auto it = m_controllers.find(deviceName);
  if (it != m_controllers.end()) {
    // Find the config for this device to get IP and port
    DeviceConfig* config = FindDeviceConfig(deviceName);
    if (!config) {
      std::cout << "ACSControllerManager: " << deviceName << " config not found" << std::endl;
      return false;
    }

    bool success = it->second->Connect(config->ipAddress, config->port);
    std::cout << "ACSControllerManager: " << deviceName << " connect: "
      << (success ? "✅ OK" : "❌ FAIL") << std::endl;
    return success;
  }
  std::cout << "ACSControllerManager: " << deviceName << " not found" << std::endl;
  return false;
}

bool ACSControllerManagerStandardized::DisconnectDevice(const std::string& deviceName) {
  auto it = m_controllers.find(deviceName);
  if (it != m_controllers.end()) {
    bool success = it->second->Disconnect();
    std::cout << "ACSControllerManager: " << deviceName << " disconnect: "
      << (success ? "✅ OK" : "❌ FAIL") << std::endl;
    return success;
  }
  std::cout << "ACSControllerManager: " << deviceName << " not found" << std::endl;
  return false;
}

bool ACSControllerManagerStandardized::IsDeviceConnected(const std::string& deviceName) const {
  auto it = m_controllers.find(deviceName);
  return (it != m_controllers.end()) ? it->second->IsConnected() : false;
}

// === DEVICE IDENTIFICATION ===
bool ACSControllerManagerStandardized::GetDeviceIdentification(const std::string& deviceName, std::string& manufacturerInfo) {
  auto it = m_controllers.find(deviceName);
  if (it == m_controllers.end()) {
    manufacturerInfo = "Device not found";
    return false;
  }

  ACSController* controller = it->second.get();
  if (!controller->IsConnected()) {
    manufacturerInfo = "ACS Controller [DISCONNECTED]";
    return false;
  }

  if (controller->GetDeviceIdentification(manufacturerInfo)) {
    std::cout << "ACSControllerManager: " << deviceName << " ID: " << manufacturerInfo << std::endl;
    return true;
  }
  else {
    manufacturerInfo = "ACS Controller [ID failed]";
    return false;
  }
}

// === ADDITIONAL UTILITY ===
void ACSControllerManagerStandardized::PrintDeviceStatus() const {
  std::cout << "\n=== ACS Device Status ===" << std::endl;
  std::cout << "Total devices: " << m_controllers.size() << std::endl;

  for (const auto& [deviceName, controller] : m_controllers) {
    bool connected = controller->IsConnected();
    std::cout << "  " << deviceName << ": " << (connected ? "✅ CONNECTED" : "❌ DISCONNECTED") << std::endl;
  }
  std::cout << "=========================" << std::endl;
}

// === PRIVATE HELPERS ===
void ACSControllerManagerStandardized::LoadDevicesFromConfig() {
  m_deviceConfigs.clear();

  std::cout << "ACSControllerManager: Loading ACS devices from configuration..." << std::endl;

  try {
    // Get all motion devices from config - this is the correct way
    auto devices = Config::Motion::GetAllDevices();

    // Filter for ACS controller devices
    for (const auto& device : devices) {
      if (device.typeController == "ACS" && device.isEnabled) {
        DeviceConfig config;
        config.name = device.name;
        config.ipAddress = device.ipAddress;
        config.port = device.port;
        config.isEnabled = device.isEnabled;
        config.installAxes = device.installAxes;

        m_deviceConfigs.push_back(config);

        std::cout << "ACSControllerManager: Found ACS device: " << config.name
          << " @ " << config.ipAddress << ":" << config.port
          << " [" << config.installAxes << "]" << std::endl;
      }
    }

    std::cout << "ACSControllerManager: Loaded " << m_deviceConfigs.size()
      << " ACS devices from configuration" << std::endl;

  }
  catch (const std::exception& e) {
    std::cout << "ACSControllerManager: Error loading from config: " << e.what() << std::endl;

    // Fallback to hardcoded devices if config fails
    DeviceConfig fallback;
    fallback.name = "gantry-main";
    fallback.ipAddress = "192.168.1.100";
    fallback.port = 701;
    fallback.isEnabled = true;
    fallback.installAxes = "XYZ";

    m_deviceConfigs.push_back(fallback);
    std::cout << "ACSControllerManager: Using fallback device configuration" << std::endl;
  }
}

ACSControllerManagerStandardized::DeviceConfig*
ACSControllerManagerStandardized::FindDeviceConfig(const std::string& deviceName) {
  for (auto& config : m_deviceConfigs) {
    if (config.name == deviceName) {
      return &config;
    }
  }
  return nullptr;
}