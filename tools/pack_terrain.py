#!/usr/bin/env python3
"""
Download WARDOGS terrain heightmap chunks and pack them into a compact .dat
file for the overlay calculator.

Usage:
    python pack_terrain.py --map bakurani --output ../data/terrain/bakurani.dat

Supported maps: bakurani, ozeti, zestafona
"""

import argparse
import json
import struct
import sys
import os

try:
    import requests
except ImportError:
    print("ERROR: 'requests' not installed. Run:  pip install requests")
    sys.exit(1)

MANIFESTS = {
    "bakurani":  "https://assets.wardogs-artillery.com/releases/assets-v1/data/terrain/bakurani/manifest.json",
    "ozeti":     "https://assets.wardogs-artillery.com/releases/assets-v1/data/terrain/ozeti/manifest.json",
    "zestafona": "https://assets.wardogs-artillery.com/releases/assets-v1/data/terrain/zestafona/manifest.json",
}

def main():
    parser = argparse.ArgumentParser(description="Pack WARDOGS terrain into .dat")
    parser.add_argument("--map", required=True, choices=MANIFESTS.keys())
    parser.add_argument("--output", required=True, help="Output .dat file path")
    parser.add_argument("--resolution", type=float, default=2.0,
                        help="Grid resolution in metres per vertex (default: 2)")
    args = parser.parse_args()

    manifest_url = MANIFESTS[args.map]
    base_url = manifest_url.rsplit("/", 1)[0] + "/"

    print(f"Downloading manifest for {args.map}...")
    resp = requests.get(manifest_url, timeout=30)
    resp.raise_for_status()
    manifest = resp.json()

    chunks = manifest["chunks"]
    vps = manifest["verticesPerSide"]        # 511
    chunk_quads = manifest["chunkQuads"]      # 510
    world_z_off = manifest["worldZOffsetMeters"]
    world_z_scale = manifest["worldZScaleMetersPerLocalUnit"]
    offset_x = manifest["globalQuadOffsetX"]
    offset_y = manifest["globalQuadOffsetY"]
    qpgu_x = manifest.get("gameUnitsToLandscapeQuadsX",
                          manifest.get("gameUnitsToLandscapeQuads", 50))
    qpgu_y = manifest.get("gameUnitsToLandscapeQuadsY",
                          manifest.get("gameUnitsToLandscapeQuads", 50))

    coverage = manifest.get("coverage", {})
    game_x_min = coverage.get("gameXMin", 0)
    game_x_max = coverage.get("gameXMax", 163.2)
    game_y_min = coverage.get("gameYMin", 0)
    game_y_max = coverage.get("gameYMax", 163.2)

    # Output grid dimensions
    cols = int((game_x_max - game_x_min) / args.resolution) + 1
    rows = int((game_y_max - game_y_min) / args.resolution) + 1
    print(f"Output grid: {cols} x {rows}  ({cols*rows*4/1024/1024:.1f} MB)")

    # Download all chunks
    chunk_data = {}
    total = len(chunks)
    for i, (key, entry) in enumerate(chunks.items(), 1):
        url = base_url + entry["file"]
        min_z = entry["minLocalZ"]
        max_z = entry["maxLocalZ"]
        print(f"  [{i}/{total}] {key} ({entry['bytes']//1024} KB)...", end=" ", flush=True)

        r = requests.get(url, timeout=60)
        r.raise_for_status()
        raw = r.content
        if len(raw) != entry["bytes"]:
            print(f"SIZE MISMATCH {len(raw)} != {entry['bytes']}")
            continue

        # Decode U16 → float heights
        num_vals = len(raw) // 2
        u16_vals = struct.unpack(f"<{num_vals}H", raw)
        heights = []
        for v in u16_vals:
            local_z = min_z + (v / 65535.0) * (max_z - min_z)
            world_z = world_z_off + local_z * world_z_scale
            heights.append(world_z)

        chunk_data[key] = {
            "cx": int(key.split(",")[0]),
            "cy": int(key.split(",")[1]),
            "heights": heights
        }
        print("OK")

    # Sample the output grid
    print(f"Building {cols}x{rows} height grid...")
    grid = [0.0] * (rows * cols)

    for row in range(rows):
        gy = game_y_min + row * args.resolution
        for col in range(cols):
            gx = game_x_min + col * args.resolution

            # Game coords → quad coords
            qx = offset_x + gx * qpgu_x
            qy = offset_y + gy * qpgu_y

            # Quad → chunk + local
            cx = int(qx // chunk_quads)
            cy = int(qy // chunk_quads)
            lx = qx - cx * chunk_quads
            ly = qy - cy * chunk_quads

            key = f"{cx},{cy}"
            chunk = chunk_data.get(key)
            if not chunk:
                continue

            # Bilinear sample within chunk
            x0 = max(0, min(int(lx), vps - 2))
            y0 = max(0, min(int(ly), vps - 2))
            fx = lx - x0
            fy = ly - y0

            h = chunk["heights"]
            h00 = h[y0 * vps + x0]
            h10 = h[y0 * vps + x0 + 1]
            h01 = h[(y0 + 1) * vps + x0]
            h11 = h[(y0 + 1) * vps + x0 + 1]

            grid[row * cols + col] = (
                h00 * (1-fx) * (1-fy) +
                h10 * fx * (1-fy) +
                h01 * (1-fx) * fy +
                h11 * fx * fy
            )

    # Write .dat
    os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
    with open(args.output, "wb") as f:
        f.write(struct.pack("<II", rows, cols))
        for v in grid:
            f.write(struct.pack("<f", v))

    size_mb = os.path.getsize(args.output) / (1024 * 1024)
    print(f"Written: {args.output} ({size_mb:.2f} MB)")

if __name__ == "__main__":
    main()