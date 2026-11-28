using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace Aetheria.Editor
{
    /// <summary>
    /// Dresses the untextured Paragon character meshes by meaning instead of by slot index.
    ///
    /// The GLBs ship without images, so <c>AetheriaProjectSetup.StageHeroPrefab</c> cycled three Char_* materials over
    /// every sub-mesh (faces in cloak cloth, brass eyes). Here every sub-mesh of every analysed GLB carries a role
    /// (skin, hair, eye, cloth, leather, metal, gem...) found offline from its bounds, triangle count, flatness and the
    /// bones it is skinned to. Each prefab maps those roles to a small library of HDRP/Lit materials
    /// (Assets/Generated/Materials/CharPBR_*.mat) built from tiling textures already in the project, so skins of the
    /// same body (Gideon / Inquisitor / Mephisto) keep distinct palettes.
    ///
    /// The prefab also receives an <see cref="AetheriaCharacterLook"/> with the slot classes, which is what
    /// HeroController.ApplyLookTints needs to recolour the player's hero (skin by race, hair colour, armour tint).
    /// Material names deliberately do NOT start with "Char_": AetheriaSurfaceDetail.Apply treats "Char_*" as a
    /// borrowed atlas and would overwrite the albedo; CharPBR_* materials with their own map are left as authored.
    ///
    /// Idempotent. Only prefabs are changed (scene overrides such as the Npc_CaveTutor Sage_* materials still win).
    /// Re-running "Aetheria/1. Setup Project" re-stages the prefabs with the old cycling: run this menu again afterwards.
    /// </summary>
    public static class AetheriaCharacterMaterials
    {
        private const string MatFolder = "Assets/Generated/Materials";
        private const string Wiz = "Assets/_AAA_Imported/WizardRaw/GLBModels/hero_archmage_";
        private const string Surf = "Assets/Resources/Surfaces/";
        private const string Heroes = "Assets/_AAA_Imported/Heroes/";

        // ------------------------------------------------------------------ material library

        private sealed class Spec
        {
            public string Albedo, Normal, EmissiveMap;
            public Color Tint = Color.white;
            public float Tiling = 1f, Metallic, Smoothness = 0.3f, NormalScale = 1f;
            public Color Emissive = Color.black;   // linear HDR, already multiplied by intensity
        }

        private static Spec Plain(Color c, float smooth, float metal = 0f) =>
            new Spec { Tint = c, Smoothness = smooth, Metallic = metal };

        private static Spec Tex(string albedo, string normal, Color c, float tiling, float smooth, float metal, float nScale = 1f) =>
            new Spec { Albedo = albedo, Normal = normal, Tint = c, Tiling = tiling, Smoothness = smooth, Metallic = metal, NormalScale = nScale };

        private static Spec Gem(Color baseCol, Color glow, float intensity) =>
            new Spec
            {
                Tint = baseCol, Smoothness = 0.85f, Metallic = 0f,
                EmissiveMap = Wiz + "d4_rune_filigree_emissive.png", Tiling = 2f,
                Emissive = glow * intensity
            };

        private static Dictionary<string, Spec> BuildSpecs()
        {
            string wool = Wiz + "d4_woven_wool_albedo.png", woolN = Wiz + "d4_woven_wool_normal.png";
            string herr = Surf + "poly_wool_herringbone/poly_wool_herringbone_diff_2k.jpg", herrN = Surf + "poly_wool_herringbone/poly_wool_herringbone_nor_gl_2k.jpg";
            string linen = Surf + "rough_linen/rough_linen_diff_2k.jpg", linenN = Surf + "rough_linen/rough_linen_nor_gl_2k.jpg";
            string leath = Surf + "brown_leather/brown_leather_albedo_2k.jpg", leathN = Surf + "brown_leather/brown_leather_nor_gl_2k.jpg";
            string fiber = Wiz + "char_dark_cloth_albedo.png", fiberN = Wiz + "char_dark_cloth_normal.png";
            string steel = Wiz + "d4_hammered_steel_albedo.png", steelN = Wiz + "d4_hammered_steel_normal.png";
            string obs = Wiz + "char_obsidian_steel_albedo.png", obsN = Wiz + "char_obsidian_steel_normal.png";
            string brass = Wiz + "d4_gilded_brass_albedo.png", brassN = Wiz + "d4_gilded_brass_normal.png";
            string iron = Wiz + "forged_iron_albedo.png", ironN = Wiz + "forged_iron_normal.png";
            string rock = "Assets/_AAA_Imported/Megascans/Surfaces/LayeredRockCliff/T_Layered_Rock_Cliff_ubulehofw_D.png";

            return new Dictionary<string, Spec>
            {
                // Skin: no tiling pattern (it would read as fabric); soft, slightly glossy.
                ["Skin_Warm"] = Plain(new Color(0.80f, 0.58f, 0.47f), 0.40f),
                ["Skin_Tan"] = Plain(new Color(0.62f, 0.42f, 0.30f), 0.40f),
                ["Skin_Pale"] = Plain(new Color(0.88f, 0.76f, 0.70f), 0.42f),
                ["Skin_Grey"] = Plain(new Color(0.58f, 0.60f, 0.64f), 0.40f),     // Morigesh, ashen witch
                ["Skin_Ash"] = Plain(new Color(0.24f, 0.20f, 0.22f), 0.38f),      // demonic / Sevarog
                // Hair: fine fibre texture tiled high so it reads as strands, not plastic.
                ["Hair_Grey"] = Tex(fiber, fiberN, new Color(0.62f, 0.60f, 0.58f), 8f, 0.45f, 0f, 0.6f),
                ["Hair_Brown"] = Tex(fiber, fiberN, new Color(0.30f, 0.20f, 0.13f), 8f, 0.45f, 0f, 0.6f),
                ["Hair_Black"] = Tex(fiber, fiberN, new Color(0.08f, 0.07f, 0.07f), 8f, 0.50f, 0f, 0.6f),
                ["Hair_White"] = Tex(fiber, fiberN, new Color(0.86f, 0.86f, 0.84f), 8f, 0.42f, 0f, 0.6f),
                ["Hair_Auburn"] = Tex(fiber, fiberN, new Color(0.50f, 0.22f, 0.10f), 8f, 0.45f, 0f, 0.6f),
                ["Eye"] = Plain(new Color(0.035f, 0.03f, 0.028f), 0.92f),
                ["Teeth"] = Plain(new Color(0.72f, 0.64f, 0.56f), 0.55f),
                ["Bone"] = Tex(linen, linenN, new Color(0.86f, 0.80f, 0.66f), 3f, 0.35f, 0f, 0.5f),
                // Cloth: woven wool tiled 5x, tinted.
                ["Cloth_Crimson"] = Tex(wool, woolN, new Color(0.62f, 0.10f, 0.11f), 5f, 0.20f, 0f, 1.1f),
                ["Cloth_Ivory"] = Tex(wool, woolN, new Color(0.88f, 0.84f, 0.74f), 5f, 0.20f, 0f, 1.1f),
                ["Cloth_Midnight"] = Tex(wool, woolN, new Color(0.10f, 0.12f, 0.28f), 5f, 0.20f, 0f, 1.1f),
                ["Cloth_Charcoal"] = Tex(wool, woolN, new Color(0.18f, 0.17f, 0.18f), 5f, 0.18f, 0f, 1.1f),
                ["Cloth_Violet"] = Tex(wool, woolN, new Color(0.30f, 0.12f, 0.36f), 5f, 0.20f, 0f, 1.1f),
                ["Cloth_Frost"] = Tex(wool, woolN, new Color(0.62f, 0.74f, 0.86f), 5f, 0.22f, 0f, 1.1f),
                ["Cloth_Sand"] = Tex(wool, woolN, new Color(0.74f, 0.60f, 0.40f), 5f, 0.20f, 0f, 1.1f),
                ["Cloth_Forest"] = Tex(wool, woolN, new Color(0.16f, 0.28f, 0.16f), 5f, 0.20f, 0f, 1.1f),
                // Secondary cloth: herringbone wool, tiled 6x.
                ["Wool_Grey"] = Tex(herr, herrN, new Color(0.62f, 0.60f, 0.58f), 6f, 0.15f, 0f, 1f),
                ["Wool_Brown"] = Tex(herr, herrN, new Color(0.52f, 0.38f, 0.26f), 6f, 0.15f, 0f, 1f),
                // Leather: Poly Haven brown leather, tiled 4x.
                ["Leather_Brown"] = Tex(leath, leathN, new Color(1.00f, 0.92f, 0.86f), 4f, 0.40f, 0f, 1.1f),
                ["Leather_Dark"] = Tex(leath, leathN, new Color(0.42f, 0.36f, 0.34f), 4f, 0.42f, 0f, 1.1f),
                // Fur / beast hide: herringbone gives a coarse, matte, fibrous break-up.
                ["Hide_Beast"] = Tex(herr, herrN, new Color(0.48f, 0.34f, 0.22f), 9f, 0.22f, 0f, 1.4f),
                ["Hide_Grey"] = Tex(herr, herrN, new Color(0.58f, 0.58f, 0.60f), 9f, 0.22f, 0f, 1.4f),
                ["Stone_Rock"] = Tex(rock, null, new Color(0.70f, 0.68f, 0.66f), 2f, 0.20f, 0f, 1f),
                // Metals.
                ["Metal_Steel"] = Tex(steel, steelN, new Color(0.92f, 0.94f, 0.98f), 3f, 0.66f, 0.90f, 1f),
                ["Metal_DarkSteel"] = Tex(obs, obsN, new Color(0.42f, 0.44f, 0.50f), 3f, 0.60f, 0.90f, 1f),
                ["Metal_Brass"] = Tex(brass, brassN, new Color(1.00f, 0.90f, 0.70f), 3f, 0.70f, 0.90f, 1f),
                ["Metal_Iron"] = Tex(iron, ironN, new Color(0.62f, 0.58f, 0.56f), 2f, 0.42f, 0.85f, 1.1f),
                ["Metal_Bloodiron"] = Tex(obs, obsN, new Color(0.55f, 0.16f, 0.16f), 3f, 0.58f, 0.90f, 1f),
                // Emissive gems / portals.
                ["Gem_Arcane"] = Gem(new Color(0.20f, 0.08f, 0.32f), new Color(0.55f, 0.25f, 1.00f), 6f),
                ["Gem_Holy"] = Gem(new Color(0.40f, 0.32f, 0.12f), new Color(1.00f, 0.78f, 0.35f), 6f),
                ["Gem_Ember"] = Gem(new Color(0.30f, 0.06f, 0.02f), new Color(1.00f, 0.30f, 0.06f), 6f),
                ["Gem_Frost"] = Gem(new Color(0.10f, 0.24f, 0.32f), new Color(0.30f, 0.80f, 1.00f), 5f),
            };
        }

        private static Material BuildMaterial(string key, Spec s, Shader lit)
        {
            var m = new Material(lit) { name = "CharPBR_" + key };
            var tile = new Vector2(s.Tiling, s.Tiling);
            m.SetColor("_BaseColor", s.Tint);
            if (m.HasProperty("_Color")) m.SetColor("_Color", s.Tint);
            var alb = s.Albedo != null ? AssetDatabase.LoadAssetAtPath<Texture2D>(s.Albedo) : null;
            if (s.Albedo != null && alb == null) Debug.LogWarning($"[CharPBR] {key}: albedo not found at {s.Albedo}; plain colour used.");
            m.SetTexture("_BaseColorMap", alb);
            // HDRP/Lit drives every UV0 map from _BaseColorMap_ST; the others are set for clarity only.
            m.SetTextureScale("_BaseColorMap", tile);

            var nrm = s.Normal != null && EnsureNormalImporter(s.Normal) ? AssetDatabase.LoadAssetAtPath<Texture2D>(s.Normal) : null;
            if (nrm != null)
            {
                m.SetTexture("_NormalMap", nrm);
                m.SetTextureScale("_NormalMap", tile);
                m.SetFloat("_NormalScale", s.NormalScale);
                m.SetFloat("_NormalMapSpace", 0f);
                m.EnableKeyword("_NORMALMAP");
                m.EnableKeyword("_NORMALMAP_TANGENT_SPACE");
            }
            else
            {
                m.SetTexture("_NormalMap", null);
                m.DisableKeyword("_NORMALMAP");
            }

            m.SetTexture("_MaskMap", null);    // no packed mask: the sliders below are used as-is
            m.SetFloat("_Metallic", s.Metallic);
            m.SetFloat("_Smoothness", s.Smoothness);
            m.SetFloat("_MetallicRemapMax", s.Metallic);
            m.SetFloat("_SmoothnessRemapMax", s.Smoothness);
            m.SetFloat("_MaterialID", 1f);     // standard Lit
            m.SetFloat("_SurfaceType", 0f);
            m.SetFloat("_DoubleSidedEnable", 0f);

            bool glows = s.Emissive.maxColorComponent > 0.001f;
            m.SetFloat("_UseEmissiveIntensity", 0f);
            m.SetFloat("_EmissiveColorMode", 1f);
            m.SetColor("_EmissiveColor", glows ? s.Emissive : Color.black);
            m.SetColor("_EmissiveColorLDR", glows ? s.Emissive / Mathf.Max(1f, s.Emissive.maxColorComponent) : Color.black);
            var emMap = glows && s.EmissiveMap != null ? AssetDatabase.LoadAssetAtPath<Texture2D>(s.EmissiveMap) : null;
            m.SetTexture("_EmissiveColorMap", emMap);
            if (emMap != null) m.SetTextureScale("_EmissiveColorMap", tile);
            m.globalIlluminationFlags = glows ? MaterialGlobalIlluminationFlags.RealtimeEmissive : MaterialGlobalIlluminationFlags.EmissiveIsBlack;

            AetheriaHdrpConvert.ValidateHdMaterial(m);
            return m;
        }

        /// <summary>The Poly Haven nor_gl JPGs were imported as colour textures; a normal map must be linear and unpacked.</summary>
        private static bool EnsureNormalImporter(string path)
        {
            if (!(AssetImporter.GetAtPath(path) is TextureImporter ti)) return false;
            if (ti.textureType != TextureImporterType.NormalMap)
            {
                ti.textureType = TextureImporterType.NormalMap;
                ti.wrapMode = TextureWrapMode.Repeat;
                ti.SaveAndReimport();
                Debug.Log($"[CharPBR] {path}: importer set to Normal Map.");
            }
            return true;
        }

        /// <summary>Creates or updates every CharPBR_* material in place (GUIDs survive, so prefabs keep their links).</summary>
        private static Dictionary<string, Material> BuildLibrary()
        {
            var lit = Shader.Find("HDRP/Lit");
            if (lit == null) throw new InvalidOperationException("HDRP/Lit shader not found.");
            if (!AssetDatabase.IsValidFolder(MatFolder)) Directory.CreateDirectory(MatFolder);

            var lib = new Dictionary<string, Material>();
            foreach (var kv in BuildSpecs())
            {
                string path = $"{MatFolder}/CharPBR_{kv.Key}.mat";
                var fresh = BuildMaterial(kv.Key, kv.Value, lit);
                var existing = AssetDatabase.LoadAssetAtPath<Material>(path);
                if (existing != null)
                {
                    EditorUtility.CopySerialized(fresh, existing);
                    existing.name = fresh.name;
                    AetheriaHdrpConvert.ValidateHdMaterial(existing);
                    EditorUtility.SetDirty(existing);
                    UnityEngine.Object.DestroyImmediate(fresh);
                    lib[kv.Key] = existing;
                }
                else
                {
                    AssetDatabase.CreateAsset(fresh, path);
                    lib[kv.Key] = fresh;
                }
            }
            AssetDatabase.SaveAssets();
            return lib;
        }

        // ------------------------------------------------------------------ per-mesh roles (offline GLB analysis)
        //
        // One character per sub-mesh, in glTF primitive order (= glTFast sub-mesh order; every primitive of these
        // GLBs has the same attribute set, so glTFast keeps them in one mesh in this order):
        //   S skin   H hair/lashes/beard/mane   E eyeball or eyelid/tear/occlusion shell   T teeth+tongue
        //   C main cloth   A secondary cloth   L leather/wraps/belts   M main metal/armour   B trim metal/jewellery
        //   W weapon   G emissive (portal, holy wings, ghost beast, soul gem)   F hide/fur body
        private static readonly Dictionary<string, string> MeshRoles = new Dictionary<string, string>
        {
            // 0 head | 1 coat+hood+arms | 2 legs/lower robe | 3 loincloth+belt | 4 back ring (outer_ring bones) |
            // 5 portal disc | 6 shoulder pieces | 7 beard | 8 teeth | 9 lashes | 10-12 lid/tear shells | 13 eyeballs
            [Heroes + "Gideon/Meshes/Gideon.glb"] = "SCALBGMHTHEEEE",
            [Heroes + "Gideon/Meshes/Gideon_Inquisitor.glb"] = "SCALBGMHTHEEEE",
            [Heroes + "Gideon/Meshes/Gideon_Mephisto.glb"] = "SCALBGMHTHEEEE",
            [Heroes + "Gideon/Meshes/Gideon_Tough.glb"] = "MHCAEEEHESBLGT",
            // 0 eyes (head-only, face front) | 1 torso suit | 2 tentacle scarf | 3 back thrusters | 4 arms/gloves |
            // 5 swords+daggers | 6 legs | 7 head/mask
            [Heroes + "Kallari/Meshes/Kallari.glb"] = "ECAMLWLS",
            [Heroes + "Kallari/Meshes/Kallari_DeathLotus.glb"] = "ECAMLWLS",
            // 0 beast head+torso | 1 shin/forearm wraps | 2 claw blades | 3 mane | 4 teeth | 5 ghost beast | 6 armour | 7 horns/face
            [Heroes + "Khaimera/Meshes/Khaimera.glb"] = "FLWHTGMB",
            // 0 head+torso | 1 wraps | 2 claws | 3 weapon tails | 4 teeth | 5 armour | 6 horns | 7 necklace | 8 loincloth | 9 ghost beast
            [Heroes + "Khaimera/Meshes/Khaimera_GruxPelt.glb"] = "FLWLTMBBCG",
            // 0 trousers+hip plates | 1 collar armour | 2 hood/scarf | 3 pouch | 4 tunic+sleeves | 5 bare arms+hands |
            // 6 eyeballs | 7-8 lid shells | 9 shoulder mantle | 10 topknot | 11 head | 12 ponytail | 13 teeth |
            // 14 shoulder plate | 15 lid shell | 16 hip flap | 17 sword
            [Heroes + "Kwang/Meshes/Kwang_GDC.glb"] = "AMCLCSEEEAHSHTBELW",
            [Heroes + "Kwang/Meshes/KwangAlbino.glb"] = "AMCLCSEEEAHSHTBELW",
            [Heroes + "Kwang/Meshes/KwangSunrise.glb"] = "AMCLCSEEEAHSHTBELW",
            [Heroes + "Kwang/Meshes/KwangRosewood.glb"] = "AMCLCSEEEAHSHTBELW",
            // 0 head | 1 torso | 2 right sleeve | 3 shoulder plate+pad | 4 shoulder bows | 5 bracers | 6 hands |
            // 7 hair/hood | 8 teeth | 9 eyeballs | 10-12 lid shells | 13 belt | 14 legs | 15 skirt flaps | 16 sword |
            // 17 hip flap | 18 strap | 19 pouch
            [Heroes + "Kwang/Meshes/KwangFrostwalker.glb"] = "SCLMBLSHTEEEELACWLLL",
            [Heroes + "Kwang/Meshes/KwangManbun.glb"] = "SCLMBLSHTEEEELACWLLL",
            // 0 body skin | 1 arm wraps | 2 chains | 3 leg wraps+dagger | 4 top | 5 collar | 6 dress | 7-8 eyes |
            // 9 hair | 10 hair dangles | 11 doll binding | 12 doll holder | 13 ankle feathers | 14 teeth |
            // 15 doll head | 16 doll arms | 17-18 lid shells
            [Heroes + "Morigesh/Meshes/Morigesh.glb"] = "SLBLABCEEHHLBATLCEE",
            // 0 staff+doll | 1 sleeves | 2 hands | 3 loincloth | 4 shawl | 5 lid shell | 6 eyeballs | 7 lid shell |
            // 8 hair | 9 head | 10 teeth | 11 legs | 12 fur hood/collar | 13-14 lid shells
            [Heroes + "Morigesh/Meshes/Morigesh_NorthernMystic.glb"] = "LASACEEEHSTLFEE",
            // 0 robe/sleeves | 1 scythe+armour | 2 legs | 3 head | 4 feet | 5 eyes | 6 hair | 7 holy shoulder trim |
            // 8 teeth | 9-11 lid shells | 12-13 light wings | 14 loincloth
            [Heroes + "Serath/Meshes/Serath.glb"] = "CMASLEHBTEEEGGC",
            [Heroes + "Serath/Meshes/SerathFrost.glb"] = "CMASLEHBTEEEGGC",
            [Heroes + "Serath/Meshes/SerathGold.glb"] = "CMASLEHBTEEEGGC",
            // 0 torso | 1 shoulder armour | 2 forearm armour | 3 pelvis cloth | 4 weapon | 5 chest soul gem
            ["Assets/_AAA_Imported/Boss_Sevarog/Meshes/Sevarog.glb"] = "LMBCWG",
            ["Assets/_AAA_Imported/Boss_Sevarog/Meshes/SevarogBloodred.glb"] = "LMBCWG",
            // 0 head+body | 1 back rocks | 2 arms | 3 chains/belt | 4 loincloth | 5 teeth | 6-8 lid shells | 9 eyes
            ["Assets/_AAA_Imported/Enemies/Rampage/Meshes/Rampage.glb"] = "FMFLCTEEEE",
            // Not listed: lane minions, Buff_*, SK_Whisper (one sub-mesh each, nothing to split; Whisper is textured)
            // and the dragons (textured with RedHPTex).
        };

        private static SlotClass ClassOf(char role)
        {
            switch (role)
            {
                case 'S': return SlotClass.Skin;
                case 'H': return SlotClass.Hair;
                case 'E': return SlotClass.Eye;
                case 'T': return SlotClass.Mouth;
                case 'C': return SlotClass.Cloth;
                case 'A': return SlotClass.ClothAlt;
                case 'L': return SlotClass.Leather;
                case 'M':
                case 'B': return SlotClass.Metal;
                case 'W': return SlotClass.Weapon;
                case 'G': return SlotClass.Emissive;
                case 'F': return SlotClass.Hide;
                default: return SlotClass.Unknown;
            }
        }

        // ------------------------------------------------------------------ per-prefab palettes

        private const string DefaultPalette =
            "S=Skin_Warm H=Hair_Brown E=Eye T=Teeth C=Cloth_Charcoal A=Wool_Grey L=Leather_Brown " +
            "M=Metal_Steel B=Metal_Brass W=Metal_Steel G=Gem_Arcane F=Hide_Beast";

        /// <summary>Prefab name -> role overrides on top of DefaultPalette (keeps skins of one body distinct).</summary>
        private static readonly Dictionary<string, string> Palettes = new Dictionary<string, string>
        {
            ["Hero_Gideon"] = "C=Cloth_Midnight A=Cloth_Charcoal H=Hair_Grey G=Gem_Arcane",
            ["Hero_GideonInq"] = "C=Cloth_Ivory A=Cloth_Crimson H=Hair_Grey M=Metal_Steel G=Gem_Holy",
            ["Hero_GideonMeph"] = "S=Skin_Ash C=Cloth_Crimson A=Cloth_Charcoal H=Hair_Black M=Metal_DarkSteel B=Metal_Iron G=Gem_Ember",
            ["Hero_Kallari"] = "S=Skin_Pale C=Cloth_Midnight A=Cloth_Violet L=Leather_Dark M=Metal_DarkSteel",
            ["Hero_KallariLotus"] = "S=Skin_Pale C=Cloth_Crimson A=Cloth_Charcoal L=Leather_Dark M=Metal_Brass W=Metal_DarkSteel",
            ["Hero_Khaimera"] = "H=Hair_Brown M=Metal_Iron B=Bone G=Gem_Frost",
            ["Hero_KhaimeraPelt"] = "F=Hide_Grey H=Hair_White M=Metal_Iron B=Bone C=Cloth_Sand G=Gem_Ember",
            ["Hero_Kwang"] = "S=Skin_Tan H=Hair_Black C=Cloth_Crimson A=Wool_Grey",
            ["Hero_KwangAlbino"] = "S=Skin_Pale H=Hair_White C=Cloth_Ivory A=Cloth_Charcoal",
            ["Hero_KwangSunrise"] = "S=Skin_Tan H=Hair_Auburn C=Cloth_Sand A=Cloth_Crimson M=Metal_Brass",
            ["Hero_KwangFrost"] = "S=Skin_Pale H=Hair_White C=Cloth_Frost A=Wool_Grey L=Leather_Dark B=Metal_Steel G=Gem_Frost",
            ["Hero_KwangManbun"] = "S=Skin_Tan H=Hair_Black C=Cloth_Charcoal A=Cloth_Crimson M=Metal_DarkSteel",
            ["Hero_Morigesh"] = "S=Skin_Grey H=Hair_Black C=Cloth_Violet A=Cloth_Charcoal L=Leather_Dark",
            ["Hero_MorigeshMystic"] = "S=Skin_Pale H=Hair_White C=Cloth_Frost A=Wool_Grey F=Hide_Grey",
            ["Hero_Serath"] = "H=Hair_Auburn C=Cloth_Ivory A=Cloth_Midnight G=Gem_Holy",
            ["Hero_SerathGold"] = "H=Hair_Auburn C=Cloth_Ivory A=Cloth_Crimson M=Metal_Brass G=Gem_Holy",
            ["Hero_SerathFrost"] = "S=Skin_Pale H=Hair_White C=Cloth_Frost A=Cloth_Midnight B=Metal_Steel G=Gem_Frost",
            ["MalakorBoss"] = "L=Leather_Dark M=Metal_Bloodiron B=Metal_DarkSteel C=Cloth_Crimson W=Metal_DarkSteel G=Gem_Ember",
            ["Rampage"] = "F=Hide_Beast M=Stone_Rock C=Cloth_Sand",
        };

        private static Dictionary<char, string> PaletteFor(string prefabName)
        {
            var map = new Dictionary<char, string>();
            void Parse(string s)
            {
                foreach (var tok in s.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries))
                    if (tok.Length > 2 && tok[1] == '=') map[tok[0]] = tok.Substring(2);
            }
            Parse(DefaultPalette);
            if (Palettes.TryGetValue(prefabName, out var over)) Parse(over);
            return map;
        }

        // ------------------------------------------------------------------ apply

        private static readonly string[] PrefabFolders = { "Assets/Resources/Heroes", "Assets/Resources/Creatures" };

        [MenuItem("Aetheria/4b. Dress Character Materials")]
        public static void DressAll()
        {
            var lib = BuildLibrary();
            int dressed = 0, skipped = 0;
            var folders = Array.FindAll(PrefabFolders, AssetDatabase.IsValidFolder);
            foreach (var guid in AssetDatabase.FindAssets("t:Prefab", folders))
            {
                string path = AssetDatabase.GUIDToAssetPath(guid);
                if (DressPrefab(path, lib)) dressed++; else skipped++;
            }
            AssetDatabase.SaveAssets();
            Debug.Log($"[CharPBR] Dressed {dressed} prefab(s), left {skipped} untouched; {lib.Count} CharPBR_* materials in {MatFolder}.");
        }

        private static bool DressPrefab(string prefabPath, Dictionary<string, Material> lib)
        {
            string prefabName = Path.GetFileNameWithoutExtension(prefabPath);
            var root = PrefabUtility.LoadPrefabContents(prefabPath);
            try
            {
                var palette = PaletteFor(prefabName);
                var slots = new List<AetheriaCharacterLook.RendererSlots>();

                foreach (var smr in root.GetComponentsInChildren<SkinnedMeshRenderer>(true))
                {
                    if (smr.sharedMesh == null) continue;
                    string meshPath = AssetDatabase.GetAssetPath(smr.sharedMesh);
                    if (!MeshRoles.TryGetValue(meshPath, out var roles)) continue;

                    var mats = smr.sharedMaterials;
                    if (mats.Length != roles.Length || smr.sharedMesh.subMeshCount != roles.Length)
                    {
                        Debug.LogWarning($"[CharPBR] {prefabName}/{smr.name}: {mats.Length} materials, {smr.sharedMesh.subMeshCount} sub-meshes, " +
                                         $"table has {roles.Length} for {meshPath}. Left as is.");
                        continue;
                    }

                    var classes = new SlotClass[roles.Length];
                    bool ok = true;
                    for (int i = 0; i < roles.Length && ok; i++)
                    {
                        if (palette.TryGetValue(roles[i], out var key) && lib.TryGetValue(key, out var mat) && mat != null)
                        {
                            mats[i] = mat;
                            classes[i] = ClassOf(roles[i]);
                        }
                        else
                        {
                            Debug.LogError($"[CharPBR] {prefabName}: no material for role '{roles[i]}' (slot {i}).");
                            ok = false;
                        }
                    }
                    if (!ok) continue;

                    smr.sharedMaterials = mats;
                    slots.Add(new AetheriaCharacterLook.RendererSlots { Renderer = smr, Classes = classes });
                }

                if (slots.Count == 0) return false;

                // Slot metadata for HeroController.ApplyLookTints (player customisation by meaning).
                var look = root.GetComponent<AetheriaCharacterLook>();
                if (look == null) look = root.AddComponent<AetheriaCharacterLook>();
                look.Renderers = slots;
                look.IsHumanoid = prefabPath.StartsWith("Assets/Resources/Heroes/", StringComparison.Ordinal);

                PrefabUtility.SaveAsPrefabAsset(root, prefabPath);
                Debug.Log($"[CharPBR] {prefabName}: dressed {slots.Count} renderer(s).");
                return true;
            }
            finally
            {
                PrefabUtility.UnloadPrefabContents(root);
            }
        }
    }
}
