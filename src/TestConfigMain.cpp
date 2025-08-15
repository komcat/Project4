// TestConfigMain.cpp
// Simple test for ConfigManager with motion configuration files
#include "core/ConfigManager.h"
#include "core/ConfigRegistry.h"
#include "utils/LoggerAdapter.h"  // Use your emoji logger!
#include <iostream>
#include <memory>
#include <chrono>

int main() {
  ConfigLogger::ConfigTestStart();

  try {
    // === SETUP ===
    auto loggerAdapter = std::make_unique<LoggerAdapter>();

    // Initialize ConfigManager
    auto& configManager = ConfigManager::Instance();
    configManager.SetLogger(loggerAdapter.get());
    configManager.SetConfigDirectory("config");

    Logger::Info(L"🔧 ConfigManager initialized with emoji logger support!");

    std::cout << "\n=== TESTING MOTION CONFIG FILES ===" << std::endl;

    // Test loading motion-specific configurations
    std::vector<std::string> motionConfigs = {
        "motion_config_devices.json",
        "motion_config_graph.json",
        "motion_config_positions.json",
        "transformation_matrix.json"
    };

    int successCount = 0;
    for (const auto& configFile : motionConfigs) {
      Logger::Info(L"\n🔍 Testing: " + ConfigLogger::StringToWString(configFile));

      if (configManager.LoadConfig(configFile)) {
        ConfigLogger::ConfigLoaded(configFile);
        successCount++;

        // Validate that it contains valid JSON
        auto config = configManager.GetConfig(configFile);
        if (!config.empty()) {
          ConfigLogger::ConfigValidated(configFile);

          // Print basic info about the config
          if (configFile == "motion_config_devices.json") {
            if (config.contains("MotionDevices")) {
              Logger::Info(L"📱 Found " + std::to_wstring(config["MotionDevices"].size()) + L" motion devices");
              for (const auto& [name, device] : config["MotionDevices"].items()) {
                bool enabled = ConfigHelper::GetValue<bool>(device, "IsEnabled", false);
                std::string type = ConfigHelper::GetValue<std::string>(device, "typeController", "unknown");
                ConfigLogger::MotionDeviceFound(name, type, enabled);
              }
            }
          }
          else if (configFile == "motion_config_positions.json") {
            Logger::Info(L"📍 Position data for devices:");
            for (const auto& [device, positions] : config.items()) {
              if (positions.is_object()) {
                Logger::Info(L"    🤖 " + ConfigLogger::StringToWString(device) + L": " +
                  std::to_wstring(positions.size()) + L" positions");
              }
            }
          }
          else if (configFile == "motion_config_graph.json") {
            if (config.contains("Graphs") && config["Graphs"].contains("Process_Flow")) {
              auto graph = config["Graphs"]["Process_Flow"];
              int nodeCount = graph.contains("Nodes") ? graph["Nodes"].size() : 0;
              int edgeCount = graph.contains("Edges") ? graph["Edges"].size() : 0;
              Logger::Info(L"🔗 Process flow: " + std::to_wstring(nodeCount) + L" nodes, " +
                std::to_wstring(edgeCount) + L" edges");
            }
          }
          else if (configFile == "transformation_matrix.json") {
            if (config.is_array()) {
              Logger::Info(L"🔄 Found " + std::to_wstring(config.size()) + L" transformation matrices");
            }
          }
        }
        else {
          Logger::Warning(L"⚠️ Empty or invalid JSON");
        }
      }
      else {
        ConfigLogger::ConfigError(configFile, "Failed to load");
      }
    }

    std::cout << "\n=== MOTION CONFIG LOADING SUMMARY ===" << std::endl;
    std::cout << "Successfully loaded: " << successCount << "/" << motionConfigs.size() << " files" << std::endl;

    // === TEST ConfigRegistry MOTION HELPERS ===
    std::cout << "\n=== TESTING ConfigRegistry MOTION HELPERS ===" << std::endl;

    if (ConfigRegistry::LoadMotionConfigs()) {
      std::cout << "✅ ConfigRegistry motion configs loaded" << std::endl;

      // Test getting motion devices
      auto devices = Config::Motion::GetAllDevices();
      std::cout << "📱 Found " << devices.size() << " motion devices via ConfigRegistry:" << std::endl;
      for (const auto& device : devices) {
        std::cout << "  - " << device.name << " [ID:" << device.id << "] "
          << device.typeController << " @ " << device.ipAddress << ":" << device.port
          << " (" << (device.isEnabled ? "ENABLED" : "DISABLED") << ")" << std::endl;
      }

      // Test getting specific positions
      if (!devices.empty()) {
        std::string deviceName = devices[0].name;
        std::cout << "\n📍 Testing position access for: " << deviceName << std::endl;

        auto homePos = Config::Motion::GetPosition(deviceName, "home");
        std::cout << "  Home position: X=" << homePos.x << ", Y=" << homePos.y << ", Z=" << homePos.z << std::endl;

        auto safePos = Config::Motion::GetPosition(deviceName, "safe");
        std::cout << "  Safe position: X=" << safePos.x << ", Y=" << safePos.y << ", Z=" << safePos.z << std::endl;
      }
    }
    else {
      std::cout << "❌ ConfigRegistry motion configs failed to load" << std::endl;
    }

    // === TEST POSITION MODIFICATION ===
    std::cout << "\n=== TESTING POSITION MODIFICATION ===" << std::endl;

    Config::Motion::Position testPos = { 123.45, 67.89, 10.11, 0.1, 0.2, 0.3 };
    if (Config::Motion::SetPosition("gantry-main", "test_position", testPos)) {
      std::cout << "✅ Successfully saved test position" << std::endl;

      // Verify it was saved
      auto retrievedPos = Config::Motion::GetPosition("gantry-main", "test_position");
      if (retrievedPos.x == testPos.x && retrievedPos.y == testPos.y && retrievedPos.z == testPos.z) {
        std::cout << "✅ Position retrieved correctly: X=" << retrievedPos.x
          << ", Y=" << retrievedPos.y << ", Z=" << retrievedPos.z << std::endl;
      }
      else {
        std::cout << "❌ Position mismatch after save/load" << std::endl;
      }
    }
    else {
      std::cout << "❌ Failed to save test position" << std::endl;
    }

    // === TEST CONFIGURATION VALIDATION ===
    std::cout << "\n=== TESTING CONFIGURATION VALIDATION ===" << std::endl;

    int validCount = 0;
    for (const auto& configFile : motionConfigs) {
      if (configManager.ValidateConfig(configFile)) {
        std::cout << "✅ " << configFile << " is valid" << std::endl;
        validCount++;
      }
      else {
        std::cout << "❌ " << configFile << " validation failed" << std::endl;
      }
    }
    std::cout << "Validation: " << validCount << "/" << motionConfigs.size() << " files valid" << std::endl;

    // === TEST BACKUP FUNCTIONALITY ===
    std::cout << "\n=== TESTING BACKUP FUNCTIONALITY ===" << std::endl;

    if (ConfigRegistry::BackupAllConfigs("test_backup")) {
      std::cout << "✅ Configuration backup created successfully" << std::endl;
    }
    else {
      std::cout << "❌ Configuration backup failed" << std::endl;
    }

    // === TEST DIRECT JSON ACCESS ===
    std::cout << "\n=== TESTING DIRECT JSON ACCESS ===" << std::endl;

    auto devicesConfig = configManager.GetConfig("motion_config_devices.json");
    if (!devicesConfig.empty()) {
      // Test modifying configuration directly
      if (devicesConfig.contains("Settings")) {
        auto originalTimeout = ConfigHelper::GetValue<int>(devicesConfig["Settings"], "ConnectionTimeout", 5000);
        std::cout << "Original connection timeout: " << originalTimeout << "ms" << std::endl;

        // Modify and save
        devicesConfig["Settings"]["ConnectionTimeout"] = 7500;
        configManager.SetConfig("motion_config_devices.json", devicesConfig);

        // Verify change
        auto modifiedConfig = configManager.GetConfig("motion_config_devices.json");
        auto newTimeout = ConfigHelper::GetValue<int>(modifiedConfig["Settings"], "ConnectionTimeout", 5000);
        std::cout << "Modified connection timeout: " << newTimeout << "ms" << std::endl;

        if (newTimeout == 7500) {
          std::cout << "✅ Direct JSON modification successful" << std::endl;
        }
        else {
          std::cout << "❌ Direct JSON modification failed" << std::endl;
        }
      }
    }

    // === TEST ALL KNOWN CONFIGS ===
    std::cout << "\n=== TESTING ALL KNOWN CONFIGURATIONS ===" << std::endl;

    if (ConfigRegistry::LoadAllKnownConfigs()) {
      std::cout << "✅ All known configurations loaded" << std::endl;
    }
    else {
      std::cout << "⚠️ Some known configurations failed to load (expected if files don't exist)" << std::endl;
    }

    auto allConfigFiles = ConfigRegistry::GetAllConfigFiles();
    std::cout << "📁 Known configuration files (" << allConfigFiles.size() << "):" << std::endl;
    for (const auto& file : allConfigFiles) {
      bool hasConfig = configManager.HasConfig(file);
      std::cout << "  " << (hasConfig ? "✅" : "❌") << " " << file << std::endl;
    }

    // === PERFORMANCE TEST ===
    std::cout << "\n=== PERFORMANCE TEST ===" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // Load all motion configs 100 times
    for (int i = 0; i < 100; i++) {
      configManager.GetConfig("motion_config_devices.json");
      configManager.GetConfig("motion_config_positions.json");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "⏱️ 200 config access operations took: " << duration.count() << " microseconds" << std::endl;
    std::cout << "⏱️ Average per operation: " << (duration.count() / 200.0) << " microseconds" << std::endl;

    // === CLEANUP ===
    std::cout << "\n=== CLEANUP ===" << std::endl;
    configManager.ClearCache();
    std::cout << "✅ Configuration cache cleared" << std::endl;

  }
  catch (const std::exception& e) {
    std::cout << "✗ Error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "\n=== ConfigManager Test Completed Successfully ===" << std::endl;
  return 0;
}