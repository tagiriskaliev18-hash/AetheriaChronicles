using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria
{
    /// <summary>
    /// Procedural textures and meshes shared by every effect. Generated once at runtime, so effects
    /// never depend on hand-authored assets.
    /// </summary>
    public static class FxAssets
    {
        public static Texture2D Glow { get; private set; }
        public static Texture2D Spark { get; private set; }
        public static Texture2D Smoke { get; private set; }
        public static Texture2D Flame { get; private set; }
        public static Texture2D Star { get; private set; }
        public static Texture2D Noise { get; private set; }
        public static Mesh Ring { get; private set; }
        public static Mesh Disc { get; private set; }
        public static Mesh Beam { get; private set; }
        public static Mesh Sphere { get; private set; }
        public static Mesh Rock { get; private set; }
        public static Mesh Icosphere { get; private set; }

        public static bool Ready { get; private set; }

        public static void Ensure()
        {
            if (Ready) return;
            Ready = true;
            Glow = Radial(128, 1.15f, softEdge: 0.0f, core: 0.42f);
            Spark = Radial(64, 1.0f, softEdge: 0.35f, core: 0.0f);
            Smoke = Puff(128);
            Flame = FlameTexture(128);
            Star = StarTexture(128);
            Noise = NoiseTexture(256);
            Ring = Annulus(56, 0.80f);
            Disc = Annulus(56, 0.0f);
            Beam = Primitive(PrimitiveType.Cylinder);
            Sphere = Primitive(PrimitiveType.Sphere);
            Rock = Primitive(PrimitiveType.Cube);
            Icosphere = Primitive(PrimitiveType.Sphere);
        }

        /// <summary>Smooth ease-out curve from a to b (AnimationCurve.EaseOut does not exist).</summary>
        public static AnimationCurve Curve(float a, float b)
        {
            var c = new AnimationCurve(new Keyframe(0f, a, 0f, (b - a) * 2f), new Keyframe(1f, b, 0f, 0f));
            return c;
        }

        private static Mesh Primitive(PrimitiveType type)
        {
            var go = GameObject.CreatePrimitive(type);
            var mesh = go.GetComponent<MeshFilter>().sharedMesh;
            Object.Destroy(go);
            return mesh;
        }

        /// <summary>Lies in the XZ plane, outer radius 1.</summary>
        public static Mesh Annulus(int segments, float inner)
        {
            var mesh = new Mesh { name = "FxAnnulus" };
            var verts = new Vector3[segments * 2];
            var uv = new Vector2[segments * 2];
            var tris = new int[segments * 6];
            for (int i = 0; i < segments; i++)
            {
                float a = i / (float)segments * Mathf.PI * 2f;
                float c = Mathf.Cos(a), s = Mathf.Sin(a);
                verts[i * 2] = new Vector3(c, 0f, s);
                verts[i * 2 + 1] = new Vector3(c * inner, 0f, s * inner);
                uv[i * 2] = new Vector2(0.5f + c * 0.5f, 0.5f + s * 0.5f);
                uv[i * 2 + 1] = new Vector2(0.5f + c * 0.5f * inner, 0.5f + s * 0.5f * inner);
                int n = (i + 1) % segments;
                // clockwise seen from above
                tris[i * 6] = i * 2; tris[i * 6 + 1] = n * 2 + 1; tris[i * 6 + 2] = n * 2;
                tris[i * 6 + 3] = i * 2; tris[i * 6 + 4] = i * 2 + 1; tris[i * 6 + 5] = n * 2 + 1;
            }
            mesh.SetVertices(verts); mesh.SetUVs(0, uv); mesh.SetTriangles(tris, 0);
            mesh.RecalculateNormals(); mesh.RecalculateBounds();
            return mesh;
        }

        private static Texture2D Radial(int size, float power, float softEdge, float core)
        {
            var t = new Texture2D(size, size, TextureFormat.RGBA32, false) { name = "FxGlow", wrapMode = TextureWrapMode.Clamp };
            var px = new Color[size * size];
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                {
                    float u = (x + 0.5f) / size * 2f - 1f, v = (y + 0.5f) / size * 2f - 1f;
                    float d = Mathf.Sqrt(u * u + v * v);
                    float a = Mathf.Pow(Mathf.Clamp01(1f - d), power);
                    a = Mathf.Lerp(a, 1f, Mathf.Clamp01(1f - d / Mathf.Max(0.001f, core)));
                    a *= Mathf.SmoothStep(0f, 0.12f + softEdge * 0.2f, 1f - d);
                    px[y * size + x] = new Color(a, a, a, a);
                }
            t.SetPixels(px); t.Apply(false, true);
            return t;
        }

        private static Texture2D Puff(int size)
        {
            var t = new Texture2D(size, size, TextureFormat.RGBA32, false) { name = "FxSmoke", wrapMode = TextureWrapMode.Clamp };
            var px = new Color[size * size];
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                {
                    float u = (x + 0.5f) / size * 2f - 1f, v = (y + 0.5f) / size * 2f - 1f;
                    float d = Mathf.Sqrt(u * u + v * v);
                    float n = Mathf.PerlinNoise(x / (float)size * 4.2f + 3.1f, y / (float)size * 4.2f + 7.7f);
                    float a = Mathf.Clamp01(1f - d) * Mathf.Lerp(0.45f, 1f, n);
                    a = a * a * 1.25f;
                    px[y * size + x] = new Color(a, a, a, a);
                }
            t.SetPixels(px); t.Apply(false, true);
            return t;
        }

        private static Texture2D FlameTexture(int size)
        {
            var t = new Texture2D(size, size, TextureFormat.RGBA32, false) { name = "FxFlame", wrapMode = TextureWrapMode.Clamp };
            var px = new Color[size * size];
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                {
                    float u = (x + 0.5f) / size * 2f - 1f, v = (y + 0.5f) / size * 2f - 1f;
                    float n = Mathf.PerlinNoise(x / (float)size * 5.5f + 11.3f, y / (float)size * 5.5f + 2.9f);
                    // Teardrop: wide at the bottom, tapering upward, broken by noise
                    float w = Mathf.Lerp(1f, 0.25f, Mathf.Clamp01(v * 0.5f + 0.5f));
                    float d = Mathf.Abs(u) / w + (0.55f - n * 0.6f) * (v + 1f) * 0.5f;
                    float a = Mathf.Clamp01(1f - d) * Mathf.Clamp01(1f - Mathf.Abs(v));
                    a = a * a;
                    px[y * size + x] = new Color(a, a, a, a);
                }
            t.SetPixels(px); t.Apply(false, true);
            return t;
        }

        private static Texture2D StarTexture(int size)
        {
            var t = new Texture2D(size, size, TextureFormat.RGBA32, false) { name = "FxStar", wrapMode = TextureWrapMode.Clamp };
            var px = new Color[size * size];
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                {
                    float u = (x + 0.5f) / size * 2f - 1f, v = (y + 0.5f) / size * 2f - 1f;
                    float d = Mathf.Sqrt(u * u + v * v);
                    float cross = Mathf.Max(Mathf.Exp(-Mathf.Abs(u) * 26f) * Mathf.Exp(-Mathf.Abs(v) * 2.4f), Mathf.Exp(-Mathf.Abs(v) * 26f) * Mathf.Exp(-Mathf.Abs(u) * 2.4f));
                    float a = Mathf.Max(Mathf.Pow(Mathf.Clamp01(1f - d), 2f), cross * Mathf.Clamp01(1f - d));
                    px[y * size + x] = new Color(a, a, a, a);
                }
            t.SetPixels(px); t.Apply(false, true);
            return t;
        }

        /// <summary>Tileable RGBA value noise (each channel an independent octave pattern).</summary>
        public static Texture2D NoiseTexture(int size)
        {
            var t = new Texture2D(size, size, TextureFormat.RGBA32, false) { name = "FxNoise", wrapMode = TextureWrapMode.Repeat };
            t.name = "FxNoise";
            var px = new Color[size * size];
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                {
                    float u = x / (float)size, v = y / (float)size;
                    float r = Tileable(u, v, 4), g = Tileable(u, v, 8), b = Tileable(u, v, 16), a = Tileable(u, v, 32);
                    px[y * size + x] = new Color(r, g, b, a);
                }
            t.SetPixels(px); t.Apply(false, true);
            return t;
        }

        private static float Tileable(float u, float v, int cells)
        {
            float x = u * cells, y = v * cells;
            int x0 = Mathf.FloorToInt(x), y0 = Mathf.FloorToInt(y);
            float fx = x - x0, fy = y - y0;
            fx = fx * fx * (3f - 2f * fx);
            fy = fy * fy * (3f - 2f * fy);
            float A(int ix, int iy) => AetheriaWorld.Hash((ix % cells + cells) % cells, (iy % cells + cells) % cells, cells);
            float top = Mathf.Lerp(A(x0, y0), A(x0 + 1, y0), fx);
            float bot = Mathf.Lerp(A(x0, y0 + 1), A(x0 + 1, y0 + 1), fx);
            return Mathf.Lerp(top, bot, fy);
        }
    }

    /// <summary>
    /// Gameplay effect library: light flashes, sparks, shockwaves, beams, lightning, debris and camera shake.
    /// Everything is built from pooled lights and particle bursts, so there is nothing to clean up by hand.
    /// </summary>
    public class AetheriaFx : MonoBehaviour
    {
        private static AetheriaFx _instance;
        private static bool _quitting;

        public static AetheriaFx Instance
        {
            get
            {
                if (_instance == null && !_quitting)
                {
                    FxAssets.Ensure();
                    var go = new GameObject("AetheriaFx");
                    _instance = go.AddComponent<AetheriaFx>();
                }
                return _instance;
            }
        }

        private sealed class Flash
        {
            public Light Light;
            public float Age, Duration, Peak;
        }

        private readonly List<Flash> _pool = new List<Flash>();
        private readonly List<Flash> _active = new List<Flash>();
        private const int MaxLights = 18;

        private float _shakeAmp, _shakeTime, _shakeDur;
        private Vector3 _shakeOffset;
        private float _shakeRoll;
        public Vector3 ShakeOffset => _shakeOffset;
        public float ShakeRoll => _shakeRoll;

        private void OnApplicationQuit() => _quitting = true;
        private void OnDestroy() { if (_instance == this) _instance = null; }

        // ------------------------------------------------------------------ camera shake

        public static void Shake(float amplitude = 0.25f, float duration = 0.35f)
        {
            var fx = Instance;
            if (amplitude > fx._shakeAmp || fx._shakeTime <= 0f)
            {
                fx._shakeAmp = amplitude;
                fx._shakeDur = duration;
                fx._shakeTime = duration;
            }
        }

        private void Update()
        {
            float dt = Time.unscaledDeltaTime;

            for (int i = _active.Count - 1; i >= 0; i--)
            {
                var f = _active[i];
                f.Age += dt;
                float k = 1f - f.Age / f.Duration;
                if (k <= 0f)
                {
                    f.Light.enabled = false;
                    _active.RemoveAt(i);
                    continue;
                }
                f.Light.intensity = f.Peak * k * k;
            }

            if (_shakeTime > 0f)
            {
                _shakeTime -= dt;
                float k = Mathf.Clamp01(_shakeTime / Mathf.Max(0.01f, _shakeDur));
                float a = _shakeAmp * k * k;
                float t = Time.unscaledTime * 34f;
                _shakeOffset = new Vector3(Mathf.Sin(t * 1.13f) * a, Mathf.Cos(t * 1.71f) * a * 0.8f, 0f);
                _shakeRoll = Mathf.Sin(t * 0.97f) * a * 2.2f;
                if (_shakeTime <= 0f) { _shakeOffset = Vector3.zero; _shakeRoll = 0f; _shakeAmp = 0f; }
            }
        }

        // ------------------------------------------------------------------ lights

        private Light TakeLight()
        {
            for (int i = 0; i < _pool.Count; i++)
                if (!_pool[i].Light.enabled) return _pool[i].Light;
            if (_pool.Count >= MaxLights) return null;
            var go = new GameObject("FxLight") { hideFlags = HideFlags.HideAndDontSave };
            go.transform.SetParent(transform, false);
            var l = go.AddComponent<Light>();
            l.type = LightType.Point;
            l.shadows = LightShadows.None;
            l.enabled = false;
            AetheriaRender.PrepareFxLight(l);
            _pool.Add(new Flash { Light = l });
            return l;
        }

        /// <summary>Point light that flares instantly and decays smoothly.</summary>
        public static void FlashLight(Vector3 pos, Color color, float intensity = 18f, float range = 11f, float duration = 0.45f)
        {
            var fx = Instance;
            Light l = fx.TakeLight();
            if (l == null) return;
            l.transform.position = pos;
            AetheriaRender.StyleFxLight(l, color, range, intensity);
            l.enabled = true;
            fx._active.Add(new Flash { Light = l, Age = 0f, Duration = Mathf.Max(0.05f, duration), Peak = intensity });
        }

        // ------------------------------------------------------------------ particle bursts

        private static ParticleSystem Spawn(Vector3 pos, Material mat, Texture2D map)
        {
            var go = new GameObject("FxBurst") { hideFlags = HideFlags.HideAndDontSave };
            go.transform.position = pos;
            go.transform.SetParent(Instance.transform, true);
            var ps = go.AddComponent<ParticleSystem>();
            var r = ps.GetComponent<ParticleSystemRenderer>();
            r.sharedMaterial = mat;
            r.shadowCastingMode = ShadowCastingMode.Off;
            r.receiveShadows = false;
            r.lightProbeUsage = LightProbeUsage.Off;
            r.reflectionProbeUsage = ReflectionProbeUsage.Off;
            return ps;
        }

        private static ParticleSystem.MinMaxGradient Fade(Color color, float endAlpha, Color? endColor = null)
        {
            var g = new Gradient();
            Color cEnd = endColor ?? color;
            g.SetKeys(
                new[] { new GradientColorKey(Color.white, 0f), new GradientColorKey(color, 0.18f), new GradientColorKey(cEnd, 1f) },
                new[] { new GradientAlphaKey(0f, 0f), new GradientAlphaKey(color.a, 0.10f), new GradientAlphaKey(endAlpha, 1f) });
            return new ParticleSystem.MinMaxGradient(g);
        }

        /// <summary>Radial sparks / embers / dust.</summary>
        public static void Burst(Vector3 pos, Color color, int count, float speedMin, float speedMax, float sizeMin, float sizeMax,
            float lifetime, float gravity = 0.2f, float drag = 1.6f, Color? endColor = null, bool additive = true, Vector3 dir = default, float focus = 0f)
        {
            if (count <= 0) return;
            FxAssets.Ensure();
            var ps = Spawn(pos, AetheriaRender.Particle(Color.white, additive, FxAssets.Glow), FxAssets.Glow);
            var main = ps.main;
            main.startLifetime = lifetime;
            main.startSpeed = new ParticleSystem.MinMaxCurve(speedMin, speedMax);
            main.startSize = new ParticleSystem.MinMaxCurve(sizeMin, sizeMax);
            main.startRotation = new ParticleSystem.MinMaxCurve(0f, Mathf.PI * 2f);
            main.startColor = Color.white;
            main.gravityModifier = gravity;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.duration = 0.05f;
            main.loop = false;
            main.scalingMode = ParticleSystemScalingMode.Hierarchy;
            var em = ps.emission; em.rateOverTime = 0f;
            var shape = ps.shape;
            if (focus > 0.01f && dir.sqrMagnitude > 0.001f)
            {
                shape.shapeType = ParticleSystemShapeType.Cone;
                shape.angle = Mathf.Lerp(35f, 3f, Mathf.Clamp01(focus));
                ps.transform.rotation = Quaternion.LookRotation(dir.normalized, Vector3.up);
                shape.radius = 0.1f;
            }
            else
            {
                shape.shapeType = ParticleSystemShapeType.Sphere;
                shape.radius = 0.12f;
            }
            // drag is approximated by the lifetime fade; VelocityOverLifetime has no drag in this Unity version
            var col = ps.colorOverLifetime; col.enabled = true; col.color = Fade(color, 0f, endColor);
            var sol = ps.sizeOverLifetime; sol.enabled = true;
            sol.size = new ParticleSystem.MinMaxCurve(1f, FxAssets.Curve(1f, 0.25f));
            var r = ps.GetComponent<ParticleSystemRenderer>();
            r.renderMode = ParticleSystemRenderMode.Billboard;
            ps.Emit(count);
            Object.Destroy(ps.gameObject, lifetime + 0.5f);
        }

        // overload without drag
        public static void Burst(Vector3 pos, Color color, int count, float speedMin, float speedMax, float sizeMin, float sizeMax,
            float lifetime, float gravity, Color? endColor, bool additive = true, Vector3 dir = default, float focus = 0f)
            => Burst(pos, color, count, speedMin, speedMax, sizeMin, sizeMax, lifetime, gravity, 1.2f, endColor, additive, dir, focus);

        /// <summary>Soft rising puffs (smoke, mist, dust).</summary>
        public static void Puff(Vector3 pos, Color color, int count, float speed, float size, float lifetime, float rise = 1.2f)
        {
            if (count <= 0) return;
            FxAssets.Ensure();
            var ps = Spawn(pos, AetheriaRender.Particle(Color.white, false, FxAssets.Smoke), FxAssets.Smoke);
            var main = ps.main;
            main.startLifetime = new ParticleSystem.MinMaxCurve(lifetime * 0.6f, lifetime);
            main.startSpeed = new ParticleSystem.MinMaxCurve(speed * 0.4f, speed);
            main.startSize = new ParticleSystem.MinMaxCurve(size * 0.5f, size);
            main.startRotation = new ParticleSystem.MinMaxCurve(0f, Mathf.PI * 2f);
            main.gravityModifier = -rise / 9.81f * 0.6f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = false;
            var em = ps.emission; em.rateOverTime = 0f;
            var shape = ps.shape; shape.shapeType = ParticleSystemShapeType.Sphere; shape.radius = 0.35f;
            var col = ps.colorOverLifetime; col.enabled = true;
            col.color = Fade(color, 0f, Color.Lerp(color, Color.white, 0.15f));
            var sol = ps.sizeOverLifetime; sol.enabled = true;
            sol.size = new ParticleSystem.MinMaxCurve(1f, AnimationCurve.EaseInOut(0f, 0.35f, 1f, 1.7f));
            ps.Emit(count);
            Object.Destroy(ps.gameObject, lifetime + 0.5f);
        }

        /// <summary>One expanding ring (ground shockwave). Vertical = stands upright facing along <paramref name="facing"/>.</summary>
        public static void Shockwave(Vector3 pos, Color color, float radius, float duration = 0.55f, float width = 0.22f, bool vertical = false, Vector3 facing = default)
        {
            FxAssets.Ensure();
            var ps = Spawn(pos, AetheriaRender.Particle(Color.white, true, FxAssets.Glow), FxAssets.Glow);
            ps.transform.rotation = vertical
                ? Quaternion.LookRotation(facing.sqrMagnitude > 0.001f ? facing : Vector3.forward, Vector3.up) * Quaternion.Euler(90f, 0f, 0f)
                : Quaternion.identity;
            var main = ps.main;
            main.startLifetime = duration;
            main.startSpeed = 0f;
            main.startSize3D = true; main.startSizeX = radius; main.startSizeY = 1f; main.startSizeZ = radius;
            main.simulationSpace = ParticleSystemSimulationSpace.Local;
            main.loop = false;
            main.duration = 0.05f;
            var em = ps.emission; em.rateOverTime = 0f;
            var col = ps.colorOverLifetime; col.enabled = true;
            var g = new Gradient();
            g.SetKeys(new[] { new GradientColorKey(Color.white, 0f), new GradientColorKey(color, 0.35f), new GradientColorKey(color, 1f) },
                      new[] { new GradientAlphaKey(0.95f, 0f), new GradientAlphaKey(0.7f, 0.45f), new GradientAlphaKey(0f, 1f) });
            col.color = new ParticleSystem.MinMaxGradient(g);
            var sol = ps.sizeOverLifetime; sol.enabled = true;
            sol.size = new ParticleSystem.MinMaxCurve(1f, FxAssets.Curve(0.12f, 1f));
            var r = ps.GetComponent<ParticleSystemRenderer>();
            r.renderMode = ParticleSystemRenderMode.Mesh;
            r.mesh = FxAssets.Ring;
            r.alignment = ParticleSystemRenderSpace.Local;
            ps.Emit(1);
            Object.Destroy(ps.gameObject, duration + 0.3f);
        }

        /// <summary>Tall cylinder of light falling from the sky (orbital strikes) with a soft ground disc.</summary>
        public static void Beam(Vector3 pos, Color color, float radius, float height, float duration, float intensity = 26f)
        {
            FxAssets.Ensure();
            var ps = Spawn(pos + Vector3.up * (height * 0.5f), AetheriaRender.Particle(Color.white, true, FxAssets.Noise), FxAssets.Noise);
            var main = ps.main;
            main.startLifetime = duration;
            main.startSpeed = 0f;
            main.startSize3D = true; main.startSizeX = radius * 2f; main.startSizeY = height * 0.5f; main.startSizeZ = radius * 2f;
            main.startRotationX = 0f; main.startRotationY = Random.value * Mathf.PI; main.startRotationZ = 0f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = false;
            main.duration = 0.05f;
            var em = ps.emission; em.rateOverTime = 0f;
            var col = ps.colorOverLifetime; col.enabled = true;
            var g = new Gradient();
            g.SetKeys(new[] { new GradientColorKey(Color.white, 0f), new GradientColorKey(color, 0.3f), new GradientColorKey(color, 1f) },
                      new[] { new GradientAlphaKey(0f, 0f), new GradientAlphaKey(0.92f, 0.12f), new GradientAlphaKey(0.75f, 0.6f), new GradientAlphaKey(0f, 1f) });
            col.color = new ParticleSystem.MinMaxGradient(g);
            var r = ps.GetComponent<ParticleSystemRenderer>();
            r.renderMode = ParticleSystemRenderMode.Mesh;
            r.mesh = FxAssets.Beam;
            r.alignment = ParticleSystemRenderSpace.Local;
            ps.Emit(1);
            Object.Destroy(ps.gameObject, duration + 0.3f);

            // Bright core + ground disc
            Shockwave(pos + Vector3.up * 0.12f, color, radius * 2.6f, duration * 0.9f, vertical: false);
            var core = Spawn(pos + Vector3.up * (height * 0.5f), AetheriaRender.Particle(Color.white, true, FxAssets.Glow), FxAssets.Glow);
            var cm = core.main;
            cm.startLifetime = duration * 0.8f; cm.startSpeed = 0f;
            cm.startSize3D = true; cm.startSizeX = radius * 0.9f; cm.startSizeY = height * 0.5f; cm.startSizeZ = radius * 0.9f;
            cm.simulationSpace = ParticleSystemSimulationSpace.World; cm.loop = false; cm.duration = 0.05f;
            var ce = core.emission; ce.rateOverTime = 0f;
            var cc = core.colorOverLifetime; cc.enabled = true; cc.color = Fade(new Color(1f, 1f, 0.92f, 0.85f), 0f);
            var cr = core.GetComponent<ParticleSystemRenderer>();
            cr.renderMode = ParticleSystemRenderMode.Mesh; cr.mesh = FxAssets.Beam; cr.alignment = ParticleSystemRenderSpace.Local;
            core.Emit(1);
            Object.Destroy(core.gameObject, duration + 0.3f);

            FlashLight(pos + Vector3.up * 2.5f, color, intensity, radius * 9f, duration);
        }

        /// <summary>Glowing sphere that expands and dissolves (novas, impacts).</summary>
        public static void SpherePulse(Vector3 pos, Color color, float radius, float duration = 0.45f, float alpha = 0.55f)
        {
            FxAssets.Ensure();
            var ps = Spawn(pos, AetheriaRender.Particle(Color.white, true, FxAssets.Noise), FxAssets.Noise);
            var main = ps.main;
            main.startLifetime = duration; main.startSpeed = 0f;
            { var s = radius * 2f; main.startSize3D = true; main.startSizeX = s; main.startSizeY = s; main.startSizeZ = s; }
            main.simulationSpace = ParticleSystemSimulationSpace.World; main.loop = false; main.duration = 0.05f;
            var em = ps.emission; em.rateOverTime = 0f;
            var col = ps.colorOverLifetime; col.enabled = true;
            var g = new Gradient();
            g.SetKeys(new[] { new GradientColorKey(Color.white, 0f), new GradientColorKey(color, 0.4f), new GradientColorKey(color, 1f) },
                      new[] { new GradientAlphaKey(alpha, 0f), new GradientAlphaKey(alpha * 0.6f, 0.5f), new GradientAlphaKey(0f, 1f) });
            col.color = new ParticleSystem.MinMaxGradient(g);
            var sol = ps.sizeOverLifetime; sol.enabled = true;
            sol.size = new ParticleSystem.MinMaxCurve(1f, FxAssets.Curve(0.25f, 1f));
            var r = ps.GetComponent<ParticleSystemRenderer>();
            r.renderMode = ParticleSystemRenderMode.Mesh; r.mesh = FxAssets.Sphere; r.alignment = ParticleSystemRenderSpace.Local;
            ps.Emit(1);
            Object.Destroy(ps.gameObject, duration + 0.3f);
        }

        /// <summary>Camera-facing star flare (impact sparks, shrine kindling).</summary>
        public static void Flare(Vector3 pos, Color color, float size, float duration = 0.3f)
        {
            FxAssets.Ensure();
            var ps = Spawn(pos, AetheriaRender.Particle(Color.white, true, FxAssets.Star), FxAssets.Star);
            var main = ps.main;
            main.startLifetime = duration; main.startSpeed = 0f;
            main.startSize = new ParticleSystem.MinMaxCurve(size * 0.8f, size * 1.2f);
            main.startRotationX = 0f; main.startRotationY = 0f; main.startRotationZ = Random.value * Mathf.PI;
            main.simulationSpace = ParticleSystemSimulationSpace.World; main.loop = false; main.duration = 0.05f;
            var em = ps.emission; em.rateOverTime = 0f;
            var sol = ps.sizeOverLifetime; sol.enabled = true;
            sol.size = new ParticleSystem.MinMaxCurve(1f, FxAssets.Curve(0.3f, 1f));
            var col = ps.colorOverLifetime; col.enabled = true; col.color = Fade(color, 0f);
            var r = ps.GetComponent<ParticleSystemRenderer>();
            r.renderMode = ParticleSystemRenderMode.Billboard;
            ps.Emit(1);
            Object.Destroy(ps.gameObject, duration + 0.3f);
        }

        /// <summary>Physics debris chunks (stylized rocks / shards) thrown away from the impact point.</summary>
        public static void Debris(Vector3 pos, Color color, int count, Vector3 impulseOrigin, float force = 9f, float size = 0.35f, bool emissive = false, Mesh mesh = null)
        {
            if (count <= 0) return;
            FxAssets.Ensure();
            Material mat = emissive ? AetheriaRender.Glow(color, 3.5f) : AetheriaRender.Lit(color, 0.15f, 0f);
            for (int i = 0; i < count; i++)
            {
                var go = new GameObject("FxDebris");
                go.transform.position = pos + Random.insideUnitSphere * 0.35f;
                go.AddComponent<MeshFilter>().sharedMesh = mesh ?? (i % 3 == 0 ? FxAssets.Rock : FxAssets.Icosphere);
                var mr = go.AddComponent<MeshRenderer>();
                mr.sharedMaterial = mat;
                mr.shadowCastingMode = ShadowCastingMode.Off;
                float s = size * Random.Range(0.55f, 1.35f);
                go.transform.localScale = Vector3.one * s;
                var rb = go.AddComponent<Rigidbody>();
                rb.mass = 1.2f;
                rb.drag = 0.4f;
                Vector3 dir = (go.transform.position - impulseOrigin);
                dir.y = Mathf.Abs(dir.y) + 0.6f;
                rb.velocity = dir.normalized * Random.Range(force * 0.45f, force) + Vector3.up * Random.Range(1.5f, 5.5f);
                rb.angularVelocity = Random.insideUnitSphere * 9f;
                Object.Destroy(go, Random.Range(2.6f, 4.2f));
            }
        }

        /// <summary>Jagged electric arc between two points. Re-struck a few times, then gone.</summary>
        public static void Lightning(Vector3 from, Vector3 to, Color color, float duration = 0.22f, float width = 0.10f, int segments = 10, float jitter = 1.1f)
        {
            var go = new GameObject("FxLightning") { hideFlags = HideFlags.HideAndDontSave };
            go.transform.SetParent(Instance.transform, true);
            go.AddComponent<MeshFilter>();
            go.AddComponent<MeshRenderer>();
            var arc = go.AddComponent<FxLightningArc>();
            arc.Setup(from, to, color, duration, width, segments, jitter);
        }

        /// <summary>Short-lived glowing ring left on the ground by a spell (terrain scar).</summary>
        public static GameObject GroundRing(Vector3 pos, Color color, float radius, float duration, float width = 0.14f)
        {
            var go = new GameObject("FxGroundRing");
            go.transform.position = pos + Vector3.up * 0.12f;
            go.AddComponent<MeshFilter>().sharedMesh = FxAssets.Ring;
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterial = AetheriaRender.TransparentGlow(color, true, FxAssets.Glow);
            mr.shadowCastingMode = ShadowCastingMode.Off;
            go.transform.localScale = new Vector3(radius, 1f, radius);
            if (duration > 0f) Object.Destroy(go, duration);
            return go;
        }
    }

    /// <summary>Rebuilds a camera-facing ribbon along a jagged path for the lifetime of a lightning strike.</summary>
    public class FxLightningArc : MonoBehaviour
    {
        private Vector3 _from, _to;
        private Color _color;
        private float _duration, _width, _jitter;
        private int _segments;
        private float _age;
        private float _rebuild;
        private Mesh _mesh;
        private MeshRenderer _renderer;

        public void Setup(Vector3 from, Vector3 to, Color color, float duration, float width, int segments, float jitter)
        {
            _from = from; _to = to; _color = color; _duration = duration; _width = width; _segments = segments; _jitter = jitter;
            _mesh = new Mesh { name = "FxLightning" };
            _mesh.MarkDynamic();
            GetComponent<MeshFilter>().sharedMesh = _mesh;
            _renderer = GetComponent<MeshRenderer>();
            _renderer.sharedMaterial = AetheriaRender.TransparentGlow(color, true, FxAssets.Glow);
            _renderer.shadowCastingMode = ShadowCastingMode.Off;
            Rebuild();
        }

        private void Update()
        {
            if (_mesh == null) { Destroy(gameObject); return; }
            _age += Time.unscaledDeltaTime;
            if (_age >= _duration) { Destroy(gameObject); return; }
            _rebuild -= Time.unscaledDeltaTime;
            if (_rebuild <= 0f) { _rebuild = 0.045f; Rebuild(); }
        }

        private void Rebuild()
        {
            var cam = Camera.main;
            Vector3 viewDir = cam != null ? cam.transform.forward : Vector3.forward;
            int n = _segments + 1;
            var verts = new Vector3[n * 2];
            var uv = new Vector2[n * 2];
            var tris = new int[_segments * 6];
            Vector3 prev = Vector3.zero;
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)_segments;
                Vector3 p = Vector3.Lerp(_from, _to, t);
                if (i > 0 && i < n - 1)
                {
                    Vector3 axis = (_to - _from).normalized;
                    Vector3 side = Vector3.Cross(axis, viewDir).normalized;
                    Vector3 up = Vector3.Cross(side, axis).normalized;
                    p += side * ((AetheriaWorld.Hash(i, Mathf.FloorToInt(Time.time * 1000f)) - 0.5f) * 2f * _jitter)
                       + up * ((AetheriaWorld.Hash(i, 77, 5) - 0.5f) * 2f * _jitter);
                }
                Vector3 local = transform.InverseTransformPoint(p);
                Vector3 dir = (local - prev);
                if (i == 0) dir = transform.InverseTransformPoint(Vector3.Lerp(_from, _to, 1f / _segments)) - local;
                Vector3 w = Vector3.Cross(dir, transform.InverseTransformDirection(viewDir)).normalized * (_width * Mathf.Lerp(1f, 0.35f, t));
                verts[i * 2] = local - w; verts[i * 2 + 1] = local + w;
                uv[i * 2] = new Vector2(0f, t); uv[i * 2 + 1] = new Vector2(1f, t);
                prev = local;
                if (i < _segments)
                {
                    tris[i * 6] = i * 2; tris[i * 6 + 1] = i * 2 + 1; tris[i * 6 + 2] = i * 2 + 2;
                    tris[i * 6 + 3] = i * 2 + 1; tris[i * 6 + 4] = i * 2 + 3; tris[i * 6 + 5] = i * 2 + 2;
                }
            }
            _mesh.Clear();
            _mesh.SetVertices(verts); _mesh.SetUVs(0, uv); _mesh.SetTriangles(tris, 0);
            _mesh.RecalculateBounds();
            // Flicker
            // Quantised so the material cache stays tiny
            float k = Mathf.Round(Mathf.Lerp(1f, 0.2f, _age / _duration) * (0.75f + 0.25f * Random.value) * 5f) / 5f;
            _renderer.sharedMaterial = AetheriaRender.TransparentGlow(_color * k, true, FxAssets.Glow);
        }

        private void OnDestroy()
        {
            if (_mesh != null) Destroy(_mesh);
        }
    }
}
