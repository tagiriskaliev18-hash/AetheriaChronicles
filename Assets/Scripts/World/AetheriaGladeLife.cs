using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Small life for a scenic spot: butterflies that wander on noise paths with flapping wings, plus drifting
    /// pollen motes in the sunlight. Meshes and the wing material are authored by the prologue builder.
    /// </summary>
    public class AetheriaGladeLife : MonoBehaviour
    {
        public Mesh[] LeftWings;
        public Mesh[] RightWings;
        public Material WingMaterial;
        public int Butterflies = 14;
        public float Radius = 9f;
        public float MoteCount = 120;

        private Transform[] _bodies;
        private Transform[] _left, _right;
        private Vector3[] _seed;
        private float[] _flap;

        private void Start()
        {
            if (LeftWings == null || RightWings == null || LeftWings.Length == 0 || WingMaterial == null) return;
            _bodies = new Transform[Butterflies];
            _left = new Transform[Butterflies];
            _right = new Transform[Butterflies];
            _seed = new Vector3[Butterflies];
            _flap = new float[Butterflies];
            for (int i = 0; i < Butterflies; i++)
            {
                int pattern = i % LeftWings.Length;
                var body = new GameObject("Butterfly_" + i).transform;
                body.SetParent(transform, false);
                _left[i] = Wing(body, LeftWings[pattern]);
                _right[i] = Wing(body, RightWings[pattern]);
                body.localScale = Vector3.one * Random.Range(0.8f, 1.3f);
                _bodies[i] = body;
                _seed[i] = new Vector3(Random.value * 100f, Random.value * 100f, Random.value * 100f);
                _flap[i] = Random.Range(9f, 14f);
            }
            SpawnMotes();
        }

        private Transform Wing(Transform body, Mesh mesh)
        {
            var go = new GameObject("Wing");
            go.transform.SetParent(body, false);
            go.AddComponent<MeshFilter>().sharedMesh = mesh;
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterial = WingMaterial;
            mr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            return go.transform;
        }

        private void Update()
        {
            if (_bodies == null) return;
            float t = Time.time;
            for (int i = 0; i < _bodies.Length; i++)
            {
                Vector3 s = _seed[i];
                float k = t * 0.12f;
                // Lazy figure-eights over the flowers, dipping toward them now and then.
                Vector3 p = new Vector3(
                    (Mathf.PerlinNoise(s.x + k, s.y) - 0.5f) * 2f * Radius,
                    0.5f + Mathf.PerlinNoise(s.z, s.x + k * 1.7f) * 1.8f,
                    (Mathf.PerlinNoise(s.y, s.z + k) - 0.5f) * 2f * Radius);
                Vector3 ahead = new Vector3(
                    (Mathf.PerlinNoise(s.x + k + 0.05f, s.y) - 0.5f) * 2f * Radius,
                    p.y,
                    (Mathf.PerlinNoise(s.y, s.z + k + 0.05f) - 0.5f) * 2f * Radius);
                var body = _bodies[i];
                Vector3 world = transform.TransformPoint(p);
                world.y = Mathf.Max(world.y, AetheriaWorld.SampleHeight(world) + 0.35f);
                body.position = world;
                Vector3 dir = transform.TransformDirection(ahead - p);
                dir.y = 0f;
                if (dir.sqrMagnitude > 1e-5f) body.rotation = Quaternion.LookRotation(dir.normalized, Vector3.up);
                float flap = Mathf.Sin(t * _flap[i] + s.x) * 65f + 10f;
                _left[i].localRotation = Quaternion.Euler(0f, 0f, flap);
                _right[i].localRotation = Quaternion.Euler(0f, 0f, -flap);
            }
        }

        private void SpawnMotes()
        {
            var go = new GameObject("PollenMotes");
            go.transform.SetParent(transform, false);
            go.transform.localPosition = Vector3.up * 1.2f;
            var ps = go.AddComponent<ParticleSystem>();
            var main = ps.main;
            main.loop = true;
            main.startLifetime = new ParticleSystem.MinMaxCurve(6f, 10f);
            main.startSpeed = new ParticleSystem.MinMaxCurve(0.02f, 0.12f);
            main.startSize = new ParticleSystem.MinMaxCurve(0.02f, 0.05f);
            main.startColor = new Color(1f, 0.93f, 0.7f, 0.8f);
            main.maxParticles = Mathf.RoundToInt(MoteCount);
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.gravityModifier = -0.002f;
            var em = ps.emission;
            em.rateOverTime = MoteCount / 8f;
            var sh = ps.shape;
            sh.shapeType = ParticleSystemShapeType.Box;
            sh.scale = new Vector3(Radius * 1.6f, 2.4f, Radius * 1.6f);
            var noise = ps.noise;
            noise.enabled = true;
            noise.strength = 0.25f;
            noise.frequency = 0.3f;
            var col = ps.colorOverLifetime;
            col.enabled = true;
            var g = new Gradient();
            g.SetKeys(new[] { new GradientColorKey(Color.white, 0f), new GradientColorKey(Color.white, 1f) },
                      new[] { new GradientAlphaKey(0f, 0f), new GradientAlphaKey(1f, 0.2f), new GradientAlphaKey(1f, 0.8f), new GradientAlphaKey(0f, 1f) });
            col.color = g;
            var r = go.GetComponent<ParticleSystemRenderer>();
            r.sharedMaterial = AetheriaRender.Particle(new Color(1f, 0.9f, 0.65f), true, FxAssets.Star);
            ps.Play();
        }
    }
}
