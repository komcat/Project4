// src/ui/services/manual/PIControlService.cpp

#include "PIControlService.h"
#include "devices/motions/PIController.h"
#include "devices/motions/PIControllerManagerStandardized.h"
#include <map>
#include <vector>

PIControlService::PIControlService() {
  // Constructor - any initialization if needed
}

PIControlService::~PIControlService() {
  // Destructor - cleanup if needed
}

// IUIService interface implementation
void PIControlService::RenderUI() {
  auto piManager = GetPIManager();

  ImGui::SetWindowFontScale(1.5f);
  ImGui::Text("🤖 PI Controllers");
  ImGui::SetWindowFontScale(1.0f);

  ImGui::Spacing();
  ImGui::Text("Precision Motion Control System");
  ImGui::Separator();

  if (piManager) {
    RenderDevicesList();
    ImGui::Separator();
    RenderSelectedDeviceControl();
  }
  else {
    RenderNoManagerInterface();
  }
}

std::string PIControlService::GetServiceName() const {
  return "pi_control";
}

std::string PIControlService::GetDisplayName() const {
  return "PI Controllers";
}

std::string PIControlService::GetCategory() const {
  return "Manual";
}

bool PIControlService::IsAvailable() const {
  return true;
}

// Private rendering methods
void PIControlService::RenderDevicesList() {
  auto piManager = GetPIManager();
  auto deviceNames = piManager->GetDeviceNames();

  ImGui::Text("📋 Available Devices:");
  ImGui::Spacing();

  // Device list with status
  for (size_t i = 0; i < deviceNames.size(); i++) {
    const auto& deviceName = deviceNames[i];
    bool isConnected = piManager->IsDeviceConnected(deviceName);

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
    ImGui::Text("| PI Physik Instrumente");
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
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "⚠️ No PI devices configured");
  }
}

void PIControlService::RenderSelectedDeviceControl() {
  auto piManager = GetPIManager();
  auto deviceNames = piManager->GetDeviceNames();

  if (deviceNames.empty() || selectedDeviceIndex >= (int)deviceNames.size()) {
    return;
  }

  const auto& deviceName = deviceNames[selectedDeviceIndex];
  bool isConnected = piManager->IsDeviceConnected(deviceName);

  ImGui::Text("🎮 Device Control: %s", deviceName.c_str());
  ImGui::Spacing();

  if (!isConnected) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
      "Device must be connected to access controls");
    return;
  }

  auto device = piManager->GetDevice(deviceName);
  if (!device) return;

  // Velocity Control
  ImGui::Text("⚡ System Velocity:");
  ImGui::SameLine(150);
  if (ImGui::SliderFloat("##velocity", &systemVelocity, 0.1f, 100.0f, "%.1f mm/s")) {
    device->SetSystemVelocity(systemVelocity);
  }
  ImGui::SameLine();
  if (ImGui::Button("Apply##velocity")) {
    device->SetSystemVelocity(systemVelocity);
    std::cout << "Set velocity to " << systemVelocity << " mm/s for " << deviceName << std::endl;
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Position Display
  ImGui::Text("📍 Current Positions:");
  std::map<std::string, double> positions;
  if (device->GetPositions(positions)) {
    for (const auto& [axis, pos] : positions) {
      ImGui::BulletText("%s: %.3f mm %s",
        axis.c_str(), pos,
        device->IsMoving(axis) ? "(Moving)" : "(Stopped)");
    }
  }
  else {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "⚠️ Could not read positions");
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Jog Distance Setting
  ImGui::Text("📏 Jog Distance:");
  ImGui::SameLine(150);
  ImGui::SliderFloat("##jog_distance", &jogDistance, 0.01f, 10.0f, "%.2f mm");

  ImGui::Spacing();

  // XYZ Jog Buttons
  ImGui::Text("🕹️ XYZ Jog Controls:");
  RenderJogButtonGroup("X", device);
  RenderJogButtonGroup("Y", device);
  RenderJogButtonGroup("Z", device);

  ImGui::Spacing();

  // UVW Jog Buttons
  ImGui::Text("🔄 UVW Jog Controls:");
  RenderJogButtonGroup("U", device);
  RenderJogButtonGroup("V", device);
  RenderJogButtonGroup("W", device);

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
}

void PIControlService::RenderJogButtonGroup(const std::string& axis, PIController* device) {
  ImGui::Text("%s:", axis.c_str());
  ImGui::SameLine(30);

  // Negative jog
  if (ImGui::Button((axis + "-").c_str(), ImVec2(40, 25))) {
    device->MoveRelative(axis, -jogDistance, false);
    std::cout << "Jogging " << axis << " by -" << jogDistance << " mm" << std::endl;
  }

  ImGui::SameLine();

  // Positive jog  
  if (ImGui::Button((axis + "+").c_str(), ImVec2(40, 25))) {
    device->MoveRelative(axis, jogDistance, false);
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

void PIControlService::RenderNoManagerInterface() {
  ImGui::Spacing();
  ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "❌ PI Manager Not Available");
  ImGui::Spacing();
  ImGui::Text("System Requirements:");
  ImGui::BulletText("PI controller manager service must be initialized");
  ImGui::BulletText("Check system configuration and restart if needed");
}

// Helper methods
void PIControlService::ConnectDevice(const std::string& deviceName) {
  auto piManager = GetPIManager();
  if (piManager) {
    if (piManager->ConnectDevice(deviceName)) {
      std::cout << "Successfully connected to " << deviceName << std::endl;
    }
    else {
      std::cout << "Failed to connect to " << deviceName << std::endl;
    }
  }
}

void PIControlService::DisconnectDevice(const std::string& deviceName) {
  auto piManager = GetPIManager();
  if (piManager) {
    piManager->DisconnectDevice(deviceName);
    std::cout << "Disconnected from " << deviceName << std::endl;
  }
}

void PIControlService::HomeAllAxes(PIController* device) {
  std::vector<std::string> axes = { "X", "Y", "Z", "U", "V", "W" };
  for (const auto& axis : axes) {
    device->HomeAxis(axis);
  }
  std::cout << "Homing all axes" << std::endl;
}

// Utility methods
PIControllerManagerStandardized* PIControlService::GetPIManager() {
  return Services.PI();
}