// ServiceLocator.cpp - Complete Implementation with Full Type Definitions
#include "ServiceLocator.h"

// Include the full class definitions here - NOT in the header
#include "../devices/motions/PIControllerManagerStandardized.h"
#include "../devices/motions/ACSControllerManagerStandardized.h"
#include "ConfigManager.h"

// Note: Only include headers for services you actually need in the batch operations
// Other services (Camera, IO, etc.) are only stored as pointers, so forward declarations are sufficient

// ========================================================================
// BATCH OPERATIONS IMPLEMENTATION (Motion Controllers Only)
// ========================================================================

bool ServiceLocator::InitializeAllMotion() {
  bool allSuccess = true;

  if (HasPI()) {
    std::cout << "Initializing PI controllers..." << std::endl;
    auto piManager = PI();
    if (piManager && !piManager->Initialize()) {
      std::cout << "❌ PI initialization failed" << std::endl;
      allSuccess = false;
    }
    else if (piManager) {
      std::cout << "✅ PI initialized" << std::endl;
    }
  }

  if (HasACS()) {
    std::cout << "Initializing ACS controllers..." << std::endl;
    auto acsManager = ACS();
    if (acsManager && !acsManager->Initialize()) {
      std::cout << "❌ ACS initialization failed" << std::endl;
      allSuccess = false;
    }
    else if (acsManager) {
      std::cout << "✅ ACS initialized" << std::endl;
    }
  }

  return allSuccess;
}

bool ServiceLocator::ConnectAllMotion() {
  bool allSuccess = true;

  if (HasPI()) {
    std::cout << "Connecting PI controllers..." << std::endl;
    auto piManager = PI();
    if (piManager && !piManager->ConnectAll()) {
      std::cout << "⚠️ Some PI controllers failed to connect" << std::endl;
      allSuccess = false;
    }
    else if (piManager) {
      std::cout << "✅ PI controllers connected" << std::endl;
    }
  }

  if (HasACS()) {
    std::cout << "Connecting ACS controllers..." << std::endl;
    auto acsManager = ACS();
    if (acsManager && !acsManager->ConnectAll()) {
      std::cout << "⚠️ Some ACS controllers failed to connect" << std::endl;
      allSuccess = false;
    }
    else if (acsManager) {
      std::cout << "✅ ACS controllers connected" << std::endl;
    }
  }

  return allSuccess;
}

void ServiceLocator::DisconnectAllMotion() {
  if (HasPI()) {
    std::cout << "Disconnecting PI controllers..." << std::endl;
    auto piManager = PI();
    if (piManager) {
      piManager->DisconnectAll();
    }
  }

  if (HasACS()) {
    std::cout << "Disconnecting ACS controllers..." << std::endl;
    auto acsManager = ACS();
    if (acsManager) {
      acsManager->DisconnectAll();
    }
  }
}

// ========================================================================
// STATIC STORAGE DEFINITIONS - ALL SERVICES
// ========================================================================

// Define all static member variables
ConfigManager* ServiceLocator::configManager = nullptr;
PIControllerManagerStandardized* ServiceLocator::piManager = nullptr;
ACSControllerManagerStandardized* ServiceLocator::acsManager = nullptr;
CameraManager* ServiceLocator::cameraManager = nullptr;
EziIOManager* ServiceLocator::ioManager = nullptr;
CLD101xManager* ServiceLocator::cldManager = nullptr;
Keithley2400Manager* ServiceLocator::smuManager = nullptr;
PneumaticManager* ServiceLocator::pneumaticManager = nullptr;
MachineOperations* ServiceLocator::machineOperations = nullptr;