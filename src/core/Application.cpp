// Application.cpp - Updated to use ServiceLocator with Zero Dependencies
// ConfigManager is now a service, not a direct dependency

#include "Application.h"
#include "../utils/Logger.h"
#include "../utils/Unicode.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include "../core/ServiceLocator.h"  // Use ServiceLocator instead of UniversalServices
#include "../devices/motions/PIControllerManagerStandardized.h"
#include "../devices/motions/ACSControllerManagerStandardized.h"
#include "../core/ConfigManager.h"     // For ConfigManager
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

  // **STEP 2**: AFTER home page is shown, initialize services (ConfigManager + Motion managers)
  InitializeServices();

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

void Application::InitializeServices() {
  Logger::Info(L"Initializing Services with zero dependencies...");

  try {
    // ========================================================================
    // STEP 1: Create and Register ConfigManager as Service
    // ========================================================================
    Logger::Info(L"Creating ConfigManager service...");

    m_loggerAdapter = std::make_unique<LoggerAdapter>();

    // ConfigManager is a singleton, but we register it as a service for zero dependencies
    auto& configManager = ConfigManager::Instance();
    configManager.SetLogger(m_loggerAdapter.get());
    configManager.SetConfigDirectory("config");

    // Register ConfigManager as a service
    ServiceLocator::Get().RegisterConfigManager(&configManager);
    Logger::Success(L"✅ ConfigManager registered as service");

    // Load configurations through the service
    ConfigLogger::ConfigTestStart();
    if (ConfigRegistry::LoadMotionConfigs()) {
      ConfigLogger::ConfigLoaded("Motion configurations");
    }
    else {
      ConfigLogger::ConfigError("Motion configurations", "Failed to load some configs");
    }

    // ========================================================================
    // STEP 2: Create Motion Managers (they get ConfigManager via ServiceLocator)
    // ========================================================================
    Logger::Info(L"Creating motion managers...");

    try {
      // Create PI Controller Manager - it will get ConfigManager from ServiceLocator
      Logger::Info(L"Creating PI Manager...");

      // PI Manager gets ConfigManager via ServiceLocator (zero dependencies!)
      m_piManager = std::make_unique<PIControllerManagerStandardized>(
        *ServiceLocator::Get().Config(),  // Get ConfigManager from services
        true  // hardware mode
      );

      // Register PI Manager as service
      ServiceLocator::Get().RegisterPI(m_piManager.get());
      Logger::Success(L"✅ PI Manager created and registered");
    }
    catch (const std::exception& e) {
      Logger::Error(L"Failed to create PI Manager: " +
        std::wstring(e.what(), e.what() + strlen(e.what())));
      // Continue without PI manager
    }

    try {
      // Create ACS Controller Manager - it will get ConfigManager from ServiceLocator
      Logger::Info(L"Creating ACS Manager...");

      // ACS Manager gets ConfigManager via ServiceLocator (zero dependencies!)
      m_acsManager = std::make_unique<ACSControllerManagerStandardized>(
        *ServiceLocator::Get().Config()  // Get ConfigManager from services
      );

      // Register ACS Manager as service
      ServiceLocator::Get().RegisterACS(m_acsManager.get());
      Logger::Success(L"✅ ACS Manager created and registered");
    }
    catch (const std::exception& e) {
      Logger::Error(L"Failed to create ACS Manager: " +
        std::wstring(e.what(), e.what() + strlen(e.what())));
      // Continue without ACS manager
    }

    // ========================================================================
    // STEP 3: Print Service Status
    // ========================================================================
    Logger::Info(L"📊 Service Registration Complete:");
    ServiceLocator::Get().PrintStatus();

    // ========================================================================
    // STEP 4: Initialize All Services
    // ========================================================================
    Logger::Info(L"Initializing all motion services...");

    if (ServiceLocator::Get().InitializeAllMotion()) {
      Logger::Success(L"✅ All motion services initialized");
    }
    else {
      Logger::Warning(L"⚠️ Some motion services failed to initialize");
    }

    // ========================================================================
    // STEP 5: Connect to Hardware in Background Thread
    // ========================================================================
    if (ServiceLocator::Get().HasPI() || ServiceLocator::Get().HasACS()) {
      std::thread hardwareThread([this]() {
        Logger::Info(L"🔗 Connecting to motion hardware in background...");

        bool connected = ServiceLocator::Get().ConnectAllMotion();

        if (connected) {
          Logger::Success(L"🎉 Motion hardware connection complete!");
        }
        else {
          Logger::Warning(L"⚠️ Motion system running in degraded mode (some hardware failed to connect)");
        }
      });

      // Detach thread so it runs independently
      hardwareThread.detach();
    }

    Logger::Success(L"Service initialization complete (hardware connecting in background)");

  }
  catch (const std::exception& e) {
    Logger::Error(L"Critical error in service initialization: " +
      std::wstring(e.what(), e.what() + strlen(e.what())));
  }
  catch (...) {
    Logger::Error(L"Unknown exception in service initialization");
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

  // ========================================================================
  // CLEANUP MOTION SERVICES (using ServiceLocator)
  // ========================================================================
  if (ServiceLocator::Get().HasPI() || ServiceLocator::Get().HasACS()) {
    Logger::Info(L"🛑 Stopping and disconnecting motion services...");

    // Use ServiceLocator batch operations for clean shutdown
    ServiceLocator::Get().DisconnectAllMotion();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Logger::Info(L"🧹 Clearing all services...");
    ServiceLocator::Get().ClearAll();

    // Clear manager pointers before destruction (critical for safe cleanup)
    Logger::Info(L"🔧 Releasing motion managers...");
    m_piManager.reset();
    m_acsManager.reset();

    // Clear config manager logger reference (ConfigManager is singleton)
    if (ServiceLocator::Get().HasConfig()) {
      auto& configManager = ConfigManager::Instance();
      configManager.SetLogger(nullptr);
    }

    // Clear logger adapter
    m_loggerAdapter.reset();

    Logger::Success(L"Motion services cleaned up safely");
  }

  // ========================================================================
  // CLEANUP IMGUI CONTEXTS
  // ========================================================================
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

  // ========================================================================
  // CLEANUP WINDOWS
  // ========================================================================
  window1.reset();
  window2.reset();

  // ========================================================================
  // CLEANUP UI RENDERERS
  // ========================================================================
  uiRenderer1.reset();
  uiRenderer2.reset();

  SDL_Quit();
  Logger::Success(L"Application cleanup complete");
}