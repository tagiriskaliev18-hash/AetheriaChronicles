using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace Aetheria.Editor
{
    /// <summary>
    /// Procedural ground flora with no dependency on card textures that lost their alpha in the 1k JPG
    /// downloads: grass clumps of real blade geometry, flowers, leafy bushes and butterfly wings. Colour comes
    /// from small generated gradient / atlas textures, because HDRP/Lit ignores vertex colours.
    /// </summary>
    public static class AetheriaProcFlora
    {
        public const string Folder = "Assets/Generated/Dressing";

        // ------------------------------------------------------------------ textures

        /// <summary>Vertical gradient: v=0 root colour, v=1 tip colour (blades and stems sample u=0.5).</summary>
        public static Texture2D Gradient(string name, Color root, Color mid, Color tip)
        {
            string path = $"{Folder}/{name}.png";
            var tex = new Texture2D(16, 128, TextureFormat.RGBA32, false);
            for (int y = 0; y < 128; y++)
            {
                float v = y / 127f;
                Color c = v < 0.5f ? Color.Lerp(root, mid, v * 2f) : Color.Lerp(mid, tip, (v - 0.5f) * 2f);
                for (int x = 0; x < 16; x++)
                {
                    float edge = 1f - Mathf.Abs(x / 15f - 0.5f) * 0.25f;   // a hint of a midrib
                    Color cc = c * edge; cc.a = 1f;
                    tex.SetPixel(x, y, cc);
                }
            }
            return Save(tex, path, false);
        }

        /// <summary>4 x 2 atlas: top row four petal colours (rounded petal with alpha), bottom row centre/stem/leaf.</summary>
        public static Texture2D FlowerAtlas()
        {
            string path = $"{Folder}/Flower_Atlas.png";
            const int cell = 128;
            var tex = new Texture2D(cell * 4, cell * 2, TextureFormat.RGBA32, true);
            Color[] petals =
            {
                new Color(0.98f, 0.97f, 0.92f),   // white daisy
                new Color(1f, 0.82f, 0.18f),      // buttercup
                new Color(0.62f, 0.42f, 0.95f),   // violet
                new Color(0.93f, 0.28f, 0.32f),   // poppy red
            };
            for (int i = 0; i < 4; i++)
            for (int y = 0; y < cell; y++)
            for (int x = 0; x < cell; x++)
            {
                // Petal: an ellipse from the bottom centre (the flower's middle) up to the tip.
                float u = (x + 0.5f) / cell - 0.5f, v = (y + 0.5f) / cell;
                float w = 0.42f * Mathf.Sin(Mathf.Clamp01(v) * Mathf.PI) + 0.02f;
                float a = Mathf.Clamp01((w - Mathf.Abs(u)) * 40f) * Mathf.Clamp01((0.98f - v) * 30f);
                Color c = petals[i] * Mathf.Lerp(0.78f, 1f, v);
                c.a = a;
                tex.SetPixel(i * cell + x, cell + y, c);
            }
            for (int y = 0; y < cell; y++)
            for (int x = 0; x < cell; x++)
            {
                float u = (x + 0.5f) / cell - 0.5f, v = (y + 0.5f) / cell - 0.5f;
                float r = Mathf.Sqrt(u * u + v * v);
                Color centre = Color.Lerp(new Color(1f, 0.75f, 0.1f), new Color(0.55f, 0.32f, 0.05f), r * 2.2f);
                centre.a = Mathf.Clamp01((0.48f - r) * 40f);
                tex.SetPixel(x, y, centre);                                                       // cell 0: centre disc
                tex.SetPixel(cell + x, y, Color.Lerp(new Color(0.18f, 0.32f, 0.08f), new Color(0.38f, 0.6f, 0.2f), v + 0.5f)); // cell 1: stem
                float leafW = 0.4f * Mathf.Sin(Mathf.Clamp01(v + 0.5f) * Mathf.PI);
                Color leaf = new Color(0.26f, 0.5f, 0.16f) * (0.8f + 0.2f * (v + 0.5f));
                leaf.a = Mathf.Clamp01((leafW - Mathf.Abs(u)) * 40f);
                tex.SetPixel(cell * 2 + x, y, leaf);                                               // cell 2: leaf
                tex.SetPixel(cell * 3 + x, y, new Color(0, 0, 0, 0));
            }
            return Save(tex, path, true);
        }

        /// <summary>2 x 2 atlas of butterfly wing patterns (each cell is one right wing; mirrored for the left).</summary>
        public static Texture2D WingAtlas()
        {
            string path = $"{Folder}/Butterfly_Wings.png";
            const int cell = 128;
            var tex = new Texture2D(cell * 2, cell * 2, TextureFormat.RGBA32, true);
            Color[] baseCol = { new Color(1f, 0.55f, 0.08f), new Color(0.2f, 0.55f, 1f), new Color(0.98f, 0.95f, 0.85f), new Color(0.95f, 0.85f, 0.15f) };
            for (int i = 0; i < 4; i++)
            {
                int ox = (i % 2) * cell, oy = (i / 2) * cell;
                for (int y = 0; y < cell; y++)
                for (int x = 0; x < cell; x++)
                {
                    float u = (x + 0.5f) / cell, v = (y + 0.5f) / cell - 0.5f;   // u: out from the body
                    // Fore wing (upper lobe) and hind wing (lower lobe).
                    float fore = Mathf.Pow((u - 0.45f) / 0.5f, 2f) + Mathf.Pow((v - 0.18f) / 0.32f, 2f);
                    float hind = Mathf.Pow((u - 0.32f) / 0.34f, 2f) + Mathf.Pow((v + 0.22f) / 0.26f, 2f);
                    float inside = Mathf.Min(fore, hind);
                    float a = Mathf.Clamp01((1f - inside) * 12f) * Mathf.Clamp01(u * 20f);
                    Color c = baseCol[i];
                    float rim = Mathf.Clamp01((inside - 0.72f) * 5f);
                    c = Color.Lerp(c, new Color(0.06f, 0.05f, 0.05f), rim);                       // dark margin
                    float spot = Mathf.Pow((u - 0.7f) / 0.08f, 2f) + Mathf.Pow((v - 0.25f) / 0.08f, 2f);
                    if (spot < 1f) c = Color.Lerp(c, Color.white, 0.85f);
                    c.a = a;
                    tex.SetPixel(ox + x, oy + y, c);
                }
            }
            return Save(tex, path, true);
        }

        private static Texture2D Save(Texture2D tex, string path, bool cutout)
        {
            Directory.CreateDirectory(Folder);
            tex.Apply();
            File.WriteAllBytes(path, tex.EncodeToPNG());
            Object.DestroyImmediate(tex);
            AssetDatabase.ImportAsset(path);
            var imp = (TextureImporter)AssetImporter.GetAtPath(path);
            imp.alphaIsTransparency = true;
            imp.wrapMode = TextureWrapMode.Clamp;
            imp.mipmapEnabled = true;
            imp.mipMapsPreserveCoverage = cutout;
            imp.alphaTestReferenceValue = 0.5f;
            imp.SaveAndReimport();
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }

        // ------------------------------------------------------------------ meshes

        private sealed class Builder
        {
            public readonly List<Vector3> V = new List<Vector3>();
            public readonly List<Vector3> N = new List<Vector3>();
            public readonly List<Vector2> UV = new List<Vector2>();
            public readonly List<int> T = new List<int>();

            public void Quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector2 uv0, Vector2 uv1, Vector3 n)
            {
                int i = V.Count;
                V.AddRange(new[] { a, b, c, d });
                N.AddRange(new[] { n, n, n, n });
                UV.AddRange(new[] { new Vector2(uv0.x, uv0.y), new Vector2(uv1.x, uv0.y), new Vector2(uv1.x, uv1.y), new Vector2(uv0.x, uv1.y) });
                T.AddRange(new[] { i, i + 2, i + 1, i, i + 3, i + 2 });
            }

            public Mesh Build(string name)
            {
                var m = new Mesh { name = name };
                if (V.Count > 65000) m.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
                m.SetVertices(V); m.SetNormals(N); m.SetUVs(0, UV); m.SetTriangles(T, 0);
                m.RecalculateBounds(); m.RecalculateTangents();
                return m;
            }
        }

        /// <summary>A clump of curved, tapering blades (real geometry, ~25-40 blades).</summary>
        public static Mesh GrassClump(int seed, float height = 0.55f, int blades = 26)
        {
            var rng = new System.Random(seed);
            var b = new Builder();
            for (int i = 0; i < blades; i++)
            {
                float a = (float)rng.NextDouble() * Mathf.PI * 2f;
                float r = (float)rng.NextDouble() * 0.22f;
                Vector3 root = new Vector3(Mathf.Cos(a) * r, 0f, Mathf.Sin(a) * r);
                float h = height * (0.6f + (float)rng.NextDouble() * 0.7f);
                float lean = 0.15f + (float)rng.NextDouble() * 0.45f;
                float face = (float)rng.NextDouble() * Mathf.PI * 2f;
                Vector3 bend = new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a)) * lean;
                Vector3 side = new Vector3(Mathf.Cos(face), 0f, Mathf.Sin(face)) * 0.022f;
                const int segs = 4;
                for (int s = 0; s < segs; s++)
                {
                    float t0 = s / (float)segs, t1 = (s + 1) / (float)segs;
                    Vector3 p0 = root + Vector3.up * h * t0 + bend * h * t0 * t0;
                    Vector3 p1 = root + Vector3.up * h * t1 + bend * h * t1 * t1;
                    float w0 = 1f - t0, w1 = 1f - t1 * 0.98f;
                    Vector3 n = Vector3.Cross(side, p1 - p0).normalized;
                    n = (n + Vector3.up * 0.8f).normalized;   // soft, sky-facing normals: no black backsides
                    b.Quad(p0 - side * w0, p0 + side * w0, p1 + side * w1, p1 - side * w1, new Vector2(0f, t0), new Vector2(1f, t1), n);
                }
            }
            return b.Build("GrassClump_" + seed);
        }

        /// <summary>A flower: stem, two leaves, a ring of petals and a centre; colour 0..3 picks the atlas cell.</summary>
        public static Mesh Flower(int seed, int colour)
        {
            var rng = new System.Random(seed);
            var b = new Builder();
            float h = 0.28f + (float)rng.NextDouble() * 0.22f;
            Vector2 stem0 = new Vector2(0.25f, 0f), stem1 = new Vector2(0.5f, 0.5f);
            Vector3 top = new Vector3(((float)rng.NextDouble() - 0.5f) * 0.06f, h, ((float)rng.NextDouble() - 0.5f) * 0.06f);
            for (int k = 0; k < 2; k++)
            {
                Vector3 s = k == 0 ? new Vector3(0.008f, 0, 0) : new Vector3(0, 0, 0.008f);
                b.Quad(-s, s, top + s, top - s, stem0, stem1, Vector3.up);
            }
            for (int l = 0; l < 2; l++)
            {
                float a = l * Mathf.PI + (float)rng.NextDouble();
                Vector3 dir = new Vector3(Mathf.Cos(a), 0.55f, Mathf.Sin(a)).normalized;
                Vector3 side = Vector3.Cross(dir, Vector3.up).normalized * 0.035f;
                Vector3 r0 = Vector3.up * h * 0.25f;
                b.Quad(r0 - side, r0 + side, r0 + dir * 0.14f + side, r0 + dir * 0.14f - side, new Vector2(0.5f, 0f), new Vector2(0.75f, 0.5f), Vector3.up);
            }
            int petals = colour == 2 ? 5 : 8;
            float pl = colour == 3 ? 0.07f : 0.055f;
            float u0 = colour * 0.25f, u1 = u0 + 0.25f;
            float cup = colour == 3 ? 0.55f : 0.18f;   // poppies cup upward, daisies lie flat
            for (int p = 0; p < petals; p++)
            {
                float a = p / (float)petals * Mathf.PI * 2f;
                Vector3 dir = new Vector3(Mathf.Cos(a), cup, Mathf.Sin(a)).normalized;
                Vector3 side = Vector3.Cross(dir, Vector3.up).normalized * pl * 0.55f;
                b.Quad(top - side, top + side, top + dir * pl * 2f + side, top + dir * pl * 2f - side,
                    new Vector2(u0, 0.5f), new Vector2(u1, 1f), Vector3.up);
            }
            float c = pl * 0.45f;
            Vector3 cu = top + Vector3.up * 0.004f;
            b.Quad(cu + new Vector3(-c, 0, -c), cu + new Vector3(c, 0, -c), cu + new Vector3(c, 0, c), cu + new Vector3(-c, 0, c),
                new Vector2(0f, 0f), new Vector2(0.25f, 0.5f), Vector3.up);
            return b.Build($"Flower_{colour}_{seed}");
        }

        /// <summary>A low leafy bush of crossed leaf cards around a dome (uses the broadleaf RGBA card texture).</summary>
        public static Mesh Bush(int seed, float radius = 0.8f)
        {
            var rng = new System.Random(seed);
            var b = new Builder();
            Vector3 centre = Vector3.up * radius * 0.55f;
            for (int i = 0; i < 26; i++)
            {
                float th = (float)rng.NextDouble() * Mathf.PI * 2f;
                float ph = Mathf.Acos(Mathf.Lerp(-0.1f, 1f, (float)rng.NextDouble()));
                Vector3 outDir = new Vector3(Mathf.Sin(ph) * Mathf.Cos(th), Mathf.Cos(ph), Mathf.Sin(ph) * Mathf.Sin(th));
                Vector3 p = centre + Vector3.Scale(outDir, new Vector3(radius, radius * 0.7f, radius)) * (0.5f + (float)rng.NextDouble() * 0.5f);
                for (int k = 0; k < 2; k++)
                {
                    Vector3 d = (outDir + new Vector3((float)rng.NextDouble() - 0.5f, (float)rng.NextDouble() - 0.5f, (float)rng.NextDouble() - 0.5f)).normalized;
                    Vector3 side = Vector3.Cross(d, k == 0 ? Vector3.up : Vector3.right).normalized * 0.32f;
                    Vector3 n = (p - centre).normalized * 0.7f + Vector3.up * 0.3f;
                    b.Quad(p - d * 0.35f - side, p - d * 0.35f + side, p + d * 0.35f + side, p + d * 0.35f - side, Vector2.zero, Vector2.one, n.normalized);
                }
            }
            return b.Build("Bush_" + seed);
        }

        /// <summary>One butterfly wing as a quad hinged on the local Z axis (the body); u runs out from the body.</summary>
        public static Mesh Wing(int pattern, bool left)
        {
            var b = new Builder();
            float u0 = (pattern % 2) * 0.5f, v0 = (pattern / 2) * 0.5f;
            float s = left ? -1f : 1f;
            Vector3 a = new Vector3(0, 0, -0.045f), bb = new Vector3(0.09f * s, 0, -0.045f), c = new Vector3(0.09f * s, 0, 0.045f), d = new Vector3(0, 0, 0.045f);
            if (left) b.Quad(bb, a, d, c, new Vector2(u0 + 0.5f, v0), new Vector2(u0, v0 + 0.5f), Vector3.up);
            else b.Quad(a, bb, c, d, new Vector2(u0, v0), new Vector2(u0 + 0.5f, v0 + 0.5f), Vector3.up);
            return b.Build($"Wing_{pattern}_{(left ? "L" : "R")}");
        }

        public static T SaveAsset<T>(T asset, string name) where T : Object
        {
            Directory.CreateDirectory(Folder + "/Meshes");
            string path = $"{Folder}/Meshes/{name}.asset";
            if (AssetDatabase.LoadAssetAtPath<T>(path) != null) AssetDatabase.DeleteAsset(path);
            AssetDatabase.CreateAsset(asset, path);
            return AssetDatabase.LoadAssetAtPath<T>(path);
        }
    }
}
