#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

// Forward declare logger interface - keep it simple
class ILogger;

/**
 * Centralized Configuration Manager
 *
 * Simple, unified access to all JSON configuration files.
 * Handles loading, saving, and caching of configurations.
 *
 * Usage:
 *   auto& config = ConfigManager::Instance();
 *   config.LoadConfig("camera_config.json");
 *   auto data = config.GetConfig("camera_config.json");
 */
class ConfigManager {
public:
  // Singleton pattern - simple access
  static ConfigManager& Instance();

  // Core operations
  bool LoadConfig(const std::string& filename);
  bool SaveConfig(const std::string& filename);
  bool SaveConfig(const std::string& filename, const nlohmann::json& data);

  // Data access
  nlohmann::json GetConfig(const std::string& filename);
  void SetConfig(const std::string& filename, const nlohmann::json& data);

  // Convenience methods
  bool HasConfig(const std::string& filename) const;
  void ClearCache();
  void SetLogger(ILogger* logger);

  // Bulk operations
  std::vector<std::string> LoadAllConfigs();
  void SaveAllConfigs();

  // Config validation
  bool ValidateConfig(const std::string& filename);

  // Path management
  void SetConfigDirectory(const std::string& path);
  std::string GetConfigDirectory() const;

private:
  ConfigManager() = default;
  ~ConfigManager() = default;
  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  // Internal data
  std::unordered_map<std::string, nlohmann::json> m_configCache;
  std::string m_configDirectory = "config";
  ILogger* m_logger = nullptr;

  // Helper methods
  std::string GetFullPath(const std::string& filename) const;
  void LogInfo(const std::string& message) const;
  void LogError(const std::string& message) const;
  void LogWarning(const std::string& message) const;
};

/**
 * Configuration Helper Templates
 *
 * Type-safe configuration access with default values
 */
namespace ConfigHelper {

  template<typename T>
  T GetValue(const nlohmann::json& config, const std::string& key, const T& defaultValue) {
    try {
      if (config.contains(key)) {
        return config[key].get<T>();
      }
    }
    catch (const std::exception&) {
      // Fallback to default on any conversion error
    }
    return defaultValue;
  }

  template<typename T>
  T GetNestedValue(const nlohmann::json& config, const std::vector<std::string>& keys, const T& defaultValue) {
    try {
      nlohmann::json current = config;
      for (const auto& key : keys) {
        if (!current.contains(key)) {
          return defaultValue;
        }
        current = current[key];
      }
      return current.get<T>();
    }
    catch (const std::exception&) {
      return defaultValue;
    }
  }

  // Convenience macro for nested access
#define GET_CONFIG_VALUE(config, path, type, default) \
        ConfigHelper::GetNestedValue<type>(config, path, default)
}

/**
 * Simple logger interface for ConfigManager
 */
class ILogger {
public:
  virtual ~ILogger() = default;
  virtual void LogInfo(const std::string& message) = 0;
  virtual void LogError(const std::string& message) = 0;
  virtual void LogWarning(const std::string& message) = 0;
};