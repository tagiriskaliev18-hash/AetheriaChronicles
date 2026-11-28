using System.Collections.Generic;
using UnityEngine;

namespace Aetheria.Editor
{
    /// <summary>
    /// Procedural medieval buildings in local space (origin at the centre of the ground floor, +Z = front door):
    /// stone plinth, one or two storeys of plaster with a timber frame, gable roof with overhang, door, windows
    /// with shutters and a chimney. Returned as one mesh with five submeshes:
    /// 0 stone, 1 plaster, 2 timber, 3 roof, 4 dark recesses (door/window openings).
    /// </summary>
    public static class AetheriaHouseKit
    {
        public const int Stone = 0, Plaster = 1, Timber = 2, Roof = 3, Dark = 4, Count = 5;

        public struct HouseSpec
        {
            public float Width, Depth, StoreyHeight, Plinth, Pitch, Overhang;
            public int Storeys;
            public bool StoneGround;    // ground storey in stone, upper in plaster
            public bool Chimney;
            public int Seed;
        }

        private sealed class Builder
        {
            public readonly List<Vector3> V = new List<Vector3>();
            public readonly List<Vector3> N = new List<Vector3>();
            public readonly List<Vector2> UV = new List<Vector2>();
            public readonly List<int>[] T = new List<int>[Count];
            public Builder() { for (int i = 0; i < Count; i++) T[i] = new List<int>(); }

            /// <summary>Quad a-b-c-d facing the side cross(b - a, d - a) points to; UVs in metres / tile.</summary>
            public void Quad(int sub, Vector3 a, Vector3 b, Vector3 c, Vector3 d, float tile = 2.5f)
            {
                Vector3 n = Vector3.Cross(b - a, d - a).normalized;
                Vector3 u = (b - a).normalized, w = Vector3.Cross(n, u);
                int i = V.Count;
                foreach (var p in new[] { a, b, c, d })
                {
                    V.Add(p); N.Add(n);
                    UV.Add(new Vector2(Vector3.Dot(p, u), Vector3.Dot(p, w)) / tile);
                }
                T[sub].AddRange(new[] { i, i + 1, i + 2, i, i + 2, i + 3 });
            }

            public void Tri(int sub, Vector3 a, Vector3 b, Vector3 c, float tile = 2.5f)
            {
                Vector3 n = Vector3.Cross(b - a, c - a).normalized;
                Vector3 u = (b - a).normalized, w = Vector3.Cross(n, u);
                int i = V.Count;
                foreach (var p in new[] { a, b, c })
                {
                    V.Add(p); N.Add(n);
                    UV.Add(new Vector2(Vector3.Dot(p, u), Vector3.Dot(p, w)) / tile);
                }
                T[sub].AddRange(new[] { i, i + 1, i + 2 });
            }

            /// <summary>Axis-aligned-ish box from two corners in a local frame (right, up, forward).</summary>
            public void Box(int sub, Vector3 centre, Vector3 right, Vector3 up, Vector3 fwd, float tile = 1.2f)
            {
                Vector3 c = centre;
                Vector3[] p =
                {
                    c - right - up - fwd, c + right - up - fwd, c + right + up - fwd, c - right + up - fwd,
                    c - right - up + fwd, c + right - up + fwd, c + right + up + fwd, c - right + up + fwd,
                };
                Quad(sub, p[1], p[0], p[3], p[2], tile);   // back (-fwd)
                Quad(sub, p[4], p[5], p[6], p[7], tile);   // front
                Quad(sub, p[0], p[4], p[7], p[3], tile);   // left
                Quad(sub, p[5], p[1], p[2], p[6], tile);   // right
                Quad(sub, p[3], p[7], p[6], p[2], tile);   // top
                Quad(sub, p[0], p[1], p[5], p[4], tile);   // bottom
            }

            /// <summary>A timber beam between two points with a square section.</summary>
            public void Beam(Vector3 a, Vector3 b, float thick, Vector3 outward)
            {
                Vector3 dir = b - a;
                Vector3 fwd = outward.normalized * thick * 0.5f;
                Vector3 side = Vector3.Cross(dir.normalized, outward).normalized * thick * 0.5f;
                Box(Timber, (a + b) * 0.5f, side, dir * 0.5f, fwd, 1f);
            }

            public Mesh Build(string name)
            {
                var m = new Mesh { name = name };
                m.SetVertices(V); m.SetNormals(N); m.SetUVs(0, UV);
                m.subMeshCount = Count;
                for (int i = 0; i < Count; i++) m.SetTriangles(T[i], i);
                m.RecalculateTangents(); m.RecalculateBounds();
                return m;
            }
        }

        public static HouseSpec RandomSpec(System.Random rng, bool large)
        {
            return new HouseSpec
            {
                Width = (large ? 7.5f : 5.5f) + (float)rng.NextDouble() * 2f,
                Depth = (large ? 6.5f : 5f) + (float)rng.NextDouble() * 1.5f,
                StoreyHeight = 2.9f + (float)rng.NextDouble() * 0.3f,
                Storeys = large || rng.NextDouble() < 0.45 ? 2 : 1,
                Plinth = 0.7f,
                Pitch = 42f + (float)rng.NextDouble() * 10f,
                Overhang = 0.55f,
                StoneGround = rng.NextDouble() < 0.6,
                Chimney = rng.NextDouble() < 0.7,
                Seed = rng.Next(),
            };
        }

        public static Mesh House(HouseSpec s, out float ridgeHeight)
        {
            var b = new Builder();
            var rng = new System.Random(s.Seed);
            float hw = s.Width * 0.5f, hd = s.Depth * 0.5f;
            float wallTop = s.Plinth + s.Storeys * s.StoreyHeight;

            // Plinth: a stone base that also runs 2.5 m into the ground for sloped lots.
            const float bury = 2.5f;
            b.Box(Stone, new Vector3(0f, (s.Plinth - bury) * 0.5f, 0f), Vector3.right * (hw + 0.15f), Vector3.up * (s.Plinth + bury) * 0.5f, Vector3.forward * (hd + 0.15f));

            // Walls per storey.
            for (int f = 0; f < s.Storeys; f++)
            {
                float y0 = s.Plinth + f * s.StoreyHeight, y1 = y0 + s.StoreyHeight;
                int sub = (f == 0 && s.StoneGround) ? Stone : Plaster;
                Walls(b, sub, hw, hd, y0, y1);
                if (sub == Plaster) Frame(b, hw, hd, y0, y1, rng);
                // A jettied upper storey: the floor beam sticks out a little.
                b.Box(Timber, new Vector3(0f, y0, 0f), Vector3.right * (hw + 0.12f), Vector3.up * 0.1f, Vector3.forward * (hd + 0.12f), 1f);
            }

            // Gables (front and back triangles) in plaster with a timber king post.
            float rise = Mathf.Tan(s.Pitch * Mathf.Deg2Rad) * hw;
            ridgeHeight = wallTop + rise;
            for (int side = -1; side <= 1; side += 2)
            {
                float z = hd * side;
                Vector3 l = new Vector3(-hw, wallTop, z), r = new Vector3(hw, wallTop, z), top = new Vector3(0f, wallTop + rise, z);
                if (side > 0) b.Tri(Plaster, l, r, top); else b.Tri(Plaster, r, l, top);
                Vector3 outward = Vector3.forward * side;
                b.Beam(new Vector3(0f, wallTop, z + side * 0.06f), top + outward * 0.06f, 0.18f, outward);
                b.Beam(new Vector3(-hw, wallTop, z + side * 0.06f), new Vector3(hw, wallTop, z + side * 0.06f), 0.2f, outward);
            }

            // Roof: two sloped planes with overhang and a little thickness at the edges.
            float o = s.Overhang;
            Vector3 eaveL0 = new Vector3(-hw - o, wallTop - o * Mathf.Tan(s.Pitch * Mathf.Deg2Rad), -hd - o);
            Vector3 eaveL1 = new Vector3(-hw - o, eaveL0.y, hd + o);
            Vector3 eaveR0 = new Vector3(hw + o, eaveL0.y, -hd - o);
            Vector3 eaveR1 = new Vector3(hw + o, eaveL0.y, hd + o);
            Vector3 ridge0 = new Vector3(0f, ridgeHeight + 0.05f, -hd - o), ridge1 = new Vector3(0f, ridgeHeight + 0.05f, hd + o);
            // Winding: a quad faces the side cross(b - a, d - a) points to.
            b.Quad(Roof, eaveL0, eaveL1, ridge1, ridge0, 2f);
            b.Quad(Roof, eaveR1, eaveR0, ridge0, ridge1, 2f);
            Vector3 down = Vector3.down * 0.18f;
            b.Quad(Roof, eaveL1 + down, eaveL0 + down, ridge0 + down, ridge1 + down, 2f);   // underside
            b.Quad(Roof, eaveR0 + down, eaveR1 + down, ridge1 + down, ridge0 + down, 2f);
            b.Quad(Timber, eaveL1 + down, eaveL1, eaveL0, eaveL0 + down, 1f);               // fascia boards
            b.Quad(Timber, eaveR0 + down, eaveR0, eaveR1, eaveR1 + down, 1f);
            b.Beam(ridge0 + Vector3.up * 0.04f, ridge1 + Vector3.up * 0.04f, 0.22f, Vector3.up);

            // Door in the front, windows on all sides.
            Door(b, new Vector3(((float)rng.NextDouble() - 0.5f) * (s.Width - 2.6f), s.Plinth, hd));
            for (int f = 0; f < s.Storeys; f++)
            {
                float y = s.Plinth + f * s.StoreyHeight + 1.15f;
                int count = Mathf.Max(1, Mathf.FloorToInt(s.Width / 2.6f));
                for (int i = 0; i < count; i++)
                {
                    float x = -hw + (i + 0.5f) * s.Width / count;
                    if (f == 0 && Mathf.Abs(x) < 1.3f) continue;
                    Window(b, new Vector3(x, y, hd), Vector3.forward, Vector3.right);
                    Window(b, new Vector3(-x, y, -hd), Vector3.back, Vector3.left);
                }
                Window(b, new Vector3(hw, y, 0f), Vector3.right, Vector3.back);
                Window(b, new Vector3(-hw, y, 0f), Vector3.left, Vector3.forward);
            }

            if (s.Chimney)
            {
                float cx = hw * 0.55f * (rng.NextDouble() < 0.5 ? -1f : 1f), cz = -hd * 0.4f;
                float slopeY = wallTop + (hw - Mathf.Abs(cx)) * Mathf.Tan(s.Pitch * Mathf.Deg2Rad);
                float h = ridgeHeight + 1.1f - slopeY;
                b.Box(Stone, new Vector3(cx, slopeY + h * 0.5f - 0.6f, cz), Vector3.right * 0.38f, Vector3.up * (h * 0.5f + 0.6f), Vector3.forward * 0.38f, 1f);
            }
            return b.Build("House_" + s.Seed);
        }

        private static void Walls(Builder b, int sub, float hw, float hd, float y0, float y1)
        {
            Vector3 a = new Vector3(-hw, y0, hd), bb = new Vector3(hw, y0, hd), c = new Vector3(hw, y0, -hd), d = new Vector3(-hw, y0, -hd);
            Vector3 up = Vector3.up * (y1 - y0);
            b.Quad(sub, a, bb, bb + up, a + up);     // front
            b.Quad(sub, bb, c, c + up, bb + up);     // right
            b.Quad(sub, c, d, d + up, c + up);       // back
            b.Quad(sub, d, a, a + up, d + up);       // left
        }

        private static void Frame(Builder b, float hw, float hd, float y0, float y1, System.Random rng)
        {
            const float t = 0.16f, off = 0.05f;
            var walls = new[]
            {
                (from: new Vector3(-hw, 0, hd), to: new Vector3(hw, 0, hd), outN: Vector3.forward),
                (from: new Vector3(hw, 0, hd), to: new Vector3(hw, 0, -hd), outN: Vector3.right),
                (from: new Vector3(hw, 0, -hd), to: new Vector3(-hw, 0, -hd), outN: Vector3.back),
                (from: new Vector3(-hw, 0, -hd), to: new Vector3(-hw, 0, hd), outN: Vector3.left),
            };
            foreach (var w in walls)
            {
                float len = Vector3.Distance(w.from, w.to);
                int bays = Mathf.Max(2, Mathf.RoundToInt(len / 1.5f));
                Vector3 o = w.outN * off;
                for (int i = 0; i <= bays; i++)
                {
                    Vector3 p = Vector3.Lerp(w.from, w.to, i / (float)bays) + o;
                    b.Beam(new Vector3(p.x, y0, p.z), new Vector3(p.x, y1, p.z), t, w.outN);
                }
                Vector3 s0 = w.from + o, s1 = w.to + o;
                b.Beam(new Vector3(s0.x, y1 - 0.08f, s0.z), new Vector3(s1.x, y1 - 0.08f, s1.z), t, w.outN);
                b.Beam(new Vector3(s0.x, y0 + 1.0f, s0.z), new Vector3(s1.x, y0 + 1.0f, s1.z), t * 0.8f, w.outN);
                // Diagonal braces in the end bays.
                for (int e = 0; e < 2; e++)
                {
                    if (rng.NextDouble() < 0.25) continue;
                    float i0 = e == 0 ? 0 : bays - 1, i1 = i0 + 1;
                    Vector3 p0 = Vector3.Lerp(w.from, w.to, i0 / bays) + o, p1 = Vector3.Lerp(w.from, w.to, i1 / bays) + o;
                    if (e == 1) { var tmp = p0; p0 = p1; p1 = tmp; }
                    b.Beam(new Vector3(p0.x, y0 + 1.0f, p0.z), new Vector3(p1.x, y1 - 0.08f, p1.z), t * 0.8f, w.outN);
                }
            }
        }

        private static void Door(Builder b, Vector3 bottomCentre)
        {
            const float w = 0.65f, h = 2.15f;
            Vector3 o = Vector3.forward * 0.02f;
            Vector3 l = bottomCentre + Vector3.left * w + o, r = bottomCentre + Vector3.right * w + o;
            b.Quad(Dark, l, r, r + Vector3.up * h, l + Vector3.up * h);
            // Plank door leaf slightly recessed look: timber quad inset, frame posts and lintel.
            Vector3 inset = Vector3.forward * 0.035f;
            b.Quad(Timber, l + Vector3.right * 0.06f + inset, r + Vector3.left * 0.06f + inset, r + Vector3.left * 0.06f + Vector3.up * (h - 0.08f) + inset, l + Vector3.right * 0.06f + Vector3.up * (h - 0.08f) + inset, 0.9f);
            b.Beam(l + Vector3.left * 0.08f, l + Vector3.left * 0.08f + Vector3.up * (h + 0.1f), 0.18f, Vector3.forward);
            b.Beam(r + Vector3.right * 0.08f, r + Vector3.right * 0.08f + Vector3.up * (h + 0.1f), 0.18f, Vector3.forward);
            b.Beam(l + Vector3.up * (h + 0.1f) + Vector3.left * 0.2f, r + Vector3.up * (h + 0.1f) + Vector3.right * 0.2f, 0.22f, Vector3.forward);
            // Step in front of the door.
            b.Box(Stone, bottomCentre + Vector3.forward * 0.35f + Vector3.down * 0.2f, Vector3.right * (w + 0.3f), Vector3.up * 0.2f, Vector3.forward * 0.35f, 1f);
        }

        private static void Window(Builder b, Vector3 centre, Vector3 outN, Vector3 right)
        {
            const float w = 0.45f, h = 0.6f;
            Vector3 o = outN * 0.03f, up = Vector3.up;
            Vector3 c = centre + o;
            b.Quad(Dark, c - right * w - up * h, c + right * w - up * h, c + right * w + up * h, c - right * w + up * h);
            b.Beam(c - up * (h + 0.06f) - right * (w + 0.15f), c - up * (h + 0.06f) + right * (w + 0.15f), 0.14f, outN);
            b.Beam(c + up * (h + 0.06f) - right * (w + 0.12f), c + up * (h + 0.06f) + right * (w + 0.12f), 0.12f, outN);
            b.Beam(c - right * (w + 0.06f) - up * h, c - right * (w + 0.06f) + up * h, 0.1f, outN);
            b.Beam(c + right * (w + 0.06f) - up * h, c + right * (w + 0.06f) + up * h, 0.1f, outN);
            b.Beam(c - up * h, c + up * h, 0.06f, outN);   // mullion
            // Open shutters on both sides.
            foreach (float s in new[] { -1f, 1f })
            {
                Vector3 hinge = c + right * s * (w + 0.12f) + outN * 0.02f;
                Vector3 leaf = (right * s * 0.8f + outN * 0.35f).normalized * w;
                b.Quad(Timber, hinge - up * h, hinge + leaf - up * h, hinge + leaf + up * h, hinge + up * h, 0.8f);
                b.Quad(Timber, hinge + leaf - up * h, hinge - up * h, hinge + up * h, hinge + leaf + up * h, 0.8f);
            }
        }

        // ------------------------------------------------------------------ small structures

        /// <summary>A market stall: four posts, a counter and a sloped canvas roof (submesh 3 = canvas).</summary>
        public static Mesh Stall(int seed)
        {
            var b = new Builder();
            float w = 1.6f, d = 1.1f, h = 2.3f;
            foreach (var x in new[] { -w, w })
            foreach (var z in new[] { -d, d })
                b.Beam(new Vector3(x, 0f, z), new Vector3(x, z > 0 ? h : h + 0.4f, z), 0.12f, Vector3.forward);
            b.Box(Timber, new Vector3(0f, 0.9f, d - 0.2f), Vector3.right * w, Vector3.up * 0.06f, Vector3.forward * 0.35f, 1f);
            b.Box(Timber, new Vector3(0f, 0.45f, d - 0.2f), Vector3.right * w, Vector3.up * 0.45f, Vector3.forward * 0.02f, 1f);
            Vector3 a = new Vector3(-w - 0.2f, h + 0.45f, -d - 0.25f), bb = new Vector3(w + 0.2f, h + 0.45f, -d - 0.25f);
            Vector3 c = new Vector3(w + 0.2f, h - 0.1f, d + 0.4f), e = new Vector3(-w - 0.2f, h - 0.1f, d + 0.4f);
            b.Quad(Roof, e, c, bb, a, 1.5f);
            b.Quad(Roof, a, bb, c, e, 1.5f);
            return b.Build("Stall_" + seed);
        }

        /// <summary>A stone well with a little roof and a crank beam.</summary>
        public static Mesh Well()
        {
            var b = new Builder();
            const int seg = 12;
            for (int i = 0; i < seg; i++)
            {
                float a0 = i / (float)seg * Mathf.PI * 2f, a1 = (i + 1) / (float)seg * Mathf.PI * 2f;
                Vector3 p0 = new Vector3(Mathf.Cos(a0), 0, Mathf.Sin(a0)), p1 = new Vector3(Mathf.Cos(a1), 0, Mathf.Sin(a1));
                b.Quad(Stone, p1 * 1.1f, p0 * 1.1f, p0 * 1.1f + Vector3.up * 0.9f, p1 * 1.1f + Vector3.up * 0.9f, 1f);
                b.Quad(Stone, p0 * 0.8f, p1 * 0.8f, p1 * 0.8f + Vector3.up * 0.9f, p0 * 0.8f + Vector3.up * 0.9f, 1f);
                b.Quad(Stone, p1 * 1.1f + Vector3.up * 0.9f, p0 * 1.1f + Vector3.up * 0.9f, p0 * 0.8f + Vector3.up * 0.9f, p1 * 0.8f + Vector3.up * 0.9f, 1f);
                b.Quad(Dark, p1 * 0.8f + Vector3.up * 0.3f, p0 * 0.8f + Vector3.up * 0.3f, Vector3.up * 0.3f, Vector3.up * 0.3f, 1f);
            }
            b.Beam(new Vector3(-1f, 0.9f, 0f), new Vector3(-1f, 2.6f, 0f), 0.14f, Vector3.left);
            b.Beam(new Vector3(1f, 0.9f, 0f), new Vector3(1f, 2.6f, 0f), 0.14f, Vector3.right);
            b.Beam(new Vector3(-1.1f, 2.1f, 0f), new Vector3(1.1f, 2.1f, 0f), 0.1f, Vector3.up);
            Vector3 r0 = new Vector3(-1.4f, 2.4f, -0.9f), r1 = new Vector3(1.4f, 2.4f, -0.9f), r2 = new Vector3(1.4f, 2.4f, 0.9f), r3 = new Vector3(-1.4f, 2.4f, 0.9f);
            Vector3 top0 = new Vector3(-1.4f, 3.1f, 0f), top1 = new Vector3(1.4f, 3.1f, 0f);
            b.Quad(Roof, r3, r2, top1, top0, 1.5f); b.Quad(Roof, r1, r0, top0, top1, 1.5f);
            b.Quad(Roof, r2, r3, top0, top1, 1.5f); b.Quad(Roof, r0, r1, top1, top0, 1.5f);
            return b.Build("Well");
        }

        /// <summary>A post-and-rail fence segment along +X of the given length.</summary>
        public static Mesh Fence(float length, int seed)
        {
            var b = new Builder();
            var rng = new System.Random(seed);
            int posts = Mathf.Max(2, Mathf.RoundToInt(length / 2f) + 1);
            for (int i = 0; i < posts; i++)
            {
                float x = length * i / (posts - 1);
                float lean = ((float)rng.NextDouble() - 0.5f) * 0.08f;
                b.Beam(new Vector3(x, -0.3f, 0f), new Vector3(x + lean, 1.15f, 0f), 0.13f, Vector3.forward);
            }
            b.Beam(new Vector3(0, 0.95f, 0.07f), new Vector3(length, 0.95f, 0.07f), 0.08f, Vector3.up);
            b.Beam(new Vector3(0, 0.5f, 0.07f), new Vector3(length, 0.52f, 0.07f), 0.08f, Vector3.up);
            return b.Build("Fence_" + seed);
        }

        /// <summary>A lamp post: timber post, arm and a hanging lantern frame (the light is added by the builder).</summary>
        public static Mesh LampPost()
        {
            var b = new Builder();
            b.Beam(new Vector3(0, -0.4f, 0), new Vector3(0, 3.1f, 0), 0.16f, Vector3.forward);
            b.Beam(new Vector3(0, 2.95f, 0), new Vector3(0.75f, 2.95f, 0), 0.1f, Vector3.up);
            b.Beam(new Vector3(0, 2.4f, 0), new Vector3(0.55f, 2.95f, 0), 0.07f, Vector3.forward);
            b.Box(Dark, new Vector3(0.75f, 2.62f, 0), Vector3.right * 0.13f, Vector3.up * 0.2f, Vector3.forward * 0.13f, 1f);
            return b.Build("LampPost");
        }
    }
}
