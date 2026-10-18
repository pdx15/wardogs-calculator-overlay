# =====================================================================
#  CMakeLists.txt  —  WARDOGS Artillery Calculator Overlay
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
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# ── Find SDL2 ───────────────────────────────────────────────────────
#  Option 1: vcpkg  →  cmake -DCMAKE_TOOLCHAIN_FILE=...
#  Option 2: system  →  apt install libsdl2-dev  /  brew install sdl2
#  Option 3: manual  →  cmake -DSDL2_DIR=C:/SDL2/cmake

find_package(SDL2 REQUIRED)

# ── ImGui sources ───────────────────────────────────────────────────
set(IMGUI_DIR ${imgui_SOURCE_DIR})

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
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

target_link_libraries(calculator PRIVATE SDL2::SDL2)

# Windows subsystem (hides console window in Release)
if(WIN32)
    set_target_properties(calculator PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()

# ── Install ─────────────────────────────────────────────────────────
install(TARGETS calculator RUNTIME DESTINATION bin)