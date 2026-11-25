# HDRP 17.3.0 (Unity 6000.3.29f1) API research report

Status: **partial** (exploration was stopped by a time-box). Everything not explicitly marked **NOT VERIFIED** was read from the on-disk sources listed below.

Path legend (relative to `D:\Unity\Editors\6000.3.29f1\Editor\Data\Resources\PackageManager\BuiltInPackages\`):

- `HDRP/` = `com.unity.render-pipelines.high-definition/`
- `CORE/` = `com.unity.render-pipelines.core/`
- `ENGINE/` = `D:\Unity\Editors\6000.3.29f1\Editor\Data\Managed\UnityEngine\` (engine DLLs: read-only Mono.Cecil metadata dump + `*.xml` docs; no engine code was executed)

How engine-level API visibility was checked: member accessibility of `UnityEngine.CoreModule.dll` and `UnityEditor.CoreModule.dll` was read with `Unity.Cecil.dll` (metadata only). XML docs alone were not trusted because they also list internal members.

Verified versions (`package.json`): HDRP 17.3.0, Core 17.3.0, HDRP-config 17.3.0, shadergraph 17.3.0, visualeffectgraph 17.3.0, URP 17.3.0; HDRP `"unity": "6000.3"`.

---

## 0. Blockers and "does not exist in this version" (read this first)

| Item | Finding | Source |
|---|---|---|
| `HDRenderPipelineGlobalSettings` | **internal** class (all 3 partial declarations have no access modifier). `Ensure(bool canCreateNewAsset = true)`, `instance`, `defaultPath` are internal. Not nameable from project scripts. | `HDRP/Runtime/RenderPipeline/HDRenderPipelineGlobalSettings.cs:28,49,52`, `.Migration.cs:11`, `HDRP/Runtime/Deprecated.cs:770` |
| `HDRenderPipelineAssetFactory` / `Create<>()` | **Does not exist.** The editor factory is `HDAssetFactory` (internal static partial class) with only a menu item that does `CreateInstance` + `AssetDatabase.CreateAsset`. | `HDRP/Editor/RenderPipeline/HDAssetFactory.cs:10-29` |
| `HDProjectSettings` | Exists but is **internal** and is NOT the graphics-settings container: it only stores material/ShaderGraph upgrade versions and `projectSettingsFolderPath`. No `volumeProfile`, `lookDevVolumeProfile`, `defaultVolumeProfile`, `renderingPathDefaultCameraFrameSettings`. | `HDRP/Editor/RenderPipelineResources/HDProjectSettings.cs:14`, `HDRP/Runtime/RenderPipelineResources/HDProjectSettingsReadOnlyBase.cs:13-20` |
| `RenderingPathFrameSettings` | **internal** class; `GetDefaultFrameSettings()` is `internal ref`. Default Frame Settings cannot be changed by public API (docs confirm). | `HDRP/Runtime/Settings/RenderingPathFrameSettings.cs:11,36`; `HDRP/Documentation~/Frame-Settings-API.md` |
| `HDRenderPipelineEditorAssets`, `HDRenderPipelineEditorMaterials`, `HDRPRayTracingResources`, `VolumeUtils` | all **internal** (`VolumeUtils` is `internal static class`) | `HDRP/Runtime/Settings/HDRenderPipelineEditorAssets.cs:9`, `HDRenderPipelineEditorMaterials.cs:9`, `HDRPRayTracingResources.cs:8`, `HDRP/Runtime/Utilities/VolumeUtils.cs:9` |
| `HDRenderPipeline.currentAsset / currentPipeline / isReady / pipelineSupportsRayTracing / assetSupportsRayTracing` | all **internal static** | `HDRP/Runtime/RenderPipeline/HDRenderPipeline.cs:50-61` |
| NVIDIA module | Project manifest has **no** `com.unity.modules.nvidia`, so `ENABLE_NVIDIA_MODULE` is undefined and HDRP's DLSS pass is compiled to stubs. | `D:\AetheriaUnity\Packages\manifest.json`; `HDRP/Runtime/Unity.RenderPipelines.HighDefinition.Runtime.asmdef` (versionDefines) |
| `ENABLE_UPSCALER_FRAMEWORK` | Not defined in this project (IUpscaler path incl. "DLSS (IUpscaler)" is unavailable). | compile rsp + compiled DLL check, see section 2 |
| `UpscalerSelection` / `UpscalerFlags` / `UpscalerSettings` / `us1` | Not present in `GlobalDynamicResolutionSettings` or `RenderPipelineSettings` (both files read in full). Package-wide symbol search **NOT VERIFIED**. | `CORE/Runtime/Common/GlobalDynamicResolutionSettings.cs`, `HDRP/Runtime/RenderPipeline/Settings/RenderPipelineSettings.cs` |
| `HDAdditionalCameraData.exposure`, `motionVectorSupportMode`, `allowMotionVectorForMotionBlur` | Do not exist in HDRP Runtime or Core Runtime (grep: zero hits). | grep over `HDRP/Runtime`, `CORE/Runtime` |
| `useCameraDepthMismatch` | Does not exist (grep over `.cs`/`.md`: zero hits) | HDRP package |
| `supportRayTracingNewShaderGenerator`, `RTXDI`, `LightBinning` | Do not exist anywhere in the HDRP package (grep incl. `.hlsl`/`.shader`/`.md`: zero hits) | HDRP package |
| `[RenderPipelineGlobalSettingsAttr]` | Not present; the attribute used is `[SupportedOnRenderPipeline(typeof(HDRenderPipelineAsset))]` | `ENGINE/UnityEngine.CoreModule.xml` (no hits), `HDRP/Runtime/RenderPipeline/HDRenderPipelineGlobalSettings.cs:26` |

---

## 1. Programmatic HDRP setup from an Editor script

### 1.1 Creating the HDRP asset

Class: `UnityEngine.Rendering.HighDefinition.HDRenderPipelineAsset` (`public partial class HDRenderPipelineAsset : RenderPipelineAsset<HDRenderPipeline>, IVirtualTexturingEnabledRenderPipeline, IProbeVolumeEnabledRenderPipeline, IGPUResidentRenderPipeline, IRenderGraphEnabledRenderPipeline, ISTPEnabledRenderPipeline`). Source: `HDRP/Runtime/RenderPipeline/HDRenderPipelineAsset.cs:25`.

- Constructor is `HDRenderPipelineAsset() { }` (private, empty) -> created with `ScriptableObject.CreateInstance<HDRenderPipelineAsset>()`. Lines 40-42.
- `[SerializeField, FormerlySerializedAs("renderPipelineSettings")] RenderPipelineSettings m_RenderPipelineSettings = RenderPipelineSettings.NewDefault();` (line 134). There is **no** `m_FrameSettings`, `m_PipelineDebugLevel` or similar field on the asset in this version.
- `public RenderPipelineSettings currentPlatformRenderPipelineSettings { get => m_RenderPipelineSettings; set { m_RenderPipelineSettings = value; OnValidate(); } }` (line 140). Struct copy semantics: modify a local copy and assign back.
- `OnEnable()` (44-58): calls `Migrate()`, sets `dynamicResolutionSettings.lowResVolumetricCloudsMinimumThreshold = 50` if 0, calls `HDDynamicResolutionPlatformCapabilities.SetupFeatures()`.
- `Reset()` (60-67, editor only): `HDRenderPipelineGlobalSettings.Ensure(canCreateNewAsset: true); OnValidate();`
- `protected override void EnsureGlobalSettings()` (72-79): `base.EnsureGlobalSettings(); HDRenderPipelineGlobalSettings.Ensure();` (engine base member `RenderPipelineAsset.EnsureGlobalSettings()` is `protected virtual`, verified in `ENGINE/UnityEngine.CoreModule.dll`; who calls it is native code, **NOT VERIFIED** from source).
- `protected override void OnValidate()` (97-109): only calls `base.OnValidate()` when `GraphicsSettings.currentRenderPipeline == this`.
- `protected override bool requiresCompatibleRenderPipelineGlobalSettings => true;` (35), `public override string renderPipelineShaderTag => HDRenderPipeline.k_ShaderTagName;` (= `"HDRenderPipeline"`, `HDRP/Runtime/RenderPipeline/HDRenderPipeline.cs:70`).
- `public VolumeProfile volumeProfile { get; set; }` (173): per-asset profile that overrides the global default volume profile.
- `internal bool enableSRPBatcher = true;` (internal field), `public MaterialQuality availableMaterialQualityLevels`, `public MaterialQuality defaultMaterialQualityLevel { get; }`, `public bool isStpUsed { get; }`, `public bool supportProbeVolume { get; }`, `public ProbeVolumeSHBands maxSHBands { get; }`, `public GPUResidentDrawerMode gpuResidentDrawerMode { get; set; }`, `public bool virtualTexturingEnabled { get; }`.
- Editor-only (`HDRP/Runtime/RenderPipeline/HDRenderPipelineAsset.DefaultResources.cs:7-24`): `public override Material defaultMaterial`, `public Material GetDefaultDecalMaterial()`, `public Material GetDefaultMirrorMaterial()`, `public override Material defaultParticleMaterial`, `public override Material defaultTerrainMaterial`, `public override Shader defaultShader` (line 31).

What HDRP's own editor does (`HDRP/Editor/RenderPipeline/HDAssetFactory.cs:12-22`):

```csharp
var newAsset = CreateInstance<HDRenderPipelineAsset>();
newAsset.name = Path.GetFileName(pathName);
AssetDatabase.CreateAsset(newAsset, pathName);
```

No post-creation initialisation is performed by the factory.

What the HDRP Wizard does (`HDRP/Editor/Wizard/HDWizard.UIElement.cs:122-150`):

```csharp
if (!AssetDatabase.IsValidFolder("Assets/" + HDProjectSettings.projectSettingsFolderPath))
    AssetDatabase.CreateFolder("Assets", HDProjectSettings.projectSettingsFolderPath);   // default "HDRPDefaultResources"
var asset = ScriptableObject.CreateInstance<T>();            // T = HDRenderPipelineAsset
asset.name = typeof(T).Name;
AssetDatabase.CreateAsset(asset, "Assets/" + HDProjectSettings.projectSettingsFolderPath + "/" + asset.name + ".asset");
AssetDatabase.SaveAssets();
AssetDatabase.Refresh();
GraphicsSettings.defaultRenderPipeline = asset as HDRenderPipelineAsset;
```

Default folder name: `HDProjectSettingsReadOnlyBase.m_ProjectSettingFolderPath = "HDRPDefaultResources"` (`HDRP/Runtime/RenderPipelineResources/HDProjectSettingsReadOnlyBase.cs:18`); singleton file `ProjectSettings/HDRPProjectSettings.asset` (line 15).

### 1.2 Global Settings: registration and creation

- Class declaration and attributes: `HDRP/Runtime/RenderPipeline/HDRenderPipelineGlobalSettings.cs:24-28`:
  `[HDRPHelpURL("Default-Settings-Window")] [DisplayInfo(name = "HDRP Global Settings Asset", order = ...)] [SupportedOnRenderPipeline(typeof(HDRenderPipelineAsset))] [DisplayName("HDRP")] partial class HDRenderPipelineGlobalSettings : RenderPipelineGlobalSettings<HDRenderPipelineGlobalSettings, HDRenderPipeline>`.
- Data: `[SerializeField] RenderPipelineGraphicsSettingsContainer m_Settings = new();` (30); `protected override List<IRenderPipelineGraphicsSettings> settingsList => m_Settings.settingsList;` (31).
- `internal static string defaultPath => $"Assets/{HDProjectSettingsReadOnlyBase.projectSettingsFolderPath}/HDRenderPipelineGlobalSettings.asset";` (49).
- `internal static HDRenderPipelineGlobalSettings Ensure(bool canCreateNewAsset = true)` (52-88): reads `GraphicsSettings.GetSettingsForRenderPipeline<HDRenderPipeline>() as HDRenderPipelineGlobalSettings`, then `RenderPipelineGlobalSettingsUtils.TryEnsure<HDRenderPipelineGlobalSettings, HDRenderPipeline>(ref currentInstance, defaultPath, canCreateNewAsset)`, migrates if needed.
- `public override void Initialize(RenderPipelineGlobalSettings source = null)` (90-95): `SetUpRPAssetIncluded(); InitializeDefaultVolumeProfile();`
- `internal HDRPDefaultVolumeProfileSettings InitializeDefaultVolumeProfile()` (96-128): if `HDRPDefaultVolumeProfileSettings.volumeProfile == null` sets it to `VolumeUtils.CopyVolumeProfileFromResourcesToAssets(assets.defaultVolumeProfile)` and fills the diffusion profile list.
- **Automatic creation** (`HDRP/Editor/AssetProcessors/HDRenderPipelineGlobalSettingsPostprocessor.cs:8-16`):

```csharp
static void OnPostprocessAllAssets(string[] importedAssets, string[] __, string[] ___, string[] ____, bool didDomainReload)
{
    if (didDomainReload || importedAssets.Contains("ProjectSettings/GraphicsSettings.asset"))
        HDRenderPipelineGlobalSettings.Ensure();
}
```

  So assigning an HDRP asset to `GraphicsSettings.defaultRenderPipeline`, saving, and letting the domain reload / GraphicsSettings import happen creates `Assets/HDRPDefaultResources/HDRenderPipelineGlobalSettings.asset`. Runtime timing **NOT VERIFIED** (source call chain only).
- Build time also enforces it: `HDRPBuildDataValidator.IsProjectValidForBuilding` calls `HDRenderPipelineGlobalSettings.Ensure()` and `HDRPPreprocessBuild.OnPreprocessBuild` throws `BuildFailedException` if the default volume profile cannot be initialised (`HDRP/Editor/BuildProcessors/HDRPPreprocessBuild.cs:39-52`, `HDRPBuildDataValidator.cs:46-68`).

Public Core helper (editor only, namespace `UnityEngine.Rendering`), `CORE/Runtime/RenderPipeline/RenderPipelineGlobalSettingsUtils.cs`:

- `public static class RenderPipelineGlobalSettingsUtils` (13)
- `public static TGlobalSetting Create<TGlobalSetting>(string path, TGlobalSetting dataSource = null)` (22)
- `public static RenderPipelineGlobalSettings Create(Type renderPipelineGlobalSettingsType, string path, RenderPipelineGlobalSettings dataSource = null)` (35-70): ensures folders, `GenerateUniqueAssetPath`, `ScriptableObject.CreateInstance(type)`, `AssetDatabase.CreateAsset`, `EditorGraphicsSettings.PopulateRenderPipelineGraphicsSettings(assetCreated)`, `assetCreated.Initialize(dataSource)`, save + refresh. It does **not** register the asset.
- `public static bool TryEnsure<TGlobalSetting, TRenderPipeline>(ref TGlobalSetting instance, string defaultPath = "", bool canCreateNewAsset = true)` (86-97): loads from `defaultPath`, else `CoreUtils.LoadAllAssets<TGlobalSetting>().FirstOrDefault()`, else creates; always ends with `EditorGraphicsSettings.SetRenderPipelineGlobalSettingsAsset<TRenderPipeline>(instance)` (138).

Base class (public): `CORE/Runtime/RenderPipeline/RenderPipelineGlobalSettings.cs:14` `public abstract class RenderPipelineGlobalSettings<TGlobalRenderPipelineSettings, TRenderPipeline> : RenderPipelineGlobalSettings` with `public static TGlobalRenderPipelineSettings instance` (editor: `EditorGraphicsSettings.GetRenderPipelineGlobalSettingsAsset<TRenderPipeline>() as TGlobalRenderPipelineSettings`) and `public virtual void Reset()`.

Engine API (verified in DLL metadata; namespace `UnityEngine.Rendering` unless noted):

- `public static class GraphicsSettings`: `defaultRenderPipeline { get; set; }`, `renderPipelineAsset { get; set; }` (XML: "Deprecated, use GraphicsSettings.defaultRenderPipeline instead"), `currentRenderPipeline { get; }`, `allConfiguredRenderPipelines { get; }`, `RegisterRenderPipelineSettings(Type renderPipelineType, RenderPipelineGlobalSettings settings)`, `RegisterRenderPipelineSettings<T>(RenderPipelineGlobalSettings settings)`, `UnregisterRenderPipelineSettings(Type)` / `<T>()`, `GetSettingsForRenderPipeline(Type)` / `<T>() : RenderPipelineGlobalSettings`, `GetRenderPipelineSettings<T>() : T`, `TryGetRenderPipelineSettings<T>(out T settings) : bool`, `TryGetCurrentRenderPipelineGlobalSettings(out RenderPipelineGlobalSettings) : bool`, `TrySetCurrentRenderPipelineGlobalSettings(RenderPipelineGlobalSettings) : bool`.
- `public abstract class RenderPipelineGlobalSettings`: `public virtual void Initialize(RenderPipelineGlobalSettings source)`; `Add`, `Remove`, `TryGet`, `Contains`, `TryGetFirstSettingsImplementingInterface<>`, `GetSettingsImplementingInterface<>` are **protected internal** (not callable from project code on a base-typed reference); `settingsList { get; protected }`.
- `UnityEditor.Rendering.EditorGraphicsSettings` (public static): `SetRenderPipelineGlobalSettingsAsset(Type renderPipelineType, RenderPipelineGlobalSettings newSettings)`, `SetRenderPipelineGlobalSettingsAsset<T>(RenderPipelineGlobalSettings)`, `GetRenderPipelineGlobalSettingsAsset(Type)` / `<T>()`, `TryGetRenderPipelineSettingsForPipeline<TSettings, TPipeline>(out TSettings settings)`, `TryGetRenderPipelineSettingsForPipeline<TSettings>(Type renderPipelineType, out TSettings settings)`, `GetSupportedRenderPipelineGraphicsSettingsTypesForPipeline<T>()`, `PopulateRenderPipelineGraphicsSettings(RenderPipelineGlobalSettings settings)`.
- `UnityEditor.Rendering.RenderPipelineGraphicsSettingsEditorUtility` (public static): `Rebind(IRenderPipelineGraphicsSettings settings, Type renderPipelineType)`, `RecreateRenderPipelineMatchingSettings(IRenderPipelineGraphicsSettings settings)`, `RemoveRenderPipelineGraphicsSettingsWithMissingScript()`.
- `UnityEngine.Rendering.RenderPipelineGraphicsSettingsExtensions.SetValueAndNotify<T>(this IRenderPipelineGraphicsSettings settings, ref T currentPropertyValue, T newValue, string propertyName)` (public static).
- `UnityEngine.Rendering.SupportedOnRenderPipelineAttribute(Type renderPipeline)` / `(Type[] renderPipeline)`; `static IsTypeSupportedOnRenderPipeline(Type type, Type renderPipelineAssetType)`.
- `UnityEngine.Rendering.IRenderPipelineGraphicsSettings`: `int version`, `bool isAvailableInPlayerBuild`, `void Reset()`.

HDRP's own `Ensure` via reflection (fallback if the automatic path is not relied on):

```csharp
var t = typeof(HDRenderPipeline).Assembly.GetType("UnityEngine.Rendering.HighDefinition.HDRenderPipelineGlobalSettings", true);
t.GetMethod("Ensure", BindingFlags.Static | BindingFlags.NonPublic).Invoke(null, new object[] { true });
```

Reflection-free alternative (public APIs only, `Type` obtained by reflection):

```csharp
var gs = RenderPipelineGlobalSettingsUtils.Create(t, "Assets/HDRPDefaultResources/HDRenderPipelineGlobalSettings.asset");
EditorGraphicsSettings.SetRenderPipelineGlobalSettingsAsset(typeof(HDRenderPipeline), gs);
```

### 1.3 Graphics-settings containers that replaced the old HDProjectSettings fields

| Old-style field | Actual class (all `UnityEngine.Rendering.HighDefinition`) | Member | Access |
|---|---|---|---|
| default volume profile | `HDRPDefaultVolumeProfileSettings : IDefaultVolumeProfileSettings` (`HDRP/Runtime/Settings/HDRPDefaultVolumeProfileSetting.cs:42`) | `public VolumeProfile volumeProfile { get; set /* SetValueAndNotify */ }` | public |
| lookDev volume profile | `LookDevVolumeProfileSettings : IRenderPipelineGraphicsSettings` (`HDRP/Runtime/Settings/LookDevVolumeProfileSettings.cs:42`) | `public VolumeProfile volumeProfile { get; set }` | public |
| default frame settings | `RenderingPathFrameSettings` (`HDRP/Runtime/Settings/RenderingPathFrameSettings.cs:11`) | `m_Camera`, `m_CustomOrBakedReflection`, `m_RealtimeReflection`; `internal ref FrameSettings GetDefaultFrameSettings(FrameSettingsRenderType type)` | **internal** |
| package default profiles | `HDRenderPipelineEditorAssets` (`HDRP/Runtime/Settings/HDRenderPipelineEditorAssets.cs:9`) | `defaultVolumeProfile`, `lookDevVolumeProfile`, `defaultDiffusionProfileSettingsList`; ResourcePaths `Editor/RenderPipelineResources/DefaultSettingsVolumeProfile.asset`, `Editor/RenderPipelineResources/DefaultLookDevProfile.asset` | **internal** |
| default materials | `HDRenderPipelineEditorMaterials` (`HDRP/Runtime/Settings/HDRenderPipelineEditorMaterials.cs:9`) | `defaultMaterial`, `defaultParticleMaterial`, `defaultTerrainMaterial`, `defaultMirrorMaterial`, `defaultDecalMaterial`, `GUITextureBlit2SRGB`; ResourcePaths `Runtime/RenderPipelineResources/Material/DefaultHDMaterial.mat`, `DefaultHDParticleMaterial.mat`, `DefaultHDTerrainMaterial.mat`, `DefaultHDMirrorMaterial.mat`, `DefaultHDDecalMaterial.mat` | **internal** |

Access pattern used by HDRP editor code: `GraphicsSettings.GetRenderPipelineSettings<HDRPDefaultVolumeProfileSettings>()`, `GraphicsSettings.TryGetRenderPipelineSettings<T>(out var settings)`, `EditorGraphicsSettings.TryGetRenderPipelineSettingsForPipeline<TSettings, TPipeline>(out var rpgs)` (`HDRP/Editor/Wizard/HDWizard.Configuration.cs:527-576`, `HDRP/Runtime/Settings/HDRPDefaultVolumeProfileSetting.cs:78`).

Wizard fix for the default volume profile (`HDRP/Editor/Wizard/HDWizard.Configuration.cs:570-583`):

```csharp
var defaultVolumeProfileSettings = GraphicsSettings.GetRenderPipelineSettings<HDRPDefaultVolumeProfileSettings>();
var defaultValuesAsset = GraphicsSettings.GetRenderPipelineSettings<HDRenderPipelineEditorAssets>().defaultVolumeProfile;
var volumeProfileCopy = VolumeUtils.CopyVolumeProfileFromResourcesToAssets(defaultValuesAsset);   // VolumeUtils is internal
defaultVolumeProfileSettings.volumeProfile = volumeProfileCopy;
EditorUtility.SetDirty(HDRenderPipelineGlobalSettings.instance);
if (VolumeManager.instance.isInitialized)
    VolumeManager.instance.SetGlobalDefaultProfile(volumeProfileCopy);
```

### 1.4 Default render pipeline vs quality levels

- HDRP Wizard (`HDRP/Editor/Wizard/HDWizard.Configuration.cs`):
  - Global check: `IsHdrpAssetGraphicsUsedCorrect() => GraphicsSettings.defaultRenderPipeline is HDRenderPipelineAsset (at last version)` (443); fix assigns `GraphicsSettings.defaultRenderPipeline = asset` (445-453).
  - Quality check (455-468): passes when `QualitySettings.renderPipeline == null`; fails when it is a non-HDRP asset; **fix sets `QualitySettings.renderPipeline = null;`**.
  - UI text (`HDRP/Editor/Wizard/HDWizard.cs:94`): "The RenderPipelineAsset assigned in the current Quality must be null or a HDRenderPipelineAsset. If it is null, the asset for the current Quality will be the one in Graphics Settings."
  - Conclusion: both are **not** needed. HDRP's own wizard configures `GraphicsSettings.defaultRenderPipeline = hdrpAsset` and leaves every quality level override `null`.
- Engine: `GraphicsSettings.allConfiguredRenderPipelines` returns "the default render pipeline and any quality level overrides"; `GraphicsSettings.currentRenderPipeline` is "the RenderPipelineAsset that defines the active render pipeline for the current quality level" (`ENGINE/UnityEngine.CoreModule.xml`).
- Engine APIs used by HDRP editor code: `QualitySettings.renderPipeline`, `QualitySettings.GetRenderPipelineAssetAt(int)`, `QualitySettings.ForEach((tier, name) => {...})` and `QualitySettings.ForEach(() => {...})` (`HDRP/Editor/BuildProcessors/HDRPPreprocessBuild.cs:259-265`, `HDRP/Editor/Lighting/Reflection/HDProbeUI.Drawers.cs:402-404`, `HDWizard.Configuration.cs:429,437`). `QualitySettings.SetRenderPipelineAssetAt`: **NOT VERIFIED** (not found in package sources; engine signature not dumped).
- Other wizard fixes: `PlayerSettings.colorSpace = ColorSpace.Linear` (388-391); `QualitySettings.ForEach(() => QualitySettings.shadows = ShadowQuality.All)` (427-430); `QualitySettings.ForEach(() => QualitySettings.shadowmaskMode = ShadowmaskMode.DistanceShadowmask)` (435-438); SRP Batcher `hdrpAsset.enableSRPBatcher = true` (internal field, 506-517); lightmap encoding High via reflection on internal `PlayerSettings.SetLightmapEncodingQualityForPlatform` (75-77, 408-422).
- Set automatically when the pipeline is built (`HDRP/Runtime/RenderPipeline/HDRenderPipeline.cs:802-814`): `GraphicsSettings.lightsUseLinearIntensity = true`, `lightsUseColorTemperature = true`, `useScriptableRenderPipelineBatching = m_Asset.enableSRPBatcher`, `defaultGateFitMode = Camera.GateFitMode.Vertical`, `QualitySettings.shadowmaskMode = ShadowmaskMode.DistanceShadowmask`.

### 1.5 Volume profiles

`UnityEditor.Rendering.VolumeProfileFactory` (public static class, `CORE/Editor/Volume/VolumeProfileFactory.cs:13`):

- `public static VolumeProfile CreateVolumeProfileAtPath(string path)` / `CreateVolumeProfileAtPath(string path, VolumeProfile dataSource)` (52, 60)
- `public static VolumeProfile CreateVolumeProfile(Scene scene, string targetName)` (89)
- `public static T CreateVolumeComponent<T>(VolumeProfile profile, bool overrides = false, bool saveAsset = true) where T : VolumeComponent` (104-118): `profile.Add<T>(overrides)`, hideFlags `HideInInspector | HideInHierarchy`, `AssetDatabase.AddObjectToAsset(comp, profile)`, optional `SaveAssets` + `Refresh`.
- `public static void CreateVolumeProfileWithCallback(string fullPath, Action<VolumeProfile> callback)` (33)

Runtime (`UnityEngine.Rendering.VolumeProfile`, `CORE/Runtime/Volume/VolumeProfile.cs:14`): `public List<VolumeComponent> components` (19), `public T Add<T>(bool overrides = false)` (92), `public VolumeComponent Add(Type type, bool overrides = false)` (109), `Remove<T>()`, `Remove(Type)`, `Has<T>()`, `TryGet<T>(out T component)` (231).

`UnityEditor.Rendering.VolumeProfileUtils` (public static class, `CORE/Editor/Volume/VolumeProfileUtils.cs:12`):

- `public static void EnsureAllOverridesForDefaultProfile(VolumeProfile profile, VolumeProfile defaultValueSource = null)` (166): no-op if the profile is inside a read-only package; removes obsolete components; activates every component and sets every `overrideState = true`; adds every missing non-obsolete VolumeComponent type with `profile.Add(type, overrides: true)` + `AssetDatabase.AddObjectToAsset`; `VolumeManager.instance.OnVolumeProfileChanged(profile)`.
- `public static void UpdateGlobalDefaultVolumeProfile<TRenderPipeline>(VolumeProfile globalDefaultVolumeProfile, VolumeProfile defaultValueSource = null)` (108): does nothing unless `RenderPipelineManager.currentPipeline is TRenderPipeline`; then the above + `VolumeManager.instance.SetGlobalDefaultProfile(...)`.
- `public static bool UpdateGlobalDefaultVolumeProfileWithConfirmation<TRenderPipeline>(...)` (79).

`UnityEngine.Rendering.VolumeManager` (`CORE/Runtime/Volume/VolumeManager.cs`): `public static VolumeManager instance` (55), `public bool isInitialized { get; }` (241), `public VolumeProfile globalDefaultProfile { get; private set; }` (164), `public void SetGlobalDefaultProfile(VolumeProfile profile)` (310), `SetQualityDefaultProfile`, `OnVolumeProfileChanged(VolumeProfile)` (347).

HDRP's own menu code for a sky/fog volume (`HDRP/Editor/RenderPipeline/HDRenderPipelineMenuItems.cs:108-127`):

```csharp
var settings = CoreEditorUtils.CreateGameObject("Sky and Fog Global Volume", parent);
var profile = VolumeProfileFactory.CreateVolumeProfile(settings.scene, "Sky and Fog Settings");
var visualEnv = VolumeProfileFactory.CreateVolumeComponent<VisualEnvironment>(profile, true, false);
visualEnv.skyType.value = SkySettings.GetUniqueID<PhysicallyBasedSky>();
visualEnv.skyAmbientMode.overrideState = false;
VolumeProfileFactory.CreateVolumeComponent<PhysicallyBasedSky>(profile, false, false);
var fog = VolumeProfileFactory.CreateVolumeComponent<Fog>(profile, false, true);
fog.enabled.Override(true);
fog.enableVolumetricFog.Override(true);
var volume = settings.AddComponent<Volume>();
volume.isGlobal = true;
volume.sharedProfile = profile;
```

### 1.6 Suggested end-to-end snippet (assembled from the verified pieces)

```csharp
// Editor-only. using System; using System.Collections.Generic; using System.Reflection;
// using UnityEditor; using UnityEditor.Rendering; using UnityEngine; using UnityEngine.Rendering;
// using UnityEngine.Rendering.HighDefinition;

var asset = ScriptableObject.CreateInstance<HDRenderPipelineAsset>();      // defaults = RenderPipelineSettings.NewDefault()
asset.name = "HDRenderPipelineAsset";
AssetDatabase.CreateAsset(asset, "Assets/HDRPDefaultResources/HDRenderPipelineAsset.asset");

var s = asset.currentPlatformRenderPipelineSettings;                       // struct copy
s.supportRayTracing = true; s.supportSSR = true; s.supportSSRTransparent = true; s.supportSSGI = true;
s.hdShadowInitParams.supportScreenSpaceShadows = true;
s.dynamicResolutionSettings.enabled = true;
s.dynamicResolutionSettings.advancedUpscalerNames = new List<string> { "DLSS", "STP" };
s.dynamicResolutionSettings.DLSSPerfQualitySetting = (uint)UnityEngine.NVIDIA.DLSSQuality.MaximumQuality;
asset.currentPlatformRenderPipelineSettings = s;                            // setter calls OnValidate()
EditorUtility.SetDirty(asset);

GraphicsSettings.defaultRenderPipeline = asset;                            // HDRP Wizard: Graphics Settings holds the asset
QualitySettings.renderPipeline = null;                                     // HDRP Wizard: quality overrides stay null
AssetDatabase.SaveAssets();                                                // reimport of GraphicsSettings.asset -> HDRP Global Settings Ensure()

// Default volume profile (HDRP must be the active pipeline for VolumeManager type discovery)
var dvp = GraphicsSettings.GetRenderPipelineSettings<HDRPDefaultVolumeProfileSettings>();
if (dvp != null && dvp.volumeProfile == null)
{
    var profile = VolumeProfileFactory.CreateVolumeProfileAtPath("Assets/HDRPDefaultResources/DefaultSettingsVolumeProfile.asset");
    VolumeProfileUtils.EnsureAllOverridesForDefaultProfile(profile);       // public Core API; adds every component with overrides on
    dvp.volumeProfile = profile;
    VolumeManager.instance.SetGlobalDefaultProfile(profile);
}
```

Notes: `EnsureAllOverridesForDefaultProfile` default-constructs values (it does not copy HDRP's curated defaults). To copy HDRP's defaults use `CreateVolumeProfileAtPath(path, dataSource)` with `dataSource` = the package profile `Editor/RenderPipelineResources/DefaultSettingsVolumeProfile.asset` (path verified in `HDRenderPipelineEditorAssets.cs:16`; loading it via a `Packages/...` path is **NOT VERIFIED**). The assignment order above is a recommendation, not a verified HDRP code path.

---

## 2. DLSS

### 2.1 How DLSS is selected (exact names)

Asset side: `HDRP/Runtime/RenderPipeline/Settings/RenderPipelineSettings.cs:472` `public GlobalDynamicResolutionSettings dynamicResolutionSettings;` (struct in `UnityEngine.Rendering`, `CORE/Runtime/Common/GlobalDynamicResolutionSettings.cs:80`).

- **Selection is a string list**: `public List<string> advancedUpscalerNames;` (line 129). HDRP switches on the strings `"DLSS"`, `"FSR2"`, `"STP"` (`HDRP/Runtime/RenderPipeline/HDRenderPipeline.cs:1422-1483`). Default: `advancedUpscalerNames = new List<string>() { AdvancedUpscalers.STP.ToString() }` (line 113), i.e. `["STP"]`.
- Enum used only for names/bitmasks: `public enum AdvancedUpscalers : byte { DLSS = 0, FSR2 = 1, STP = 2, IUpscaler = 3 /* only if ENABLE_UPSCALER_FRAMEWORK */ }` (55-76). `AdvancedUpscalers.DLSS.ToString() == "DLSS"`.
- Obsolete (migration only): `public List<AdvancedUpscalers> advancedUpscalersByPriority` (126), `public bool enableDLSS` (229). Do not use.
- Other fields (lines 119-229): `bool enabled`, `bool useMipBias`, `uint DLSSPerfQualitySetting`, `DynamicResolutionHandler.UpsamplerScheduleType DLSSInjectionPoint`, `TAAUInjectionPoint`, `STPInjectionPoint`, `defaultInjectionPoint`, `bool DLSSUseOptimalSettings`, `float DLSSSharpness`, `uint DLSSRenderPresetForQuality`, `DLSSRenderPresetForBalanced`, `DLSSRenderPresetForPerformance`, `DLSSRenderPresetForUltraPerformance`, `DLSSRenderPresetForDLAA`, FSR2 fields, `float maxPercentage`, `float minPercentage`, `DynamicResolutionType dynResType`, `DynamicResUpscaleFilter upsampleFilter`, `bool forceResolution`, `float forcedPercentage`, `lowResTransparencyMinimumThreshold`, `rayTracingHalfResThreshold`, `lowResSSGIMinimumThreshold`, `lowResVolumetricCloudsMinimumThreshold`.
- Defaults (`NewDefault()`, 84-117): `enabled = false`, `maxPercentage = 100`, `minPercentage = 100`, `dynResType = DynamicResolutionType.Hardware`, `upsampleFilter = DynamicResUpscaleFilter.CatmullRom`, `DLSSUseOptimalSettings = true`, `DLSSPerfQualitySetting = 0`, `DLSSSharpness = 0.5`, presets `0`, `DLSSInjectionPoint = UpsamplerScheduleType.BeforePost`.
- Enums: `DynamicResolutionType { Software, Hardware }` (9), `DynamicResUpscaleFilter { Bilinear [Obsolete], CatmullRom, Lanczos [Obsolete], ContrastAdaptiveSharpen, EdgeAdaptiveScalingUpres, TAAU }` (25).

Camera side (`HDRP/Runtime/RenderPipeline/Camera/HDAdditionalCameraData.cs`, region `DLSS_OVERRIDES` 396-421):

- `public bool allowDeepLearningSuperSampling = true;` (399)
- `public bool deepLearningSuperSamplingUseCustomQualitySettings = false;` (403)
- `public uint deepLearningSuperSamplingQuality = 0;` (407)
- `public bool deepLearningSuperSamplingUseCustomAttributes = false;` (411)
- `public bool deepLearningSuperSamplingUseOptimalSettings = true;` (415)
- `[Range(0, 1)] public float deepLearningSuperSamplingSharpening = 0;` (420)
- `public bool allowDynamicResolution = false;` (375) - required.
- Runtime state: `internal bool cameraCanRenderDLSS` (456). `HDCamera.IsDLSSEnabled()` is internal (`HDRP/Runtime/RenderPipeline/Camera/HDCamera.cs:1003`).

There is no `UpscalerSelection` enum, no `upscalerSettings` object and no HDRP-side `UpscalerFlags` in the structs above.

### 2.2 Requirements for DLSS to actually run (code path)

`HDRP/Runtime/RenderPipeline/HDRenderPipeline.cs`:

1. Line 2333: `cameraRequestedDynamicRes = hdCam.allowDynamicResolution && camera.cameraType == CameraType.Game;` and 2336 `cameraRequestedDynamicRes &= !HDCamera.GetOrCreate(camera).IsPathTracingEnabled();` -> only Game cameras; disabled when Path Tracing is on.
2. `SetupPartnerUpscalers` (1391-1497): returns if `!cameraRequestedDynamicRes || !dynamicResolutionSettings.enabled`; iterates `advancedUpscalerNames` **in order, first that succeeds wins**; `case "DLSS": hdCam.cameraCanRenderDLSS = HDDynamicResolutionPlatformCapabilities.DLSSDetected && hdCam.allowDeepLearningSuperSampling;`
3. `DoDLSSPasses` (`HDRP/Runtime/RenderPipeline/HDRenderPipeline.PostProcess.cs:912-931`) only runs when `upsamplerSchedule == currentAsset.currentPlatformRenderPipelineSettings.dynamicResolutionSettings.DLSSInjectionPoint`.
4. Upsampler choice (`GetCurrentUpsamplerData`, PostProcess.cs:523-650): DLSS/FSR2 are chosen independently of the camera anti-aliasing mode; STP and TAAU require `antialiasing == TemporalAntialiasing`. TAA is therefore **not** required for DLSS; HDRP jitters the camera itself: `RequiresCameraJitter()` is true for `IsDLSSEnabled()` (`HDCamera.cs:1146-1154`).
5. Motion vectors: DLSS is created with flags `IsHDR`, `MVLowRes`, `DepthInverted` (`HDRP/Runtime/RenderPipeline/RenderPass/DLSSPass.cs:228-230`) and receives the camera motion-vector texture (PostProcess.cs:958-961). They are controlled by asset `supportMotionVectors` (default `true`) and Frame Settings `MotionVectors`, `ObjectMotionVectors`, `TransparentsWriteMotionVector` (all on by default for cameras, `HDRP/Runtime/RenderPipeline/Settings/FrameSettingsDefaults.cs:31-32,75`). There is no per-camera `motionVectorSupportMode`.
6. Scaling: with `DLSSUseOptimalSettings == true`, `DLSSPass.SetupDRSScaling` overwrites `dynamicResolutionSettings.maxPercentage/minPercentage` from `NVIDIA.GraphicsDevice.GetOptimalSettings` and installs a system scaler (`DLSSPass.cs:456-499`). With it `false`, the project's `min/maxPercentage` are used (defaults 100/100 -> no upscaling).
7. Pass creation: `m_DLSSPass = DLSSPass.Create();` (`HDRenderPipeline.PostProcess.cs:269`). `DLSSPass.Create()` returns `null` unless `ENABLE_NVIDIA && ENABLE_NVIDIA_MODULE` and `SetupFeature()` passes (`DLSSPass.cs:18-51`).

Availability check (HDRP code): `public static class HDDynamicResolutionPlatformCapabilities` (`HDRP/Runtime/RenderPipeline/HDDynamicResolutionPlatformCapabilities.cs:8`): `public static bool DLSSDetected { get; }`, `public static bool FSR2Detected { get; }`; refreshed by `SetupFeatures()` (internal) from `HDRenderPipelineAsset.OnEnable()` and the pipeline constructor (`HDRenderPipeline.cs:866`). `DLSSPass.SetupFeature()` (`DLSSPass.cs:18-38`):

```csharp
#if ENABLE_NVIDIA && ENABLE_NVIDIA_MODULE
    if (!NVIDIA.NVUnityPlugin.IsLoaded()) return false;
    if (s_ExpectedDeviceVersion != NVIDIA.GraphicsDevice.version) return false;   // s_ExpectedDeviceVersion = 0x06
    if (!SystemInfo.graphicsDeviceVendor.ToLowerInvariant().Contains("nvidia")) return false;
    var device = NVIDIA.GraphicsDevice.CreateGraphicsDevice();
    return device != null && device.IsFeatureAvailable(NVIDIA.GraphicsDeviceFeature.DLSS);
#else
    return false;
#endif
```

It does **not** use `SystemInfo.supports...` nor `NVUnityPlugin.Load()` (only `IsLoaded()`).

### 2.3 NVIDIA module and defines (verified against this project)

- `ENABLE_NVIDIA` and `ENABLE_AMD` are engine-wide defines: present in `D:\AetheriaUnity\Library\Bee\artifacts\1900b0aE.dag\Unity.RenderPipelines.Core.Runtime.rsp` (`-define:ENABLE_NVIDIA`, `-define:ENABLE_AMD`).
- `ENABLE_NVIDIA_MODULE` is a package versionDefine: `"name": "com.unity.modules.nvidia", "expression": "1.0.0", "define": "ENABLE_NVIDIA_MODULE"` in `HDRP/Runtime/Unity.RenderPipelines.HighDefinition.Runtime.asmdef`, `HDRP/Editor/Unity.RenderPipelines.HighDefinition.Editor.asmdef` and `CORE/Runtime/Unity.RenderPipelines.Core.Runtime.asmdef`.
- `D:\AetheriaUnity\Packages\manifest.json` does **not** list `com.unity.modules.nvidia` (nor `com.unity.modules.amd`, HDRP, shadergraph, visualeffectgraph). Consequence: `ENABLE_NVIDIA_MODULE` is undefined -> `DLSSPass` is a stub -> `DLSSDetected == false` -> DLSS silently falls through to the next upscaler in the list.
- The NVIDIA built-in package exists at `BuiltInPackages/com.unity.modules.nvidia` (package.json `"name": "com.unity.modules.nvidia"`, `"version": "1.0.0"`). HDRP's inspector offers a "Install NVIDIA Package" fix that calls `PackageManager.Client.Add("com.unity.modules.nvidia")` under `#if ENABLE_NVIDIA && !ENABLE_NVIDIA_MODULE` (`HDRP/Editor/RenderPipeline/HDRenderPipelineUI.cs:560-562, 1101-1109`).
- `ENABLE_UPSCALER_FRAMEWORK` is not in the rsp, and `UpscalerRegistry`, `AbstractUpscaler`, `DLSSIUpscaler`, `UpscalerOptions` are **absent** from the project's compiled `D:\AetheriaUnity\Library\ScriptAssemblies\Unity.RenderPipelines.Core.Runtime.dll` (Cecil check). The IUpscaler framework sources (`CORE/Runtime/Upscaling/IUpscaler.cs`, `Upscaling.cs`, `DLSSIUpscaler.cs`, `DLSSOptions.cs`) are entirely wrapped in `#if ENABLE_UPSCALER_FRAMEWORK`; they are not usable here. `UpscalerUtils.cs` (`HDRP/Runtime/RenderPipeline/RenderPass/UpscalerUtils.cs`) only contains internal helpers (`UpscalerCameras`, `UpscalerResolution`, `UpscalerResources`), no public API.

### 2.4 NVIDIA module API (Cecil dump of `ENGINE/UnityEngine.NVIDIAModule.dll`, namespace `UnityEngine.NVIDIA`)

- `public enum DLSSQuality { MaximumPerformance = 0, Balanced = 1, MaximumQuality = 2, UltraPerformance = 3, DLAA = 4 }`
- `public enum DLSSPreset { Preset_Default = 0, Preset_F = 1, Preset_J = 2, Preset_K = 4, Preset_L = 8, Preset_M = 16 }`
- `public enum DLSSFeatureFlags { None = 0, IsHDR = 1, MVLowRes = 2, MVJittered = 4, DepthInverted = 8, DoSharpening = 16, AutoExposure = 32 }`
- `public class NVUnityPlugin { static bool Load(); static bool IsLoaded(); }`
- `public class GraphicsDevice`: `static GraphicsDevice device`, `static GraphicsDevice CreateGraphicsDevice()`, `IsFeatureAvailable(GraphicsDeviceFeature)`, `GetOptimalSettings(uint, uint, DLSSQuality, out OptimalDLSSSettingsData)`, `GetAvailableDLSSPresetsForQuality(DLSSQuality)`, `CreateFeature`, `ExecuteDLSS`, `DestroyFeature`; `GraphicsDeviceFeature.DLSS`.
- `OptimalDLSSSettingsData { outRenderWidth, outRenderHeight, sharpness, maxWidth, maxHeight, minWidth, minHeight }`.
- Same members are documented in `ENGINE/UnityEngine.NVIDIAModule.xml` (lines 7-553).

Doc: `HDRP/Documentation~/deep-learning-super-sampling-in-hdrp.md` ("HDRP natively supports DLSS 4.5 Super Resolution"; platforms DirectX 11/12 and Vulkan on Windows 64-bit; steps: install NVIDIA package -> enable Dynamic Resolution + DLSS on the HDRP asset -> per camera Allow Dynamic Resolution + Allow DLSS).

### 2.5 Quality storage and runtime changes

- Project quality: `dynamicResolutionSettings.DLSSPerfQualitySetting` (`uint`, cast of `NVIDIA.DLSSQuality`). 0 = MaximumPerformance, 1 = Balanced, 2 = MaximumQuality, 3 = UltraPerformance, 4 = DLAA.
- Per-camera override: `deepLearningSuperSamplingUseCustomQualitySettings = true` + `deepLearningSuperSamplingQuality`.
- Attributes override: `deepLearningSuperSamplingUseCustomAttributes` selects the camera's `deepLearningSuperSamplingUseOptimalSettings` instead of the asset's `DLSSUseOptimalSettings` (`HDRenderPipeline.cs:1433-1436`).
- Presets are project-wide only (`DLSSRenderPresetFor*`), copied per frame in `DLSSPass.InternalNVIDIARender` (`DLSSPass.cs:544-555`), which also re-reads the quality every frame; the DLSS context is re-created when `perfQuality` changes (`DLSSPass.cs:209-245`). Camera fields are plain public fields -> changeable at runtime. Asset changes go through the `currentPlatformRenderPipelineSettings` setter (pipeline rebuild).
- In the editor UI: `UnityEngine.NVIDIA.GraphicsDevice.GetAvailableDLSSPresetsForQuality(...)` is used to validate presets (`HDRenderPipelineUI.cs:735-816`).

Snippet (asset + camera):

```csharp
var s = asset.currentPlatformRenderPipelineSettings;
var drs = s.dynamicResolutionSettings;
drs.enabled = true;
drs.advancedUpscalerNames = new List<string> { "DLSS", "STP" };     // first supported wins
drs.DLSSPerfQualitySetting = (uint)UnityEngine.NVIDIA.DLSSQuality.MaximumQuality;   // requires the NVIDIA module reference
drs.DLSSUseOptimalSettings = true;
drs.DLSSInjectionPoint = UnityEngine.Rendering.DynamicResolutionHandler.UpsamplerScheduleType.BeforePost;
s.dynamicResolutionSettings = drs;
asset.currentPlatformRenderPipelineSettings = s;

var hd = camera.gameObject.AddComponent<HDAdditionalCameraData>();
hd.allowDynamicResolution = true;
hd.allowDeepLearningSuperSampling = true;
hd.deepLearningSuperSamplingUseCustomQualitySettings = true;
hd.deepLearningSuperSamplingQuality = (uint)UnityEngine.NVIDIA.DLSSQuality.Balanced;
```

SerializedObject path used by HDRP's inspector: `m_RenderPipelineSettings.dynamicResolutionSettings.enabled`, `.advancedUpscalerNames`, `.DLSSPerfQualitySetting` (uint), `.DLSSInjectionPoint`, `.DLSSUseOptimalSettings`, `.DLSSRenderPresetFor*` (`HDRP/Editor/RenderPipeline/HDRenderPipelineUI.cs:567,603-610,724-730,810-816`). Whether user code may reference `UnityEngine.NVIDIA` without the module installed: **NOT VERIFIED**.

---

## 3. Hardware ray tracing

### 3.1 Asset fields (`HDRP/Runtime/RenderPipeline/Settings/RenderPipelineSettings.cs`)

- `public bool supportRayTracing;` (456, default `false`), `public bool supportVFXRayTracing;` (458, default `false`)
- `public SupportedRayTracingMode supportedRayTracingMode;` (461, default `Both`); `public enum SupportedRayTracingMode { Performance = 1 << 0, Quality = 1 << 1, Both = Performance | Quality }` (104-112)
- Rasterized feature switches that gate the ray-traced versions (default values from `NewDefault()`, 114-202): `supportSSR` (false), `supportSSRTransparent` (false), `supportSSGI` (false), `supportSSAO` (true), `hdShadowInitParams` (`HDShadowInitParameters`; `supportScreenSpaceShadows = false`, `maxScreenSpaceShadowSlots = 4`, `HDRP/Runtime/Lighting/Shadow/HDShadowManager.cs:374-393`).
- No `supportRayTracingNewShaderGenerator`, no RTXDI, no light-binning field exists (section 0).
- Wizard DXR fix (`HDRP/Editor/Wizard/HDWizard.Configuration.cs:862-876`):

```csharp
var serializedObject = new SerializedObject(HDRenderPipeline.currentAsset);
var propertySupportRayTracing = serializedObject.FindProperty("m_RenderPipelineSettings.supportRayTracing");
propertySupportRayTracing.boolValue = true;
serializedObject.ApplyModifiedPropertiesWithoutUndo();
```

- Wizard optional checks (784-847): `supportScreenSpaceShadows` + default camera Frame Setting `ScreenSpaceShadows`; `supportSSR` + `SSR`; `supportSSRTransparent` + `TransparentSSR`; `supportSSGI` + `SSGI`; `supportVFXRayTracing` + `RaytracingVFX`.

### 3.2 Availability

- `public bool HDRenderPipeline.rayTracingSupported { get; }` (`HDRP/Runtime/RenderPipeline/HDRenderPipeline.cs:476`) - the only public HDRP query. Usage: `(RenderPipelineManager.currentPipeline as HDRenderPipeline)?.rayTracingSupported == true`.
- How it is computed (548-551): `m_RayTracingSupported = PipelineSupportsRayTracing(settings) && rayTracingResources != null;`
- `HDRaytracingManager.PipelineSupportsRayTracing(rpSetting) => rpSetting.supportRayTracing && currentSystemSupportsRayTracing` and `currentSystemSupportsRayTracing => SystemInfo.supportsRayTracing && (Editor: activeBuildTarget is StandaloneWindows64, StandaloneWindows, GameCoreXboxSeries or PS5)` (`HDRP/Runtime/RenderPipeline/Raytracing/HDRaytracingManager.cs:909-920`).
- Resources: `GraphicsSettings.GetRenderPipelineSettings<HDRPRayTracingResources>()` (type is **internal**; wizard check `GraphicsSettings.TryGetRenderPipelineSettings<HDRPRayTracingResources>(out var _) && (SystemInfo.supportsRayTracing || console target)`, `HDWizard.Configuration.cs:762-767`). Public way to test for the resource from project code: **NOT VERIFIED**.
- Also usable: `SystemInfo.supportsRayTracing`, `HDUtils.hdrpSettings` (public static, `HDRP/Runtime/RenderPipeline/Utility/HDUtils.cs:111`), `GraphicsSettings.currentRenderPipeline as HDRenderPipelineAsset`.

### 3.3 Frame Settings

- Values (`HDRP/Runtime/RenderPipeline/Settings/FrameSettings.cs:107+`): `RayTracing = 92`, `RaytracingVFX = 100`, `SSR = 23`, `TransparentSSR = 94`, `SSAO = 24`, `SSGI = 95`, `ScreenSpaceShadows = 34`, `ContactShadows = 21`, `ShadowMaps = 20`, `MotionVectors = 10`, `ObjectMotionVectors = 11`, `TransparentsWriteMotionVector = 16`, `Postprocess = 15`, `Decals = 12`, `HighQualityLineRendering = 103`, `ExposureControl = 32`.
- Defaults for cameras include `RayTracing`, `SSR`, `TransparentSSR`, `SSAO`, `SSGI`, `ScreenSpaceShadows` (`FrameSettingsDefaults.cs:7-90`); they are sanitised against the asset afterwards.
- Public API: `FrameSettings.IsEnabled(FrameSettingsField field)` (508), `FrameSettings.SetEnabled(FrameSettingsField field, bool value)` (512), `FrameSettingsOverrideMask.mask` (`BitArray128`, 441).

Per camera (`HDRP/Runtime/RenderPipeline/Camera/HDAdditionalCameraData.cs:379,514,520`; example from `HDRP/Documentation~/Frame-Settings-API.md`):

```csharp
hdCam.customRenderingSettings = true;
hdCam.renderingPathCustomFrameSettingsOverrideMask.mask[(int)FrameSettingsField.RayTracing] = true;
hdCam.renderingPathCustomFrameSettings.SetEnabled(FrameSettingsField.RayTracing, true);
```

Serialized name of the custom frame settings is `m_RenderingPathCustomFrameSettings` (FormerlySerializedAs `renderingPathCustomFrameSettings`).

### 3.4 Volume components (namespace `UnityEngine.Rendering.HighDefinition`)

**ScreenSpaceReflection** - `public class ScreenSpaceReflection : VolumeComponentWithQuality`, `[VolumeComponentMenu("Lighting/Screen Space Reflection")]` (`HDRP/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceReflection.cs:46-49`): `BoolParameter enabled = new(true, EnumPopup)` (67), `BoolParameter enabledTransparent = new(true, ...)` (71), `RayCastingModeParameter tracing = new RayCastingModeParameter(RayCastingMode.RayMarching)` (76), `float minSmoothness` (82), `float smoothnessFadeStart` (99), `BoolParameter reflectSky = new(true)` (118), `SSRAlgoParameter usedAlgorithm = Approximation` (121), `ClampedFloatParameter depthBufferThickness = 0.01` (127), `screenFadeDistance = 0.1` (132), `accumulationFactor = 0.75` (137), `biasFactor = 0.5` (143), `int rayMaxIterations` (185), `RayTracingFallbackHierachyParameter rayMiss = ReflectionProbesAndSky` (207), `lastBounceFallbackHierarchy` (213), `ClampedFloatParameter ambientProbeDimmer = 1` (220), `LayerMaskParameter layerMask = -1` (225), `ClampedFloatParameter textureLodBias = 1` (230), `float rayLength` (235), `float clampValue` (252), `bool denoise` (267), `float denoiserRadius` (285), `float denoiserAntiFlickeringStrength` (303), `RayTracingModeParameter mode = new RayTracingModeParameter(RayTracingMode.Quality)` (321), `bool fullResolution` (326), `ClampedIntParameter sampleCount = 1 (1..32)` (345), `ClampedIntParameter bounceCount = 1 (1..8)` (349), `int rayMaxIterationsRT` (354), `RenderingLayerMaskParameter adaptiveProbeVolumesLayerMask` (374).

**GlobalIllumination** - `public sealed class GlobalIllumination : VolumeComponentWithQuality`, `[VolumeComponentMenu("Lighting/Screen Space Global Illumination")]` (`HDRP/Runtime/Lighting/GlobalIllumination.cs:9-13`): `BoolParameter enable = new(false, EnumPopup)` (26), `RayCastingModeParameter tracing = RayMarching` (31), `RayMarchingFallbackHierarchyParameter rayMiss = ReflectionProbesAndSky` (39), `RenderingLayerMaskParameter adaptiveProbeVolumesLayerMask` (46), `ClampedFloatParameter depthBufferThickness = 0.1` (54), `BoolParameter fullResolutionSS = true` (59), `int maxRaySteps` (64), `bool denoiseSS` (83), `bool halfResolutionDenoiserSS` (100), `float denoiserRadiusSS`, `bool secondDenoiserPassSS` (136), `lastBounceFallbackHierarchy` (159), `ClampedFloatParameter ambientProbeDimmer = 0` (166), `LayerMaskParameter layerMask = -1` (172), `ClampedFloatParameter textureLodBias = 7` (178), `RayTracingModeParameter mode = Quality` (216), `bool fullResolution` (222), `ClampedIntParameter sampleCount = 2 (1..32)` (242), `ClampedIntParameter bounceCount = 1 (1..8)` (248), `bool denoise` (254), `bool halfResolutionDenoiser` (272), `bool secondDenoiserPass` (308), `BoolParameter receiverMotionRejection = true` (346). There is no member named `rayTracing`; the switch is `tracing`.

**ScreenSpaceAmbientOcclusion** - `public sealed class ScreenSpaceAmbientOcclusion : VolumeComponentWithQuality`, `[VolumeComponentMenu("Lighting/Ambient Occlusion")]` (`HDRP/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceAmbientOcclusion.cs:11-14`): `BoolParameter rayTracing = new(false)` (19), `ClampedFloatParameter intensity = 0 (0..4)` (24), `directLightingStrength = 0 (0..1)` (28), `radius = 2 (0.25..5)` (32), `spatialBilateralAggressiveness = 0.15` (37), `BoolParameter temporalAccumulation = true` (43), `ghostingReduction = 0.5` (49), `blurSharpness = 0.1` (55), `LayerMaskParameter layerMask = -1` (61), `specularOcclusion = 0.5` (67), `float rayLength` (72), `int sampleCount` (87), `bool denoise` (102), `float denoiserRadius` (117), `int stepCount` (133), `bool fullResolution` (148), `int maximumRadiusInPixels` (165), `bool bilateralUpsample` (180), `int directionCount` (195), `BoolParameter occluderMotionRejection = true` (211), `receiverMotionRejection = true` (217).

**RayTracingSettings** - `public sealed class RayTracingSettings : VolumeComponent`, `[VolumeComponentMenu("Ray Tracing/Ray Tracing Settings")]` (`HDRP/Runtime/RenderPipeline/Raytracing/RayTracingSettings.cs:63-67`): `ClampedFloatParameter rayBias = 0.001 (0..0.1)`, `distantRayBias = 0.001`, `BoolParameter extendShadowCulling = true`, `extendCameraCulling = false`, `MinFloatParameter directionalShadowRayLength = 1000`, `ClampedFloatParameter directionalShadowFallbackIntensity = 1 (0..1)`, `RTASBuildModeParameter buildMode = Automatic`, `RTASCullingModeParameter cullingMode = ExtendedFrustum`, `MinFloatParameter cullingDistance = 1000`, `ClampedFloatParameter minSolidAngle = 4 (0.01..180)`. Enums `RTASBuildMode { Automatic, Manual }` (9), `RTASCullingMode { ExtendedFrustum, Sphere, None, SolidAngle }` (34).

**LightCluster** - `public sealed class LightCluster : VolumeComponent`, `[VolumeComponentMenu("Ray Tracing/Light Cluster")]` (`HDRP/Runtime/RenderPipeline/Raytracing/LightCluster.cs:8-12`): `MinFloatParameter cameraClusterRange = 10 (min 0.001)`.

**RecursiveRendering** - `public sealed class RecursiveRendering : VolumeComponent`, `[VolumeComponentMenu("Ray Tracing/Recursive Rendering")]` (`HDRP/Runtime/RenderPipeline/Raytracing/RecursiveRendering.cs:9-13`): `BoolParameter enable = false (EnumPopup)`, `LayerMaskParameter layerMask = -1`, `ClampedIntParameter maxDepth = 4 (1..10)`, `MinFloatParameter rayLength = 10`, `ClampedFloatParameter minSmoothness = 0.5`, `RayTracingFallbackHierachyParameter rayMiss = ReflectionProbesAndSky`, `lastBounce = ReflectionProbesAndSky`, `ClampedFloatParameter ambientProbeDimmer = 1`.

**PathTracing** - `public sealed class PathTracing : VolumeComponent`, `[VolumeComponentMenu("Ray Tracing/Path Tracing")]` (`HDRP/Runtime/RenderPipeline/PathTracing/PathTracing.cs:132-136`): `BoolParameter enable = false`, `LayerMaskParameter layerMask = -1`, `ClampedIntParameter maximumSamples = 256 (1..16384)`, `minimumDepth = 1 (1..32)`, `maximumDepth = 4 (1..32)`, `MinFloatParameter maximumIntensity = 10`, `DenoiserParameter denoising = HDDenoiserType.None`, `BoolParameter useAOVs = true`, `temporal = false`, `separateVolumetrics = false`, `asyncDenoising = true`, `SkyImportanceSamplingParameter skyImportanceSampling = HDRIOnly`, `Vector4Parameter tilingParameters`, `SeedModeParameter seedMode = NonRepeating`, `IntParameter customSeed = 0`. Path Tracing turns dynamic resolution off for the camera.

**ContactShadows** - `public class ContactShadows : VolumeComponentWithQuality`, `[VolumeComponentMenu("Shadowing/Contact Shadows")]` (`HDRP/Runtime/Lighting/Shadow/ContactShadows.cs:9-13`): `BoolParameter enable = new(false, EnumPopup)` (18), `ClampedFloatParameter length = 0.15 (0..1)` (23), `opacity = 1 (0..1)` (27), `distanceScaleFactor = 0.5` (31), `MinFloatParameter maxDistance = 50` (35), `minDistance = 0` (39), `fadeDistance = 5` (43), `fadeInDistance = 0` (47), `rayBias = 0.2 (0..1)` (51), `thicknessScale = 0.15 (0.02..1)` (55), `int sampleCount` (61). No ray-tracing toggle here.

**HDShadowSettings** - `public class HDShadowSettings : VolumeComponent, ISerializationCallbackReceiver`, `[VolumeComponentMenu("Shadowing/Shadows")]` (`HDRP/Runtime/Lighting/Shadow/HDShadowSettings.cs:8-12`): `NoInterpMinFloatParameter maxShadowDistance = 500` (99), `ClampedFloatParameter directionalTransmissionMultiplier = 1 (0..1)` (103), `NoInterpClampedIntParameter cascadeShadowSplitCount = 4 (1..4)` (107), `CascadePartitionSplitParameter cascadeShadowSplit0 = 0.05`, `cascadeShadowSplit1 = 0.15`, `cascadeShadowSplit2 = 0.3` (110-116), `CascadeEndBorderParameter cascadeShadowBorder0..3 = 0` (119-128). No ray-tracing fields.

Enums: `public enum RayCastingMode { RayMarching = 1 << 0, RayTracing = 1 << 1, Mixed = 1 << 2 }` + `RayCastingModeParameter` (`HDRP/Runtime/RenderPipeline/Raytracing/RayCastingMode.cs:8`); `public enum RayTracingMode { Performance = 1 << 0, Quality = 1 << 1 }` + `RayTracingModeParameter` (`.../RayTracingMode.cs:8`). `RayTracingFallbackHierachy` members other than `ReflectionProbesAndSky`: **NOT VERIFIED**.

### 3.5 How to enable each effect (verified conditions)

- **Ray-traced reflections**: asset `supportSSR = true`, `supportRayTracing = true`; camera Frame Settings `SSR` and `RayTracing` on; volume `ScreenSpaceReflection.enabled.Override(true)`, `tracing.Override(RayCastingMode.RayTracing)` (or `Mixed`). Condition (`HDRP/Runtime/RenderPipeline/Raytracing/HDRenderPipeline.RaytracingReflection.cs:82-106`): `frameSettings.IsEnabled(SSR) && settings.enabled.value && frameSettings.IsEnabled(RayTracing) && settings.tracing.value != RayCastingMode.RayMarching && GetRayTracingState()`. Performance vs Quality (773-776): if the asset is `Both`, Quality is used when `tracing == RayTracing && mode == RayTracingMode.Quality`; otherwise the asset mode decides.
- **Ray-traced GI**: asset `supportSSGI = true`, `supportRayTracing = true`; Frame Settings `SSGI`, `RayTracing`; volume `GlobalIllumination.enable.Override(true)`, `tracing.Override(RayCastingMode.RayTracing)`. `Mixed` only works in Deferred (`mixedTracing = tracing == Mixed && litShaderMode == LitShaderMode.Deferred`, `HDRenderPipeline.RaytracingIndirectDiffuse.cs:61`; asset default `supportedLitShaderMode = DeferredOnly`).
- **Ray-traced AO**: asset `supportSSAO`; Frame Settings `SSAO`; volume `ScreenSpaceAmbientOcclusion.rayTracing.Override(true)`, `intensity.Override(>0)`. Exact runtime condition: **NOT VERIFIED**.
- **Ray-traced shadows**: asset `hdShadowInitParams.supportScreenSpaceShadows = true`, `supportRayTracing`; Frame Settings `ScreenSpaceShadows`; light: `hdLight.useRayTracedShadows = true`, for Directional lights also `hdLight.useScreenSpaceShadows = true` (`HDRP/Runtime/Lighting/Light/HDAdditionalLightData.cs:637,812`; `HDRP/Documentation~/Ray-Traced-Shadows.md`). The light's shadow enable property name on `HDAdditionalLightData`: **NOT VERIFIED**.
- **Ray-traced contact shadows**: per light `hdLight.useContactShadow` (`BoolScalableSettingValue`: `useOverride`, `@override`, `level`, line 1539) and `hdLight.rayTraceContactShadow = true` (1546). Exact runtime condition: **NOT VERIFIED**.
- **Dynamic resolution is not a requirement** for ray tracing. The only link is `dynamicResolutionSettings.rayTracingHalfResThreshold` (default 50): `DynamicResolutionHandler.instance.GetCurrentScale() >= (rayTracingHalfResThreshold / 100.0f)` decides full-resolution tracing (`HDRaytracingManager.cs:857`). MSAA is disabled with ray tracing (`FrameSettings.cs:126` tooltip).
- Platform requirements (`HDRP/Documentation~/Ray-Tracing-Getting-Started.md`): DirectX 12 only, disable Static Batching for Play Mode, x86_64 build, enable the rasterized version of each effect first, cameras need the `RayTracing` Frame Setting. Scene check menu: "Edit > Rendering > Check Scene Content for HDRP Ray Tracing" (`HDRenderPipelineMenuItems.cs:507`).

Quality settings on `VolumeComponentWithQuality` types (important when setting values from code), `HDRP/Runtime/Utilities/VolumeComponentWithQuality.cs:9-81`: `public ScalableSettingLevelParameter quality = new ScalableSettingLevelParameter((int)ScalableSettingLevelParameter.Level.Medium, false);` and `protected bool UsesQualitySettings() => !quality.levelAndOverride.useOverride && (HDRenderPipeline)RenderPipelineManager.currentPipeline != null;`. Plain properties such as `sampleCount`, `denoise`, `fullResolution`, `denoiseSS`, `minSmoothness` return the HDRP asset's `lightingQualitySettings.*[level]` arrays unless the component is in "custom" mode. To use per-volume values:

```csharp
var ssr = VolumeProfileFactory.CreateVolumeComponent<ScreenSpaceReflection>(profile, false, false);
ssr.quality.levelAndOverride = ((int)ScalableSettingLevelParameter.Level.High, true);   // useOverride = true
ssr.enabled.Override(true);
ssr.tracing.Override(RayCastingMode.RayTracing);
ssr.mode.Override(RayTracingMode.Quality);
ssr.denoise = true;
```

`ScalableSettingLevelParameter : NoInterpIntParameter` (`HDRP/Runtime/RenderPipeline/GlobalPostProcessingQualitySettings.cs:9-52`): `const int LevelCount = 3`, `enum Level { Low, Medium, High }`, `(int level, bool useOverride) levelAndOverride { get; set; }`. Per-parameter quality values live in `GlobalLightingQualitySettings` arrays `RTAORayLength`, `RTAOSampleCount`, `RTAODenoise`, `RTAODenoiserRadius`, `RTGIRayLength`, `RTGIFullResolution`, `RTGIRaySteps`, `RTGIDenoise`, `RTGIHalfResDenoise`, `RTGIDenoiserRadius`, `RTGISecondDenoise`, `RTRMinSmoothness`, `RTRSmoothnessFadeStart`, `RTRRayLength`, `RTRFullResolution`, `RTRRayMaxIterations`, `RTRDenoise`, `RTRDenoiserRadiusDimmer`, `RTRDenoiserAntiFlicker` (`HDRP/Runtime/RenderPipeline/GlobalLightingQualitySettings.cs:204-255`).

---

## 4. Lighting / camera / sky API

### 4.1 HDAdditionalCameraData (fully read)

`UnityEngine.Rendering.HighDefinition.HDAdditionalCameraData : MonoBehaviour, IFrameSettingsHistoryContainer, IAdditionalData`, `[AddComponentMenu("")] [DisallowMultipleComponent, ExecuteAlways] [RequireComponent(typeof(Camera))]` (`HDRP/Runtime/RenderPipeline/Camera/HDAdditionalCameraData.cs:158-161`). HDRP's own menu code adds it with a plain `camera.gameObject.AddComponent<HDAdditionalCameraData>()` (`HDRenderPipelineMenuItems.cs:326`).

Enums: `ClearColorMode { Sky, Color, None }` (226); `AntialiasingMode { None, FastApproximateAntialiasing, TemporalAntialiasing, SubpixelMorphologicalAntiAliasing }` (239; inspector names "No Anti-aliasing", "Fast Approximate Anti-aliasing (FXAA)", "Temporal Anti-aliasing (TAA)", "Subpixel Morphological Anti-aliasing (SMAA)"); `SMAAQualityLevel { Low, Medium, High }` (258); `TAAQualityLevel { Low, Medium, High }` (271); `TAASharpenMode { LowQuality, PostSharpen, ContrastAdaptiveSharpening }` (284); `FlipYMode { Automatic, ForceFlipY }` (166); `BufferAccessType { Depth = 1, Normal = 2, Color = 4 }` (178). There are no `SMAALow/SMAAMedium/SMAAHigh` anti-aliasing members.

Fields (line numbers in the same file): `clearColorMode = ClearColorMode.Sky` (295), `backgroundColorHDR = new Color(0.025f, 0.07f, 0.19f, 0.0f)` (298), `clearDepth = true` (300), `volumeLayerMask = 1` (304), `volumeAnchorOverride` (307), `antialiasing = AntialiasingMode.None` (310), `SMAAQuality = SMAAQualityLevel.High` (312), `dithering = false` (314), `stopNaNs = false` (316), `taaSharpenStrength = 0.5` (320), `TAAQuality = TAAQualityLevel.Medium` (323), `taaSharpenMode = TAASharpenMode.LowQuality` (326), `taaRingingReduction = 0` (330), `taaHistorySharpening = 0.35` (334), `taaAntiFlicker = 0.5` (338), `taaMotionVectorRejection = 0` (343), `taaAntiHistoryRinging = false` (346), `taaBaseBlendFactor = 0.875` (350), `taaJitterScale = 1` (354), `[Obsolete] physicalParameters` (360; physical camera values moved to `Camera`), `flipYMode` (364), `xrRendering = true` (367), `fullscreenPassthrough = false` (371), `allowDynamicResolution = false` (375), `customRenderingSettings = false` (379), `invertFaceCulling = false` (382), `probeLayerMask = ~0` (385), `hasPersistentHistory = false` (388), `screenSizeOverride` (391), `screenCoordScaleBias` (394), FSR2 fields `allowFidelityFX2SuperResolution`, `fidelityFX2SuperResolutionUseCustomQualitySettings`, `fidelityFX2SuperResolutionQuality`, `fidelityFX2SuperResolutionUseCustomAttributes`, `fidelityFX2SuperResolutionUseOptimalSettings`, `fidelityFX2SuperResolutionEnableSharpening`, `fidelityFX2SuperResolutionSharpening` (423-452), FSR1 `fsrOverrideSharpness` (479), `fsrSharpness` (484), `event Action<ScriptableRenderContext, HDCamera> customRender` (488), `event RequestAccessDelegate requestGraphicsBuffer` (497), `exposureTarget` (GameObject, 500), `materialMipBias = 0` (503), `FrameSettingsOverrideMask renderingPathCustomFrameSettingsOverrideMask` (514), `FrameSettingsRenderType defaultFrameSettings` (517), `public ref FrameSettings renderingPathCustomFrameSettings => ref m_RenderingPathCustomFrameSettings;` (520), `public NonObliqueProjectionGetter nonObliqueProjectionGetter` (737).

Methods: `CopyTo(HDAdditionalCameraData data)` (663), `GetNonObliqueProjection(Camera)` (744), `SetAOVRequests(AOVRequestDataCollection)` (639), `aovRequests` (647), `isEditorCameraPreview` (656), `GetGraphicsBuffer(BufferAccessType type)` (809).

- **No `exposure` member.** Exposure is the `Exposure` Volume component gated by `FrameSettingsField.ExposureControl`; the camera only has `exposureTarget` (metering target). `Exposure` component fields: **NOT VERIFIED**.
- **No motion-vector member** (section 0).
- `OnEnable` (749-770) returns immediately unless HDRP is the active pipeline, then forces `Camera.allowMSAA = false` and `Camera.allowHDR = false`.
- `internal static void InitDefaultHDAdditionalCameraData(HDAdditionalCameraData)` (773) is internal: maps `camera.clearFlags` to `clearColorMode` and sets `clearDepth = camera.clearFlags != CameraClearFlags.Nothing`.
- RenderTexture / overlay cameras: `camera.targetTexture` support in HDRP, and whether an overlay UI camera needs special settings: **NOT VERIFIED**. Verified side fact: dynamic resolution / DLSS only apply when `camera.cameraType == CameraType.Game` (`HDRenderPipeline.cs:2333`).

### 4.2 HDAdditionalLightData (partially read)

HDRP's own creation code (`HDRP/Editor/RenderPipeline/HDRenderPipelineMenuItems.cs:83-87` and 309-313):

```csharp
var light = go.AddComponent<Light>();
light.type = LightType.Directional;
var hdLight = go.AddComponent<HDAdditionalLightData>();
HDAdditionalLightData.InitDefaultHDAdditionalLightData(hdLight);
```

Accessibility of `InitDefaultHDAdditionalLightData`: **NOT VERIFIED**. Other verified facts:

- `HDAdditionalLightData.CelestialBodyShadingSource.ReflectSunLight` with `celestialBodyShadingSource`, `distance`, `diameterMultiplerMode`, `diameterOverride`, `flareSize`, `surfaceTexture`; directional intensity comment "0.5 lux" (menu code 89-101).
- `public bool useScreenSpaceShadows { get; set; }` (637), `public bool interactsWithSky { get; }` ("Controls if the directional light affect the Physically Based sky. This have no effect on other skies.", 649-661), `public bool useRayTracedShadows { get; set; }` (812), `[Range(1, 32)] int m_NumRayTracingSamples = 4` (property name **NOT VERIFIED**), `public BoolScalableSettingValue useContactShadow { get; }` (1539), `public bool rayTraceContactShadow { get; set; }` (1546), `Color m_ShadowTint = Color.black` (1559), `public ShadowUpdateMode shadowUpdateMode { get; set; }` (1649), `public void SetShadowUpdateMode(ShadowUpdateMode updateMode)` (3415), `public bool linkShadowLayers` (1086), light layer mask getter (1068), `shadowFadeDistance`, `areaLightShadowCone`, `distance` (area lights).
- Legacy mapping in `HDRP/Runtime/Deprecated.cs:740-770`: enum `HDLightTypeAndShape { BoxSpot, PyramidSpot, ConeSpot, Directional, RectangleArea, TubeArea, DiscArea }` -> `LightType.Box, Pyramid, Spot, Directional, Rectangle, Tube, Disc` via an internal `AddHDLight(gameObject, type)` helper.
- Light unit API, `SetSpotAngle`, `shapeRadius`, `shapeWidth/Height`, `useShadowConstantBias`, `lightUnit` / `LightUnit`: **NOT VERIFIED**. Engine facts: `GraphicsSettings.lightsUseLinearIntensity = true` and `lightsUseColorTemperature = true` are forced by HDRP (section 1.4).

### 4.3 Volume components for look and sky

Verified only:

- `VisualEnvironment.skyType` (`.value` int) and `skyAmbientMode.overrideState`; sky selection by `visualEnv.skyType.value = SkySettings.GetUniqueID<PhysicallyBasedSky>();` (`HDRenderPipelineMenuItems.cs:115-118`). This is the exact mechanism: `SkySettings.GetUniqueID<TSky>()`. Other `VisualEnvironment` members and `skyTypePrefab`: **NOT VERIFIED**.
- `Fog.enabled.Override(true)`, `Fog.enableVolumetricFog.Override(true)` (120-122). Other `Fog` members (`baseHeight`, `maxHeight`, `albedo`, `density`, `anisotropy`...): **NOT VERIFIED**.
- Classes `PhysicallyBasedSky`, `GradientSky`, `HDRISky`, `Bloom`, `Tonemapping` (`TonemappingMode`), `Vignette`, `ColorAdjustments`, `Exposure`, `AmbientOcclusion`, `IndirectLightingController`, `FilmGrain`, `DepthOfField`, `SplitToning`, `WhiteBalance`, `ChannelMixer`, `LiftGammaGain`, `ShadowsMidtonesHighlights`: **NOT VERIFIED** (not read). Doc files that exist for them: `HDRP/Documentation~/physically-based-sky-volume-override-reference.md`, `gradient-sky-volume-override-reference.md`, `hdri-sky-volume-override-reference.md`, `visual-environment-volume-override-reference.md`, `reference-fog.md`, `Understand-Fog.md`, `Post-Processing-Bloom.md`, `Post-Processing-Tonemapping.md`, `reference-override-exposure.md`, `reference-ambient-occlusion.md`, `reference-indirect-lighting-controller.md`, `Post-Processing-Vignette.md`, `Post-Processing-Color-Adjustments.md`.
- Sun binding (`RenderSettings.sun` vs brightest directional light): **NOT VERIFIED**. Related verified fact only: `HDAdditionalLightData.interactsWithSky` (above).
- `Bloom`, `Exposure`, `DepthOfField`, `ChromaticAberration` are `VolumeComponentWithQuality` types (their editors call `CopySetting(ref ..., settings.postProcessQualitySettings.XXX[level])`, `HDRP/Editor/PostProcessing/BloomEditor.cs:85-87`, `DepthOfFieldEditor.cs:246-253`), so the quality caveat in section 3.5 applies to their resolution/sample properties.

---

## 5. Materials (mostly NOT VERIFIED)

Verified:

- Shader names used by HDRP's own editor code: `"HDRP/Lit"`, `"HDRP/LayeredLit"`, `"HDRP/LitTessellation"`, `"HDRP/LayeredLitTessellation"` (`HDRP/Editor/RenderPipeline/HDRenderPipelineMenuItems.cs:26-39`).
- Material keywords referenced: `_SURFACE_TYPE_TRANSPARENT`, `_DOUBLESIDED_ON`; `Material.doubleSidedGI`; transparent range `HDRenderQueue.k_RenderQueue_Transparent.lowerBound / upperBound` (580-589).
- `HDShaderUtils.ResetMaterialKeywords(mat)`, `LitAPI.ValidateMaterial(mat)`, `LayeredLitAPI.ValidateMaterial(mat)`, `CoreEditorUtils.RemoveMaterialKeywords(mat)`, `HDUtils.GetRenderPipelineMaterialList()` (internal) + `RenderPipelineMaterial.BuildOffline(ref resources)` (26-47, 136-149, 445). Accessibility: **NOT VERIFIED**.
- Default materials without a package path: `GraphicsSettings.defaultRenderPipeline.defaultMaterial` / `.defaultParticleMaterial` / `.defaultTerrainMaterial` (`RenderPipelineAsset`, engine) and `((HDRenderPipelineAsset)GraphicsSettings.defaultRenderPipeline).GetDefaultDecalMaterial()` (editor only). Engine also has `RenderPipelineAsset.defaultLineMaterial { get; }` (public, `ENGINE/UnityEngine.CoreModule.dll`); whether HDRP overrides it: **NOT VERIFIED**.
- Upgrade entry points: `MaterialReimporter.ReimportAllMaterials()` (menu "Edit/Rendering/Materials/Upgrade HDRP Materials to Latest Version"), `MaterialUpgrader.FetchAllUpgradersForPipeline(typeof(HDRenderPipelineAsset))`, `MaterialUpgrader.ProjectContainsNonAutomaticUpgradePath(...)`, `MaterialUpgraderEditMenus.UpgradeMaterialsProject`, `MaterialUpgraderEditMenus.UpgradeMaterialsSelection` (`HDRP/Editor/RenderPipeline/HDRenderPipelineMenuItems.cs:129-134`, `HDRP/Editor/Wizard/HDWizard.cs:394-448`). Namespaces, accessibility and a per-folder/per-list conversion API (`URPtoHDRPMaterialUpgrader`, `BuiltInToHDRPMaterialUpgrader`, `UnityEditor.Rendering.RenderPipelineConverter`): **NOT VERIFIED**.
- Decals: `UnityEngine.Rendering.HighDefinition.DecalProjector` creation and default decal material path beyond `GetDefaultDecalMaterial()` / `HDRenderPipelineEditorMaterials.defaultDecalMaterial`: **NOT VERIFIED**.
- `HDRP/Lit`, `HDRP/Unlit`, `HDRP/Particles/Unlit`, `HDRP/Particles/Lit` shader presence and all property names / vertex-color keywords: **NOT VERIFIED**. Relevant doc files that exist: `HDRP/Documentation~/lit-material.md`, `lit-material-inspector-reference.md`, `unlit-material.md`, `Mask-Map-and-Detail-Map.md`, `Surface-Type.md`, `Double-Sided.md`, `shader-materials-reference.md`, `decal-projector-reference.md`, `create-decals-at-runtime.md`, `convert-project-from-built-in-render-pipeline.md`, `convert-from-built-in-convert-materials-and-shaders.md`, `convert-from-built-in-convert-project-with-hdrp-wizard.md`.

---

## 6. Terrain, LineRenderer and other gotchas

- Terrain shader `HDRP/TerrainLit`: **NOT VERIFIED**. Verified: HDRP ships a default terrain material `Runtime/RenderPipelineResources/Material/DefaultHDTerrainMaterial.mat` and overrides `defaultTerrainMaterial` (editor only); asset flag `supportTerrainHole` (default false, `RenderPipelineSettings.cs:424`); docs `HDRP/Documentation~/terrain-lit-material.md`, `terrain-lit-material-inspector-reference.md`.
- Ray tracing and terrain (doc, `Ray-Tracing-Getting-Started.md`): "Ray tracing is not compatible with the detail meshes and trees in the Terrain system. It is compatible with terrain geometry."
- LineRenderer default material: **NOT VERIFIED**. Related: asset `supportHighQualityLineRendering` (default false, `RenderPipelineSettings.cs:378`), `highQualityLineRenderingMemoryBudget`, Frame Setting `HighQualityLineRendering = 103`; docs `high-quality-line-rendering.md`.
- Render Pipeline Converter for terrain layers: **NOT VERIFIED**.
- Verified gotcha: renderers need the default rendering layer mask (HDRP menu "Add HDRP Default Layer Mask to Loaded Mesh Renderers and Terrains"): `renderer.renderingLayerMask |= RenderingLayerMask.defaultRenderingLayerMask;` and the same on `Terrain` (`HDRP/Editor/RenderPipeline/HDRenderPipelineMenuItems.cs:151-171`).
- Verified gotcha: ray-traced effects ignore meshes whose material lacks the `HDRenderPipeline` shader tag (doc above); more than 32 sub-meshes and mixed single/double-sided sub-materials produce warnings (`HDRenderPipelineMenuItems.cs:557-613`).

---

## 7. Shader Graph / VFX Graph and HDRP dependencies (verified)

- `BuiltInPackages/com.unity.shadergraph/package.json` -> `"version": "17.3.0"`; `BuiltInPackages/com.unity.visualeffectgraph/package.json` -> `"version": "17.3.0"` (directories and manifests exist).
- `HDRP/package.json` dependencies: `com.unity.modules.video: 1.0.0`, `com.unity.modules.animation: 1.0.0`, `com.unity.modules.imageconversion: 1.0.0`, `com.unity.render-pipelines.core: 17.3.0`, `com.unity.shadergraph: 17.3.0`, `com.unity.visualeffectgraph: 17.3.0`, `com.unity.render-pipelines.high-definition-config: 17.3.0`.
- `CORE/package.json` dependencies: `com.unity.burst: 1.8.14`, `com.unity.mathematics: 1.3.2`, `com.unity.ugui: 2.0.0`, `com.unity.collections: 2.4.3`, `com.unity.modules.physics: 1.0.0`, `com.unity.modules.terrain: 1.0.0`, `com.unity.modules.jsonserialize: 1.0.0`.
- `com.unity.render-pipelines.high-definition-config/package.json` dependencies: `com.unity.render-pipelines.core: 17.3.0`.
- `HDRP/Runtime/Unity.RenderPipelines.HighDefinition.Runtime.asmdef`: references `Unity.RenderPipelines.Core.Runtime`, `Unity.RenderPipelines.Core.Runtime.Shared`, `Unity.ugui`, `Unity.Mathematics`, `Unity.Collections`, `Unity.Burst`, `Unity.RenderPipelines.HighDefinition.Config.Runtime`, `Unity.Rendering.Denoising.Runtime`, `Unity.Timeline`, `Unity.VisualEffectGraph.Runtime`, `Unity.RenderPipelines.GPUDriven.Runtime`; `allowUnsafeCode: true`, `autoReferenced: true`; versionDefines `ENABLE_NVIDIA_MODULE` (com.unity.modules.nvidia 1.0.0), `ENABLE_AMD_MODULE` (com.unity.modules.amd), `ENABLE_VR_MODULE`, `ENABLE_XR_MODULE`, `ENABLE_UNITY_DENOISING_PLUGIN` (com.unity.rendering.denoising 1.0.0), `HDRP_HAS_TIMELINE`, `HDRP_1_OR_NEWER`.
- `HDRP/Runtime/AssemblyInfo.cs` `InternalsVisibleTo`: `Unity.RenderPipelines.HighDefinition.Editor`, `Unity.RenderPipelines.HighDefinition.Editor.Tests`, `Unity.RenderPipelines.HighDefinition.Runtime.Tests`, `Unity.VisualEffectGraph.EditorTests(-testable)`, `HDRP_TestRunner`, `HDRP_PlayModeTests`, `Unity.TextMeshPro.Editor`, `Unity.Industrial.Materials.AVRD.Runtime/Editor`, `Unity.RenderPipelines.Multiple_SRP.RuntimeTests/EditorTests`, `Unity.GI.ReflectionProbes.Test`.

---

## 8. Player build requirements (verified parts)

- **Color space**: Linear. Wizard check/fix `PlayerSettings.colorSpace == ColorSpace.Linear` / `PlayerSettings.colorSpace = ColorSpace.Linear` (`HDRP/Editor/Wizard/HDWizard.Configuration.cs:387-391`); pipeline logs an error in Gamma (`HDRenderPipeline.cs:854-858`).
- **Direct3D 12** is required only for ray tracing (HDRP itself accepts Direct3D11, Direct3D12, Vulkan, Metal and consoles, `HDUtils.IsSupportedGraphicDevice`, `HDRP/Runtime/RenderPipeline/Utility/HDUtils.cs:808-829`). Wizard code (`HDWizard.Configuration.cs:658-760`):
  - `PlayerSettings.GetUseDefaultGraphicsAPIs(BuildTarget.StandaloneWindows64)`, `PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.StandaloneWindows64, false)` ("Auto graphics API" off),
  - `PlayerSettings.GetGraphicsAPIs(target)`, `PlayerSettings.SetGraphicsAPIs(target, new[] { GraphicsDeviceType.Direct3D12 }.Concat(restWithoutD3D12))` so D3D12 is first,
  - runtime check `SystemInfo.graphicsDeviceType == GraphicsDeviceType.Direct3D12`; changing the editor's first API needs an editor restart; warning "DXR is supported only with DX12".
  - Doc (`Ray-Tracing-Getting-Started.md`): "In Unity 6, DirectX 12 is enabled by default."; disable Static Batching for Play Mode with ray tracing; build architecture x86_64.
- **Build target for DXR**: `StandaloneWindows64`, `GameCoreXboxSeries` or `PS5` (`IsValidBuildTarget`, 849-860). HDRP-supported standalone targets (`HDUtils.IsSupportedBuildTarget`, 842-861): StandaloneWindows, StandaloneWindows64, StandaloneLinux64, StandaloneOSX, WSAPlayer, XboxOne, GameCoreXboxOne, GameCoreXboxSeries, Switch2 (env-var gated), PS4, PS5, LinuxHeadlessSimulation; iOS is not supported.
- **Lightmaps**: wizard sets lightmap encoding High for all standalone platforms through the internal `PlayerSettings.SetLightmapEncodingQualityForPlatform` via reflection (75-77, 408-422).
- **Quality**: `QualitySettings.shadows = ShadowQuality.All`, `QualitySettings.shadowmaskMode = ShadowmaskMode.DistanceShadowmask` on every level (424-438).
- **Build-time validation** (`HDRP/Editor/BuildProcessors/HDRPPreprocessBuild.cs:30-69`, `HDRPBuildDataValidator.cs:46-68`): `BuildFailedException` when an HDRP asset or the HDRP Global Settings is missing or not at the last version, when the platform/graphics API is unsupported, or when the default volume profile cannot be initialised. It also runs `VolumeProfileUtils.TryEnsureAllOverridesForDefaultProfile` and rewrites `QualitySettings.maximumLODLevel` per quality level.
- **"Allow 'unsafe' code"** in Player Settings: no requirement found in the files read; HDRP asmdefs set `allowUnsafeCode: true` for their own assemblies. **NOT VERIFIED** for the project setting.
- **HDR display / "Use HDR Display"**: **NOT VERIFIED** (doc `HDRP/Documentation~/HDR-Output.md` exists; `HDROutputUtils.HDRDisplayInformation` in Core).
- Other player-side processors that exist but were not read: `HDRP/Editor/BuildProcessors/HDRPPreprocessShaders.cs`, `HDRPProcessScene.cs`, `SettingsStrippers/`, `ShaderStrippers/` (**NOT VERIFIED**).

---

## 9. Asset defaults worth knowing (`RenderPipelineSettings.NewDefault()`, `HDRP/Runtime/RenderPipeline/Settings/RenderPipelineSettings.cs:114-202`)

On: `supportShadowMask`, `supportSSAO`, `supportSubsurfaceScattering`, `subsurfaceScatteringAttenuation`, `supportVolumetrics`, `supportDistortion`, `supportTransparentBackface`, `supportTransparentDepthPrepass`, `supportTransparentDepthPostpass`, `supportCustomPass`, `supportVariableRateShading`, `supportDecals`, `supportSurfaceGradient`, `supportMotionVectors`, `supportScreenSpaceLensFlare`, `supportDataDrivenLensFlare`, `supportWaterExclusion`, `supportWaterDecals`, `supportProbeVolumeScenarioBlending`.
Off: `supportSSR`, `supportSSRTransparent`, `supportSSGI`, `supportRayTracing`, `supportVFXRayTracing`, `supportVolumetricClouds`, `supportLightLayers`, `supportDecalLayers`, `supportMSAA` (`msaaSampleCount = MSAASamples.None`), `supportWater`, `supportTerrainHole`, `supportRuntimeAOVAPI`, `supportHighQualityLineRendering`, `supportProbeVolumeGPUStreaming`, `supportProbeVolumeDiskStreaming`.
Values: `supportedLitShaderMode = DeferredOnly`, `supportedRayTracingMode = Both`, `colorBufferFormat = R11G11B10`, `depthBufferFormat = Auto`, `lightProbeSystem = AdaptiveProbeVolumes`, `probeVolumeSHBands = SphericalHarmonicsL1`, `dynamicResolutionSettings.enabled = false` with `advancedUpscalerNames = ["STP"]`.
