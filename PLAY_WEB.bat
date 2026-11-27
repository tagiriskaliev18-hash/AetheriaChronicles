@echo off
title Aethelgard: Ashes of the Drake (Web / Three.js)
cd /d "%~dp0"
if not exist "FantasyWeb\PLAY.bat" (
    echo [ERROR] FantasyWeb\PLAY.bat not found.
    pause
    exit /b 1
)
if not exist "FantasyWeb\dist-desktop\index.html" (
    echo Building FantasyWeb desktop bundle...
    pushd "%~dp0FantasyWeb"
    call npm run build:desktop
    popd
)
call "%~dp0FantasyWeb\PLAY.bat"
