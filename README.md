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

Run `build.bat` and select your compiler from the menu. CMake, SDL3 and ImGui are downloaded and built automatically — no manual dependencies.

### Terrain data

For terrain height correction, download the heightmap PNGs and place them into the corresponding `wardogs-data` folder:

```
wardogs-data/assets/maps/europe/heightmap.png
wardogs-data/assets/maps/kavkazi/heightmap.png
wardogs-data/assets/maps/north-america/heightmap.png
```

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