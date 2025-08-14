// ACSControllerManagerStandardized.h
#pragma once
#include "devices/IDeviceManagerInterface.h"
#include <iostream>  // For std::cout
// #include "include/motions/acs_controller_manager.h"  // Commented out for testing
// #include "include/motions/acs_controller.h"          // Commented out for testing

// Forward declarations for testing
class ACSController;
class MockMotionConfigManager;  // Forward declare our mock

/**
 * Standardized ACS Controller Manager - TEST STUB VERSION
 * Returns mock data for testing without requiring actual hardware
 */
class ACSControllerManagerStandardized : public DeviceManagerBase<ACSController> {
private:
  // Mock data for testing
  std::vector<std::string> m_mockDeviceNames = { "gantry-main", "gantry-secondary" };
  std::vector<bool> m_mockConnectionStates = { false, true }; // Different states for variety

public:
  explicit ACSControllerManagerStandardized(MockMotionConfigManager& configManager)
    : DeviceManagerBase("ACS_Controller_Manager") {
    // Don't create original manager for testing
    std::cout << "ACSControllerManagerStandardized: Created in TEST MODE" << std::endl;
    (void)configManager; // Suppress unused parameter warning
  }

  // === CORE LIFECYCLE ===
  bool Initialize() override {
    if (m_isInitialized) return true;

    std::cout << "ACSControllerManagerStandardized: Initialize() - TEST STUB" << std::endl;
    m_isInitialized = true;  // Always succeed in test mode
    return true;
  }

  bool ConnectAll() override {
    if (!m_isInitialized) return false;

    std::cout << "ACSControllerManagerStandardized: ConnectAll() - TEST STUB" << std::endl;
    // Mock: Try to connect all devices
    for (size_t i = 0; i < m_mockConnectionStates.size(); ++i) {
      if (i < m_mockDeviceNames.size()) {
        m_mockConnectionStates[i] = (i % 3 != 0); // Different pattern than PI
        std::cout << "  Mock device '" << m_mockDeviceNames[i]
          << "': " << (m_mockConnectionStates[i] ? "CONNECTED" : "FAILED") << std::endl;
      }
    }
    return true; // Always succeed in test mode
  }

  bool DisconnectAll() override {
    std::cout << "ACSControllerManagerStandardized: DisconnectAll() - TEST STUB" << std::endl;
    // Mock: Disconnect all devices
    for (size_t i = 0; i < m_mockConnectionStates.size(); ++i) {
      m_mockConnectionStates[i] = false;
    }
    return true;
  }

  // === DEVICE ACCESS ===
  ACSController* GetDevice(const std::string& deviceName) override {
    // Return nullptr for test mode (no actual devices)
    std::cout << "ACSControllerManagerStandardized: GetDevice('" << deviceName << "') - TEST STUB returning nullptr" << std::endl;
    return nullptr;
  }

  const ACSController* GetDevice(const std::string& deviceName) const override {
    return nullptr; // Test mode
  }

  // === DEVICE ENUMERATION ===
  int GetDeviceCount() const override {
    return static_cast<int>(m_mockDeviceNames.size());
  }

  std::vector<std::string> GetDeviceNames() const override {
    return m_mockDeviceNames;
  }

  // === INDIVIDUAL DEVICE CONTROL ===
  bool ConnectDevice(const std::string& deviceName) override {
    std::cout << "ACSControllerManagerStandardized: ConnectDevice('" << deviceName << "') - TEST STUB" << std::endl;

    // Find device in mock list
    for (size_t i = 0; i < m_mockDeviceNames.size(); ++i) {
      if (m_mockDeviceNames[i] == deviceName) {
        m_mockConnectionStates[i] = true;
        std::cout << "  Mock connected device: " << deviceName << std::endl;
        return true;
      }
    }

    std::cout << "  Mock device not found: " << deviceName << std::endl;
    return false;
  }

  bool DisconnectDevice(const std::string& deviceName) override {
    std::cout << "ACSControllerManagerStandardized: DisconnectDevice('" << deviceName << "') - TEST STUB" << std::endl;

    // Find device in mock list
    for (size_t i = 0; i < m_mockDeviceNames.size(); ++i) {
      if (m_mockDeviceNames[i] == deviceName) {
        m_mockConnectionStates[i] = false;
        std::cout << "  Mock disconnected device: " << deviceName << std::endl;
        return true;
      }
    }

    return false;
  }

  bool IsDeviceConnected(const std::string& deviceName) const override {
    // Check mock connection state
    for (size_t i = 0; i < m_mockDeviceNames.size(); ++i) {
      if (m_mockDeviceNames[i] == deviceName) {
        return i < m_mockConnectionStates.size() ? m_mockConnectionStates[i] : false;
      }
    }
    return false;
  }

  // === TESTING UTILITIES ===
  void SetMockDeviceConnected(const std::string& deviceName, bool connected) {
    for (size_t i = 0; i < m_mockDeviceNames.size(); ++i) {
      if (m_mockDeviceNames[i] == deviceName) {
        if (i < m_mockConnectionStates.size()) {
          m_mockConnectionStates[i] = connected;
          std::cout << "Mock: Set " << deviceName << " to "
            << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
        }
        break;
      }
    }
  }
};