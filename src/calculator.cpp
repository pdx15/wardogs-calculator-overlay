/* =====================================================================
   calculator.cpp  —  WARDOGS ballistic calculation engine
   ===================================================================== */

#include "calculator.h"
#include <cstring>
#include <cmath>
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>

// ── stb_image (16-bit PNG heightmap loader) ───────────────────────────
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace wd {

// =====================================================================
//  Built-in weapons
// =====================================================================

static const Weapon g_mortar = {
    "mortar", "L81 Mortar",
    132.0, 684.0,    // min/max range metres
    150.0, 850.0,    // min/max MIL
    // single table
    {
        {80,950},{87,940},{93,930},{99,920},{105,910},{110,900},
        {115,890},{118,880},{122,870},{127,860},{132,850},{140,840},
        {151,830},{163,820},{175,810},{187,800},{198,790},{208,780},
        {219,770},{229,760},{239,750},{250,740},{260,730},{270,720},
        {280,710},{290,700},{300,690},{310,680},{319,670},{329,660},
        {339,650},{348,640},{358,630},{367,620},{376,610},{385,600},
        {394,590},{403,580},{412,570},{420,560},{429,550},{437,540},
        {446,530},{454,520},{462,510},{470,500},{478,490},{486,480},
        {494,470},{501,460},{509,450},{516,440},{524,430},{531,420},
        {538,410},{545,400},{552,390},{559,380},{565,370},{572,360},
        {578,350},{585,340},{591,330},{597,320},{603,310},{609,300},
        {615,290},{620,280},{626,270},{631,260},{636,250},{641,240},
        {646,230},{651,220},{656,210},{661,200},{666,190},{670,180},
        {675,170},{680,160},{684,150},{688,140},{693,130},{697,120}
    },
    {}, {} // no low/high for mortar
};

static const Weapon g_sph2 = {
    "spg", "SPH-2",
    780.0, 2629.0,
    20.0, 1390.0,
    {}, // no single
    // low arc
    {
        {1181,20},{1232,30},{1283,40},{1334,50},{1384,60},{1433,70},
        {1482,80},{1529,90},{1576,100},{1622,110},{1666,120},{1709,130},
        {1751,140},{1792,150},{1832,160},{1870,170},{1907,180},{1944,190},
        {1979,200},{2014,210},{2046,220},{2079,230},{2110,240},{2139,250},
        {2168,260},{2196,270},{2223,280},{2249,290},{2273,300},{2296,310},
        {2319,320},{2341,330},{2362,340},{2383,350},{2403,360},{2422,370},
        {2439,380},{2456,390},{2471,400},{2485,410},{2499,420},{2513,430},
        {2526,440},{2538,450},{2550,460},{2561,470},{2570,480},{2579,490},
        {2586,500},{2593,510},{2599,520},{2605,530},{2610,540},{2615,550},
        {2620,560},{2623,570},{2626,580},{2628,590},{2629,600}
    },
    // high arc (ascending by distance)
    {
        {735,1400},{780,1390},{825,1380},{869,1370},{913,1360},{956,1350},
        {999,1340},{1041,1330},{1083,1320},{1124,1310},{1165,1300},{1205,1290},
        {1245,1280},{1285,1270},{1324,1260},{1363,1250},{1401,1240},{1438,1230},
        {1475,1220},{1512,1210},{1547,1200},{1582,1190},{1616,1180},{1650,1170},
        {1684,1160},{1717,1150},{1750,1140},{1782,1130},{1813,1120},{1844,1110},
        {1875,1100},{1905,1090},{1934,1080},{1963,1070},{1991,1060},{2019,1050},
        {2046,1040},{2072,1030},{2098,1020},{2123,1010},{2147,1000},{2171,990},
        {2194,980},{2217,970},{2239,960},{2261,950},{2282,940},{2303,930},
        {2323,920},{2342,910},{2360,900},{2378,890},{2395,880},{2412,870},
        {2429,860},{2444,850},{2460,840},{2474,830},{2488,820},{2501,810},
        {2513,800},{2524,790},{2536,780},{2546,770},{2557,760},{2567,750},
        {2576,740},{2584,730},{2592,720},{2599,710},{2604,700},{2609,690},
        {2613,680},{2617,670},{2621,660},{2624,650},{2626,640},{2628,630},
        {2629,620},{2629,610}
    }
};

const Weapon& mortar() { return g_mortar; }
const Weapon& sph2()   { return g_sph2; }

const Weapon* findWeapon(const char* id) {
    if (!id) return nullptr;
    if (std::strcmp(id, "mortar") == 0) return &g_mortar;
    if (std::strcmp(id, "spg")    == 0) return &g_sph2;
    return nullptr;
}

// =====================================================================
//  Built-in maps
// =====================================================================

static const MapDef g_maps[] = {
    // id           name         dataWorld
    { "bakurani",  "Bakurani",  "kavkazi"      },
    { "ozeti",     "Ozeti",     "europe"       },
    { "zestafona", "Zestafona", "north-america"}
};

const MapDef& mapBakurani()  { return g_maps[0]; }
const MapDef& mapOzeti()     { return g_maps[1]; }
const MapDef& mapZestafona() { return g_maps[2]; }

const MapDef* findMap(const char* id) {
    if (!id) return nullptr;
    for (auto& m : g_maps)
        if (std::strcmp(id, m.id) == 0) return &m;
    return nullptr;
}

// =====================================================================
//  Interpolation  (fixed: was comparing dist vs mil)
// =====================================================================

double interpolateTable(const BallisticTable& tbl, double dist) {
    if (tbl.empty()) return -1;
    if (dist <= tbl.front().dist) return tbl.front().mil;
    if (dist >= tbl.back().dist)  return tbl.back().mil;
    for (size_t i = 0; i + 1 < tbl.size(); ++i) {
        if (dist >= tbl[i].dist && dist <= tbl[i + 1].dist) {
            double t = (dist - tbl[i].dist) / (tbl[i + 1].dist - tbl[i].dist);
            return lerp(tbl[i].mil, tbl[i + 1].mil, t);
        }
    }
    return -1;
}

// =====================================================================
//  SPH-2 Platform correction
// =====================================================================

Calculator::Vec3 Calculator::sphBasisForward() const {
    double p = SPH_BASELINE_PITCH_DEG * DEG;
    // forward: { cos(p), 0, sin(p) }
    return { std::cos(p), 0.0, std::sin(p) };
}

Calculator::Vec3 Calculator::sphBasisRight() const {
    double p = SPH_BASELINE_PITCH_DEG * DEG;
    double r = SPH_BASELINE_ROLL_DEG  * DEG;
    // right: { sin(r)*sin(p), cos(r), -sin(r)*cos(p) }
    return { std::sin(r) * std::sin(p),
             std::cos(r),
            -std::sin(r) * std::cos(p) };
}

Calculator::Vec3 Calculator::sphBasisUp() const {
    double p = SPH_BASELINE_PITCH_DEG * DEG;
    double r = SPH_BASELINE_ROLL_DEG  * DEG;
    // up: { -cos(r)*sin(p), sin(r), cos(r)*cos(p) }
    return { -std::cos(r) * std::sin(p),
              std::sin(r),
              std::cos(r) * std::cos(p) };
}

Calculator::Vec3 Calculator::sphLocalToWorld(Vec3 d) const {
    Vec3 f = sphBasisForward();
    Vec3 r = sphBasisRight();
    Vec3 u = sphBasisUp();
    return {
        d.x*f.x + d.y*r.x + d.z*u.x,
        d.x*f.y + d.y*r.y + d.z*u.y,
        d.x*f.z + d.y*r.z + d.z*u.z
    };
}

Calculator::Vec3 Calculator::sphWorldToLocal(Vec3 d) const {
    Vec3 f = sphBasisForward();
    Vec3 r = sphBasisRight();
    Vec3 u = sphBasisUp();
    return {
        d.x*f.x + d.y*f.y + d.z*f.z,
        d.x*r.x + d.y*r.y + d.z*r.z,
        d.x*u.x + d.y*u.y + d.z*u.z
    };
}

double Calculator::sphElevRad(Vec3 d) const {
    return std::atan2(d.z, std::hypot(d.x, d.y));
}

Calculator::Vec3 Calculator::sphLocalDir(double milMrad, double relYawDeg) const {
    double elev = milMrad / 1000.0;
    double yaw  = relYawDeg * DEG;
    double ce = std::cos(elev);
    return { ce * std::cos(yaw), ce * std::sin(yaw), std::sin(elev) };
}

double Calculator::sphCorrectMil(double flatMil, double azDeg) const {
    if (std::isnan(m_hullDeg)) return flatMil;

    // Reference local bore (relative yaw = 0)
    Vec3 refLocal = sphLocalDir(flatMil, 0.0);
    Vec3 refWorld = sphLocalToWorld(refLocal);
    double desiredElev = sphElevRad(refWorld);

    // Relative yaw = azimuth - hull heading
    double relYaw = azDeg - m_hullDeg;
    // Normalize to [-180, 180]
    relYaw = std::fmod(relYaw + 540.0, 360.0) - 180.0;

    double ce = std::cos(desiredElev);
    Vec3 desiredWorld = {
        ce * std::cos(relYaw * DEG),
        ce * std::sin(relYaw * DEG),
        std::sin(desiredElev)
    };
    Vec3 correctedLocal = sphWorldToLocal(desiredWorld);
    return sphElevRad(correctedLocal) * 1000.0;
}

// =====================================================================
//  Field calibration  (Bakurani high-arc profile)
// =====================================================================

static const CalibRow g_calibProfile[] = {
    {780,    0.0, -15.4, -26.2},
    {815,   16.0, -15.4, -26.2},
    {900,   13.0,  -7.5, -30.4},
    {1057,   8.0, -39.0, -29.5},
    {1266,   2.0, -40.2, -33.8},
    {1461.1,13.0, -46.0, -26.0},
    {1650,   0.0, -46.0, -26.0}
};

static constexpr double FULL_DZ_WINDOW   = 5.0;
static constexpr double FADE_DZ_WINDOW   = 20.0;
static constexpr double FULL_YAW_WINDOW  = 5.0;
static constexpr double FADE_YAW_WINDOW  = 20.0;

static double windowWeight(double offset, double full, double fade) {
    if (full < 0 || fade <= full) return 0;
    return offset <= full ? 1.0 : std::max(0.0, (fade - offset) / (fade - full));
}

double Calculator::fieldCalibrationMil(
    double dist, double deltaZ, double relYaw) const
{
    if (!m_weapon || std::strcmp(m_weapon->id, "spg") != 0) return 0;
    if (!m_map    || std::strcmp(m_map->id, "bakurani") != 0) return 0;
    if (std::isnan(m_hullDeg)) return 0;

    // Interpolate profile
    constexpr int N = sizeof(g_calibProfile) / sizeof(g_calibProfile[0]);
    if (dist < g_calibProfile[0].dist || dist > g_calibProfile[N-1].dist)
        return 0;

    double milDelta = 0, refDZ = 0, refYaw = 0;
    for (int i = 1; i < N; ++i) {
        if (dist <= g_calibProfile[i].dist) {
            double t = (dist - g_calibProfile[i-1].dist)
                     / (g_calibProfile[i].dist - g_calibProfile[i-1].dist);
            milDelta = lerp(g_calibProfile[i-1].milDelta,
                           g_calibProfile[i].milDelta, t);
            refDZ    = lerp(g_calibProfile[i-1].refDZ,
                           g_calibProfile[i].refDZ, t);
            refYaw   = lerp(g_calibProfile[i-1].refYaw,
                           g_calibProfile[i].refYaw, t);
            break;
        }
    }

    double hw = windowWeight(std::abs(deltaZ - refDZ), FULL_DZ_WINDOW, FADE_DZ_WINDOW);
    double yw = windowWeight(std::abs(relYaw - refYaw), FULL_YAW_WINDOW, FADE_YAW_WINDOW);

    return milDelta * hw * yw;
}

// =====================================================================
//  Terrain loading
// =====================================================================

bool Calculator::loadTerrain(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;

    auto sz = f.tellg();
    if (sz < 8) return false;

    f.seekg(0);
    uint32_t rows = 0, cols = 0;
    f.read(reinterpret_cast<char*>(&rows), 4);
    f.read(reinterpret_cast<char*>(&cols), 4);

    if (rows == 0 || cols == 0 || rows > 50000 || cols > 50000) return false;

    size_t expected = 8 + (size_t)rows * cols * sizeof(float);
    if (static_cast<size_t>(sz) != expected) return false;

    m_terrain.rows = rows;
    m_terrain.cols = cols;
    m_terrain.heights.resize((size_t)rows * cols);
    f.read(reinterpret_cast<char*>(m_terrain.heights.data()),
           (size_t)rows * cols * sizeof(float));

    // Legacy: coordinate bounds must be set separately or via loadFromWardogsData
    // (terrain.json provides bounds for PNG heightmaps)

    return f.good();
}

double Calculator::terrainHeightAt(Vec2 p) const {
    if (!m_terrain.loaded()) return 0.0;
    // Convert game coordinates to metres:
    // game coord 0 = terrain metres min, 1 unit = METRES_PER_UNIT metres
    double mx = p.x * METRES_PER_UNIT + m_terrain.xMin;
    double my = p.y * METRES_PER_UNIT + m_terrain.yMin;
    return m_terrain.heightAt(mx, my);
}

// =====================================================================
//  Wardogs-data loading (terrain.json + 16-bit PNG heightmap)
// =====================================================================

// ── Minimal JSON extractors ─────────────────────────────────────────

// Read entire file into string
static std::string readFileStr(const std::string& path) {
    std::ifstream f(path);
    if (!f) return {};
    return std::string(std::istreambuf_iterator<char>(f),
                        std::istreambuf_iterator<char>());
}

// Find the content of a top-level JSON object by key (between matching braces)
static std::string jsonSection(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto p = json.find(needle);
    if (p == std::string::npos) return {};
    p = json.find('{', p);
    if (p == std::string::npos) return {};
    int depth = 0;
    size_t start = p;
    for (size_t i = p; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}') { --depth; if (depth == 0) return json.substr(start + 1, i - start - 1); }
    }
    return {};
}

// Extract a double value by key from a JSON section
static double jsonDouble(const std::string& sec, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto p = sec.find(needle);
    if (p == std::string::npos) return NAN;
    p = sec.find(':', p + needle.size());
    if (p == std::string::npos) return NAN;
    return std::strtod(sec.c_str() + p + 1, nullptr);
}

// Extract two doubles from a JSON array by key (e.g. "min": [-16320, -20400])
static bool jsonArray2(const std::string& sec, const std::string& key,
                        double& v0, double& v1) {
    std::string needle = "\"" + key + "\"";
    auto p = sec.find(needle);
    if (p == std::string::npos) return false;
    p = sec.find('[', p + needle.size());
    if (p == std::string::npos) return false;
    char* end0;
    v0 = std::strtod(sec.c_str() + p + 1, &end0);
    char* end1;
    v1 = std::strtod(end0 + 1, &end1);   // skip comma
    return (end0 != sec.c_str() + p + 1);
}

// Extract an integer by key
static long jsonInt(const std::string& sec, const std::string& key) {
    return (long)jsonDouble(sec, key);
}

// ── Load from wardogs-data directory ────────────────────────────────

bool Calculator::loadFromWardogsData(const std::string& dataDir) {
    if (!m_map || !m_map->dataWorld) return false;

    std::string world = m_map->dataWorld;

    // ── 1. Parse terrain.json ──────────────────────────────────────
    std::string terrainPath = dataDir + "/data/maps/" + world + "/terrain.json";
    std::string terrainJson = readFileStr(terrainPath);
    if (terrainJson.empty()) {
        std::fprintf(stderr, "Cannot read %s\n", terrainPath.c_str());
        return false;
    }

    std::string boundsSec  = jsonSection(terrainJson, "bounds");
    std::string zRangeSec  = jsonSection(terrainJson, "zRange");
    std::string heightmapSec = jsonSection(terrainJson, "heightmap");

    double xMin = 0, xMax = 0, yMin = 0, yMax = 0;
    double zMin = 0, zMax = 0;
    long   hmW = 0, hmH = 0;

    if (!boundsSec.empty()) {
        // "min": [xMin, yMin],  "max": [xMax, yMax]
        // Note: JSON stores as [x, y]
        if (!jsonArray2(boundsSec, "min", xMin, yMin) ||
            !jsonArray2(boundsSec, "max", xMax, yMax)) {
            std::fprintf(stderr, "Failed to parse bounds in %s\n", terrainPath.c_str());
            return false;
        }
    }

    if (!zRangeSec.empty()) {
        zMin = jsonDouble(zRangeSec, "min");
        zMax = jsonDouble(zRangeSec, "max");
    }

    if (!heightmapSec.empty()) {
        hmW = jsonInt(heightmapSec, "width");
        hmH = jsonInt(heightmapSec, "height");
    }

    if (hmW <= 0 || hmH <= 0 || std::isnan(zMin) || std::isnan(zMax)) {
        std::fprintf(stderr, "Invalid terrain metadata in %s\n", terrainPath.c_str());
        return false;
    }

    // ── 2. Load heightmap PNG (16-bit grayscale) ───────────────────
    std::string pngPath = dataDir + "/assets/maps/" + world + "/heightmap.png";
    int w = 0, h = 0, channels = 0;
    unsigned short* pixels = stbi_load_16(pngPath.c_str(), &w, &h, &channels, 1);
    if (!pixels) {
        std::fprintf(stderr, "Failed to load heightmap: %s (%s)\n",
                     pngPath.c_str(), stbi_failure_reason());
        return false;
    }

    if (w != (int)hmW || h != (int)hmH) {
        std::fprintf(stderr, "Heightmap size mismatch: expected %ldx%ld, got %dx%d\n",
                     hmW, hmH, w, h);
        stbi_image_free(pixels);
        return false;
    }

    // ── 3. Decode pixel values → heights and store ─────────────────
    //
    //  encoding:  z = zMin + pixel_value / 65535 * (zMax - zMin)
    //
    //  Terrain grid stored in METRES. Game coords are converted
    //  to metres in terrainHeightAt() before lookup.
    //
    m_terrain.rows = h;
    m_terrain.cols = w;
    m_terrain.xMin = xMin;
    m_terrain.xMax = xMax;
    m_terrain.yMin = yMin;
    m_terrain.yMax = yMax;
    m_terrain.heights.resize((size_t)w * h);

    double zRange = zMax - zMin;
    for (size_t i = 0; i < (size_t)w * h; ++i) {
        m_terrain.heights[i] = (float)(zMin + pixels[i] / 65535.0 * zRange);
    }

    stbi_image_free(pixels);

    std::fprintf(stderr, "Loaded terrain: %dx%d, bounds [%.0f,%.0f]→[%.0f,%.0f], "
                 "zRange [%.2f, %.2f]\n",
                 w, h, xMin, yMin, xMax, yMax, zMin, zMax);

    return true;
}

// =====================================================================
//  Setters
// =====================================================================

void Calculator::setWeapon(const char* id) {
    m_weapon = findWeapon(id);
    if (!m_weapon) m_weapon = &g_mortar;
}

void Calculator::setMap(const char* id) {
    m_map = findMap(id);
    if (!m_map) m_map = &g_maps[0];
}

void Calculator::setOrigin(Vec2 p) { m_origin = p; }
void Calculator::setTarget(Vec2 p) { m_target = p; }
void Calculator::setHullHeading(double deg) { m_hullDeg = deg; }
void Calculator::setUseHighArc(bool high) { m_highArc = high; }
void Calculator::setManualMilAdj(double mil) { m_manualAdj = mil; }

// =====================================================================
//  Solve
// =====================================================================

Solution Calculator::solve() const {
    Solution s;

    if (!m_weapon || !m_map) return s;

    // Distance
    double dx = m_target.x - m_origin.x;
    double dy = m_target.y - m_origin.y;
    double worldDist = std::hypot(dx, dy);
    s.distanceM = worldDist * METRES_PER_UNIT;

    // Azimuth
    s.azimuthDeg = std::atan2(dx, dy) * 180.0 / PI;
    if (s.azimuthDeg < 0) s.azimuthDeg += 360.0;

    // Terrain heights
    s.originZ = terrainHeightAt(m_origin);
    s.targetZ = terrainHeightAt(m_target);
    s.deltaZ  = s.targetZ - s.originZ;

    // Range check
    double minM = m_weapon->minRangeM;
    double maxM = m_weapon->maxRangeM;
    s.inRange = (s.distanceM >= minM - 0.5 && s.distanceM <= maxM + 0.5);

    // Flat-table MIL
    double lowMil  = interpolateTable(m_weapon->low,    s.distanceM);
    double highMil = interpolateTable(m_weapon->high,   s.distanceM);
    double singleMil = interpolateTable(m_weapon->single, s.distanceM);

    if (singleMil > 0) {
        s.flatLowMil = s.flatHighMil = singleMil;
        s.hasLow = s.hasHigh = true;
    } else {
        s.flatLowMil  = lowMil;
        s.flatHighMil = highMil;
        s.hasLow  = (lowMil  > 0);
        s.hasHigh = (highMil > 0);
    }

    // Hull correction
    double az = s.azimuthDeg;
    double relYaw = std::fmod(az - m_hullDeg + 540.0, 360.0) - 180.0;

    double corrLow  = s.hasLow  ? sphCorrectMil(s.flatLowMil,  az) : 0;
    double corrHigh = s.hasHigh ? sphCorrectMil(s.flatHighMil, az) : 0;

    s.hullCorrectionLowMil  = corrLow  - s.flatLowMil;
    s.hullCorrectionHighMil = corrHigh - s.flatHighMil;

    // Field calibration (high arc only, Bakurani only)
    double fieldCal = fieldCalibrationMil(s.distanceM, s.deltaZ, relYaw);

    // Manual adjustment
    double manual = m_manualAdj;

    // Final MIL (base: flat + platform correction)
    s.finalLowMil  = s.flatLowMil  + s.hullCorrectionLowMil  + manual;
    s.finalHighMil = s.flatHighMil + s.hullCorrectionHighMil + manual;

    // ── Numerical terrain correction (RK4 physics solver) ─────────
    // Replaces fieldCalibrationMil for all distances and deltaZ values.
    // Uses the same physics model as Apollyon's terrain-height-solver.
    if (m_terrain.loaded() && m_weapon && std::strcmp(m_weapon->id, "spg") == 0
        && std::isfinite(s.deltaZ) && std::abs(s.deltaZ) > 0.5) {

        struct TrajectoryModel { double speed, gravity, drag, offset; };
        // "full" fitted models from Apollyon solver
        static const TrajectoryModel kHigh = {
            9389.43881652221, 147.2063947858103, 0.0008516971618643472, -960.0413878819479
        };
        static const TrajectoryModel kLow = {
            300.0, 8.85397505384648, 0.0007351626163587786, 95.03019643400405
        };

        // Speed-dependent drag coefficient (Cd multiplier)
        static const double gcX[] = {0,30,61,91,122,150,183,213,244,274,
                                     300,335,366,396,427,450,488,518,549,579,600};
        static const double gcY[] = {.71,.69,.66,.64,.65,.66,.68,.72,.80,.92,
                                     1.10,1.25,1.33,1.39,1.44,1.48,1.50,1.54,1.57,1.58,1.60};
        constexpr int GC_N = 21;

        auto dragCoeff = [](double speed) -> double {
            if (speed <= gcX[0]) return gcY[0];
            if (speed >= gcX[GC_N - 1]) return gcY[GC_N - 1];
            int lo = 0, hi = GC_N - 1;
            while (lo + 1 < hi) { int mid = (lo + hi) >> 1; if (gcX[mid] <= speed) lo = mid; else hi = mid; }
            double f = (speed - gcX[lo]) / (gcX[hi] - gcX[lo]);
            return gcY[lo] + (gcY[hi] - gcY[lo]) * f;
        };

        // RK4 trajectory: returns height at given horizontal distance
        auto heightAtDist = [&](double angle, double dist,
                                const TrajectoryModel& m) -> double {
            if (angle <= 0 || angle >= PI / 2.0 || dist <= 0) return NAN;
            double x = 0, z = 0;
            double vx = m.speed * std::cos(angle);
            double vz = m.speed * std::sin(angle);
            const double g = m.gravity, dt = 0.02;
            for (int i = 0; i < 6000; i++) {
                auto acc = [&](double vx_, double vz_, double& ax_, double& az_) {
                    double q = m.drag * dragCoeff(std::hypot(vx_, vz_)) * std::hypot(vx_, vz_);
                    ax_ = -q * vx_;  az_ = -g - q * vz_;
                };
                double a1x,a1z,a2x,a2z,a3x,a3z,a4x,a4z;
                acc(vx, vz, a1x, a1z);
                double v2x=vx+a1x*dt/2, v2z=vz+a1z*dt/2; acc(v2x,v2z,a2x,a2z);
                double v3x=vx+a2x*dt/2, v3z=vz+a2z*dt/2; acc(v3x,v3z,a3x,a3z);
                double v4x=vx+a3x*dt,   v4z=vz+a3z*dt;   acc(v4x,v4z,a4x,a4z);
                double nx = x + dt*(vx + 2*v2x + 2*v3x + v4x)/6;
                double nz = z + dt*(vz + 2*v2z + 2*v3z + v4z)/6;
                if (!std::isfinite(nx) || !std::isfinite(nz)) return NAN;
                if (nx >= dist) return z + (nz-z)*(dist-x)/(nx-x) + m.offset;
                vx += dt*(a1x + 2*a2x + 2*a3x + a4x)/6;
                vz += dt*(a1z + 2*a2z + 2*a3z + a4z)/6;
                x = nx; z = nz;
                if (vx <= 1e-7) return NAN;
            }
            return NAN;
        };

        // Bisection solver
        auto bisect = [&](std::function<double(double)> fn,
                          double lo, double hi, double tol) -> double {
            double fl=fn(lo), fh=fn(hi);
            if (!std::isfinite(fl)||!std::isfinite(fh)||fl*fh>0) return NAN;
            if (std::abs(fl)<1e-8) return lo;
            if (std::abs(fh)<1e-8) return hi;
            for (int i=0; i<50 && (hi-lo)>tol; i++) {
                double mid=(lo+hi)/2, fm=fn(mid);
                if (!std::isfinite(fm)) return NAN;
                if (std::abs(fm)<1e-8) return mid;
                if (fl*fm<=0) { hi=mid; fh=fm; } else { lo=mid; fl=fm; }
            }
            return (lo+hi)/2;
        };

        auto solveArc = [&](const TrajectoryModel& model,
                            double flatMil, double distM, double dz,
                            double minMil, double maxMil) -> double {
            double flatRad = flatMil / 1000.0;
            // 1) Find reference angle (flat terrain, height=0)
            double refAngle = NAN, bestErr = 1e9;
            for (double r = std::max(minMil/1000.0, flatRad-0.3);
                 r <= std::min(maxMil/1000.0, flatRad+0.3); r += 0.0005) {
                double h = heightAtDist(r, distM, model);
                if (std::isfinite(h) && std::abs(h) < std::abs(bestErr)) {
                    bestErr = h; refAngle = r;
                    if (std::abs(h) < 0.05) break;
                }
            }
            if (!std::isfinite(refAngle)) return NAN;

            // 2) Find corrected MIL where trajectory height = dz
            auto cmdFn = [&](double mil) -> double {
                double h = heightAtDist(refAngle + (mil - flatMil)/1000.0, distM, model);
                return std::isfinite(h) ? h - dz : NAN;
            };
            // Scan for sign change
            double prevM=NAN, prevV=NAN, corrected=NAN;
            for (double m = minMil; m <= maxMil; m += 10) {
                double v = cmdFn(m);
                if (std::isfinite(v) && std::abs(v) < 0.05) { corrected = m; break; }
                if (std::isfinite(v) && std::isfinite(prevV) && prevV * v <= 0) {
                    double root = bisect(cmdFn, prevM, m, 0.025);
                    if (std::isfinite(root)) { corrected = root; break; }
                }
                prevM = m; prevV = v;
            }
            return corrected;
        };

        double distM = s.distanceM;
        double dz = s.deltaZ;

        // High arc correction
        if (s.hasHigh) {
            double corr = solveArc(kHigh, s.flatHighMil, distM, dz, 610, 1390);
            if (std::isfinite(corr))
                s.finalHighMil = corr + s.hullCorrectionHighMil + manual;
        }
        // Low arc correction
        if (s.hasLow) {
            double corr = solveArc(kLow, s.flatLowMil, distM, dz, 20, 600);
            if (std::isfinite(corr))
                s.finalLowMil = corr + s.hullCorrectionLowMil + manual;
        }
    }

    s.useHighArc = m_highArc;

    return s;
}

} // namespace wd