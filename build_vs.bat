@echo off
setlocal
REM ── WARDOGS Calculator — Build with Visual Studio (MSVC) ───────
REM
REM  Prerequisites:
REM    - Visual Studio 2019+ with "C++ desktop development" workload
REM    - SDL2 development library (SDL2-devel-2.x.x-VC.zip from libsdl.org)
REM
REM  Usage:
REM    build_vs.bat                    (auto-detect SDL2)
REM    build_vs.bat C:\SDL2            (specify SDL2 path)
REM    build_vs.bat C:\SDL2 Debug      (debug build)
REM ────────────────────────────────────────────────────────────────

set CONFIG=Release
set SDL2_DIR=

REM Parse arguments
if not "%~1"=="" (
    if /i "%~1"=="--help" goto :help
    if /i "%~1"=="-h" goto :help
    set "SDL2_DIR=%~1"
)
if not "%~2"=="" set "CONFIG=%~2"

REM Auto-detect SDL2
if "%SDL2_DIR%"=="" (
    if exist "SDL2\cmake"          set "SDL2_DIR=%CD%\SDL2"
    if exist "C:\SDL2\cmake"       set "SDL2_DIR=C:\SDL2"
    if exist "C:\SDL2-2.28.5\cmake" set "SDL2_DIR=C:\SDL2-2.28.5"
    if exist "C:\SDL2-2.30.0\cmake" set "SDL2_DIR=C:\SDL2-2.30.0"
)

if "%SDL2_DIR%"=="" (
    echo [ERROR] SDL2 not found. Download from https://github.com/libsdl-org/SDL/releases
    echo         Extract and pass path: build_vs.bat C:\SDL2
    echo         Or place SDL2 folder next to this script.
    exit /b 1
)

if not exist "%SDL2_DIR%\cmake\SDL2Config.cmake" (
    if not exist "%SDL2_DIR%\cmake\sdl2-config.cmake" (
        echo [ERROR] SDL2Config.cmake not found in %SDL2_DIR%\cmake\
        exit /b 1
    )
)

echo [1/3] Configuring (%CONFIG%, SDL2=%SDL2_DIR%)...
cmake -B build -G "Visual Studio 17 2022" -A x64 -DSDL2_DIR="%SDL2_DIR%\cmake" 2>nul
if errorlevel 1 (
    cmake -B build -G "Visual Studio 16 2019" -A x64 -DSDL2_DIR="%SDL2_DIR%\cmake"
)
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b 1
)

echo [2/3] Building...
cmake --build build --config %CONFIG% -j
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [3/3] Done.
echo.
if exist "build\%CONFIG%\calculator.exe" (
    echo Output: build\%CONFIG%\calculator.exe
    copy "%SDL2_DIR%\lib\x64\SDL2.dll" "build\%CONFIG%\" >nul 2>&1
    echo Copied SDL2.dll to build\%CONFIG%\
) else (
    echo Output: build\calculator.exe
)
echo.
goto :eof

:help
echo Usage: build_vs.bat [SDL2_PATH] [CONFIG]
echo   SDL2_PATH  Path to SDL2 development directory (default: auto-detect)
echo   CONFIG     Release or Debug (default: Release)
echo.
echo Examples:
echo   build_vs.bat
echo   build_vs.bat C:\SDL2
echo   build_vs.bat C:\SDL2 Debug