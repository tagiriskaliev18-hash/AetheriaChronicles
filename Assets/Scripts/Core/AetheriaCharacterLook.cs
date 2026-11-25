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
            if (m.HasProperty("_UseEmissiveIntensity")) m.SetFloat("_UseEmissiveIntensity", 0f);
            bool hasEmission = Mathf.Max(emissive.r, Mathf.Max(emissive.g, emissive.b)) > 0.01f;
            if (hasEmission) m.EnableKeyword("_EMISSION");
            else m.DisableKeyword("_EMISSION");
            if (m.HasProperty("_EmissiveColor")) m.SetColor("_EmissiveColor", emissive);
            if (m.HasProperty("_EmissiveColorLDR")) m.SetColor("_EmissiveColorLDR", emissive);
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

        private static Material _particleTemplate;

        private static bool GraphicsSettingsHasRayTracing()
        {
            var rp = UnityEngine.Rendering.GraphicsSettings.defaultRenderPipeline
                     ?? UnityEngine.Rendering.GraphicsSettings.currentRenderPipeline;
            return (rp as UnityEngine.Rendering.HighDefinition.HDRenderPipelineAsset)?
                .currentPlatformRenderPipelineSettings.supportRayTracing == true;
        }

        private static string Key(Color c) =>
            $"{(int)(Mathf.Clamp01(c.r) * 255f):X2}{(int)(Mathf.Clamp01(c.g) * 255f):X2}{(int)(Mathf.Clamp01(c.b) * 255f):X2}{(int)(Mathf.Clamp01(c.a) * 255f):X2}";

        private static Material BuildLit(Color baseColor, float smoothness, float metallic, Color emissive)
        {
            var sh = Shader.Find("HDRP/Lit") ?? Shader.Find("Standard");
            var m = new Material(sh) { color = baseColor };
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", baseColor);
            if (m.HasProperty("_Smoothness")) m.SetFloat("_Smoothness", smoothness);
            if (m.HasProperty("_Metallic")) m.SetFloat("_Metallic", metallic);
            if (m.HasProperty("_UseEmissiveIntensity")) m.SetFloat("_UseEmissiveIntensity", 0f);
            if (Mathf.Max(emissive.r, Mathf.Max(emissive.g, emissive.b)) > 0.01f)
            {
                m.EnableKeyword("_EMISSION");
                if (m.HasProperty("_EmissiveColor")) m.SetColor("_EmissiveColor", emissive);
                if (m.HasProperty("_EmissiveColorLDR")) m.SetColor("_EmissiveColorLDR", emissive);
                if (m.HasProperty("_EmissionColor")) m.SetColor("_EmissionColor", emissive);
            }
            return m;
        }

        private static Material BuildGlow(Color color, float intensity)
        {
            var sh = Shader.Find("HDRP/Unlit") ?? Shader.Find("Unlit/Color");
            Color hdr = color * intensity;
            Color ldr = color * Mathf.Min(intensity, 8f);
            var m = new Material(sh) { color = ldr, globalIlluminationFlags = MaterialGlobalIlluminationFlags.None };
            if (m.HasProperty("_UnlitColor")) m.SetColor("_UnlitColor", ldr);
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", ldr);
            if (m.HasProperty("_UseEmissiveIntensity")) m.SetFloat("_UseEmissiveIntensity", 0f);
            if (m.HasProperty("_EmissiveColor")) m.SetColor("_EmissiveColor", hdr);
            if (m.HasProperty("_EmissiveColorLDR")) m.SetColor("_EmissiveColorLDR", ldr);
            if (intensity > 0.01f) m.EnableKeyword("_EMISSION");
            return m;
        }

        private static Material BuildTransparentGlow(Color color, bool additive, Texture2D map)
        {
            var sh = Shader.Find("HDRP/Unlit") ?? Shader.Find("Sprites/Default");
            var m = new Material(sh) { color = color, renderQueue = (int)UnityEngine.Rendering.RenderQueue.Transparent };
            m.SetOverrideTag("RenderType", "Transparent");
            if (m.HasProperty("_UnlitColor")) m.SetColor("_UnlitColor", color);
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", color);
            if (m.HasProperty("_SurfaceType")) m.SetFloat("_SurfaceType", 1f);
            if (m.HasProperty("_BlendMode")) m.SetFloat("_BlendMode", additive ? 1f : 0f);
            if (m.HasProperty("_ZWrite")) m.SetFloat("_ZWrite", 0f);
            if (m.HasProperty("_TransparentZWrite")) m.SetFloat("_TransparentZWrite", 0f);
            if (m.HasProperty("_SrcBlend")) m.SetFloat("_SrcBlend", (float)UnityEngine.Rendering.BlendMode.One);
            if (m.HasProperty("_DstBlend")) m.SetFloat("_DstBlend", additive ? (float)UnityEngine.Rendering.BlendMode.One : (float)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            m.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
            if (additive) m.EnableKeyword("_BLENDMODE_ADD");
            else m.DisableKeyword("_BLENDMODE_ADD");
            if (map != null)
            {
                if (m.HasProperty("_UnlitColorMap")) m.SetTexture("_UnlitColorMap", map);
                if (m.HasProperty("_BaseColorMap")) m.SetTexture("_BaseColorMap", map);
                if (m.HasProperty("_BaseMap")) m.SetTexture("_BaseMap", map);
                m.mainTexture = map;
            }
            return m;
        }

        private static Material BuildParticle(Color color, bool additive, Texture2D map)
        {
            if (_particleTemplate == null)
                _particleTemplate = Resources.Load<Material>("Materials/URP_Particles_Unlit_Template");
            Material m = _particleTemplate != null
                ? new Material(_particleTemplate)
                : new Material(Shader.Find("HDRP/Unlit") ?? Shader.Find("Sprites/Default"));
            m.color = color;
            m.renderQueue = (int)UnityEngine.Rendering.RenderQueue.Transparent;
            m.SetOverrideTag("RenderType", "Transparent");
            if (m.HasProperty("_UnlitColor")) m.SetColor("_UnlitColor", color);
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", color);
            if (m.HasProperty("_SurfaceType")) m.SetFloat("_SurfaceType", 1f);
            if (m.HasProperty("_BlendMode")) m.SetFloat("_BlendMode", additive ? 1f : 0f);
            if (m.HasProperty("_ZWrite")) m.SetFloat("_ZWrite", 0f);
            if (m.HasProperty("_TransparentZWrite")) m.SetFloat("_TransparentZWrite", 0f);
            if (m.HasProperty("_SrcBlend")) m.SetFloat("_SrcBlend", (float)UnityEngine.Rendering.BlendMode.One);
            if (m.HasProperty("_DstBlend")) m.SetFloat("_DstBlend", additive ? (float)UnityEngine.Rendering.BlendMode.One : (float)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            m.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
            if (additive) m.EnableKeyword("_BLENDMODE_ADD");
            else m.DisableKeyword("_BLENDMODE_ADD");
            if (map != null)
            {
                if (m.HasProperty("_UnlitColorMap")) m.SetTexture("_UnlitColorMap", map);
                if (m.HasProperty("_BaseColorMap")) m.SetTexture("_BaseColorMap", map);
                if (m.HasProperty("_BaseMap")) m.SetTexture("_BaseMap", map);
                m.mainTexture = map;
            }
            return m;
        }
    }
}
