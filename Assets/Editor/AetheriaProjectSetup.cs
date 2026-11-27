using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEditor.Animations;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;
using UnityEngine.SceneManagement;

namespace Aetheria.Editor
{
    /// <summary>
    /// End-to-end Editor Setup for Aetheria: Chronicles of the Drake (Unity 6.3 HDRP 17.3).
    /// Configures Normal Maps & Sobel derivations, builds HDRP PBR Materials & 5 TerrainLayers,
    /// builds AnimatorControllers & stages Hero/Weapon/Cosmetic/Creature prefabs into Resources/,
    /// sculpts the 4km 5-biome Terrain, populates landmarks & quests, and builds the Windows Standalone .exe.
    /// </summary>
    public static class AetheriaProjectSetup
    {
        private const string ScenePath = "Assets/Scenes/AetheriaMain.unity";
        private const string BuildExePath = "Build/Aetheria.exe";
        private const string AnimatorsFolder = "Assets/Generated/Animators";

        [MenuItem("Aetheria/1. Setup Project, Assets & Scene")]
        public static void SetupProjectAndScene()
        {
            Debug.Log("[AetheriaSetup] Starting full HDRP project & scene assembly...");
            EnsureFolders();
            EnsureShaderTemplates();
            ConfigureNormalMapImporters();
            var materials = BuildPbrMaterials();
            var layers = BuildTerrainLayers(materials);
            BuildAnimatorControllers();
            StageResourcesPrefabs(materials);
            BuildMainScene(materials, layers);
            Debug.Log("[AetheriaSetup] Scene & Resources setup complete!");
        }

        [MenuItem("Aetheria/2. Build Windows Standalone (.exe)")]
        public static void BuildWindowsExecutable()
        {
            if (EditorApplication.isPlaying) throw new InvalidOperationException("Leave Play Mode before building.");
            if (!File.Exists(ScenePath))
                throw new FileNotFoundException($"Main scene not found at '{ScenePath}'. Run 'Aetheria/1. Setup Project, Assets & Scene' first.", ScenePath);

            AetheriaDemoAnimations.Prepare();
            PrepareDemoScene();
            string fullExePath = Path.GetFullPath(BuildExePath);
            Directory.CreateDirectory(Path.GetDirectoryName(fullExePath)!);

            // Configure DX12 first for HDRP Windows Standalone
            PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.StandaloneWindows64, false);
            var apis = new List<GraphicsDeviceType> { GraphicsDeviceType.Direct3D12 };
            foreach (var api in PlayerSettings.GetGraphicsAPIs(BuildTarget.StandaloneWindows64))
                if (api != GraphicsDeviceType.Direct3D12) apis.Add(api);
            PlayerSettings.SetGraphicsAPIs(BuildTarget.StandaloneWindows64, apis.ToArray());

            // IL2CPP needs its Windows player module installed in the Editor; without it the build
            // aborts outright, so fall back to Mono rather than shipping nothing.
            var backend = PlayerSettings.GetScriptingBackend(NamedBuildTarget.Standalone);
            bool il2cppReady = Directory.Exists(Path.Combine(EditorApplication.applicationContentsPath,
                "PlaybackEngines/WindowsStandaloneSupport/Variations/win64_player_il2cpp_nondevelopment"));
            if (backend == ScriptingImplementation.IL2CPP && !il2cppReady)
            {
                Debug.LogWarning("[AetheriaSetup] IL2CPP Windows module is not installed in this Editor; building with Mono instead. " +
                                 "Install the 'Windows Build Support (IL2CPP)' module to get the faster IL2CPP player.");
                PlayerSettings.SetScriptingBackend(NamedBuildTarget.Standalone, ScriptingImplementation.Mono2x);
            }

            var scenes = new[] { ScenePath };
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
            var opts = new BuildPlayerOptions
            {
                scenes = scenes,
                locationPathName = fullExePath,
                target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.CompressWithLz4
            };
            var report = BuildPipeline.BuildPlayer(opts);
            Debug.Log($"[AetheriaSetup] Build finished: result={report.summary.result} errors={report.summary.totalErrors} size={report.summary.totalSize / (1024 * 1024)}MB backend={PlayerSettings.GetScriptingBackend(NamedBuildTarget.Standalone)} -> {fullExePath}");
            if (report.summary.result != BuildResult.Succeeded)
            {
                var errSb = new StringBuilder();
                errSb.AppendLine($"Windows64 build failed: result={report.summary.result}, errors={report.summary.totalErrors}, warnings={report.summary.totalWarnings}");
                if (report.steps != null)
                {
                    foreach (var step in report.steps)
                    {
                        foreach (var msg in step.messages)
                        {
                            if (msg.type == LogType.Error || msg.type == LogType.Exception || msg.type == LogType.Assert)
                                errSb.AppendLine($"  [{step.name}] {msg.content}");
                        }
                    }
                }
                throw new Exception(errSb.ToString());
            }
        }

        public static void VerifyDemoInEditor()
        {
            var args = Environment.GetCommandLineArgs();
            if (!args.Contains("-verify-demo"))
                throw new ArgumentException("Run VerifyDemoInEditor with -verify-demo.");
            AetheriaDemoAnimations.Prepare();
            PrepareDemoScene();
            EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            EditorApplication.EnterPlaymode();
        }

        [MenuItem("Aetheria/Prepare Demo Scene")]
        public static void PrepareDemoScene()
        {
            var scene = SceneManager.GetSceneByPath(ScenePath);
            bool openedHere = !scene.isLoaded;
            if (openedHere) scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Additive);
            try
            {
                var objects = scene.GetRootGameObjects().SelectMany(root => root.GetComponentsInChildren<Transform>(true));
                var bridge = objects.FirstOrDefault(t => t.name == "MirrorLakeStoneBridge");
                bool dirty = false;
                if (bridge != null)
                {
                    foreach (var filter in bridge.GetComponentsInChildren<MeshFilter>(true))
                    {
                        if (filter.sharedMesh == null || filter.GetComponent<Collider>() != null) continue;
                        filter.gameObject.AddComponent<MeshCollider>().sharedMesh = filter.sharedMesh;
                        dirty = true;
                    }
                }
                var enemy = objects.FirstOrDefault(t => t.name == "CalderaBrute")?.GetComponent<EnemyAndBossAI>();
                if (enemy != null && !enemy.Ranged)
                {
                    enemy.Ranged = true;
                    enemy.AttackRange = 10f;
                    PrefabUtility.RecordPrefabInstancePropertyModifications(enemy);
                    dirty = true;
                }
                if (dirty)
                {
                    EditorSceneManager.MarkSceneDirty(scene);
                    if (!EditorSceneManager.SaveScene(scene))
                        throw new IOException($"Could not save demo scene at {ScenePath}.");
                }
            }
            finally
            {
                if (openedHere) EditorSceneManager.CloseScene(scene, true);
            }
        }

        /// <summary>The HDRP Windows player: Scenes/AetheriaMain.unity -> Build/Aetheria.exe (release, IL2CPP, DX12).</summary>
        [MenuItem("Aetheria/9. Build Windows Player")]
        public static void BuildWindowsPlayer() => BuildWindowsExecutable();

        public static void SetupProjectAndBuildAll()
        {
            SetupProjectAndScene();
            BuildWindowsExecutable();
        }

        private static void EnsureFolders()
        {
            string[] dirs =
            {
                "Assets/Scenes",
                "Assets/Generated/Normals",
                "Assets/Generated/Materials",
                "Assets/Generated/TerrainLayers",
                AnimatorsFolder,
                "Assets/Resources/Materials",
                "Assets/Resources/Heroes",
                "Assets/Resources/Weapons",
                "Assets/Resources/Cosmetics",
                "Assets/Resources/Creatures",
                "Assets/Resources/Audio",
                "Assets/Resources/UI",
                "Assets/Resources/Grimoire",
                "Assets/Resources/Fonts"
            };
            foreach (var d in dirs)
            {
                if (!Directory.Exists(d)) Directory.CreateDirectory(d);
            }
            PlayerSettings.productName = "Aetheria: Chronicles of the Drake";
            PlayerSettings.companyName = "Mind Aetheria Studio";
            PlayerSettings.defaultScreenWidth = 1920;
            PlayerSettings.defaultScreenHeight = 1080;
            PlayerSettings.fullScreenMode = FullScreenMode.Windowed;
            PlayerSettings.runInBackground = true;
            AssetDatabase.Refresh();
        }

        private static void EnsureShaderTemplates()
        {
            var litSh = Shader.Find("HDRP/Lit") ?? Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
            var litMat = new Material(litSh) { color = Color.white };
            if (litMat.HasProperty("_BaseColor")) litMat.SetColor("_BaseColor", Color.white);
            if (litMat.HasProperty("_Smoothness")) litMat.SetFloat("_Smoothness", 0.36f);
            if (litMat.HasProperty("_SmoothnessRemapMax")) litMat.SetFloat("_SmoothnessRemapMax", 0.36f);
            AetheriaHdrpConvert.ValidateHdMaterial(litMat);
            SaveOrReplaceAsset(litMat, "Assets/Resources/Materials/URP_Lit_Template.mat");

            // Soft volumetric radial falloff baked into BOTH RGB and Alpha so Additive (One, One) NEVER draws hard square edges
            const int S = 128;
            var glowTex = new Texture2D(S, S, TextureFormat.RGBA32, false);
            for (int y = 0; y < S; y++)
            {
                for (int x = 0; x < S; x++)
                {
                    float u = (x / (float)(S - 1)) * 2f - 1f;
                    float v = (y / (float)(S - 1)) * 2f - 1f;
                    float dist = Mathf.Sqrt(u * u + v * v);
                    float t = Mathf.Clamp01(1f - dist / 0.88f);
                    float a = t * t * t;
                    glowTex.SetPixel(x, y, new Color(a, a, a, a));
                }
            }
            glowTex.Apply();
            string glowPath = "Assets/Generated/SoftGlowParticle.png";
            File.WriteAllBytes(glowPath, glowTex.EncodeToPNG());
            UnityEngine.Object.DestroyImmediate(glowTex);
            AssetDatabase.ImportAsset(glowPath, ImportAssetOptions.ForceUpdate);
            if (AssetImporter.GetAtPath(glowPath) is TextureImporter glowTi)
            {
                glowTi.alphaSource = TextureImporterAlphaSource.FromInput;
                glowTi.alphaIsTransparency = true;
                glowTi.wrapMode = TextureWrapMode.Clamp;
                glowTi.mipmapEnabled = false;
                glowTi.SaveAndReimport();
            }
            var loadedGlow = AssetDatabase.LoadAssetAtPath<Texture2D>(glowPath);

            var pSh = Shader.Find("HDRP/Unlit") ?? Shader.Find("Universal Render Pipeline/Particles/Unlit") ?? Shader.Find("Particles/Standard Unlit");
            var pMat = new Material(pSh) { color = Color.white };
            if (loadedGlow != null)
            {
                pMat.mainTexture = loadedGlow;
                if (pMat.HasProperty("_BaseMap")) pMat.SetTexture("_BaseMap", loadedGlow);
                if (pMat.HasProperty("_UnlitColorMap")) pMat.SetTexture("_UnlitColorMap", loadedGlow);
            }
            if (pMat.HasProperty("_BaseColor")) pMat.SetColor("_BaseColor", Color.white);
            if (pMat.HasProperty("_UnlitColor")) pMat.SetColor("_UnlitColor", Color.white);
            if (pMat.HasProperty("_Surface")) pMat.SetFloat("_Surface", 1f);    // Transparent
            if (pMat.HasProperty("_Blend")) pMat.SetFloat("_Blend", 2f);        // Additive
            if (pMat.HasProperty("_ColorMode")) pMat.SetFloat("_ColorMode", 0f);// Multiply
            if (pMat.HasProperty("_SrcBlend")) pMat.SetFloat("_SrcBlend", (float)BlendMode.One);
            if (pMat.HasProperty("_DstBlend")) pMat.SetFloat("_DstBlend", (float)BlendMode.One);
            if (pMat.HasProperty("_ZWrite")) pMat.SetFloat("_ZWrite", 0f);
            if (pMat.HasProperty("_SurfaceType")) pMat.SetFloat("_SurfaceType", 1f);   // HDRP: transparent
            if (pMat.HasProperty("_BlendMode")) pMat.SetFloat("_BlendMode", 1f);       // HDRP: additive
            if (pMat.HasProperty("_AlphaSrcBlend")) pMat.SetFloat("_AlphaSrcBlend", (float)BlendMode.One);
            if (pMat.HasProperty("_AlphaDstBlend")) pMat.SetFloat("_AlphaDstBlend", (float)BlendMode.One);
            if (pMat.HasProperty("_EnableFogOnTransparent")) pMat.SetFloat("_EnableFogOnTransparent", 0f);
            pMat.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
            pMat.EnableKeyword("_BLENDMODE_ADD");
            pMat.DisableKeyword("_ALPHATEST_ON");
            pMat.DisableKeyword("_ALPHAPREMULTIPLY_ON");
            pMat.SetOverrideTag("RenderType", "Transparent");
            pMat.renderQueue = 3000;
            AetheriaHdrpConvert.ValidateHdMaterial(pMat);
            SaveOrReplaceAsset(pMat, "Assets/Resources/Materials/URP_Particles_Unlit_Template.mat");
        }

        private static void ConfigureNormalMapImporters()
        {
            if (!AssetDatabase.IsValidFolder("Assets/_AAA_Imported")) return;
            string[] guids = AssetDatabase.FindAssets("t:Texture2D", new[] { "Assets/_AAA_Imported" });
            bool dirty = false;
            foreach (var g in guids)
            {
                string path = AssetDatabase.GUIDToAssetPath(g);
                if (path.EndsWith("_Normal.png", StringComparison.OrdinalIgnoreCase) ||
                    path.EndsWith("_Normal_Raw.png", StringComparison.OrdinalIgnoreCase) ||
                    path.EndsWith("_N.png", StringComparison.OrdinalIgnoreCase) ||
                    path.EndsWith("_NRM.png", StringComparison.OrdinalIgnoreCase) ||
                    path.EndsWith("_nor_gl_2k.jpg", StringComparison.OrdinalIgnoreCase))
                {
                    if (AssetImporter.GetAtPath(path) is TextureImporter ti && ti.textureType != TextureImporterType.NormalMap)
                    {
                        ti.textureType = TextureImporterType.NormalMap;
                        ti.SaveAndReimport();
                        dirty = true;
                    }
                }
            }
            if (dirty) AssetDatabase.Refresh();
        }

        private static Dictionary<string, Material> BuildPbrMaterials()
        {
            var dict = new Dictionary<string, Material>();
            var litSh = Shader.Find("HDRP/Lit") ?? Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");

            Material MakeSurface(string key, string albedoPath, string existingNormalPath, string ordpForSobelPath,
                Color tint, float smooth, float metal, float tiling = 1f, float normalStr = 1.15f, Color? emission = null)
            {
                var mat = new Material(litSh);
                var alb = AssetDatabase.LoadAssetAtPath<Texture2D>(albedoPath);
                if (alb != null)
                {
                    if (mat.HasProperty("_BaseMap")) mat.SetTexture("_BaseMap", alb);
                    if (mat.HasProperty("_BaseColorMap")) mat.SetTexture("_BaseColorMap", alb);
                    mat.mainTexture = alb;
                }
                if (mat.HasProperty("_BaseColor")) mat.SetColor("_BaseColor", tint);
                mat.color = tint;
                if (mat.HasProperty("_Smoothness")) mat.SetFloat("_Smoothness", smooth);
                if (mat.HasProperty("_SmoothnessRemapMax")) mat.SetFloat("_SmoothnessRemapMax", smooth);
                if (mat.HasProperty("_Metallic")) mat.SetFloat("_Metallic", metal);
                if (mat.HasProperty("_MetallicRemapMax")) mat.SetFloat("_MetallicRemapMax", metal);
                mat.mainTextureScale = new Vector2(tiling, tiling);
                var tileVec = new Vector2(tiling, tiling);
                if (mat.HasProperty("_BaseColorMap")) mat.SetTextureScale("_BaseColorMap", tileVec);
                if (mat.HasProperty("_NormalMap")) mat.SetTextureScale("_NormalMap", tileVec);

                Texture2D nrm = !string.IsNullOrEmpty(existingNormalPath)
                    ? AssetDatabase.LoadAssetAtPath<Texture2D>(existingNormalPath)
                    : null;
                if (nrm == null && !string.IsNullOrEmpty(ordpForSobelPath))
                    nrm = DeriveSobelNormalTexture(key, ordpForSobelPath);
                if (nrm == null && !string.IsNullOrEmpty(albedoPath))
                    nrm = DeriveSobelNormalTexture(key, albedoPath);

                if (nrm != null)
                {
                    mat.EnableKeyword("_NORMALMAP");
                    if (mat.HasProperty("_BumpMap")) mat.SetTexture("_BumpMap", nrm);
                    if (mat.HasProperty("_BumpScale")) mat.SetFloat("_BumpScale", normalStr);
                    if (mat.HasProperty("_NormalMap"))
                    {
                        mat.SetTexture("_NormalMap", nrm);
                        mat.SetTextureScale("_NormalMap", tileVec);
                    }
                    if (mat.HasProperty("_NormalScale")) mat.SetFloat("_NormalScale", normalStr);
                    if (mat.HasProperty("_NormalMapSpace")) mat.SetFloat("_NormalMapSpace", 0f);
                }

                if (emission.HasValue)
                {
                    mat.EnableKeyword("_EMISSION");
                    if (mat.HasProperty("_EmissionColor")) mat.SetColor("_EmissionColor", emission.Value);
                    if (mat.HasProperty("_EmissiveColor")) mat.SetColor("_EmissiveColor", emission.Value);
                    if (mat.HasProperty("_EmissiveColorLDR")) mat.SetColor("_EmissiveColorLDR", emission.Value);
                    if (mat.HasProperty("_EmissiveColorMode")) mat.SetFloat("_EmissiveColorMode", 0f);
                    if (mat.HasProperty("_UseEmissiveIntensity")) mat.SetFloat("_UseEmissiveIntensity", 0f);
                    mat.EnableKeyword("_EMISSIVE_COLOR");
                }

                AetheriaHdrpConvert.ValidateHdMaterial(mat);
                SaveOrReplaceAsset(mat, $"Assets/Generated/Materials/{key}.mat");
                dict[key] = AssetDatabase.LoadAssetAtPath<Material>($"Assets/Generated/Materials/{key}.mat");
                return dict[key];
            }

            MakeSurface("RomanStoneFloor",
                "Assets/_AAA_Imported/Megascans/Surfaces/RomanStoneFloor/T_Roman_Stone_Floor_thsjdfxq_D.png",
                null,
                "Assets/_AAA_Imported/Megascans/Surfaces/RomanStoneFloor/T_RomanStoneFloor_thsjdfxq_ORDp.png",
                Color.white, 0.28f, 0.05f, 1f, 1.35f);

            MakeSurface("RockySoil",
                "Assets/_AAA_Imported/Megascans/Surfaces/RockySoilGround/T_Rocky_Soil_Ground_vb2qdgwew_D.png",
                null,
                "Assets/_AAA_Imported/Megascans/Surfaces/RockySoilGround/T_RockySoilGround_vb2qdgwew_ORDp.png",
                new Color(0.78f, 0.84f, 0.68f), 0.16f, 0.0f, 1f, 1.25f);

            MakeSurface("LayeredCliff",
                "Assets/_AAA_Imported/Megascans/Surfaces/LayeredRockCliff/T_Layered_Rock_Cliff_ubulehofw_D.png",
                null,
                "Assets/_AAA_Imported/Megascans/Surfaces/LayeredRockCliff/T_LayeredRockCliff_ubulehofw_ORDp.png",
                new Color(0.84f, 0.80f, 0.78f), 0.22f, 0.04f, 1f, 1.45f);

            MakeSurface("GravelGlacier",
                "Assets/_AAA_Imported/Megascans/Surfaces/GravelGround/T_Gravel_Ground_vi0maebg_D.png",
                null,
                "Assets/_AAA_Imported/Megascans/Surfaces/GravelGround/T_GravelGround_vi0maebg_ORDp.png",
                new Color(0.80f, 0.90f, 1.00f), 0.42f, 0.08f, 1f, 1.20f);

            MakeSurface("BurntAsh",
                "Assets/_AAA_Imported/Megascans/Surfaces/BurntGrass/T_Burnt_Grass_smlmnnn_D.png",
                null,
                "Assets/_AAA_Imported/Megascans/Surfaces/BurntGrass/T_BurntGrass_smlmnnn_DpRA.png",
                new Color(0.46f, 0.34f, 0.32f), 0.18f, 0.05f, 1f, 1.25f);

            MakeSurface("NordicCliff3D",
                "Assets/_AAA_Imported/Megascans/3D/Cliffs/T_HugeNordicCoastalCliff_ulkiejkva_D.png",
                "Assets/_AAA_Imported/Megascans/3D/Cliffs/T_HugeNordicCoastalCliff_ulkiejkva_N.png",
                null,
                Color.white, 0.24f, 0.04f, 1f, 1.25f);

            MakeSurface("ShrineFloor3D",
                "Assets/_AAA_Imported/Megascans/3D/StoneFloor/T_Japanese_Shrine_Stone_Floor_ugrxbjkfa_D.png",
                null,
                "Assets/_AAA_Imported/Megascans/3D/StoneFloor/T_Japanese_Shrine_Stone_Floor_ugrxbjkfa_DpRF.png",
                Color.white, 0.30f, 0.05f, 1f, 1.25f);

            MakeSurface("Boulder3D",
                "Assets/_AAA_Imported/Megascans/3D/Boulders/T_Beach_Boulder_uisjbbis_D.png",
                null,
                "Assets/_AAA_Imported/Megascans/3D/Boulders/T_BeachBoulder_uisjbbis_DpR.png",
                Color.white, 0.24f, 0.03f, 1f, 1.15f);

            MakeSurface("DeadTree3D",
                "Assets/_AAA_Imported/Megascans/3D/DeadTrees/T_Dead_Tree_qlEtl_D.png",
                null,
                "Assets/_AAA_Imported/Megascans/3D/DeadTrees/T_DeadTree_qlEtl_DpR.png",
                Color.white, 0.14f, 0.0f, 1f, 1.15f);

            MakeSurface("GothicArcMat",
                "Assets/_AAA_Imported/Environment/Gothic_props_1/Textures/T_Arc_BaseColor.png",
                "Assets/_AAA_Imported/Environment/Gothic_props_1/Textures/T_Arc__Normal.png",
                null,
                Color.white, 0.32f, 0.08f, 1f, 1.20f);

            MakeSurface("GothicBridgeMat",
                "Assets/_AAA_Imported/Environment/Gothic_props_1/Textures/T_Bridge1_BaseColor.png",
                "Assets/_AAA_Imported/Environment/Gothic_props_1/Textures/T_Bridge1_Normal.png",
                null,
                Color.white, 0.28f, 0.06f, 1f, 1.25f);

            MakeSurface("StatueMat",
                "Assets/_AAA_Imported/Environment/Statue/Textures/T_TA_Sample_Statue_01_BaseColor.png",
                "Assets/_AAA_Imported/Environment/Statue/Textures/T_TA_Sample_Statue_01_Normal.png",
                null,
                Color.white, 0.40f, 0.15f, 1f, 1.15f);

            MakeSurface("WhisperEnemyMat",
                "Assets/_AAA_Imported/Enemies/Whisper/Textures/T_Whisper_Body_Albedo.png",
                "Assets/_AAA_Imported/Enemies/Whisper/Textures/T_Whisper_Body_Normal.png",
                null,
                Color.white, 0.45f, 0.20f, 1f, 1.15f);

            // Real 2K/4K Character, Dragon & Boss Multi-Slot PBR Materials
            MakeSurface("DragonRedMat",
                "Assets/_AAA_Imported/Dragon/Textures/RedHPTex.png",
                null,
                "Assets/_AAA_Imported/Dragon/Textures/RedHPTex.png",
                new Color(1.0f, 0.92f, 0.90f), 0.52f, 0.26f, 1f, 1.35f, new Color(0.18f, 0.03f, 0.01f));

            MakeSurface("Char_ObsidianSteel",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_char_obsidian_steel_albedo.png",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_char_obsidian_steel_normal.png",
                null,
                new Color(0.75f, 0.78f, 0.85f), 0.64f, 0.68f, 2f, 1.25f);

            MakeSurface("Char_GildedBrass",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_d4_gilded_brass_albedo.png",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_d4_gilded_brass_normal.png",
                null,
                new Color(1.0f, 0.88f, 0.58f), 0.72f, 0.82f, 2f, 1.20f);

            MakeSurface("Char_DarkCloth",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_char_dark_cloth_albedo.png",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_char_dark_cloth_normal.png",
                null,
                new Color(0.68f, 0.64f, 0.66f), 0.24f, 0.06f, 2f, 1.20f);

            MakeSurface("Char_CrimsonCloak",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_d4_woven_wool_albedo.png",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_d4_woven_wool_normal.png",
                null,
                new Color(0.68f, 0.16f, 0.18f), 0.22f, 0.05f, 2f, 1.15f);

            MakeSurface("Char_WarmSkin",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_d4_woven_wool_albedo.png",
                null,
                null,
                new Color(0.84f, 0.64f, 0.54f), 0.34f, 0.0f, 1f, 0.45f);

            MakeSurface("Boss_CrimsonIron",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/mob_boss_archon_char_obsidian_steel_albedo.png",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/mob_boss_archon_char_obsidian_steel_normal.png",
                null,
                new Color(0.55f, 0.18f, 0.20f), 0.62f, 0.72f, 2f, 1.35f, new Color(0.24f, 0.03f, 0.02f));

            MakeSurface("Boss_Carapace",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/mob_boss_archon_char_carapace_albedo.png",
                "Assets/_AAA_Imported/WizardRaw/GLBModels/mob_boss_archon_char_carapace_normal.png",
                null,
                new Color(0.32f, 0.10f, 0.14f), 0.45f, 0.38f, 2f, 1.35f, new Color(0.45f, 0.05f, 0.04f));

            MakeSurface("Mob_BruiserMat",
                "Assets/_AAA_Imported/RawArchive/Textures/T_Mob_Bruiser_00_Albedo.png",
                "Assets/_AAA_Imported/RawArchive/Textures/T_Mob_Bruiser_00_Normal.png",
                null,
                new Color(0.68f, 0.82f, 0.96f), 0.40f, 0.25f, 1f, 1.25f);

            MakeSurface("Mob_SwarmMat",
                "Assets/_AAA_Imported/RawArchive/Textures/T_Mob_Swarm_00_Albedo.png",
                "Assets/_AAA_Imported/RawArchive/Textures/T_Mob_Swarm_00_Normal.png",
                null,
                Color.white, 0.35f, 0.25f, 1f, 1.25f);

            MakeSurface("Mob_RangedMat",
                "Assets/_AAA_Imported/RawArchive/Textures/T_Mob_Ranged_00_Albedo.png",
                "Assets/_AAA_Imported/RawArchive/Textures/T_Mob_Ranged_00_Normal.png",
                null,
                Color.white, 0.35f, 0.25f, 1f, 1.25f);

            var waterfall = new Material(litSh);
            waterfall.color = new Color(0.58f, 0.88f, 1.0f, 0.90f);
            if (waterfall.HasProperty("_BaseColor")) waterfall.SetColor("_BaseColor", waterfall.color);
            if (waterfall.HasProperty("_Smoothness")) waterfall.SetFloat("_Smoothness", 0.92f);
            if (waterfall.HasProperty("_SmoothnessRemapMax")) waterfall.SetFloat("_SmoothnessRemapMax", 0.92f);
            if (waterfall.HasProperty("_SurfaceType")) waterfall.SetFloat("_SurfaceType", 1f);
            if (waterfall.HasProperty("_BlendMode")) waterfall.SetFloat("_BlendMode", 1f);
            if (waterfall.HasProperty("_ZWrite")) waterfall.SetFloat("_ZWrite", 0f);
            if (waterfall.HasProperty("_SrcBlend")) waterfall.SetFloat("_SrcBlend", (float)BlendMode.One);
            if (waterfall.HasProperty("_DstBlend")) waterfall.SetFloat("_DstBlend", (float)BlendMode.One);
            if (waterfall.HasProperty("_AlphaSrcBlend")) waterfall.SetFloat("_AlphaSrcBlend", (float)BlendMode.One);
            if (waterfall.HasProperty("_AlphaDstBlend")) waterfall.SetFloat("_AlphaDstBlend", (float)BlendMode.One);
            if (waterfall.HasProperty("_EnableFogOnTransparent")) waterfall.SetFloat("_EnableFogOnTransparent", 1f);
            waterfall.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
            waterfall.EnableKeyword("_BLENDMODE_ADD");
            waterfall.SetOverrideTag("RenderType", "Transparent");
            waterfall.renderQueue = (int)RenderQueue.Transparent;
            waterfall.EnableKeyword("_EMISSION");
            if (waterfall.HasProperty("_EmissionColor")) waterfall.SetColor("_EmissionColor", new Color(0.18f, 0.45f, 0.72f));
            if (waterfall.HasProperty("_EmissiveColor")) waterfall.SetColor("_EmissiveColor", new Color(0.18f, 0.45f, 0.72f));
            if (waterfall.HasProperty("_EmissiveColorLDR")) waterfall.SetColor("_EmissiveColorLDR", new Color(0.18f, 0.45f, 0.72f));
            if (waterfall.HasProperty("_EmissiveColorMode")) waterfall.SetFloat("_EmissiveColorMode", 0f);
            if (waterfall.HasProperty("_UseEmissiveIntensity")) waterfall.SetFloat("_UseEmissiveIntensity", 0f);
            waterfall.EnableKeyword("_EMISSIVE_COLOR");
            AetheriaHdrpConvert.ValidateHdMaterial(waterfall);
            SaveOrReplaceAsset(waterfall, "Assets/Generated/Materials/WaterfallMist.mat");
            dict["WaterfallMist"] = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/WaterfallMist.mat");

            void ConfigureHdrpEmission(Material m, Color col)
            {
                m.EnableKeyword("_EMISSION");
                if (m.HasProperty("_EmissionColor")) m.SetColor("_EmissionColor", col);
                if (m.HasProperty("_EmissiveColor")) m.SetColor("_EmissiveColor", col);
                if (m.HasProperty("_EmissiveColorLDR")) m.SetColor("_EmissiveColorLDR", col);
                if (m.HasProperty("_EmissiveColorMode")) m.SetFloat("_EmissiveColorMode", 0f);
                if (m.HasProperty("_UseEmissiveIntensity")) m.SetFloat("_UseEmissiveIntensity", 0f);
                m.EnableKeyword("_EMISSIVE_COLOR");
            }

            // Stylized emissive & liquid environment materials
            var ice = new Material(litSh);
            ice.color = new Color(0.45f, 0.88f, 1.0f);
            if (ice.HasProperty("_BaseColor")) ice.SetColor("_BaseColor", ice.color);
            if (ice.HasProperty("_Smoothness")) ice.SetFloat("_Smoothness", 0.86f);
            if (ice.HasProperty("_SmoothnessRemapMax")) ice.SetFloat("_SmoothnessRemapMax", 0.86f);
            if (ice.HasProperty("_Metallic")) ice.SetFloat("_Metallic", 0.12f);
            if (ice.HasProperty("_MetallicRemapMax")) ice.SetFloat("_MetallicRemapMax", 0.12f);
            ConfigureHdrpEmission(ice, new Color(0.05f, 0.42f, 0.65f));
            AetheriaHdrpConvert.ValidateHdMaterial(ice);
            SaveOrReplaceAsset(ice, "Assets/Generated/Materials/CrystalIce.mat");
            dict["CrystalIce"] = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/CrystalIce.mat");

            var sunCrystal = new Material(litSh);
            sunCrystal.color = new Color(1.0f, 0.44f, 0.08f);
            if (sunCrystal.HasProperty("_BaseColor")) sunCrystal.SetColor("_BaseColor", sunCrystal.color);
            ConfigureHdrpEmission(sunCrystal, new Color(2.4f, 0.72f, 0.12f));
            AetheriaHdrpConvert.ValidateHdMaterial(sunCrystal);
            SaveOrReplaceAsset(sunCrystal, "Assets/Generated/Materials/BindCrystalGlow.mat");
            dict["BindCrystalGlow"] = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/BindCrystalGlow.mat");

            var wexBeacon = new Material(litSh);
            wexBeacon.color = new Color(0.62f, 0.34f, 1.0f);
            if (wexBeacon.HasProperty("_BaseColor")) wexBeacon.SetColor("_BaseColor", wexBeacon.color);
            ConfigureHdrpEmission(wexBeacon, new Color(1.3f, 0.55f, 2.5f));
            AetheriaHdrpConvert.ValidateHdMaterial(wexBeacon);
            SaveOrReplaceAsset(wexBeacon, "Assets/Generated/Materials/WexConductorGlow.mat");
            dict["WexConductorGlow"] = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/WexConductorGlow.mat");

            var obsidian = new Material(litSh);
            obsidian.color = new Color(0.10f, 0.07f, 0.12f);
            if (obsidian.HasProperty("_BaseColor")) obsidian.SetColor("_BaseColor", obsidian.color);
            if (obsidian.HasProperty("_Smoothness")) obsidian.SetFloat("_Smoothness", 0.68f);
            if (obsidian.HasProperty("_SmoothnessRemapMax")) obsidian.SetFloat("_SmoothnessRemapMax", 0.68f);
            if (obsidian.HasProperty("_Metallic")) obsidian.SetFloat("_Metallic", 0.45f);
            if (obsidian.HasProperty("_MetallicRemapMax")) obsidian.SetFloat("_MetallicRemapMax", 0.45f);
            ConfigureHdrpEmission(obsidian, new Color(1.4f, 0.22f, 0.05f));
            AetheriaHdrpConvert.ValidateHdMaterial(obsidian);
            SaveOrReplaceAsset(obsidian, "Assets/Generated/Materials/ObsidianSeal.mat");
            dict["ObsidianSeal"] = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/ObsidianSeal.mat");

            var lava = new Material(litSh);
            lava.color = new Color(0.95f, 0.28f, 0.04f);
            if (lava.HasProperty("_BaseColor")) lava.SetColor("_BaseColor", lava.color);
            ConfigureHdrpEmission(lava, new Color(3.6f, 0.88f, 0.12f));
            AetheriaHdrpConvert.ValidateHdMaterial(lava);
            SaveOrReplaceAsset(lava, "Assets/Generated/Materials/EmberLava.mat");
            dict["EmberLava"] = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/EmberLava.mat");

            var lake = new Material(litSh);
            lake.color = new Color(0.06f, 0.24f, 0.38f, 0.92f);
            if (lake.HasProperty("_BaseColor")) lake.SetColor("_BaseColor", lake.color);
            if (lake.HasProperty("_Smoothness")) lake.SetFloat("_Smoothness", 0.95f);
            if (lake.HasProperty("_SmoothnessRemapMax")) lake.SetFloat("_SmoothnessRemapMax", 0.95f);
            if (lake.HasProperty("_Metallic")) lake.SetFloat("_Metallic", 0.35f);
            if (lake.HasProperty("_MetallicRemapMax")) lake.SetFloat("_MetallicRemapMax", 0.35f);
            if (lake.HasProperty("_SurfaceType")) lake.SetFloat("_SurfaceType", 1f);
            if (lake.HasProperty("_BlendMode")) lake.SetFloat("_BlendMode", 0f);
            if (lake.HasProperty("_ZWrite")) lake.SetFloat("_ZWrite", 0f);
            if (lake.HasProperty("_SrcBlend")) lake.SetFloat("_SrcBlend", (float)BlendMode.SrcAlpha);
            if (lake.HasProperty("_DstBlend")) lake.SetFloat("_DstBlend", (float)BlendMode.OneMinusSrcAlpha);
            if (lake.HasProperty("_AlphaSrcBlend")) lake.SetFloat("_AlphaSrcBlend", (float)BlendMode.One);
            if (lake.HasProperty("_AlphaDstBlend")) lake.SetFloat("_AlphaDstBlend", (float)BlendMode.OneMinusSrcAlpha);
            if (lake.HasProperty("_EnableFogOnTransparent")) lake.SetFloat("_EnableFogOnTransparent", 1f);
            if (lake.HasProperty("_ReceivesSSRTransparent")) lake.SetFloat("_ReceivesSSRTransparent", 1f);
            lake.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
            lake.DisableKeyword("_DISABLE_SSR_TRANSPARENT");
            lake.SetOverrideTag("RenderType", "Transparent");
            lake.renderQueue = (int)RenderQueue.Transparent;
            AetheriaHdrpConvert.ValidateHdMaterial(lake);
            SaveOrReplaceAsset(lake, "Assets/Generated/Materials/MirrorLakeWater.mat");
            dict["MirrorLakeWater"] = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/MirrorLakeWater.mat");

            var sapphireLeaf = new Material(litSh);
            sapphireLeaf.color = new Color(0.16f, 0.46f, 0.88f);
            if (sapphireLeaf.HasProperty("_BaseColor")) sapphireLeaf.SetColor("_BaseColor", sapphireLeaf.color);
            ConfigureHdrpEmission(sapphireLeaf, new Color(0.12f, 0.42f, 0.95f));
            AetheriaHdrpConvert.ValidateHdMaterial(sapphireLeaf);
            SaveOrReplaceAsset(sapphireLeaf, "Assets/Generated/Materials/SapphireCanopy.mat");
            dict["SapphireCanopy"] = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/SapphireCanopy.mat");

            return dict;
        }

        private static Texture2D DeriveSobelNormalTexture(string key, string srcPngPath)
        {
            string dstPath = $"Assets/Generated/Normals/{key}_NRM.png";
            if (!File.Exists(srcPngPath)) return null;

            byte[] bytes = File.ReadAllBytes(srcPngPath);
            var raw = new Texture2D(2, 2, TextureFormat.RGBA32, false);
            if (!raw.LoadImage(bytes))
            {
                UnityEngine.Object.DestroyImmediate(raw);
                return null;
            }

            const int N = 512;
            var hGrid = new float[N * N];
            for (int y = 0; y < N; y++)
            {
                float v = (y + 0.5f) / N;
                for (int x = 0; x < N; x++)
                {
                    float u = (x + 0.5f) / N;
                    Color c = raw.GetPixelBilinear(u, v);
                    hGrid[y * N + x] = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
                }
            }
            UnityEngine.Object.DestroyImmediate(raw);

            var nrm = new Texture2D(N, N, TextureFormat.RGBA32, false);
            float strength = 3.2f;
            for (int y = 0; y < N; y++)
            {
                int ym = (y - 1 + N) % N;
                int yp = (y + 1) % N;
                for (int x = 0; x < N; x++)
                {
                    int xm = (x - 1 + N) % N;
                    int xp = (x + 1) % N;
                    float tl = hGrid[yp * N + xm], t = hGrid[yp * N + x], tr = hGrid[yp * N + xp];
                    float l  = hGrid[y * N + xm],                         r  = hGrid[y * N + xp];
                    float bl = hGrid[ym * N + xm], b = hGrid[ym * N + x], br = hGrid[ym * N + xp];
                    float dx = (tr + 2f * r + br) - (tl + 2f * l + bl);
                    float dy = (tl + 2f * t + tr) - (bl + 2f * b + br);
                    Vector3 vec = new Vector3(-dx * strength, -dy * strength, 1f).normalized;
                    nrm.SetPixel(x, y, new Color(vec.x * 0.5f + 0.5f, vec.y * 0.5f + 0.5f, vec.z * 0.5f + 0.5f, 1f));
                }
            }
            nrm.Apply();
            File.WriteAllBytes(dstPath, nrm.EncodeToPNG());
            UnityEngine.Object.DestroyImmediate(nrm);
            AssetDatabase.ImportAsset(dstPath, ImportAssetOptions.ForceUpdate);

            if (AssetImporter.GetAtPath(dstPath) is TextureImporter ti)
            {
                ti.textureType = TextureImporterType.NormalMap;
                ti.wrapMode = TextureWrapMode.Repeat;
                ti.SaveAndReimport();
            }
            return AssetDatabase.LoadAssetAtPath<Texture2D>(dstPath);
        }

        private static TerrainLayer[] BuildTerrainLayers(Dictionary<string, Material> mats)
        {
            string[] keys = { "RomanStoneFloor", "RockySoil", "LayeredCliff", "GravelGlacier", "BurntAsh" };
            var layers = new TerrainLayer[keys.Length];
            for (int i = 0; i < keys.Length; i++)
            {
                var mat = mats[keys[i]];
                Texture2D diffuse = (mat.HasProperty("_BaseColorMap") ? mat.GetTexture("_BaseColorMap") as Texture2D : null)
                                 ?? (mat.HasProperty("_BaseMap") ? mat.GetTexture("_BaseMap") as Texture2D : null);
                Texture2D normal = (mat.HasProperty("_NormalMap") ? mat.GetTexture("_NormalMap") as Texture2D : null)
                                ?? (mat.HasProperty("_BumpMap") ? mat.GetTexture("_BumpMap") as Texture2D : null);
                var layer = new TerrainLayer
                {
                    diffuseTexture = diffuse,
                    normalMapTexture = normal,
                    normalScale = 1.25f,
                    tileSize = i == 0 ? new Vector2(10f, 10f) : new Vector2(18f, 18f),
                    smoothness = i == 3 ? 0.42f : 0.18f,
                    metallic = 0.04f
                };
                string path = $"Assets/Generated/TerrainLayers/TL_{keys[i]}.terrainlayer";
                SaveOrReplaceAsset(layer, path);
                layers[i] = AssetDatabase.LoadAssetAtPath<TerrainLayer>(path);
            }
            return layers;
        }

        [MenuItem("Aetheria/4. Build Animator Controllers")]
        public static void BuildAnimatorControllers()
        {
            if (!Directory.Exists(AnimatorsFolder)) Directory.CreateDirectory(AnimatorsFolder);

            var allClips = new List<(string path, AnimationClip clip)>();
            if (AssetDatabase.IsValidFolder("Assets/_AAA_Imported"))
            {
                foreach (string guid in AssetDatabase.FindAssets("t:Model", new[] { "Assets/_AAA_Imported" }))
                {
                    string path = AssetDatabase.GUIDToAssetPath(guid);
                    if (!path.EndsWith(".fbx", StringComparison.OrdinalIgnoreCase) &&
                        !path.EndsWith(".glb", StringComparison.OrdinalIgnoreCase)) continue;
                    foreach (var obj in AssetDatabase.LoadAllAssetsAtPath(path))
                    {
                        if (obj is AnimationClip clip && !clip.name.StartsWith("__preview__", StringComparison.OrdinalIgnoreCase))
                            allClips.Add((path, clip));
                    }
                }
            }

            if (allClips.Count == 0)
            {
                Debug.Log("[AetheriaSetup] No source FBX/GLB AnimationClips found under Assets/_AAA_Imported; skipping AnimatorController generation.");
                return;
            }

            AnimationClip FindClip(string folderFilter, bool requireHuman, params string[] keywords)
            {
                foreach (var kw in keywords)
                {
                    foreach (var entry in allClips)
                    {
                        if (requireHuman && !entry.clip.humanMotion) continue;
                        if (!string.IsNullOrEmpty(folderFilter) &&
                            entry.path.IndexOf(folderFilter, StringComparison.OrdinalIgnoreCase) < 0) continue;
                        if (entry.clip.name.IndexOf(kw, StringComparison.OrdinalIgnoreCase) >= 0 ||
                            entry.path.IndexOf(kw, StringComparison.OrdinalIgnoreCase) >= 0)
                            return entry.clip;
                    }
                }
                return null;
            }

            AnimatorController BuildController(string assetName, string folderFilter, bool requireHuman, bool isDragon)
            {
                string ctrlPath = $"{AnimatorsFolder}/{assetName}.controller";
                if (File.Exists(ctrlPath)) AssetDatabase.DeleteAsset(ctrlPath);
                var ctrl = AnimatorController.CreateAnimatorControllerAtPath(ctrlPath);
                var sm = ctrl.layers[0].stateMachine;

                // Shared + Hero + Creature + Dragon parameters
                ctrl.AddParameter("Speed", AnimatorControllerParameterType.Float);
                ctrl.AddParameter("CombatSpeed", AnimatorControllerParameterType.Float);
                ctrl.AddParameter("Grounded", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("InAttack", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("InStun", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("Dead", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("Moving", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("Blocking", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("Action", AnimatorControllerParameterType.Int);
                ctrl.AddParameter("AttackIndex", AnimatorControllerParameterType.Int);
                ctrl.AddParameter("DoAction", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Attack", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Attack1", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Attack2", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Attack3", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Cast", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Parry", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Dash", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("HitReact", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Hit", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Die", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Respawn", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Revive", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Slam", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("Breath", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("Flying", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("Scream", AnimatorControllerParameterType.Trigger);
                ctrl.AddParameter("DragonMounted", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("DragonFlying", AnimatorControllerParameterType.Bool);
                ctrl.AddParameter("DragonBreath", AnimatorControllerParameterType.Bool);

                var ps = ctrl.parameters;
                for (int i = 0; i < ps.Length; i++)
                {
                    if (ps[i].name == "CombatSpeed") ps[i].defaultFloat = 1f;
                    if (ps[i].name == "Grounded") ps[i].defaultBool = true;
                }
                ctrl.parameters = ps;

                AnimationClip idle = FindClip(folderFilter, requireHuman, "idle", "stand", "hover") ?? FindClip(null, requireHuman, "idle");
                AnimationClip walk = FindClip(folderFilter, requireHuman, "walk", "jog", "move", "fly") ?? idle;
                AnimationClip run = FindClip(folderFilter, requireHuman, "run", "sprint", "fly", "jog") ?? walk;
                AnimationClip atk1 = FindClip(folderFilter, requireHuman, "attack1", "attack_1", "primary", "bite", "slash", "attack") ?? idle;
                AnimationClip atk2 = FindClip(folderFilter, requireHuman, "attack2", "attack_2", "combo", "claw", "swing") ?? atk1;
                AnimationClip atk3 = FindClip(folderFilter, requireHuman, "attack3", "attack_3", "heavy", "spin") ?? atk1;
                AnimationClip atk4 = FindClip(folderFilter, requireHuman, "attack4", "attack_4", "finisher", "slam") ?? atk3;
                AnimationClip heavy = FindClip(folderFilter, requireHuman, "heavy", "slam", "smash") ?? atk3;
                AnimationClip cast = FindClip(folderFilter, requireHuman, "cast", "ability", "spell", "breath") ?? atk2;
                AnimationClip parry = FindClip(folderFilter, requireHuman, "block", "parry", "guard") ?? idle;
                AnimationClip dash = FindClip(folderFilter, requireHuman, "dash", "dodge", "roll") ?? run;
                AnimationClip stun = FindClip(folderFilter, requireHuman, "stun", "dizzy") ?? idle;
                AnimationClip hit = FindClip(folderFilter, requireHuman, "hit", "impact", "hurt") ?? idle;
                AnimationClip death = FindClip(folderFilter, requireHuman, "death", "die", "dead") ?? idle;

                var locoState = ctrl.CreateBlendTreeInController("Idle_Walk_Run", out var locoTree, 0);
                locoTree.blendParameter = "Speed";
                locoTree.blendType = BlendTreeType.Simple1D;
                if (idle != null) locoTree.AddChild(idle, 0f);
                if (walk != null) locoTree.AddChild(walk, 3.0f);
                if (run != null) locoTree.AddChild(run, 6.2f);
                sm.defaultState = locoState;

                AnimatorState AddActionState(string stateName, AnimationClip clip, bool speedMult = false, bool exitToLoco = true)
                {
                    var st = sm.AddState(stateName);
                    st.motion = clip;
                    if (speedMult)
                    {
                        st.speedParameterActive = true;
                        st.speedParameter = "CombatSpeed";
                    }
                    if (exitToLoco)
                    {
                        var back = st.AddTransition(locoState);
                        back.hasExitTime = true;
                        back.exitTime = 0.9f;
                        back.duration = 0.12f;
                    }
                    return st;
                }

                var sAtk1 = AddActionState("Attack1", atk1, true);
                var sAtk2 = AddActionState("Attack2", atk2, true);
                var sAtk3 = AddActionState("Attack3", atk3, true);
                var sAtk4 = AddActionState("Attack4", atk4, true);
                var sHeavy = AddActionState("HeavyAttack", heavy, true);
                var sSlam = AddActionState("Slam", heavy, true);
                var sCast = AddActionState("Cast", cast, true);
                var sParry = AddActionState("Parry", parry, false);
                var sDash = AddActionState("Dash", dash, false);
                var sStun = AddActionState("Stun", stun, false);
                var sHit = AddActionState("Hit", hit, false);
                var sDeath = AddActionState("Death", death, false, exitToLoco: false);

                void AnyTrigger(AnimatorState target, string trig)
                {
                    var t = sm.AddAnyStateTransition(target);
                    t.hasExitTime = false;
                    t.duration = 0.08f;
                    t.AddCondition(AnimatorConditionMode.If, 0f, trig);
                }

                void AnyIndexedTrigger(AnimatorState target, string trig, string intParam, int val)
                {
                    var t = sm.AddAnyStateTransition(target);
                    t.hasExitTime = false;
                    t.duration = 0.08f;
                    t.AddCondition(AnimatorConditionMode.If, 0f, trig);
                    t.AddCondition(AnimatorConditionMode.Equals, val, intParam);
                }

                AnyIndexedTrigger(sAtk1, "Attack", "AttackIndex", 1);
                AnyIndexedTrigger(sAtk2, "Attack", "AttackIndex", 2);
                AnyIndexedTrigger(sAtk3, "Attack", "AttackIndex", 3);
                AnyIndexedTrigger(sAtk4, "Attack", "AttackIndex", 4);
                AnyIndexedTrigger(sAtk1, "DoAction", "Action", 1);
                AnyIndexedTrigger(sAtk2, "DoAction", "Action", 2);
                AnyIndexedTrigger(sAtk3, "DoAction", "Action", 3);
                AnyIndexedTrigger(sAtk4, "DoAction", "Action", 4);
                AnyIndexedTrigger(sCast, "DoAction", "Action", 5);
                AnyIndexedTrigger(sStun, "DoAction", "Action", 6);
                AnyTrigger(sAtk1, "Attack1");
                AnyTrigger(sAtk2, "Attack2");
                AnyTrigger(sAtk3, "Attack3");
                AnyTrigger(sCast, "Cast");
                AnyTrigger(sParry, "Parry");
                AnyTrigger(sDash, "Dash");
                AnyTrigger(sSlam, "Slam");
                AnyTrigger(sHit, "HitReact");
                AnyTrigger(sHit, "Hit");
                AnyTrigger(sDeath, "Die");

                var reviveT1 = sDeath.AddTransition(locoState);
                reviveT1.hasExitTime = false;
                reviveT1.duration = 0.1f;
                reviveT1.AddCondition(AnimatorConditionMode.If, 0f, "Respawn");

                var reviveT2 = sDeath.AddTransition(locoState);
                reviveT2.hasExitTime = false;
                reviveT2.duration = 0.1f;
                reviveT2.AddCondition(AnimatorConditionMode.If, 0f, "Revive");

                if (isDragon)
                {
                    var sFly = sm.AddState("Fly");
                    sFly.motion = run ?? walk ?? idle;
                    var toFly = locoState.AddTransition(sFly);
                    toFly.hasExitTime = false;
                    toFly.duration = 0.2f;
                    toFly.AddCondition(AnimatorConditionMode.If, 0f, "Flying");
                    var toGround = sFly.AddTransition(locoState);
                    toGround.hasExitTime = false;
                    toGround.duration = 0.2f;
                    toGround.AddCondition(AnimatorConditionMode.IfNot, 0f, "Flying");

                    var sBreath = AddActionState("Breath", FindClip(folderFilter, false, "breath", "fire", "scream") ?? cast);
                    var toBreath = sm.AddAnyStateTransition(sBreath);
                    toBreath.hasExitTime = false;
                    toBreath.duration = 0.1f;
                    toBreath.AddCondition(AnimatorConditionMode.If, 0f, "Breath");

                    var sScream = AddActionState("Scream", FindClip(folderFilter, false, "scream", "roar") ?? cast);
                    AnyTrigger(sScream, "Scream");
                }

                EditorUtility.SetDirty(ctrl);
                return ctrl;
            }

            BuildController("Hero_Animator", "/Heroes/", requireHuman: true, isDragon: false);
            BuildController("Creature_Animator", "/Enemies/", requireHuman: false, isDragon: false);
            BuildController("Boss_Animator", "/Boss_Sevarog/", requireHuman: false, isDragon: false);
            BuildController("Dragon_Animator", "/Dragon/", requireHuman: false, isDragon: true);
            AssetDatabase.SaveAssets();
        }

        private static void AssignControllerIfAvailable(GameObject inst, string controllerName)
        {
            var ctrl = AssetDatabase.LoadAssetAtPath<RuntimeAnimatorController>($"{AnimatorsFolder}/{controllerName}.controller");
            if (ctrl == null) return;
            var anim = inst.GetComponentInChildren<Animator>(true);
            if (anim != null) anim.runtimeAnimatorController = ctrl;
        }

        private static void StageResourcesPrefabs(Dictionary<string, Material> mats)
        {
            void StageHeroPrefab(string srcAssetPath, string dstResPath, Material armorMat, Material clothMat, Material trimMat, string animatorName = "Hero_Animator")
            {
                var src = AssetDatabase.LoadAssetAtPath<GameObject>(srcAssetPath);
                if (src == null)
                {
                    Debug.LogWarning($"[AetheriaSetup] Missing source model: {srcAssetPath}");
                    return;
                }
                var inst = (GameObject)PrefabUtility.InstantiatePrefab(src);
                if (inst == null) return;
                foreach (var r in inst.GetComponentsInChildren<Renderer>(true))
                {
                    var shared = r.sharedMaterials;
                    for (int i = 0; i < shared.Length; i++)
                    {
                        bool hasTex = shared[i] != null &&
                                      ((shared[i].HasProperty("_BaseColorMap") && shared[i].GetTexture("_BaseColorMap") != null) ||
                                       (shared[i].HasProperty("_BaseMap") && shared[i].GetTexture("_BaseMap") != null) ||
                                       (shared[i].HasProperty("baseColorTexture") && shared[i].GetTexture("baseColorTexture") != null));
                        if (hasTex) continue;
                        shared[i] = (i % 4) switch
                        {
                            0 => armorMat,
                            1 => clothMat,
                            2 => trimMat,
                            _ => i == shared.Length - 1 ? mats["Char_WarmSkin"] : armorMat
                        };
                    }
                    r.sharedMaterials = shared;
                }
                AssignControllerIfAvailable(inst, animatorName);
                PrefabUtility.SaveAsPrefabAsset(inst, dstResPath);
                UnityEngine.Object.DestroyImmediate(inst);
            }

            void StagePrefab(string srcAssetPath, string dstResPath, Material fallbackMat = null, bool forceAllSlots = false, string animatorName = null)
            {
                var src = AssetDatabase.LoadAssetAtPath<GameObject>(srcAssetPath);
                if (src == null)
                {
                    Debug.LogWarning($"[AetheriaSetup] Missing source model: {srcAssetPath}");
                    return;
                }
                var inst = (GameObject)PrefabUtility.InstantiatePrefab(src);
                if (inst == null) return;
                if (forceAllSlots && fallbackMat != null)
                {
                    foreach (var r in inst.GetComponentsInChildren<Renderer>(true))
                    {
                        var shared = r.sharedMaterials;
                        for (int i = 0; i < shared.Length; i++) shared[i] = fallbackMat;
                        r.sharedMaterials = shared;
                    }
                }
                else
                {
                    EnsureValidRenderMaterials(inst, fallbackMat ?? mats["RomanStoneFloor"]);
                }
                if (!string.IsNullOrEmpty(animatorName))
                    AssignControllerIfAvailable(inst, animatorName);
                PrefabUtility.SaveAsPrefabAsset(inst, dstResPath);
                UnityEngine.Object.DestroyImmediate(inst);
            }

            // 17 Textured Multi-Slot Hero Skins (Paragon Male & Female)
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Kwang/Meshes/Kwang_GDC.glb", "Assets/Resources/Heroes/Hero_Kwang.prefab", mats["Char_ObsidianSteel"], mats["Char_CrimsonCloak"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Kwang/Meshes/KwangManbun.glb", "Assets/Resources/Heroes/Hero_KwangManbun.prefab", mats["Char_ObsidianSteel"], mats["Char_DarkCloth"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Kwang/Meshes/KwangFrostwalker.glb", "Assets/Resources/Heroes/Hero_KwangFrost.prefab", mats["Mob_BruiserMat"], mats["Char_DarkCloth"], mats["CrystalIce"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Kwang/Meshes/KwangAlbino.glb", "Assets/Resources/Heroes/Hero_KwangAlbino.prefab", mats["Char_DarkCloth"], mats["Char_ObsidianSteel"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Kwang/Meshes/KwangSunrise.glb", "Assets/Resources/Heroes/Hero_KwangSunrise.prefab", mats["Char_GildedBrass"], mats["Char_CrimsonCloak"], mats["Char_ObsidianSteel"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Gideon/Meshes/Gideon.glb", "Assets/Resources/Heroes/Hero_Gideon.prefab", mats["Char_DarkCloth"], mats["Char_CrimsonCloak"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Gideon/Meshes/Gideon_Inquisitor.glb", "Assets/Resources/Heroes/Hero_GideonInq.prefab", mats["Char_ObsidianSteel"], mats["Char_CrimsonCloak"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Gideon/Meshes/Gideon_Mephisto.glb", "Assets/Resources/Heroes/Hero_GideonMeph.prefab", mats["Boss_CrimsonIron"], mats["Char_DarkCloth"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Khaimera/Meshes/Khaimera.glb", "Assets/Resources/Heroes/Hero_Khaimera.prefab", mats["Char_CrimsonCloak"], mats["Char_DarkCloth"], mats["Char_ObsidianSteel"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Khaimera/Meshes/Khaimera_GruxPelt.glb", "Assets/Resources/Heroes/Hero_KhaimeraPelt.prefab", mats["Char_DarkCloth"], mats["Char_CrimsonCloak"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Serath/Meshes/Serath.glb", "Assets/Resources/Heroes/Hero_Serath.prefab", mats["Char_GildedBrass"], mats["Char_ObsidianSteel"], mats["Char_CrimsonCloak"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Serath/Meshes/SerathGold.glb", "Assets/Resources/Heroes/Hero_SerathGold.prefab", mats["Char_GildedBrass"], mats["Char_CrimsonCloak"], mats["Char_ObsidianSteel"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Serath/Meshes/SerathFrost.glb", "Assets/Resources/Heroes/Hero_SerathFrost.prefab", mats["Mob_BruiserMat"], mats["Char_DarkCloth"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Kallari/Meshes/Kallari.glb", "Assets/Resources/Heroes/Hero_Kallari.prefab", mats["Char_DarkCloth"], mats["Char_ObsidianSteel"], mats["Char_CrimsonCloak"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Kallari/Meshes/Kallari_DeathLotus.glb", "Assets/Resources/Heroes/Hero_KallariLotus.prefab", mats["Boss_CrimsonIron"], mats["Char_DarkCloth"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Morigesh/Meshes/Morigesh.glb", "Assets/Resources/Heroes/Hero_Morigesh.prefab", mats["Char_DarkCloth"], mats["Char_CrimsonCloak"], mats["Char_GildedBrass"]);
            StageHeroPrefab("Assets/_AAA_Imported/Heroes/Morigesh/Meshes/Morigesh_NorthernMystic.glb", "Assets/Resources/Heroes/Hero_MorigeshMystic.prefab", mats["Mob_BruiserMat"], mats["Char_DarkCloth"], mats["CrystalIce"]);

            // Weapons & Cosmetics
            StagePrefab("Assets/_AAA_Imported/FabFBX/Dark_Knight_Longsword___Game_Ready_Weapon-cc7c30c4/fbx/longsword_extracted/LongSword.fbx",
                "Assets/Resources/Weapons/LongSword.prefab", mats["Char_ObsidianSteel"]);
            StagePrefab("Assets/_AAA_Imported/FabFBX/Free_-_Stylized_Dark_Sword-cfb429b8/fbx/demon_sword_extracted/Demon Sword/Sword_A.fbx",
                "Assets/Resources/Weapons/DemonSword.prefab", mats["Boss_CrimsonIron"]);
            StagePrefab("Assets/_AAA_Imported/FabFBX/Thornblade_Sword____Fantasy_Game_Ready_Weapon-c2d48a7a/fbx/freesword_extracted/Sword.fbx",
                "Assets/Resources/Weapons/Thornblade.prefab", mats["Char_GildedBrass"]);
            StagePrefab("Assets/_AAA_Imported/WizardRaw/Meshes/SM_Staff_Arcane01.fbx",
                "Assets/Resources/Weapons/ArcaneStaff.prefab", mats["WexConductorGlow"]);
            StagePrefab("Assets/_AAA_Imported/WizardRaw/Meshes/SM_Staff_CrystalScepter.fbx",
                "Assets/Resources/Weapons/CrystalScepter.prefab", mats["CrystalIce"]);

            StagePrefab("Assets/_AAA_Imported/WizardRaw/Meshes/SM_Head_AbyssHood.fbx", "Assets/Resources/Cosmetics/AbyssHood.prefab", mats["Char_DarkCloth"]);
            StagePrefab("Assets/_AAA_Imported/WizardRaw/Meshes/SM_Head_ChronoHalo.fbx", "Assets/Resources/Cosmetics/CrownCirclet.prefab", mats["Char_GildedBrass"]);
            StagePrefab("Assets/_AAA_Imported/WizardRaw/Meshes/SM_Head_BoneCrown.fbx", "Assets/Resources/Cosmetics/BoneCrown.prefab", mats["StatueMat"]);

            // Boss, Dragon & Enemies with Real PBR Textures!
            StagePrefab("Assets/_AAA_Imported/Dragon/Meshes/DragonTheUsurperSK.glb", "Assets/Resources/Creatures/IgnisDragon.prefab", mats["DragonRedMat"], forceAllSlots: true, animatorName: "Dragon_Animator");
            StageHeroPrefab("Assets/_AAA_Imported/Boss_Sevarog/Meshes/SevarogBloodred.glb", "Assets/Resources/Creatures/MalakorBoss.prefab", mats["Boss_CrimsonIron"], mats["Boss_Carapace"], mats["Char_GildedBrass"], animatorName: "Boss_Animator");
            StagePrefab("Assets/_AAA_Imported/Enemies/Whisper/Meshes/SK_Whisper.glb", "Assets/Resources/Creatures/Whisper.prefab", mats["WhisperEnemyMat"], forceAllSlots: true, animatorName: "Creature_Animator");
            StagePrefab("Assets/_AAA_Imported/Enemies/Rampage/Meshes/Rampage.glb", "Assets/Resources/Creatures/Rampage.prefab", mats["Mob_BruiserMat"], forceAllSlots: true, animatorName: "Creature_Animator");
            StagePrefab("Assets/_AAA_Imported/Enemies/Minions/Meshes/Minion_Lane_Melee_Dusk.glb", "Assets/Resources/Creatures/MinionMelee.prefab", mats["Mob_SwarmMat"], forceAllSlots: true, animatorName: "Creature_Animator");
            StagePrefab("Assets/_AAA_Imported/Enemies/Minions/Meshes/Minion_Lane_Ranged_Core_Dusk.glb", "Assets/Resources/Creatures/MinionRanged.prefab", mats["Mob_RangedMat"], forceAllSlots: true, animatorName: "Creature_Animator");
        }

        private static void EnsureValidRenderMaterials(GameObject go, Material fallbackMat)
        {
            var lit = Shader.Find("HDRP/Lit");
            foreach (var r in go.GetComponentsInChildren<Renderer>(true))
            {
                var shared = r.sharedMaterials;
                bool changed = false;
                for (int i = 0; i < shared.Length; i++)
                {
                    var m = shared[i];
                    string shName = m != null && m.shader != null ? m.shader.name : string.Empty;
                    bool needsUpgrade = m == null || m.shader == null || shName.Contains("Error") ||
                                        shName == "Standard" || shName.StartsWith("Universal Render Pipeline/", StringComparison.Ordinal);
                    if (needsUpgrade)
                    {
                        if (m != null && m.shader != null &&
                            (shName == "Standard" || shName.StartsWith("Universal Render Pipeline/", StringComparison.Ordinal)) &&
                            lit != null)
                        {
                            var tex = m.HasProperty("_BaseMap") ? m.GetTexture("_BaseMap") : (m.HasProperty("_MainTex") ? m.mainTexture : null);
                            var col = m.HasProperty("_BaseColor") ? m.GetColor("_BaseColor") : (m.HasProperty("_Color") ? m.color : Color.white);
                            var cloned = new Material(lit);
                            if (tex != null && cloned.HasProperty("_BaseColorMap")) cloned.SetTexture("_BaseColorMap", tex);
                            if (cloned.HasProperty("_BaseColor")) cloned.SetColor("_BaseColor", col);
                            AetheriaHdrpConvert.ValidateHdMaterial(cloned);
                            shared[i] = cloned;
                        }
                        else
                        {
                            shared[i] = fallbackMat;
                        }
                        changed = true;
                    }
                }
                if (changed) r.sharedMaterials = shared;
            }
        }

        private static void BuildMainScene(Dictionary<string, Material> mats, TerrainLayer[] layers)
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            // HDRP first: the pipeline asset and the look profile must exist before the volume is created.
            AetheriaHdrpSetup.Configure();
            AetheriaHdrpSetup.BuildLook();

            // 1. Directional sun. Sky, fog and post come from the HDRP volume profile (AetheriaHdrpSetup.BuildLook).
            var sunGo = new GameObject("AetheriaSunLight");
            var sun = sunGo.AddComponent<Light>();
            sun.type = LightType.Directional;
            sun.intensity = 2.8f;
            sun.color = new Color(1.0f, 0.78f, 0.52f);
            sun.shadows = LightShadows.Soft;
            sunGo.transform.rotation = Quaternion.Euler(18f, 155f, 0f);

            var volGo = new GameObject("GlobalVolume");
            var vol = volGo.AddComponent<Volume>();
            vol.isGlobal = true;
            vol.sharedProfile = AssetDatabase.LoadAssetAtPath<VolumeProfile>(AetheriaHdrpSetup.ProfilePath);

            // 2. Sculpted 4km x 4km 5-Biome PBR Terrain
            Terrain terrain = CreateSculptedTerrain(layers);

            // 3. World Manager Root
            var worldRoot = new GameObject("AetheriaWorldServices");
            var worldGen = worldRoot.AddComponent<AetheriaWorldGenerator>();
            worldGen.SunLight = sun;
            worldGen.GlobalVolume = vol;

            // 4. Mirror Lake Reflective Water Plane
            var lake = GameObject.CreatePrimitive(PrimitiveType.Cube);
            lake.name = "MirrorLakeSurface";
            lake.transform.position = new Vector3(14f, 0.65f, -8f);
            lake.transform.localScale = new Vector3(54f, 0.2f, 44f);
            lake.GetComponent<Renderer>().sharedMaterial = mats["MirrorLakeWater"];
            UnityEngine.Object.DestroyImmediate(lake.GetComponent<Collider>());
            worldGen.MirrorLakeSurface = lake.transform;

            // 5. Landmark Architecture & Props
            PopulateWorldLandmarks(terrain, mats);

            // 6. Hero Player + Main Camera
            var camGo = new GameObject("Main Camera") { tag = "MainCamera" };
            var cam = camGo.AddComponent<Camera>();
            cam.farClipPlane = 2400f;
            cam.fieldOfView = 60f;
            camGo.AddComponent<AudioListener>();
            var hdCam = camGo.AddComponent<HDAdditionalCameraData>();
            hdCam.antialiasing = HDAdditionalCameraData.AntialiasingMode.TemporalAntialiasing;
            hdCam.clearColorMode = HDAdditionalCameraData.ClearColorMode.Sky;

            float heroGroundY = SampleY(terrain, new Vector3(0f, 0f, 38f));
            var heroGo = new GameObject("HeroPlayer");
            heroGo.transform.position = new Vector3(0f, heroGroundY + 0.1f, 38f);
            heroGo.AddComponent<CharacterController>();
            var hero = heroGo.AddComponent<HeroController>();
            var vRoot = new GameObject("VisualRoot");
            vRoot.transform.SetParent(heroGo.transform, false);
            hero.VisualRoot = vRoot.transform;

            // Pre-pose Main Camera cleanly overlooking Hero + Open Oakhaven Plaza & Archway
            camGo.transform.position = new Vector3(1.6f, heroGroundY + 2.4f, 33.4f);
            camGo.transform.LookAt(new Vector3(0f, heroGroundY + 1.5f, 44f));

            // 7. Named NPCs, Dragon Ignis, Boss Malakor, Enemies & Breakable Quest Props
            SpawnEntitiesAndQuests(terrain, mats);

            // HDRP owns the look: shaders, light units, camera data, sky and the terrain material.
            AetheriaHdrpConvert.ConvertMaterials();
            AetheriaHdrpConvert.FixSceneObjects();

            EditorSceneManager.SaveScene(scene, ScenePath);
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
        }

        private static Terrain CreateSculptedTerrain(TerrainLayer[] layers)
        {
            const int HRes = 513;
            const int ARes = 512;
            var td = new TerrainData
            {
                heightmapResolution = HRes,
                alphamapResolution = ARes,
                size = new Vector3(4000f, 120f, 4000f),
                terrainLayers = layers
            };

            float[,] heights = new float[HRes, HRes];
            float[,,] alphas = new float[ARes, ARes, layers.Length];

            for (int y = 0; y < HRes; y++)
            {
                float wz = -2000f + (y / (float)(HRes - 1)) * 4000f;
                for (int x = 0; x < HRes; x++)
                {
                    float wx = -2000f + (x / (float)(HRes - 1)) * 4000f;
                    heights[y, x] = Mathf.Clamp01(EvaluateDesiredHeight(wx, wz) / 120f);
                }
            }
            td.SetHeights(0, 0, heights);

            for (int y = 0; y < ARes; y++)
            {
                float wz = -2000f + ((y + 0.5f) / ARes) * 4000f;
                for (int x = 0; x < ARes; x++)
                {
                    float wx = -2000f + ((x + 0.5f) / ARes) * 4000f;
                    float steep = td.GetSteepness((x + 0.5f) / ARes, (y + 0.5f) / ARes);
                    ComputeBiomeWeights(wx, wz, steep, out float l0, out float l1, out float l2, out float l3, out float l4);
                    alphas[y, x, 0] = l0;
                    alphas[y, x, 1] = l1;
                    alphas[y, x, 2] = l2;
                    alphas[y, x, 3] = l3;
                    alphas[y, x, 4] = l4;
                }
            }
            td.SetAlphamaps(0, 0, alphas);

            SaveOrReplaceAsset(td, "Assets/Generated/AetheriaTerrainData.asset");
            var loadedTd = AssetDatabase.LoadAssetAtPath<TerrainData>("Assets/Generated/AetheriaTerrainData.asset");

            var tGo = Terrain.CreateTerrainGameObject(loadedTd);
            tGo.name = "AetheriaTerrain";
            tGo.transform.position = new Vector3(-2000f, 0f, -2000f);
            return tGo.GetComponent<Terrain>();
        }

        private static float SmoothRange(float edge0, float edge1, float x) =>
            Mathf.SmoothStep(0f, 1f, Mathf.InverseLerp(edge0, edge1, x));

        private static float EvaluateDesiredHeight(float x, float z)
        {
            float baseH = 3.2f + Mathf.PerlinNoise(x * 0.012f + 100f, z * 0.012f + 100f) * 4.5f
                               + Mathf.PerlinNoise(x * 0.04f, z * 0.04f) * 1.4f;

            // Oakhaven Valley Plaza (0, 42) flat ~2.4m
            float dOak = Vector2.Distance(new Vector2(x, z), new Vector2(0f, 42f));
            if (dOak < 36f) baseH = Mathf.Lerp(2.4f, baseH, SmoothRange(24f, 36f, dOak));

            // Mirror Lake Basin (14, -8) carved down to -2.2m
            float dLake = Vector2.Distance(new Vector2(x * 0.85f, z), new Vector2(14f * 0.85f, -8f));
            if (dLake < 28f) baseH = Mathf.Lerp(-2.2f, baseH, SmoothRange(14f, 28f, dLake));

            // Prologue Awakening Cave Cliff Plateau (-27, -8) at ~22m
            float dCave = Vector2.Distance(new Vector2(x, z), new Vector2(-27f, -8f));
            if (dCave < 22f) baseH = Mathf.Lerp(22.0f, baseH, SmoothRange(11f, 22f, dCave));

            // Dragon Aerie (-34, 22) elevated crag ~7.5m
            float dAerie = Vector2.Distance(new Vector2(x, z), new Vector2(-34f, 22f));
            if (dAerie < 20f) baseH = Mathf.Lerp(7.5f, baseH, SmoothRange(12f, 20f, dAerie));

            // Highkeep Citadel (0, -104) grand approach ramp z = -42..-104 up to 16m
            if (Mathf.Abs(x) < 38f && z < -42f && z > -148f)
            {
                float rampT = Mathf.Clamp01((-42f - z) / 56f);
                float targetH = Mathf.Lerp(2.6f, 16.0f, rampT);
                float edgeBlend = 1f - SmoothRange(22f, 38f, Mathf.Abs(x));
                baseH = Mathf.Lerp(baseH, targetH, edgeBlend);
            }

            // Ash Caldera (94, -24) cauldron crater rim + sunken floor
            float dCal = Vector2.Distance(new Vector2(x, z), new Vector2(94f, -24f));
            if (dCal < 38f)
            {
                float ring = Mathf.Exp(-Mathf.Pow((dCal - 28f) / 6f, 2f)) * 9.5f;
                float inner = dCal < 22f ? Mathf.Lerp(2.0f, baseH, SmoothRange(16f, 22f, dCal)) : baseH;
                baseH = inner + ring;
            }

            // Crystal Glacier Peaks (-82, -68) elevated ice shelf 12m
            float dGlac = Vector2.Distance(new Vector2(x, z), new Vector2(-82f, -68f));
            if (dGlac < 44f) baseH = Mathf.Lerp(12.2f, baseH, SmoothRange(24f, 44f, dGlac));

            // Distant Mountain Ring framing the valley horizon (150m..460m)
            float radial = Mathf.Sqrt(x * x + z * z);
            if (radial > 145f)
            {
                float rimT = Mathf.Clamp01((radial - 145f) / 170f);
                float ridge = Mathf.PerlinNoise(x * 0.008f + 50f, z * 0.008f + 50f) * 38f
                            + Mathf.PerlinNoise(x * 0.022f + 12f, z * 0.022f + 12f) * 18f;
                baseH += rimT * (14f + ridge);
            }

            return Mathf.Max(0.1f, baseH);
        }

        private static void ComputeBiomeWeights(float x, float z, float steep,
            out float stone, out float soil, out float cliff, out float glacier, out float ash)
        {
            stone = soil = cliff = glacier = ash = 0f;

            // Stone Road & Plazas
            bool onRoad = (Mathf.Abs(x - 2f) < 5.5f && z > -112f && z < 56f) ||
                          Vector2.Distance(new Vector2(x, z), new Vector2(0f, 42f)) < 26f ||
                          Vector2.Distance(new Vector2(x, z), new Vector2(0f, -104f)) < 28f ||
                          Vector2.Distance(new Vector2(x, z), new Vector2(-34f, 22f)) < 11f;

            if (steep > 25f) cliff = 1f;
            else if (onRoad) stone = 1f;
            else
            {
                var b = QuestAndMapService.GetBiomeAt(new Vector3(x, 0f, z));
                switch (b)
                {
                    case BiomeId.CrystalGlacier:  glacier = 1f; break;
                    case BiomeId.AshCaldera:      ash = 1f; break;
                    case BiomeId.HighkeepCitadel: stone = 0.65f; cliff = 0.35f; break;
                    default:                      soil = 0.85f; stone = 0.15f; break;
                }
            }
            float sum = Mathf.Max(0.0001f, stone + soil + cliff + glacier + ash);
            stone /= sum; soil /= sum; cliff /= sum; glacier /= sum; ash /= sum;
        }

        private static void PopulateWorldLandmarks(Terrain terrain, Dictionary<string, Material> mats)
        {
            var envRoot = new GameObject("AetheriaEnvironment").transform;

            GameObject PlaceStatic(string assetPath, Material overrideMat, Vector3 xzPos, float targetH, float yaw, string label = null, float pitchOffset = 0f)
            {
                var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(assetPath);
                if (prefab == null) return null;
                var go = (GameObject)PrefabUtility.InstantiatePrefab(prefab);
                if (!string.IsNullOrEmpty(label)) go.name = label;
                go.transform.SetParent(envRoot, true);
                if (overrideMat != null)
                {
                    foreach (var r in go.GetComponentsInChildren<Renderer>(true)) r.sharedMaterial = overrideMat;
                }
                else
                {
                    EnsureValidRenderMaterials(go, mats["RomanStoneFloor"]);
                }
                // Preserve baked prefab Z-up conversion (e.g. -90 X on Megascans/UE FBX) while rotating around world Y!
                go.transform.rotation = Quaternion.Euler(0f, yaw, 0f) * go.transform.rotation * Quaternion.Euler(pitchOffset, 0f, 0f);
                NormalizeAndGround(go, terrain, xzPos, targetH);
                go.isStatic = true;
                return go;
            }

            // Oakhaven Sanctuary Courtyard — Clean Y-Up GLB Gothic Ruins (100% clear central sightline!)
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_arch.glb", null, new Vector3(0f, 0f, 59f), 8.2f, 0f, "OakhavenMainArch");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_statue.glb", null, new Vector3(11.5f, 0f, 50f), 5.6f, 215f, "OakhavenHeroStatue");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_shrine_altar.glb", null, new Vector3(-14f, 0f, 48f), 3.6f, 35f, "Shrine_OakhavenForge");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_brazier.glb", null, new Vector3(-6.5f, 0f, 44f), 2.1f, 0f);
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_brazier.glb", null, new Vector3(6.5f, 0f, 44f), 2.1f, 0f);
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", null, new Vector3(-15f, 0f, 36f), 6.8f, 0f);
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", null, new Vector3(15f, 0f, 36f), 6.8f, 0f);
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_barrel_intact.glb", null, new Vector3(11f, 0f, 42f), 1.35f, 20f);
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_chest.glb", null, new Vector3(-11f, 0f, 42f), 1.25f, -35f);

            // Mirror Lake Arched Stone Bridge (z = -8, bridging lake waters cleanly)
            var br = PlaceStatic("Assets/_AAA_Imported/Environment/Gothic_props_1/Meshes/SM_Bridge1.fbx", mats["GothicBridgeMat"], new Vector3(14f, 0f, -8f), 5.2f, 90f, "MirrorLakeStoneBridge");
            if (br != null) { br.transform.position = new Vector3(14f, 0.55f, -8f); br.transform.localScale *= 2.0f; }

            // Prologue Awakening Cave (-27, -8) & Mist Cascade into Mirror Lake
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_boulder.glb", null, new Vector3(-36f, 0f, -15.5f), 5.2f, 35f, "CaveEntranceBoulderL");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_boulder.glb", null, new Vector3(-34.5f, 0f, -1.5f), 5.0f, 140f, "CaveEntranceBoulderR");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_arch.glb", null, new Vector3(-20.5f, 0f, -8f), 6.2f, 90f, "PrologueCliffArch");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_brazier.glb", null, new Vector3(-29.5f, 0f, -5.6f), 2.0f, 0f);
            AddTorches(envRoot, new Vector3(-28.5f, SampleY(terrain, new Vector3(-28.5f, 0f, -6.5f)) + 1.8f, -6.5f), new Color(1f, 0.56f, 0.16f), 22f, 8.5f);

            // Cliffside Mist Cascade below Cliff Lip (-15.5, -8) — zero geometry pipes!
            var wfMist = new GameObject("PrologueWaterfallMist");
            wfMist.transform.SetParent(envRoot, true);
            wfMist.transform.position = new Vector3(-15.5f, SampleY(terrain, new Vector3(-15.5f, 0f, -8f)) - 1.5f, -8f);
            var wfPs = wfMist.AddComponent<ParticleSystem>();
            var wfMain = wfPs.main;
            wfMain.startColor = new Color(0.72f, 0.90f, 1.0f, 0.35f);
            wfMain.startSpeed = new ParticleSystem.MinMaxCurve(3.5f, 7.5f);
            wfMain.startSize = new ParticleSystem.MinMaxCurve(1.4f, 3.2f);
            wfMain.startLifetime = 2.1f;
            wfMain.gravityModifier = 0.45f;
            var wfEm = wfPs.emission; wfEm.rateOverTime = 28f;
            var wfSh = wfPs.shape; wfSh.shapeType = ParticleSystemShapeType.Box; wfSh.scale = new Vector3(1.2f, 0.5f, 4.5f);
            wfPs.GetComponent<ParticleSystemRenderer>().sharedMaterial = AssetDatabase.LoadAssetAtPath<Material>("Assets/Resources/Materials/URP_Particles_Unlit_Template.mat");

            // Dragon Aerie Shrine (-34, 22)
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_arch.glb", null, new Vector3(-40f, 0f, 22f), 7.5f, 90f, "DragonAerieArch");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_boulder.glb", null, new Vector3(-44f, 0f, 28f), 6.4f, -40f);
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_boulder.glb", null, new Vector3(-27f, 0f, 29f), 4.2f, 55f);

            // Sapphire Whispering Woods & Valley Foliage (Megascans DeadTrees + DryGrass + Base Crystalline Shards)
            string[] grassPrefabs =
            {
                "Assets/_AAA_Imported/Megascans/3D/DryGrass/SM_Dry_Grass_tbbqejqr_01_N1.fbx",
                "Assets/_AAA_Imported/Megascans/3D/DryGrass/SM_Dry_Grass_tbbqejqr_02_N1.fbx",
                "Assets/_AAA_Imported/Megascans/3D/DryGrass/SM_Dry_Grass_tbbqejqr_03_N1.fbx",
                "Assets/_AAA_Imported/Megascans/3D/DryGrass/SM_Dry_Grass_tbbqejqr_04_N1.fbx",
                "Assets/_AAA_Imported/Megascans/3D/DryGrass/SM_Dry_Grass_tbbqejqr_05_N1.fbx",
            };
            for (int i = 0; i < 16; i++)
            {
                float a = i * (360f / 16f) * Mathf.Deg2Rad;
                Vector3 tp = new Vector3(64f + Mathf.Cos(a) * (12f + (i % 3) * 11f), 0f, 32f + Mathf.Sin(a) * (12f + (i % 3) * 11f));
                PlaceStatic("Assets/_AAA_Imported/Megascans/3D/DeadTrees/SM_Dead_Tree_qlEtl_01_N1.fbx", mats["DeadTree3D"], tp, 7.5f + (i % 3) * 1.5f, i * 47f);
                PlaceStatic(grassPrefabs[i % grassPrefabs.Length], mats["RockySoil"], tp + new Vector3(1.6f, 0f, -1.2f), 0.95f + (i % 2) * 0.35f, i * 73f);
                if (i % 3 == 0)
                    PlaceStatic("Assets/_AAA_Imported/Megascans/3D/LimestoneRocks/SM_Limestone_Rocks_N1.fbx", mats["CrystalIce"], tp + new Vector3(-1.4f, 0f, 1.3f), 1.4f, i * 29f);
            }

            // Valley & Oakhaven Wild Grass Clusters + Road Boulders
            Vector2[] valleyBoulders = { new(-22, 52), new(24, 54), new(-26, 36), new(26, 26), new(32, -4), new(-16, -24), new(22, -26), new(34, 18), new(-22, -48), new(24, -48) };
            for (int i = 0; i < valleyBoulders.Length; i++)
            {
                Vector3 p = new Vector3(valleyBoulders[i].x, 0f, valleyBoulders[i].y);
                PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_boulder.glb", null, p, 3.2f + (i % 3) * 1.6f, i * 67f);
                PlaceStatic(grassPrefabs[i % grassPrefabs.Length], mats["RockySoil"], p + new Vector3(2.1f, 0f, -1.5f), 0.9f, i * 51f);
                if (i % 2 == 0)
                    PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", null, p + new Vector3(3.5f, 0f, 2.0f), 5.4f, i * 31f);
            }

            // Ash Caldera & Ember Crypt (94, -24) — Molten Magma Core & Obsidian Pillars
            var lavaPool = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            lavaPool.name = "EmberCryptMagmaPool";
            lavaPool.transform.SetParent(envRoot, true);
            lavaPool.transform.position = new Vector3(94f, SampleY(terrain, new Vector3(94f, 0f, -24f)) + 0.15f, -24f);
            lavaPool.transform.localScale = new Vector3(22f, 0.25f, 22f);
            lavaPool.GetComponent<Renderer>().sharedMaterial = mats["EmberLava"];
            AddTorches(envRoot, lavaPool.transform.position + Vector3.up * 3.5f, new Color(1f, 0.34f, 0.08f), 28f, 12f);
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", null, new Vector3(88f, 0f, -18f), 8.2f, 0f);
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", null, new Vector3(100f, 0f, -30f), 8.2f, 0f);

            // Crystal Glacier Peaks (-82, -68) — Sculpted Crystalline Rock Spires & Warming Bonfires
            for (int i = 0; i < 8; i++)
            {
                float a = i * 45f * Mathf.Deg2Rad;
                Vector3 gp = new Vector3(-82f + Mathf.Cos(a) * 16f, 0f, -68f + Mathf.Sin(a) * 16f);
                PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_boulder.glb", mats["CrystalIce"], gp, 6.5f + (i % 2) * 2.5f, i * 45f);
            }
            AddTorches(envRoot, new Vector3(-76f, SampleY(terrain, new Vector3(-76f, 0f, -60f)) + 1.4f, -60f), new Color(1f, 0.52f, 0.15f), 24f, 8f);

            // Highkeep Citadel Arena (0, -104) — Upright Y-Up GLB Pillars, Archway, Statues & Braziers!
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_arch.glb", null, new Vector3(0f, 0f, -74f), 9.5f, 0f, "HighkeepGateArch");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_statue.glb", null, new Vector3(-10f, 0f, -114f), 6.2f, 145f, "HighkeepGuardianStatueL");
            PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_statue.glb", null, new Vector3(10f, 0f, -114f), 6.2f, -145f, "HighkeepGuardianStatueR");
            for (int z = -76; z >= -116; z -= 10)
            {
                PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", null, new Vector3(-14f, 0f, z), 8.5f, 0f);
                PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", null, new Vector3(14f, 0f, z), 8.5f, 0f);
                PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_brazier.glb", null, new Vector3(-11.5f, 0f, z), 2.1f, 0f);
                PlaceStatic("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_brazier.glb", null, new Vector3(11.5f, 0f, z), 2.1f, 0f);
                AddTorches(envRoot, new Vector3(-11.5f, SampleY(terrain, new Vector3(-11.5f, 0f, z)) + 2.4f, z), new Color(1f, 0.28f, 0.12f), 22f, 7f);
                AddTorches(envRoot, new Vector3(11.5f, SampleY(terrain, new Vector3(11.5f, 0f, z)) + 2.4f, z), new Color(1f, 0.28f, 0.12f), 22f, 7f);
            }
        }

        private static void SpawnEntitiesAndQuests(Terrain terrain, Dictionary<string, Material> mats)
        {
            var entRoot = new GameObject("AetheriaEntities").transform;

            void SpawnNPC(string id, string display, string role, string questId, string prefabRes, Vector3 xz, float yaw)
            {
                var p = AssetDatabase.LoadAssetAtPath<GameObject>($"Assets/Resources/{prefabRes}.prefab");
                var go = p != null ? (GameObject)PrefabUtility.InstantiatePrefab(p) : GameObject.CreatePrimitive(PrimitiveType.Capsule);
                go.name = id;
                go.transform.SetParent(entRoot, true);
                go.transform.rotation = Quaternion.Euler(0f, yaw, 0f);
                NormalizeAndGround(go, terrain, xz, 1.85f);
                AetheriaPalette.RelaxHumanoidArms(go);
                var npc = go.AddComponent<AetheriaNPC>();
                npc.NpcId = id; npc.DisplayName = display; npc.RoleTitle = role; npc.QuestId = questId;
            }

            SpawnNPC("Npc_CaveTutor", "Хранитель Вейлен", "Awakening Guardian", "Q_PrologueAwakening", "Heroes/Hero_Gideon", new Vector3(-26.8f, 0f, -7.8f), 238f);
            SpawnNPC("ElderAldric", "Elder Aldric", "Keeper of Oakhaven", "Q_AwakenExort", "Heroes/Hero_GideonInq", new Vector3(-3.5f, 0f, 44.5f), 160f);
            SpawnNPC("ScholarLyra", "Scholar Lyra", "Dragon Lorekeeper", "Q_TameDragon", "Heroes/Hero_Serath", new Vector3(-16f, 0f, 33f), 120f);
            SpawnNPC("BlacksmithTorin", "Blacksmith Torin", "Storm-Forge Master", "Q_StormSpires", "Heroes/Hero_Khaimera", new Vector3(12.5f, 0f, 46f), 205f);
            SpawnNPC("WardenKael", "Warden Kael", "Ash Caldera Watcher", "Q_EmberCrypt", "Heroes/Hero_KwangManbun", new Vector3(28f, 0f, -8f), 270f);
            SpawnNPC("SeerVaelis", "Seer Vaelis", "Glacier Oracle", "Q_GlacierSilence", "Heroes/Hero_MorigeshMystic", new Vector3(-44f, 0f, -36f), 45f);
            SpawnNPC("CaptainRowena", "Captain Rowena", "Highkeep Vanguard", "Q_FallOfSovereign", "Heroes/Hero_SerathFrost", new Vector3(0f, 0f, -66f), 180f);

            // Rideable Crimson Dragon Ignis on Dragon Aerie (-34, 22)
            var drakePrefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Resources/Creatures/IgnisDragon.prefab");
            var drakeGo = drakePrefab != null ? (GameObject)PrefabUtility.InstantiatePrefab(drakePrefab) : GameObject.CreatePrimitive(PrimitiveType.Capsule);
            drakeGo.name = "IgnisCrimsonDragon";
            drakeGo.transform.SetParent(entRoot, true);
            drakeGo.transform.rotation = Quaternion.Euler(0f, 120f, 0f);
            NormalizeAndGround(drakeGo, terrain, new Vector3(-34f, 0f, 22f), 4.8f);
            drakeGo.AddComponent<DragonController>();

            // Boss Malakor inside Highkeep Citadel (0, -104)
            var bossPrefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Resources/Creatures/MalakorBoss.prefab");
            var bossGo = bossPrefab != null ? (GameObject)PrefabUtility.InstantiatePrefab(bossPrefab) : GameObject.CreatePrimitive(PrimitiveType.Capsule);
            bossGo.name = "Boss_Malakor";
            bossGo.transform.SetParent(entRoot, true);
            bossGo.transform.rotation = Quaternion.Euler(0f, 0f, 0f);
            NormalizeAndGround(bossGo, terrain, new Vector3(0f, 0f, -104f), 3.6f);
            AetheriaPalette.RelaxHumanoidArms(bossGo);
            var bCap = bossGo.AddComponent<CapsuleCollider>(); bCap.center = new Vector3(0f, 1.8f, 0f); bCap.radius = 0.85f; bCap.height = 3.6f;
            var bossAi = bossGo.AddComponent<EnemyAndBossAI>();
            bossAi.IsBoss = true; bossAi.DisplayName = "Malakor, Sovereign of the Shattered Seal";
            bossAi.QuestIdOnDeath = "Q_FallOfSovereign"; bossAi.MaxHP = 680f; bossAi.HP = 680f;
            bossAi.AggroRadius = 26f; bossAi.AttackRange = 3.8f; bossAi.AttackDamage = 28f;

            // Biome Enemies & Breakable Quest Props
            void SpawnEnemy(string name, string prefabRes, EnemyArchetype arch, Vector3 xz, float targetH, float hp, string questId)
            {
                var pr = AssetDatabase.LoadAssetAtPath<GameObject>($"Assets/Resources/{prefabRes}.prefab");
                var go = pr != null ? (GameObject)PrefabUtility.InstantiatePrefab(pr) : GameObject.CreatePrimitive(PrimitiveType.Capsule);
                go.name = name;
                go.transform.SetParent(entRoot, true);
                NormalizeAndGround(go, terrain, xz, targetH);
                var cc = go.AddComponent<CapsuleCollider>(); cc.center = new Vector3(0f, targetH * 0.5f, 0f); cc.radius = 0.55f; cc.height = targetH;
                var ai = go.AddComponent<EnemyAndBossAI>();
                ai.Archetype = arch; ai.DisplayName = name; ai.MaxHP = hp; ai.HP = hp; ai.QuestIdOnDeath = questId;
            }

            SpawnEnemy("RisenOutrider_1", "Creatures/MinionMelee", EnemyArchetype.Skeleton, new Vector3(-6f, 0f, 22f), 1.9f, 95f, "");
            SpawnEnemy("RisenOutrider_2", "Creatures/MinionMelee", EnemyArchetype.Skeleton, new Vector3(7f, 0f, 24f), 1.9f, 95f, "");
            SpawnEnemy("TempestStalker", "Creatures/Whisper", EnemyArchetype.TempestStalker, new Vector3(66f, 0f, 34f), 2.3f, 220f, "Q_StormSpires");
            SpawnEnemy("CalderaBrute", "Creatures/MinionRanged", EnemyArchetype.Skeleton, new Vector3(92f, 0f, -18f), 2.1f, 180f, "");
            SpawnEnemy("FrostWarden", "Creatures/Rampage", EnemyArchetype.FrostWarden, new Vector3(-82f, 0f, -68f), 2.9f, 340f, "Q_GlacierSilence");

            void SpawnQuestBreakable(string name, string assetPath, Material mat, Vector3 xz, float targetH, DestructibleKind kind, string qId, bool sun, bool meteor, bool tintWholeMesh = true)
            {
                var pr = AssetDatabase.LoadAssetAtPath<GameObject>(assetPath);
                var go = pr != null ? (GameObject)PrefabUtility.InstantiatePrefab(pr) : GameObject.CreatePrimitive(PrimitiveType.Cylinder);
                go.name = name;
                go.transform.SetParent(entRoot, true);
                if (tintWholeMesh)
                {
                    foreach (var r in go.GetComponentsInChildren<Renderer>(true)) r.sharedMaterial = mat;
                }
                else
                {
                    EnsureValidRenderMaterials(go, kind == DestructibleKind.Stone ? mats["RomanStoneFloor"] : mats["Boulder3D"]);
                }
                NormalizeAndGround(go, terrain, xz, targetH);
                if (!tintWholeMesh)
                {
                    var orb = GameObject.CreatePrimitive(PrimitiveType.Sphere);
                    orb.name = $"{name}_CrystalCore";
                    UnityEngine.Object.DestroyImmediate(orb.GetComponent<Collider>());
                    orb.transform.SetParent(go.transform, true);
                    orb.transform.position = new Vector3(xz.x, SampleY(terrain, xz) + targetH + 0.35f, xz.z);
                    orb.transform.localScale = Vector3.one * 0.48f;
                    orb.GetComponent<Renderer>().sharedMaterial = mat;
                    var pl = orb.AddComponent<Light>();
                    pl.type = LightType.Point;
                    pl.color = mat != null ? mat.color : new Color(1f, 0.55f, 0.16f);
                    pl.range = 10f;
                    pl.intensity = 4.5f;
                }
                if (go.GetComponentInChildren<Collider>() == null)
                {
                    var box = go.AddComponent<BoxCollider>();
                    box.size = new Vector3(1.5f, 3.0f, 1.5f);
                    box.center = new Vector3(0f, 1.5f, 0f);
                }
                var bp = go.AddComponent<BreakableProp>();
                bp.Kind = kind; bp.LinkedQuestId = qId; bp.RequiresSunStrike = sun; bp.RequiresMeteor = meteor; bp.HP = 70f; bp.MaxHP = 70f;
            }

            // Q_AwakenExort Beacons (Oakhaven Outer Ring)
            SpawnQuestBreakable("OakhavenBeacon_West", "Assets/_AAA_Imported/WizardRaw/GLBModels/prop_shrine_altar.glb", mats["BindCrystalGlow"], new Vector3(-8f, 0f, 54f), 2.8f, DestructibleKind.Stone, "Q_AwakenExort", false, false, false);
            SpawnQuestBreakable("OakhavenBeacon_East", "Assets/_AAA_Imported/WizardRaw/GLBModels/prop_shrine_altar.glb", mats["BindCrystalGlow"], new Vector3(8f, 0f, 54f), 2.8f, DestructibleKind.Stone, "Q_AwakenExort", false, false, false);
            // Q_TameDragon Bind-Crystals (Dragon Aerie)
            SpawnQuestBreakable("AerieBindCrystal_1", "Assets/_AAA_Imported/Megascans/3D/LimestoneRocks/SM_Limestone_Rocks_N1.fbx", mats["BindCrystalGlow"], new Vector3(-37.5f, 0f, 24f), 2.8f, DestructibleKind.Crystal, "Q_TameDragon", true, false, false);
            SpawnQuestBreakable("AerieBindCrystal_2", "Assets/_AAA_Imported/Megascans/3D/LimestoneRocks/SM_Limestone_Rocks_N1.fbx", mats["BindCrystalGlow"], new Vector3(-30.5f, 0f, 20f), 2.8f, DestructibleKind.Crystal, "Q_TameDragon", true, false, false);
            // Q_StormSpires Conductors (Sapphire Woods)
            SpawnQuestBreakable("TempestConductor_1", "Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", mats["WexConductorGlow"], new Vector3(60f, 0f, 28f), 4.2f, DestructibleKind.Crystal, "Q_StormSpires", false, false, false);
            SpawnQuestBreakable("TempestConductor_2", "Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", mats["WexConductorGlow"], new Vector3(68f, 0f, 30f), 4.2f, DestructibleKind.Crystal, "Q_StormSpires", false, false, false);
            // Q_EmberCrypt Obsidian Seal
            SpawnQuestBreakable("EmberCryptObsidianSeal", "Assets/_AAA_Imported/WizardRaw/GLBModels/prop_iron_gate.glb", mats["ObsidianSeal"], new Vector3(94f, 0f, -24f), 4.5f, DestructibleKind.Obsidian, "Q_EmberCrypt", false, true, true);
        }

        private static void NormalizeAndGround(GameObject go, Terrain terrain, Vector3 xz, float targetHeightMeters)
        {
            go.transform.position = new Vector3(xz.x, SampleY(terrain, xz), xz.z);
            go.transform.localScale = Vector3.one;
            var rends = go.GetComponentsInChildren<Renderer>(true);
            if (rends.Length > 0)
            {
                Bounds b = rends[0].bounds;
                for (int i = 1; i < rends.Length; i++) b.Encapsulate(rends[i].bounds);
                float scale = targetHeightMeters / Mathf.Max(0.05f, b.size.y);
                go.transform.localScale = Vector3.one * scale;
                rends = go.GetComponentsInChildren<Renderer>(true);
                b = rends[0].bounds;
                for (int i = 1; i < rends.Length; i++) b.Encapsulate(rends[i].bounds);
                float desiredY = SampleY(terrain, xz) - b.min.y + go.transform.position.y - 0.05f;
                go.transform.position = new Vector3(xz.x, desiredY, xz.z);
            }
        }

        private static void AddTorches(Transform parent, Vector3 pos, Color c, float range = 16f, float lum = 6f)
        {
            var go = new GameObject("TorchLight");
            go.transform.SetParent(parent, true);
            go.transform.position = pos;
            var l = go.AddComponent<Light>();
            l.type = LightType.Point; l.color = c; l.range = range; l.intensity = lum;
        }

        private static float SampleY(Terrain t, Vector3 p) =>
            t != null ? t.SampleHeight(p) + t.transform.position.y : 0f;

        private static void SaveOrReplaceAsset(UnityEngine.Object obj, string path)
        {
            if (File.Exists(path)) AssetDatabase.DeleteAsset(path);
            AssetDatabase.CreateAsset(obj, path);
            AssetDatabase.SaveAssets();
        }
    }
}
