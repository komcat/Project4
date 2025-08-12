// ui/UIRenderer.h
#pragma once

#include <string>
#include <atomic>
#include "FontManager.h"


class UIRenderer {
public:
  UIRenderer(const FontManager& fontManager, const std::string& windowType);
  ~UIRenderer() = default;

  void Render(std::atomic<bool>& running);

private:
  // Panel rendering methods
  void RenderControlPanel(std::atomic<bool>& running);
  void RenderStatusPanel();
  void RenderToolsPanel(std::atomic<bool>& running);
  void RenderComprehensiveTestPanel(std::atomic<bool>& running);

  // Window-specific UI
  void RenderWindow1UI(std::atomic<bool>& running);
  void RenderWindow2UI(std::atomic<bool>& running);

  // Helper methods
  const char* GetEmojiOrFallback(const char* emoji, const char* fallback) const;
  void LogAction(const std::wstring& action) const;

  // State
  const FontManager& fontManager;
  std::string windowType;

  // UI state for Window 1
  float sliderValue = 50.0f;
  bool featureEnabled = true;

  // UI state for Window 2
  int counter = 0;
  char textBuffer[128] = "Hello World";
  float bgColor[3] = { 0.4f, 0.7f, 0.0f };



};
