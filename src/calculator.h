/* =====================================================================
   calculator.h  —  WARDOGS ballistic calculation engine
   ===================================================================== */
#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>
#include <optional>

namespace wd {

// ── Constants ──────────────────────────────────────────────────────────
constexpr double PI = 3.14159265358979323846;
constexpr double DEG = PI / 180.0;

// ── Map coordinate → metres ────────────────────────────────────────────
//  Game uses real-meter coordinates (e.g. x ∈ [-16320, 0]).
constexpr double METRES_PER_UNIT = 1.0;

// ── 2-D point (game coordinates) ──────────────────────────────────────
struct Vec2 {
    double x = 0, y = 0;
};

// ── Ballistic table entry:  distance_m  →  MIL ────────────────────────
struct BallisticPoint { double dist; double mil; };

using BallisticTable = std::vector<BallisticPoint>;

// ── Weapon definition ─────────────────────────────────────────────────
struct Weapon {
    const char* id;
    const char* name;
    double minRangeM;      // metres
    double maxRangeM;      // metres
    double minMil;         // elevation limits
    double maxMil;
    BallisticTable single; // Mortar (one trajectory)
    BallisticTable low;    // SPH-2 low arc
    BallisticTable high;   // SPH-2 high arc
};

// ── Terrain heightmap (packed binary) ──────────────────────────────────
//
//  File format:
//    uint32 rows,  uint32 cols   (little-endian)
//    rows * cols * float32       (little-endian, metres)
//
//  Coordinate mapping:  col = gameX index, row = gameY index (bottom-up)
//
struct TerrainGrid {
    int rows = 0, cols = 0;
    double xMin = 0, xMax = 163.2;
    double yMin = 0, yMax = 163.2;
    std::vector<float> heights;

    bool loaded() const { return rows > 0 && cols > 0
                                  && heights.size() == (size_t)rows * cols; }

    double heightAt(double gameX, double gameY) const {
        if (!loaded()) return 0.0;

        // Map game coords → grid indices (fractional)
        double gx = (gameX - xMin) / (xMax - xMin) * (cols - 1);
        double gy = (gameY - yMin) / (yMax - yMin) * (rows - 1);

        int x0 = std::clamp((int)gx, 0, cols - 2);
        int y0 = std::clamp((int)gy, 0, rows - 2);
        double fx = gx - x0;
        double fy = gy - y0;

        int r0 = rows - 1 - y0;           // flip Y: row 0 = top
        int r1 = std::max(0, r0 - 1);

        double h00 = heights[r0 * cols + x0];
        double h10 = heights[r0 * cols + x0 + 1];
        double h01 = heights[r1 * cols + x0];
        double h11 = heights[r1 * cols + x0 + 1];

        return h00 * (1 - fx) * (1 - fy)
             + h10 * fx       * (1 - fy)
             + h01 * (1 - fx) * fy
             + h11 * fx       * fy;
    }
};

// ── Map metadata ──────────────────────────────────────────────────────
struct MapDef {
    const char* id;          // e.g. "bakurani"
    const char* name;        // e.g. "Bakurani"
    const char* dataWorld;   // wardogs-data folder name (e.g. "kavkazi"), nullptr if absent
    double xMin, xMax;      // map bounds in metres
    double yMin, yMax;
};

// ── SPH-2 hull correction model ────────────────────────────────────────
constexpr double SPH_BASELINE_PITCH_DEG = 5.0;
constexpr double SPH_BASELINE_ROLL_DEG  = 0.0;

// ── SPH-2 field calibration profile ────────────────────────────────────
//    Row: { distance_m, mil_delta, reference_deltaZ, reference_relativeYaw }
struct CalibRow { double dist; double milDelta; double refDZ; double refYaw; };

// ── Firing solution result ─────────────────────────────────────────────
struct Solution {
    double distanceM = 0;
    double azimuthDeg = 0;
    double originZ = 0;
    double targetZ = 0;
    double deltaZ = 0;

    bool   inRange = false;

    // Flat-table values (before hull correction)
    double flatLowMil  = 0;
    double flatHighMil = 0;
    bool   hasLow  = false;
    bool   hasHigh = false;

    // After hull correction
    double finalLowMil  = 0;
    double finalHighMil = 0;
    double hullCorrectionLowMil  = 0;
    double hullCorrectionHighMil = 0;

    bool   useHighArc = true; // user-selected arc for SPH-2
};

// ── Built-in weapons ──────────────────────────────────────────────────
inline double lerp(double a, double b, double t) { return a + (b - a) * t; }
const Weapon& mortar();
const Weapon& sph2();
const Weapon* findWeapon(const char* id);

// ── Built-in maps ─────────────────────────────────────────────────────
const MapDef& mapBakurani();
const MapDef& mapOzeti();
const MapDef& mapZestafona();
const MapDef* findMap(const char* id);

// ── Calculator ─────────────────────────────────────────────────────────
class Calculator {
public:
    // Inputs
    void setWeapon(const char* id);
    void setMap(const char* id);
    void setOrigin(Vec2 p);
    void setTarget(Vec2 p);
    void setHullHeading(double deg);      // NaN = disabled
    void setUseHighArc(bool high);
    void setManualMilAdj(double mil);     // extra manual ± MIL

    // Load terrain from packed binary file (legacy)
    bool loadTerrain(const std::string& path);

    // Load terrain from wardogs-data directory
    //  dataDir: path to wardogs-data root (contains data/ and assets/ subdirs)
    bool loadFromWardogsData(const std::string& dataDir);

    // Check if terrain is loaded
    bool terrainLoaded() const { return m_terrain.loaded(); }

    // Compute everything
    Solution solve() const;

    // Getters
    const Weapon* weapon() const { return m_weapon; }
    const MapDef* currentMap() const { return m_map; }
    Vec2 origin() const { return m_origin; }
    Vec2 target() const { return m_target; }
    double hullHeading() const { return m_hullDeg; }
    bool   useHighArc() const { return m_highArc; }

private:
    const Weapon* m_weapon = nullptr;
    const MapDef* m_map    = nullptr;
    Vec2 m_origin{5, 5};
    Vec2 m_target{5.5, 5.5};
    double m_hullDeg = NAN;
    bool   m_highArc = true;
    double m_manualAdj = 0;
    TerrainGrid m_terrain;

    // Internal helpers
    double terrainHeightAt(Vec2 p) const;

    // SPH-2 platform correction
    struct Vec3 { double x, y, z; };

    Vec3 sphLocalDir(double milMrad, double relYawDeg) const;
    Vec3 sphBasisForward() const;
    Vec3 sphBasisRight()   const;
    Vec3 sphBasisUp()      const;
    Vec3 sphLocalToWorld(Vec3 d) const;
    Vec3 sphWorldToLocal(Vec3 d) const;
    double sphElevRad(Vec3 d) const;
    double sphCorrectMil(double flatMil, double azDeg) const;

    // Field calibration
    double fieldCalibrationMil(double dist, double deltaZ,
                               double relYaw) const;
};

} // namespace wd