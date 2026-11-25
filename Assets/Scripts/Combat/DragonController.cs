using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria
{
    /// <summary>
    /// Rideable fire-breathing dragon Ignis: animated flight with banking, take-off and landing, a
    /// cone of dragonfire that scorches the ground, and fog-of-war reveal while airborne.
    /// </summary>
    public class DragonController : MonoBehaviour
    {
        public static DragonController Instance { get; private set; }

        public bool Mounted { get; private set; }
        public bool Airborne { get; private set; }
        public float FireEnergy = 100f;
        public float MaxFireEnergy = 100f;
        public bool BreathingFire { get; private set; }
        public bool ForceBreathForDemo = false;
        public float FlightSpeed { get; private set; } = 18f;
        public Transform SaddlePoint;
        public Transform MouthPoint;

        private HeroController _rider;
        private Light _mouthLight;
        private ParticleSystem _fire;
        private ParticleSystem _smoke;
        private ParticleSystem[] _wingTrails;
        private Animator _anim;
        private float _yaw, _pitch, _roll;
        private float _fireDamageAccum;
        private float _revealAccum;
        private float _bob;
        private float _screamCooldown;
        private const float CruiseSpeed = 22f;

        private static readonly int PFlying = Animator.StringToHash("Flying");
        private static readonly int PSpeed = Animator.StringToHash("Speed");
        private static readonly int TBreath = Animator.StringToHash("Breath");
        private static readonly int TScream = Animator.StringToHash("Scream");
        private static readonly int THit = Animator.StringToHash("Hit");

        private void Awake()
        {
            Instance = this;
            FxAssets.Ensure();
            _anim = GetComponentInChildren<Animator>(true);
            if (_anim != null)
            {
                if (_anim.runtimeAnimatorController == null || _anim.avatar == null || !_anim.avatar.isValid) _anim = null;
                else { _anim.applyRootMotion = false; _anim.cullingMode = AnimatorCullingMode.AlwaysAnimate; }
            }

            if (SaddlePoint == null)
            {
                Transform back = AetheriaPalette.FindBoneAny(transform, "Chest", "Spine02", "Spine01");
                var s = new GameObject("SaddlePoint");
                s.transform.SetParent(back != null ? back : transform, false);
                s.transform.localPosition = back != null ? new Vector3(0f, 0.55f, 0f) : new Vector3(0f, 2.2f, 0f);
                SaddlePoint = s.transform;
            }
            if (MouthPoint == null)
            {
                Transform head = AetheriaPalette.FindBoneAny(transform, "Head", "Jaw01");
                var m = new GameObject("DragonMouthAnchor");
                m.transform.SetParent(head != null ? head : transform, false);
                m.transform.localPosition = head != null ? new Vector3(0f, 0.1f, 0.9f) : new Vector3(0f, 1.8f, 3.2f);
                MouthPoint = m.transform;
            }
            SetupBreath();
        }

        private void SetupBreath()
        {
            // Undo the dragon's scale so fire and light sizes are in metres
            Vector3 inv = Vector3.one / Mathf.Max(0.05f, Mathf.Max(MouthPoint.lossyScale.x, Mathf.Max(MouthPoint.lossyScale.y, MouthPoint.lossyScale.z)));
            MouthPoint.localScale = inv;

            _mouthLight = AetheriaRender.CreatePointLight(MouthPoint, Vector3.zero, new Color(1f, 0.45f, 0.12f), 26f, 0f);

            var root = new GameObject("BreathRoot").transform;
            root.SetParent(MouthPoint, false);
            _fire = MakeBreath(root, false, 240f);
            _smoke = MakeBreath(root, true, 34f);

            // Wingtip trails
            _wingTrails = new ParticleSystem[2];
            for (int i = 0; i < 2; i++)
            {
                Transform tip = AetheriaPalette.FindBoneAny(transform, i == 0 ? "Wing03_L" : "Wing03_R", i == 0 ? "Wing02_L" : "Wing02_R");
                if (tip == null) continue;
                var go = new GameObject("WingTrail");
                go.transform.SetParent(tip, false);
                go.transform.localScale = inv;
                var ps = go.AddComponent<ParticleSystem>();
                var r = ps.GetComponent<ParticleSystemRenderer>();
                r.sharedMaterial = AetheriaRender.Particle(new Color(1f, 0.55f, 0.35f, 0.35f), true, FxAssets.Glow);
                r.shadowCastingMode = ShadowCastingMode.Off;
                var main = ps.main;
                main.startLifetime = 0.7f; main.startSpeed = 0f; main.startSize = new ParticleSystem.MinMaxCurve(0.5f, 0.9f);
                main.simulationSpace = ParticleSystemSimulationSpace.World; main.loop = true; main.maxParticles = 60;
                var em = ps.emission; em.rateOverTime = 46f;
                var col = ps.colorOverLifetime; col.enabled = true;
                var g = new Gradient();
                g.SetKeys(new[] { new GradientColorKey(new Color(1f, 0.75f, 0.5f), 0f), new GradientColorKey(new Color(0.9f, 0.25f, 0.1f), 1f) },
                          new[] { new GradientAlphaKey(0.5f, 0f), new GradientAlphaKey(0f, 1f) });
                col.color = new ParticleSystem.MinMaxGradient(g);
                ps.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
                _wingTrails[i] = ps;
            }
        }

        private static ParticleSystem MakeBreath(Transform parent, bool smoke, float rate)
        {
            var go = new GameObject(smoke ? "BreathSmoke" : "BreathFire");
            go.transform.SetParent(parent, false);
            var ps = go.AddComponent<ParticleSystem>();
            var r = ps.GetComponent<ParticleSystemRenderer>();
            r.sharedMaterial = AetheriaRender.Particle(Color.white, !smoke, smoke ? FxAssets.Smoke : FxAssets.Flame);
            r.shadowCastingMode = ShadowCastingMode.Off;
            r.receiveShadows = false;
            r.sortingOrder = smoke ? 1 : 6;
            r.alignment = ParticleSystemRenderSpace.Local;

            var main = ps.main;
            main.loop = true;
            main.startLifetime = smoke ? new ParticleSystem.MinMaxCurve(1.2f, 2.0f) : new ParticleSystem.MinMaxCurve(0.35f, 0.7f);
            main.startSpeed = smoke ? new ParticleSystem.MinMaxCurve(10f, 15f) : new ParticleSystem.MinMaxCurve(20f, 28f);
            main.startSize = smoke ? new ParticleSystem.MinMaxCurve(1.4f, 2.6f) : new ParticleSystem.MinMaxCurve(1.0f, 1.9f);
            main.startRotation = new ParticleSystem.MinMaxCurve(-0.4f, 0.4f);
            main.gravityModifier = smoke ? 0.02f : 0.08f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.maxParticles = smoke ? 120 : 400;
            main.scalingMode = ParticleSystemScalingMode.Hierarchy;

            var g = new Gradient();
            if (smoke)
            {
                g.SetKeys(new[] { new GradientColorKey(new Color(0.35f, 0.22f, 0.18f), 0f), new GradientColorKey(new Color(0.18f, 0.14f, 0.14f), 1f) },
                          new[] { new GradientAlphaKey(0f, 0f), new GradientAlphaKey(0.55f, 0.2f), new GradientAlphaKey(0f, 1f) });
            }
            else
            {
                g.SetKeys(new[] { new GradientColorKey(new Color(1f, 0.96f, 0.75f), 0f), new GradientColorKey(new Color(1f, 0.62f, 0.18f), 0.35f),
                                  new GradientColorKey(new Color(0.95f, 0.25f, 0.05f), 0.7f), new GradientColorKey(new Color(0.4f, 0.05f, 0.02f), 1f) },
                          new[] { new GradientAlphaKey(1f, 0f), new GradientAlphaKey(0.95f, 0.5f), new GradientAlphaKey(0f, 1f) });
            }
            var col = ps.colorOverLifetime; col.enabled = true; col.color = new ParticleSystem.MinMaxGradient(g);
            var sol = ps.sizeOverLifetime; sol.enabled = true;
            sol.size = smoke ? new ParticleSystem.MinMaxCurve(1f, AnimationCurve.EaseInOut(0f, 0.3f, 1f, 2.2f))
                             : new ParticleSystem.MinMaxCurve(1f, FxAssets.Curve(0.25f, 1.5f));
            var em = ps.emission; em.rateOverTime = rate;
            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Cone; shape.angle = smoke ? 14f : 7f; shape.radius = smoke ? 0.5f : 0.25f;
            shape.rotation = new Vector3(-90f, 0f, 0f);
            var vol = ps.velocityOverLifetime; vol.enabled = true; vol.space = ParticleSystemSimulationSpace.Local;
            vol.x = new ParticleSystem.MinMaxCurve(-1.2f, 1.2f); vol.y = new ParticleSystem.MinMaxCurve(-0.6f, 1.4f); vol.z = new ParticleSystem.MinMaxCurve(0f, 6f);
            ps.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
            return ps;
        }

        public void Mount(HeroController hero)
        {
            if (hero == null || Mounted) return;
            _rider = hero;
            Mounted = true;
            Airborne = true;
            hero.CC.enabled = false;
            hero.transform.SetParent(SaddlePoint, true);
            // Keep the rider's real size no matter how the dragon is scaled
            Vector3 ls = SaddlePoint.lossyScale;
            hero.transform.localScale = new Vector3(1f / Mathf.Max(0.01f, ls.x), 1f / Mathf.Max(0.01f, ls.y), 1f / Mathf.Max(0.01f, ls.z));
            hero.transform.localPosition = new Vector3(0f, 0.15f, 0f);
            hero.transform.localRotation = Quaternion.identity;
            _yaw = transform.eulerAngles.y;
            _pitch = -8f;
            transform.position += Vector3.up * 5f;
            SetFlying(true);
            if (_anim != null) _anim.SetTrigger(TScream);
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_BossRoar", transform.position, 0.7f);
            AetheriaFx.Puff(transform.position, new Color(0.85f, 0.78f, 0.66f, 0.6f), 26, 7f, 3.2f, 1.3f, 0.3f);
            AetheriaFx.Shockwave(transform.position + Vector3.up * 0.2f, new Color(1f, 0.6f, 0.3f), 10f, 0.7f);
            AetheriaFx.Shake(0.3f, 0.5f);
            QuestAndMapService.Instance?.PushNotification("ВЫ В САДЛЕ · [W/S] тяга · [A/D] разворот · [Space/Shift] набор высоты / пике · [ЛКМ] пламя · [F] приземлиться");
        }

        private void SetFlying(bool flying)
        {
            if (_anim == null) return;
            _anim.SetBool(PFlying, flying);
            _anim.applyRootMotion = false;
        }

        public void Dismount()
        {
            if (!Mounted) return;
            Mounted = false;
            BreathingFire = false;
            _fire.Stop(true, ParticleSystemStopBehavior.StopEmitting);
            _smoke.Stop(true, ParticleSystemStopBehavior.StopEmitting);
            SetBreath(false);

            float ty = AetheriaWorld.SampleHeight(transform.position);
            transform.position = new Vector3(transform.position.x, ty, transform.position.z);
            transform.rotation = Quaternion.Euler(0f, _yaw, 0f);
            Airborne = false;
            SetFlying(false);

            if (_rider != null)
            {
                HeroController rider = _rider;
                rider.transform.SetParent(null, true);
                rider.transform.localScale = Vector3.one;
                Vector3 side = transform.position + transform.right * (5.2f * Mathf.Max(1f, transform.localScale.x)) + transform.forward * 1.5f;
                side.y = AetheriaWorld.SampleHeight(side) + 0.35f;
                rider.transform.position = side;
                rider.transform.rotation = Quaternion.Euler(0f, _yaw, 0f);
                rider.CC.enabled = true;
                rider.SnapCameraBehindHero();
                _rider = null;
            }
            AetheriaFx.Puff(transform.position, new Color(0.85f, 0.78f, 0.66f, 0.55f), 22, 6f, 2.8f, 1.2f, 0.25f);
            AetheriaFx.Shake(0.18f, 0.35f);
        }

        private void SetBreath(bool on)
        {
            if (_fire == null) return;
            var em = _fire.emission; em.enabled = on;
            var ems = _smoke.emission; ems.enabled = on;
            if (on) { _fire.Play(); _smoke.Play(); }
            else { _fire.Stop(true, ParticleSystemStopBehavior.StopEmitting); _smoke.Stop(true, ParticleSystemStopBehavior.StopEmitting); }
        }

        private void Update()
        {
            float dt = Time.deltaTime;
            _bob += dt;

            if (_anim != null)
            {
                // Grounded dragon breathes in its idle; airborne it strokes with flight speed
                _anim.SetFloat(PSpeed, Mounted ? Mathf.Clamp01(FlightSpeed / CruiseSpeed) : 0f);
            }
            foreach (var t in _wingTrails)
            {
                if (t == null) continue;
                var em = t.emission;
                bool trail = Airborne && FlightSpeed > 14f;
                if (em.enabled != trail) em.enabled = trail;
            }
            _screamCooldown -= dt;

            if (!Mounted)
            {
                FireEnergy = Mathf.Min(MaxFireEnergy, FireEnergy + 18f * dt);
                if (_mouthLight != null) _mouthLight.intensity = 0f;
                return;
            }

            bool modal = AetheriaHUDManager.Instance != null && AetheriaHUDManager.Instance.IsModalOpen;
            if (modal) { SetBreath(false); return; }

            if (Input.GetKeyDown(KeyCode.F)) { Dismount(); return; }

            float fwd = Input.GetAxisRaw("Vertical");
            float turn = Input.GetAxisRaw("Horizontal");
            float mx = Input.GetAxis("Mouse X") * 2.2f;
            float my = Input.GetAxis("Mouse Y") * 1.8f;
            float up = (Input.GetKey(KeyCode.Space) ? 1f : 0f) - (Input.GetKey(KeyCode.LeftShift) ? 1f : 0f);

            float speedMult = _rider != null ? _rider.SkillTree.DragonSpeedMult * _rider.Customization.Derived.DragonSpeedMultiplier : 1f;
            float fireMult = _rider != null ? _rider.SkillTree.DragonFireMult * _rider.Customization.Derived.DragonFireMultiplier : 1f;

            _yaw += (turn * 62f + mx * 85f) * dt;
            _pitch = Mathf.Clamp(_pitch - my * 52f * dt - up * 26f * dt, -44f, 48f);
            if (Mathf.Abs(my) < 0.05f && Mathf.Abs(up) < 0.05f) _pitch = Mathf.Lerp(_pitch, 0f, dt * 1.4f);
            _roll = Mathf.Lerp(_roll, -Mathf.Clamp(turn * 38f + mx * 26f, -46f, 46f), dt * 4.5f);
            transform.rotation = Quaternion.Euler(_pitch, _yaw, _roll);

            float targetSpeed = (fwd > 0.1f ? 38f : (fwd < -0.1f ? 11f : CruiseSpeed)) * speedMult * (1f + Mathf.Max(0f, -_pitch) * 0.012f);
            FlightSpeed = Mathf.Lerp(FlightSpeed, targetSpeed, dt * 2.4f);

            Vector3 nextPos = transform.position + (transform.forward * FlightSpeed + Vector3.up * (up * 15f + Mathf.Sin(_bob * 1.7f) * 0.6f)) * dt;
            float ceiling = 150f;
            float minGround = AetheriaWorld.SampleHeight(nextPos) + 3.5f;
            if (nextPos.y < minGround) nextPos.y = minGround;
            if (nextPos.y > ceiling) { nextPos.y = ceiling; _pitch = Mathf.Min(_pitch, 0f); }
            float lim = AetheriaWorld.HalfExtent - 12f;
            nextPos.x = Mathf.Clamp(nextPos.x, -lim, lim);
            nextPos.z = Mathf.Clamp(nextPos.z, -lim, lim);
            transform.position = nextPos;

            // Dragonfire
            bool wantsFire = (Input.GetMouseButton(0) || ForceBreathForDemo) && FireEnergy > 1f;
            if (wantsFire != BreathingFire)
            {
                BreathingFire = wantsFire;
                SetBreath(wantsFire);
                if (_anim != null) _anim.SetBool(TBreath, wantsFire);
            }
            if (wantsFire)
            {
                FireEnergy = Mathf.Max(0f, FireEnergy - 16f * dt);
                _fireDamageAccum += dt;
                AetheriaRender.StyleFxLight(_mouthLight, new Color(1f, 0.5f, 0.16f), 30f + Mathf.Sin(_bob * 31f) * 5f, 110f);
                if (_fireDamageAccum >= 0.18f)
                {
                    _fireDamageAccum = 0f;
                    ApplyFireSweep(28f * fireMult);
                }
            }
            else
            {
                FireEnergy = Mathf.Min(MaxFireEnergy, FireEnergy + 14f * dt);
                if (_mouthLight != null) _mouthLight.intensity = Mathf.Lerp(_mouthLight.intensity, 0f, dt * 10f);
            }

            // Chase camera with speed feel
            if (Camera.main != null)
            {
                Vector3 camDesired = transform.position - transform.forward * (13f + FlightSpeed * 0.14f)
                                    + transform.up * 5.2f + transform.right * (_roll * 0.035f);
                Camera.main.transform.position = Vector3.Lerp(Camera.main.transform.position, camDesired, Mathf.Clamp01(dt * 7f));
                Camera.main.transform.LookAt(transform.position + transform.forward * 10f + Vector3.up * 1.5f);
                var fx = AetheriaFx.Instance;
                if (fx != null && fx.ShakeOffset.sqrMagnitude > 1e-8f)
                    Camera.main.transform.position += Camera.main.transform.right * fx.ShakeOffset.x + Camera.main.transform.up * fx.ShakeOffset.y;
            }

            _revealAccum += dt;
            if (_revealAccum > 0.45f)
            {
                _revealAccum = 0f;
                QuestAndMapService.Instance?.RevealAround(transform.position, 150f);
            }
        }

        private void ApplyFireSweep(float dmg)
        {
            Vector3 orig = MouthPoint.position;
            Vector3 fwd = transform.forward;
            Vector3 ground = orig + fwd * 13f;
            ground.y = AetheriaWorld.SampleHeight(ground) + 0.2f;
            if (ground.y > orig.y) ground = orig + fwd * 11f;

            AetheriaWorldGenerator.Instance?.RegisterSpellTerrainScar(ground, 7f, OrbType.Exort);
            AetheriaFx.Puff(ground, new Color(0.30f, 0.20f, 0.16f, 0.6f), 6, 3f, 3.4f, 1.6f, 1.6f);
            AetheriaFx.Burst(ground, new Color(1f, 0.55f, 0.12f), 14, 3f, 9f, 0.10f, 0.22f, 0.7f, 0.5f, 1.4f, new Color(0.7f, 0.12f, 0.02f, 0f));

            foreach (var c in Physics.OverlapSphere(ground, 9f))
            {
                var enemy = c.GetComponentInParent<EnemyAndBossAI>();
                if (enemy != null) enemy.TakeDamage(dmg, _rider != null ? _rider.gameObject : gameObject, fwd * 6f, OrbType.Exort);
                var prop = c.GetComponentInParent<BreakableProp>();
                if (prop != null) prop.ApplyDamage(dmg * 1.5f, ground, false, true);
            }
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_Fireball_Boom", ground, 0.35f);
        }

        public void Scream()
        {
            if (_anim == null || _screamCooldown > 0f) return;
            _screamCooldown = 4f;
            _anim.SetTrigger(TScream);
        }

        public void Flinch()
        {
            if (_anim != null) _anim.SetTrigger(THit);
        }
    }
}
