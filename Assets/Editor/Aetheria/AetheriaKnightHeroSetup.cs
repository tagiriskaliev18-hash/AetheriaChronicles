using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEngine;
using Object = UnityEngine.Object;

namespace Aetheria.Editor
{
    /// <summary>
    /// Builds Assets/Resources/Heroes/Hero_Knight.prefab from the CC-BY "Medieval Knight | Sculpture | Game ready"
    /// (by__Rx, Sketchfab 6cdd055b4afa41eb9360dbbfe75c7f10): Mixamo rig, own PBR textures (re-imported as compressed
    /// HDRP/Lit maps in Assets/Generated/Knight/), and a separate "Weapon_Sword" under the right-hand bone.
    /// Run order: 4c. Build Knight Hero -> Prepare Demo Animations -> build.
    /// </summary>
    public static class AetheriaKnightHeroSetup
    {
        public const string GlbPath = "Assets/_AAA_Imported/Sketchfab/MedievalKnight_byRx.glb";
        public const string PrefabPath = "Assets/Resources/Heroes/Hero_Knight.prefab";
        public const string OutFolder = "Assets/Generated/Knight";
        public const string TexFolder = OutFolder + "/Textures";
        private const string SwordDir = "Assets/_AAA_Imported/FabFBX/Dark_Knight_Longsword___Game_Ready_Weapon-cc7c30c4/fbx/longsword_extracted/";
        public const string SwordFbxPath = SwordDir + "LongSword.fbx";
        private const string SwordBaseColor = SwordDir + "LongSword_low_LongSword_BaseColor_sRGB.png";
        private const string SwordNormal = SwordDir + "LongSword_low_LongSword_Normal_Raw.png";
        private const string SwordOrm = SwordDir + "LongSword_low_LongSword_OcclusionRoughnessMetallic_Raw.png";   // R=AO G=rough B=metal
        private const string SwordEmissive = SwordDir + "LongSword_low_LongSword_Emissive_sRGB.png";
        public const string WeaponNodeName = "Weapon_Sword";   // HeroController.ModelWeaponNodeName

        // ================================================================ TUNING (edit, then re-run 4c + Prepare)
        /// <summary>Standing height of the prefab (HeroCustomizationService.BaseHeightMeters).</summary>
        public const float HeroHeightMeters = 1.85f;
        /// <summary>Import cap for every generated knight/sword texture (8 GB GPU budget). 4096 sources -> 2048.</summary>
        public const int MaxTextureSize = 2048;
        /// <summary>World length of the sword, pommel to tip, on the 1.85 m hero (source FBX is 1.37 m).</summary>
        public const float SwordLengthMeters = 1.15f;
        /// <summary>Grip centre along the hand, as a fraction of the wrist -> middle-knuckle distance (0 = wrist).</summary>
        public const float GripAlongFingers = 0.55f;
        /// <summary>Grip centre pushed off the hand bone toward the palm, same unit (fraction of hand length).</summary>
        public const float GripTowardPalm = 0.22f;
        /// <summary>Grip shift toward the thumb (+) / little finger (-), same unit.</summary>
        public const float GripTowardThumb = 0.0f;
        /// <summary>Extra rotation of the sword about the grip, degrees, in Weapon_Sword space
        /// (+Y = blade toward tip, +X = edge toward the knuckles, +Z = out of the palm).</summary>
        public static readonly Vector3 GripEulerTweak = new Vector3(0f, 0f, 0f);
        /// <summary>true = blade leaves the fist on the little-finger side (reverse grip).</summary>
        public static readonly bool ReverseGrip = false;
        /// <summary>Where the hand closes on the hilt: 0 = guard, 1 = pommel end.</summary>
        public const float GripOnHilt = 0.42f;
        /// <summary>Sword normal map authored DirectX-style (green down)? Writes a flipped copy when true.</summary>
        public static readonly bool SwordNormalIsDirectX = false;
        /// <summary>HDRP emissive intensity (nits) of the sword's emissive mask; 0 = no emission.</summary>
        public const float SwordEmissiveNits = 0f;
        /// <summary>Regenerate textures even when the generated PNGs already exist.</summary>
        public static bool ForceTextures = false;
        // ================================================================

        [MenuItem("Aetheria/4c. Build Knight Hero")]
        public static void Build()
        {
            if (!File.Exists(GlbPath)) { Debug.LogError($"[Knight] Missing {GlbPath}."); return; }
            var lit = Shader.Find("HDRP/Lit");
            if (lit == null) { Debug.LogError("[Knight] Shader HDRP/Lit not found."); return; }
            Directory.CreateDirectory(TexFolder);
            ConfigureGlbImporter();
            AssetDatabase.Refresh();

            var gltf = ReadGlb(GlbPath, out byte[] bin, out int binStart);
            var materials = BuildKnightMaterials(gltf, bin, binStart, lit);
            var swordMat = BuildSwordMaterial(lit);
            var swordMesh = BuildSwordMesh(out var swordGrip, out var swordTip, out var swordEdge, out float swordLength);

            var glb = AssetDatabase.LoadAssetAtPath<GameObject>(GlbPath);
            if (glb == null) { Debug.LogError($"[Knight] {GlbPath} did not import as a GameObject (glTFast)."); return; }

            var root = new GameObject("Hero_Knight");
            try
            {
                var holder = new GameObject("KnightModel").transform;
                holder.SetParent(root.transform, false);
                var model = (GameObject)PrefabUtility.InstantiatePrefab(glb, holder);
                if (PrefabUtility.IsPartOfPrefabInstance(model))
                    PrefabUtility.UnpackPrefabInstance(model, PrefabUnpackMode.Completely, InteractionMode.AutomatedAction);
                model.transform.localPosition = Vector3.zero;

                // glTFast's own clip playback must not fight the baked controller on the root.
                foreach (var a in model.GetComponentsInChildren<Animation>(true)) Object.DestroyImmediate(a);
                foreach (var a in model.GetComponentsInChildren<Animator>(true)) Object.DestroyImmediate(a);

                AssignMaterials(model, gltf, materials);
                NormaliseBody(root.transform, holder);
                AttachSword(root.transform, swordMesh, swordMat, swordGrip, swordTip, swordEdge, swordLength);

                var animator = root.AddComponent<Animator>();   // controller comes from Prepare Demo Animations
                animator.applyRootMotion = false;
                animator.cullingMode = AnimatorCullingMode.AlwaysAnimate;

                PrefabUtility.SaveAsPrefabAsset(root, PrefabPath, out bool ok);
                if (!ok) throw new IOException("SaveAsPrefabAsset failed for " + PrefabPath);
                AssetDatabase.SaveAssets();
                Debug.Log($"[Knight] {PrefabPath} built ({materials.Count} HDRP/Lit materials, sword {SwordLengthMeters:0.00} m). " +
                          "Next: Aetheria/Prepare Demo Animations.");
            }
            finally
            {
                Object.DestroyImmediate(root);
            }
        }

        /// <summary>Batch entry: Unity -batchmode -executeMethod Aetheria.Editor.AetheriaKnightHeroSetup.BuildAndBake</summary>
        public static void BuildAndBake()
        {
            Build();
            AetheriaDemoAnimations.Prepare();
        }

        // ------------------------------------------------------------------ glTFast importer

        /// <summary>No animation import (the embedded "mixamo.com" take is unused; the baker retargets the
        /// Quaternius library), mip maps on. Set through SerializedObject: no compile-time glTFast dependency.</summary>
        private static void ConfigureGlbImporter()
        {
            var importer = AssetImporter.GetAtPath(GlbPath);
            if (importer == null) return;
            var so = new SerializedObject(importer);
            bool changed = false;
            var anim = so.FindProperty("importSettings.animationMethod");
            if (anim != null && anim.intValue != 0) { anim.intValue = 0; changed = true; }   // AnimationMethod.None
            var mips = so.FindProperty("importSettings.generateMipMaps");
            if (mips != null && !mips.boolValue) { mips.boolValue = true; changed = true; }
            if (!changed) return;
            so.ApplyModifiedPropertiesWithoutUndo();
            importer.SaveAndReimport();
        }

        // ------------------------------------------------------------------ GLB reading (images come straight from the BIN chunk)

#pragma warning disable 0649 // filled by JsonUtility
        [Serializable] private class GRoot { public GImage[] images; public GView[] bufferViews; public GTex[] textures; public GMat[] materials; }
        [Serializable] private class GImage { public int bufferView = -1; public string mimeType; public string name; }
        [Serializable] private class GView { public int buffer; public int byteOffset; public int byteLength; }
        [Serializable] private class GTex { public int source = -1; }
        [Serializable] private class GRef { public int index = -1; public float scale = 1f; public float strength = 1f; }
        [Serializable] private class GPbr { public float[] baseColorFactor; public GRef baseColorTexture; public GRef metallicRoughnessTexture; public float metallicFactor = 1f; public float roughnessFactor = 1f; }
        [Serializable] private class GMat { public string name; public string alphaMode = "OPAQUE"; public float alphaCutoff = 0.5f; public bool doubleSided; public GPbr pbrMetallicRoughness; public GRef normalTexture; public GRef occlusionTexture; }

#pragma warning restore 0649

        private static GRoot ReadGlb(string path, out byte[] data, out int binStart)
        {
            data = File.ReadAllBytes(path);
            if (data.Length < 28 || BitConverter.ToUInt32(data, 0) != 0x46546C67) throw new InvalidDataException(path + " is not a GLB.");
            int jsonLen = BitConverter.ToInt32(data, 12);
            string json = Encoding.UTF8.GetString(data, 20, jsonLen);
            binStart = 20 + jsonLen + 8;   // the BIN chunk header (length, type) follows the JSON chunk
            var root = JsonUtility.FromJson<GRoot>(json);
            if (root == null || root.materials == null || root.images == null) throw new InvalidDataException(path + ": no materials/images.");
            return root;
        }

        private static int ImageIndex(GRoot g, GRef r) =>
            r == null || r.index < 0 || g.textures == null || r.index >= g.textures.Length ? -1 : g.textures[r.index].source;

        private static byte[] ImageBytes(GRoot g, byte[] data, int binStart, GRef texRef)
        {
            int img = ImageIndex(g, texRef);
            if (img < 0 || img >= g.images.Length || g.images[img].bufferView < 0) return null;
            var view = g.bufferViews[g.images[img].bufferView];
            var bytes = new byte[view.byteLength];
            Buffer.BlockCopy(data, binStart + view.byteOffset, bytes, 0, view.byteLength);
            return bytes;
        }

        // ------------------------------------------------------------------ textures

        private enum TexKind { Color, Normal, Data }

        /// <summary>Writes the image into TexFolder (unless present) and imports it compressed and capped.</summary>
        private static Texture2D WriteTexture(string fileName, byte[] bytes, TexKind kind)
        {
            if (bytes == null) return null;
            string path = $"{TexFolder}/{fileName}";
            if (ForceTextures || !File.Exists(path)) File.WriteAllBytes(path, bytes);
            return ImportTexture(path, kind);
        }

        private static Texture2D ImportTexture(string path, TexKind kind)
        {
            AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceSynchronousImport);
            var ti = AssetImporter.GetAtPath(path) as TextureImporter;
            if (ti == null) return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            ti.textureType = kind == TexKind.Normal ? TextureImporterType.NormalMap : TextureImporterType.Default;
            ti.sRGBTexture = kind == TexKind.Color;
            ti.mipmapEnabled = true;
            ti.streamingMipmaps = true;
            ti.isReadable = false;
            ti.alphaSource = TextureImporterAlphaSource.FromInput;
            ti.alphaIsTransparency = false;
            ti.maxTextureSize = MaxTextureSize;
            ti.textureCompression = TextureImporterCompression.CompressedHQ;
            var ps = ti.GetPlatformTextureSettings("Standalone");
            ps.overridden = true;
            ps.maxTextureSize = MaxTextureSize;
            ps.format = kind == TexKind.Normal ? TextureImporterFormat.BC5 : TextureImporterFormat.BC7;
            ti.SetPlatformTextureSettings(ps);
            ti.SaveAndReimport();
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }

        private static Texture2D Decode(byte[] bytes)
        {
            var t = new Texture2D(2, 2, TextureFormat.RGBA32, false, true);
            if (!t.LoadImage(bytes, false)) { Object.DestroyImmediate(t); return null; }
            return t;
        }

        /// <summary>HDRP mask map (R metal, G AO, B detail mask, A smoothness) from glTF/ORM packing
        /// (occlusion R, roughness G, metallic B). Occlusion may live in a second image of the same size.</summary>
        private static Texture2D WriteMaskMap(string fileName, byte[] metalRough, byte[] occlusion, bool occlusionInSameImage)
        {
            if (metalRough == null) return null;
            string path = $"{TexFolder}/{fileName}";
            if (ForceTextures || !File.Exists(path))
            {
                var mr = Decode(metalRough);
                if (mr == null) return null;
                var ao = !occlusionInSameImage && occlusion != null ? Decode(occlusion) : null;
                var a = mr.GetPixels32();
                Color32[] o = ao != null && ao.width == mr.width && ao.height == mr.height ? ao.GetPixels32() : null;
                var dst = new Color32[a.Length];
                for (int i = 0; i < a.Length; i++)
                {
                    byte occ = occlusionInSameImage ? a[i].r : o != null ? o[i].r : (byte)255;
                    dst[i] = new Color32(a[i].b, occ, 0, (byte)(255 - a[i].g));
                }
                var outTex = new Texture2D(mr.width, mr.height, TextureFormat.RGBA32, false, true);
                outTex.SetPixels32(dst);
                File.WriteAllBytes(path, outTex.EncodeToPNG());
                Object.DestroyImmediate(outTex);
                Object.DestroyImmediate(mr);
                if (ao != null) Object.DestroyImmediate(ao);
            }
            return ImportTexture(path, TexKind.Data);
        }

        private static Texture2D FlippedGreenCopy(string srcPath, string fileName)
        {
            string path = $"{TexFolder}/{fileName}";
            if (ForceTextures || !File.Exists(path))
            {
                var t = Decode(File.ReadAllBytes(srcPath));
                if (t == null) return null;
                var px = t.GetPixels32();
                for (int i = 0; i < px.Length; i++) px[i].g = (byte)(255 - px[i].g);
                t.SetPixels32(px);
                File.WriteAllBytes(path, t.EncodeToPNG());
                Object.DestroyImmediate(t);
            }
            return ImportTexture(path, TexKind.Normal);
        }

        // ------------------------------------------------------------------ materials

        private static Material SaveMaterial(string name, Shader lit)
        {
            string path = $"{OutFolder}/{name}.mat";
            var mat = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (mat == null) { mat = new Material(lit) { name = name }; AssetDatabase.CreateAsset(mat, path); }
            else if (mat.shader != lit) mat.shader = lit;
            return mat;
        }

        private static void SetF(Material m, string p, float v) { if (m.HasProperty(p)) m.SetFloat(p, v); }
        private static void SetT(Material m, string p, Texture t) { if (m.HasProperty(p)) m.SetTexture(p, t); }
        private static void SetC(Material m, string p, Color c) { if (m.HasProperty(p)) m.SetColor(p, c); }

        /// <summary>HDRP/Lit set-up shared by the knight and the sword. Factors follow glTF semantics.</summary>
        private static void SetupLit(Material m, Texture baseMap, Color baseColor, Texture normal, float normalScale, Texture mask,
            float metallic, float roughness, float occlusionStrength, bool alphaClip, float cutoff, bool doubleSided)
        {
            SetC(m, "_BaseColor", baseColor);
            SetT(m, "_BaseColorMap", baseMap);
            SetT(m, "_NormalMap", normal);
            SetF(m, "_NormalScale", normalScale);
            SetT(m, "_MaskMap", mask);
            // With a mask map HDRP reads metal/smoothness/AO through remaps: smoothness = 1 - roughness * G.
            SetF(m, "_Metallic", metallic);
            SetF(m, "_Smoothness", 1f - roughness);
            SetF(m, "_MetallicRemapMin", 0f);
            SetF(m, "_MetallicRemapMax", metallic);
            SetF(m, "_SmoothnessRemapMin", 1f - roughness);
            SetF(m, "_SmoothnessRemapMax", 1f);
            SetF(m, "_AORemapMin", 1f - occlusionStrength);
            SetF(m, "_AORemapMax", 1f);
            SetF(m, "_AlphaCutoffEnable", alphaClip ? 1f : 0f);
            SetF(m, "_AlphaCutoff", cutoff);
            SetF(m, "_AlphaCutoffShadow", cutoff);
            SetF(m, "_DoubleSidedEnable", doubleSided ? 1f : 0f);
            SetF(m, "_DoubleSidedNormalMode", 0f);   // Flip: glTF back faces use the reversed normal
        }

        private static Dictionary<int, Material> BuildKnightMaterials(GRoot g, byte[] bin, int binStart, Shader lit)
        {
            var result = new Dictionary<int, Material>();
            for (int i = 0; i < g.materials.Length; i++)
            {
                var gm = g.materials[i];
                string key = "Knight_" + (string.IsNullOrEmpty(gm.name) ? "Mat" + i : gm.name);
                var pbr = gm.pbrMetallicRoughness ?? new GPbr();
                var baseTex = WriteTexture(key + "_BaseColor.png", ImageBytes(g, bin, binStart, pbr.baseColorTexture), TexKind.Color);
                var normal = WriteTexture(key + "_Normal.png", ImageBytes(g, bin, binStart, gm.normalTexture), TexKind.Normal);
                int mrImage = ImageIndex(g, pbr.metallicRoughnessTexture);
                int aoImage = ImageIndex(g, gm.occlusionTexture);
                var mask = WriteMaskMap(key + "_Mask.png", ImageBytes(g, bin, binStart, pbr.metallicRoughnessTexture),
                    aoImage >= 0 && aoImage != mrImage ? ImageBytes(g, bin, binStart, gm.occlusionTexture) : null,
                    aoImage >= 0 && aoImage == mrImage);

                var f = pbr.baseColorFactor;
                Color baseColor = f != null && f.Length >= 3 ? new Color(f[0], f[1], f[2], f.Length > 3 ? f[3] : 1f) : Color.white;
                var mat = SaveMaterial(key, lit);
                SetupLit(mat, baseTex, baseColor, normal, gm.normalTexture != null && gm.normalTexture.index >= 0 ? gm.normalTexture.scale : 1f,
                    mask, mask != null ? pbr.metallicFactor : 0f, mask != null ? pbr.roughnessFactor : 0.6f,
                    gm.occlusionTexture != null && gm.occlusionTexture.index >= 0 ? gm.occlusionTexture.strength : 0f,
                    gm.alphaMode == "MASK", gm.alphaCutoff, gm.doubleSided);
                AetheriaHdrpConvert.ValidateHdMaterial(mat);
                EditorUtility.SetDirty(mat);
                result[i] = mat;
            }
            AssetDatabase.SaveAssets();
            return result;
        }

        private static Material BuildSwordMaterial(Shader lit)
        {
            var baseTex = AssetDatabase.LoadAssetAtPath<Texture2D>(SwordBaseColor);
            var normal = SwordNormalIsDirectX && File.Exists(SwordNormal)
                ? FlippedGreenCopy(SwordNormal, "Knight_Sword_NormalGL.png")
                : AssetDatabase.LoadAssetAtPath<Texture2D>(SwordNormal);
            var mask = File.Exists(SwordOrm) ? WriteMaskMap("Knight_Sword_Mask.png", File.ReadAllBytes(SwordOrm), null, true) : null;
            var mat = SaveMaterial("Knight_Sword", lit);
            SetupLit(mat, baseTex, Color.white, normal, 1f, mask, mask != null ? 1f : 0.9f, mask != null ? 1f : 0.35f, 1f, false, 0.5f, false);
            var emissive = AssetDatabase.LoadAssetAtPath<Texture2D>(SwordEmissive);
            if (SwordEmissiveNits > 0f && emissive != null)
            {
                SetT(mat, "_EmissiveColorMap", emissive);
                SetF(mat, "_UseEmissiveIntensity", 1f);
                SetF(mat, "_EmissiveIntensity", SwordEmissiveNits);
                SetC(mat, "_EmissiveColorLDR", Color.white);
                SetC(mat, "_EmissiveColor", Color.white * SwordEmissiveNits);
            }
            else
            {
                SetT(mat, "_EmissiveColorMap", null);
                SetC(mat, "_EmissiveColor", Color.black);
            }
            AetheriaHdrpConvert.ValidateHdMaterial(mat);
            EditorUtility.SetDirty(mat);
            AssetDatabase.SaveAssets();
            return mat;
        }

        /// <summary>Every sub-mesh gets the HDRP/Lit copy of its glTF material (matched by material name, else by
        /// the "_blinnN_" part of the glTF mesh name). glTFast's shader-graph materials stay unused.</summary>
        private static void AssignMaterials(GameObject model, GRoot g, Dictionary<int, Material> mats)
        {
            int unmatched = 0;
            foreach (var r in model.GetComponentsInChildren<Renderer>(true))
            {
                var shared = r.sharedMaterials;
                string meshName = r is SkinnedMeshRenderer smr && smr.sharedMesh != null ? smr.sharedMesh.name
                    : r.GetComponent<MeshFilter>() != null && r.GetComponent<MeshFilter>().sharedMesh != null ? r.GetComponent<MeshFilter>().sharedMesh.name : r.name;
                for (int s = 0; s < shared.Length; s++)
                {
                    string n = shared[s] != null ? shared[s].name : "";
                    int k = Array.FindIndex(g.materials, m => !string.IsNullOrEmpty(m.name) && m.name == n);
                    if (k < 0) k = Array.FindIndex(g.materials, m => !string.IsNullOrEmpty(m.name) && meshName.Contains("_" + m.name + "_"));
                    if (k < 0 && g.materials.Length == 1) k = 0;
                    if (k >= 0 && mats.TryGetValue(k, out var mat)) shared[s] = mat; else unmatched++;
                }
                r.sharedMaterials = shared;
                r.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.On;
                if (r is SkinnedMeshRenderer sk) sk.skinnedMotionVectors = true;
            }
            if (unmatched > 0) Debug.LogWarning($"[Knight] {unmatched} material slot(s) kept their glTFast material (no name match).");
        }

        // ------------------------------------------------------------------ bones and body

        /// <summary>"mixamorig:RightHand_030" / "mixamorig_RightHand" -> "RightHand".</summary>
        private static string BoneKey(string raw)
        {
            string s = raw.Trim();
            if (s.StartsWith("mixamorig", StringComparison.OrdinalIgnoreCase)) s = s.Substring("mixamorig".Length).TrimStart(':', '_');
            int u = s.LastIndexOf('_');
            bool digits = u > 0 && u < s.Length - 1;
            for (int i = u + 1; digits && i < s.Length; i++) digits = char.IsDigit(s[i]);
            return digits ? s.Substring(0, u) : s;
        }

        private static Transform Bone(Transform root, params string[] keys)
        {
            var all = root.GetComponentsInChildren<Transform>(true);
            foreach (var k in keys)
            {
                var hit = all.FirstOrDefault(t => string.Equals(BoneKey(t.name), k, StringComparison.OrdinalIgnoreCase));
                if (hit != null) return hit;
            }
            return null;
        }

        /// <summary>
        /// World bounds of the skinned body in its current pose, from CPU-skinned vertices. BakeMesh's output space
        /// is ambiguous under a scaled parent chain (this GLB: 0.01 on the ".fbx" node), so both readings are tried
        /// - (a) unscaled vertices placed with TRS(pos, rot, 1) and (b) vertices through localToWorldMatrix - and the
        /// one whose height matches the skeleton (head top to lowest foot joint) wins. v1 took (a) blindly and got
        /// 297 "m" for a 2.96 m body, which shrank the hero to 1.85 cm (invisible).
        /// </summary>
        internal static Bounds SkinnedWorldBounds(Transform root)
        {
            // Joint positions are unambiguous in world space whatever scale sits in the hierarchy (BakeMesh was
            // not: both readings disagreed with the skeleton). Bounds of all skin joints, padded for the skin
            // that lies outside them: soles under the toe joints, helmet above the head-top joint.
            bool any = false;
            var b = new Bounds();
            foreach (var smr in root.GetComponentsInChildren<SkinnedMeshRenderer>(true))
                foreach (var bone in smr.bones)
                {
                    if (bone == null) continue;
                    if (!any) { b = new Bounds(bone.position, Vector3.zero); any = true; } else b.Encapsulate(bone.position);
                }
            if (!any) throw new InvalidOperationException("[Knight] no skinned joints in the GLB.");
            float h = b.size.y;
            b.Encapsulate(new Vector3(b.center.x, b.min.y - 0.035f * h, b.center.z));
            b.Encapsulate(new Vector3(b.center.x, b.max.y + 0.02f * h, b.center.z));
            b.Expand(new Vector3(0.12f * h, 0f, 0.12f * h));
            return b;
        }

        private static Bounds BakedBounds(Transform root, bool throughLocalToWorld)
        {
            bool any = false;
            var b = new Bounds();
            var tmp = new Mesh();
            foreach (var smr in root.GetComponentsInChildren<SkinnedMeshRenderer>(true))
            {
                if (smr.sharedMesh == null) continue;
                smr.BakeMesh(tmp, !throughLocalToWorld);
                var m = throughLocalToWorld ? smr.transform.localToWorldMatrix
                                            : Matrix4x4.TRS(smr.transform.position, smr.transform.rotation, Vector3.one);
                foreach (var v in tmp.vertices)
                {
                    var w = m.MultiplyPoint3x4(v);
                    if (!any) { b = new Bounds(w, Vector3.zero); any = true; } else b.Encapsulate(w);
                }
            }
            Object.DestroyImmediate(tmp);
            if (!any) throw new InvalidOperationException("[Knight] no skinned meshes in the GLB.");
            return b;
        }

        /// <summary>World height of the joints: head top (or head) above the lowest foot/toe joint; 0 if unknown.</summary>
        internal static float SkeletonHeight(Transform root)
        {
            var top = Bone(root, "HeadTop_End", "Head");
            var feet = new[] { Bone(root, "LeftToe_End"), Bone(root, "RightToe_End"), Bone(root, "LeftToeBase"),
                               Bone(root, "RightToeBase"), Bone(root, "LeftFoot"), Bone(root, "RightFoot") }.Where(t => t != null).ToArray();
            if (top == null || feet.Length == 0) return 0f;
            return top.position.y - feet.Min(t => t.position.y);
        }

        /// <summary>Faces +Z, stands HeroHeightMeters tall, feet on y = 0, pelvis over the origin. All on the holder,
        /// so the prefab root keeps scale 1 (HeroController.NormalizeGroundedHeight resets the root anyway).</summary>
        private static void NormaliseBody(Transform root, Transform holder)
        {
            var footL = Bone(root, "LeftFoot"); var toeL = Bone(root, "LeftToeBase");
            var footR = Bone(root, "RightFoot"); var toeR = Bone(root, "RightToeBase");
            var armL = Bone(root, "LeftArm"); var armR = Bone(root, "RightArm");
            if (footL != null && toeL != null && footR != null && toeR != null)
            {
                Vector3 fwd = Vector3.ProjectOnPlane((toeL.position - footL.position) + (toeR.position - footR.position), Vector3.up);
                if (fwd.sqrMagnitude > 1e-8f && Vector3.Dot(fwd, Vector3.forward) < 0f) holder.Rotate(0f, 180f, 0f, Space.World);
                fwd = Vector3.ProjectOnPlane((toeL.position - footL.position) + (toeR.position - footR.position), Vector3.up);
                if (Vector3.Angle(fwd, Vector3.forward) > 30f) Debug.LogWarning($"[Knight] feet point {fwd.normalized}, not +Z.");
            }
            if (armL != null && armR != null && Vector3.Dot(armR.position - armL.position, Vector3.right) < 0f)
                Debug.LogWarning("[Knight] right arm is on -X after facing +Z: the model looks mirrored.");

            var b = SkinnedWorldBounds(root);
            float raw = Mathf.Max(0.05f, b.size.y);
            holder.localScale *= HeroHeightMeters / raw;
            b = SkinnedWorldBounds(root);
            var hips = Bone(root, "Hips");
            Vector3 centre = hips != null ? hips.position : b.center;
            holder.position += new Vector3(-centre.x, -b.min.y, -centre.z);
            b = SkinnedWorldBounds(root);
            Debug.Log($"[Knight] self-check bind pose: mesh height {b.size.y:0.000} m (want {HeroHeightMeters}), skeleton {SkeletonHeight(root):0.000} m, feet y {b.min.y:0.000}.");
            if (Mathf.Abs(b.size.y - HeroHeightMeters) > 0.05f * HeroHeightMeters)
                throw new InvalidOperationException($"[Knight] normalised height {b.size.y:0.000} m is not {HeroHeightMeters} m; prefab not saved.");
            FitRendererBounds(root, b);
            Debug.Log($"[Knight] source height {raw:0.000} -> {HeroHeightMeters} m (scale x{HeroHeightMeters / raw:0.0000}).");
        }

        /// <summary>Explicit, generous skinned bounds (glTFast leaves m_AABB empty/dirty) plus updateWhenOffscreen,
        /// so the 44 renderers are never culled whatever the parent scale or pose.</summary>
        private static void FitRendererBounds(Transform root, Bounds world)
        {
            foreach (var smr in root.GetComponentsInChildren<SkinnedMeshRenderer>(true))
            {
                smr.updateWhenOffscreen = true;
                var space = smr.rootBone != null ? smr.rootBone : smr.transform;
                // World box (padded for arm swings and rolls) -> rootBone local space, as Unity expects for localBounds.
                var c = space.InverseTransformPoint(world.center);
                float pad = Mathf.Max(world.size.y, 1f);
                var ext = space.InverseTransformVector(new Vector3(pad, pad, pad));
                smr.localBounds = new Bounds(c, new Vector3(Mathf.Abs(ext.x), Mathf.Abs(ext.y), Mathf.Abs(ext.z)) * 1.2f);
            }
        }

        // ------------------------------------------------------------------ sword

        /// <summary>
        /// The FBX sword (skinned to a 2-bone rig) baked into one static mesh asset, in metres, and measured:
        /// long axis = blade, widest slice = cross-guard, the guard's near end = hilt. Outputs are in mesh space.
        /// </summary>
        private static Mesh BuildSwordMesh(out Vector3 grip, out Vector3 tipDir, out Vector3 edgeDir, out float length)
        {
            var asset = AssetDatabase.LoadAssetAtPath<GameObject>(SwordFbxPath) ?? Resources.Load<GameObject>("Weapons/LongSword");
            if (asset == null) throw new FileNotFoundException("[Knight] sword model missing: " + SwordFbxPath);
            var inst = Object.Instantiate(asset);
            var parts = new List<CombineInstance>();
            try
            {
                inst.transform.SetPositionAndRotation(Vector3.zero, Quaternion.identity);
                inst.transform.localScale = Vector3.one;
                foreach (var smr in inst.GetComponentsInChildren<SkinnedMeshRenderer>(true))
                {
                    if (smr.sharedMesh == null) continue;
                    var baked = new Mesh();
                    smr.BakeMesh(baked, true);
                    for (int s = 0; s < baked.subMeshCount; s++)
                        parts.Add(new CombineInstance { mesh = baked, subMeshIndex = s, transform = Matrix4x4.TRS(smr.transform.position, smr.transform.rotation, Vector3.one) });
                }
                foreach (var mf in inst.GetComponentsInChildren<MeshFilter>(true))
                {
                    if (mf.sharedMesh == null) continue;
                    for (int s = 0; s < mf.sharedMesh.subMeshCount; s++)
                        parts.Add(new CombineInstance { mesh = mf.sharedMesh, subMeshIndex = s, transform = mf.transform.localToWorldMatrix });
                }
            }
            finally { Object.DestroyImmediate(inst); }
            if (parts.Count == 0) throw new InvalidOperationException("[Knight] sword has no meshes.");

            var mesh = new Mesh { name = "Knight_Sword", indexFormat = UnityEngine.Rendering.IndexFormat.UInt32 };
            mesh.CombineMeshes(parts.ToArray(), true, true);
            foreach (var p in parts) if (p.mesh != null && !AssetDatabase.Contains(p.mesh)) Object.DestroyImmediate(p.mesh);
            mesh.RecalculateBounds();

            // ---- measure
            var v = mesh.vertices;
            var size = mesh.bounds.size;
            int la = size.x >= size.y && size.x >= size.z ? 0 : size.y >= size.z ? 1 : 2;
            int ea = Enumerable.Range(0, 3).Where(i => i != la).OrderByDescending(i => size[i]).First();
            float lo = mesh.bounds.min[la], hi = mesh.bounds.max[la];
            float len = hi - lo;
            length = len;
            const int bins = 48;
            var wMin = Enumerable.Repeat(float.MaxValue, bins).ToArray();
            var wMax = Enumerable.Repeat(float.MinValue, bins).ToArray();
            int Bin(float x) => Mathf.Clamp((int)((x - lo) / Mathf.Max(1e-6f, len) * bins), 0, bins - 1);
            foreach (var p in v) { int k = Bin(p[la]); wMin[k] = Mathf.Min(wMin[k], p[ea]); wMax[k] = Mathf.Max(wMax[k], p[ea]); }
            float W(int k) => wMax[k] >= wMin[k] ? wMax[k] - wMin[k] : 0f;
            int guard = Enumerable.Range(0, bins).OrderByDescending(W).First();
            bool hiltAtMax = guard > bins / 2;
            // Walk from the widest slice toward the hilt until the guard narrows: that is where the grip starts.
            int edge = guard, step = hiltAtMax ? 1 : -1;
            while (edge + step >= 0 && edge + step < bins && W(edge + step) > 0.5f * W(guard)) edge += step;
            float guardEnd = lo + (edge + (hiltAtMax ? 1f : 0f)) * length / bins;
            float hiltEnd = hiltAtMax ? hi : lo;
            float g = Mathf.Lerp(guardEnd, hiltEnd, GripOnHilt);
            // Cross-axis centre of the hilt around the grip point.
            Vector3 sum = Vector3.zero; int n = 0;
            foreach (var p in v) if (Mathf.Abs(p[la] - g) < length * 0.03f) { sum += p; n++; }
            grip = n > 0 ? sum / n : mesh.bounds.center;
            grip[la] = g;
            tipDir = Vector3.zero; tipDir[la] = hiltAtMax ? -1f : 1f;
            edgeDir = Vector3.zero; edgeDir[ea] = 1f;

            // ---- save (keeps the asset GUID on re-runs)
            string path = $"{OutFolder}/Knight_Sword_Mesh.asset";
            var existing = AssetDatabase.LoadAssetAtPath<Mesh>(path);
            if (existing == null) { AssetDatabase.CreateAsset(mesh, path); existing = mesh; }
            else { EditorUtility.CopySerialized(mesh, existing); Object.DestroyImmediate(mesh); }
            AssetDatabase.SaveAssets();
            Debug.Log($"[Knight] sword {length:0.000} m long, blade axis {"XYZ"[la]}, guard axis {"XYZ"[ea]}, hilt at {(hiltAtMax ? "+" : "-")} end, grip {grip}.");
            return existing;
        }

        /// <summary>
        /// Weapon_Sword sits on the right-hand bone at the grip point: +Y = blade (out of the thumb side of the fist),
        /// +X = edge toward the knuckles, +Z = out of the palm. Its child "SwordMesh" carries the scaled sword.
        /// </summary>
        private static void AttachSword(Transform root, Mesh mesh, Material mat, Vector3 grip, Vector3 tipDir, Vector3 edgeDir, float length)
        {
            var hand = Bone(root, "RightHand");
            var middle = Bone(root, "RightHandMiddle1", "RightHandIndex1");
            var index = Bone(root, "RightHandIndex1", "RightHandMiddle1");
            var ring = Bone(root, "RightHandRing1", "RightHandPinky1");
            var thumb = Bone(root, "RightHandThumb1");
            if (hand == null || middle == null || index == null || ring == null)
            {
                Debug.LogError("[Knight] right-hand bones not found; sword not attached.");
                return;
            }

            Vector3 fingers = middle.position - hand.position;
            float handLen = fingers.magnitude;
            Vector3 f = fingers / Mathf.Max(1e-6f, handLen);
            Vector3 across = Vector3.ProjectOnPlane(index.position - ring.position, f).normalized;   // toward the thumb
            Vector3 palm = Vector3.Cross(f, across);
            if (thumb != null)
            {
                Vector3 t = thumb.position - hand.position;
                Vector3 side = t - Vector3.Dot(t, f) * f - Vector3.Dot(t, across) * across;   // thumb base sits on the palm side
                if (side.sqrMagnitude > 1e-10f && Vector3.Dot(side, palm) < 0f) palm = -palm;
            }
            palm.Normalize();

            Vector3 blade = ReverseGrip ? -across : across;
            Vector3 edge = Vector3.ProjectOnPlane(f, blade).normalized;
            Vector3 gripWorld = hand.position + handLen * (f * GripAlongFingers + palm * GripTowardPalm + across * GripTowardThumb);
            Quaternion frame = Quaternion.LookRotation(Vector3.Cross(edge, blade), blade) * Quaternion.Euler(GripEulerTweak);

            var weapon = new GameObject(WeaponNodeName).transform;
            weapon.SetPositionAndRotation(gripWorld, frame);
            weapon.localScale = Vector3.one;
            weapon.SetParent(hand, true);   // keeps world pose; the hand's (tiny) lossy scale goes into localScale

            var swordGo = new GameObject("SwordMesh");
            swordGo.transform.SetParent(weapon, false);
            float s = SwordLengthMeters / Mathf.Max(0.01f, length);
            // Maps sword-mesh axes onto Weapon_Sword axes: tip -> +Y, edge -> +X; then the grip lands on the origin.
            Quaternion toCanonical = Quaternion.Inverse(Quaternion.LookRotation(Vector3.Cross(edgeDir, tipDir), tipDir));
            swordGo.transform.localRotation = toCanonical;
            swordGo.transform.localScale = Vector3.one * s;
            swordGo.transform.localPosition = -(toCanonical * (grip * s));
            swordGo.AddComponent<MeshFilter>().sharedMesh = mesh;
            var mr = swordGo.AddComponent<MeshRenderer>();
            mr.sharedMaterial = mat;
            mr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.On;
            Debug.Log($"[Knight] Weapon_Sword on {hand.name}: hand length {handLen:0.000} m, local pos {weapon.localPosition}, local rot {weapon.localEulerAngles}.");
        }
    }
}
