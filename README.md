# WARDOGS Artillery Calculator — Overlay

Lightweight C++ desktop overlay for WARDOGS artillery calculations.
Displays MIL, azimuth, and distance with full terrain height correction.

## Features

- **Ballistic tables** — L81 Mortar and SPH-2 (low + high arc)
- **RK4 terrain correction** — numerical physics solver matching Apollyon
- **Hull heading correction** — SPH-2 platform model (pitch 5°)
- **Save/Load targets** — per-map target bookmarks
- **Always-on-top overlay** — F10 hide, F11 click-through, F12 reset
- **System tray icon** — left-click toggles overlay, right-click for menu

## Supported maps

| Map | World folder |
|-----|-------------|
| Bakurani | kavkazi |
| Ozeti | europe |
| Zestafona | north-america |

## Weapons

| Weapon | ID | Arc |
|--------|-----|-----|
| L81 Mortar | mortar | single |
| SPH-2 | spg | low / high |

## Build

Run `build.bat` and select your compiler from the menu. CMake, SDL3 and ImGui are downloaded and built automatically — no manual dependencies.

### Terrain data

For terrain height correction, download and place the full `wardogs-data` folder next to `calculator.exe`:

```
wardogs-data/
├── data/maps/
│   ├── bakurani/terrain.json
│   ├── ozeti/terrain.json
│   └── zestafona/terrain.json
└── assets/maps/
    ├── kavkazi/heightmap.png
    ├── europe/heightmap.png
    └── north-america/heightmap.png
```

Without `wardogs-data` the calculator still works — MIL is computed from ballistic tables, but without terrain height correction (Delta Z = 0).

### Loading terrain

If `wardogs-data` is placed next to `calculator.exe`, terrain loads automatically on startup. Otherwise, enter the path manually and click **Load**.

## Usage

1. Select weapon and map
2. Enter artillery (origin) and target coordinates (game units, decimals allowed)
3. MIL is calculated automatically when coordinates change
4. **Save** — bookmark the current target with a name (per-map)
5. **Load** — select a previously saved target
6. For SPH-2: enable **Hull heading** and set the vehicle heading for platform correction

## Hotkeys

| Key | Action |
|-----|--------|
| F10 | Show / hide overlay |
| F11 | Toggle click-through |
| F12 | Reset (show + focus) |
| Tray icon left-click | Toggle show / hide |
| Tray icon right-click | Context menu (Show/Hide, Exit) |