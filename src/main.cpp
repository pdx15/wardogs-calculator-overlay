/* =====================================================================
   main.cpp  —  WARDOGS Artillery Calculator  (C++ / ImGui / SDL3)
   ===================================================================== */

// ── ImGui + SDL3 backends ───────────────────────────────────────────
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>

#include "calculator.h"

// ── Win32 overlay helpers ──────────────────────────────────────────────
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON   (WM_USER + 1)
#define ID_TRAY_EXIT  1002

static NOTIFYICONDATAW g_nid{};
static HWND  g_hwndTray = nullptr;
static UINT  g_taskbarRestart = 0;
static bool* g_visiblePtr = nullptr;
static bool  g_titleBarHover = false;
static bool  g_exitRequested = false;
static int*  g_hotkeyVkPtr = nullptr;  // points to app.hotkeyVk

static void trayAdd();
static void trayRemove();

static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == g_taskbarRestart) {
        trayAdd();
        return 0;
    }
    if (msg == WM_TRAYICON) {
        if (lp == WM_LBUTTONUP || lp == WM_LBUTTONDBLCLK) {
            if (g_visiblePtr) *g_visiblePtr = !*g_visiblePtr;
            return 0;
        }
        if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
            PostMessage(hwnd, WM_NULL, 0, 0);
            return 0;
        }
    }
    if (msg == WM_COMMAND) {
        if (LOWORD(wp) == ID_TRAY_EXIT) {
            g_exitRequested = true;
        }
        return 0;
    }
    if (msg == WM_HOTKEY) {
        if (g_visiblePtr) *g_visiblePtr = !*g_visiblePtr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void trayCreateWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = TrayWndProc;
    wc.hInstance      = GetModuleHandleW(nullptr);
    wc.lpszClassName  = L"WarDogsTrayClass";
    RegisterClassExW(&wc);

    g_hwndTray = CreateWindowExW(0, wc.lpszClassName, L"WarDogsTray",
                                  0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                  wc.hInstance, nullptr);
    g_taskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
}

static std::string getExeDir();

static HICON loadAppIcon() {
    // Try embedded resource first (icon ID = 1, from icon.rc)
    HICON h = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1));
    if (h) return h;

    // Fallback: search for file near exe
    std::string dir = getExeDir();
    int wlen = MultiByteToWideChar(CP_UTF8, 0, dir.c_str(), -1, nullptr, 0);
    std::wstring wdir(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, dir.c_str(), -1, wdir.data(), wlen);

    std::wstring candidates[] = {
        wdir + L"\\assets\\y.ico",
        wdir + L"\\..\\assets\\y.ico",
        wdir + L"\\..\\..\\assets\\y.ico",
    };
    for (auto& path : candidates) {
        h = (HICON)LoadImageW(nullptr, path.c_str(), IMAGE_ICON, 0, 0,
                               LR_LOADFROMFILE | LR_DEFAULTSIZE);
        if (h) return h;
    }
    return nullptr;
}

static void trayAdd() {
    if (!g_hwndTray) return;
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = g_hwndTray;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    g_nid.hIcon = loadAppIcon();
    if (!g_nid.hIcon)
        g_nid.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    wcscpy(g_nid.szTip, L"WARDOGS Artillery Calculator");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void trayRemove() {
    if (g_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        if (g_nid.hIcon) DestroyIcon(g_nid.hIcon);
        g_nid = {};
    }
    if (g_hwndTray) {
        DestroyWindow(g_hwndTray);
        g_hwndTray = nullptr;
    }
}

static void registerHotkey(int vk) {
    if (!g_hwndTray) return;
    UnregisterHotKey(g_hwndTray, 1);
    if (vk > 0) RegisterHotKey(g_hwndTray, 1, 0, vk);
}

// Convert VK code to display name
static const char* vkToName(int vk) {
    if (vk == 0) return "None";
    if (vk >= VK_F1 && vk <= VK_F24) {
        static char buf[8];
        std::snprintf(buf, sizeof(buf), "F%d", vk - VK_F1 + 1);
        return buf;
    }
    if (vk >= '0' && vk <= '9') { static char c[2]; c[0]=(char)vk; c[1]=0; return c; }
    if (vk >= 'A' && vk <= 'Z') { static char c[2]; c[0]=(char)vk; c[1]=0; return c; }
    switch (vk) {
        case VK_OEM_3: return "~";
        case VK_OEM_MINUS: return "-";
        case VK_OEM_PLUS: return "=";
        case VK_OEM_4: return "[";
        case VK_OEM_6: return "]";
        case VK_OEM_1: return ";";
        case VK_OEM_7: return "'";
        case VK_OEM_5: return "\\";
        case VK_OEM_COMMA: return ",";
        case VK_OEM_PERIOD: return ".";
        case VK_OEM_2: return "/";
        case VK_SPACE: return "Space";
        case VK_TAB: return "Tab";
        case VK_ESCAPE: return "Esc";
        default: return "?";
    }
}

static HWND getHWND(SDL_Window* w) {
    SDL_PropertiesID props = SDL_GetWindowProperties(w);
    return (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
}

static void win32SetAlwaysOnTop(SDL_Window* w) {
    HWND hwnd = getHWND(w);
    if (hwnd) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE);
        SetWindowLongW(hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
    }
}

static void win32SetClickThrough(SDL_Window* w, bool enable) {
    HWND hwnd = getHWND(w);
    if (hwnd) {
        LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE);
        if (enable) ex |= WS_EX_TRANSPARENT;
        else        ex &= ~WS_EX_TRANSPARENT;
        SetWindowLongW(hwnd, GWL_EXSTYLE, ex);
    }
}

// Set window icon (embedded resource or file)
static void win32SetWindowIcon(SDL_Window* w) {
    HICON hIcon = loadAppIcon();
    if (!hIcon) return;
    HWND hwnd = getHWND(w);
    if (hwnd) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
    // Don't destroy — Windows keeps a reference
}

static std::string getExeDir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    wchar_t* slash = wcsrchr(buf, L'\\');
    if (slash) *slash = L'\0';
    int len = WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0, nullptr, nullptr);
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, buf, -1, result.data(), len, nullptr, nullptr);
    return result;
}

#else
static void trayCreateWindow() {}
static void trayAdd() {}
static void trayRemove() {}
static void win32SetAlwaysOnTop(SDL_Window*) {}
static void win32SetClickThrough(SDL_Window*, bool) {}
static void win32SetWindowIcon(SDL_Window*) {}

#include <unistd.h>
static std::string getExeDir() {
    char buf[4096]{};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        char* slash = strrchr(buf, '/');
        if (slash) *slash = '\0';
        return buf;
    }
    return ".";
}
#endif

// Auto-detect wardogs-data/ next to exe, in cwd, or one level up
static std::string findWardogsData() {
    std::string exeDir = getExeDir();

    std::string paths[] = {
        exeDir + "/wardogs-data",
        "wardogs-data",
        exeDir + "/../wardogs-data",
    };

    for (auto& c : paths) {
        std::string test = c + "/data/maps/kavkazi/terrain.json";
        std::ifstream f(test);
        if (f) return c;
    }
    return {};
}

// ── Application state ──────────────────────────────────────────────────
struct SavedTarget {
    char name[64];
    float x, y;
};

struct App {
    wd::Calculator calc;

    int   weaponIdx = 0;     // 0 = mortar, 1 = spg
    char  mapId[16]      = "bakurani";
    float originX = 0.0f, originY = 0.0f;
    float targetX = 0.0f, targetY = 0.0f;
    float hullDeg = 0.0f;
    bool  hullEnabled = false;
    bool  useHighArc = true;

    // Terrain
    bool  terrainLoaded = false;
    char  dataDir[512] = "";
    char  lastMapId[16] = "";
    char  statusMsg[256] = "";

    // Saved targets
    std::vector<SavedTarget> savedTargets;
    int   loadIdx = -1;  // -1 = nothing selected
    char  saveName[64] = "";

    // Result
    wd::Solution solution;
    bool dirty = true;  // recalculate on first frame

    // Overlay
    bool visible = true;
    bool settingsOpen = true;  // collapsible: weapon/map/terrain block

    // Global hotkey (Win32 virtual key code, default F5=0x74)
    int hotkeyVk = 0x74;  // VK_F5
    bool hotkeyRegistered = false;
    bool waitingForKey = false;
};

static const char* weaponIds[] = { "mortar", "spg" };
static const char* weaponNames[] = { "L81", "SPH-2" };
static const char* mapIds[] = { "bakurani", "ozeti", "zestafona" };

static void tryLoadTerrain(App& app) {
    app.terrainLoaded = false;
    app.statusMsg[0] = '\0';
    if (app.dataDir[0] == '\0') {
        std::snprintf(app.statusMsg, sizeof(app.statusMsg), "No data directory set");
        return;
    }
    app.calc.setMap(app.mapId);
    if (app.calc.currentMap() && app.calc.currentMap()->dataWorld) {
        if (app.calc.loadFromWardogsData(app.dataDir)) {
            app.terrainLoaded = true;
            std::snprintf(app.statusMsg, sizeof(app.statusMsg), "Terrain loaded OK");
        } else {
            const char* world = app.calc.currentMap()->dataWorld;
            std::snprintf(app.statusMsg, sizeof(app.statusMsg),
                "Failed: %s/data/maps/%s/terrain.json", app.dataDir, world);
        }
    } else {
        std::snprintf(app.statusMsg, sizeof(app.statusMsg), "No terrain data for this map");
    }
    std::strcpy(app.lastMapId, app.mapId);
}

// ── Saved targets file I/O ──────────────────────────────────────────
static std::string getSavePath() {
    return getExeDir() + "/saved_targets.txt";
}

static void loadSavedTargets(App& app) {
    app.savedTargets.clear();
    std::ifstream f(getSavePath());
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        // Format: mapId:name:x:y
        auto p1 = line.find(':');
        if (p1 == std::string::npos) continue;
        auto p2 = line.find(':', p1 + 1);
        if (p2 == std::string::npos) continue;
        auto p3 = line.find(':', p2 + 1);
        if (p3 == std::string::npos) continue;
        SavedTarget t{};
        std::string map = line.substr(0, p1);
        std::string name = line.substr(p1 + 1, p2 - p1 - 1);
        t.x = std::strtof(line.substr(p2 + 1, p3 - p2 - 1).c_str(), nullptr);
        t.y = std::strtof(line.substr(p3 + 1).c_str(), nullptr);
        if (map != app.mapId) continue;
        std::strncpy(t.name, name.c_str(), sizeof(t.name) - 1);
        app.savedTargets.push_back(t);
    }
}

static void saveTarget(App& app, const char* name) {
    // Ensure file ends with newline before appending
    {
        std::ifstream in(getSavePath(), std::ios::ate | std::ios::binary);
        if (in.tellg() > 0) {
            in.seekg(-1, std::ios::end);
            char last = 0;
            in.get(last);
            if (last != '\n') {
                in.close();
                std::ofstream out(getSavePath(), std::ios::app | std::ios::binary);
                out << "\n";
            }
        }
    }
    std::ofstream f(getSavePath(), std::ios::app);
    if (!f) return;
    f << app.mapId << ":" << name << ":" << app.targetX << ":" << app.targetY << "\n";
    f.close();
    loadSavedTargets(app);
    // Select the newly saved target
    for (int i = 0; i < (int)app.savedTargets.size(); i++) {
        if (std::strcmp(app.savedTargets[i].name, name) == 0) {
            app.loadIdx = i;
            break;
        }
    }
}

static void deleteSavedTarget(App& app, int idx) {
    if (idx < 0 || idx >= (int)app.savedTargets.size()) return;
    // Rewrite file without the deleted entry
    std::string path = getSavePath();
    // Read all entries from file (all maps)
    std::vector<std::string> lines;
    {
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            // Parse map and name to find the one to delete
            auto p1 = line.find(':');
            if (p1 == std::string::npos) { lines.push_back(line); continue; }
            auto p2 = line.find(':', p1 + 1);
            if (p2 == std::string::npos) { lines.push_back(line); continue; }
            std::string map = line.substr(0, p1);
            std::string name = line.substr(p1 + 1, p2 - p1 - 1);
            // Skip the entry that matches current map + selected name
            if (map == app.mapId && name == app.savedTargets[idx].name)
                continue;
            lines.push_back(line);
        }
    }
    {
        std::ofstream f(path);
        for (auto& l : lines) f << l << "\n";
    }
    loadSavedTargets(app);
    if (app.savedTargets.empty()) {
        app.loadIdx = -1;
    } else if (app.loadIdx >= (int)app.savedTargets.size()) {
        app.loadIdx = (int)app.savedTargets.size() - 1;
    }
}

static void recalc(App& app) {
    app.calc.setWeapon(weaponIds[app.weaponIdx]);
    app.calc.setMap(app.mapId);
    app.calc.setOrigin({(double)app.originX, (double)app.originY});
    app.calc.setTarget({(double)app.targetX, (double)app.targetY});
    app.calc.setHullHeading(app.hullEnabled ? (double)app.hullDeg : NAN);
    app.calc.setUseHighArc(app.useHighArc);
    app.calc.setManualMilAdj(0);

    if (std::strcmp(app.mapId, app.lastMapId) != 0) {
        tryLoadTerrain(app);
        loadSavedTargets(app);
        app.loadIdx = -1;
    }

    app.solution = app.calc.solve();
}

// ── GUI ────────────────────────────────────────────────────────────────

static void drawCalculator(App& app) {
    ImGui::SetNextWindowSize({380, 0}, ImGuiCond_FirstUseEver);

    ImGui::Begin("WARDOGS Artillery Calculator", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    // Title bar hover detection for window dragging
    {
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        float  th = ImGui::GetFrameHeight();
        ImVec2 mp = ImGui::GetIO().MousePos;
        g_titleBarHover = (mp.x >= wp.x && mp.x <= wp.x + ws.x
                        && mp.y >= wp.y && mp.y <= wp.y + th);
    }

    // Track if any input changed this frame
    bool changed = false;

    // ── Settings (collapsible) ──────────────────────────────────────
    if (ImGui::CollapsingHeader("Settings",
            app.settingsOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
        app.settingsOpen = true;

        // Weapon
        if (ImGui::BeginCombo("Weapon", weaponNames[app.weaponIdx])) {
            for (int i = 0; i < 2; i++) {
                if (ImGui::Selectable(weaponNames[i], app.weaponIdx == i)) {
                    app.weaponIdx = i;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }

        // Map
        const char* currentMapName = app.mapId;
        for (int i = 0; i < 3; i++) {
            if (std::strcmp(app.mapId, mapIds[i]) == 0) {
                currentMapName = mapIds[i];
                break;
            }
        }
        if (ImGui::BeginCombo("Map", currentMapName)) {
            for (int i = 0; i < 3; i++) {
                if (ImGui::Selectable(mapIds[i], std::strcmp(app.mapId, mapIds[i]) == 0)) {
                    std::strcpy(app.mapId, mapIds[i]);
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }

        // Terrain + Hotkey
        if (app.terrainLoaded) {
            ImGui::TextColored({0.3f,1,0.3f,1}, "Terrain: OK");
            ImGui::SameLine();
            if (ImGui::SmallButton("change")) {
                app.terrainLoaded = false;
            }
            ImGui::SameLine();
            // Hotkey button
            {
                const char* label = app.waitingForKey
                    ? "[Press a key...]"
                    : (app.hotkeyVk > 0
                        ? (std::string("Hotkey: ") + vkToName(app.hotkeyVk)).c_str()
                        : "Hotkey: None");
                if (ImGui::SmallButton(label)) {
                    app.waitingForKey = !app.waitingForKey;
                }
                if (app.waitingForKey) {
                    ImGui::SameLine();
                    ImGui::TextColored({1,1,0,1}, "(Esc=off)");
                }
            }
        } else {
            ImGui::Text("wardogs-data:");
            ImGui::SetNextItemWidth(260);
            if (ImGui::InputText("##ddir", app.dataDir, sizeof(app.dataDir),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                tryLoadTerrain(app);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                tryLoadTerrain(app);
            }
            if (app.statusMsg[0])
                ImGui::TextWrapped("%s", app.statusMsg);
        }
    } else {
        app.settingsOpen = false;
    }

    // ── Coordinates ─────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("Origin:");
    ImGui::SameLine(70);
    ImGui::SetNextItemWidth(90);
    ImGui::InputFloat("X##ox", &app.originX, 0, 0, "%.2f");
    changed |= ImGui::IsItemEdited();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::InputFloat("Y##oy", &app.originY, 0, 0, "%.2f");
    changed |= ImGui::IsItemEdited();

    ImGui::Text("Target:");
    ImGui::SameLine(70);
    ImGui::SetNextItemWidth(90);
    ImGui::InputFloat("X##tx", &app.targetX, 0, 0, "%.2f");
    changed |= ImGui::IsItemEdited();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::InputFloat("Y##ty", &app.targetY, 0, 0, "%.2f");
    changed |= ImGui::IsItemEdited();

    if (ImGui::Button("Swap")) {
        std::swap(app.originX, app.targetX);
        std::swap(app.originY, app.targetY);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        ImGui::OpenPopup("Save Target");
        std::memset(app.saveName, 0, sizeof(app.saveName));
    }
    ImGui::SameLine();
    // Load combo — shows saved targets for current map
    {
        const char* preview = app.savedTargets.empty()
            ? "(no saved targets)"
            : (app.loadIdx >= 0 && app.loadIdx < (int)app.savedTargets.size()
                ? app.savedTargets[app.loadIdx].name : "(select)");
        ImGui::SetNextItemWidth(160);
        if (ImGui::BeginCombo("##load", preview)) {
            for (int i = 0; i < (int)app.savedTargets.size(); i++) {
                if (ImGui::Selectable(app.savedTargets[i].name, app.loadIdx == i)) {
                    app.loadIdx = i;
                    app.targetX = app.savedTargets[i].x;
                    app.targetY = app.savedTargets[i].y;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
    }
    ImGui::SameLine();
    // Del button — red, deletes selected saved target
    {
        ImVec4 red(0.8f, 0.2f, 0.2f, 1.0f);
        ImVec4 redHover(1.0f, 0.3f, 0.3f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redHover);
        bool canDel = !app.savedTargets.empty()
                   && app.loadIdx >= 0
                   && app.loadIdx < (int)app.savedTargets.size();
        if (!canDel) ImGui::BeginDisabled();
        if (ImGui::Button("Del")) {
            deleteSavedTarget(app, app.loadIdx);
        }
        if (!canDel) ImGui::EndDisabled();
        ImGui::PopStyleColor(2);
    }

    // Save Target popup
    if (ImGui::BeginPopup("Save Target")) {
        ImGui::Text("Target name:");
        ImGui::SetNextItemWidth(200);
        bool enter = ImGui::InputText("##sname", app.saveName, sizeof(app.saveName),
                                       ImGuiInputTextFlags_EnterReturnsTrue);
        if ((ImGui::Button("Save") || enter) && app.saveName[0]) {
            saveTarget(app, app.saveName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ── SPH-2 controls ──────────────────────────────────────────────
    bool isSph = (app.weaponIdx == 1);
    if (isSph) {
        ImGui::Separator();
        ImGui::Text("Platform:");
        ImGui::SameLine(70);
        ImGui::Checkbox("Hull heading", &app.hullEnabled);
        changed |= ImGui::IsItemEdited();
        if (app.hullEnabled) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::InputFloat("deg##hd", &app.hullDeg, 0, 0, "%.1f");
            changed |= ImGui::IsItemEdited();
            if (app.hullDeg < 0)   app.hullDeg += 360;
            if (app.hullDeg >=360) app.hullDeg -= 360;
        }

        ImGui::Text("Trajectory:");
        ImGui::SameLine(70);
        if (ImGui::RadioButton("Low", !app.useHighArc)) { app.useHighArc = false; changed = true; }
        ImGui::SameLine();
        if (ImGui::RadioButton("High", app.useHighArc))  { app.useHighArc = true;  changed = true; }
    }

    // ── Results ─────────────────────────────────────────────────────
    ImGui::Separator();
    app.dirty |= changed;
    if (app.dirty) {
        recalc(app);
        app.dirty = false;
    }
    auto& s = app.solution;

    ImGui::Text("Distance:  %.0f m  (%.2f km)", s.distanceM, s.distanceM/1000.0);
    ImGui::Text("Azimuth:   %.1f deg", s.azimuthDeg);

    if (app.terrainLoaded) {
        ImGui::Text("Delta Z:   %+.1f m", s.deltaZ);
    }

    ImGui::Separator();

    // MIL display — always visible
    double displayMil = s.useHighArc ? s.finalHighMil : s.finalLowMil;
    bool hasSol = s.useHighArc ? s.hasHigh : s.hasLow;

    if (!s.inRange) {
        ImGui::TextColored({1,0.4f,0.4f,1}, "OUT OF RANGE");
    } else if (!hasSol) {
        ImGui::TextColored({1,0.4f,0.4f,1}, "NO FIRING SOLUTION");
    } else {
        ImGui::TextColored({0.5f,1,0.6f,1}, "MIL:  %.0f", std::round(displayMil));
    }

    if (isSph && app.hullEnabled) {
        double corr = s.useHighArc
            ? s.hullCorrectionHighMil : s.hullCorrectionLowMil;
        if (std::abs(corr) > 0.05)
            ImGui::Text("Hull correction: %+.1f MIL", corr);
    }

    ImGui::End();
}

// ── Main ───────────────────────────────────────────────────────────────

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "WARDOGS Calculator Overlay",
        440, 700,
        SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT
    );
    if (!window) {
        // Fallback: try without transparent (some drivers don't support it)
        window = SDL_CreateWindow(
            "WARDOGS Calculator Overlay",
            440, 700,
            SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS
        );
    }
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    // ── Window icon (from .ico file, same as tray) ─────────────────
    win32SetWindowIcon(window);

    // Platform-specific overlay setup
    win32SetAlwaysOnTop(window);

    // ── System tray icon ──────────────────────────────────────────
    trayCreateWindow();
    trayAdd();

    // ── ImGui init ──────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Font with Cyrillic support
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\verdana.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };
    bool fontLoaded = false;
    for (auto& path : fontPaths) {
        ImFont* f = io.Fonts->AddFontFromFileTTF(path, 16.0f, nullptr,
                                                  io.Fonts->GetGlyphRangesCyrillic());
        if (f) { fontLoaded = true; break; }
    }
    if (!fontLoaded) {
        // Fallback: default font with Cyrillic (may show boxes)
        io.Fonts->AddFontDefault();
    }

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding  = 4.0f;
    // Fully opaque ImGui window (transparent only outside)
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // ── Application state ───────────────────────────────────────────
    App app;
    g_visiblePtr = &app.visible;
    g_hotkeyVkPtr = &app.hotkeyVk;
    registerHotkey(app.hotkeyVk);
    app.calc.setWeapon(weaponIds[app.weaponIdx]);
    app.calc.setMap(app.mapId);
    app.calc.setOrigin({(double)app.originX, (double)app.originY});
    app.calc.setTarget({(double)app.targetX, (double)app.targetY});

    // Auto-detect wardogs-data/
    {
        std::string found = findWardogsData();
        if (!found.empty()) {
            std::strncpy(app.dataDir, found.c_str(), sizeof(app.dataDir) - 1);
            tryLoadTerrain(app);
        }
    }
    loadSavedTargets(app);

    // Window dragging state
    bool dragging = false;
    int dragOffX = 0, dragOffY = 0;  // offset from mouse to window top-left

    bool running = true;
    while (running) {
        if (g_exitRequested) running = false;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            // Start drag: record offset from mouse to window origin
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                && event.button.button == SDL_BUTTON_LEFT
                && g_titleBarHover) {
                dragging = true;
                float mx, my;
                int wx, wy;
                SDL_GetGlobalMouseState(&mx, &my);
                SDL_GetWindowPosition(window, &wx, &wy);
                dragOffX = (int)mx - wx;
                dragOffY = (int)my - wy;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP
                && event.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
            }
        }

        if (!app.visible) {
            SDL_HideWindow(window);
            SDL_Delay(50);
            continue;
        } else {
            SDL_ShowWindow(window);
        }

        // Smooth drag: update window position every frame from global mouse
        if (dragging) {
            float mx, my;
            SDL_GetGlobalMouseState(&mx, &my);
            SDL_SetWindowPosition(window, (int)mx - dragOffX, (int)my - dragOffY);
        }

        // Process Win32 messages for tray window (hotkey, tray icon)
        {
            MSG msg;
            while (PeekMessageW(&msg, g_hwndTray, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        drawCalculator(app);

        // Hotkey reassignment: capture key via ImGui after frame
        if (app.waitingForKey) {
            for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; k++) {
                if (ImGui::IsKeyPressed((ImGuiKey)k)) {
                    int vk = 0;
                    if (k >= ImGuiKey_F1 && k <= ImGuiKey_F24) vk = VK_F1 + (k - ImGuiKey_F1);
                    else if (k >= ImGuiKey_0 && k <= ImGuiKey_9) vk = '0' + (k - ImGuiKey_0);
                    else if (k >= ImGuiKey_A && k <= ImGuiKey_Z) vk = 'A' + (k - ImGuiKey_A);
                    else if (k == ImGuiKey_GraveAccent) vk = VK_OEM_3;
                    else if (k == ImGuiKey_Escape) vk = 0;
                    if (vk != 0 || k == ImGuiKey_Escape) {
                        app.hotkeyVk = vk;
                        registerHotkey(app.hotkeyVk);
                        app.waitingForKey = false;
                    }
                    break;
                }
            }
        }

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        // Cap frame rate ~60fps to reduce CPU when idle
        SDL_Delay(16);
    }

    trayRemove();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}