# =====================================================================
#  CMakeLists.txt — WARDOGS Artillery Calculator Overlay (SDL3 + ImGui)
# =====================================================================

cmake_minimum_required(VERSION 3.20)
project(wardogs-calculator-overlay LANGUAGES CXX C)

# ── Version (passed via -DVERSION=X.Y.Z or defaults) ──────────────
if(NOT DEFINED VERSION OR VERSION STREQUAL "")
    set(VERSION "1.0.0")
endif()
# Strip leading/trailing dots and whitespace
string(REGEX REPLACE "^[. ]+" "" VERSION "${VERSION}")
string(REGEX REPLACE "[. ]+$" "" VERSION "${VERSION}")
string(REPLACE "." ";" _ver_list "${VERSION}")
list(LENGTH _ver_list _ver_len)
if(_ver_len LESS 1)
    set(VERSION "1.0.0")
    string(REPLACE "." ";" _ver_list "${VERSION}")
    set(_ver_len 3)
endif()
list(GET _ver_list 0 VERSION_MAJOR)
if(_ver_len GREATER 1)
    list(GET _ver_list 1 VERSION_MINOR)
else()
    set(VERSION_MINOR 0)
endif()
if(_ver_len GREATER 2)
    list(GET _ver_list 2 VERSION_PATCH)
else()
    set(VERSION_PATCH 0)
endif()
set(VERSION_STR "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
message(STATUS "Building version: ${VERSION_STR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Static runtime on MSVC ────────────────────────────────────────
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ── Fetch SDL3 (built from source, static) ────────────────────────
include(FetchContent)

set(SDL_SHARED OFF  CACHE BOOL "" FORCE)
set(SDL_STATIC ON   CACHE BOOL "" FORCE)
set(SDL_TEST   OFF  CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.18
    GIT_SHALLOW    TRUE
)

# ── Fetch ImGui ────────────────────────────────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.93.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(SDL3 imgui)

# ── ImGui sources ──────────────────────────────────────────────────
set(IMGUI_DIR ${imgui_SOURCE_DIR})

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# ── Executable ─────────────────────────────────────────────────────
set(APP_SOURCES
    src/main.cpp
    src/calculator.cpp
    ${IMGUI_SOURCES}
)

# Windows: embed icon + version resource
if(WIN32)
    configure_file(assets/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
    list(APPEND APP_SOURCES assets/icon.rc ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

add_executable(calculator ${APP_SOURCES})

target_include_directories(calculator PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    src
)

# Static SDL3 (no DLL needed)
target_link_libraries(calculator PRIVATE SDL3::SDL3-static)

# Windows: tray icon, icon loading
if(WIN32)
    target_link_libraries(calculator PRIVATE shell32 gdi32 ole32 imm32 version winmm setupapi)
endif()

# Hide console in Release
if(WIN32)
    set_target_properties(calculator PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()
# MSVC needs /ENTRY:mainCRTStartup for int main() with /subsystem:windows
if(MSVC)
    target_link_options(calculator PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
endif()

# ── Install ────────────────────────────────────────────────────────
install(TARGETS calculator RUNTIME DESTINATION bin)