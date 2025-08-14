// IDeviceManagerInterface.h
#pragma once
#include <string>
#include <vector>

/**
 * Standardized Device Manager Interface - KISS design
 * All device managers (PI, ACS, Camera, IO, etc.) should implement this
 */
template<typename DeviceType>
class IDeviceManagerInterface {
public:
  virtual ~IDeviceManagerInterface() = default;

  // === CORE LIFECYCLE ===
  virtual bool Initialize() = 0;
  virtual bool ConnectAll() = 0;
  virtual bool DisconnectAll() = 0;

  // === DEVICE ACCESS ===
  virtual DeviceType* GetDevice(const std::string& deviceName) = 0;
  virtual const DeviceType* GetDevice(const std::string& deviceName) const = 0;

  // === DEVICE ENUMERATION ===
  virtual int GetDeviceCount() const = 0;
  virtual std::vector<std::string> GetDeviceNames() const = 0;
  virtual bool HasDevice(const std::string& deviceName) const = 0;

  // === INDIVIDUAL DEVICE CONTROL ===
  virtual bool ConnectDevice(const std::string& deviceName) = 0;
  virtual bool DisconnectDevice(const std::string& deviceName) = 0;
  virtual bool IsDeviceConnected(const std::string& deviceName) const = 0;

  // === MANAGER INFO ===
  virtual std::string GetManagerType() const = 0;
  virtual bool IsInitialized() const = 0;
};

/**
 * Simplified base implementation that provides common functionality
 * Device managers can inherit from this to reduce boilerplate
 */
template<typename DeviceType>
class DeviceManagerBase : public IDeviceManagerInterface<DeviceType> {
protected:
  bool m_isInitialized = false;
  std::string m_managerType;

public:
  explicit DeviceManagerBase(const std::string& managerType)
    : m_managerType(managerType) {
  }

  // Common implementations
  std::string GetManagerType() const override {
    return m_managerType;
  }

  bool IsInitialized() const override {
    return m_isInitialized;
  }

  bool HasDevice(const std::string& deviceName) const override {
    // Use this-> to explicitly call the derived class implementation
    return this->GetDevice(deviceName) != nullptr;
  }
};