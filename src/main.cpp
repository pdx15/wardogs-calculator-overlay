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
#define ID_TRAY_SHOW  1001
#define ID_TRAY_EXIT  1002

static NOTIFYICONDATAW g_nid{};
static HWND  g_hwndTray = nullptr;
static UINT  g_taskbarRestart = 0;
static bool* g_visiblePtr = nullptr;

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
            AppendMenuW(menu, MF_STRING, ID_TRAY_SHOW,
                        g_visiblePtr && *g_visiblePtr ? L"Hide overlay\tF10" : L"Show overlay\tF10");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
            PostMessage(hwnd, WM_NULL, 0, 0);
            return 0;
        }
    }
    if (msg == WM_COMMAND) {
        if (LOWORD(wp) == ID_TRAY_SHOW && g_visiblePtr) {
            *g_visiblePtr = !*g_visiblePtr;
        } else if (LOWORD(wp) == ID_TRAY_EXIT) {
            PostQuitMessage(0);
        }
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

static HICON loadIcoFromExeDir(const wchar_t* icoName) {
    std::string dir = getExeDir();
    // Convert to wide
    int wlen = MultiByteToWideChar(CP_UTF8, 0, dir.c_str(), -1, nullptr, 0);
    std::wstring wdir(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, dir.c_str(), -1, wdir.data(), wlen);

    // Search: exe dir, parent, grandparent (handles build/Release/ structure)
    std::wstring candidates[] = {
        wdir + L"\\" + icoName,
        wdir + L"\\..\\" + icoName,
        wdir + L"\\..\\..\\" + icoName,
    };
    for (auto& path : candidates) {
        HICON h = (HICON)LoadImageW(nullptr, path.c_str(), IMAGE_ICON, 0, 0,
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

    g_nid.hIcon = loadIcoFromExeDir(L"assets\\y.ico");
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

// Set window icon from .ico file (uses Win32 API, works for taskbar + alt-tab)
static void win32SetWindowIcon(SDL_Window* w) {
    HICON hIcon = loadIcoFromExeDir(L"assets\\y.ico");
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

    // Result
    wd::Solution solution;

    // Overlay
    bool clickThrough = false;
    bool visible = true;
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
    }

    app.solution = app.calc.solve();
}

// ── GUI ────────────────────────────────────────────────────────────────

static void drawCalculator(App& app) {
    ImGui::SetNextWindowSize({380, 0}, ImGuiCond_FirstUseEver);

    ImGui::Begin("WARDOGS Artillery Calculator", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize);

    // ── Weapon ──────────────────────────────────────────────────────
    if (ImGui::BeginCombo("Weapon", weaponNames[app.weaponIdx])) {
        for (int i = 0; i < 2; i++) {
            if (ImGui::Selectable(weaponNames[i], app.weaponIdx == i))
                app.weaponIdx = i;
        }
        ImGui::EndCombo();
    }

    // ── Map ─────────────────────────────────────────────────────────
    const char* currentMapName = app.mapId;
    for (int i = 0; i < 3; i++) {
        if (std::strcmp(app.mapId, mapIds[i]) == 0) {
            currentMapName = mapIds[i];
            break;
        }
    }
    if (ImGui::BeginCombo("Map", currentMapName)) {
        for (int i = 0; i < 3; i++) {
            if (ImGui::Selectable(mapIds[i], std::strcmp(app.mapId, mapIds[i]) == 0))
                std::strcpy(app.mapId, mapIds[i]);
        }
        ImGui::EndCombo();
    }

    // ── Terrain ─────────────────────────────────────────────────────
    if (app.terrainLoaded) {
        ImGui::Separator();
        ImGui::TextColored({0.3f,1,0.3f,1}, "Terrain: OK");
        ImGui::SameLine();
        if (ImGui::SmallButton("change")) {
            app.terrainLoaded = false;
        }
    } else {
        ImGui::Separator();
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

    // ── Coordinates ─────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("Artillery position (m):");
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("X##ox", &app.originX, 0, 0, "%.2f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("Y##oy", &app.originY, 0, 0, "%.2f");

    ImGui::Text("Target position (m):");
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("X##tx", &app.targetX, 0, 0, "%.2f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("Y##ty", &app.targetY, 0, 0, "%.2f");

    if (ImGui::Button("Swap")) {
        std::swap(app.originX, app.targetX);
        std::swap(app.originY, app.targetY);
    }

    // ── SPH-2 controls ──────────────────────────────────────────────
    bool isSph = (app.weaponIdx == 1);
    if (isSph) {
        ImGui::Separator();
        ImGui::Text("SPH-2 Platform Correction:");

        ImGui::Checkbox("Hull heading", &app.hullEnabled);
        if (app.hullEnabled) {
            ImGui::SetNextItemWidth(120);
            ImGui::InputFloat("Heading (deg)", &app.hullDeg, 0, 0, "%.1f");
            if (app.hullDeg < 0)   app.hullDeg += 360;
            if (app.hullDeg >=360) app.hullDeg -= 360;
        }

        ImGui::Text("Trajectory:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Low", !app.useHighArc)) app.useHighArc = false;
        ImGui::SameLine();
        if (ImGui::RadioButton("High", app.useHighArc))  app.useHighArc = true;
    }

    // ── Results ─────────────────────────────────────────────────────
    ImGui::Separator();
    recalc(app);
    auto& s = app.solution;

    ImGui::Text("Distance:  %.0f m  (%.2f km)", s.distanceM, s.distanceM/1000.0);
    ImGui::Text("Azimuth:   %.1f deg", s.azimuthDeg);

    if (app.terrainLoaded) {
        ImGui::Text("Delta Z:   %+.1f m", s.deltaZ);
    }

    ImGui::Separator();

    // MIL display
    double displayMil = s.useHighArc ? s.finalHighMil : s.finalLowMil;
    bool hasSol = s.useHighArc ? s.hasHigh : s.hasLow;

    if (!s.inRange) {
        ImGui::TextColored({1,0.4f,0.4f,1}, "OUT OF RANGE");
    } else if (!hasSol) {
        ImGui::TextColored({1,0.4f,0.4f,1}, "NO FIRING SOLUTION");
    } else {
        ImGui::PushFont(nullptr);
        ImGui::TextColored({0.5f,1,0.6f,1}, "MIL:  %.0f", std::round(displayMil));
        ImGui::PopFont();
    }

    if (isSph && s.hasLow && s.hasHigh) {
        ImGui::Text("Low:   %.0f MIL", std::round(s.finalLowMil));
        ImGui::Text("High:  %.0f MIL", std::round(s.finalHighMil));
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
        400, 620,
        SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT
    );
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

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = event.key.key;
                if (key == SDLK_F10) {
                    app.visible = !app.visible;
                    if (app.visible) SDL_ShowWindow(window);
                    else             SDL_HideWindow(window);
                }
                if (key == SDLK_F11) {
                    app.clickThrough = !app.clickThrough;
                    win32SetClickThrough(window, app.clickThrough);
                }
                if (key == SDLK_F12) {
                    app.clickThrough = false;
                    app.visible = true;
                    win32SetClickThrough(window, false);
                    SDL_ShowWindow(window);
                    SDL_RaiseWindow(window);
                }
            }
        }

        if (!app.visible) {
            SDL_Delay(50);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        drawCalculator(app);

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
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