@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul 2>&1
title WARDOGS Artillery Calculator - Build

set "CONFIG=Release"
set "COMPILER="
set "BUILD_DIR=build"

REM -- Parse arguments (non-interactive mode) -----------------------
if not "%~1"=="" (
    for %%A in (%*) do (
        if /i "%%A"=="vs"       set "COMPILER=vs"
        if /i "%%A"=="mingw64"  set "COMPILER=mingw64"
        if /i "%%A"=="mingw32"  set "COMPILER=mingw32"
        if /i "%%A"=="Debug"    set "CONFIG=Debug"
        if /i "%%A"=="Release"  set "CONFIG=Release"
        if /i "%%A"=="--help"   goto :help
        if /i "%%A"=="-h"       goto :help
    )
    if not "!COMPILER!"=="" goto :check_cmake
)

REM ================================================================
REM  Interactive mode
REM ================================================================

:check_cmake
where cmake >nul 2>&1
if errorlevel 1 (
    echo.
    echo  [ERROR] cmake not found in PATH.
    echo          Install: winget install Kitware.CMake
    echo          Or: choco install cmake
    echo          Or: https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

REM -- Detect compilers ---------------------------------------------
set "HAS_VS=0"
set "HAS_MINGW64=0"
set "HAS_MINGW32=0"
set "VCVARSALL="

REM Visual Studio
for %%V in (
    "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
) do (
    if exist "%%~V" (
        set "HAS_VS=1"
        if not defined VCVARSALL set "VCVARSALL=%%~V"
    )
)

REM MinGW in PATH
where g++ >nul 2>&1
if not errorlevel 1 (
    for /f "usebackq tokens=*" %%M in (`g++ -dumpmachine 2^>nul`) do (
        echo %%M | findstr /i "x86_64" >nul 2>&1
        if not errorlevel 1 set "HAS_MINGW64=1"
        echo %%M | findstr /i "i686 i386" >nul 2>&1
        if not errorlevel 1 set "HAS_MINGW32=1"
    )
)

REM MSYS2
if exist "C:\msys64\mingw64\bin\g++.exe" set "HAS_MINGW64=1"
if exist "C:\msys64\mingw32\bin\g++.exe" set "HAS_MINGW32=1"

REM Standalone MinGW
if exist "C:\mingw64\mingw64\bin\g++.exe" set "HAS_MINGW64=1"
if exist "C:\mingw64\bin\g++.exe"         set "HAS_MINGW64=1"
if exist "C:\mingw32\mingw32\bin\g++.exe" set "HAS_MINGW32=1"
if exist "C:\mingw32\bin\g++.exe"         set "HAS_MINGW32=1"
if exist "C:\mingw\bin\g++.exe"           set "HAS_MINGW64=1"
if exist "C:\MinGW\bin\g++.exe"           set "HAS_MINGW32=1"

REM -- If compiler already set via args, skip menu -------------------
if not "!COMPILER!"=="" goto :compiler_done

REM -- Compiler menu ------------------------------------------------
echo.
echo  ============================================================
echo            WARDOGS Artillery Calculator - Build
echo  ============================================================
echo    Detected compilers:
if "!HAS_VS!"=="1"      (echo    [x] Visual Studio MSVC)       else (echo    [ ] Visual Studio MSVC)
if "!HAS_MINGW64!"=="1" (echo    [x] MinGW-w64 64-bit)         else (echo    [ ] MinGW-w64 64-bit)
if "!HAS_MINGW32!"=="1" (echo    [x] MinGW32 32-bit)            else (echo    [ ] MinGW32 32-bit)
echo  ------------------------------------------------------------
echo    Select compiler:
echo.

set "IDX=0"
set "MAP_VS=0"
set "MAP_M64=0"
set "MAP_M32=0"

if "!HAS_VS!"=="1" (
    set /a IDX+=1
    echo      !IDX!^) Visual Studio
    set "MAP_VS=!IDX!"
)
if "!HAS_MINGW64!"=="1" (
    set /a IDX+=1
    echo      !IDX!^) MinGW-w64
    set "MAP_M64=!IDX!"
)
if "!HAS_MINGW32!"=="1" (
    set /a IDX+=1
    echo      !IDX!^) MinGW32
    set "MAP_M32=!IDX!"
)

if "!IDX!"=="0" (
    echo.
    echo    No compilers found!
    echo    Install Visual Studio or MSYS2 with MinGW.
    echo.
    pause
    exit /b 1
)

echo.
set /p "CHOICE=  Enter number: "

if "!MAP_VS!"=="!CHOICE!"   set "COMPILER=vs"
if "!MAP_M64!"=="!CHOICE!"  set "COMPILER=mingw64"
if "!MAP_M32!"=="!CHOICE!"  set "COMPILER=mingw32"

if "!COMPILER!"=="" (
    echo.
    echo  Invalid choice.
    pause
    exit /b 1
)

:compiler_done

REM -- Config menu (interactive only) --------------------------------
if not "%~1"=="" goto :config_done

echo.
echo  Build configuration:
echo    1^) Release
echo    2^) Debug
echo.
set /p "CFG=  Enter number [1]: "
if "!CFG!"=="2" set "CONFIG=Debug"

:config_done

echo.
echo  ============================================================
echo   Compiler:  !COMPILER!
echo   Config:    !CONFIG!
echo   SDL3:      built from source (static, no DLL needed)
echo  ============================================================
echo.

REM ================================================================
REM  Build (SDL3 is fetched and built from source by CMake)
REM ================================================================

if "!COMPILER!"=="vs" goto :build_vs
if "!COMPILER!"=="mingw64" goto :build_mingw64
if "!COMPILER!"=="mingw32" goto :build_mingw32
echo  [ERROR] Unknown compiler: !COMPILER!
pause
exit /b 1

REM ---------------------------------------------------------------
:build_vs
REM ---------------------------------------------------------------
if not defined VCVARSALL (
    echo  [ERROR] vcvarsall.bat not found.
    pause
    exit /b 1
)

echo  [1/3] Setting up MSVC...
call "!VCVARSALL!" x64
if errorlevel 1 (
    echo  [ERROR] vcvarsall.bat failed.
    pause
    exit /b 1
)

echo  [2/3] Configuring (SDL3 built from source)...
cmake -B !BUILD_DIR! -G "Visual Studio 18 2026" -A x64 --fresh 2>nul
if errorlevel 1 (
    cmake -B !BUILD_DIR! -G "Visual Studio 17 2022" -A x64 --fresh 2>nul
)
if errorlevel 1 (
    cmake -B !BUILD_DIR! -G "Visual Studio 16 2019" -A x64 --fresh
)
if errorlevel 1 (
    echo  [ERROR] CMake configure failed.
    pause
    exit /b 1
)

echo  [3/3] Building !CONFIG! (first build compiles SDL3)...
cmake --build !BUILD_DIR! --config !CONFIG! -j
if errorlevel 1 (
    echo  [ERROR] Build failed.
    pause
    exit /b 1
)

set "OUTPUT=!BUILD_DIR!\!CONFIG!\calculator.exe"
goto :done

REM ---------------------------------------------------------------
:build_mingw64
REM ---------------------------------------------------------------
if exist "C:\msys64\mingw64\bin\g++.exe"   set "PATH=C:\msys64\mingw64\bin;!PATH!"
if exist "C:\mingw64\mingw64\bin\g++.exe"  set "PATH=C:\mingw64\mingw64\bin;!PATH!"
if exist "C:\mingw64\bin\g++.exe"          set "PATH=C:\mingw64\bin;!PATH!"
if exist "C:\mingw\bin\g++.exe"            set "PATH=C:\mingw\bin;!PATH!"

echo  [1/3] Configuring (MinGW-w64, SDL3 built from source)...
cmake -B !BUILD_DIR! -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=!CONFIG! --fresh
if errorlevel 1 (
    echo  [ERROR] CMake configure failed.
    pause
    exit /b 1
)

echo  [2/3] Building (first build compiles SDL3)...
cmake --build !BUILD_DIR! -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo  [ERROR] Build failed.
    pause
    exit /b 1
)

set "OUTPUT=!BUILD_DIR!\calculator.exe"
goto :done

REM ---------------------------------------------------------------
:build_mingw32
REM ---------------------------------------------------------------
if exist "C:\msys64\mingw32\bin\g++.exe"   set "PATH=C:\msys64\mingw32\bin;!PATH!"
if exist "C:\mingw32\mingw32\bin\g++.exe"  set "PATH=C:\mingw32\mingw32\bin;!PATH!"
if exist "C:\mingw32\bin\g++.exe"          set "PATH=C:\mingw32\bin;!PATH!"
if exist "C:\MinGW\bin\g++.exe"            set "PATH=C:\MinGW\bin;!PATH!"

set "BUILD_DIR=build32"
echo  [1/3] Configuring (MinGW32, SDL3 built from source)...
cmake -B !BUILD_DIR! -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=!CONFIG! --fresh
if errorlevel 1 (
    echo  [ERROR] CMake configure failed.
    pause
    exit /b 1
)

echo  [2/3] Building (first build compiles SDL3)...
cmake --build !BUILD_DIR! -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo  [ERROR] Build failed.
    pause
    exit /b 1
)

set "OUTPUT=!BUILD_DIR!\calculator.exe"
goto :done

REM ---------------------------------------------------------------
:done
REM ---------------------------------------------------------------
echo.
echo  ============================================================
echo   BUILD SUCCESSFUL
echo  ============================================================
echo   Output:  !OUTPUT!
echo   DLL:     none (SDL3 statically linked)
echo.
echo   Run: place wardogs-data/ next to calculator.exe
echo  ============================================================
echo.
pause
goto :eof

REM ---------------------------------------------------------------
:help
REM ---------------------------------------------------------------
echo.
echo  WARDOGS Artillery Calculator - Build Script
echo.
echo  SDL3 is built from source (statically linked, no DLL).
echo  First build takes ~2 min to compile SDL3.
echo.
echo  Interactive mode (double-click):
echo    build.bat
echo.
echo  Command-line mode:
echo    build.bat [COMPILER] [CONFIG]
echo.
echo    COMPILER:   vs, mingw64, mingw32
echo    CONFIG:     Release or Debug
echo.
echo  Examples:
echo    build.bat
echo    build.bat vs
echo    build.bat mingw64 Debug
echo.
pause