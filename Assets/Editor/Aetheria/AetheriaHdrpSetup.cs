using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.Rendering;
using UnityEditor.Build;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;

namespace Aetheria.Editor
{
    /// <summary>
    /// Turns the project into an HDRP project: pipeline asset (DLSS + hardware ray tracing on),
    /// global settings, default volume profile, quality levels, DX12 and the look of the world sky.
    /// Idempotent: safe to run again.
    /// </summary>
    public static class AetheriaHdrpSetup
    {
        public const string Folder = "Assets/HDRPDefaultResources";
        public const string AssetPath = Folder + "/HDRenderPipelineAsset.asset";
        public const string ProfilePath = Folder + "/AetheriaGlobalVolume.asset";

        [MenuItem("Aetheria/0. Configure HDRP (DLSS + Ray Tracing)")]
        public static void Configure()
        {
            Directory.CreateDirectory(Folder);

            var asset = AssetDatabase.LoadAssetAtPath<HDRenderPipelineAsset>(AssetPath);
            if (asset == null)
            {
                asset = ScriptableObject.CreateInstance<HDRenderPipelineAsset>();
                AssetDatabase.CreateAsset(asset, AssetPath);
            }

            var s = asset.currentPlatformRenderPipelineSettings;

            // Hardware ray tracing and the rasterized effects that gate it
            s.supportRayTracing = true;
            s.supportedRayTracingMode = RenderPipelineSettings.SupportedRayTracingMode.Both;
            s.supportSSR = true;
            s.supportSSRTransparent = true;
            s.supportSSGI = true;
            s.supportSSAO = true;
            s.hdShadowInitParams.supportScreenSpaceShadows = true;
            s.supportVFXRayTracing = false;                 // particles stay raster for performance
            s.supportDecals = true;
            s.supportVolumetrics = true;
            s.supportMotionVectors = true;

            // DLSS through dynamic resolution; STP stays as the fallback for non-NVIDIA machines
            var drs = s.dynamicResolutionSettings;
            drs.enabled = true;
            drs.advancedUpscalerNames = new List<string> { "DLSS", "FSR2", "STP" };
            drs.DLSSPerfQualitySetting = (uint)UnityEngine.NVIDIA.DLSSQuality.MaximumQuality;
            drs.DLSSUseOptimalSettings = true;
            drs.DLSSInjectionPoint = DynamicResolutionHandler.UpsamplerScheduleType.BeforePost;
            drs.maxPercentage = 100f;
            drs.minPercentage = 66f;
            s.dynamicResolutionSettings = drs;

            asset.currentPlatformRenderPipelineSettings = s;
            EditorUtility.SetDirty(asset);

            GraphicsSettings.defaultRenderPipeline = asset;
            QualitySettings.renderPipeline = null;

            PlayerSettings.colorSpace = ColorSpace.Linear;
            PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.StandaloneWindows64, false);
            var apis = new List<GraphicsDeviceType> { GraphicsDeviceType.Direct3D12 };
            foreach (var api in PlayerSettings.GetGraphicsAPIs(BuildTarget.StandaloneWindows64))
                if (api != GraphicsDeviceType.Direct3D12) apis.Add(api);
            PlayerSettings.SetGraphicsAPIs(BuildTarget.StandaloneWindows64, apis.ToArray());
            PlayerSettings.SetScriptingBackend(NamedBuildTarget.Standalone, ScriptingImplementation.IL2CPP);
            PlayerSettings.SetApiCompatibilityLevel(NamedBuildTarget.Standalone, ApiCompatibilityLevel.NET_Standard);
            QualitySettings.ForEach(() =>
            {
                QualitySettings.renderPipeline = null;
                QualitySettings.shadows = ShadowQuality.All;
                QualitySettings.shadowmaskMode = ShadowmaskMode.DistanceShadowmask;
                QualitySettings.vSyncCount = 0;
            });

            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            Debug.Log($"[HDRP] Pipeline configured at {AssetPath}: ray tracing on, upscalers {string.Join("/", drs.advancedUpscalerNames)}, DX12 first.");
        }

        /// <summary>The project's look: sky, fog, exposure, bloom, tonemapping, contact shadows, RT settings.</summary>
        [MenuItem("Aetheria/0b. Build Look & Volume Profile")]
        public static void BuildLook()
        {
            var asset = AssetDatabase.LoadAssetAtPath<HDRenderPipelineAsset>(AssetPath);
            if (asset == null) { Configure(); asset = AssetDatabase.LoadAssetAtPath<HDRenderPipelineAsset>(AssetPath); }

            var profile = AssetDatabase.LoadAssetAtPath<VolumeProfile>(ProfilePath);
            if (profile == null)
            {
                profile = VolumeProfileFactory.CreateVolumeProfileAtPath(ProfilePath);
            }
            asset.volumeProfile = profile;
            EditorUtility.SetDirty(asset);

            // Environment
            var env = Get<VisualEnvironment>(profile);
            env.skyType.value = SkySettings.GetUniqueID<GradientSky>();
            env.skyAmbientMode.value = SkyAmbientMode.Dynamic;
            var grad = Get<GradientSky>(profile);
            grad.top.Override(new Color(0.12f, 0.26f, 0.52f));
            grad.middle.Override(new Color(0.55f, 0.66f, 0.80f));
            grad.bottom.Override(new Color(0.92f, 0.72f, 0.50f));
            grad.gradientDiffusion.Override(1.4f);

            var fog = Get<Fog>(profile);
            fog.enabled.Override(true);
            fog.enableVolumetricFog.Override(true);
            fog.baseHeight.Override(0f);
            fog.maximumHeight.Override(220f);
            fog.meanFreePath.Override(420f);
            fog.albedo.Override(new Color(0.72f, 0.78f, 0.88f));
            fog.globalLightProbeDimmer.Override(0.55f);
            fog.anisotropy.Override(0.45f);
            fog.depthExtent.Override(260f);
            fog.sliceDistributionUniformity.Override(0.5f);

            var exposure = Get<Exposure>(profile);
            exposure.mode.Override(ExposureMode.Automatic);
            exposure.meteringMode.Override(MeteringMode.CenterWeighted);
            exposure.compensation.Override(0.35f);
            exposure.limitMin.Override(-6f);
            exposure.limitMax.Override(14f);

            var tone = Get<Tonemapping>(profile);
            tone.mode.Override(TonemappingMode.ACES);

            var bloom = Get<Bloom>(profile);
            bloom.intensity.Override(0.35f);
            bloom.threshold.Override(0.95f);
            bloom.scatter.Override(0.72f);
            bloom.quality.levelAndOverride = ((int)ScalableSettingLevelParameter.Level.High, true);

            var ca = Get<ColorAdjustments>(profile);
            ca.contrast.Override(12f);
            ca.saturation.Override(8f);
            ca.postExposure.Override(0f);

            var vig = Get<Vignette>(profile);
            vig.intensity.Override(0.22f);
            vig.smoothness.Override(0.42f);

            var shadow = Get<HDShadowSettings>(profile);
            shadow.maxShadowDistance.Override(240f);
            shadow.cascadeShadowSplitCount.Override(4);

            var contact = Get<ContactShadows>(profile);
            contact.enable.Override(true);
            contact.length.Override(0.09f);
            contact.opacity.Override(0.85f);
            contact.maxDistance.Override(40f);

            // Ray tracing look
            var ssr = Get<ScreenSpaceReflection>(profile);
            ssr.quality.levelAndOverride = ((int)ScalableSettingLevelParameter.Level.High, true);
            ssr.enabled.Override(true);
            ssr.enabledTransparent.Override(true);
            ssr.tracing.Override(RayCastingMode.RayTracing);
            ssr.mode.Override(RayTracingMode.Quality);
            ssr.minSmoothness = 0.3f;
            ssr.denoise = true;

            var gi = Get<GlobalIllumination>(profile);
            gi.quality.levelAndOverride = ((int)ScalableSettingLevelParameter.Level.High, true);
            gi.enable.Override(true);
            gi.tracing.Override(RayCastingMode.RayTracing);
            gi.mode.Override(RayTracingMode.Quality);
            gi.denoise = true;

            var ao = Get<ScreenSpaceAmbientOcclusion>(profile);
            ao.quality.levelAndOverride = ((int)ScalableSettingLevelParameter.Level.High, true);
            ao.rayTracing.Override(true);
            ao.intensity.Override(0.9f);
            ao.radius.Override(1.6f);
            ao.directLightingStrength.Override(0.25f);

            var rts = Get<RayTracingSettings>(profile);
            rts.rayBias.Override(0.001f);
            rts.extendShadowCulling.Override(true);

            var indirect = Get<IndirectLightingController>(profile);
            indirect.indirectDiffuseLightingMultiplier.Override(1.1f);
            indirect.reflectionLightingMultiplier.Override(1.2f);

            EditorUtility.SetDirty(profile);
            AssetDatabase.SaveAssets();

            VolumeProfileUtils.EnsureAllOverridesForDefaultProfile(profile);
            if (VolumeManager.instance != null && VolumeManager.instance.isInitialized)
                VolumeManager.instance.SetGlobalDefaultProfile(profile);
            Debug.Log("[HDRP] Look profile built (gradient sky, volumetric fog, RT reflections/GI/AO, ACES).");
        }

        private static T Get<T>(VolumeProfile profile) where T : VolumeComponent
        {
            if (profile.TryGet<T>(out var c)) return c;
            return VolumeProfileFactory.CreateVolumeComponent<T>(profile, false, true);
        }

    }
}
