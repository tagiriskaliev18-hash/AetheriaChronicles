using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria
{
    /// <summary>Simple glowing shot fired by ranged enemies.</summary>
    public class EnemyProjectile : MonoBehaviour
    {
        public float Damage = 12f;
        public float Speed = 17f;
        public Color Color = new Color(0.75f, 0.35f, 1f);
        public Vector3 Target;
        private Vector3 _dir;
        private float _life;
        private GameObject _owner;
        private Light _light;

        public static void Fire(GameObject owner, Vector3 from, Vector3 target, float damage, Color color)
        {
            FxAssets.Ensure();
            var go = new GameObject("EnemyProjectile");
            go.transform.position = from;
            var p = go.AddComponent<EnemyProjectile>();
            p.Damage = damage; p.Color = color; p._owner = owner;
            Vector3 to = target - from;
            p._dir = to.sqrMagnitude > 0.01f ? to.normalized : Vector3.forward;
            go.AddComponent<MeshFilter>().sharedMesh = FxAssets.Sphere;
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterial = AetheriaRender.Glow(color, 7f);
            mr.shadowCastingMode = ShadowCastingMode.Off;
            go.transform.localScale = Vector3.one * 0.42f;
            p._light = AetheriaRender.CreatePointLight(go.transform, Vector3.zero, color, 6f, 9f);
            AetheriaFx.FlashLight(from, color, 10f, 6f, 0.2f);
        }

        private void Update()
        {
            float dt = Time.deltaTime;
            transform.position += _dir * Speed * dt;
            _life += dt;
            if (Time.frameCount % 4 == 0)
                AetheriaFx.Burst(transform.position, Color, 2, 0.2f, 0.6f, 0.12f, 0.22f, 0.0f, 1.5f, new Color(Color.r * 0.4f, Color.g * 0.3f, Color.b * 0.4f, 0f));

            var hero = HeroController.Instance;
            bool hitHero = hero != null && !hero.IsDead && (hero.transform.position + Vector3.up * 1.0f - transform.position).sqrMagnitude < 1.45f * 1.45f;
            bool hitGround = transform.position.y <= AetheriaWorld.SampleHeight(transform.position) + 0.15f;
            if (hitHero) hero.TakeDamage(Damage, _owner);
            if (hitHero || hitGround || _life > 3f)
            {
                AetheriaFx.FlashLight(transform.position, Color, 22f, 9f, 0.3f);
                AetheriaFx.Burst(transform.position, Color, 22, 2.5f, 8f, 0.08f, 0.20f, 0.5f, 2.2f, new Color(Color.r * 0.4f, Color.g * 0.3f, Color.b * 0.4f, 0f));
                AetheriaFx.Shockwave(transform.position, Color, 1.8f, 0.3f, vertical: true, facing: -_dir);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_HitImpact", transform.position, 0.6f);
                Destroy(gameObject);
            }
        }
    }

    /// <summary>
    /// Biome enemies and the multi-phase boss Malakor. Attacks are real animation clips: the blow lands on
    /// the swing frame, telegraphs tell amber (parry) from crimson (dodge), and elements react on hit.
    /// </summary>
    public class EnemyAndBossAI : MonoBehaviour
    {
        public static EnemyAndBossAI ActiveBoss { get; private set; }

        public bool IsBoss = false;
        public bool IsElite = false;
        public string EliteTitle = "";
        public EnemyArchetype Archetype = EnemyArchetype.Skeleton;
        public string DisplayName = "Risen Warrior";
        public string QuestIdOnDeath = "";
        public float HP = 120f;
        public float MaxHP = 120f;
        public int XPReward = 70;
        public int GoldReward = 35;
        public float AggroRadius = 22f;
        public float AttackRange = 2.8f;
        public float AttackDamage = 14f;
        public float MoveSpeed = 3.6f;
        public bool Ranged = false;
        public Color ProjectileColor = new Color(0.75f, 0.35f, 1f);
        public float BodyRadius = 0.6f;
        public bool Dead { get; private set; }

        // Boss state
        public int BossPhase = 1;
        public float StaggerGauge = 0f;
        public float MaxStagger = 100f;
        public bool IsStaggerBroken { get; private set; }
        public bool IsTelegraphing { get; private set; }
        public BossAttackKind CurrentBossAttack { get; private set; }
        public bool IsCurrentAttackParryable =>
            CurrentBossAttack == BossAttackKind.CrimsonCleave ||
            CurrentBossAttack == BossAttackKind.TempestLunge ||
            CurrentBossAttack == BossAttackKind.ShatterSpin;

        public CreatureAnimator Anim { get; private set; }

        private enum State { Idle, Chase, Windup, Recover, Stagger, Dead }
        private State _state = State.Idle;
        private Vector3 _homePos;
        private float _attackCooldown = 1.2f;
        private float _staggerBreakTimer;
        private float _burnTimer, _freezeTimer, _shockTimer;
        private int _cycle;
        private int _pendingAction;
        private float _pendingHitFrame;
        private bool _strikeDone;
        private Vector3 _knockback;
        private float _deathTimer;
        private float _burnTick;
        private float _shockTick;
        private float _hitFlash;
        private float _stateTimer;
        private const float RegularRecoverTime = 0.45f;
        private const float BossRecoverTime = 0.65f;

        private GameObject _telegraphRing;
        private Light _telegraphLight;
        private readonly List<Material> _materials = new List<Material>();
        private readonly List<Color> _origEmissives = new List<Color>();
        private float _staggerFxTick;

        private static readonly int[] MonsterActions = { 1, 2, 1, 3, 2, 1 };

        private void Awake()
        {
            _homePos = transform.position;
            if (IsBoss) ActiveBoss = this;

            Anim = GetComponent<CreatureAnimator>() ?? gameObject.AddComponent<CreatureAnimator>();
            Anim.CreatureKind = IsBoss ? CreatureAnimator.Kind.Boss : Ranged || Archetype == EnemyArchetype.TempestStalker ? CreatureAnimator.Kind.Whisper : CreatureAnimator.Kind.Minion;
            Anim.Bind(GetComponentInChildren<Animator>(true));
            Anim.MoveAnimationSpeed = IsBoss ? 4.4f : MoveSpeed;

            // Instance materials so hits can flash and frost can tint them
            foreach (var r in GetComponentsInChildren<Renderer>(true))
            {
                var src = r.sharedMaterials;
                var mats = new Material[src.Length];
                for (int i = 0; i < src.Length; i++)
                {
                    mats[i] = src[i] != null ? new Material(src[i]) : null;
                    if (mats[i] != null)
                    {
                        Color origEm = Color.black;
                        if (mats[i].HasProperty("_EmissiveColor")) origEm = mats[i].GetColor("_EmissiveColor");
                        else if (mats[i].HasProperty("_EmissionColor")) origEm = mats[i].GetColor("_EmissionColor");
                        _materials.Add(mats[i]);
                        _origEmissives.Add(origEm);
                    }
                }
                r.sharedMaterials = mats;
            }
            if (IsElite) transform.localScale *= 1.28f;

            if (IsBoss) BuildTelegraph();
        }

        private void BuildTelegraph()
        {
            _telegraphRing = new GameObject("BossTelegraphRing");
            _telegraphRing.transform.SetParent(transform, false);
            _telegraphRing.transform.localPosition = new Vector3(0f, 0.14f, 0f);
            _telegraphRing.AddComponent<MeshFilter>().sharedMesh = FxAssets.Ring;
            var mr = _telegraphRing.AddComponent<MeshRenderer>();
            mr.sharedMaterial = AetheriaRender.TransparentGlow(AetheriaPalette.TextAmber, true, FxAssets.Glow);
            mr.shadowCastingMode = ShadowCastingMode.Off;
            _telegraphRing.SetActive(false);
            _telegraphLight = AetheriaRender.CreatePointLight(transform, Vector3.up * 2.2f, AetheriaPalette.TextAmber, 16f, 0f);
        }

        private void Update()
        {
            if (Dead)
            {
                _deathTimer += Time.deltaTime;
                if (!IsBoss && _deathTimer > 2.4f)
                {
                    // Sink into the ground and fade away
                    transform.position -= Vector3.up * (0.9f * Time.deltaTime);
                    if (_deathTimer > 4.2f) Destroy(gameObject);
                }
                return;
            }

            float dt = Time.deltaTime;
            var hero = HeroController.Instance;
            if (hero == null || !hero.HasEnteredWorld) return;
            float dist = Vector3.Distance(transform.position, hero.transform.position);
            if (IsBoss) AudioAndQualityService.Instance?.SetBossActive(dist < 40f);

            bool frozen = AetheriaHUDManager.Instance != null && AetheriaHUDManager.Instance.IsModalOpen && AetheriaHUDManager.Instance.ActiveOverlay != OverlayMode.DevConsole;
            if (frozen) { Anim.Drive(0f); return; }

            if (_hitFlash > 0f)
            {
                _hitFlash -= dt;
                float k = Mathf.Clamp01(_hitFlash / 0.16f);
                for (int i = 0; i < _materials.Count; i++)
                {
                    Color orig = i < _origEmissives.Count ? _origEmissives[i] : Color.black;
                    AetheriaRender.SetEmission(_materials[i], _hitFlash <= 0f ? orig : orig + Color.white * (k * 2.4f));
                }
            }

            // Elemental damage over time and their tell-tale effects
            if (_burnTimer > 0f)
            {
                _burnTimer -= dt;
                HP = Mathf.Max(0f, HP - 8f * dt);
                _burnTick -= dt;
                if (_burnTick <= 0f) { _burnTick = 0.12f; AetheriaFx.Burst(transform.position + Vector3.up * 1.1f, new Color(1f, 0.55f, 0.15f), 3, 0.6f, 1.8f, 0.12f, 0.26f, 0.45f, -0.1f, 1f, new Color(0.6f, 0.1f, 0f, 0f)); }
                if (HP <= 0f) { Die(); return; }
            }
            if (_freezeTimer > 0f) _freezeTimer -= dt;
            if (_shockTimer > 0f)
            {
                _shockTimer -= dt;
                _shockTick -= dt;
                if (_shockTick <= 0f)
                {
                    _shockTick = 0.22f;
                    AetheriaFx.Lightning(transform.position + Random.insideUnitSphere * 0.6f + Vector3.up * 1.3f,
                        transform.position + Random.insideUnitSphere * 0.9f + Vector3.up * 0.8f, new Color(0.65f, 0.45f, 1f), 0.16f, 0.07f, 5, 0.5f);
                }
            }

            if (IsBoss && IsStaggerBroken)
            {
                _staggerBreakTimer -= dt;
                // Stars wheeling over the reeling boss
                _staggerFxTick -= dt;
                if (_staggerFxTick <= 0f)
                {
                    _staggerFxTick = 0.22f;
                    AetheriaFx.Burst(transform.position + Vector3.up * 2.7f, AetheriaPalette.TextAmber, 4, 1.2f, 2.6f, 0.08f, 0.16f, 0.7f, -0.15f, 0.8f, new Color(1f, 0.5f, 0.1f, 0f));
                }
                if (_staggerBreakTimer <= 0f)
                {
                    IsStaggerBroken = false;
                    StaggerGauge = 0f;
                    _state = State.Recover;
                    _stateTimer = 0f;
                    _attackCooldown = 0.5f;
                }
                return;
            }

            _knockback = Vector3.Lerp(_knockback, Vector3.zero, Mathf.Clamp01(dt * 8f));
            if (_knockback.sqrMagnitude > 0.02f)
            {
                transform.position += _knockback * dt;
                PlaceOnGround();
            }

            _stateTimer += dt;
            switch (_state)
            {
                case State.Windup:
                    WindupTick(dist);
                    break;
                case State.Recover:
                {
                    float recoverDur = IsBoss ? BossRecoverTime : RegularRecoverTime;
                    if (((!Anim.Valid || !Anim.InAttack || Anim.ActionTime > 2.2f) && _stateTimer >= recoverDur) || _stateTimer >= 1.8f)
                    {
                        _state = State.Chase;
                        _stateTimer = 0f;
                        _attackCooldown = IsBoss ? (BossPhase == 3 ? 1.2f : 1.7f) : 1.55f;
                    }
                    break;
                }
                default:
                    Think(hero, dist, dt);
                    break;
            }

            if (_telegraphRing != null && _telegraphRing.activeSelf)
            {
                ApplyTelegraphScale(1f + Mathf.Sin(Time.time * 18f) * 0.025f);
            }
            FaceHero(hero, dt, _state == State.Windup);
        }

        private void ApplyTelegraphScale(float pulse)
        {
            if (_telegraphRing == null) return;
            float parentXZ = Mathf.Max(0.01f, Mathf.Max(transform.lossyScale.x, transform.lossyScale.z));
            float parentY = Mathf.Max(0.01f, transform.lossyScale.y);
            float r = _telegraphRadius / parentXZ;
            _telegraphRing.transform.localScale = new Vector3(r * pulse, 1f / parentY, r * pulse);
        }

        private float _telegraphRadius = 5f;

        private void Think(HeroController hero, float dist, float dt)
        {
            if (hero.IsDead || dist > AggroRadius * 1.9f) { Anim.Drive(0f); return; }
            _attackCooldown -= dt;

            if (dist > AttackRange)
            {
                // Pursue, steering around obstacles and other enemies
                Vector3 toHero = hero.transform.position - transform.position;
                toHero.y = 0f;
                Vector3 dir = toHero.sqrMagnitude > 0.01f ? toHero.normalized : transform.forward;
                float speed = MoveSpeed * (_freezeTimer > 0f ? 0.45f : 1f);
                Vector3 step = dir * speed * dt;
                if (Physics.SphereCast(transform.position + Vector3.up * 0.9f, BodyRadius, dir, out RaycastHit hit, speed * dt + BodyRadius, ~0, QueryTriggerInteraction.Ignore))
                {
                    if (hit.collider.GetComponentInParent<EnemyAndBossAI>() == null && hit.collider.GetComponent<HeroController>() == null)
                    {
                        Vector3 slide = Vector3.ProjectOnPlane(dir, hit.normal).normalized;
                        step = slide * speed * dt;
                    }
                }
                Vector3 next = transform.position + step;
                if (AetheriaWorld.SampleHeight(next) > AetheriaWorld.SeaLevel + 0.2f && Vector3.Distance(_homePos, next) < 70f)
                    transform.position = next;
                PlaceOnGround();
                Anim.Drive(speed);
            }
            else
            {
                Anim.Drive(0f);
                if (_attackCooldown <= 0f) BeginAttack(hero, dist);
            }
        }

        private void PlaceOnGround()
        {
            Vector3 p = transform.position;
            p.y = AetheriaWorld.SampleHeight(p);
            transform.position = p;
        }

        private void FaceHero(HeroController hero, float dt, bool locked)
        {
            if (locked) return;
            Vector3 toHero = hero.transform.position - transform.position;
            toHero.y = 0f;
            if (toHero.sqrMagnitude > 0.04f)
                transform.rotation = Quaternion.RotateTowards(transform.rotation, Quaternion.LookRotation(toHero, Vector3.up), 300f * dt);
        }

        private void BeginAttack(HeroController hero, float dist)
        {
            if (IsBoss) { BeginBossAttack(); return; }
            if (Ranged)
            {
                _pendingAction = 5;
                _pendingHitFrame = 0.50f;
            }
            else
            {
                _pendingAction = MonsterActions[_cycle % MonsterActions.Length];
                _cycle++;
                _pendingHitFrame = 0.46f;
            }
            _strikeDone = false;
            _state = State.Windup;
            _stateTimer = 0f;
            Anim.PlayAction(_pendingAction);
            AudioAndQualityService.Instance?.PlaySfxAt(Ranged ? "SFX_Enemy_Ranged_Cast" : "SFX_Boss_Melee", transform.position, Ranged ? 0.6f : 0.3f);
        }

        private void WindupTick(float dist)
        {
            if (_strikeDone)
            {
                _state = State.Recover;
                _stateTimer = 0f;
                return;
            }
            if (IsBoss)
            {
                // Telegraph pulses until the swing frame of the animation
                UpdateBossTelegraphVisual();
            }

            float fallbackWindupSec = IsBoss ? 0.65f : 0.42f;
            bool hitReady = Anim.Valid
                ? ((Anim.InActionState(_pendingAction, out float t) && t >= _pendingHitFrame) || Anim.ActionTime > 1.6f)
                : (_stateTimer >= fallbackWindupSec);
            if (hitReady)
            {
                _strikeDone = true;
                ExecuteStrike();
                _state = State.Recover;
                _stateTimer = 0f;
            }
        }

        private void ExecuteStrike()
        {
            var hero = HeroController.Instance;
            if (hero == null) return;
            float dist = Vector3.Distance(transform.position, hero.transform.position);
            Vector3 forward = transform.forward;

            if (IsBoss)
            {
                float radius = BossAttackRadius(CurrentBossAttack);
                float dmg = CurrentBossAttack == BossAttackKind.ChronoCataclysm ? 42f : 28f;
                bool amber = IsCurrentAttackParryable;
                Color col = amber ? AetheriaPalette.TextAmber : AetheriaPalette.TextCrimson;
                Vector3 at = transform.position + forward * (radius * 0.35f) + Vector3.up * 0.3f;

                if (CurrentBossAttack == BossAttackKind.SeismicSlam)
                {
                    AetheriaFx.Shockwave(transform.position, col, radius, 0.6f);
                    AetheriaFx.Shockwave(transform.position + Vector3.up * 0.6f, col, radius * 0.7f, 0.5f);
                    AetheriaFx.Burst(at, col, 60, 4f, 13f, 0.12f, 0.30f, 0.8f, 1.2f, 2.0f, new Color(0.4f, 0.2f, 0.05f, 0f));
                    AetheriaFx.Puff(at, new Color(0.55f, 0.45f, 0.36f, 0.6f), 18, 4f, 2.4f, 1.3f, 0.5f);
                    AetheriaFx.Debris(at, new Color(0.35f, 0.30f, 0.28f), 14, transform.position, 11f, 0.34f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_Boss_Slam", at, 1f);
                    AetheriaFx.Shake(0.55f, 0.5f);
                }
                else if (CurrentBossAttack == BossAttackKind.AbyssalFireVolley)
                {
                    Vector3 muzzle = transform.position + forward * 1.4f + Vector3.up * 1.8f;
                    Vector3 toHero = (hero.transform.position + Vector3.up * 1.1f) - muzzle;
                    Vector3 baseDir = toHero.sqrMagnitude > 0.01f ? toHero.normalized : forward;
                    Color fireCol = new Color(1f, 0.38f, 0.08f);
                    for (int i = -2; i <= 2; i++)
                    {
                        Vector3 spreadDir = Quaternion.Euler(0f, i * 11f, 0f) * baseDir;
                        EnemyProjectile.Fire(gameObject, muzzle, muzzle + spreadDir * 22f, 22f, fireCol);
                    }
                    Vector3 scorchAt = transform.position + forward * 3.0f;
                    scorchAt.y = AetheriaWorld.SampleHeight(scorchAt) + 0.12f;
                    AetheriaWorldGenerator.Instance?.RegisterSpellTerrainScar(scorchAt, 4.5f, OrbType.Exort);
                    AetheriaFx.GroundRing(scorchAt, fireCol, 2.2f, 5.2f, 0.45f);
                    AetheriaFx.Shockwave(scorchAt, fireCol, 5.2f, 0.5f);
                    AetheriaFx.Burst(muzzle, fireCol, 52, 4f, 12f, 0.10f, 0.26f, 0.6f, 0.8f, 1.8f, new Color(0.6f, 0.12f, 0.02f, 0f));
                    AetheriaFx.Puff(scorchAt + Vector3.up * 0.4f, new Color(0.35f, 0.20f, 0.14f, 0.65f), 12, 3.2f, 2.0f, 1.2f, 0.6f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_Fireball_Boom", muzzle, 0.9f);
                    AetheriaFx.Shake(0.35f, 0.4f);
                }
                else if (CurrentBossAttack == BossAttackKind.ChronoCataclysm)
                {
                    AetheriaFx.SpherePulse(transform.position + Vector3.up * 1.6f, col, radius, 0.9f, 0.5f);
                    AetheriaFx.Shockwave(transform.position + Vector3.up * 0.4f, col, radius * 1.25f, 0.9f);
                    AetheriaFx.Shockwave(transform.position + Vector3.up * 1.6f, col, radius, 0.8f, vertical: true, facing: forward);
                    AetheriaFx.Burst(transform.position + Vector3.up * 1.6f, col, 110, 5f, 16f, 0.10f, 0.32f, 0.3f, 1.0f, 2.0f, new Color(0.5f, 0.05f, 0.4f, 0f));
                    for (int i = 0; i < 7; i++)
                        AetheriaFx.Lightning(transform.position + Vector3.up * 2.6f, transform.position + Random.insideUnitSphere * radius * 0.9f + Vector3.up * 0.2f, col, 0.3f, 0.14f, 11, 1.6f);
                    AetheriaFx.FlashLight(transform.position + Vector3.up * 2.6f, col, 60f, 26f, 0.8f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_BossRoar", at, 1f);
                    AetheriaFx.Shake(0.6f, 0.6f);
                }
                else
                {
                    AetheriaFx.Shockwave(at, col, radius * 0.8f, 0.42f, vertical: true, facing: forward);
                    AetheriaFx.Shockwave(transform.position, col, radius, 0.5f);
                    AetheriaFx.Burst(at, col, 46, 4f, 12f, 0.10f, 0.26f, 0.4f, 1.6f, 2.0f, new Color(0.5f, 0.1f, 0.05f, 0f));
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_Boss_Melee", at, 1f);
                    AetheriaFx.Shake(0.30f, 0.35f);
                }
                AetheriaFx.FlashLight(at, col, 44f, 20f, 0.45f);

                float hitRadius = CurrentBossAttack == BossAttackKind.AbyssalFireVolley ? 3.8f : radius;
                if (dist <= hitRadius) hero.TakeDamage(dmg, gameObject, amber);
                if (_telegraphRing != null) _telegraphRing.SetActive(false);
                if (_telegraphLight != null) _telegraphLight.enabled = false;
                IsTelegraphing = false;
            }
            else if (Ranged)
            {
                Vector3 from = transform.position + Vector3.up * 1.2f + forward * 0.5f;
                EnemyProjectile.Fire(gameObject, from, hero.transform.position + Vector3.up * 1.1f, AttackDamage, ProjectileColor);
            }
            else
            {
                Vector3 at = transform.position + forward * (AttackRange * 0.6f) + Vector3.up * 1.1f;
                AetheriaFx.Shockwave(at, new Color(0.95f, 0.30f, 0.25f), 1.6f, 0.28f, vertical: true, facing: forward);
                Vector3 toHero = hero.transform.position - transform.position;
                bool inFront = Vector3.Dot(new Vector3(toHero.x, 0f, toHero.z).normalized, new Vector3(forward.x, 0f, forward.z).normalized) > 0.15f;
                if (dist <= AttackRange + 0.9f && inFront) hero.TakeDamage(AttackDamage, gameObject);
            }
        }

        // ------------------------------------------------------------------ boss

        private void BeginBossAttack()
        {
            _cycle++;
            BossPhase = HP < MaxHP * 0.32f ? 3 : (HP < MaxHP * 0.65f ? 2 : 1);
            BossAttackKind[] pool = BossPhase == 3
                ? new[] { BossAttackKind.CrimsonCleave, BossAttackKind.SeismicSlam, BossAttackKind.ShatterSpin, BossAttackKind.AbyssalFireVolley, BossAttackKind.ChronoCataclysm, BossAttackKind.TempestLunge }
                : (BossPhase == 2
                    ? new[] { BossAttackKind.AbyssalFireVolley, BossAttackKind.TempestLunge, BossAttackKind.SeismicSlam }
                    : new[] { BossAttackKind.CrimsonCleave, BossAttackKind.SeismicSlam, BossAttackKind.TempestLunge });
            CurrentBossAttack = pool[_cycle % pool.Length];
            IsTelegraphing = true;

            switch (CurrentBossAttack)
            {
                case BossAttackKind.CrimsonCleave:     _pendingAction = 1; _pendingHitFrame = 0.40f; break;
                case BossAttackKind.SeismicSlam:       _pendingAction = 2; _pendingHitFrame = 0.46f; break;
                case BossAttackKind.TempestLunge:      _pendingAction = 3; _pendingHitFrame = 0.34f; break;
                case BossAttackKind.ShatterSpin:       _pendingAction = 1; _pendingHitFrame = 0.30f; break;
                case BossAttackKind.AbyssalFireVolley: _pendingAction = 5; _pendingHitFrame = 0.45f; break;
                default:                               _pendingAction = 4; _pendingHitFrame = 0.52f; break;
            }
            _strikeDone = false;
            _state = State.Windup;
            _stateTimer = 0f;

            Color col = IsCurrentAttackParryable ? AetheriaPalette.TextAmber : AetheriaPalette.TextCrimson;
            _telegraphRadius = BossAttackRadius(CurrentBossAttack);
            if (_telegraphRing != null)
            {
                _telegraphRing.GetComponent<MeshRenderer>().sharedMaterial = AetheriaRender.TransparentGlow(col, true, FxAssets.Glow);
                ApplyTelegraphScale(1f);
                _telegraphRing.SetActive(true);
            }
            if (_telegraphLight != null)
            {
                AetheriaRender.StyleFxLight(_telegraphLight, col, 18f, 20f);
                _telegraphLight.enabled = true;
            }
            Anim.PlayAction(_pendingAction);
            QuestAndMapService.Instance?.PushNotification(
                $"МАЛАКОР: {BossAttackName(CurrentBossAttack)} · {(IsCurrentAttackParryable ? "ЯНТАРНЫЙ — ПАРИРУЙТЕ [RMB]" : "АЛЫЙ — УКЛОНЯЙТЕСЬ [V]")}");
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_BossRoar", transform.position, 0.5f);
        }

        private void UpdateBossTelegraphVisual()
        {
            // Lunge closes the distance during the wind-up
            if (CurrentBossAttack == BossAttackKind.TempestLunge)
            {
                var hero = HeroController.Instance;
                if (hero != null)
                {
                    Vector3 toHero = hero.transform.position - transform.position; toHero.y = 0f;
                    if (toHero.magnitude > AttackRange * 0.8f)
                    {
                        transform.position += toHero.normalized * (11f * Time.deltaTime);
                        PlaceOnGround();
                    }
                }
            }
        }

        private static float BossAttackRadius(BossAttackKind k) => k switch
        {
            BossAttackKind.ChronoCataclysm => 12.5f,
            BossAttackKind.SeismicSlam => 8.2f,
            BossAttackKind.CrimsonCleave => 5.6f,
            BossAttackKind.TempestLunge => 5.0f,
            _ => 6.4f
        };

        private static string BossAttackName(BossAttackKind k) => k switch
        {
            BossAttackKind.CrimsonCleave => "Багровый Рассекатель",
            BossAttackKind.SeismicSlam => "Земной Тяжёлый Удар",
            BossAttackKind.TempestLunge => "Бросок Бури",
            BossAttackKind.ShatterSpin => "Дробящий Вихрь",
            BossAttackKind.AbyssalFireVolley => "Бездонный Залп",
            _ => "Хроно-Катастрофа"
        };

        public void ApplyParryStagger(float mult)
        {
            if (Dead) return;
            StaggerGauge += 34f * mult;
            if (StaggerGauge >= MaxStagger && !IsStaggerBroken)
            {
                IsStaggerBroken = true;
                IsTelegraphing = false;
                _strikeDone = true;
                _state = State.Stagger;
                _stateTimer = 0f;
                if (_telegraphRing != null) _telegraphRing.SetActive(false);
                if (_telegraphLight != null) _telegraphLight.enabled = false;
                _staggerBreakTimer = 2.8f + (HeroController.Instance?.SkillTree?.StaggerWindowBonus ?? 0f);
                Anim.PlayAction(6);
                Vector3 at = transform.position + Vector3.up * 2.4f;
                AetheriaFx.FlashLight(at, AetheriaPalette.TextAmber, 60f, 22f, 0.8f);
                AetheriaFx.Flare(at, AetheriaPalette.TextAmber, 4f, 0.6f);
                AetheriaFx.Shockwave(at, AetheriaPalette.TextAmber, 7f, 0.6f, vertical: true);
                AetheriaFx.Shake(0.3f, 0.4f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_Boss_Death", at, 0.6f);
                QuestAndMapService.Instance?.PushNotification("РАСКОЛОТ! Малакор оглушён — бейте (+35% урона)");
            }
        }

        public void TakeDamage(float dmg, GameObject instigator, Vector3 impulse, OrbType elem)
        {
            if (Dead) return;
            float mult = IsStaggerBroken ? 1.35f : 1f;
            HP = Mathf.Max(0f, HP - dmg * mult);
            if (IsBoss) StaggerGauge = Mathf.Min(MaxStagger, StaggerGauge + 8f);

            // Elemental reactions
            if (elem == OrbType.Exort)
            {
                if (_freezeTimer > 0f) { HP -= 35f; _freezeTimer = 0f; AetheriaFx.Shockwave(transform.position + Vector3.up, new Color(0.6f, 0.9f, 1f), 3.5f, 0.4f); QuestAndMapService.Instance?.PushNotification("ТЕРМОШОК!"); }
                _burnTimer = 4f;
            }
            else if (elem == OrbType.Wex)
            {
                if (_burnTimer > 0f) { HP -= 30f; _burnTimer = 0f; AetheriaFx.Shockwave(transform.position + Vector3.up, new Color(1f, 0.55f, 0.2f), 3.8f, 0.4f); QuestAndMapService.Instance?.PushNotification("ПЕРЕГРУЗКА!"); }
                _shockTimer = 4f;
            }
            else if (elem == OrbType.Quas)
            {
                if (_shockTimer > 0f) { HP -= 32f; _shockTimer = 0f; AetheriaFx.Shockwave(transform.position + Vector3.up, new Color(0.55f, 0.85f, 1f), 3.8f, 0.4f); QuestAndMapService.Instance?.PushNotification("СВЕРХПРОВОДИМОСТЬ!"); }
                _freezeTimer = 3.5f;
            }

            // Recoil and flinch
            Vector3 push = new Vector3(impulse.x, 0f, impulse.z);
            _knockback = push * (IsBoss ? 0.12f : 0.55f);
            _hitFlash = 0.16f;
            if (!IsBoss && (_state != State.Windup || HP < MaxHP * 0.3f)) Anim.PlayHit();

            if (HP <= 0f) Die();
        }

        private void Die()
        {
            if (Dead) return;
            Dead = true;
            _state = State.Dead;
            _stateTimer = 0f;
            IsTelegraphing = false;
            if (_telegraphRing != null) _telegraphRing.SetActive(false);
            if (_telegraphLight != null) _telegraphLight.enabled = false;
            Anim.PlayDeath();

            var hero = HeroController.Instance;
            if (hero != null)
            {
                hero.EnemiesSlain++;
                hero.SkillTree?.AddXP(XPReward);
                hero.SkillTree?.AddGold(GoldReward);
            }
            if (!string.IsNullOrEmpty(QuestIdOnDeath))
                QuestAndMapService.Instance?.AdvanceQuestProgress(QuestIdOnDeath, 1);

            Vector3 at = transform.position + Vector3.up * (IsBoss ? 2f : 1.1f);
            AetheriaFx.FlashLight(at, AetheriaPalette.TextGold, 40f, 14f, 0.7f);
            AetheriaFx.Burst(at, AetheriaPalette.TextGold, IsBoss ? 120 : 44, 3f, 11f, 0.10f, 0.28f, 0.8f, 1.0f, 2.0f, new Color(0.8f, 0.4f, 0.05f, 0f));
            AetheriaFx.Shockwave(transform.position + Vector3.up * 0.2f, AetheriaPalette.TextGold, IsBoss ? 12f : 3.5f, IsBoss ? 0.9f : 0.45f);
            AetheriaFx.Shake(IsBoss ? 0.5f : 0.12f, IsBoss ? 0.6f : 0.2f);
            AudioAndQualityService.Instance?.PlaySfxAt(IsBoss ? "SFX_Boss_Death" : "SFX_Hit_Stone", at, IsBoss ? 1f : 0.5f);

            if (IsBoss)
            {
                AudioAndQualityService.Instance?.SetBossActive(false);
                Invoke(nameof(HideBoss), 4.5f);
            }
        }

        private void HideBoss() => gameObject.SetActive(false);

        public void ResetBoss()
        {
            if (!IsBoss) return;
            HP = MaxHP;
            BossPhase = 1;
            StaggerGauge = 0f;
            IsStaggerBroken = false;
            IsTelegraphing = false;
            Dead = false;
            _state = State.Idle;
            _stateTimer = 0f;
            _strikeDone = false;
            _attackCooldown = 1.2f;
            _burnTimer = _freezeTimer = _shockTimer = 0f;
            _burnTick = _shockTick = _hitFlash = 0f;
            _knockback = Vector3.zero;
            if (_telegraphRing != null) _telegraphRing.SetActive(false);
            if (_telegraphLight != null) _telegraphLight.enabled = false;
            for (int i = 0; i < _materials.Count && i < _origEmissives.Count; i++)
                AetheriaRender.SetEmission(_materials[i], _origEmissives[i]);
            CancelInvoke();
            transform.position = _homePos;
            gameObject.SetActive(true);
            Anim.Reset();
            AudioAndQualityService.Instance?.SetBossActive(false);
        }

        private void OnDestroy()
        {
            foreach (var m in _materials) if (m != null) Destroy(m);
            if (ActiveBoss == this) ActiveBoss = null;
        }
    }
}
