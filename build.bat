@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul 2>&1
title WARDOGS Artillery Calculator — Build

set "CONFIG=Release"
set "COMPILER="
set "SDL3_DIR="
set "BUILD_DIR=build"
set "SDL3_VERSION=3.4.18"

REM ── Parse arguments (non-interactive mode) ─────────────────────
if not "%~1"=="" (
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
    goto :check_cmake
)

REM ══════════════════════════════════════════════════════════════
REM  Interactive mode — detect and show menu
REM ══════════════════════════════════════════════════════════════

:check_cmake
where cmake >nul 2>&1
if errorlevel 1 (
    echo.
    echo  [ERROR] cmake not found in PATH.
    echo          Install: winget install Kitware.CMake
    echo                   choco install cmake
    echo          Or download: https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

REM ── Detect compilers ──────────────────────────────────────────
set "HAS_VS=0"
set "HAS_MINGW64=0"
set "HAS_MINGW32=0"
set "VCVARSALL="

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
        if not defined VCVARSALL set "VCVARSALL=%%~V"
    )
)

where g++ >nul 2>&1
if !errorlevel!==0 (
    for /f %%M in ('g++ -dumpmachine 2^>nul') do (
        echo %%M | findstr /i "x86_64\|mingw64" >nul 2>&1
        if !errorlevel!==0 set "HAS_MINGW64=1"
        echo %%M | findstr /i "i686\|i386" >nul 2>&1
        if !errorlevel!==0 set "HAS_MINGW32=1"
    )
)
if exist "C:\msys64\mingw64\bin\g++.exe" set "HAS_MINGW64=1"
if exist "C:\msys64\mingw32\bin\g++.exe" set "HAS_MINGW32=1"

REM ── If compiler specified via args, skip menu ──────────────────
if not "%COMPILER%"=="" goto :compiler_done

REM ── Compiler menu ──────────────────────────────────────────────
echo.
echo  ╔═══════════════════════════════════════════════════════════╗
echo  ║         WARDOGS Artillery Calculator — Build             ║
echo  ╠═══════════════════════════════════════════════════════════╣
echo  ║  Detected compilers:                                     ║
if !HAS_VS!==1      echo  ║    [x] Visual Studio (MSVC)                               ║
if !HAS_VS!==0      echo  ║    [ ] Visual Studio (not found)                          ║
if !HAS_MINGW64!==1 echo  ║    [x] MinGW-w64 (64-bit)                                 ║
if !HAS_MINGW64!==0 echo  ║    [ ] MinGW-w64 (not found)                              ║
if !HAS_MINGW32!==1 echo  ║    [x] MinGW32 (32-bit)                                   ║
if !HAS_MINGW32!==0 echo  ║    [ ] MinGW32 (not found)                                ║
echo  ╠═══════════════════════════════════════════════════════════╣
echo  ║  Select compiler:                                        ║

set "IDX=0"
set "MAP_VS=0"
set "MAP_M64=0"
set "MAP_M32=0"

if !HAS_VS!==1 (
    set /a IDX+=1
    echo  ║    !IDX^) Visual Studio                                     ║
    set "MAP_VS=!IDX!"
)
if !HAS_MINGW64!==1 (
    set /a IDX+=1
    echo  ║    !IDX^) MinGW-w64                                        ║
    set "MAP_M64=!IDX!"
)
if !HAS_MINGW32!==1 (
    set /a IDX+=1
    echo  ║    !IDX^) MinGW32                                          ║
    set "MAP_M32=!IDX!"
)

if !IDX!==0 (
    echo  ║    (none found — install a compiler first)               ║
    echo  ╚═══════════════════════════════════════════════════════════╝
    echo.
    pause
    exit /b 1
)

echo  ║                                                           ║
echo  ╚═══════════════════════════════════════════════════════════╝
echo.

set /p "CHOICE=  Enter number: "

if !MAP_VS!==!CHOICE!     set "COMPILER=vs"
if !MAP_M64!==!CHOICE!    set "COMPILER=mingw64"
if !MAP_M32!==!CHOICE!    set "COMPILER=mingw32"

if "%COMPILER%"=="" (
    echo  Invalid choice.
    pause
    exit /b 1
)

:compiler_done

REM ── Config menu ────────────────────────────────────────────────
if not "%~1"=="" goto :config_done

echo.
echo  Build configuration:
echo    1^) Release (default)
echo    2^) Debug
echo.
set /p "CFG=  Enter number [1]: "
if "!CFG!"=="2" set "CONFIG=Debug"

:config_done

echo.
echo  ═══════════════════════════════════════════════════════════
echo   Compiler:  %COMPILER%
echo   Config:    %CONFIG%
echo  ═══════════════════════════════════════════════════════════

REM ── Auto-detect SDL3 ──────────────────────────────────────────
if not "%SDL3_DIR%"=="" goto :sdl3_found

if exist "SDL3\cmake\SDL3\SDL3Config.cmake"               set "SDL3_DIR=%CD%\SDL3"
if exist "C:\SDL3\cmake\SDL3\SDL3Config.cmake"            set "SDL3_DIR=C:\SDL3"
if exist "C:\SDL3-%SDL3_VERSION%\cmake\SDL3\SDL3Config.cmake" set "SDL3_DIR=C:\SDL3-%SDL3_VERSION%"
if exist "C:\msys64\mingw64\lib\cmake\SDL3\SDL3Config.cmake" (
    if "%COMPILER%"=="mingw64" set "SDL3_DIR=C:\msys64\mingw64"
)
if exist "C:\msys64\mingw32\lib\cmake\SDL3\SDL3Config.cmake" (
    if "%COMPILER%"=="mingw32" set "SDL3_DIR=C:\msys64\mingw32"
)

if not "%SDL3_DIR%"=="" goto :sdl3_found

REM ── SDL3 not found — offer download ───────────────────────────
echo.
echo  SDL3 not found locally.
echo.
echo  Download from GitHub? [%SDL3_VERSION%]
echo    1^) Yes, download SDL3-%SDL3_VERSION%-win32-x64.zip
echo    2^) No, I'll provide the path manually
echo.
set /p "DL=  Enter number [1]: "
if "!DL!"=="2" goto :sdl3_manual

echo.
echo  Downloading SDL3 %SDL3_VERSION%...
set "SDL3_ZIP=%TEMP%\SDL3-%SDL3_VERSION%-win32-x64.zip"
set "SDL3_URL=https://github.com/libsdl-org/SDL/releases/download/release-%SDL3_VERSION%/SDL3-%SDL3_VERSION%-win32-x64.zip"

REM Try curl first (available on Windows 10+)
where curl >nul 2>&1
if !errorlevel!==0 (
    curl -L -o "%SDL3_ZIP%" "%SDL3_URL%"
) else (
    powershell -Command "Invoke-WebRequest -Uri '%SDL3_URL%' -OutFile '%SDL3_ZIP%'"
)
if errorlevel 1 (
    echo  [ERROR] Download failed.
    echo  Manual download: %SDL3_URL%
    pause
    exit /b 1
)

echo  Extracting to C:\SDL3-%SDL3_VERSION%...
powershell -Command "Expand-Archive -Path '%SDL3_ZIP%' -DestinationPath 'C:\' -Force"
if errorlevel 1 (
    echo  [ERROR] Extraction failed.
    pause
    exit /b 1
)

set "SDL3_DIR=C:\SDL3-%SDL3_VERSION%"
echo  SDL3 installed to %SDL3_DIR%
goto :sdl3_found

:sdl3_manual
echo.
set /p "SDL3_DIR=  Enter SDL3 path: "
if "%SDL3_DIR%"=="" (
    echo  [ERROR] No path provided.
    pause
    exit /b 1
)

:sdl3_found

REM Resolve cmake config path
set "SDL3_CMAKE=%SDL3_DIR%\cmake\SDL3"
if exist "%SDL3_DIR%\lib\cmake\SDL3\SDL3Config.cmake" set "SDL3_CMAKE=%SDL3_DIR%\lib\cmake\SDL3"

if not exist "%SDL3_CMAKE%\SDL3Config.cmake" (
    echo  [ERROR] SDL3Config.cmake not found in: %SDL3_CMAKE%
    echo          Make sure SDL3 development package is extracted correctly.
    pause
    exit /b 1
)

echo  SDL3:      %SDL3_DIR%
echo.

REM ══════════════════════════════════════════════════════════════
REM  Build
REM ══════════════════════════════════════════════════════════════

if "%COMPILER%"=="vs" goto :build_vs
if "%COMPILER%"=="mingw64" goto :build_mingw64
if "%COMPILER%"=="mingw32" goto :build_mingw32
echo  [ERROR] Unknown compiler: %COMPILER%
pause
exit /b 1

REM ──────────────────────────────────────────────────────────────
:build_vs
REM ──────────────────────────────────────────────────────────────
if not defined VCVARSALL (
    echo  [ERROR] Visual Studio vcvarsall.bat not found.
    pause
    exit /b 1
)

echo  [1/3] Setting up MSVC environment...
call "%VCVARSALL%" x64
if errorlevel 1 (
    echo  [ERROR] vcvarsall.bat failed.
    pause
    exit /b 1
)

echo  [2/3] Configuring...
cmake -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64 -DSDL3_DIR="%SDL3_CMAKE%" 2>nul
if errorlevel 1 (
    cmake -B %BUILD_DIR% -G "Visual Studio 16 2019" -A x64 -DSDL3_DIR="%SDL3_CMAKE%"
)
if errorlevel 1 (
    echo  [ERROR] CMake configure failed.
    pause
    exit /b 1
)

echo  [3/3] Building %CONFIG%...
cmake --build %BUILD_DIR% --config %CONFIG% -j
if errorlevel 1 (
    echo  [ERROR] Build failed.
    pause
    exit /b 1
)

set "OUTPUT=%BUILD_DIR%\%CONFIG%\calculator.exe"
if exist "%SDL3_DIR%\lib\x64\SDL3.dll" copy "%SDL3_DIR%\lib\x64\SDL3.dll" "%BUILD_DIR%\%CONFIG%\" >nul 2>&1
if exist "%SDL3_DIR%\SDL3.dll" copy "%SDL3_DIR%\SDL3.dll" "%BUILD_DIR%\%CONFIG%\" >nul 2>&1
goto :done

REM ──────────────────────────────────────────────────────────────
:build_mingw64
REM ──────────────────────────────────────────────────────────────
echo  [1/3] Configuring (MinGW-w64)...
cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL3_DIR="%SDL3_CMAKE%"
if errorlevel 1 (
    echo  [ERROR] CMake configure failed.
    pause
    exit /b 1
)

echo  [2/3] Building...
cmake --build %BUILD_DIR% -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo  [ERROR] Build failed.
    pause
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
echo  [1/3] Configuring (MinGW32)...
cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL3_DIR="%SDL3_CMAKE%"
if errorlevel 1 (
    echo  [ERROR] CMake configure failed.
    pause
    exit /b 1
)

echo  [2/3] Building...
cmake --build %BUILD_DIR% -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo  [ERROR] Build failed.
    pause
    exit /b 1
)

set "OUTPUT=%BUILD_DIR%\calculator.exe"
if exist "%SDL3_DIR%\bin\SDL3.dll" copy "%SDL3_DIR%\bin\SDL3.dll" "%BUILD_DIR%\" >nul 2>&1
goto :done

REM ──────────────────────────────────────────────────────────────
:done
REM ──────────────────────────────────────────────────────────────
echo.
echo  ══════════════════════════════════════════════════════════════
echo   BUILD SUCCESSFUL
echo  ══════════════════════════════════════════════════════════════
echo   Output:  %OUTPUT%
echo.
if exist "%BUILD_DIR%\SDL3.dll" echo   SDL3.dll: %BUILD_DIR%\SDL3.dll
echo.
echo   Run: place wardogs-data/ next to calculator.exe
echo  ══════════════════════════════════════════════════════════════
echo.
pause
goto :eof

REM ──────────────────────────────────────────────────────────────
:help
REM ──────────────────────────────────────────────────────────────
echo.
echo  WARDOGS Artillery Calculator — Build Script (SDL3)
echo.
echo  Interactive mode (double-click):
echo    build.bat
echo.
echo  Command-line mode:
echo    build.bat [COMPILER] [SDL3_PATH] [CONFIG]
echo.
echo    COMPILER:   vs, mingw64, mingw32
echo    SDL3_PATH:  path to SDL3 directory
echo    CONFIG:     Release or Debug
echo.
echo  Examples:
echo    build.bat
echo    build.bat vs
echo    build.bat vs C:\SDL3
echo    build.bat mingw64 C:\SDL3 Debug
echo.
pause