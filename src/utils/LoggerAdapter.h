// src/utils/LoggerAdapter.h
// Adapter to bridge between your emoji Logger and ConfigManager's ILogger interface
#pragma once

#include "core/ConfigManager.h"  // For ILogger interface
#include "Logger.h"              // Your existing emoji logger
#include <string>
#include <codecvt>
#include <locale>

/**
 * LoggerAdapter - Bridges your emoji Logger with ConfigManager's ILogger interface
 *
 * This allows ConfigManager to use your beautiful emoji logger while maintaining
 * the simple ILogger interface.
 */
class LoggerAdapter : public ILogger {
public:
  LoggerAdapter() = default;
  ~LoggerAdapter() override = default;

  void LogInfo(const std::string& message) override {
    Logger::Info(StringToWString(message));
  }

  void LogError(const std::string& message) override {
    Logger::Error(StringToWString(message));
  }

  void LogWarning(const std::string& message) override {
    Logger::Warning(StringToWString(message));
  }

  // Additional method to use your success logger
  void LogSuccess(const std::string& message) {
    Logger::Success(StringToWString(message));
  }

private:
  // Convert std::string to std::wstring for your Logger
  std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();

    try {
      // Use codecvt for proper conversion
      std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
      return converter.from_bytes(str);
    }
    catch (...) {
      // Fallback: simple assignment (may not handle Unicode properly)
      return std::wstring(str.begin(), str.end());
    }
  }
};

/**
 * Enhanced ConfigManager Logger
 *
 * Adds convenience methods with emoji support for ConfigManager operations
 */
class ConfigLogger {
public:
  static void ConfigLoaded(const std::string& filename) {
    Logger::Success(L"📁 Config loaded: " + StringToWString(filename));
  }

  static void ConfigSaved(const std::string& filename) {
    Logger::Success(L"💾 Config saved: " + StringToWString(filename));
  }

  static void ConfigError(const std::string& filename, const std::string& error) {
    Logger::Error(L"❌ Config error in " + StringToWString(filename) + L": " + StringToWString(error));
  }

  static void ConfigValidated(const std::string& filename) {
    Logger::Success(L"✅ Config validated: " + StringToWString(filename));
  }

  static void ConfigBackup(const std::string& backupName) {
    Logger::Info(L"🔄 Config backup created: " + StringToWString(backupName));
  }

  static void ConfigCacheCleared() {
    Logger::Info(L"🧹 Config cache cleared");
  }

  static void ConfigTestStart() {
    Logger::Info(L"🚀 Starting ConfigManager tests...");
  }

  static void ConfigTestEnd(bool success) {
    if (success) {
      Logger::Success(L"✅ All ConfigManager tests passed!");
    }
    else {
      Logger::Error(L"❌ Some ConfigManager tests failed!");
    }
  }

  static void MotionDeviceFound(const std::string& deviceName, const std::string& type, bool enabled) {
    std::wstring status = enabled ? L"🟢 ENABLED" : L"🔴 DISABLED";
    Logger::Info(L"🤖 Motion device: " + StringToWString(deviceName) +
      L" [" + StringToWString(type) + L"] " + status);
  }

  static void PositionLoaded(const std::string& device, const std::string& position) {
    Logger::Info(L"📍 Position loaded: " + StringToWString(device) + L"::" + StringToWString(position));
  }

  static void PositionSaved(const std::string& device, const std::string& position) {
    Logger::Success(L"💾 Position saved: " + StringToWString(device) + L"::" + StringToWString(position));
  }
  static std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();

    try {
      std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
      return converter.from_bytes(str);
    }
    catch (...) {
      return std::wstring(str.begin(), str.end());
    }
  }
private:

};