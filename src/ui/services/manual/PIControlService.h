// src/ui/services/manual/PIControlService.h
#pragma once

#include "../UIServiceRegistry.h"
#include "core/ServiceLocator.h"
#include "imgui.h"
#include <string>
#include <memory>

// Forward declarations
class PIController;
class PIControllerManagerStandardized;

class PIControlService : public IUIService {
public:
  PIControlService();
  ~PIControlService();

  // IUIService interface
  void RenderUI() override;
  std::string GetServiceName() const override;
  std::string GetDisplayName() const override;
  std::string GetCategory() const override;
  bool IsAvailable() const override;

private:
  // Static UI state
  static inline int selectedDeviceIndex = 0;
  static inline float jogDistance = 1.0f;
  static inline float systemVelocity = 10.0f;

  // Rendering methods
  void RenderDevicesList();
  void RenderSelectedDeviceControl();
  void RenderJogButtonGroup(const std::string& axis, PIController* device);
  void RenderNoManagerInterface();

  // Helper methods
  void ConnectDevice(const std::string& deviceName);
  void DisconnectDevice(const std::string& deviceName);
  void HomeAllAxes(PIController* device);

  // Utility methods
  PIControllerManagerStandardized* GetPIManager();
};