# Terrain data extraction tool

## Prerequisites

Python 3.8+

```bash
pip install requests
```

## Usage

```bash
python pack_terrain.py --map bakurani --output ../data/terrain/bakurani.dat
python pack_terrain.py --map ozeti    --output ../data/terrain/ozeti.dat
python pack_terrain.py --map zestafona --output ../data/terrain/zestafona.dat
```

This downloads the terrain manifest and chunk binaries from the game CDN,
extracts height data, and packs it into a compact .dat file for the overlay.

## Output format

Each .dat file:
- Header: `uint32` rows, `uint32` cols (little-endian)
- Data: `rows * cols * float32` (little-endian, metres)
- Coordinate mapping: col = gameX index, row = gameY index (bottom-up)