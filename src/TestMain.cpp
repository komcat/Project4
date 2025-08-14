// TestMain.cpp
// Simple test for PI and ACS standardized managers - TEST STUB VERSION
#include "devices/UniversalServices.h"
#include "devices/motions/PIControllerManagerStandardized.h"
#include "devices/motions/ACSControllerManagerStandardized.h"
// #include "include/motions/MotionConfigManager.h"  // Not needed for testing
#include <iostream>
#include <memory>

// Mock MotionConfigManager for testing
class MockMotionConfigManager {
public:
  MockMotionConfigManager(const std::string& configFile) {
    std::cout << "MockMotionConfigManager: Loaded " << configFile << " (TEST MODE)" << std::endl;
  }
};

int main() {
  std::cout << "=== Testing Standardized Device Managers (STUB VERSION) ===" << std::endl;

  try {
    // === SETUP ===
    MockMotionConfigManager configManager("motion_config.json");

    // Create standardized managers (now using test stubs)
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
      std::cout << "✓ All managers initialized successfully" << std::endl;
    }
    else {
      std::cout << "✗ Some managers failed to initialize" << std::endl;
    }

    // === CONNECTION ===
    std::cout << "\n=== Connection ===" << std::endl;
    if (Services::ConnectAll()) {
      std::cout << "✓ All managers connected successfully" << std::endl;
    }
    else {
      std::cout << "⚠ Some managers failed to connect (this is normal if hardware isn't available)" << std::endl;
    }

    // === DEVICE ENUMERATION ===
    std::cout << "\n=== Device Enumeration ===" << std::endl;

    if (Services::HasPIManager()) {
      auto* piMgr = Services::PIManager();
      auto piDevices = piMgr->GetDeviceNames();
      std::cout << "PI Devices (" << piDevices.size() << "):" << std::endl;
      for (const auto& name : piDevices) {
        bool connected = piMgr->IsDeviceConnected(name);
        std::cout << "  - " << name << ": " << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
      }
    }

    if (Services::HasACSManager()) {
      auto* acsMgr = Services::ACSManager();
      auto acsDevices = acsMgr->GetDeviceNames();
      std::cout << "ACS Devices (" << acsDevices.size() << "):" << std::endl;
      for (const auto& name : acsDevices) {
        bool connected = acsMgr->IsDeviceConnected(name);
        std::cout << "  - " << name << ": " << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
      }
    }

    // === DEVICE ACCESS TEST ===
    std::cout << "\n=== Device Access Test ===" << std::endl;

    // Try to get specific devices using convenience methods
    if (auto* hexLeft = Services::GetPIDevice("hex-left")) {
      std::cout << "✓ Found PI device 'hex-left'" << std::endl;
    }
    else {
      std::cout << "ℹ PI device 'hex-left' returned nullptr (expected in test mode)" << std::endl;
    }

    if (auto* gantryMain = Services::GetACSDevice("gantry-main")) {
      std::cout << "✓ Found ACS device 'gantry-main'" << std::endl;
    }
    else {
      std::cout << "ℹ ACS device 'gantry-main' returned nullptr (expected in test mode)" << std::endl;
    }

    // === INDIVIDUAL DEVICE CONNECTION TEST ===
    std::cout << "\n=== Individual Device Connection Test ===" << std::endl;

    if (Services::HasPIManager()) {
      auto* piMgr = Services::PIManager();

      // Test connecting individual devices
      std::cout << "Testing PI device connections:" << std::endl;
      if (piMgr->ConnectDevice("hex-left")) {
        std::cout << "  ✓ Connected hex-left" << std::endl;
      }

      // Check connection status
      bool connected = piMgr->IsDeviceConnected("hex-left");
      std::cout << "  hex-left status: " << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;

      // Test disconnect
      if (piMgr->DisconnectDevice("hex-left")) {
        std::cout << "  ✓ Disconnected hex-left" << std::endl;
      }
    }

    if (Services::HasACSManager()) {
      auto* acsMgr = Services::ACSManager();

      std::cout << "Testing ACS device connections:" << std::endl;
      if (acsMgr->ConnectDevice("gantry-main")) {
        std::cout << "  ✓ Connected gantry-main" << std::endl;
      }

      bool connected = acsMgr->IsDeviceConnected("gantry-main");
      std::cout << "  gantry-main status: " << (connected ? "CONNECTED" : "DISCONNECTED") << std::endl;
    }

    // === UNIVERSAL OPERATIONS TEST ===
    std::cout << "\n=== Universal Operations Test ===" << std::endl;

    // Test manager-level operations
    if (Services::HasPIManager()) {
      auto* piMgr = Services::PIManager();
      std::cout << "PI Manager Type: " << piMgr->GetManagerType() << std::endl;
      std::cout << "PI Manager Initialized: " << (piMgr->IsInitialized() ? "Yes" : "No") << std::endl;
    }

    if (Services::HasACSManager()) {
      auto* acsMgr = Services::ACSManager();
      std::cout << "ACS Manager Type: " << acsMgr->GetManagerType() << std::endl;
      std::cout << "ACS Manager Initialized: " << (acsMgr->IsInitialized() ? "Yes" : "No") << std::endl;
    }

    // === CLEANUP ===
    std::cout << "\n=== Cleanup ===" << std::endl;
    Services::DisconnectAll();
    Services::Clear();
    std::cout << "✓ Cleanup completed" << std::endl;

  }
  catch (const std::exception& e) {
    std::cout << "✗ Error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "\n=== Test Completed Successfully ===" << std::endl;
  return 0;
}