// Renders a 2D plan of Aetheria (height, biomes, roads, landmark sites) to Tools/Reports/world_plan.png
// so the continent layout can be judged before the 3D world is built. Read-only for the project.
// Run:  unity command script run --file Tools/Inspect/PreviewWorld.cs --entry AetheriaTools.PreviewWorld.Run --timeout 300 --args [512]
using System;
using System.IO;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using Aetheria;

namespace AetheriaTools
{
    public static class PreviewWorld
    {
        public static string Run(int size = 512)
        {
            size = Mathf.Clamp(size, 128, 1024);
            var tex = new Texture2D(size, size, TextureFormat.RGBA32, false);
            float half = AetheriaWorld.HalfExtent;
            var sb = new StringBuilder();
            var sw = System.Diagnostics.Stopwatch.StartNew();

            for (int py = 0; py < size; py++)
            {
                for (int px = 0; px < size; px++)
                {
                    float x = -half + (px + 0.5f) / size * 2f * half;
                    float z = -half + (py + 0.5f) / size * 2f * half;
                    tex.SetPixel(px, py, AetheriaWorld.MapColor(x, z));
                }
            }
            sw.Stop();
            sb.AppendLine($"MapColor {size}x{size}: {sw.ElapsedMilliseconds} ms ({sw.ElapsedMilliseconds / (float)(size * size) * 1000f:F2} us/px)");

            // Roads in a bright dashed tone
            foreach (var road in RoadsForPreview())
            {
                for (int i = 0; i < road.Length - 1; i++)
                {
                    Vector2 a = road[i], b = road[i + 1];
                    int steps = Mathf.CeilToInt(Vector2.Distance(a, b) / 2f);
                    for (int s = 0; s <= steps; s++)
                    {
                        Vector2 p = Vector2.Lerp(a, b, s / (float)steps);
                        int px = Mathf.RoundToInt((p.x + half) / (2f * half) * (size - 1));
                        int py = Mathf.RoundToInt((p.y + half) / (2f * half) * (size - 1));
                        if (px < 0 || py < 0 || px >= size || py >= size) continue;
                        tex.SetPixel(px, py, new Color(0.95f, 0.93f, 0.80f));
                    }
                }
            }

            // Sites: white cross + black outline
            foreach (var site in AetheriaWorld.Sites)
            {
                int cx = Mathf.RoundToInt((site.Position.x + half) / (2f * half) * (size - 1));
                int cy = Mathf.RoundToInt((site.Position.z + half) / (2f * half) * (size - 1));
                for (int d = -5; d <= 5; d++)
                {
                    Mark(tex, cx + d, cy, size, Color.black);
                    Mark(tex, cx, cy + d, size, Color.black);
                }
                for (int d = -4; d <= 4; d++)
                {
                    Mark(tex, cx + d, cy, size, site.Kind == SiteKind.Shrine ? new Color(0.2f, 1f, 0.9f) : Color.white);
                    Mark(tex, cx, cy + d, size, site.Kind == SiteKind.Shrine ? new Color(0.2f, 1f, 0.9f) : Color.white);
                }
                float h = AetheriaWorld.HeightAt(site.Position.x, site.Position.z);
                sb.AppendLine($"{site.Id,-18} ({site.Position.x,6:F0},{site.Position.z,6:F0}) h={h,6:F1}  biome={AetheriaWorld.BiomeAt(site.Position.x, site.Position.z)}");
            }

            // Height samples along the main axes & region stats
            sb.AppendLine("--- samples (x,z,h) ---");
            foreach (var p in new Vector2[] { new(0, 0), new(0, -640), new(0, 640), new(640, 0), new(-640, 0), new(-130, -60), new(-190, -60), new(-70, -60), new(0, 300), new(0, 360), new(0, 440), new(360, 300), new(360, 242), new(-360, 300), new(40, -480), new(-380, -300), new(240, -290), new(70, -338), new(270, 60), new(60, 230), new(-120, 520) })
                sb.AppendLine($"  ({p.x,5:F0},{p.y,5:F0}) h={AetheriaWorld.HeightAt(p.x, p.y),6:F1}");

            float minH = float.MaxValue, maxH = float.MinValue; int land = 0;
            for (int y = 0; y < 160; y++) for (int x = 0; x < 160; x++)
            {
                float wx = -half + (x + 0.5f) * 8f, wz = -half + (y + 0.5f) * 8f;
                float h = AetheriaWorld.HeightAt(wx, wz);
                minH = Mathf.Min(minH, h); maxH = Mathf.Max(maxH, h); if (h > 0f) land++;
            }
            sb.AppendLine($"range h=[{minH:F1}..{maxH:F1}]  land fraction of world square={land / 25600f:P0}");

            Directory.CreateDirectory("Tools/Reports");
            File.WriteAllBytes("Tools/Reports/world_plan.png", tex.EncodeToPNG());
            UnityEngine.Object.DestroyImmediate(tex);
            File.WriteAllText("Tools/Reports/world_plan.txt", sb.ToString());
            return sb.ToString();
        }

        private static void Mark(Texture2D t, int x, int y, int size, Color c)
        {
            if (x < 0 || y < 0 || x >= size || y >= size) return;
            t.SetPixel(x, y, c);
        }

        // Mirrors of the private road table (kept here only for the preview drawing)
        private static Vector2[][] RoadsForPreview() => new Vector2[][]
        {
            new[] { new Vector2(0f, 20f),   new Vector2(38f, 120f), new Vector2(60f, 230f), new Vector2(22f, 300f), new Vector2(0f, 352f), new Vector2(0f, 410f) },
            new[] { new Vector2(-8f, 14f),  new Vector2(-62f, -12f), new Vector2(-70f, -60f), new Vector2(-190f, -60f), new Vector2(-235f, -60f), new Vector2(-268f, 10f), new Vector2(-300f, 60f) },
            new[] { new Vector2(10f, 24f),  new Vector2(140f, 44f), new Vector2(270f, 60f), new Vector2(330f, 180f), new Vector2(360f, 250f) },
            new[] { new Vector2(6f, 10f),   new Vector2(22f, -90f), new Vector2(62f, -220f), new Vector2(70f, -340f), new Vector2(44f, -400f) },
            new[] { new Vector2(22f, -90f), new Vector2(-110f, -176f), new Vector2(-230f, -250f), new Vector2(-310f, -282f), new Vector2(-380f, -300f) },
            new[] { new Vector2(150f, 48f), new Vector2(228f, -140f), new Vector2(240f, -260f) },
        };
    }
}
