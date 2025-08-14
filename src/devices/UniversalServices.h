// src/devices/UniversalServices.h
// Simplified version for testing with PI and ACS managers only
#pragma once
#include "IDeviceManagerInterface.h"
#include <string>
#include <iostream>
#include <unordered_map>

// Forward declarations - ZERO dependencies!
class PIController;
class ACSController;

/**
 * Universal Services Locator - Testing Version
 * Only supports PI and ACS managers for now
 */
class Services {
private:
  // Simple static storage for the two managers we're testing
  static IDeviceManagerInterface<PIController>* s_piManager;
  static IDeviceManagerInterface<ACSController>* s_acsManager;

public:
  // === REGISTRATION (called once at startup) ===
  static void RegisterPIManager(IDeviceManagerInterface<PIController>* manager) {
    s_piManager = manager;
  }

  static void RegisterACSManager(IDeviceManagerInterface<ACSController>* manager) {
    s_acsManager = manager;
  }

  // === MANAGER ACCESS ===
  static IDeviceManagerInterface<PIController>* PIManager() {
    return s_piManager;
  }

  static IDeviceManagerInterface<ACSController>* ACSManager() {
    return s_acsManager;
  }

  // === CONVENIENCE DEVICE ACCESS ===
  static PIController* GetPIDevice(const std::string& deviceName) {
    return s_piManager ? s_piManager->GetDevice(deviceName) : nullptr;
  }

  static ACSController* GetACSDevice(const std::string& deviceName) {
    return s_acsManager ? s_acsManager->GetDevice(deviceName) : nullptr;
  }

  // === AVAILABILITY CHECKS ===
  static bool HasPIManager() { return s_piManager != nullptr; }
  static bool HasACSManager() { return s_acsManager != nullptr; }

  // === UNIVERSAL OPERATIONS ===
  static bool InitializeAll() {
    bool allSuccess = true;

    if (s_piManager && !s_piManager->Initialize()) {
      allSuccess = false;
    }

    if (s_acsManager && !s_acsManager->Initialize()) {
      allSuccess = false;
    }

    return allSuccess;
  }

  static bool ConnectAll() {
    bool allSuccess = true;

    if (s_piManager && !s_piManager->ConnectAll()) {
      allSuccess = false;
    }

    if (s_acsManager && !s_acsManager->ConnectAll()) {
      allSuccess = false;
    }

    return allSuccess;
  }

  static void DisconnectAll() {
    if (s_piManager) {
      s_piManager->DisconnectAll();
    }

    if (s_acsManager) {
      s_acsManager->DisconnectAll();
    }
  }

  // === UTILITY ===
  static void Clear() {
    s_piManager = nullptr;
    s_acsManager = nullptr;
  }

  static int GetManagerCount() {
    int count = 0;
    if (s_piManager) count++;
    if (s_acsManager) count++;
    return count;
  }

  // === TESTING/DEBUG HELPERS ===
  static void PrintStatus() {
    std::cout << "=== Services Status ===" << std::endl;
    std::cout << "PI Manager: " << (HasPIManager() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "ACS Manager: " << (HasACSManager() ? "REGISTERED" : "NOT REGISTERED") << std::endl;
    std::cout << "Total Managers: " << GetManagerCount() << std::endl;

    if (HasPIManager()) {
      auto* pi = PIManager();
      std::cout << "  PI: " << pi->GetDeviceCount() << " devices, "
        << (pi->IsInitialized() ? "initialized" : "not initialized") << std::endl;
    }

    if (HasACSManager()) {
      auto* acs = ACSManager();
      std::cout << "  ACS: " << acs->GetDeviceCount() << " devices, "
        << (acs->IsInitialized() ? "initialized" : "not initialized") << std::endl;
    }
  }
};

// Static storage definitions
IDeviceManagerInterface<PIController>* Services::s_piManager = nullptr;
IDeviceManagerInterface<ACSController>* Services::s_acsManager = nullptr;