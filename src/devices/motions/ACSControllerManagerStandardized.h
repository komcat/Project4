// ACSControllerManagerStandardized.h
#pragma once
#include "devices/IDeviceManagerInterface.h"
#include "core/ConfigManager.h"
#include <vector>
#include <string>

// Forward declarations for testing
class ACSController;

/**
 * Standardized ACS Controller Manager - TEST STUB VERSION
 * Returns mock data for testing without requiring actual hardware
 * Now uses real ConfigManager to load device configuration
 */
class ACSControllerManagerStandardized : public DeviceManagerBase<ACSController> {
private:
  // Mock data for testing
  std::vector<std::string> m_mockDeviceNames;
  std::vector<bool> m_mockConnectionStates;

  // Configuration manager reference
  ConfigManager& m_configManager;

public:
  explicit ACSControllerManagerStandardized(ConfigManager& configManager);
  ~ACSControllerManagerStandardized() override = default;

  // === CORE LIFECYCLE ===
  bool Initialize() override;
  bool ConnectAll() override;
  bool DisconnectAll() override;

  // === DEVICE ACCESS ===
  ACSController* GetDevice(const std::string& deviceName) override;
  const ACSController* GetDevice(const std::string& deviceName) const override;

  // === DEVICE ENUMERATION ===
  int GetDeviceCount() const override;
  std::vector<std::string> GetDeviceNames() const override;

  // === INDIVIDUAL DEVICE CONTROL ===
  bool ConnectDevice(const std::string& deviceName) override;
  bool DisconnectDevice(const std::string& deviceName) override;
  bool IsDeviceConnected(const std::string& deviceName) const override;

  // === TESTING UTILITIES ===
  void SetMockDeviceConnected(const std::string& deviceName, bool connected);

private:
  // Helper methods
  size_t FindDeviceIndex(const std::string& deviceName) const;
  void LoadDevicesFromConfig();
};