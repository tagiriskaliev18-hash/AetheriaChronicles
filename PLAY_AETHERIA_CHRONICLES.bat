@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

set "PROJ_DIR=%~dp0"
if "%PROJ_DIR:~-1%"=="\" set "PROJ_DIR=%PROJ_DIR:~0,-1%"
set "UPROJECT=%PROJ_DIR%\AetheriaChronicles.uproject"

set "UE_ROOT="
for /f "usebackq delims=" %%I in (`powershell -NoProfile -Command "$f=\"$env:ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat\"; if(Test-Path $f){ $j=Get-Content $f -Raw | ConvertFrom-Json; $u=$j.InstallationList | Where-Object { $_.AppName -eq 'UE_5.8' } | Select-Object -First 1; if($u){ $u.InstallLocation } }"`) do set "UE_ROOT=%%I"
if not defined UE_ROOT if exist "%ProgramFiles%\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
if not defined UE_ROOT if exist "%USERPROFILE%\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%USERPROFILE%\Epic Games\UE_5.8"

if not defined UE_ROOT (
  echo [ERROR] Unreal Engine 5.8 not found.
  pause
  exit /b 1
)

set "UE-LocalDataCachePath=C:\UE5_Cache\DDC"
if not exist "C:\UE5_Cache\DDC" mkdir "C:\UE5_Cache\DDC"
if not exist "C:\UE5_Cache\Zen\Data" mkdir "C:\UE5_Cache\Zen\Data"
echo Launching Aetheria: Chronicles of the Drake (UE 5.8.4 — D3D12 SM6 RTX + NVIDIA DLSS 4.5)...
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%UPROJECT%" -game -d3d12 -sm6 -WindowedFullscreen
exit /b 0
