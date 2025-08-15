// TestMain.cpp
// Hardware testing for PI standardized managers with real ConfigManager
#include "devices/UniversalServices.h"
#include "devices/motions/PIControllerManagerStandardized.h"
#include "core/ConfigManager.h"
#include "core/ConfigRegistry.h"
#include "utils/LoggerAdapter.h"
#include "devices/motions/PIController.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

void TestPIDeviceOperations(PIController* device, const std::string& deviceName) {
  if (!device || !device->IsConnected()) {
    std::cout << "    ❌ Device " << deviceName << " not available for testing" << std::endl;
    return;
  }

  std::cout << "    🧪 Testing PI device operations for: " << deviceName << std::endl;

  try {
    // Test 0: Basic device information
    std::cout << "    📋 Device Information:" << std::endl;
    std::cout << "      Controller ID: " << device->GetControllerId() << std::endl;
    std::cout << "      Available Axes: ";
    for (const auto& axis : device->GetAvailableAxes()) {
      std::cout << axis << " ";
    }
    std::cout << std::endl;

    // Test 0.5: Device manufacturer identification
    std::cout << "    🏭 Device Manufacturer Information:" << std::endl;
    std::string manufacturerInfo;
    if (device->GetDeviceIdentification(manufacturerInfo)) {
      std::cout << "      Identification: " << manufacturerInfo << std::endl;
    }
    else {
      std::cout << "      ⚠️ Failed to retrieve device identification" << std::endl;
    }

    // Test 1: Get current positions
    std::map<std::string, double> positions;
    if (device->GetPositions(positions)) {
      std::cout << "    📍 Current positions:" << std::endl;
      for (const auto& [axis, pos] : positions) {
        std::cout << "      " << axis << ": " << std::fixed << std::setprecision(6) << pos << " mm" << std::endl;
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
      else {
        std::cout << "      " << axis << ": QUERY FAILED" << std::endl;
      }
    }

    // Test 3: Check motion status
    std::cout << "    🏃 Motion status:" << std::endl;
    for (const auto& axis : device->GetAvailableAxes()) {
      bool moving = device->IsMoving(axis);
      std::cout << "      " << axis << ": " << (moving ? "MOVING" : "IDLE") << std::endl;
    }

    // Test 4: System velocity
    double velocity = 0.0;
    if (device->GetSystemVelocity(velocity)) {
      std::cout << "    🚀 Current system velocity: " << std::fixed << std::setprecision(3) << velocity << " mm/s" << std::endl;
    }
    else {
      std::cout << "    ⚠️ Failed to read system velocity" << std::endl;
    }

    // Test 5: Individual axis velocities
    std::cout << "    🎯 Individual axis velocities:" << std::endl;
    for (const auto& axis : device->GetAvailableAxes()) {
      double axisVel = 0.0;
      if (device->GetVelocity(axis, axisVel)) {
        std::cout << "      " << axis << ": " << std::fixed << std::setprecision(3) << axisVel << " mm/s" << std::endl;
      }
      else {
        std::cout << "      " << axis << ": QUERY FAILED" << std::endl;
      }
    }

    // Test 6: Analog channels information
    int analogChannels = 0;
    if (device->GetAnalogChannelCount(analogChannels)) {
      std::cout << "    📊 Analog channels available: " << analogChannels << std::endl;

      if (analogChannels > 0) {
        std::cout << "    📈 Analog channel readings:" << std::endl;
        for (int ch = 1; ch <= (std::min)(analogChannels, 6); ch++) {  // Read first 6 channels
          double voltage = 0.0;
          if (device->GetAnalogVoltage(ch, voltage)) {
            std::cout << "      Channel " << ch << ": " << std::fixed << std::setprecision(4) << voltage << " V" << std::endl;
          }
          else {
            std::cout << "      Channel " << ch << ": READ FAILED" << std::endl;
          }
        }
      }
    }
    else {
      std::cout << "    ⚠️ Failed to query analog channel count" << std::endl;
    }

    // Test 7: Connection status details
    std::cout << "    🔌 Connection details:" << std::endl;
    std::cout << "      Connected: " << (device->IsConnected() ? "YES" : "NO") << std::endl;
    std::cout << "      Analog reading enabled: " << (device->IsAnalogReadingEnabled() ? "YES" : "NO") << std::endl;

    // Test 8: Small relative move test (OPTIONAL - only if user confirms)
    std::cout << "    🏃 Would you like to test a small movement? (0.1mm on X axis) [y/N]: ";
    char response;
    std::cin >> response;
    std::cin.ignore(); // Clear the input buffer

    if (response == 'y' || response == 'Y') {
      std::cout << "    🏃 Testing small relative move on X axis (+0.1mm)..." << std::endl;
      if (device->MoveRelative("X", 0.1, true)) {
        std::cout << "    ✅ Relative move completed successfully" << std::endl;

        // Read new position
        double newPos = 0.0;
        if (device->GetPosition("X", newPos)) {
          std::cout << "    📍 New X position: " << std::fixed << std::setprecision(6) << newPos << " mm" << std::endl;
        }

        // Move back to original position
        std::cout << "    🔄 Moving back to original position..." << std::endl;
        if (device->MoveRelative("X", -0.1, true)) {
          std::cout << "    ✅ Return move completed successfully" << std::endl;
        }
        else {
          std::cout << "    ⚠️ Return move failed" << std::endl;
        }
      }
      else {
        std::cout << "    ❌ Relative move failed" << std::endl;
      }
    }
    else {
      std::cout << "    ℹ️ Movement test skipped" << std::endl;
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

    // === PI DEVICE IDENTIFICATION TEST ===
    if (anyConnected) {
      std::cout << "\n=== PI Device Identification ===" << std::endl;
      std::cout << "🏭 Retrieving manufacturer information for connected devices..." << std::endl;

      for (const auto& deviceName : piDevices) {
        if (piManager->IsDeviceConnected(deviceName)) {
          std::cout << "  " << deviceName << ": ";

          std::string manufacturerInfo;
          if (piManager->GetDeviceIdentification(deviceName, manufacturerInfo)) {
            std::cout << "✅ " << manufacturerInfo << std::endl;
          }
          else {
            std::cout << "❌ Failed to get identification" << std::endl;
          }
        }
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

    // CRITICAL: Clear PI manager pointer before destruction
    std::cout << "🔧 Releasing PI manager..." << std::endl;
    piManager.reset();  // Explicitly destroy PI manager first

    // CRITICAL: Clear logger adapter before ConfigManager
    std::cout << "🔧 Releasing logger adapter..." << std::endl;
    configManager.SetLogger(nullptr);  // Remove logger reference first
    loggerAdapter.reset();  // Then destroy logger

    std::cout << "✅ All resources cleaned up safely" << std::endl;
    ConfigLogger::ConfigLoaded("Safe shutdown completed");

  }
  catch (const std::exception& e) {
    std::cout << "❌ CRITICAL ERROR: " << e.what() << std::endl;

    // DON'T use ConfigLogger during error handling - it might be the source!
    std::cout << "❌ Config error in System: " << e.what() << std::endl;

    // Emergency cleanup - be very careful about order
    std::cout << "🚨 Performing emergency cleanup..." << std::endl;
    try {
      // Clear services first (this is accessible)
      Services::Clear();

      // Clear logger reference (ConfigManager is singleton, so this is safe)
      auto& configManager = ConfigManager::Instance();
      configManager.SetLogger(nullptr);

      std::cout << "✅ Emergency cleanup completed" << std::endl;
    }
    catch (...) {
      std::cout << "⚠️ Emergency cleanup failed - forcing exit" << std::endl;
    }

    return 1;
  }

  ConfigLogger::ConfigTestEnd(true);
  std::cout << "\n🎉 PI hardware testing completed successfully!" << std::endl;
  return 0;
}