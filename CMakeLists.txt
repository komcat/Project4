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
set(SDL2_DIR "${EXTERNAL_DIR}/sdl2")
set(FREETYPE_DIR "${EXTERNAL_DIR}/freetype")

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

# Filter out test files from main app sources
set(MAIN_APP_SOURCES ${ALL_PROJECT_SOURCES})
filter_out(MAIN_APP_SOURCES ".*TestMain\\.cpp$")
filter_out(MAIN_APP_SOURCES ".*test.*\\.cpp$")  # Remove any other test files
filter_out(MAIN_APP_SOURCES ".*Test.*\\.cpp$")  # Remove any Test*.cpp files

# Create test app sources (only if TestMain.cpp exists)
set(TEST_APP_SOURCES)
foreach(source ${ALL_PROJECT_SOURCES})
    if(source MATCHES ".*TestMain\\.cpp$")
        list(APPEND TEST_APP_SOURCES ${source})
    endif()
    # Add utility sources that tests might need
    if(source MATCHES ".*utils/.*\\.cpp$")
        list(APPEND TEST_APP_SOURCES ${source})
    endif()
endforeach()

# Remove duplicate utils from test sources
list(REMOVE_DUPLICATES TEST_APP_SOURCES)

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

# Print discovered files for debugging
print_file_list("ALL PROJECT SOURCES" "${ALL_PROJECT_SOURCES}")
print_file_list("ALL PROJECT HEADERS" "${ALL_PROJECT_HEADERS}")
print_file_list("MAIN APP SOURCES" "${MAIN_APP_SOURCES}")
print_file_list("TEST APP SOURCES" "${TEST_APP_SOURCES}")

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
    )
    
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
    
    if(FREETYPE_ENABLED)
        target_link_libraries(Project4 ${FREETYPE_LIBRARIES})
        target_compile_definitions(Project4 PRIVATE IMGUI_ENABLE_FREETYPE)
    endif()
    
    # Create source groups for IDE organization
    foreach(source ${MAIN_APP_SOURCES})
        file(RELATIVE_PATH rel_source "${CMAKE_SOURCE_DIR}/src" "${source}")
        get_filename_component(source_dir "${rel_source}" DIRECTORY)
        string(REPLACE "/" "\\" source_group_name "${source_dir}")
        source_group("Source Files\\${source_group_name}" FILES "${source}")
    endforeach()
    
    foreach(header ${ALL_PROJECT_HEADERS})
        file(RELATIVE_PATH rel_header "${CMAKE_SOURCE_DIR}" "${header}")
        get_filename_component(header_dir "${rel_header}" DIRECTORY)
        string(REPLACE "/" "\\" header_group_name "${header_dir}")
        source_group("Header Files\\${header_group_name}" FILES "${header}")
    endforeach()
    
    source_group("External\\ImGui" FILES ${IMGUI_SOURCES})
    
else()
    message(FATAL_ERROR "No main application sources found!")
endif()

# ========================================
# BUILD TEST APPLICATION (TestMain)
# ========================================

# Check if we have TestMain.cpp and can build test app
set(HAS_TEST_MAIN FALSE)
foreach(source ${TEST_APP_SOURCES})
    if(source MATCHES ".*TestMain\\.cpp$")
        set(HAS_TEST_MAIN TRUE)
        break()
    endif()
endforeach()

if(HAS_TEST_MAIN AND TEST_APP_SOURCES)
    message(STATUS "Building test application: TestMain")
    
    add_executable(TestMain ${TEST_APP_SOURCES})
    
    # Include directories (minimal for test)
    target_include_directories(TestMain PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/include"
    )
    
    # Create source groups for test app
    foreach(source ${TEST_APP_SOURCES})
        file(RELATIVE_PATH rel_source "${CMAKE_SOURCE_DIR}/src" "${source}")
        get_filename_component(source_dir "${rel_source}" DIRECTORY)
        string(REPLACE "/" "\\" source_group_name "${source_dir}")
        source_group("Test\\${source_group_name}" FILES "${source}")
    endforeach()
    
else()
    message(STATUS "TestMain.cpp not found or insufficient sources - skipping test executable")
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
    
    # Compiler settings for both targets
    set(WINDOWS_COMPILE_OPTIONS /W3)
    set(WINDOWS_COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
else()
    set(UNIX_COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
endif()

# Apply compiler settings to all targets
foreach(target Project4 TestMain)
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
    message(STATUS "Test Application: YES")
else()
    message(STATUS "Test Application: NO")
endif()
message(STATUS "SDL2 Libraries: ${SDL2_LIBRARIES}")
message(STATUS "FreeType Enabled: ${FREETYPE_ENABLED}")
list(LENGTH IMGUI_SOURCES imgui_count)
message(STATUS "ImGui Sources: ${imgui_count} files")
message(STATUS "C++ Standard: 20")
message(STATUS "===============================")