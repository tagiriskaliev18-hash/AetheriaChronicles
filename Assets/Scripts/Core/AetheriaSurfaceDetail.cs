using System.Collections.Generic;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Gives untextured character slots a real material. The Paragon GLBs carry flat colours only, so every
    /// part read as painted cardboard: here each slot class gets a tiling PBR set (albedo, normal, roughness)
    /// from Resources/Surfaces and physically sensible metal/smoothness, while the slot's tint still drives
    /// the colour the customiser chooses.
    /// </summary>
    public static class AetheriaSurfaceDetail
    {
        private struct Set
        {
            public Texture2D Albedo, Normal;
            public float Tiling, Metallic, Smoothness, NormalScale;
        }

        private static readonly Dictionary<string, Texture2D> Cache = new Dictionary<string, Texture2D>();

        private static Texture2D Load(string id, string map)
        {
            string key = id + "/" + map;
            if (Cache.TryGetValue(key, out var t)) return t;
            // Poly Haven names the albedo either _diff or _albedo.
            t = Resources.Load<Texture2D>($"Surfaces/{id}/{id}_{map}_2k");
            if (t == null && map == "diff") t = Resources.Load<Texture2D>($"Surfaces/{id}/{id}_albedo_2k");
            Cache[key] = t;
            return t;
        }

        private static bool TryGet(SlotClass cls, out Set set)
        {
            set = default;
            switch (cls)
            {
                case SlotClass.Metal:
                case SlotClass.Weapon:
                    set = new Set { Albedo = Load("metal_plate", "diff"), Normal = Load("metal_plate", "nor_gl"), Tiling = 3f, Metallic = 0.9f, Smoothness = 0.62f, NormalScale = 0.8f };
                    break;
                case SlotClass.Leather:
                case SlotClass.Hide:
                    set = new Set { Albedo = Load("brown_leather", "diff"), Normal = Load("brown_leather", "nor_gl"), Tiling = 4f, Metallic = 0f, Smoothness = 0.38f, NormalScale = 1.1f };
                    break;
                case SlotClass.Cloth:
                    set = new Set { Albedo = Load("rough_linen", "diff"), Normal = Load("rough_linen", "nor_gl"), Tiling = 6f, Metallic = 0f, Smoothness = 0.12f, NormalScale = 1f };
                    break;
                case SlotClass.ClothAlt:
                    set = new Set { Albedo = Load("poly_wool_herringbone", "diff"), Normal = Load("poly_wool_herringbone", "nor_gl"), Tiling = 6f, Metallic = 0f, Smoothness = 0.15f, NormalScale = 1f };
                    break;
                case SlotClass.Skin:
                    // Skin: no tiling pattern, just a soft, slightly glossy response instead of matte plastic.
                    set = new Set { Tiling = 1f, Metallic = 0f, Smoothness = 0.48f, NormalScale = 0f };
                    return true;
                case SlotClass.Hair:
                    set = new Set { Tiling = 1f, Metallic = 0f, Smoothness = 0.55f, NormalScale = 0f };
                    return true;
                case SlotClass.Eye:
                    set = new Set { Tiling = 1f, Metallic = 0f, Smoothness = 0.35f, NormalScale = 0f };
                    return true;
                case SlotClass.Mouth:
                    set = new Set { Tiling = 1f, Metallic = 0f, Smoothness = 0.2f, NormalScale = 0f };
                    return true;
                default:
                    return false;
            }
            return set.Albedo != null;
        }

        public static void Apply(Material m, SlotClass cls)
        {
            if (m == null || !m.HasProperty("_BaseColor") || !TryGet(cls, out var s)) return;
            // Only slots without their own albedo: real textures (Kwang, Serath) are left as authored.
            bool hasOwnMap = m.HasProperty("_BaseColorMap") && m.GetTexture("_BaseColorMap") != null;
            // The shared Char_* materials borrow another model's texture atlas, which lands on the wrong UVs.
            bool borrowed = m.name.StartsWith("Char_");
            // Keep authored PBR values as well as the authored albedo. Replacing metallic and
            // smoothness on a textured material flattens the artist's intended surface response.
            if (hasOwnMap && !borrowed) return;
            if (borrowed && (cls == SlotClass.Skin || cls == SlotClass.Eye || cls == SlotClass.Mouth || cls == SlotClass.Hair))
            {
                // A borrowed armor atlas and mask are not valid on face, eyes or teeth.
                m.SetTexture("_BaseColorMap", null);
                if (m.HasProperty("_NormalMap")) m.SetTexture("_NormalMap", null);
                if (m.HasProperty("_MaskMap")) m.SetTexture("_MaskMap", null);
                m.DisableKeyword("_NORMALMAP");
                m.DisableKeyword("_MASKMAP");
                if (cls == SlotClass.Eye) AetheriaRender.SetBaseColor(m, new Color(0.7f, 0.68f, 0.62f));
                if (cls == SlotClass.Mouth) AetheriaRender.SetBaseColor(m, new Color(0.55f, 0.46f, 0.39f));
            }
            if ((!hasOwnMap || borrowed) && s.Albedo != null)
            {
                m.SetTexture("_BaseColorMap", s.Albedo);
                m.SetTextureScale("_BaseColorMap", Vector2.one * s.Tiling);
                // The texture carries its own colour; lift it toward neutral so the slot tint still decides the hue.
                Color c = m.GetColor("_BaseColor");
                m.SetColor("_BaseColor", Color.Lerp(c, c * 1.6f, 0.5f));
                if (s.Normal != null && m.HasProperty("_NormalMap") && (borrowed || m.GetTexture("_NormalMap") == null))
                {
                    m.SetTexture("_NormalMap", s.Normal);
                    m.SetTextureScale("_NormalMap", Vector2.one * s.Tiling);
                    m.SetFloat("_NormalScale", s.NormalScale);
                    m.EnableKeyword("_NORMALMAP");
                    m.EnableKeyword("_NORMALMAP_TANGENT_SPACE");
                }
            }
            if (m.HasProperty("_Metallic")) m.SetFloat("_Metallic", s.Metallic);
            if (m.HasProperty("_Smoothness")) m.SetFloat("_Smoothness", s.Smoothness);
            if (m.HasProperty("_SmoothnessRemapMax")) m.SetFloat("_SmoothnessRemapMax", s.Smoothness);
            if (m.HasProperty("_MetallicRemapMax")) m.SetFloat("_MetallicRemapMax", s.Metallic);
        }
    }
}
