// Application.h - Clean and Simple
#pragma once

#include <memory>
#include <atomic>
#include <SDL.h>
#include "Window.h"
#include "../ui/FontManager.h"
#include "../ui/UIRenderer.h"
#include "../core/ConfigManager.h"
#include "../utils/LoggerAdapter.h"

// Forward declarations for managers
class PIControllerManagerStandardized;
class ACSControllerManagerStandardized;

class Application {
public:
  Application();
  ~Application();

  // Main application flow - KISS principle
  bool Initialize();
  void Run();
  void Cleanup();

private:
  // === INITIALIZATION PHASE ===
  bool InitializeSDL();
  bool InitializeImGui();
  bool CreateWindows();
  void InitializeImGuiForWindow(Window& window);
  void SetupOpenGLAttributes();

  // === RUNTIME PHASE ===
  // Step 1: Show home page immediately
  void RenderInitialHomePage();

  // Step 2: Initialize Motion managers AFTER UI is visible
  void InitializeMotionManagers();

  // Step 3: Main loop methods
  void ProcessEvents();
  void Render();
  void RenderWindow(Window& window, ImGuiContext* context, UIRenderer& renderer);

  // === EVENT HANDLING ===
  void HandleWindowEvent(const SDL_WindowEvent& windowEvent);
  void HandleGlobalEvent(const SDL_Event& event);

  // === UTILITY ===
  bool ShouldClose();

  // === STATE ===
  std::atomic<bool> running;

  // === CORE SYSTEMS ===
  FontManager fontManager;

  // === WINDOWS AND CONTEXTS ===
  std::unique_ptr<Window> window1;
  std::unique_ptr<Window> window2;
  ImGuiContext* imgui_context1 = nullptr;
  ImGuiContext* imgui_context2 = nullptr;

  // === UI RENDERERS ===
  std::unique_ptr<UIRenderer> uiRenderer1;
  std::unique_ptr<UIRenderer> uiRenderer2;

  // === MOTION MANAGERS (initialized after UI) ===
  std::unique_ptr<LoggerAdapter> m_loggerAdapter;
  std::unique_ptr<PIControllerManagerStandardized> m_piManager;
  std::unique_ptr<ACSControllerManagerStandardized> m_acsManager;

  // Note: ConfigManager is a singleton, so we don't store it
  // Note: Managers are also registered with UniversalServices for global access
};