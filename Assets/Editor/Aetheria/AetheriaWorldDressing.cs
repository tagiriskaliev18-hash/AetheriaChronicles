using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using Object = UnityEngine.Object;

namespace Aetheria.Editor
{
    /// <summary>
    /// Dresses the compact courtyard world the game actually ships (the sculpted terrain that every NPC, enemy,
    /// quest marker and the prologue are laid out on) without moving any gameplay object:
    ///
    ///   * repaints the terrain per biome: grass and meadow in the valley, forest floor in the woods, snow on the
    ///     glacier, ash in the caldera, stone roads and plazas, rock on every steep slope, soft blends between them
    ///   * plants instanced grass and ferns as terrain details (distance-culled by the terrain itself)
    ///   * plants procedural pines and broadleaf trees, boulders and dead trees as terrain trees, by biome
    ///
    /// Biomes come from <see cref="AetheriaWorld.ReadSceneLandmarks"/>, the same source the quest system and
    /// the minimap use, so what you see on the ground is what the map says.
    /// </summary>
    public static class AetheriaWorldDressing
    {
        private const string ScenePath = "Assets/Scenes/AetheriaMain.unity";
        private const string Folder = "Assets/Generated/Dressing";
        private const string PH = "Assets/_AAA_Imported/PolyHaven/";

        // Splat layer slots
        private const int LGrass = 0, LMeadow = 1, LForest = 2, LCliff = 3, LSnow = 4, LAsh = 5, LRoad = 6, LGravel = 7, LCount = 8;

        private const float DressRadius = 230f;   // the playable valley; the mountain ring gets trees only

        [MenuItem("Aetheria/6. Dress World (biomes, grass, trees)")]
        public static void Dress()
        {
            var scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            var terrain = Object.FindObjectsByType<Terrain>(FindObjectsSortMode.None).FirstOrDefault();
            if (terrain == null) throw new InvalidOperationException("[Dressing] No terrain in the main scene.");
            Directory.CreateDirectory(Folder);
            Directory.CreateDirectory(Folder + "/Meshes");
            Directory.CreateDirectory(Folder + "/Prefabs");
            AssetDatabase.Refresh();

            AetheriaWorld.ReadSceneLandmarks();
            var clear = CollectClearings();

            var td = terrain.terrainData;
            Paint(terrain, td, BuildLayers());
            PlantDetails(terrain, td, clear);
            PlantTrees(terrain, td, clear);

            terrain.detailObjectDistance = 120f;
            terrain.detailObjectDensity = 1f;
            terrain.treeDistance = 900f;
            terrain.treeBillboardDistance = 900f;   // mesh trees: no billboards
            terrain.treeMaximumFullLODCount = 4000;
            terrain.drawTreesAndFoliage = true;
            terrain.heightmapPixelError = 3f;
            terrain.basemapDistance = 900f;

            EditorUtility.SetDirty(td);
            AssetDatabase.SaveAssets();
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene);
            Debug.Log($"[Dressing] Done: {td.treeInstanceCount} trees, {td.detailPrototypes.Length} detail kinds, " +
                      $"{td.alphamapLayers} ground layers at {td.alphamapResolution}^2.");
        }

        // ------------------------------------------------------------------ biome sampling

        private struct Ground
        {
            public float[] W;      // per-layer weights, normalised
            public float Road, Cliff, Height, Forest, Grass, Meadow, Snow, Ash;
        }

        private static readonly float[] Scratch = new float[LCount];

        private static void BiomeMix(BiomeId b, float[] w, float k)
        {
            switch (b)
            {
                case BiomeId.SapphireWoods:
                case BiomeId.GlimmercapGrove:
                case BiomeId.NocturneHollow:
                    w[LForest] += 0.85f * k; w[LGrass] += 0.15f * k; break;
                case BiomeId.AshCaldera:
                case BiomeId.SunblightWastes:
                    w[LAsh] += 1f * k; break;
                case BiomeId.CrystalGlacier:
                    w[LSnow] += 0.8f * k; w[LGravel] += 0.2f * k; break;
                case BiomeId.HighkeepCitadel:
                    w[LGravel] += 0.35f * k; w[LMeadow] += 0.35f * k; w[LGrass] += 0.3f * k; break;
                case BiomeId.DragonAerie:
                    w[LMeadow] += 0.55f * k; w[LGravel] += 0.25f * k; w[LGrass] += 0.2f * k; break;
                case BiomeId.MirrorLake:
                    w[LGrass] += 0.65f * k; w[LMeadow] += 0.35f * k; break;
                default:
                    w[LGrass] += 0.7f * k; w[LMeadow] += 0.3f * k; break;
            }
        }

        private static float Smooth(float a, float b, float x) => Mathf.SmoothStep(0f, 1f, Mathf.InverseLerp(a, b, x));

        /// <summary>Roads and plazas of the courtyard layout, softened (same shapes the sculpt used).</summary>
        private static float RoadMask(float x, float z)
        {
            float strip = (1f - Smooth(3.2f, 5.8f, Mathf.Abs(x - 2f))) * Smooth(-116f, -108f, z) * (1f - Smooth(52f, 60f, z));
            float plaza = 1f - Smooth(18f, 26f, Vector2.Distance(new Vector2(x, z), new Vector2(0f, 42f)));
            float keep = 1f - Smooth(20f, 28f, Vector2.Distance(new Vector2(x, z), new Vector2(0f, -104f)));
            float aerie = 1f - Smooth(7f, 11f, Vector2.Distance(new Vector2(x, z), new Vector2(-34f, 22f)));
            return Mathf.Max(Mathf.Max(strip, plaza), Mathf.Max(keep, aerie * 0.6f));
        }

        private static Ground Sample(Terrain terrain, TerrainData td, float x, float z)
        {
            var o = terrain.transform.position;
            float u = (x - o.x) / td.size.x, v = (z - o.z) / td.size.z;
            float h = td.GetInterpolatedHeight(u, v) + o.y;
            float steep = td.GetSteepness(u, v);
            var w = new float[LCount];

            // Biome: soft nearest-site blend (12 m falloff) so borders read as transitions, not seams.
            var sites = AetheriaWorld.Sites;
            float best = float.MaxValue;
            foreach (var s in sites) best = Mathf.Min(best, Vector2.Distance(new Vector2(s.Position.x, s.Position.z), new Vector2(x, z)));
            float total = 0f;
            Array.Clear(Scratch, 0, LCount);
            foreach (var s in sites)
            {
                float d = Vector2.Distance(new Vector2(s.Position.x, s.Position.z), new Vector2(x, z));
                float k = Mathf.Exp(-(d - best) / 12f);
                if (k < 0.01f) continue;
                BiomeMix(s.Biome, Scratch, k);
                total += k;
            }
            for (int i = 0; i < LCount; i++) w[i] = total > 0f ? Scratch[i] / total : (i == LGrass ? 1f : 0f);

            // Mountain ring around the valley: meadow and rock, snow on the tops.
            float radial = new Vector2(x, z).magnitude;
            float ring = Smooth(150f, 280f, radial);
            for (int i = 0; i < LCount; i++) w[i] *= 1f - ring;
            w[LMeadow] += ring * 0.55f; w[LGravel] += ring * 0.15f; w[LForest] += ring * 0.3f;

            // Patchiness: grass <-> meadow, a little forest litter in the open.
            float n = Mathf.PerlinNoise(x * 0.035f + 11.3f, z * 0.035f + 7.1f);
            float shift = (n - 0.5f) * 0.8f * (w[LGrass] + w[LMeadow]);
            w[LGrass] = Mathf.Max(0f, w[LGrass] - shift);
            w[LMeadow] = Mathf.Max(0f, w[LMeadow] + shift);

            float snow = Smooth(34f, 52f, h);
            for (int i = 0; i < LCount; i++) w[i] *= 1f - snow;
            w[LSnow] += snow;

            float shore = Smooth(1.4f, 0.3f, h);
            for (int i = 0; i < LCount; i++) w[i] *= 1f - shore * 0.8f;
            w[LGravel] += shore * 0.8f;

            float road = RoadMask(x, z) * (0.8f + 0.2f * Mathf.PerlinNoise(x * 0.3f, z * 0.3f));
            for (int i = 0; i < LCount; i++) w[i] *= 1f - road;
            w[LRoad] += road;

            float cliff = Smooth(24f, 40f, steep);
            for (int i = 0; i < LCount; i++) w[i] *= 1f - cliff;
            w[LCliff] += cliff;

            float sum = 0f;
            for (int i = 0; i < LCount; i++) sum += w[i];
            for (int i = 0; i < LCount; i++) w[i] = sum > 1e-5f ? w[i] / sum : (i == LGrass ? 1f : 0f);

            return new Ground
            {
                W = w, Road = road, Cliff = cliff, Height = h,
                Forest = w[LForest], Grass = w[LGrass], Meadow = w[LMeadow], Snow = w[LSnow], Ash = w[LAsh]
            };
        }

        // ------------------------------------------------------------------ ground layers

        private static TerrainLayer[] BuildLayers()
        {
            TerrainLayer Make(string name, string diff, string nor, float tile, float smooth)
            {
                string path = $"{Folder}/TL_{name}.terrainlayer";
                var layer = AssetDatabase.LoadAssetAtPath<TerrainLayer>(path);
                bool create = layer == null;
                if (create) layer = new TerrainLayer();
                layer.diffuseTexture = AssetDatabase.LoadAssetAtPath<Texture2D>(diff);
                layer.normalMapTexture = AssetDatabase.LoadAssetAtPath<Texture2D>(nor);
                layer.normalScale = 1f;
                layer.tileSize = new Vector2(tile, tile);
                layer.smoothness = smooth;
                layer.metallic = 0f;
                if (create) AssetDatabase.CreateAsset(layer, path); else EditorUtility.SetDirty(layer);
                return layer;
            }

            TerrainLayer Reuse(string existing, Func<TerrainLayer> fallback) =>
                AssetDatabase.LoadAssetAtPath<TerrainLayer>($"Assets/Generated/TerrainLayers/{existing}.terrainlayer") ?? fallback();

            var layers = new TerrainLayer[LCount];
            layers[LGrass] = Make("Grass", PH + "Textures/leafy_grass/leafy_grass_diff.jpg", PH + "Textures/leafy_grass/leafy_grass_nor.jpg", 6f, 0.12f);
            layers[LMeadow] = Make("Meadow", PH + "Textures/aerial_grass_rock/aerial_grass_rock_diff.jpg", PH + "Textures/aerial_grass_rock/aerial_grass_rock_nor.jpg", 9f, 0.15f);
            layers[LForest] = Make("Forest", PH + "Textures/forest_ground_04/forest_ground_04_diff.jpg", PH + "Textures/forest_ground_04/forest_ground_04_nor.jpg", 7f, 0.1f);
            layers[LCliff] = Reuse("TL_LayeredCliff", () => Make("Cliff", PH + "Textures/mossy_rock/mossy_rock_diff.jpg", PH + "Textures/mossy_rock/mossy_rock_nor.jpg", 14f, 0.2f));
            layers[LSnow] = Make("Snow", PH + "Textures/snow_02/snow_02_diff.jpg", PH + "Textures/snow_02/snow_02_nor.jpg", 10f, 0.45f);
            layers[LAsh] = Reuse("TL_BurntAsh", () => Make("Ash", PH + "Textures/forest_ground_04/forest_ground_04_diff.jpg", PH + "Textures/forest_ground_04/forest_ground_04_nor.jpg", 10f, 0.1f));
            layers[LRoad] = Make("Road", PH + "Textures/mossy_cobblestone/mossy_cobblestone_diff.jpg", PH + "Textures/mossy_cobblestone/mossy_cobblestone_nor.jpg", 5f, 0.25f);
            layers[LGravel] = Reuse("TL_GravelGlacier", () => Make("Gravel", PH + "Textures/grass_path_2/grass_path_2_diff.jpg", PH + "Textures/grass_path_2/grass_path_2_nor.jpg", 8f, 0.2f));
            layers[LGrass].diffuseRemapMax = new Vector4(0.78f, 0.95f, 0.7f, 1f);    // the photo is sun-bleached yellow
            layers[LMeadow].diffuseRemapMax = new Vector4(0.85f, 0.95f, 0.78f, 1f);
            foreach (var l in layers)
                if (l.diffuseTexture == null) Debug.LogWarning($"[Dressing] Terrain layer {l.name} has no albedo.");
            return layers;
        }

        private static void Paint(Terrain terrain, TerrainData td, TerrainLayer[] layers)
        {
            td.terrainLayers = layers;
            const int res = 2048;
            td.alphamapResolution = res;
            td.baseMapResolution = 2048;
            var o = terrain.transform.position;
            var alphas = new float[res, res, LCount];
            for (int y = 0; y < res; y++)
            {
                float z = o.z + (y + 0.5f) / res * td.size.z;
                for (int x = 0; x < res; x++)
                {
                    float wx = o.x + (x + 0.5f) / res * td.size.x;
                    var g = Sample(terrain, td, wx, z);
                    for (int l = 0; l < LCount; l++) alphas[y, x, l] = g.W[l];
                }
                if (y % 256 == 0) EditorUtility.DisplayProgressBar("Dressing", "Painting ground", y / (float)res);
            }
            EditorUtility.ClearProgressBar();
            td.SetAlphamaps(0, 0, alphas);
        }

        // ------------------------------------------------------------------ clearings

        private static List<(Vector2 p, float r)> CollectClearings()
        {
            var list = new List<(Vector2, float)>();
            foreach (var rootName in new[] { "AetheriaEntities", "AetheriaEnvironment" })
            {
                var root = GameObject.Find(rootName);
                if (root == null) continue;
                foreach (Transform child in root.transform)
                {
                    var rends = child.GetComponentsInChildren<Renderer>(true);
                    float r = 2.5f;
                    if (rends.Length > 0)
                    {
                        var b = rends[0].bounds;
                        foreach (var rr in rends) b.Encapsulate(rr.bounds);
                        r = Mathf.Clamp(Mathf.Max(b.extents.x, b.extents.z) + 1.5f, 2.5f, 30f);
                    }
                    list.Add((new Vector2(child.position.x, child.position.z), r));
                }
            }
            var hero = GameObject.Find("HeroPlayer");
            if (hero != null) list.Add((new Vector2(hero.transform.position.x, hero.transform.position.z), 4f));
            return list;
        }

        private static bool Cleared(List<(Vector2 p, float r)> clear, float x, float z, float extra)
        {
            var q = new Vector2(x, z);
            foreach (var (p, r) in clear)
                if ((p - q).sqrMagnitude < (r + extra) * (r + extra)) return true;
            return false;
        }

        // ------------------------------------------------------------------ grass and ferns

        private static Material LitMaterial(string name, string diffuse, string normal, Color tint, bool cutout, bool twoSided,
            Texture2D diffuseOverride = null)
        {
            string path = $"{Folder}/{name}.mat";
            var mat = AssetDatabase.LoadAssetAtPath<Material>(path);
            bool create = mat == null;
            if (create) mat = new Material(Shader.Find("HDRP/Lit"));
            else mat.shader = Shader.Find("HDRP/Lit");
            var d = diffuseOverride != null ? diffuseOverride : (diffuse != null ? AssetDatabase.LoadAssetAtPath<Texture2D>(diffuse) : null);
            var n = normal != null ? AssetDatabase.LoadAssetAtPath<Texture2D>(normal) : null;
            mat.SetTexture("_BaseColorMap", d);
            mat.SetColor("_BaseColor", tint);
            if (n != null) { mat.SetTexture("_NormalMap", n); mat.SetFloat("_NormalScale", 1f); }
            mat.SetFloat("_Smoothness", 0.2f);
            mat.SetFloat("_SmoothnessRemapMax", 0.25f);
            mat.SetFloat("_Metallic", 0f);
            mat.SetFloat("_AlphaCutoffEnable", cutout ? 1f : 0f);
            mat.SetFloat("_AlphaCutoff", 0.5f);
            mat.SetFloat("_AlphaCutoffShadow", 0.5f);
            mat.SetFloat("_UseShadowThreshold", cutout ? 1f : 0f);
            mat.SetFloat("_DoubleSidedEnable", twoSided ? 1f : 0f);
            mat.SetFloat("_DoubleSidedNormalMode", 2f);   // none: the cards carry their own soft normals
            mat.enableInstancing = true;
            if (create) AssetDatabase.CreateAsset(mat, path);
            AetheriaHdrpConvert.ValidateHdMaterial(mat);
            EditorUtility.SetDirty(mat);
            return mat;
        }

        private static GameObject MeshPrefab(string name, Mesh mesh, Material[] mats, bool collider = false, float trunkRadius = 0f, float height = 0f)
        {
            var go = new GameObject(name);
            go.AddComponent<MeshFilter>().sharedMesh = mesh;
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterials = mats;
            if (collider)
            {
                var cap = go.AddComponent<CapsuleCollider>();
                cap.radius = trunkRadius; cap.height = height; cap.center = new Vector3(0f, height * 0.5f, 0f);
            }
            var prefab = PrefabUtility.SaveAsPrefabAsset(go, $"{Folder}/Prefabs/{name}.prefab");
            Object.DestroyImmediate(go);
            return prefab;
        }

        private static List<Mesh> ModelMeshes(string path, int maxTris)
        {
            var model = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (model == null) return new List<Mesh>();
            return model.GetComponentsInChildren<MeshFilter>(true).Select(f => f.sharedMesh)
                .Where(m => m != null && Tris(m) <= maxTris).Distinct().ToList();
        }

        private static long Tris(Mesh m)
        {
            long n = 0;
            for (int i = 0; i < m.subMeshCount; i++) n += m.GetIndexCount(i) / 3;
            return n;
        }

        private static void PlantDetails(Terrain terrain, TerrainData td, List<(Vector2 p, float r)> clear)
        {
            var grassMat = LitMaterial("Grass_Blades", PH + "Models/grass_medium_01/textures/grass_medium_01_diff_1k.jpg",
                PH + "Models/grass_medium_01/textures/grass_medium_01_nor_gl_1k.jpg", new Color(0.86f, 0.95f, 0.78f), false, true);
            var fernMat = LitMaterial("Fern_Fronds", PH + "Models/fern_02/textures/fern_02_diff_1k.jpg",
                PH + "Models/fern_02/textures/fern_02_nor_gl_1k.jpg", Color.white, false, true);

            // Light variants only: the 4-6k triangle clumps would cost millions of triangles as a carpet.
            var grass = ModelMeshes(PH + "Models/grass_medium_01/grass_medium_01_1k.gltf", 1300)
                .Concat(ModelMeshes(PH + "Models/grass_medium_02/grass_medium_02_1k.gltf", 1300))
                .Where(m => Tris(m) >= 150).Take(4).ToList();
            var ferns = ModelMeshes(PH + "Models/fern_02/fern_02_1k.gltf", 900).Take(2).ToList();

            var protos = new List<DetailPrototype>();
            var kinds = new List<int>();   // 0 grass, 1 fern
            DetailPrototype Proto(GameObject prefab, float minH, float maxH)
            {
                return new DetailPrototype
                {
                    usePrototypeMesh = true,
                    prototype = prefab,
                    renderMode = DetailRenderMode.VertexLit,
                    useInstancing = true,
                    minWidth = minH, maxWidth = maxH, minHeight = minH, maxHeight = maxH,
                    noiseSpread = 0.35f,
                    healthyColor = Color.white,
                    dryColor = new Color(0.92f, 0.88f, 0.72f),
                };
            }
            for (int i = 0; i < grass.Count; i++)
            {
                protos.Add(Proto(MeshPrefab("Detail_Grass_" + i, grass[i], new[] { grassMat }), 0.8f, 1.35f));
                kinds.Add(0);
            }
            for (int i = 0; i < ferns.Count; i++)
            {
                protos.Add(Proto(MeshPrefab("Detail_Fern_" + i, ferns[i], new[] { fernMat }), 0.75f, 1.3f));
                kinds.Add(1);
            }
            if (protos.Count == 0) { Debug.LogWarning("[Dressing] No grass/fern meshes found."); return; }

            const int res = 2048;
            // Instance-count mode: each cell value is a number of plants. The default coverage mode reads the
            // same values as a 0..255 coverage fraction, which left the valley almost bare.
            td.SetDetailScatterMode(DetailScatterMode.InstanceCountMode);
            td.SetDetailResolution(res, 32);
            td.detailPrototypes = protos.ToArray();
            var layers = protos.Select(_ => new int[res, res]).ToArray();
            var o = terrain.transform.position;
            var rng = new System.Random(4242);
            int grassKinds = kinds.Count(k => k == 0), fernKinds = kinds.Count(k => k == 1);
            for (int y = 0; y < res; y++)
            {
                float z = o.z + (y + 0.5f) / res * td.size.z;
                if (Mathf.Abs(z) > DressRadius) continue;
                for (int x = 0; x < res; x++)
                {
                    float wx = o.x + (x + 0.5f) / res * td.size.x;
                    if (wx * wx + z * z > DressRadius * DressRadius) continue;
                    var g = Sample(terrain, td, wx, z);
                    if (g.Height < 0.6f || g.Road > 0.35f || g.Cliff > 0.35f || g.Snow > 0.4f || g.Ash > 0.4f) continue;
                    if (Cleared(clear, wx, z, 0.5f)) continue;
                    float clump = Mathf.PerlinNoise(wx * 0.11f + 3.7f, z * 0.11f + 9.2f);
                    float grassAmount = (g.Grass + g.Meadow * 0.8f) * (0.35f + clump);
                    int count = Mathf.RoundToInt(grassAmount * 5f);
                    for (int c = 0; c < count && grassKinds > 0; c++)
                        layers[rng.Next(grassKinds)][y, x]++;
                    float fernAmount = g.Forest * (0.4f + clump);
                    int ferns2 = Mathf.RoundToInt(fernAmount * 2.2f);
                    for (int c = 0; c < ferns2 && fernKinds > 0; c++)
                        layers[grassKinds + rng.Next(fernKinds)][y, x]++;
                }
                if (y % 256 == 0) EditorUtility.DisplayProgressBar("Dressing", "Planting grass", y / (float)res);
            }
            EditorUtility.ClearProgressBar();
            for (int i = 0; i < layers.Length; i++) td.SetDetailLayer(0, 0, i, layers[i]);
        }

        // ------------------------------------------------------------------ trees

        /// <summary>Joins the albedo JPG and the separate alpha PNG of a PolyHaven card set into one RGBA texture.</summary>
        private static Texture2D CardTexture(string name, string diffusePath, string alphaPath)
        {
            string outPath = $"{Folder}/{name}.png";
            if (!File.Exists(outPath))
            {
                var diff = new Texture2D(2, 2); diff.LoadImage(File.ReadAllBytes(diffusePath));
                var alpha = new Texture2D(2, 2); alpha.LoadImage(File.ReadAllBytes(alphaPath));
                var outTex = new Texture2D(diff.width, diff.height, TextureFormat.RGBA32, false);
                var px = diff.GetPixels();
                for (int y = 0; y < diff.height; y++)
                for (int x = 0; x < diff.width; x++)
                {
                    float a = alpha.GetPixelBilinear((x + 0.5f) / diff.width, (y + 0.5f) / diff.height).r;
                    px[y * diff.width + x].a = a;
                }
                outTex.SetPixels(px);
                File.WriteAllBytes(outPath, outTex.EncodeToPNG());
                Object.DestroyImmediate(diff); Object.DestroyImmediate(alpha); Object.DestroyImmediate(outTex);
                AssetDatabase.ImportAsset(outPath);
            }
            var imp = (TextureImporter)AssetImporter.GetAtPath(outPath);
            imp.alphaIsTransparency = true;
            imp.mipMapsPreserveCoverage = true;   // leaves keep their volume in the distance
            imp.alphaTestReferenceValue = 0.5f;
            imp.maxTextureSize = 2048;
            imp.SaveAndReimport();
            return AssetDatabase.LoadAssetAtPath<Texture2D>(outPath);
        }

        private sealed class MeshBuilder
        {
            public readonly List<Vector3> V = new List<Vector3>();
            public readonly List<Vector3> N = new List<Vector3>();
            public readonly List<Vector2> UV = new List<Vector2>();
            public readonly List<Color> C = new List<Color>();
            public readonly List<int>[] Tris = { new List<int>(), new List<int>() };

            public void Quad(int sub, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 normal, Color col)
            {
                int i = V.Count;
                V.Add(a); V.Add(b); V.Add(c); V.Add(d);
                N.Add(normal); N.Add(normal); N.Add(normal); N.Add(normal);
                UV.Add(new Vector2(0, 0)); UV.Add(new Vector2(1, 0)); UV.Add(new Vector2(1, 1)); UV.Add(new Vector2(0, 1));
                C.Add(col); C.Add(col); C.Add(col); C.Add(col);
                Tris[sub].AddRange(new[] { i, i + 2, i + 1, i, i + 3, i + 2 });
            }

            public Mesh Build(string name)
            {
                var m = new Mesh { name = name, indexFormat = UnityEngine.Rendering.IndexFormat.UInt32 };
                m.SetVertices(V); m.SetNormals(N); m.SetUVs(0, UV); m.SetColors(C);
                m.subMeshCount = 2;
                m.SetTriangles(Tris[0], 0); m.SetTriangles(Tris[1], 1);
                m.RecalculateBounds();
                m.RecalculateTangents();
                return m;
            }
        }

        private static void Trunk(MeshBuilder mb, float height, float r0, float r1, Vector2 lean, System.Random rng)
        {
            const int sides = 9, rings = 8;
            int start = mb.V.Count;
            for (int ring = 0; ring <= rings; ring++)
            {
                float t = ring / (float)rings;
                float r = Mathf.Lerp(r0, r1, Mathf.Pow(t, 0.8f)) * (ring == 0 ? 1.35f : 1f);
                Vector3 c = new Vector3(lean.x * t * t, height * t, lean.y * t * t);
                for (int s = 0; s <= sides; s++)
                {
                    float a = s / (float)sides * Mathf.PI * 2f;
                    var n = new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a));
                    mb.V.Add(c + n * r);
                    mb.N.Add(n);
                    mb.UV.Add(new Vector2(s / (float)sides * 2f, height * t / 1.6f));
                    mb.C.Add(Color.white);
                }
            }
            for (int ring = 0; ring < rings; ring++)
            for (int s = 0; s < sides; s++)
            {
                int a = start + ring * (sides + 1) + s, b = a + sides + 1;
                mb.Tris[0].AddRange(new[] { a, b, a + 1, a + 1, b, b + 1 });
            }
        }

        /// <summary>A crossed pair of cards hanging from 'root' along 'dir'; normals bend away from the crown centre.</summary>
        private static void Spray(MeshBuilder mb, Vector3 root, Vector3 dir, float length, float width, Vector3 crown, Color col)
        {
            dir.Normalize();
            Vector3 side = Vector3.Cross(dir, Vector3.up);
            if (side.sqrMagnitude < 1e-4f) side = Vector3.right;
            side.Normalize();
            Vector3 up = Vector3.Cross(side, dir).normalized;
            for (int k = 0; k < 2; k++)
            {
                Vector3 across = (k == 0 ? side : up) * (width * 0.5f);
                Vector3 a = root - across, b = root + across, c = root + dir * length + across, d = root + dir * length - across;
                Vector3 n = ((root + dir * length * 0.5f) - crown).normalized * 0.7f + Vector3.up * 0.3f;
                mb.Quad(1, a, b, c, d, n.normalized, col);
            }
        }

        private static Mesh Pine(int seed)
        {
            var rng = new System.Random(seed);
            var mb = new MeshBuilder();
            float h = 9f + (float)rng.NextDouble() * 4f;
            Trunk(mb, h, 0.26f, 0.04f, new Vector2((float)rng.NextDouble() - 0.5f, (float)rng.NextDouble() - 0.5f) * 0.4f, rng);
            Vector3 crown = new Vector3(0f, h * 0.55f, 0f);
            int tiers = 16;
            for (int t = 0; t < tiers; t++)
            {
                float u = t / (float)(tiers - 1);
                float y = Mathf.Lerp(h * 0.22f, h * 0.97f, u);
                float reach = Mathf.Lerp(2.7f, 0.45f, u) * (0.85f + (float)rng.NextDouble() * 0.3f);
                int sprays = Mathf.Max(3, Mathf.RoundToInt(Mathf.Lerp(8f, 4f, u)));
                float phase = (float)rng.NextDouble() * Mathf.PI * 2f;
                for (int s = 0; s < sprays; s++)
                {
                    float a = phase + s / (float)sprays * Mathf.PI * 2f;
                    float droop = Mathf.Lerp(-0.45f, -0.1f, u) + ((float)rng.NextDouble() - 0.5f) * 0.15f;
                    var dir = new Vector3(Mathf.Cos(a), droop, Mathf.Sin(a));
                    float shade = Mathf.Lerp(0.72f, 1f, u);
                    Spray(mb, new Vector3(0f, y, 0f), dir, reach, reach * 0.62f + 0.35f, crown, new Color(shade, shade, shade));
                }
            }
            Spray(mb, new Vector3(0f, h * 0.93f, 0f), Vector3.up, h * 0.12f + 0.5f, 0.7f, crown, Color.white);
            return mb.Build("Pine_" + seed);
        }

        private static Mesh Broadleaf(int seed)
        {
            var rng = new System.Random(seed);
            var mb = new MeshBuilder();
            float h = 6.5f + (float)rng.NextDouble() * 2.5f;
            Trunk(mb, h * 0.7f, 0.3f, 0.12f, new Vector2((float)rng.NextDouble() - 0.5f, (float)rng.NextDouble() - 0.5f) * 0.6f, rng);
            Vector3 crown = new Vector3(0f, h * 0.72f, 0f);
            float rx = 2.6f + (float)rng.NextDouble(), ry = 2.1f + (float)rng.NextDouble() * 0.6f;
            for (int c = 0; c < 46; c++)
            {
                // Clusters on an ellipsoid shell, each a small burst of crossed cards.
                float theta = (float)rng.NextDouble() * Mathf.PI * 2f;
                float phi = Mathf.Acos(Mathf.Lerp(-0.35f, 1f, (float)rng.NextDouble()));
                Vector3 dirOut = new Vector3(Mathf.Sin(phi) * Mathf.Cos(theta), Mathf.Cos(phi), Mathf.Sin(phi) * Mathf.Sin(theta));
                Vector3 p = crown + Vector3.Scale(dirOut, new Vector3(rx, ry, rx)) * (0.55f + (float)rng.NextDouble() * 0.45f);
                float shade = Mathf.Lerp(0.7f, 1f, (dirOut.y + 1f) * 0.5f);
                for (int k = 0; k < 2; k++)
                {
                    Vector3 d = (dirOut + new Vector3((float)rng.NextDouble() - 0.5f, (float)rng.NextDouble() - 0.5f, (float)rng.NextDouble() - 0.5f)).normalized;
                    Spray(mb, p - d * 0.6f, d, 1.5f, 1.4f, crown, new Color(shade, shade, shade));
                }
            }
            return mb.Build("Broadleaf_" + seed);
        }

        private static void PlantTrees(Terrain terrain, TerrainData td, List<(Vector2 p, float r)> clear)
        {
            var pineTex = CardTexture("Pine_Twig_RGBA", PH + "Textures/pine_cards/pine_twig_diff.jpg", PH + "Textures/pine_cards/pine_twig_alpha.png");
            var leafTex = CardTexture("Broadleaf_RGBA", PH + "Textures/broadleaf_cards/broadleaf_diff.jpg", PH + "Textures/broadleaf_cards/broadleaf_alpha.png");
            var bark = LitMaterial("Bark_Pine", PH + "Textures/pine_cards/pine_bark_diff.jpg", PH + "Textures/pine_cards/pine_bark_nor.jpg", Color.white, false, false);
            var needles = LitMaterial("Leaves_Pine", null, null, new Color(0.5f, 0.66f, 0.46f), true, true, pineTex);
            var leaves = LitMaterial("Leaves_Broadleaf", null, PH + "Textures/broadleaf_cards/broadleaf_nor.jpg", new Color(0.72f, 0.86f, 0.62f), true, true, leafTex);

            var protos = new List<TreePrototype>();
            var pineIdx = new List<int>(); var leafIdx = new List<int>(); var rockIdx = new List<int>(); var deadIdx = new List<int>();
            for (int i = 0; i < 3; i++)
            {
                var mesh = Pine(100 + i);
                string mp = $"{Folder}/Meshes/{mesh.name}.asset";
                if (AssetDatabase.LoadAssetAtPath<Mesh>(mp) != null) AssetDatabase.DeleteAsset(mp);
                AssetDatabase.CreateAsset(mesh, mp);
                pineIdx.Add(protos.Count);
                protos.Add(new TreePrototype { prefab = MeshPrefab("Tree_Pine_" + i, mesh, new[] { bark, needles }, true, 0.3f, 6f) });
            }
            for (int i = 0; i < 3; i++)
            {
                var mesh = Broadleaf(200 + i);
                string mp = $"{Folder}/Meshes/{mesh.name}.asset";
                if (AssetDatabase.LoadAssetAtPath<Mesh>(mp) != null) AssetDatabase.DeleteAsset(mp);
                AssetDatabase.CreateAsset(mesh, mp);
                leafIdx.Add(protos.Count);
                protos.Add(new TreePrototype { prefab = MeshPrefab("Tree_Broadleaf_" + i, mesh, new[] { bark, leaves }, true, 0.35f, 4f) });
            }
            var rockMat = LitMaterial("Rock_Moss", PH + "Models/rock_moss_set_01/textures/rock_moss_set_01_diff_1k.jpg",
                PH + "Models/rock_moss_set_01/textures/rock_moss_set_01_nor_gl_1k.jpg", Color.white, false, false);
            foreach (var m in ModelMeshes(PH + "Models/rock_moss_set_01/rock_moss_set_01_1k.gltf", 12000).Take(4))
            {
                rockIdx.Add(protos.Count);
                protos.Add(new TreePrototype { prefab = MeshPrefab("Rock_" + rockIdx.Count, m, new[] { rockMat }) });
            }
            var deadMat = AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/DeadTree3D.mat");
            var deadModel = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/Megascans/3D/DeadTrees/SM_Dead_Tree_qlEtl_01_N1.fbx");
            if (deadModel != null && deadMat != null)
            {
                var mf = deadModel.GetComponentInChildren<MeshFilter>();
                if (mf != null)
                {
                    // Terrain trees need the renderer on the prefab root, so the FBX node rotation (Z-up) and
                    // scale are baked into a copy of the mesh.
                    var baked = Object.Instantiate(mf.sharedMesh);
                    baked.name = "DeadTree_Baked";
                    var m = deadModel.transform.worldToLocalMatrix * mf.transform.localToWorldMatrix;
                    m.SetColumn(3, new Vector4(0f, 0f, 0f, 1f));
                    var verts = baked.vertices; var norms = baked.normals;
                    for (int i = 0; i < verts.Length; i++) verts[i] = m.MultiplyPoint3x4(verts[i]);
                    for (int i = 0; i < norms.Length; i++) norms[i] = m.MultiplyVector(norms[i]).normalized;
                    baked.vertices = verts; baked.normals = norms;
                    baked.RecalculateBounds(); baked.RecalculateTangents();
                    string bp = Folder + "/Meshes/DeadTree_Baked.asset";
                    if (AssetDatabase.LoadAssetAtPath<Mesh>(bp) != null) AssetDatabase.DeleteAsset(bp);
                    AssetDatabase.CreateAsset(baked, bp);
                    var prefab = MeshPrefab("Tree_Dead", baked, new[] { deadMat });
                    deadIdx.Add(protos.Count);
                    protos.Add(new TreePrototype { prefab = prefab });
                }
            }
            td.treePrototypes = protos.ToArray();
            td.RefreshPrototypes();

            var trees = new List<TreeInstance>();
            var rng = new System.Random(777);
            var o = terrain.transform.position;
            void Add(List<int> kinds, float x, float z, float scale)
            {
                if (kinds.Count == 0) return;
                float u = (x - o.x) / td.size.x, v = (z - o.z) / td.size.z;
                if (u < 0f || u > 1f || v < 0f || v > 1f) return;
                trees.Add(new TreeInstance
                {
                    position = new Vector3(u, 0f, v),
                    prototypeIndex = kinds[rng.Next(kinds.Count)],
                    widthScale = scale, heightScale = scale * (0.9f + (float)rng.NextDouble() * 0.2f),
                    rotation = (float)rng.NextDouble() * Mathf.PI * 2f,
                    color = Color.Lerp(Color.white, new Color(0.85f, 0.9f, 0.8f), (float)rng.NextDouble()),
                    lightmapColor = Color.white,
                });
            }

            const float step = 4f;
            for (float z = -600f; z <= 600f; z += step)
            for (float x = -600f; x <= 600f; x += step)
            {
                float jx = x + ((float)rng.NextDouble() - 0.5f) * step * 0.9f;
                float jz = z + ((float)rng.NextDouble() - 0.5f) * step * 0.9f;
                float radial = new Vector2(jx, jz).magnitude;
                var g = Sample(terrain, td, jx, jz);
                if (g.Height < 0.8f || g.Road > 0.15f) continue;
                if (Cleared(clear, jx, jz, 3.5f)) continue;
                float r = (float)rng.NextDouble();
                float groves = Mathf.PerlinNoise(jx * 0.02f + 40f, jz * 0.02f + 13f);

                if (g.Cliff > 0.5f)
                {
                    if (r < 0.03f && radial < 400f) Add(rockIdx, jx, jz, 0.8f + (float)rng.NextDouble() * 1.8f);
                    continue;
                }
                if (g.Ash > 0.5f) { if (r < 0.025f) Add(deadIdx, jx, jz, 0.8f + (float)rng.NextDouble() * 0.6f); continue; }
                if (g.Snow > 0.5f) { if (r < 0.02f) Add(pineIdx, jx, jz, 0.7f + (float)rng.NextDouble() * 0.4f); continue; }

                if (radial > DressRadius)
                {
                    // Mountain ring: conifer forest on the slopes for a living horizon.
                    if (r < 0.12f * Mathf.Clamp01(groves * 1.6f)) Add(pineIdx, jx, jz, 0.9f + (float)rng.NextDouble() * 0.6f);
                    continue;
                }
                float forest = g.Forest * (0.5f + groves);
                if (r < 0.26f * forest)
                {
                    Add(rng.NextDouble() < 0.65 ? pineIdx : leafIdx, jx, jz, 0.85f + (float)rng.NextDouble() * 0.5f);
                    continue;
                }
                float open = (g.Grass + g.Meadow) * Mathf.Clamp01(groves - 0.45f) * 2f;
                if (r < 0.03f * open) { Add(leafIdx, jx, jz, 0.8f + (float)rng.NextDouble() * 0.5f); continue; }
                if (r < 0.006f) Add(rockIdx, jx, jz, 0.4f + (float)rng.NextDouble() * 0.9f);
            }
            td.SetTreeInstances(trees.ToArray(), true);
        }
    }
}
