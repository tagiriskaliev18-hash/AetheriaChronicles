using System;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Renders crisp, zero-clutter dark-fantasy modals for Title Screen, Live 3D Hero Customizer ([C]),
    /// 64x64 Fog-of-War World Map ([M]), Skill Tree ([K]), Quests ([J]), Grimoire ([B]), NPC Dialogue, and Dev Console ([~]).
    /// </summary>
    public class AetheriaOverlays : MonoBehaviour
    {
        public static AetheriaOverlays Instance { get; private set; }

        private AetheriaNPC _dialogueNPC;
        private string _consoleInput = "";
        private string _consoleLog = "Aetheria Arcane Console ready. Type /help or /godmode";
        private Vector2 _scrollPos;
        private GUIStyle _panelStyle, _cardStyle, _titleStyle, _bodyStyle, _goldBtnStyle;
        private Texture2D _mapTexture;
        private Texture2D _dotTex;
        private Vector2 _mapScroll;
        private Texture2D[] _grimoirePages;
        private int _grimoireIdx = 0;

        // The panels are authored for a 1920x1080 canvas and scaled to whatever the game view is,
        // so a small Game view window and a 4K player show the same layout instead of oversized boxes.
        private const float RefWidth = 1920f;
        private const float RefHeight = 1080f;
        private static float UiScale => Mathf.Clamp(Mathf.Min(Screen.width / RefWidth, Screen.height / RefHeight), 0.4f, 3f);
        private static float Vw => Screen.width / UiScale;
        private static float Vh => Screen.height / UiScale;

        private void Awake()
        {
            Instance = this;
            _grimoirePages = Resources.LoadAll<Texture2D>("Grimoire");
        }

        public void SetDialogueTarget(AetheriaNPC npc) => _dialogueNPC = npc;

        public void OnOverlayChanged(OverlayMode mode)
        {
            if (mode == OverlayMode.WorldMap) RebuildWorldMapTexture();
        }

        private void EnsureStyles()
        {
            if (_panelStyle != null) return;
            Texture2D MakeTex(Color c) { var t = new Texture2D(1, 1); t.SetPixel(0, 0, c); t.Apply(); return t; }

            _panelStyle = new GUIStyle(GUI.skin.box) { normal = { background = MakeTex(AetheriaPalette.PanelBgDeep) } };
            _cardStyle = new GUIStyle(GUI.skin.box) { normal = { background = MakeTex(AetheriaPalette.CardBg) }, padding = new RectOffset(10, 10, 8, 8) };
            _titleStyle = new GUIStyle(GUI.skin.label) { fontSize = 24, fontStyle = FontStyle.Bold, normal = { textColor = AetheriaPalette.TextGold } };
            _bodyStyle = new GUIStyle(GUI.skin.label) { fontSize = 14, wordWrap = true, normal = { textColor = AetheriaPalette.TextPrimary } };
            _goldBtnStyle = new GUIStyle(GUI.skin.button) { fontSize = 15, fontStyle = FontStyle.Bold, normal = { textColor = AetheriaPalette.TextGold } };
        }

        private void OnGUI()
        {
            var hud = AetheriaHUDManager.Instance;
            if (hud == null || hud.ActiveOverlay == OverlayMode.None) return;
            EnsureStyles();

            float scale = UiScale;
            GUI.matrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(scale, scale, 1f));
            switch (hud.ActiveOverlay)
            {
                case OverlayMode.MainMenu:       DrawMainMenu(hud); break;
                case OverlayMode.HeroCustomizer: DrawHeroCustomizer(hud); break;
                case OverlayMode.WorldMap:       DrawWorldMap(hud); break;
                case OverlayMode.SkillTree:      DrawSkillTree(hud); break;
                case OverlayMode.QuestLog:       DrawQuestLog(hud); break;
                case OverlayMode.Grimoire:       DrawGrimoire(hud); break;
                case OverlayMode.Dialogue:       DrawDialogue(hud); break;
                case OverlayMode.DevConsole:     DrawDevConsole(hud); break;
                case OverlayMode.PauseMenu:      DrawPauseMenu(hud); break;
            }
        }

        private void DrawMainMenu(AetheriaHUDManager hud)
        {
            GUI.Box(new Rect(48, Vh * 0.16f, 440, 440), "", _panelStyle);
            GUILayout.BeginArea(new Rect(72, Vh * 0.19f, 392, 400));
            GUILayout.Label("AETHERIA", new GUIStyle(_titleStyle) { fontSize = 40 });
            GUILayout.Label("CHRONICLES OF THE DRAKE · UNITY 6 HDRP", _bodyStyle);
            GUILayout.Space(24);
            if (GUILayout.Button("NEW GAME (HERO CREATOR & PROLOGUE)", _goldBtnStyle, GUILayout.Height(46)))
            {
                HeroController.Instance.SetGodMode(false);
                hud.SetOverlay(OverlayMode.HeroCustomizer);
            }
            GUILayout.Space(10);
            if (SkillTreeAndSaveService.HasSaveFile() && GUILayout.Button("CONTINUE SAVED JOURNEY", _goldBtnStyle, GUILayout.Height(42)))
            {
                HeroController.Instance.SkipToOpenWorld(true);
                hud.SetOverlay(OverlayMode.None);
            }
            GUILayout.Space(10);
            if (GUILayout.Button("ENTER OAKHAVEN DIRECTLY", _goldBtnStyle, GUILayout.Height(42)))
            {
                HeroController.Instance.SkipToOpenWorld(false);
                hud.SetOverlay(OverlayMode.None);
            }
            GUILayout.Space(10);
            if (GUILayout.Button($"GRAPHICS: {AudioAndQualityService.Instance?.ActiveTier} [F10]", GUILayout.Height(36)))
                AudioAndQualityService.Instance?.ToggleRenderTier();
            GUILayout.Space(10);
            if (GUILayout.Button("EXIT TO DESKTOP", GUILayout.Height(36)))
                Application.Quit();
            GUILayout.EndArea();
        }

        private void DrawHeroCustomizer(AetheriaHUDManager hud)
        {
            var hero = HeroController.Instance;
            var cust = hero.Customization;
            var p = cust.Profile;

            // Left panel: Race, Stature (1.10m .. 3.00m), Appearance (center kept clean for 3D Hero Preview!)
            GUI.Box(new Rect(20, 20, 390, Vh - 40), "", _panelStyle);
            GUILayout.BeginArea(new Rect(34, 34, 362, Vh - 68));
            GUILayout.Label("HERO CREATOR STUDIO", _titleStyle);
            GUILayout.BeginHorizontal();
            if (GUILayout.Button($"GENDER: {p.Gender}", _goldBtnStyle, GUILayout.Height(32)))
            { p.Gender = p.Gender == HeroGender.Male ? HeroGender.Female : HeroGender.Male; cust.Recalculate(); }
            if (GUILayout.Button("ROTATE HERO", GUILayout.Height(32)))
                hero.transform.Rotate(Vector3.up, 35f);
            GUILayout.EndHorizontal();

            GUILayout.Space(6);
            GUILayout.Label($"RACE: {HeroCustomizationService.GetRaceLabel(p.Race)}", _titleStyle);
            if (GUILayout.Button("CYCLE PLAYABLE RACE"))
                cust.SetRace((HeroRace)(((int)p.Race + 1) % 6));
            GUILayout.Label(cust.Derived.RacePassiveSummary ?? "", _bodyStyle);

            GUILayout.Space(8);
            GUILayout.Label($"PHYSICAL STATURE: {cust.Derived.StatureSummary}", _bodyStyle);
            float nh = GUILayout.HorizontalSlider(p.HeightMeters, 1.10f, 3.00f);
            if (Mathf.Abs(nh - p.HeightMeters) > 0.005f) { p.HeightMeters = nh; cust.Recalculate(); }
            if (!cust.Derived.CanCastSpells)
                GUILayout.Label("STATURE < 1.45m: SPELLS LOCKED (AGILE MELEE)", new GUIStyle(_bodyStyle) { normal = { textColor = AetheriaPalette.TextCrimson } });

            GUILayout.Label($"MUSCULATURE: {(p.Musculature * 100):F0}%", _bodyStyle);
            float nm = GUILayout.HorizontalSlider(p.Musculature, 0f, 1f);
            if (Mathf.Abs(nm - p.Musculature) > 0.01f) { p.Musculature = nm; cust.Recalculate(); }

            GUILayout.Label($"SHOULDERS: {(p.ShoulderWidth * 100):F0}%", _bodyStyle);
            float ns = GUILayout.HorizontalSlider(p.ShoulderWidth, 0f, 1f);
            if (Mathf.Abs(ns - p.ShoulderWidth) > 0.01f) { p.ShoulderWidth = ns; cust.Recalculate(); }

            GUILayout.Space(6);
            if (GUILayout.Button($"HAIR: {p.HairStyle} ({HeroCustomizationService.HairColorNames[p.HairColorIndex]})"))
            { p.HairStyle = (HeroHairStyle)(((int)p.HairStyle + 1) % 8); p.HairColorIndex = (p.HairColorIndex + 1) % 8; cust.Recalculate(); }
            if (GUILayout.Button($"BEARD: {p.BeardStyle} · MUSTACHE: {p.MustacheStyle}"))
            { p.BeardStyle = (HeroBeardStyle)(((int)p.BeardStyle + 1) % 7); p.MustacheStyle = (HeroMustacheStyle)(((int)p.MustacheStyle + 1) % 6); cust.Recalculate(); }
            if (GUILayout.Button($"EYES: {p.EyeStyle} · TATTOO: {p.TattooStyle}"))
            { p.EyeStyle = (HeroEyeStyle)(((int)p.EyeStyle + 1) % 8); p.TattooStyle = (HeroTattooStyle)(((int)p.TattooStyle + 1) % 7); cust.Recalculate(); }
            if (GUILayout.Button($"OUTFIT: {p.OutfitStyle} · QUALITY: {p.DetailQuality}"))
            { p.OutfitStyle = (HeroOutfitStyle)(((int)p.OutfitStyle + 1) % 6); p.DetailQuality = (HeroDetailQuality)(((int)p.DetailQuality + 1) % 5); cust.Recalculate(); }
            GUILayout.EndArea();

            // Right panel: 6 RPG Attributes + Live Stats + Awaken Button
            GUI.Box(new Rect(Vw - 410, 20, 390, Vh - 40), "", _panelStyle);
            GUILayout.BeginArea(new Rect(Vw - 396, 34, 362, Vh - 68));
            GUILayout.Label($"ATTRIBUTES (PTS: {p.Attributes.UnspentPoints})", _titleStyle);
            foreach (HeroAttribute attr in Enum.GetValues(typeof(HeroAttribute)))
            {
                GUILayout.BeginHorizontal(_cardStyle);
                GUILayout.Label($"{attr}: {cust.GetBaseAttr(attr)}", _bodyStyle, GUILayout.Width(145));
                if (GUILayout.Button("-", GUILayout.Width(36))) cust.ModifyAttribute(attr, -1);
                if (GUILayout.Button("+", GUILayout.Width(36))) cust.ModifyAttribute(attr, 1);
                GUILayout.EndHorizontal();
                GUILayout.Label(HeroCustomizationService.GetAttributeHint(attr), new GUIStyle(_bodyStyle) { fontSize = 11, normal = { textColor = AetheriaPalette.TextSecondary } });
            }

            GUILayout.Space(10);
            var d = cust.Derived;
            GUILayout.Label($"HP {d.MaxHP:F0} · MANA {d.MaxMana:F0} · STAM {d.MaxStamina:F0}\nMelee x{d.MeleeDamageMultiplier:F2} · Spell x{d.SpellPowerMultiplier:F2} · Crit {d.CritChance * 100:F0}%\nMove {d.MoveSpeedMetersPerSec:F1}m/s · Turn {d.TurnRateDegPerSec:F0}°/s", _bodyStyle);
            GUILayout.FlexibleSpace();
            if (GUILayout.Button(hero.HasEnteredWorld ? "APPLY & RESUME ADVENTURE [C]" : "AWAKEN IN AETHERIA (PROLOGUE)", _goldBtnStyle, GUILayout.Height(48)))
            {
                hero.SkillTree.SaveProgress(hero);
                if (!hero.HasEnteredWorld) hero.StartPrologueAwakening();
                hud.SetOverlay(OverlayMode.None);
            }
            GUILayout.EndArea();
        }

        private void RebuildWorldMapTexture()
        {
            var map = QuestAndMapService.Instance;
            if (map == null) return;

            const int N = AetheriaMapTexture.Size;
            if (_mapTexture == null)
                _mapTexture = new Texture2D(N, N, TextureFormat.RGBA32, false) { filterMode = FilterMode.Bilinear, wrapMode = TextureWrapMode.Clamp };

            var basePixels = AetheriaMapTexture.BasePixels;
            var cols = new Color[N * N];
            float half = AetheriaWorld.HalfExtent, size = AetheriaWorld.WorldSize;
            for (int y = 0; y < N; y++)
            {
                float wz = -half + (y + 0.5f) / N * size;
                for (int x = 0; x < N; x++)
                {
                    float wx = -half + (x + 0.5f) / N * size;
                    Color c = basePixels[y * N + x];
                    cols[y * N + x] = map.IsExploredAt(wx, wz) ? c : AetheriaMapTexture.Fogged(c);
                }
            }
            _mapTexture.SetPixels(cols);
            _mapTexture.Apply(false);
        }

        /// <summary>World point to a rect-local position on the drawn map.</summary>
        private static Vector2 WorldToMapPoint(Vector2 world, Rect mapRect)
        {
            Vector2 uv = AetheriaWorld.WorldToUV(world.x, world.y);
            return new Vector2(mapRect.x + uv.x * mapRect.width, mapRect.y + (1f - uv.y) * mapRect.height);
        }

        private void DrawDot(Vector2 center, float radius, Color color)
        {
            if (_dotTex == null)
            {
                _dotTex = new Texture2D(4, 4, TextureFormat.RGBA32, false);
                var px = new Color[16];
                for (int i = 0; i < 16; i++) px[i] = Color.white;
                _dotTex.SetPixels(px);
                _dotTex.Apply();
            }
            var prev = GUI.color;
            GUI.color = color;
            GUI.DrawTexture(new Rect(center.x - radius, center.y - radius, radius * 2f, radius * 2f), _dotTex);
            GUI.color = prev;
        }

        private void DrawWorldMap(AetheriaHUDManager hud)
        {
            var map = QuestAndMapService.Instance;
            if (map == null) return;
            var hero = HeroController.Instance;
            bool god = hero != null && hero.GodMode;

            float sz = Mathf.Min(Vh - 120f, Mathf.Min(Vw - 420f, 760f));
            Rect panel = new Rect((Vw - sz - 300f) * 0.5f, 48, sz + 300, sz + 44);
            GUI.Box(panel, "", _panelStyle);
            Rect mapRect = new Rect(panel.x + 22, panel.y + 22, sz, sz);

            if (_mapTexture != null) GUI.DrawTexture(mapRect, _mapTexture, ScaleMode.StretchToFill, false);
            GUI.Box(mapRect, "", _cardStyle);   // cartographic frame

            foreach (var pin in map.MapPins)
            {
                if (!pin.Discovered && !god) continue;
                Vector2 p = WorldToMapPoint(new Vector2(pin.WorldLocation.x, pin.WorldLocation.z), mapRect);
                DrawDot(p, pin.FastTravel ? 5.5f : 4f, new Color(0f, 0f, 0f, 0.6f));
                DrawDot(p, pin.FastTravel ? 4f : 3f, pin.Color);
            }

            if (hero != null)
            {
                Vector2 hp = WorldToMapPoint(new Vector2(hero.transform.position.x, hero.transform.position.z), mapRect);
                DrawDot(hp, 6.5f, new Color(0f, 0f, 0f, 0.65f));
                DrawDot(hp, 4.5f, AetheriaPalette.TextCyan);
            }

            GUILayout.BeginArea(new Rect(mapRect.xMax + 18, panel.y + 22, 258, sz));
            GUILayout.Label($"WORLD MAP · {map.GetExplorationPercent():F0}%", _titleStyle);
            GUILayout.Label($"Lands charted: {map.MapPins.FindAll(p => p.Discovered).Count}/{map.MapPins.Count}", _bodyStyle);
            GUILayout.Space(6);
            _mapScroll = GUILayout.BeginScrollView(_mapScroll);
            foreach (var pin in map.MapPins)
            {
                if (!pin.Discovered && !god) continue;
                GUILayout.BeginHorizontal(_cardStyle);
                GUILayout.Label(pin.Label, _bodyStyle);
                if (pin.FastTravel && GUILayout.Button("GO", GUILayout.Width(52)))
                {
                    hero?.TeleportSafe(pin.WorldLocation);
                    hud.SetOverlay(OverlayMode.None);
                }
                GUILayout.EndHorizontal();
            }
            GUILayout.EndScrollView();
            if (GUILayout.Button("CLOSE [M / ESC]", _goldBtnStyle, GUILayout.Height(38))) hud.SetOverlay(OverlayMode.None);
            GUILayout.EndArea();
        }

        private void DrawSkillTree(AetheriaHUDManager hud)
        {
            var st = HeroController.Instance.SkillTree;
            Rect r = new Rect(Vw * 0.1f, 60, Vw * 0.8f, Vh - 120);
            GUI.Box(r, "", _panelStyle);
            GUILayout.BeginArea(new Rect(r.x + 24, r.y + 20, r.width - 48, r.height - 40));
            GUILayout.Label($"SKILL TREE · LVL {st.Level} · UNSPENT SKILL POINTS: {st.UnspentSkillPoints}", _titleStyle);
            _scrollPos = GUILayout.BeginScrollView(_scrollPos);
            GUILayout.BeginHorizontal();
            foreach (SkillBranch br in Enum.GetValues(typeof(SkillBranch)))
            {
                GUILayout.BeginVertical(_cardStyle, GUILayout.Width((r.width - 80) * 0.24f));
                GUILayout.Label(br.ToString().ToUpper(), _titleStyle);
                foreach (var n in st.Nodes.FindAll(x => x.Branch == br))
                {
                    GUILayout.BeginVertical(_panelStyle);
                    GUILayout.Label($"T{n.Tier}: {n.Title} ({n.CurrentRank}/{n.MaxRank})", _bodyStyle);
                    GUILayout.Label(n.Description, new GUIStyle(_bodyStyle) { fontSize = 12, normal = { textColor = AetheriaPalette.TextSecondary } });
                    GUI.enabled = st.CanUpgrade(n.NodeId);
                    if (GUILayout.Button(GUI.enabled ? "UPGRADE (+1)" : (n.CurrentRank >= n.MaxRank ? "MAXED" : "LOCKED")))
                        st.UpgradeNode(n.NodeId);
                    GUI.enabled = true;
                    GUILayout.EndVertical();
                    GUILayout.Space(4);
                }
                GUILayout.EndVertical();
            }
            GUILayout.EndHorizontal();
            GUILayout.EndScrollView();
            if (GUILayout.Button("CLOSE [K / ESC]", _goldBtnStyle, GUILayout.Height(38))) hud.SetOverlay(OverlayMode.None);
            GUILayout.EndArea();
        }

        private void DrawQuestLog(AetheriaHUDManager hud)
        {
            var map = QuestAndMapService.Instance;
            Rect r = new Rect(Vw * 0.2f, 80, Vw * 0.6f, Vh - 160);
            GUI.Box(r, "", _panelStyle);
            GUILayout.BeginArea(new Rect(r.x + 24, r.y + 20, r.width - 48, r.height - 40));
            GUILayout.Label("AETHERIA STORY CHRONICLES", _titleStyle);
            _scrollPos = GUILayout.BeginScrollView(_scrollPos);
            foreach (var q in map.Quests)
            {
                GUILayout.BeginVertical(_cardStyle);
                GUILayout.Label($"{q.Title} [{q.State}] ({q.Progress}/{q.TargetCount}) · {q.GiverName}", _titleStyle);
                GUILayout.Label(q.Summary, _bodyStyle);
                if (GUILayout.Button("TRACK ON HUD", GUILayout.Width(160))) { map.TrackedQuestId = q.QuestId; }
                GUILayout.EndVertical();
                GUILayout.Space(6);
            }
            GUILayout.EndScrollView();
            if (GUILayout.Button("CLOSE [J / ESC]", _goldBtnStyle, GUILayout.Height(38))) hud.SetOverlay(OverlayMode.None);
            GUILayout.EndArea();
        }

        private void DrawGrimoire(AetheriaHUDManager hud)
        {
            var sp = HeroController.Instance.Spells;
            Rect r = new Rect(Vw * 0.14f, 60, Vw * 0.72f, Vh - 120);
            GUI.Box(r, "", _panelStyle);
            GUILayout.BeginArea(new Rect(r.x + 24, r.y + 20, r.width - 48, r.height - 40));
            GUILayout.Label($"ARCANE INVOKER GRIMOIRE · UNLOCKED SPELLS: {sp.GetUnlockedSpellCount()}/10", _titleStyle);
            GUILayout.BeginHorizontal();
            if (_grimoirePages != null && _grimoirePages.Length > 0)
            {
                GUILayout.BeginVertical(GUILayout.Width(320));
                GUILayout.Box(_grimoirePages[_grimoireIdx % _grimoirePages.Length], GUILayout.Width(310), GUILayout.Height(380));
                if (GUILayout.Button("NEXT ILLUSTRATED PAGE")) _grimoireIdx++;
                GUILayout.EndVertical();
            }
            _scrollPos = GUILayout.BeginScrollView(_scrollPos);
            foreach (var d in SpellSystem.GetAllSpellDefs())
            {
                bool unl = sp.IsSpellUnlocked(d.Id);
                GUILayout.BeginVertical(_cardStyle);
                GUILayout.Label($"[{d.ComboKey}] {d.DisplayName} {(unl ? "· UNLOCKED" : "· LOCKED")}", _bodyStyle);
                GUILayout.Label($"{d.Description} (Mana {d.ManaCost:F0} · Dmg {d.BaseDamage:F0})", new GUIStyle(_bodyStyle) { fontSize = 12, normal = { textColor = AetheriaPalette.TextSecondary } });
                GUILayout.EndVertical();
                GUILayout.Space(4);
            }
            GUILayout.EndScrollView();
            GUILayout.EndHorizontal();
            if (GUILayout.Button("CLOSE [B / ESC]", _goldBtnStyle, GUILayout.Height(38))) hud.SetOverlay(OverlayMode.None);
            GUILayout.EndArea();
        }

        private void DrawDialogue(AetheriaHUDManager hud)
        {
            if (_dialogueNPC == null) { hud.SetOverlay(OverlayMode.None); return; }
            var map = QuestAndMapService.Instance;
            var q = map.FindQuestForGiver(_dialogueNPC.NpcId);
            Rect r = new Rect(Vw * 0.22f, Vh * 0.25f, Vw * 0.56f, 360);
            GUI.Box(r, "", _panelStyle);
            GUILayout.BeginArea(new Rect(r.x + 24, r.y + 20, r.width - 48, r.height - 40));
            GUILayout.Label($"{_dialogueNPC.DisplayName} — {_dialogueNPC.RoleTitle}", _titleStyle);
            GUILayout.Space(10);
            if (q == null) GUILayout.Label("May the Sunstone guard your steps across Aetheria.", _bodyStyle);
            else
            {
                string line = q.State == QuestState.ReadyToTurnIn ? q.TurnInDialogue : q.OfferDialogue;
                GUILayout.Label(line, _bodyStyle);
                GUILayout.FlexibleSpace();
                if (q.State == QuestState.Available && GUILayout.Button($"ACCEPT QUEST: {q.Title}", _goldBtnStyle, GUILayout.Height(40)))
                { map.AcceptQuest(q.QuestId); hud.SetOverlay(OverlayMode.None); }
                else if (q.State == QuestState.ReadyToTurnIn && GUILayout.Button($"COMPLETE & CLAIM REWARD (+{q.RewardGold}G / +{q.RewardXP}XP)", _goldBtnStyle, GUILayout.Height(40)))
                { map.TurnInQuest(q.QuestId, HeroController.Instance); hud.SetOverlay(OverlayMode.None); }
            }
            GUILayout.Space(6);
            if (GUILayout.Button("GOODBYE [ESC]", GUILayout.Height(34))) hud.SetOverlay(OverlayMode.None);
            GUILayout.EndArea();
        }

        private void DrawDevConsole(AetheriaHUDManager hud)
        {
            Rect r = new Rect(20, 20, Vw - 40, 260);
            GUI.Box(r, "", _panelStyle);
            GUILayout.BeginArea(new Rect(34, 30, r.width - 28, r.height - 20));
            GUILayout.Label("ARCANE DEV CONSOLE (/godmode · /restore · /heal · /gold 500 · /time · /help)", _titleStyle);
            GUILayout.Label(_consoleLog, _bodyStyle, GUILayout.Height(140));
            GUILayout.BeginHorizontal();
            GUI.SetNextControlName("ConsoleField");
            _consoleInput = GUILayout.TextField(_consoleInput, GUILayout.Height(32));
            GUI.FocusControl("ConsoleField");
            if (GUILayout.Button("RUN", _goldBtnStyle, GUILayout.Width(100), GUILayout.Height(32)) ||
                (Event.current.type == EventType.KeyDown && Event.current.keyCode == KeyCode.Return))
            {
                ExecConsole(_consoleInput);
                _consoleInput = "";
            }
            if (GUILayout.Button("CLOSE", GUILayout.Width(90), GUILayout.Height(32))) hud.SetOverlay(OverlayMode.None);
            GUILayout.EndHorizontal();
            GUILayout.EndArea();
        }

        public void ExecConsole(string raw)
        {
            string cmd = (raw ?? "").Trim().TrimStart('`', '~', 'ё', 'Ё').Trim().ToLowerInvariant();
            if (string.IsNullOrEmpty(cmd)) return;
            var hero = HeroController.Instance;
            if (cmd.StartsWith("/godmode") || cmd == "godmode" || cmd == "/god")
            {
                bool next = !(hero != null && hero.GodMode);
                hero?.SetGodMode(next);
                _consoleLog = next ? "[GODMODE ON] Immortality + All 10/10 Invoker Spells + Dragon Unlocked!" : "[GODMODE OFF]";
            }
            else if (cmd.StartsWith("/restore") || cmd == "restore")
            {
                int n = AetheriaWorldGenerator.Instance != null ? AetheriaWorldGenerator.Instance.RestoreAllScarsAndPropsNow() : 0;
                _consoleLog = $"[RESTORE] Magically restored {n} terrain scars & destructibles!";
            }
            else if (cmd.StartsWith("/heal"))
            {
                if (hero != null) { hero.HP = hero.MaxHP; hero.Stamina = hero.MaxStamina; hero.Spells.Mana = hero.Spells.MaxMana; }
                _consoleLog = "[HEAL] Full HP, Mana & Stamina restored.";
            }
            else if (cmd.StartsWith("/gold"))
            {
                hero?.SkillTree?.AddGold(500);
                _consoleLog = "[GOLD] +500 Gold added.";
            }
            else if (cmd.StartsWith("/time"))
            {
                AetheriaWorldGenerator.Instance?.CycleTimeOfDayPreset();
                _consoleLog = $"[TIME] Sky -> {AetheriaWorldGenerator.Instance?.GetTimeOfDayLabel()}";
            }
            else
            {
                _consoleLog = "Commands: /godmode, /restore, /heal, /gold, /time, /help";
            }
        }

        private void DrawPauseMenu(AetheriaHUDManager hud)
        {
            Rect r = new Rect(Vw * 0.5f - 190, Vh * 0.3f, 380, 290);
            GUI.Box(r, "", _panelStyle);
            GUILayout.BeginArea(new Rect(r.x + 24, r.y + 20, r.width - 48, r.height - 40));
            GUILayout.Label("PAUSED", _titleStyle);
            if (GUILayout.Button("RESUME ADVENTURE", _goldBtnStyle, GUILayout.Height(40))) hud.SetOverlay(OverlayMode.None);
            GUILayout.Space(8);
            if (GUILayout.Button("SAVE PROGRESS NOW", _goldBtnStyle, GUILayout.Height(36)))
            { HeroController.Instance?.SkillTree?.SaveProgress(HeroController.Instance); QuestAndMapService.Instance?.PushNotification("Progress Saved!"); }
            GUILayout.Space(8);
            if (GUILayout.Button("RETURN TO TITLE SCREEN", GUILayout.Height(36))) hud.SetOverlay(OverlayMode.MainMenu);
            GUILayout.Space(8);
            if (GUILayout.Button("QUIT GAME", GUILayout.Height(36))) Application.Quit();
            GUILayout.EndArea();
        }
    }
}
