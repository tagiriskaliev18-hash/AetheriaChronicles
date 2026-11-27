@echo off
title Aetheria: Chronicles of the Drake (Unity 6.3 HDRP)
cd /d "%~dp0"
if not exist "Build\Aetheria.exe" (
    echo [ERROR] Build\Aetheria.exe not found.
    echo Run Unity build: Aetheria ^> 9. Build Windows Player
    pause
    exit /b 1
)
echo Launching Aetheria: Chronicles of the Drake ^(Unity 6.3 HDRP^)...
start "" "%~dp0Build\Aetheria.exe" %*
