using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria
{
    /// <summary>
    /// Spell effects. <see cref="SpawnSpell"/> picks the effect for a spell; each effect is a small
    /// component that owns its lifetime, its damage moment and its cleanup.
    /// </summary>
    public class SpellVfxExecutor : MonoBehaviour
    {
        public static void SpawnSpell(
            SpellDef def, GameObject caster, Vector3 origin, Vector3 dir, Vector3 target,
            float dmg, float powerScale, float speedScale, float controlScale)
        {
            FxAssets.Ensure();
            dir = dir.sqrMagnitude > 0.01f ? dir.normalized : Vector3.forward;
            float radius = def.BlastRadius * Mathf.Lerp(1f, 1.35f, (powerScale - 1f) * 0.5f);
            SpellCasterHerald(def, caster, origin, dir, target);
            switch (def.Id)
            {
                case SpellId.SunStrike:      SunStrike.Begin(target, radius, dmg, caster, powerScale); break;
                case SpellId.ChaosMeteor:    ChaosMeteor.Begin(target, dir, radius, dmg, caster, speedScale); break;
                case SpellId.StormAlacrity:  StormAlacrity.Begin(caster, origin, radius, dmg, powerScale); break;
                case SpellId.StormEMP:       Nova.Begin(origin + Vector3.up * 0.6f, radius, dmg, caster, OrbType.Wex, powerScale); break;
                case SpellId.ColdSnap:       SpellProjectile.Begin(origin, dir, target, radius, dmg, caster, speedScale, SpellId.ColdSnap); break;
                case SpellId.ChronoBlink:    ChronoBlink.Begin(caster, origin, dir, radius, dmg, controlScale); break;
                case SpellId.IcePrismWall:   IcePrismWall.Begin(target, dir, radius, dmg, caster, controlScale); break;
                case SpellId.GravityTornado: SpellProjectile.Begin(origin, dir, target, radius, dmg, caster, speedScale, SpellId.GravityTornado); break;
                case SpellId.EmberConduit:   Nova.Begin(target, radius, dmg, caster, OrbType.Exort, powerScale); break;
                case SpellId.PrimalBlast:    PrimalBlast.Begin(caster, origin, dir, radius, dmg, powerScale); break;
            }
        }

        /// <summary>The caster's body and the world both answer the cast.</summary>
        private static void SpellCasterHerald(SpellDef def, GameObject caster, Vector3 origin, Vector3 dir, Vector3 target)
        {
            caster?.GetComponent<HeroController>()?.PlayCastAnimation();
            AetheriaFx.FlashLight(origin, def.PrimaryColor, 22f, 9f, 0.35f);
            AetheriaFx.Flare(origin + dir * 0.5f, def.PrimaryColor, 1.8f, 0.25f);
            AudioAndQualityService.Instance?.PlaySfxAt(def.Id == SpellId.ColdSnap || def.Id == SpellId.IcePrismWall ? "SFX_ChronoBlink" : "SW_MagicCast_FireBall1_1", origin, 0.75f);
        }

        // ------------------------------------------------------------------ shared damage + scorch

        public static void DamageBurst(Vector3 center, float radius, float dmg, GameObject caster, OrbType element, bool pullIn, float upward, bool sunStrike, bool meteor, float dmgMult = 1f)
        {
            AetheriaWorldGenerator.Instance?.RegisterSpellTerrainScar(center, radius, element);
            foreach (var col in Physics.OverlapSphere(center, radius))
            {
                if (caster != null && col.gameObject == caster) continue;
                var enemy = col.GetComponentInParent<EnemyAndBossAI>();
                if (enemy != null)
                {
                    Vector3 away = enemy.transform.position - center; away.y = 0f;
                    Vector3 push = pullIn ? -away.normalized : (away.sqrMagnitude > 0.01f ? away.normalized : Vector3.up);
                    enemy.TakeDamage(dmg * dmgMult, caster, push * 8f + Vector3.up * upward, element);
                }
                var prop = col.GetComponentInParent<BreakableProp>();
                if (prop != null) prop.ApplyDamage(dmg * 1.6f * dmgMult, center, sunStrike, meteor);
            }
        }

        /// <summary>Point on the terrain surface under a world position.</summary>
        public static Vector3 OnGround(Vector3 p)
        {
            p.y = AetheriaWorld.SampleHeight(p) + 0.12f;
            return p;
        }

        /// <summary>Builds a glowing primitive (mesh + material, no collider) for meteors, shards and crystals.</summary>
        public static GameObject GlowShape(Mesh mesh, Color color, float emissive, Vector3 scale, Vector3 pos, Transform parent = null)
        {
            var go = new GameObject("FxShape");
            if (parent != null) go.transform.SetParent(parent, true);
            go.transform.position = pos;
            go.transform.localScale = scale;
            go.AddComponent<MeshFilter>().sharedMesh = mesh;
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterial = AetheriaRender.Glow(color, emissive);
            mr.shadowCastingMode = ShadowCastingMode.Off;
            return go;
        }

        // ------------------------------------------------------------------ EEE: Solar Cataclysm
        private class SunStrike : MonoBehaviour
        {
            private Vector3 _target; private float _radius, _dmg, _power; private GameObject _caster; private float _t;
            private const float Delay = 0.22f;
            private static Color Fire => new Color(1f, 0.56f, 0.12f);

            public static void Begin(Vector3 target, float radius, float dmg, GameObject caster, float power)
            {
                var go = new GameObject("Vfx_SunStrike");
                go.transform.position = target;
                var s = go.AddComponent<SunStrike>();
                s._target = target; s._radius = radius; s._dmg = dmg; s._caster = caster; s._power = power;
            }

            private void Start()
            {
                // Warning: a ring of fire tightens on the target
                AetheriaFx.Shockwave(SpellVfxExecutor.OnGround(_target), Fire, _radius * 1.5f, Delay + 0.05f);
                AetheriaFx.FlashLight(_target + Vector3.up * 2f, Fire, 16f, 10f, Delay + 0.1f);
            }

            private void Update()
            {
                _t += Time.deltaTime;
                if (_t < Delay) return;

                Vector3 g = SpellVfxExecutor.OnGround(_target);
                float beamRadius = Mathf.Lerp(1.6f, 2.6f, Mathf.Clamp01(_power - 0.5f));
                AetheriaFx.Beam(g, Fire, beamRadius, 120f, 1.05f, 90f * _power);
                AetheriaFx.SpherePulse(g + Vector3.up * 1.2f, new Color(1f, 0.9f, 0.6f), _radius * 0.8f, 0.5f, 0.7f);
                AetheriaFx.Shockwave(g, Fire, _radius * 2.2f, 0.75f);
                AetheriaFx.Shockwave(g + Vector3.up * 0.5f, new Color(1f, 0.82f, 0.4f), _radius * 1.5f, 0.5f);
                AetheriaFx.Burst(g + Vector3.up * 0.6f, new Color(1f, 0.92f, 0.6f), 120, 5f, 17f, 0.12f, 0.34f, 0.9f, 0.6f, 1.6f, new Color(0.9f, 0.3f, 0.05f, 0f));
                AetheriaFx.Burst(g + Vector3.up * 3f, Fire, 70, 1.5f, 7f, 0.14f, 0.36f, 1.5f, 0.1f, 1.0f, new Color(0.5f, 0.08f, 0.02f, 0f));
                AetheriaFx.Puff(g + Vector3.up * 1.2f, new Color(0.42f, 0.24f, 0.14f, 0.7f), 22, 5f, 3.0f, 1.9f, 1.1f);
                AetheriaFx.Debris(g, new Color(0.22f, 0.14f, 0.08f), 14, g, 15f, 0.30f, false);
                AetheriaFx.Debris(g, Fire, 12, g, 13f, 0.16f, true);
                AetheriaFx.Flare(g + Vector3.up * 2f, new Color(1f, 0.93f, 0.7f), 9f, 0.45f);
                AetheriaFx.Shake(0.55f, 0.6f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_Fireball_Boom", g, 1f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_ChaosMeteor_Impact", g, 0.8f);
                AudioAndQualityService.Instance?.RequestHitStop(0.06f, 0.05f);
                SpellVfxExecutor.DamageBurst(g, _radius, _dmg, _caster, OrbType.Exort, false, 6.5f, true, false);
                Destroy(gameObject);
            }
        }

        // ------------------------------------------------------------------ EEW: Chaos Meteor
        private class ChaosMeteor : MonoBehaviour
        {
            private Vector3 _dir; private float _radius, _dmg, _speed; private GameObject _caster; private Transform _rock;
            private Light _light; private float _trail, _smoke, _roll, _tick; private bool _landed; private bool _done;
            private static readonly Vector3 FallVelocity = new Vector3(0f, -30f, 0f);
            private static Color Fire => new Color(1f, 0.36f, 0.06f);

            public static void Begin(Vector3 target, Vector3 dir, float radius, float dmg, GameObject caster, float speed)
            {
                var go = new GameObject("Vfx_ChaosMeteor");
                go.transform.position = target - dir * 9f + Vector3.up * 30f;
                var m = go.AddComponent<ChaosMeteor>();
                m._dir = dir; m._radius = radius; m._dmg = dmg; m._caster = caster; m._speed = speed;
                m._rock = SpellVfxExecutor.GlowShape(FxAssets.Icosphere, new Color(1f, 0.5f, 0.1f), 7f, Vector3.one * 4.6f, go.transform.position, go.transform).transform;
                m._light = AetheriaRender.CreatePointLight(go.transform, Vector3.zero, Fire, 30f, 70f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_ChaosMeteor", go.transform.position, 0.9f);
            }

            private void Update()
            {
                float dt = Time.deltaTime;
                Vector3 vel = _landed ? _dir * (11.5f * _speed) : FallVelocity + _dir * 9f;
                Vector3 next = transform.position + vel * dt;
                float ground = AetheriaWorld.SampleHeight(next) + 1.6f;

                _trail += dt; _smoke += dt;
                if (_trail > 0.025f) { _trail = 0f; AetheriaFx.Burst(transform.position, Fire, 4, 0.5f, 3f, 0.5f, 1.1f, 0.5f, _landed ? 0.3f : 0.9f, new Color(0.6f, 0.08f, 0.01f, 0f)); }
                if (_smoke > 0.09f) { _smoke = 0f; AetheriaFx.Puff(transform.position + Vector3.up * 0.8f, new Color(0.30f, 0.20f, 0.16f, 0.7f), 2, 1.5f, 2.0f, 1.8f, 1.0f); }
                _rock.Rotate(Vector3.one * 260f * dt, Space.Self);

                if (!_landed)
                {
                    if (next.y <= ground) Impact();
                    else transform.position = next;
                    return;
                }

                // Rolling: leaves a burning trail and grinds whatever it rolls over
                next.y = ground;
                transform.position = next;
                _roll += dt; _tick += dt;
                if (_tick > 0.32f)
                {
                    _tick = 0f;
                    AetheriaFx.GroundRing(SpellVfxExecutor.OnGround(transform.position), Fire, 2.4f, 7f, 0.3f);
                    AetheriaFx.Burst(SpellVfxExecutor.OnGround(transform.position) + Vector3.up * 0.4f, Fire, 16, 2f, 6f, 0.10f, 0.24f, 0.9f, 0.4f, 1.6f, new Color(0.6f, 0.1f, 0.01f, 0f));
                    AetheriaFx.Shake(0.08f, 0.2f);
                    SpellVfxExecutor.DamageBurst(transform.position, _radius, _dmg, _caster, OrbType.Exort, false, 2.5f, false, true, 0.35f);
                }
                if (_roll > 2.3f && !_done)
                {
                    _done = true;
                    Vector3 g = SpellVfxExecutor.OnGround(transform.position);
                    AetheriaFx.Burst(g + Vector3.up, Fire, 70, 4f, 12f, 0.10f, 0.28f, 0.8f, 0.6f, 1.8f, new Color(0.6f, 0.1f, 0.01f, 0f));
                    AetheriaFx.Puff(g + Vector3.up, new Color(0.32f, 0.22f, 0.18f, 0.75f), 14, 3f, 2.6f, 2.0f, 0.8f);
                    AetheriaFx.Debris(g + Vector3.up, new Color(0.20f, 0.11f, 0.06f), 12, g + Vector3.up, 8f, 0.26f, false);
                    Destroy(_rock.gameObject);
                    Destroy(gameObject, 0.1f);
                }
            }

            private void Impact()
            {
                _landed = true;
                Vector3 g = SpellVfxExecutor.OnGround(transform.position);
                transform.position = g + Vector3.up * 1.6f;
                AetheriaFx.SpherePulse(g + Vector3.up * 1.5f, Fire, _radius, 0.6f, 0.6f);
                AetheriaFx.Shockwave(g, Fire, _radius * 2.0f, 0.7f);
                AetheriaFx.Shockwave(g + Vector3.up * 0.4f, new Color(1f, 0.8f, 0.3f), _radius * 1.4f, 0.5f);
                AetheriaFx.Burst(g + Vector3.up * 1f, new Color(1f, 0.86f, 0.5f), 130, 6f, 20f, 0.12f, 0.36f, 1.0f, 0.5f, 1.4f, new Color(0.9f, 0.25f, 0.03f, 0f));
                AetheriaFx.Puff(g + Vector3.up * 1.5f, new Color(0.40f, 0.22f, 0.14f, 0.8f), 26, 6f, 3.6f, 2.2f, 1.0f);
                AetheriaFx.Debris(g + Vector3.up, new Color(0.20f, 0.11f, 0.06f), 20, g + Vector3.up * 0.5f, 16f, 0.34f, false);
                AetheriaFx.Debris(g + Vector3.up, Fire, 16, g + Vector3.up * 0.5f, 14f, 0.20f, true);
                AetheriaFx.Flare(g + Vector3.up * 2f, new Color(1f, 0.85f, 0.5f), 8f, 0.4f);
                AetheriaFx.FlashLight(g + Vector3.up * 3f, Fire, 110f, 34f, 0.9f);
                AetheriaFx.Shake(0.6f, 0.7f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_ChaosMeteor_Impact", g, 1f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_Fireball_Boom", g, 0.9f);
                AudioAndQualityService.Instance?.RequestHitStop(0.07f, 0.04f);
                SpellVfxExecutor.DamageBurst(g, _radius, _dmg, _caster, OrbType.Exort, false, 7f, false, true);
            }
        }

        // ------------------------------------------------------------------ QQQ / QWW: shard and tornado projectiles
        private class SpellProjectile : MonoBehaviour
        {
            private Vector3 _dir, _velocity; private float _radius, _dmg, _speed, _life, _trail; private GameObject _caster; private SpellId _id;
            private Transform _shape;
            private bool _pull;
            private float _lifted;

            public static void Begin(Vector3 origin, Vector3 dir, Vector3 target, float radius, float dmg, GameObject caster, float speed, SpellId id)
            {
                var go = new GameObject("Vfx_" + id);
                Vector3 start = origin + dir * 1.4f;
                go.transform.position = start;
                var p = go.AddComponent<SpellProjectile>();
                p._dir = dir; p._radius = radius; p._dmg = dmg; p._caster = caster; p._speed = speed; p._id = id;
                p._pull = id == SpellId.GravityTornado;
                Color col = id == SpellId.ColdSnap ? new Color(0.35f, 0.92f, 1f) : new Color(0.62f, 0.58f, 1f);
                if (id == SpellId.ColdSnap)
                {
                    p._velocity = dir * (26f * speed);
                    p._shape = SpellVfxExecutor.GlowShape(FxAssets.Icosphere, col, 9f, new Vector3(0.5f, 1.5f, 0.5f), start, go.transform).transform;
                    p._shape.rotation = Quaternion.LookRotation(dir) * Quaternion.Euler(90f, 0f, 0f);
                    p._life = 1.4f;
                }
                else
                {
                    p._velocity = dir * (9f * speed);
                    p._shape = SpellVfxExecutor.GlowShape(FxAssets.Icosphere, col, 2.4f, Vector3.one * 1.8f, start, go.transform).transform;
                    p._life = 1.7f;
                }
                AetheriaRender.CreatePointLight(go.transform, Vector3.zero, col, 14f, 22f);
            }

            private void Update()
            {
                float dt = Time.deltaTime;
                _life -= dt;
                Vector3 next = transform.position + _velocity * dt;
                float ground = AetheriaWorld.SampleHeight(next) + (_pull ? 2.0f : 0.6f);
                if (next.y < ground)
                {
                    if (_pull) next.y = ground; else { transform.position = next; Detonate(); return; }
                }
                transform.position = next;
                if (_shape != null && !_pull) _shape.Rotate(Vector3.up * 720f * dt, Space.Self);

                _trail += dt;
                if (_trail > (_pull ? 0.035f : 0.025f))
                {
                    _trail = 0f;
                    Color col = _pull ? new Color(0.62f, 0.58f, 1f) : new Color(0.55f, 0.95f, 1f);
                    if (_pull)
                    {
                        // Spiral of wind: sparks shot sideways at the rim, rising
                        float a = Time.time * 22f;
                        for (int k = 0; k < 3; k++)
                        {
                            float ang = a + k * 2.094f;
                            Vector3 rim = transform.position + new Vector3(Mathf.Cos(ang), 0f, Mathf.Sin(ang)) * 2.2f - Vector3.up * 1.6f;
                            Vector3 tangent = new Vector3(-Mathf.Sin(ang), 0.6f, Mathf.Cos(ang));
                            AetheriaFx.Burst(rim, col, 2, 2f, 5f, 0.14f, 0.30f, 0.45f, -0.1f, new Color(0.3f, 0.4f, 1f, 0f), true, tangent, 0.9f);
                        }
                        AetheriaFx.GroundRing(SpellVfxExecutor.OnGround(transform.position), col, 2.6f, 0.25f, 0.2f);
                        _lifted -= dt;
                        if (_lifted <= 0f)
                        {
                            _lifted = 0.18f;
                            foreach (var c in Physics.OverlapSphere(transform.position, _radius * 0.8f))
                            {
                                var enemy = c.GetComponentInParent<EnemyAndBossAI>();
                                if (enemy != null && c.gameObject != _caster)
                                    enemy.TakeDamage(_dmg * 0.16f, _caster, (transform.position - enemy.transform.position).normalized * 6f + Vector3.up * 9f, OrbType.Wex);
                            }
                        }
                    }
                    else
                    {
                        AetheriaFx.Burst(transform.position, col, 3, 0.3f, 1.6f, 0.10f, 0.22f, 0.35f, 0.2f, 1.8f, new Color(0.2f, 0.5f, 1f, 0f));
                    }
                }

                if (!_pull && _life > 0f)
                {
                    foreach (var c in Physics.OverlapSphere(transform.position, 1.6f))
                    {
                        if (_caster != null && c.gameObject == _caster) continue;
                        if (c.GetComponentInParent<EnemyAndBossAI>() != null || c.GetComponentInParent<BreakableProp>() != null) { Detonate(); return; }
                    }
                }
                if (_life <= 0f) Detonate();
            }

            private void Detonate()
            {
                Vector3 at = transform.position;
                if (_pull)
                {
                    Color col = new Color(0.62f, 0.58f, 1f);
                    AetheriaFx.SpherePulse(at, col, _radius, 0.5f, 0.45f);
                    AetheriaFx.Shockwave(SpellVfxExecutor.OnGround(at), col, _radius * 1.8f, 0.6f);
                    AetheriaFx.Burst(at, col, 90, 4f, 13f, 0.12f, 0.30f, 0.9f, 0.4f, new Color(0.3f, 0.3f, 0.9f, 0f));
                    AetheriaFx.FlashLight(at, col, 50f, 22f, 0.5f);
                    AetheriaFx.Shake(0.25f, 0.35f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_EyeOfTheStorm", at, 0.9f);
                    SpellVfxExecutor.DamageBurst(at, _radius, _dmg, _caster, OrbType.Wex, false, 9f, false, false);
                }
                else
                {
                    Color ice = new Color(0.45f, 0.94f, 1f);
                    AetheriaFx.SpherePulse(at, ice, _radius, 0.45f, 0.5f);
                    AetheriaFx.Shockwave(SpellVfxExecutor.OnGround(at), ice, _radius * 1.9f, 0.55f);
                    AetheriaFx.Burst(at, new Color(0.85f, 0.98f, 1f), 80, 4f, 13f, 0.08f, 0.22f, 0.7f, 0.5f, new Color(0.15f, 0.55f, 1f, 0f));
                    AetheriaFx.Flare(at, ice, 5f, 0.3f);
                    AetheriaFx.FlashLight(at, ice, 46f, 20f, 0.5f);
                    AetheriaFx.Shake(0.2f, 0.3f);
                    SpellVfxExecutor.IceSpikes(SpellVfxExecutor.OnGround(at), _radius * 0.8f, 9, 1.9f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_Chronosphere", at, 0.8f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_Hit_Shield_Absorb", at, 0.6f);
                    SpellVfxExecutor.DamageBurst(at, _radius, _dmg, _caster, OrbType.Quas, false, 4f, false, false);
                }
                if (_shape != null) Destroy(_shape.gameObject);
                Destroy(gameObject);
            }
        }

        /// <summary>Crystals bursting out of the ground around a point; they sink back after a moment.</summary>
        public static void IceSpikes(Vector3 ground, float ringRadius, int count, float height)
        {
            for (int i = 0; i < count; i++)
            {
                float a = i / (float)count * Mathf.PI * 2f + Random.value * 0.5f;
                float r = i == 0 ? 0f : ringRadius * Random.Range(0.45f, 1f);
                Vector3 p = ground + new Vector3(Mathf.Cos(a) * r, 0f, Mathf.Sin(a) * r);
                p.y = AetheriaWorld.SampleHeight(p);
                float h = height * Random.Range(0.6f, 1.2f);
                var shard = SpellVfxExecutor.GlowShape(FxAssets.Icosphere, new Color(0.55f, 0.94f, 1f), 5f, new Vector3(0.34f, h * 0.5f, 0.34f), p + Vector3.up * (h * 0.5f));
                shard.transform.rotation = Quaternion.Euler(Random.Range(-22f, 22f), Random.value * 360f, Random.Range(-22f, 22f));
                var rise = shard.AddComponent<FxRiseSink>();
                rise.Duration = 2.6f;
            }
            AetheriaFx.Puff(ground + Vector3.up * 0.4f, new Color(0.85f, 0.97f, 1f, 0.6f), 10, 2.5f, 1.2f, 0.9f, 0.6f);
        }

        // ------------------------------------------------------------------ QQE: Cryo-Prism Wall
        private class IcePrismWall : MonoBehaviour
        {
            public static void Begin(Vector3 target, Vector3 dir, float radius, float dmg, GameObject caster, float control)
            {
                Vector3 g = SpellVfxExecutor.OnGround(target);
                Vector3 side = Vector3.Cross(Vector3.up, dir).normalized;
                var go = new GameObject("Vfx_IcePrismWall");
                go.transform.position = g;
                int count = 7;
                float spacing = 1.15f;
                Color ice = new Color(0.50f, 0.94f, 1f);
                for (int i = 0; i < count; i++)
                {
                    float t = i - (count - 1) * 0.5f;
                    Vector3 p = g + side * (t * spacing) + dir * (Mathf.Abs(t) * -0.18f);
                    float h = (3.4f - Mathf.Abs(t) * 0.32f) * Mathf.Lerp(1f, 1.35f, control - 0.5f) * Random.Range(0.9f, 1.1f);
                    Vector3 pg = p; pg.y = AetheriaWorld.SampleHeight(p);
                    var prism = SpellVfxExecutor.GlowShape(FxAssets.Icosphere, ice, 4.5f, new Vector3(0.62f, h * 0.5f, 0.62f), pg + Vector3.up * (h * 0.5f), go.transform);
                    prism.transform.rotation = Quaternion.Euler(Random.Range(-9f, 9f), Random.value * 360f, Random.Range(-9f, 9f));
                    var rise = prism.AddComponent<FxRiseSink>();
                    rise.Duration = 5.0f;
                    rise.RiseSeconds = 0.22f;
                }
                var box = go.AddComponent<BoxCollider>();
                box.center = new Vector3(0f, 1.5f, 0f);
                box.size = new Vector3(count * spacing, 3f, 0.9f);
                go.transform.rotation = Quaternion.LookRotation(dir, Vector3.up);
                AetheriaRender.CreatePointLight(go.transform, Vector3.up * 1.8f, ice, 16f, 34f);
                AetheriaFx.Shockwave(g, ice, radius * 1.6f, 0.5f);
                AetheriaFx.Burst(g + Vector3.up * 1f, new Color(0.85f, 0.98f, 1f), 90, 3f, 12f, 0.10f, 0.26f, 0.8f, 0.4f, 1.4f, new Color(0.15f, 0.55f, 1f, 0f));
                AetheriaFx.Shake(0.18f, 0.3f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_Chronosphere", g, 0.9f);
                SpellVfxExecutor.DamageBurst(g + dir * 0.5f, radius, dmg, caster, OrbType.Quas, false, 2.5f, false, false);
                var life = go.AddComponent<FxDestroyAfter>();
                life.Seconds = 5.2f;
                life.OnExpire = () =>
                {
                    AetheriaFx.Burst(g + Vector3.up * 1.2f, ice, 70, 3f, 10f, 0.10f, 0.26f, 0.8f, 0.6f, 1.6f, new Color(0.15f, 0.5f, 1f, 0f));
                    AetheriaFx.Debris(g + Vector3.up * 1.2f, ice, 16, g, 8f, 0.16f, true);
                    AetheriaFx.FlashLight(g + Vector3.up * 1.5f, ice, 30f, 14f, 0.4f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_Hit_Stone", g, 0.6f);
                };
            }
        }

        // ------------------------------------------------------------------ WWW / QEE: novas
        private class Nova : MonoBehaviour
        {
            public static void Begin(Vector3 center, float radius, float dmg, GameObject caster, OrbType element, float power)
            {
                Color col = element == OrbType.Wex ? new Color(0.76f, 0.36f, 1f) : new Color(1f, 0.36f, 0.10f);
                AetheriaFx.SpherePulse(center, col, radius, 0.6f, 0.5f);
                AetheriaFx.Shockwave(SpellVfxExecutor.OnGround(center), col, radius * 2.0f, 0.65f);
                AetheriaFx.Shockwave(center + Vector3.up * 1.2f, col, radius * 1.3f, 0.5f, vertical: true, facing: Vector3.forward);
                AetheriaFx.Shockwave(center + Vector3.up * 1.2f, col, radius * 1.3f, 0.5f, vertical: true, facing: Vector3.right);
                AetheriaFx.Burst(center, Color.Lerp(col, Color.white, 0.55f), 150, 6f, 19f, 0.12f, 0.32f, 0.8f, 0.2f, 1.5f, col * 0.5f);
                AetheriaFx.Flare(center, col, 7f, 0.4f);
                AetheriaFx.FlashLight(center + Vector3.up * 2f, col, 90f * power, 30f, 0.7f);
                AetheriaFx.Shake(0.4f, 0.5f);
                if (element == OrbType.Wex)
                {
                    for (int i = 0; i < 9; i++)
                        AetheriaFx.Lightning(center, center + Random.onUnitSphere * radius * Random.Range(0.7f, 1.1f) + Vector3.up, col, 0.3f, 0.13f, 12, 1.4f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_ChainLightning", center, 1f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_EyeOfTheStorm", center, 0.8f);
                }
                else
                {
                    IceOrFireColumn(center, radius, power);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_Macropyre", center, 0.95f);
                }
                AudioAndQualityService.Instance?.RequestHitStop(0.05f, 0.05f);
                SpellVfxExecutor.DamageBurst(center, radius, dmg, caster, element, false, 5.5f, false, false);
            }

            /// <summary>Lava conduit: a column of fire with three rising shockwaves and a bed of embers.</summary>
            private static void IceOrFireColumn(Vector3 center, float radius, float power)
            {
                var go = new GameObject("Vfx_LavaConduit");
                go.transform.position = SpellVfxExecutor.OnGround(center);
                go.AddComponent<FireColumn>().Radius = radius;
                AetheriaFx.Beam(SpellVfxExecutor.OnGround(center), new Color(1f, 0.42f, 0.08f), 1.1f * power, 12f, 2.4f, 70f);
            }
        }

        /// <summary>Keeps a lava conduit alive for a while: rising embers and waves of heat.</summary>
        public class FireColumn : MonoBehaviour
        {
            public float Radius = 5f;
            private float _t, _ember, _wave;
            private Light _light;

            private void Start()
            {
                _light = AetheriaRender.CreatePointLight(transform, Vector3.up * 2f, new Color(1f, 0.42f, 0.1f), 30f, 60f);
                AetheriaFx.GroundRing(transform.position, new Color(1f, 0.42f, 0.08f), Radius * 0.8f, 3.5f, 0.5f);
            }

            private void Update()
            {
                _t += Time.deltaTime; _ember += Time.deltaTime; _wave += Time.deltaTime;
                AetheriaRender.StyleFxLight(_light, new Color(1f, 0.42f, 0.1f), 30f, 60f * (1f - _t / 3.5f) * (0.85f + 0.15f * Mathf.Sin(_t * 31f)));
                if (_ember > 0.05f)
                {
                    _ember = 0f;
                    Vector3 p = transform.position + Random.insideUnitSphere * Radius * 0.6f; p.y = transform.position.y;
                    AetheriaFx.Burst(p, new Color(1f, 0.5f, 0.1f), 2, 2.5f, 6.5f, 0.12f, 0.28f, 1.3f, -0.15f, 0.9f, new Color(0.7f, 0.08f, 0.01f, 0f));
                }
                if (_wave > 0.45f && _t < 2.2f)
                {
                    _wave = 0f;
                    AetheriaFx.Shockwave(transform.position, new Color(1f, 0.5f, 0.12f), Radius * 1.7f, 0.6f);
                }
                if (_t > 3.5f) Destroy(gameObject);
            }
        }

        // ------------------------------------------------------------------ EWW: Tempest Surge (buff aura)
        private class StormAlacrity : MonoBehaviour
        {
            public static void Begin(GameObject caster, Vector3 origin, float radius, float dmg, float power)
        {
            Color gold = new Color(1f, 0.86f, 0.30f);
            Color storm = new Color(0.62f, 0.52f, 1f);
            for (int i = 0; i < 3; i++)
            {
                AetheriaFx.Shockwave(origin + Vector3.up * (0.3f + i * 0.7f), i % 2 == 0 ? gold : storm, radius * (1.3f - i * 0.2f), 0.5f, vertical: true);
            }
            AetheriaFx.Burst(origin, gold, 70, 3.5f, 12f, 0.10f, 0.26f, 0.9f, 0.2f, 1.8f, new Color(0.9f, 0.5f, 0.1f, 0f));
            for (int i = 0; i < 8; i++)
                AetheriaFx.Lightning(origin + Vector3.up * 1.2f, origin + Random.onUnitSphere * 2.4f + Vector3.up, storm, 0.26f, 0.10f, 8, 0.8f);
            AetheriaFx.FlashLight(origin + Vector3.up * 1.2f, gold, 50f, 18f, 0.5f);
            AetheriaFx.Shake(0.14f, 0.25f);
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_ChainLightning", origin, 0.7f);
            caster?.GetComponent<HeroController>()?.ApplyAlacrityBuff(4.5f, 1.32f);
            if (caster != null)
            {
                var aura = new GameObject("TempestSurgeAura");
                aura.transform.SetParent(caster.transform, false);
                var a = aura.AddComponent<TempestAura>();
                a.Duration = 4.5f;
                a.ColorA = gold; a.ColorB = storm;
            }
            SpellVfxExecutor.DamageBurst(origin, radius, dmg, caster, OrbType.Wex, false, 3.5f, false, false);
        }
    }
    }

    /// <summary>Orbiting sparks and crackling arcs around the buffed hero.</summary>
    public class TempestAura : MonoBehaviour
    {
        public float Duration = 4.5f;
        public Color ColorA = Color.yellow, ColorB = Color.cyan;
        private float _t, _spark, _arc;

        private void Update()
        {
            _t += Time.deltaTime; _spark += Time.deltaTime; _arc += Time.deltaTime;
            if (_spark > 0.045f)
            {
                _spark = 0f;
                float a = Time.time * 9f;
                for (int k = 0; k < 2; k++)
                {
                    float ang = a + k * Mathf.PI;
                    Vector3 p = transform.position + new Vector3(Mathf.Cos(ang) * 0.75f, 0.35f + Mathf.PingPong(Time.time * 1.7f + k, 1.5f), Mathf.Sin(ang) * 0.75f);
                    AetheriaFx.Burst(p, k == 0 ? ColorA : ColorB, 2, 0.4f, 1.8f, 0.06f, 0.13f, 0.35f, -0.1f, 1.6f, new Color(ColorB.r * 0.5f, ColorB.g * 0.5f, ColorB.b, 0f));
                }
            }
            if (_arc > 0.5f)
            {
                _arc = 0f;
                Vector3 c = transform.position + Vector3.up * 1.2f;
                AetheriaFx.Lightning(c + Random.insideUnitSphere * 0.9f, c + Random.insideUnitSphere * 0.9f, ColorB, 0.16f, 0.06f, 6, 0.5f);
            }
            if (_t >= Duration) Destroy(gameObject);
        }
    }

    // ------------------------------------------------------------------ QQW: Chrono-Ghost Walk
    internal static class ChronoBlink
    {
        public static void Begin(GameObject caster, Vector3 origin, Vector3 dir, float radius, float dmg, float control)
        {
            Color cyan = new Color(0.40f, 0.92f, 1f);
            var hero = caster != null ? caster.GetComponent<HeroController>() : null;
            if (hero == null) return;

            Vector3 from = caster.transform.position;
            Vector3 to = from + dir * 11f * Mathf.Lerp(0.85f, 1.25f, control - 0.5f);
            to.y = AetheriaWorld.SampleHeight(to) + 0.3f;

            // Leave an echo behind that detonates
            AetheriaFx.Burst(from + Vector3.up * 1f, cyan, 60, 2.5f, 9f, 0.10f, 0.26f, 0.6f, 0.1f, 2.0f, new Color(0.1f, 0.4f, 1f, 0f));
            AetheriaFx.Puff(from + Vector3.up * 0.9f, new Color(0.55f, 0.9f, 1f, 0.55f), 12, 2f, 1.3f, 0.8f, 0.2f);
            AetheriaFx.Lightning(from + Vector3.up * 1.1f, to + Vector3.up * 1.1f, cyan, 0.28f, 0.34f, 6, 0.5f);

            hero.TeleportSafe(to);
            hero.SnapCameraBehindHero();

            AetheriaFx.FlashLight(to + Vector3.up * 1.2f, cyan, 60f, 20f, 0.5f);
            AetheriaFx.Flare(to + Vector3.up * 1.2f, new Color(0.85f, 1f, 1f), 4f, 0.3f);
            AetheriaFx.Shockwave(SpellVfxExecutor.OnGround(to), cyan, radius * 1.5f, 0.5f);
            AetheriaFx.Shockwave(to + Vector3.up * 1.1f, cyan, 2.2f, 0.4f, vertical: true);
            AetheriaFx.Burst(to + Vector3.up * 1f, new Color(0.85f, 1f, 1f), 70, 3.5f, 12f, 0.10f, 0.26f, 0.6f, 0.1f, 2.0f, new Color(0.1f, 0.4f, 1f, 0f));
            AetheriaFx.Shake(0.18f, 0.3f);
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_ChronoBlink", to, 0.95f);

            // The echo pops after a heartbeat
            var echo = new GameObject("Vfx_ChronoEcho");
            echo.transform.position = from;
            var d = echo.AddComponent<FxDestroyAfter>();
            d.Seconds = 0.28f;
            d.OnExpire = () =>
            {
                AetheriaFx.SpherePulse(from + Vector3.up * 1f, cyan, radius, 0.45f, 0.45f);
                AetheriaFx.Shockwave(SpellVfxExecutor.OnGround(from), cyan, radius * 1.8f, 0.5f);
                AetheriaFx.Burst(from + Vector3.up * 1f, cyan, 70, 4f, 13f, 0.10f, 0.26f, 0.7f, 0.4f, 1.6f, new Color(0.1f, 0.4f, 1f, 0f));
                AetheriaFx.FlashLight(from + Vector3.up * 1.2f, cyan, 44f, 18f, 0.4f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_Chronosphere", from, 0.7f);
                SpellVfxExecutor.DamageBurst(from + Vector3.up, radius, dmg, caster, OrbType.Quas, false, 4f, false, false);
            };
        }
    }

    // ------------------------------------------------------------------ QWE: Primal Deafening Blast
    internal static class PrimalBlast
    {
        public static void Begin(GameObject caster, Vector3 origin, Vector3 dir, float radius, float dmg, float power)
        {
            Color amber = new Color(1f, 0.58f, 0.16f), violet = new Color(0.70f, 0.42f, 1f), cyan = new Color(0.30f, 0.92f, 1f);
            Vector3 c = origin + dir * 1.2f;
            AetheriaFx.Shockwave(c, amber, radius * 2.2f, 0.6f, vertical: true, facing: dir);
            AetheriaFx.Shockwave(c + dir * 1.4f, violet, radius * 1.9f, 0.62f, vertical: true, facing: dir);
            AetheriaFx.Shockwave(c + dir * 2.8f, cyan, radius * 1.6f, 0.64f, vertical: true, facing: dir);
            AetheriaFx.Shockwave(SpellVfxExecutor.OnGround(c), amber, radius * 2.0f, 0.7f);
            AetheriaFx.Burst(c, Color.white, 190, 8f, 24f, 0.12f, 0.34f, 0.8f, 0.1f, 1.3f, violet, true, dir, 0.35f);
            AetheriaFx.Flare(c + dir * 1.5f, new Color(1f, 0.9f, 0.7f), 11f, 0.45f);
            AetheriaFx.FlashLight(c + dir * 2f, amber, 130f * power, 36f, 0.8f);
            for (int i = 0; i < 6; i++)
                AetheriaFx.Lightning(c, c + dir * Random.Range(4f, 9f) + Random.insideUnitSphere * 3f, i % 2 == 0 ? violet : cyan, 0.3f, 0.15f, 10, 1.2f);
            AetheriaFx.Shake(0.7f, 0.75f);
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_CataclysmNova", c, 1f);
            AudioAndQualityService.Instance?.RequestHitStop(0.09f, 0.03f);
            Vector3 hitCenter = c + dir * (radius * 0.7f);
            SpellVfxExecutor.DamageBurst(hitCenter, radius, dmg, caster, OrbType.Quas, false, 8f, false, false);
            SpellVfxExecutor.DamageBurst(hitCenter, radius * 0.9f, 1f, caster, OrbType.Exort, false, 0f, false, false, 0f);
            SpellVfxExecutor.DamageBurst(hitCenter, radius * 0.9f, 1f, caster, OrbType.Wex, false, 0f, false, false, 0f);
        }
    }

    // ------------------------------------------------------------------ small lifetime helpers

    /// <summary>Grows an object out of the ground, then sinks it back and removes it.</summary>
    public class FxRiseSink : MonoBehaviour
    {
        public float Duration = 2.5f;
        public float RiseSeconds = 0.12f;
        private Vector3 _fullScale, _basePos;
        private float _t;

        private void Start()
        {
            _fullScale = transform.localScale;
            _basePos = transform.position;
            transform.localScale = new Vector3(_fullScale.x, _fullScale.y * 0.05f, _fullScale.z);
        }

        private void Update()
        {
            _t += Time.deltaTime;
            float rise = Mathf.Clamp01(_t / Mathf.Max(0.02f, RiseSeconds));
            float sink = Mathf.Clamp01((_t - (Duration - 0.45f)) / 0.45f);
            float k = rise * (1f - sink);
            transform.localScale = new Vector3(_fullScale.x * (0.4f + 0.6f * rise), _fullScale.y * Mathf.Max(0.05f, k), _fullScale.z * (0.4f + 0.6f * rise));
            transform.position = _basePos - Vector3.up * (_fullScale.y * (1f - k) * 0.5f);
            if (_t >= Duration) Destroy(gameObject);
        }
    }

    /// <summary>Runs a callback, then destroys the object after a delay.</summary>
    public class FxDestroyAfter : MonoBehaviour
    {
        public float Seconds = 1f;
        public System.Action OnExpire;
        private bool _fired;

        private IEnumerator Start()
        {
            yield return new WaitForSeconds(Seconds);
            OnExpire?.Invoke();
            Destroy(gameObject);
        }
    }
}
