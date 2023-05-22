@echo off

@REM set output directory
set BUILD_DIR=%~dp0..\out
set DEBUG_TARGET=handmade_win32.exe

echo "Reminder: Use F11 to start debug session"
start devenv %BUILD_DIR%\%DEBUG_TARGET%

@echo on