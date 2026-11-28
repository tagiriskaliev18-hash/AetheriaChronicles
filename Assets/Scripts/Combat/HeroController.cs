using System.Collections.Generic;
using System.Linq;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Third-person hero: animated locomotion and a 4-hit combo whose blows land on the swing frame,
    /// guard with a timed parry, dash-dodge with invulnerability, casting, reactions, death and respawn.
    /// Camera, world queries and effects go through small services so this class stays about the fight.
    /// </summary>
    [RequireComponent(typeof(CharacterController))]
    public class HeroController : MonoBehaviour
    {
        public CharacterController CC { get; private set; }
        public SpellSystem Spells { get; private set; }
        public SkillTreeAndSaveService SkillTree { get; private set; }
        public HeroCustomizationService Customization { get; private set; }
        public Camera MainCam { get; private set; }

        public float HP = 120f;
        public float MaxHP = 120f;
        public float Stamina = 100f;
        public float MaxStamina = 100f;

        public bool IsDead = false;
        public bool IsBlocking = false;
        public bool DragonUnlocked = false;
        public bool GodMode = false;
        public float MouseSensitivity = 1.0f;
        public int EnemiesSlain = 0;

        /// <summary>0=InCaveTalkToTutor, 1=WalkOutToOverlook, 2=ApproachWaterfall, 3=PlayingCutscene, 4=OpenWorld</summary>
        public int PrologueStage = 4;
        public bool HasEnteredWorld = false;
        /// <summary>Set by story cinematics: the hero keeps idling but takes no input and leaves the camera alone.</summary>
        public bool CinematicLock;

        public Transform VisualRoot;
        public Animator ActiveAnimator { get; private set; }
        public HeroAnimationOverlay Overlay { get; private set; }
        /// <summary>Planar velocity the character is moving with (m/s), for animation and effects.</summary>
        public Vector3 Velocity { get; private set; }
        public float Speed => new Vector2(Velocity.x, Velocity.z).magnitude;
        public bool IsDashing => _dashTimer > 0f;
        public bool IsInvulnerable => _invulnTimer > 0f;
        public bool IsMounting => DragonController.Instance != null && DragonController.Instance.Mounted;

        private string _loadedHeroKey = "";
        private string _loadedWeaponKey = "";
        private GameObject _heroModelInstance;
        private GameObject _weaponInstance;
        private Transform _cosmeticsRoot;
        private readonly GameObject[] _orbitSpheres = new GameObject[3];
        private readonly Light[] _orbitLights = new Light[3];
        private readonly OrbType[] _orbMatType = new OrbType[3];
        private readonly List<(Material mat, SlotClass cls, Color baseColor)> _instancedLookMaterials = new List<(Material, SlotClass, Color)>();
        private readonly HashSet<EnemyAndBossAI> _meleeHitEnemies = new HashSet<EnemyAndBossAI>();
        private readonly HashSet<BreakableProp> _meleeHitProps = new HashSet<BreakableProp>();
        private readonly RaycastHit[] _aimHits = new RaycastHit[16];

        // Camera & locomotion
        private float _camYaw = 0f;
        private float _camPitch = 13f;
        private float _camDist = 5.6f;
        private float _verticalVel = 0f;
        private float _orbOrbitAngle = 0f;
        private float _alacrityTimer = 0f;
        private float _alacrityMult = 1f;
        private float _parryWindowTimer = 0f;
        private float _meleeCooldown = 0f;
        private float _comboResetTimer = 0f;
        private int _comboStep = 0;
        private int _pendingHitStep = 0;
        private bool _pendingHitApplied = true;
        private float _pendingHitAge = 0f;
        private float _pendingAttackSpeed = 1f;
        private float _dashTimer = 0f;
        private float _dashTotal = 0.36f;
        private Vector3 _dashDir = Vector3.forward;
        private float _invulnTimer = 0f;
        private Vector3 _knockback = Vector3.zero;
        private Vector3 _smoothVel = Vector3.zero;
        private Vector3 _lastSafeGround;
        private float _safeTimer = 0f;
        private float _mapRevealCooldown = 0f;
        private float _stepTimer = 0f;
        private int _footstepIndex;
        private static readonly string[] FootstepSounds =
        {
            "SFX_Step_Stone_01", "SFX_Step_Stone_02", "SFX_Step_Stone_03", "SFX_Step_Stone_04"
        };
        private float _airTime = 0f;

        // Creator Studio Mode
        public bool CreatorStudioActive { get; private set; }
        private float _studioOrbitYaw = 165f;
        private int _studioZoomMode = 0;

        // Prologue Cutscene
        private float _waterfallTimer = 0f;
        private Vector3 _cutsceneStartPos;

        // Animator parameter hashes
        private static readonly int AnimSpeed = Animator.StringToHash("Speed");
        private static readonly int AnimAttack = Animator.StringToHash("Attack");
        private static readonly int AnimAttackIndex = Animator.StringToHash("AttackIndex");
        private static readonly int AnimCast = Animator.StringToHash("Cast");
        private static readonly int AnimHit = Animator.StringToHash("HitReact");
        private static readonly int AnimDie = Animator.StringToHash("Die");
        private static readonly int AnimRespawn = Animator.StringToHash("Respawn");
        private static readonly int AnimBlocking = Animator.StringToHash("Blocking");
        private static readonly int AnimCombatSpeed = Animator.StringToHash("CombatSpeed");
        private static readonly int AnimMoveScale = Animator.StringToHash("MoveScale");
        private const float JogAuthoredSpeed = 4.0f;   // m/s the Quaternius Jog_Fwd_Loop covers per cycle (approx.)
        private bool? _animHasMoveScale;
        private Animator _animMoveScaleOwner;

        public static HeroController Instance { get; private set; }
        private Vector3 _villageSpawn;
        public Vector3 OakhavenRespawnPoint => _villageSpawn;

        private void Awake()
        {
            Instance = this;
            _villageSpawn = transform.position;
            CC = GetComponent<CharacterController>();
            CC.height = 1.85f;
            CC.radius = 0.38f;
            CC.center = new Vector3(0f, 0.925f, 0f);
            CC.stepOffset = 0.5f;
            CC.slopeLimit = 52f;
            CC.minMoveDistance = 0f;

            Overlay = gameObject.AddComponent<HeroAnimationOverlay>();
            Spells = gameObject.AddComponent<SpellSystem>();
            SkillTree = gameObject.AddComponent<SkillTreeAndSaveService>();
            Customization = gameObject.AddComponent<HeroCustomizationService>();
            Customization.OnProfileChanged += RefreshDerivedStatsAndVisuals;

            BuildOrbitingOrbs();
        }

        private void Start()
        {
            MainCam = Camera.main;
            Customization.Recalculate();
            HP = MaxHP;
            Stamina = MaxStamina;
            _lastSafeGround = transform.position;
        }

        private void BuildOrbitingOrbs()
        {
            for (int i = 0; i < 3; i++)
            {
                var sp = GameObject.CreatePrimitive(PrimitiveType.Sphere);
                sp.name = $"OrbitOrb_{i}";
                Destroy(sp.GetComponent<Collider>());
                sp.transform.SetParent(transform, false);
                sp.transform.localScale = Vector3.one * 0.17f;
                sp.GetComponent<Renderer>().material = AetheriaRender.Glow(Color.white, 5f);
                sp.GetComponent<Renderer>().shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                sp.SetActive(false);
                _orbitSpheres[i] = sp;
                _orbitLights[i] = AetheriaRender.CreatePointLight(sp.transform, Vector3.zero, Color.white, 4.5f, 2.4f);
            }
        }

        public void RefreshDerivedStatsAndVisuals()
        {
            var d = Customization.Derived;
            float hpPct = MaxHP > 0.1f ? HP / MaxHP : 1f;
            float stamPct = MaxStamina > 0.1f ? Stamina / MaxStamina : 1f;

            MaxHP = d.MaxHP + SkillTree.MaxHPBonus;
            MaxStamina = d.MaxStamina + SkillTree.MaxStaminaBonus;
            HP = Mathf.Clamp(hpPct * MaxHP, 1f, MaxHP);
            Stamina = Mathf.Clamp(stamPct * MaxStamina, 0f, MaxStamina);

            if (Spells != null)
            {
                Spells.MaxMana = d.MaxMana + SkillTree.MaxManaBonus;
                Spells.Mana = Mathf.Min(Spells.Mana, Spells.MaxMana);
                Spells.ManaRegenPerSec = d.ManaRegenPerSec * SkillTree.ManaRegenMult;
                Spells.ManaCostMultiplier = d.ManaCostMultiplier;
                Spells.CooldownMultiplier = d.SpellCooldownMultiplier * SkillTree.WexCooldownMult;
                Spells.ExortPowerScale = d.SpellPowerMultiplier * d.ExortBonusMultiplier * SkillTree.ExortDamageMult;
                Spells.WexSpeedScale = SkillTree.WexDamageMult;
                Spells.QuasControlScale = SkillTree.QuasControlMult;
                Spells.BlockedByStature = !d.CanCastSpells;
            }

            if (VisualRoot == null)
            {
                var vr = transform.Find("VisualRoot");
                if (vr != null) VisualRoot = vr;
                else
                {
                    var go = new GameObject("VisualRoot");
                    go.transform.SetParent(transform, false);
                    VisualRoot = go.transform;
                }
            }
            if (Overlay != null) Overlay.VisualRoot = VisualRoot;

            RebuildHeroVisualAppearance();

            float hScale = Mathf.Clamp(Customization.Profile.HeightMeters / HeroCustomizationService.BaseHeightMeters, 0.59f, 1.62f);
            float bulkXY = hScale * Mathf.Lerp(0.84f, 1.24f, Customization.Profile.Musculature)
                                  * Mathf.Lerp(0.92f, 1.12f, Customization.Profile.ShoulderWidth);
            VisualRoot.localScale = new Vector3(bulkXY, hScale, bulkXY);
            CC.height = 1.85f * hScale;
            CC.radius = Mathf.Max(0.28f, 0.38f * bulkXY);
            CC.center = new Vector3(0f, CC.height * 0.5f, 0f);
        }

        private void Update()
        {
            float dt = Time.deltaTime;
            UpdateOrbitingOrbs(dt);

            if (AetheriaHUDManager.Instance == null || !HasEnteredWorld)
            {
                UpdateStudioCameraIfActive(dt);
                return;
            }

            if (CinematicLock)
            {
                // Keep him settled on the ground (isGrounded only updates through Move).
                if (CC.enabled) CC.Move(Vector3.down * 3f * dt);
                DriveAnimator(0f, false, dt);
                return;
            }

            if (PrologueStage == 3)
            {
                UpdatePrologueCutscene(dt);
                return;
            }

            bool modal = AetheriaHUDManager.Instance.IsModalOpen;
            if (IsDead)
            {
                if (Input.GetKeyDown(KeyCode.E)) RespawnAtOakhaven();
                return;
            }

            if (IsMounting)
            {
                Overlay.LookWeight = 0f; Overlay.BlockWeight = 0f; Overlay.TuckWeight = 0f;
                Overlay.LeanPitch = 0f; Overlay.LeanRoll = 0f;
                return;
            }

            UpdateTimers(dt);

            if (CreatorStudioActive || modal)
            {
                // Menus freeze the hero but keep him breathing in his idle pose
                DriveAnimator(0f, false, dt);
                UpdateStudioCameraIfActive(dt);
                return;
            }

            HandleCameraAndMovement(dt);
            HandleCombatAndSpells();
            ApplyPendingMeleeHit();
            UpdatePrologueStageTriggers();
            DriveAnimator(Speed, IsBlocking, dt);

            _mapRevealCooldown -= dt;
            if (_mapRevealCooldown <= 0f)
            {
                _mapRevealCooldown = 0.6f;
                float reveal = 70f * (Customization != null ? Customization.Derived.MapRevealMultiplier : 1f);
                QuestAndMapService.Instance?.RevealAround(transform.position, reveal);
                var zone = QuestAndMapService.GetBiomeAt(transform.position);
                AudioAndQualityService.Instance?.SetZoneVillage(zone == BiomeId.OakhavenValley);
            }
        }

        private void UpdateTimers(float dt)
        {
            var d = Customization.Derived;
            if (_alacrityTimer > 0f) { _alacrityTimer -= dt; if (_alacrityTimer <= 0f) _alacrityMult = 1f; }
            if (_parryWindowTimer > 0f) _parryWindowTimer = Mathf.Max(0f, _parryWindowTimer - dt);
            if (_meleeCooldown > 0f) _meleeCooldown = Mathf.Max(0f, _meleeCooldown - dt);
            if (_comboResetTimer > 0f) { _comboResetTimer -= dt; if (_comboResetTimer <= 0f) _comboStep = 0; }
            if (_dashTimer > 0f) _dashTimer = Mathf.Max(0f, _dashTimer - dt);
            _pendingHitAge += dt;
            if (_invulnTimer > 0f) _invulnTimer = Mathf.Max(0f, _invulnTimer - dt);
            _knockback = Vector3.Lerp(_knockback, Vector3.zero, Mathf.Clamp01(dt * 9f));

            float regenHP = d.HPRegenPerSec + SkillTree.HPRegenBonus;
            HP = Mathf.Min(MaxHP, HP + (GodMode ? 50f : regenHP) * dt);

            float stamRegen = IsBlocking ? 4f : (d.StaminaRegenPerSec * SkillTree.StaminaRegenMult);
            Stamina = GodMode ? MaxStamina : Mathf.Min(MaxStamina, Stamina + stamRegen * dt);
        }

        private void HandleCameraAndMovement(float dt)
        {
            float sens = Mathf.Clamp(MouseSensitivity, 0.2f, 4.0f);
            float mx = Input.GetAxis("Mouse X") * 2.6f * sens;
            float my = Input.GetAxis("Mouse Y") * 2.1f * sens;
            _camYaw += mx;
            _camPitch = Mathf.Clamp(_camPitch - my, -24f, 68f);
            float scroll = Input.GetAxis("Mouse ScrollWheel");
            if (Mathf.Abs(scroll) > 0.001f) _camDist = Mathf.Clamp(_camDist - scroll * 3.5f, 2.6f, 12.5f);

            float ix = Input.GetAxisRaw("Horizontal");
            float iz = Input.GetAxisRaw("Vertical");
            Vector3 fwd = Quaternion.Euler(0f, _camYaw, 0f) * Vector3.forward;
            Vector3 right = Quaternion.Euler(0f, _camYaw, 0f) * Vector3.right;
            Vector3 moveDir = fwd * iz + right * ix;
            if (moveDir.sqrMagnitude > 1f) moveDir.Normalize();

            var d = Customization.Derived;
            bool moving = moveDir.sqrMagnitude > 0.01f;
            bool sprinting = Input.GetKey(KeyCode.LeftShift) && moving && !IsBlocking && Stamina > 2f;
            if (sprinting && !GodMode) Stamina = Mathf.Max(0f, Stamina - 14f * dt);

            float speed = d.MoveSpeedMetersPerSec * SkillTree.MoveSpeedMult * _alacrityMult;
            if (sprinting) speed *= 1.45f;
            if (IsBlocking) speed *= 0.42f;
            if (_pendingHitStep > 0 && !_pendingHitApplied) speed *= 0.22f;

            Vector3 target = Vector3.zero;
            if (_dashTimer > 0f)
            {
                float k = _dashTimer / _dashTotal;
                target = _dashDir * (speed * 2.5f * d.DodgeDistanceMultiplier) * Mathf.Lerp(0.55f, 1f, k);
            }
            else if (moving)
            {
                target = moveDir * speed;
                if (!IsBlocking)
                {
                    Quaternion targetRot = Quaternion.LookRotation(moveDir, Vector3.up);
                    transform.rotation = Quaternion.RotateTowards(transform.rotation, targetRot, d.TurnRateDegPerSec * dt);
                }
                else
                {
                    // Guarding: face where the camera looks, move slower sideways
                    Quaternion guardRot = Quaternion.LookRotation(fwd, Vector3.up);
                    transform.rotation = Quaternion.RotateTowards(transform.rotation, guardRot, 420f * dt);
                }
            }

            // Smooth acceleration gives the animation its weight
            float accel = (42f * d.AccelMultiplier) * dt;
            _smoothVel = Vector3.MoveTowards(_smoothVel, target, accel);
            Velocity = _smoothVel;

            if (CC.isGrounded)
            {
                _verticalVel = -2.5f;
                _airTime = 0f;
                if (Input.GetKeyDown(KeyCode.Space))
                {
                    if (Input.GetKey(KeyCode.LeftControl) || Input.GetKey(KeyCode.V) || BossNearDanger()) PerformDash(moveDir);
                    else if (GodMode || Stamina >= 8f)
                    {
                        if (!GodMode) Stamina -= 8f;
                        _verticalVel = 7.6f;
                        AetheriaFx.Puff(transform.position, new Color(0.85f, 0.78f, 0.62f, 0.5f), 5, 1.4f, 0.5f, 0.6f, 0.4f);
                    }
                }
            }
            else
            {
                _verticalVel -= 21f * dt;
                _airTime += dt;
            }

            CC.Move((_smoothVel + _knockback + Vector3.up * _verticalVel) * dt);
            KeepInsideWorld();

            // Footstep dust & rhythm
            if (CC.isGrounded && Speed > 1.5f)
            {
                _stepTimer -= dt * (Speed / 5f);
                if (_stepTimer <= 0f)
                {
                    _stepTimer = sprinting ? 0.26f : 0.34f;
                    AetheriaFx.Puff(transform.position + Vector3.up * 0.06f, new Color(0.86f, 0.80f, 0.66f, 0.22f), 2, 0.9f, sprinting ? 0.42f : 0.28f, 0.55f, 0.5f);
                    AudioAndQualityService.Instance?.PlaySfxAt(FootstepSounds[_footstepIndex], transform.position, 0.22f);
                    _footstepIndex = (_footstepIndex + 1) % FootstepSounds.Length;
                }
            }

            // Remember the last dry, flat spot: the sea returns the hero there
            float ground = AetheriaWorld.SampleHeight(transform.position);
            _safeTimer -= dt;
            if (CC.isGrounded && transform.position.y >= ground - CC.skinWidth &&
                ground > AetheriaWorld.SeaLevel + 0.25f && _safeTimer <= 0f)
            {
                _lastSafeGround = transform.position;
                _safeTimer = 0.5f;
            }
            if (transform.position.y < AetheriaWorld.SeaLevel - 0.85f || transform.position.y < ground - 0.65f)
            {
                TeleportSafe(_lastSafeGround);
                QuestAndMapService.Instance?.PushNotification("Холодные воды Этерии вынесли вас на берег");
            }

            UpdateChaseCamera(dt);
        }

        private void KeepInsideWorld()
        {
            var p = transform.position;
            float lim = AetheriaWorld.HalfExtent - 6f;
            p.x = Mathf.Clamp(p.x, -lim, lim);
            p.z = Mathf.Clamp(p.z, -lim, lim);
            if (p != transform.position) transform.position = p;
        }

        private void UpdateChaseCamera(float dt)
        {
            if (MainCam == null) return;
            float hRatio = Mathf.Clamp(Customization.Profile.HeightMeters / 1.85f, 0.59f, 1.62f);
            Vector3 pivot = transform.position + Vector3.up * (1.5f * hRatio);
            Quaternion camRot = Quaternion.Euler(_camPitch, _camYaw, 0f);
            Vector3 desired = pivot - (camRot * Vector3.forward) * _camDist;
            if (Physics.Linecast(pivot, desired, out RaycastHit hit, ~0, QueryTriggerInteraction.Ignore))
            {
                if (hit.collider.gameObject != gameObject)
                    desired = hit.point + hit.normal * 0.3f;
            }
            float follow = 1f - Mathf.Exp(-dt * 14f);
            MainCam.transform.position = Vector3.Lerp(MainCam.transform.position, desired, follow);
            MainCam.transform.LookAt(pivot);

            // Camera shake from impacts (unscaled so it still reads during hit-stop)
            var fx = AetheriaFx.Instance;
            if (fx != null && fx.ShakeOffset.sqrMagnitude > 1e-8f)
            {
                MainCam.transform.position += MainCam.transform.right * fx.ShakeOffset.x + MainCam.transform.up * fx.ShakeOffset.y;
                MainCam.transform.Rotate(0f, 0f, fx.ShakeRoll, Space.Self);
            }
        }

        private bool BossNearDanger()
        {
            var boss = EnemyAndBossAI.ActiveBoss;
            return boss != null && boss.IsTelegraphing && Vector3.Distance(transform.position, boss.transform.position) < 16f;
        }

        private void HandleCombatAndSpells()
        {
            bool rmb = Input.GetMouseButton(1) && !IsDashing;
            if (rmb != IsBlocking)
            {
                IsBlocking = rmb;
                if (IsBlocking) _parryWindowTimer = 0.28f + Customization.Derived.ParryWindowBonusSec;
            }

            if (Input.GetMouseButtonDown(0)) PerformMeleeAttack();

            // [E] Talk, kindle a shrine, or push an Exort orb
            if (Input.GetKeyDown(KeyCode.E))
            {
                var npc = FindNearestNPC(4.4f);
                var shrine = AetheriaShrine.FindNearest(transform.position, 6.5f);
                bool shrineReady = shrine != null && !shrine.Kindled;
                if (npc != null && (!shrineReady || Vector3.Distance(transform.position, npc.transform.position) <= Vector3.Distance(transform.position, shrine.transform.position)))
                    AetheriaHUDManager.Instance?.OpenDialogue(npc);
                else if (shrineReady)
                    shrine.Kindle();
                else if (Spells != null)
                    Spells.PushOrb(OrbType.Exort);
            }
            if (Input.GetKeyDown(KeyCode.Z)) Spells.PushOrb(OrbType.Exort);
            if (Input.GetKeyDown(KeyCode.X)) Spells.PushOrb(OrbType.Wex);
            if (Input.GetKeyDown(KeyCode.Q)) Spells.PushOrb(OrbType.Quas);
            if (Input.GetKeyDown(KeyCode.V)) PerformDash(CameraFlatForward());

            if (Input.GetKeyDown(KeyCode.R)) Spells.InvokeCurrentSpell();

            Vector3 aimOrigin = transform.position + Vector3.up * 1.4f;
            Ray camRay = MainCam != null ? MainCam.ViewportPointToRay(new Vector3(0.5f, 0.5f, 0f)) : new Ray(aimOrigin, transform.forward);
            Vector3 targetPt = camRay.origin + camRay.direction * 22f;
            int hitCount = Physics.RaycastNonAlloc(camRay, _aimHits, 80f, ~0, QueryTriggerInteraction.Ignore);
            float bestDist = float.MaxValue;
            bool foundHit = false;
            for (int i = 0; i < hitCount; i++)
            {
                var h = _aimHits[i];
                if (h.collider == null || h.distance < 0.5f || h.collider.transform.IsChildOf(transform)) continue;
                if (h.distance < bestDist)
                {
                    bestDist = h.distance;
                    targetPt = h.point;
                    foundHit = true;
                }
            }
            if (!foundHit) targetPt.y = AetheriaWorld.SampleHeight(targetPt);
            Vector3 toTarget = targetPt - aimOrigin;
            Vector3 aimDir = toTarget.sqrMagnitude > 0.01f ? toTarget.normalized : CameraFlatForward();

            if (Input.GetKeyDown(KeyCode.G)) Spells.CastSlot(false, aimOrigin, aimDir, targetPt);
            if (Input.GetKeyDown(KeyCode.H)) Spells.CastSlot(true, aimOrigin, aimDir, targetPt);
            if (Input.GetKeyDown(KeyCode.T)) AetheriaWorldGenerator.Instance?.CycleTimeOfDayPreset();

            KeyCode[] numKeys = { KeyCode.Alpha1, KeyCode.Alpha2, KeyCode.Alpha3, KeyCode.Alpha4, KeyCode.Alpha5,
                                  KeyCode.Alpha6, KeyCode.Alpha7, KeyCode.Alpha8, KeyCode.Alpha9, KeyCode.Alpha0 };
            for (int i = 0; i < numKeys.Length; i++)
            {
                if (Input.GetKeyDown(numKeys[i]))
                    Spells.CastSpecificSpell((SpellId)(i + 1), aimOrigin, aimDir, targetPt);
            }

            if (Input.GetKeyDown(KeyCode.F) && DragonController.Instance != null)
            {
                float dist = Vector3.Distance(transform.position, DragonController.Instance.transform.position);
                if (DragonUnlocked || dist < 16f || GodMode)
                    DragonController.Instance.Mount(this);
                else
                    QuestAndMapService.Instance?.PushNotification("Сначала укротите Игниса: «Укрощение Алого Крыла» у Драконьего Гнезда");
            }
        }

        public Vector3 CameraFlatForward()
        {
            Vector3 f = MainCam != null ? MainCam.transform.forward : transform.forward;
            f.y = 0f;
            return f.sqrMagnitude > 0.001f ? f.normalized : transform.forward;
        }

        // ------------------------------------------------------------------ animation driver

        private bool InState(int shortNameHash, out float normalizedTime)
        {
            normalizedTime = 0f;
            if (ActiveAnimator == null || !ActiveAnimator.isActiveAndEnabled || ActiveAnimator.runtimeAnimatorController == null) return false;
            var st = ActiveAnimator.GetCurrentAnimatorStateInfo(0);
            if (st.shortNameHash == shortNameHash)
            {
                normalizedTime = st.normalizedTime;
                return true;
            }
            if (ActiveAnimator.IsInTransition(0))
            {
                var next = ActiveAnimator.GetNextAnimatorStateInfo(0);
                if (next.shortNameHash == shortNameHash)
                {
                    normalizedTime = next.normalizedTime;
                    return true;
                }
            }
            return false;
        }

        private static readonly int StateAttack1 = Animator.StringToHash("Attack1");
        private static readonly int StateAttack2 = Animator.StringToHash("Attack2");
    private static readonly int StateAttack3 = Animator.StringToHash("Attack3");
        private static readonly int StateAttack4 = Animator.StringToHash("Attack4");
        private static readonly int StateCast = Animator.StringToHash("Cast");
        private static readonly int StateDeath = Animator.StringToHash("Death");

        private bool AttackState(int step, out float t)
        {
            switch (step)
            {
                case 1: return InState(StateAttack1, out t);
                case 2: return InState(StateAttack2, out t);
                case 3: return InState(StateAttack3, out t);
                case 4: return InState(StateAttack4, out t);
                default: t = 0f; return false;
            }
        }

        public bool IsAttacking => _pendingHitStep > 0 || (_heroModelInstance != null && ActiveAnimator != null &&
            (InState(StateAttack1, out _) || InState(StateAttack2, out _) || InState(StateAttack3, out _) || InState(StateAttack4, out _)));

        private void DriveAnimator(float speed, bool blocking, float dt)
        {
            bool animated = ActiveAnimator != null && ActiveAnimator.isActiveAndEnabled && ActiveAnimator.runtimeAnimatorController != null;
            if (animated)
            {
                // Jog is authored at about 6.2 m/s; CombatSpeed scales attack and cast clips by the hero's attack speed
                ActiveAnimator.SetFloat(AnimSpeed, speed);
                // Time-scale the jog to the real speed so walking, jogging and sprinting all keep feet planted
                if (_animHasMoveScale == null || _animMoveScaleOwner != ActiveAnimator)
                {
                    _animMoveScaleOwner = ActiveAnimator;
                    _animHasMoveScale = CreatureAnimator.HasParameter(ActiveAnimator, AnimMoveScale);
                }
                if (_animHasMoveScale == true)
                    ActiveAnimator.SetFloat(AnimMoveScale, Mathf.Clamp(speed / JogAuthoredSpeed, 0.4f, 2.0f));
                float attackSpeed = Mathf.Max(0.5f, Customization.Derived.AttackSpeedMultiplier * _alacrityMult);
                ActiveAnimator.SetFloat(AnimCombatSpeed, attackSpeed);
                ActiveAnimator.SetBool(AnimBlocking, blocking);
            }
            else if (Time.frameCount % 30 == 0 && _heroModelInstance != null)
            {
                // No controller: fall back to the relaxed mannequin stance
                AetheriaPalette.RelaxHumanoidArms(_heroModelInstance);
            }

            if (Overlay == null) return;

            // Overlay weights ease in and out so poses never snap
            Overlay.BlockWeight = Mathf.MoveTowards(Overlay.BlockWeight, blocking && !IsDashing ? 1f : 0f, dt * 7f);
            float tuckTarget = (!CC.isGrounded && _airTime > 0.08f) ? 1f : (IsDashing ? 0.3f : 0f);
            Overlay.TuckWeight = Mathf.MoveTowards(Overlay.TuckWeight, tuckTarget, dt * 6.5f);

            // Lean into acceleration and turns
            Vector3 local = transform.InverseTransformDirection(Velocity);
            float pitchTarget = Mathf.Clamp(local.z * 0.6f, -3f, 5f);   // the jog clip already leans forward
            float rollTarget = Mathf.Clamp(-local.x * 1.9f, -12f, 12f);
            Overlay.LeanPitch = Mathf.MoveTowards(Overlay.LeanPitch, IsDashing ? 10f : pitchTarget, dt * 60f);
            Overlay.LeanRoll = Mathf.MoveTowards(Overlay.LeanRoll, rollTarget, dt * 90f);

            // The head follows the look direction while the body is free
            bool headTrack = !CinematicLock && !blocking && !IsDashing && !IsAttacking && !InState(StateCast, out _);
            // Head tracking of the camera is off: on these rigs it made the head swivel constantly with every
            // camera move. The animation clips own the head.
            Overlay.LookWeight = 0f;
            if (MainCam != null) Overlay.LookTarget = transform.position + MainCam.transform.forward * 9f + Vector3.up * 1.55f;
        }


        public void PerformMeleeAttack()
        {
            if (_meleeCooldown > 0f || _dashTimer > 0f || IsBlocking || IsDead || IsMounting) return;
            var d = Customization.Derived;
            if (!GodMode && Stamina < 10f) return;
            if (!GodMode) Stamina -= 10f;

            _comboStep = (_comboStep % 4) + 1;
            _comboResetTimer = 1.25f;
            float attackSpeed = Mathf.Max(0.5f, d.AttackSpeedMultiplier * _alacrityMult);
            _meleeCooldown = 0.46f / attackSpeed;

            // Gently auto-face nearest alive enemy within 4.5m, else camera forward
            Vector3 faceDir = CameraFlatForward();
            float bestDistSq = 4.5f * 4.5f;
            foreach (var col in Physics.OverlapSphere(transform.position, 4.5f, ~0, QueryTriggerInteraction.Ignore))
            {
                if (col.transform.IsChildOf(transform)) continue;
                var enemy = col.GetComponentInParent<EnemyAndBossAI>();
                if (enemy == null || enemy.Dead) continue;
                Vector3 toEnemy = enemy.transform.position - transform.position;
                toEnemy.y = 0f;
                if (toEnemy.sqrMagnitude > 0.04f && toEnemy.sqrMagnitude < bestDistSq)
                {
                    bestDistSq = toEnemy.sqrMagnitude;
                    faceDir = toEnemy.normalized;
                }
            }
            if (faceDir.sqrMagnitude > 0.001f)
                transform.rotation = Quaternion.RotateTowards(transform.rotation, Quaternion.LookRotation(faceDir, Vector3.up), 120f);

            // The blow lands on the swing frame, not on the click
            _pendingHitStep = _comboStep;
            _pendingHitApplied = false;
            _pendingHitAge = 0f;
            _pendingAttackSpeed = attackSpeed;

            bool animated = ActiveAnimator != null && ActiveAnimator.isActiveAndEnabled && ActiveAnimator.runtimeAnimatorController != null;
            if (animated)
            {
                ActiveAnimator.SetInteger(AnimAttackIndex, _comboStep);
                ActiveAnimator.SetTrigger(AnimAttack);
            }

            // Small lunge toward the target for weight
            _smoothVel += transform.forward * (2.6f * attackSpeed);
        }

        /// <summary>Deals the damage of the current swing once its clip passes the strike frame.</summary>
        private void ApplyPendingMeleeHit()
        {
            if (_pendingHitStep <= 0 || _pendingHitApplied) return;

            float invSpeed = 1f / Mathf.Max(0.5f, _pendingAttackSpeed);
            bool animated = ActiveAnimator != null && ActiveAnimator.isActiveAndEnabled && ActiveAnimator.runtimeAnimatorController != null;
            if (animated)
            {
                float timeout = 0.7f * invSpeed;
                bool reached = AttackState(_pendingHitStep, out float t) && t >= HitFrameOf(_pendingHitStep);
                if (!reached && _pendingHitAge < timeout) return;
            }
            else
            {
                if (_pendingHitAge < 0.22f * invSpeed) return;
            }
            _pendingHitApplied = true;

            var d = Customization.Derived;
            int step = _pendingHitStep;
            float reach = (3.3f + step * 0.15f) * d.MeleeReachMultiplier;
            float dmg = (28f + step * 8f) * d.MeleeDamageMultiplier * SkillTree.MeleeDamageMult;
            bool crit = Random.value < d.CritChance;
            if (crit) dmg *= d.CritDamageMultiplier;

            Vector3 hitCenter = transform.position + transform.forward * (reach * 0.55f) + Vector3.up * 1.15f;
            int hits = 0;
            _meleeHitEnemies.Clear();
            _meleeHitProps.Clear();
            foreach (var col in Physics.OverlapSphere(hitCenter, reach * 0.68f, ~0, QueryTriggerInteraction.Ignore))
            {
                if (col.gameObject == gameObject || col.transform.IsChildOf(transform)) continue;
                var enemy = col.GetComponentInParent<EnemyAndBossAI>();
                if (enemy != null && _meleeHitEnemies.Add(enemy))
                {
                    float mult = enemy.IsStaggerBroken ? SkillTree.ExecutionerMult : 1f;
                    enemy.TakeDamage(dmg * mult, gameObject, transform.forward * (6f + step * 2f) + Vector3.up * (step == 4 ? 5f : 2f), OrbType.None);
                    hits++;
                }
                var prop = col.GetComponentInParent<BreakableProp>();
                if (prop != null && _meleeHitProps.Add(prop) && prop.ApplyDamage(dmg * d.DestructionDamageMultiplier, hitCenter, false, false)) hits++;
            }

            // Swing trail
            Vector3 trailAt = transform.position + Vector3.up * 1.25f + transform.right * 0.35f;
            AetheriaFx.Burst(trailAt + transform.forward * 0.5f, new Color(1f, 0.88f, 0.66f), 12, 3f, 7.5f, 0.10f, 0.22f, 0.0f, 2.4f, new Color(1f, 0.55f, 0.2f, 0f), true,
                transform.forward + transform.right * (step % 2 == 0 ? -0.5f : 0.5f), 0.2f);

            if (hits > 0)
            {
                AudioAndQualityService.Instance?.PlaySfxAt(crit ? "SFX_HitImpact" : "SFX_Hit_Flesh", hitCenter, 0.9f);
                AudioAndQualityService.Instance?.RequestHitStop(step == 4 ? 0.075f : 0.045f);
                AetheriaFx.FlashLight(hitCenter, new Color(1f, 0.62f, 0.30f), crit ? 26f : 14f, 9f, 0.22f);
                AetheriaFx.Burst(hitCenter, new Color(1f, 0.70f, 0.36f), crit ? 26 : 14, 3f, 9f, 0.08f, 0.20f, 0.40f, 0.5f, 2.2f, new Color(0.8f, 0.2f, 0.05f, 0f));
                if (crit) AetheriaFx.Flare(hitCenter, new Color(1f, 0.85f, 0.5f), 1.6f, 0.22f);
                AetheriaFx.Shake(step == 4 ? 0.22f : 0.09f, step == 4 ? 0.32f : 0.16f);
                AudioAndQualityService.Instance?.NotifyCombat();
            }

            _pendingHitStep = 0;
        }

        private static float HitFrameOf(int step) => step switch { 1 => 0.36f, 2 => 0.34f, 3 => 0.38f, _ => 0.46f };

        public void PerformDash(Vector3 dir)
        {
            if (_dashTimer > 0f || IsDead || IsMounting) return;
            var d = Customization.Derived;
            float cost = 18f * d.DodgeStaminaCostMultiplier;
            if (!GodMode && Stamina < cost) return;
            if (!GodMode) Stamina -= cost;

            _dashDir = dir.sqrMagnitude > 0.05f ? new Vector3(dir.x, 0f, dir.z).normalized : transform.forward;
            _dashTimer = _dashTotal = 0.36f;
            _invulnTimer = 0.38f + d.DodgeIFrameBonusSec + (SkillTree != null ? SkillTree.DodgeIFrameMult - 1f : 0f) * 0.3f;
            _pendingHitStep = 0;
            transform.rotation = Quaternion.LookRotation(_dashDir, Vector3.up);

            AetheriaFx.FlashLight(transform.position + Vector3.up, new Color(0.30f, 0.86f, 1f), 11f, 7f, 0.25f);
            AetheriaFx.Puff(transform.position + Vector3.up * 0.2f, new Color(0.78f, 0.92f, 1f, 0.40f), 9, 2.6f, 0.75f, 0.55f, 0.2f);
            AetheriaFx.Shockwave(transform.position + Vector3.up * 0.9f, new Color(0.36f, 0.90f, 1f), 1.5f, 0.32f, vertical: true, facing: _dashDir);
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_Dash", transform.position, 0.75f);
        }

        public void TakeDamage(float rawDmg, GameObject attacker, bool isParryableBossStrike = false)
        {
            if (IsDead || GodMode || _invulnTimer > 0f) return;

            // Timed parry
            if (IsBlocking && _parryWindowTimer > 0f)
            {
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_Hit_Armor", transform.position, 0.9f);
                AudioAndQualityService.Instance?.RequestHitStop(0.09f, 0.05f);
                Spells.Mana = Mathf.Min(Spells.MaxMana, Spells.Mana + 20f + SkillTree.ParryManaRestore);
                Vector3 at = transform.position + transform.forward * 0.7f + Vector3.up * 1.2f;
                AetheriaFx.FlashLight(at, new Color(1f, 0.78f, 0.16f), 34f, 12f, 0.34f);
                AetheriaFx.Flare(at, new Color(1f, 0.84f, 0.35f), 2.4f, 0.28f);
                AetheriaFx.Burst(at, new Color(1f, 0.86f, 0.45f), 30, 4f, 11f, 0.08f, 0.20f, 0.1f, 1.2f, new Color(1f, 0.5f, 0.1f, 0f));
                AetheriaFx.Shockwave(at, new Color(1f, 0.80f, 0.30f), 2.6f, 0.30f, vertical: true, facing: transform.forward);
                AetheriaFx.Shake(0.16f, 0.24f);
                QuestAndMapService.Instance?.PushNotification("ИДЕАЛЬНОЕ ПАРИРОВАНИЕ! +Мана +Оглушение");
                if (attacker != null) attacker.GetComponentInParent<EnemyAndBossAI>()?.ApplyParryStagger(Customization.Derived.ParryStaggerBonusMult);
                return;
            }

            float mit = Customization.Derived.ArmorReduction + (IsBlocking ? 0.45f + SkillTree.ShieldMitigation : 0f);
            float dmg = Mathf.Max(1f, rawDmg * (1f - Mathf.Clamp(mit, 0f, 0.85f)));
            HP = Mathf.Max(0f, HP - dmg);

            Vector3 hitAt = transform.position + Vector3.up * 1.2f;
            Color blood = new Color(0.95f, 0.16f, 0.12f);
            AetheriaFx.FlashLight(hitAt, blood, 20f, 10f, 0.30f);
            AetheriaFx.Burst(hitAt, blood, IsBlocking ? 10 : 22, 2.5f, 8f, 0.08f, 0.20f, 0.6f, 1.8f, new Color(0.45f, 0.05f, 0.02f, 0f));
            if (IsBlocking) AetheriaFx.Lightning(hitAt, hitAt + (attacker != null ? (attacker.transform.position - hitAt).normalized : transform.forward) * 1.4f, new Color(1f, 0.85f, 0.5f), 0.14f, 0.06f, 4, 0.4f);
            AetheriaFx.Shake(IsBlocking ? 0.10f : 0.24f, 0.26f);
            AudioAndQualityService.Instance?.PlaySfxAt(IsBlocking ? "SFX_Hit_Shield_Absorb" : "SFX_Hit_Armor", hitAt, 0.9f);

            // Recoil: a shove away from the attacker and a flinch
            Vector3 away = attacker != null ? transform.position - attacker.transform.position : -transform.forward;
            away.y = 0f;
            _knockback = away.normalized * (IsBlocking ? 2.2f : 5.5f);
            if (!IsBlocking && ActiveAnimator != null && ActiveAnimator.isActiveAndEnabled && ActiveAnimator.runtimeAnimatorController != null)
            {
                ActiveAnimator.SetTrigger(AnimHit);
                _pendingHitStep = 0;
            }

            if (HP <= 0f) Die();
        }

        public void Die()
        {
            if (IsDead) return;
            if (DragonController.Instance != null && DragonController.Instance.Mounted)
                DragonController.Instance.Dismount();

            IsDead = true;
            IsBlocking = false;
            _pendingHitStep = 0;
            _pendingHitApplied = true;
            _dashTimer = 0f;
            _invulnTimer = 0f;
            if (ActiveAnimator != null && ActiveAnimator.isActiveAndEnabled && ActiveAnimator.runtimeAnimatorController != null)
                ActiveAnimator.SetTrigger(AnimDie);
            AetheriaFx.Shake(0.5f, 0.6f);
            AetheriaFx.Puff(transform.position + Vector3.up * 0.4f, new Color(0.25f, 0.22f, 0.28f, 0.55f), 16, 2.2f, 1.4f, 1.6f, 0.6f);
            QuestAndMapService.Instance?.PushNotification("ВЫ ПАЛИ — нажмите [E], чтобы вернуться в Оукхейвен");
        }

        public void ReviveAtSanctuary() => RespawnAtOakhaven();

        public void RespawnAtOakhaven()
        {
            AetheriaPrologueDirector.Instance?.Abort();
            if (DragonController.Instance != null && DragonController.Instance.Mounted)
                DragonController.Instance.Dismount();
            IsDead = false;
            IsBlocking = false;
            _pendingHitStep = 0;
            _pendingHitApplied = true;
            _dashTimer = 0f;
            _invulnTimer = 0f;
            _comboStep = 0;
            _comboResetTimer = 0f;
            HP = MaxHP;
            Stamina = MaxStamina;
            if (Spells != null) Spells.Mana = Spells.MaxMana;
            if (ActiveAnimator != null && ActiveAnimator.isActiveAndEnabled && ActiveAnimator.runtimeAnimatorController != null)
                ActiveAnimator.SetTrigger(AnimRespawn);
            TeleportSafe(OakhavenRespawnPoint);
            EnemyAndBossAI.ActiveBoss?.ResetBoss();
            AetheriaFx.FlashLight(transform.position + Vector3.up, new Color(0.45f, 1f, 0.85f), 30f, 14f, 0.9f);
            AetheriaFx.Shockwave(transform.position + Vector3.up * 0.15f, new Color(0.45f, 1f, 0.85f), 5f, 0.9f);
        }

        public void ResetHeroForNewGame()
        {
            if (DragonController.Instance != null && DragonController.Instance.Mounted)
                DragonController.Instance.Dismount();
            IsDead = false;
            IsBlocking = false;
            DragonUnlocked = false;
            EnemiesSlain = 0;
            _pendingHitStep = 0;
            _pendingHitApplied = true;
            _dashTimer = 0f;
            _invulnTimer = 0f;
            _alacrityTimer = 0f;
            _alacrityMult = 1f;
            _comboStep = 0;
            _comboResetTimer = 0f;
            _meleeCooldown = 0f;
            _parryWindowTimer = 0f;
            _knockback = Vector3.zero;
            _smoothVel = Vector3.zero;
            _verticalVel = 0f;
            Customization?.Recalculate();
            HP = MaxHP;
            Stamina = MaxStamina;
            if (Spells != null)
            {
                if (!GodMode)
                {
                    Spells.UnlockedExort = false;
                    Spells.UnlockedWex = false;
                    Spells.UnlockedQuas = false;
                    Spells.ActiveOrbs.Clear();
                    Spells.SlotD = SpellId.None;
                    Spells.SlotF = SpellId.None;
                }
                Spells.Mana = Spells.MaxMana;
            }
            if (ActiveAnimator != null && ActiveAnimator.isActiveAndEnabled && ActiveAnimator.runtimeAnimatorController != null)
                ActiveAnimator.SetTrigger(AnimRespawn);
        }

        public void TeleportSafe(Vector3 target)
        {
            CC.enabled = false;
            target.y = Mathf.Max(target.y, AetheriaWorld.SampleHeight(target) + 0.35f);
            transform.position = target;
            _smoothVel = Vector3.zero; _verticalVel = 0f; _knockback = Vector3.zero;
            if (AetheriaWorld.SampleHeight(target) > AetheriaWorld.SeaLevel + 0.25f)
                _lastSafeGround = target;
            CC.enabled = true;
            SnapCameraBehindHero();
        }

        public void SnapCameraBehindHero()
        {
            if (MainCam == null) MainCam = Camera.main;
            if (MainCam == null) return;
            _camYaw = transform.eulerAngles.y;
            float hRatio = Mathf.Clamp(Customization != null ? Customization.Profile.HeightMeters / 1.85f : 1f, 0.59f, 1.62f);
            Vector3 pivot = transform.position + Vector3.up * (1.5f * hRatio);
            Quaternion camRot = Quaternion.Euler(_camPitch, _camYaw, 0f);
            MainCam.transform.position = pivot - (camRot * Vector3.forward) * _camDist;
            MainCam.transform.LookAt(pivot);
        }

        public void ApplyAlacrityBuff(float dur, float mult) { _alacrityTimer = dur; _alacrityMult = mult; }

        /// <summary>Spell system hook: the body joins the cast.</summary>
        public void PlayCastAnimation()
        {
            if (IsDead || IsDashing) return;
            if (ActiveAnimator != null && ActiveAnimator.isActiveAndEnabled && ActiveAnimator.runtimeAnimatorController != null)
            {
                ActiveAnimator.SetTrigger(AnimCast);
            }
            AetheriaFx.Shockwave(transform.position + Vector3.up * 1.1f, new Color(0.75f, 0.85f, 1f), 1.4f, 0.4f, vertical: true, facing: transform.forward);
        }

        public void SetGodMode(bool on)
        {
            GodMode = on;
            Spells?.SetGodMode(on);
            if (on) { DragonUnlocked = true; HP = MaxHP; Stamina = MaxStamina; IsDead = false; }
        }

        public void SetCreatorStudioCamera(bool active, float orbitYaw = 165f, int zoom = 0)
        {
            CreatorStudioActive = active;
            _studioOrbitYaw = orbitYaw;
            _studioZoomMode = zoom;
        }

        private void UpdateStudioCameraIfActive(float dt)
        {
            if (!CreatorStudioActive || MainCam == null) return;
            float hRatio = Mathf.Clamp(Customization.Profile.HeightMeters / 1.85f, 0.59f, 1.62f);
            float focusY = (_studioZoomMode == 1 ? 1.55f : 0.95f) * hRatio;
            float dist = (_studioZoomMode == 1 ? 1.8f : 3.4f) * Mathf.Max(0.8f, hRatio);
            Vector3 pivot = transform.position + Vector3.up * focusY;
            Quaternion q = Quaternion.Euler(7f, transform.eulerAngles.y + _studioOrbitYaw, 0f);
            MainCam.transform.position = Vector3.Lerp(MainCam.transform.position, pivot - q * Vector3.forward * dist, Mathf.Clamp01(dt * 10f));
            MainCam.transform.LookAt(pivot);
        }

        public void StartPrologueAwakening()
        {
            // The directed prologue (cave, sage, collapse, waterfall, glade) when the scene has one.
            if (AetheriaPrologueDirector.Instance != null)
            {
                AetheriaPrologueDirector.Instance.Begin(this);
                return;
            }
            HasEnteredWorld = true;
            PrologueStage = 0;
            var tutor = AetheriaNPC.AllNPCs.Find(npc => npc.NpcId == "Npc_CaveTutor");
            var cliff = tutor != null ? tutor.transform.position : AetheriaWorld.FindSite("PrologueCliff").Position;
            transform.rotation = Quaternion.Euler(0f, 90f, 0f);
            TeleportSafe(new Vector3(cliff.x - 2.4f, AetheriaWorld.SeaLevel, cliff.z));
            QuestAndMapService.Instance.TrackedQuestId = "Q_PrologueAwakening";
            QuestAndMapService.Instance?.PushNotification("ПЕЩЕРА ПРОБУЖДЕНИЯ · Поговорите с Хранителем Вейленом [E]");
        }

        public void SkipToOpenWorld(bool loadSave)
        {
            AetheriaPrologueDirector.Instance?.Abort();
            HasEnteredWorld = true;
            PrologueStage = 4;
            if (loadSave && SkillTree.LoadProgress(this)) return;
            transform.rotation = Quaternion.Euler(0f, 0f, 0f);
            TeleportSafe(OakhavenRespawnPoint);
            QuestAndMapService.Instance?.PushNotification("Оукхейвен · Подойдите к старейшине Олдрику [E]");
        }

        private void UpdatePrologueStageTriggers()
        {
            if (PrologueStage >= 4 || AetheriaPrologueDirector.Instance != null) return;
            var quest = QuestAndMapService.Instance?.FindQuest("Q_PrologueAwakening");
            if (quest == null || quest.State != QuestState.Active) return;
            Vector3 offset = transform.position - quest.ObjectiveWorldLocation;
            offset.y = 0f;
            if (PrologueStage <= 1 && offset.sqrMagnitude < 9f)
            {
                PrologueStage = 3;
                _waterfallTimer = 0f;
                _cutsceneStartPos = transform.position;
                QuestAndMapService.Instance?.AdvanceQuestProgress("Q_PrologueAwakening", 1);
            }
        }

        private void UpdatePrologueCutscene(float dt)
        {
            _waterfallTimer += dt;
            float u = Mathf.Clamp01(_waterfallTimer / 3.6f);
            var bridge = AetheriaWorld.FindSite("MirrorLake").Position;
            Vector3 landing = AetheriaWorld.UsesCourtyardLayout ? bridge : new Vector3(-84f, 0f, -60f);
            if (AetheriaWorld.UsesCourtyardLayout)
            {
                landing = Vector3.MoveTowards(landing, OakhavenRespawnPoint, 18f);
                while ((landing - OakhavenRespawnPoint).sqrMagnitude > 1f && AetheriaWorld.SampleHeight(landing) < 1.1f)
                    landing = Vector3.MoveTowards(landing, OakhavenRespawnPoint, 2f);
            }
            landing.y = AetheriaWorld.SampleHeight(landing) + 0.4f;
            Vector3 pos = Vector3.Lerp(_cutsceneStartPos, landing, u);
            pos.y += Mathf.Sin(u * Mathf.PI) * 6.5f;
            CC.enabled = false;
            transform.position = pos;
            CC.enabled = true;
            if (MainCam != null)
            {
                MainCam.transform.position = pos + new Vector3(-5f, 3.6f, -7f);
                MainCam.transform.LookAt(pos + new Vector3(1.5f, 1f, 9f));
            }
            if (u >= 1f)
            {
                PrologueStage = 4;
                TeleportSafe(landing);
                QuestAndMapService.Instance?.TurnInQuest("Q_PrologueAwakening", this);
                AetheriaFx.Shockwave(landing, new Color(0.55f, 0.88f, 1f), 4.5f, 0.55f);
                AetheriaFx.Puff(landing, new Color(0.84f, 0.90f, 0.96f, 0.55f), 14, 3.2f, 1.4f, 1.0f, 0.35f);
                AetheriaFx.Burst(landing + Vector3.up * 0.25f, new Color(0.65f, 0.92f, 1f), 22, 2.5f, 7f, 0.08f, 0.18f, 0.6f, 1.2f, new Color(0.3f, 0.7f, 1f, 0f));
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_Step_Stone_01", landing, 0.7f);
                AetheriaFx.Shake(0.18f, 0.25f);
                QuestAndMapService.Instance?.PushNotification("Зеркальное озеро · через Каменный мост в Оукхейвен");
            }
        }

        private void UpdateOrbitingOrbs(float dt)
        {
            _orbOrbitAngle += dt * 115f;
            for (int i = 0; i < 3; i++)
            {
                bool show = Spells != null && i < Spells.ActiveOrbs.Count && !IsDead;
                _orbitSpheres[i].SetActive(show);
                if (!show) continue;
                float ang = (_orbOrbitAngle + i * 120f) * Mathf.Deg2Rad;
                float hRatio = VisualRoot != null ? VisualRoot.localScale.y : 1f;
                _orbitSpheres[i].transform.localPosition = new Vector3(Mathf.Cos(ang) * 0.74f, (1.55f + Mathf.Sin(ang * 2f) * 0.12f) * hRatio, Mathf.Sin(ang) * 0.74f - 0.15f);
                Color col = AetheriaPalette.GetOrbColor(Spells.ActiveOrbs[i]);
                if (_orbMatType[i] != Spells.ActiveOrbs[i])
                {
                    _orbMatType[i] = Spells.ActiveOrbs[i];
                    _orbitSpheres[i].GetComponent<Renderer>().sharedMaterial = AetheriaRender.Glow(col, 5f);
                }
                AetheriaRender.StyleFxLight(_orbitLights[i], col, 4.5f, 2.6f);
            }
        }

        public static AetheriaNPC FindNearestNPC(float maxDist)
        {
            if (Instance == null) return null;
            AetheriaNPC best = null;
            float dBest = maxDist;
            foreach (var n in AetheriaNPC.AllNPCs)
            {
                if (n == null) continue;
                float d = Vector3.Distance(Instance.transform.position, n.transform.position);
                if (d < dBest) { dBest = d; best = n; }
            }
            return best;
        }

        // ---- Live 3D Visual Customization & Bone Attachment ----
        private void RebuildHeroVisualAppearance()
        {
            if (VisualRoot == null || Customization == null) return;
            var p = Customization.Profile;

            if (_heroModelInstance == null && VisualRoot.childCount > 0)
            {
                for (int i = 0; i < VisualRoot.childCount; i++)
                {
                    var ch = VisualRoot.GetChild(i).gameObject;
                    if (ch.name != "HeroCosmeticsRoot" && ch.name != "HeroWeaponAnchor")
                    {
                        _heroModelInstance = ch;
                        break;
                    }
                }
            }

            string wantHero = HeroCustomizationService.GetHeroPrefabResourceName(p);
            if (_loadedHeroKey != wantHero || _heroModelInstance == null)
            {
                var prefab = Resources.Load<GameObject>(wantHero) ?? Resources.Load<GameObject>("Heroes/Hero_Kwang");
                    DestroyLookMaterials();
                    if (_heroModelInstance != null) Destroy(_heroModelInstance);
                    _heroModelInstance = null;
                    ActiveAnimator = null;
                if (prefab != null)
                {
                    _heroModelInstance = Instantiate(prefab, VisualRoot);
                    _heroModelInstance.name = "HeroModelMesh";
                    NormalizeGroundedHeight(_heroModelInstance, HeroCustomizationService.BaseHeightMeters);
                    _loadedHeroKey = wantHero;
                    _loadedWeaponKey = "";
                }
                if (_heroModelInstance == null)
                {
                    var cap = GameObject.CreatePrimitive(PrimitiveType.Capsule);
                    cap.name = "HeroFallbackBody";
                    cap.GetComponent<Collider>().enabled = false;
                    cap.transform.SetParent(VisualRoot, false);
                    cap.transform.localPosition = new Vector3(0f, 0.925f, 0f);
                    _heroModelInstance = cap;
                }
            }

            SetupAnimator();
            AttachRightHandWeapon(p);
            RebuildCosmeticsAndMaterials(p);
        }

        private void SetupAnimator()
        {
            if (_heroModelInstance == null) { ActiveAnimator = null; return; }
            var anim = _heroModelInstance.GetComponentInChildren<Animator>(true);
            if (anim != null && anim.isActiveAndEnabled && anim.runtimeAnimatorController != null &&
                (anim.avatar == null || anim.avatar.isValid))
            {
                anim.applyRootMotion = false;
                anim.cullingMode = AnimatorCullingMode.AlwaysAnimate;
                anim.updateMode = AnimatorUpdateMode.Normal;
                ActiveAnimator = anim;
                if (Overlay != null) Overlay.Bind(_heroModelInstance);
                return;
            }
            ActiveAnimator = null;
            if (anim != null) anim.enabled = false;
            AetheriaPalette.RelaxHumanoidArms(_heroModelInstance);
            if (Overlay != null) Overlay.Bind(_heroModelInstance);
        }

        private void NormalizeGroundedHeight(GameObject model, float targetHeight)
        {
            model.transform.localPosition = Vector3.zero;
            model.transform.localRotation = Quaternion.identity;
            model.transform.localScale = Vector3.one;
            var rends = model.GetComponentsInChildren<SkinnedMeshRenderer>(true);
            if (rends.Length == 0) return;
            Bounds b = rends[0].bounds;
            for (int i = 1; i < rends.Length; i++) b.Encapsulate(rends[i].bounds);
            float h = Mathf.Max(0.05f, b.size.y);
            model.transform.localScale = Vector3.one * (targetHeight / h);
            rends = model.GetComponentsInChildren<SkinnedMeshRenderer>(true);
            b = rends[0].bounds;
            for (int i = 1; i < rends.Length; i++) b.Encapsulate(rends[i].bounds);
            model.transform.position += Vector3.up * (transform.position.y - b.min.y);
        }

        private void AttachRightHandWeapon(HeroCustomizationProfile p)
        {
            string wantWep = HeroCustomizationService.GetWeaponResourceName(p);
            bool bakedMeleeHero = (_loadedHeroKey.Contains("Kwang") || _loadedHeroKey.Contains("Serath") ||
                                   _loadedHeroKey.Contains("Kallari") || _loadedHeroKey.Contains("Khaimera") ||
                                   _loadedHeroKey.Contains("Morigesh"))
                && !wantWep.Contains("Staff") && !wantWep.Contains("Scepter");
            if (bakedMeleeHero)
            {
                if (_weaponInstance != null) Destroy(_weaponInstance);
                _weaponInstance = null;
                _loadedWeaponKey = wantWep;
                return;
            }
            if (_loadedWeaponKey == wantWep && _weaponInstance != null) return;
            if (_weaponInstance != null) Destroy(_weaponInstance);
            _loadedWeaponKey = wantWep;

            var wepPrefab = Resources.Load<GameObject>(wantWep) ?? Resources.Load<GameObject>("Weapons/LongSword");
            if (wepPrefab == null) return;

            Transform hand = AetheriaPalette.FindBoneAny(_heroModelInstance != null ? _heroModelInstance.transform : VisualRoot,
                "weapon_r", "hand_r", "RightHand", "Bip001_R_Hand");
            Transform parentTf = hand != null ? hand : VisualRoot;
            _weaponInstance = Instantiate(wepPrefab, parentTf);
            _weaponInstance.name = "HeroEquippedWeapon";
            // Undo parent hierarchy scale so weapons keep a consistent world size on any hero stature
            Vector3 pls = parentTf.lossyScale;
            _weaponInstance.transform.localScale = new Vector3(1f / Mathf.Max(0.25f, pls.x), 1f / Mathf.Max(0.25f, pls.y), 1f / Mathf.Max(0.25f, pls.z));
            if (hand != null)
            {
                _weaponInstance.transform.localPosition = new Vector3(0.02f, 0.03f, 0.05f);
                _weaponInstance.transform.localRotation = Quaternion.Euler(0f, 0f, 0f);
            }
            else
            {
                _weaponInstance.transform.localPosition = new Vector3(0.46f, 0.96f, 0.34f);
                _weaponInstance.transform.localRotation = Quaternion.Euler(28f, 0f, -15f);
            }
            foreach (var c in _weaponInstance.GetComponentsInChildren<Collider>(true)) c.enabled = false;
        }

        private void RebuildCosmeticsAndMaterials(HeroCustomizationProfile p)
        {
            if (_cosmeticsRoot == null)
            {
                var go = new GameObject("HeroCosmeticsRoot");
                go.transform.SetParent(VisualRoot, false);
                _cosmeticsRoot = go.transform;
            }
            for (int i = _cosmeticsRoot.childCount - 1; i >= 0; i--) Destroy(_cosmeticsRoot.GetChild(i).gameObject);

            if (p.HairStyle == HeroHairStyle.ArchmageHood || p.HairStyle == HeroHairStyle.CrownCirclet)
            {
                string cosKey = p.HairStyle == HeroHairStyle.ArchmageHood ? "Cosmetics/AbyssHood" : "Cosmetics/CrownCirclet";
                var cosPrefab = Resources.Load<GameObject>(cosKey);
                if (cosPrefab != null)
                {
                    var cg = Instantiate(cosPrefab, _cosmeticsRoot);
                    cg.transform.localPosition = new Vector3(0f, 1.70f, 0.02f);
                    cg.transform.localScale = Vector3.one * 0.24f;
                    foreach (var c in cg.GetComponentsInChildren<Collider>(true)) c.enabled = false;
                }
            }

            ApplyLookTints();
        }

        private void DestroyLookMaterials()
        {
            for (int i = 0; i < _instancedLookMaterials.Count; i++)
            {
                if (_instancedLookMaterials[i].mat != null) Destroy(_instancedLookMaterials[i].mat);
            }
            _instancedLookMaterials.Clear();
        }

        /// <summary>
        /// Recolors the hero by meaning: skin follows the race, hair the hair color, armor and leather the armor tint,
        /// cloth only takes a whisper of it. Each renderer slot keeps its own cloned material instance so unique textures stay intact.
        /// </summary>
        private void ApplyLookTints()
        {
            if (_heroModelInstance == null) return;
            var look = _heroModelInstance.GetComponent<AetheriaCharacterLook>();
            // The original Kwang prefab was staged without the slot metadata used by
            // the customizer. Its numbered fallback materials even put metal on the eyes.
            // Restrict this mapping to the audited base mesh; other skins need their own audit.
            if (look == null && _heroModelInstance.name == "Hero_Kwang(Clone)")
            {
                var renderer = _heroModelInstance.GetComponentsInChildren<SkinnedMeshRenderer>(true)
                    .FirstOrDefault(r => (r.transform == _heroModelInstance.transform || r.name == "Hero_Kwang") && r.sharedMesh != null && r.sharedMesh.subMeshCount == 18);
                if (renderer != null)
                {
                    look = _heroModelInstance.AddComponent<AetheriaCharacterLook>();
                    var classes = new SlotClass[renderer.sharedMaterials.Length];
                    for (int i = 0; i < classes.Length; i++) classes[i] = SlotClass.Unknown;
                    if (classes.Length >= 18)
                    {
                        classes[6] = SlotClass.Eye;
                        classes[7] = classes[8] = classes[11] = classes[15] = SlotClass.Skin;
                        classes[13] = SlotClass.Mouth;
                        classes[17] = SlotClass.Weapon;
                    }
                    look.Renderers.Add(new AetheriaCharacterLook.RendererSlots { Renderer = renderer, Classes = classes });
                }
            }
            if (look == null || look.Renderers.Count == 0) return;

            // First dress-up of this model: clone each slot's material individually and remember its untinted color
            if (_instancedLookMaterials.Count == 0)
            {
                foreach (var rs in look.Renderers)
                {
                    if (rs.Renderer == null) continue;
                    var src = rs.Renderer.sharedMaterials;
                    var mats = new Material[src.Length];
                    for (int i = 0; i < src.Length; i++)
                    {
                        SlotClass cls = i < rs.Classes.Length ? rs.Classes[i] : SlotClass.Unknown;
                        if (src[i] == null) { mats[i] = null; continue; }
                        var inst = new Material(src[i]) { name = src[i].name + " (Instance)" };
                        AetheriaSurfaceDetail.Apply(inst, cls);   // real PBR surface: the model ships without albedo maps
                        Color baseCol = src[i].HasProperty("_BaseColor")
                            ? src[i].GetColor("_BaseColor")
                            : (src[i].HasProperty("_Color") ? src[i].GetColor("_Color") : Color.white);
                        _instancedLookMaterials.Add((inst, cls, baseCol));
                        mats[i] = inst;
                    }
                    rs.Renderer.sharedMaterials = mats;
                }
            }

            var p = Customization.Profile;
            Color hairCol = HeroCustomizationService.HairColors[Mathf.Clamp(p.HairColorIndex, 0, HeroCustomizationService.HairColors.Length - 1)];
            Color skinCol = HeroCustomizationService.GetRaceSkinTint(p.Race);
            Color armorCol = HeroCustomizationService.ArmorTints[Mathf.Clamp(p.ArmorTintIndex, 0, HeroCustomizationService.ArmorTints.Length - 1)];

            for (int i = 0; i < _instancedLookMaterials.Count; i++)
            {
                var entry = _instancedLookMaterials[i];
                if (entry.mat == null) continue;
                Color tint; float amount;
                switch (entry.cls)
                {
                    case SlotClass.Skin:     tint = skinCol; amount = 0.82f; break;
                    case SlotClass.Hair:     tint = hairCol; amount = 1.00f; break;
                    case SlotClass.Metal:    tint = armorCol; amount = 0.55f; break;
                    case SlotClass.Leather:  tint = armorCol * 0.55f; amount = 0.35f; break;
                    case SlotClass.ClothAlt: tint = armorCol; amount = 0.20f; break;
                    case SlotClass.Cloth:    tint = armorCol; amount = 0.12f; break;
                    default: continue;
                }
                AetheriaRender.SetBaseColor(entry.mat, Color.Lerp(entry.baseColor, tint, amount));
            }
        }

        private void OnDestroy()
        {
            DestroyLookMaterials();
            if (Customization != null) Customization.OnProfileChanged -= RefreshDerivedStatsAndVisuals;
            if (Instance == this) Instance = null;
        }
    }
}
