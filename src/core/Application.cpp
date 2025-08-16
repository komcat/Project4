// Application.cpp - Simplified Structure with UniversalServices Integration
// Keep it conceptually simple: Initialize -> Run Loop -> Cleanup

#include "Application.h"
#include "../utils/Logger.h"
#include "../utils/Unicode.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include "../devices/UniversalServices.h"
#include "../devices/motions/PIControllerManagerStandardized.h"
#include "../devices/motions/ACSControllerManagerStandardized.h"
#include "../core/ConfigRegistry.h"
#include "../utils/LoggerAdapter.h"
#include <GL/gl.h>
#include <thread>

Application::Application() : running(false) {
  Logger::Info(L"Application created");
}

Application::~Application() {
  Cleanup();
}

bool Application::Initialize() {
  if (!InitializeSDL()) return false;
  if (!InitializeImGui()) return false;

  // Create windows and UI renderers
  if (!CreateWindows()) return false;

  Logger::Success(L"Application initialized successfully");
  return true;
}

void Application::Run() {
  running = true;
  Logger::Info(L"Starting main application loop");

  // **STEP 1**: Render home page first
  RenderInitialHomePage();

  // **STEP 2**: AFTER home page is shown, initialize Motion managers
  InitializeMotionManagers();

  // **STEP 3**: Main loop
  while (running && !ShouldClose()) {
    ProcessEvents();
    Render();

    // Small delay to prevent excessive CPU usage
    SDL_Delay(16); // ~60 FPS
  }

  Logger::Info(L"Application main loop ended");
}

void Application::RenderInitialHomePage() {
  Logger::Info(L"Rendering initial home page...");

  // Render a few frames to show the home page
  for (int frame = 0; frame < 3; frame++) {
    ProcessEvents();
    Render();
    SDL_Delay(16);
  }

  Logger::Success(L"Home page rendered");
}

void Application::InitializeMotionManagers() {
  Logger::Info(L"Initializing Motion managers after UI render...");

  try {
    // === SETUP CONFIGURATION SYSTEM (following your test pattern) ===
    m_loggerAdapter = std::make_unique<LoggerAdapter>();

    // Get ConfigManager singleton (don't store it, just configure it)
    auto& configManager = ConfigManager::Instance();
    configManager.SetLogger(m_loggerAdapter.get());
    configManager.SetConfigDirectory("config");

    // Load motion configurations
    ConfigLogger::ConfigTestStart();
    if (ConfigRegistry::LoadMotionConfigs()) {
      ConfigLogger::ConfigLoaded("Motion configurations");
    }
    else {
      ConfigLogger::ConfigError("Motion configurations", "Failed to load some configs");
    }

    // === CREATE MOTION MANAGERS (with try-catch for safety) ===
    Logger::Info(L"Creating standardized motion managers...");

    try {
      // Create PI Controller Manager in HARDWARE MODE
      Logger::Info(L"Creating PI Manager...");
      m_piManager = std::make_unique<PIControllerManagerStandardized>(configManager, true); // true = hardware mode
      Logger::Success(L"✅ PI Manager created");
    }
    catch (const std::exception& e) {
      Logger::Error(L"Failed to create PI Manager: " +
        std::wstring(e.what(), e.what() + strlen(e.what())));
      // Continue without PI manager
    }

    try {
      // Create ACS Controller Manager  
      Logger::Info(L"Creating ACS Manager...");
      m_acsManager = std::make_unique<ACSControllerManagerStandardized>(configManager);
      Logger::Success(L"✅ ACS Manager created");
    }
    catch (const std::exception& e) {
      Logger::Error(L"Failed to create ACS Manager: " +
        std::wstring(e.what(), e.what() + strlen(e.what())));
      // Continue without ACS manager
    }

    // === REGISTER WITH UNIVERSAL SERVICES (only if managers exist) ===
    if (m_piManager) {
      Services::RegisterPIManager(m_piManager.get());
    }
    if (m_acsManager) {
      Services::RegisterACSManager(m_acsManager.get());
    }

    Logger::Info(L"📊 Services Status:");
    Logger::Info(L"  PI Manager: " + std::wstring(Services::HasPIManager() ? L"REGISTERED" : L"NOT REGISTERED"));
    Logger::Info(L"  ACS Manager: " + std::wstring(Services::HasACSManager() ? L"REGISTERED" : L"NOT REGISTERED"));

    // === INITIALIZE MANAGERS ===
    Logger::Info(L"Initializing managers...");

    if (m_piManager) {
      try {
        if (m_piManager->Initialize()) {
          ConfigLogger::ConfigLoaded("PI manager initialized successfully");
        }
        else {
          ConfigLogger::ConfigError("PI Manager", "Failed to initialize");
        }
      }
      catch (const std::exception& e) {
        Logger::Error(L"PI Manager initialization exception: " +
          std::wstring(e.what(), e.what() + strlen(e.what())));
      }
    }

    if (m_acsManager) {
      try {
        if (m_acsManager->Initialize()) {
          ConfigLogger::ConfigLoaded("ACS manager initialized successfully");
        }
        else {
          ConfigLogger::ConfigError("ACS Manager", "Failed to initialize");
        }
      }
      catch (const std::exception& e) {
        Logger::Error(L"ACS Manager initialization exception: " +
          std::wstring(e.what(), e.what() + strlen(e.what())));
      }
    }

    // === CONNECT TO HARDWARE IN BACKGROUND THREAD ===
    // Only start hardware thread if we have at least one manager
    if (m_piManager || m_acsManager) {
      std::thread hardwareThread([this]() {
        Logger::Info(L"🔗 Connecting to motion hardware in background...");

        bool piConnected = false;
        bool acsConnected = false;

        // Connect PI Controllers
        if (m_piManager) {
          try {
            Logger::Info(L"Connecting PI controllers...");
            piConnected = m_piManager->ConnectAll();
            if (piConnected) {
              Logger::Success(L"✅ PI controllers connected");
            }
            else {
              Logger::Warning(L"⚠️ Some PI controllers failed to connect");
            }
          }
          catch (const std::exception& e) {
            Logger::Error(L"PI connection exception: " +
              std::wstring(e.what(), e.what() + strlen(e.what())));
          }
        }

        // Connect ACS Controllers
        if (m_acsManager) {
          try {
            Logger::Info(L"Connecting ACS controllers...");
            acsConnected = m_acsManager->ConnectAll();
            if (acsConnected) {
              Logger::Success(L"✅ ACS controllers connected");
            }
            else {
              Logger::Warning(L"⚠️ Some ACS controllers failed to connect");
            }
          }
          catch (const std::exception& e) {
            Logger::Error(L"ACS connection exception: " +
              std::wstring(e.what(), e.what() + strlen(e.what())));
          }
        }

        // Final status
        if (piConnected || acsConnected) {
          Logger::Success(L"🎉 Motion hardware initialization complete!");
        }
        else {
          Logger::Warning(L"⚠️ Motion system running in degraded mode (no hardware connected)");
        }
      });

      // Detach thread so it runs independently
      hardwareThread.detach();
    }

    Logger::Success(L"Motion manager initialization started (hardware connecting in background)");

  }
  catch (const std::exception& e) {
    Logger::Error(L"Critical error in motion manager initialization: " +
      std::wstring(e.what(), e.what() + strlen(e.what())));
  }
  catch (...) {
    Logger::Error(L"Unknown exception in motion manager initialization");
  }
}

bool Application::CreateWindows() {
  // Create window1 (main window) - following your existing pattern
  window1 = std::make_unique<Window>(
    "Project4 - Main", 800, 600,
    ImVec4(0.2f, 0.3f, 0.4f, 1.0f)
  );

  // Create window2 (secondary window)
  window2 = std::make_unique<Window>(
    "Project4 - Secondary", 600, 400,
    ImVec4(0.4f, 0.2f, 0.4f, 1.0f)
  );

  if (!window1->Initialize() || !window2->Initialize()) {
    Logger::Error(L"Failed to create windows");
    return false;
  }

  // Initialize ImGui contexts for each window
  window1->MakeContextCurrent();
  imgui_context1 = ImGui::CreateContext();
  ImGui::SetCurrentContext(imgui_context1);
  InitializeImGuiForWindow(*window1);

  window2->MakeContextCurrent();
  imgui_context2 = ImGui::CreateContext();
  ImGui::SetCurrentContext(imgui_context2);
  InitializeImGuiForWindow(*window2);

  // Create UI renderers
  uiRenderer1 = std::make_unique<UIRenderer>(fontManager, "Window 1");
  uiRenderer2 = std::make_unique<UIRenderer>(fontManager, "Window 2");

  Logger::Success(L"Windows and UI renderers created");
  return true;
}

bool Application::InitializeSDL() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
    Logger::Error(L"Failed to initialize SDL");
    return false;
  }

  SetupOpenGLAttributes();
  Logger::Success(L"SDL initialized");
  return true;
}

bool Application::InitializeImGui() {
  // Font manager initialization (following your existing pattern)
  Logger::Success(L"ImGui systems initialized");
  return true;
}

void Application::InitializeImGuiForWindow(Window& window) {
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Setup ImGui style
  ImGui::StyleColorsDark();

  // Setup platform/renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window.GetSDLWindow(), window.GetGLContext());
  ImGui_ImplOpenGL3_Init("#version 130");

  // Load fonts (following your existing pattern)
  auto fontResult = fontManager.SetupComprehensiveFonts(io);
  if (!fontResult.success) {
    Logger::Warning(L"Font setup had issues: " +
      UnicodeUtils::StringToWString(fontResult.errorMessage));
  }
  else {
    Logger::Success(L"Fonts loaded successfully with emoji support!");
  }
}

void Application::SetupOpenGLAttributes() {
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

bool Application::ShouldClose() {
  return (window1 && window1->ShouldClose()) ||
    (window2 && window2->ShouldClose());
}

void Application::ProcessEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_WINDOWEVENT) {
      HandleWindowEvent(event.window);
    }
    else {
      HandleGlobalEvent(event);
    }
  }
}

void Application::HandleWindowEvent(const SDL_WindowEvent& windowEvent) {
  Uint32 windowID = windowEvent.windowID;

  if (window1 && windowID == window1->GetWindowID()) {
    window1->MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context1);

    SDL_Event event;
    event.type = SDL_WINDOWEVENT;
    event.window = windowEvent;
    ImGui_ImplSDL2_ProcessEvent(&event);

    if (windowEvent.event == SDL_WINDOWEVENT_CLOSE) {
      window1->SetShouldClose(true);
    }
  }
  else if (window2 && windowID == window2->GetWindowID()) {
    window2->MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context2);

    SDL_Event event;
    event.type = SDL_WINDOWEVENT;
    event.window = windowEvent;
    ImGui_ImplSDL2_ProcessEvent(&event);

    if (windowEvent.event == SDL_WINDOWEVENT_CLOSE) {
      window2->SetShouldClose(true);
    }
  }
}

void Application::HandleGlobalEvent(const SDL_Event& event) {
  // Send global events to both windows
  if (window1 && imgui_context1) {
    window1->MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context1);
    ImGui_ImplSDL2_ProcessEvent(&event);
  }

  if (window2 && imgui_context2) {
    window2->MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context2);
    ImGui_ImplSDL2_ProcessEvent(&event);
  }

  if (event.type == SDL_QUIT) {
    running = false;
  }
}

void Application::Render() {
  if (window1 && uiRenderer1) {
    RenderWindow(*window1, imgui_context1, *uiRenderer1);
  }

  if (window2 && uiRenderer2) {
    RenderWindow(*window2, imgui_context2, *uiRenderer2);
  }
}

void Application::RenderWindow(Window& window, ImGuiContext* context, UIRenderer& renderer) {
  window.MakeContextCurrent();
  ImGui::SetCurrentContext(context);

  // Start new ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  // Set viewport
  int width, height;
  window.GetSize(width, height);
  glViewport(0, 0, width, height);

  // Clear screen
  ImVec4 clearColor = window.GetClearColor();
  glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
  glClear(GL_COLOR_BUFFER_BIT);

  // Render UI content
  renderer.Render(running);

  // Finish ImGui rendering
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  // Swap buffers
  window.SwapBuffers();
}

void Application::Cleanup() {
  Logger::Info(L"Starting application cleanup");

  // === CLEANUP MOTION MANAGERS FIRST (following your test pattern) ===
  if (m_piManager || m_acsManager) {
    Logger::Info(L"🛑 Stopping all motion devices before disconnection...");

    // Stop all devices safely
    if (m_piManager) {
      m_piManager->StopAllDevices();
    }

    Logger::Info(L"🔌 Disconnecting motion devices...");

    // Disconnect all devices
    if (m_piManager) {
      m_piManager->DisconnectAll();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (m_acsManager) {
      m_acsManager->DisconnectAll();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    Logger::Info(L"🧹 Clearing universal services...");
    Services::Clear();

    // Clear manager pointers before destruction (critical for safe cleanup)
    Logger::Info(L"🔧 Releasing motion managers...");
    m_piManager.reset();
    m_acsManager.reset();

    // Clear config manager logger reference (ConfigManager is singleton)
    auto& configManager = ConfigManager::Instance();
    configManager.SetLogger(nullptr);

    // Clear logger adapter
    m_loggerAdapter.reset();

    Logger::Success(L"Motion managers cleaned up safely");
  }

  // === CLEANUP IMGUI CONTEXTS ===
  if (imgui_context1) {
    if (window1) window1->MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context1);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(imgui_context1);
    imgui_context1 = nullptr;
  }

  if (imgui_context2) {
    if (window2) window2->MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context2);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(imgui_context2);
    imgui_context2 = nullptr;
  }

  // === CLEANUP WINDOWS ===
  window1.reset();
  window2.reset();

  // === CLEANUP UI RENDERERS ===
  uiRenderer1.reset();
  uiRenderer2.reset();

  SDL_Quit();
  Logger::Success(L"Application cleanup complete");
}