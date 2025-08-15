# CMakeList.txt : CMake project for Project4 with auto-discovery
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

# ========================================
# AUTO-DISCOVERY MACROS
# ========================================

# Macro to recursively find all source files
macro(discover_sources target_var directory)
    file(GLOB_RECURSE ${target_var}
        "${directory}/*.cpp"
        "${directory}/*.c"
        "${directory}/*.cxx"
        "${directory}/*.cc"
    )
endmacro()

# Macro to recursively find all header files
macro(discover_headers target_var directory)
    file(GLOB_RECURSE ${target_var}
        "${directory}/*.h"
        "${directory}/*.hpp"
        "${directory}/*.hxx"
    )
endmacro()

# Macro to filter out specific files from a list
macro(filter_out list_var pattern)
    list(FILTER ${list_var} EXCLUDE REGEX ${pattern})
endmacro()

# Print discovered files for debugging (simplified)
function(print_file_list title file_list)
    message(STATUS "=== ${title} ===")
    foreach(file IN LISTS file_list)
        file(RELATIVE_PATH rel_file "${CMAKE_SOURCE_DIR}" "${file}")
        message(STATUS "  Found: ${rel_file}")
    endforeach()
    list(LENGTH file_list file_count)
    message(STATUS "  Total: ${file_count} files")
    message(STATUS "")
endfunction()

# ========================================
# EXTERNAL LIBRARIES SETUP
# ========================================

# Define external libraries paths
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(SDL2_DIR "${EXTERNAL_DIR}/sdl2")
set(FREETYPE_DIR "${EXTERNAL_DIR}/freetype")
set(NLOHMANN_JSON_DIR "${EXTERNAL_DIR}/nlohmann_json")
set(PI_DIR "${EXTERNAL_DIR}/PI")
set(SRCUTILS_DIR "${CMAKE_SOURCE_DIR}/src/utils")

# ========================================
# PI GCS2 DLL SETUP - THIS WAS MISSING!
# ========================================
set(PI_GCS2_INCLUDE_DIR "${PI_DIR}")

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # x64 build
        set(PI_GCS2_LIBRARIES "${PI_DIR}/PI_GCS2_DLL_x64.lib")
        set(PI_GCS2_DLLS "${PI_DIR}/PI_GCS2_DLL_x64.dll")
    else()
        # x86 build  
        set(PI_GCS2_LIBRARIES "${PI_DIR}/PI_GCS2_DLL.lib")
        set(PI_GCS2_DLLS "${PI_DIR}/PI_GCS2_DLL.dll")
    endif()
    
    # Check if the library file exists
    if(NOT EXISTS "${PI_GCS2_LIBRARIES}")
        message(FATAL_ERROR "PI GCS2 library not found at: ${PI_GCS2_LIBRARIES}")
    endif()
    
    message(STATUS "PI GCS2 library found: ${PI_GCS2_LIBRARIES}")
else()
    message(WARNING "PI GCS2 DLL setup is only configured for Windows")
endif()

# Setup nlohmann/json
set(NLOHMANN_JSON_INCLUDE_DIR "")
if(EXISTS "${NLOHMANN_JSON_DIR}/single_include")
    set(NLOHMANN_JSON_INCLUDE_DIR "${NLOHMANN_JSON_DIR}/single_include")
    message(STATUS "Found nlohmann/json single header at: ${NLOHMANN_JSON_INCLUDE_DIR}")
elseif(EXISTS "${NLOHMANN_JSON_DIR}/include")
    set(NLOHMANN_JSON_INCLUDE_DIR "${NLOHMANN_JSON_DIR}/include")
    message(STATUS "Found nlohmann/json include directory at: ${NLOHMANN_JSON_INCLUDE_DIR}")
else()
    message(WARNING "nlohmann/json not found in expected locations")
endif()

# Find SDL2
if(WIN32)
    set(SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_LIBRARIES 
            "${SDL2_DIR}/lib/x64/SDL2.lib"
            "${SDL2_DIR}/lib/x64/SDL2main.lib"
        )
        set(SDL2_DLLS "${SDL2_DIR}/lib/x64/SDL2.dll")
    else()
        set(SDL2_LIBRARIES 
            "${SDL2_DIR}/lib/x86/SDL2.lib"
            "${SDL2_DIR}/lib/x86/SDL2main.lib"
        )
        set(SDL2_DLLS "${SDL2_DIR}/lib/x86/SDL2.dll")
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# Setup FreeType
set(FREETYPE_INCLUDE_DIRS "${FREETYPE_DIR}/include")
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if(EXISTS "${FREETYPE_DIR}/lib/static/vs2015-2022/win64/freetype.lib")
            set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/lib/static/vs2015-2022/win64/freetype.lib")
        elseif(EXISTS "${FREETYPE_DIR}/lib/win64/freetype.lib")
            set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/lib/win64/freetype.lib")
        else()
            set(FREETYPE_LIBRARIES "")
        endif()
    else()
        set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/lib/win32/freetype.lib")
    endif()
else()
    set(FREETYPE_LIBRARIES "${FREETYPE_DIR}/lib/libfreetype.a")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# ImGui setup
set(IMGUI_DIR "${EXTERNAL_DIR}/imgui-docking")
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
    
    if(FREETYPE_LIBRARIES AND EXISTS "${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp")
        list(APPEND IMGUI_SOURCES "${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp")
        set(FREETYPE_ENABLED TRUE)
    else()
        set(FREETYPE_ENABLED FALSE)
    endif()
else()
    set(IMGUI_SOURCES "")
    set(FREETYPE_ENABLED FALSE)
endif()

# ========================================
# AUTO-DISCOVER PROJECT FILES
# ========================================

# Discover all sources and headers
discover_sources(ALL_PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/src")
discover_headers(ALL_PROJECT_HEADERS "${CMAKE_SOURCE_DIR}/src")

# Also discover headers from include directory if it exists
if(EXISTS "${CMAKE_SOURCE_DIR}/include")
    discover_headers(LEGACY_HEADERS "${CMAKE_SOURCE_DIR}/include")
    list(APPEND ALL_PROJECT_HEADERS ${LEGACY_HEADERS})
endif()

# ========================================
# SEPARATE MAIN APP, TESTS, AND SHARED SOURCES
# ========================================

set(MAIN_APP_SOURCES ${ALL_PROJECT_SOURCES})
set(TEST_MAIN_SOURCES)
set(TEST_CONFIG_SOURCES)
set(SHARED_SOURCES)

# Separate different types of sources
foreach(source ${ALL_PROJECT_SOURCES})
    if(source MATCHES ".*main\\.cpp$")
        # This is the main application entry point (but don't add to MAIN_APP_SOURCES here, we'll filter later)
        # Skip for now
    elseif(source MATCHES ".*TestMain\\.cpp$")
        # This is the original TestMain
        list(APPEND TEST_MAIN_SOURCES ${source})
    elseif(source MATCHES ".*TestConfigMain\\.cpp$")
        # This is the new ConfigManager test
        list(APPEND TEST_CONFIG_SOURCES ${source})
    elseif(source MATCHES ".*Test.*\\.cpp$" OR source MATCHES ".*test.*\\.cpp$")
        # Other test files - skip them for now
        # We'll handle them separately if needed in the future
    else()
        # These are shared sources that can be used by all executables
        list(APPEND SHARED_SOURCES ${source})
    endif()
endforeach()

# Remove test sources from main app
filter_out(MAIN_APP_SOURCES ".*TestMain\\.cpp$")
filter_out(MAIN_APP_SOURCES ".*TestConfigMain\\.cpp$")
filter_out(MAIN_APP_SOURCES ".*Test.*\\.cpp$")
filter_out(MAIN_APP_SOURCES ".*test.*\\.cpp$")

# Define which shared sources each test needs
set(CONFIG_TEST_SHARED_SOURCES)
foreach(source ${SHARED_SOURCES})
    # ConfigManager test needs core utilities and ConfigManager itself
    if(source MATCHES ".*core/ConfigManager\\.cpp$" OR
       source MATCHES ".*core/ConfigRegistry\\.cpp$" OR
       source MATCHES ".*utils/.*\\.cpp$")
        list(APPEND CONFIG_TEST_SHARED_SOURCES ${source})
    endif()
endforeach()

# Define shared sources for original TestMain
set(TESTMAIN_SHARED_SOURCES)
foreach(source ${SHARED_SOURCES})
    # TestMain needs device-related sources, config sources, and utils
    if(source MATCHES ".*devices/.*\\.cpp$" OR
       source MATCHES ".*core/ConfigManager\\.cpp$" OR
       source MATCHES ".*core/ConfigRegistry\\.cpp$" OR
       source MATCHES ".*utils/.*\\.cpp$")
        list(APPEND TESTMAIN_SHARED_SOURCES ${source})
    endif()
endforeach()

# Print discovered files for debugging
print_file_list("ALL PROJECT SOURCES" "${ALL_PROJECT_SOURCES}")
print_file_list("MAIN APP SOURCES" "${MAIN_APP_SOURCES}")
print_file_list("TEST MAIN SOURCES" "${TEST_MAIN_SOURCES}")
print_file_list("TEST CONFIG SOURCES" "${TEST_CONFIG_SOURCES}")
print_file_list("CONFIG TEST SHARED" "${CONFIG_TEST_SHARED_SOURCES}")

# ========================================
# BUILD MAIN APPLICATION (Project4)
# ========================================

if(MAIN_APP_SOURCES)
    message(STATUS "Building main application: Project4")
    
    add_executable(Project4 
        ${MAIN_APP_SOURCES}
        ${IMGUI_SOURCES}
    )
    
    # Include directories
    target_include_directories(Project4 PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/include"
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${PI_GCS2_INCLUDE_DIR}  # ADD PI GCS2 INCLUDE
    )
    
    if(FREETYPE_ENABLED)
        target_include_directories(Project4 PRIVATE
            ${IMGUI_DIR}/misc/freetype
            ${FREETYPE_INCLUDE_DIRS}
        )
    endif()
    
    # Link libraries - ADD PI GCS2 LIBRARY HERE!
    target_link_libraries(Project4 
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${PI_GCS2_LIBRARIES}  # THIS WAS MISSING!
    )
    
    if(FREETYPE_ENABLED)
        target_link_libraries(Project4 ${FREETYPE_LIBRARIES})
        target_compile_definitions(Project4 PRIVATE IMGUI_ENABLE_FREETYPE)
    endif()
    
else()
    message(FATAL_ERROR "No main application sources found!")
endif()

# ========================================
# BUILD ORIGINAL TEST APPLICATION (TestMain)
# ========================================

if(TEST_MAIN_SOURCES)
    message(STATUS "Building original test application: TestMain")
    
    add_executable(TestMain 
        ${TEST_MAIN_SOURCES}
        ${TESTMAIN_SHARED_SOURCES}
        "external/PI/PI_GCS2_DLL.h"
    )
    
    # Include directories (minimal for test)
    target_include_directories(TestMain PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/include"
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${PI_GCS2_INCLUDE_DIR}  # ADD PI GCS2 INCLUDE FOR TEST
        ${SRCUTILS_DIR}
    )
    
    # Link PI GCS2 library for TestMain too
    target_link_libraries(TestMain 
        ${PI_GCS2_LIBRARIES}  # ADD THIS LINE!
    )
    
else()
    message(STATUS "TestMain.cpp not found - skipping original test executable")
endif()

# ========================================
# BUILD CONFIG MANAGER TEST (TestConfigManager)
# ========================================

if(TEST_CONFIG_SOURCES)
    message(STATUS "Building ConfigManager test application: TestConfigManager")
    
    add_executable(TestConfigManager 
        ${TEST_CONFIG_SOURCES}
        ${CONFIG_TEST_SHARED_SOURCES}
    )
    
    # Include directories for ConfigManager test
    target_include_directories(TestConfigManager PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/include"
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${PI_GCS2_INCLUDE_DIR}  # ADD PI GCS2 INCLUDE FOR CONFIG TEST TOO
    )
    
    # Link PI GCS2 library for TestConfigManager if it uses PI functions
    # (Only add this if TestConfigManager actually uses PI functions)
    # target_link_libraries(TestConfigManager ${PI_GCS2_LIBRARIES})
    
    # Create a custom target to run the config test
    add_custom_target(run_config_test
        COMMAND TestConfigManager
        DEPENDS TestConfigManager
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running ConfigManager tests"
    )
    
else()
    message(STATUS "TestConfigMain.cpp not found - skipping ConfigManager test executable")
endif()

# ========================================
# COPY DLL FILES TO OUTPUT DIRECTORY
# ========================================

# Copy PI GCS2 DLL to output directory for main app
if(WIN32 AND TARGET Project4 AND PI_GCS2_DLLS AND EXISTS ${PI_GCS2_DLLS})
    add_custom_command(TARGET Project4 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PI_GCS2_DLLS}
        $<TARGET_FILE_DIR:Project4>
        COMMENT "Copying PI GCS2 DLL to output directory"
    )
endif()

# Copy PI GCS2 DLL for TestMain too
if(WIN32 AND TARGET TestMain AND PI_GCS2_DLLS AND EXISTS ${PI_GCS2_DLLS})
    add_custom_command(TARGET TestMain POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PI_GCS2_DLLS}
        $<TARGET_FILE_DIR:TestMain>
        COMMENT "Copying PI GCS2 DLL to TestMain output directory"
    )
endif()

# ========================================
# SOURCE GROUPS FOR IDE ORGANIZATION
# ========================================

# Create source groups for main app
if(TARGET Project4)
    foreach(source ${MAIN_APP_SOURCES})
        file(RELATIVE_PATH rel_source "${CMAKE_SOURCE_DIR}/src" "${source}")
        get_filename_component(source_dir "${rel_source}" DIRECTORY)
        string(REPLACE "/" "\\" source_group_name "${source_dir}")
        source_group("Source Files\\${source_group_name}" FILES "${source}")
    endforeach()
    
    source_group("External\\ImGui" FILES ${IMGUI_SOURCES})
endif()

# Create source groups for headers
foreach(header ${ALL_PROJECT_HEADERS})
    file(RELATIVE_PATH rel_header "${CMAKE_SOURCE_DIR}" "${header}")
    get_filename_component(header_dir "${rel_header}" DIRECTORY)
    string(REPLACE "/" "\\" header_group_name "${header_dir}")
    source_group("Header Files\\${header_group_name}" FILES "${header}")
endforeach()

# Create source groups for test applications
if(TARGET TestMain)
    foreach(source ${TEST_MAIN_SOURCES} ${TESTMAIN_SHARED_SOURCES})
        file(RELATIVE_PATH rel_source "${CMAKE_SOURCE_DIR}/src" "${source}")
        get_filename_component(source_dir "${rel_source}" DIRECTORY)
        string(REPLACE "/" "\\" source_group_name "${source_dir}")
        source_group("TestMain\\${source_group_name}" FILES "${source}")
    endforeach()
endif()

if(TARGET TestConfigManager)
    foreach(source ${TEST_CONFIG_SOURCES} ${CONFIG_TEST_SHARED_SOURCES})
        file(RELATIVE_PATH rel_source "${CMAKE_SOURCE_DIR}/src" "${source}")
        get_filename_component(source_dir "${rel_source}" DIRECTORY)
        string(REPLACE "/" "\\" source_group_name "${source_dir}")
        source_group("TestConfigManager\\${source_group_name}" FILES "${source}")
    endforeach()
endif()

# ========================================
# PLATFORM-SPECIFIC SETTINGS
# ========================================

if(WIN32)
    # Main app Windows settings
    if(TARGET Project4)
        if(SDL2_DLLS AND EXISTS ${SDL2_DLLS})
            add_custom_command(TARGET Project4 POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${SDL2_DLLS}
                $<TARGET_FILE_DIR:Project4>)
        endif()
        
        target_link_libraries(Project4 winmm gdi32 opengl32)
    endif()
    
    # Compiler settings for all targets
    set(WINDOWS_COMPILE_OPTIONS /W3)
    set(WINDOWS_COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
else()
    set(UNIX_COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
endif()

# Apply compiler settings to all targets
foreach(target Project4 TestMain TestConfigManager)
    if(TARGET ${target})
        if(MSVC)
            target_compile_options(${target} PRIVATE ${WINDOWS_COMPILE_OPTIONS})
            target_compile_definitions(${target} PRIVATE ${WINDOWS_COMPILE_DEFINITIONS})
        else()
            target_compile_options(${target} PRIVATE ${UNIX_COMPILE_OPTIONS})
        endif()
        
        if(CMAKE_VERSION VERSION_GREATER 3.12)
            set_property(TARGET ${target} PROPERTY CXX_STANDARD 20)
        endif()
    endif()
endforeach()

# ========================================
# DEBUG OUTPUT
# ========================================

message(STATUS "=== Build Configuration Summary ===")
if(TARGET Project4)
    message(STATUS "Main Application: YES")
else()
    message(STATUS "Main Application: NO")
endif()
if(TARGET TestMain)
    message(STATUS "Original Test Application: YES")
else()
    message(STATUS "Original Test Application: NO")
endif()
if(TARGET TestConfigManager)
    message(STATUS "ConfigManager Test Application: YES")
else()
    message(STATUS "ConfigManager Test Application: NO")
endif()
message(STATUS "SDL2 Libraries: ${SDL2_LIBRARIES}")
message(STATUS "PI GCS2 Libraries: ${PI_GCS2_LIBRARIES}")  # ADD THIS LINE
message(STATUS "PI GCS2 DLLs: ${PI_GCS2_DLLS}")          # ADD THIS LINE
message(STATUS "FreeType Enabled: ${FREETYPE_ENABLED}")
list(LENGTH IMGUI_SOURCES imgui_count)
message(STATUS "ImGui Sources: ${imgui_count} files")
message(STATUS "C++ Standard: 20")
message(STATUS "===============================")