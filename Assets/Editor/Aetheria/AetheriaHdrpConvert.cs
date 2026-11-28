using System;
using System.Collections.Generic;
using System.Reflection;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;
using UnityEngine.SceneManagement;

namespace Aetheria.Editor
{
    /// <summary>
    /// Converts everything authored for URP into HDRP: material shaders and their property names,
    /// the sun, the camera, the sky and the post-process volume. Without this the HDRP pipeline
    /// draws every URP material magenta, which is exactly what the build was doing.
    /// Idempotent: running it twice changes nothing.
    /// </summary>
    public static class AetheriaHdrpConvert
    {
        private const string ScenePath = "Assets/Scenes/AetheriaMain.unity";

        [MenuItem("Aetheria/3. Convert Project to HDRP")]
        public static void ConvertAll()
        {
            int materials = ConvertMaterials();
            ConvertOpenScene();
            AssetDatabase.SaveAssets();
            Debug.Log($"[HDRP] Converted {materials} materials and the main scene.");
        }

        // ------------------------------------------------------------------ materials

        /// <summary>Maps a URP/built-in shader name to its HDRP counterpart.</summary>
        private static string TargetShader(string current)
        {
            if (current.StartsWith("HDRP/")) return current;
            switch (current)
            {
                case "Universal Render Pipeline/Lit":
                case "Standard":
                case "Diffuse":
                case "Legacy Shaders/Diffuse":
                    return "HDRP/Lit";
                case "Universal Render Pipeline/Unlit":
                case "Unlit/Color":
                case "Unlit/Texture":
                case "Sprites/Default":
                    return "HDRP/Unlit";
                case "Universal Render Pipeline/Particles/Unlit":
                case "Particles/Standard Unlit":
                case "Legacy Shaders/Particles/Additive":
                case "Mobile/Particles/Additive":
                    return "HDRP/Unlit";          // HDRP 17 ships no dedicated particle shader
                case "Universal Render Pipeline/Terrain/Lit":
                case "Nature/Terrain/Standard":
                case "Nature/Terrain/Diffuse":
                    return "HDRP/TerrainLit";
                case "Skybox/Procedural":
                case "Skybox/6 Sided":
                case "Skybox/Cubemap":
                    return null;                  // HDRP skies live in the volume, not in a skybox material
                default:
                    // Anything else that is not HDRP is assumed to be a lit surface.
                    return current.Contains("Particles") ? "HDRP/Unlit"
                         : current.Contains("Unlit") || current.Contains("Sprite") ? "HDRP/Unlit"
                         : "HDRP/Lit";
            }
        }

        public static int ConvertMaterials()
        {
            var guids = AssetDatabase.FindAssets("t:Material");
            int converted = 0;

            foreach (var guid in guids)
            {
                string path = AssetDatabase.GUIDToAssetPath(guid);
                // Only our own content: package materials belong to the render pipelines themselves.
                if (!path.StartsWith("Assets/")) continue;
                var mat = AssetDatabase.LoadAssetAtPath<Material>(path);
                if (mat == null || mat.shader == null) continue;

                string target = TargetShader(mat.shader.name);
                if (target == null)
                {
                    // A URP skybox material: HDRP draws the sky from the volume instead.
                    Debug.Log($"[HDRP] {path}: skybox material no longer used (HDRP sky comes from the volume).");
                    continue;
                }
                if (mat.shader.name == target) continue;

                var shader = Shader.Find(target);
                if (shader == null)
                {
                    Debug.LogWarning($"[HDRP] Shader '{target}' not found for {path}; left as is.");
                    continue;
                }

                // Read what the URP material had before the swap.
                Color baseColor = ReadColor(mat, "_BaseColor", "_Color");
                Texture baseMap = ReadTexture(mat, "_BaseMap", "_MainTex");
                Texture bumpMap = ReadTexture(mat, "_BumpMap", "_DetailNormalMap");
                float smoothness = ReadFloat(mat, "_Smoothness", "_Glossiness", 0.25f);
                float metallic = ReadFloat(mat, "_Metallic", null, 0f);
                bool hasEmission = mat.IsKeywordEnabled("_EMISSION") || ReadColor(mat, "_EmissionColor", null).maxColorComponent > 0.001f;
                Color emission = ReadColor(mat, "_EmissionColor", null);
                // mainTexture* logs an error on shaders without _MainTex, so only read it when it exists.
                Vector2 tiling = mat.HasProperty("_MainTex") ? mat.mainTextureScale : Vector2.one;
                Vector2 offset = mat.HasProperty("_MainTex") ? mat.mainTextureOffset : Vector2.zero;
                bool transparent = IsTransparent(mat);
                // Foliage cards and chains need their cut-out and both faces; dropping them turns every
                // leaf, grass card and fur strip into an opaque, one-sided quad.
                bool alphaClip = mat.IsKeywordEnabled("_ALPHATEST_ON") || ReadFloat(mat, "_AlphaClip", null, 0f) > 0.5f;
                float cutoff = ReadFloat(mat, "_Cutoff", null, 0.5f);
                bool doubleSided = mat.HasProperty("_Cull") && mat.GetFloat("_Cull") < 0.5f;

                mat.shader = shader;

                WriteColor(mat, "_BaseColor", baseColor);
                WriteColor(mat, "_UnlitColor", baseColor);
                if (baseMap != null)
                {
                    WriteTexture(mat, "_BaseMap", baseMap);
                    WriteTexture(mat, "_UnlitColorMap", baseMap);
                }
                if (bumpMap != null)
                {
                    WriteTexture(mat, "_NormalMap", bumpMap);
                    WriteTexture(mat, "_NormalMapOS", bumpMap);
                    mat.SetFloat("_NormalMapSpace", 0f);   // tangent space
                    mat.EnableKeyword("_NORMALMAP");
                }
                WriteFloat(mat, "_Smoothness", smoothness);
                WriteFloat(mat, "_SmoothnessRemapMin", 0f);
                WriteFloat(mat, "_SmoothnessRemapMax", smoothness);
                WriteFloat(mat, "_Metallic", metallic);
                WriteFloat(mat, "_MetallicRemapMin", 0f);
                WriteFloat(mat, "_MetallicRemapMax", metallic);
                if (hasEmission)
                {
                    WriteColor(mat, "_EmissiveColor", emission);
                    WriteColor(mat, "_EmissiveColorLDR", emission);
                    WriteFloat(mat, "_EmissiveColorMode", 0f);
                    WriteFloat(mat, "_UseEmissiveIntensity", 0f);
                    mat.EnableKeyword("_EMISSIVE_COLOR");
                    mat.globalIlluminationFlags = MaterialGlobalIlluminationFlags.None;
                }
                WriteTexture(mat, "_BaseColorMap", baseMap);
                if (mat.HasProperty("_MainTex"))
                {
                    mat.mainTextureScale = tiling;
                    mat.mainTextureOffset = offset;
                }
                if (mat.HasProperty("_BaseColorMap"))
                {
                    mat.SetTextureScale("_BaseColorMap", tiling);
                    mat.SetTextureOffset("_BaseColorMap", offset);
                }
                if (mat.HasProperty("_NormalMap"))
                {
                    mat.SetTextureScale("_NormalMap", tiling);
                    mat.SetTextureOffset("_NormalMap", offset);
                }

                if (shader.name == "HDRP/Lit")
                {
                    mat.SetFloat("_MaterialID", 1f);          // Lit
                    mat.SetFloat("_SurfaceType", transparent ? 1f : 0f);
                    mat.SetFloat("_BlendMode", 0f);
                    mat.SetFloat("_ZWrite", transparent ? 0f : 1f);
                    mat.SetFloat("_SrcBlend", transparent ? (float)BlendMode.SrcAlpha : (float)BlendMode.One);
                    mat.SetFloat("_DstBlend", transparent ? (float)BlendMode.OneMinusSrcAlpha : (float)BlendMode.Zero);
                    mat.SetFloat("_AlphaSrcBlend", 1f);
                    mat.SetFloat("_AlphaDstBlend", transparent ? (float)BlendMode.OneMinusSrcAlpha : 0f);
                    mat.SetFloat("_AlphaCutoffEnable", alphaClip ? 1f : 0f);
                    if (alphaClip)
                    {
                        WriteFloat(mat, "_AlphaCutoff", cutoff);
                        WriteFloat(mat, "_AlphaCutoffShadow", cutoff);
                        WriteFloat(mat, "_UseShadowThreshold", 1f);
                    }
                    mat.SetFloat("_EnableBlendModePreserveSpecularLighting", transparent ? 1f : 0f);
                    mat.SetFloat("_EnableFogOnTransparent", 1f);
                    mat.SetFloat("_DoubleSidedEnable", doubleSided || alphaClip ? 1f : 0f);
                    mat.SetFloat("_ReceivesSSR", 1f);
                    mat.SetFloat("_SupportDecals", 1f);
                    mat.SetFloat("_SupportDecalsTransparent", 0f);
                    mat.SetFloat("_TransparentBackfaceEnable", 0f);
                }
                else if (shader.name == "HDRP/Unlit")
                {
                    mat.SetFloat("_SurfaceType", transparent ? 1f : 0f);
                    mat.SetFloat("_BlendMode", 0f);
                    mat.SetFloat("_ZWrite", transparent ? 0f : 1f);
                    mat.SetFloat("_SrcBlend", transparent ? (float)BlendMode.SrcAlpha : (float)BlendMode.One);
                    mat.SetFloat("_DstBlend", transparent ? (float)BlendMode.OneMinusSrcAlpha : (float)BlendMode.Zero);
                    mat.SetFloat("_EnableFogOnTransparent", 1f);
                }

                if (transparent)
                {
                    mat.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
                    mat.SetOverrideTag("RenderType", "Transparent");
                    mat.renderQueue = (int)RenderQueue.Transparent;
                }
                else
                {
                    mat.DisableKeyword("_SURFACE_TYPE_TRANSPARENT");
                    mat.SetOverrideTag("RenderType", "Opaque");
                    mat.renderQueue = (int)RenderQueue.Geometry;
                }
                mat.SetFloat("_StencilRef", 0f);
                ValidateHdMaterial(mat);

                EditorUtility.SetDirty(mat);
                converted++;
            }

            if (converted > 0) AssetDatabase.SaveAssets();
            return converted;
        }

        private static bool IsTransparent(Material m)
        {
            if (m.renderQueue >= (int)RenderQueue.Transparent) return true;
            if (m.IsKeywordEnabled("_SURFACE_TYPE_TRANSPARENT")) return true;
            if (m.HasProperty("_SurfaceType") && m.GetFloat("_SurfaceType") > 0.5f) return true;
            if (m.HasProperty("_Mode") && Mathf.Approximately(m.GetFloat("_Mode"), 3f)) return true;
            return false;
        }

        private static Color ReadColor(Material m, string a, string b)
        {
            if (a != null && m.HasProperty(a)) return m.GetColor(a);
            if (b != null && m.HasProperty(b)) return m.GetColor(b);
            return Color.white;
        }

        private static Texture ReadTexture(Material m, string a, string b)
        {
            if (a != null && m.HasProperty(a)) { var t = m.GetTexture(a); if (t != null) return t; }
            if (b != null && m.HasProperty(b)) { var t = m.GetTexture(b); if (t != null) return t; }
            return null;
        }

        private static float ReadFloat(Material m, string a, string b, float fallback)
        {
            if (a != null && m.HasProperty(a)) return m.GetFloat(a);
            if (b != null && m.HasProperty(b)) return m.GetFloat(b);
            return fallback;
        }

        private static void WriteColor(Material m, string prop, Color c)
        {
            if (m.HasProperty(prop)) m.SetColor(prop, c);
        }

        private static void WriteTexture(Material m, string prop, Texture t)
        {
            if (t != null && m.HasProperty(prop)) m.SetTexture(prop, t);
        }

        private static void WriteFloat(Material m, string prop, float v)
        {
            if (m.HasProperty(prop)) m.SetFloat(prop, v);
        }

        private static MethodInfo _validateMaterialMethod;
        private static bool _validateMaterialLookupDone;

        public static void ValidateHdMaterial(Material mat)
        {
            if (mat == null || mat.shader == null || !mat.shader.name.StartsWith("HDRP/")) return;
            if (!_validateMaterialLookupDone)
            {
                _validateMaterialLookupDone = true;
                var hdMatType = Type.GetType("UnityEditor.Rendering.HighDefinition.HDMaterial, Unity.RenderPipelines.HighDefinition.Editor");
                _validateMaterialMethod = hdMatType?.GetMethod("ValidateMaterial", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static, null, new[] { typeof(Material) }, null);
            }
            _validateMaterialMethod?.Invoke(null, new object[] { mat });
        }

        private static void RemoveComponentByTypeName(GameObject go, string fullTypeName)
        {
            if (go == null) return;
            foreach (var asm in AppDomain.CurrentDomain.GetAssemblies())
            {
                var t = asm.GetType(fullTypeName, false);
                if (t != null && typeof(Component).IsAssignableFrom(t))
                {
                    var comp = go.GetComponent(t);
                    if (comp != null) UnityEngine.Object.DestroyImmediate(comp, true);
                    return;
                }
            }
        }

        // ------------------------------------------------------------------ scene

        public static void ConvertOpenScene()
        {
            var scene = EditorSceneManager.GetActiveScene();
            if (!scene.IsValid() || string.IsNullOrEmpty(scene.path))
                scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            FixSceneObjects();
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, ScenePath);
        }

        private const string TerrainMaterialPath = "Assets/Generated/Materials/AetheriaTerrainLit.mat";

        /// <summary>
        /// Lights, cameras, volume, sky and terrain of the open scene, in HDRP terms. Does not save.
        /// </summary>
        public static void FixSceneObjects()
        {
            // Lights: physical units and HDRP data. A light converted by an earlier pass of this
            // tool may sit at the clamp ceiling (20 x scale); bring the sun back to its authored 2.8.
            foreach (var light in UnityEngine.Object.FindObjectsByType<Light>(FindObjectsSortMode.None))
            {
                AetheriaRenderImpl.ConfigureSceneLight(light, true);
                var hd = light.GetComponent<HDAdditionalLightData>();
                if (hd != null && light.type == LightType.Directional)
                {
                    if (!hd.useRayTracedShadows) hd.useRayTracedShadows = true;
                    if (light.intensity >= 19.99f * AetheriaRenderImpl.SunUnitScale)
                        light.intensity = 2.8f * AetheriaRenderImpl.SunUnitScale;
                    hd.SetShadowResolution(2048);
                }
                RemoveComponentByTypeName(light.gameObject, "UnityEngine.Rendering.Universal.UniversalAdditionalLightData");
                EditorUtility.SetDirty(light);
            }

            // Cameras: swap URP camera data for HDRP camera data, DLSS on the main one.
            var mainCam = Camera.main;
            foreach (var cam in UnityEngine.Object.FindObjectsByType<Camera>(FindObjectsSortMode.None))
            {
                var hd = cam.GetComponent<HDAdditionalCameraData>() ?? cam.gameObject.AddComponent<HDAdditionalCameraData>();
                hd.clearColorMode = HDAdditionalCameraData.ClearColorMode.Sky;
                hd.clearDepth = true;
                hd.antialiasing = HDAdditionalCameraData.AntialiasingMode.TemporalAntialiasing;
                hd.TAAQuality = HDAdditionalCameraData.TAAQualityLevel.High;
                hd.dithering = true;
                hd.stopNaNs = true;
                bool isMain = cam == mainCam;
                if (isMain)
                {
                    hd.allowDynamicResolution = true;
                    hd.allowDeepLearningSuperSampling = true;
                    hd.deepLearningSuperSamplingUseCustomQualitySettings = false;
                    hd.deepLearningSuperSamplingUseOptimalSettings = true;
                    hd.allowFidelityFX2SuperResolution = true;
                }
                cam.allowHDR = true;
                cam.allowMSAA = false;

                RemoveComponentByTypeName(cam.gameObject, "UnityEngine.Rendering.Universal.UniversalAdditionalCameraData");
                EditorUtility.SetDirty(cam);
            }

            // Volume: the URP profile cannot drive HDRP, so point the volume at the HDRP profile.
            var hdrpProfile = AssetDatabase.LoadAssetAtPath<VolumeProfile>("Assets/HDRPDefaultResources/AetheriaGlobalVolume.asset");
            foreach (var vol in UnityEngine.Object.FindObjectsByType<Volume>(FindObjectsSortMode.None))
            {
                vol.isGlobal = true;
                vol.priority = 0f;
                if (hdrpProfile != null) vol.sharedProfile = hdrpProfile;
                EditorUtility.SetDirty(vol);
            }

            // Terrain: the scene was authored with URP's terrain shader, which HDRP draws with the error shader.
            var terrainMat = EnsureTerrainMaterial();
            foreach (var terrain in UnityEngine.Object.FindObjectsByType<Terrain>(FindObjectsSortMode.None))
            {
                var current = terrain.materialTemplate;
                if (terrainMat != null && (current == null || current.shader == null || current.shader.name != "HDRP/TerrainLit"))
                    terrain.materialTemplate = terrainMat;
                terrain.renderingLayerMask |= UnityEngine.RenderingLayerMask.defaultRenderingLayerMask;
                EditorUtility.SetDirty(terrain);
            }

            // Sky: the built-in procedural skybox does not exist in HDRP.
            RenderSettings.skybox = null;
            RenderSettings.sun = null;
            RenderSettings.fog = false;      // HDRP fog lives in the volume

            // Report every shader still drawn by a renderer here, so a non-HDRP one cannot hide.
            var shaderUse = new Dictionary<string, int>();
            var offenders = new List<string>();
            foreach (var r in UnityEngine.Object.FindObjectsByType<Renderer>(FindObjectsSortMode.None))
            {
                r.renderingLayerMask |= UnityEngine.RenderingLayerMask.defaultRenderingLayerMask;
                foreach (var m in r.sharedMaterials)
                {
                    string n = m == null || m.shader == null ? "<missing>" : m.shader.name;
                    shaderUse[n] = shaderUse.TryGetValue(n, out int c) ? c + 1 : 1;
                    if (n.StartsWith("Shader Graphs/") || n.StartsWith("Universal") || n == "Standard")
                    {
                        string path = m != null ? AssetDatabase.GetAssetPath(m) : "";
                        offenders.Add($"{n} on '{r.name}' ({(r.transform.parent != null ? r.transform.parent.name : "-")}) mat={path}");
                    }
                }
            }
            foreach (var kv in shaderUse) Debug.Log($"[HDRP] shader '{kv.Key}' x{kv.Value}");
            foreach (var line in offenders) Debug.LogWarning($"[HDRP] NON-HDRP {line}");
        }

        private static Material EnsureTerrainMaterial()
        {
            var mat = AssetDatabase.LoadAssetAtPath<Material>(TerrainMaterialPath);
            if (mat != null && mat.shader != null && mat.shader.name == "HDRP/TerrainLit") return mat;
            var shader = Shader.Find("HDRP/TerrainLit");
            if (shader == null) { Debug.LogWarning("[HDRP] HDRP/TerrainLit not found."); return null; }
            if (mat != null) AssetDatabase.DeleteAsset(TerrainMaterialPath);
            mat = new Material(shader) { name = "AetheriaTerrainLit" };
            AssetDatabase.CreateAsset(mat, TerrainMaterialPath);
            mat.SetOverrideTag("SplatCount", "8");   // 5 layers in use; ray tracing wants a multiple of 4
            EditorUtility.SetDirty(mat);
            AssetDatabase.SaveAssets();
            return mat;
        }
    }
}
