// MotionConfigManager.h - Simplified version using centralized ConfigManager
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include "MotionTypes.h"

// Forward declarations
class ServiceLocator;

/**
 * Simplified Motion Configuration Manager
 *
 * This class is now a thin wrapper that:
 * 1. Uses centralized ConfigManager for all file I/O
 * 2. Focuses only on parsing/interpreting motion-specific JSON data
 * 3. Provides type-safe access to motion configurations
 *
 * KISS principle: Keep It Simple, Stupid
 */
class MotionConfigManager {
public:
  //MotionConfigManager() = default;
  //~MotionConfigManager() = default;

  // Singleton pattern - simple access
  static MotionConfigManager& Instance();

  // Initialize - just validate that configs are loadable
  bool Initialize();

  // === DEVICE ACCESS (from motion_config_devices.json) ===
  std::optional<std::reference_wrapper<const MotionDevice>> GetDevice(const std::string& deviceName);
  std::vector<std::string> GetEnabledDevices() const;
  bool IsDeviceEnabled(const std::string& deviceName) const;

  // === POSITION ACCESS (from motion_config_positions.json) ===
  std::optional<std::reference_wrapper<const PositionStruct>> GetNamedPosition(
    const std::string& deviceName,
    const std::string& positionName);

  std::vector<std::string> GetPositionNames(const std::string& deviceName) const;
  void AddPosition(const std::string& deviceName,
    const std::string& positionName,
    const PositionStruct& position);

  // === GRAPH ACCESS (from motion_config_graph.json) ===
  std::optional<std::reference_wrapper<const Graph>> GetGraph(const std::string& graphName);
  std::vector<std::string> GetAvailableGraphs() const;

  // === SETTINGS ACCESS ===
  const Settings& GetSettings() const { return m_settings; }

  // === SAVE/RELOAD using ConfigManager ===
  bool SaveConfig();    // Save all changes back through ConfigManager
  bool ReloadConfig();  // Reload and reparse from ConfigManager

private:
  // Parse JSON data from ConfigManager into our typed structures
  void ParseDevices();
  void ParsePositions();
  void ParseGraphs();
  void ParseSettings();

  // Helper to get JSON from ConfigManager
  nlohmann::json GetConfigJSON(const std::string& filename);
  bool SaveConfigJSON(const std::string& filename, const nlohmann::json& data);

  // Cached parsed data
  std::map<std::string, MotionDevice> m_devices;
  std::map<std::string, std::map<std::string, PositionStruct>> m_positions;
  std::map<std::string, Graph> m_graphs;
  Settings m_settings;

  // Config file names (constants)
  static constexpr const char* CONFIG_DEVICES = "motion_config_devices.json";
  static constexpr const char* CONFIG_POSITIONS = "motion_config_positions.json";
  static constexpr const char* CONFIG_GRAPH = "motion_config_graph.json";
};