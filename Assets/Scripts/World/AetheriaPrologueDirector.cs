using System.Collections.Generic;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Runs the opening of the game as one directed sequence:
    ///
    ///   0 Wake      the hero comes to in a dark cave; daylight glows at the far end
    ///   1 Light     walking toward it, the sage Weylan materialises in a swirl of light
    ///   2 Talk      he jokes and tells of Aetheria (letterboxed two-shot, any key advances)
    ///   3 Vista     he sends the hero out onto the cliff above the valley
    ///   4 Collapse  the ledge gives way; the hero is carried down the waterfall and blacks out
    ///   5 Shore     he wakes on the bank of a forest glade with flowers and butterflies
    ///
    /// Scene content (cave, ledge, waterfall, glade) is authored by AetheriaPrologueBuilder; this component
    /// only holds references to it, so the sequence can be re-staged without touching code.
    /// </summary>
    public class AetheriaPrologueDirector : MonoBehaviour
    {
        public static AetheriaPrologueDirector Instance { get; private set; }

        public enum Stage { Idle, Wake, Light, SageAppear, Talk, Vista, Collapse, Shore, Done }

        [Header("Authored points")]
        public Transform WakePoint;
        public Transform LightTrigger;       // reaching this starts the sage's entrance
        public Transform SagePoint;
        public Transform CaveMouth;
        public Transform LedgeTrigger;       // stepping past this collapses the ledge
        public Transform LedgeRoot;          // the slab pieces that fall
        public Transform[] FallPath;         // ledge -> waterfall lip -> down the fall -> plunge pool
        public Transform GladeWakePoint;
        public Transform GladeCamera;
        public Bounds CaveBounds;            // world-space box of the cave interior

        public Stage Current { get; private set; } = Stage.Idle;
        public bool DialogueOpen => Current == Stage.Talk;
        public float WaterfallProgress => Current == Stage.Collapse ? FallU() : 0f;
        public Vector3 WaterfallVelocity => Current == Stage.Collapse ? FallVelocity() : Vector3.zero;

        private HeroController _hero;
        private AetheriaNPC _sage;
        private Vector3 _sageBaseScale = Vector3.one;
        private float _t;
        private int _line;
        private float _lineTime;
        private float _fade;                 // 0 clear .. 1 black
        private string _title;
        private float _titleAlpha;
        private Camera _cam;
        private Vector3 _camVel;
        private readonly List<Rigidbody> _falling = new List<Rigidbody>();
        private bool _spawnedSplash;
        private Vector3 _fallEntryPosition;
        private GUIStyle _name, _body, _hint, _big;
        private Texture2D _black;

        private static readonly string[] SageLines =
        {
            "О! Живой. А я уже третий раз за неделю репетировал над тобой погребальную песнь. Жаль, такой был припев.",
            "Не пугайся. Я Вейлен, Хранитель этой пещеры. Ну, как хранитель… в основном храню паутину и свои колени от сквозняков.",
            "Ты в Этерии, Странник. Тысячу лет этот мир держался на Печати Дракона, что связывала пламя, бурю и лёд.",
            "Но Печать раскололась. Малакор, Повелитель Расколотой Печати, поднял мёртвых в Высокой Твердыне, а драконы ушли за облака.",
            "Говорят, лишь тот, кто сплетёт три сферы — Exort, Wex и Quas — и договорится с драконом Игнисом, запечатает тьму снова. Звучит как работа для кого-то с очень свободным графиком. То есть для тебя.",
            "Ступай к свету и взгляни на свою Этерию. Только под ноги смотри: эти скалы старше моих шуток. А мои шутки очень старые.",
        };

        private void Awake() => Instance = this;
        private void OnDestroy() { if (Instance == this) { Instance = null; AetheriaHUDManager.Instance?.SetCinematicHudHidden(false); } if (_black != null) Destroy(_black); }

        private void Start()
        {
            // The sage waits unseen until his entrance.
            _sage = AetheriaNPC.AllNPCs.Find(n => n.NpcId == "Npc_CaveTutor");
            if (_sage != null)
            {
                _sageBaseScale = _sage.transform.localScale;
                if (SagePoint != null)
                {
                    _sage.transform.position = SagePoint.position;
                    _sage.transform.rotation = SagePoint.rotation;
                }
                _sage.gameObject.SetActive(false);
            }
        }

        // ------------------------------------------------------------------ entry points

        public void Begin(HeroController hero)
        {
            _hero = hero;
            _cam = hero.MainCam != null ? hero.MainCam : Camera.main;
            hero.HasEnteredWorld = true;
            hero.PrologueStage = 0;
            hero.CinematicLock = true;
            hero.transform.rotation = WakePoint.rotation;
            hero.TeleportSafe(WakePoint.position);
            var quests = QuestAndMapService.Instance;
            if (quests != null)
            {
                quests.TrackedQuestId = "Q_PrologueAwakening";
                var q = quests.FindQuest("Q_PrologueAwakening");
                if (q != null && LightTrigger != null) q.ObjectiveWorldLocation = LightTrigger.position;
            }
            if (_sage != null) _sage.gameObject.SetActive(false);
            _fade = 1f;
            Enter(Stage.Wake);
        }

        public bool InCave(Vector3 p) => CaveBounds.Contains(p);

        /// <summary>Stops the sequence (skipping to the open world, loading a save, respawning).</summary>
        public void Abort()
        {
            if (Current == Stage.Idle || Current == Stage.Done) return;
            Current = Stage.Done;
            _fade = 0f;
            _titleAlpha = 0f;
            SetWaterfallSwim(false);
            AetheriaHUDManager.Instance?.SetCinematicHudHidden(false);
            foreach (var rb in _falling) if (rb != null) Destroy(rb.gameObject);
            _falling.Clear();
            if (_sage != null) { _sage.gameObject.SetActive(true); _sage.transform.localScale = _sageBaseScale; }
            if (_hero != null) { _hero.CinematicLock = false; _hero.CC.enabled = true; }
        }

        /// <summary>Verification hooks: jump the story forward the way a player would get there.</summary>
        public void DebugReachLight() { if (Current == Stage.Wake || Current == Stage.Light) Enter(Stage.SageAppear); }
        public void DebugFinishDialogue() { if (Current == Stage.SageAppear || Current == Stage.Talk) { _line = SageLines.Length; Enter(Stage.Vista); } }

        private void Enter(Stage s)
        {
            Current = s;
            _t = 0f;
            AetheriaHUDManager.Instance?.SetCinematicHudHidden(s == Stage.SageAppear || s == Stage.Talk || s == Stage.Collapse || s == Stage.Shore);
            Debug.Log($"[Prologue] Stage {s} at {(_hero != null ? _hero.transform.position : Vector3.zero)}");
            switch (s)
            {
                case Stage.Wake:
                    ShowTitle("ПЕЩЕРА ПРОБУЖДЕНИЯ");
                    break;
                case Stage.Light:
                    _hero.CinematicLock = false;
                    QuestAndMapService.Instance?.PushNotification("Идите на свет");
                    break;
                case Stage.SageAppear:
                    _hero.CinematicLock = true;
                    FaceEachOther();
                    if (_sage != null)
                    {
                        _sage.gameObject.SetActive(true);
                        _sage.transform.localScale = _sageBaseScale;
                        if (SagePoint != null) _sage.transform.position = SagePoint.position;
                    }
                    break;
                case Stage.Talk:
                    _line = 0; _lineTime = 0f;
                    break;
                case Stage.Vista:
                    _hero.CinematicLock = false;
                    if (_sage != null) _sage.transform.localScale = _sageBaseScale;
                    var quests = QuestAndMapService.Instance;
                    if (quests != null)
                    {
                        var q = quests.FindQuest("Q_PrologueAwakening");
                        if (q != null && LedgeTrigger != null) q.ObjectiveWorldLocation = LedgeTrigger.position;
                        quests.AcceptQuest("Q_PrologueAwakening");
                        quests.PushNotification("[ЛКМ] удар · [ПКМ] блок · [Space] прыжок · [M] карта · [C] герой");
                    }
                    break;
                case Stage.Collapse:
                    _hero.CinematicLock = true;
                    _hero.CC.enabled = false;
                    _spawnedSplash = false;
                    _fallEntryPosition = _hero.transform.position;
                    BreakLedge();
                    AetheriaFx.Shake(0.45f, 1.1f);
                    AudioAndQualityService.Instance?.PlaySfxAt("SFX_Hit_Stone", _hero.transform.position, 1f);
                    TriggerHero("HitReact");
                    break;
                case Stage.Shore:
                    SetWaterfallSwim(false);
                    _hero.transform.SetPositionAndRotation(GladeWakePoint.position, GladeWakePoint.rotation);
                    _hero.CC.enabled = false;
                    TriggerHero("Die");                 // lying on the bank while the screen is dark
                    foreach (var rb in _falling) if (rb != null) Destroy(rb.gameObject);
                    _falling.Clear();
                    break;
                case Stage.Done:
                    Finish();
                    break;
            }
        }

        private void Finish()
        {
            _fade = 0f;
            _hero.CC.enabled = true;
            _hero.TeleportSafe(GladeWakePoint.position);
            _hero.CinematicLock = false;
            _hero.PrologueStage = 4;
            var quests = QuestAndMapService.Instance;
            if (quests != null)
            {
                var q = quests.FindQuest("Q_PrologueAwakening");
                if (q != null && q.State == QuestState.Available) quests.AcceptQuest(q.QuestId);
                if (q != null && q.State != QuestState.Completed)
                {
                    quests.AdvanceQuestProgress(q.QuestId, q.TargetCount);
                    quests.TurnInQuest(q.QuestId, _hero);
                }
                quests.PushNotification("ЛЕСНАЯ ЧАЩА · Оукхейвен лежит к северу — найдите старейшину Олдрика");
            }
        }

        // ------------------------------------------------------------------ per frame

        private void Update()
        {
            if (Current == Stage.Idle || Current == Stage.Done || _hero == null) return;
            float dt = Time.deltaTime;
            _t += dt;
            _titleAlpha = Mathf.MoveTowards(_titleAlpha, 0f, dt * 0.25f);

            switch (Current)
            {
                case Stage.Wake:
                    _fade = 1f - Mathf.Clamp01((_t - 0.6f) / 2.8f);
                    if (_t > 3.4f) Enter(Stage.Light);
                    break;

                case Stage.Light:
                    if (LightTrigger != null && Flat(_hero.transform.position - LightTrigger.position).sqrMagnitude < 2.6f * 2.6f)
                        Enter(Stage.SageAppear);
                    break;

                case Stage.SageAppear:
                    UpdateSageEntrance();
                    if (_t > 3.2f) Enter(Stage.Talk);
                    break;

                case Stage.Talk:
                    _lineTime += dt;
                    if (_lineTime > 0.25f && (Input.GetKeyDown(KeyCode.Space) || Input.GetKeyDown(KeyCode.E) ||
                                              Input.GetKeyDown(KeyCode.Return) || Input.GetMouseButtonDown(0)))
                    {
                        if (_lineTime < Typed(SageLines[_line]).Length * 0.03f) _lineTime = 999f;   // first press: show the whole line
                        else { _line++; _lineTime = 0f; }
                        if (_line >= SageLines.Length) Enter(Stage.Vista);
                    }
                    break;

                case Stage.Vista:
                    if (_t > 0.5f && LedgeTrigger != null && Flat(_hero.transform.position - LedgeTrigger.position).sqrMagnitude < 2.2f * 2.2f)
                        Enter(Stage.Collapse);
                    break;

                case Stage.Collapse:
                    UpdateFall();
                    break;

                case Stage.Shore:
                    // Black, then the glade fades in; he lies on the bank, then gets up.
                    _fade = 1f - Mathf.Clamp01((_t - 2.2f) / 2.4f);
                    if (_t > 1.0f && _t < 1.1f) ShowTitle("…");
                    if (_t > 5.4f && _t - Time.deltaTime <= 5.4f) TriggerHero("Respawn");
                    if (_t > 7.2f) Enter(Stage.Done);
                    break;
            }
        }

        private void LateUpdate()
        {
            if (_cam == null || _hero == null) return;
            Vector3 hp = _hero.transform.position;
            switch (Current)
            {
                case Stage.Wake:
                {
                    // Low, close, looking past him toward the distant light.
                    Vector3 toLight = LightTrigger != null ? Flat(LightTrigger.position - hp).normalized : _hero.transform.forward;
                    Vector3 want = hp - toLight * 2.4f + Vector3.up * 0.9f + Vector3.Cross(Vector3.up, toLight) * 0.6f;
                    _cam.transform.position = want;
                    _cam.transform.rotation = Quaternion.LookRotation((hp + toLight * 6f + Vector3.up * 1.1f) - want, Vector3.up);
                    break;
                }
                case Stage.SageAppear:
                case Stage.Talk:
                {
                    if (_sage == null) break;
                    Vector3 sp = _sage.transform.position;
                    Vector3 axis = Flat(sp - hp).normalized;
                    Vector3 side = Vector3.Cross(Vector3.up, axis);
                    // Over-the-shoulder two-shot, drifting very slowly.
                    float drift = Mathf.Sin(Time.time * 0.15f) * 0.25f;
                    Vector3 want = hp - axis * 1.6f + side * (0.55f + drift) + Vector3.up * 1.75f;
                    // Keep the camera out of the cave walls.
                    Vector3 head = hp + Vector3.up * 1.6f;
                    if (Physics.Linecast(head, want, out RaycastHit hit, ~0, QueryTriggerInteraction.Ignore) && !hit.collider.transform.IsChildOf(_hero.transform))
                        want = hit.point + (head - want).normalized * 0.3f;
                    _cam.transform.position = Vector3.SmoothDamp(_cam.transform.position, want, ref _camVel, 0.35f);
                    _cam.transform.rotation = Quaternion.Slerp(_cam.transform.rotation,
                        Quaternion.LookRotation((sp + Vector3.up * 1.45f) - _cam.transform.position, Vector3.up), Time.deltaTime * 4f);
                    break;
                }
                case Stage.Collapse:
                {
                    Vector3 vel = FallVelocity();
                    Vector3 back = vel.sqrMagnitude > 0.01f ? vel.normalized : _hero.transform.forward;
                    Vector3 side = Vector3.Cross(Vector3.up, Flat(back).sqrMagnitude > 0.01f ? Flat(back).normalized : Vector3.forward);
                    Vector3 want = hp - back * 5.5f + side * 2.5f + Vector3.up * 2.2f;
                    _cam.transform.position = Vector3.SmoothDamp(_cam.transform.position, want, ref _camVel, 0.25f);
                    _cam.transform.rotation = Quaternion.LookRotation(hp + Vector3.up * 0.8f - _cam.transform.position, Vector3.up);
                    break;
                }
                case Stage.Shore:
                {
                    if (GladeCamera == null) break;
                    float orbit = _t * 2.5f;
                    var rot = Quaternion.Euler(0f, orbit, 0f);
                    Vector3 pivot = GladeWakePoint.position;
                    Vector3 offset = rot * (GladeCamera.position - pivot);
                    _cam.transform.position = pivot + offset;
                    _cam.transform.rotation = Quaternion.LookRotation(pivot + Vector3.up * 0.6f - _cam.transform.position, Vector3.up);
                    break;
                }
            }
        }

        // ------------------------------------------------------------------ sage

        private void FaceEachOther()
        {
            if (_sage == null) return;
            Vector3 toSage = Flat(_sage.transform.position - _hero.transform.position);
            if (toSage.sqrMagnitude > 0.01f) _hero.transform.rotation = Quaternion.LookRotation(toSage.normalized, Vector3.up);
            _sage.transform.rotation = Quaternion.LookRotation(-toSage.normalized, Vector3.up);
        }

        private void UpdateSageEntrance()
        {
            if (_sage == null) return;
            Vector3 p = SagePoint != null ? SagePoint.position : _sage.transform.position;
            // The sage appears at his authored scale: the light conveys the entrance
            // without visibly inflating or stretching the character model.
            Color gold = new Color(1f, 0.85f, 0.45f), teal = new Color(0.45f, 0.95f, 1f);
            if (Mathf.Repeat(_t, 0.18f) < Time.deltaTime && _t < 1.8f)
            {
                float a = _t * 9f;
                Vector3 orbit = new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a)) * 0.9f;
                AetheriaFx.Burst(p + orbit + Vector3.up * (0.3f + _t), Color.Lerp(gold, teal, Mathf.PingPong(_t, 1f)), 14,
                    0.6f, 2.2f, 0.05f, 0.14f, 0.9f, -0.4f, 1.2f);
            }
            if (_t < Time.deltaTime * 1.5f)
            {
                AetheriaFx.Shockwave(p + Vector3.up * 0.05f, gold, 3.2f, 0.9f);
                AetheriaFx.FlashLight(p + Vector3.up * 1.4f, gold, 26f, 9f, 1.4f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_PortalOpen", p, 0.9f);
            }
            if (_t > 1.5f && _t - Time.deltaTime <= 1.5f)
            {
                AetheriaFx.Puff(p + Vector3.up * 0.2f, new Color(1f, 0.92f, 0.7f, 0.5f), 18, 1.6f, 1.2f, 1.4f, 0.8f);
                var anim = _sage.GetComponentInChildren<Animator>();
                if (anim != null && HasParam(anim, "Cast")) anim.SetTrigger("Cast");
            }
        }

        // ------------------------------------------------------------------ collapse and fall

        private void BreakLedge()
        {
            _falling.Clear();
            if (LedgeRoot == null) return;
            var rng = new System.Random(7);
            foreach (Transform piece in LedgeRoot)
            {
                var rb = piece.gameObject.GetComponent<Rigidbody>() ?? piece.gameObject.AddComponent<Rigidbody>();
                rb.mass = 200f;
                rb.isKinematic = false;
                rb.AddForce(new Vector3((float)rng.NextDouble() - 0.5f, -0.2f, (float)rng.NextDouble() - 0.5f) * 600f, ForceMode.Impulse);
                rb.AddTorque(new Vector3((float)rng.NextDouble(), (float)rng.NextDouble(), (float)rng.NextDouble()) * 300f, ForceMode.Impulse);
                _falling.Add(rb);
                AetheriaFx.Puff(piece.position, new Color(0.62f, 0.56f, 0.5f, 0.6f), 10, 2.2f, 1.6f, 1.6f, 0.4f);
            }
        }

        private const float FallStart = 0.9f, FallDuration = 9f;

        private Vector3 PathPoint(float u)
        {
            if (FallPath == null || FallPath.Length == 0) return _hero.transform.position;
            int n = FallPath.Length;
            if (n == 1) return FallPath[0].position;
            float f = Mathf.Clamp01(u) * (n - 1);
            int i = Mathf.Min(n - 2, Mathf.FloorToInt(f));
            float t = f - i;
            Vector3 p0 = FallPath[Mathf.Max(0, i - 1)].position, p1 = FallPath[i].position,
                    p2 = FallPath[i + 1].position, p3 = FallPath[Mathf.Min(n - 1, i + 2)].position;
            return 0.5f * (2f * p1 + (-p0 + p2) * t + (2f * p0 - 5f * p1 + 4f * p2 - p3) * t * t + (-p0 + 3f * p1 - 3f * p2 + p3) * t * t * t);
        }

        private float FallU() => Mathf.Clamp01((_t - FallStart) / FallDuration);
        private Vector3 FallPosition(float u)
        {
            u = Mathf.Clamp01(u);
            float eased = u * u * (3f - 2f * u) * 0.35f + u * u * 0.65f;
            float entryBlend = Mathf.SmoothStep(0f, 1f, Mathf.Clamp01(u / 0.12f));
            return Vector3.Lerp(_fallEntryPosition + Vector3.down * 0.45f, PathPoint(eased), entryBlend);
        }

        private Vector3 FallVelocity()
        {
            float u = FallU();
            float a = Mathf.Max(0f, u - 0.01f), b = Mathf.Min(1f, u + 0.01f);
            return (FallPosition(b) - FallPosition(a)) / Mathf.Max(0.001f, (b - a) * FallDuration);
        }

        private void UpdateFall()
        {
            float u = FallU();
            // Accelerating fall: slow tumble off the ledge, fast down the water.
            // The slab starts dropping during the reaction beat. Follow it down instead
            // of leaving the character standing motionless above the broken ledge.
            Vector3 p = _t < FallStart
                ? _fallEntryPosition + Vector3.down * Mathf.SmoothStep(0f, 0.45f, _t / FallStart)
                : FallPosition(u);
            Vector3 v = FallVelocity();
            _hero.transform.position = p;
            SetWaterfallSwim(u >= 0.18f && u < 0.98f);
            if (Flat(v).sqrMagnitude > 0.01f) _hero.transform.rotation = Quaternion.Slerp(_hero.transform.rotation, Quaternion.LookRotation(Flat(v).normalized), Time.deltaTime * 4f);

            if (u > 0.18f && Mathf.Repeat(_t, 0.12f) < Time.deltaTime)
                AetheriaFx.Puff(p, new Color(0.85f, 0.93f, 1f, 0.5f), 6, 2.5f, 1.4f, 1.0f, 0.5f);   // spray on the fall
            if (u >= 0.98f && !_spawnedSplash)
            {
                _spawnedSplash = true;
                AetheriaFx.Burst(p, new Color(0.8f, 0.92f, 1f), 60, 3f, 9f, 0.08f, 0.22f, 1.2f, 1.6f, 0.6f);
                AetheriaFx.Puff(p, new Color(0.9f, 0.95f, 1f, 0.7f), 30, 4f, 2.4f, 2.0f, 1.2f);
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_Wall_Slam", p, 0.8f);
                TriggerHero("Die");
            }
            float end = FallStart + FallDuration;
            _fade = Mathf.Clamp01((_t - (end - 0.6f)) / 1.6f);   // the water closes over him
            if (_t > end + 1.4f) Enter(Stage.Shore);
        }

        // ------------------------------------------------------------------ helpers

        private void SetWaterfallSwim(bool active)
        {
            var anim = _hero != null ? _hero.ActiveAnimator : null;
            if (anim != null && anim.isActiveAndEnabled && HasParam(anim, "WaterfallSwim"))
                anim.SetBool("WaterfallSwim", active);
        }

        private void TriggerHero(string trigger)
        {
            var anim = _hero.ActiveAnimator;
            if (anim != null && anim.isActiveAndEnabled && HasParam(anim, trigger)) anim.SetTrigger(trigger);
        }

        private static bool HasParam(Animator a, string name)
        {
            foreach (var p in a.parameters) if (p.name == name) return true;
            return false;
        }

        private static Vector3 Flat(Vector3 v) { v.y = 0f; return v; }

        private void ShowTitle(string text) { _title = text; _titleAlpha = 1.4f; }

        private string Typed(string s) => s;

        private void OnGUI()
        {
            if (Current == Stage.Idle || Current == Stage.Done) return;
            if (_black == null) { _black = new Texture2D(1, 1); _black.SetPixel(0, 0, Color.black); _black.Apply(); }
            if (_body == null)
            {
                _name = new GUIStyle(GUI.skin.label) { fontSize = Mathf.RoundToInt(Screen.height * 0.026f), fontStyle = FontStyle.Bold };
                _name.normal.textColor = new Color(1f, 0.82f, 0.45f);
                _body = new GUIStyle(GUI.skin.label) { fontSize = Mathf.RoundToInt(Screen.height * 0.024f), wordWrap = true };
                _body.normal.textColor = new Color(0.96f, 0.93f, 0.86f);
                _hint = new GUIStyle(GUI.skin.label) { fontSize = Mathf.RoundToInt(Screen.height * 0.016f), alignment = TextAnchor.LowerRight };
                _hint.normal.textColor = new Color(1f, 1f, 1f, 0.55f);
                _big = new GUIStyle(GUI.skin.label) { fontSize = Mathf.RoundToInt(Screen.height * 0.04f), alignment = TextAnchor.MiddleCenter, fontStyle = FontStyle.Italic };
            }
            bool cinematic = Current == Stage.SageAppear || Current == Stage.Talk || Current == Stage.Collapse || Current == Stage.Shore;
            if (cinematic)
            {
                float bar = Screen.height * 0.1f;
                GUI.DrawTexture(new Rect(0, 0, Screen.width, bar), _black);
                GUI.DrawTexture(new Rect(0, Screen.height - bar, Screen.width, bar), _black);
            }
            if (Current == Stage.Talk && _line < SageLines.Length)
            {
                string line = SageLines[_line];
                int shown = Mathf.Clamp(Mathf.FloorToInt(_lineTime / 0.03f), 0, line.Length);
                float w = Screen.width * 0.62f, h = Screen.height * 0.2f;
                var r = new Rect((Screen.width - w) * 0.5f, Screen.height * 0.66f, w, h);
                var prev = GUI.color;
                GUI.color = new Color(0f, 0f, 0f, 0.55f);
                GUI.DrawTexture(r, _black);
                GUI.color = prev;
                GUI.Label(new Rect(r.x + 24, r.y + 14, r.width - 48, 40), "Хранитель Вейлен", _name);
                GUI.Label(new Rect(r.x + 24, r.y + 54, r.width - 48, r.height - 70), line.Substring(0, shown), _body);
                GUI.Label(new Rect(r.x, r.y, r.width - 18, r.height - 8), $"{_line + 1}/{SageLines.Length}   [Пробел] далее", _hint);
            }
            if (_titleAlpha > 0.01f && !string.IsNullOrEmpty(_title))
            {
                var c = _big.normal.textColor; c = new Color(0.95f, 0.9f, 0.78f, Mathf.Clamp01(_titleAlpha)); _big.normal.textColor = c;
                GUI.Label(new Rect(0, Screen.height * 0.36f, Screen.width, Screen.height * 0.12f), _title, _big);
            }
            if (_fade > 0.001f)
            {
                var prev = GUI.color;
                GUI.color = new Color(0f, 0f, 0f, _fade);
                GUI.DrawTexture(new Rect(0, 0, Screen.width, Screen.height), _black);
                GUI.color = prev;
                if (_titleAlpha > 0.01f && !string.IsNullOrEmpty(_title))
                {
                    var c = new Color(0.95f, 0.9f, 0.78f, Mathf.Clamp01(_titleAlpha));
                    _big.normal.textColor = c;
                    GUI.Label(new Rect(0, Screen.height * 0.36f, Screen.width, Screen.height * 0.12f), _title, _big);
                }
            }
        }
    }
}
