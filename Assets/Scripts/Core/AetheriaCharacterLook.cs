using System;
using System.Collections.Generic;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// What a material slot of a character mesh represents. Assigned at build time by analysing which
    /// bones the slot is skinned to, so every character can be recolored by meaning (skin, hair, cloth...)
    /// instead of by slot index.
    /// </summary>
    public enum SlotClass { Skin, Hair, Eye, Mouth, Cloth, ClothAlt, Leather, Metal, Emissive, Weapon, Hide, Wing, Unknown }

    /// <summary>Attached to staged character prefabs: the semantic class of every material slot of every renderer.</summary>
    public class AetheriaCharacterLook : MonoBehaviour
    {
        [Serializable]
        public sealed class RendererSlots
        {
            public Renderer Renderer;
            public SlotClass[] Classes;
        }

        public List<RendererSlots> Renderers = new List<RendererSlots>();

        /// <summary>Humanoid heroes and NPCs share one animation set; creatures carry their own controller.</summary>
        public bool IsHumanoid;
    }

    /// <summary>
    /// The single place that knows the render pipeline's shaders and light units. Gameplay code asks for
    /// "a lit material", "an additive glow" or "a point light" and never touches pipeline types itself.
    /// </summary>
    public static class AetheriaRender
    {
        private static readonly Dictionary<string, Material> Cache = new Dictionary<string, Material>();

        /// <summary>Opaque stylized lit surface.</summary>
        public static Material Lit(Color baseColor, float smoothness = 0.2f, float metallic = 0f, Color? emissive = null)
        {
            Color em = emissive ?? Color.black;
            string key = $"lit|{Key(baseColor)}|{smoothness:F2}|{metallic:F2}|{Key(em)}";
            if (Cache.TryGetValue(key, out var m)) return m;
            m = BuildLit(baseColor, smoothness, metallic, em);
            Cache[key] = m;
            return m;
        }

        /// <summary>Opaque surface that glows (crystals, orbs, molten rock).</summary>
        public static Material Glow(Color color, float intensity = 6f)
        {
            string key = $"glow|{Key(color)}|{intensity:F1}";
            if (Cache.TryGetValue(key, out var m)) return m;
            m = BuildGlow(color, intensity);
            Cache[key] = m;
            return m;
        }

        /// <summary>Unlit transparent surface for beams, rings, blades of light. Additive by default.</summary>
        public static Material TransparentGlow(Color color, bool additive = true, Texture2D map = null)
        {
            string key = $"tglow|{Key(color)}|{additive}|{(map != null ? map.name : "none")}";
            if (Cache.TryGetValue(key, out var m)) return m;
            m = BuildTransparentGlow(color, additive, map);
            Cache[key] = m;
            return m;
        }

        /// <summary>Particle material (soft particles, additive or alpha blended).</summary>
        public static Material Particle(Color color, bool additive = true, Texture2D map = null)
        {
            string key = $"part|{Key(color)}|{additive}|{(map != null ? map.name : "none")}";
            if (Cache.TryGetValue(key, out var m)) return m;
            m = BuildParticle(color, additive, map);
            Cache[key] = m;
            return m;
        }

        public static void SetBaseColor(Material m, Color c)
        {
            if (m == null) return;
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", c);
            if (m.HasProperty("_UnlitColor")) m.SetColor("_UnlitColor", c);
        }

        public static void SetEmission(Material m, Color emissive)
        {
            if (m == null) return;
            if (m.HasProperty("_EmissiveColor")) m.SetColor("_EmissiveColor", emissive);
            if (m.HasProperty("_EmissionColor")) m.SetColor("_EmissionColor", emissive);
        }

        /// <summary>True when the active pipeline can trace rays on this GPU.</summary>
        public static bool RayTracingAvailable =>
            SystemInfo.supportsRayTracing && GraphicsSettingsHasRayTracing();

        /// <summary>Gives a runtime-created effect light the pipeline's extra data (no-op on pipelines that need none).</summary>
        public static void PrepareFxLight(Light l)
        {
            if (l == null) return;
            AetheriaRenderImpl.PrepareFxLight(l);
        }

        /// <summary>Points the sun with an authored strength (0..4) and colour, in pipeline-correct units.</summary>
        public static void StyleSun(Light sun, float authoredIntensity, Color color)
        {
            AetheriaRenderImpl.StyleSun(sun, authoredIntensity, color);
        }

        /// <summary>Retints the sky gradient and the fog under it (linear colours). False until the pipeline has a sky.</summary>
        public static bool SkyReady => AetheriaRenderImpl.SkyReady;

        public static bool SetSkyColors(Color top, Color middle, Color bottom, Color fogAlbedo)
        {
            return AetheriaRenderImpl.SetSkyColors(top, middle, bottom, fogAlbedo);
        }

        /// <summary>Ultra = DLSS; otherwise the pipeline's lighter upscaler.</summary>
        public static void SetUpscalerEnabled(bool ultra) => AetheriaRenderImpl.SetUpscalerEnabled(ultra);

        /// <summary>Points, colors and powers a pooled effect light in pipeline-correct units.</summary>
        public static void StyleFxLight(Light l, Color color, float range, float intensity)
        {
            if (l == null) return;
            AetheriaRenderImpl.StyleFxLight(l, color, range, intensity);
        }

        /// <summary>Creates a point light attached to a parent (torches, orbiting orbs, shrine flames).</summary>
        public static Light CreatePointLight(Transform parent, Vector3 localPos, Color color, float range, float intensity, bool shadows = false)
        {
            return AetheriaRenderImpl.CreatePointLight(parent, localPos, color, range, intensity, shadows);
        }

        // ----------------------------------------------------------------------------------
        // Implementation (swapped when the project moves to another render pipeline)
        // ----------------------------------------------------------------------------------

        private static bool GraphicsSettingsHasRayTracing()
        {
            var rp = UnityEngine.Rendering.GraphicsSettings.currentRenderPipeline;
            return rp != null && rp.GetType().Name.Contains("HighDefinition");
        }

        private static string Key(Color c) => $"{(int)(c.r * 255f):X2}{(int)(c.g * 255f):X2}{(int)(c.b * 255f):X2}";

        private static Material BuildLit(Color baseColor, float smoothness, float metallic, Color emissive)
        {
            var sh = Shader.Find("HDRP/Lit") ?? Shader.Find("Universal Render Pipeline/Lit");
            var m = new Material(sh) { color = baseColor };
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", baseColor);
            if (m.HasProperty("_Smoothness")) m.SetFloat("_Smoothness", smoothness);
            if (m.HasProperty("_Metallic")) m.SetFloat("_Metallic", metallic);
            if (Mathf.Max(emissive.r, Mathf.Max(emissive.g, emissive.b)) > 0.001f)
            {
                if (m.HasProperty("_EmissiveColor")) m.SetColor("_EmissiveColor", emissive);
                if (m.HasProperty("_EmissionColor")) { m.EnableKeyword("_EMISSION"); m.SetColor("_EmissionColor", emissive); }
            }
            return m;
        }

        private static Material BuildGlow(Color color, float intensity)
        {
            var sh = Shader.Find("HDRP/Unlit") ?? Shader.Find("Universal Render Pipeline/Unlit");
            var m = new Material(sh) { color = color * Mathf.Min(intensity, 8f), globalIlluminationFlags = MaterialGlobalIlluminationFlags.None };
            if (m.HasProperty("_UnlitColor")) m.SetColor("_UnlitColor", color * Mathf.Min(intensity, 8f));
            if (m.HasProperty("_EmissiveColor")) m.SetColor("_EmissiveColor", color * intensity);
            return m;
        }

        private static Material BuildTransparentGlow(Color color, bool additive, Texture2D map)
        {
            var sh = Shader.Find("HDRP/Unlit") ?? Shader.Find("Universal Render Pipeline/Unlit");
            var m = new Material(sh) { color = color, renderQueue = (int)UnityEngine.Rendering.RenderQueue.Transparent };
            if (m.HasProperty("_UnlitColor")) m.SetColor("_UnlitColor", color);
            if (m.HasProperty("_SurfaceType")) m.SetFloat("_SurfaceType", 1f);
            if (m.HasProperty("_BlendMode")) m.SetFloat("_BlendMode", additive ? 2f : 0f);
            if (m.HasProperty("_ZWrite")) m.SetFloat("_ZWrite", 0f);
            if (m.HasProperty("_SrcBlend")) m.SetFloat("_SrcBlend", additive ? (float)UnityEngine.Rendering.BlendMode.One : (float)UnityEngine.Rendering.BlendMode.SrcAlpha);
            if (m.HasProperty("_DstBlend")) m.SetFloat("_DstBlend", additive ? (float)UnityEngine.Rendering.BlendMode.One : (float)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            if (m.HasProperty("_BaseMap") && map != null) m.SetTexture("_BaseMap", map);
            if (m.HasProperty("_UnlitColorMap") && map != null) m.SetTexture("_UnlitColorMap", map);
            if (map != null) m.mainTexture = map;
            return m;
        }

        private static Material BuildParticle(Color color, bool additive, Texture2D map)
        {
            var sh = Shader.Find("HDRP/Particles/Unlit") ?? Shader.Find("Universal Render Pipeline/Particles/Unlit") ?? Shader.Find("Sprites/Default");
            var m = new Material(sh) { color = color, renderQueue = (int)UnityEngine.Rendering.RenderQueue.Transparent };
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", color);
            if (m.HasProperty("_SurfaceType")) m.SetFloat("_SurfaceType", 1f);
            if (m.HasProperty("_BlendMode")) m.SetFloat("_BlendMode", additive ? 2f : 0f);
            if (m.HasProperty("_BlendModePreserveSpecularLighting")) m.SetFloat("_BlendModePreserveSpecularLighting", 0f);
            if (m.HasProperty("_ZWrite")) m.SetFloat("_ZWrite", 0f);
            if (m.HasProperty("_SrcBlend")) m.SetFloat("_SrcBlend", additive ? (float)UnityEngine.Rendering.BlendMode.One : (float)UnityEngine.Rendering.BlendMode.SrcAlpha);
            if (m.HasProperty("_DstBlend")) m.SetFloat("_DstBlend", additive ? (float)UnityEngine.Rendering.BlendMode.One : (float)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            if (m.HasProperty("_BaseMap") && map != null) m.SetTexture("_BaseMap", map);
            if (map != null) m.mainTexture = map;
            return m;
        }
    }
}
