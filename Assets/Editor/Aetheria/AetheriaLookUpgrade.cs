using System.Collections.Generic;
using UnityEditor;
using UnityEditor.Rendering;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;

namespace Aetheria.Editor
{
    /// <summary>
    /// Pushes the HDRP volume profile from "correct" to "epic": the golden-hour look the world was
    /// designed for. Every setting here is a volume override, so it can be dialled back per-biome later
    /// without touching code.
    ///
    /// The reference is a Crimson Desert sunset: warm low sun, long shadows, thick valley haze that
    /// catches light shafts, a saturated sky that goes violet at the top, and enough bloom that the sun
    /// and the magic read as sources rather than decals.
    /// </summary>
    public static class AetheriaLookUpgrade
    {
        private const string ProfilePath = "Assets/HDRPDefaultResources/AetheriaGlobalVolume.asset";
        private const string ScenePath = "Assets/Scenes/AetheriaMain.unity";

        [MenuItem("Aetheria/6. Upgrade Look (epic atmosphere)")]
        public static void UpgradeLook()
        {
            var profile = AssetDatabase.LoadAssetAtPath<VolumeProfile>(ProfilePath);
            if (profile == null)
            {
                Debug.LogError($"[Look] Missing {ProfilePath} — run 'Aetheria/0b. Build Look & Volume Profile' first.");
                return;
            }

            // ---- sky: three-stop gradient, warm horizon, violet zenith ----------------------
            var grad = Get<GradientSky>(profile);
            grad.top.Override(new Color(0.055f, 0.115f, 0.325f));      // deep blue-violet
            grad.middle.Override(new Color(0.42f, 0.55f, 0.78f));      // cool mid sky
            grad.bottom.Override(new Color(0.98f, 0.68f, 0.38f));      // molten horizon
            grad.gradientDiffusion.Override(2.2f);

            var env = Get<VisualEnvironment>(profile);
            env.skyAmbientMode.value = SkyAmbientMode.Dynamic;

            // ---- volumetric fog: the single biggest "epic vista" lever ----------------------
            var fog = Get<Fog>(profile);
            fog.enabled.Override(true);
            fog.enableVolumetricFog.Override(true);
            fog.baseHeight.Override(0f);
            fog.maximumHeight.Override(320f);
            fog.meanFreePath.Override(260f);            // denser than 420: haze pools in the valley
            fog.albedo.Override(new Color(0.86f, 0.78f, 0.70f));   // warm scattering, not grey
            fog.globalLightProbeDimmer.Override(0.62f);
            fog.anisotropy.Override(0.72f);              // strong forward scatter -> visible sun shafts
            fog.depthExtent.Override(420f);
            fog.sliceDistributionUniformity.Override(0.62f);
            fog.enableVolumetricFog.Override(true);
            fog.colorMode.value = FogColorMode.SkyColor;

            // ---- exposure & tone ---------------------------------------------------------
            var exposure = Get<Exposure>(profile);
            exposure.mode.Override(ExposureMode.Automatic);
            exposure.meteringMode.Override(MeteringMode.CenterWeighted);
            exposure.compensation.Override(0.55f);
            exposure.limitMin.Override(-7f);
            exposure.limitMax.Override(15f);

            var tone = Get<Tonemapping>(profile);
            tone.mode.Override(TonemappingMode.ACES);

            // ---- bloom: makes the sun, crystals and magic glow ---------------------------
            var bloom = Get<Bloom>(profile);
            bloom.intensity.Override(0.62f);
            bloom.threshold.Override(0.88f);
            bloom.scatter.Override(0.78f);
            bloom.anamorphic.Override(false);
            bloom.quality.levelAndOverride = ((int)ScalableSettingLevelParameter.Level.High, true);

            // ---- grade: warm highlights, cool shadows, filmic contrast -------------------
            var ca = Get<ColorAdjustments>(profile);
            ca.postExposure.Override(0f);
            ca.contrast.Override(18f);
            ca.saturation.Override(14f);
            ca.colorFilter.Override(new Color(1.02f, 0.99f, 0.95f));

            var wb = Get<WhiteBalance>(profile);
            wb.temperature.Override(6f);        // a touch warm
            wb.tint.Override(2f);

            var curves = Get<ColorCurves>(profile);
            // Lift the shadows slightly blue, roll the highlights warm — the cinematic "teal & orange".
            curves.red.Override(new TextureCurve(Curve(0.02f, 1f), 0f, false, new Vector2(0f, 0f)));
            curves.green.Override(new TextureCurve(Curve(0.015f, 1f), 0f, false, new Vector2(0f, 0f)));
            curves.blue.Override(new TextureCurve(Curve(0.06f, 0.97f), 0f, false, new Vector2(0f, 0f)));

            var split = Get<SplitToning>(profile);
            split.shadows.Override(new Color(0.16f, 0.24f, 0.38f));
            split.highlights.Override(new Color(0.48f, 0.36f, 0.20f));
            split.balance.Override(-12f);

            var vig = Get<Vignette>(profile);
            vig.intensity.Override(0.30f);
            vig.smoothness.Override(0.46f);

            var grain = Get<FilmGrain>(profile);
            grain.type.value = FilmGrainLookup.Thin1;
            grain.intensity.Override(0.14f);
            grain.response.Override(0.8f);

            // ---- depth of field is off: it fights the vista shots ------------------------
            var dof = Get<DepthOfField>(profile);
            dof.focusDistance.Override(60f);

            // ---- ambient occlusion & GI: contact shadowing on the new vegetation ---------
            var ao = Get<ScreenSpaceAmbientOcclusion>(profile);
            ao.intensity.Override(1.1f);
            ao.radius.Override(2.0f);
            ao.directLightingStrength.Override(0.35f);

            var indirect = Get<IndirectLightingController>(profile);
            indirect.indirectDiffuseLightingMultiplier.Override(1.35f);
            indirect.reflectionLightingMultiplier.Override(1.30f);

            EditorUtility.SetDirty(profile);
            AssetDatabase.SaveAssets();
            Debug.Log("[Look] Epic atmosphere applied: dense warm volumetric fog, 3-stop sky, ACES + split-tone grade, strong bloom.");
        }

        /// <summary>Sets the sun to the golden-hour angle the whole palette is tuned for, and saves the scene.</summary>
        [MenuItem("Aetheria/7. Set Golden Hour Sun")]
        public static void SetGoldenHour()
        {
            var scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            if (!scene.IsValid()) { Debug.LogError("[Look] Cannot open scene."); return; }

            var sunGo = GameObject.Find("AetheriaSunLight");
            if (sunGo == null) { Debug.LogError("[Look] AetheriaSunLight not found."); return; }

            var sun = sunGo.GetComponent<Light>();
            if (sun != null)
            {
                sun.type = LightType.Directional;
                sun.color = new Color(1.0f, 0.72f, 0.44f);
                sun.intensity = 3.4f;
                sun.shadows = LightShadows.Soft;
                sun.shadowStrength = 0.92f;
            }
            // Low western sun: long raking shadows across the valley, sun in frame from the village.
            sunGo.transform.rotation = Quaternion.Euler(11f, 168f, 0f);

            var gen = Object.FindFirstObjectByType<AetheriaWorldGenerator>();
            if (gen != null)
            {
                gen.SunLight = sun;
                gen.TimeOfDay = 0.68f;
            }

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, ScenePath);
            Debug.Log("[Look] Golden-hour sun set: pitch 11 deg, warm 3.4 intensity, soft shadows.");
        }

        private static T Get<T>(VolumeProfile profile) where T : VolumeComponent
        {
            if (profile.TryGet<T>(out var c)) return c;
            return VolumeProfileFactory.CreateVolumeComponent<T>(profile, false, true);
        }

        /// <summary>Straight line from (0, shadowLift) to (1, highlightGain), as a sampled curve.</summary>
        private static AnimationCurve Curve(float shadowLift, float highlightGain)
        {
            return AnimationCurve.Linear(0f, shadowLift, 1f, highlightGain);
        }
    }
}
