// =====================================================
// COMPREHENSIVE IMGUI EMOJI FIX
// Addresses all font atlas and Unicode rendering issues
// =====================================================

#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <string>
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

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

// Enhanced wprint function with proper Unicode console support
void wprint(const std::wstring& message) {
#ifdef _WIN32
  static bool consoleInitialized = false;
  if (!consoleInitialized) {
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    consoleInitialized = true;
  }

  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD n_written;
  WriteConsoleW(handle, message.c_str(), (DWORD)message.length(), &n_written, NULL);
#else
  std::wcout << message;
#endif
}

// Helper function to convert string to wstring
std::wstring StringToWString(const std::string& str) {
  if (str.empty()) return std::wstring();
#ifdef _WIN32
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
  return wstrTo;
#else
  // Simple fallback for non-Windows
  return std::wstring(str.begin(), str.end());
#endif
}

// =====================================================
// COMPREHENSIVE FONT MANAGER CLASS
// Addresses all font atlas and Unicode issues
// =====================================================
class ComprehensiveFontManager {
private:
  bool emojiSupported = false;
  bool greekSupported = false;
  bool mathSupported = false;
  ImFont* baseFont = nullptr;
  ImFont* emojiFont = nullptr;

public:
  struct FontLoadResult {
    bool success = false;
    std::string fontPath;
    int glyphCount = 0;
    std::string errorMessage;
  };

  // SOLUTION 1: Load fonts with comprehensive Unicode ranges
  FontLoadResult SetupComprehensiveFonts(ImGuiIO& io) {
    FontLoadResult result;

    wprint(L"🔧 Starting comprehensive font setup...\n");

    // Clear any existing fonts to start fresh
    io.Fonts->Clear();

    // STEP 1: Load base font with Latin + Greek + Math support
    result = LoadBaseFont(io);
    if (!result.success) {
      wprint(L"⚠️ Base font loading failed, using ImGui default\n");
      baseFont = io.Fonts->AddFontDefault();
    }

    // STEP 2: Load emoji font with proper merging
    LoadEmojiFont(io);

    // STEP 3: Build font atlas with comprehensive error checking
    return BuildFontAtlas(io);
  }

private:
  FontLoadResult LoadBaseFont(ImGuiIO& io) {
    FontLoadResult result;

    // Try fonts that support Greek, Latin, and Math symbols
    const char* baseFontCandidates[] = {
        "C:/Windows/Fonts/arial.ttf",           // Arial - excellent Unicode support
        "C:/Windows/Fonts/calibri.ttf",         // Calibri - good Greek support  
        "C:/Windows/Fonts/segoeui.ttf",         // Segoe UI - Microsoft's Unicode font
        "C:/Windows/Fonts/times.ttf",           // Times - classical Greek support
        "assets/fonts/Roboto-Regular.ttf"       // Project font
    };

    // Define comprehensive ranges for base font
    static const ImWchar comprehensive_base_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0x0180, 0x024F, // Latin Extended-B
        0x0370, 0x03FF, // Greek and Coptic
        0x1F00, 0x1FFF, // Greek Extended
        0x2000, 0x206F, // General Punctuation
        0x2070, 0x209F, // Superscripts and Subscripts
        0x20A0, 0x20CF, // Currency Symbols
        0x2100, 0x214F, // Letterlike Symbols
        0x2150, 0x218F, // Number Forms
        0x2190, 0x21FF, // Arrows
        0x2200, 0x22FF, // Mathematical Operators
        0x2300, 0x23FF, // Miscellaneous Technical
        0x2460, 0x24FF, // Enclosed Alphanumerics
        0x25A0, 0x25FF, // Geometric Shapes
        0x2600, 0x26FF, // Miscellaneous Symbols
        0, // Terminator
    };

    for (const char* fontPath : baseFontCandidates) {
      if (std::filesystem::exists(fontPath)) {
        wprint(L"🔤 Attempting base font: " + StringToWString(fontPath) + L"\n");

        ImFontConfig baseConfig;
        baseConfig.PixelSnapH = true;
        baseConfig.OversampleH = 2;
        baseConfig.OversampleV = 1;

        try {
          baseFont = io.Fonts->AddFontFromFileTTF(
            fontPath,
            16.0f,
            &baseConfig,
            comprehensive_base_ranges
          );

          if (baseFont) {
            result.success = true;
            result.fontPath = fontPath;
            wprint(L"✅ Base font loaded: " + StringToWString(fontPath) + L"\n");

            // Test Greek support
            const ImFontGlyph* alphaGlyph = baseFont->FindGlyph(0x03B1); // α
            if (alphaGlyph) {
              greekSupported = true;
              wprint(L"✅ Greek letters supported in base font\n");
            }

            // Test math support  
            const ImFontGlyph* plusMinusGlyph = baseFont->FindGlyph(0x00B1); // ±
            if (plusMinusGlyph) {
              mathSupported = true;
              wprint(L"✅ Math symbols supported in base font\n");
            }

            break;
          }
        }
        catch (...) {
          wprint(L"❌ Exception loading base font: " + StringToWString(fontPath) + L"\n");
        }
      }
    }

    return result;
  }

  void LoadEmojiFont(ImGuiIO& io) {
    // SOLUTION 2: Proper emoji font with correct ranges
    const char* emojiFontCandidates[] = {
        "assets/fonts/NotoColorEmoji.ttf",       // Best: True color emoji
        "assets/fonts/AppleColorEmoji.ttc",      // Apple emoji (if available)
        "C:/Windows/Fonts/seguiemj.ttf",         // Windows emoji font
        "C:/Windows/Fonts/segoeui.ttf"           // Fallback with some emoji
    };

    // SOLUTION 2: Comprehensive emoji ranges
    static const ImWchar comprehensive_emoji_ranges[] = {
      // Core Emoji Blocks
      0x1F600, 0x1F64F, // Emoticons
      0x1F300, 0x1F5FF, // Misc Symbols and Pictographs
      0x1F680, 0x1F6FF, // Transport and Map Symbols
      0x1F700, 0x1F77F, // Alchemical Symbols
      0x1F780, 0x1F7FF, // Geometric Shapes Extended
      0x1F800, 0x1F8FF, // Supplemental Arrows-C
      0x1F900, 0x1F9FF, // Supplemental Symbols and Pictographs
      0x1FA00, 0x1FA6F, // Chess Symbols
      0x1FA70, 0x1FAFF, // Symbols and Pictographs Extended-A
      0x1FB00, 0x1FBFF, // Symbols and Pictographs Extended-B

      // Additional Symbol Blocks
      0x2600, 0x26FF,   // Miscellaneous Symbols
      0x2700, 0x27BF,   // Dingbats
      0x2B00, 0x2BFF,   // Miscellaneous Symbols and Arrows
      0x1F100, 0x1F1FF, // Enclosed Alphanumeric Supplement
      0x1F200, 0x1F2FF, // Enclosed Ideographic Supplement

      // Technical and UI Symbols
      0x2190, 0x21FF,   // Arrows
      0x2300, 0x23FF,   // Miscellaneous Technical
      0x25A0, 0x25FF,   // Geometric Shapes
      0x2460, 0x24FF,   // Enclosed Alphanumerics

      // Special Characters
      0x2010, 0x201F,   // Punctuation
      0x2020, 0x206F,   // General Punctuation
      0x20A0, 0x20CF,   // Currency Symbols
      0x2100, 0x214F,   // Letterlike Symbols

      // Variation Selectors (CRITICAL for proper emoji display)
      0xFE00, 0xFE0F,   // Variation Selectors
      0xE0100, 0xE01EF, // Variation Selectors Supplement

      // Combining Characters
      0x200D, 0x200D,   // Zero Width Joiner (for emoji sequences)
      0x20E3, 0x20E3,   // Combining Enclosing Keycap

      0, // Terminator
    };

    for (const char* emojiPath : emojiFontCandidates) {
      if (std::filesystem::exists(emojiPath)) {
        wprint(L"😀 Attempting emoji font: " + StringToWString(emojiPath) + L"\n");

        ImFontConfig emojiConfig;
        emojiConfig.MergeMode = true;  // CRITICAL: Merge with base font
        emojiConfig.PixelSnapH = true;
        emojiConfig.GlyphMinAdvanceX = 16.0f; // Monospace emoji

#ifdef IMGUI_ENABLE_FREETYPE
        // SOLUTION 3: Enable color emoji rendering
        emojiConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
        wprint(L"✅ FreeType color emoji enabled for: " + StringToWString(emojiPath) + L"\n");
#endif

        try {
          emojiFont = io.Fonts->AddFontFromFileTTF(
            emojiPath,
            16.0f,
            &emojiConfig,
            comprehensive_emoji_ranges
          );

          if (emojiFont) {
            emojiSupported = true;
            wprint(L"✅ Emoji font loaded successfully: " + StringToWString(emojiPath) + L"\n");
            break;
          }
        }
        catch (...) {
          wprint(L"❌ Exception loading emoji font: " + StringToWString(emojiPath) + L"\n");
        }
      }
    }

    if (!emojiSupported) {
      wprint(L"⚠️ No emoji font loaded - emojis will show as squares or question marks\n");
    }
  }

  // SOLUTION 4: Proper font atlas building with error checking
  FontLoadResult BuildFontAtlas(ImGuiIO& io) {
    FontLoadResult result;

    wprint(L"🔨 Building font atlas...\n");

    // CRITICAL: Ensure FreeType is configured before building
#ifdef IMGUI_ENABLE_FREETYPE
    io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
    io.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
    wprint(L"✅ FreeType renderer configured\n");
#endif

    // Build the atlas
    bool buildSuccess = io.Fonts->Build();

    if (!buildSuccess) {
      result.success = false;
      result.errorMessage = "Font atlas build failed";
      wprint(L"❌ Font atlas build FAILED\n");
      return result;
    }

    // Verify atlas was built
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (!pixels || width <= 0 || height <= 0) {
      result.success = false;
      result.errorMessage = "Font atlas texture data invalid";
      wprint(L"❌ Font atlas texture data is invalid\n");
      return result;
    }

    result.success = true;
    wprint(L"✅ Font atlas built successfully\n");
    wprint(L"   Atlas size: " + std::to_wstring(width) + L"x" + std::to_wstring(height) + L"\n");
    wprint(L"   Total fonts: " + std::to_wstring(io.Fonts->Fonts.Size) + L"\n");

    // Debug: Show glyph counts
    for (int i = 0; i < io.Fonts->Fonts.Size; i++) {
      ImFont* font = io.Fonts->Fonts[i];
      wprint(L"   Font " + std::to_wstring(i) + L": " +
        std::to_wstring(font->Glyphs.Size) + L" glyphs, " +
        std::to_wstring((int)font->FontSize) + L"px\n");
    }

    // Set default font to the base font (with merged emoji)
    if (baseFont) {
      io.FontDefault = baseFont;
      wprint(L"✅ Default font set to base font with merged emoji\n");
    }

    return result;
  }

public:
  // Getters for status
  bool IsEmojiSupported() const { return emojiSupported; }
  bool IsGreekSupported() const { return greekSupported; }
  bool IsMathSupported() const { return mathSupported; }

  // Test specific glyphs
  bool TestGlyph(ImWchar codepoint) const {
    ImGuiIO& io = ImGui::GetIO();
    if (io.FontDefault) {
      return io.FontDefault->FindGlyph(codepoint) != nullptr;
    }
    return false;
  }
};

// =====================================================
// MAIN APPLICATION WITH COMPREHENSIVE FONT SUPPORT
// =====================================================
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
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
      );

    int x_pos = (title == "Window 1") ? 100 : 950;
    int y_pos = 100;

    window = SDL_CreateWindow(title.c_str(), x_pos, y_pos, width, height, window_flags);
    if (!window) {
      wprint(L"❌ Failed to create window: " + StringToWString(title) + L"\n");
      return false;
    }

    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
      wprint(L"❌ Failed to create GL context for: " + StringToWString(title) + L"\n");
      return false;
    }

    wprint(L"✅ Created window: " + StringToWString(title) + L"\n");
    return true;
  }

  void MakeContextCurrent() { SDL_GL_MakeCurrent(window, gl_context); }
  void SwapBuffers() { SDL_GL_SwapWindow(window); }
  Uint32 GetWindowID() { return SDL_GetWindowID(window); }

  void Cleanup() {
    if (gl_context) SDL_GL_DeleteContext(gl_context);
    if (window) SDL_DestroyWindow(window);
    wprint(L"✅ Cleaned up: " + StringToWString(title) + L"\n");
  }
};

class ComprehensiveEmojiApp {
private:
  SimpleWindow window1;
  SimpleWindow window2;  // RESTORED: Second window
  ImGuiContext* imgui_context1;
  ImGuiContext* imgui_context2;  // RESTORED: Second context
  ComprehensiveFontManager fontManager;

public:
  ComprehensiveEmojiApp()
    : window1("Window 1 - Comprehensive Test", 800, 600, ImVec4(0.2f, 0.3f, 0.4f, 1.0f))
    , window2("Window 2 - Secondary Tools", 600, 400, ImVec4(0.4f, 0.2f, 0.4f, 1.0f))  // RESTORED
    , imgui_context1(nullptr)
    , imgui_context2(nullptr) {  // RESTORED
  }

  bool Initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
      wprint(L"❌ SDL Initialization failed\n");
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

    // RESTORED: Initialize both windows
    if (!window1.Initialize() || !window2.Initialize()) return false;
    if (!SetupImGui()) return false;

    wprint(L"✅ Application initialized successfully with dual windows\n");
    return true;
  }

  bool SetupImGui() {
    // RESTORED: Setup ImGui for both windows

    // Setup ImGui for Window 1
    window1.MakeContextCurrent();

    IMGUI_CHECKVERSION();
    imgui_context1 = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context1);

    ImGuiIO& io1 = ImGui::GetIO();
    io1.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // Comprehensive font setup for Window 1
    auto fontResult1 = fontManager.SetupComprehensiveFonts(io1);
    if (!fontResult1.success) {
      wprint(L"⚠️ Window 1 font setup had issues: " + StringToWString(fontResult1.errorMessage) + L"\n");
    }

    if (!ImGui_ImplSDL2_InitForOpenGL(window1.window, window1.gl_context)) {
      wprint(L"❌ Failed to initialize ImGui SDL2 backend for Window 1\n");
      return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
      wprint(L"❌ Failed to initialize ImGui OpenGL3 backend for Window 1\n");
      return false;
    }

    // Setup ImGui for Window 2
    window2.MakeContextCurrent();

    imgui_context2 = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context2);

    ImGuiIO& io2 = ImGui::GetIO();
    io2.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // Comprehensive font setup for Window 2
    auto fontResult2 = fontManager.SetupComprehensiveFonts(io2);
    if (!fontResult2.success) {
      wprint(L"⚠️ Window 2 font setup had issues: " + StringToWString(fontResult2.errorMessage) + L"\n");
    }

    if (!ImGui_ImplSDL2_InitForOpenGL(window2.window, window2.gl_context)) {
      wprint(L"❌ Failed to initialize ImGui SDL2 backend for Window 2\n");
      return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
      wprint(L"❌ Failed to initialize ImGui OpenGL3 backend for Window 2\n");
      return false;
    }

    wprint(L"✅ ImGui initialized with comprehensive font support for both windows\n");
    return true;
  }

  void Run() {
    wprint(L"🚀 Starting dual window comprehensive emoji test...\n");

    while (g_running && !window1.should_close && !window2.should_close) {
      ProcessEvents();

      // RESTORED: Render both windows
      RenderWindow1();
      RenderWindow2();

      SDL_Delay(16);
    }

    wprint(L"🛑 Dual window application ended\n");
  }

private:
  void ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      // RESTORED: Handle window-specific events
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

  // RESTORED: Window 1 rendering
  void RenderWindow1() {
    window1.MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context1);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    glViewport(0, 0, window1.width, window1.height);
    glClearColor(window1.clear_color.x, window1.clear_color.y, window1.clear_color.z, window1.clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // Main control panel
    RenderWindow1UI();

    // Comprehensive emoji test
    RenderComprehensiveTest();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    window1.SwapBuffers();
  }

  // RESTORED: Window 2 rendering  
  void RenderWindow2() {
    window2.MakeContextCurrent();
    ImGui::SetCurrentContext(imgui_context2);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    glViewport(0, 0, window2.width, window2.height);
    glClearColor(window2.clear_color.x, window2.clear_color.y, window2.clear_color.z, window2.clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // Secondary tools
    RenderWindow2UI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    window2.SwapBuffers();
  }

  // RESTORED: Window 1 UI
  void RenderWindow1UI() {
    ImGui::Begin("Main Control Panel");

    // Use emoji if supported
    if (fontManager.IsEmojiSupported()) {
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
      wprint(L"Window 1: Button clicked! Value: " + std::to_wstring(slider_val) + L"\n");
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
    ImGui::Text("Emoji Support: %s", fontManager.IsEmojiSupported() ? "Yes" : "No");
    ImGui::Text("Greek Support: %s", fontManager.IsGreekSupported() ? "Yes" : "No");
    ImGui::Text("Math Support: %s", fontManager.IsMathSupported() ? "Yes" : "No");
    ImGui::End();
  }

  // RESTORED: Window 2 UI
  void RenderWindow2UI() {
    ImGui::Begin("Secondary Tools");

    // Use emoji if supported
    if (fontManager.IsEmojiSupported()) {
      ImGui::Text("🔵 Window 2 - Secondary Tools");
    }
    else {
      ImGui::Text("[*] Window 2 - Secondary Tools");
    }
    ImGui::Separator();

    static int counter = 0;
    if (ImGui::Button("Count Up")) {
      counter++;
      wprint(L"Window 2: Counter = " + std::to_wstring(counter) + L"\n");
    }
    ImGui::SameLine();
    ImGui::Text("Count: %d", counter);

    static char buffer[128] = "Hello World";
    ImGui::InputText("Text Input", buffer, sizeof(buffer));

    static ImVec4 color = ImVec4(0.4f, 0.7f, 0.0f, 1.0f);
    ImGui::ColorEdit3("Background Color", (float*)&color);

    // RESTORED: Emoji button tests
    ImGui::Separator();
    ImGui::Text("Emoji Button Tests:");

    if (fontManager.IsEmojiSupported()) {
      if (ImGui::Button(reinterpret_cast<const char*>(u8"🤖 Robot Button"))) {
        wprint(L"🤖 Robot emoji button clicked from Window 2!\n");
      }

      if (ImGui::Button(reinterpret_cast<const char*>(u8"⚡ Lightning Button"))) {
        wprint(L"⚡ Lightning emoji button clicked from Window 2!\n");
      }

      if (ImGui::Button(reinterpret_cast<const char*>(u8"🔧 Tool Button"))) {
        wprint(L"🔧 Tool emoji button clicked from Window 2!\n");
      }
    }
    else {
      if (ImGui::Button("Robot Button [ASCII]")) {
        wprint(L"Robot button clicked from Window 2 (ASCII mode)!\n");
      }
    }

    if (ImGui::Button("Close This Window")) {
      window2.should_close = true;
    }

    if (ImGui::Button("Close Application")) {
      g_running = false;
    }

    ImGui::End();

    // RESTORED: Tools window
    ImGui::Begin("Tools##2");
    ImGui::Text("Tools Panel");
    ImGui::Text("Window: Secondary");
    ImGui::Text("Thread: Main");
    ImGui::Text("Font Status: %s", fontManager.IsEmojiSupported() ? "Emoji Ready" : "ASCII Only");

    // Quick emoji test in tools window
    if (fontManager.IsEmojiSupported()) {
      ImGui::Separator();
      ImGui::Text("Quick Emoji Test:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"😀 😊 👍 🔥 🚀"));
    }

    ImGui::End();
  }

  void RenderComprehensiveTest() {
    ImGui::Begin("🎉 Comprehensive Unicode & Emoji Test", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Status indicators
    ImGui::Text("📊 Font System Status:");
    ImGui::Separator();

    ImVec4 green = ImVec4(0, 1, 0, 1);
    ImVec4 red = ImVec4(1, 0, 0, 1);

    ImGui::TextColored(fontManager.IsEmojiSupported() ? green : red,
      "%s Emoji Support", fontManager.IsEmojiSupported() ? "✅" : "❌");
    ImGui::TextColored(fontManager.IsGreekSupported() ? green : red,
      "%s Greek Letters", fontManager.IsGreekSupported() ? "✅" : "❌");
    ImGui::TextColored(fontManager.IsMathSupported() ? green : red,
      "%s Math Symbols", fontManager.IsMathSupported() ? "✅" : "❌");

    ImGui::Separator();

    // SOLUTION 6: Comprehensive Unicode testing with proper UTF-8 encoding
    if (ImGui::CollapsingHeader("😀 Emoji Tests", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Basic emotions:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"😀 😃 😄 😁 😊 😍 🥰 😘"));

      ImGui::Text("Hand gestures:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"👍 👎 👌 ✌️ 🤞 🤟 🤘 👋"));

      ImGui::Text("Hearts and symbols:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"❤️ 💙 💚 💛 🧡 💜 🖤 🤍"));

      ImGui::Text("Objects and tools:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🔧 🔨 ⚙️ 🖥️ 💻 📱 ⌚ 🔋"));

      ImGui::Text("Transportation:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🚗 🚕 🚙 🚌 🚎 🏎️ 🚓 🚑"));
    }

    if (ImGui::CollapsingHeader("🔤 Greek Letters", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Lowercase Greek:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"α β γ δ ε ζ η θ ι κ λ μ ν ξ ο π ρ σ τ υ φ χ ψ ω"));

      ImGui::Text("Uppercase Greek:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"Α Β Γ Δ Ε Ζ Η Θ Ι Κ Λ Μ Ν Ξ Ο Π Ρ Σ Τ Υ Φ Χ Ψ Ω"));

      ImGui::Text("Common in science:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"π (pi) μ (mu) α (alpha) β (beta) γ (gamma) δ (delta) λ (lambda) Ω (omega)"));
    }

    if (ImGui::CollapsingHeader("📐 Mathematical Symbols", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Basic math operators:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"± ÷ × ≠ ≈ ≤ ≥ ∞ ∑ ∏ ∫ ∂ ∇"));

      ImGui::Text("Set theory:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"∈ ∉ ∋ ∩ ∪ ⊂ ⊃ ⊆ ⊇ ∅ ℕ ℤ ℚ ℝ ℂ"));

      ImGui::Text("Logic symbols:");
      ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"∧ ∨ ¬ → ↔ ∀ ∃ ⊤ ⊥ ⊢ ⊨"));
    }

    if (ImGui::CollapsingHeader("🔍 Glyph Debugging", ImGuiTreeNodeFlags_DefaultOpen)) {
      // Individual glyph testing
      struct TestChar {
        ImWchar code;
        const char* name;
        const char* utf8;
      };

      TestChar testChars[] = {
          {0x03B1, "Greek alpha", "α"},
          {0x03C0, "Greek pi", "π"},
          {0x1F600, "Grinning face", "😀"},
          {0x1F44D, "Thumbs up", "👍"},
          {0x2764, "Red heart", "❤"},
          {0x00B1, "Plus-minus", "±"},
          {0x221E, "Infinity", "∞"},
          {0x2192, "Right arrow", "→"}
      };

      for (const auto& tc : testChars) {
        bool hasGlyph = fontManager.TestGlyph(tc.code);
        ImGui::TextColored(hasGlyph ? green : red,
          "%s U+%04X (%s): %s",
          hasGlyph ? "✓" : "✗",
          tc.code, tc.name, tc.utf8);
      }
    }

    if (ImGui::Button(reinterpret_cast<const char*>(u8"🎉 Test Button with Emoji!"))) {
      wprint(L"🎉 Emoji button clicked successfully!\n");
    }

    if (ImGui::Button("Close Application")) {
      g_running = false;
    }

    ImGui::End();
  }

public:
  void Cleanup() {
    // RESTORED: Cleanup both ImGui contexts
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

    // RESTORED: Cleanup both windows
    window1.Cleanup();
    window2.Cleanup();

    SDL_Quit();
    wprint(L"✅ Dual window comprehensive cleanup complete\n");
  }
};

int main(int argc, char* argv[]) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  _setmode(_fileno(stdout), _O_U8TEXT);
#endif

  wprint(L"🎯 === COMPREHENSIVE IMGUI EMOJI TEST ===\n");
  wprint(L"This test addresses all common emoji display issues:\n");
  wprint(L"✅ 1. Proper emoji font loading\n");
  wprint(L"✅ 2. Comprehensive Unicode ranges\n");
  wprint(L"✅ 3. Correct font atlas building\n");
  wprint(L"✅ 4. UTF-8 string encoding\n");
  wprint(L"✅ 5. FreeType color emoji support\n");
  wprint(L"\n");

  ComprehensiveEmojiApp app;

  if (!app.Initialize()) {
    wprint(L"❌ Failed to initialize application\n");
    return -1;
  }

  app.Run();
  app.Cleanup();

  wprint(L"👋 Comprehensive emoji test completed successfully! 🎉\n");
  return 0;
}