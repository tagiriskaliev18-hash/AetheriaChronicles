using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria
{
    /// <summary>
    /// White-water particle effects for a waterfall, built entirely at runtime (no prefabs or assets):
    /// spray peeling off the lip, stretched streaks sliding down the fall, soft billowing mist at the
    /// plunge pool, a slowly spreading ring of foam on the water and occasional splash bursts at the impact.
    /// Configure the public fields (normally from an editor builder) before Start; emission scales with
    /// Width and fall height. HDRP/Unlit ignores particle vertex colour, so every fade is baked into the
    /// frames of procedural flipbook textures (TextureSheetAnimation) or done with size curves.
    /// Particles simulate in world space with scaled time, so slow motion slows the water too.
    /// </summary>
    public class AetheriaWaterfallFx : MonoBehaviour
    {
        /// <summary>World-space points of the fall, from the lip (index 0) down to the plunge pool (last).</summary>
        public Vector3[] Path;
        /// <summary>Unit vector across the fall (width direction).</summary>
        public Vector3 Side = Vector3.right;
        /// <summary>Full width at the lip; the fall widens about 1.6x toward the bottom.</summary>
        public float Width = 6f;
        /// <summary>Plunge pool water surface height.</summary>
        public float WaterY;
        /// <summary>Multiplier on every emission rate.</summary>
        public float Density = 1f;
        /// <summary>Unlit colour scale. HDRP/Unlit ignores scene light, so keep it below 1 in dim scenes.</summary>
        public float Brightness = 0.7f;
        /// <summary>No new particles are emitted while the main camera is farther than this from the pool.</summary>
        public float ActiveDistance = 220f;
        /// <summary>Seed for every random choice (textures and emission).</summary>
        public int Seed = 7417;

        private const int Tiles = 4;   // 4x4 flipbook frames = 16 fade steps
        private static readonly Color Water = new Color(0.86f, 0.9f, 0.93f);   // cool white, never blue

        private readonly List<Object> _owned = new List<Object>();
        private System.Random _rng;
        private float[] _cum;
        private float _length, _wScale, _hScale;
        private Vector3 _impact, _outLip, _outBottom, _tanLip;
        private ParticleSystem _spray, _streaks, _mist, _foam, _splash;
        private float _accSpray, _accStreak, _accMist, _accFoam, _nextSplash;

        private void Start()
        {
            if (Path == null || Path.Length < 2) { enabled = false; return; }
            _rng = new System.Random(Seed);
            Side = Side.sqrMagnitude > 1e-4f ? Side.normalized : Vector3.right;
            _cum = new float[Path.Length];
            for (int i = 1; i < Path.Length; i++) _cum[i] = _cum[i - 1] + Vector3.Distance(Path[i - 1], Path[i]);
            _length = Mathf.Max(0.01f, _cum[Path.Length - 1]);
            float height = Mathf.Max(1f, Path[0].y - WaterY);
            _wScale = Mathf.Max(0.2f, Width / 6f);
            _hScale = Mathf.Clamp(height / 20f, 0.6f, 2.5f);
            Vector3 last = Path[Path.Length - 1];
            _impact = new Vector3(last.x, WaterY, last.z);
            Vector3 fallback = Vector3.Cross(Side, Vector3.up).normalized;
            _tanLip = (Path[1] - Path[0]).normalized;
            _outLip = Flat(Path[1] - Path[0], fallback);
            _outBottom = Flat(last - Path[Path.Length - 2], _outLip);
            _nextSplash = R(0.3f, 1.2f);

            var blob = Own(FadeAtlas(64, false, Seed));
            var foamTex = Own(FadeAtlas(64, true, Seed + 1));
            var streak = Own(StreakTexture(32, 128, Seed + 2));
            float b = Brightness;

            _spray = Make("Fx_LipSpray", Mat(blob, Water * b, 0.55f), 350, 1f, true);
            Size(_spray, FxAssets.Curve(1f, 0.55f));
            Sorting(_spray, -5f, 0.5f);

            _streaks = Make("Fx_FallStreaks", Mat(streak, Water * b, 0.3f), 500, 0f, false);
            Size(_streaks, new AnimationCurve(new Keyframe(0f, 0f), new Keyframe(0.15f, 1f), new Keyframe(0.7f, 0.9f), new Keyframe(1f, 0f)));
            var sr = _streaks.GetComponent<ParticleSystemRenderer>();
            sr.renderMode = ParticleSystemRenderMode.Stretch;
            sr.lengthScale = 1.5f;
            sr.velocityScale = 0.07f;
            sr.cameraVelocityScale = 0f;

            _mist = Make("Fx_BaseMist", Mat(blob, Water * (b * 0.85f), 0.12f), 120, -0.015f, true);
            Size(_mist, FxAssets.Curve(0.55f, 1.6f));
            Sorting(_mist, 20f, 2.5f);

            _foam = Make("Fx_FoamRing", Mat(foamTex, Water * (b * 0.95f), 0.6f), 120, 0f, true);
            Size(_foam, FxAssets.Curve(0.5f, 1.7f));
            Sorting(_foam, 30f, 3f);
            _foam.GetComponent<ParticleSystemRenderer>().renderMode = ParticleSystemRenderMode.HorizontalBillboard;

            _splash = Make("Fx_SplashBursts", Mat(blob, Water * b, 0.5f), 160, 1.1f, true);
            Size(_splash, FxAssets.Curve(0.6f, 1.5f));
            Sorting(_splash, -5f, 1f);
        }

        private void Update()
        {
            float dt = Time.deltaTime;   // scaled: the slow-motion camera slows the emitters with the particles
            if (dt <= 0f || _rng == null) return;
            var cam = Camera.main;
            if (cam != null && (cam.transform.position - _impact).sqrMagnitude > ActiveDistance * ActiveDistance) return;
            float d = Mathf.Max(0f, Density);
            for (int i = Count(ref _accSpray, 60f * _wScale * d, dt); i > 0; i--) EmitSpray();
            for (int i = Count(ref _accStreak, 220f * _wScale * _hScale * d, dt); i > 0; i--) EmitStreak();
            for (int i = Count(ref _accMist, 6f * _wScale * Mathf.Sqrt(_hScale) * d, dt); i > 0; i--) EmitMist(1f);
            for (int i = Count(ref _accFoam, 5f * _wScale * d, dt); i > 0; i--) EmitFoam();
            _nextSplash -= dt;
            if (_nextSplash <= 0f && d > 0f)
            {
                _nextSplash = R(0.7f, 2.2f) / Mathf.Max(0.3f, d * Mathf.Sqrt(_wScale));
                Burst();
            }
        }

        private void OnDestroy()
        {
            for (int i = 0; i < _owned.Count; i++)
                if (_owned[i] != null) Destroy(_owned[i]);
            _owned.Clear();
        }

        // ------------------------------------------------------------------ emission

        private void EmitSpray()
        {
            bool sheet = _rng.NextDouble() < 0.15;   // torn sheets among the fine droplets
            var p = new ParticleSystem.EmitParams
            {
                position = Path[0] + Side * (R(-0.5f, 0.5f) * Width) + Vector3.up * R(-0.1f, 0.25f),
                velocity = _tanLip * R(2.5f, 5f) + _outLip * R(0f, 1.5f) + Vector3.up * R(0f, 1.2f) + Side * R(-0.8f, 0.8f),
                startSize = sheet ? R(0.5f, 1.3f) : R(0.06f, 0.3f),
                startLifetime = R(1.1f, 2.1f),
                rotation = R(0f, 360f),
                angularVelocity = R(-90f, 90f),
            };
            _spray.Emit(p, 1);
        }

        private void EmitStreak()
        {
            float s = R(0f, _length * 0.97f);
            Sample(s, out Vector3 pos, out Vector3 tan);
            float w = Width * Mathf.Lerp(1f, 1.6f, s / _length);
            float speed = 3f + Mathf.Sqrt(2f * 9.81f * Mathf.Max(0f, Path[0].y - pos.y));
            var p = new ParticleSystem.EmitParams
            {
                position = pos + Side * (R(-0.5f, 0.5f) * w) + Flat(tan, _outLip) * R(0.05f, 0.5f),
                velocity = tan * (speed * R(0.85f, 1.1f)),
                startSize = R(0.06f, 0.2f),
                // short lives keep the straight streaks on the curved path and out of the pool
                startLifetime = Mathf.Clamp((_length - s) / speed + 0.1f, 0.12f, R(0.3f, 0.55f)),
            };
            _streaks.Emit(p, 1);
        }

        private void EmitMist(float lift)
        {
            float wb = Width * 1.6f;
            Vector3 offset = Side * (R(-0.5f, 0.5f) * wb) + _outBottom * (R(-0.8f, 2.5f) * _wScale);
            var p = new ParticleSystem.EmitParams
            {
                position = _impact + offset + Vector3.up * R(0.2f, 1.2f),
                velocity = Flat(offset, _outBottom) * R(0.4f, 1.8f) + Vector3.up * (R(0.3f, 1.1f) * lift),
                startSize = R(3.5f, 8f) * Mathf.Sqrt(_wScale),
                startLifetime = R(5f, 8.5f),
                rotation = R(0f, 360f),
                angularVelocity = R(-12f, 12f),
            };
            _mist.Emit(p, 1);
        }

        private void EmitFoam()
        {
            float a = R(0f, Mathf.PI * 2f);
            var dir = new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a));
            Vector3 pos = _impact + dir * (R(0.3f, 0.75f) * Width * 0.8f);
            pos.y = WaterY + 0.05f;
            var p = new ParticleSystem.EmitParams
            {
                position = pos,
                velocity = dir * R(0.25f, 0.9f) + _outBottom * R(0f, 0.4f),
                startSize = R(1.4f, 3.2f) * Mathf.Sqrt(_wScale),
                startLifetime = R(6f, 10f),
                rotation = R(0f, 360f),
                angularVelocity = R(-8f, 8f),
            };
            _foam.Emit(p, 1);
        }

        private void Burst()
        {
            Vector3 c = _impact + Side * (R(-0.4f, 0.4f) * Width * 1.6f) + Vector3.up * 0.2f;
            int n = Mathf.RoundToInt(R(14f, 26f) * Mathf.Sqrt(_wScale));
            float up = Mathf.Sqrt(_hScale);
            for (int i = 0; i < n; i++)
            {
                float a = R(0f, Mathf.PI * 2f);
                var dir = new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a));
                var p = new ParticleSystem.EmitParams
                {
                    position = c + dir * R(0f, 0.6f),
                    velocity = dir * R(1f, 3.5f) + Vector3.up * (R(4f, 9f) * up),
                    startSize = R(0.25f, 1f),
                    startLifetime = R(0.9f, 1.7f),
                    rotation = R(0f, 360f),
                    angularVelocity = R(-60f, 60f),
                };
                _splash.Emit(p, 1);
            }
            EmitMist(2f);
            EmitMist(2f);
        }

        // ------------------------------------------------------------------ setup helpers

        private ParticleSystem Make(string name, Material mat, int max, float gravity, bool flipbook)
        {
            var go = new GameObject(name);
            go.transform.SetParent(transform, false);
            go.SetActive(false);   // configure before the system's own Awake runs
            var ps = go.AddComponent<ParticleSystem>();
            ps.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
            var main = ps.main;
            main.playOnAwake = false;
            main.loop = true;
            main.duration = 5f;
            main.startSpeed = 0f;
            main.gravityModifier = gravity;
            main.maxParticles = max;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.scalingMode = ParticleSystemScalingMode.Hierarchy;
            main.useUnscaledTime = false;
            var em = ps.emission; em.enabled = false;   // everything is emitted by hand from Update
            var shape = ps.shape; shape.enabled = false;
            if (flipbook)
            {
                var tsa = ps.textureSheetAnimation;
                tsa.enabled = true;
                tsa.mode = ParticleSystemAnimationMode.Grid;
                tsa.numTilesX = Tiles;
                tsa.numTilesY = Tiles;
                tsa.animation = ParticleSystemAnimationType.WholeSheet;
                tsa.cycleCount = 1;
                tsa.frameOverTime = new ParticleSystem.MinMaxCurve(1f, AnimationCurve.Linear(0f, 0f, 1f, 0.999f));
            }
            ps.useAutoRandomSeed = false;
            ps.randomSeed = (uint)(Seed + max);
            var r = go.GetComponent<ParticleSystemRenderer>();
            r.sharedMaterial = mat;
            r.renderMode = ParticleSystemRenderMode.Billboard;
            r.shadowCastingMode = ShadowCastingMode.Off;
            r.receiveShadows = false;
            r.lightProbeUsage = LightProbeUsage.Off;
            r.reflectionProbeUsage = ReflectionProbeUsage.Off;
            r.motionVectorGenerationMode = MotionVectorGenerationMode.Camera;
            go.SetActive(true);
            ps.Play();
            return ps;
        }

        /// <summary>Copy of the project's HDRP particle material (AetheriaRender.Particle), alpha blended, with fog.</summary>
        private Material Mat(Texture2D tex, Color c, float alpha)
        {
            var m = Own(new Material(AetheriaRender.Particle(Color.white, false, null)) { name = "Waterfall_" + tex.name });
            c.a = alpha;
            AetheriaRender.SetBaseColor(m, c);
            if (m.HasProperty("_UnlitColorMap")) m.SetTexture("_UnlitColorMap", tex);
            if (m.HasProperty("_BaseColorMap")) m.SetTexture("_BaseColorMap", tex);
            m.mainTexture = tex;
            // The template ships with fog off; transparent HDRP materials need the keyword to be fogged.
            if (m.HasProperty("_EnableFogOnTransparent")) m.SetFloat("_EnableFogOnTransparent", 1f);
            m.EnableKeyword("_ENABLE_FOG_ON_TRANSPARENT");
            if (m.HasProperty("_CullMode")) m.SetFloat("_CullMode", (float)CullMode.Off);
            if (m.HasProperty("_CullModeForward")) m.SetFloat("_CullModeForward", (float)CullMode.Off);
            return m;
        }

        private static void Size(ParticleSystem ps, AnimationCurve curve)
        {
            var sol = ps.sizeOverLifetime;
            sol.enabled = true;
            sol.size = new ParticleSystem.MinMaxCurve(1f, curve);
        }

        private static void Sorting(ParticleSystem ps, float fudge, float maxScreenSize)
        {
            var r = ps.GetComponent<ParticleSystemRenderer>();
            r.sortingFudge = fudge;   // larger = drawn earlier: foam under mist under spray
            r.maxParticleSize = maxScreenSize;
        }

        private static int Count(ref float acc, float rate, float dt)
        {
            acc += rate * dt;
            int n = (int)acc;
            acc -= n;
            return Mathf.Min(n, 40);   // cap after a hitch
        }

        private void Sample(float s, out Vector3 pos, out Vector3 tan)
        {
            int i = 0;
            while (i < Path.Length - 2 && _cum[i + 1] < s) i++;
            float seg = Mathf.Max(1e-4f, _cum[i + 1] - _cum[i]);
            pos = Vector3.Lerp(Path[i], Path[i + 1], Mathf.Clamp01((s - _cum[i]) / seg));
            tan = (Path[i + 1] - Path[i]) / seg;
        }

        private static Vector3 Flat(Vector3 v, Vector3 fallback)
        {
            v.y = 0f;
            return v.sqrMagnitude > 1e-6f ? v.normalized : fallback;
        }

        private float R(float a, float b) => a + (float)_rng.NextDouble() * (b - a);

        private T Own<T>(T o) where T : Object { _owned.Add(o); return o; }

        // ------------------------------------------------------------------ procedural textures

        /// <summary>
        /// 4x4 flipbook of one soft shape whose alpha fades in and out across the frames, so a particle
        /// playing the sheet once over its life fades without vertex colour. Foam frames also erode.
        /// </summary>
        private static Texture2D FadeAtlas(int cell, bool foam, int seed)
        {
            int size = cell * Tiles, frames = Tiles * Tiles;
            var tex = new Texture2D(size, size, TextureFormat.RGBA32, true)
            { name = foam ? "FoamAtlas" : "MistAtlas", wrapMode = TextureWrapMode.Clamp, filterMode = FilterMode.Trilinear };
            var px = new Color32[size * size];
            var rng = new System.Random(seed);
            float ox = (float)rng.NextDouble() * 100f, oy = (float)rng.NextDouble() * 100f;
            for (int k = 0; k < frames; k++)
            {
                float f = (k + 0.5f) / frames;
                float env = Mathf.SmoothStep(0f, 1f, f / 0.15f) * (1f - Mathf.SmoothStep(0f, 1f, (f - 0.4f) / 0.6f));
                int cx = (k % Tiles) * cell, cy = (Tiles - 1 - k / Tiles) * cell;   // frame 0 is top-left
                for (int y = 0; y < cell; y++)
                    for (int x = 0; x < cell; x++)
                    {
                        float u = (x + 0.5f) / cell * 2f - 1f, v = (y + 0.5f) / cell * 2f - 1f;
                        float d = Mathf.Sqrt(u * u + v * v);
                        float nx = x / (float)cell + f * 0.5f, ny = y / (float)cell;
                        float n = Mathf.PerlinNoise(ox + nx * 3f, oy + ny * 3f) * 0.65f
                                + Mathf.PerlinNoise(ox * 2f + nx * 8f, oy * 2f + ny * 8f) * 0.35f;
                        float a;
                        if (foam)
                        {
                            float lace = Mathf.Clamp01((n - Mathf.Lerp(0.3f, 0.62f, f)) / 0.2f);   // breaks up with age
                            a = lace * Mathf.SmoothStep(0f, 1f, (1f - d) / 0.5f);
                        }
                        else a = Mathf.Pow(Mathf.Clamp01(1f - d), 1.6f) * Mathf.Lerp(0.55f, 1f, n);
                        px[(cy + y) * size + cx + x] = new Color32(255, 255, 255, (byte)(Mathf.Clamp01(a * env) * 255f));
                    }
            }
            tex.SetPixels32(px);
            tex.Apply(true, true);
            return tex;
        }

        /// <summary>Thin vertical streak, soft across and tapered at both ends, with noisy breaks along it.</summary>
        private static Texture2D StreakTexture(int w, int h, int seed)
        {
            var tex = new Texture2D(w, h, TextureFormat.RGBA32, true)
            { name = "StreakTex", wrapMode = TextureWrapMode.Clamp, filterMode = FilterMode.Trilinear };
            var px = new Color32[w * h];
            float o = (float)new System.Random(seed).NextDouble() * 100f;
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                {
                    float u = (x + 0.5f) / w * 2f - 1f, v = (y + 0.5f) / h;
                    float across = Mathf.Exp(-u * u * 6f) * Mathf.Clamp01((1f - Mathf.Abs(u)) * 4f);
                    float along = Mathf.Pow(Mathf.Sin(v * Mathf.PI), 0.7f) * Mathf.Lerp(0.5f, 1f, Mathf.PerlinNoise(o, v * 6f));
                    px[y * w + x] = new Color32(255, 255, 255, (byte)(Mathf.Clamp01(across * along) * 255f));
                }
            tex.SetPixels32(px);
            tex.Apply(true, true);
            return tex;
        }
    }
}
