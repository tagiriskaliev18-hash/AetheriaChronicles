using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

namespace Aetheria
{
    /// <summary>
    /// Runtime World Atmosphere & Spell-Reactive Terrain Scar / 60-Second Regrowth Manager.
    /// Coordinates the day cycle (sun and sky are one palette) and UI/World Bootstrap.
    /// </summary>
    public class AetheriaWorldGenerator : MonoBehaviour
    {
        public static AetheriaWorldGenerator Instance { get; private set; }

        [Range(0f, 1f)]
        public float TimeOfDay = 0.68f; // Default: Crimson Desert Golden Sunset

        public Light SunLight;
        public Volume GlobalVolume;
        public Transform MirrorLakeSurface;

        private readonly float[] _presets = { 0.68f, 0.04f, 0.28f, 0.50f };
        private readonly string[] _presetLabels = { "Golden Sunset", "Starry Full Moon", "Misty Dawn", "Highland Noon" };
        private int _presetIdx = 0;
        private bool _skyPending;
        private int _skyTries;

        public static readonly string[] ShotNames =
        {
            "01_title_oakhaven", "02_hero_customizer", "03_prologue_cave", "04_oakhaven_arrival",
            "05_oakhaven_combat_sunstrike", "06_dragon_ignis_flight", "07_malakor_boss_highkeep", "08_world_map_fog"
        };

        private class TerrainScar
        {
            public GameObject DecalObj;
            public float Remaining;
        }

        private readonly List<TerrainScar> _scars = new List<TerrainScar>();
        private static readonly List<BreakableProp> _breakables = new List<BreakableProp>();

        public static void RegisterBreakable(BreakableProp p) { if (!_breakables.Contains(p)) _breakables.Add(p); }
        public static void UnregisterBreakable(BreakableProp p) => _breakables.Remove(p);

        private void Awake()
        {
            Instance = this;
            if (GetComponent<AudioAndQualityService>() == null) gameObject.AddComponent<AudioAndQualityService>();
            if (GetComponent<QuestAndMapService>() == null) gameObject.AddComponent<QuestAndMapService>();
            if (GetComponent<AetheriaHUDManager>() == null) gameObject.AddComponent<AetheriaHUDManager>();
            if (GetComponent<AetheriaOverlays>() == null) gameObject.AddComponent<AetheriaOverlays>();

            if (SunLight == null)
            {
                foreach (var l in FindObjectsByType<Light>(FindObjectsSortMode.None))
                    if (l.type == LightType.Directional) { SunLight = l; break; }
            }
            if (GlobalVolume == null) GlobalVolume = FindFirstObjectByType<Volume>();
        }

        private void Start()
        {
            ApplyTimeOfDay(TimeOfDay);
            string[] args = System.Environment.GetCommandLineArgs();
            for (int i = 0; i < args.Length - 1; i++)
            {
                if (args[i] == "-verify-screenshots")
                {
                    StartCoroutine(RunVerificationCaptureRoutine(args[i + 1]));
                    break;
                }
            }
        }

        /// <summary>
        /// Sets up verification shot n (see <see cref="ShotNames"/>) and holds the scene there,
        /// so the caller decides when to capture. Shots build on each other: call them in order.
        /// </summary>
        public void StageShot(int n)
        {
            var hud = AetheriaHUDManager.Instance;
            var hero = HeroController.Instance;
            if (hud == null || hero == null) return;
            var map = QuestAndMapService.Instance;

            switch (n)
            {
                case 0:  // Main Title over Oakhaven Valley
                {
                    hud.SetOverlay(OverlayMode.MainMenu);
                    break;
                }
                case 1:  // Hero Creator Studio [C] with Highland Nord + Vanguard Plate
                {
                    hero.Customization.SetRace(HeroRace.HighlandNord, false);
                    hero.Customization.Profile.HeightMeters = 2.05f;
                    hero.Customization.Profile.ArmorTintIndex = 2; // Sovereign Crimson
                    hero.Customization.Recalculate();
                    hero.SetCreatorStudioCamera(true, 175f, 0);
                    hud.SetOverlay(OverlayMode.HeroCustomizer);
                    break;
                }
                case 2:  // Prologue Awakening Cave & Guardian Weylan
                {
                    hero.Customization.SetRace(HeroRace.Human, true);
                    hero.StartPrologueAwakening();
                    hud.SetOverlay(OverlayMode.None);
                    break;
                }
                case 3:  // Arrival in the open world
                {
                    hud.SetOverlay(OverlayMode.None);
                    hero.SkipToOpenWorld(false);
                    hero.SetGodMode(true);
                    break;
                }
                case 4:  // Oakhaven Combat & EEE Solar Cataclysm
                {
                    hud.SetOverlay(OverlayMode.None);
                    map?.RevealAround(new Vector3(0f, 0f, 42f), 260f, true);
                    map?.RevealAround(new Vector3(14f, 0f, -8f), 200f, true);
                    Vector3 castTarget = hero.transform.position + hero.transform.forward * 9f;
                    hero.Spells.CastSpecificSpell(SpellId.SunStrike, hero.transform.position + Vector3.up * 1.4f, hero.transform.forward, castTarget);
                    break;
                }
                case 5:  // Rideable Dragon Ignis over Mirror Lake & Bridge
                {
                    hud.SetOverlay(OverlayMode.None);
                    var drake = DragonController.Instance;
                    if (drake != null)
                    {
                        drake.transform.rotation = Quaternion.Euler(0f, 38f, 0f);
                        drake.transform.position = new Vector3(-4f, 16f, -24f);
                        drake.Mount(hero);
                        drake.ForceBreathForDemo = true;
                    }
                    break;
                }
                case 6:  // Highkeep Citadel Boss Malakor
                {
                    hud.SetOverlay(OverlayMode.None);
                    var drake = DragonController.Instance;
                    if (drake != null)
                    {
                        drake.ForceBreathForDemo = false;
                        if (drake.Mounted) drake.Dismount();
                    }
                    map?.RevealAround(new Vector3(0f, 16f, -104f), 220f, true);
                    while (map != null && map.PopNotification() != null) { }
                    hero.transform.rotation = Quaternion.Euler(0f, 180f, 0f);
                    hero.TeleportSafe(new Vector3(0f, 16f, -92f));
                    break;
                }
                case 7:  // World Map Fog-of-War & Teleport Landmarks
                {
                    hud.SetOverlay(OverlayMode.WorldMap);
                    break;
                }
            }
        }

        /// <summary>Seconds a staged shot needs before it has settled enough to capture.</summary>
        public static float ShotSettleTime(int n)
        {
            switch (n)
            {
                case 3: return 1.5f;
                case 4: return 0.35f;
                case 5: return 0.95f;
                case 6: return 1.35f;
                default: return 0.7f;
            }
        }

        private System.Collections.IEnumerator RunVerificationCaptureRoutine(string outDir)
        {
            System.IO.Directory.CreateDirectory(outDir);
            yield return new WaitForSeconds(1.0f);
            if (AetheriaHUDManager.Instance == null || HeroController.Instance == null) { Application.Quit(2); yield break; }

            for (int i = 0; i < ShotNames.Length; i++)
            {
                StageShot(i);
                yield return new WaitForSeconds(ShotSettleTime(i));
                ScreenCapture.CaptureScreenshot(System.IO.Path.Combine(outDir, ShotNames[i] + ".png"));
                yield return new WaitForSeconds(0.4f);
            }

            UnityEngine.Debug.Log($"VERIFICATION_CAPTURES_DONE dir={outDir}");
            Application.Quit(0);
        }

        private void Update()
        {
            // The render pipeline is created after Start, so the opening sky may have to wait for its first frame.
            if (_skyPending && AetheriaRender.SkyReady && _skyTries++ < 120) ApplyTimeOfDay(TimeOfDay);

            for (int i = _scars.Count - 1; i >= 0; i--)
            {
                _scars[i].Remaining -= Time.deltaTime;
                if (_scars[i].Remaining <= 0f)
                {
                    if (_scars[i].DecalObj != null)
                    {
                        AetheriaFx.Burst(_scars[i].DecalObj.transform.position, AetheriaPalette.TextEmerald, 18, 1.5f, 4f, 0.1f, 0.2f, 0.5f, 1f);
                        Destroy(_scars[i].DecalObj);
                    }
                    _scars.RemoveAt(i);
                }
            }
        }

        public void CycleTimeOfDayPreset()
        {
            _presetIdx = (_presetIdx + 1) % _presets.Length;
            ApplyTimeOfDay(_presets[_presetIdx]);
            QuestAndMapService.Instance?.PushNotification($"Sky Time -> {GetTimeOfDayLabel()}");
        }

        public string GetTimeOfDayLabel() => _presetLabels[_presetIdx];

        // ------------------------------------------------------------------ day cycle

        /// <summary>One moment of the day cycle: the sun and the sky it sits in (linear colours).</summary>
        private struct SkyKey
        {
            public float T, Sun;
            public Color SunColor, Top, Middle, Bottom;
        }

        private static Color C(float r, float g, float b) => new Color(r, g, b, 1f);

        private static SkyKey Key(float t, float sun, Color sunColor, Color top, Color middle, Color bottom) =>
            new SkyKey { T = t, Sun = sun, SunColor = sunColor, Top = top, Middle = middle, Bottom = bottom };

        private static readonly SkyKey Night = Key(0.00f, 0.10f, C(0.45f, 0.58f, 0.92f),
            C(0.030f, 0.045f, 0.100f), C(0.050f, 0.070f, 0.140f), C(0.090f, 0.100f, 0.180f));

        private static readonly SkyKey[] SkyKeys =
        {
            Night,                                                                                     // starry full moon
            Key(0.28f, 1.80f, C(1.00f, 0.80f, 0.62f), C(0.200f, 0.300f, 0.500f), C(0.620f, 0.600f, 0.660f), C(0.900f, 0.700f, 0.580f)),  // misty dawn
            Key(0.50f, 3.20f, C(1.00f, 0.96f, 0.88f), C(0.100f, 0.260f, 0.580f), C(0.500f, 0.660f, 0.860f), C(0.800f, 0.820f, 0.840f)),  // highland noon
            Key(0.68f, 2.60f, C(1.00f, 0.76f, 0.50f), C(0.120f, 0.260f, 0.520f), C(0.550f, 0.660f, 0.800f), C(0.920f, 0.720f, 0.500f)),  // golden sunset
            Key(0.82f, 0.60f, C(1.00f, 0.55f, 0.35f), C(0.040f, 0.060f, 0.160f), C(0.300f, 0.220f, 0.340f), C(0.700f, 0.380f, 0.280f)),  // ember dusk
            new SkyKey { T = 1f, Sun = Night.Sun, SunColor = Night.SunColor, Top = Night.Top, Middle = Night.Middle, Bottom = Night.Bottom },
        };

        private static SkyKey SampleSky(float t)
        {
            for (int i = 0; i < SkyKeys.Length - 1; i++)
            {
                SkyKey a = SkyKeys[i], b = SkyKeys[i + 1];
                if (t < a.T || t > b.T) continue;
                float k = Mathf.SmoothStep(0f, 1f, (t - a.T) / (b.T - a.T));
                return new SkyKey
                {
                    T = t,
                    Sun = Mathf.Lerp(a.Sun, b.Sun, k),
                    SunColor = Color.Lerp(a.SunColor, b.SunColor, k),
                    Top = Color.Lerp(a.Top, b.Top, k),
                    Middle = Color.Lerp(a.Middle, b.Middle, k),
                    Bottom = Color.Lerp(a.Bottom, b.Bottom, k),
                };
            }
            return SkyKeys[SkyKeys.Length - 1];
        }

        /// <summary>Applies a moment of the day cycle: sun direction, sun strength/colour and the matching sky.</summary>
        public void ApplyTimeOfDay(float t)
        {
            TimeOfDay = Mathf.Repeat(t, 1f);
            bool night = TimeOfDay < 0.22f || TimeOfDay > 0.80f;
            bool lowSun = TimeOfDay > 0.60f && TimeOfDay < 0.78f;
            float sunPitch = lowSun ? 15f : (night ? -14f : 48f);

            // The palette is authored here; the render pipeline applies it in its own units.
            SkyKey sky = SampleSky(TimeOfDay);
            if (SunLight != null)
            {
                SunLight.transform.rotation = Quaternion.Euler(sunPitch, 145f + TimeOfDay * 180f, 0f);
                AetheriaRender.StyleSun(SunLight, sky.Sun, sky.SunColor);
            }
            AetheriaRender.SetSkyColors(sky.Top, sky.Middle, sky.Bottom, Color.Lerp(sky.Middle, sky.Bottom, 0.5f));
            _skyPending = !AetheriaRender.SkyReady;
            HeroController.Instance?.Customization?.Recalculate();
        }

        public void RegisterSpellTerrainScar(Vector3 worldPos, float radius, OrbType element)
        {
            if (_scars.Count >= 28)
            {
                if (_scars[0].DecalObj != null) Destroy(_scars[0].DecalObj);
                _scars.RemoveAt(0);
            }

            float gy = Terrain.activeTerrain != null ? Terrain.activeTerrain.SampleHeight(worldPos) + Terrain.activeTerrain.transform.position.y : 0f;
            var go = new GameObject("SpellTerrainScar");
            go.transform.position = new Vector3(worldPos.x, gy + 0.08f, worldPos.z);
            var lr = go.AddComponent<LineRenderer>();
            lr.useWorldSpace = false;
            lr.loop = true;
            lr.positionCount = 36;
            lr.startWidth = 0.22f;
            lr.endWidth = 0.22f;
            Color elemCol = AetheriaPalette.GetOrbColor(element);
            lr.startColor = elemCol;
            lr.endColor = elemCol;
            lr.material = AetheriaPalette.GetLitMaterial(elemCol, elemCol * 1.6f);
            float r = Mathf.Max(1.2f, radius * 0.45f);
            for (int i = 0; i < 36; i++)
            {
                float a = (i / 36f) * Mathf.PI * 2f;
                lr.SetPosition(i, new Vector3(Mathf.Cos(a) * r, 0f, Mathf.Sin(a) * r));
            }
            _scars.Add(new TerrainScar { DecalObj = go, Remaining = 60f });
        }

        public int RestoreAllScarsAndPropsNow()
        {
            int count = _scars.Count;
            foreach (var s in _scars)
            {
                if (s.DecalObj != null)
                {
                    AetheriaFx.Burst(s.DecalObj.transform.position, AetheriaPalette.TextEmerald, 18, 1.5f, 4f, 0.1f, 0.2f, 0.5f, 1f);
                    Destroy(s.DecalObj);
                }
            }
            _scars.Clear();

            foreach (var b in _breakables)
            {
                if (b != null && b.Broken) { b.RestoreNow(); count++; }
            }
            return count;
        }
    }
}
