using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;
using Object = UnityEngine.Object;

namespace Aetheria.Editor
{
    /// <summary>
    /// Turns Oakhaven into a lived-in village around the existing plaza, without moving any quest object:
    /// timber-frame houses ring the square and line the road facing the street, a market with stalls and
    /// goods, a well with benches, lamp posts, garden fences, barrels/crates/baskets by the doors, and a
    /// wooden pier on Mirror Lake. Re-runnable (replaces its own root). Run after "6. Dress World".
    /// </summary>
    public static class AetheriaVillageBuilder
    {
        private const string ScenePath = "Assets/Scenes/AetheriaMain.unity";
        private const string Folder = AetheriaProcFlora.Folder;
        private const string PHM = "Assets/_AAA_Imported/PolyHaven/Models/";
        private const string PHT = "Assets/_AAA_Imported/PolyHaven/Textures/";
        private static readonly Vector3 Plaza = new Vector3(0f, 0f, 42f);

        private static Terrain _terrain;
        private static float H(Vector3 p) => _terrain.SampleHeight(new Vector3(p.x, 0f, p.z)) + _terrain.transform.position.y;
        private static Vector3 Flat(Vector3 v) { v.y = 0f; return v; }

        private struct Obstacle { public Vector2 P; public float R; }

        [MenuItem("Aetheria/8. Build Oakhaven Village")]
        public static void Build()
        {
            var scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            _terrain = Object.FindObjectsByType<Terrain>(FindObjectsSortMode.None).FirstOrDefault();
            if (_terrain == null) throw new System.InvalidOperationException("[Village] No terrain.");
            var old = GameObject.Find("Aetheria_Village");
            if (old != null) Object.DestroyImmediate(old);
            var root = new GameObject("Aetheria_Village").transform;

            var obstacles = CollectObstacles();
            var mats = HouseMaterials();
            var rng = new System.Random(1337);
            var placed = new List<Obstacle>();
            int houses = 0;

            // ---- houses around the square, facing it ------------------------------------------------
            for (int ring = 0; ring < 2; ring++)
            {
                float radius = ring == 0 ? 33f : 45f;
                int slots = ring == 0 ? 20 : 26;
                for (int i = 0; i < slots; i++)
                {
                    float a = (i + (ring == 0 ? 0f : 0.5f)) / slots * Mathf.PI * 2f;
                    Vector3 p = Plaza + new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a)) * (radius + (float)rng.NextDouble() * 3f);
                    if (TryHouse(root, mats, rng, p, Flat(Plaza - p), obstacles, placed, ring == 0)) houses++;
                }
            }
            // ---- houses along the road south of the square -----------------------------------------
            for (float z = 10f; z > -60f; z -= 9f)
            for (int side = -1; side <= 1; side += 2)
            {
                Vector3 p = new Vector3(2f + side * (10.5f + (float)rng.NextDouble() * 2f), 0f, z + (float)rng.NextDouble() * 3f);
                if (TryHouse(root, mats, rng, p, new Vector3(-side, 0f, 0f), obstacles, placed, false)) houses++;
            }

            // ---- market, well, lamps, fences, pier ---------------------------------------------------
            int stalls = BuildMarket(root, mats, rng, obstacles, placed);
            BuildWell(root, mats, obstacles, placed);
            int lamps = BuildLamps(root, mats, rng, obstacles, placed);
            BuildPier(root);
            ClearFoliage(placed);

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene);
            AssetDatabase.SaveAssets();
            Debug.Log($"[Village] {houses} houses, {stalls} stalls, {lamps} lamps.");
        }

        // ------------------------------------------------------------------ obstacles

        private static List<Obstacle> CollectObstacles()
        {
            var list = new List<Obstacle>();
            foreach (var rootName in new[] { "AetheriaEntities", "AetheriaEnvironment", "AetheriaPrologue" })
            {
                var r = GameObject.Find(rootName);
                if (r == null) continue;
                foreach (Transform child in r.transform)
                {
                    float radius = 3f;
                    var rends = child.GetComponentsInChildren<Renderer>(true);
                    if (rends.Length > 0)
                    {
                        var b = rends[0].bounds;
                        foreach (var rr in rends) b.Encapsulate(rr.bounds);
                        if (b.size.x > 60f || b.size.z > 60f) continue;   // big parents (vegetation batches, cave) are handled elsewhere
                        radius = Mathf.Max(b.extents.x, b.extents.z) + 2f;
                    }
                    list.Add(new Obstacle { P = new Vector2(child.position.x, child.position.z), R = radius });
                }
            }
            var hero = GameObject.Find("HeroPlayer");
            if (hero != null) list.Add(new Obstacle { P = new Vector2(hero.transform.position.x, hero.transform.position.z), R = 5f });
            return list;
        }

        private static bool Blocked(List<Obstacle> obstacles, List<Obstacle> placed, Vector3 p, float r)
        {
            var q = new Vector2(p.x, p.z);
            foreach (var o in obstacles) if ((o.P - q).magnitude < o.R + r) return true;
            foreach (var o in placed) if ((o.P - q).magnitude < o.R + r) return true;
            return false;
        }

        private static bool OnRoad(Vector3 p, float margin) =>
            (Mathf.Abs(p.x - 2f) < 5.5f + margin && p.z > -115f && p.z < 58f) ||
            Vector3.Distance(Flat(p), Plaza) < 27f + margin;

        // ------------------------------------------------------------------ materials

        private static Texture2D Tex(string path, bool normal)
        {
            var imp = AssetImporter.GetAtPath(path) as TextureImporter;
            if (imp == null) return null;
            bool dirty = false;
            if (normal && imp.textureType != TextureImporterType.NormalMap) { imp.textureType = TextureImporterType.NormalMap; dirty = true; }
            if (!normal && path.Contains("_arm") && imp.sRGBTexture) { imp.sRGBTexture = false; dirty = true; }
            if (dirty) imp.SaveAndReimport();
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }

        private static Material Lit(string name, string texId, Color tint, float smooth, bool twoSided = false)
        {
            string path = $"{Folder}/{name}.mat";
            var m = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (m == null) { m = new Material(Shader.Find("HDRP/Lit")); AssetDatabase.CreateAsset(m, path); }
            m.shader = Shader.Find("HDRP/Lit");
            if (texId != null)
            {
                m.SetTexture("_BaseColorMap", Tex($"{PHT}{texId}/{texId}_diff_2k.jpg", false));
                var n = Tex($"{PHT}{texId}/{texId}_nor_gl_2k.jpg", true);
                if (n != null) { m.SetTexture("_NormalMap", n); m.SetFloat("_NormalScale", 1.2f); }
            }
            m.SetColor("_BaseColor", tint);
            m.SetFloat("_Smoothness", smooth);
            m.SetFloat("_SmoothnessRemapMax", smooth);
            m.SetFloat("_Metallic", 0f);
            m.SetFloat("_DoubleSidedEnable", twoSided ? 1f : 0f);
            m.enableInstancing = true;
            AetheriaHdrpConvert.ValidateHdMaterial(m);
            EditorUtility.SetDirty(m);
            return m;
        }

        private sealed class Mats
        {
            public Material Stone, Plaster, PlasterOld, Timber, Thatch, Slate, Dark, Wood;
            public Material[] Canvas;
        }

        private static Mats HouseMaterials() => new Mats
        {
            Stone = Lit("House_Stone", "stacked_stone_wall", Color.white, 0.2f),
            Plaster = Lit("House_Plaster", "white_plaster_rough_01", new Color(0.95f, 0.92f, 0.86f), 0.15f),
            PlasterOld = Lit("House_PlasterOld", "worn_mossy_plasterwall", Color.white, 0.15f),
            Timber = Lit("House_Timber", "dark_wooden_planks", new Color(0.8f, 0.75f, 0.7f), 0.25f),
            Thatch = Lit("House_Thatch", "thatch_roof_angled", Color.white, 0.1f, true),
            Slate = Lit("House_Slate", "red_slate_roof_tiles_01", Color.white, 0.35f, true),
            Dark = Lit("House_Dark", null, new Color(0.04f, 0.03f, 0.025f), 0.1f),
            Wood = Lit("Village_Wood", "weathered_brown_planks", Color.white, 0.2f, true),
            Canvas = new[]
            {
                Lit("Canvas_Red", null, new Color(0.55f, 0.12f, 0.1f), 0.1f, true),
                Lit("Canvas_Cream", null, new Color(0.82f, 0.76f, 0.62f), 0.1f, true),
                Lit("Canvas_Blue", null, new Color(0.16f, 0.25f, 0.45f), 0.1f, true),
                Lit("Canvas_Green", null, new Color(0.2f, 0.36f, 0.18f), 0.1f, true),
            },
        };

        // ------------------------------------------------------------------ houses

        private static bool TryHouse(Transform root, Mats m, System.Random rng, Vector3 p, Vector3 facing, List<Obstacle> obstacles,
            List<Obstacle> placed, bool large)
        {
            var spec = AetheriaHouseKit.RandomSpec(rng, large && rng.NextDouble() < 0.5);
            float footprint = Mathf.Max(spec.Width, spec.Depth) * 0.5f + 1.2f;
            if (OnRoad(p, footprint) || Blocked(obstacles, placed, p, footprint)) return false;
            Quaternion rot = Quaternion.LookRotation(facing.normalized, Vector3.up);
            // Ground level: the lowest corner, so the plinth never floats; reject lots that are too steep.
            float lo = float.MaxValue, hi = float.MinValue;
            foreach (var c in new[] { new Vector3(-1, 0, -1), new Vector3(1, 0, -1), new Vector3(1, 0, 1), new Vector3(-1, 0, 1), Vector3.zero })
            {
                Vector3 q = p + rot * Vector3.Scale(c, new Vector3(spec.Width * 0.5f + 0.5f, 0f, spec.Depth * 0.5f + 0.5f));
                float h = H(q);
                lo = Mathf.Min(lo, h); hi = Mathf.Max(hi, h);
            }
            if (hi - lo > 2.4f || lo < 0.6f) return false;

            var mesh = AetheriaProcFlora.SaveAsset(AetheriaHouseKit.House(spec, out float ridge), "House_" + placed.Count);
            var go = new GameObject("House_" + placed.Count);
            go.transform.SetParent(root, false);
            go.transform.SetPositionAndRotation(new Vector3(p.x, lo, p.z), rot);
            go.isStatic = true;
            go.AddComponent<MeshFilter>().sharedMesh = mesh;
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterials = new[]
            {
                m.Stone, rng.NextDouble() < 0.35 ? m.PlasterOld : m.Plaster, m.Timber,
                rng.NextDouble() < 0.55 ? m.Thatch : m.Slate, m.Dark,
            };
            float wallTop = spec.Plinth + spec.Storeys * spec.StoreyHeight;
            var box = go.AddComponent<BoxCollider>();
            box.center = new Vector3(0f, wallTop * 0.5f, 0f);
            box.size = new Vector3(spec.Width + 0.3f, wallTop, spec.Depth + 0.3f);
            placed.Add(new Obstacle { P = new Vector2(p.x, p.z), R = footprint });

            DressHouse(root, go.transform, spec, rng);
            return true;
        }

        private static readonly string[] YardProps =
        {
            "wooden_barrels_01", "wine_barrel_01", "Barrel_01", "wooden_crate_01", "wooden_crate_02", "wooden_bucket_01",
            "wicker_basket_01", "wicker_basket_02", "wooden_stool_01", "painted_wooden_bench",
        };

        private static void DressHouse(Transform root, Transform house, AetheriaHouseKit.HouseSpec spec, System.Random rng)
        {
            int n = 2 + rng.Next(3);
            for (int i = 0; i < n; i++)
            {
                // Against the front wall beside the door, or along a side wall.
                bool front = rng.NextDouble() < 0.6;
                float x = front ? (rng.NextDouble() < 0.5 ? -1f : 1f) * (spec.Width * 0.5f - 0.6f - (float)rng.NextDouble() * 1.2f)
                                : (rng.NextDouble() < 0.5 ? -1f : 1f) * (spec.Width * 0.5f + 0.55f);
                float z = front ? spec.Depth * 0.5f + 0.55f : ((float)rng.NextDouble() - 0.5f) * (spec.Depth - 1.5f);
                Vector3 local = new Vector3(x, 0f, z);
                Vector3 p = house.TransformPoint(local);
                Prop(root, YardProps[rng.Next(YardProps.Length)], p, house.rotation * Quaternion.Euler(0f, (float)rng.NextDouble() * 50f - 25f, 0f));
            }
            if (rng.NextDouble() < 0.3)
            {
                Vector3 p = house.TransformPoint(new Vector3(spec.Width * 0.5f + 0.3f, 0f, -spec.Depth * 0.25f));
                Prop(root, "wooden_ladder", p, house.rotation * Quaternion.Euler(0f, 90f, 0f));
            }
        }

        private static GameObject Prop(Transform root, string id, Vector3 p, Quaternion rot, float scale = 1f)
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>($"{PHM}{id}/{id}_1k.gltf");
            if (prefab == null) return null;
            var go = (GameObject)PrefabUtility.InstantiatePrefab(prefab);
            go.transform.SetParent(root, true);
            go.transform.rotation = rot;
            go.transform.localScale = Vector3.one * scale;
            go.transform.position = new Vector3(p.x, H(p), p.z);
            go.isStatic = true;
            // Small props get a box collider so the hero bumps into them instead of walking through.
            var rends = go.GetComponentsInChildren<Renderer>();
            if (rends.Length > 0)
            {
                var b = rends[0].bounds;
                foreach (var r in rends) b.Encapsulate(r.bounds);
                var bc = go.AddComponent<BoxCollider>();
                bc.center = go.transform.InverseTransformPoint(b.center);
                bc.size = Vector3.Scale(go.transform.InverseTransformVector(b.size), new Vector3(1, 1, 1));
                bc.size = new Vector3(Mathf.Abs(bc.size.x), Mathf.Abs(bc.size.y), Mathf.Abs(bc.size.z));
            }
            return go;
        }

        // ------------------------------------------------------------------ market and square

        private static int BuildMarket(Transform root, Mats m, System.Random rng, List<Obstacle> obstacles, List<Obstacle> placed)
        {
            int count = 0;
            var goods = new[] { "wicker_basket_01", "wicker_basket_02", "wooden_crate_01", "wooden_crate_02", "wooden_barrels_01", "wooden_bucket_01" };
            for (int i = 0; i < 14 && count < 7; i++)
            {
                float a = Mathf.Deg2Rad * (200f + i * 11f);
                Vector3 p = Plaza + new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a)) * 21f;
                if (Mathf.Abs(p.x - 2f) < 7f || Blocked(obstacles, placed, p, 2.6f)) continue;
                var mesh = AetheriaProcFlora.SaveAsset(AetheriaHouseKit.Stall(i), "Stall_" + i);
                var go = new GameObject("Market_Stall_" + count);
                go.transform.SetParent(root, false);
                go.transform.SetPositionAndRotation(new Vector3(p.x, H(p), p.z), Quaternion.LookRotation(Flat(Plaza - p).normalized));
                go.AddComponent<MeshFilter>().sharedMesh = mesh;
                go.AddComponent<MeshRenderer>().sharedMaterials = new[] { m.Stone, m.Plaster, m.Wood, m.Canvas[count % m.Canvas.Length], m.Dark };
                var bc = go.AddComponent<BoxCollider>();
                bc.center = new Vector3(0f, 0.5f, 0.6f); bc.size = new Vector3(3.3f, 1f, 1f);
                placed.Add(new Obstacle { P = new Vector2(p.x, p.z), R = 2.6f });
                // Goods on and around the counter.
                for (int g = 0; g < 3; g++)
                {
                    Vector3 local = new Vector3(-1.2f + g * 1.2f, 0f, -0.9f - (float)rng.NextDouble() * 0.4f);
                    Prop(root, goods[rng.Next(goods.Length)], go.transform.TransformPoint(local), go.transform.rotation * Quaternion.Euler(0f, (float)rng.NextDouble() * 360f, 0f));
                }
                count++;
            }
            return count;
        }

        private static void BuildWell(Transform root, Mats m, List<Obstacle> obstacles, List<Obstacle> placed)
        {
            for (int i = 0; i < 16; i++)
            {
                float a = i / 16f * Mathf.PI * 2f;
                Vector3 p = Plaza + new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a)) * 13f;
                if (Mathf.Abs(p.x - 2f) < 6f || Blocked(obstacles, placed, p, 3f)) continue;
                var go = new GameObject("Village_Well");
                go.transform.SetParent(root, false);
                go.transform.SetPositionAndRotation(new Vector3(p.x, H(p), p.z), Quaternion.LookRotation(Flat(Plaza - p).normalized));
                go.AddComponent<MeshFilter>().sharedMesh = AetheriaProcFlora.SaveAsset(AetheriaHouseKit.Well(), "Well");
                go.AddComponent<MeshRenderer>().sharedMaterials = new[] { m.Stone, m.Plaster, m.Timber, m.Thatch, m.Dark };
                var cc = go.AddComponent<CapsuleCollider>(); cc.radius = 1.15f; cc.height = 2f; cc.center = Vector3.up;
                placed.Add(new Obstacle { P = new Vector2(p.x, p.z), R = 3f });
                Prop(root, "painted_wooden_bench", go.transform.TransformPoint(new Vector3(2.6f, 0f, 0f)), go.transform.rotation * Quaternion.Euler(0f, -90f, 0f));
                Prop(root, "wooden_bucket_01", go.transform.TransformPoint(new Vector3(-1.5f, 0f, 0.6f)), go.transform.rotation);
                return;
            }
        }

        private static int BuildLamps(Transform root, Mats m, System.Random rng, List<Obstacle> obstacles, List<Obstacle> placed)
        {
            var mesh = AetheriaProcFlora.SaveAsset(AetheriaHouseKit.LampPost(), "LampPost");
            var glow = AssetDatabase.LoadAssetAtPath<Material>($"{Folder}/Lamp_Glow.mat");
            if (glow == null) { glow = new Material(Shader.Find("HDRP/Lit")); AssetDatabase.CreateAsset(glow, $"{Folder}/Lamp_Glow.mat"); }
            glow.SetColor("_BaseColor", new Color(0.3f, 0.2f, 0.1f));
            glow.SetColor("_EmissiveColor", new Color(1f, 0.62f, 0.25f) * 40f);
            glow.EnableKeyword("_EMISSIVE_COLOR");
            AetheriaHdrpConvert.ValidateHdMaterial(glow);
            int count = 0;
            var spots = new List<(Vector3 p, Vector3 face)>();
            for (float z = 54f; z > -60f; z -= 16f)
            {
                spots.Add((new Vector3(-4.6f, 0f, z), Vector3.right));
                spots.Add((new Vector3(8.6f, 0f, z - 8f), Vector3.left));
            }
            for (int i = 0; i < 12; i++)
            {
                float a = i / 12f * Mathf.PI * 2f;
                Vector3 p = Plaza + new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a)) * 27.5f;
                spots.Add((p, Flat(Plaza - p).normalized));
            }
            foreach (var (p, face) in spots)
            {
                if (Blocked(obstacles, placed, p, 0.8f)) continue;
                var go = new GameObject("Lamp_Post");
                go.transform.SetParent(root, false);
                go.transform.SetPositionAndRotation(new Vector3(p.x, H(p), p.z), Quaternion.LookRotation(Vector3.Cross(Vector3.up, face)));
                go.AddComponent<MeshFilter>().sharedMesh = mesh;
                go.AddComponent<MeshRenderer>().sharedMaterials = new[] { m.Stone, m.Plaster, m.Timber, m.Thatch, glow };
                var cc = go.AddComponent<CapsuleCollider>(); cc.radius = 0.15f; cc.height = 3.2f; cc.center = Vector3.up * 1.4f;
                var lightGo = new GameObject("Lamp_Light");
                lightGo.transform.SetParent(go.transform, false);
                lightGo.transform.localPosition = new Vector3(0.75f, 2.55f, 0f);
                var l = lightGo.AddComponent<Light>();
                var hd = lightGo.AddComponent<HDAdditionalLightData>();
                l.type = LightType.Point;
                l.color = new Color(1f, 0.68f, 0.38f);
                l.lightUnit = LightUnit.Lumen;
                l.intensity = 900f;
                l.range = 11f;
                l.shadows = LightShadows.None;
                hd.affectsVolumetric = true;
                placed.Add(new Obstacle { P = new Vector2(p.x, p.z), R = 0.8f });
                count++;
            }
            return count;
        }

        private static void BuildPier(Transform root)
        {
            var lake = GameObject.Find("MirrorLakeSurface");
            if (lake == null) return;
            Vector3 c = lake.transform.position;
            Vector3 toVillage = Flat(Plaza - c).normalized;
            for (float r = 4f; r < 60f; r += 0.5f)
            {
                Vector3 p = c + toVillage * r;
                if (H(p) < c.y + 0.3f) continue;
                var prefab = AssetDatabase.LoadAssetAtPath<GameObject>($"{PHM}modular_wooden_pier/modular_wooden_pier_1k.gltf");
                if (prefab == null) return;
                var go = (GameObject)PrefabUtility.InstantiatePrefab(prefab);
                go.name = "Village_Pier";
                go.transform.SetParent(root, true);
                go.transform.rotation = Quaternion.LookRotation(-toVillage);
                go.transform.position = new Vector3(p.x, c.y + 0.35f, p.z) - toVillage * 2f;
                return;
            }
        }

        private static void ClearFoliage(List<Obstacle> placed)
        {
            var td = _terrain.terrainData;
            var o = _terrain.transform.position;
            var keep = td.treeInstances.Where(t =>
            {
                Vector3 p = Vector3.Scale(t.position, td.size) + o;
                var q = new Vector2(p.x, p.z);
                foreach (var ob in placed) if ((ob.P - q).magnitude < ob.R + 1.5f) return false;
                return true;
            }).ToArray();
            td.SetTreeInstances(keep, true);

            int res = td.detailResolution;
            if (res <= 0) return;
            for (int layer = 0; layer < td.detailPrototypes.Length; layer++)
            {
                foreach (var ob in placed)
                {
                    int cx = Mathf.RoundToInt((ob.P.x - o.x) / td.size.x * res), cz = Mathf.RoundToInt((ob.P.y - o.z) / td.size.z * res);
                    int rad = Mathf.CeilToInt(ob.R / td.size.x * res);
                    int x0 = Mathf.Clamp(cx - rad, 0, res - 1), z0 = Mathf.Clamp(cz - rad, 0, res - 1);
                    int w = Mathf.Clamp(rad * 2, 1, res - x0), h = Mathf.Clamp(rad * 2, 1, res - z0);
                    var map = td.GetDetailLayer(x0, z0, w, h, layer);
                    for (int z = 0; z < h; z++) for (int x = 0; x < w; x++) map[z, x] = 0;
                    td.SetDetailLayer(x0, z0, layer, map);
                }
            }
        }
    }
}
