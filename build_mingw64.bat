@echo off
setlocal
REM ── WARDOGS Calculator — Build with MinGW-w64 ──────────────────
REM
REM  Prerequisites:
REM    - MinGW-w64 (MSYS2 or standalone)
REM    - SDL2 development library
REM
REM  Usage:
REM    build_mingw64.bat                     (auto-detect SDL2)
REM    build_mingw64.bat C:\SDL2             (specify SDL2 path)
REM    build_mingw64.bat C:\SDL2 Debug       (debug build)
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
    REM MSYS2 paths
    if exist "C:\msys64\mingw64\lib\cmake\SDL2" set "SDL2_DIR=C:\msys64\mingw64"
)

if "%SDL2_DIR%"=="" (
    echo [ERROR] SDL2 not found. Install via MSYS2:
    echo         pacman -S mingw-w64-x86_64-SDL2
    echo         Or download from https://github.com/libsdl-org/SDL/releases
    echo         and pass path: build_mingw64.bat C:\SDL2
    exit /b 1
)

REM Check for cmake config — MinGW/MSYS2 uses different path structure
set CMAKE_SDL2_DIR=%SDL2_DIR%\cmake
if exist "%SDL2_DIR%\lib\cmake\SDL2\SDL2Config.cmake" (
    set "CMAKE_SDL2_DIR=%SDL2_DIR%\lib\cmake\SDL2"
)

echo [1/3] Configuring (%CONFIG%, SDL2=%SDL2_DIR%)...
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL2_DIR="%CMAKE_SDL2_DIR%"
if errorlevel 1 (
    echo [ERROR] CMake configure failed. Is MinGW-w64 in PATH?
    exit /b 1
)

echo [2/3] Building...
cmake --build build -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [3/3] Done.
echo.
echo Output: build\calculator.exe
if exist "%SDL2_DIR%\bin\SDL2.dll" (
    copy "%SDL2_DIR%\bin\SDL2.dll" "build\" >nul 2>&1
    echo Copied SDL2.dll to build\
)
if exist "%SDL2_DIR%\lib\x64\SDL2.dll" (
    copy "%SDL2_DIR%\lib\x64\SDL2.dll" "build\" >nul 2>&1
    echo Copied SDL2.dll to build\
)
echo.
goto :eof

:help
echo Usage: build_mingw64.bat [SDL2_PATH] [CONFIG]
echo   SDL2_PATH  Path to SDL2 directory (default: auto-detect)
echo   CONFIG     Release or Debug (default: Release)
echo.
echo Examples:
echo   build_mingw64.bat
echo   build_mingw64.bat C:\SDL2
echo   build_mingw64.bat C:\SDL2 Debug