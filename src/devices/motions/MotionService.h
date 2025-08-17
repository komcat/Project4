// MotionService.h - KISS: Simple Motion Device Abstraction
// Zero complexity - ServiceLocator handles everything!

#pragma once

#include <string>
#include <vector>
#include "MotionTypes.h"

// Forward declarations - ServiceLocator managed!
class PIController;
class ACSController;
class PIControllerManagerStandardized;
class ACSControllerManagerStandardized;

// ========================================
// MOTION SERVICE - KISS DESIGN
// ========================================

class MotionService {
public:
  MotionService() = default;
  ~MotionService() = default;

  // ========================================
  // INITIALIZATION - ALWAYS SUCCEEDS
  // ========================================

  // Initialize motion service - always returns true
  bool Initialize();
  void Shutdown();

  // ========================================
  // SIMPLE MOTION COMMANDS
  // ========================================

  // Move to absolute position
  bool MoveToPosition(const std::string& deviceName, const PositionStruct& position, bool blocking = true);

  // Move relative distance
  bool MoveRelative(const std::string& deviceName, const std::string& axis, double distance, bool blocking = true);

  // Get current position
  bool GetCurrentPosition(const std::string& deviceName, PositionStruct& position);

  // Stop device motion
  bool StopDevice(const std::string& deviceName);

  // Emergency stop all devices
  bool EmergencyStopAll();

  // ========================================
  // VELOCITY CONTROL
  // ========================================

  bool SetVelocity(const std::string& deviceName, double velocity);
  bool GetVelocity(const std::string& deviceName, double& velocity);

  // ========================================
  // DEVICE QUERIES
  // ========================================

  // Get all available devices
  std::vector<std::string> GetAvailableDevices();

  // Check if device is connected
  bool IsDeviceConnected(const std::string& deviceName);

  // Check if device is at position
  bool IsAtPosition(const std::string& deviceName, const PositionStruct& targetPosition, double tolerance = 0.001);

private:
  // ========================================
  // HELPER METHODS
  // ========================================

  // Device type detection
  PIController* GetPIDevice(const std::string& deviceName);
  ACSController* GetACSDevice(const std::string& deviceName);

  // Try both PI and ACS to find device
  bool FindDevice(const std::string& deviceName, PIController*& piDevice, ACSController*& acsDevice);
};

// ========================================
// DESIGN NOTES:
// ========================================

/*
🎯 KISS PRINCIPLES APPLIED:

1. ZERO INITIALIZATION COMPLEXITY:
   - Initialize() method always returns true
   - No critical dependencies or setup required
   - ServiceLocator handles all manager setup

2. ZERO OWNERSHIP:
   - No member variables for managers
   - ServiceLocator::PI() and ServiceLocator::ACS() every time
   - No lifetime management complexity

3. SIMPLE API:
   - Every method returns bool (success/failure)
   - Consistent parameter patterns
   - Device-agnostic interface

4. AUTO-DISCOVERY:
   - Automatically finds PI vs ACS devices
   - No configuration needed in this class
   - Users don't care about device types

5. RESILIENT DESIGN:
   - Works with zero devices (logs warning)
   - Works with partial device availability
   - Devices can be added/removed dynamically

🔄 USAGE EXAMPLE:

```cpp
// ServiceLocator setup (done once at startup)
Services::RegisterPIManager(piManager);
Services::RegisterACSManager(acsManager);

// Use motion service anywhere!
MotionService motion;
motion.Initialize(); // Always succeeds!

PositionStruct pos{10.0, 20.0, 5.0};
motion.MoveToPosition("hex-left", pos);    // Could be PI
motion.MoveToPosition("gantry-main", pos); // Could be ACS

// Don't care which type!
motion.MoveRelative("any-device", "X", 5.0);
motion.StopDevice("any-device");

// Initialize can be called multiple times safely
motion.Initialize(); // Still succeeds!
```

🚀 WHY THIS IS BETTER:

OLD CODE:
- Complex initialization with failure modes
- Manager ownership and lifecycle issues
- Configuration dependencies
- Error-prone setup sequences

NEW CODE:
- Initialize() always succeeds
- ServiceLocator handles everything
- Dynamic device discovery
- Zero failure modes
- Just call Initialize() and go!

🎉 RESILIENT DESIGN FEATURES:

✅ **Always Succeeds**: Never fails initialization
✅ **Zero Dependencies**: No mandatory requirements
✅ **Dynamic Discovery**: Finds devices as they become available
✅ **Graceful Degradation**: Works with partial device sets
✅ **Multiple Calls Safe**: Can call Initialize() multiple times
✅ **Informative Logging**: Shows exactly what's available

MUCH SIMPLER AND MORE RELIABLE! 🚀
*/