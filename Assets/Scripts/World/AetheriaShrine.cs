using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria
{
    /// <summary>
    /// A fast-travel shrine. Cold stones until the hero kindles the flame with [E]; from then on the
    /// shrine burns, counts for the pilgrimage quest and can be travelled to from the world map.
    /// </summary>
    public class AetheriaShrine : MonoBehaviour
    {
        public static readonly List<AetheriaShrine> All = new List<AetheriaShrine>();

        public string SiteId;
        public Transform FlameAnchor;          // where the fire burns
        public Transform Crystal;              // optional glowing focus (scaled up when lit)
        public Color FlameColor = new Color(0.45f, 1f, 0.85f);

        public bool Kindled { get; private set; }
        private Light _light;
        private ParticleSystem _fire;
        private ParticleSystem _embers;
        private Material _crystalMat;
        private bool _lit;

        private void OnEnable() { if (!All.Contains(this)) All.Add(this); }
        private void OnDisable() => All.Remove(this);

        public static AetheriaShrine FindNearest(Vector3 pos, float maxDist)
        {
            AetheriaShrine best = null;
            float d = maxDist;
            foreach (var s in All)
            {
                if (s == null || s.Kindled) continue;
                float dist = Vector3.Distance(pos, s.transform.position);
                if (dist < d) { d = dist; best = s; }
            }
            return best;
        }

        public static AetheriaShrine Find(string siteId)
        {
            foreach (var s in All) if (s != null && s.SiteId == siteId) return s;
            return null;
        }

        private void Start()
        {
            if (FlameAnchor == null)
            {
                var a = new GameObject("FlameAnchor");
                a.transform.SetParent(transform, false);
                a.transform.localPosition = new Vector3(0f, 1.5f, 0f);
                FlameAnchor = a.transform;
            }
            if (Crystal != null)
            {
                _crystalMat = AetheriaRender.Glow(FlameColor, 6f);
                foreach (var r in Crystal.GetComponentsInChildren<Renderer>(true)) r.sharedMaterial = _crystalMat;
            }
            var map = QuestAndMapService.Instance;
            if (map != null && map.IsKindled(SiteId)) SetLit(true, false);
            else SetLit(false, false);
        }

        /// <summary>Lights the shrine. Returns false if it was already burning.</summary>
        public bool Kindle()
        {
            if (Kindled) return false;
            var map = QuestAndMapService.Instance;
            if (map == null) return false;
            SetLit(true, true);
            map.KindleShrine(SiteId);
            return true;
        }

        private void SetLit(bool lit, bool celebrate)
    {
        if (_lit && lit) return;
        Kindled = lit;
        _lit = lit;

        if (lit)
        {
            if (_light == null) _light = AetheriaRender.CreatePointLight(transform, Vector3.zero, FlameColor, 16f, 0f);
            _light.transform.position = FlameAnchor.position + Vector3.up * 0.4f;
            AetheriaRender.StyleFxLight(_light, FlameColor, 17f, 22f);
            if (_fire == null)
            {
                _fire = FxShrineFlame.Make(FlameAnchor, FlameColor, false);
                _embers = FxShrineFlame.Make(FlameAnchor, FlameColor, true);
            }
            _fire.gameObject.SetActive(true);
            _embers.gameObject.SetActive(true);
            if (Crystal != null) AetheriaRender.SetEmission(_crystalMat, FlameColor * 9f);

            if (celebrate)
            {
                Vector3 at = FlameAnchor.position;
                AetheriaFx.FlashLight(at, FlameColor, 46f, 22f, 0.9f);
                AetheriaFx.Flare(at, FlameColor, 3.4f, 0.5f);
                AetheriaFx.Shockwave(transform.position + Vector3.up * 0.2f, FlameColor, 9f, 1.0f);
                AetheriaFx.Burst(at, FlameColor, 60, 3f, 10f, 0.08f, 0.20f, 1.4f, 0.2f, 1.4f, new Color(FlameColor.r, FlameColor.g * 0.6f, FlameColor.b * 0.4f, 0f));
                AetheriaFx.Shake(0.12f, 0.35f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_LevelUp", at, 0.8f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_BeaconCapture", at, 0.9f);
            }
        }
        else if (_light != null)
        {
            _light.enabled = false;
            if (_fire != null) _fire.gameObject.SetActive(false);
            if (_embers != null) _embers.gameObject.SetActive(false);
            if (Crystal != null) AetheriaRender.SetEmission(_crystalMat, FlameColor * 0.35f);
        }
    }

    /// <summary>Restores the burning state after loading a save.</summary>
    public void RefreshFromSave()
    {
        var map = QuestAndMapService.Instance;
        if (map == null) return;
        SetLit(map.IsKindled(SiteId), false);
    }

    private void Update()
    {
        if (!Kindled || _light == null) return;
        // Living flame: gentle flicker
        float t = Time.time * 9f + transform.position.x;
        AetheriaRender.StyleFxLight(_light, FlameColor, 17f, 22f + Mathf.Sin(t) * 2.2f + Mathf.Sin(t * 2.7f) * 1.4f);
    }
}

/// <summary>Small looping flame + ember emitters used by shrines, braziers and torches.</summary>
public static class FxShrineFlame
{
    public static ParticleSystem Make(Transform anchor, Color color, bool embers)
    {
        FxAssets.Ensure();
        var go = new GameObject(embers ? "Embers" : "Flame");
        go.transform.SetParent(anchor, false);
        go.transform.localPosition = embers ? Vector3.up * 0.2f : Vector3.zero;
        var ps = go.AddComponent<ParticleSystem>();
        var r = ps.GetComponent<ParticleSystemRenderer>();
        r.sharedMaterial = AetheriaRender.Particle(Color.white, true, embers ? FxAssets.Glow : FxAssets.Flame);
        r.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
        r.receiveShadows = false;
        r.lightProbeUsage = LightProbeUsage.Off;
        r.reflectionProbeUsage = ReflectionProbeUsage.Off;
        r.sortingOrder = 5;
        r.alignment = ParticleSystemRenderSpace.Local;

        var main = ps.main;
        main.loop = true;
        main.startLifetime = embers ? new ParticleSystem.MinMaxCurve(1.4f, 2.8f) : new ParticleSystem.MinMaxCurve(0.55f, 0.95f);
        main.startSpeed = embers ? new ParticleSystem.MinMaxCurve(1.2f, 2.6f) : new ParticleSystem.MinMaxCurve(0.5f, 1.3f);
        main.startSize = embers ? new ParticleSystem.MinMaxCurve(0.05f, 0.10f) : new ParticleSystem.MinMaxCurve(0.75f, 1.15f);
        main.startRotation = new ParticleSystem.MinMaxCurve(-0.12f, 0.12f);
        main.gravityModifier = embers ? -0.02f : -0.06f;
        main.simulationSpace = ParticleSystemSimulationSpace.World;
        main.maxParticles = embers ? 60 : 90;
        main.scalingMode = ParticleSystemScalingMode.Hierarchy;

        var g = new Gradient();
        if (embers)
        {
            g.SetKeys(new[] { new GradientColorKey(new Color(1f, 0.95f, 0.7f), 0f), new GradientColorKey(color, 0.4f), new GradientColorKey(color * 0.5f, 1f) },
                      new[] { new GradientAlphaKey(1f, 0f), new GradientAlphaKey(1f, 0.55f), new GradientAlphaKey(0f, 1f) });
        }
        else
        {
            g.SetKeys(new[] { new GradientColorKey(new Color(1f, 1f, 0.9f), 0f), new GradientColorKey(color, 0.45f), new GradientColorKey(color * 0.45f, 1f) },
                      new[] { new GradientAlphaKey(0.9f, 0f), new GradientAlphaKey(0.75f, 0.4f), new GradientAlphaKey(0f, 1f) });
        }
        var col = ps.colorOverLifetime; col.enabled = true; col.color = new ParticleSystem.MinMaxGradient(g);
        var sol = ps.sizeOverLifetime; sol.enabled = true;
        sol.size = embers ? new ParticleSystem.MinMaxCurve(1f, AnimationCurve.Linear(0f, 1f, 1f, 0.2f))
                          : new ParticleSystem.MinMaxCurve(1f, FxAssets.Curve(0.45f, 1.25f));

        var em = ps.emission;
        em.rateOverTime = embers ? 22f : 48f;
        var shape = ps.shape;
        shape.shapeType = ParticleSystemShapeType.Cone;
        shape.angle = embers ? 16f : 9f;
        shape.radius = embers ? 0.12f : 0.2f;
        shape.rotation = new Vector3(-90f, 0f, 0f);
        var vol = ps.velocityOverLifetime; vol.enabled = true; vol.space = ParticleSystemSimulationSpace.World;
        vol.x = embers ? new ParticleSystem.MinMaxCurve(-0.25f, 0.25f) : new ParticleSystem.MinMaxCurve(-0.12f, 0.12f);
        vol.y = new ParticleSystem.MinMaxCurve(0f, embers ? 0.9f : 0.5f);
        vol.z = embers ? new ParticleSystem.MinMaxCurve(-0.25f, 0.25f) : new ParticleSystem.MinMaxCurve(-0.12f, 0.12f);

        go.SetActive(true);
        return ps;
    }
}
}
