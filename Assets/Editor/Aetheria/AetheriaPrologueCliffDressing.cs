using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;
using Object = UnityEngine.Object;

namespace Aetheria.Editor
{
    /// <summary>
    /// Turns the prologue drop into a readable cliff: photoscanned Megascans cliff walls along the edge under the
    /// collapsing ledge (hiding the stretched terrain slope), a recessed wall behind the waterfall, boulders around
    /// the plunge pool, a second faster water sheet in front of the first, and drifting local volumetric mist at the
    /// foot of the fall. Called by AetheriaPrologueBuilder after the waterfall exists.
    /// </summary>
    public static class AetheriaPrologueCliffDressing
    {
        private const string MS = "Assets/_AAA_Imported/Megascans/3D/";
        private const string Folder = AetheriaProcFlora.Folder;

        public static void Dress(Transform root, Vector3 c, Vector3 outDir, Vector3 side, float top, float waterY,
                                 IList<Vector3> fall, System.Func<Vector3, float> height)
        {
            var parent = new GameObject("Prologue_CliffFace").transform;
            parent.SetParent(root, false);

            var cliffMat = Lit("MS_NordicCliff", MS + "Cliffs/T_Huge_Nordic_Coastal_Cliff_ulkiejkva_D.png",
                               MS + "Cliffs/T_Huge_Nordic_Coastal_Cliff_ulkiejkva_N.png", 0.18f, new Color(0.82f, 0.8f, 0.78f));
            var wetMat = Lit("MS_NordicCliff_Wet", MS + "Cliffs/T_Huge_Nordic_Coastal_Cliff_ulkiejkva_D.png",
                             MS + "Cliffs/T_Huge_Nordic_Coastal_Cliff_ulkiejkva_N.png", 0.62f, new Color(0.5f, 0.52f, 0.52f));
            var boulderMat = Lit("MS_IcelandicRock", MS + "RockAssembly/T_Icelandic_Rock_Assembly_tdbmcflda_D.png", null, 0.25f, Color.white);
            var beachMat = Lit("MS_BeachBoulder", MS + "Boulders/T_Beach_Boulder_uisjbbis_D.png", null, 0.35f, new Color(0.8f, 0.8f, 0.78f));

            // Where the plateau stops: walk outward until the ground has dropped a few metres below the top.
            float edge = 12f;
            for (float r = 10f; r < 30f; r += 0.25f) { if (height(c + outDir * r) < top - 3f) { edge = r; break; } }
            float drop = top - waterY;

            // Cliff walls either side of the waterfall slot, faces toward the lake.
            string[] walls = { "Cliffs/SM_Huge_Nordic_Coastal_Cliff_ulkiejkva_01_N1.fbx", "Cliffs/SM_Huge_Nordic_Coastal_Cliff_ulkiejkva_02_N1.fbx" };
            var rng = new System.Random(321);
            int placed = 0;
            foreach (float sgn in new[] { -1f, 1f })
            {
                float off = 5.5f;
                for (int i = 0; i < 3; i++)
                {
                    var go = Spawn(MS + walls[(i + (sgn > 0 ? 1 : 0)) % 2], parent, "CliffWall");
                    if (go == null) break;
                    float h = drop + 5f + (float)rng.NextDouble() * 3f;
                    FaceAndFit(go, outDir, h, sgn * (8f + (float)rng.NextDouble() * 10f));
                    var b = Bounds(go);
                    float halfW = Mathf.Abs(Vector3.Dot(b.extents, new Vector3(Mathf.Abs(side.x), 0f, Mathf.Abs(side.z))));
                    halfW = Mathf.Max(halfW, 3f);
                    Vector3 at = c + outDir * (edge + 0.8f) + side * sgn * (off + halfW * 0.85f);
                    // Top a little under the lip so the walkable plateau stays clear; the base sinks into the lake.
                    at.y = top - 1.2f - (float)rng.NextDouble() * 1.5f;
                    PlaceTop(go, at, outDir);
                    go.AddComponent<MeshCollider>();
                    SetMaterial(go, cliffMat);
                    off += halfW * 1.55f;
                    placed++;
                }
            }

            // A darker, wet wall recessed behind the water so the fall pours over rock, not grass.
            if (fall.Count > 1)
            {
                var back = Spawn(MS + walls[0], parent, "CliffWall_BehindFall");
                if (back != null)
                {
                    float h = fall[1].y - waterY + 4f;
                    FaceAndFit(back, outDir, h, 0f);
                    Vector3 at = fall[1] - outDir * 1.6f;
                    at.y = fall[1].y + 1.0f;
                    PlaceTop(back, at, outDir);
                    SetMaterial(back, wetMat);
                    placed++;
                }
            }

            // Boulders around the plunge pool, half drowned.
            if (fall.Count > 0)
            {
                Vector3 pool = fall[fall.Count - 1];
                for (int i = 0; i < 6; i++)
                {
                    float a = (i / 6f) * Mathf.PI * 2f + (float)rng.NextDouble() * 0.6f;
                    Vector3 dir = Quaternion.AngleAxis(a * Mathf.Rad2Deg, Vector3.up) * outDir;
                    if (Vector3.Dot(dir, outDir) > 0.55f) continue;   // keep the outflow toward the lake open
                    bool big = i % 2 == 0;
                    var rock = Spawn(MS + (big ? "RockAssembly/SM_Icelandic_Rock_Assembly_tdbmcflda_N1.fbx" : "Boulders/SM_Beach_Boulder_uisjbbis_01_N1.fbx"), parent, "PoolRock");
                    if (rock == null) continue;
                    rock.transform.rotation = Quaternion.Euler(0f, (float)rng.NextDouble() * 360f, 0f);
                    FitLargest(rock, big ? 5.5f + (float)rng.NextDouble() * 3f : 2.4f + (float)rng.NextDouble() * 1.6f);
                    Vector3 at = pool + dir * (4.5f + (float)rng.NextDouble() * 3.5f);
                    at.y = waterY - 0.6f;
                    PlaceBottom(rock, at);
                    SetMaterial(rock, big ? boulderMat : beachMat);
                    placed++;
                }
            }

            AddMist(parent, fall, side, waterY);
            Debug.Log($"[Prologue] Cliff face: edge at {edge:0.0} m, drop {drop:0.0} m, {placed} rock pieces.");
        }

        // ------------------------------------------------------------------ waterfall extras

        /// <summary>A second, narrower and faster sheet just in front of the first gives the fall depth and speed.</summary>
        public static void AddFrontSheet(GameObject waterfall, Vector3 outDir)
        {
            if (waterfall == null) return;
            var mf = waterfall.GetComponent<MeshFilter>();
            var mr = waterfall.GetComponent<MeshRenderer>();
            if (mf == null || mr == null) return;
            var front = new GameObject("Prologue_Waterfall_Front");
            front.transform.SetParent(waterfall.transform.parent, false);
            front.transform.position = outDir * 0.35f;
            front.transform.localScale = new Vector3(1f, 1f, 1f);
            front.AddComponent<MeshFilter>().sharedMesh = mf.sharedMesh;
            var fr = front.AddComponent<MeshRenderer>();
            fr.sharedMaterial = mr.sharedMaterial;
            fr.shadowCastingMode = ShadowCastingMode.Off;
            front.AddComponent<AetheriaUVScroller>().Speed = new Vector2(0.05f, 2.6f);
        }

        private static void AddMist(Transform parent, IList<Vector3> fall, Vector3 side, float waterY)
        {
            if (fall.Count == 0) return;
            var mask = MistTexture();
            void Fog(string name, Vector3 centre, Vector3 size, float meanFree)
            {
                var go = new GameObject(name);
                go.transform.SetParent(parent, false);
                go.transform.position = centre;
                var fog = go.AddComponent<LocalVolumetricFog>();
                var p = fog.parameters;
                p.albedo = new Color(0.86f, 0.9f, 0.95f);
                p.meanFreePath = meanFree;
                p.size = size;
                p.positiveFade = new Vector3(0.45f, 0.6f, 0.45f);
                p.negativeFade = new Vector3(0.45f, 0.15f, 0.45f);
                p.volumeMask = mask;
                p.textureTiling = new Vector3(1.5f, 1f, 1.5f);
                p.textureScrollingSpeed = new Vector3(0.02f, 0.06f, 0.015f);
                fog.parameters = p;
            }
            Vector3 pool = fall[fall.Count - 1];
            Fog("Prologue_PoolMist", new Vector3(pool.x, waterY + 3f, pool.z), new Vector3(16f, 7f, 16f), 6f);
            if (fall.Count > 2)
            {
                Vector3 mid = (fall[1] + pool) * 0.5f;
                Fog("Prologue_FallSpray", mid, new Vector3(10f, Mathf.Abs(fall[1].y - pool.y) + 4f, 9f), 14f);
            }
        }

        private static Texture3D MistTexture()
        {
            string path = $"{Folder}/Mist_Noise3D.asset";
            var tex = AssetDatabase.LoadAssetAtPath<Texture3D>(path);
            if (tex != null) return tex;
            const int n = 32;
            tex = new Texture3D(n, n, n, TextureFormat.RGBA32, false) { wrapMode = TextureWrapMode.Repeat };
            var cols = new Color[n * n * n];
            for (int z = 0; z < n; z++)
            for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float a = Mathf.PerlinNoise(x * 0.21f + z * 0.13f, y * 0.19f) * 0.6f + Mathf.PerlinNoise(z * 0.27f, x * 0.11f + y * 0.23f) * 0.4f;
                a = Mathf.SmoothStep(0.15f, 0.85f, a);
                cols[x + y * n + z * n * n] = new Color(1f, 1f, 1f, a);
            }
            tex.SetPixels(cols);
            tex.Apply();
            AssetDatabase.CreateAsset(tex, path);
            return tex;
        }

        // ------------------------------------------------------------------ helpers

        private static GameObject Spawn(string path, Transform parent, string name)
        {
            var asset = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (asset == null) { Debug.LogWarning($"[Prologue] Missing {path}"); return null; }
            var go = (GameObject)PrefabUtility.InstantiatePrefab(asset);
            go.name = name;
            go.transform.SetParent(parent, true);
            go.isStatic = true;
            foreach (var t in go.GetComponentsInChildren<Transform>()) t.gameObject.isStatic = true;
            return go;
        }

        /// <summary>Rotates the mesh so its dominant face points along <paramref name="outDir"/>, then scales it to height <paramref name="h"/>.</summary>
        private static void FaceAndFit(GameObject go, Vector3 outDir, float h, float yawJitter)
        {
            Vector3 face = DominantFace(go);
            float yaw = Vector3.SignedAngle(face, outDir, Vector3.up) + yawJitter;
            go.transform.rotation = Quaternion.AngleAxis(yaw, Vector3.up) * go.transform.rotation;
            var b = Bounds(go);
            go.transform.localScale *= h / Mathf.Max(0.1f, b.size.y);
        }

        private static void FitLargest(GameObject go, float size)
        {
            var b = Bounds(go);
            go.transform.localScale *= size / Mathf.Max(0.1f, Mathf.Max(b.size.x, Mathf.Max(b.size.y, b.size.z)));
        }

        /// <summary>Area-weighted horizontal normal of all triangles: the side the scanned face looks toward.</summary>
        private static Vector3 DominantFace(GameObject go)
        {
            Vector3 sum = Vector3.zero;
            foreach (var mf in go.GetComponentsInChildren<MeshFilter>())
            {
                var m = mf.sharedMesh;
                if (m == null) continue;
                var v = m.vertices; var tri = m.triangles;
                var l2w = mf.transform.localToWorldMatrix;
                for (int i = 0; i < tri.Length; i += 3)
                {
                    Vector3 a = l2w.MultiplyPoint3x4(v[tri[i]]), b = l2w.MultiplyPoint3x4(v[tri[i + 1]]), cc = l2w.MultiplyPoint3x4(v[tri[i + 2]]);
                    Vector3 n = Vector3.Cross(b - a, cc - a);   // length = 2x area
                    n.y = 0f;
                    sum += n;
                }
            }
            sum.y = 0f;
            return sum.sqrMagnitude > 1e-6f ? sum.normalized : Vector3.forward;
        }

        /// <summary>Puts the top of the mesh at <paramref name="at"/>.y, its face plane centred on at's x/z.</summary>
        private static void PlaceTop(GameObject go, Vector3 at, Vector3 outDir)
        {
            var b = Bounds(go);
            Vector3 centre = b.center;
            Vector3 delta = new Vector3(at.x - centre.x, at.y - b.max.y, at.z - centre.z);
            go.transform.position += delta;
        }

        private static void PlaceBottom(GameObject go, Vector3 at)
        {
            var b = Bounds(go);
            go.transform.position += new Vector3(at.x - b.center.x, at.y - b.min.y, at.z - b.center.z);
        }

        private static Bounds Bounds(GameObject go)
        {
            var rends = go.GetComponentsInChildren<Renderer>();
            if (rends.Length == 0) return new Bounds(go.transform.position, Vector3.one);
            var b = rends[0].bounds;
            foreach (var r in rends) b.Encapsulate(r.bounds);
            return b;
        }

        private static void SetMaterial(GameObject go, Material m)
        {
            if (m == null) return;
            foreach (var r in go.GetComponentsInChildren<MeshRenderer>())
                r.sharedMaterials = Enumerable.Repeat(m, Mathf.Max(1, r.sharedMaterials.Length)).ToArray();
        }

        private static Material Lit(string name, string albedo, string normal, float smooth, Color tint)
        {
            string path = $"{Folder}/{name}.mat";
            var m = AssetDatabase.LoadAssetAtPath<Material>(path);
            bool create = m == null;
            if (create) m = new Material(Shader.Find("HDRP/Lit"));
            else m.shader = Shader.Find("HDRP/Lit");
            var a = AssetDatabase.LoadAssetAtPath<Texture2D>(albedo);
            if (a != null) m.SetTexture("_BaseColorMap", a);
            m.SetColor("_BaseColor", tint);
            if (normal != null)
            {
                var n = AssetDatabase.LoadAssetAtPath<Texture2D>(normal);
                if (n != null) { m.SetTexture("_NormalMap", n); m.EnableKeyword("_NORMALMAP"); m.SetFloat("_NormalScale", 1.2f); }
            }
            m.SetFloat("_Metallic", 0f);
            m.SetFloat("_Smoothness", smooth);
            AetheriaHdrpConvert.ValidateHdMaterial(m);
            if (create) AssetDatabase.CreateAsset(m, path);
            EditorUtility.SetDirty(m);
            return m;
        }
    }
}
