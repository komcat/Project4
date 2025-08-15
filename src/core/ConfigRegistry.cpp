#include "ConfigRegistry.h"
#include <chrono>
#include <iomanip>
#include <sstream>

// Static member initialization
std::unordered_set<std::string> ConfigRegistry::s_knownFiles;

// Initialize known configuration files
void ConfigRegistry::InitializeKnownFiles() {
  if (!s_knownFiles.empty()) return; // Already initialized

  s_knownFiles = {
      Files::CAMERA_CALIBRATION,
      Files::CAMERA_CONFIG,
      Files::CAMERA_EXPOSURE,
      Files::CAMERA_OFFSET,
      Files::DATA_SERVER,
      Files::IO_CONFIG,
      Files::MOTION_DEVICES,
      Files::MOTION_GRAPH,
      Files::MOTION_POSITIONS,
      Files::SMU_CONFIG,
      Files::TRANSFORMATION_MATRIX,
      Files::VISION_CIRCLE
  };
}

// Load all known configurations
bool ConfigRegistry::LoadAllKnownConfigs() {
  InitializeKnownFiles();

  auto& configManager = ConfigManager::Instance();
  bool allSuccess = true;
  int successCount = 0;

  for (const auto& filename : s_knownFiles) {
    if (configManager.LoadConfig(filename)) {
      successCount++;
    }
    else {
      allSuccess = false;
    }
  }

  std::cout << "[ConfigRegistry] Loaded " << successCount << " out of "
    << s_knownFiles.size() << " known configurations" << std::endl;

  return allSuccess;
}

// Get all configuration files
std::vector<std::string> ConfigRegistry::GetAllConfigFiles() {
  InitializeKnownFiles();
  return std::vector<std::string>(s_knownFiles.begin(), s_knownFiles.end());
}

// Check if file is known
bool ConfigRegistry::IsKnownConfig(const std::string& filename) {
  InitializeKnownFiles();
  return s_knownFiles.find(filename) != s_knownFiles.end();
}

// Load camera-related configurations
bool ConfigRegistry::LoadCameraConfigs() {
  auto& configManager = ConfigManager::Instance();
  bool success = true;

  success &= configManager.LoadConfig(Files::CAMERA_CONFIG);
  success &= configManager.LoadConfig(Files::CAMERA_CALIBRATION);
  success &= configManager.LoadConfig(Files::CAMERA_EXPOSURE);
  success &= configManager.LoadConfig(Files::CAMERA_OFFSET);

  return success;
}

// Load motion-related configurations
bool ConfigRegistry::LoadMotionConfigs() {
  auto& configManager = ConfigManager::Instance();
  bool success = true;

  success &= configManager.LoadConfig(Files::MOTION_DEVICES);
  success &= configManager.LoadConfig(Files::MOTION_GRAPH);
  success &= configManager.LoadConfig(Files::MOTION_POSITIONS);
  success &= configManager.LoadConfig(Files::TRANSFORMATION_MATRIX);

  return success;
}

// Load IO configurations
bool ConfigRegistry::LoadIOConfigs() {
  auto& configManager = ConfigManager::Instance();
  return configManager.LoadConfig(Files::IO_CONFIG);
}

// Load vision configurations
bool ConfigRegistry::LoadVisionConfigs() {
  auto& configManager = ConfigManager::Instance();
  return configManager.LoadConfig(Files::VISION_CIRCLE);
}

// Quick access methods
nlohmann::json ConfigRegistry::GetCameraConfig() {
  return ConfigManager::Instance().GetConfig(Files::CAMERA_CONFIG);
}

nlohmann::json ConfigRegistry::GetMotionDevices() {
  return ConfigManager::Instance().GetConfig(Files::MOTION_DEVICES);
}

nlohmann::json ConfigRegistry::GetMotionPositions() {
  return ConfigManager::Instance().GetConfig(Files::MOTION_POSITIONS);
}

nlohmann::json ConfigRegistry::GetIOConfig() {
  return ConfigManager::Instance().GetConfig(Files::IO_CONFIG);
}

// Validate all configurations
bool ConfigRegistry::ValidateAllConfigs() {
  InitializeKnownFiles();

  auto& configManager = ConfigManager::Instance();
  bool allValid = true;
  int validCount = 0;

  for (const auto& filename : s_knownFiles) {
    if (configManager.ValidateConfig(filename)) {
      validCount++;
    }
    else {
      allValid = false;
      std::cerr << "[ConfigRegistry] Invalid config: " << filename << std::endl;
    }
  }

  std::cout << "[ConfigRegistry] " << validCount << " out of "
    << s_knownFiles.size() << " configurations are valid" << std::endl;

  return allValid;
}

// Backup all configurations
bool ConfigRegistry::BackupAllConfigs(const std::string& backupSuffix) {
  auto& configManager = ConfigManager::Instance();

  // Generate timestamp if no suffix provided
  std::string suffix = backupSuffix;
  if (suffix.empty()) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    suffix = ss.str();
  }

  bool allSuccess = true;
  int backupCount = 0;

  for (const auto& filename : GetAllConfigFiles()) {
    auto config = configManager.GetConfig(filename);
    if (!config.empty()) {
      std::string backupFilename = filename + ".backup_" + suffix;
      if (configManager.SaveConfig(backupFilename, config)) {
        backupCount++;
      }
      else {
        allSuccess = false;
      }
    }
  }

  std::cout << "[ConfigRegistry] Backed up " << backupCount
    << " configuration files with suffix: " << suffix << std::endl;

  return allSuccess;
}

// Restore configurations from backup
bool ConfigRegistry::RestoreAllConfigs(const std::string& backupSuffix) {
  auto& configManager = ConfigManager::Instance();

  bool allSuccess = true;
  int restoredCount = 0;

  for (const auto& filename : GetAllConfigFiles()) {
    std::string backupFilename = filename + ".backup_" + backupSuffix;

    auto backupConfig = configManager.GetConfig(backupFilename);
    if (!backupConfig.empty()) {
      if (configManager.SaveConfig(filename, backupConfig)) {
        restoredCount++;
      }
      else {
        allSuccess = false;
      }
    }
  }

  std::cout << "[ConfigRegistry] Restored " << restoredCount
    << " configuration files from backup: " << backupSuffix << std::endl;

  return allSuccess;
}

// === Config namespace implementations ===

// Camera configuration helpers
std::vector<Config::Camera::CameraInfo> Config::Camera::GetAllCameras() {
  auto config = ConfigRegistry::GetCameraConfig();
  std::vector<CameraInfo> cameras;

  if (config.contains("cameras") && config["cameras"].is_array()) {
    for (const auto& cam : config["cameras"]) {
      CameraInfo info;
      info.id = ConfigHelper::GetValue<std::string>(cam, "id", "");
      info.display_name = ConfigHelper::GetValue<std::string>(cam, "display_name", "");
      info.ip_address = ConfigHelper::GetValue<std::string>(cam, "ip_address", "");
      info.port = ConfigHelper::GetValue<int>(cam, "port", 0);
      info.enabled = ConfigHelper::GetValue<bool>(cam, "enabled", false);
      info.auto_connect = ConfigHelper::GetValue<bool>(cam, "auto_connect", false);
      info.description = ConfigHelper::GetValue<std::string>(cam, "description", "");
      info.exposure_time = ConfigHelper::GetValue<int>(cam, "exposure_time", 1000);
      info.gain = ConfigHelper::GetValue<double>(cam, "gain", 1.0);

      cameras.push_back(info);
    }
  }

  return cameras;
}

Config::Camera::CameraInfo Config::Camera::GetCamera(const std::string& id) {
  auto cameras = GetAllCameras();
  for (const auto& cam : cameras) {
    if (cam.id == id) {
      return cam;
    }
  }
  return CameraInfo{}; // Return empty if not found
}

bool Config::Camera::IsCameraEnabled(const std::string& id) {
  auto camera = GetCamera(id);
  return camera.enabled;
}

double Config::Camera::GetPixelToMmX() {
  auto config = ConfigManager::Instance().GetConfig(ConfigRegistry::Files::CAMERA_CALIBRATION);
  return ConfigHelper::GetValue<double>(config, "pixelToMillimeterFactorX", 0.00248);
}

double Config::Camera::GetPixelToMmY() {
  auto config = ConfigManager::Instance().GetConfig(ConfigRegistry::Files::CAMERA_CALIBRATION);
  return ConfigHelper::GetValue<double>(config, "pixelToMillimeterFactorY", 0.00252);
}

// Motion configuration helpers
std::vector<Config::Motion::DeviceInfo> Config::Motion::GetAllDevices() {
  auto config = ConfigRegistry::GetMotionDevices();
  std::vector<DeviceInfo> devices;

  if (config.contains("MotionDevices")) {
    for (const auto& [name, device] : config["MotionDevices"].items()) {
      DeviceInfo info;
      info.id = ConfigHelper::GetValue<int>(device, "Id", 0);
      info.name = name;
      info.ipAddress = ConfigHelper::GetValue<std::string>(device, "IpAddress", "");
      info.port = ConfigHelper::GetValue<int>(device, "Port", 0);
      info.isEnabled = ConfigHelper::GetValue<bool>(device, "IsEnabled", false);
      info.installAxes = ConfigHelper::GetValue<std::string>(device, "installAxes", "");
      info.typeController = ConfigHelper::GetValue<std::string>(device, "typeController", "");

      devices.push_back(info);
    }
  }

  return devices;
}

Config::Motion::DeviceInfo Config::Motion::GetDevice(const std::string& name) {
  auto devices = GetAllDevices();
  for (const auto& device : devices) {
    if (device.name == name) {
      return device;
    }
  }
  return DeviceInfo{}; // Return empty if not found
}

Config::Motion::Position Config::Motion::GetPosition(const std::string& device, const std::string& positionName) {
  auto config = ConfigRegistry::GetMotionPositions();
  Position pos = { 0, 0, 0, 0, 0, 0 };

  if (config.contains(device) && config[device].contains(positionName)) {
    auto posData = config[device][positionName];
    pos.x = ConfigHelper::GetValue<double>(posData, "x", 0.0);
    pos.y = ConfigHelper::GetValue<double>(posData, "y", 0.0);
    pos.z = ConfigHelper::GetValue<double>(posData, "z", 0.0);
    pos.u = ConfigHelper::GetValue<double>(posData, "u", 0.0);
    pos.v = ConfigHelper::GetValue<double>(posData, "v", 0.0);
    pos.w = ConfigHelper::GetValue<double>(posData, "w", 0.0);
  }

  return pos;
}

bool Config::Motion::SetPosition(const std::string& device, const std::string& positionName, const Position& pos) {
  auto& configManager = ConfigManager::Instance();
  auto config = configManager.GetConfig(ConfigRegistry::Files::MOTION_POSITIONS);

  // Create structure if it doesn't exist
  if (!config.contains(device)) {
    config[device] = nlohmann::json::object();
  }

  // Set position data
  config[device][positionName] = {
      {"x", pos.x},
      {"y", pos.y},
      {"z", pos.z},
      {"u", pos.u},
      {"v", pos.v},
      {"w", pos.w}
  };

  // Update in cache and save
  configManager.SetConfig(ConfigRegistry::Files::MOTION_POSITIONS, config);
  return configManager.SaveConfig(ConfigRegistry::Files::MOTION_POSITIONS);
}

// Hardware offset helpers
Config::Hardware::Offset Config::Hardware::GetOffset(const std::string& hardwareName) {
  auto config = ConfigManager::Instance().GetConfig(ConfigRegistry::Files::CAMERA_OFFSET);
  Offset offset = { 0, 0, 0, "", "" };

  if (config.contains("hardware_offsets") && config["hardware_offsets"].contains(hardwareName)) {
    auto offsetData = config["hardware_offsets"][hardwareName];

    if (offsetData.contains("coordinates")) {
      auto coords = offsetData["coordinates"];
      offset.x = ConfigHelper::GetValue<double>(coords, "x", 0.0);
      offset.y = ConfigHelper::GetValue<double>(coords, "y", 0.0);
      offset.z = ConfigHelper::GetValue<double>(coords, "z", 0.0);
    }

    offset.description = ConfigHelper::GetValue<std::string>(offsetData, "description", "");
    offset.lastCalibrated = ConfigHelper::GetValue<std::string>(offsetData, "last_calibrated", "");
  }

  return offset;
}

bool Config::Hardware::SetOffset(const std::string& hardwareName, const Offset& offset) {
  auto& configManager = ConfigManager::Instance();
  auto config = configManager.GetConfig(ConfigRegistry::Files::CAMERA_OFFSET);

  // Create structure if it doesn't exist
  if (!config.contains("hardware_offsets")) {
    config["hardware_offsets"] = nlohmann::json::object();
  }

  // Set offset data
  config["hardware_offsets"][hardwareName] = {
      {"coordinates", {
          {"x", offset.x},
          {"y", offset.y},
          {"z", offset.z}
      }},
      {"description", offset.description},
      {"last_calibrated", offset.lastCalibrated},
      {"units", "mm"}
  };

  // Update in cache and save
  configManager.SetConfig(ConfigRegistry::Files::CAMERA_OFFSET, config);
  return configManager.SaveConfig(ConfigRegistry::Files::CAMERA_OFFSET);
}

// IO configuration helpers
std::vector<Config::IO::PneumaticSlide> Config::IO::GetPneumaticSlides() {
  auto config = ConfigRegistry::GetIOConfig();
  std::vector<PneumaticSlide> slides;

  if (config.contains("pneumaticSlides") && config["pneumaticSlides"].is_array()) {
    for (const auto& slide : config["pneumaticSlides"]) {
      PneumaticSlide info;
      info.name = ConfigHelper::GetValue<std::string>(slide, "name", "");
      info.timeoutMs = ConfigHelper::GetValue<int>(slide, "timeoutMs", 5000);

      if (slide.contains("output")) {
        auto output = slide["output"];
        info.outputDevice = ConfigHelper::GetValue<std::string>(output, "deviceName", "");
        info.outputPin = ConfigHelper::GetValue<std::string>(output, "pinName", "");
      }

      if (slide.contains("extendedInput")) {
        auto extInput = slide["extendedInput"];
        info.extendedInputDevice = ConfigHelper::GetValue<std::string>(extInput, "deviceName", "");
        info.extendedInputPin = ConfigHelper::GetValue<std::string>(extInput, "pinName", "");
      }

      if (slide.contains("retractedInput")) {
        auto retInput = slide["retractedInput"];
        info.retractedInputDevice = ConfigHelper::GetValue<std::string>(retInput, "deviceName", "");
        info.retractedInputPin = ConfigHelper::GetValue<std::string>(retInput, "pinName", "");
      }

      slides.push_back(info);
    }
  }

  return slides;
}

Config::IO::PneumaticSlide Config::IO::GetPneumaticSlide(const std::string& name) {
  auto slides = GetPneumaticSlides();
  for (const auto& slide : slides) {
    if (slide.name == name) {
      return slide;
    }
  }
  return PneumaticSlide{}; // Return empty if not found
}