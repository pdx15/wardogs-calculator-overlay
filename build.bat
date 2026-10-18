@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul 2>&1
REM ═══════════════════════════════════════════════════════════════
REM  WARDOGS Artillery Calculator — Build Script
REM ═══════════════════════════════════════════════════════════════
REM  Usage:
REM    build.bat                  (auto-detect everything)
REM    build.bat vs               (force Visual Studio)
REM    build.bat mingw64          (force MinGW-w64)
REM    build.bat mingw32          (force MinGW32)
REM    build.bat vs C:\SDL2       (specify SDL2 path)
REM    build.bat vs C:\SDL2 Debug (debug build)
REM ═══════════════════════════════════════════════════════════════

set "CONFIG=Release"
set "COMPILER="
set "SDL2_DIR="
set "BUILD_DIR=build"
set "GENERATOR="

REM ── Parse arguments ─────────────────────────────────────────────
for %%A in (%*) do (
    if /i "%%A"=="vs"       set "COMPILER=vs"
    if /i "%%A"=="mingw64"  set "COMPILER=mingw64"
    if /i "%%A"=="mingw32"  set "COMPILER=mingw32"
    if /i "%%A"=="Debug"    set "CONFIG=Debug"
    if /i "%%A"=="Release"  set "CONFIG=Release"
    if /i "%%A"=="--help"   goto :help
    if /i "%%A"=="-h"       goto :help
    REM Check if it looks like a path (contains :\ or starts with .)
    echo %%A | findstr /r "^[A-Za-z]:\\ ^\." >nul 2>&1
    if !errorlevel!==0 set "SDL2_DIR=%%A"
)

REM ── Check CMake ────────────────────────────────────────────────
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH.
    echo         Install from https://cmake.org/download/ or via:
    echo           winget install Kitware.CMake
    echo           choco install cmake
    exit /b 1
)

REM ── Auto-detect compilers ──────────────────────────────────────
set "HAS_VS=0"
set "HAS_MINGW64=0"
set "HAS_MINGW32=0"

REM Check Visual Studio (vcvarsall.bat)
for %%V in (
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
) do (
    if exist %%V (
        set "HAS_VS=1"
        set "VCVARSALL=%%~V"
    )
)

REM Check MinGW in PATH
where g++ >nul 2>&1
if !errorlevel!==0 (
    for /f "tokens=3" %%V in ('g++ --version 2^>^&1 ^| findstr /i "gcc\|g++\|MinGW"') do (
        echo %%V | findstr /i "i686\|i386\|mingw32" >nul 2>&1
        if !errorlevel!==0 set "HAS_MINGW32=1"
        echo %%V | findstr /i "x86_64\|mingw64" >nul 2>&1
        if !errorlevel!==0 set "HAS_MINGW64=1"
    )
    REM If we couldn't determine arch from version string, check g++ -dumpmachine
    if !HAS_MINGW32!==0 if !HAS_MINGW64!==0 (
        for /f %%M in ('g++ -dumpmachine 2^>nul') do (
            echo %%M | findstr /i "i686\|i386" >nul 2>&1
            if !errorlevel!==0 set "HAS_MINGW32=1"
            echo %%M | findstr /i "x86_64\|mingw64" >nul 2>&1
            if !errorlevel!==0 set "HAS_MINGW64=1"
        )
    )
)

REM Check MSYS2 paths (not in PATH by default)
if exist "C:\msys64\mingw64\bin\g++.exe" set "HAS_MINGW64=1"
if exist "C:\msys64\mingw32\bin\g++.exe" set "HAS_MINGW32=1"

REM ── Select compiler ────────────────────────────────────────────
if not "%COMPILER%"=="" goto :compiler_selected

REM Auto-select: prefer MinGW64 > VS > MinGW32
if !HAS_MINGW64!==1 ( set "COMPILER=mingw64" & goto :compiler_selected )
if !HAS_VS!==1      ( set "COMPILER=vs"      & goto :compiler_selected )
if !HAS_MINGW32!==1 ( set "COMPILER=mingw32" & goto :compiler_selected )

echo [ERROR] No C++ compiler found.
echo.
echo Available compilers detected:
if !HAS_VS!==1      echo   [x] Visual Studio
if !HAS_VS!==0      echo   [ ] Visual Studio
if !HAS_MINGW64!==1 echo   [x] MinGW-w64 (64-bit)
if !HAS_MINGW64!==0 echo   [ ] MinGW-w64 (64-bit)
if !HAS_MINGW32!==1 echo   [x] MinGW32 (32-bit)
if !HAS_MINGW32!==0 echo   [ ] MinGW32 (32-bit)
echo.
echo Install one of:
echo   Visual Studio: https://visualstudio.microsoft.com/
echo   MSYS2:         https://www.msys2.org/  then: pacman -S mingw-w64-x86_64-gcc
exit /b 1

:compiler_selected
echo ══════════════════════════════════════════════════════════════
echo  Compiler:  %COMPILER%
echo  Config:    %CONFIG%
echo ══════════════════════════════════════════════════════════════

REM Show detected compilers
echo.
echo Detected:
if !HAS_VS!==1      echo   [x] Visual Studio
if !HAS_VS!==0      echo   [ ] Visual Studio
if !HAS_MINGW64!==1 echo   [x] MinGW-w64 (64-bit)
if !HAS_MINGW64!==0 echo   [ ] MinGW-w64 (64-bit)
if !HAS_MINGW32!==1 echo   [x] MinGW32 (32-bit)
if !HAS_MINGW32!==0 echo   [ ] MinGW32 (32-bit)
echo.

REM ── Auto-detect SDL2 ──────────────────────────────────────────
if "%SDL2_DIR%"=="" (
    if exist "SDL2\cmake\SDL2Config.cmake"            set "SDL2_DIR=%CD%\SDL2"
    if exist "SDL2\cmake\sdl2-config.cmake"           set "SDL2_DIR=%CD%\SDL2"
    if exist "C:\SDL2\cmake\SDL2Config.cmake"         set "SDL2_DIR=C:\SDL2"
    if exist "C:\SDL2-2.28.5\cmake\SDL2Config.cmake"  set "SDL2_DIR=C:\SDL2-2.28.5"
    if exist "C:\SDL2-2.28.4\cmake\SDL2Config.cmake"  set "SDL2_DIR=C:\SDL2-2.28.4"
    if exist "C:\SDL2-2.30.0\cmake\SDL2Config.cmake"  set "SDL2_DIR=C:\SDL2-2.30.0"
    REM MSYS2
    if exist "C:\msys64\mingw64\lib\cmake\SDL2\SDL2Config.cmake" (
        if "%COMPILER%"=="mingw64" set "SDL2_DIR=C:\msys64\mingw64"
    )
    if exist "C:\msys64\mingw32\lib\cmake\SDL2\SDL2Config.cmake" (
        if "%COMPILER%"=="mingw32" set "SDL2_DIR=C:\msys64\mingw32"
    )
)

if "%SDL2_DIR%"=="" (
    echo [ERROR] SDL2 not found. Download from:
    echo         https://github.com/libsdl-org/SDL/releases
    echo         Extract and run: build.bat [compiler] C:\SDL2
    echo.
    echo         Or via MSYS2: pacman -S mingw-w64-x86_64-SDL2
    exit /b 1
)

REM Resolve cmake config path
set "SDL2_CMAKE=%SDL2_DIR%\cmake"
if exist "%SDL2_DIR%\lib\cmake\SDL2\SDL2Config.cmake" set "SDL2_CMAKE=%SDL2_DIR%\lib\cmake\SDL2"
if exist "%SDL2_DIR%\lib\cmake\SDL2\sdl2-config.cmake" set "SDL2_CMAKE=%SDL2_DIR%\lib\cmake\SDL2"

echo SDL2:      %SDL2_DIR%
echo.

REM ── Configure + Build ──────────────────────────────────────────

if "%COMPILER%"=="vs" goto :build_vs
if "%COMPILER%"=="mingw64" goto :build_mingw64
if "%COMPILER%"=="mingw32" goto :build_mingw32
echo [ERROR] Unknown compiler: %COMPILER%
exit /b 1

REM ──────────────────────────────────────────────────────────────
:build_vs
REM ──────────────────────────────────────────────────────────────
if not defined VCVARSALL (
    echo [ERROR] Visual Studio vcvarsall.bat not found.
    exit /b 1
)

echo [1/3] Setting up MSVC environment...
call "%VCVARSALL%" x64
if errorlevel 1 (
    echo [ERROR] vcvarsall.bat failed.
    exit /b 1
)

echo [2/3] Configuring...
cmake -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64 -DSDL2_DIR="%SDL2_CMAKE%" 2>nul
if errorlevel 1 (
    cmake -B %BUILD_DIR% -G "Visual Studio 16 2019" -A x64 -DSDL2_DIR="%SDL2_CMAKE%"
)
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b 1
)

echo [3/3] Building %CONFIG%...
cmake --build %BUILD_DIR% --config %CONFIG% -j
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

set "OUTPUT=%BUILD_DIR%\%CONFIG%\calculator.exe"
if exist "%SDL2_DIR%\lib\x64\SDL2.dll" (
    copy "%SDL2_DIR%\lib\x64\SDL2.dll" "%BUILD_DIR%\%CONFIG%\" >nul 2>&1
)
goto :done

REM ──────────────────────────────────────────────────────────────
:build_mingw64
REM ──────────────────────────────────────────────────────────────
echo [1/3] Configuring (MinGW-w64)...
cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL2_DIR="%SDL2_CMAKE%"
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b 1
)

echo [2/3] Building...
cmake --build %BUILD_DIR% -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

set "OUTPUT=%BUILD_DIR%\calculator.exe"
if exist "%SDL2_DIR%\bin\SDL2.dll" copy "%SDL2_DIR%\bin\SDL2.dll" "%BUILD_DIR%\" >nul 2>&1
if exist "%SDL2_DIR%\lib\x64\SDL2.dll" copy "%SDL2_DIR%\lib\x64\SDL2.dll" "%BUILD_DIR%\" >nul 2>&1
goto :done

REM ──────────────────────────────────────────────────────────────
:build_mingw32
REM ──────────────────────────────────────────────────────────────
set "BUILD_DIR=build32"
echo [1/3] Configuring (MinGW32)...
cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL2_DIR="%SDL2_CMAKE%"
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b 1
)

echo [2/3] Building...
cmake --build %BUILD_DIR% -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

set "OUTPUT=%BUILD_DIR%\calculator.exe"
if exist "%SDL2_DIR%\bin\SDL2.dll" copy "%SDL2_DIR%\bin\SDL2.dll" "%BUILD_DIR%\" >nul 2>&1
goto :done

REM ──────────────────────────────────────────────────────────────
:done
REM ──────────────────────────────────────────────────────────────
echo.
echo ══════════════════════════════════════════════════════════════
echo  BUILD SUCCESSFUL
echo ══════════════════════════════════════════════════════════════
echo  Output: %OUTPUT%
echo.
if exist "%BUILD_DIR%\SDL2.dll" echo  SDL2.dll:  %BUILD_DIR%\SDL2.dll
echo.
echo  To run: place wardogs-data/ next to calculator.exe
echo ══════════════════════════════════════════════════════════════
goto :eof

REM ──────────────────────────────────────────────────────────────
:help
REM ──────────────────────────────────────────────────────────────
echo WARDOGS Artillery Calculator — Build Script
echo.
echo Usage:
echo   build.bat [COMPILER] [SDL2_PATH] [CONFIG]
echo.
echo   COMPILER:   vs, mingw64, mingw32 (default: auto-detect)
echo   SDL2_PATH:  path to SDL2 directory (default: auto-detect)
echo   CONFIG:     Release or Debug (default: Release)
echo.
echo Examples:
echo   build.bat                        auto-detect compiler + SDL2
echo   build.bat vs                     force Visual Studio
echo   build.bat mingw64                force MinGW-w64
echo   build.bat vs C:\SDL2             specify SDL2 path
echo   build.bat mingw64 C:\SDL2 Debug  debug build with MinGW
echo.
echo Auto-detect search paths:
echo   Compiler:  PATH, MSYS2 (C:\msys64\mingw64\bin)
echo   Visual Studio: 2019/2022 Community/Pro/Enterprise
echo   SDL2:      SDL2\, C:\SDL2, C:\SDL2-2.28.x, MSYS2