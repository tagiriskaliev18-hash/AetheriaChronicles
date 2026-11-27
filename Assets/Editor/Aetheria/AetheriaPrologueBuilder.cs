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
    /// Builds the stage for <see cref="AetheriaPrologueDirector"/> around the prologue plateau:
    /// a dark rock cave with daylight at its mouth, a ledge that collapses over the cliff, a waterfall down to
    /// Mirror Lake and a flowered forest glade on the far shore with butterflies. Re-runnable: it replaces
    /// its own root object. Run after "6. Dress World".
    /// </summary>
    public static class AetheriaPrologueBuilder
    {
        private const string ScenePath = "Assets/Scenes/AetheriaMain.unity";
        private const string Folder = AetheriaProcFlora.Folder;
        private const string PH = "Assets/_AAA_Imported/PolyHaven/";

        private static Terrain _terrain;

        private static float H(Vector3 p) => _terrain.SampleHeight(new Vector3(p.x, 0f, p.z)) + _terrain.transform.position.y;
        private static Vector3 Flat(Vector3 v) { v.y = 0f; return v; }

        [MenuItem("Aetheria/7. Build Prologue (cave, sage, waterfall, glade)")]
        public static void Build()
        {
            var scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);
            _terrain = Object.FindObjectsByType<Terrain>(FindObjectsSortMode.None).FirstOrDefault();
            if (_terrain == null) throw new System.InvalidOperationException("[Prologue] No terrain.");
            var old = GameObject.Find("AetheriaPrologue");
            if (old != null) Object.DestroyImmediate(old);
            AetheriaWorld.ReadSceneLandmarks();

            var tutor = GameObject.Find("Npc_CaveTutor");
            var lakeObj = GameObject.Find("MirrorLakeSurface");
            if (tutor == null || lakeObj == null) throw new System.InvalidOperationException("[Prologue] Needs Npc_CaveTutor and MirrorLakeSurface in the scene.");
            Vector3 c = Flat(tutor.transform.position);
            Vector3 lake = lakeObj.transform.position;
            float waterY = lake.y;
            Vector3 outDir = Flat(lake - c).normalized;
            Vector3 side = Vector3.Cross(Vector3.up, outDir);
            float top = H(c);

            var root = new GameObject("AetheriaPrologue").transform;
            var rock = RockMaterial();
            var director = root.gameObject.AddComponent<AetheriaPrologueDirector>();

            // ---- cave --------------------------------------------------------------------------
            Vector3 mouth = c - outDir * 5.5f;
            Bounds caveBounds;
            var cave = BuildCave(root, mouth, -outDir, top + 0.05f, rock, out caveBounds, out var path);
            Vector3 back = path[path.Count - 3];
            director.WakePoint = Marker(root, "Prologue_Wake", back + Vector3.up * 0.1f, Flat(path[path.Count - 8] - back));
            director.LightTrigger = Marker(root, "Prologue_LightTrigger", path[7] + Vector3.up * 0.1f, outDir);
            director.SagePoint = Marker(root, "Prologue_SagePoint", path[3] + Vector3.up * 0.05f, -outDir);
            director.CaveMouth = Marker(root, "Prologue_CaveMouth", mouth + Vector3.up * 0.05f, outDir);
            director.CaveBounds = caveBounds;
            LightCave(cave, path, mouth, outDir, caveBounds);
            CarveUnderCave(path, top + 0.05f);
            DressCaveRocks(cave.transform, path, mouth, outDir, side);

            // ---- ledge over the cliff ----------------------------------------------------------
            var ledge = new GameObject("Prologue_Ledge").transform;
            ledge.SetParent(root, false);
            BuildLedge(ledge, c, outDir, side, top, rock);
            director.LedgeRoot = ledge;
            director.LedgeTrigger = Marker(root, "Prologue_LedgeTrigger", c + outDir * 14.2f + Vector3.up * (top + 0.1f), outDir);

            // ---- fall path and waterfall -------------------------------------------------------
            var pts = new List<Vector3>
            {
                c + outDir * 14.2f + Vector3.up * (top + 0.4f),
                c + outDir * 17.5f + Vector3.up * (top - 2.5f),
            };
            Vector3 lip = c + outDir * 19f;
            for (float r = 19f; r < 90f; r += 3f)
            {
                Vector3 p = c + outDir * r;
                float h = H(p);
                if (h < waterY - 0.2f) { pts.Add(new Vector3(p.x, waterY - 1.2f, p.z)); break; }
                pts.Add(new Vector3(p.x, h + 0.45f, p.z));
            }
            var fall = new List<Transform>();
            for (int i = 0; i < pts.Count; i++) fall.Add(Marker(root, "Prologue_Fall_" + i, pts[i], outDir));
            director.FallPath = fall.ToArray();
            BuildWaterfall(root, pts.Skip(1).ToList(), side, top);

            // ---- the glade on the far shore ---------------------------------------------------
            Vector3 shore = FindGladeShore(c, lake, waterY);
            Vector3 toLake = Flat(lake - shore).normalized;
            Vector3 wake = shore - toLake * 1.6f;
            wake.y = H(wake);
            director.GladeWakePoint = Marker(root, "Prologue_GladeWake", wake, Quaternion.AngleAxis(70f, Vector3.up) * toLake);
            Vector3 gladeSide = Vector3.Cross(Vector3.up, toLake);
            Vector3 camPos = wake + gladeSide * 3.4f - toLake * 1.2f;
            camPos.y = H(camPos) + 1.7f;
            director.GladeCamera = Marker(root, "Prologue_GladeCamera", camPos, wake - camPos);
            Vector3 gladeCentre = wake - toLake * 5f;
            BuildGlade(root, gladeCentre, wake, toLake);   // root sits at the origin: glade meshes are in world space

            ClearTrees(caveBounds, c + outDir * 12f, pts, gladeCentre);

            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene);
            AssetDatabase.SaveAssets();
            Debug.Log($"[Prologue] Built: cave {caveBounds.size}, fall {pts.Count} points, glade at {gladeCentre}.");
        }

        private static Transform Marker(Transform parent, string name, Vector3 pos, Vector3 forward)
        {
            var t = new GameObject(name).transform;
            t.SetParent(parent, false);
            t.position = pos;
            forward.y = 0f;
            if (forward.sqrMagnitude > 1e-4f) t.rotation = Quaternion.LookRotation(forward.normalized, Vector3.up);
            return t;
        }

        // ------------------------------------------------------------------ materials

        private static Material Mat(string name, Shader shader)
        {
            string path = $"{Folder}/{name}.mat";
            var m = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (m == null) { m = new Material(shader); AssetDatabase.CreateAsset(m, path); }
            else m.shader = shader;
            return m;
        }

        /// <summary>HDRP/Lit with world-space triplanar projection: no UV stretching on organic rock shapes.</summary>
        private static Material Triplanar(string name, string texId, float worldScale, Color tint)
        {
            var m = Mat(name, Shader.Find("HDRP/Lit"));
            string t = "Assets/_AAA_Imported/PolyHaven/Textures/" + texId + "/" + texId;
            var nImp = AssetImporter.GetAtPath(t + "_nor_gl_2k.jpg") as TextureImporter;
            if (nImp != null && nImp.textureType != TextureImporterType.NormalMap) { nImp.textureType = TextureImporterType.NormalMap; nImp.SaveAndReimport(); }
            m.SetTexture("_BaseColorMap", AssetDatabase.LoadAssetAtPath<Texture2D>(t + "_diff_2k.jpg"));
            m.SetTexture("_NormalMap", AssetDatabase.LoadAssetAtPath<Texture2D>(t + "_nor_gl_2k.jpg"));
            m.SetFloat("_NormalScale", 1.3f);
            m.SetColor("_BaseColor", tint);
            m.SetFloat("_UVBase", 5f);              // Triplanar
            m.SetFloat("_TexWorldScale", worldScale);
            m.SetFloat("_Smoothness", 0.22f);
            m.SetFloat("_SmoothnessRemapMax", 0.3f);
            m.SetFloat("_Metallic", 0f);
            m.SetFloat("_DoubleSidedEnable", 1f);
            m.SetFloat("_DoubleSidedNormalMode", 0f);
            AetheriaHdrpConvert.ValidateHdMaterial(m);
            // Validation alone left the UV0 path active (streaks along the tunnel): force the triplanar variant.
            m.DisableKeyword("_MAPPING_PLANAR");
            m.EnableKeyword("_MAPPING_TRIPLANAR");
            AetheriaHdrpConvert.ValidateHdMaterial(m);
            EditorUtility.SetDirty(m);
            return m;
        }

        private static float Noise3(Vector3 p) =>
            (Mathf.PerlinNoise(p.x, p.y) + Mathf.PerlinNoise(p.y + 31.7f, p.z) + Mathf.PerlinNoise(p.z + 7.3f, p.x + 13.1f)) / 3f;

        private static float Ridged(Vector3 p)
        {
            float sum = 0f, amp = 0.6f, f = 1f;
            for (int o = 0; o < 3; o++) { sum += amp * (1f - Mathf.Abs(Noise3(p * f) * 2f - 1f)); amp *= 0.5f; f *= 2.1f; }
            return sum;
        }

        /// <summary>The walkable floor: a strip with gentle bumps, its own rocky-ground material.</summary>
        private static Mesh CaveFloor(List<Vector3> axis, List<float> halfWidth, Vector3 side, float floorY)
        {
            const int across = 12;
            var v = new List<Vector3>(); var tri = new List<int>(); var uv = new List<Vector2>();
            for (int r = 0; r < axis.Count; r++)
            {
                for (int i = 0; i <= across; i++)
                {
                    float u = i / (float)across * 2f - 1f;
                    Vector3 p = axis[r] + side * (u * halfWidth[r]);
                    float edge = Mathf.Pow(Mathf.Abs(u), 3f);
                    p.y = floorY + 0.02f + 0.05f * Noise3(p * 2.5f) + edge * 0.18f;
                    v.Add(p); uv.Add(new Vector2(u, r));
                }
            }
            for (int r = 0; r < axis.Count - 1; r++)
            for (int i = 0; i < across; i++)
            {
                int a = r * (across + 1) + i, b = a + across + 1;
                tri.AddRange(new[] { a, b, a + 1, a + 1, b, b + 1 });
            }
            var m = new Mesh { name = "Cave_Floor", indexFormat = IndexFormat.UInt32 };
            m.SetVertices(v); m.SetUVs(0, uv); m.SetTriangles(tri, 0);
            m.RecalculateNormals();
            var n = m.normals;
            for (int i = 0; i < n.Length; i++) if (n[i].y < 0f) n[i] = -n[i];   // floor faces up whatever the strip order
            m.normals = n;
            m.RecalculateTangents(); m.RecalculateBounds();
            return m;
        }

        private static Material RockMaterial()
        {
            return Triplanar("Cave_Rock", "cliff_side", 0.2f, new Color(0.78f, 0.76f, 0.74f));   // natural rock (rock_wall_08 read as masonry)
        }

        private static Material RockMaterialLegacy()
        {
            var m = Mat("Cave_Rock", Shader.Find("HDRP/Lit"));
            m.SetTexture("_BaseColorMap", AssetDatabase.LoadAssetAtPath<Texture2D>(PH + "Textures/mossy_rock/mossy_rock_diff.jpg"));
            m.SetTexture("_NormalMap", AssetDatabase.LoadAssetAtPath<Texture2D>(PH + "Textures/mossy_rock/mossy_rock_nor.jpg"));
            m.SetFloat("_NormalScale", 1.4f);
            m.SetColor("_BaseColor", new Color(0.78f, 0.76f, 0.72f));
            m.SetFloat("_Smoothness", 0.25f);
            m.SetFloat("_SmoothnessRemapMax", 0.3f);
            m.SetFloat("_Metallic", 0f);
            // Two-sided: the cave is seen from inside and the hill from outside, whatever the loft winding.
            m.SetFloat("_DoubleSidedEnable", 1f);
            m.SetFloat("_DoubleSidedNormalMode", 0f);
            AetheriaHdrpConvert.ValidateHdMaterial(m);
            EditorUtility.SetDirty(m);
            return m;
        }

        private static Material Emissive(string name, Color color, float nits)
        {
            var m = Mat(name, Shader.Find("HDRP/Lit"));
            m.SetColor("_BaseColor", color * 0.4f);
            m.SetFloat("_Smoothness", 0.85f);
            m.SetColor("_EmissiveColor", color * nits);
            m.SetColor("_EmissiveColorLDR", color);
            m.SetFloat("_EmissiveIntensity", nits);
            m.SetFloat("_UseEmissiveIntensity", 0f);
            m.EnableKeyword("_EMISSIVE_COLOR");
            AetheriaHdrpConvert.ValidateHdMaterial(m);
            EditorUtility.SetDirty(m);
            return m;
        }

        // ------------------------------------------------------------------ cave

        private static GameObject BuildCave(Transform parent, Vector3 mouth, Vector3 inward, float floorY, Material rock,
            out Bounds bounds, out List<Vector3> centreline)
        {
            // Dense rings with layered rock noise, so the walls read as broken stone rather than a smooth tube.
            const int rings = 112, k = 72;
            const float length = 27f, step = length / (rings - 1);
            Vector3 side = Vector3.Cross(Vector3.up, inward);
            var inner = new Vector3[rings, k];
            var outer = new Vector3[rings, k];
            var axisHi = new List<Vector3>();
            var halfWidth = new List<float>();
            centreline = new List<Vector3>();
            for (int r = 0; r < 30; r++)
            {
                float s = r * (length / 29f), t = s / length;
                Vector3 a = mouth + inward * s + side * Mathf.Sin(t * Mathf.PI) * 3.2f;
                a.y = floorY;
                centreline.Add(a);
            }
            for (int r = 0; r < rings; r++)
            {
                float s = r * step;
                float t = s / length;
                Vector3 axis = mouth + inward * s + side * Mathf.Sin(t * Mathf.PI) * 3.2f;
                axis.y = floorY;
                axisHi.Add(axis);
                float widen = 1f + 0.45f * Mathf.SmoothStep(0f, 1f, Mathf.InverseLerp(0.6f, 0.95f, t)) - 0.15f * Mathf.Exp(-Mathf.Pow((t - 0.4f) / 0.12f, 2f));
                float rw = 2.9f * widen, rh = 3.3f * widen;
                float centreY = floorY + 1.4f;
                halfWidth.Add(rw * Mathf.Sqrt(Mathf.Max(0.05f, 1f - Mathf.Pow(1.4f / rh, 2f))) * 0.95f);
                for (int i = 0; i < k; i++)
                {
                    float th = i / (float)k * Mathf.PI * 2f;
                    Vector3 dir = side * Mathf.Cos(th) * rw + Vector3.up * Mathf.Sin(th) * rh;
                    Vector3 basePt = axis + Vector3.up * (centreY - floorY) + dir;
                    float d = 0.7f * Ridged(basePt * 0.35f) + 0.38f * Ridged(basePt * 1.15f + Vector3.one * 3.3f) + 0.12f * Noise3(basePt * 3.5f) - 0.75f;
                    Vector3 p = basePt + dir.normalized * d;
                    if (p.y < floorY - 0.05f) p.y = floorY - 0.05f;
                    inner[r, i] = p;
                    float thick = 2.4f + 2.4f * Noise3(basePt * 0.18f + Vector3.one * 40f);
                    Vector3 q = basePt + dir.normalized * (thick + 1.3f * Ridged(basePt * 0.2f + Vector3.one * 7f));
                    float ground = H(q) - 1.2f;
                    if (Mathf.Sin(th) < 0.15f) q.y = Mathf.Min(q.y, ground);   // the lower hull is buried in the hill
                    outer[r, i] = q;
                }
            }

            var go = new GameObject("Prologue_Cave");
            go.transform.SetParent(parent, false);
            go.isStatic = true;
            var innerMesh = Loft(inner, rings, k, true, "Cave_Inner");
            var outerMesh = Loft(outer, rings, k, false, "Cave_Outer");
            var mouthMesh = MouthAndCaps(inner, outer, rings, k, axisHi, inward, "Cave_Caps");
            var floorMesh = CaveFloor(axisHi, halfWidth, side, floorY);
            var hill = Triplanar("Cave_Hill", "cliff_side", 0.12f, new Color(0.85f, 0.83f, 0.8f));
            var groundMat = Triplanar("Cave_Ground", "rocky_trail", 0.45f, new Color(0.75f, 0.72f, 0.68f));
            foreach (var (mesh, name, mat) in new[] { (innerMesh, "Inner", rock), (outerMesh, "Outer", hill), (mouthMesh, "Caps", hill), (floorMesh, "Floor", groundMat) })
            {
                var part = new GameObject(name);
                part.transform.SetParent(go.transform, false);
                part.isStatic = true;
                var saved = AetheriaProcFlora.SaveAsset(mesh, mesh.name);
                part.AddComponent<MeshFilter>().sharedMesh = saved;
                var mr = part.AddComponent<MeshRenderer>();
                mr.sharedMaterial = mat;
                mr.shadowCastingMode = ShadowCastingMode.TwoSided;
                part.AddComponent<MeshCollider>().sharedMesh = saved;
            }

            // A solid walkable floor independent of the loft winding: one thin box per tunnel segment.
            for (int r = 0; r < centreline.Count - 1; r++)
            {
                Vector3 a0 = centreline[r], a1 = centreline[r + 1];
                var seg = new GameObject("Cave_Floor_" + r);
                seg.transform.SetParent(go.transform, false);
                seg.transform.position = (a0 + a1) * 0.5f + Vector3.down * 0.25f;
                seg.transform.rotation = Quaternion.LookRotation(Flat(a1 - a0).normalized);
                var fb = seg.AddComponent<BoxCollider>();
                fb.size = new Vector3(4.4f, 0.5f, Vector3.Distance(a0, a1) + 0.6f);
            }

            bounds = new Bounds(centreline[0], Vector3.zero);
            for (int r = 0; r < rings; r++) for (int i = 0; i < k; i++) bounds.Encapsulate(inner[r, i]);
            return go;
        }

        /// <summary>Scanned rocks: cliff faces framing the mouth and boulders along the walls inside.</summary>
        private static void DressCaveRocks(Transform parent, List<Vector3> path, Vector3 mouth, Vector3 outDir, Vector3 side)
        {
            GameObject Rock(string id, Vector3 p, float size, float yaw)
            {
                var prefab = AssetDatabase.LoadAssetAtPath<GameObject>($"Assets/_AAA_Imported/PolyHaven/Models/{id}/{id}_1k.gltf");
                if (prefab == null) return null;
                var go = (GameObject)PrefabUtility.InstantiatePrefab(prefab);
                go.transform.SetParent(parent, true);
                go.transform.rotation = Quaternion.Euler(0f, yaw, 0f);
                var rends = go.GetComponentsInChildren<Renderer>();
                if (rends.Length == 0) return go;
                var b = rends[0].bounds; foreach (var r in rends) b.Encapsulate(r.bounds);
                float sc = size / Mathf.Max(0.1f, Mathf.Max(b.size.x, Mathf.Max(b.size.y, b.size.z)));
                go.transform.localScale = Vector3.one * sc;
                rends = go.GetComponentsInChildren<Renderer>();
                b = rends[0].bounds; foreach (var r in rends) b.Encapsulate(r.bounds);
                go.transform.position += p - new Vector3(b.center.x, b.min.y, b.center.z);
                go.isStatic = true;
                return go;
            }
            var rng = new System.Random(77);
            float yawOut = Quaternion.LookRotation(outDir).eulerAngles.y;
            Rock("namaqualand_cliff_01", mouth + side * 4.6f + outDir * 0.5f + Vector3.down * 0.6f, 7.5f, yawOut + 90f);
            Rock("namaqualand_cliff_01", mouth - side * 4.6f + outDir * 0.5f + Vector3.down * 0.6f, 7f, yawOut - 70f);
            Rock("rock_face_01", mouth + Vector3.up * 5.5f + outDir * 0.8f, 6f, yawOut);
            string[] small = { "boulder_01", "rock_07", "rock_09", "stone_01" };
            for (int i = 4; i < path.Count - 2; i += 3)
            {
                Vector3 dir = Vector3.Cross(Vector3.up, (path[i + 1] - path[i]).normalized);
                float sgn = rng.NextDouble() < 0.5 ? -1f : 1f;
                Rock(small[rng.Next(small.Length)], path[i] + dir * sgn * 2.2f + Vector3.down * 0.15f, 0.8f + (float)rng.NextDouble() * 1.1f, (float)rng.NextDouble() * 360f);
            }
        }

        /// <summary>Pushes the terrain below the cave floor along the tunnel so the hill never pokes through it.</summary>
        private static void CarveUnderCave(List<Vector3> path, float floorY)
        {
            var td = _terrain.terrainData;
            var o = _terrain.transform.position;
            int res = td.heightmapResolution;
            var heights = td.GetHeights(0, 0, res, res);
            float cell = td.size.x / (res - 1);
            float target = (floorY - 1.2f - o.y) / td.size.y;
            for (int i = 2; i < path.Count; i++)   // not at the mouth: the plateau outside stays level
            {
                var p = path[i];
                int cx = Mathf.RoundToInt((p.x - o.x) / cell), cz = Mathf.RoundToInt((p.z - o.z) / cell);
                int r = Mathf.CeilToInt(4.5f / cell);
                for (int z = cz - r; z <= cz + r; z++)
                for (int x = cx - r; x <= cx + r; x++)
                {
                    if (x < 0 || z < 0 || x >= res || z >= res) continue;
                    if (heights[z, x] > target) heights[z, x] = target;
                }
            }
            td.SetHeights(0, 0, heights);
        }

        private static Mesh Loft(Vector3[,] ring, int rings, int k, bool inward, string name)
        {
            var v = new List<Vector3>(); var uv = new List<Vector2>(); var tri = new List<int>();
            for (int r = 0; r < rings; r++)
            {
                float perim = 0f;
                for (int i = 0; i <= k; i++)
                {
                    if (i > 0) perim += Vector3.Distance(ring[r, (i - 1) % k], ring[r, i % k]);
                    v.Add(ring[r, i % k]);
                    uv.Add(new Vector2(perim / 4f, r * 0.93f / 4f * 3f));
                }
            }
            for (int r = 0; r < rings - 1; r++)
            for (int i = 0; i < k; i++)
            {
                int a = r * (k + 1) + i, b = a + k + 1;
                // Verified in a build: this order gives the inner tube inward-facing triangles (floor facing up).
                if (inward) tri.AddRange(new[] { a, b, a + 1, a + 1, b, b + 1 });
                else tri.AddRange(new[] { a, a + 1, b, a + 1, b + 1, b });
            }
            var m = new Mesh { name = name, indexFormat = IndexFormat.UInt32 };
            m.SetVertices(v); m.SetUVs(0, uv); m.SetTriangles(tri, 0);
            m.RecalculateNormals(); m.RecalculateTangents(); m.RecalculateBounds();
            return m;
        }

        private static Mesh MouthAndCaps(Vector3[,] inner, Vector3[,] outer, int rings, int k, List<Vector3> axis, Vector3 inward, string name)
        {
            var v = new List<Vector3>(); var tri = new List<int>(); var uv = new List<Vector2>();
            // Rock lip at the entrance: inner ring 0 to outer ring 0.
            for (int i = 0; i < k; i++)
            {
                int j = (i + 1) % k, b0 = v.Count;
                v.Add(inner[0, i]); v.Add(inner[0, j]); v.Add(outer[0, i]); v.Add(outer[0, j]);
                uv.Add(new Vector2(i * 0.4f, 0)); uv.Add(new Vector2(j * 0.4f, 0)); uv.Add(new Vector2(i * 0.4f, 0.8f)); uv.Add(new Vector2(j * 0.4f, 0.8f));
                tri.AddRange(new[] { b0, b0 + 2, b0 + 1, b0 + 1, b0 + 2, b0 + 3 });
            }
            // Back wall of the chamber (inner) and the back of the hill (outer), as fans.
            int last = rings - 1;
            Vector3 ic = axis[last] + Vector3.up * 1.4f + inward * 1.6f;
            Vector3 oc = axis[last] + Vector3.up * 1.4f + inward * 5f;
            int ci = v.Count; v.Add(ic); uv.Add(new Vector2(0.5f, 0.5f));
            for (int i = 0; i < k; i++) { v.Add(inner[last, i]); uv.Add(new Vector2(Mathf.Cos(i), Mathf.Sin(i))); }
            for (int i = 0; i < k; i++) tri.AddRange(new[] { ci, ci + 1 + (i + 1) % k, ci + 1 + i });
            int co = v.Count; v.Add(oc); uv.Add(new Vector2(0.5f, 0.5f));
            for (int i = 0; i < k; i++) { v.Add(outer[last, i]); uv.Add(new Vector2(Mathf.Cos(i) * 2f, Mathf.Sin(i) * 2f)); }
            for (int i = 0; i < k; i++) tri.AddRange(new[] { co, co + 1 + i, co + 1 + (i + 1) % k });
            var m = new Mesh { name = name };
            m.SetVertices(v); m.SetUVs(0, uv); m.SetTriangles(tri, 0);
            m.RecalculateNormals(); m.RecalculateTangents(); m.RecalculateBounds();
            return m;
        }

        private static void LightCave(GameObject cave, List<Vector3> path, Vector3 mouth, Vector3 outDir, Bounds bounds)
        {
            // Local exposure: inside, the eye adapts to the dark, so the mouth burns white like real daylight.
            string profPath = $"{Folder}/Cave_VolumeProfile.asset";
            var prof = AssetDatabase.LoadAssetAtPath<VolumeProfile>(profPath);
            if (prof == null) { prof = ScriptableObject.CreateInstance<VolumeProfile>(); AssetDatabase.CreateAsset(prof, profPath); }
            if (!prof.TryGet<Exposure>(out var exp)) exp = prof.Add<Exposure>(true);
            exp.mode.Override(ExposureMode.Fixed);
            exp.fixedExposure.Override(6.2f);
            if (!prof.TryGet<Fog>(out var fog)) fog = prof.Add<Fog>(true);
            fog.enabled.Override(true);
            fog.enableVolumetricFog.Override(true);
            fog.meanFreePath.Override(40f);
            fog.albedo.Override(new Color(0.75f, 0.8f, 0.9f));
            EditorUtility.SetDirty(prof);

            // Dark adaptation only in the deep part: near the mouth the normal exposure takes over, so the
            // sage and the way out are not blown out to white.
            var deep = new Bounds(path[9], Vector3.zero);
            for (int i = 9; i < path.Count; i++) deep.Encapsulate(path[i]);
            deep.Expand(new Vector3(7f, 9f, 7f));
            var volGo = new GameObject("Cave_LocalVolume");
            volGo.transform.SetParent(cave.transform, false);
            volGo.transform.position = deep.center + Vector3.up * 2f;
            var box = volGo.AddComponent<BoxCollider>();
            box.isTrigger = true;
            box.size = deep.size;
            var vol = volGo.AddComponent<Volume>();
            vol.isGlobal = false;
            vol.priority = 20f;
            vol.blendDistance = 6f;
            vol.sharedProfile = prof;

            Light MakeLight(string name, Vector3 pos, LightType type, Color col, float lumen, float range, Vector3 dir = default, float angle = 60f, bool shadows = false)
            {
                var go = new GameObject(name);
                go.transform.SetParent(cave.transform, false);
                go.transform.position = pos;
                if (dir != default) go.transform.rotation = Quaternion.LookRotation(dir);
                var l = go.AddComponent<Light>();
                var hd = go.AddComponent<HDAdditionalLightData>();
                l.type = type;
                l.color = col;
                l.lightUnit = LightUnit.Lumen;
                l.intensity = lumen;
                l.range = range;
                if (type == LightType.Spot) { l.spotAngle = angle; l.innerSpotAngle = angle * 0.4f; }
                l.shadows = shadows ? LightShadows.Soft : LightShadows.None;
                hd.affectsVolumetric = true;
                hd.volumetricDimmer = 2f;
                return l;
            }

            // Daylight pouring in through the mouth, with a volumetric shaft.
            MakeLight("Cave_Daylight", mouth + outDir * 1.5f + Vector3.up * 4f, LightType.Spot, new Color(1f, 0.92f, 0.78f), 26000f, 34f,
                (path[10] - (mouth + outDir * 1.5f + Vector3.up * 4f)).normalized, 70f, true);
            MakeLight("Cave_MouthFill", mouth + Vector3.up * 2f, LightType.Point, new Color(1f, 0.9f, 0.75f), 2500f, 12f);
            // A dim cold glow where he wakes, and crystals along the way.
            MakeLight("Cave_WakeGlow", path[path.Count - 4] + Vector3.up * 2.2f, LightType.Point, new Color(0.55f, 0.7f, 1f), 320f, 7f);
            var crystalMat = Emissive("Cave_Crystal", new Color(0.35f, 0.7f, 1f), 18f);
            var rng = new System.Random(5);
            for (int i = 8; i < path.Count - 3; i += 5)
            {
                Vector3 axis = path[i];
                Vector3 sideDir = Vector3.Cross(Vector3.up, (path[i + 1] - path[i]).normalized) * (rng.NextDouble() < 0.5 ? -1f : 1f);
                Vector3 p = axis + sideDir * 2.3f + Vector3.up * 0.1f;
                Crystal(cave.transform, p, crystalMat, rng);
                MakeLight("Cave_CrystalLight", p + Vector3.up * 0.8f - sideDir * 0.4f, LightType.Point, new Color(0.4f, 0.75f, 1f), 260f, 6f);
            }
        }

        private static void Crystal(Transform parent, Vector3 at, Material mat, System.Random rng)
        {
            var mesh = new Mesh { name = "Crystal" };
            var v = new List<Vector3>(); var t = new List<int>();
            for (int c = 0; c < 4; c++)
            {
                float a = (float)rng.NextDouble() * Mathf.PI * 2f, tilt = (float)rng.NextDouble() * 0.5f;
                Vector3 dir = new Vector3(Mathf.Cos(a) * tilt, 1f, Mathf.Sin(a) * tilt).normalized;
                float len = 0.4f + (float)rng.NextDouble() * 0.6f, rad = 0.07f + (float)rng.NextDouble() * 0.06f;
                Vector3 s1 = Vector3.Cross(dir, Vector3.right).normalized * rad, s2 = Vector3.Cross(dir, s1).normalized * rad;
                int b = v.Count;
                v.Add(Vector3.zero); v.Add(s1 + dir * len * 0.2f); v.Add(s2 + dir * len * 0.2f); v.Add(-s1 + dir * len * 0.2f); v.Add(-s2 + dir * len * 0.2f); v.Add(dir * len);
                int[] ring = { 1, 2, 3, 4 };
                for (int i = 0; i < 4; i++)
                {
                    int p = b + ring[i], q = b + ring[(i + 1) % 4];
                    t.AddRange(new[] { b, q, p, p, q, b + 5 });
                }
            }
            mesh.SetVertices(v); mesh.SetTriangles(t, 0); mesh.RecalculateNormals(); mesh.RecalculateBounds();
            var go = new GameObject("Cave_Crystal");
            go.transform.SetParent(parent, false);
            go.transform.position = at;
            go.AddComponent<MeshFilter>().sharedMesh = AetheriaProcFlora.SaveAsset(mesh, "Crystal_" + Mathf.Abs(at.GetHashCode()));
            go.AddComponent<MeshRenderer>().sharedMaterial = mat;
        }

        // ------------------------------------------------------------------ ledge

        private static void BuildLedge(Transform ledge, Vector3 c, Vector3 outDir, Vector3 side, float top, Material rock)
        {
            var rng = new System.Random(11);
            int idx = 0;
            for (int row = 0; row < 3; row++)
            for (int col = 0; col < 2; col++)
            {
                float r0 = 9.6f + row * 2.3f;
                Vector3 centre = c + outDir * (r0 + 1.15f) + side * (col - 0.5f) * 2.7f;
                centre.y = top - 0.7f;
                var mesh = Slab(rng, 2.3f, 1.4f, 2.7f);
                var go = new GameObject("LedgePiece_" + idx++);
                go.transform.SetParent(ledge, false);
                go.transform.position = centre;
                go.transform.rotation = Quaternion.LookRotation(outDir);
                go.AddComponent<MeshFilter>().sharedMesh = AetheriaProcFlora.SaveAsset(mesh, "LedgePiece_" + idx);
                go.AddComponent<MeshRenderer>().sharedMaterial = rock;
                var bc = go.AddComponent<BoxCollider>();
                bc.center = Vector3.zero;
                bc.size = new Vector3(2.7f, 1.4f, 2.3f);
            }
        }

        /// <summary>A rough stone block: box with jittered corners, top face slightly domed.</summary>
        private static Mesh Slab(System.Random rng, float lengthZ, float height, float widthX)
        {
            var corners = new Vector3[8];
            for (int i = 0; i < 8; i++)
            {
                float x = (i & 1) == 0 ? -0.5f : 0.5f, y = (i & 2) == 0 ? -0.5f : 0.5f, z = (i & 4) == 0 ? -0.5f : 0.5f;
                Vector3 j = new Vector3((float)rng.NextDouble() - 0.5f, (float)rng.NextDouble() - 0.5f, (float)rng.NextDouble() - 0.5f) * 0.18f;
                if (y > 0) j.y = Mathf.Abs(j.y) * 0.2f;
                corners[i] = Vector3.Scale(new Vector3(x, y, z) + j, new Vector3(widthX, height, lengthZ));
            }
            int[][] faces = { new[] { 0, 2, 3, 1 }, new[] { 4, 5, 7, 6 }, new[] { 0, 1, 5, 4 }, new[] { 2, 6, 7, 3 }, new[] { 0, 4, 6, 2 }, new[] { 1, 3, 7, 5 } };
            var v = new List<Vector3>(); var t = new List<int>(); var uv = new List<Vector2>();
            foreach (var f in faces)
            {
                int b = v.Count;
                foreach (int i in f) v.Add(corners[i]);
                uv.Add(new Vector2(0, 0)); uv.Add(new Vector2(0.6f, 0)); uv.Add(new Vector2(0.6f, 0.6f)); uv.Add(new Vector2(0, 0.6f));
                t.AddRange(new[] { b, b + 1, b + 2, b, b + 2, b + 3 });
            }
            var m = new Mesh { name = "Slab" };
            m.SetVertices(v); m.SetUVs(0, uv); m.SetTriangles(t, 0);
            m.RecalculateNormals(); m.RecalculateTangents(); m.RecalculateBounds();
            return m;
        }

        // ------------------------------------------------------------------ waterfall

        private static Texture2D WaterStreaks()
        {
            string path = $"{Folder}/Water_Streaks.png";
            var tex = new Texture2D(128, 256, TextureFormat.RGBA32, true);
            var rng = new System.Random(3);
            var cols = new float[128];
            for (int x = 0; x < 128; x++) cols[x] = (float)rng.NextDouble();
            for (int y = 0; y < 256; y++)
            for (int x = 0; x < 128; x++)
            {
                float streak = Mathf.PerlinNoise(x * 0.18f, y * 0.012f) * 0.7f + cols[x] * 0.3f;
                float foam = Mathf.PerlinNoise(x * 0.05f + 10f, y * 0.04f) > 0.62f ? 0.35f : 0f;
                float a = Mathf.Clamp01(0.35f + streak * 0.55f + foam);
                tex.SetPixel(x, y, new Color(0.82f + streak * 0.18f, 0.9f + streak * 0.1f, 1f, a));
            }
            tex.Apply();
            System.IO.File.WriteAllBytes(path, tex.EncodeToPNG());
            Object.DestroyImmediate(tex);
            AssetDatabase.ImportAsset(path);
            var imp = (TextureImporter)AssetImporter.GetAtPath(path);
            imp.alphaIsTransparency = true;
            imp.wrapMode = TextureWrapMode.Repeat;
            imp.SaveAndReimport();
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }

        private static void BuildWaterfall(Transform parent, List<Vector3> pts, Vector3 side, float top)
        {
            var mat = Mat("Waterfall_Water", Shader.Find("HDRP/Unlit"));
            mat.SetTexture("_UnlitColorMap", WaterStreaks());
            mat.SetColor("_UnlitColor", new Color(0.5f, 0.64f, 0.74f, 0.62f));
            mat.SetFloat("_SurfaceType", 1f);
            mat.SetFloat("_BlendMode", 0f);
            mat.SetFloat("_DoubleSidedEnable", 1f);
            mat.SetFloat("_EnableFogOnTransparent", 1f);
            AetheriaHdrpConvert.ValidateHdMaterial(mat);
            EditorUtility.SetDirty(mat);

            // The sheet starts as a spring under the ledge, then follows the cliff down to the lake.
            var path = new List<Vector3> { pts[0] + Vector3.up * 1.5f };
            path.AddRange(pts);
            var v = new List<Vector3>(); var uv = new List<Vector2>(); var t = new List<int>();
            float dist = 0f;
            for (int i = 0; i < path.Count; i++)
            {
                if (i > 0) dist += Vector3.Distance(path[i - 1], path[i]);
                float w = Mathf.Lerp(2.6f, 5.2f, i / (float)(path.Count - 1));
                v.Add(path[i] - side * w); v.Add(path[i] + side * w);
                uv.Add(new Vector2(0, dist / 6f)); uv.Add(new Vector2(1, dist / 6f));
                if (i > 0) { int b = (i - 1) * 2; t.AddRange(new[] { b, b + 2, b + 1, b + 1, b + 2, b + 3 }); }
            }
            var mesh = new Mesh { name = "Waterfall_Sheet" };
            mesh.SetVertices(v); mesh.SetUVs(0, uv); mesh.SetTriangles(t, 0);
            mesh.RecalculateNormals(); mesh.RecalculateBounds();
            var go = new GameObject("Prologue_Waterfall");
            go.transform.SetParent(parent, false);
            go.AddComponent<MeshFilter>().sharedMesh = AetheriaProcFlora.SaveAsset(mesh, "Waterfall_Sheet");
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterial = mat;
            mr.shadowCastingMode = ShadowCastingMode.Off;
            go.AddComponent<AetheriaUVScroller>().Speed = new Vector2(0f, 1.6f);
            var splash = new GameObject("Waterfall_Splash");
            splash.transform.SetParent(go.transform, false);
            splash.transform.position = pts[pts.Count - 1] + Vector3.up * 1.2f;
            splash.AddComponent<AetheriaSplashEmitter>();
        }

        // ------------------------------------------------------------------ glade

        private static Vector3 FindGladeShore(Vector3 plateau, Vector3 lake, float waterY)
        {
            Vector3 best = lake + Vector3.forward * 20f;
            float bestScore = float.MinValue;
            var oakhaven = AetheriaWorld.FindSite("OakhavenVillage");
            for (int a = 0; a < 72; a++)
            {
                Vector3 dir = Quaternion.AngleAxis(a * 5f, Vector3.up) * Vector3.forward;
                for (float r = 6f; r < 60f; r += 0.5f)
                {
                    Vector3 p = lake + dir * r;
                    if (H(p) < waterY + 0.35f) continue;
                    // First dry point along this ray: score it.
                    Vector3 inland = p + dir * 8f;
                    float slope = Mathf.Abs(H(inland) - H(p));
                    float road = 0f;
                    for (float d = 0f; d < 10f; d += 2f) road = Mathf.Max(road, RoadLike(p + dir * d));
                    float score = Vector3.Distance(Flat(p), Flat(plateau)) * 0.6f - slope * 6f - road * 80f;
                    if (oakhaven != null && Vector3.Distance(Flat(p), Flat(oakhaven.Position)) < 30f) score -= 100f;
                    if (score > bestScore) { bestScore = score; best = p; }
                    break;
                }
            }
            best.y = H(best);
            return best;
        }

        private static float RoadLike(Vector3 p)
        {
            float x = p.x, z = p.z;
            float strip = Mathf.Abs(x - 2f) < 6f && z > -112f && z < 56f ? 1f : 0f;
            float plaza = Vector2.Distance(new Vector2(x, z), new Vector2(0f, 42f)) < 27f ? 1f : 0f;
            return Mathf.Max(strip, plaza);
        }

        private static void BuildGlade(Transform parent, Vector3 centre, Vector3 wake, Vector3 toLake)
        {
            var glade = new GameObject("Prologue_Glade").transform;
            glade.SetParent(parent, false);
            glade.position = centre;
            var rng = new System.Random(2027);

            // Flowers: one combined mesh in four colours.
            var atlas = AetheriaProcFlora.FlowerAtlas();
            var flowerMat = Mat("Glade_Flowers", Shader.Find("HDRP/Lit"));
            flowerMat.SetTexture("_BaseColorMap", atlas);
            flowerMat.SetColor("_BaseColor", Color.white);
            flowerMat.SetFloat("_AlphaCutoffEnable", 1f);
            flowerMat.SetFloat("_AlphaCutoff", 0.5f);
            flowerMat.SetFloat("_DoubleSidedEnable", 1f);
            flowerMat.SetFloat("_DoubleSidedNormalMode", 2f);
            flowerMat.SetFloat("_Smoothness", 0.25f);
            AetheriaHdrpConvert.ValidateHdMaterial(flowerMat);
            EditorUtility.SetDirty(flowerMat);
            var variants = new List<Mesh>();
            for (int colour = 0; colour < 4; colour++)
            for (int s = 0; s < 3; s++) variants.Add(AetheriaProcFlora.Flower(500 + colour * 10 + s, colour));
            var combine = new List<CombineInstance>();
            for (int i = 0; i < 420; i++)
            {
                float a = (float)rng.NextDouble() * Mathf.PI * 2f;
                float r = Mathf.Sqrt((float)rng.NextDouble()) * 11f;
                Vector3 p = centre + new Vector3(Mathf.Cos(a) * r, 0f, Mathf.Sin(a) * r);
                if (Vector3.Distance(Flat(p), Flat(wake)) < 1.3f) continue;
                p.y = H(p) - 0.02f;
                // Flowers grow in drifts: each colour favours its own patch.
                int colour = Mathf.Clamp(Mathf.FloorToInt(Mathf.PerlinNoise(p.x * 0.15f, p.z * 0.15f) * 4.4f), 0, 3);
                var m = variants[colour * 3 + rng.Next(3)];
                float sc = 0.85f + (float)rng.NextDouble() * 0.6f;
                combine.Add(new CombineInstance { mesh = m, transform = Matrix4x4.TRS(p, Quaternion.Euler(0f, (float)rng.NextDouble() * 360f, 0f), Vector3.one * sc) });
            }
            var flowers = new Mesh { name = "Glade_Flowers", indexFormat = IndexFormat.UInt32 };
            flowers.CombineMeshes(combine.ToArray(), true, true);
            flowers.RecalculateBounds();
            var fgo = new GameObject("Glade_Flowers");
            fgo.transform.SetParent(parent, false);   // the prologue root sits at the origin: combined vertices are world space
            fgo.AddComponent<MeshFilter>().sharedMesh = AetheriaProcFlora.SaveAsset(flowers, "Glade_Flowers");
            fgo.AddComponent<MeshRenderer>().sharedMaterial = flowerMat;

            // Bushes around the edge of the clearing.
            var leaves = AssetDatabase.LoadAssetAtPath<Material>($"{Folder}/Leaves_Broadleaf.mat");
            var bushMeshes = new List<Mesh>();
            for (int i = 0; i < 3; i++) bushMeshes.Add(AetheriaProcFlora.SaveAsset(AetheriaProcFlora.Bush(600 + i, 0.8f + i * 0.2f), "GladeBush_" + i));
            for (int i = 0; i < 22; i++)
            {
                float a = (float)rng.NextDouble() * Mathf.PI * 2f;
                float r = 6f + (float)rng.NextDouble() * 7f;
                Vector3 p = centre + new Vector3(Mathf.Cos(a) * r, 0f, Mathf.Sin(a) * r);
                if (Vector3.Dot(Flat(p - wake).normalized, toLake) > 0.6f) continue;   // keep the view to the water open
                p.y = H(p) - 0.1f;
                var go = new GameObject("Glade_Bush");
                go.transform.SetParent(glade, true);
                go.transform.SetPositionAndRotation(p, Quaternion.Euler(0f, (float)rng.NextDouble() * 360f, 0f));
                go.transform.localScale = Vector3.one * (0.8f + (float)rng.NextDouble() * 0.6f);
                go.AddComponent<MeshFilter>().sharedMesh = bushMeshes[rng.Next(bushMeshes.Count)];
                go.AddComponent<MeshRenderer>().sharedMaterial = leaves;
            }

            // Trees framing the glade.
            var treePrefabs = new[] { "Tree_Broadleaf_0", "Tree_Broadleaf_1", "Tree_Broadleaf_2", "Tree_Pine_0", "Tree_Pine_1" }
                .Select(n => AssetDatabase.LoadAssetAtPath<GameObject>($"{Folder}/Prefabs/{n}.prefab")).Where(p => p != null).ToArray();
            for (int i = 0; i < 12 && treePrefabs.Length > 0; i++)
            {
                float a = i / 12f * Mathf.PI * 2f + (float)rng.NextDouble() * 0.4f;
                float r = 11f + (float)rng.NextDouble() * 6f;
                Vector3 p = centre + new Vector3(Mathf.Cos(a) * r, 0f, Mathf.Sin(a) * r);
                if (Vector3.Dot(Flat(p - wake).normalized, toLake) > 0.4f) continue;
                p.y = H(p) - 0.2f;
                var go = (GameObject)PrefabUtility.InstantiatePrefab(treePrefabs[rng.Next(treePrefabs.Length)]);
                go.transform.SetParent(glade, true);
                go.transform.SetPositionAndRotation(p, Quaternion.Euler(0f, (float)rng.NextDouble() * 360f, 0f));
                go.transform.localScale = Vector3.one * (0.9f + (float)rng.NextDouble() * 0.5f);
            }

            // Butterflies and pollen.
            var wingTex = AetheriaProcFlora.WingAtlas();
            var wingMat = Mat("Butterfly_Wings", Shader.Find("HDRP/Lit"));
            wingMat.SetTexture("_BaseColorMap", wingTex);
            wingMat.SetFloat("_AlphaCutoffEnable", 1f);
            wingMat.SetFloat("_AlphaCutoff", 0.5f);
            wingMat.SetFloat("_DoubleSidedEnable", 1f);
            wingMat.SetFloat("_DoubleSidedNormalMode", 0f);
            AetheriaHdrpConvert.ValidateHdMaterial(wingMat);
            EditorUtility.SetDirty(wingMat);
            var life = glade.gameObject.AddComponent<AetheriaGladeLife>();
            life.LeftWings = Enumerable.Range(0, 4).Select(i => AetheriaProcFlora.SaveAsset(AetheriaProcFlora.Wing(i, true), $"Wing_{i}_L")).ToArray();
            life.RightWings = Enumerable.Range(0, 4).Select(i => AetheriaProcFlora.SaveAsset(AetheriaProcFlora.Wing(i, false), $"Wing_{i}_R")).ToArray();
            life.WingMaterial = wingMat;
            life.Butterflies = 16;
            life.Radius = 9f;

            // Dense grass in the clearing.
            var td = _terrain.terrainData;
            int res = td.detailResolution;
            if (res > 0 && td.detailPrototypes.Length > 0)
            {
                var o = _terrain.transform.position;
                int cx = Mathf.RoundToInt((centre.x - o.x) / td.size.x * res), cz = Mathf.RoundToInt((centre.z - o.z) / td.size.z * res);
                int rad = Mathf.CeilToInt(13f / td.size.x * res);
                int x0 = Mathf.Clamp(cx - rad, 0, res - 1), z0 = Mathf.Clamp(cz - rad, 0, res - 1);
                int w = Mathf.Min(res - x0, rad * 2), h = Mathf.Min(res - z0, rad * 2);
                for (int layer = 0; layer < td.detailPrototypes.Length; layer++)
                {
                    var map = td.GetDetailLayer(x0, z0, w, h, layer);
                    bool grassLayer = td.detailPrototypes[layer].prototype != null && td.detailPrototypes[layer].prototype.name.Contains("Grass");
                    for (int z = 0; z < h; z++)
                    for (int x = 0; x < w; x++)
                    {
                        float dx = (x0 + x - cx), dz = (z0 + z - cz);
                        float d = Mathf.Sqrt(dx * dx + dz * dz) / rad;
                        if (d > 1f) continue;
                        map[z, x] = grassLayer ? Mathf.Max(map[z, x], d < 0.15f ? 1 : 3) : 0;
                    }
                    td.SetDetailLayer(x0, z0, layer, map);
                }
            }
        }

        // ------------------------------------------------------------------ tidy

        private static void ClearTrees(Bounds cave, Vector3 ledge, List<Vector3> fall, Vector3 glade)
        {
            var td = _terrain.terrainData;
            var o = _terrain.transform.position;
            var big = cave; big.Expand(new Vector3(14f, 40f, 14f));
            var keep = new List<TreeInstance>();
            foreach (var t in td.treeInstances)
            {
                Vector3 p = Vector3.Scale(t.position, td.size) + o;
                if (big.Contains(new Vector3(p.x, big.center.y, p.z))) continue;
                if (Vector3.Distance(Flat(p), Flat(glade)) < 9f) continue;
                if (Vector3.Distance(Flat(p), Flat(ledge)) < 12f) continue;
                bool nearFall = false;
                foreach (var f in fall) if (Vector3.Distance(Flat(p), Flat(f)) < 6f) { nearFall = true; break; }
                if (nearFall) continue;
                keep.Add(t);
            }
            td.SetTreeInstances(keep.ToArray(), true);
        }
    }
}
