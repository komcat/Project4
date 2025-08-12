#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <SDL.h>
#include <SDL_opengl.h>

// ImGui includes
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

// FreeType support for color emoji
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif

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
  bool emojiSupported;

public:
  DualWindowApp()
    : window1("Window 1", 800, 600, ImVec4(0.2f, 0.3f, 0.4f, 1.0f))
    , window2("Window 2", 600, 400, ImVec4(0.4f, 0.2f, 0.4f, 1.0f))
    , imgui_context1(nullptr)
    , imgui_context2(nullptr)
    , emojiSupported(false) {
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

  // Add this new method to safely setup fonts
  void SetupBasicFonts(ImGuiIO& io) {
    // Load default font first
    ImFont* defaultFont = io.Fonts->AddFontDefault();

    // Try to add comprehensive emoji support with extended ranges
    if (std::filesystem::exists("C:/Windows/Fonts/seguiemj.ttf")) {
      std::cout << "Attempting to load emoji font..." << std::endl;

      ImFontConfig emojiConfig;
      emojiConfig.MergeMode = true;
      emojiConfig.PixelSnapH = true;

#ifdef IMGUI_ENABLE_FREETYPE
      // CRITICAL: Enable color emoji support
      emojiConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
      std::cout << "✅ Color emoji support enabled" << std::endl;
#else
      std::cout << "⚠️ Color emoji not available without FreeType" << std::endl;
#endif

      // Use the same comprehensive ranges as uaa3App
      static const ImWchar extended_emoji_ranges[] = {
        // Basic emoji blocks
        0x1F600, 0x1F64F, // Emoticons
        0x1F300, 0x1F5FF, // Misc Symbols and Pictographs  
        0x1F680, 0x1F6FF, // Transport and Map
        0x1F700, 0x1F77F, // Alchemical Symbols
        0x1F780, 0x1F7FF, // Geometric Shapes Extended
        0x1F800, 0x1F8FF, // Supplemental Arrows-C
        0x1F900, 0x1F9FF, // Supplemental Symbols and Pictographs
        0x1FA00, 0x1FA6F, // Chess Symbols  
        0x1FA70, 0x1FAFF, // Symbols and Pictographs Extended-A

        // Additional useful ranges
        0x2600, 0x26FF,   // Miscellaneous Symbols
        0x2700, 0x27BF,   // Dingbats
        0x231A, 0x231B,   // Watch symbols
        0x2764, 0x2764,   // Heavy black heart
        0x2049, 0x2049,   // Exclamation question mark
        0x203C, 0x203C,   // Double exclamation mark

        // Enclosed alphanumerics (for circled numbers like ①②③④⑤)
        0x2460, 0x24FF,   // Enclosed Alphanumerics
        0x1F100, 0x1F1FF, // Enclosed Alphanumeric Supplement

        // Technical symbols
        0x2300, 0x23FF,   // Miscellaneous Technical
        0x25A0, 0x25FF,   // Geometric Shapes

        // Arrows and symbols
        0x2190, 0x21FF,   // Arrows
        0x2200, 0x22FF,   // Mathematical Operators

        // Greek and Coptic
        0x0370, 0x03FF,   // Greek and Coptic

        // Variation selectors (important for emoji presentation)
        0xFE00, 0xFE0F,   // Variation Selectors
        0xE0100, 0xE01EF, // Variation Selectors Supplement

        // Zero-width joiner and other combining characters
        0x200D, 0x200D,   // Zero Width Joiner (for emoji sequences)
        0x20E3, 0x20E3,   // Combining Enclosing Keycap (for number emojis like 1️⃣)

        0, // Terminator
      };

      try {
        ImFont* emojiFont = io.Fonts->AddFontFromFileTTF(
          "C:/Windows/Fonts/seguiemj.ttf",
          16.0f,
          &emojiConfig,
          extended_emoji_ranges  // Use comprehensive ranges
        );

        if (emojiFont) {
          std::cout << "✅ Emoji font loaded successfully with extended ranges" << std::endl;
          emojiSupported = true;
        }
        else {
          std::cout << "⚠️ Emoji font failed to load with extended ranges" << std::endl;
        }
      }
      catch (...) {
        std::cout << "❌ Exception while loading emoji font with extended ranges" << std::endl;
      }
    }
    else {
      std::cout << "⚠️ Emoji font file not found" << std::endl;
    }

    // Build the font atlas
    if (!io.Fonts->Build()) {
      std::cout << "❌ Failed to build font atlas" << std::endl;
    }
    else {
      std::cout << "✅ Font atlas built successfully" << std::endl;

      // Debug: Show glyph counts like in uaa3App
      for (int i = 0; i < io.Fonts->Fonts.Size; i++) {
        ImFont* font = io.Fonts->Fonts[i];
        std::cout << "   Font " << i << ": " << font->Glyphs.Size << " glyphs, "
          << font->FontSize << "px" << std::endl;
      }

#ifdef IMGUI_ENABLE_FREETYPE
      if (emojiSupported) {
        std::cout << "🎉 Color emoji support enabled with extended character ranges" << std::endl;
      }
#endif
    }

    // Set default font
    io.FontDefault = defaultFont;
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

#ifdef IMGUI_ENABLE_FREETYPE
    // Enable FreeType for better font rendering and color emoji
    io1.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
    io1.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
    std::cout << "✅ FreeType enabled for Window 1" << std::endl;
#else
    std::cout << "❌ FreeType disabled - add #define IMGUI_ENABLE_FREETYPE to imconfig.h" << std::endl;
#endif

    // SAFE FONT SETUP FOR WINDOW 1
    SetupBasicFonts(io1);

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

#ifdef IMGUI_ENABLE_FREETYPE
    // Enable FreeType for better font rendering and color emoji
    io2.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
    io2.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
    std::cout << "✅ FreeType enabled for Window 2" << std::endl;
#else
    std::cout << "❌ FreeType disabled for Window 2" << std::endl;
#endif

    // SAFE FONT SETUP FOR WINDOW 2
    SetupBasicFonts(io2);

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

      // Render Window 1 (includes emoji test)
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

  // SAFE emoji test function with proper error checking
  void RenderEmojiTestWindow() {
    static bool showEmojiTest = true;

    if (!showEmojiTest) return;

    try {
      if (ImGui::Begin("Emoji Test", &showEmojiTest)) {

        ImGui::Text("Font Status:");
        ImGui::Separator();

        if (emojiSupported) {
          ImGui::TextColored(ImVec4(0, 1, 0, 1), "✓ Emoji font loaded");
        }
        else {
          ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "⚠ Using fallback font");
        }

        ImGui::Separator();
        ImGui::Text("Basic Tests:");

        // Always safe - ASCII characters
        ImGui::Text("ASCII: Hello World 123");

        // Test basic symbols (should work with most fonts)
        ImGui::Text("Basic symbols: + - * / = < > ! ?");

        if (emojiSupported) {
          ImGui::Separator();
          ImGui::Text("Unicode Test (with emoji font):");

          // Test mathematical symbols using reinterpret_cast
          ImGui::Text("Math symbols:");
          ImGui::SameLine();
          ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"± ÷ × ≠ ∞ ≈ ≤ ≥"));

          // Test arrows using reinterpret_cast
          ImGui::Text("Arrows:");
          ImGui::SameLine();
          ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"← → ↑ ↓ ↔ ↕"));

          // Test Greek letters
          ImGui::Text("Greek letters:");
          ImGui::SameLine();
          ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"α β γ δ μ π θ λ"));

          // Test basic emojis using reinterpret_cast
          ImGui::Text("Basic emojis:");
          ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"😀 😃 😄 😁 😊"));

          ImGui::Text("More emojis:");
          ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"👍 👎 ❤️ 🔥 🚀"));

          ImGui::Text("Technical symbols:");
          ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"⚡ ⚙️ 🔧 🔨 ⚠️"));

          ImGui::Text("Robots and tools:");
          ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🤖 🦾 💨 🎯 ✅"));

          // Test in buttons using reinterpret_cast
          ImGui::Separator();
          if (ImGui::Button(reinterpret_cast<const char*>(u8"🤖 Robot Button"))) {
            std::cout << "Robot emoji button clicked!" << std::endl;
          }

          if (ImGui::Button(reinterpret_cast<const char*>(u8"⚡ Lightning Button"))) {
            std::cout << "Lightning emoji button clicked!" << std::endl;
          }

        }
        else {
          ImGui::Separator();
          ImGui::Text("Emoji font not available - using ASCII alternatives:");
          ImGui::Text("Math: +/- div x != inf");
          ImGui::Text("Arrows: <- -> up down");
          ImGui::Text("Status: [OK] [FAIL] [WARN]");

          if (ImGui::Button("Test Button [OK]")) {
            std::cout << "ASCII button clicked!" << std::endl;
          }
        }

        ImGui::Separator();
        if (ImGui::Button("Close Test Window")) {
          showEmojiTest = false;
        }
      }
      ImGui::End();

    }
    catch (const std::exception& e) {
      std::cout << "❌ Exception in emoji test: " << e.what() << std::endl;
      showEmojiTest = false; // Disable on error
    }
    catch (...) {
      std::cout << "❌ Unknown exception in emoji test" << std::endl;
      showEmojiTest = false; // Disable on error
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

    // Render emoji test within the same frame scope
    RenderEmojiTestWindow();

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

    // Use safe emoji display
    if (emojiSupported) {
      ImGui::Text("🟢 Window 1 - Main Control");
    }
    else {
      ImGui::Text("[OK] Window 1 - Main Control");
    }
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
    ImGui::Text("Emoji Support: %s", emojiSupported ? "Yes" : "No");
    ImGui::End();
  }

  void RenderWindow2UI() {
    ImGui::Begin("Secondary Tools");

    // Use safe emoji display
    if (emojiSupported) {
      ImGui::Text("🔵 Window 2 - Secondary Tools");
    }
    else {
      ImGui::Text("[*] Window 2 - Secondary Tools");
    }
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
    ImGui::Text("Font Status: %s", emojiSupported ? "Emoji Ready" : "ASCII Only");
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