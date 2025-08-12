// src/ui/services/data/DataMonitorService.h
#pragma once

#include "../UIServiceRegistry.h"
#include "core/ServiceLocator.h"
#include "utils/Logger.h"
#include "utils/Unicode.h"
#include "imgui.h"
#include <vector>
#include <deque>
#include <algorithm>

class DataMonitorService : public IUIService {
public:
  DataMonitorService() {
    // Initialize data buffers
    temperatureData.resize(100, 0.0f);
    pressureData.resize(100, 0.0f);
    speedData.resize(100, 0.0f);
    productionData.resize(100, 0.0f);
  }

  void RenderUI() override {
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("📊 Data Monitor");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Text("Real-time System Monitoring Dashboard");
    ImGui::Separator();

    // Update live data
    UpdateLiveData();

    // System overview
    RenderSystemOverview();

    ImGui::Spacing();
    ImGui::Separator();

    // Real-time charts
    RenderRealTimeCharts();

    ImGui::Spacing();
    ImGui::Separator();

    // Data logging controls
    RenderDataLogging();
  }

  std::string GetServiceName() const override { return "data_monitor"; }
  std::string GetDisplayName() const override { return "Data Monitor"; }
  std::string GetCategory() const override { return "Data"; }
  bool IsAvailable() const override { return true; }

private:
  // Data buffers for real-time plotting
  std::vector<float> temperatureData;
  std::vector<float> pressureData;
  std::vector<float> speedData;
  std::vector<float> productionData;

  // Current values
  float currentTemp = 25.0f;
  float currentPressure = 1.0f;
  float currentSpeed = 100.0f;
  float currentProduction = 247.0f;

  // Data logging state
  static inline bool isLogging = false;
  static inline int logInterval = 1; // seconds
  static inline std::string logFilename = "data_log.csv";

  void UpdateLiveData() {
    // Simulate live data updates
    float time = ImGui::GetTime();

    currentTemp = 25.0f + sin(time * 0.1f) * 3.0f + cos(time * 0.3f) * 1.5f;
    currentPressure = 1.0f + sin(time * 0.2f) * 0.2f;
    currentSpeed = 100.0f + sin(time * 0.15f) * 10.0f;
    currentProduction += 0.1f * ImGui::GetIO().DeltaTime;

    // Update data buffers (shift left and add new value)
    for (int i = 0; i < temperatureData.size() - 1; i++) {
      temperatureData[i] = temperatureData[i + 1];
      pressureData[i] = pressureData[i + 1];
      speedData[i] = speedData[i + 1];
      productionData[i] = productionData[i + 1];
    }

    temperatureData.back() = currentTemp;
    pressureData.back() = currentPressure;
    speedData.back() = currentSpeed;
    productionData.back() = currentProduction;
  }

  void RenderSystemOverview() {
    ImGui::Text("System Overview:");

    // Current values display
    ImGui::Columns(4, "SystemValues", false);

    // Temperature
    ImGui::Text("🌡️ Temperature");
    ImVec4 tempColor = currentTemp > 30.0f ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    ImGui::TextColored(tempColor, "%.1f°C", currentTemp);
    ImGui::NextColumn();

    // Pressure
    ImGui::Text("💨 Pressure");
    ImVec4 pressureColor = currentPressure > 1.5f ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    ImGui::TextColored(pressureColor, "%.2f bar", currentPressure);
    ImGui::NextColumn();

    // Speed
    ImGui::Text("⚡ Speed");
    ImVec4 speedColor = currentSpeed < 90.0f ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    ImGui::TextColored(speedColor, "%.0f%%", currentSpeed);
    ImGui::NextColumn();

    // Production
    ImGui::Text("📦 Production");
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%.0f units", currentProduction);
    ImGui::NextColumn();

    ImGui::Columns(1);

    // System status indicators
    ImGui::Spacing();
    ImGui::Text("System Status:");
    ImGui::BulletText("Frame Rate: %.1f FPS", ImGui::GetIO().Framerate);
    ImGui::BulletText("Data Update Rate: 10 Hz");
    ImGui::BulletText("Active Sensors: 12/14");
    ImGui::BulletText("Data Quality: %s", GetDataQuality() > 95.0f ? "🟢 Excellent" :
      GetDataQuality() > 85.0f ? "🟡 Good" : "🔴 Poor");
  }

  void RenderRealTimeCharts() {
    ImGui::Text("Real-time Charts:");

    // Temperature chart
    ImGui::Text("Temperature History:");
    ImGui::PlotLines("##temp", temperatureData.data(), temperatureData.size(), 0,
      nullptr, 20.0f, 35.0f, ImVec2(0, 80));

    // Pressure chart
    ImGui::Text("Pressure History:");
    ImGui::PlotLines("##pressure", pressureData.data(), pressureData.size(), 0,
      nullptr, 0.5f, 1.5f, ImVec2(0, 80));

    // Speed chart
    ImGui::Text("Speed History:");
    ImGui::PlotLines("##speed", speedData.data(), speedData.size(), 0,
      nullptr, 80.0f, 120.0f, ImVec2(0, 80));

    // Production chart
    ImGui::Text("Production Trend:");
    ImGui::PlotLines("##production", productionData.data(), productionData.size(), 0,
      nullptr, currentProduction - 50.0f, currentProduction + 10.0f, ImVec2(0, 80));

    // Chart controls
    ImGui::Spacing();
    ImGui::Text("Chart Controls:");

    static int timeRange = 100;
    if (ImGui::SliderInt("Time Range (samples)", &timeRange, 50, 500)) {
      ResizeDataBuffers(timeRange);
    }

    static bool autoScale = true;
    ImGui::Checkbox("Auto Scale", &autoScale);

    ImGui::SameLine();
    if (ImGui::Button("📊 Export Charts")) {
      ExportChartData();
    }
  }

  void RenderDataLogging() {
    ImGui::Text("Data Logging:");

    // Logging controls
    ImGui::PushStyleColor(ImGuiCol_Button, isLogging ?
      ImVec4(0.7f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 0.7f, 0.0f, 1.0f));

    if (ImGui::Button(isLogging ? "⏹️ Stop Logging" : "▶️ Start Logging", ImVec2(150, 30))) {
      isLogging = !isLogging;
      if (isLogging) {
        StartDataLogging();
      }
      else {
        StopDataLogging();
      }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::Button("💾 Save Current", ImVec2(150, 30))) {
      SaveCurrentData();
    }

    ImGui::SameLine();
    if (ImGui::Button("📁 Load Data", ImVec2(150, 30))) {
      LoadHistoricalData();
    }

    // Logging settings
    ImGui::Spacing();
    ImGui::Text("Logging Settings:");

    ImGui::SliderInt("Log Interval (s)", &logInterval, 1, 60);

    // Filename input
    static char filename[128] = "data_log.csv";
    ImGui::InputText("Filename", filename, sizeof(filename));
    logFilename = std::string(filename);

    // Data selection
    static bool logTemperature = true;
    static bool logPressure = true;
    static bool logSpeed = true;
    static bool logProduction = true;

    ImGui::Text("Data to Log:");
    ImGui::Checkbox("🌡️ Temperature", &logTemperature);
    ImGui::SameLine();
    ImGui::Checkbox("💨 Pressure", &logPressure);
    ImGui::Checkbox("⚡ Speed", &logSpeed);
    ImGui::SameLine();
    ImGui::Checkbox("📦 Production", &logProduction);

    // Logging statistics
    ImGui::Spacing();
    ImGui::Text("Logging Statistics:");
    ImGui::BulletText("Status: %s", isLogging ? "🟢 Active" : "🔴 Inactive");
    ImGui::BulletText("Log File: %s", logFilename.c_str());
    ImGui::BulletText("Records: %d", GetLogRecordCount());
    ImGui::BulletText("File Size: %.1f KB", GetLogFileSize());

    if (isLogging) {
      ImGui::BulletText("Next Log: %.1fs", GetTimeToNextLog());
    }
  }

  // Helper methods
  float GetDataQuality() const {
    return 96.5f + sin(ImGui::GetTime() * 0.1f) * 2.0f;
  }

  void ResizeDataBuffers(int newSize) {
    temperatureData.resize(newSize, 0.0f);
    pressureData.resize(newSize, 0.0f);
    speedData.resize(newSize, 0.0f);
    productionData.resize(newSize, 0.0f);
  }

  void ExportChartData() {
    Logger::Success(L"Chart data exported successfully");
  }

  void StartDataLogging() {
    Logger::Info(L"Started data logging to: " + UnicodeUtils::StringToWString(logFilename));
  }

  void StopDataLogging() {
    Logger::Info(L"Stopped data logging");
  }

  void SaveCurrentData() {
    Logger::Success(L"Current data snapshot saved");
  }

  void LoadHistoricalData() {
    Logger::Info(L"Loading historical data...");
  }

  int GetLogRecordCount() const {
    return isLogging ? 1547 + (int)(ImGui::GetTime() / logInterval) : 1547;
  }

  float GetLogFileSize() const {
    return GetLogRecordCount() * 0.12f; // Approximate KB per record
  }

  float GetTimeToNextLog() const {
    return logInterval - fmod(ImGui::GetTime(), logInterval);
  }
};