// PIControllerManagerStandardized.h
#pragma once

#include "devices/IDeviceManagerInterface.h"
#include "core/ConfigManager.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <iostream>

// Forward declaration - include PIController.h only in .cpp file
class PIController;
struct MotionDevice;

/**
 * Standardized PI Controller Manager
 * Manages PIController instances based on configuration
 * Supports both hardware and mock modes
 */
class PIControllerManagerStandardized : public DeviceManagerBase<PIController> {
public:
  // Device configuration structure
  struct PIDeviceConfig {
    std::string name;
    std::string ipAddress = "192.168.1.100";
    int port = 50000;
    int id = 1;
    bool isEnabled = false;
    bool isConnected = false;
    std::string installAxes = "X Y Z U V W";
    std::string typeController = "PI";

    PIDeviceConfig() = default;
    PIDeviceConfig(const std::string& deviceName, const std::string& ip, int portNum)
      : name(deviceName), ipAddress(ip), port(portNum), isEnabled(true) {
    }
  };

private:
  // Real PIController devices storage
  std::unordered_map<std::string, std::unique_ptr<PIController>> m_realDevices;

  // Device configurations
  std::unordered_map<std::string, PIDeviceConfig> m_deviceConfigs;

  // Mock data for testing/fallback
  std::vector<std::string> m_mockDeviceNames;
  std::vector<bool> m_mockConnectionStates;

  // Configuration manager reference
  ConfigManager& m_configManager;

  // Operating mode
  bool m_hardwareMode;

  // Thread safety
  mutable std::mutex m_devicesMutex;

public:
  // Constructor
  explicit PIControllerManagerStandardized(ConfigManager& configManager, bool hardwareMode = true);

  // Destructor - explicitly declared for unique_ptr with forward declaration
  ~PIControllerManagerStandardized();

  // === CORE LIFECYCLE METHODS ===
  bool Initialize() override;
  bool ConnectAll() override;
  bool DisconnectAll() override;

  // === DEVICE ACCESS METHODS ===
  PIController* GetDevice(const std::string& deviceName) override;
  const PIController* GetDevice(const std::string& deviceName) const override;

  // === DEVICE ENUMERATION METHODS ===
  int GetDeviceCount() const override;
  std::vector<std::string> GetDeviceNames() const override;

  // === INDIVIDUAL DEVICE CONTROL ===
  bool ConnectDevice(const std::string& deviceName) override;
  bool DisconnectDevice(const std::string& deviceName) override;
  bool IsDeviceConnected(const std::string& deviceName) const override;

  // === CONFIGURATION METHODS ===
  bool AddDeviceConfig(const std::string& deviceName, const std::string& ipAddress, int port = 50000);
  bool RemoveDeviceConfig(const std::string& deviceName);
  PIDeviceConfig GetDeviceConfig(const std::string& deviceName) const;
  std::vector<PIDeviceConfig> GetAllDeviceConfigs() const;

  // === MODE CONTROL ===
  void SetHardwareMode(bool enabled);
  bool IsHardwareMode() const { return m_hardwareMode; }

  // === TESTING/MOCK UTILITIES ===
  void SetMockDeviceConnected(const std::string& deviceName, bool connected);
  void AddMockDevice(const std::string& deviceName);

  // === STATUS AND DIAGNOSTICS ===
  int GetConnectedDeviceCount() const;
  bool IsDeviceResponding(const std::string& deviceName) const;
  std::string GetDeviceInfo(const std::string& deviceName) const;
  void PrintDeviceStatus() const;

  // === BATCH OPERATIONS ===
  bool HomeAllDevices();
  bool StopAllDevices();
  std::vector<std::string> GetConnectedDeviceNames() const;

private:
  // === PRIVATE HELPER METHODS ===
  void LoadDevicesFromConfig();
  void CreateDefaultConfigs();

  // Real device management
  bool CreateRealDevice(const std::string& deviceName);
  void DestroyRealDevice(const std::string& deviceName);
  PIController* GetRealDevice(const std::string& deviceName);
  const PIController* GetRealDevice(const std::string& deviceName) const;
  bool IsRealDeviceConnected(const std::string& deviceName) const;

  // Mock device management
  size_t FindMockDeviceIndex(const std::string& deviceName) const;

  // Configuration helpers
  PIDeviceConfig* GetMutableDeviceConfig(const std::string& deviceName);
  const PIDeviceConfig* GetConstDeviceConfig(const std::string& deviceName) const;
  MotionDevice CreateMotionDeviceFromConfig(const PIDeviceConfig& config) const;

  // Validation
  bool ValidateDeviceConfig(const PIDeviceConfig& config) const;
  bool IsValidDeviceName(const std::string& deviceName) const;
};