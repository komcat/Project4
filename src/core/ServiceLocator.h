// ============================================================================
// SERVICE LOCATOR - Zero Dependencies, All Services + New Motion Architecture
// ============================================================================

// ServiceLocator.h - Updated version with MotionService and PathExecutor
#pragma once
#include <iostream>
#include <memory>

// Forward declarations - ZERO dependencies!
class PIControllerManagerStandardized;
class ACSControllerManagerStandardized;
class ConfigManager;
class CameraManager;
class EziIOManager;
class CLD101xManager;
class Keithley2400Manager;
class PneumaticManager;
class MachineOperations;

// NEW: Forward declarations for new motion architecture
class MotionService;
class MotionConfigManager;

/**
 * ServiceLocator - Complete Zero Dependencies Service Registry
 *
 * NEW: Now includes MotionService and PathExecutor for clean motion architecture
 * All services (including ConfigManager) are registered here
 * No direct dependencies between classes - everything goes through services
 * Includes all original services for backward compatibility
 */
class ServiceLocator {
public:
  // Singleton access
  static ServiceLocator& Instance() {
    static ServiceLocator instance;
    return instance;
  }

  // Convenience accessor
  static ServiceLocator& Get() { return Instance(); }

  // ========================================================================
  // SERVICE REGISTRATION (called once during startup)
  // ========================================================================

  // NEW: Modern Motion Architecture Services (consistent raw pointers)
  void RegisterMotionService(MotionService* service) {
    motionService = service;
    if (service) std::cout << "✅ MotionService registered" << std::endl;
  }

  void RegisterMotionConfigManager(MotionConfigManager* service) {
    motionConfigManager = service;
    if (service) std::cout << "✅ MotionConfigManager registered" << std::endl;
	}

  // ConfigManager service (zero dependencies!)
  void RegisterConfigManager(ConfigManager* service) {
    configManager = service;
    if (service) std::cout << "✅ ConfigManager Service registered" << std::endl;
  }

  // Original motion managers (kept for backward compatibility)
  void RegisterPI(PIControllerManagerStandardized* service) {
    piManager = service;
    if (service) std::cout << "✅ PI Controller Service registered" << std::endl;
  }

  void RegisterACS(ACSControllerManagerStandardized* service) {
    acsManager = service;
    if (service) std::cout << "✅ ACS Controller Service registered" << std::endl;
  }

  // All original services (backward compatibility)
  void RegisterCamera(CameraManager* service) {
    cameraManager = service;
    if (service) std::cout << "✅ Camera Service registered" << std::endl;
  }

  void RegisterIO(EziIOManager* service) {
    ioManager = service;
    if (service) std::cout << "✅ IO Service registered" << std::endl;
  }

  void RegisterCLD101x(CLD101xManager* service) {
    cldManager = service;
    if (service) std::cout << "✅ CLD101x Service registered" << std::endl;
  }

  void RegisterSMU(Keithley2400Manager* service) {
    smuManager = service;
    if (service) std::cout << "✅ SMU Service registered" << std::endl;
  }

  void RegisterPneumatic(PneumaticManager* service) {
    pneumaticManager = service;
    if (service) std::cout << "✅ Pneumatic Service registered" << std::endl;
  }

  void RegisterMachineOps(MachineOperations* service) {
    machineOperations = service;
    if (service) std::cout << "✅ Machine Operations Service registered" << std::endl;
  }

  // ========================================================================
  // SERVICE ACCESS (used everywhere, zero parameters needed!)
  // ========================================================================

  // NEW: Modern Motion Architecture Access
  MotionService* Motion() const {
    if (!motionService) {
      std::cout << "❌ MotionService not available" << std::endl;
    }
    return motionService;
  }

  MotionConfigManager* MotionConfig() const {
    if (!motionConfigManager) {
      std::cout << "❌ MotionConfigManager not available" << std::endl;
    }
    return motionConfigManager;
	}


  // Core services with enhanced error checking
  ConfigManager* Config() const {
    if (!configManager) {
      std::cout << "❌ ConfigManager not available" << std::endl;
    }
    return configManager;
  }

  // Original motion managers (for backward compatibility)
  PIControllerManagerStandardized* PI() const {
    if (!piManager) {
      std::cout << "❌ PI Controller Manager not available" << std::endl;
    }
    return piManager;
  }

  ACSControllerManagerStandardized* ACS() const {
    if (!acsManager) {
      std::cout << "❌ ACS Controller Manager not available" << std::endl;
    }
    return acsManager;
  }

  // All original service accessors (no error checking for backward compatibility)
  CameraManager* Camera() const { return cameraManager; }
  EziIOManager* IO() const { return ioManager; }
  CLD101xManager* CLD101x() const { return cldManager; }
  Keithley2400Manager* SMU() const { return smuManager; }
  PneumaticManager* Pneumatic() const { return pneumaticManager; }
  MachineOperations* MachineOps() const { return machineOperations; }

  // ========================================================================
  // AVAILABILITY CHECKS (for conditional logic)
  // ========================================================================

  // NEW: Motion architecture availability
  bool HasMotionService() const { return motionService != nullptr; }
	bool HasMotionConfigManager() const { return motionConfigManager != nullptr; }


  // Original availability checks
  bool HasConfig() const { return configManager != nullptr; }
  bool HasPI() const { return piManager != nullptr; }
  bool HasACS() const { return acsManager != nullptr; }
  bool HasCamera() const { return cameraManager != nullptr; }
  bool HasIO() const { return ioManager != nullptr; }
  bool HasCLD101x() const { return cldManager != nullptr; }
  bool HasSMU() const { return smuManager != nullptr; }
  bool HasPneumatic() const { return pneumaticManager != nullptr; }
  bool HasMachineOps() const { return machineOperations != nullptr; }

  // ========================================================================
  // UTILITY METHODS
  // ========================================================================

  // Clear all services (for cleanup)
  void ClearAll() {
    // Clear all services (ServiceLocator doesn't own them, so just nullify)
    motionService = nullptr;
    motionConfigManager = nullptr;
    configManager = nullptr;
    piManager = nullptr;
    acsManager = nullptr;
    cameraManager = nullptr;
    ioManager = nullptr;
    cldManager = nullptr;
    smuManager = nullptr;
    pneumaticManager = nullptr;
    machineOperations = nullptr;
    std::cout << "🔄 All services cleared" << std::endl;
  }

  // Get service status summary
  int GetAvailableServiceCount() const {
    int count = 0;
    if (HasMotionService()) count++;
		if (HasMotionConfigManager()) count++;
    if (HasConfig()) count++;
    if (HasPI()) count++;
    if (HasACS()) count++;
    if (HasCamera()) count++;
    if (HasIO()) count++;
    if (HasCLD101x()) count++;
    if (HasSMU()) count++;
    if (HasPneumatic()) count++;
    if (HasMachineOps()) count++;
    return count;
  }

  // Print status for debugging
  void PrintStatus() const {
    std::cout << "=== Service Status ===" << std::endl;
    std::cout << "--- Modern Motion Architecture ---" << std::endl;
    std::cout << "MotionService: " << (HasMotionService() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
		std::cout << "MotionConfigManager: " << (HasMotionConfigManager() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "--- Core Services ---" << std::endl;
    std::cout << "ConfigManager: " << (HasConfig() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "--- Legacy Motion (Backward Compatibility) ---" << std::endl;
    std::cout << "PI Manager: " << (HasPI() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "ACS Manager: " << (HasACS() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "--- Other Services ---" << std::endl;
    std::cout << "Camera: " << (HasCamera() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "IO: " << (HasIO() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "CLD101x: " << (HasCLD101x() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "SMU: " << (HasSMU() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "Pneumatic: " << (HasPneumatic() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "Machine Ops: " << (HasMachineOps() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "Total Services: " << GetAvailableServiceCount() << std::endl;
  }

  // ========================================================================
  // BATCH OPERATIONS (convenience methods) - DECLARATIONS ONLY
  // ========================================================================

  // NEW: Modern motion system initialization
  bool InitializeModernMotion();

  // Initialize all available motion controllers (legacy)
  bool InitializeAllMotion();

  // Connect all available motion controllers (legacy)
  bool ConnectAllMotion();

  // Disconnect all motion controllers
  void DisconnectAllMotion();

private:
  // Private constructor for singleton
  ServiceLocator() = default;

  // All services as static pointers (not owned by ServiceLocator)
  static MotionService* motionService;
	static MotionConfigManager* motionConfigManager;
  static ConfigManager* configManager;
  static PIControllerManagerStandardized* piManager;
  static ACSControllerManagerStandardized* acsManager;
  static CameraManager* cameraManager;
  static EziIOManager* ioManager;
  static CLD101xManager* cldManager;
  static Keithley2400Manager* smuManager;
  static PneumaticManager* pneumaticManager;
  static MachineOperations* machineOperations;
};

// ========================================================================
// CONVENIENCE MACROS (updated with new services)
// ========================================================================

#define Services ServiceLocator::Get()

// NEW: Motion convenience macros
#define MOTION_SERVICE() Services.Motion()
#define PATH_EXECUTOR() Services.Paths()

// Example usage:
// auto motion = Services.Motion();
// auto paths = Services.Paths();
// auto config = Services.Config();
// auto pi = Services.PI();
// auto camera = Services.Camera();

// ========================================================================
// SAFER SERVICE ACCESS WITH AUTOMATIC NULL CHECKS (Updated)
// ========================================================================

// Template helper for safe service access
template<typename T>
class SafeService {
public:
  SafeService(T* service) : service_(service) {}

  // Implicit bool conversion
  operator bool() const { return service_ != nullptr; }

  // Arrow operator for direct access
  T* operator->() const { return service_; }

  // Get raw pointer
  T* get() const { return service_; }

  // Execute if available
  template<typename Func>
  void IfAvailable(Func&& func) {
    if (service_) {
      func(service_);
    }
  }

private:
  T* service_;
};

// Safe accessors for all services (updated with new services)
namespace SafeServices {
  // NEW: Modern motion services
  inline SafeService<MotionService> Motion() {
    return SafeService<MotionService>(Services.Motion());
  }



  // Original safe accessors
  inline SafeService<ConfigManager> Config() {
    return SafeService<ConfigManager>(Services.Config());
  }

  inline SafeService<PIControllerManagerStandardized> PI() {
    return SafeService<PIControllerManagerStandardized>(Services.PI());
  }

  inline SafeService<ACSControllerManagerStandardized> ACS() {
    return SafeService<ACSControllerManagerStandardized>(Services.ACS());
  }

  inline SafeService<CameraManager> Camera() {
    return SafeService<CameraManager>(Services.Camera());
  }

  inline SafeService<EziIOManager> IO() {
    return SafeService<EziIOManager>(Services.IO());
  }

  inline SafeService<CLD101xManager> CLD101x() {
    return SafeService<CLD101xManager>(Services.CLD101x());
  }

  inline SafeService<Keithley2400Manager> SMU() {
    return SafeService<Keithley2400Manager>(Services.SMU());
  }

  inline SafeService<PneumaticManager> Pneumatic() {
    return SafeService<PneumaticManager>(Services.Pneumatic());
  }

  inline SafeService<MachineOperations> MachineOps() {
    return SafeService<MachineOperations>(Services.MachineOps());
  }
}

// ========================================================================
// USAGE EXAMPLES (Updated)
// ========================================================================

/*
// NEW: Modern motion architecture usage
auto motion = Services.Motion();
if (motion) {
    motion->MoveToNamedPosition("hex-left", "home");
}

auto paths = Services.Paths();
if (paths) {
    paths->ExecutePath("Process_Flow", "start", "end");
}

// Safe service with automatic checking
auto motionSafe = SafeServices::Motion();
if (motionSafe) {
    motionSafe->EmergencyStopAll();
}

// Execute if available
SafeServices::Paths().IfAvailable([](auto* paths) {
    paths->MoveToNode("Process_Flow", "home");
});

// All original services still work exactly the same
auto config = Services.Config();
auto io = Services.IO();
auto smu = Services.SMU();
auto pneumatic = Services.Pneumatic();
*/