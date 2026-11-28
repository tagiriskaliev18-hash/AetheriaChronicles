using System.IO;
using System.Linq;
using System.Reflection;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using Object = UnityEngine.Object;

namespace Aetheria.Editor
{
    /// <summary>
    /// Regenerates the authored valley heightfield (the one "1. Setup Project" sculpts:
    /// 4 km, 120 m, Oakhaven plaza, Mirror Lake, the 22 m prologue plateau) into the existing
    /// Assets/Generated/AetheriaTerrainData.asset, keeping its GUID so the scene's Terrain and
    /// TerrainCollider stay wired. Use it when that binary asset is missing or corrupted, then run
    /// "6. Dress World", "7. Build Prologue" and "8. Build Oakhaven Village" to repaint layers,
    /// grass and trees and to re-carve the cave.
    /// </summary>
    public static class AetheriaTerrainRepair
    {
        private const string ScenePath = "Assets/Scenes/AetheriaMain.unity";
        private const string DataPath = "Assets/Generated/AetheriaTerrainData.asset";
        private const string TempPath = "Assets/Generated/AetheriaTerrainData_regen.asset";

        // 4097 / 2049 / 1025 / 513 are valid. 2049 gives ~2 m per sample over the 4 km valley (the
        // original 513 gave ~7.8 m, which smeared the prologue cliff into a ramp).
        private const int HeightRes = 2049;
        private static readonly Vector3 Size = new Vector3(4000f, 120f, 4000f);

        [MenuItem("Aetheria/Repair/Regenerate Valley Terrain Heights")]
        public static void Regenerate()
        {
            var scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            var evaluate = typeof(AetheriaProjectSetup).GetMethod("EvaluateDesiredHeight", BindingFlags.NonPublic | BindingFlags.Static);
            if (evaluate == null) throw new System.MissingMethodException("AetheriaProjectSetup.EvaluateDesiredHeight not found.");
            var f = (System.Func<float, float, float>)System.Delegate.CreateDelegate(typeof(System.Func<float, float, float>), evaluate);

            var td = new TerrainData { heightmapResolution = HeightRes };
            td.size = Size;
            var heights = new float[HeightRes, HeightRes];
            float half = Size.x * 0.5f;
            for (int y = 0; y < HeightRes; y++)
            {
                float wz = -half + y / (float)(HeightRes - 1) * Size.z;
                for (int x = 0; x < HeightRes; x++)
                {
                    float wx = -half + x / (float)(HeightRes - 1) * Size.x;
                    heights[y, x] = Mathf.Clamp01(f(wx, wz) / Size.y);
                }
            }
            td.SetHeights(0, 0, heights);

            // Write through a temporary asset, then copy its bytes over the broken file so the original
            // .meta (and GUID) is kept.
            if (File.Exists(TempPath)) AssetDatabase.DeleteAsset(TempPath);
            AssetDatabase.CreateAsset(td, TempPath);
            AssetDatabase.SaveAssets();
            File.Copy(TempPath, DataPath, true);
            AssetDatabase.DeleteAsset(TempPath);
            AssetDatabase.ImportAsset(DataPath, ImportAssetOptions.ForceUpdate | ImportAssetOptions.ForceSynchronousImport);

            var loaded = AssetDatabase.LoadAssetAtPath<TerrainData>(DataPath);
            if (loaded == null) throw new System.InvalidOperationException("[TerrainRepair] Regenerated terrain data did not load.");
            var terrain = Object.FindObjectsByType<Terrain>(FindObjectsSortMode.None).FirstOrDefault();
            if (terrain == null) throw new System.InvalidOperationException("[TerrainRepair] No terrain in the scene.");
            terrain.terrainData = loaded;
            var col = terrain.GetComponent<TerrainCollider>();
            if (col != null) col.terrainData = loaded;
            terrain.transform.position = new Vector3(-half, 0f, -half);
            EditorUtility.SetDirty(terrain);
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene);
            Debug.Log($"[TerrainRepair] Valley heights regenerated at {HeightRes}^2, size {loaded.size}, " +
                      $"plateau {terrain.SampleHeight(new Vector3(-27f, 0f, -8f)):0.0} m, lake {terrain.SampleHeight(new Vector3(14f, 0f, -8f)):0.0} m.");
        }
    }
}
