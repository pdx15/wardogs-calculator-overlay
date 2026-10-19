@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul 2>&1
REM ═══════════════════════════════════════════════════════════════
REM  WARDOGS Artillery Calculator — Build Script (SDL3)
REM ═══════════════════════════════════════════════════════════════
REM  Usage:
REM    build.bat                  (auto-detect everything)
REM    build.bat vs               (force Visual Studio)
REM    build.bat mingw64          (force MinGW-w64)
REM    build.bat mingw32          (force MinGW32)
REM    build.bat vs C:\SDL3       (specify SDL3 path)
REM    build.bat vs C:\SDL3 Debug (debug build)
REM ═══════════════════════════════════════════════════════════════

set "CONFIG=Release"
set "COMPILER="
set "SDL3_DIR="
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
    echo %%A | findstr /r "^[A-Za-z]:\\ ^\." >nul 2>&1
    if !errorlevel!==0 set "SDL3_DIR=%%A"
)

REM ── Check CMake ────────────────────────────────────────────────
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH.
    echo         Install from https://cmake.org/download/
    echo           winget install Kitware.CMake
    echo           choco install cmake
    exit /b 1
)

REM ── Auto-detect compilers ──────────────────────────────────────
set "HAS_VS=0"
set "HAS_MINGW64=0"
set "HAS_MINGW32=0"

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

where g++ >nul 2>&1
if !errorlevel!==0 (
    for /f %%M in ('g++ -dumpmachine 2^>nul') do (
        echo %%M | findstr /i "i686\|i386" >nul 2>&1
        if !errorlevel!==0 set "HAS_MINGW32=1"
        echo %%M | findstr /i "x86_64\|mingw64" >nul 2>&1
        if !errorlevel!==0 set "HAS_MINGW64=1"
    )
)

if exist "C:\msys64\mingw64\bin\g++.exe" set "HAS_MINGW64=1"
if exist "C:\msys64\mingw32\bin\g++.exe" set "HAS_MINGW32=1"

REM ── Select compiler ────────────────────────────────────────────
if not "%COMPILER%"=="" goto :compiler_selected

if !HAS_MINGW64!==1 ( set "COMPILER=mingw64" & goto :compiler_selected )
if !HAS_VS!==1      ( set "COMPILER=vs"      & goto :compiler_selected )
if !HAS_MINGW32!==1 ( set "COMPILER=mingw32" & goto :compiler_selected )

echo [ERROR] No C++ compiler found.
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

echo.
echo Detected:
if !HAS_VS!==1      echo   [x] Visual Studio
if !HAS_VS!==0      echo   [ ] Visual Studio
if !HAS_MINGW64!==1 echo   [x] MinGW-w64 (64-bit)
if !HAS_MINGW64!==0 echo   [ ] MinGW-w64 (64-bit)
if !HAS_MINGW32!==1 echo   [x] MinGW32 (32-bit)
if !HAS_MINGW32!==0 echo   [ ] MinGW32 (32-bit)
echo.

REM ── Auto-detect SDL3 ──────────────────────────────────────────
if "%SDL3_DIR%"=="" (
    if exist "SDL3\cmake\SDL3\SDL3Config.cmake"           set "SDL3_DIR=%CD%\SDL3"
    if exist "C:\SDL3\cmake\SDL3\SDL3Config.cmake"        set "SDL3_DIR=C:\SDL3"
    if exist "C:\SDL3-3.4.18\cmake\SDL3\SDL3Config.cmake" set "SDL3_DIR=C:\SDL3-3.4.18"
    REM MSYS2
    if exist "C:\msys64\mingw64\lib\cmake\SDL3\SDL3Config.cmake" (
        if "%COMPILER%"=="mingw64" set "SDL3_DIR=C:\msys64\mingw64"
    )
    if exist "C:\msys64\mingw32\lib\cmake\SDL3\SDL3Config.cmake" (
        if "%COMPILER%"=="mingw32" set "SDL3_DIR=C:\msys64\mingw32"
    )
)

if "%SDL3_DIR%"=="" (
    echo [ERROR] SDL3 not found. Download from:
    echo         https://github.com/libsdl-org/SDL/releases
    echo         Look for: SDL3-3.x.x-win32-x64.zip
    echo         Extract and run: build.bat [compiler] C:\SDL3
    echo.
    echo         Or via MSYS2: pacman -S mingw-w64-x86_64-sdl3
    exit /b 1
)

REM Resolve cmake config path
set "SDL3_CMAKE=%SDL3_DIR%\cmake\SDL3"
if exist "%SDL3_DIR%\lib\cmake\SDL3\SDL3Config.cmake" set "SDL3_CMAKE=%SDL3_DIR%\lib\cmake\SDL3"

echo SDL3:      %SDL3_DIR%
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
cmake -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64 -DSDL3_DIR="%SDL3_CMAKE%" 2>nul
if errorlevel 1 (
    cmake -B %BUILD_DIR% -G "Visual Studio 16 2019" -A x64 -DSDL3_DIR="%SDL3_CMAKE%"
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
if exist "%SDL3_DIR%\lib\x64\SDL3.dll" (
    copy "%SDL3_DIR%\lib\x64\SDL3.dll" "%BUILD_DIR%\%CONFIG%\" >nul 2>&1
)
if exist "%SDL3_DIR%\SDL3.dll" (
    copy "%SDL3_DIR%\SDL3.dll" "%BUILD_DIR%\%CONFIG%\" >nul 2>&1
)
goto :done

REM ──────────────────────────────────────────────────────────────
:build_mingw64
REM ──────────────────────────────────────────────────────────────
echo [1/3] Configuring (MinGW-w64)...
cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL3_DIR="%SDL3_CMAKE%"
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
if exist "%SDL3_DIR%\bin\SDL3.dll" copy "%SDL3_DIR%\bin\SDL3.dll" "%BUILD_DIR%\" >nul 2>&1
if exist "%SDL3_DIR%\lib\x64\SDL3.dll" copy "%SDL3_DIR%\lib\x64\SDL3.dll" "%BUILD_DIR%\" >nul 2>&1
if exist "%SDL3_DIR%\SDL3.dll" copy "%SDL3_DIR%\SDL3.dll" "%BUILD_DIR%\" >nul 2>&1
goto :done

REM ──────────────────────────────────────────────────────────────
:build_mingw32
REM ──────────────────────────────────────────────────────────────
set "BUILD_DIR=build32"
echo [1/3] Configuring (MinGW32)...
cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL3_DIR="%SDL3_CMAKE%"
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
if exist "%SDL3_DIR%\bin\SDL3.dll" copy "%SDL3_DIR%\bin\SDL3.dll" "%BUILD_DIR%\" >nul 2>&1
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
if exist "%BUILD_DIR%\SDL3.dll" echo  SDL3.dll:  %BUILD_DIR%\SDL3.dll
echo.
echo  To run: place wardogs-data/ next to calculator.exe
echo ══════════════════════════════════════════════════════════════
goto :eof

REM ──────────────────────────────────────────────────────────────
:help
REM ──────────────────────────────────────────────────────────────
echo WARDOGS Artillery Calculator — Build Script (SDL3)
echo.
echo Usage:
echo   build.bat [COMPILER] [SDL3_PATH] [CONFIG]
echo.
echo   COMPILER:   vs, mingw64, mingw32 (default: auto-detect)
echo   SDL3_PATH:  path to SDL3 directory (default: auto-detect)
echo   CONFIG:     Release or Debug (default: Release)
echo.
echo Examples:
echo   build.bat                        auto-detect compiler + SDL3
echo   build.bat vs                     force Visual Studio
echo   build.bat mingw64                force MinGW-w64
echo   build.bat vs C:\SDL3             specify SDL3 path
echo   build.bat mingw64 C:\SDL3 Debug  debug build with MinGW
echo.
echo Auto-detect search paths:
echo   Compiler:  PATH, MSYS2 (C:\msys64\mingw64\bin)
echo   Visual Studio: 2019/2022 Community/Pro/Enterprise
echo   SDL3:      SDL3\, C:\SDL3, C:\SDL3-3.4.18, MSYS2