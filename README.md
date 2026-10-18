# WARDOGS Artillery Calculator — Overlay

Lightweight C++ desktop overlay for WARDOGS artillery calculations.
Displays MIL, azimuth, and distance with full terrain height correction.

## Features

- **Ballistic tables** — L81 Mortar and SPH-2 (low + high arc)
- **Hull heading correction** — SPH-2 platform model (pitch 5°)
- **Field calibration** — Bakurani high-arc profile
- **Terrain height correction** — loads 16-bit PNG heightmaps from wardogs-data
- **Always-on-top overlay** — F10 hide, F11 click-through, F12 reset
- **System tray icon** — left-click toggles overlay, right-click for menu

## Build

### Prerequisites

- CMake 3.20+
- C++17 compiler (MSVC 2019+, GCC 9+, Clang 10+)
- SDL2

#### Windows (MSYS2 / MinGW)

```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-sdl2
```

#### Windows (vcpkg)

```bash
vcpkg install sdl2
```

#### Linux

```bash
sudo apt install cmake g++ libsdl2-dev   # Debian/Ubuntu
```

### Build

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"    # or "Visual Studio 17 2022", "Unix Makefiles"
cmake --build . --config Release
```

Output: `build/calculator.exe` (or `build/calculator` on Linux)

### Loading in the app

1. Enter the path to your `wardogs-data` directory in the **wardogs-data directory** field
2. Click **Load** (or press Enter)
3. Status shows "OK" when terrain is loaded

## Hotkeys

| Key | Action |
|-----|--------|
| F10 | Show / hide overlay |
| F11 | Toggle click-through |
| F12 | Reset (show + focus) |
| Tray icon left-click | Toggle show / hide |
| Tray icon right-click | Context menu (Show/Hide, Exit) |