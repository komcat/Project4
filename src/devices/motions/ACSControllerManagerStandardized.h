// =====================================================
// ACSControllerManagerStandardized.h - UPDATED FOR NEW INTERFACE
// =====================================================

#pragma once
#include "devices/IDeviceManagerInterface.h"
#include "core/ConfigManager.h"
#include "devices/motions/ACSController.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

/**
 * ACS Controller Manager - Now fully compliant with IDeviceManagerInterface
 * KISS design - simple and focused on essential operations
 */
class ACSControllerManagerStandardized : public DeviceManagerBase<ACSController> {
private:
  // Device management
  std::unordered_map<std::string, std::unique_ptr<ACSController>> m_controllers;
  std::vector<std::string> m_deviceNames;
  ConfigManager& m_configManager;

  // Device configuration
  struct DeviceConfig {
    std::string name;
    std::string ipAddress;
    int port;
    bool isEnabled;
    std::string installAxes;
  };
  std::vector<DeviceConfig> m_deviceConfigs;

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
  bool HasDevice(const std::string& deviceName) const override;

  // === INDIVIDUAL DEVICE CONTROL ===
  bool ConnectDevice(const std::string& deviceName) override;
  bool DisconnectDevice(const std::string& deviceName) override;
  bool IsDeviceConnected(const std::string& deviceName) const override;

  // === DEVICE IDENTIFICATION ===
  bool GetDeviceIdentification(const std::string& deviceName, std::string& manufacturerInfo) override;

  // === ADDITIONAL UTILITY ===
  void PrintDeviceStatus() const;

private:
  void LoadDevicesFromConfig();
  DeviceConfig* FindDeviceConfig(const std::string& deviceName);
};