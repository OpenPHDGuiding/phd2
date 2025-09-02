@echo off
REM Windows build script for PHD2
REM This script builds PHD2 on Windows systems using Visual Studio

setlocal enabledelayedexpansion

REM Default values
set BUILD_TYPE=Release
set BUILD_DIR=build
set CLEAN_BUILD=false
set GENERATOR=Visual Studio 17 2022
set PLATFORM=Win32
set OPENSOURCE_ONLY=false

REM Parse command line arguments
:parse_args
if "%~1"=="" goto end_parse
if "%~1"=="-d" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if "%~1"=="-c" (
    set CLEAN_BUILD=true
    shift
    goto parse_args
)
if "%~1"=="--clean" (
    set CLEAN_BUILD=true
    shift
    goto parse_args
)
if "%~1"=="-x64" (
    set PLATFORM=x64
    shift
    goto parse_args
)
if "%~1"=="-o" (
    set OPENSOURCE_ONLY=true
    shift
    goto parse_args
)
if "%~1"=="--opensource" (
    set OPENSOURCE_ONLY=true
    shift
    goto parse_args
)
if "%~1"=="-h" goto show_help
if "%~1"=="--help" goto show_help
echo Unknown option: %~1
goto show_help

:end_parse

echo PHD2 Windows Build Script
echo =========================
echo Build type: %BUILD_TYPE%
echo Build directory: %BUILD_DIR%
echo Generator: %GENERATOR%
echo Platform: %PLATFORM%
echo Opensource only: %OPENSOURCE_ONLY%
echo Clean build: %CLEAN_BUILD%
echo.

REM Clean build if requested
if "%CLEAN_BUILD%"=="true" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure CMake
echo Configuring CMake...
set CMAKE_ARGS=-G "%GENERATOR%" -A %PLATFORM%

if "%OPENSOURCE_ONLY%"=="true" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DOPENSOURCE_ONLY=ON
)

cmake %CMAKE_ARGS% ..
if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo Building PHD2...
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable: %BUILD_DIR%\%BUILD_TYPE%\phd2.exe
goto end

:show_help
echo Usage: %0 [OPTIONS]
echo Options:
echo   -d, --debug           Build debug version
echo   -c, --clean           Clean build (remove build directory first)
echo   -x64                  Build for x64 platform (default: Win32)
echo   -o, --opensource      Build with opensource drivers only
echo   -h, --help            Show this help message

:end
