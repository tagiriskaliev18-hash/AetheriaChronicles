using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;

namespace Aetheria
{
    /// <summary>
    /// HDRP implementation behind <see cref="AetheriaRender"/>. All engine-version-specific names live here
    /// and nowhere else: gameplay asks for "a lit material" or "a point light" and stays pipeline-agnostic.
    /// </summary>
    public static class AetheriaRenderImpl
    {
        private static bool _prepared;
        private static bool _upscalerUltra = true;
        private static Volume _cachedSkyVolume;

        private static void Prepare()
        {
            if (_prepared) return;
            _prepared = true;
            // HDRP reads the light's unit and colour temperature; the engine defaults are fine.
            GraphicsSettings.lightsUseLinearIntensity = true;
        }

        internal static Volume FindHighestPriorityGlobalVolume(bool requireGradientSky = false)
        {
            if (requireGradientSky && _cachedSkyVolume != null && _cachedSkyVolume.isActiveAndEnabled && _cachedSkyVolume.isGlobal)
            {
                var p = Application.isPlaying ? _cachedSkyVolume.profile : _cachedSkyVolume.sharedProfile;
                if (p != null && p.Has<GradientSky>()) return _cachedSkyVolume;
            }

            Volume best = null;
            float bestPri = float.NegativeInfinity;
            var volumes = Object.FindObjectsByType<Volume>(FindObjectsSortMode.None);
            for (int i = 0; i < volumes.Length; i++)
            {
                var v = volumes[i];
                if (v == null || !v.isActiveAndEnabled || !v.isGlobal) continue;
                var p = v.sharedProfile != null ? v.sharedProfile : v.profile;
                if (p == null) continue;
                if (requireGradientSky && !p.Has<GradientSky>()) continue;
                if (best == null || v.priority > bestPri)
                {
                    best = v;
                    bestPri = v.priority;
                }
            }
            if (requireGradientSky && best != null) _cachedSkyVolume = best;
            return best;
        }

        // ------------------------------------------------------------------ lights

        /// <summary>Sun strength is authored as 0..4; HDRP reads it in lux.</summary>
        public const float SunUnitScale = 10000f;

        /// <summary>Points the sun with an authored strength and colour, in the pipeline's own unit.</summary>
        public static void StyleSun(Light sun, float authoredIntensity, Color color)
        {
            Prepare();
            if (sun == null) return;
            sun.color = color;
            sun.intensity = Mathf.Max(0f, authoredIntensity) * SunUnitScale;
        }

        /// <summary>True once the render pipeline exists. It is created after the scene's Start,
        /// so the sky of the opening moment may have to wait for its first frame.</summary>
        public static bool SkyReady =>
            FindHighestPriorityGlobalVolume(requireGradientSky: true) != null ||
            (VolumeManager.instance != null && VolumeManager.instance.stack != null);

        /// <summary>
        /// Retints the sky gradient; the fog takes the colour of the horizon so land fades into its own sky.
        /// Returns false while there is no sky to tint yet.
        /// </summary>
        public static bool SetSkyColors(Color top, Color middle, Color bottom, Color fogAlbedo)
        {
            bool updated = false;
            var vol = FindHighestPriorityGlobalVolume(requireGradientSky: true);
            if (vol != null)
            {
                var prof = Application.isPlaying ? vol.profile : vol.sharedProfile;
                if (prof != null)
                {
                    if (prof.TryGet<GradientSky>(out var sky))
                    {
                        sky.top.Override(top);
                        sky.middle.Override(middle);
                        sky.bottom.Override(bottom);
                        updated = true;
                    }
                    if (prof.TryGet<Fog>(out var fog))
                    {
                        fog.albedo.Override(fogAlbedo);
                    }
                    VolumeManager.instance?.OnVolumeProfileChanged(prof);
                }
            }

            var stack = VolumeManager.instance?.stack;
            if (stack != null)
            {
                var sky = stack.GetComponent<GradientSky>();
                if (sky != null)
                {
                    sky.top.value = top;
                    sky.middle.value = middle;
                    sky.bottom.value = bottom;
                    updated = true;
                }
                var fog = stack.GetComponent<Fog>();
                if (fog != null) fog.albedo.value = fogAlbedo;
            }
            return updated;
        }

        private static void ApplyUpscaler(HDAdditionalCameraData hd, bool ultra)
        {
            if (hd == null) return;
            // Ultra: DLSS in DLAA mode (native resolution, the network only anti-aliases) on RTX cards.
            // Without DLSS the frame stays at full resolution with TAA: dynamic resolution without an
            // upscaler used to drop to 66 % and smear every texture.
            bool useDlss = ultra && HDDynamicResolutionPlatformCapabilities.DLSSDetected;
            bool useFsr2 = !ultra && !useDlss && HDDynamicResolutionPlatformCapabilities.FSR2Detected;
            hd.allowDynamicResolution = useDlss || useFsr2;
            hd.antialiasing = HDAdditionalCameraData.AntialiasingMode.TemporalAntialiasing;
            hd.TAAQuality = HDAdditionalCameraData.TAAQualityLevel.High;
            hd.allowDeepLearningSuperSampling = useDlss;
            hd.deepLearningSuperSamplingUseCustomQualitySettings = true;
            hd.deepLearningSuperSamplingQuality = 4;   // DLAA
            hd.deepLearningSuperSamplingUseOptimalSettings = false;
            hd.allowFidelityFX2SuperResolution = useFsr2;
            hd.fidelityFX2SuperResolutionUseCustomQualitySettings = false;
            hd.fidelityFX2SuperResolutionUseOptimalSettings = true;
        }

        /// <summary>Ultra lets DLSS reconstruct the frame; the performance tier falls back to the lighter upscaler.</summary>
        public static void SetUpscalerEnabled(bool ultra)
        {
            _upscalerUltra = ultra;
            var cam = Camera.main;
            if (cam == null) return;
            var hd = cam.GetComponent<HDAdditionalCameraData>();
            if (hd == null) return;
            ApplyUpscaler(hd, ultra);
        }

        public static void PrepareFxLight(Light l)
        {
            Prepare();
            if (l == null) return;
            var hd = l.GetComponent<HDAdditionalLightData>() ?? l.gameObject.AddComponent<HDAdditionalLightData>();
            l.type = LightType.Point;
            l.shadows = LightShadows.None;
            l.lightUnit = LightUnit.Lumen;
            l.colorTemperature = 6500f;
            l.useColorTemperature = false;
            hd.affectsVolumetric = true;
            hd.lightDimmer = 1f;
            hd.shadowDimmer = 1f;
            hd.useContactShadow.useOverride = true;
            hd.useContactShadow.@override = false;
        }

        /// <summary>Pooled effect light: range in metres, intensity in lumens (HDRP-correct power).</summary>
        public static void StyleFxLight(Light l, Color color, float range, float intensity)
        {
            if (l == null) return;
            l.enabled = true;
            l.color = color;
            l.lightUnit = LightUnit.Lumen;
            l.intensity = Mathf.Max(0f, intensity * 220f);   // authored values read as "candela-ish"; lumens need this scale
            l.range = range;
        }

        public static Light CreatePointLight(Transform parent, Vector3 localPos, Color color, float range, float intensity, bool shadows = false)
        {
            Prepare();
            var go = new GameObject("PointLight");
            go.transform.SetParent(parent, false);
            go.transform.localPosition = localPos;
            var l = go.AddComponent<Light>();
            l.type = LightType.Point;
            l.shadows = shadows ? LightShadows.Soft : LightShadows.None;
            l.lightUnit = LightUnit.Lumen;
            l.color = color;
            l.intensity = Mathf.Max(0f, intensity * 220f);
            l.range = range;
            var hd = go.AddComponent<HDAdditionalLightData>();
            hd.affectsVolumetric = true;
            hd.useContactShadow.useOverride = true;
            hd.useContactShadow.@override = false;
            if (shadows)
            {
                hd.shadowUpdateMode = ShadowUpdateMode.EveryFrame;
            }
            return l;
        }

        /// <summary>
        /// Turns an authored scene light (sun, torches) into an HDRP light with sane units.
        /// Lights with unscaled intensity (&lt;50) are converted even if HDAdditionalLightData already exists.
        /// </summary>
        public static void ConfigureSceneLight(Light l, bool rayTracedShadows)
        {
            Prepare();
            if (l == null) return;
            bool fresh = l.GetComponent<HDAdditionalLightData>() == null;
            var hd = l.GetComponent<HDAdditionalLightData>() ?? l.gameObject.AddComponent<HDAdditionalLightData>();
            if (l.type == LightType.Directional)
            {
                l.lightUnit = LightUnit.Lux;
                if (fresh || l.intensity < 50f) l.intensity = Mathf.Clamp(l.intensity, 1f, 20f) * SunUnitScale;   // 2.8 -> 28 000 lx (golden hour)
                hd.affectsVolumetric = true;
                hd.useScreenSpaceShadows = rayTracedShadows;
                hd.useRayTracedShadows = rayTracedShadows;
                hd.angularDiameter = 0.6f;
            }
            else
            {
                l.lightUnit = LightUnit.Lumen;
                if (fresh || l.intensity < 50f) l.intensity = Mathf.Clamp(l.intensity, 0.5f, 20f) * 220f;
                hd.affectsVolumetric = true;
            }
        }

        // ------------------------------------------------------------------ camera

        public static void ConfigureCamera(Camera cam, bool main)
        {
            Prepare();
            if (cam == null) return;
            var hd = cam.GetComponent<HDAdditionalCameraData>() ?? cam.gameObject.AddComponent<HDAdditionalCameraData>();
            hd.clearColorMode = HDAdditionalCameraData.ClearColorMode.Sky;
            hd.clearDepth = true;
            hd.antialiasing = HDAdditionalCameraData.AntialiasingMode.TemporalAntialiasing;
            hd.TAAQuality = HDAdditionalCameraData.TAAQualityLevel.High;
            hd.dithering = true;
            hd.stopNaNs = true;
            if (main)
            {
                ApplyUpscaler(hd, _upscalerUltra);
            }
            cam.allowHDR = true;
            cam.allowMSAA = false;
        }

        /// <summary>Camera that draws the world map into a texture: no AA history, no dynamic resolution.</summary>
        public static void ConfigureOffscreenCamera(Camera cam)
        {
            Prepare();
            if (cam == null) return;
            var hd = cam.GetComponent<HDAdditionalCameraData>() ?? cam.gameObject.AddComponent<HDAdditionalCameraData>();
            hd.clearColorMode = HDAdditionalCameraData.ClearColorMode.Sky;
            hd.antialiasing = HDAdditionalCameraData.AntialiasingMode.FastApproximateAntialiasing;
            hd.allowDynamicResolution = false;
            hd.allowDeepLearningSuperSampling = false;
            cam.allowHDR = true;
            cam.allowMSAA = false;
        }
    }
}
