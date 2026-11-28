@echo off
REM Build the Aetheria world end to end: HDRP look -> real terrain + vegetation -> golden hour -> screenshots.
REM Run after Unity has been opened once and signed in (license activation is a one-time manual step).
setlocal enabledelayedexpansion
chcp 65001 >nul

set "PROJ=%~dp0"
if "%PROJ:~-1%"=="\" set "PROJ=%PROJ:~0,-1%"
set "UNITY=D:\Unity\Editors\6000.3.29f1\Editor\Unity.exe"
if not exist "%UNITY%" set "UNITY=C:\Program Files\Unity\Hub\Editor\6000.3.29f1\Editor\Unity.exe"
set "LOG=%PROJ%\Logs"
if not exist "%LOG%" mkdir "%LOG%"

if not exist "%UNITY%" (
  echo [ERROR] Unity not found at "%UNITY%"
  exit /b 1
)

echo [1/3] Building the world (terrain + vegetation + landmarks)...
"%UNITY%" -batchmode -nographics -quit -projectPath "%PROJ%" ^
  -executeMethod Aetheria.Editor.AetheriaWorldRebuild.RebuildWorld ^
  -logFile "%LOG%\world_rebuild.log"
if errorlevel 1 (
  echo [ERROR] World rebuild failed - see "%LOG%\world_rebuild.log"
  exit /b 1
)

echo [2/3] Upgrading the look and setting golden hour...
"%UNITY%" -batchmode -nographics -quit -projectPath "%PROJ%" ^
  -executeMethod Aetheria.Editor.AetheriaLookUpgrade.UpgradeLook ^
  -logFile "%LOG%\look.log"
"%UNITY%" -batchmode -nographics -quit -projectPath "%PROJ%" ^
  -executeMethod Aetheria.Editor.AetheriaLookUpgrade.SetGoldenHour ^
  -logFile "%LOG%\golden.log"

echo [3/3] Done. Open the project and press Play, or run:
echo   "%UNITY%" -projectPath "%PROJ%" -executeMethod Aetheria.Editor.AetheriaProjectSetup.BuildWindowsExecutable
exit /b 0
