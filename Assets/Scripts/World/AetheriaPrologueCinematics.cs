using System.Reflection;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;

namespace Aetheria
{
    /// <summary>
    /// Film language for the prologue, layered on top of <see cref="AetheriaPrologueDirector"/> without changing
    /// its story logic: the sage materialises close to the hero, the dialogue cuts between over-the-shoulder,
    /// close-up and wide shots with shallow depth of field and a rim light, the ledge collapse is a readable
    /// eye-level reaction shot with a beat of slow motion, and the fall is followed from above and behind so the
    /// drop and the plunge pool stay in frame. Runs after the director, so its camera wins.
    /// </summary>
    [DefaultExecutionOrder(1000)]
    public class AetheriaPrologueCinematics : MonoBehaviour
    {
        private const float SageDistance = 2.7f;
        // The director keeps its dialogue cursor private; read it rather than coupling the two classes.
        private static readonly FieldInfo LineField = typeof(AetheriaPrologueDirector).GetField("_line", BindingFlags.NonPublic | BindingFlags.Instance);

        private AetheriaPrologueDirector _d;
        private HeroController _hero;
        private AetheriaNPC _sage;
        private Camera _cam;
        private float _baseFov = 60f;
        private bool _fovStored;

        private AetheriaPrologueDirector.Stage _stage = AetheriaPrologueDirector.Stage.Idle;
        private float _t;              // game time in the current stage
        private float _realT;          // real time in the current stage (slow motion uses it)
        private int _shotLine = -1;
        private float _shotT;
        private Vector3 _camVel;
        private Vector3 _fallFwd = Vector3.forward;
        private bool _tumbling;
        private Quaternion _visualRest = Quaternion.identity;

        private Volume _volume;
        private DepthOfField _dof;
        private Vignette _vignette;
        private Light _rim, _fill;

        private void Awake() => _d = GetComponent<AetheriaPrologueDirector>();

        private void OnDisable()
        {
            RestoreTime();
            if (_hero != null) _hero.SetWeaponVisible(true);
        }

        private void LateUpdate()
        {
            if (_d == null) return;
            if (_hero == null) _hero = HeroController.Instance;
            if (_hero == null) return;
            if (_cam == null) _cam = _hero.MainCam != null ? _hero.MainCam : Camera.main;
            if (_cam == null) return;
            if (_sage == null) _sage = AetheriaNPC.AllNPCs.Find(n => n != null && n.NpcId == "Npc_CaveTutor");

            var stage = _d.Current;
            if (stage != _stage) OnStageChanged(_stage, stage);
            _t += Time.deltaTime;
            _realT += Time.unscaledDeltaTime;

            switch (stage)
            {
                case AetheriaPrologueDirector.Stage.SageAppear: SageEntranceShot(); break;
                case AetheriaPrologueDirector.Stage.Talk: DialogueShot(); break;
                case AetheriaPrologueDirector.Stage.Collapse: CollapseShot(); break;
            }
            UpdateLook(stage);
        }

        // ------------------------------------------------------------------ stage changes

        private void OnStageChanged(AetheriaPrologueDirector.Stage from, AetheriaPrologueDirector.Stage to)
        {
            _stage = to;
            _t = 0f;
            _realT = 0f;
            _shotLine = -1;
            bool cinematic = to == AetheriaPrologueDirector.Stage.SageAppear || to == AetheriaPrologueDirector.Stage.Talk ||
                             to == AetheriaPrologueDirector.Stage.Collapse;
            if (cinematic && !_fovStored) { _baseFov = _cam.fieldOfView; _fovStored = true; }
            if (!cinematic && _fovStored) { _cam.fieldOfView = _baseFov; _fovStored = false; }
            if (from == AetheriaPrologueDirector.Stage.Collapse) RestoreTime();
            if (to == AetheriaPrologueDirector.Stage.Wake) AetheriaHUDManager.Instance?.SetCinematicHudHidden(true);   // clean opening frame
            if (_tumbling && _hero != null && _hero.VisualRoot != null) { _hero.VisualRoot.localRotation = _visualRest; _tumbling = false; }

            // The hero listens to the sage with empty hands; the sword comes back once the talk is over.
            if (_hero != null)
                _hero.SetWeaponVisible(to != AetheriaPrologueDirector.Stage.SageAppear && to != AetheriaPrologueDirector.Stage.Talk);
            if (to == AetheriaPrologueDirector.Stage.SageAppear) BringSageClose();
            if (to == AetheriaPrologueDirector.Stage.Collapse)
            {
                var path = _d.FallPath;
                if (path != null && path.Length > 1)
                {
                    Vector3 f = Flat(path[path.Length - 1].position - path[0].position);
                    if (f.sqrMagnitude > 0.01f) _fallFwd = f.normalized;
                }
            }
        }

        /// <summary>The sage materialises a few steps in front of the hero, between him and the daylight.</summary>
        private void BringSageClose()
        {
            if (_sage == null) return;
            Vector3 hp = _hero.transform.position;
            Vector3 toLight = _d.CaveMouth != null ? Flat(_d.CaveMouth.position - hp) : Flat(_sage.transform.position - hp);
            if (toLight.sqrMagnitude < 0.01f) return;
            toLight.Normalize();
            Vector3 spot = hp + toLight * SageDistance;
            if (Physics.Raycast(spot + Vector3.up * 2f, Vector3.down, out var hit, 5f, ~0, QueryTriggerInteraction.Ignore)
                && !hit.collider.transform.IsChildOf(_hero.transform) && !hit.collider.transform.IsChildOf(_sage.transform))
                spot.y = hit.point.y;
            else spot.y = hp.y;
            if (_d.SagePoint != null) _d.SagePoint.position = spot;
            _sage.transform.position = spot;
            _sage.transform.rotation = Quaternion.LookRotation(-toLight, Vector3.up);
            _hero.transform.rotation = Quaternion.LookRotation(toLight, Vector3.up);
        }

        // ------------------------------------------------------------------ shots

        private void SageEntranceShot()
        {
            if (_sage == null) return;
            Vector3 hp = _hero.transform.position, sp = _sage.transform.position;
            Vector3 axis = Flat(sp - hp).normalized, side = Vector3.Cross(Vector3.up, axis);
            // Low, wide, from behind the hero: the sage rises out of the light of the cave mouth.
            float push = Mathf.SmoothStep(0f, 1f, Mathf.Clamp01(_t / 3.2f)) * 0.6f;
            Vector3 want = hp - axis * (2.3f - push) + side * 0.85f + Vector3.up * 1.15f;
            Place(want, sp + Vector3.up * (0.9f + 0.4f * Mathf.Clamp01(_t / 2f)), 42f, sp, true);
        }

        private void DialogueShot()
        {
            if (_sage == null) return;
            int line = LineField != null ? (int)LineField.GetValue(_d) : 0;
            if (line != _shotLine)
            {
                _shotLine = line;
                _shotT = 0f;
                if ((line == 2 || line == 5) && _sage != null)
                {
                    var anim = _sage.GetComponentInChildren<Animator>();
                    if (anim != null && anim.isActiveAndEnabled && HasParam(anim, "Cast")) anim.SetTrigger("Cast");
                }
            }
            _shotT += Time.deltaTime;
            float dolly = Mathf.Min(_shotT * 0.05f, 0.35f);

            Vector3 hp = _hero.transform.position, sp = _sage.transform.position;
            Vector3 axis = Flat(sp - hp).normalized, side = Vector3.Cross(Vector3.up, axis);
            Vector3 sageHead = sp + Vector3.up * 1.62f, heroHead = hp + Vector3.up * 1.6f;
            switch (line % 6)
            {
                case 0: // over the hero's right shoulder, medium shot of the sage
                    Place(hp - axis * (0.9f - dolly) + side * 0.62f + Vector3.up * 1.72f, sageHead - Vector3.up * 0.12f, 34f, sp, false);
                    break;
                case 1: // close-up of the sage, three-quarter from the hero's left
                    Place(sp - axis * (1.55f - dolly) - side * 0.75f + Vector3.up * 1.6f, sageHead, 26f, sp, false);
                    break;
                case 2: // wide profile two-shot: both figures against the light
                    Place((hp + sp) * 0.5f + side * (3.1f - dolly) + Vector3.up * 1.35f, (hp + sp) * 0.5f + Vector3.up * 1.2f, 40f, sp, false);
                    break;
                case 3: // low close-up of the sage, slightly heroic
                    Place(sp - axis * (1.4f - dolly) + side * 0.5f + Vector3.up * 1.15f, sageHead + Vector3.up * 0.05f, 30f, sp, false);
                    break;
                case 4: // reverse: over the sage's shoulder onto the listening hero
                    Place(sp + axis * (0.85f - dolly) - side * 0.6f + Vector3.up * 1.72f, heroHead - Vector3.up * 0.1f, 32f, hp, false);
                    break;
                default: // the way out: from behind the hero toward the daylight, the sage beside the path
                    Place(hp - axis * (2.0f - dolly) - side * 0.7f + Vector3.up * 1.9f, sp + axis * 3f + Vector3.up * 1.3f, 42f, sp, false);
                    break;
            }
        }

        private void CollapseShot()
        {
            Vector3 hp = _hero.transform.position;
            Vector3 fwd = _fallFwd, side = Vector3.Cross(Vector3.up, fwd);
            const float fallStart = 0.9f;

            // A beat of slow motion as the slab drops away under his feet.
            float slow = _realT < 0.35f ? 1f : _realT < 1.35f ? 0.3f : Mathf.Lerp(0.3f, 1f, Mathf.Clamp01((_realT - 1.35f) / 0.4f));
            Time.timeScale = slow;

            Vector3 shake = new Vector3(Mathf.PerlinNoise(_realT * 13f, 0.3f) - 0.5f, Mathf.PerlinNoise(0.7f, _realT * 11f) - 0.5f, 0f)
                            * (0.22f * Mathf.Clamp01(1.6f - _realT));
            if (_t < fallStart)
            {
                // Eye-level reaction shot in profile, the drop opening up beyond him.
                Vector3 want = hp + side * 3.1f - fwd * 0.9f + Vector3.up * 1.45f;
                Place(want + _cam.transform.TransformVector(shake), hp + Vector3.up * 1.05f + fwd * 1.6f, 38f, hp, true);
            }
            else
            {
                // Shift from a wide establishing view to a closer downstream tracking view.
                float waterY = _d.FallPath != null && _d.FallPath.Length > 0 ? _d.FallPath[_d.FallPath.Length - 1].position.y + 1.2f : hp.y - 20f;
                float progress = _d.WaterfallProgress;
                float close = Mathf.SmoothStep(0f, 1f, Mathf.InverseLerp(0.25f, 0.65f, progress));
                // Stay on the visible side of the water sheet as its slope changes.
                // A fixed forward offset can put the lens inside the transparent waterfall.
                Vector3 tangent = _d.WaterfallVelocity;
                Vector3 outward = tangent.sqrMagnitude > 0.01f ? Vector3.Cross(side, tangent.normalized) : fwd + Vector3.up;
                if (Vector3.Dot(outward, fwd + Vector3.up) < 0f) outward = -outward;
                if (outward.sqrMagnitude < 0.01f) outward = fwd + Vector3.up;
                outward.Normalize();
                Vector3 want = hp + outward * Mathf.Lerp(8f, 6f, close) + side * Mathf.Lerp(5f, 4f, close);
                want.y = Mathf.Max(want.y, waterY + 2.2f);
                Vector3 from = hp + Vector3.up * 0.8f;
                if (Physics.Linecast(from, want, out var hit, ~0, QueryTriggerInteraction.Ignore) && !hit.collider.transform.IsChildOf(_hero.transform))
                    want = hit.point + hit.normal * 0.4f;
                _cam.transform.position = Vector3.SmoothDamp(_cam.transform.position, want + _cam.transform.TransformVector(shake), ref _camVel, 0.25f);
                Vector3 look = hp + Vector3.up * 0.35f;
                look.y = Mathf.Max(look.y, waterY - 0.2f);
                _cam.transform.rotation = Quaternion.Slerp(_cam.transform.rotation, Quaternion.LookRotation(look - _cam.transform.position, Vector3.up), Time.unscaledDeltaTime * 6f);
                _cam.fieldOfView = Mathf.Lerp(_cam.fieldOfView, 48f, Time.unscaledDeltaTime * 3f);
                // Before entering the current he tips over the ledge. Once in the water,
                // the dedicated swim clip moves the body instead of rotating it as a rigid object.
                if (_hero.VisualRoot != null)
                {
                    if (!_tumbling) { _visualRest = _hero.VisualRoot.localRotation; _tumbling = true; }
                    float tilt = 1f - Mathf.SmoothStep(0f, 1f, Mathf.InverseLerp(0.12f, 0.24f, progress));
                    _hero.VisualRoot.localRotation = _visualRest * Quaternion.Euler(20f * tilt, 0f, Mathf.Sin(_t * 1.7f) * 9f * tilt);
                }
                SetFocus(Vector3.Distance(_cam.transform.position, hp), false);
            }
        }

        /// <summary>Cuts (or eases) the camera to a framing, keeping it out of the rock and focusing on the subject.</summary>
        private void Place(Vector3 want, Vector3 look, float fov, Vector3 subject, bool ease)
        {
            if (Physics.Linecast(look, want, out var hit, ~0, QueryTriggerInteraction.Ignore)
                && !hit.collider.transform.IsChildOf(_hero.transform) && (_sage == null || !hit.collider.transform.IsChildOf(_sage.transform)))
                want = hit.point + hit.normal * 0.25f;
            bool cut = _shotT < Time.deltaTime * 1.5f && !ease;
            if (cut || (_cam.transform.position - want).sqrMagnitude > 25f)
            {
                _cam.transform.position = want;
                _cam.transform.rotation = Quaternion.LookRotation(look - want, Vector3.up);
                _cam.fieldOfView = fov;
                _camVel = Vector3.zero;
            }
            else
            {
                _cam.transform.position = Vector3.SmoothDamp(_cam.transform.position, want, ref _camVel, ease ? 0.3f : 0.12f);
                _cam.transform.rotation = Quaternion.Slerp(_cam.transform.rotation, Quaternion.LookRotation(look - _cam.transform.position, Vector3.up), Time.unscaledDeltaTime * 6f);
                _cam.fieldOfView = Mathf.Lerp(_cam.fieldOfView, fov, Time.unscaledDeltaTime * 4f);
            }
            SetFocus(Vector3.Distance(_cam.transform.position, subject + Vector3.up * 1.4f), !ease);
        }

        // ------------------------------------------------------------------ look: depth of field, vignette, rim light

        private void EnsureLook()
        {
            if (_volume != null) return;
            var go = new GameObject("Prologue_CinematicLook");
            go.transform.SetParent(transform, false);
            _volume = go.AddComponent<Volume>();
            _volume.isGlobal = true;
            _volume.priority = 60f;
            _volume.weight = 0f;
            var profile = ScriptableObject.CreateInstance<VolumeProfile>();
            _volume.sharedProfile = profile;
            _dof = profile.Add<DepthOfField>(true);
            _dof.focusMode.Override(DepthOfFieldMode.Manual);
            _dof.nearFocusStart.Override(0f);
            _dof.nearFocusEnd.Override(0.5f);
            _dof.farFocusStart.Override(4f);
            _dof.farFocusEnd.Override(9f);
            _vignette = profile.Add<Vignette>(true);
            _vignette.intensity.Override(0.38f);
            _vignette.smoothness.Override(0.45f);

            _rim = MakeLight("Prologue_SageRim", LightType.Spot, new Color(1f, 0.86f, 0.62f), 2200f, 9f, 55f);
            _fill = MakeLight("Prologue_SageFill", LightType.Point, new Color(0.62f, 0.72f, 1f), 180f, 5f, 0f);
        }

        private Light MakeLight(string name, LightType type, Color color, float lumen, float range, float angle)
        {
            var go = new GameObject(name);
            go.transform.SetParent(transform, false);
            var l = go.AddComponent<Light>();
            if (go.GetComponent<HDAdditionalLightData>() == null) go.AddComponent<HDAdditionalLightData>();
            l.type = type;
            l.color = color;
            l.lightUnit = LightUnit.Lumen;
            l.intensity = lumen;
            l.range = range;
            if (type == LightType.Spot) { l.spotAngle = angle; l.innerSpotAngle = angle * 0.5f; }
            l.shadows = LightShadows.None;
            l.enabled = false;
            return l;
        }

        /// <summary>Shallow focus for dialogue portraits; deep focus for action, so the drop stays readable.</summary>
        private void SetFocus(float distance, bool shallow)
        {
            if (_dof == null) return;
            _dof.nearFocusEnd.Override(shallow ? Mathf.Clamp(distance * 0.3f, 0.2f, 1.2f) : 0.1f);
            _dof.farFocusStart.Override(distance + (shallow ? 0.9f : 6f));
            _dof.farFocusEnd.Override(distance + (shallow ? 6f : 60f));
        }

        private void UpdateLook(AetheriaPrologueDirector.Stage stage)
        {
            bool dialogue = stage == AetheriaPrologueDirector.Stage.SageAppear || stage == AetheriaPrologueDirector.Stage.Talk;
            bool cinematic = dialogue || stage == AetheriaPrologueDirector.Stage.Collapse;
            if (cinematic) EnsureLook();
            if (_volume == null) return;
            _volume.weight = Mathf.MoveTowards(_volume.weight, cinematic ? 1f : 0f, Time.unscaledDeltaTime * 2f);
            bool lit = dialogue && _sage != null;
            _rim.enabled = lit;
            _fill.enabled = lit;
            if (!lit) return;
            // Rim from behind the sage (the daylight side), soft cool fill from the camera side.
            Vector3 sp = _sage.transform.position, hp = _hero.transform.position;
            Vector3 axis = Flat(sp - hp).normalized;
            _rim.transform.position = sp + axis * 1.8f + Vector3.up * 2.6f;
            _rim.transform.rotation = Quaternion.LookRotation(sp + Vector3.up * 1.3f - _rim.transform.position, Vector3.up);
            _fill.transform.position = _cam.transform.position + _cam.transform.right * 0.6f + Vector3.up * 0.2f;
        }

        private void RestoreTime()
        {
            if (Time.timeScale != 1f && Time.timeScale > 0f) Time.timeScale = 1f;
        }

        private static bool HasParam(Animator a, string name)
        {
            foreach (var p in a.parameters) if (p.name == name) return true;
            return false;
        }

        private static Vector3 Flat(Vector3 v) { v.y = 0f; return v; }
    }
}
