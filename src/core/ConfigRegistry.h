#pragma once

#include "ConfigManager.h"
#include <vector>
#include <string>
#include <unordered_set>

/**
 * Configuration Registry
 *
 * Defines all known configuration files and provides type-safe access.
 * Keeps everything simple and organized.
 */
class ConfigRegistry {
public:
  // All known configuration files
  struct Files {
    static constexpr const char* CAMERA_CALIBRATION = "camera_calibration.json";
    static constexpr const char* CAMERA_CONFIG = "camera_config.json";
    static constexpr const char* CAMERA_EXPOSURE = "camera_exposure_config.json";
    static constexpr const char* CAMERA_OFFSET = "camera_to_object_offset.json";
    static constexpr const char* DATA_SERVER = "DataServerConfig.json";
    static constexpr const char* IO_CONFIG = "IOConfig.json";
    static constexpr const char* MOTION_DEVICES = "motion_config_devices.json";
    static constexpr const char* MOTION_GRAPH = "motion_config_graph.json";
    static constexpr const char* MOTION_POSITIONS = "motion_config_positions.json";
    static constexpr const char* SMU_CONFIG = "smu_config.json";
    static constexpr const char* TRANSFORMATION_MATRIX = "transformation_matrix.json";
    static constexpr const char* VISION_CIRCLE = "vision_circle_params.json";
  };

  // Initialize all known configurations
  static bool LoadAllKnownConfigs();

  // Get list of all known configuration files
  static std::vector<std::string> GetAllConfigFiles();

  // Check if a file is a known configuration
  static bool IsKnownConfig(const std::string& filename);

  // Load specific configuration groups
  static bool LoadCameraConfigs();
  static bool LoadMotionConfigs();
  static bool LoadIOConfigs();
  static bool LoadVisionConfigs();

  // Quick access to common configurations
  static nlohmann::json GetCameraConfig();
  static nlohmann::json GetMotionDevices();
  static nlohmann::json GetMotionPositions();
  static nlohmann::json GetIOConfig();

  // Configuration validation
  static bool ValidateAllConfigs();

  // Configuration backup/restore
  static bool BackupAllConfigs(const std::string& backupSuffix = "");
  static bool RestoreAllConfigs(const std::string& backupSuffix);

private:
  static std::unordered_set<std::string> s_knownFiles;
  static void InitializeKnownFiles();
};

/**
 * Type-safe configuration accessors
 *
 * Provides strongly-typed access to specific configuration values
 */
namespace Config {

  // Camera configuration helpers
  namespace Camera {
    struct CameraInfo {
      std::string id;
      std::string display_name;
      std::string ip_address;
      int port;
      bool enabled;
      bool auto_connect;
      std::string description;
      int exposure_time;
      double gain;
    };

    std::vector<CameraInfo> GetAllCameras();
    CameraInfo GetCamera(const std::string& id);
    bool IsCameraEnabled(const std::string& id);

    // Calibration
    double GetPixelToMmX();
    double GetPixelToMmY();
  }

  // Motion configuration helpers
  namespace Motion {
    struct DeviceInfo {
      int id;
      std::string name;
      std::string ipAddress;
      int port;
      bool isEnabled;
      std::string installAxes;
      std::string typeController;
    };

    struct Position {
      double x, y, z, u, v, w;
    };

    std::vector<DeviceInfo> GetAllDevices();
    DeviceInfo GetDevice(const std::string& name);
    Position GetPosition(const std::string& device, const std::string& positionName);
    bool SetPosition(const std::string& device, const std::string& positionName, const Position& pos);
  }

  // IO configuration helpers
  namespace IO {
    struct PneumaticSlide {
      std::string name;
      std::string outputDevice;
      std::string outputPin;
      std::string extendedInputDevice;
      std::string extendedInputPin;
      std::string retractedInputDevice;
      std::string retractedInputPin;
      int timeoutMs;
    };

    std::vector<PneumaticSlide> GetPneumaticSlides();
    PneumaticSlide GetPneumaticSlide(const std::string& name);
  }

  // Hardware offset helpers
  namespace Hardware {
    struct Offset {
      double x, y, z;
      std::string description;
      std::string lastCalibrated;
    };

    Offset GetOffset(const std::string& hardwareName);
    bool SetOffset(const std::string& hardwareName, const Offset& offset);
  }
}