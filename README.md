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

## Opening & Building Locally
1. Open the root folder in **Unity `6000.3.29f1`**.
2. Open `Assets/Scenes/AetheriaMain.unity`.
3. Rebuild via menu `Aetheria > 9. Build Windows Player` (or `Aetheria > 2. Build Windows Standalone (.exe)`), or batchmode:
   ```powershell
   "D:\Unity\Editors\6000.3.29f1\Editor\Unity.exe" -batchmode -quit -projectPath "." -buildTarget Win64 -executeMethod Aetheria.Editor.AetheriaProjectSetup.BuildWindowsExecutable
   ```

*(Generated caches `Library/`, `Temp/`, `Build/`, `Logs/`, `Captures/`, and `Tools/PacksInspect/` are excluded via `.gitignore`.)*
