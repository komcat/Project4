// MotionConfigManager.cpp - Simplified implementation using ConfigManager
#include "MotionConfigManager.h"
#include "ServiceLocator.h"
#include "ConfigManager.h"
#include <iostream>




// Singleton instance
MotionConfigManager& MotionConfigManager::Instance() {
  static MotionConfigManager instance;
  return instance;
}

// Initialize - just validate configs are accessible
bool MotionConfigManager::Initialize() {
  try {
    // Get ConfigManager instance through ServiceLocator
    auto* configManager = ServiceLocator::Instance().Config();
    if (!configManager) {
      std::cerr << "MotionConfigManager: ConfigManager not available in ServiceLocator" << std::endl;
      return false;
    }

    // Load all motion config files through ConfigManager
    bool devicesLoaded = configManager->LoadConfig(CONFIG_DEVICES);
    bool positionsLoaded = configManager->LoadConfig(CONFIG_POSITIONS);
    bool graphLoaded = configManager->LoadConfig(CONFIG_GRAPH);

    if (!devicesLoaded || !positionsLoaded || !graphLoaded) {
      std::cerr << "MotionConfigManager: Failed to load one or more config files" << std::endl;
      std::cerr << "  Devices: " << (devicesLoaded ? "OK" : "FAILED") << std::endl;
      std::cerr << "  Positions: " << (positionsLoaded ? "OK" : "FAILED") << std::endl;
      std::cerr << "  Graph: " << (graphLoaded ? "OK" : "FAILED") << std::endl;
      return false;
    }

    // Parse all configurations
    ParseDevices();
    ParsePositions();
    ParseGraphs();
    ParseSettings();

    std::cout << "MotionConfigManager: Initialized successfully" << std::endl;
    std::cout << "  Devices: " << m_devices.size() << std::endl;
    std::cout << "  Graphs: " << m_graphs.size() << std::endl;

    return true;
  }
  catch (const std::exception& e) {
    std::cerr << "MotionConfigManager: Exception during initialization: " << e.what() << std::endl;
    return false;
  }
}

// Helper to get JSON from ConfigManager
nlohmann::json MotionConfigManager::GetConfigJSON(const std::string& filename) {
  auto* configManager = ServiceLocator::Instance().Config();
  if (!configManager) {
    return nlohmann::json();
  }
  return configManager->GetConfig(filename);
}

// Helper to save JSON through ConfigManager
bool MotionConfigManager::SaveConfigJSON(const std::string& filename, const nlohmann::json& data) {
  auto* configManager = ServiceLocator::Instance().Config();
  if (!configManager) {
    return false;
  }
  configManager->SetConfig(filename, data);
  return configManager->SaveConfig(filename);
}

// Parse devices from JSON
void MotionConfigManager::ParseDevices() {
  m_devices.clear();

  auto config = GetConfigJSON(CONFIG_DEVICES);
  if (config.empty() || !config.contains("MotionDevices")) {
    return;
  }

  for (const auto& [name, deviceJson] : config["MotionDevices"].items()) {
    MotionDevice device;
    device.Name = name;
    device.IsEnabled = ConfigHelper::GetValue<bool>(deviceJson, "IsEnabled", false);
    device.IpAddress = ConfigHelper::GetValue<std::string>(deviceJson, "IpAddress", "");
    device.Port = ConfigHelper::GetValue<int>(deviceJson, "Port", 0);
    device.Id = ConfigHelper::GetValue<int>(deviceJson, "Id", 0);
    device.TypeController = ConfigHelper::GetValue<std::string>(deviceJson, "typeController", "PI");
    device.InstalledAxes = ConfigHelper::GetValue<std::string>(deviceJson, "installAxes", "XYZUVW");

    m_devices[name] = device;
  }
}

// Parse positions from JSON
void MotionConfigManager::ParsePositions() {
  m_positions.clear();

  auto config = GetConfigJSON(CONFIG_POSITIONS);
  if (config.empty()) {
    return;
  }

  // Each top-level key is a device name
  for (const auto& [deviceName, positions] : config.items()) {
    if (!positions.is_object()) continue;

    // Each sub-key is a position name
    for (const auto& [posName, posData] : positions.items()) {
      PositionStruct pos;
      pos.x = ConfigHelper::GetValue<double>(posData, "x", 0.0);
      pos.y = ConfigHelper::GetValue<double>(posData, "y", 0.0);
      pos.z = ConfigHelper::GetValue<double>(posData, "z", 0.0);
      pos.u = ConfigHelper::GetValue<double>(posData, "u", 0.0);
      pos.v = ConfigHelper::GetValue<double>(posData, "v", 0.0);
      pos.w = ConfigHelper::GetValue<double>(posData, "w", 0.0);

      m_positions[deviceName][posName] = pos;
    }
  }
}

// Parse graphs from JSON
void MotionConfigManager::ParseGraphs() {
  m_graphs.clear();

  auto config = GetConfigJSON(CONFIG_GRAPH);
  if (config.empty() || !config.contains("Graphs")) {
    return;
  }

  for (const auto& [graphName, graphData] : config["Graphs"].items()) {
    Graph graph;

    // Parse nodes
    if (graphData.contains("Nodes") && graphData["Nodes"].is_array()) {
      for (const auto& nodeData : graphData["Nodes"]) {
        Node node;
        node.Id = ConfigHelper::GetValue<std::string>(nodeData, "Id", "");
        node.Label = ConfigHelper::GetValue<std::string>(nodeData, "Label", "");
        node.Device = ConfigHelper::GetValue<std::string>(nodeData, "Device", "");
        node.Position = ConfigHelper::GetValue<std::string>(nodeData, "Position", "");
        node.X = ConfigHelper::GetValue<int>(nodeData, "X", 0);
        node.Y = ConfigHelper::GetValue<int>(nodeData, "Y", 0);
        graph.Nodes.push_back(node);
      }
    }

    // Parse edges
    if (graphData.contains("Edges") && graphData["Edges"].is_array()) {
      for (const auto& edgeData : graphData["Edges"]) {
        Edge edge;
        edge.Id = ConfigHelper::GetValue<std::string>(edgeData, "Id", "");
        edge.Source = ConfigHelper::GetValue<std::string>(edgeData, "Source", "");
        edge.Target = ConfigHelper::GetValue<std::string>(edgeData, "Target", "");
        edge.Label = ConfigHelper::GetValue<std::string>(edgeData, "Label", "");

        if (edgeData.contains("Conditions")) {
          const auto& conditions = edgeData["Conditions"];
          edge.Conditions.RequiresOperatorApproval =
            ConfigHelper::GetValue<bool>(conditions, "RequiresOperatorApproval", false);
          edge.Conditions.TimeoutSeconds =
            ConfigHelper::GetValue<int>(conditions, "TimeoutSeconds", 0);
          edge.Conditions.IsBidirectional =
            ConfigHelper::GetValue<bool>(conditions, "IsBidirectional", false);
        }

        graph.Edges.push_back(edge);
      }
    }

    m_graphs[graphName] = graph;
  }
}

// Parse settings from devices config
void MotionConfigManager::ParseSettings() {
  auto config = GetConfigJSON(CONFIG_DEVICES);

  if (config.contains("Settings")) {
    const auto& settings = config["Settings"];
    m_settings.DefaultSpeed = ConfigHelper::GetValue<double>(settings, "DefaultSpeed", 10.0);
    m_settings.DefaultAcceleration = ConfigHelper::GetValue<double>(settings, "DefaultAcceleration", 5.0);
    m_settings.LogLevel = ConfigHelper::GetValue<std::string>(settings, "LogLevel", "info");
    m_settings.AutoReconnect = ConfigHelper::GetValue<bool>(settings, "AutoReconnect", true);
    m_settings.ConnectionTimeout = ConfigHelper::GetValue<int>(settings, "ConnectionTimeout", 5000);
    m_settings.PositionTolerance = ConfigHelper::GetValue<double>(settings, "PositionTolerance", 0.001);
  }
}

// Get device by name
std::optional<std::reference_wrapper<const MotionDevice>>
MotionConfigManager::GetDevice(const std::string& deviceName) {
  auto it = m_devices.find(deviceName);
  if (it != m_devices.end()) {
    return std::ref(it->second);
  }
  return std::nullopt;
}

// Get enabled devices
std::vector<std::string> MotionConfigManager::GetEnabledDevices() const {
  std::vector<std::string> enabled;
  for (const auto& [name, device] : m_devices) {
    if (device.IsEnabled) {
      enabled.push_back(name);
    }
  }
  return enabled;
}

// Check if device is enabled
bool MotionConfigManager::IsDeviceEnabled(const std::string& deviceName) const {
  auto it = m_devices.find(deviceName);
  return (it != m_devices.end()) && it->second.IsEnabled;
}

// Get named position
std::optional<std::reference_wrapper<const PositionStruct>>
MotionConfigManager::GetNamedPosition(const std::string& deviceName, const std::string& positionName) {
  auto deviceIt = m_positions.find(deviceName);
  if (deviceIt != m_positions.end()) {
    auto posIt = deviceIt->second.find(positionName);
    if (posIt != deviceIt->second.end()) {
      return std::ref(posIt->second);
    }
  }
  return std::nullopt;
}

// Get position names for device
std::vector<std::string> MotionConfigManager::GetPositionNames(const std::string& deviceName) const {
  std::vector<std::string> names;
  auto it = m_positions.find(deviceName);
  if (it != m_positions.end()) {
    for (const auto& [name, _] : it->second) {
      names.push_back(name);
    }
  }
  return names;
}

// Add/update position
void MotionConfigManager::AddPosition(const std::string& deviceName,
  const std::string& positionName,
  const PositionStruct& position) {
  m_positions[deviceName][positionName] = position;
}

// Get graph by name
std::optional<std::reference_wrapper<const Graph>>
MotionConfigManager::GetGraph(const std::string& graphName) {
  auto it = m_graphs.find(graphName);
  if (it != m_graphs.end()) {
    return std::ref(it->second);
  }
  return std::nullopt;
}

// Get available graph names
std::vector<std::string> MotionConfigManager::GetAvailableGraphs() const {
  std::vector<std::string> names;
  for (const auto& [name, _] : m_graphs) {
    names.push_back(name);
  }
  return names;
}

// Save all configs back through ConfigManager
bool MotionConfigManager::SaveConfig() {
  try {
    // Convert devices back to JSON
    nlohmann::json devicesJson;
    devicesJson["MotionDevices"] = nlohmann::json::object();

    for (const auto& [name, device] : m_devices) {
      nlohmann::json deviceJson;
      deviceJson["IsEnabled"] = device.IsEnabled;
      deviceJson["IpAddress"] = device.IpAddress;
      deviceJson["Port"] = device.Port;
      deviceJson["Id"] = device.Id;
      deviceJson["typeController"] = device.TypeController;
      deviceJson["installAxes"] = device.InstalledAxes;
      devicesJson["MotionDevices"][name] = deviceJson;
    }

    // Add settings to devices config
    devicesJson["Settings"] = {
        {"DefaultSpeed", m_settings.DefaultSpeed},
        {"DefaultAcceleration", m_settings.DefaultAcceleration},
        {"LogLevel", m_settings.LogLevel},
        {"AutoReconnect", m_settings.AutoReconnect},
        {"ConnectionTimeout", m_settings.ConnectionTimeout},
        {"PositionTolerance", m_settings.PositionTolerance}
    };

    // Convert positions back to JSON
    nlohmann::json positionsJson;
    for (const auto& [deviceName, positions] : m_positions) {
      positionsJson[deviceName] = nlohmann::json::object();
      for (const auto& [posName, pos] : positions) {
        positionsJson[deviceName][posName] = {
            {"x", pos.x},
            {"y", pos.y},
            {"z", pos.z},
            {"u", pos.u},
            {"v", pos.v},
            {"w", pos.w}
        };
      }
    }

    // Save through ConfigManager
    bool success = true;
    success &= SaveConfigJSON(CONFIG_DEVICES, devicesJson);
    success &= SaveConfigJSON(CONFIG_POSITIONS, positionsJson);
    // Note: Graph config typically doesn't change at runtime, but could save if needed

    return success;
  }
  catch (const std::exception& e) {
    std::cerr << "MotionConfigManager: Failed to save config: " << e.what() << std::endl;
    return false;
  }
}

// Reload configs from ConfigManager
bool MotionConfigManager::ReloadConfig() {
  try {
    // Re-parse all configurations
    ParseDevices();
    ParsePositions();
    ParseGraphs();
    ParseSettings();

    std::cout << "MotionConfigManager: Reloaded configuration" << std::endl;
    return true;
  }
  catch (const std::exception& e) {
    std::cerr << "MotionConfigManager: Failed to reload config: " << e.what() << std::endl;
    return false;
  }
}