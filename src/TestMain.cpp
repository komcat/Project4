
// Hardware testing for PI standardized managers with real ConfigManager
#include "devices/UniversalServices.h"
#include "devices/motions/PIControllerManagerStandardized.h"
#include "core/ConfigManager.h"
#include "core/ConfigRegistry.h"
#include "utils/LoggerAdapter.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "devices/motions/PIController.h" 

void TestPIDeviceOperations(PIController* device, const std::string& deviceName) {
  if (!device || !device->IsConnected()) {
    std::cout << "    ❌ Device " << deviceName << " not available for testing" << std::endl;
    return;
  }

  std::cout << "    🧪 Testing PI device operations for: " << deviceName << std::endl;

  try {
    // Test 1: Get current positions
    std::map<std::string, double> positions;
    if (device->GetPositions(positions)) {
      std::cout << "    📍 Current positions:" << std::endl;
      for (const auto& [axis, pos] : positions) {
        std::cout << "      " << axis << ": " << pos << " mm" << std::endl;
      }
    }
    else {
      std::cout << "    ⚠️ Failed to read current positions" << std::endl;
    }

    // Test 2: Check servo status
    std::cout << "    🔧 Servo status:" << std::endl;
    for (const auto& axis : device->GetAvailableAxes()) {
      bool enabled = false;
      if (device->IsServoEnabled(axis, enabled)) {
        std::cout << "      " << axis << ": " << (enabled ? "ENABLED" : "DISABLED") << std::endl;
      }
    }

    // Test 3: Small relative move (safe test)
    std::cout << "    🏃 Testing small relative move on X axis (+0.1mm)..." << std::endl;
    if (device->MoveRelative("X", 0.1, true)) {
      std::cout << "    ✅ Relative move completed successfully" << std::endl;

      // Move back to original position
      std::cout << "    🔄 Moving back to original position..." << std::endl;
      device->MoveRelative("X", -0.1, true);
    }
    else {
      std::cout << "    ❌ Relative move failed" << std::endl;
    }

    // Test 4: Get analog readings (if available)
    int analogChannels = 0;
    if (device->GetAnalogChannelCount(analogChannels)) {
      std::cout << "    📊 Analog channels available: " << analogChannels << std::endl;

      if (analogChannels > 0) {
        double voltage = 0.0;
        if (device->GetAnalogVoltage(1, voltage)) {
          std::cout << "    📈 Analog channel 1 voltage: " << voltage << " V" << std::endl;
        }
      }
    }

    // Test 5: System velocity
    double velocity = 0.0;
    if (device->GetSystemVelocity(velocity)) {
      std::cout << "    🚀 Current system velocity: " << velocity << " mm/s" << std::endl;
    }

  }
  catch (const std::exception& e) {
    std::cout << "    ❌ Exception during PI device testing: " << e.what() << std::endl;
  }
}


int main() {
  std::cout << "=== Hardware Testing for PI Controller Manager ===" << std::endl;
  std::cout << "⚠️  WARNING: This test will attempt to connect to real PI hardware!" << std::endl;
  std::cout << "🔌 Ensure PI controllers are powered and connected to network" << std::endl;
  std::cout << "\nPress Enter to continue or Ctrl+C to abort..." << std::endl;
  std::cin.get();

  try {
    // === SETUP CONFIGURATION SYSTEM ===
    std::cout << "\n=== Setting up Configuration System ===" << std::endl;

    auto loggerAdapter = std::make_unique<LoggerAdapter>();

    auto& configManager = ConfigManager::Instance();
    configManager.SetLogger(loggerAdapter.get());
    configManager.SetConfigDirectory("config");

    // Load motion configurations
    ConfigLogger::ConfigTestStart();
    if (ConfigRegistry::LoadMotionConfigs()) {
      ConfigLogger::ConfigLoaded("Motion configurations");
    }
    else {
      ConfigLogger::ConfigError("Motion configurations", "Failed to load some configs");
    }

    // === SETUP PI DEVICE MANAGER ===
    std::cout << "\n=== Creating PI Device Manager in HARDWARE MODE ===" << std::endl;

    // Create standardized PI manager in HARDWARE MODE
    auto piManager = std::make_unique<PIControllerManagerStandardized>(configManager, true);  // TRUE = hardware mode

    // Register with Services
    Services::RegisterPIManager(piManager.get());

    // Print initial status
    std::cout << "📊 Services Status:" << std::endl;
    std::cout << "  PI Manager: " << (Services::HasPIManager() ? "REGISTERED" : "NOT REGISTERED") << std::endl;

    // === INITIALIZATION ===
    std::cout << "\n=== Initialization ===" << std::endl;
    if (piManager->Initialize()) {
      ConfigLogger::ConfigLoaded("PI manager initialized successfully");
    }
    else {
      ConfigLogger::ConfigError("PI Manager", "Failed to initialize");
    }

    // Print device configurations before attempting connection
    std::cout << "\n=== PI Device Configurations ===" << std::endl;

    std::cout << "📋 PI Device Configurations:" << std::endl;
    auto configs = piManager->GetAllDeviceConfigs();
    for (const auto& config : configs) {
      std::cout << "  " << config.name
        << " @ " << config.ipAddress << ":" << config.port
        << " [" << (config.isEnabled ? "ENABLED" : "DISABLED") << "]"
        << " Axes: " << config.installAxes << std::endl;
    }

    // === HARDWARE CONNECTION TEST ===
    std::cout << "\n=== Attempting PI Hardware Connection ===" << std::endl;
    std::cout << "🔌 This will attempt to connect to actual PI controllers..." << std::endl;

    // Track connection results
    bool anyConnected = false;
    auto piDevices = piManager->GetDeviceNames();

    std::cout << "🤖 Attempting to connect " << piDevices.size() << " PI devices:" << std::endl;

    for (const auto& deviceName : piDevices) {
      std::cout << "  Connecting to " << deviceName << "... ";

      bool connected = piManager->ConnectDevice(deviceName);
      if (connected) {
        std::cout << "✅ SUCCESS" << std::endl;
        anyConnected = true;
        ConfigLogger::MotionDeviceFound(deviceName, "PI", true);
      }
      else {
        std::cout << "❌ FAILED" << std::endl;
        ConfigLogger::MotionDeviceFound(deviceName, "PI", false);
      }
    }

    // === CONNECTION STATUS SUMMARY ===
    std::cout << "\n=== PI Connection Status Summary ===" << std::endl;
    piManager->PrintDeviceStatus();

    if (!anyConnected) {
      std::cout << "⚠️  No devices connected successfully." << std::endl;
      std::cout << "   This is normal if:" << std::endl;
      std::cout << "   - Hardware is not powered on" << std::endl;
      std::cout << "   - Network configuration is incorrect" << std::endl;
      std::cout << "   - IP addresses in config don't match hardware" << std::endl;
      std::cout << "   - Controllers are already connected to another application" << std::endl;
    }
    else {
      std::cout << "🎉 Successfully connected to some devices!" << std::endl;
    }

    // === HARDWARE OPERATIONS TEST ===
    if (anyConnected) {
      std::cout << "\n=== PI Hardware Operations Test ===" << std::endl;
      std::cout << "🧪 Testing connected PI devices..." << std::endl;

      // Test PI devices
      for (const auto& deviceName : piDevices) {
        if (piManager->IsDeviceConnected(deviceName)) {
          PIController* device = piManager->GetDevice(deviceName);
          TestPIDeviceOperations(device, deviceName);
        }
      }

      // === BATCH OPERATIONS TEST ===
      std::cout << "\n=== PI Batch Operations Test ===" << std::endl;

      std::cout << "🛑 Testing emergency stop for all PI devices..." << std::endl;
      if (piManager->StopAllDevices()) {
        std::cout << "✅ All PI devices stopped successfully" << std::endl;
      }
      else {
        std::cout << "⚠️ Some PI devices failed to stop" << std::endl;
      }

      // === POSITION MONITORING ===
      std::cout << "\n=== PI Position Monitoring Test ===" << std::endl;
      std::cout << "📍 Monitoring PI positions for 5 seconds..." << std::endl;

      for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (const auto& deviceName : piDevices) {
          if (piManager->IsDeviceConnected(deviceName)) {
            PIController* device = piManager->GetDevice(deviceName);
            if (device) {
              std::map<std::string, double> positions;
              if (device->GetPositions(positions)) {
                std::cout << "  " << deviceName << " [" << (i + 1) << "/5]: ";
                for (const auto& [axis, pos] : positions) {
                  std::cout << axis << "=" << std::fixed << std::setprecision(3) << pos << " ";
                }
                std::cout << std::endl;
              }
            }
          }
        }
      }
    }

    // === CONFIGURATION VERIFICATION ===
    std::cout << "\n=== PI Configuration Verification ===" << std::endl;

    auto motionDevices = Config::Motion::GetAllDevices();
    Logger::Info(L"📋 Total devices in configuration: " + std::to_wstring(motionDevices.size()));

    int piCount = 0, connectedPICount = 0;
    for (const auto& device : motionDevices) {
      if (device.typeController == "PI" && device.isEnabled) {
        piCount++;
        if (piManager->IsDeviceConnected(device.name)) {
          connectedPICount++;
        }
      }
    }

    std::cout << "📊 PI Summary:" << std::endl;
    std::cout << "  PI devices configured: " << piCount << ", connected: " << connectedPICount << std::endl;

    // === CLEANUP ===
    std::cout << "\n=== Safe Shutdown ===" << std::endl;
    std::cout << "🛑 Stopping all PI devices before disconnection..." << std::endl;

    piManager->StopAllDevices();

    std::cout << "🔌 Disconnecting all PI devices..." << std::endl;
    piManager->DisconnectAll();

    std::cout << "🧹 Clearing services..." << std::endl;
    Services::Clear();

    ConfigLogger::ConfigLoaded("Safe shutdown completed");

  }
  catch (const std::exception& e) {
    std::cout << "❌ CRITICAL ERROR: " << e.what() << std::endl;
    ConfigLogger::ConfigError("System", e.what());

    // Emergency cleanup
    std::cout << "🚨 Performing emergency cleanup..." << std::endl;
    try {
      Services::Clear();
    }
    catch (...) {
      std::cout << "⚠️ Emergency cleanup failed" << std::endl;
    }

    return 1;
  }

  ConfigLogger::ConfigTestEnd(true);
  std::cout << "\n🎉 PI hardware testing completed successfully!" << std::endl;
  return 0;
}