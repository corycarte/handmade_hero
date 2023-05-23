@echo off
echo Starting build...

set BUILD_DIR=%~dp0\..\out

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

pushd %~dp0\..\out
set WINMAIN_TARGET=handmade_win32.cpp
set LIBRARIES=user32.lib Gdi32.lib

@REM Build with various flags
@REM   -Zi: Generate complete debug information.

cl -Zi ..\src\%WINMAIN_TARGET% %LIBRARIES%

if %ERRORLEVEL% == 0 ( 
    goto SUCCESS
) else (
    goto FAIL
)

:SUCCESS
    echo Compilation complete
    goto end

:FAIL
    echo Failure during compilation!
    goto end

:end
    popd
    @echo on
