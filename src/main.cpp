/* =====================================================================
   main.cpp  —  WARDOGS Artillery Calculator  (C++ / ImGui / SDL2)
   ===================================================================== */

// ── ImGui + SDL2 + OpenGL3 backends ────────────────────────────────────
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>

#include "calculator.h"
#include "icon_data.h"

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

// Forward declaration
static void trayAdd();
static void trayRemove();

static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == g_taskbarRestart) {
        trayAdd(); // Explorer restarted — re-add tray icon
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

// Create hidden window for tray messages
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

static void trayAdd() {
    if (!g_hwndTray) return;
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = g_hwndTray;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    // Try loading icon from exe directory, fall back to resource
    wchar_t exeDir[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    // Strip filename → append assets\y.ico
    wchar_t* lastSlash = wcsrchr(exeDir, L'\\');
    if (lastSlash) {
        wcscpy(lastSlash + 1, L"assets\\y.ico");
        g_nid.hIcon = (HICON)LoadImageW(nullptr, exeDir, IMAGE_ICON,
                                          16, 16, LR_LOADFROMFILE);
    }
    if (!g_nid.hIcon)
        g_nid.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1));
    if (!g_nid.hIcon)
        g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
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

static void win32SetAlwaysOnTop(SDL_Window* w) {
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(w, &info)) {
        HWND hwnd = info.info.win.window;
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE);
        SetWindowLongW(hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
    }
}

static void win32SetClickThrough(SDL_Window* w, bool enable) {
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(w, &info)) {
        HWND hwnd = info.info.win.window;
        LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE);
        if (enable)
            ex |= WS_EX_TRANSPARENT;
        else
            ex &= ~WS_EX_TRANSPARENT;
        SetWindowLongW(hwnd, GWL_EXSTYLE, ex);
    }
}

static void win32RegisterHotkeys(HWND hwnd) {
    RegisterHotKey(hwnd, 1, 0, VK_F10);
    RegisterHotKey(hwnd, 2, 0, VK_F11);
    RegisterHotKey(hwnd, 3, 0, VK_F12);
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
        exeDir + "/wardogs-data",        // next to exe
        "wardogs-data",                  // in cwd
        exeDir + "/../wardogs-data",     // one level up (e.g. exe in build/)
    };

    for (auto& c : paths) {
        std::string test = c + "/data/maps/kavkazi/terrain.json";
        std::ifstream f(test);
        if (f) return c;
    }
    return {};
}
#endif

// ── Application state ──────────────────────────────────────────────────
struct App {
    wd::Calculator calc;

    // Current input strings (for ImGui input fields)
    char  weaponId[16]   = "mortar";
    char  mapId[16]      = "bakurani";
    float originX = -8000.0f, originY = -12000.0f;
    float targetX = -7500.0f, targetY = -11800.0f;
    float hullDeg = 0.0f;
    bool  hullEnabled = false;
    bool  useHighArc = true;
    float manualAdj = 0.0f;

    // Terrain (wardogs-data)
    bool  terrainLoaded = false;
    char  dataDir[512] = "";
    char  lastMapId[16] = "";      // track map changes for auto-reload
    char  statusMsg[256] = "";

    // Result
    wd::Solution solution;

    // Overlay state
    bool clickThrough = false;
    bool visible = true;
};

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
            std::snprintf(app.statusMsg, sizeof(app.statusMsg), "Failed to load terrain");
        }
    } else {
        std::snprintf(app.statusMsg, sizeof(app.statusMsg), "No terrain data for this map");
    }
    std::strcpy(app.lastMapId, app.mapId);
}

static void recalc(App& app) {
    app.calc.setWeapon(app.weaponId);
    app.calc.setMap(app.mapId);
    app.calc.setOrigin({(double)app.originX, (double)app.originY});
    app.calc.setTarget({(double)app.targetX, (double)app.targetY});
    app.calc.setHullHeading(app.hullEnabled ? (double)app.hullDeg : NAN);
    app.calc.setUseHighArc(app.useHighArc);
    app.calc.setManualMilAdj((double)app.manualAdj);

    // Auto-reload terrain if map changed
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
    if (ImGui::BeginCombo("Weapon", app.weaponId)) {
        if (ImGui::Selectable("mortar", std::strcmp(app.weaponId,"mortar")==0))
            std::strcpy(app.weaponId, "mortar");
        if (ImGui::Selectable("spg", std::strcmp(app.weaponId,"spg")==0))
            std::strcpy(app.weaponId, "spg");
        ImGui::EndCombo();
    }

    // ── Map ─────────────────────────────────────────────────────────
    if (ImGui::BeginCombo("Map", app.mapId)) {
        const char* maps[] = {"bakurani","ozeti","zestafona"};
        for (auto m : maps) {
            if (ImGui::Selectable(m, std::strcmp(app.mapId, m)==0))
                std::strcpy(app.mapId, m);
        }
        ImGui::EndCombo();
    }

    // ── Terrain (wardogs-data) ────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("wardogs-data directory:");
    ImGui::SetNextItemWidth(260);
    if (ImGui::InputText("##ddir", app.dataDir, sizeof(app.dataDir),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        tryLoadTerrain(app);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        tryLoadTerrain(app);
    }
    ImGui::SameLine();
    ImGui::TextColored(app.terrainLoaded
        ? ImVec4(0.3f,1,0.3f,1) : ImVec4(1,0.5f,0.3f,1),
        app.terrainLoaded ? "OK" : "---");
    if (app.statusMsg[0])
        ImGui::TextWrapped("%s", app.statusMsg);

    // ── Coordinates ─────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("Artillery position (m):");
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("X##ox", &app.originX, 0, 0, "%.0f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("Y##oy", &app.originY, 0, 0, "%.0f");

    ImGui::Text("Target position (m):");
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("X##tx", &app.targetX, 0, 0, "%.0f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("Y##ty", &app.targetY, 0, 0, "%.0f");

    if (ImGui::Button("Swap")) {
        std::swap(app.originX, app.targetX);
        std::swap(app.originY, app.targetY);
    }

    // ── SPH-2 controls ──────────────────────────────────────────────
    bool isSph = (std::strcmp(app.weaponId, "spg") == 0);
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

        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("Manual MIL adj", &app.manualAdj, 0, 0, "%.1f");
    }

    // ── Results ─────────────────────────────────────────────────────
    ImGui::Separator();
    recalc(app);
    auto& s = app.solution;

    ImGui::Text("Distance:  %.0f m  (%.2f km)", s.distanceM, s.distanceM/1000.0);
    ImGui::Text("Azimuth:   %.1f°", s.azimuthDeg);

    if (app.terrainLoaded) {
        ImGui::Text("Origin Z:  %.1f m", s.originZ);
        ImGui::Text("Target Z:  %.1f m", s.targetZ);
        ImGui::Text("Delta Z:   %+.1f m", s.deltaZ);
    }

    ImGui::Separator();

    if (!s.inRange) {
        ImGui::TextColored({1,0.4f,0.4f,1}, "OUT OF RANGE");
    } else {
        // Determine displayed MIL
        double displayMil = s.useHighArc ? s.finalHighMil : s.finalLowMil;
        bool hasSol = s.useHighArc ? s.hasHigh : s.hasLow;

        if (!hasSol) {
            ImGui::TextColored({1,0.4f,0.4f,1}, "NO FIRING SOLUTION");
        } else {
            ImGui::PushFont(nullptr); // uses default; could use a large font
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
    }

    ImGui::End();
}

// ── Main ───────────────────────────────────────────────────────────────

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Transparent, borderless, always-on-top window
    SDL_Window* window = SDL_CreateWindow(
        "WARDOGS Calculator Overlay",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        400, 620,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
          | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_SKIP_TASKBAR
    );
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // ── Window icon (embedded RGBA data) ──────────────────────────
    {
        SDL_Surface* iconSurf = SDL_CreateRGBSurfaceFrom(
            (void*)ICON_RGBA, ICON_W, ICON_H, 32, ICON_W * 4,
            0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        if (iconSurf) {
            SDL_SetWindowIcon(window, iconSurf);
            SDL_FreeSurface(iconSurf);
        }
    }

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

    // Dark theme with transparency
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding  = 4.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.92f;

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // ── Application state ───────────────────────────────────────────
    App app;
    g_visiblePtr = &app.visible;   // connect tray toggle to visibility
    app.calc.setWeapon(app.weaponId);
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
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                running = false;
            }

            // Global hotkeys (F10/F11/F12) via SDL
            if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;
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

        // ── New frame ───────────────────────────────────────────────
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        drawCalculator(app);

        // ── Render ──────────────────────────────────────────────────
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // ── Cleanup ─────────────────────────────────────────────────────
    trayRemove();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}