# =====================================================================
#  CMakeLists.txt — WARDOGS Artillery Calculator Overlay (SDL3 + ImGui)
# =====================================================================

cmake_minimum_required(VERSION 3.20)
project(wardogs-calculator-overlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Fetch ImGui ─────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.93.0
    GIT_SHALLOW    TRUE
)

# ── Find SDL3 ───────────────────────────────────────────────────────
#  Option 1: vcpkg  →  cmake -DCMAKE_TOOLCHAIN_FILE=...
#  Option 2: manual →  cmake -DSDL3_DIR=C:/SDL3/cmake
#  Option 3: system →  apt install libsdl3-dev

find_package(SDL3 REQUIRED CONFIG)

# ── Fetch ImGui after SDL3 is found ─────────────────────────────────
FetchContent_MakeAvailable(imgui)

# ── ImGui sources ───────────────────────────────────────────────────
set(IMGUI_DIR ${imgui_SOURCE_DIR})

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# ── Executable ──────────────────────────────────────────────────────
add_executable(calculator
    src/main.cpp
    src/calculator.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(calculator PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    src
)

target_link_libraries(calculator PRIVATE SDL3::SDL3)

# Windows: link shell32 for tray icon, gdi32 for icon handling
if(WIN32)
    target_link_libraries(calculator PRIVATE shell32 gdi32)
endif()

# Windows subsystem (hides console window in Release)
if(WIN32)
    set_target_properties(calculator PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()

# ── Install ─────────────────────────────────────────────────────────
install(TARGETS calculator RUNTIME DESTINATION bin)