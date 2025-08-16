// src/ui/services/manual/GantryService.h
#pragma once

#include "../UIServiceRegistry.h"
#include "core/ServiceLocator.h"
#include "imgui.h"
#include <string>
#include <memory>

// Forward declarations
class ACSController;
class ACSControllerManagerStandardized;

class GantryService : public IUIService {
public:
  GantryService();
  ~GantryService();

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
  static inline float axisVelocity = 50.0f;

  // Rendering methods
  void RenderDevicesList();
  void RenderSelectedDeviceControl();
  void RenderJogButtonGroup(const std::string& axis, ACSController* device);
  void RenderNoManagerInterface();

  // Helper methods
  void ConnectDevice(const std::string& deviceName);
  void DisconnectDevice(const std::string& deviceName);
  void HomeAllAxes(ACSController* device);

  // Utility methods
  ACSControllerManagerStandardized* GetACSManager();
};