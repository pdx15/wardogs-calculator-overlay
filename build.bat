@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul 2>&1
title WARDOGS Artillery Calculator - Build

set "CONFIG=Release"
set "COMPILER="
set "SDL3_DIR="
set "BUILD_DIR=build"
set "SDL3_VERSION=3.4.18"

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
    if not "%COMPILER%"=="" goto :check_cmake
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

REM -- If compiler already set via args, skip menu -------------------
if not "%COMPILER%"=="" goto :compiler_done

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
echo   Compiler:  %COMPILER%
echo   Config:    %CONFIG%
echo  ============================================================

REM -- Auto-detect SDL3 ---------------------------------------------
if not "!SDL3_DIR!"=="" goto :sdl3_found

REM Search paths
for %%P in (
    "%CD%\SDL3"
    "C:\SDL3"
    "C:\SDL3-%SDL3_VERSION%"
) do (
    if exist "%%~P\cmake\SDL3\SDL3Config.cmake" (
        set "SDL3_DIR=%%~P"
        goto :sdl3_found
    )
    if exist "%%~P\lib\cmake\SDL3\SDL3Config.cmake" (
        set "SDL3_DIR=%%~P"
        goto :sdl3_found
    )
)

REM MSYS2
if "%COMPILER%"=="mingw64" (
    if exist "C:\msys64\mingw64\lib\cmake\SDL3\SDL3Config.cmake" (
        set "SDL3_DIR=C:\msys64\mingw64"
        goto :sdl3_found
    )
)
if "%COMPILER%"=="mingw32" (
    if exist "C:\msys64\mingw32\lib\cmake\SDL3\SDL3Config.cmake" (
        set "SDL3_DIR=C:\msys64\mingw32"
        goto :sdl3_found
    )
)

REM -- SDL3 not found - offer download ------------------------------
echo.
echo  SDL3 not found locally.
echo.
echo  Download from GitHub [%SDL3_VERSION%]?
echo    1^) Yes - download SDL3-%SDL3_VERSION%-win32-x64.zip
echo    2^) No - I'll provide the path manually
echo.
set /p "DL=  Enter number [1]: "
if "!DL!"=="2" goto :sdl3_manual

echo.
echo  Downloading SDL3 %SDL3_VERSION%...
set "SDL3_ZIP=%TEMP%\SDL3-%SDL3_VERSION%-win32-x64.zip"
set "SDL3_URL=https://github.com/libsdl-org/SDL/releases/download/release-%SDL3_VERSION%/SDL3-%SDL3_VERSION%-win32-x64.zip"

where curl >nul 2>&1
if not errorlevel 1 (
    curl -L -o "!SDL3_ZIP!" "!SDL3_URL!"
) else (
    powershell -Command "Invoke-WebRequest -Uri '!SDL3_URL!' -OutFile '!SDL3_ZIP!'"
)
if errorlevel 1 (
    echo.
    echo  [ERROR] Download failed. Try manually:
    echo  !SDL3_URL!
    echo.
    pause
    exit /b 1
)

echo  Extracting to C:\SDL3-%SDL3_VERSION%...
powershell -Command "Expand-Archive -Path '!SDL3_ZIP!' -DestinationPath 'C:\' -Force"
if errorlevel 1 (
    echo.
    echo  [ERROR] Extraction failed.
    pause
    exit /b 1
)

set "SDL3_DIR=C:\SDL3-%SDL3_VERSION%"
echo  Done: !SDL3_DIR!
goto :sdl3_found

:sdl3_manual
echo.
set /p "SDL3_DIR=  Enter SDL3 path: "
if "!SDL3_DIR!"=="" (
    echo  [ERROR] No path provided.
    pause
    exit /b 1
)

:sdl3_found

REM Resolve cmake config path
set "SDL3_CMAKE=!SDL3_DIR!\cmake\SDL3"
if exist "!SDL3_DIR!\lib\cmake\SDL3\SDL3Config.cmake" set "SDL3_CMAKE=!SDL3_DIR!\lib\cmake\SDL3"

if not exist "!SDL3_CMAKE!\SDL3Config.cmake" (
    echo.
    echo  [ERROR] SDL3Config.cmake not found in: !SDL3_CMAKE!
    pause
    exit /b 1
)

echo  SDL3:      !SDL3_DIR!
echo.

REM ================================================================
REM  Build
REM ================================================================

if "%COMPILER%"=="vs" goto :build_vs
if "%COMPILER%"=="mingw64" goto :build_mingw64
if "%COMPILER%"=="mingw32" goto :build_mingw32
echo  [ERROR] Unknown compiler: %COMPILER%
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

echo  [2/3] Configuring...
cmake -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64 -DSDL3_DIR="!SDL3_CMAKE!" 2>nul
if errorlevel 1 (
    cmake -B %BUILD_DIR% -G "Visual Studio 16 2019" -A x64 -DSDL3_DIR="!SDL3_CMAKE!"
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
if exist "!SDL3_DIR!\lib\x64\SDL3.dll" copy "!SDL3_DIR!\lib\x64\SDL3.dll" "%BUILD_DIR%\%CONFIG%\" >nul 2>&1
if exist "!SDL3_DIR!\SDL3.dll" copy "!SDL3_DIR!\SDL3.dll" "%BUILD_DIR%\%CONFIG%\" >nul 2>&1
goto :done

REM ---------------------------------------------------------------
:build_mingw64
REM ---------------------------------------------------------------
echo  [1/3] Configuring (MinGW-w64)...
cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL3_DIR="!SDL3_CMAKE!"
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
if exist "!SDL3_DIR!\bin\SDL3.dll"     copy "!SDL3_DIR!\bin\SDL3.dll"     "%BUILD_DIR%\" >nul 2>&1
if exist "!SDL3_DIR!\lib\x64\SDL3.dll" copy "!SDL3_DIR!\lib\x64\SDL3.dll" "%BUILD_DIR%\" >nul 2>&1
if exist "!SDL3_DIR!\SDL3.dll"         copy "!SDL3_DIR!\SDL3.dll"         "%BUILD_DIR%\" >nul 2>&1
goto :done

REM ---------------------------------------------------------------
:build_mingw32
REM ---------------------------------------------------------------
set "BUILD_DIR=build32"
echo  [1/3] Configuring (MinGW32)...
cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG% -DSDL3_DIR="!SDL3_CMAKE!"
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
if exist "!SDL3_DIR!\bin\SDL3.dll" copy "!SDL3_DIR!\bin\SDL3.dll" "%BUILD_DIR%\" >nul 2>&1
goto :done

REM ---------------------------------------------------------------
:done
REM ---------------------------------------------------------------
echo.
echo  ============================================================
echo   BUILD SUCCESSFUL
echo  ============================================================
echo   Output:  %OUTPUT%
echo.
if exist "%BUILD_DIR%\SDL3.dll" echo   SDL3.dll copied to build dir
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