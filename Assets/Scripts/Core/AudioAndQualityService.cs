using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.HighDefinition;

namespace Aetheria
{
    public enum MusicState { None, Menu, Village, Field, Battle, Boss }

    /// <summary>
    /// Auto-detects GPU hardware capabilities for UltraHDRP vs ClassicPerformance,
    /// manages dual-deck orchestral music crossfades + SFX, and drives Wukong-style micro HitStop.
    /// </summary>
    public class AudioAndQualityService : MonoBehaviour
    {
        public static AudioAndQualityService Instance { get; private set; }

        public RenderTier ActiveTier { get; private set; } = RenderTier.CrimsonHdrpUltra;
        public string GPUName { get; private set; } = "Unknown GPU";
        public MusicState CurrentMusic { get; private set; } = MusicState.None;
        public float MasterVolume = 1.0f;
        public float MusicVolume = 0.65f;
        public float SfxVolume = 0.80f;
        public bool MusicMuted = false;
        public bool SfxMuted = false;

        private AudioSource _deckA;
        private AudioSource _deckB;
        private AudioSource _ambientDeck;
        private AudioSource _sfxSource;
        private int _activeDeck = 0;
        private float _combatTimer = 0f;
        private bool _inMenu = true;
        private bool _bossActive = false;
        private bool _inVillage = true;

        private readonly Dictionary<MusicState, AudioClip> _musicClips = new Dictionary<MusicState, AudioClip>();
        private readonly Dictionary<string, AudioClip> _sfxCache = new Dictionary<string, AudioClip>();
        private Coroutine _hitStopRoutine;
        private Coroutine _crossfadeRoutine;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;

            DetectAndApplyQualityTier();
            SetupAudioDecks();
        }

        private void Start()
        {
            TransitionMusic(MusicState.Menu, 0.8f);
        }

        private void Update()
        {
            if (_combatTimer > 0f && !_inMenu && !_bossActive)
            {
                _combatTimer -= Time.unscaledDeltaTime;
                if (_combatTimer <= 0f)
                {
                    TransitionMusic(_inVillage ? MusicState.Village : MusicState.Field, 1.8f);
                }
            }
        }

        public void DetectAndApplyQualityTier()
        {
            GPUName = SystemInfo.graphicsDeviceName ?? "Generic GPU";
            string upper = GPUName.ToUpperInvariant();
            bool highEnd = SystemInfo.graphicsMemorySize >= 4096 &&
                           (upper.Contains("RTX") || upper.Contains("RX 5") || upper.Contains("RX 6") ||
                            upper.Contains("RX 7") || upper.Contains("RX 8") || upper.Contains("RX 9") ||
                            upper.Contains("GTX 10") || upper.Contains("GTX 16") || upper.Contains("ARC") ||
                            upper.Contains("TITAN") || upper.Contains("QUADRO") || upper.Contains("APPLE"));
            ApplyRenderTier(highEnd ? RenderTier.CrimsonHdrpUltra : RenderTier.ClassicPerformance);
        }

        public void ToggleRenderTier()
        {
            ApplyRenderTier(ActiveTier == RenderTier.CrimsonHdrpUltra ? RenderTier.ClassicPerformance : RenderTier.CrimsonHdrpUltra);
            QuestAndMapService.Instance?.PushNotification($"Graphics Mode -> {ActiveTier} ({GPUName})");
        }

        public void ApplyRenderTier(RenderTier tier)
        {
            ActiveTier = tier;
            bool ultra = tier == RenderTier.CrimsonHdrpUltra;
            QualitySettings.shadows = UnityEngine.ShadowQuality.All;
            QualitySettings.lodBias = ultra ? 1.8f : 0.9f;
            QualitySettings.globalTextureMipmapLimit = ultra ? 0 : 1;

            var vol = AetheriaRenderImpl.FindHighestPriorityGlobalVolume(requireGradientSky: false);
            if (vol != null)
            {
                var prof = Application.isPlaying ? vol.profile : vol.sharedProfile;
                if (prof != null)
                {
                    if (!prof.TryGet<HDShadowSettings>(out var shadows))
                        shadows = prof.Add<HDShadowSettings>(true);
                    shadows.maxShadowDistance.Override(ultra ? 180f : 75f);
                    shadows.cascadeShadowSplitCount.Override(ultra ? 4 : 2);
                    ApplyLightingQuality(prof, ultra);
                    VolumeManager.instance?.OnVolumeProfileChanged(prof);
                }
            }

            AetheriaRender.SetUpscalerEnabled(ultra);
            ApplySunShadows(ultra);
        }

        /// <summary>Ambient occlusion, screen-space GI and reflections, contact shadows and volumetric clouds.</summary>
        private static void ApplyLightingQuality(VolumeProfile prof, bool ultra)
        {
            if (!prof.TryGet<ScreenSpaceAmbientOcclusion>(out var ao)) ao = prof.Add<ScreenSpaceAmbientOcclusion>(false);
            ao.active = true;
            ao.intensity.Override(ultra ? 1.1f : 0.8f);
            ao.radius.Override(ultra ? 1.6f : 1.0f);
            ao.directLightingStrength.Override(0.35f);

            if (!prof.TryGet<GlobalIllumination>(out var gi)) gi = prof.Add<GlobalIllumination>(false);
            gi.active = true;
            gi.enable.Override(ultra);
            gi.tracing.Override(RayCastingMode.RayMarching);   // ray tracing was configured but never ran in the player

            if (!prof.TryGet<ScreenSpaceReflection>(out var ssr)) ssr = prof.Add<ScreenSpaceReflection>(false);
            ssr.active = true;
            ssr.enabled.Override(true);
            ssr.tracing.Override(RayCastingMode.RayMarching);

            if (!prof.TryGet<ContactShadows>(out var contact)) contact = prof.Add<ContactShadows>(false);
            contact.active = true;
            contact.enable.Override(true);
            contact.length.Override(0.25f);

            if (!prof.TryGet<VolumetricClouds>(out var clouds)) clouds = prof.Add<VolumetricClouds>(false);
            clouds.active = true;
            clouds.enable.Override(ultra);
            clouds.cloudPreset = VolumetricClouds.CloudPresets.Sparse;
        }

        private static void ApplySunShadows(bool ultra)
        {
            foreach (var l in Object.FindObjectsByType<Light>(FindObjectsSortMode.None))
            {
                if (l.type != LightType.Directional) continue;
                l.shadows = LightShadows.Soft;
                var hd = l.GetComponent<HDAdditionalLightData>();
                if (hd == null) continue;
                hd.SetShadowResolutionLevel(ultra ? 3 : 1);
                hd.shadowDimmer = 1f;
            }
        }

        private void SetupAudioDecks()
        {
            AudioSource MakeSource(string label, bool loop)
            {
                var go = new GameObject(label);
                go.transform.SetParent(transform);
                var src = go.AddComponent<AudioSource>();
                src.spatialBlend = 0f;
                src.loop = loop;
                src.playOnAwake = false;
                return src;
            }

            _deckA = MakeSource("MusicDeckA", true);
            _deckB = MakeSource("MusicDeckB", true);
            _ambientDeck = MakeSource("AmbientDeck", true);
            _sfxSource = MakeSource("UISfxSource", false);

            // Bind exported audio clips from Resources/Audio if copied during project setup
            LoadMusic(MusicState.Menu, "Audio/WAV_Menu_Loop", "Audio/BGM_Menu_Sanctuary", "Audio/BGM_Sanctuary_Gothic");
            LoadMusic(MusicState.Village, "Audio/WAV_Village_Loop", "Audio/BGM_Sanctuary_Gothic");
            LoadMusic(MusicState.Field, "Audio/WAV_Field_Loop", "Audio/BGM_World_Expedition");
            LoadMusic(MusicState.Battle, "Audio/WAV_Battle_Loop", "Audio/BGM_Abyssal_Expedition");
            LoadMusic(MusicState.Boss, "Audio/WAV_BossBattle_Loop", "Audio/BGM_Boss_Dread");

            var amb = Resources.Load<AudioClip>("Audio/SFX_Amb_Brazier_Loop");
            if (amb != null)
            {
                _ambientDeck.clip = amb;
                _ambientDeck.volume = MusicMuted ? 0f : MusicVolume * MasterVolume * 0.22f;
                _ambientDeck.Play();
            }
        }

        private void LoadMusic(MusicState state, params string[] candidates)
        {
            foreach (var c in candidates)
            {
                var clip = Resources.Load<AudioClip>(c);
                if (clip != null) { _musicClips[state] = clip; return; }
            }
        }

        public void SetMenuActive(bool inMenu)
        {
            _inMenu = inMenu;
            if (_inMenu) TransitionMusic(MusicState.Menu, 0.9f);
            else TransitionMusic(_bossActive ? MusicState.Boss : (_inVillage ? MusicState.Village : MusicState.Field), 1.2f);
        }

        public void SetZoneVillage(bool inVillage)
        {
            if (_inVillage == inVillage) return;
            _inVillage = inVillage;
            if (!_inMenu && !_bossActive && _combatTimer <= 0f)
                TransitionMusic(_inVillage ? MusicState.Village : MusicState.Field, 1.5f);
        }

        public void NotifyCombat()
        {
            _combatTimer = 5.0f;
            if (!_inMenu && !_bossActive && CurrentMusic != MusicState.Battle)
                TransitionMusic(MusicState.Battle, 0.7f);
        }

        public void SetBossActive(bool active)
        {
            _bossActive = active;
            if (!_inMenu)
                TransitionMusic(active ? MusicState.Boss : (_inVillage ? MusicState.Village : MusicState.Field), 1.0f);
        }

        public void SetMasterVolume(float volume)
        {
            MasterVolume = Mathf.Clamp01(volume);
            ApplyMusicVolumes();
        }

        public void SetMusicVolume(float volume)
        {
            MusicVolume = Mathf.Clamp01(volume);
            ApplyMusicVolumes();
        }

        public void SetSfxVolume(float volume)
        {
            SfxVolume = Mathf.Clamp01(volume);
        }

        public void ToggleSfxMute()
        {
            SfxMuted = !SfxMuted;
        }

        public void SetSfxMuted(bool mute)
        {
            SfxMuted = mute;
        }

        public void SetMuted(bool mute)
        {
            MusicMuted = mute;
            ApplyMusicVolumes();
        }

        private void ApplyMusicVolumes()
        {
            float vol = MusicMuted ? 0f : MusicVolume * MasterVolume;
            if (_deckA != null && _deckB != null)
                (_activeDeck == 0 ? _deckA : _deckB).volume = vol;
            if (_ambientDeck != null)
                _ambientDeck.volume = vol * 0.22f;
        }

        public void TransitionMusic(MusicState next, float fadeSec)
        {
            if (CurrentMusic == next) return;
            if (!_musicClips.TryGetValue(next, out var clip) || clip == null) return;
            CurrentMusic = next;
            if (_crossfadeRoutine != null) StopCoroutine(_crossfadeRoutine);
            var incoming = _activeDeck == 0 ? _deckB : _deckA;
            var outgoing = _activeDeck == 0 ? _deckA : _deckB;
            _activeDeck = 1 - _activeDeck;
            _crossfadeRoutine = StartCoroutine(CrossfadeRoutine(incoming, outgoing, clip, fadeSec));
        }

        private IEnumerator CrossfadeRoutine(AudioSource inc, AudioSource outg, AudioClip clip, float dur)
        {
            inc.clip = clip;
            inc.volume = 0f;
            inc.Play();
            float target = MusicMuted ? 0f : MusicVolume * MasterVolume;
            float startOut = outg.volume;
            float t = 0f;
            float safeDur = Mathf.Max(0.05f, dur);
            while (t < safeDur)
            {
                t += Time.unscaledDeltaTime;
                float u = Mathf.Clamp01(t / safeDur);
                target = MusicMuted ? 0f : MusicVolume * MasterVolume;
                inc.volume = Mathf.Lerp(0f, target, u);
                outg.volume = Mathf.Lerp(startOut, 0f, u);
                yield return null;
            }
            outg.Stop();
            inc.volume = MusicMuted ? 0f : MusicVolume * MasterVolume;
            _crossfadeRoutine = null;
        }

        public void PlaySfx2D(string clipName, float vol = 1f)
        {
            if (SfxMuted || string.IsNullOrEmpty(clipName)) return;
            if (!_sfxCache.TryGetValue(clipName, out var clip))
            {
                clip = Resources.Load<AudioClip>("Audio/" + clipName);
                _sfxCache[clipName] = clip;
            }
            if (clip != null && _sfxSource != null) _sfxSource.PlayOneShot(clip, vol * SfxVolume * MasterVolume);
        }

        public void PlaySfxAt(string clipName, Vector3 pos, float vol = 0.9f)
        {
            if (SfxMuted || string.IsNullOrEmpty(clipName)) return;
            if (!_sfxCache.TryGetValue(clipName, out var clip))
            {
                clip = Resources.Load<AudioClip>("Audio/" + clipName);
                _sfxCache[clipName] = clip;
            }
            if (clip != null) AudioSource.PlayClipAtPoint(clip, pos, vol * SfxVolume * MasterVolume);
        }

        private static bool IsGamePaused()
        {
            if (Time.timeScale <= 0.001f) return true;
            var hud = AetheriaHUDManager.Instance;
            return hud != null && hud.IsPaused;
        }

        public void RequestHitStop(float durationSec = 0.055f, float scale = 0.08f)
        {
            if (IsGamePaused()) return;
            if (_hitStopRoutine != null) StopCoroutine(_hitStopRoutine);
            _hitStopRoutine = StartCoroutine(HitStopRoutine(durationSec, scale));
        }

        private IEnumerator HitStopRoutine(float durationSec, float scale)
        {
            Time.timeScale = Mathf.Clamp(scale, 0.02f, 1f);
            yield return new WaitForSecondsRealtime(durationSec);
            if (Time.timeScale > 0.001f && (AetheriaHUDManager.Instance == null || !AetheriaHUDManager.Instance.IsPaused))
                Time.timeScale = 1f;
            _hitStopRoutine = null;
        }
    }
}
