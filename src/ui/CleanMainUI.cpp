// src/ui/CleanMainUI.cpp
#include "CleanMainUI.h"
#include "imgui.h"
#include <iostream>
#include <ctime>

void CleanMainUI::RenderUI(std::atomic<bool>& running) {
  // Handle keyboard shortcuts first
  HandleKeyboardShortcuts();

  // Full screen window
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::Begin("CleanMainApplication", nullptr, window_flags);

  // Top navigation
  if (currentCategory == "Home") {
    RenderTopMenuBar();
  }
  else {
    RenderBackButton();
  }

  RenderDateTime();
  RenderBreadcrumbs();
  ImGui::Separator();
  RenderMainContent(running);

  ImGui::End();
}

void CleanMainUI::RenderTopMenuBar() {
  ImGui::Dummy(ImVec2(0.0f, 12.0f));

  // Simple "Home" indicator with quick navigation hint
  ImGui::Text("Home  (Press 1-6 for quick navigation)");
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "1=Config  2=Vision  3=Data  4=Program  5=Manual  6=Run");
  ImGui::Spacing();
}

void CleanMainUI::RenderBackButton() {
  ImGui::Dummy(ImVec2(0.0f, 15.0f));

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15, 8));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

  if (ImGui::Button("<< BACK")) {
    NavigateBack();
  }

  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar();

  // Add escape key hint
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(or press Escape)");
}

void CleanMainUI::RenderDateTime() {
  time_t rawtime;
  struct tm timeinfo;
  char buffer[80];

  time(&rawtime);
#ifdef _WIN32
  localtime_s(&timeinfo, &rawtime);
#else
  timeinfo = *localtime(&rawtime);
#endif

  strftime(buffer, sizeof(buffer), "%d %b %Y\n%H:%M:%S", &timeinfo);

  ImVec2 textSize = ImGui::CalcTextSize(buffer);
  ImGui::SameLine(ImGui::GetWindowWidth() - textSize.x - 20);
  ImGui::SetCursorPosY(10);
  ImGui::Text("%s", buffer);
}

void CleanMainUI::RenderBreadcrumbs() {
  std::string breadcrumb = "Home";

  if (currentCategory != "Home") {
    breadcrumb += " > " + GetCategoryDisplayName(currentCategory);

    if (!currentService.empty()) {
      auto service = UIServiceRegistry::Instance().GetService(currentService);
      if (service) {
        breadcrumb += " > " + service->GetDisplayName();
      }
    }
  }

  ImGui::Text("%s", breadcrumb.c_str());
}

void CleanMainUI::RenderMainContent(std::atomic<bool>& running) {
  ImGui::SetCursorPosY(100);

  if (currentCategory == "Home") {
    RenderHomePage();
  }
  else if (currentService.empty()) {
    RenderCategoryPage(currentCategory);
  }
  else {
    RenderServicePage(currentService, running);
  }
}

void CleanMainUI::RenderHomePage() {
  // Welcome header
  ImGui::SetWindowFontScale(2.0f);
  ImGui::Text("Welcome to Project4 UI");
  ImGui::SetWindowFontScale(1.0f);

  ImGui::Spacing();
  ImGui::Text("Select a category from the grid below to begin:");

  // Comprehensive emoji test section
  ImGui::Spacing();
  ImGui::Text("Emoji Test (UTF-8 encoded):");

  // Use proper UTF-8 encoding for emoji strings with reinterpret_cast
  ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🔧 ⚙️ 📊 🕹️ 👁️ 🚀"));
  ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"Target: 🎯, Robot: 🤖, Lightning: ⚡, Camera: 📷"));

  // Test individual emoji
  ImGui::Text("Individual tests:");
  ImGui::SameLine(); ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🎯"));
  ImGui::SameLine(); ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🤖"));
  ImGui::SameLine(); ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"⚡"));
  ImGui::SameLine(); ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"📷"));
  ImGui::SameLine(); ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"👁️"));
  ImGui::SameLine(); ImGui::TextUnformatted(reinterpret_cast<const char*>(u8"🚀"));

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // MAIN GRID LAYOUT - 3x2 grid of large buttons
  RenderCategoryGrid();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // System Status section
  RenderSystemStatus();

  // Font debug info
  RenderFontDebugInfo();
}

void CleanMainUI::RenderCategoryGrid() {
  auto& registry = UIServiceRegistry::Instance();
  auto categories = registry.GetAllCategories();

  // Calculate grid layout
  float windowWidth = ImGui::GetContentRegionAvail().x;
  float buttonWidth = (windowWidth - 40.0f) / 3.0f; // 3 columns with spacing
  float buttonHeight = 120.0f; // Large buttons

  // Push style for rounded corners and spacing
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f); // Rounded corners
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 20));

  // Define category order with GREY color scheme
  struct CategoryInfo {
    std::string name;
    ImVec4 color;
    ImVec4 hoverColor;
  };

  std::vector<CategoryInfo> categoryLayout = {
    // Light grey to dark grey gradient scheme
    {"Config", ImVec4(0.45f, 0.45f, 0.45f, 1.0f), ImVec4(0.55f, 0.55f, 0.55f, 1.0f)},
    {"Vision", ImVec4(0.50f, 0.50f, 0.50f, 1.0f), ImVec4(0.60f, 0.60f, 0.60f, 1.0f)},
    {"Data", ImVec4(0.55f, 0.55f, 0.55f, 1.0f), ImVec4(0.65f, 0.65f, 0.65f, 1.0f)},
    {"Program", ImVec4(0.40f, 0.40f, 0.40f, 1.0f), ImVec4(0.50f, 0.50f, 0.50f, 1.0f)},
    {"Manual", ImVec4(0.48f, 0.48f, 0.48f, 1.0f), ImVec4(0.58f, 0.58f, 0.58f, 1.0f)},
    {"Run", ImVec4(0.52f, 0.52f, 0.52f, 1.0f), ImVec4(0.62f, 0.62f, 0.62f, 1.0f)}
  };

  // Render top row (3 buttons)
  for (int i = 0; i < 3 && i < categoryLayout.size(); i++) {
    if (i > 0) ImGui::SameLine();

    const auto& catInfo = categoryLayout[i];

    // Find if this category exists in registry
    bool categoryExists = std::find(categories.begin(), categories.end(), catInfo.name) != categories.end();

    ImGui::PushStyleColor(ImGuiCol_Button, catInfo.color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, catInfo.hoverColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(catInfo.hoverColor.x * 0.8f, catInfo.hoverColor.y * 0.8f, catInfo.hoverColor.z * 0.8f, 1.0f));

    // Use UTF-8 encoded emoji in button text
    std::string buttonText = GetCategoryIcon(catInfo.name) + "\n" + GetCategoryDisplayName(catInfo.name);

    if (ImGui::Button(buttonText.c_str(), ImVec2(buttonWidth, buttonHeight))) {
      if (categoryExists) {
        NavigateToCategory(catInfo.name);
      }
    }

    ImGui::PopStyleColor(3);
  }

  // Add spacing between rows
  ImGui::Dummy(ImVec2(0, 20));

  // Render bottom row (3 buttons)
  for (int i = 3; i < 6 && i < categoryLayout.size(); i++) {
    if (i > 3) ImGui::SameLine();

    const auto& catInfo = categoryLayout[i];

    // Find if this category exists in registry
    bool categoryExists = std::find(categories.begin(), categories.end(), catInfo.name) != categories.end();

    ImGui::PushStyleColor(ImGuiCol_Button, catInfo.color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, catInfo.hoverColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(catInfo.hoverColor.x * 0.8f, catInfo.hoverColor.y * 0.8f, catInfo.hoverColor.z * 0.8f, 1.0f));

    // Use UTF-8 encoded emoji in button text
    std::string buttonText = GetCategoryIcon(catInfo.name) + "\n" + GetCategoryDisplayName(catInfo.name);

    if (ImGui::Button(buttonText.c_str(), ImVec2(buttonWidth, buttonHeight))) {
      if (categoryExists) {
        NavigateToCategory(catInfo.name);
      }
    }

    ImGui::PopStyleColor(3);
  }

  ImGui::PopStyleVar(2); // Pop rounding and padding
}

void CleanMainUI::RenderSystemStatus() {
  ImGui::SetWindowFontScale(1.3f);
  ImGui::Text("System Status");
  ImGui::SetWindowFontScale(1.0f);

  ImGui::Spacing();

  auto& registry = UIServiceRegistry::Instance();
  auto categories = registry.GetAllCategories();

  // Service summary
  for (const auto& category : categories) {
    auto services = registry.GetServicesByCategory(category);
    int availableCount = 0;
    for (const auto& service : services) {
      if (service.available) availableCount++;
    }

    ImGui::BulletText("%s %s - %d services available (%d total)",
      GetCategoryIcon(category).c_str(),
      GetCategoryDisplayName(category).c_str(),
      availableCount,
      (int)services.size());
  }

  ImGui::Spacing();
  ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ UI Service Registry: Ready");
  ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Clean Architecture: Active");

  int totalServices = 0;
  int availableServices = 0;
  for (const auto& category : categories) {
    auto services = registry.GetServicesByCategory(category);
    totalServices += services.size();
    for (const auto& service : services) {
      if (service.available) availableServices++;
    }
  }

  ImGui::Text("✓ Services: %d available of %d registered", availableServices, totalServices);
}

void CleanMainUI::RenderFontDebugInfo() {
  ImGui::Spacing();
  ImGui::Text("Font System Debug:");
  ImGui::BulletText("Font Atlas Built: %s", ImGui::GetIO().Fonts->IsBuilt() ? "Yes" : "No");
  ImGui::BulletText("Font Count: %d", ImGui::GetIO().Fonts->Fonts.Size);

  if (ImGui::GetIO().FontDefault) {
    ImGui::BulletText("Default Font Glyphs: %d", ImGui::GetIO().FontDefault->Glyphs.Size);

    // Test specific emoji glyphs
    ImFont* font = ImGui::GetIO().FontDefault;
    ImWchar testChars[] = { 0x1F3AF, 0x1F916, 0x26A1, 0x1F4F7, 0 };  // 🎯🤖⚡📷

    for (int i = 0; testChars[i] != 0; i++) {
      const ImFontGlyph* glyph = font->FindGlyph(testChars[i]);
      ImGui::BulletText("U+%04X: %s", testChars[i], glyph ? "Found" : "Missing");
    }
  }
  else {
    ImGui::BulletText("Default Font: NULL");
  }
}

void CleanMainUI::RenderCategoryPage(const std::string& category) {
  ImGui::SetWindowFontScale(1.5f);
  ImGui::Text("%s %s", GetCategoryIcon(category).c_str(), GetCategoryDisplayName(category).c_str());
  ImGui::SetWindowFontScale(1.0f);

  ImGui::Spacing();
  ImGui::Text("Select a service:");
  ImGui::Spacing();

  auto& registry = UIServiceRegistry::Instance();
  auto services = registry.GetServicesByCategory(category);

  for (size_t i = 0; i < services.size(); ++i) {
    const auto& service = services[i];

    std::string buttonText = service.icon + " " + service.displayName;

    // Color code based on availability - using grey tones
    if (service.available) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    }
    else {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
    }

    if (ImGui::Button(buttonText.c_str(), ImVec2(250, 50))) {
      if (service.available) {
        NavigateToService(service.serviceName);
      }
    }

    ImGui::PopStyleColor(2);

    if (!service.available) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(Not Available)");
    }
  }
}

void CleanMainUI::RenderServicePage(const std::string& serviceName, std::atomic<bool>& running) {
  auto& registry = UIServiceRegistry::Instance();
  auto service = registry.GetService(serviceName);

  if (service && service->IsAvailable()) {
    service->RenderUI();
  }
  else {
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Service Not Available");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    if (service) {
      ImGui::Text("Service '%s' is registered but not available.", service->GetDisplayName().c_str());
      ImGui::Text("This typically means required dependencies are not initialized.");
    }
    else {
      ImGui::Text("Service '%s' is not registered.", serviceName.c_str());
    }
  }
}

// Helper methods
std::string CleanMainUI::GetCategoryDisplayName(const std::string& category) {
  if (category == "Manual") return "Manual Control";
  if (category == "Data") return "Data & Instrument";
  if (category == "Program") return "Programming";
  if (category == "Config") return "Configuration";
  if (category == "Vision") return "Vision System";
  if (category == "Run") return "Run";
  return category;
}

std::string CleanMainUI::GetCategoryIcon(const std::string& category) {
  // Use UTF-8 encoded emoji strings with reinterpret_cast
  if (category == "Manual") return reinterpret_cast<const char*>(u8"🕹️");
  if (category == "Data") return reinterpret_cast<const char*>(u8"📊");
  if (category == "Program") return reinterpret_cast<const char*>(u8"⚙️");
  if (category == "Config") return reinterpret_cast<const char*>(u8"🔧");
  if (category == "Vision") return reinterpret_cast<const char*>(u8"👁️");
  if (category == "Run") return reinterpret_cast<const char*>(u8"🚀");
  return reinterpret_cast<const char*>(u8"📋");
}

void CleanMainUI::NavigateToCategory(const std::string& category) {
  currentCategory = category;
  currentService = "";
}

void CleanMainUI::NavigateToService(const std::string& serviceName) {
  currentService = serviceName;
}

void CleanMainUI::NavigateBack() {
  if (!currentService.empty()) {
    currentService = "";
  }
  else {
    currentCategory = "Home";
  }
}

void CleanMainUI::HandleKeyboardShortcuts() {
  // Handle Escape key from any page for universal back navigation
  if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    NavigateBack();
    return; // Exit early after handling escape
  }

  // Only handle number shortcuts when we're on the home page
  if (currentCategory != "Home") return;

  // Check for number key presses (1-6) using modern ImGui key handling
  if (ImGui::IsKeyPressed(ImGuiKey_1)) {
    NavigateToCategory("Config");
  }
  else if (ImGui::IsKeyPressed(ImGuiKey_2)) {
    NavigateToCategory("Vision");
  }
  else if (ImGui::IsKeyPressed(ImGuiKey_3)) {
    NavigateToCategory("Data");
  }
  else if (ImGui::IsKeyPressed(ImGuiKey_4)) {
    NavigateToCategory("Program");
  }
  else if (ImGui::IsKeyPressed(ImGuiKey_5)) {
    NavigateToCategory("Manual");
  }
  else if (ImGui::IsKeyPressed(ImGuiKey_6)) {
    NavigateToCategory("Run");
  }
}