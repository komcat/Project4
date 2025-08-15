#include "ConfigManager.h"

// Singleton instance
ConfigManager& ConfigManager::Instance() {
  static ConfigManager instance;
  return instance;
}

// Load configuration from file
bool ConfigManager::LoadConfig(const std::string& filename) {
  try {
    std::string fullPath = GetFullPath(filename);

    if (!std::filesystem::exists(fullPath)) {
      LogError("Config file not found: " + fullPath);
      return false;
    }

    std::ifstream file(fullPath);
    if (!file.is_open()) {
      LogError("Failed to open config file: " + fullPath);
      return false;
    }

    nlohmann::json config;
    file >> config;

    // Cache the loaded config
    m_configCache[filename] = config;

    LogInfo("Loaded config: " + filename);
    return true;

  }
  catch (const std::exception& e) {
    LogError("Failed to load config " + filename + ": " + e.what());
    return false;
  }
}

// Save configuration to file
bool ConfigManager::SaveConfig(const std::string& filename) {
  auto it = m_configCache.find(filename);
  if (it == m_configCache.end()) {
    LogError("Config not found in cache: " + filename);
    return false;
  }

  return SaveConfig(filename, it->second);
}

// Save configuration with data
bool ConfigManager::SaveConfig(const std::string& filename, const nlohmann::json& data) {
  try {
    std::string fullPath = GetFullPath(filename);

    // Ensure directory exists
    std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());

    std::ofstream file(fullPath);
    if (!file.is_open()) {
      LogError("Failed to create config file: " + fullPath);
      return false;
    }

    // Pretty print with 2-space indentation
    file << data.dump(2);

    // Update cache
    m_configCache[filename] = data;

    LogInfo("Saved config: " + filename);
    return true;

  }
  catch (const std::exception& e) {
    LogError("Failed to save config " + filename + ": " + e.what());
    return false;
  }
}

// Get configuration data
nlohmann::json ConfigManager::GetConfig(const std::string& filename) {
  auto it = m_configCache.find(filename);
  if (it != m_configCache.end()) {
    return it->second;
  }

  // Try to load if not in cache
  if (LoadConfig(filename)) {
    return m_configCache[filename];
  }

  // Return empty JSON on failure
  LogWarning("Returning empty JSON for config: " + filename);
  return nlohmann::json{};
}

// Set configuration data
void ConfigManager::SetConfig(const std::string& filename, const nlohmann::json& data) {
  m_configCache[filename] = data;
  LogInfo("Config updated in cache: " + filename);
}

// Check if config exists in cache
bool ConfigManager::HasConfig(const std::string& filename) const {
  return m_configCache.find(filename) != m_configCache.end();
}

// Clear all cached configurations
void ConfigManager::ClearCache() {
  m_configCache.clear();
  LogInfo("Configuration cache cleared");
}

// Set logger interface
void ConfigManager::SetLogger(ILogger* logger) {
  m_logger = logger;
}

// Load all configuration files from directory
std::vector<std::string> ConfigManager::LoadAllConfigs() {
  std::vector<std::string> loadedFiles;

  try {
    if (!std::filesystem::exists(m_configDirectory)) {
      LogWarning("Config directory does not exist: " + m_configDirectory);
      return loadedFiles;
    }

    LogInfo("Loading all configs from: " + m_configDirectory);

    for (const auto& entry : std::filesystem::directory_iterator(m_configDirectory)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        std::string filename = entry.path().filename().string();

        if (LoadConfig(filename)) {
          loadedFiles.push_back(filename);
        }
      }
    }

    LogInfo("Loaded " + std::to_string(loadedFiles.size()) + " configuration files");

  }
  catch (const std::exception& e) {
    LogError("Failed to load all configs: " + std::string(e.what()));
  }

  return loadedFiles;
}

// Save all cached configurations
void ConfigManager::SaveAllConfigs() {
  LogInfo("Saving all cached configurations");

  int savedCount = 0;
  for (const auto& [filename, data] : m_configCache) {
    if (SaveConfig(filename, data)) {
      savedCount++;
    }
  }

  LogInfo("Saved " + std::to_string(savedCount) + " configuration files");
}

// Validate configuration file
bool ConfigManager::ValidateConfig(const std::string& filename) {
  try {
    std::string fullPath = GetFullPath(filename);

    if (!std::filesystem::exists(fullPath)) {
      return false;
    }

    std::ifstream file(fullPath);
    if (!file.is_open()) {
      return false;
    }

    nlohmann::json config;
    file >> config;

    // Basic validation - check if it's valid JSON
    return !config.is_null();

  }
  catch (const std::exception&) {
    return false;
  }
}

// Set configuration directory
void ConfigManager::SetConfigDirectory(const std::string& path) {
  m_configDirectory = path;
  LogInfo("Config directory set to: " + path);
}

// Get configuration directory
std::string ConfigManager::GetConfigDirectory() const {
  return m_configDirectory;
}

// Get full file path
std::string ConfigManager::GetFullPath(const std::string& filename) const {
  return (std::filesystem::path(m_configDirectory) / filename).string();
}

// Logging helpers
void ConfigManager::LogInfo(const std::string& message) const {
  if (m_logger) {
    m_logger->LogInfo("[ConfigManager] " + message);
  }
  else {
    std::cout << "[ConfigManager INFO] " + message << std::endl;
  }
}

void ConfigManager::LogError(const std::string& message) const {
  if (m_logger) {
    m_logger->LogError("[ConfigManager] " + message);
  }
  else {
    std::cerr << "[ConfigManager ERROR] " + message << std::endl;
  }
}

void ConfigManager::LogWarning(const std::string& message) const {
  if (m_logger) {
    m_logger->LogWarning("[ConfigManager] " + message);
  }
  else {
    std::cout << "[ConfigManager WARNING] " + message << std::endl;
  }
}