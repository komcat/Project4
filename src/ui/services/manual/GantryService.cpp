// src/ui/services/manual/GantryService.cpp

#include "GantryService.h"
#include "devices/motions/ACSControllerManagerStandardized.h"
#include <map>
#include <vector>
#include <iostream>

GantryService::GantryService() {
  // Constructor - any initialization if needed
}

GantryService::~GantryService() {
  // Destructor - cleanup if needed
}

// IUIService interface implementation
void GantryService::RenderUI() {
  auto acsManager = GetACSManager();

  ImGui::SetWindowFontScale(1.5f);
  ImGui::Text("🦾 ACS Gantry Controllers");
  ImGui::SetWindowFontScale(1.0f);

  ImGui::Spacing();
  ImGui::Text("High-Speed Gantry Motion Control System");
  ImGui::Separator();

  if (acsManager) {
    RenderDevicesList();
    ImGui::Separator();
    RenderSelectedDeviceControl();
  }
  else {
    RenderNoManagerInterface();
  }
}

std::string GantryService::GetServiceName() const {
  return "gantry_control";
}

std::string GantryService::GetDisplayName() const {
  return "Gantry Control";
}

std::string GantryService::GetCategory() const {
  return "Manual";
}

bool GantryService::IsAvailable() const {
  return true;
}

// Private rendering methods
void GantryService::RenderDevicesList() {
  auto acsManager = GetACSManager();
  auto deviceNames = acsManager->GetDeviceNames();

  ImGui::Text("📋 Available Gantry Controllers:");
  ImGui::Spacing();

  // Device list with status
  for (size_t i = 0; i < deviceNames.size(); i++) {
    const auto& deviceName = deviceNames[i];
    bool isConnected = acsManager->IsDeviceConnected(deviceName);

    ImGui::PushID(i);

    // Device selection radio button
    if (ImGui::RadioButton(("##device_" + std::to_string(i)).c_str(), selectedDeviceIndex == (int)i)) {
      selectedDeviceIndex = (int)i;
    }

    ImGui::SameLine();

    // Device info
    ImGui::Text("🔧 %s", deviceName.c_str());
    ImGui::SameLine(200);

    // Manufacturer
    ImGui::Text("| ACS Motion Control");
    ImGui::SameLine(350);

    // Connection status and button
    if (isConnected) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ Connected");
      ImGui::SameLine();
      if (ImGui::Button(("Disconnect##" + std::to_string(i)).c_str())) {
        DisconnectDevice(deviceName);
      }
    }
    else {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "❌ Disconnected");
      ImGui::SameLine();
      if (ImGui::Button(("Connect##" + std::to_string(i)).c_str())) {
        ConnectDevice(deviceName);
      }
    }

    ImGui::PopID();
  }

  if (deviceNames.empty()) {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "⚠️ No ACS gantry controllers configured");
  }
}

void GantryService::RenderSelectedDeviceControl() {
  auto acsManager = GetACSManager();
  auto deviceNames = acsManager->GetDeviceNames();

  if (deviceNames.empty() || selectedDeviceIndex >= (int)deviceNames.size()) {
    return;
  }

  const auto& deviceName = deviceNames[selectedDeviceIndex];
  bool isConnected = acsManager->IsDeviceConnected(deviceName);

  ImGui::Text("🎮 Gantry Control: %s", deviceName.c_str());
  ImGui::Spacing();

  if (!isConnected) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
      "Controller must be connected to access controls");
    return;
  }

  auto device = acsManager->GetDevice(deviceName);
  if (!device) return;

  // Velocity Control
  ImGui::Text("⚡ Axis Velocity:");
  ImGui::SameLine(150);
  if (ImGui::SliderFloat("##velocity", &axisVelocity, 1.0f, 500.0f, "%.1f mm/s")) {
    // Set velocity for all axes when slider changes
    std::vector<std::string> axes = { "X", "Y", "Z" };
    for (const auto& axis : axes) {
      device->SetVelocity(axis, axisVelocity);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Apply##velocity")) {
    std::vector<std::string> axes = { "X", "Y", "Z" };
    for (const auto& axis : axes) {
      device->SetVelocity(axis, axisVelocity);
    }
    std::cout << "Set velocity to " << axisVelocity << " mm/s for " << deviceName << std::endl;
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Position Display
  ImGui::Text("📍 Current Positions:");
  std::vector<std::string> axes = { "X", "Y", "Z" };
  for (const auto& axis : axes) {
    double position = 0.0;
    if (device->GetPosition(axis, position)) {
      ImGui::BulletText("%s: %.3f mm %s",
        axis.c_str(), position,
        device->IsMoving(axis) ? "(Moving)" : "(Stopped)");
    }
    else {
      ImGui::BulletText("%s: N/A", axis.c_str());
    }
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Jog Distance Setting
  ImGui::Text("📏 Jog Distance:");
  ImGui::SameLine(150);
  ImGui::SliderFloat("##jog_distance", &jogDistance, 0.01f, 50.0f, "%.2f mm");

  ImGui::Spacing();

  // XYZ Jog Buttons
  ImGui::Text("🕹️ Gantry Jog Controls:");
  RenderJogButtonGroup("X", device);
  RenderJogButtonGroup("Y", device);
  RenderJogButtonGroup("Z", device);

  ImGui::Spacing();
  ImGui::Separator();

  // Emergency Controls
  ImGui::Text("🚨 Emergency Controls:");
  if (ImGui::Button("⏹️ STOP ALL", ImVec2(120, 30))) {
    device->StopAllAxes();
    std::cout << "Emergency stop executed for " << deviceName << std::endl;
  }

  ImGui::SameLine();
  if (ImGui::Button("🏠 Home All", ImVec2(120, 30))) {
    HomeAllAxes(device);
  }

  ImGui::SameLine();
  if (ImGui::Button("🔧 Enable Servos", ImVec2(120, 30))) {
    std::vector<std::string> axes = { "X", "Y", "Z" };
    for (const auto& axis : axes) {
      device->EnableServo(axis, true);
    }
    std::cout << "Enabled servos for all axes on " << deviceName << std::endl;
  }
}

void GantryService::RenderJogButtonGroup(const std::string& axis, ACSController* device) {
  ImGui::Text("%s:", axis.c_str());
  ImGui::SameLine(30);

  // Negative jog
  if (ImGui::Button((axis + "-").c_str(), ImVec2(40, 25))) {
    device->MoveRelative(axis, -jogDistance);
    std::cout << "Jogging " << axis << " by -" << jogDistance << " mm" << std::endl;
  }

  ImGui::SameLine();

  // Positive jog  
  if (ImGui::Button((axis + "+").c_str(), ImVec2(40, 25))) {
    device->MoveRelative(axis, jogDistance);
    std::cout << "Jogging " << axis << " by +" << jogDistance << " mm" << std::endl;
  }

  ImGui::SameLine();

  // Home button
  if (ImGui::Button(("Home " + axis).c_str(), ImVec2(60, 25))) {
    device->HomeAxis(axis);
    std::cout << "Homing axis " << axis << std::endl;
  }

  ImGui::SameLine();

  // Position display
  double position = 0.0;
  if (device->GetPosition(axis, position)) {
    ImGui::Text("%.3f mm", position);
  }
  else {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "N/A");
  }
}

void GantryService::RenderNoManagerInterface() {
  ImGui::Spacing();
  ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "❌ ACS Manager Not Available");
  ImGui::Spacing();
  ImGui::Text("System Requirements:");
  ImGui::BulletText("ACS controller manager service must be initialized");
  ImGui::BulletText("Check system configuration and restart if needed");
}

// Helper methods
void GantryService::ConnectDevice(const std::string& deviceName) {
  auto acsManager = GetACSManager();
  if (acsManager) {
    if (acsManager->ConnectDevice(deviceName)) {
      std::cout << "Successfully connected to " << deviceName << std::endl;
    }
    else {
      std::cout << "Failed to connect to " << deviceName << std::endl;
    }
  }
}

void GantryService::DisconnectDevice(const std::string& deviceName) {
  auto acsManager = GetACSManager();
  if (acsManager) {
    acsManager->DisconnectDevice(deviceName);
    std::cout << "Disconnected from " << deviceName << std::endl;
  }
}

void GantryService::HomeAllAxes(ACSController* device) {
  std::vector<std::string> axes = { "X", "Y", "Z" };
  for (const auto& axis : axes) {
    device->HomeAxis(axis);
  }
  std::cout << "Homing all gantry axes" << std::endl;
}

// Utility methods
ACSControllerManagerStandardized* GantryService::GetACSManager() {
  return Services.ACS();
}