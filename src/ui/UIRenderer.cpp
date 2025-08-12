
// ui/UIRenderer.cpp
#include "UIRenderer.h"
#include "../utils/Logger.h"
#include "../utils/Unicode.h"
#include "imgui.h"


UIRenderer::UIRenderer(const FontManager& fm, const std::string& winType)
  : fontManager(fm), windowType(winType) {

}

void UIRenderer::Render(std::atomic<bool>& running) {
  if (windowType.find("Window 1") != std::string::npos) {
    RenderWindow1UI(running);
    
  }
  else if (windowType.find("Window 2") != std::string::npos) {
    RenderWindow2UI(running);
  }
}

void UIRenderer::RenderWindow1UI(std::atomic<bool>& running) {
  //RenderControlPanel(running);
  //RenderStatusPanel();
  //RenderComprehensiveTestPanel(running);
}

void UIRenderer::RenderWindow2UI(std::atomic<bool>& running) {
  //RenderToolsPanel(running);

  // Secondary status panel
  //ImGui::Begin("Tools##2");
  //ImGui::Text("Tools Panel");
  //ImGui::Text("Window: Secondary");
  //ImGui::Text("Thread: Main");
  //ImGui::Text("Font Status: %s", fontManager.IsEmojiSupported() ? "Emoji Ready" : "ASCII Only");

  //// Quick emoji test in tools window
  //if (fontManager.IsEmojiSupported()) {
  //  ImGui::Separator();
  //  ImGui::Text("Quick Emoji Test:");
  //  ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"😀 😊 👍 🔥 🚀"));
  //}

  //ImGui::End();
}

void UIRenderer::RenderControlPanel(std::atomic<bool>& running) {
  ImGui::Begin("Main Control Panel");

  // Title with emoji support
  const char* title = GetEmojiOrFallback("🟢 Window 1 - Main Control", "[OK] Window 1 - Main Control");
  ImGui::Text("%s", title);
  ImGui::Separator();

  // Controls
  ImGui::SliderFloat("Control Value", &sliderValue, 0.0f, 100.0f);
  ImGui::Checkbox("Enable Feature", &featureEnabled);

  if (ImGui::Button("Action Button")) {
    LogAction(L"Window 1: Button clicked! Value: " + std::to_wstring(sliderValue));
  }

  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

  if (ImGui::Button("Close This Window")) {
    // Window close will be handled by Application class
    LogAction(L"Window 1 close requested");
  }

  if (ImGui::Button("Close Application")) {
    running = false;
    LogAction(L"Application close requested from Window 1");
  }

  ImGui::End();
}

void UIRenderer::RenderStatusPanel() {
  ImGui::Begin("Status##1");
  ImGui::Text("Status: Running");
  ImGui::Text("Window: Main Control");
  ImGui::Text("Thread: Main");
  ImGui::Text("Emoji Support: %s", fontManager.IsEmojiSupported() ? "Yes" : "No");
  ImGui::Text("Greek Support: %s", fontManager.IsGreekSupported() ? "Yes" : "No");
  ImGui::Text("Math Support: %s", fontManager.IsMathSupported() ? "Yes" : "No");
  ImGui::End();
}

void UIRenderer::RenderToolsPanel(std::atomic<bool>& running) {
  ImGui::Begin("Secondary Tools");

  // Title with emoji support
  const char* title = GetEmojiOrFallback("🔵 Window 2 - Secondary Tools", "[*] Window 2 - Secondary Tools");
  ImGui::Text("%s", title);
  ImGui::Separator();

  // Counter
  if (ImGui::Button("Count Up")) {
    counter++;
    LogAction(L"Window 2: Counter = " + std::to_wstring(counter));
  }
  ImGui::SameLine();
  ImGui::Text("Count: %d", counter);

  // Text input
  ImGui::InputText("Text Input", textBuffer, sizeof(textBuffer));

  // Color picker
  ImGui::ColorEdit3("Background Color", bgColor);

  // Emoji button tests
  ImGui::Separator();
  ImGui::Text("Emoji Button Tests:");

  if (fontManager.IsEmojiSupported()) {
    if (ImGui::Button(reinterpret_cast<const char*>(u8"🤖 Robot Button"))) {
      LogAction(L"🤖 Robot emoji button clicked from Window 2!");
    }

    if (ImGui::Button(reinterpret_cast<const char*>(u8"⚡ Lightning Button"))) {
      LogAction(L"⚡ Lightning emoji button clicked from Window 2!");
    }

    if (ImGui::Button(reinterpret_cast<const char*>(u8"🔧 Tool Button"))) {
      LogAction(L"🔧 Tool emoji button clicked from Window 2!");
    }
  }
  else {
    if (ImGui::Button("Robot Button [ASCII]")) {
      LogAction(L"Robot button clicked from Window 2 (ASCII mode)!");
    }
  }

  if (ImGui::Button("Close This Window")) {
    LogAction(L"Window 2 close requested");
  }

  if (ImGui::Button("Close Application")) {
    running = false;
    LogAction(L"Application close requested from Window 2");
  }

  ImGui::End();
}

void UIRenderer::RenderComprehensiveTestPanel(std::atomic<bool>& running) {
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

  // Emoji tests
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

  // Greek letters
  if (ImGui::CollapsingHeader("🔤 Greek Letters", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Lowercase Greek:");
    ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"α β γ δ ε ζ η θ ι κ λ μ ν ξ ο π ρ σ τ υ φ χ ψ ω"));

    ImGui::Text("Uppercase Greek:");
    ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"Α Β Γ Δ Ε Ζ Η Θ Ι Κ Λ Μ Ν Ξ Ο Π Ρ Σ Τ Υ Φ Χ Ψ Ω"));

    ImGui::Text("Common in science:");
    ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"π (pi) μ (mu) α (alpha) β (beta) γ (gamma) δ (delta) λ (lambda) Ω (omega)"));
  }

  // Mathematical symbols
  if (ImGui::CollapsingHeader("📐 Mathematical Symbols", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Basic math operators:");
    ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"± ÷ × ≠ ≈ ≤ ≥ ∞ ∑ ∏ ∫ ∂ ∇"));

    ImGui::Text("Set theory:");
    ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"∈ ∉ ∋ ∩ ∪ ⊂ ⊃ ⊆ ⊇ ∅ ℕ ℤ ℚ ℝ ℂ"));

    ImGui::Text("Logic symbols:");
    ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"∧ ∨ ¬ → ↔ ∀ ∃ ⊤ ⊥ ⊢ ⊨"));
  }

  // Glyph debugging
  if (ImGui::CollapsingHeader("🔍 Glyph Debugging", ImGuiTreeNodeFlags_DefaultOpen)) {
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
    LogAction(L"🎉 Emoji button clicked successfully!");
  }

  if (ImGui::Button("Close Application")) {
    running = false;
    LogAction(L"Application close requested from test panel");
  }

  ImGui::End();
}

const char* UIRenderer::GetEmojiOrFallback(const char* emoji, const char* fallback) const {
  return fontManager.IsEmojiSupported() ? emoji : fallback;
}

void UIRenderer::LogAction(const std::wstring& action) const {
  Logger::Info(action);
}