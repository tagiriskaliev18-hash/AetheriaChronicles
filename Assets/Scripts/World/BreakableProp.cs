using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Destructible quest prop (runic beacons, bind-crystals, storm conductors, the obsidian seal).
    /// Cracks show damage; at zero it shatters into stylized debris and magically regrows later.
    /// </summary>
    public class BreakableProp : MonoBehaviour
    {
        public DestructibleKind Kind = DestructibleKind.Stone;
        public float HP = 100f;
        public float MaxHP = 100f;
        public string LinkedQuestId = "";
        public bool RequiresSunStrike = false;
        public bool RequiresMeteor = false;
        public float RestoreDelaySeconds = 60f;
        public Transform Focus;                 // optional glowing core (shown when the prop is whole)

        public bool Broken { get; private set; }
        private bool _questNotified = false;
        private float _restoreTimer = 0f;
        private Renderer[] _renderers;
        private Collider[] _colliders;
        private readonly List<Material> _materials = new List<Material>();
        private readonly List<Color> _baseColors = new List<Color>();
        private readonly List<Color> _emissiveColors = new List<Color>();
        private float _flash;
        private Vector3 _initialScale = Vector3.one;
        private Coroutine _regrowRoutine;

        private void Awake()
        {
            _initialScale = transform.localScale;
            _renderers = GetComponentsInChildren<Renderer>(true);
            _colliders = GetComponentsInChildren<Collider>(true);
            AetheriaWorldGenerator.RegisterBreakable(this);
            FxAssets.Ensure();

            foreach (var r in _renderers)
            {
                if (r == null) continue;
                var src = r.sharedMaterials;
                var mats = new Material[src.Length];
                for (int i = 0; i < src.Length; i++)
                {
                    mats[i] = src[i] != null ? new Material(src[i]) : null;
                    if (mats[i] == null) continue;
                    Color baseCol = mats[i].HasProperty("_BaseColor") ? mats[i].GetColor("_BaseColor") : mats[i].color;
                    Color emisCol = mats[i].HasProperty("_EmissionColor") ? mats[i].GetColor("_EmissionColor") : Color.black;
                    _baseColors.Add(baseCol);
                    _emissiveColors.Add(emisCol);
                    _materials.Add(mats[i]);
                }
                r.sharedMaterials = mats;
            }
            SetFocusActive(true);
        }

        private void OnDestroy()
        {
            AetheriaWorldGenerator.UnregisterBreakable(this);
            foreach (var m in _materials) if (m != null) Destroy(m);
        }

        private void SetFocusActive(bool active)
        {
            if (Focus == null) return;
            Focus.gameObject.SetActive(active);
            foreach (var r in Focus.GetComponentsInChildren<Renderer>(true))
                if (r != null) r.enabled = active;
            foreach (var l in Focus.GetComponentsInChildren<Light>(true))
                if (l != null) l.enabled = active;
            foreach (var ps in Focus.GetComponentsInChildren<ParticleSystem>(true))
            {
                if (ps == null) continue;
                if (active) ps.Play(true);
                else ps.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
            }
        }

        private void Update()
        {
            if (Broken)
            {
                if (_restoreTimer > 0f)
                {
                    _restoreTimer -= Time.deltaTime;
                    if (_restoreTimer <= 0f) RestoreNow();
                }
                return;
            }

            if (_flash > 0f)
            {
                _flash -= Time.deltaTime;
                float k = Mathf.Clamp01(_flash / 0.14f);
                for (int i = 0; i < _materials.Count; i++)
                {
                    Color origEm = i < _emissiveColors.Count ? _emissiveColors[i] : Color.black;
                    AetheriaRender.SetEmission(_materials[i], Color.Lerp(origEm, Color.white * 2.5f, k));
                }
                if (_flash <= 0f)
                {
                    for (int i = 0; i < _materials.Count; i++)
                    {
                        Color origEm = i < _emissiveColors.Count ? _emissiveColors[i] : Color.black;
                        AetheriaRender.SetEmission(_materials[i], origEm);
                    }
                }
            }

            // Cracks show how close the prop is to breaking
            float damage = 1f - Mathf.Clamp01(HP / Mathf.Max(1f, MaxHP));
            for (int i = 0; i < _materials.Count && i < _baseColors.Count; i++)
                AetheriaRender.SetBaseColor(_materials[i], Color.Lerp(_baseColors[i], _baseColors[i] * 0.55f, damage * 0.8f));
        }

        /// <summary>Applies damage. Returns true if the prop took it (false when it needs a specific spell).</summary>
        public bool ApplyDamage(float dmg, Vector3 origin, bool isSunStrike, bool isMeteor)
        {
            if (Broken) return false;
            var hero = HeroController.Instance;
            bool god = hero != null && hero.GodMode;
            if (RequiresSunStrike && !isSunStrike && !god)
            {
                if (_flash <= 0f) QuestAndMapService.Instance?.PushNotification("Расколоть может только Солнечная Катастрофа (Z-Z-Z → R)");
                return false;
            }
            if (RequiresMeteor && !isMeteor && !god)
            {
                if (_flash <= 0f) QuestAndMapService.Instance?.PushNotification("Обсидиан поддастся только Хаос-метеору (Z-Z-X → R)");
                return false;
            }

            HP -= dmg;
            _flash = 0.14f;
            Vector3 at = transform.position + Vector3.up * 1.0f;
            AetheriaFx.FlashLight(at, ElementColor(), 16f, 9f, 0.22f);
            AetheriaFx.Burst(origin + Vector3.up * 0.6f, ElementColor(), 12, 2.5f, 7f, 0.07f, 0.16f, 0.4f, 2.4f, new Color(0.5f, 0.3f, 0.1f, 0f));
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_Hit_Stone", at, 0.55f);
            if (HP <= 0f) Shatter(origin);
            return true;
        }

        private Color ElementColor() => Kind switch
        {
            DestructibleKind.Crystal  => AetheriaPalette.TextCyan,
            DestructibleKind.Obsidian => AetheriaPalette.TextCrimson,
            DestructibleKind.Wood     => new Color(0.95f, 0.62f, 0.28f),
            DestructibleKind.Foliage  => AetheriaPalette.TextEmerald,
            _                         => new Color(1f, 0.78f, 0.32f)
        };

        public void Shatter(Vector3 impulseOrigin)
        {
            if (Broken) return;
            Broken = true;
            _restoreTimer = RestoreDelaySeconds;
            if (_regrowRoutine != null) { StopCoroutine(_regrowRoutine); _regrowRoutine = null; }

            SetFocusActive(false);
            foreach (var r in _renderers) if (r != null) r.enabled = false;
            foreach (var c in _colliders) if (c != null) c.enabled = false;

            Color col = ElementColor();
            Vector3 at = transform.position + Vector3.up * 1.0f;
            AetheriaFx.FlashLight(at, col, 38f, 15f, 0.6f);
            AetheriaFx.Shockwave(transform.position + Vector3.up * 0.2f, col, 4.2f, 0.5f);
            AetheriaFx.Shockwave(at, col, 2.6f, 0.4f, vertical: true, facing: impulseOrigin - transform.position);
            AetheriaFx.Burst(at, col, 64, 3.5f, 12f, 0.10f, 0.28f, 0.9f, 1.4f, 2.0f, new Color(col.r * 0.4f, col.g * 0.3f, col.b * 0.3f, 0f));
            AetheriaFx.Puff(at, new Color(0.62f, 0.58f, 0.52f, 0.65f), 16, 3.5f, 2.2f, 1.5f, 0.5f);
            AetheriaFx.Debris(at, Kind == DestructibleKind.Obsidian ? new Color(0.12f, 0.08f, 0.14f) : new Color(0.55f, 0.50f, 0.44f),
                16, impulseOrigin, 11f, 0.30f, false);
            AetheriaFx.Debris(at, col, 10, impulseOrigin, 9f, 0.16f, true);
            AetheriaFx.Flare(at, col, 2.6f, 0.35f);
            AetheriaFx.Shake(0.16f, 0.32f);
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_Barrel_Break", at, 0.85f);
            AudioAndQualityService.Instance?.PlaySfxAt("SFX_Hit_Stone", at, 0.7f);

            if (!_questNotified && !string.IsNullOrEmpty(LinkedQuestId))
            {
                _questNotified = true;
                QuestAndMapService.Instance?.AdvanceQuestProgress(LinkedQuestId, 1);
            }
        }

        public void RestoreNow()
        {
            bool wasBroken = Broken;
            if (!wasBroken && HP >= MaxHP && _flash <= 0f) return;

            if (_regrowRoutine != null) { StopCoroutine(_regrowRoutine); _regrowRoutine = null; }
            Broken = false;
            _questNotified = false;
            _restoreTimer = 0f;
            _flash = 0f;
            HP = MaxHP;
            transform.localScale = _initialScale;

            foreach (var r in _renderers) if (r != null) r.enabled = true;
            foreach (var c in _colliders) if (c != null) c.enabled = true;
            SetFocusActive(true);

            for (int i = 0; i < _materials.Count; i++)
            {
                if (i < _baseColors.Count) AetheriaRender.SetBaseColor(_materials[i], _baseColors[i]);
                Color origEm = i < _emissiveColors.Count ? _emissiveColors[i] : Color.black;
                AetheriaRender.SetEmission(_materials[i], origEm);
            }

            if (wasBroken)
            {
                Vector3 at = transform.position + Vector3.up * 1.0f;
                AetheriaFx.FlashLight(at, AetheriaPalette.TextEmerald, 26f, 12f, 0.7f);
                AetheriaFx.Shockwave(transform.position + Vector3.up * 0.2f, AetheriaPalette.TextEmerald, 3.6f, 0.8f);
                AetheriaFx.Burst(at, AetheriaPalette.TextEmerald, 44, 1.2f, 4.5f, 0.07f, 0.16f, 1.1f, -0.2f, new Color(0.2f, 0.9f, 0.6f, 0f));
                AudioAndQualityService.Instance?.PlaySfxAt("SFX_BeaconCapture", at, 0.5f);
                _regrowRoutine = StartCoroutine(Regrow());
            }
        }

        private IEnumerator Regrow()
        {
            Vector3 full = _initialScale;
            float t = 0f;
            while (t < 1f)
            {
                t += Time.deltaTime * 2.2f;
                float k = 1f - (1f - Mathf.Clamp01(t)) * (1f - Mathf.Clamp01(t));
                transform.localScale = Vector3.Lerp(full * 0.2f, full, k);
                yield return null;
            }
            transform.localScale = full;
            _regrowRoutine = null;
        }
    }
}
