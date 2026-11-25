using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace Aetheria
{
    /// <summary>
    /// Persistent dark-fantasy gameplay HUD using exported obsidian panels, globes,
    /// sockets, TTF serif/display typography, tracked quest compass, minimap, boss stagger bar, and hotkey bar.
    /// </summary>
    public class AetheriaHUDManager : MonoBehaviour
    {
        public static AetheriaHUDManager Instance { get; private set; }

        public OverlayMode ActiveOverlay = OverlayMode.MainMenu;
        public bool IsModalOpen => ActiveOverlay != OverlayMode.None;
        public Canvas MainCanvas { get; private set; }
        public Font DisplayFont { get; private set; }
        public Font SerifFont { get; private set; }

        private GameObject _hudRoot;
        private Image _hpFill, _stamFill, _manaFill, _xpFill;
        private Text _vitalsText, _questText, _biomeText, _toastText, _orbsText, _slotDText, _slotFText;
        private GameObject _bossPanel;
        private Image _bossHpFill, _bossStagFill;
        private Text _bossTitleText;
        private GameObject _dragonPanel;
        private Image _dragonFireFill;
        private RawImage _minimapImg;
        private Texture2D _minimapTex;
        private Color[] _minimapPixels;
        private float _toastTimer;
        private float _minimapRefreshAt;

        /// <summary>Minimap is a deliberately tiny, cheap view: the painted chart only appears on [M].</summary>
        private const int MinimapTexSize = 96;
        private const float MinimapWorldSpan = 260f;

        private void Awake()
        {
            Instance = this;
            DisplayFont = Resources.Load<Font>("Fonts/AetheriaDisplay-Bold") ?? Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            SerifFont = Resources.Load<Font>("Fonts/AetheriaSerif-SemiBold") ?? DisplayFont;
            EnsureEventSystem();
            BuildCanvasAndHUD();
        }

        private void Start()
        {
            SetOverlay(OverlayMode.MainMenu);
        }

        private void EnsureEventSystem()
        {
            if (FindFirstObjectByType<EventSystem>() != null) return;
            var es = new GameObject("EventSystem");
            es.AddComponent<EventSystem>();
            es.AddComponent<StandaloneInputModule>();
        }

        private void BuildCanvasAndHUD()
        {
            var cgo = new GameObject("AetheriaHUDCanvas");
            MainCanvas = cgo.AddComponent<Canvas>();
            MainCanvas.renderMode = RenderMode.ScreenSpaceOverlay;
            MainCanvas.sortingOrder = 10;
            var scaler = cgo.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920, 1080);
            scaler.matchWidthOrHeight = 0.5f;
            cgo.AddComponent<GraphicRaycaster>();

            _hudRoot = CreatePanel("GameplayHUD", MainCanvas.transform, Vector2.zero, Vector2.one, Vector2.zero, Vector2.zero, Color.clear);

            // Top-Left: Biome Banner + Tracked Quest (strictly non-overlapping top/bottom bands)
            var questBox = CreatePanel("QuestTracker", _hudRoot.transform, new Vector2(0, 1), new Vector2(0, 1), new Vector2(24, -142), new Vector2(470, -20), AetheriaPalette.PanelBg, true);
            _biomeText = CreateText("BiomeLabel", questBox.transform, new Vector2(0, 1), new Vector2(1, 1), new Vector2(14, -34), new Vector2(-14, -8), 14, AetheriaPalette.TextGold, DisplayFont, TextAnchor.UpperLeft);
            _questText = CreateText("QuestInfo", questBox.transform, new Vector2(0, 0), new Vector2(1, 1), new Vector2(14, 10), new Vector2(-14, -38), 14, AetheriaPalette.TextPrimary, SerifFont, TextAnchor.UpperLeft);

            // Top-Center: Boss Malakor Health + Stagger Bar
            _bossPanel = CreatePanel("BossBar", _hudRoot.transform, new Vector2(0.5f, 1), new Vector2(0.5f, 1), new Vector2(-340, -108), new Vector2(340, -20), AetheriaPalette.PanelBgDeep, true);
            _bossTitleText = CreateText("BossName", _bossPanel.transform, new Vector2(0, 1), new Vector2(1, 1), new Vector2(16, -36), new Vector2(-16, -6), 17, AetheriaPalette.TextCrimson, DisplayFont, TextAnchor.MiddleCenter);
            _bossHpFill = CreateBar("BossHPFill", _bossPanel.transform, new Vector2(20, 28), new Vector2(-20, 44), AetheriaPalette.BarBossHP);
            _bossStagFill = CreateBar("BossStaggerFill", _bossPanel.transform, new Vector2(20, 10), new Vector2(-20, 22), AetheriaPalette.BarStagger);
            _bossPanel.SetActive(false);

            // Top-Right: Minimap (Live 64x64 Fog Texture)
            var mmFrame = CreatePanel("MinimapBox", _hudRoot.transform, new Vector2(1, 1), new Vector2(1, 1), new Vector2(-206, -214), new Vector2(-24, -20), AetheriaPalette.PanelBgDeep, true);
            var mmGo = new GameObject("MinimapTexture");
            mmGo.transform.SetParent(mmFrame.transform, false);
            var mmRt = mmGo.AddComponent<RectTransform>();
            mmRt.anchorMin = Vector2.zero; mmRt.anchorMax = Vector2.one;
            mmRt.offsetMin = new Vector2(6, 26); mmRt.offsetMax = new Vector2(-6, -6);
            _minimapImg = mmGo.AddComponent<RawImage>();
            _minimapTex = new Texture2D(MinimapTexSize, MinimapTexSize, TextureFormat.RGBA32, false) { filterMode = FilterMode.Point, wrapMode = TextureWrapMode.Clamp };
            _minimapImg.texture = _minimapTex;
            CreateText("MinimapHint", mmFrame.transform, new Vector2(0, 0), new Vector2(1, 0), new Vector2(6, 2), new Vector2(-6, 22), 12, AetheriaPalette.TextGold, DisplayFont, TextAnchor.MiddleCenter, "[M] MAP");

            // Center Toast Banner
            _toastText = CreateText("ToastBanner", _hudRoot.transform, new Vector2(0.25f, 0.72f), new Vector2(0.75f, 0.80f), Vector2.zero, Vector2.zero, 20, AetheriaPalette.TextGold, DisplayFont, TextAnchor.MiddleCenter);
            _toastText.gameObject.AddComponent<Outline>().effectColor = Color.black;

            // Bottom-Center: Vitals + Invoker Orbs + Cast Bar (150px tall dock with strict internal bands)
            var bot = CreatePanel("BottomDock", _hudRoot.transform, new Vector2(0.5f, 0), new Vector2(0.5f, 0), new Vector2(-370, 16), new Vector2(370, 166), AetheriaPalette.PanelBg, true);
            _vitalsText = CreateText("VitalsTxt", bot.transform, new Vector2(0, 1), new Vector2(1, 1), new Vector2(16, -34), new Vector2(-16, -8), 14, AetheriaPalette.TextPrimary, SerifFont, TextAnchor.MiddleLeft);
            _hpFill = CreateBar("HPBar", bot.transform, new Vector2(16, 92), new Vector2(345, 108), AetheriaPalette.BarHP);
            _stamFill = CreateBar("StamBar", bot.transform, new Vector2(16, 70), new Vector2(345, 84), AetheriaPalette.BarStamina);
            _manaFill = CreateBar("ManaBar", bot.transform, new Vector2(16, 48), new Vector2(345, 62), AetheriaPalette.BarMana);
            _xpFill = CreateBar("XPBar", bot.transform, new Vector2(16, 14), new Vector2(-16, 24), AetheriaPalette.BarXP);

            _orbsText = CreateText("OrbsTxt", bot.transform, new Vector2(0, 0), new Vector2(1, 0), new Vector2(364, 84), new Vector2(-16, 112), 16, AetheriaPalette.TextCyan, DisplayFont, TextAnchor.MiddleLeft);
            _slotDText = CreateText("SlotDTxt", bot.transform, new Vector2(0, 0), new Vector2(1, 0), new Vector2(364, 58), new Vector2(-16, 82), 14, AetheriaPalette.TextGold, SerifFont, TextAnchor.MiddleLeft);
            _slotFText = CreateText("SlotFTxt", bot.transform, new Vector2(0, 0), new Vector2(1, 0), new Vector2(364, 34), new Vector2(-16, 56), 13, AetheriaPalette.TextSecondary, SerifFont, TextAnchor.MiddleLeft);

            // Dragon Breath Panel (sits cleanly above BottomDock with zero overlap!)
            _dragonPanel = CreatePanel("DragonPanel", _hudRoot.transform, new Vector2(0.5f, 0), new Vector2(0.5f, 0), new Vector2(-190, 178), new Vector2(190, 224), AetheriaPalette.PanelBgDeep, true);
            CreateText("DrakeTitle", _dragonPanel.transform, new Vector2(0, 1), new Vector2(1, 1), new Vector2(8, -24), new Vector2(-8, -4), 12, AetheriaPalette.TextAmber, DisplayFont, TextAnchor.MiddleCenter, "IGNIS DRAGONFIRE [LMB] · [F] LAND");
            _dragonFireFill = CreateBar("DrakeFire", _dragonPanel.transform, new Vector2(14, 8), new Vector2(-14, 18), AetheriaPalette.OrbExort);
            _dragonPanel.SetActive(false);

            // Hotkey legend strip
            CreateText("HotkeysStrip", _hudRoot.transform, new Vector2(0.02f, 0.01f), new Vector2(0.32f, 0.09f), Vector2.zero, Vector2.zero,
                12, AetheriaPalette.TextSecondary, SerifFont, TextAnchor.LowerLeft,
                "[C] Customizer · [K] Skills · [J] Quests\n[B] Grimoire · [M] Map · [T] Sky · [~] Console");
        }

        private void Update()
        {
            HandleGlobalKeys();
            RefreshHUD();
        }

        private void HandleGlobalKeys()
        {
            if (Input.GetKeyDown(KeyCode.BackQuote) || Input.GetKeyDown(KeyCode.F1))
                ToggleOverlay(OverlayMode.DevConsole);
            else if (Input.GetKeyDown(KeyCode.Escape))
            {
                if (ActiveOverlay == OverlayMode.DevConsole || (IsModalOpen && ActiveOverlay != OverlayMode.MainMenu))
                    SetOverlay(OverlayMode.None);
                else if (HeroController.Instance != null && HeroController.Instance.HasEnteredWorld)
                    ToggleOverlay(OverlayMode.PauseMenu);
            }
            else if (ActiveOverlay == OverlayMode.None || IsModalOpen)
            {
                if (ActiveOverlay == OverlayMode.DevConsole) return;
                if (Input.GetKeyDown(KeyCode.C)) ToggleOverlay(OverlayMode.HeroCustomizer);
                else if (Input.GetKeyDown(KeyCode.M)) ToggleOverlay(OverlayMode.WorldMap);
                else if (Input.GetKeyDown(KeyCode.K)) ToggleOverlay(OverlayMode.SkillTree);
                else if (Input.GetKeyDown(KeyCode.J)) ToggleOverlay(OverlayMode.QuestLog);
                else if (Input.GetKeyDown(KeyCode.B)) ToggleOverlay(OverlayMode.Grimoire);
                else if (Input.GetKeyDown(KeyCode.F10)) AudioAndQualityService.Instance?.ToggleRenderTier();
            }

            if (HeroController.Instance != null && HeroController.Instance.IsDead && Input.GetKeyDown(KeyCode.E))
                HeroController.Instance.RespawnAtOakhaven();
        }

        public void ToggleOverlay(OverlayMode target) => SetOverlay(ActiveOverlay == target ? OverlayMode.None : target);

        public void SetOverlay(OverlayMode mode)
        {
            ActiveOverlay = mode;
            bool modal = mode != OverlayMode.None;
            Cursor.visible = modal;
            Cursor.lockState = modal ? CursorLockMode.None : CursorLockMode.Locked;
            _hudRoot?.SetActive(!modal);

            HeroController.Instance?.SetCreatorStudioCamera(mode == OverlayMode.HeroCustomizer);
            AudioAndQualityService.Instance?.SetMenuActive(mode == OverlayMode.MainMenu);
            AetheriaOverlays.Instance?.OnOverlayChanged(mode);
        }

        public void OpenDialogue(AetheriaNPC npc)
        {
            AetheriaOverlays.Instance?.SetDialogueTarget(npc);
            SetOverlay(OverlayMode.Dialogue);
        }

        private void RefreshHUD()
        {
            var hero = HeroController.Instance;
            if (hero == null) return;

            if (_toastTimer > 0f)
            {
                _toastTimer -= Time.deltaTime;
                if (_toastTimer <= 0f) _toastText.text = "";
            }
            else
            {
                var nextToast = QuestAndMapService.Instance?.PopNotification();
                if (!string.IsNullOrEmpty(nextToast))
                {
                    _toastText.text = nextToast;
                    _toastTimer = 4.2f;
                }
            }

            if (IsModalOpen) return;

            var st = hero.SkillTree;
            _vitalsText.text = $"LVL {st.Level} · {st.Gold}G · HP {hero.HP:F0}/{hero.MaxHP:F0} · MANA {hero.Spells.Mana:F0}/{hero.Spells.MaxMana:F0}";
            _hpFill.fillAmount = Mathf.Clamp01(hero.HP / Mathf.Max(1f, hero.MaxHP));
            _stamFill.fillAmount = Mathf.Clamp01(hero.Stamina / Mathf.Max(1f, hero.MaxStamina));
            _manaFill.fillAmount = Mathf.Clamp01(hero.Spells.Mana / Mathf.Max(1f, hero.Spells.MaxMana));
            _xpFill.fillAmount = Mathf.Clamp01(st.CurrentXP / (float)Mathf.Max(1, st.GetXPForNextLevel()));

            _orbsText.text = $"ORBS [{hero.Spells.GetOrbsString()}]  ({hero.Spells.GetUnlockedSpellCount()}/10)";
            var defD = SpellSystem.GetSpellDef(hero.Spells.SlotD);
            var defF = SpellSystem.GetSpellDef(hero.Spells.SlotF);
            _slotDText.text = $"[G/1-0] {(defD != null ? defD.DisplayName : "Orbs [Z/X/Q] + [R]")}";
            _slotFText.text = defF != null ? $"[H] Alt: {defF.DisplayName}" : "[F] Drake · [RMB] Parry";

            var map = QuestAndMapService.Instance;
            if (map != null)
            {
                var biome = QuestAndMapService.GetBiomeAt(hero.transform.position);
                _biomeText.text = $"{QuestAndMapService.GetBiomeDisplayName(biome).ToUpper()} · {map.GetExplorationPercent():F1}% EXPLORED";
                var q = map.GetTrackedQuest();
                _questText.text = q != null ? $"{q.Title}\n• {q.ObjectiveText} ({q.Progress}/{q.TargetCount})" : "All Story Quests Complete!";
                if (Time.time - _minimapRefreshAt > 0.1f)
                {
                    _minimapRefreshAt = Time.time;
                    RefreshMinimapTexture(map, hero.transform.position);
                }
            }

            var boss = EnemyAndBossAI.ActiveBoss;
            bool showBoss = boss != null && !boss.Dead && Vector3.Distance(hero.transform.position, boss.transform.position) < 42f;
            _bossPanel.SetActive(showBoss);
            if (showBoss)
            {
                _bossTitleText.text = $"MALAKOR · PHASE {boss.BossPhase}{(boss.IsStaggerBroken ? " [STAGGERED!]" : "")}";
                _bossHpFill.fillAmount = Mathf.Clamp01(boss.HP / Mathf.Max(1f, boss.MaxHP));
                _bossStagFill.fillAmount = Mathf.Clamp01(boss.StaggerGauge / Mathf.Max(1f, boss.MaxStagger));
            }

            bool riding = DragonController.Instance != null && DragonController.Instance.Mounted;
            _dragonPanel.SetActive(riding);
            if (riding) _dragonFireFill.fillAmount = Mathf.Clamp01(DragonController.Instance.FireEnergy / DragonController.Instance.MaxFireEnergy);
        }

        private void RefreshMinimapTexture(QuestAndMapService map, Vector3 heroPos)
        {
            if (_minimapTex == null) return;

            // A small window of the world around the hero, at deliberately low resolution: the big
            // painted chart only shows up when the player opens the map with [M].
            const int N = MinimapTexSize;
            const float HalfSpan = MinimapWorldSpan * 0.5f;
            if (_minimapPixels == null || _minimapPixels.Length != N * N) _minimapPixels = new Color[N * N];

            var basePixels = AetheriaMapTexture.BasePixels;
            int srcSize = AetheriaMapTexture.Size;
            float worldSpan = AetheriaWorld.WorldSize;
            float half = AetheriaWorld.HalfExtent;

            // North stays up: v grows toward +Z, which is north on the map.
            float u0 = (heroPos.x - HalfSpan + half) / worldSpan;
            float v0 = (heroPos.z - HalfSpan + half) / worldSpan;
            float step = (MinimapWorldSpan / worldSpan) / N;

            for (int y = 0; y < N; y++)
            {
                float v = v0 + (y + 0.5f) * step;
                int sy = Mathf.Clamp((int)(v * srcSize), 0, srcSize - 1);
                for (int x = 0; x < N; x++)
                {
                    float u = u0 + (x + 0.5f) * step;
                    if (u < 0f || u > 1f || v < 0f || v > 1f)
                    {
                        _minimapPixels[y * N + x] = new Color(0.03f, 0.035f, 0.05f, 1f);
                        continue;
                    }
                    int sx = Mathf.Clamp((int)(u * srcSize), 0, srcSize - 1);
                    Color c = basePixels[sy * srcSize + sx];
                    float wx = u * worldSpan - half, wz = v * worldSpan - half;
                    _minimapPixels[y * N + x] = map.IsExploredAt(wx, wz) ? c : AetheriaMapTexture.Fogged(c);
                }
            }

            // Hero arrow at the centre of the window
            int cx = N / 2, cy = N / 2;
            for (int y = -1; y <= 1; y++)
                for (int x = -1; x <= 1; x++)
                {
                    int px = Mathf.Clamp(cx + x, 0, N - 1), py = Mathf.Clamp(cy + y, 0, N - 1);
                    _minimapPixels[py * N + px] = AetheriaPalette.TextCyan;
                }

            _minimapTex.SetPixels(_minimapPixels);
            _minimapTex.Apply(false);
        }

        // ---- Static UI Factory Helpers ----
        public GameObject CreatePanel(string name, Transform parent, Vector2 amin, Vector2 amax, Vector2 omin, Vector2 omax, Color bg, bool goldBorder = false)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            var rt = go.AddComponent<RectTransform>();
            rt.anchorMin = amin; rt.anchorMax = amax;
            rt.offsetMin = omin; rt.offsetMax = omax;
            var img = go.AddComponent<Image>();
            img.color = bg;
            if (goldBorder)
            {
                var ol = go.AddComponent<Outline>();
                ol.effectColor = AetheriaPalette.HairlineGold;
                ol.effectDistance = new Vector2(1.4f, 1.4f);
            }
            return go;
        }

        public Text CreateText(string name, Transform parent, Vector2 amin, Vector2 amax, Vector2 omin, Vector2 omax, int size, Color col, Font font, TextAnchor align, string initial = "")
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            var rt = go.AddComponent<RectTransform>();
            rt.anchorMin = amin; rt.anchorMax = amax;
            rt.offsetMin = omin; rt.offsetMax = omax;
            var txt = go.AddComponent<Text>();
            txt.font = font;
            txt.fontSize = size;
            txt.color = col;
            txt.alignment = align;
            txt.text = initial;
            txt.horizontalOverflow = HorizontalWrapMode.Wrap;
            txt.verticalOverflow = VerticalWrapMode.Overflow;
            return txt;
        }

        public Text CreateText(string name, Transform parent, Vector2 offsetMin, Vector2 offsetMax, int size, Color col, Font font, TextAnchor align, string initial = "")
            => CreateText(name, parent, Vector2.zero, Vector2.one, offsetMin, offsetMax, size, col, font, align, initial);

        public Image CreateBar(string name, Transform parent, Vector2 offsetMin, Vector2 offsetMax, Color fillCol)
        {
            bool stretchRight = offsetMax.x <= 0f;
            var bg = CreatePanel(name + "_Bg", parent, Vector2.zero, new Vector2(stretchRight ? 1f : 0f, 0f), offsetMin, offsetMax, new Color(0.03f, 0.025f, 0.04f, 0.95f));
            var fillGo = new GameObject(name);
            fillGo.transform.SetParent(bg.transform, false);
            var frt = fillGo.AddComponent<RectTransform>();
            frt.anchorMin = Vector2.zero; frt.anchorMax = Vector2.one; frt.offsetMin = Vector2.zero; frt.offsetMax = Vector2.zero;
            var img = fillGo.AddComponent<Image>();
            img.color = fillCol;
            img.type = Image.Type.Filled;
            img.fillMethod = Image.FillMethod.Horizontal;
            img.fillAmount = 1f;
            return img;
        }

        public Button CreateButton(string name, Transform parent, string label, Color bg, Color txtCol, UnityEngine.Events.UnityAction onClick)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            var img = go.AddComponent<Image>();
            img.color = bg;
            var ol = go.AddComponent<Outline>();
            ol.effectColor = AetheriaPalette.HairlineGold;
            var btn = go.AddComponent<Button>();
            btn.onClick.AddListener(onClick);
            CreateText("Label", go.transform, Vector2.zero, Vector2.one, Vector2.zero, Vector2.zero, 15, txtCol, DisplayFont, TextAnchor.MiddleCenter, label);
            return btn;
        }
    }
}
