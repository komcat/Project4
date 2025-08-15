// PIControllerManagerStandardized.h
#pragma once
#include "devices/IDeviceManagerInterface.h"
#include "core/ConfigManager.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "PIController.h"  // Include your actual PI controller class header
// Forward declarations for testing
//class PIController;

/**
 * Standardized PI Controller Manager - REAL HARDWARE VERSION
 * Manages real PI controllers loaded from configuration
 * Falls back to mock behavior when hardware is unavailable
 */
class PIControllerManagerStandardized : public DeviceManagerBase<PIController> {
private:
  // Real device storage
  std::unordered_map<std::string, std::unique_ptr<PIController>> m_realDevices;

  // Device configuration storage
  struct PIDeviceConfig {
    std::string name;
    std::string ipAddress;
    int port;
    int id;
    bool isEnabled;
    std::string installAxes;
  };
  std::unordered_map<std::string, PIDeviceConfig> m_deviceConfigs;

  // Mock data for fallback (when hardware unavailable)
  std::vector<std::string> m_mockDeviceNames;
  std::vector<bool> m_mockConnectionStates;

  // Configuration manager reference
  ConfigManager& m_configManager;

  // Hardware vs Mock mode
  bool m_hardwareMode;

public:
  explicit PIControllerManagerStandardized(ConfigManager& configManager, bool hardwareMode = false);
  ~PIControllerManagerStandardized() override = default;

  // === CORE LIFECYCLE ===
  bool Initialize() override;
  bool ConnectAll() override;
  bool DisconnectAll() override;

  // === DEVICE ACCESS ===
  PIController* GetDevice(const std::string& deviceName) override;
  const PIController* GetDevice(const std::string& deviceName) const override;

  // === DEVICE ENUMERATION ===
  int GetDeviceCount() const override;
  std::vector<std::string> GetDeviceNames() const override;

  // === INDIVIDUAL DEVICE CONTROL ===
  bool ConnectDevice(const std::string& deviceName) override;
  bool DisconnectDevice(const std::string& deviceName) override;
  bool IsDeviceConnected(const std::string& deviceName) const override;

  // === TESTING UTILITIES ===
  void SetMockDeviceConnected(const std::string& deviceName, bool connected);
  void SetHardwareMode(bool enabled) { m_hardwareMode = enabled; }
  bool IsHardwareMode() const { return m_hardwareMode; }

  // === HARDWARE-SPECIFIC METHODS ===
  bool IsDeviceResponding(const std::string& deviceName) const;
  std::string GetDeviceInfo(const std::string& deviceName) const;
  PIDeviceConfig GetDeviceConfig(const std::string& deviceName) const;

private:
  // Helper methods
  size_t FindDeviceIndex(const std::string& deviceName) const;
  void LoadDevicesFromConfig();
  bool CreateRealDevice(const std::string& deviceName);
  void DestroyRealDevice(const std::string& deviceName);
  PIDeviceConfig* GetMutableDeviceConfig(const std::string& deviceName);
};