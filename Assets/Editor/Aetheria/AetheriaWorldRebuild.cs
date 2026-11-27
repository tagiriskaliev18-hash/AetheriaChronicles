using System;
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria.Editor
{
    /// <summary>
    /// Rebuilds the open world on the REAL AetheriaWorld height field.
    ///
    /// The old scene sculpted a compressed 4 km mock (lake 14 m from the plaza, castle 104 m away) while
    /// every gameplay system — quests, map, teleports, enemy spawns, the dragon — queries
    /// <see cref="AetheriaWorld.HeightAt"/>, whose lake sits at (-130,-60) and whose citadel is 440 m north.
    /// The result was a world where the terrain under your feet and the world the game believed in were
    /// two different places. This builder throws the mock away and bakes the real one:
    ///
    ///   * 1.28 km x 1.28 km terrain sampled from AetheriaWorld, one vertex every 2 m
    ///   * five PBR layers blended per biome (grass / forest floor / rock / gravel / ash / snow)
    ///   * per-biome vegetation: grass, ferns, shrubs, boulders, dead trees, pines on the ridge lines
    ///   * water for the lake basin, the roads, and every landmark placed by site id
    ///
    /// Everything is driven off <see cref="AetheriaWorld.Sites"/>, so a landmark can never drift from the
    /// coordinate the quest system teleports the player to.
    /// </summary>
    public static class AetheriaWorldRebuild
    {
        private const string ScenePath = "Assets/Scenes/AetheriaMain.unity";
        private const string LayerFolder = "Assets/Generated/TerrainLayers";
        private const string MaterialFolder = "Assets/Generated/Materials";

        // Terrain resolution: 1.28 km over 640 quads = 2 m per quad. 641^2 vertices is the sweet spot
        // where HDRP terrain tessellation and the collider both stay cheap.
        private const int HeightRes = 641;
        private const int AlphaRes = 1024;
        private const float TerrainHeight = 260f;   // vertical range in metres (peaks reach ~200 m)
        // The height field dips to about -9 m in the lake and sea basins. Unity terrain heights cannot go
        // below the terrain's origin, so the terrain sits this far below sea level; without it every basin
        // was clamped flat at y=0 and the water planes floated 5 cm over a dry bed.
        private const float TerrainBaseDepth = 16f;

        private const int LayerGrass = 0;
        private const int LayerForest = 1;
        private const int LayerRock = 2;
        private const int LayerGravel = 3;
        private const int LayerAsh = 4;
        private const int LayerSnow = 5;
        private const int LayerCount = 6;

        [MenuItem("Aetheria/5. Rebuild World (real Aetheria terrain + vegetation)")]
        public static void RebuildWorld()
        {
            var scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            if (!scene.IsValid())
            {
                Debug.LogError($"[WorldRebuild] Cannot open {ScenePath}.");
                return;
            }

            // --- clear everything the old mock builder left behind -----------------------------
            foreach (var name in new[] { "AetheriaTerrain", "AetheriaEnvironment", "MirrorLakeSurface", "AetheriaWorldServices" })
            {
                var existing = GameObject.Find(name);
                if (existing != null) UnityEngine.Object.DestroyImmediate(existing);
            }
            foreach (var t in UnityEngine.Object.FindObjectsByType<Terrain>(FindObjectsSortMode.None))
                if (t != null) UnityEngine.Object.DestroyImmediate(t.gameObject);

            var layers = BuildLayers();
            var terrain = BuildTerrain(layers);
            var envRoot = new GameObject("AetheriaEnvironment").transform;

            BuildWater(envRoot);
            BuildLandmarks(envRoot, terrain);
            var stats = BuildVegetation(envRoot, terrain);
            WireWorldServices(terrain);

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, ScenePath);
            Debug.Log($"[WorldRebuild] Terrain {AetheriaWorld.WorldSize:0} m, {stats.Instances} vegetation instances " +
                      $"in {stats.Groups} groups, {stats.Boulders} boulders, {stats.Trees} trees. Scene saved.");
        }

        // ------------------------------------------------------------------ terrain

        private static TerrainLayer[] BuildLayers()
        {
            string[] paths =
            {
                LayerFolder + "/TL_Grass.terrainlayer",
                LayerFolder + "/TL_ForestFloor.terrainlayer",
                LayerFolder + "/TL_LayeredCliff.terrainlayer",
                LayerFolder + "/TL_GravelGlacier.terrainlayer",
                LayerFolder + "/TL_BurntAsh.terrainlayer",
                LayerFolder + "/TL_Snow.terrainlayer",
            };

            var grassDiff = LoadTex("Assets/_AAA_Imported/PolyHaven/Textures/leafy_grass/leafy_grass_diff.jpg");
            var grassNor = LoadTex("Assets/_AAA_Imported/PolyHaven/Textures/leafy_grass/leafy_grass_nor.jpg");
            var forestDiff = LoadTex("Assets/_AAA_Imported/PolyHaven/Textures/forest_ground_04/forest_ground_04_diff.jpg");
            var forestNor = LoadTex("Assets/_AAA_Imported/PolyHaven/Textures/forest_ground_04/forest_ground_04_nor.jpg");
            var snowDiff = LoadTex("Assets/_AAA_Imported/PolyHaven/Textures/snow_02/snow_02_diff.jpg");
            var snowNor = LoadTex("Assets/_AAA_Imported/PolyHaven/Textures/snow_02/snow_02_nor.jpg");

            // The rock / gravel / ash layers already exist from the project setup pass; reuse them.
            var cliff = AssetDatabase.LoadAssetAtPath<TerrainLayer>(paths[LayerRock]);
            var gravel = AssetDatabase.LoadAssetAtPath<TerrainLayer>(paths[LayerGravel]);
            var ash = AssetDatabase.LoadAssetAtPath<TerrainLayer>(paths[LayerAsh]);

            var layers = new TerrainLayer[LayerCount];
            layers[LayerGrass] = MakeLayer(paths[LayerGrass], grassDiff, grassNor, 6f, 0.22f);
            layers[LayerForest] = MakeLayer(paths[LayerForest], forestDiff, forestNor, 9f, 0.18f);
            layers[LayerRock] = cliff ?? MakeLayer(paths[LayerRock], null, null, 14f, 0.2f);
            layers[LayerGravel] = gravel ?? MakeLayer(paths[LayerGravel], null, null, 16f, 0.3f);
            layers[LayerAsh] = ash ?? MakeLayer(paths[LayerAsh], null, null, 12f, 0.24f);
            layers[LayerSnow] = MakeLayer(paths[LayerSnow], snowDiff, snowNor, 20f, 0.42f);
            return layers;
        }

        private static TerrainLayer MakeLayer(string path, Texture2D diffuse, Texture2D normal, float tile, float smoothness)
        {
            var layer = new TerrainLayer
            {
                diffuseTexture = diffuse,
                normalMapTexture = normal,
                normalScale = 1.2f,
                tileSize = new Vector2(tile, tile),
                smoothness = smoothness,
                metallic = 0.02f
            };
            System.IO.Directory.CreateDirectory(LayerFolder);
            if (AssetDatabase.LoadAssetAtPath<TerrainLayer>(path) != null) AssetDatabase.DeleteAsset(path);
            AssetDatabase.CreateAsset(layer, path);
            return AssetDatabase.LoadAssetAtPath<TerrainLayer>(path);
        }

        private static Texture2D LoadTex(string path)
        {
            var tex = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            if (tex == null) Debug.LogWarning($"[WorldRebuild] Missing texture {path}");
            return tex;
        }

        private static Terrain BuildTerrain(TerrainLayer[] layers)
        {
            var td = new TerrainData
            {
                heightmapResolution = HeightRes,
                alphamapResolution = AlphaRes,
                baseMapResolution = AlphaRes,
                size = new Vector3(AetheriaWorld.WorldSize, TerrainHeight, AetheriaWorld.WorldSize),
                terrainLayers = layers
            };

            var heights = new float[HeightRes, HeightRes];
            for (int y = 0; y < HeightRes; y++)
            {
                float wz = -AetheriaWorld.HalfExtent + (y / (float)(HeightRes - 1)) * AetheriaWorld.WorldSize;
                for (int x = 0; x < HeightRes; x++)
                {
                    float wx = -AetheriaWorld.HalfExtent + (x / (float)(HeightRes - 1)) * AetheriaWorld.WorldSize;
                    heights[y, x] = Mathf.Clamp01((AetheriaWorld.HeightAt(wx, wz) + TerrainBaseDepth) / TerrainHeight);
                }
            }
            td.SetHeights(0, 0, heights);

            // Splat map: blend the six layers by biome weight, slope and altitude.
            var alphas = new float[AlphaRes, AlphaRes, LayerCount];
            var weights = new float[AetheriaWorld.BiomeCount];
            for (int y = 0; y < AlphaRes; y++)
            {
                float wz = -AetheriaWorld.HalfExtent + ((y + 0.5f) / AlphaRes) * AetheriaWorld.WorldSize;
                for (int x = 0; x < AlphaRes; x++)
                {
                    float wx = -AetheriaWorld.HalfExtent + ((x + 0.5f) / AlphaRes) * AetheriaWorld.WorldSize;
                    float h = AetheriaWorld.HeightAt(wx, wz);

                    const float e = 1.6f;
                    float hx = AetheriaWorld.HeightAt(wx + e, wz) - AetheriaWorld.HeightAt(wx - e, wz);
                    float hz = AetheriaWorld.HeightAt(wx, wz + e) - AetheriaWorld.HeightAt(wx, wz - e);
                    float slope = 1f - Mathf.Clamp01(new Vector3(-hx, 2f * e, -hz).normalized.y);

                    AetheriaWorld.BiomeWeights(wx, wz, weights);

                    float forest = weights[(int)BiomeId.SapphireWoods] + weights[(int)BiomeId.GlimmercapGrove] * 0.8f
                                 + weights[(int)BiomeId.NocturneHollow] * 0.5f;
                    float ash = weights[(int)BiomeId.AshCaldera] + weights[(int)BiomeId.SunblightWastes] * 0.35f;
                    float snow = AetheriaWorld.SmoothStep(46f, 78f, h)
                               + weights[(int)BiomeId.CrystalGlacier] * AetheriaWorld.SmoothStep(24f, 46f, h);
                    float gravel = weights[(int)BiomeId.CrystalGlacier] * 0.5f
                                 + weights[(int)BiomeId.HighkeepCitadel] * 0.35f
                                 + weights[(int)BiomeId.DragonAerie] * 0.45f;

                    float rock = Mathf.Clamp01(AetheriaWorld.SmoothStep(0.30f, 0.62f, slope) * 1.35f);
                    float beach = AetheriaWorld.SmoothStep(3.4f, 0.6f, h) * AetheriaWorld.SmoothStep(-2.0f, -0.2f, h);

                    // Final per-layer weights first, then one normalisation over exactly those values:
                    // HDRP TerrainLit does not renormalise, so a splat that sums to less than 1 renders dark.
                    snow = Mathf.Clamp01(snow) * (1f - rock);
                    float gravelW = gravel * (1f - rock) + beach * 0.5f;
                    float ashW = ash * (1f - rock);
                    float forestFloor = forest * (1f - rock * 0.7f) * (1f - snow);
                    float grass = Mathf.Max(0f, 1f - forestFloor - ashW - rock - beach - snow - gravelW);

                    float sum = grass + forestFloor + rock + gravelW + ashW + snow;
                    if (sum < 1e-4f) { grass = 1f; sum = 1f; }
                    alphas[y, x, LayerGrass] = grass / sum;
                    alphas[y, x, LayerForest] = forestFloor / sum;
                    alphas[y, x, LayerRock] = rock / sum;
                    alphas[y, x, LayerGravel] = gravelW / sum;
                    alphas[y, x, LayerAsh] = ashW / sum;
                    alphas[y, x, LayerSnow] = snow / sum;
                }
            }
            td.SetAlphamaps(0, 0, alphas);

            const string tdPath = "Assets/Generated/AetheriaTerrainData.asset";
            if (AssetDatabase.LoadAssetAtPath<TerrainData>(tdPath) != null) AssetDatabase.DeleteAsset(tdPath);
            AssetDatabase.CreateAsset(td, tdPath);
            AssetDatabase.SaveAssets();

            var go = Terrain.CreateTerrainGameObject(AssetDatabase.LoadAssetAtPath<TerrainData>(tdPath));
            go.name = "AetheriaTerrain";
            go.transform.position = new Vector3(-AetheriaWorld.HalfExtent, -TerrainBaseDepth, -AetheriaWorld.HalfExtent);
            go.isStatic = true;

            var terrain = go.GetComponent<Terrain>();
            terrain.drawInstanced = true;
            terrain.heightmapPixelError = 4f;
            terrain.basemapDistance = 700f;
            terrain.shadowCastingMode = ShadowCastingMode.On;
            return terrain;
        }

        // ------------------------------------------------------------------ water

        private static void BuildWater(Transform parent)
        {
            var waterMat = AssetDatabase.LoadAssetAtPath<Material>(MaterialFolder + "/MirrorLakeWater.mat");
            var root = new GameObject("AetheriaWater").transform;
            root.SetParent(parent, false);

            // The lake basin is carved below sea level by AetheriaWorld, so one plane at y=0 fills it and
            // the marsh pools too. Sized to the basin, not the whole map.
            var lake = GameObject.CreatePrimitive(PrimitiveType.Cube);
            lake.name = "MirrorLakeSurface";
            lake.transform.SetParent(root, false);
            lake.transform.position = new Vector3(-130f, AetheriaWorld.SeaLevel + 0.05f, -60f);
            lake.transform.localScale = new Vector3(230f, 0.12f, 190f);
            if (waterMat != null) lake.GetComponent<Renderer>().sharedMaterial = waterMat;
            UnityEngine.Object.DestroyImmediate(lake.GetComponent<Collider>());

            var marsh = GameObject.CreatePrimitive(PrimitiveType.Cube);
            marsh.name = "MurkfenWater";
            marsh.transform.SetParent(root, false);
            marsh.transform.position = new Vector3(-380f, AetheriaWorld.SeaLevel + 0.04f, -300f);
            marsh.transform.localScale = new Vector3(150f, 0.1f, 130f);
            if (waterMat != null) marsh.GetComponent<Renderer>().sharedMaterial = waterMat;
            UnityEngine.Object.DestroyImmediate(marsh.GetComponent<Collider>());
        }

        // ------------------------------------------------------------------ landmarks

        private static void BuildLandmarks(Transform parent, Terrain terrain)
        {
            var root = new GameObject("Landmarks").transform;
            root.SetParent(parent, false);

            GameObject Place(string assetPath, Vector3 xz, float targetHeight, float yaw, string label, float yOffset = 0f)
            {
                var prefab = AssetDatabase.LoadAssetAtPath<GameObject>(assetPath);
                if (prefab == null) return null;
                var go = (GameObject)PrefabUtility.InstantiatePrefab(prefab);
                go.name = label;
                go.transform.SetParent(root, true);
                go.transform.rotation = Quaternion.Euler(0f, yaw, 0f) * go.transform.rotation;
                Ground(go, terrain, xz, targetHeight, yOffset);
                go.isStatic = true;
                return go;
            }

            // Oakhaven: the village square the player starts in.
            var village = AetheriaWorld.FindSite("OakhavenVillage");
            if (village != null)
            {
                Vector3 v = village.Position;
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_arch.glb", v + new Vector3(0f, 0f, 16f), 7.5f, 0f, "OakhavenArch");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_shrine_altar.glb", v + new Vector3(-9f, 0f, 6f), 3.4f, 35f, "OakhavenAltar");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_statue.glb", v + new Vector3(10f, 0f, 4f), 5.4f, 215f, "OakhavenStatue");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_brazier.glb", v + new Vector3(-4f, 0f, 2f), 2.0f, 0f, "OakhavenBrazierL");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_brazier.glb", v + new Vector3(4f, 0f, 2f), 2.0f, 0f, "OakhavenBrazierR");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", v + new Vector3(-11f, 0f, -4f), 6.4f, 0f, "OakhavenPillarL");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb", v + new Vector3(11f, 0f, -4f), 6.4f, 0f, "OakhavenPillarR");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_barrel_intact.glb", v + new Vector3(7f, 0f, 9f), 1.3f, 20f, "OakhavenBarrel");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_chest.glb", v + new Vector3(-7f, 0f, 9f), 1.2f, -35f, "OakhavenChest");
            }

            // Mirror Lake: the arched stone bridge over the water, at the lake's own coordinate.
            var lakeSite = AetheriaWorld.FindSite("MirrorLake");
            if (lakeSite != null)
            {
                var bridge = Place("Assets/_AAA_Imported/Environment/Gothic_props_1/Meshes/SM_Bridge1.fbx",
                    lakeSite.Position, 6.5f, 90f, "MirrorLakeStoneBridge");
                if (bridge != null)
                {
                    bridge.transform.localScale *= 1.8f;
                    bridge.transform.position = new Vector3(lakeSite.Position.x, AetheriaWorld.SeaLevel + 1.4f, lakeSite.Position.z);
                }
            }

            // Prologue cliff: boulders and the arch the hero wakes under.
            var cliff = AetheriaWorld.FindSite("PrologueCliff");
            if (cliff != null)
            {
                Vector3 c = cliff.Position;
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_boulder.glb", c + new Vector3(-5f, 0f, -3f), 5.0f, 35f, "PrologueBoulderL");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_boulder.glb", c + new Vector3(-3f, 0f, 4f), 4.6f, 140f, "PrologueBoulderR");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_arch.glb", c + new Vector3(6f, 0f, 0f), 6.0f, 90f, "PrologueArch");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_brazier.glb", c + new Vector3(-1f, 0f, 2f), 2.0f, 0f, "PrologueBrazier");
            }

            // Highkeep Citadel: the boss arena. Tower, curtain walls, gate.
            var citadel = AetheriaWorld.FindSite("HighkeepCitadel");
            if (citadel != null)
            {
                Vector3 c = citadel.Position;
                Place("Assets/_AAA_Imported/Environment/Gothic_props_1/Meshes/SM_Column2.fbx", c, 26f, 0f, "CitadelKeep");
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_iron_gate.glb", c + new Vector3(0f, 0f, -30f), 8.5f, 0f, "CitadelGate");
                for (int i = 0; i < 8; i++)
                {
                    float a = i / 8f * Mathf.PI * 2f;
                    Place("Assets/_AAA_Imported/Environment/Gothic_props_1/Meshes/SM_Section1.fbx",
                        c + new Vector3(Mathf.Cos(a) * 42f, 0f, Mathf.Sin(a) * 42f), 9f,
                        a * Mathf.Rad2Deg + 90f, "CitadelWall" + i);
                }
            }

            // Ash Caldera: the crypt mouth and its pillars.
            var caldera = AetheriaWorld.FindSite("EmberCrypt");
            if (caldera != null)
            {
                Vector3 c = caldera.Position;
                for (int i = 0; i < 6; i++)
                {
                    float a = i / 6f * Mathf.PI * 2f;
                    Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_pillar.glb",
                        c + new Vector3(Mathf.Cos(a) * 18f, 0f, Mathf.Sin(a) * 18f), 7f, a * Mathf.Rad2Deg, "CalderaPillar" + i);
                }
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_arch.glb", c + new Vector3(0f, 0f, 10f), 8f, 0f, "CryptArch");
            }

            // Dragon Aerie: a lone pillar crown on the crag.
            var aerie = AetheriaWorld.FindSite("DragonAerie");
            if (aerie != null)
                Place("Assets/_AAA_Imported/Environment/Gothic_props_1/Meshes/SM_Column2.fbx", aerie.Position, 18f, 24f, "AerieSpire");

            // Shrines: one altar stone at every shrine site, so the world reads as inhabited.
            foreach (var site in AetheriaWorld.Sites)
            {
                if (site.Kind != SiteKind.Shrine) continue;
                Place("Assets/_AAA_Imported/WizardRaw/GLBModels/prop_shrine_altar.glb", site.Position, 2.8f, 0f, "Shrine_" + site.Id);
            }
        }

        /// <summary>Scales a prop to a target height and drops it so its bounds rest on the terrain.</summary>
        private static void Ground(GameObject go, Terrain terrain, Vector3 xz, float targetHeight, float yOffset)
        {
            var rends = go.GetComponentsInChildren<Renderer>(true);
            if (rends.Length == 0) return;

            Bounds b = rends[0].bounds;
            for (int i = 1; i < rends.Length; i++) b.Encapsulate(rends[i].bounds);

            float scale = targetHeight / Mathf.Max(0.05f, b.size.y);
            go.transform.localScale = Vector3.one * scale;

            rends = go.GetComponentsInChildren<Renderer>(true);
            b = rends[0].bounds;
            for (int i = 1; i < rends.Length; i++) b.Encapsulate(rends[i].bounds);

            float groundY = terrain != null ? terrain.SampleHeight(new Vector3(xz.x, 0f, xz.z)) + terrain.transform.position.y : 0f;
            go.transform.position = new Vector3(xz.x, groundY - b.min.y + yOffset, xz.z);
        }

        // ------------------------------------------------------------------ vegetation

        private struct ScatterStats
        {
            public int Instances, Groups, Boulders, Trees;
        }

        /// <summary>
        /// Scatters the PolyHaven foliage and the Megascans rocks/trees across the world, grouped into
        /// per-chunk batches so HDRP can cull them. Density follows the biome weights the terrain uses,
        /// so grass grows in the valley, ferns in the woods, ash shards in the caldera and nothing on the
        /// snowfields — the same rules that paint the ground.
        /// </summary>
        private static ScatterStats BuildVegetation(Transform parent, Terrain terrain)
        {
            var stats = new ScatterStats();
            var root = new GameObject("Vegetation").transform;
            root.SetParent(parent, false);

            var grass1 = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/grass_medium_01/grass_medium_01_1k.gltf");
            var grass2 = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/grass_medium_02/grass_medium_02_1k.gltf");
            var fern = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/fern_02/fern_02_1k.gltf");
            var shrubs = new[]
            {
                AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/shrub_01/shrub_01_1k.gltf"),
                AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/shrub_02/shrub_02_1k.gltf"),
                AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/shrub_03/shrub_03_1k.gltf"),
                AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/shrub_04/shrub_04_1k.gltf"),
            };
            var rock1 = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/rock_moss_set_01/rock_moss_set_01_1k.gltf");
            var rock2 = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/PolyHaven/Models/rock_moss_set_02/rock_moss_set_02_1k.gltf");
            var deadTree = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/_AAA_Imported/Megascans/3D/DeadTrees/SM_Dead_Tree_qlEtl_01_N1.fbx");

            var grassMeshes = CollectMeshes(grass1);
            if (grassMeshes.Count == 0) grassMeshes = CollectMeshes(grass2);
            var fernMeshes = CollectMeshes(fern);
            var shrubMeshes = new List<VegPart>();
            foreach (var s in shrubs) shrubMeshes.AddRange(CollectMeshes(s));
            var rockMeshes = new List<VegPart>();
            rockMeshes.AddRange(CollectMeshes(rock1));
            rockMeshes.AddRange(CollectMeshes(rock2));
            var treeMeshes = CollectMeshes(deadTree);

            if (grassMeshes.Count == 0) Debug.LogWarning("[WorldRebuild] No grass meshes found - valley will be bare.");
            if (rockMeshes.Count == 0) Debug.LogWarning("[WorldRebuild] No rock meshes found.");

            // Fallbacks only. Each model keeps its own imported material (alpha-cut grass/fern/leaf cards,
            // scanned rock albedo); the old pass painted every blade with the tiling ground photo instead.
            var grassMat = MakeFoliageMaterial("Veg_Grass", "Assets/_AAA_Imported/PolyHaven/Models/grass_medium_01/textures/grass_medium_01_diff_1k.jpg",
                "Assets/_AAA_Imported/PolyHaven/Models/grass_medium_01/textures/grass_medium_01_nor_gl_1k.jpg", new Color(0.62f, 0.78f, 0.34f), true);
            var forestMat = MakeFoliageMaterial("Veg_Forest", "Assets/_AAA_Imported/PolyHaven/Models/fern_02/textures/fern_02_diff_1k.jpg",
                "Assets/_AAA_Imported/PolyHaven/Models/fern_02/textures/fern_02_nor_gl_1k.jpg", new Color(0.34f, 0.52f, 0.30f), true);
            var rockMat = AssetDatabase.LoadAssetAtPath<Material>(MaterialFolder + "/Boulder3D.mat");
            var treeMat = AssetDatabase.LoadAssetAtPath<Material>(MaterialFolder + "/DeadTree3D.mat");

            // One batch per 64 m cell AND per material: a combined mesh carries one material, and keying by
            // cell alone gave the rocks, ferns and grass of a cell whichever material happened to come last.
            var batches = new Dictionary<(long cell, Material mat), List<CombineInstance>>();
            var weights = new float[AetheriaWorld.BiomeCount];
            var rng = new System.Random(20261126);

            float GroundY(float x, float z) => terrain != null
                ? terrain.SampleHeight(new Vector3(x, 0f, z)) + terrain.transform.position.y
                : AetheriaWorld.HeightAt(x, z);

            void Emit(VegPart part, Material fallback, Vector3 pos, float yaw, float scale, float tilt, float sink, bool forceFallback = false)
            {
                if (part.Mesh == null) return;
                pos.y = GroundY(pos.x, pos.z) - sink * scale;     // the visible terrain, not the analytic field
                var rot = Quaternion.Euler(tilt * (float)(rng.NextDouble() - 0.5), yaw, tilt * (float)(rng.NextDouble() - 0.5));
                var trs = Matrix4x4.TRS(pos, rot, Vector3.one * scale) * part.Local;
                long key = ((long)Mathf.FloorToInt(pos.x / 64f) << 32) ^ (uint)Mathf.FloorToInt(pos.z / 64f);
                for (int sub = 0; sub < part.Mesh.subMeshCount; sub++)
                {
                    var own = part.Materials != null && sub < part.Materials.Length ? part.Materials[sub] : null;
                    var mat = !forceFallback && Usable(own) ? own : fallback;
                    if (mat == null) continue;
                    if (!batches.TryGetValue((key, mat), out var list)) batches[(key, mat)] = list = new List<CombineInstance>();
                    list.Add(new CombineInstance { mesh = part.Mesh, subMeshIndex = sub, transform = trs });
                }
                stats.Instances++;
            }

            // Dense pass over the map: a 3 m grid with a hashed jitter, so nothing lines up.
            const float step = 3.0f;
            for (float z = -AetheriaWorld.HalfExtent + step; z < AetheriaWorld.HalfExtent - step; z += step)
            {
                for (float x = -AetheriaWorld.HalfExtent + step; x < AetheriaWorld.HalfExtent - step; x += step)
                {
                    float h = AetheriaWorld.HeightAt(x, z);
                    if (h < AetheriaWorld.SeaLevel + 0.6f) continue;              // underwater
                    if (AetheriaWorld.InSiteClearing(x, z, -6f)) continue;        // keep landmarks readable
                    if (AetheriaWorld.RoadFactor(x, z) > 0.15f) continue;         // roads stay clear

                    const float e = 1.5f;
                    float hx = AetheriaWorld.HeightAt(x + e, z) - AetheriaWorld.HeightAt(x - e, z);
                    float hz = AetheriaWorld.HeightAt(x, z + e) - AetheriaWorld.HeightAt(x, z - e);
                    float flat = Mathf.Clamp01(new Vector3(-hx, 2f * e, -hz).normalized.y);

                    float r = (float)rng.NextDouble();
                    float jx = x + ((float)rng.NextDouble() - 0.5f) * step * 1.6f;
                    float jz = z + ((float)rng.NextDouble() - 0.5f) * step * 1.6f;
                    var pos = new Vector3(jx, 0f, jz);

                    if (flat < 0.62f)
                    {
                        // Cliffs and steep banks: no foliage, but scree and boulders half buried in the slope.
                        if (r < 0.06f && rockMeshes.Count > 0)
                        {
                            Emit(rockMeshes[rng.Next(rockMeshes.Count)], rockMat, pos, (float)rng.NextDouble() * 360f,
                                 0.8f + (float)rng.NextDouble() * 2.0f, 30f, 0.35f);
                            stats.Boulders++;
                        }
                        continue;
                    }

                    AetheriaWorld.BiomeWeights(x, z, weights);
                    float meadow = weights[(int)BiomeId.OakhavenValley] + weights[(int)BiomeId.MirrorLake]
                                 + weights[(int)BiomeId.Greenhollow] + weights[(int)BiomeId.DragonAerie] * 0.6f
                                 + weights[(int)BiomeId.HighkeepCitadel] * 0.5f;
                    float woods = weights[(int)BiomeId.SapphireWoods] + weights[(int)BiomeId.GlimmercapGrove];
                    float ash = weights[(int)BiomeId.AshCaldera] + weights[(int)BiomeId.SunblightWastes];
                    float snow = weights[(int)BiomeId.CrystalGlacier];

                    // Grass carpets the meadows; the forest gets ferns and shrubs instead.
                    if (meadow > 0.35f && r < 0.55f * meadow && grassMeshes.Count > 0)
                    {
                        Emit(grassMeshes[rng.Next(grassMeshes.Count)], grassMat, pos, (float)rng.NextDouble() * 360f,
                             0.75f + (float)rng.NextDouble() * 0.7f, 6f, 0.02f);
                        continue;
                    }
                    if (woods > 0.3f)
                    {
                        if (r < 0.22f * woods && fernMeshes.Count > 0)
                        {
                            Emit(fernMeshes[rng.Next(fernMeshes.Count)], forestMat, pos,
                                 (float)rng.NextDouble() * 360f, 0.9f + (float)rng.NextDouble() * 0.9f, 4f, 0.03f);
                            continue;
                        }
                        if (r < 0.30f * woods && shrubMeshes.Count > 0)
                        {
                            Emit(shrubMeshes[rng.Next(shrubMeshes.Count)], forestMat, pos,
                                 (float)rng.NextDouble() * 360f, 0.8f + (float)rng.NextDouble() * 1.0f, 3f, 0.05f);
                            continue;
                        }
                    }
                    // Shrubs and boulders everywhere else, thinning out on ash and snow.
                    float bare = Mathf.Clamp01(1f - ash * 0.85f - snow * 0.95f);
                    if (r < 0.045f * bare && shrubMeshes.Count > 0)
                    {
                        Emit(shrubMeshes[rng.Next(shrubMeshes.Count)], forestMat, pos,
                             (float)rng.NextDouble() * 360f, 0.7f + (float)rng.NextDouble() * 0.9f, 4f, 0.05f);
                        continue;
                    }
                    if (r < 0.075f && rockMeshes.Count > 0)
                    {
                        Emit(rockMeshes[rng.Next(rockMeshes.Count)], rockMat, pos,
                             (float)rng.NextDouble() * 360f, 0.6f + (float)rng.NextDouble() * 1.6f, 14f, 0.2f);
                        stats.Boulders++;
                    }
                }
            }

            // Dead trees: sparse silhouettes on the steppe, the wastes and the high moor.
            for (float z = -AetheriaWorld.HalfExtent + 8f; z < AetheriaWorld.HalfExtent - 8f; z += 22f)
            {
                for (float x = -AetheriaWorld.HalfExtent + 8f; x < AetheriaWorld.HalfExtent - 8f; x += 22f)
                {
                    float h = AetheriaWorld.HeightAt(x, z);
                    if (h < AetheriaWorld.SeaLevel + 1.5f) continue;
                    if (AetheriaWorld.InSiteClearing(x, z, -4f) || AetheriaWorld.RoadFactor(x, z) > 0.1f) continue;
                    AetheriaWorld.BiomeWeights(x, z, weights);
                    float dry = weights[(int)BiomeId.SunblightWastes] + weights[(int)BiomeId.NocturneHollow] * 0.7f
                              + weights[(int)BiomeId.HighkeepCitadel] * 0.4f + weights[(int)BiomeId.AshCaldera] * 0.5f;
                    if ((float)rng.NextDouble() > dry * 0.22f || treeMeshes.Count == 0 || treeMat == null) continue;
                    float jx = x + ((float)rng.NextDouble() - 0.5f) * 16f;
                    float jz = z + ((float)rng.NextDouble() - 0.5f) * 16f;
                    // The Megascans FBX slots are untextured; DeadTree3D carries the albedo and normal map.
                    Emit(treeMeshes[rng.Next(treeMeshes.Count)], treeMat, new Vector3(jx, 0f, jz),
                         (float)rng.NextDouble() * 360f, 0.8f + (float)rng.NextDouble() * 0.8f, 5f, 0.3f, true);
                    stats.Trees++;
                }
            }

            // Flush the batches into combined meshes, one renderer per (cell, material).
            var groups = 0;
            foreach (var kv in batches)
            {
                if (kv.Value.Count == 0) continue;
                var mesh = new Mesh { name = $"Veg_{kv.Key.cell}_{kv.Key.mat.name}", indexFormat = IndexFormat.UInt32 };
                mesh.CombineMeshes(kv.Value.ToArray(), true, true);
                mesh.RecalculateBounds();

                var go = new GameObject(mesh.name);
                go.transform.SetParent(root, false);
                go.AddComponent<MeshFilter>().sharedMesh = mesh;
                var mr = go.AddComponent<MeshRenderer>();
                mr.sharedMaterial = kv.Key.mat;
                mr.shadowCastingMode = ShadowCastingMode.On;
                mr.receiveShadows = true;
                go.isStatic = true;
                groups++;
            }

            stats.Groups = groups;
            return stats;
        }

        /// <summary>One mesh of a vegetation model, its offset from the model root and its own materials.</summary>
        private struct VegPart
        {
            public Mesh Mesh;
            public Matrix4x4 Local;
            public Material[] Materials;
        }

        private static List<VegPart> CollectMeshes(GameObject model)
        {
            var parts = new List<VegPart>();
            if (model == null) return parts;
            Matrix4x4 toRoot = model.transform.worldToLocalMatrix;
            foreach (var mf in model.GetComponentsInChildren<MeshFilter>(true))
            {
                if (mf.sharedMesh == null) continue;
                // Keep the node's rotation and scale (FBX Z-up roots, centimetre units) but drop its
                // translation: PolyHaven packs lay their variants side by side, and each one is planted alone.
                Matrix4x4 local = toRoot * mf.transform.localToWorldMatrix;
                local.SetColumn(3, new Vector4(0f, 0f, 0f, 1f));
                var mr = mf.GetComponent<MeshRenderer>();
                parts.Add(new VegPart { Mesh = mf.sharedMesh, Local = local, Materials = mr != null ? mr.sharedMaterials : null });
            }
            return parts;
        }

        private static bool Usable(Material m) =>
            m != null && m.shader != null && m.shader.isSupported && m.shader.name != "Hidden/InternalErrorShader";

        private static Material MakeFoliageMaterial(string name, string diffuse, string normal, Color tint, bool doubleSided)
        {
            var shader = Shader.Find("HDRP/Lit") ?? Shader.Find("Standard");
            var mat = new Material(shader) { name = name };

            var d = AssetDatabase.LoadAssetAtPath<Texture2D>(diffuse);
            if (d != null)
            {
                if (mat.HasProperty("_BaseColorMap")) mat.SetTexture("_BaseColorMap", d);
                if (mat.HasProperty("_BaseMap")) mat.SetTexture("_BaseMap", d);
                mat.mainTexture = d;
            }
            var n = AssetDatabase.LoadAssetAtPath<Texture2D>(normal);
            if (n != null)
            {
                if (mat.HasProperty("_NormalMap")) mat.SetTexture("_NormalMap", n);
                if (mat.HasProperty("_BumpMap")) mat.SetTexture("_BumpMap", n);
            }
            if (mat.HasProperty("_BaseColor")) mat.SetColor("_BaseColor", tint);
            mat.color = tint;
            if (mat.HasProperty("_Smoothness")) mat.SetFloat("_Smoothness", 0.18f);
            if (mat.HasProperty("_SmoothnessRemapMax")) mat.SetFloat("_SmoothnessRemapMax", 0.18f);
            if (mat.HasProperty("_Metallic")) mat.SetFloat("_Metallic", 0f);
            if (doubleSided)
            {
                // HDRP: double-sided + alpha clip is what foliage cards need.
                if (mat.HasProperty("_DoubleSidedEnable")) mat.SetFloat("_DoubleSidedEnable", 1f);
                if (mat.HasProperty("_DoubleSidedNormalMode")) mat.SetFloat("_DoubleSidedNormalMode", 0f);
                if (mat.HasProperty("_AlphaCutoffEnable")) mat.SetFloat("_AlphaCutoffEnable", 1f);
                if (mat.HasProperty("_AlphaCutoff")) mat.SetFloat("_AlphaCutoff", 0.45f);
                if (mat.HasProperty("_AlphaCutoffShadow")) mat.SetFloat("_AlphaCutoffShadow", 0.45f);
                if (mat.HasProperty("_UseShadowThreshold")) mat.SetFloat("_UseShadowThreshold", 1f);
            }
            // Let HDRP derive keywords and passes (_ALPHATEST_ON, _DOUBLESIDED_ON, cull mode) from the floats.
            AetheriaHdrpConvert.ValidateHdMaterial(mat);

            string path = MaterialFolder + "/" + name + ".mat";
            if (AssetDatabase.LoadAssetAtPath<Material>(path) != null) AssetDatabase.DeleteAsset(path);
            AssetDatabase.CreateAsset(mat, path);
            return AssetDatabase.LoadAssetAtPath<Material>(path);
        }

        // ------------------------------------------------------------------ services

        private static void WireWorldServices(Terrain terrain)
        {
            var root = GameObject.Find("AetheriaWorldServices");
            if (root == null) root = new GameObject("AetheriaWorldServices");

            var gen = root.GetComponent<AetheriaWorldGenerator>();
            if (gen == null) gen = root.AddComponent<AetheriaWorldGenerator>();

            var sun = GameObject.Find("AetheriaSunLight");
            if (sun != null) gen.SunLight = sun.GetComponent<Light>();

            var vol = GameObject.Find("GlobalVolume") ?? GameObject.Find("GlobalPostProcessVolume");
            if (vol != null) gen.GlobalVolume = vol.GetComponent<Volume>();

            var lake = GameObject.Find("MirrorLakeSurface");
            if (lake != null) gen.MirrorLakeSurface = lake.transform;

            // Put the hero where the quest system expects him and keep him on the real ground.
            var hero = GameObject.Find("HeroPlayer");
            if (hero != null)
            {
                var start = AetheriaWorld.FindSite("OakhavenVillage");
                Vector3 p = start != null ? start.Position : Vector3.zero;
                float y = terrain != null ? terrain.SampleHeight(new Vector3(p.x, 0f, p.z)) + terrain.transform.position.y : 0f;
                hero.transform.position = new Vector3(p.x, y + 0.2f, p.z);
            }
        }
    }
}
