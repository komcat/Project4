// Application.h - Clean and Simple with Zero Dependencies
#pragma once

#include <memory>
#include <atomic>
#include <SDL.h>
#include "Window.h"
#include "../ui/FontManager.h"
#include "../ui/UIRenderer.h"
// NO MORE ConfigManager.h include - it's accessed via ServiceLocator!
#include "../utils/LoggerAdapter.h"

// Forward declarations for managers - NO direct dependencies!
class PIControllerManagerStandardized;
class ACSControllerManagerStandardized;
// ConfigManager is accessed via ServiceLocator, no forward declaration needed

/**
 * Application - Zero Dependencies Design
 *
 * All services (including ConfigManager) are accessed via ServiceLocator
 * No direct dependencies between Application and any specific managers
 */
class Application {
public:
  Application();
  ~Application();

  // Main application flow - KISS principle
  bool Initialize();
  void Run();
  void Cleanup();

private:
  // ========================================================================
  // INITIALIZATION PHASE
  // ========================================================================
  bool InitializeSDL();
  bool InitializeImGui();
  bool CreateWindows();
  void InitializeImGuiForWindow(Window& window);
  void SetupOpenGLAttributes();

  // ========================================================================
  // RUNTIME PHASE
  // ========================================================================
  // Step 1: Show home page immediately
  void RenderInitialHomePage();

  // Step 2: Initialize ALL services (ConfigManager + Motion managers)
  void InitializeServices();  // Renamed from InitializeMotionManagers

  // Step 3: Main loop methods
  void ProcessEvents();
  void Render();
  void RenderWindow(Window& window, ImGuiContext* context, UIRenderer& renderer);

  // ========================================================================
  // EVENT HANDLING
  // ========================================================================
  void HandleWindowEvent(const SDL_WindowEvent& windowEvent);
  void HandleGlobalEvent(const SDL_Event& event);

  // ========================================================================
  // UTILITY
  // ========================================================================
  bool ShouldClose();

  // ========================================================================
  // STATE
  // ========================================================================
  std::atomic<bool> running;

  // ========================================================================
  // CORE SYSTEMS
  // ========================================================================
  FontManager fontManager;

  // ========================================================================
  // WINDOWS AND CONTEXTS
  // ========================================================================
  std::unique_ptr<Window> window1;
  std::unique_ptr<Window> window2;
  ImGuiContext* imgui_context1 = nullptr;
  ImGuiContext* imgui_context2 = nullptr;

  // ========================================================================
  // UI RENDERERS
  // ========================================================================
  std::unique_ptr<UIRenderer> uiRenderer1;
  std::unique_ptr<UIRenderer> uiRenderer2;

  // ========================================================================
  // SERVICES (initialized after UI, accessed via ServiceLocator)
  // ========================================================================

  // Logger adapter (needed to configure ConfigManager)
  std::unique_ptr<LoggerAdapter> m_loggerAdapter;

  // Motion managers (stored here for memory management, but accessed via ServiceLocator)
  std::unique_ptr<PIControllerManagerStandardized> m_piManager;
  std::unique_ptr<ACSControllerManagerStandardized> m_acsManager;

  // NOTE: ConfigManager is NOT stored here - it's a singleton accessed via ServiceLocator
  // NOTE: All access to services goes through ServiceLocator::Get().Service()

  // Example of zero-dependency access:
  // Instead of: m_configManager.LoadConfigs()
  // Use:        ServiceLocator::Get().Config()->LoadConfigs()

  // Instead of: m_piManager->ConnectAll()
  // Use:        ServiceLocator::Get().PI()->ConnectAll()
};