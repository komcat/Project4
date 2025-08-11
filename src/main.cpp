#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <SDL.h>
#include <SDL_opengl.h>

// ImGui includes
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

// Global control variables
std::atomic<bool> g_running{ true };
std::mutex g_cout_mutex;

// Thread-safe logging
void ThreadSafeLog(const std::string& message) {
  std::lock_guard<std::mutex> lock(g_cout_mutex);
  std::cout << message << std::endl;
}

// Simple window class for single-threaded approach
class SimpleWindow {
public:
  SDL_Window* window;
  SDL_GLContext gl_context;
  std::string title;
  int width, height;
  ImVec4 clear_color;
  bool should_close;

  SimpleWindow(const std::string& t, int w, int h, ImVec4 color)
    : window(nullptr), gl_context(nullptr), title(t), width(w), height(h),
    clear_color(color), should_close(false) {
  }

  bool Initialize() {
    // Create window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
      );

    // Position windows side by side on the same screen
    int x_pos, y_pos;
    if (title == "Window 1") {
      x_pos = 100;  // Left side
      y_pos = 100;
    }
    else {
      x_pos = 950;  // Right side (100 + 800 + 50 spacing)
      y_pos = 100;
    }

    window = SDL_CreateWindow(
      title.c_str(),
      x_pos, y_pos,
      width, height,
      window_flags
    );

    if (!window) {
      std::cout << "❌ Failed to create window: " << title << " - " << SDL_GetError() << std::endl;
      return false;
    }

    // Create OpenGL context
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
      std::cout << "❌ Failed to create GL context for: " << title << " - " << SDL_GetError() << std::endl;
      return false;
    }

    std::cout << "✅ Created window: " << title << std::endl;
    return true;
  }

  void MakeContextCurrent() {
    SDL_GL_MakeCurrent(window, gl_context);
  }

  void SwapBuffers() {
    SDL_GL_SwapWindow(window);
  }

  void Cleanup() {
    if (gl_context) {
      SDL_GL_DeleteContext(gl_context);
    }
    if (window) {
      SDL_DestroyWindow(window);
    }
    std::cout << "✅ Cleaned up: " << title << std::endl;
  }

  Uint32 GetWindowID() {
    return SDL_GetWindowID(window);
  }
};

class DualWindowApp {
private:
  SimpleWindow window1;
  SimpleWindow window2;
  ImGuiContext* imgui_context1;
  ImGuiContext* imgui_context2;

public:
  DualWindowApp()
    : window1("Window 1", 800, 600, ImVec4(0.2f, 0.3f, 0.4f, 1.0f))
    , window2("Window 2", 600, 400, ImVec4(0.4f, 0.2f, 0.4f, 1.0f))
    , imgui_context1(nullptr)
    , imgui_context2(nullptr) {
  }

  bool Initialize() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
      std::cout << "❌ SDL Initialization failed: " << SDL_GetError() << std::endl;
      return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create windows
    if (!window1.Initialize() || !window2.Initialize()) {
      return false;
    }

    // Setup ImGui contexts
    if (!SetupImGuiContexts()) {
      return false;
    }

    std::cout << "✅ Application initialized successfully" << std::endl;
    return true;
  }

  bool SetupImGuiContexts() {
    // Setup ImGui for Window 1
    window1.MakeContextCurrent();

    IMGUI_CHECKVERSION();
    imgui_context1 = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context1);

    ImGuiIO& io1 = ImGui::GetIO();
    io1.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForOpenGL(window1.window, window1.gl_context)) {
      std::cout << "❌ Failed to initialize ImGui SDL2 backend for Window 1" << std::endl;
      return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
      std::cout << "❌ Failed to initialize ImGui OpenGL3 backend for Window 1" << std::endl;
      return false;
    }

    // Setup ImGui for Window 2
    window2.MakeContextCurrent();

    imgui_context2 = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context2);

    ImGuiIO& io2 = ImGui::GetIO();
    io2.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForOpenGL(window2.window, window2.gl_context)) {
      std::cout << "❌ Failed to initialize ImGui SDL2 backend for Window 2" << std::endl;
      return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
      std::cout << "❌ Failed to initialize ImGui OpenGL3 backend for Window 2" << std::endl;
      return false;
    }

    std::cout << "✅ ImGui contexts initialized for both windows" << std::endl;
    return true;
  }

  void Run() {
    std::cout << "🚀 Starting main loop..." << std::endl;

    while (g_running && !window1.should_close && !window2.should_close) {
      ProcessEvents();

      // Render Window 1
      RenderWindow1();

      // Render Window 2
      RenderWindow2();

      // Small delay
      SDL_Delay(16); // ~60 FPS
    }

    std::cout << "🛑 Main loop ended" << std::endl;
  }

  void ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      // Handle window-specific events
      if (event.type == SDL_WINDOWEVENT) {
        Uint32 windowID = event.window.windowID;

        if (windowID == window1.GetWindowID()) {
          // Process event for Window 1
          window1.MakeContextCurrent();
          ImGui::SetCurrentContext(imgui_context1);
          ImGui_ImplSDL2_ProcessEvent(&event);

          if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
            window1.should_close = true;
          }
        }
        else if (windowID == window2.GetWindowID()) {
          // Process event for Window 2
          window2.MakeContextCurrent();
          ImGui::SetCurrentContext(imgui_context2);
          ImGui_ImplSDL2_ProcessEvent(&event);

          if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
            window2.should_close = true;
          }
        }
      }
      else {
        // Global events - send to both windows
        // Window 1
        window1.MakeContextCurrent();
        ImGui::SetCurrentContext(imgui_context1);
        ImGui_ImplSDL2_ProcessEvent(&event);

        // Window 2
        window2.MakeContextCurrent();
        ImGui::SetCurrentContext(imgui_context2);
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
          g_running = false;
        }
      }
    }
  }

  void RenderWindow1() {
    window1.MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context1);

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Clear background
    glViewport(0, 0, window1.width, window1.height);
    glClearColor(window1.clear_color.x, window1.clear_color.y, window1.clear_color.z, window1.clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render UI
    RenderWindow1UI();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Present
    window1.SwapBuffers();
  }

  void RenderWindow2() {
    window2.MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context2);

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Clear background
    glViewport(0, 0, window2.width, window2.height);
    glClearColor(window2.clear_color.x, window2.clear_color.y, window2.clear_color.z, window2.clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render UI
    RenderWindow2UI();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Present
    window2.SwapBuffers();
  }

  void RenderWindow1UI() {
    ImGui::Begin("Main Control Panel");

    ImGui::Text("🟢 Window 1 - Main Control");
    ImGui::Separator();

    static float slider_val = 50.0f;
    ImGui::SliderFloat("Control Value", &slider_val, 0.0f, 100.0f);

    static bool enabled = true;
    ImGui::Checkbox("Enable Feature", &enabled);

    if (ImGui::Button("Action Button")) {
      std::cout << "Window 1: Button clicked! Value: " << slider_val << std::endl;
    }

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    if (ImGui::Button("Close This Window")) {
      window1.should_close = true;
    }

    if (ImGui::Button("Close Application")) {
      g_running = false;
    }

    ImGui::End();

    // Status window
    ImGui::Begin("Status##1");
    ImGui::Text("Status: Running");
    ImGui::Text("Window: Main Control");
    ImGui::Text("Thread: Main");
    ImGui::End();
  }

  void RenderWindow2UI() {
    ImGui::Begin("Secondary Tools");

    ImGui::Text("🔵 Window 2 - Secondary Tools");
    ImGui::Separator();

    static int counter = 0;
    if (ImGui::Button("Count Up")) {
      counter++;
      std::cout << "Window 2: Counter = " << counter << std::endl;
    }
    ImGui::SameLine();
    ImGui::Text("Count: %d", counter);

    static char buffer[128] = "Hello World";
    ImGui::InputText("Text Input", buffer, sizeof(buffer));

    static ImVec4 color = ImVec4(0.4f, 0.7f, 0.0f, 1.0f);
    ImGui::ColorEdit3("Background Color", (float*)&color);

    if (ImGui::Button("Close This Window")) {
      window2.should_close = true;
    }

    if (ImGui::Button("Close Application")) {
      g_running = false;
    }

    ImGui::End();

    // Tools window
    ImGui::Begin("Tools##2");
    ImGui::Text("Tools Panel");
    ImGui::Text("Window: Secondary");
    ImGui::Text("Thread: Main");
    ImGui::End();
  }

  void Cleanup() {
    // Cleanup ImGui contexts
    if (imgui_context1) {
      window1.MakeContextCurrent();
      ImGui::SetCurrentContext(imgui_context1);
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplSDL2_Shutdown();
      ImGui::DestroyContext(imgui_context1);
    }

    if (imgui_context2) {
      window2.MakeContextCurrent();
      ImGui::SetCurrentContext(imgui_context2);
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplSDL2_Shutdown();
      ImGui::DestroyContext(imgui_context2);
    }

    // Cleanup windows
    window1.Cleanup();
    window2.Cleanup();

    // Cleanup SDL
    SDL_Quit();

    std::cout << "✅ Application cleanup complete" << std::endl;
  }
};

int main(int argc, char* argv[]) {
  std::cout << "=== Simple Dual Window Application ===\n";
  std::cout << "Note: Running single-threaded for ImGui compatibility\n" << std::endl;

  DualWindowApp app;

  if (!app.Initialize()) {
    std::cout << "❌ Failed to initialize application" << std::endl;
    return -1;
  }

  app.Run();
  app.Cleanup();

  std::cout << "👋 Application terminated successfully" << std::endl;
  return 0;
}