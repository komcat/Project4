# CMakeList.txt : CMake project for Project4, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Project4")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define external libraries paths based on your actual structure
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
set(SDL2_DIR "${EXTERNAL_DIR}/sdl2")
set(FREETYPE_DIR "${EXTERNAL_DIR}/freetype")

# Find SDL2
if(WIN32)
    # For Windows, use your SDL2 structure with x64/x86 subdirectories
    set(SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # x64 build
        set(SDL2_LIBRARIES 
            "${SDL2_DIR}/lib/x64/SDL2.lib"
            "${SDL2_DIR}/lib/x64/SDL2main.lib"
        )
        set(SDL2_DLLS "${SDL2_DIR}/lib/x64/SDL2.dll")
    else()
        # x86 build
        set(SDL2_LIBRARIES 
            "${SDL2_DIR}/lib/x86/SDL2.lib"
            "${SDL2_DIR}/lib/x86/SDL2main.lib"
        )
        set(SDL2_DLLS "${SDL2_DIR}/lib/x86/SDL2.dll")
    endif()
else()
    # For Linux/Mac
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# Setup FreeType based on your actual folder structure
set(FREETYPE_INCLUDE_DIRS "${FREETYPE_DIR}/include")
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # x64 build - try both possible paths
        if(EXISTS "${FREETYPE_DIR}/lib/static/vs2015-2022/win64/freetype.lib")
            set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/lib/static/vs2015-2022/win64/freetype.lib")
        elseif(EXISTS "${FREETYPE_DIR}/lib/win64/freetype.lib")
            set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/lib/win64/freetype.lib")
        else()
            message(WARNING "FreeType library not found in expected locations")
            set(FREETYPE_LIBRARIES "")
        endif()
    else()
        # x86 build
        set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/lib/win32/freetype.lib")
    endif()
else()
    set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/lib/libfreetype.a")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# ImGui setup - using your imgui-docking folder
set(IMGUI_DIR "${EXTERNAL_DIR}/imgui-docking")

# Add ImGui sources based on your actual folder structure
if(EXISTS "${IMGUI_DIR}")
    set(IMGUI_SOURCES
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    )
    
    # Only add FreeType support if we found the library
    if(FREETYPE_LIBRARIES AND EXISTS "${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp")
        list(APPEND IMGUI_SOURCES "${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp")
        message(STATUS "ImGui FreeType support added")
        set(FREETYPE_ENABLED TRUE)
    else()
        message(STATUS "ImGui FreeType support disabled - library not found")
        set(FREETYPE_ENABLED FALSE)
    endif()
    
    message(STATUS "ImGui found at: ${IMGUI_DIR}")
else()
    set(IMGUI_SOURCES "")
    message(WARNING "ImGui not found at: ${IMGUI_DIR}")
    set(FREETYPE_ENABLED FALSE)
endif()

# Define your project source files based on current structure
set(PROJECT_SOURCES
    # Main entry point
    "src/main.cpp"
    
    # Core application files
    "src/core/Application.cpp"
    "src/core/Window.cpp"
    
    # UI system files
    "src/ui/FontManager.cpp"
    "src/ui/UIRenderer.cpp"
    
    # Utility files  
    "src/utils/Logger.cpp"
    "src/utils/Unicode.cpp"

     
)

# Define your project header files based on current structure
set(PROJECT_HEADERS
    # Core headers (in src/ directory based on your structure)
    "src/core/Application.h"
    "src/core/Window.h"
    
    # UI headers (in src/ directory)
    "src/ui/FontManager.h" 
    "src/ui/UIRenderer.h"
    
    # Utility headers (in src/ directory)
    "src/utils/Logger.h"
    "src/utils/Unicode.h"
    
    # Legacy header (if still needed)
    "include/ui/DualWindowManager.h"

    "src/ui/services/UIServiceRegistry.h"

    "src/ui/CleanMainUI.h"
    "src/ui/services/DemoUIServices.h"
)

# Check which files actually exist and warn about missing ones
foreach(source_file ${PROJECT_SOURCES})
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${source_file}")
        message(WARNING "Source file not found: ${source_file}")
    endif()
endforeach()

foreach(header_file ${PROJECT_HEADERS})
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${header_file}")
        message(WARNING "Header file not found: ${header_file}")
    endif()
endforeach()

# Add source to this project's executable
add_executable(Project4 
    ${PROJECT_SOURCES}
    ${IMGUI_SOURCES}
    # Headers are included for IDE organization
    "src/core/ServiceLocator.h" "src/ui/services/UIServiceRegistry.h" "src/ui/CleanMainUI.h" "src/ui/CleanMainUI.cpp"  "src/ui/services/run/RunProductService.h" "src/ui/services/run/ProcessSetupService.h" "src/ui/services/ServiceInitializer.h" "src/ui/services/manual/PIControlService.h" "src/ui/services/vision/VisionService.h" "src/ui/services/data/DataMonitorService.h" "src/ui/services/manual/GantryService.h" "src/ui/services/manual/IOControlService.h" "src/ui/services/manual/PneumaticService.h" "src/ui/services/data/SMUService.h" "src/ui/services/data/CLD101xService.h" "src/ui/services/vision/FiducialService.h" "src/ui/services/vision/DatumService.h" "src/ui/services/config/ConfigService.h" "src/ui/services/config/SystemInfoService.h" "src/ui/services/program/ProgrammingService.h" "src/ui/services/program/MacroService.h")

# Create source groups for better IDE organization
source_group("Source Files" FILES ${PROJECT_SOURCES})
source_group("Header Files" FILES ${PROJECT_HEADERS})
source_group("External\\ImGui" FILES ${IMGUI_SOURCES})

# Create detailed source groups for new structure
source_group("Source Files\\Core" FILES 
    "src/core/Application.cpp" 
    "src/core/Window.cpp"
)
source_group("Source Files\\UI" FILES 
    "src/ui/FontManager.cpp" 
    "src/ui/UIRenderer.cpp"
)
source_group("Source Files\\Utils" FILES 
    "src/utils/Logger.cpp" 
    "src/utils/Unicode.cpp"
)
source_group("Header Files\\Core" FILES 
    "src/core/Application.h" 
    "src/core/Window.h"
)
source_group("Header Files\\UI" FILES 
    "src/ui/FontManager.h" 
    "src/ui/UIRenderer.h"
)
source_group("Header Files\\Utils" FILES 
    "src/utils/Logger.h" 
    "src/utils/Unicode.h"
)

# Include directories - adjusted for your structure
target_include_directories(Project4 PRIVATE
    "${CMAKE_SOURCE_DIR}/src"          # This allows #include "core/Application.h" from src/
    "${CMAKE_SOURCE_DIR}/include"      # This allows #include "ui/DualWindowManager.h" (legacy)
    "${CMAKE_SOURCE_DIR}/include/ui"   # This allows #include "DualWindowManager.h" (legacy)
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Add FreeType includes only if enabled
if(FREETYPE_ENABLED)
    target_include_directories(Project4 PRIVATE
        ${IMGUI_DIR}/misc/freetype
        ${FREETYPE_INCLUDE_DIRS}
    )
endif()

# Link libraries
target_link_libraries(Project4 
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# Link FreeType only if enabled
if(FREETYPE_ENABLED)
    target_link_libraries(Project4 ${FREETYPE_LIBRARIES})
    target_compile_definitions(Project4 PRIVATE IMGUI_ENABLE_FREETYPE)
endif()

# Windows specific settings
if(WIN32)
    # Copy SDL2 DLLs to output directory
    if(SDL2_DLLS AND EXISTS ${SDL2_DLLS})
        add_custom_command(TARGET Project4 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_DLLS}
            $<TARGET_FILE_DIR:Project4>)
    endif()
    
    # Link additional Windows libraries
    target_link_libraries(Project4 
        winmm
        gdi32
        opengl32
    )
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(Project4 PRIVATE /W3)
    target_compile_definitions(Project4 PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(Project4 PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Project4 PROPERTY CXX_STANDARD 20)
endif()

# Debug output to help diagnose issues
message(STATUS "=== Project4 Build Configuration ===")
message(STATUS "Project Sources: ${PROJECT_SOURCES}")
message(STATUS "Project Headers: ${PROJECT_HEADERS}")
message(STATUS "SDL2_DIR: ${SDL2_DIR}")
message(STATUS "SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2_LIBRARIES: ${SDL2_LIBRARIES}")
message(STATUS "FREETYPE_DIR: ${FREETYPE_DIR}")
message(STATUS "FREETYPE_INCLUDE_DIRS: ${FREETYPE_INCLUDE_DIRS}")
message(STATUS "FREETYPE_LIBRARIES: ${FREETYPE_LIBRARIES}")
message(STATUS "FREETYPE_ENABLED: ${FREETYPE_ENABLED}")
message(STATUS "ImGui Sources Count: ${IMGUI_SOURCES}")
message(STATUS "New Project Structure: Enabled")
message(STATUS "=====================================")