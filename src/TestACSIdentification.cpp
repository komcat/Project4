// TestACSIdentification.cpp
// Hardware testing for ACS standardized managers with real ConfigManager
#include "devices/UniversalServices.h"
#include "devices/motions/ACSControllerManagerStandardized.h"
#include "core/ConfigManager.h"
#include "core/ConfigRegistry.h"
#include "utils/LoggerAdapter.h"
#include "devices/motions/ACSController.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>

void TestACSDeviceOperations(ACSController* device, const std::string& deviceName) {
  if (!device || !device->IsConnected()) {
    std::cout << "    ❌ Device " << deviceName << " not available for testing" << std::endl;
    return;
  }

  std::cout << "    🧪 Testing ACS device operations for: " << deviceName << std::endl;

  try {
    // Test 0: Basic device information
    std::cout << "    📋 Device Information:" << std::endl;
    std::cout << "      Controller ID: " << device->GetControllerId() << std::endl;
    std::cout << "      Available Axes: ";
    for (const auto& axis : device->GetAvailableAxes()) {
      std::cout << axis << " ";
    }
    std::cout << std::endl;

    // Test 0.5: Device manufacturer identification - NEW ACS METHODS
    std::cout << "    🏭 Device Manufacturer Information:" << std::endl;

    // Test individual new methods
    std::string firmwareVersion;
    if (device->GetFirmwareVersion(firmwareVersion)) {
      std::cout << "      Firmware Version: " << firmwareVersion << std::endl;
    }
    else {
      std::cout << "      ⚠️ Failed to retrieve firmware version" << std::endl;
    }

    std::string serialNumber;
    if (device->GetSerialNumber(serialNumber)) {
      std::cout << "      Serial Number: " << serialNumber << std::endl;
    }
    else {
      std::cout << "      ⚠️ Failed to retrieve serial number" << std::endl;
    }

    // Test combined identification method
    std::string manufacturerInfo;
    if (device->GetDeviceIdentification(manufacturerInfo)) {
      std::cout << "      Full Identification: " << manufacturerInfo << std::endl;
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

    // Test 4: Individual axis velocities
    std::cout << "    🚀 Individual axis velocities:" << std::endl;
    for (const auto& axis : device->GetAvailableAxes()) {
      double velocity = 0.0;
      if (device->GetVelocity(axis, velocity)) {
        std::cout << "      " << axis << ": " << std::fixed << std::setprecision(3) << velocity << " mm/s" << std::endl;
      }
      else {
        std::cout << "      " << axis << ": QUERY FAILED" << std::endl;
      }
    }

    // Test 5: Connection status details
    std::cout << "    🔌 Connection details:" << std::endl;
    std::cout << "      Connected: " << (device->IsConnected() ? "YES" : "NO") << std::endl;

    // Test 6: Small relative move test (OPTIONAL - only if user confirms)
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
    std::cout << "    ❌ Exception during ACS device testing: " << e.what() << std::endl;
  }
}

int main() {
  std::cout << "=== Hardware Testing for ACS Controller Manager ===" << std::endl;
  std::cout << "⚠️  WARNING: This test will attempt to connect to real ACS hardware!" << std::endl;
  std::cout << "🔌 Ensure ACS controllers are powered and connected to network" << std::endl;
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

    // === SETUP ACS DEVICE MANAGER ===
    std::cout << "\n=== Creating ACS Device Manager in HARDWARE MODE ===" << std::endl;

    // Create standardized ACS manager in HARDWARE MODE
    auto acsManager = std::make_unique<ACSControllerManagerStandardized>(configManager);

    // Register with Services
    Services::RegisterACSManager(acsManager.get());

    // Print initial status
    std::cout << "📊 Services Status:" << std::endl;
    std::cout << "  ACS Manager: " << (Services::HasACSManager() ? "REGISTERED" : "NOT REGISTERED") << std::endl;

    // === INITIALIZATION ===
    std::cout << "\n=== Initialization ===" << std::endl;
    if (acsManager->Initialize()) {
      ConfigLogger::ConfigLoaded("ACS manager initialized successfully");
    }
    else {
      ConfigLogger::ConfigError("ACS Manager", "Failed to initialize");
    }

    // Print device configurations before attempting connection
    std::cout << "\n=== ACS Device Configurations ===" << std::endl;

    std::cout << "📋 ACS Device Configurations:" << std::endl;
    auto acsDevices = acsManager->GetDeviceNames();
    std::cout << "Found " << acsDevices.size() << " configured ACS devices:" << std::endl;
    for (const auto& deviceName : acsDevices) {
      std::cout << "  " << deviceName << " [CONFIGURED]" << std::endl;
    }

    // === HARDWARE CONNECTION TEST ===
    std::cout << "\n=== Attempting ACS Hardware Connection ===" << std::endl;
    std::cout << "🔌 This will attempt to connect to actual ACS controllers..." << std::endl;

    // Track connection results
    bool anyConnected = false;

    std::cout << "🤖 Attempting to connect " << acsDevices.size() << " ACS devices:" << std::endl;

    for (const auto& deviceName : acsDevices) {
      std::cout << "  Connecting to " << deviceName << "... ";

      bool connected = acsManager->ConnectDevice(deviceName);
      if (connected) {
        std::cout << "✅ SUCCESS" << std::endl;
        anyConnected = true;
        ConfigLogger::MotionDeviceFound(deviceName, "ACS", true);
      }
      else {
        std::cout << "❌ FAILED" << std::endl;
        ConfigLogger::MotionDeviceFound(deviceName, "ACS", false);
      }
    }

    // === ACS DEVICE IDENTIFICATION TEST ===
    if (anyConnected) {
      std::cout << "\n=== ACS Device Identification ===" << std::endl;
      std::cout << "🏭 Retrieving manufacturer information for connected devices..." << std::endl;

      for (const auto& deviceName : acsDevices) {
        if (acsManager->IsDeviceConnected(deviceName)) {
          std::cout << "  " << deviceName << ": ";

          std::string manufacturerInfo;
          if (acsManager->GetDeviceIdentification(deviceName, manufacturerInfo)) {
            std::cout << "✅ " << manufacturerInfo << std::endl;
          }
          else {
            std::cout << "❌ Failed to get identification" << std::endl;
          }
        }
      }
    }
    else {
      // === TEST MOCK DEVICE IDENTIFICATION ===
      std::cout << "\n=== Testing Mock ACS Device Identification ===" << std::endl;
      std::cout << "🧪 Since no hardware connected, testing mock identification..." << std::endl;

      // Connect all devices using mock
      if (acsManager->ConnectAll()) {
        std::cout << "✅ Mock devices connected" << std::endl;

        for (const auto& deviceName : acsDevices) {
          if (acsManager->IsDeviceConnected(deviceName)) {
            std::cout << "  " << deviceName << ": ";

            std::string manufacturerInfo;
            if (acsManager->GetDeviceIdentification(deviceName, manufacturerInfo)) {
              std::cout << "✅ " << manufacturerInfo << std::endl;
            }
            else {
              std::cout << "❌ Failed to get identification" << std::endl;
            }
          }
        }

       
      }
    }

    // === CONNECTION STATUS SUMMARY ===
    std::cout << "\n=== ACS Connection Status Summary ===" << std::endl;
    std::cout << "📊 ACS Device Status:" << std::endl;
    for (const auto& deviceName : acsDevices) {
      bool connected = acsManager->IsDeviceConnected(deviceName);
      std::cout << "  " << deviceName << ": " << (connected ? "✅ CONNECTED" : "❌ DISCONNECTED") << std::endl;
    }

    if (!anyConnected) {
      std::cout << "⚠️  No real hardware devices connected successfully." << std::endl;
      std::cout << "   This is normal if:" << std::endl;
      std::cout << "   - Hardware is not powered on" << std::endl;
      std::cout << "   - Network configuration is incorrect" << std::endl;
      std::cout << "   - IP addresses in config don't match hardware" << std::endl;
      std::cout << "   - Controllers are already connected to another application" << std::endl;
      std::cout << "   📝 Mock testing was performed instead" << std::endl;
    }
    else {
      std::cout << "🎉 Successfully connected to some devices!" << std::endl;
    }

    // === HARDWARE OPERATIONS TEST ===
    if (anyConnected) {
      std::cout << "\n=== ACS Hardware Operations Test ===" << std::endl;
      std::cout << "🧪 Testing connected ACS devices..." << std::endl;

      // Test ACS devices
      for (const auto& deviceName : acsDevices) {
        if (acsManager->IsDeviceConnected(deviceName)) {
          ACSController* device = acsManager->GetDevice(deviceName);
          TestACSDeviceOperations(device, deviceName);
        }
      }

      // === BATCH OPERATIONS TEST ===
      std::cout << "\n=== ACS Batch Operations Test ===" << std::endl;

      std::cout << "🛑 Testing emergency stop for all ACS devices..." << std::endl;
      // Note: ACS manager might not have StopAllDevices method, so we'll test individual stops
      bool allStopped = true;
      for (const auto& deviceName : acsDevices) {
        if (acsManager->IsDeviceConnected(deviceName)) {
          ACSController* device = acsManager->GetDevice(deviceName);
          if (device && !device->StopAllAxes()) {
            allStopped = false;
          }
        }
      }

      if (allStopped) {
        std::cout << "✅ All ACS devices stopped successfully" << std::endl;
      }
      else {
        std::cout << "⚠️ Some ACS devices failed to stop" << std::endl;
      }

      // === POSITION MONITORING ===
      std::cout << "\n=== ACS Position Monitoring Test ===" << std::endl;
      std::cout << "📍 Monitoring ACS positions for 5 seconds..." << std::endl;

      for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (const auto& deviceName : acsDevices) {
          if (acsManager->IsDeviceConnected(deviceName)) {
            ACSController* device = acsManager->GetDevice(deviceName);
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
    std::cout << "\n=== ACS Configuration Verification ===" << std::endl;

    auto motionDevices = Config::Motion::GetAllDevices();
    Logger::Info(L"📋 Total devices in configuration: " + std::to_wstring(motionDevices.size()));

    int acsCount = 0, connectedACSCount = 0;
    for (const auto& device : motionDevices) {
      if (device.typeController == "ACS" && device.isEnabled) {
        acsCount++;
        if (acsManager->IsDeviceConnected(device.name)) {
          connectedACSCount++;
        }
      }
    }

    std::cout << "📊 ACS Summary:" << std::endl;
    std::cout << "  ACS devices configured: " << acsCount << ", connected: " << connectedACSCount << std::endl;

    // === CLEANUP ===
    std::cout << "\n=== Safe Shutdown ===" << std::endl;
    std::cout << "🛑 Stopping all ACS devices before disconnection..." << std::endl;

    // Stop all devices individually (since ACS manager might not have StopAllDevices)
    for (const auto& deviceName : acsDevices) {
      if (acsManager->IsDeviceConnected(deviceName)) {
        ACSController* device = acsManager->GetDevice(deviceName);
        if (device) {
          device->StopAllAxes();
        }
      }
    }

    std::cout << "🔌 Disconnecting all ACS devices..." << std::endl;
    acsManager->DisconnectAll();

    std::cout << "🧹 Clearing services..." << std::endl;
    Services::Clear();

    // CRITICAL: Clear ACS manager pointer before destruction
    std::cout << "🔧 Releasing ACS manager..." << std::endl;
    acsManager.reset();  // Explicitly destroy ACS manager first

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
  std::cout << "\n🎉 ACS hardware testing completed successfully!" << std::endl;
  return 0;
}