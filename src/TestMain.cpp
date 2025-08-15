// TestMain.cpp
// Simple test for PI and ACS standardized managers with real ConfigManager
#include "devices/UniversalServices.h"
#include "devices/motions/PIControllerManagerStandardized.h"
#include "devices/motions/ACSControllerManagerStandardized.h"
#include "core/ConfigManager.h"
#include "core/ConfigRegistry.h"
#include "utils/LoggerAdapter.h"
#include <iostream>
#include <memory>

int main() {
  std::cout << "=== Testing Standardized Device Managers with Real ConfigManager ===" << std::endl;

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

    // === SETUP DEVICE MANAGERS ===
    std::cout << "\n=== Creating Device Managers ===" << std::endl;

    // Create standardized managers using real ConfigManager
    auto piManager = std::make_unique<PIControllerManagerStandardized>(configManager);
    auto acsManager = std::make_unique<ACSControllerManagerStandardized>(configManager);

    // Register with Services
    Services::RegisterPIManager(piManager.get());
    Services::RegisterACSManager(acsManager.get());

    // Print initial status
    Services::PrintStatus();

    // === INITIALIZATION ===
    std::cout << "\n=== Initialization ===" << std::endl;
    if (Services::InitializeAll()) {
      ConfigLogger::ConfigLoaded("All managers initialized successfully");
    }
    else {
      ConfigLogger::ConfigError("Managers", "Some managers failed to initialize");
    }

    // === CONNECTION ===
    std::cout << "\n=== Connection ===" << std::endl;
    if (Services::ConnectAll()) {
      ConfigLogger::ConfigLoaded("All managers connected successfully");
    }
    else {
      ConfigLogger::ConfigError("Connection", "Some managers failed to connect (normal if hardware unavailable)");
    }

    // === DEVICE ENUMERATION ===
    std::cout << "\n=== Device Enumeration ===" << std::endl;

    if (Services::HasPIManager()) {
      auto* piMgr = Services::PIManager();
      auto piDevices = piMgr->GetDeviceNames();
      Logger::Info(L"🤖 PI Devices (" + std::to_wstring(piDevices.size()) + L"):");
      for (const auto& name : piDevices) {
        bool connected = piMgr->IsDeviceConnected(name);
        ConfigLogger::MotionDeviceFound(name, "PI", connected);
      }
    }

    if (Services::HasACSManager()) {
      auto* acsMgr = Services::ACSManager();
      auto acsDevices = acsMgr->GetDeviceNames();
      Logger::Info(L"🤖 ACS Devices (" + std::to_wstring(acsDevices.size()) + L"):");
      for (const auto& name : acsDevices) {
        bool connected = acsMgr->IsDeviceConnected(name);
        ConfigLogger::MotionDeviceFound(name, "ACS", connected);
      }
    }

    // === DEVICE ACCESS TEST ===
    std::cout << "\n=== Device Access Test ===" << std::endl;

    // Try to get specific devices using convenience methods
    if (auto* hexLeft = Services::GetPIDevice("hex-left")) {
      Logger::Success(L"✅ Found PI device 'hex-left'");
    }
    else {
      Logger::Info(L"ℹ️ PI device 'hex-left' returned nullptr (expected in test mode)");
    }

    if (auto* gantryMain = Services::GetACSDevice("gantry-main")) {
      Logger::Success(L"✅ Found ACS device 'gantry-main'");
    }
    else {
      Logger::Info(L"ℹ️ ACS device 'gantry-main' returned nullptr (expected in test mode)");
    }

    // === INDIVIDUAL DEVICE CONNECTION TEST ===
    std::cout << "\n=== Individual Device Connection Test ===" << std::endl;

    if (Services::HasPIManager()) {
      auto* piMgr = Services::PIManager();

      Logger::Info(L"🔌 Testing PI device connections:");
      if (piMgr->ConnectDevice("hex-left")) {
        ConfigLogger::ConfigLoaded("Connected hex-left");
      }

      // Check connection status
      bool connected = piMgr->IsDeviceConnected("hex-left");
      if (connected) {
        ConfigLogger::MotionDeviceFound("hex-left", "PI", true);
      }

      // Test disconnect
      if (piMgr->DisconnectDevice("hex-left")) {
        ConfigLogger::ConfigLoaded("Disconnected hex-left");
      }
    }

    if (Services::HasACSManager()) {
      auto* acsMgr = Services::ACSManager();

      Logger::Info(L"🔌 Testing ACS device connections:");
      if (acsMgr->ConnectDevice("gantry-main")) {
        ConfigLogger::ConfigLoaded("Connected gantry-main");
      }

      bool connected = acsMgr->IsDeviceConnected("gantry-main");
      if (connected) {
        ConfigLogger::MotionDeviceFound("gantry-main", "ACS", true);
      }
    }

    // === UNIVERSAL OPERATIONS TEST ===
    std::cout << "\n=== Universal Operations Test ===" << std::endl;

    // Test manager-level operations
    if (Services::HasPIManager()) {
      auto* piMgr = Services::PIManager();
      Logger::Info(L"📊 PI Manager Type: " + ConfigLogger::StringToWString(piMgr->GetManagerType()));
      Logger::Info(L"📊 PI Manager Initialized: " + std::wstring(piMgr->IsInitialized() ? L"Yes" : L"No"));
    }

    if (Services::HasACSManager()) {
      auto* acsMgr = Services::ACSManager();
      Logger::Info(L"📊 ACS Manager Type: " + ConfigLogger::StringToWString(acsMgr->GetManagerType()));
      Logger::Info(L"📊 ACS Manager Initialized: " + std::wstring(acsMgr->IsInitialized() ? L"Yes" : L"No"));
    }

    // === CONFIGURATION INTEGRATION TEST ===
    std::cout << "\n=== Configuration Integration Test ===" << std::endl;

    // Test accessing configuration data that the managers are using
    auto motionDevices = Config::Motion::GetAllDevices();
    Logger::Info(L"📋 Total devices in configuration: " + std::to_wstring(motionDevices.size()));

    int piCount = 0, acsCount = 0;
    for (const auto& device : motionDevices) {
      if (device.typeController == "PI" && device.isEnabled) piCount++;
      if (device.typeController == "ACS" && device.isEnabled) acsCount++;
    }

    Logger::Info(L"📋 PI devices in config: " + std::to_wstring(piCount));
    Logger::Info(L"📋 ACS devices in config: " + std::to_wstring(acsCount));

    // Test position access
    auto homePos = Config::Motion::GetPosition("gantry-main", "home");
    ConfigLogger::PositionLoaded("gantry-main", "home");
    Logger::Info(L"📍 Gantry home position: X=" + std::to_wstring(homePos.x) +
      L", Y=" + std::to_wstring(homePos.y) + L", Z=" + std::to_wstring(homePos.z));

    // === CLEANUP ===
    std::cout << "\n=== Cleanup ===" << std::endl;
    Services::DisconnectAll();
    Services::Clear();
    ConfigLogger::ConfigLoaded("Cleanup completed");

  }
  catch (const std::exception& e) {
    ConfigLogger::ConfigError("System", e.what());
    return 1;
  }

  ConfigLogger::ConfigTestEnd(true);
  return 0;
}