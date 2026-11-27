# Aetheria: Chronicles of the Drake (Unity 6.3 HDRP)

Open-world fantasy action RPG built with **Unity 6.3 (`6000.3.29f1`)** and **High Definition Render Pipeline (`HDRP 17.3.0`)** targeting Windows x64 DirectX 12 with **NVIDIA DLSS** and **Hardware Ray Tracing (DXR)** support.

## Stack & Key Features
- **Engine / Pipeline:** Unity `6000.3.29f1`, `com.unity.render-pipelines.high-definition` `17.3.0`, `com.unity.modules.nvidia`, `com.unity.cloud.gltfast` `6.21.0`, `com.unity.inputsystem` `1.20.1`.
- **Main Scene:** `Assets/Scenes/AetheriaMain.unity`
- **Runtime Systems (`Assets/Scripts/`):**
  - `Combat/`: `HeroController`, `SpellSystem`, `SpellVfxExecutor`, `DragonController`, `EnemyAndBossAI`, `AetheriaNPC`, `CreatureAnimator`, `HeroAnimationOverlay`.
  - `Core/`: `AetheriaTypes`, `AetheriaRenderImpl`, `AetheriaFx`, `AetheriaCharacterLook`, `AetheriaMapTexture`, `AudioAndQualityService`, `HeroCustomizationService`, `QuestAndMapService`, `SkillTreeAndSaveService`.
  - `World/`: `AetheriaWorld`, `AetheriaWorldGenerator`, `AetheriaShrine`, `BreakableProp`.
  - `UI/`: `AetheriaHUDManager`, `AetheriaOverlays`.
- **Editor Tooling (`Assets/Editor/`):** `AetheriaProjectSetup.cs`, `AetheriaHdrpSetup.cs`, `AetheriaHdrpConvert.cs`, `AetheriaTerrainBuilder.cs`, `AetheriaHdrpShots.cs`.

## Unified Project Layout & Launchers
- **Unity 6.3 HDRP Game (root):** `Assets/`, `Packages/`, `ProjectSettings/`, `Build\Aetheria.exe`, `REBUILD_WORLD.bat`
- **Web / Electron Edition (`FantasyWeb\`):** Three.js open-world & Electron app (`src/`, `electron/`, `assets/`, `steam/`, `build.mjs`, `package.json`, `PLAY.bat`)
- **Quick Launchers (root):**
  - `PLAY_UNITY.bat` — launches `Build\Aetheria.exe` (Unity 6.3 HDRP Windows build)
  - `PLAY_WEB.bat` — launches `FantasyWeb\PLAY.bat` (local HTTP server on `http://localhost:8080`, building `dist-desktop` if needed)

## Opening & Building Locally
1. Open the root folder in **Unity `6000.3.29f1`**.
2. Open `Assets/Scenes/AetheriaMain.unity`.
3. Rebuild via menu `Aetheria > 9. Build Windows Player` (or `Aetheria > 2. Build Windows Standalone (.exe)`), or batchmode:
   ```powershell
   & "D:\Unity\Editors\6000.3.29f1\Editor\Unity.exe" -batchmode -quit -projectPath (Get-Location).Path -buildTarget Win64 -executeMethod Aetheria.Editor.AetheriaProjectSetup.BuildWindowsExecutable
   ```

The Windows build prepares the animation controllers and the bridge collision in the main scene. Launch `Build/Aetheria.exe` (or run `PLAY_UNITY.bat`) with the rest of the `Build` folder alongside it. Choose **New Game** for the character creator and prologue. After speaking to Weylan, follow the quest marker to the cliff and then meet Elder Aldric in Oakhaven.

To build or run the Web / Electron version inside `FantasyWeb\`:
```powershell
cd FantasyWeb
node build.mjs
# or: npm run desktop
```

The Windows player has a regression check (`Assets/Scripts/World/AetheriaDemoVerification.cs`) for the prologue, the terrain collider, recovery after a fall, and Outrider attack animation. It writes its results to the player log and exits.

```powershell
Start-Process ./Build/Aetheria.exe -ArgumentList '-verify-demo','-logFile','demo-verification.log' -Wait -PassThru
```

*(Generated caches `Library/`, `Temp/`, `Build/`, `Logs/`, `Captures/`, `Tools/PacksInspect/`, `FantasyWeb/node_modules/`, `FantasyWeb/release/`, `FantasyWeb/dist-desktop/`, `.zcode/`, and `demo-verification.log` are excluded via `.gitignore`.)*
