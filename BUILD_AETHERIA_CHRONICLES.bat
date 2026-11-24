@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

set "PROJ_DIR=%~dp0"
if "%PROJ_DIR:~-1%"=="\" set "PROJ_DIR=%PROJ_DIR:~0,-1%"
set "UPROJECT=%PROJ_DIR%\AetheriaChronicles.uproject"

REM Auto-detect UE 5.8 installation from LauncherInstalled.dat (never hardcode user paths)
set "UE_ROOT="
for /f "usebackq delims=" %%I in (`powershell -NoProfile -Command "$f=\"$env:ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat\"; if(Test-Path $f){ $j=Get-Content $f -Raw | ConvertFrom-Json; $u=$j.InstallationList | Where-Object { $_.AppName -eq 'UE_5.8' } | Select-Object -First 1; if($u){ $u.InstallLocation } }"`) do set "UE_ROOT=%%I"
if not defined UE_ROOT if exist "%ProgramFiles%\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"

if not defined UE_ROOT (
  echo [ERROR] Unreal Engine 5.8 not found.
  exit /b 1
)

set "UE-LocalDataCachePath=D:\UE5_DDC"
if not exist "D:\UE5_DDC" mkdir "D:\UE5_DDC"
if not exist "D:\UE5_UBA" mkdir "D:\UE5_UBA"
if not exist "D:\UE5_Zen\Data" mkdir "D:\UE5_Zen\Data"
echo [INFO] Engine : "%UE_ROOT%"
echo [INFO] Project: "%UPROJECT%"
echo [INFO] DDC    : "D:\UE5_DDC"
echo [INFO] Zen    : "D:\UE5_Zen\Data"
echo [INFO] UBA    : "D:\UE5_UBA"
echo [INFO] Building AetheriaChroniclesEditor (Win64 Development, 3-worker balanced mode for 16GB)...

call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" AetheriaChroniclesEditor Win64 Development -Project="%UPROJECT%" -WaitMutex -MaxParallelActions=3 -NoHotReloadFromIDE -NoUBA -UBARootDir="D:\UE5_UBA"
if errorlevel 1 (
  echo [ERROR] Build failed!
  exit /b 1
)

echo [SUCCESS] AetheriaChroniclesEditor compiled cleanly!
exit /b 0
