// ServiceLocator.cpp - Complete Implementation with New Motion Architecture
#include "ServiceLocator.h"

// Include the full class definitions here - NOT in the header
#include "../devices/motions/PIControllerManagerStandardized.h"
#include "../devices/motions/ACSControllerManagerStandardized.h"
#include "ConfigManager.h"

// NEW: Include the new motion architecture classes
#include "../devices/motions/MotionService.h"
#include "../devices/motions/MotionConfigManager.h"


// Note: Only include headers for services you actually need in the batch operations
// Other services (Camera, IO, etc.) are only stored as pointers, so forward declarations are sufficient


// ========================================================================
// ORIGINAL BATCH OPERATIONS (Legacy Motion Controllers)
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

  if (HasMotionConfigManager())
  {
    std::cout << "Initializing MotionConfigManager..." << std::endl;
    auto motionConfigManager = MotionConfig();
    if (motionConfigManager && !motionConfigManager->Initialize()) {
      std::cout << "❌ MotionConfigManager initialization failed" << std::endl;
      allSuccess = false;
    }
    else if (motionConfigManager) {
      std::cout << "✅ MotionConfigManager initialized" << std::endl;
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
  // Shutdown modern motion architecture first
  if (HasMotionService()) {
    std::cout << "Shutting down MotionService..." << std::endl;
    auto motion = Motion();
    if (motion) {
      motion->Shutdown();
    }
  }

  // Original motion controller disconnection
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
// STATIC STORAGE DEFINITIONS - ALL SERVICES (Raw Pointers)
// ========================================================================

// Define static storage for all services as raw pointers
MotionService* ServiceLocator::motionService = nullptr;
MotionConfigManager* ServiceLocator::motionConfigManager = nullptr;
ConfigManager* ServiceLocator::configManager = nullptr;
PIControllerManagerStandardized* ServiceLocator::piManager = nullptr;
ACSControllerManagerStandardized* ServiceLocator::acsManager = nullptr;
CameraManager* ServiceLocator::cameraManager = nullptr;
EziIOManager* ServiceLocator::ioManager = nullptr;
CLD101xManager* ServiceLocator::cldManager = nullptr;
Keithley2400Manager* ServiceLocator::smuManager = nullptr;
PneumaticManager* ServiceLocator::pneumaticManager = nullptr;
MachineOperations* ServiceLocator::machineOperations = nullptr;