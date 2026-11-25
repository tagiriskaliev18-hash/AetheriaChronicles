// Renders a "character sheet" (front/back of every character with semantic debug colors per material slot)
// to Tools/Reports/characters.png and prints the per-slot classification. Read-only for the project.
// Run:  unity command script run --file Tools/Inspect/CharacterSheet.cs --entry AetheriaTools.CharacterSheet.Run --timeout 300
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEngine;

namespace AetheriaTools
{
    public enum SlotClass { Skin, Hair, Eye, Mouth, Cloth, ClothAlt, Leather, Metal, Emissive, Weapon, Unknown }

    public static class CharacterSheet
    {
        static readonly Color[] ClassColor =
        {
            new Color(0.95f, 0.74f, 0.60f), // Skin
            new Color(0.28f, 0.13f, 0.05f), // Hair
            new Color(1.00f, 1.00f, 1.00f), // Eye
            new Color(0.60f, 0.10f, 0.10f), // Mouth
            new Color(0.15f, 0.35f, 0.85f), // Cloth
            new Color(0.85f, 0.20f, 0.55f), // ClothAlt
            new Color(0.50f, 0.28f, 0.10f), // Leather
            new Color(0.62f, 0.64f, 0.70f), // Metal
            new Color(0.15f, 1.00f, 0.90f), // Emissive
            new Color(0.95f, 0.92f, 0.25f), // Weapon
            new Color(1.00f, 0.00f, 1.00f), // Unknown
        };

        public struct SlotInfo
        {
            public int Index;
            public int Triangles;
            public SlotClass Class;
            public float Offset;          // mean distance of the slot's vertices from the core skeleton (m)
            public string TopBones;
        }

        public static List<SlotInfo> Classify(GameObject model)
        {
            var result = new List<SlotInfo>();
            var smr = model.GetComponentInChildren<SkinnedMeshRenderer>(true);
            if (smr == null) return result;
            Mesh mesh = smr.sharedMesh;
            Transform[] bones = smr.bones;
            Vector3[] verts = mesh.vertices;
            BoneWeight[] bw = mesh.boneWeights;
            Matrix4x4[] bind = mesh.bindposes;
            int subCount = mesh.subMeshCount;

            // Core skeleton segments in bind space (renderer local)
            var segments = new List<(Vector3 a, Vector3 b)>();
            string[] core = { "pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "neck_02", "head",
                              "upperarm_l", "lowerarm_l", "hand_l", "upperarm_r", "lowerarm_r", "hand_r",
                              "thigh_l", "calf_l", "foot_l", "thigh_r", "calf_r", "foot_r" };
            var pos = new Dictionary<int, Vector3>();
            for (int b = 0; b < bones.Length; b++)
                pos[b] = bind[b].inverse.MultiplyPoint3x4(Vector3.zero);
            for (int b = 0; b < bones.Length; b++)
            {
                if (!core.Contains(bones[b].name)) continue;
                // connect to the nearest child that is also core
                foreach (var kv in pos)
                    if (kv.Key != b && bones[kv.Key] != null && bones[kv.Key].parent == bones[b] && core.Contains(bones[kv.Key].name))
                        segments.Add((pos[b], pos[kv.Key]));
            }
            if (segments.Count == 0)
                foreach (var kv in pos) segments.Add((kv.Value, kv.Value));

            var shader = Shader.Find("Unlit/Color");
            for (int s = 0; s < subCount; s++)
            {
                int[] tris = mesh.GetTriangles(s);
                var set = new HashSet<int>(tris);
                if (set.Count == 0) continue;
                var weights = new float[bones.Length];
                foreach (int v in set)
                {
                    if (v >= bw.Length) continue;
                    weights[bw[v].boneIndex0] += bw[v].weight0;
                    weights[bw[v].boneIndex1] += bw[v].weight1;
                    weights[bw[v].boneIndex2] += bw[v].weight2;
                    weights[bw[v].boneIndex3] += bw[v].weight3;
                }
                float total = weights.Sum();

                float G(Func<string, bool> f) => weights.Select((w, b) => bones[b] != null && f(bones[b].name) ? w : 0f).Sum() / Mathf.Max(1e-5f, total);
                float gWeapon = G(n => n.StartsWith("weapon") || n.StartsWith("gun_") || n.Contains("sword") || n.StartsWith("evil_weapon"));
                float gHair   = G(n => n.StartsWith("hair") || n.Contains("antler") || n.Contains("feather") || n.StartsWith("bang"));
                float gEye    = G(n => n.StartsWith("R_eye") || n.StartsWith("L_eye") || n == "eye" || n.EndsWith("_eye"));
                float gMouth  = G(n => n.Contains("teeth") || n.Contains("tongue") || n.Contains("jaw"));
                float gHead   = G(n => n == "head" || n == "faceAttach" || n.Length > 2 && (n.StartsWith("C_") || n.StartsWith("L_") || n.StartsWith("R_")) && !n.Contains("eye") && !n.Contains("lip") && !n.Contains("brow"));
                float gHood   = G(n => n.Contains("hood") || n.Contains("cowl") || n.Contains("mask") || n.Contains("helm") || n.Contains("crown") || n.Contains("horn") || n.Contains("tiara"));
                float gHand   = G(n => n.StartsWith("hand_") || n.StartsWith("index") || n.StartsWith("middle") || n.StartsWith("pinky") || n.StartsWith("ring") || n.StartsWith("thumb"));
                float gFoot   = G(n => n.StartsWith("foot") || n.StartsWith("ball"));
                float gLeg    = G(n => n.StartsWith("thigh") || n.StartsWith("calf") || n.Contains("knee"));
                float gArm    = G(n => n.Contains("clavicle") || n.Contains("upperarm") || n.Contains("lowerarm") || n.Contains("shoulder") || n.Contains("bicep") || n.Contains("tricep"));
                float gTorso  = G(n => n.StartsWith("spine") || n == "pelvis" || n.Contains("hip") || n.Contains("chest") || n.Contains("loin") || n.Contains("belt") || n.Contains("backpack"));
                float gNamedCloth = G(n => n.Contains("cloth") || n.Contains("sleeve") || n.Contains("cape") || n.Contains("loin") || n.Contains("belt") || n.Contains("scarf") || n.Contains("skirt") || n.Contains("rope"));
                float gNamedArmor = G(n => n.Contains("armor") || n.Contains("plate") || n.Contains("pauldron") || n.Contains("gauntlet") || n.Contains("bracer") || n.Contains("boot") || n.Contains("greave"));

                // Mean distance from the core skeleton: skin sits close to bones, plates further out
                float offset = 0f; int counted = 0;
                foreach (int v in set)
                {
                    if (v >= verts.Length) continue;
                    Vector3 p = verts[v];
                    float best = float.MaxValue;
                    foreach (var seg in segments)
                    {
                        Vector3 ab = seg.b - seg.a;
                        float len2 = ab.sqrMagnitude;
                        float t = len2 > 1e-8f ? Mathf.Clamp01(Vector3.Dot(p - seg.a, ab) / len2) : 0f;
                        best = Mathf.Min(best, Vector3.Distance(p, seg.a + ab * t));
                    }
                    offset += best; counted++;
                }
                offset = counted > 0 ? offset / counted : 0f;

                var topBones = weights.Select((w, b) => (w, b)).OrderByDescending(x => x.w).Take(3)
                    .Select(x => bones[x.b].name + ":" + (x.w / Mathf.Max(1e-5f, total)).ToString("F2")).ToArray();

                SlotClass cls;
                if (gWeapon > 0.30f) cls = SlotClass.Weapon;
                else if (gHair > 0.35f) cls = SlotClass.Hair;
                else if (gEye > 0.35f) cls = SlotClass.Eye;
                else if (gMouth > 0.35f) cls = SlotClass.Mouth;
                else if (gHood > 0.35f) cls = SlotClass.ClothAlt;
                else if (gHead > 0.45f) cls = SlotClass.Skin;
                else if (gFoot > 0.45f) cls = SlotClass.Leather;
                else if (gHand > 0.55f) cls = SlotClass.Skin;
                else if (gNamedArmor > 0.35f) cls = SlotClass.Metal;
                else if (gNamedCloth > 0.35f) cls = SlotClass.Cloth;
                else if (gLeg > 0.5f) cls = SlotClass.Cloth;
                else if (gArm > 0.45f || gTorso > 0.5f)
                    cls = offset < 0.075f ? SlotClass.Skin : (offset < 0.125f ? SlotClass.Cloth : SlotClass.Metal);
                else if (gHead + gHand > 0.35f) cls = SlotClass.Skin;
                else cls = SlotClass.Unknown;

                result.Add(new SlotInfo { Index = s, Triangles = tris.Length / 3, Class = cls, Offset = offset, TopBones = string.Join(",", topBones) });
            }
            return result;
        }

        public static string Run(int cell = 300)
        {
            string root = "Assets/_AAA_Imported/";
            string[] models =
            {
                "Heroes/Kwang/Meshes/Kwang_GDC.glb", "Heroes/Gideon/Meshes/Gideon.glb", "Heroes/Serath/Meshes/Serath.glb",
                "Heroes/Kallari/Meshes/Kallari.glb", "Heroes/Khaimera/Meshes/Khaimera.glb", "Heroes/Morigesh/Meshes/Morigesh.glb",
                "Boss_Sevarog/Meshes/SevarogBloodred.glb", "Enemies/Rampage/Meshes/Rampage.glb",
            };
            var sb = new StringBuilder();
            var sheet = new Texture2D(cell * 2, cell * models.Length, TextureFormat.RGBA32, false);
            var bg = new Color(0.16f, 0.17f, 0.20f);
            for (int y = 0; y < sheet.height; y++) for (int x = 0; x < sheet.width; x++) sheet.SetPixel(x, y, bg);

            var unlit = Shader.Find("Unlit/Color");
            for (int m = 0; m < models.Length; m++)
            {
                var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(root + models[m]);
                if (prefab == null) { sb.AppendLine("MISSING " + models[m]); continue; }
                var inst = UnityEngine.Object.Instantiate(prefab);
                inst.hideFlags = HideFlags.HideAndDontSave;
                var smr = inst.GetComponentInChildren<SkinnedMeshRenderer>(true);
                var slots = Classify(inst);
                sb.AppendLine($"### {models[m]}  scale-of-instance={inst.transform.lossyScale}");
                foreach (var s in slots)
                    sb.AppendLine($"   slot {s.Index,2}: {s.Class,-9} tris={s.Triangles,6} offset={s.Offset:F3}  bones={s.TopBones}");

                var baked = new Mesh();
                smr.BakeMesh(baked);
                Bounds b = baked.bounds;
                sb.AppendLine($"   baked bounds center={b.center} size={b.size}");
                Vector3 centerWorld = smr.transform.TransformPoint(b.center);
                float height = Vector3.Scale(b.size, smr.transform.lossyScale).y;
                for (int view = 0; view < 2; view++)
                {
                    var util = new PreviewRenderUtility();
                    util.camera.fieldOfView = 22f;
                    util.camera.nearClipPlane = 0.05f;
                    util.camera.farClipPlane = 40f;
                    float yaw = view == 0 ? 180f : 0f; // model faces +Z: look at its front from +Z
                    Quaternion rot = Quaternion.Euler(4f, yaw, 0f);
                    Vector3 camPos = centerWorld - rot * Vector3.forward * (height * 3.1f);
                    util.camera.transform.position = camPos;
                    util.camera.transform.rotation = Quaternion.LookRotation(centerWorld - camPos, Vector3.up);
                    util.lights[0].intensity = 1.3f; util.lights[0].transform.rotation = Quaternion.Euler(50f, 150f, 0f);
                    util.lights[1].intensity = 0.7f; util.lights[1].transform.rotation = Quaternion.Euler(30f, 210f, 0f);
                    util.ambientColor = new Color(0.35f, 0.35f, 0.4f);
                    util.BeginStaticPreview(new Rect(0, 0, cell, cell));
                    Matrix4x4 matrix = smr.transform.localToWorldMatrix;
                    GL.PushMatrix();
                    GL.modelview = util.camera.worldToCameraMatrix;
                    GL.LoadProjectionMatrix(util.camera.projectionMatrix);
                    for (int s = 0; s < baked.subMeshCount && s < slots.Count; s++)
                    {
                        var mat = new Material(unlit) { color = ClassColor[(int)slots[s].Class] };
                        mat.SetPass(0);
                        Graphics.DrawMeshNow(baked, matrix, s);
                    }
                    GL.PopMatrix();
                    var shot = util.EndStaticPreview();
                    int px = view * cell, py = sheet.height - (m + 1) * cell;
                    for (int y = 0; y < cell; y++) for (int x = 0; x < cell; x++)
                        if (shot.width > x && shot.height > y) sheet.SetPixel(px + x, py + y, shot.GetPixel(x, y));
                    util.Cleanup();
                }
                UnityEngine.Object.DestroyImmediate(baked);
                UnityEngine.Object.DestroyImmediate(inst);
            }

            Directory.CreateDirectory("Tools/Reports");
            File.WriteAllBytes("Tools/Reports/characters.png", sheet.EncodeToPNG());
            UnityEngine.Object.DestroyImmediate(sheet);
            File.WriteAllText("Tools/Reports/characters.txt", sb.ToString());
            return sb.ToString();
        }
    }
}
