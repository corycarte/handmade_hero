@echo off

if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
) else (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
)

set PATH=%~dp0;%PATH%
@echo on
