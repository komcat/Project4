
#pragma once
#include "../UIServiceRegistry.h"
#include "core/ServiceLocator.h"
#include "MotionConfigManager.h"
#include "ConfigManager.h"
#include "utils/Logger.h"
#include "utils/Unicode.h"
#include "imgui.h"

class ConfigService : public IUIService {
public:
  void RenderUI() override {
    auto config = Services.Config();
    if (config) {
      auto loadedConfigs = config->GetLoadedConfigs();

      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🔧"));
      ImGui::SameLine();
      ImGui::Text("Config: %d JSON files loaded", static_cast<int>(loadedConfigs.size()));

      if (!loadedConfigs.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("(includes %s%s)",
          loadedConfigs[0].c_str(),
          loadedConfigs.size() > 1 ? ", ..." : "");
      }
    }
    else {
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🔧"));
      ImGui::SameLine();
      ImGui::Text("Config: Not loaded");
    }

    auto motionConfig = Services.MotionConfig();
    if (motionConfig) {
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"📋"));
      ImGui::SameLine();
      ImGui::Text("Motion Config: Ready");

      // Show enabled devices
      auto enabledDevices = motionConfig->GetEnabledDevices();
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"  🤖"));
      ImGui::SameLine();
      ImGui::Text("Devices: %d enabled", static_cast<int>(enabledDevices.size()));
      if (!enabledDevices.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("(%s%s)",
          enabledDevices[0].c_str(),
          enabledDevices.size() > 1 ? ", ..." : "");
      }

      // Show available graphs
      auto graphs = motionConfig->GetAvailableGraphs();
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"  📊"));
      ImGui::SameLine();
      ImGui::Text("Graphs: %d loaded", static_cast<int>(graphs.size()));
      if (!graphs.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("(%s%s)",
          graphs[0].c_str(),
          graphs.size() > 1 ? ", ..." : "");
      }

      // Show settings
      const auto& settings = motionConfig->GetSettings();
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"  ⚙️"));
      ImGui::SameLine();
      ImGui::Text("Settings: Speed=%.1fmm/s, Tolerance=%.3fmm",
        settings.DefaultSpeed,
        settings.PositionTolerance);
    }
    else {
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"📋"));
      ImGui::SameLine();
      ImGui::Text("Motion Config: Not loaded");
    }
  }

  std::string GetServiceName() const override { return "system_config"; }
  std::string GetDisplayName() const override { return "System Configuration"; }
  std::string GetCategory() const override { return "Config"; }
  bool IsAvailable() const override { return true; }
};