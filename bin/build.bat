@echo off
echo "Starting build.."

set BUILD_DIR=%~dp0..\out

if not exist %BUILD_DIR% mkdir %BUILD_DIR

pushd %~dp0\..\out

@REM Build with various flags
@REM   -Zi: Generate complete debug information.

set WINMAIN_TARGET=handmade_win32.cpp

cl -Zi ..\src\%WINMAIN_TARGET% user32.lib

popd

echo "Done!"
@echo on