using System.Collections.Generic;
using Aetheria;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria.Editor
{
    /// <summary>
    /// Builds the stylized faceted terrain of Aetheria from <see cref="AetheriaWorld"/>: jittered grid,
    /// one flat color per facet, chunked for culling, with exact mesh colliders. Pure geometry: the caller
    /// supplies the render material so the same builder works with any render pipeline.
    /// </summary>
    public static class AetheriaTerrainBuilder
    {
        public const int ChunkCells = 64;                       // 256 m chunks
        public const int ChunksPerSide = AetheriaWorld.Cells / ChunkCells;
        public const float UVWorldMeters = 16f;

        private static Vector3[] _grid;                          // (N+1)^2 jittered world vertices
        private static int _n;

        /// <summary>Raised after the grid is built so other builders can query exact facet-mesh heights.</summary>
        public static Vector3 GridVertex(int i, int j) => _grid[j * (_n + 1) + i];
        public static int GridResolution => _n + 1;

        public static Transform Build(Transform parent, Material terrainMaterial)
        {
            BuildGrid();
            var root = new GameObject("AetheriaTerrain").transform;
            root.SetParent(parent, false);

            for (int cy = 0; cy < ChunksPerSide; cy++)
            {
                for (int cx = 0; cx < ChunksPerSide; cx++)
                {
                    var go = new GameObject($"Terrain_{cx}_{cy}");
                    go.transform.SetParent(root, false);
                    go.isStatic = true;
                    go.AddComponent<MeshFilter>().sharedMesh = BuildFacetedChunkMesh(cx, cy);
                    var mr = go.AddComponent<MeshRenderer>();
                    mr.sharedMaterial = terrainMaterial;
                    mr.shadowCastingMode = ShadowCastingMode.On;
                    mr.receiveShadows = true;
                    var col = go.AddComponent<MeshCollider>();
                    col.sharedMesh = BuildCollisionChunkMesh(cx, cy);
                    col.material = null;
                }
            }
            return root;
        }

        private static void BuildGrid()
        {
            int cells = AetheriaWorld.Cells;
            _n = cells;
            _grid = new Vector3[(cells + 1) * (cells + 1)];
            float half = AetheriaWorld.HalfExtent;
            float cell = AetheriaWorld.CellSize;
            float jitter = cell * 0.62f;

            for (int j = 0; j <= cells; j++)
            {
                for (int i = 0; i <= cells; i++)
                {
                    bool edge = i == 0 || j == 0 || i == cells || j == cells;
                    float x = -half + i * cell + (edge ? 0f : (AetheriaWorld.Hash(i, j, 1) - 0.5f) * jitter);
                    float z = -half + j * cell + (edge ? 0f : (AetheriaWorld.Hash(i, j, 2) - 0.5f) * jitter);
                    _grid[j * (cells + 1) + i] = new Vector3(x, AetheriaWorld.HeightAt(x, z), z);
                }
            }
        }

        /// <summary>Two clockwise (seen from above) triangles for one cell. Returns the split used (0 = 00-11, 1 = 10-01).</summary>
        private static int CellTriangles(int i, int j, out Vector3 a, out Vector3 b, out Vector3 c, out Vector3 d, out Vector3 e, out Vector3 f)
        {
            Vector3 p00 = GridVertex(i, j), p10 = GridVertex(i + 1, j), p01 = GridVertex(i, j + 1), p11 = GridVertex(i + 1, j + 1);
            int split = AetheriaWorld.Hash(i, j, 3) < 0.5f ? 0 : 1;
            if (split == 0) { a = p00; b = p01; c = p11; d = p00; e = p11; f = p10; }
            else            { a = p00; b = p01; c = p10; d = p10; e = p01; f = p11; }
            return split;
        }

        private static Mesh BuildFacetedChunkMesh(int cx, int cy)
        {
            int quads = ChunkCells * ChunkCells;
            var vertices = new Vector3[quads * 6];
            var normals = new Vector3[quads * 6];
            var colors = new Color[quads * 6];
            var uvs = new Vector2[quads * 6];
            int v = 0;

            for (int j = cy * ChunkCells; j < (cy + 1) * ChunkCells; j++)
            {
                for (int i = cx * ChunkCells; i < (cx + 1) * ChunkCells; i++)
                {
                    CellTriangles(i, j, out Vector3 a, out Vector3 b, out Vector3 c, out Vector3 d, out Vector3 e, out Vector3 f);

                    Emit(a, b, c, i, j, 4);
                    Emit(d, e, f, i, j, 5);

                    void Emit(Vector3 p0, Vector3 p1, Vector3 p2, int ci, int cj, int seed)
                    {
                        Vector3 n = Vector3.Cross(p1 - p0, p2 - p0);
                        float len = n.magnitude;
                        if (len < 1e-6f) n = Vector3.up; else n /= len;
                        Vector3 mid = (p0 + p1 + p2) / 3f;
                        Color col = AetheriaWorld.TerrainColor(mid.x, mid.z, mid.y, Mathf.Abs(n.y), AetheriaWorld.Hash(ci, cj, seed));
                        for (int k = 0; k < 3; k++)
                        {
                            Vector3 p = k == 0 ? p0 : k == 1 ? p1 : p2;
                            vertices[v] = p; normals[v] = n; colors[v] = col; uvs[v] = new Vector2(p.x, p.z) / UVWorldMeters;
                            v++;
                        }
                    }
                }
            }

            var mesh = new Mesh
            {
                name = $"TerrainFacets_{cx}_{cy}",
                indexFormat = v > 65535 ? UnityEngine.Rendering.IndexFormat.UInt32 : UnityEngine.Rendering.IndexFormat.UInt16
            };
            mesh.SetVertices(vertices);
            mesh.SetNormals(normals);
            mesh.SetColors(colors);
            mesh.SetUVs(0, uvs);
            var tris = new int[v];
            for (int t = 0; t < v; t++) tris[t] = t;
            mesh.SetTriangles(tris, 0);
            mesh.RecalculateBounds();
            return mesh;
        }

        private static Mesh BuildCollisionChunkMesh(int cx, int cy)
        {
            int s = ChunkCells + 1;
            var vertices = new Vector3[s * s];
            var tris = new List<int>(ChunkCells * ChunkCells * 6);

            for (int j = 0; j < s; j++)
                for (int i = 0; i < s; i++)
                    vertices[j * s + i] = GridVertex(cx * ChunkCells + i, cy * ChunkCells + j);

            int Idx(int i, int j) => j * s + i;
            for (int j = 0; j < ChunkCells; j++)
            {
                for (int i = 0; i < ChunkCells; i++)
                {
                    int gi = cx * ChunkCells + i, gj = cy * ChunkCells + j;
                    int split = AetheriaWorld.Hash(gi, gj, 3) < 0.5f ? 0 : 1;
                    int p00 = Idx(i, j), p10 = Idx(i + 1, j), p01 = Idx(i, j + 1), p11 = Idx(i + 1, j + 1);
                    if (split == 0) { tris.Add(p00); tris.Add(p01); tris.Add(p11); tris.Add(p00); tris.Add(p11); tris.Add(p10); }
                    else            { tris.Add(p00); tris.Add(p01); tris.Add(p10); tris.Add(p10); tris.Add(p01); tris.Add(p11); }
                }
            }

            var mesh = new Mesh
            {
                name = $"TerrainCollision_{cx}_{cy}",
                indexFormat = UnityEngine.Rendering.IndexFormat.UInt32
            };
            mesh.SetVertices(vertices);
            mesh.SetTriangles(tris, 0);
            mesh.RecalculateBounds();
            return mesh;
        }
    }
}
