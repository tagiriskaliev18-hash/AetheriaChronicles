using System;
using System.Collections.Generic;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Story & side quests, the fog-of-war exploration grid, landmark pins, shrine kindling and fast travel.
    /// All positions come from <see cref="AetheriaWorld"/> so quests, map and terrain always agree.
    /// </summary>
    public class QuestAndMapService : MonoBehaviour
    {
        public static QuestAndMapService Instance { get; private set; }

        public const int GridSize = 128;
        public const int TotalCells = GridSize * GridSize;
        public const float CellSizeMeters = AetheriaWorld.WorldSize / GridSize;

        public List<QuestDef> Quests = new List<QuestDef>();
        public List<MapPinInfo> MapPins = new List<MapPinInfo>();
        public string TrackedQuestId = "Q_PrologueAwakening";
        public byte[] FogGrid { get; private set; } = new byte[TotalCells];
        public readonly HashSet<string> KindledShrines = new HashSet<string>();

        private readonly Queue<string> _notifications = new Queue<string>();
        private BiomeId[] _cellBiomeCache;
        private readonly int[] _biomeTotal = new int[AetheriaWorld.BiomeCount];
        private readonly int[] _biomeExplored = new int[AetheriaWorld.BiomeCount];
        public int TotalLandCells { get; private set; }
        public int TotalExploredCells { get; private set; }
        public int LandExploredCells { get; private set; }
        public int LandmarkCount => MapPins.Count;

        public event Action OnQuestOrMapUpdated;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            AetheriaWorld.ReadSceneLandmarks();
            PopulateStoryQuests();
            SeedMapPins();
            BuildBiomeCache();
            KindledShrines.Add("Shrine_Oakhaven");
            RevealAround(SitePos("OakhavenVillage", 0f, 0f), AetheriaWorld.UsesCourtyardLayout ? 55f : 95f, suppressToast: true);
        }

        private void PopulateStoryQuests()
        {
            Quests.Clear();

            // ---------------- Main story (Act I..III, spoken by named NPCs) ----------------
            Quests.Add(new QuestDef
            {
                QuestId = "Q_PrologueAwakening", GiverId = "Npc_CaveTutor", GiverName = "Хранитель Вейлен",
                Title = "Пробуждение в Этерии",
                Summary = "Очнитесь в факельной пещере, выслушайте Хранителя Вейлена, выйдите на утёс над Зеркальным озером.",
                ObjectiveText = "Выйдите на край утёса над Зеркальным озером",
                OfferDialogue = "Добро пожаловать в Этерию, Пробуждённый! Лишь Ткач Первосфер — Exort [Z], Wex [X], Quas [Q], призыв [R] — связанный с драконом Игнисом, остановит Малакора.\n\n[LMB] — удары клинком, [RMB] — блок и парирование, [Space] — прыжок, [Space+Ctrl] или [V] — перекат, [C] — редактор героя, [M] — карта мира.\n\nСтупай на край утёса — и вниз, к водопаду!",
                TurnInDialogue = "Долина открыта! Иди через Каменный мост к старейшине Олдрику в Оукхейвен.",
                TargetCount = 1, RewardGold = 75, RewardXP = 100, UnlockOrbReward = OrbType.None,
                ObjectiveWorldLocation = SitePos("PrologueCliff", AetheriaWorld.UsesCourtyardLayout ? 0f : 20f, 0f)
            });

            Quests.Add(new QuestDef
            {
                QuestId = "Q_AwakenExort", GiverId = "ElderAldric", GiverName = "Elder Aldric",
                Title = "Пепел Очага",
                Summary = "Зажгите оба Рунических Маяка за площадью Оукхейвена, чтобы пробудить сферу Exort.",
                ObjectiveText = "Зажгите 2 Рунических Маяка Оукхейвена",
                OfferDialogue = "Очаг Оукхейвена гаснет. Ударьте клинком по обоим маякам за площадью — их искры пробудят Exort.",
                TurnInDialogue = "Золотое пламя снова отвечает! Призывайте EEE [Z-Z-Z → R], чтобы обрушить Солнечную Катастрофу.",
                TargetCount = 2, RewardGold = 120, RewardXP = 200, UnlockOrbReward = OrbType.Exort,
                ObjectiveWorldLocation = AetheriaWorld.UsesCourtyardLayout
                    ? ScenePosition("OakhavenBeacon_East", "OakhavenVillage") : SitePos("OakhavenVillage", -9f, -30f)
            });

            Quests.Add(new QuestDef
            {
                QuestId = "Q_TameDragon", GiverId = "ScholarLyra", GiverName = "Scholar Lyra",
                Title = "Укрощение Алого Крыла",
                Summary = "Игнис закован на утёсе Драконьего Гнезда. Разбейте 2 рунических кристалала Солнечной Катастрофой, чтобы получить полёт [F].",
                ObjectiveText = "Разбейте 2 кристалла у Драконьего Гнезда (EEE)",
                OfferDialogue = "Игнис рвётся с пут Малакора на западном утёсе. Разбей оба кристалла-окова Солнечной Катастрофой — и Алое Крыло станет твоим.",
                TurnInDialogue = "Цепи обратились в пепел! Подойди к Игнису и нажми [F], чтобы взлететь.",
                TargetCount = 2, RewardGold = 180, RewardXP = 260, UnlocksDragonMount = true,
                ObjectiveWorldLocation = SitePos("DragonAerie", 0f, 0f)
            });

            Quests.Add(new QuestDef
            {
                QuestId = "Q_StormSpires", GiverId = "BlacksmithTorin", GiverName = "Blacksmith Torin",
                Title = "Шпили Бури",
                Summary = "Настройте 3 Грозовых Проводника в Сапфировом Лесу и одолейте Грозового Охотника, чтобы освоить Wex.",
                ObjectiveText = "Настройте 3 Грозовых Проводника в Сапфировом Лесу",
                OfferDialogue = "Дикая молния рвёт восточный Сапфировый лес. Настрой три Грозовых Проводника и срази Грозового Охотника — буря станет твоей!",
                TurnInDialogue = "Гром поёт под твоей рукой! Соединяй Wex с Exort: Хаос-метеор (ZZX), Штормовой Порыв (ZXX), Плазменный ЭМИ (XXX).",
                TargetCount = 3, RewardGold = 240, RewardXP = 340, UnlockOrbReward = OrbType.Wex,
                ObjectiveWorldLocation = SitePos("SapphireWoods", 0f, 0f)
            });

            Quests.Add(new QuestDef
            {
                QuestId = "Q_EmberCrypt", GiverId = "WardenKael", GiverName = "Warden Kael",
                Title = "Угли Глубин",
                Summary = "Спуститесь в Угольный Склеп Пепельной Кальдеры, расколите обсидиановую печать Хаос-метеором и заберите Солнечный Камень.",
                ObjectiveText = "Расколите Обсидиановую Печать в Угольном Склепе (ZZX)",
                OfferDialogue = "Ни одна сталь не оставляет царапин на обсидиане Склепа. Обрушь Хаос-метеор [Z Z X → R] в лавовом зале и забери Солнечный Камень.",
                TurnInDialogue = "Солнечный Камень сияет чисто! Его тепло защитит тебя и на леднике, и у стен Хайкипа.",
                TargetCount = 1, RewardGold = 300, RewardXP = 420,
                ObjectiveWorldLocation = SitePos("EmberCrypt", 0f, 0f)
            });

            Quests.Add(new QuestDef
            {
                QuestId = "Q_GlacierSilence", GiverId = "SeerVaelis", GiverName = "Seer Vaelis",
                Title = "Тишина Хрустального Ледника",
                Summary = "Поднимитесь на Хрустальный Ледник мимо костров и одолейте Морозного Стража, чтобы открыть Quas и все 10 заклинаний.",
                ObjectiveText = "Одолейте Морозного Стража на Хрустальном Леднике",
                OfferDialogue = "Иди по горящим кострам ледника к ледяной арке. Одолей Морозного Стража — и третья сфера, Quas, завершит Троицу.",
                TurnInDialogue = "Exort, Wex и Quas вращаются в полном созвучии! Все 10 древних призывов у тебя под рукой.",
                TargetCount = 1, RewardGold = 360, RewardXP = 520, UnlockOrbReward = OrbType.Quas,
                ObjectiveWorldLocation = SitePos("CrystalGlacier", 0f, 0f)
            });

            Quests.Add(new QuestDef
            {
                QuestId = "Q_FallOfSovereign", GiverId = "CaptainRowena", GiverName = "Captain Rowena",
                Title = "Падение Повелителя Печати",
                Summary = "Штурмуйте Цитадель Хайкип на северном плато и низвергните Малакора, Повелителя Расколотой Печати.",
                ObjectiveText = "Одолейте Малакора в Цитадели Хайкип",
                OfferDialogue = "Врата Хайкипа проломлены, маг. Поднимись в обсидиановую арену и срази Малакора — парируй Янтарные удары [RMB], уклоняйся от Алых [Space]!",
                TurnInDialogue = "Малакор пал, и Расколотая Печать сияет целой! Вся Этерия теперь под твоим крылом.",
                TargetCount = 1, RewardGold = 650, RewardXP = 900,
                ObjectiveWorldLocation = SitePos("HighkeepCitadel", 0f, 0f)
            });

            if (AetheriaWorld.UsesCourtyardLayout) return;
            // ---------------- Side quests: discovered out in the world, completed on the spot ----------------
            Side("Q_ShrinePilgrim", "Паломник Этерии", "Зажгите очаги святилищ по всем землям — каждое открывает быстрое перемещение с карты [M].",
                "Зажгите святилища Этерии", 11, 420, 900, SitePos("Shrine_Lake", 0f, 0f));
            Side("Q_GroveWarden", "Хранитель Мерцающей Рощи", "Светящийся круг Грибной рощи осквернён. Одолейте Хранителя Рощи.",
                "Одолейте Хранителя Мерцающей Рощи", 1, 240, 380, SitePos("GlimmercapGrove", 0f, 0f));
            Side("Q_HollowRevenant", "Призрак Ноктюрна", "В проклятой деревне не гаснут фонари. Упокойте Призрака Ноктюрна.",
                "Упокойте Призрака Ноктюрна", 1, 260, 420, SitePos("NocturneHollow", 0f, 0f));
            Side("Q_WastesColossus", "Колосс Солнечного Гибла", "Среди столовых гор пустоши бродит Дюнный Колосс. Свергните его.",
                "Свергните Дюнного Колосса", 1, 300, 520, SitePos("SunblightWastes", 0f, 0f));
            Side("Q_FenLurker", "Болотный Умысел", "Из тумана Топи Мёркфен доносится шёпот. Одолейте Болотного Ловчего.",
                "Одолейте Болотного Ловчего", 1, 280, 460, SitePos("MurkfenMarsh", 0f, 0f));
        }

        private void Side(string id, string title, string summary, string objective, int target, int gold, int xp, Vector3 at)
        {
            Quests.Add(new QuestDef
            {
                QuestId = id, GiverId = "World", GiverName = "Этерия", Title = title, Summary = summary,
                ObjectiveText = objective, OfferDialogue = "", TurnInDialogue = "",
                TargetCount = target, RewardGold = gold, RewardXP = xp, ObjectiveWorldLocation = at
            });
        }

        private static Vector3 SitePos(string siteId, float dx, float dz)
        {
            var s = AetheriaWorld.FindSite(siteId);
            return s != null ? s.Position + new Vector3(dx, 0f, dz) : new Vector3(dx, 0f, dz);
        }

        private static Vector3 ScenePosition(string objectName, string siteId)
        {
            var obj = GameObject.Find(objectName);
            return obj != null ? obj.transform.position : SitePos(siteId, 0f, 0f);
        }

        private void SeedMapPins()
        {
            MapPins.Clear();
            foreach (var s in AetheriaWorld.Sites)
            {
                MapPins.Add(new MapPinInfo
                {
                    PinId = s.Id, Label = s.Label, WorldLocation = s.Position, Biome = s.Biome,
                    Color = PinColor(s.Kind), Discovered = s.StartDiscovered, FastTravel = s.Id == "Shrine_Oakhaven"
                });
            }
        }

        private static Color PinColor(SiteKind kind) => kind switch
        {
            SiteKind.Village => new Color(1.00f, 0.82f, 0.34f),
            SiteKind.Shrine  => new Color(0.30f, 0.98f, 0.92f),
            SiteKind.Lake    => new Color(0.42f, 0.80f, 1.00f),
            SiteKind.Cliff   => new Color(0.80f, 0.80f, 0.86f),
            SiteKind.Aerie   => new Color(1.00f, 0.40f, 0.16f),
            SiteKind.Farm    => new Color(0.96f, 0.86f, 0.30f),
            SiteKind.Forest  => new Color(0.34f, 0.92f, 0.74f),
            SiteKind.Citadel => new Color(0.96f, 0.20f, 0.24f),
            SiteKind.Caldera => new Color(1.00f, 0.50f, 0.12f),
            SiteKind.Glacier => new Color(0.74f, 0.94f, 1.00f),
            SiteKind.Grove   => new Color(0.80f, 0.56f, 1.00f),
            SiteKind.Hamlet  => new Color(0.66f, 0.50f, 0.90f),
            SiteKind.Wastes  => new Color(0.98f, 0.76f, 0.42f),
            _                => new Color(0.56f, 0.80f, 0.40f)
        };

        private void BuildBiomeCache()
        {
            _cellBiomeCache = _cellBiomeCache ?? new BiomeId[TotalCells];
            Array.Clear(_biomeTotal, 0, _biomeTotal.Length);
            TotalLandCells = 0;
            for (int y = 0; y < GridSize; y++)
            {
                for (int x = 0; x < GridSize; x++)
                {
                    Vector2 w = CellCenter(x, y);
                    var b = AetheriaWorld.BiomeAt(w.x, w.y);
                    _cellBiomeCache[y * GridSize + x] = b;
                    _biomeTotal[(int)b]++;
                    if (b != BiomeId.AetherianSea) TotalLandCells++;
                }
            }
        }

        public static BiomeId GetBiomeAt(Vector3 pos) => AetheriaWorld.BiomeAt(pos);
        public static string GetBiomeDisplayName(BiomeId b) => AetheriaWorld.BiomeDisplayName(b);

        public void RevealAround(Vector3 worldPos, float radiusMeters = 70f, bool suppressToast = false)
        {
            float rSq = radiusMeters * radiusMeters;
            Vector2Int minC = WorldToCell(worldPos - new Vector3(radiusMeters, 0f, radiusMeters));
            Vector2Int maxC = WorldToCell(worldPos + new Vector3(radiusMeters, 0f, radiusMeters));

            for (int y = minC.y; y <= maxC.y; y++)
            {
                for (int x = minC.x; x <= maxC.x; x++)
                {
                    int idx = y * GridSize + x;
                    if (FogGrid[idx] != 0) continue;
                    Vector2 c = CellCenter(x, y);
                    float dx = c.x - worldPos.x, dz = c.y - worldPos.z;
                    if (dx * dx + dz * dz <= rSq)
                    {
                        FogGrid[idx] = 1;
                        TotalExploredCells++;
                        var b = _cellBiomeCache[idx];
                        _biomeExplored[(int)b]++;
                        if (b != BiomeId.AetherianSea) LandExploredCells++;
                    }
                }
            }

            foreach (var pin in MapPins)
            {
                if (pin.Discovered) continue;
                float dx = worldPos.x - pin.WorldLocation.x;
                float dz = worldPos.z - pin.WorldLocation.z;
                float discover = radiusMeters + 30f;
                if (dx * dx + dz * dz <= discover * discover)
                {
                    pin.Discovered = true;
                    if (!suppressToast) PushNotification($"ОТКРЫТО: {pin.Label}");
                }
            }
        }

        public void RevealAll()
        {
            for (int i = 0; i < TotalCells; i++)
            {
                if (FogGrid[i] == 0)
                {
                    FogGrid[i] = 1;
                    TotalExploredCells++;
                    var b = _cellBiomeCache[i];
                    _biomeExplored[(int)b]++;
                    if (b != BiomeId.AetherianSea) LandExploredCells++;
                }
            }
            foreach (var pin in MapPins) pin.Discovered = true;
            OnQuestOrMapUpdated?.Invoke();
        }

        public Vector2 CellCenter(int x, int y)
        {
            float half = AetheriaWorld.HalfExtent;
            return new Vector2(-half + (x + 0.5f) * CellSizeMeters, -half + (y + 0.5f) * CellSizeMeters);
        }

        public Vector2Int WorldToCell(Vector3 pos)
        {
            Vector2 uv = WorldToNormalizedUV(pos);
            return new Vector2Int(
                Mathf.Clamp(Mathf.FloorToInt(uv.x * GridSize), 0, GridSize - 1),
                Mathf.Clamp(Mathf.FloorToInt(uv.y * GridSize), 0, GridSize - 1));
        }

        public static Vector2 WorldToNormalizedUV(Vector3 pos) => AetheriaWorld.WorldToUV(pos.x, pos.z);

        /// <summary>Fog lookup in world space: true once the cell holding this point has been revealed.</summary>
        public bool IsExploredAt(float x, float z)
        {
            Vector2 uv = AetheriaWorld.WorldToUV(x, z);
            int cx = Mathf.Clamp(Mathf.FloorToInt(uv.x * GridSize), 0, GridSize - 1);
            int cy = Mathf.Clamp(Mathf.FloorToInt(uv.y * GridSize), 0, GridSize - 1);
            return FogGrid[cy * GridSize + cx] != 0;
        }

        /// <summary>True when the point is inside the playable continent (not open sea far offshore).</summary>
        public static bool IsLandAt(float x, float z) => AetheriaWorld.BiomeAt(x, z) != BiomeId.AetherianSea;

        public float GetExplorationPercent() => TotalLandCells > 0 ? Mathf.Clamp((LandExploredCells / (float)TotalLandCells) * 100f, 0f, 100f) : 0f;

        public float GetBiomeExplorationPercent(BiomeId b)
        {
            int total = _biomeTotal[(int)b];
            return total > 0 ? Mathf.Clamp(_biomeExplored[(int)b] / (float)total * 100f, 0f, 100f) : 0f;
        }

        public bool IsKindled(string shrineId) => KindledShrines.Contains(shrineId);

        /// <summary>Lights a shrine: opens fast travel and advances the pilgrimage.</summary>
        public bool KindleShrine(string shrineId)
        {
            if (!KindledShrines.Add(shrineId)) return false;
            foreach (var pin in MapPins)
                if (pin.PinId == shrineId) { pin.Discovered = true; pin.FastTravel = true; }
            PushNotification($"СВЯТИЛИЩЕ ЗАЖЖЕНО: {LabelOf(shrineId)} · быстрое перемещение открыто [M]");
            AdvanceQuestProgress("Q_ShrinePilgrim", 1);
            var hero = HeroController.Instance;
            if (hero != null) { hero.SkillTree?.AddXP(60); }
            OnQuestOrMapUpdated?.Invoke();
            return true;
        }

        public string LabelOf(string siteId)
        {
            foreach (var pin in MapPins) if (pin.PinId == siteId) return pin.Label;
            return siteId;
        }

        public void LoadFogGrid(byte[] loaded)
        {
            Array.Clear(FogGrid, 0, FogGrid.Length);
            if (loaded != null) Array.Copy(loaded, FogGrid, Mathf.Min(loaded.Length, FogGrid.Length));
            TotalExploredCells = 0;
            LandExploredCells = 0;
            Array.Clear(_biomeExplored, 0, _biomeExplored.Length);
            for (int i = 0; i < TotalCells; i++)
            {
                if (FogGrid[i] != 0)
                {
                    TotalExploredCells++;
                    var b = _cellBiomeCache[i];
                    _biomeExplored[(int)b]++;
                    if (b != BiomeId.AetherianSea) LandExploredCells++;
                }
            }
            OnQuestOrMapUpdated?.Invoke();
        }

        public void ResetProgress()
        {
            PopulateStoryQuests();
            SeedMapPins();
            KindledShrines.Clear();
            KindledShrines.Add("Shrine_Oakhaven");
            Array.Clear(FogGrid, 0, FogGrid.Length);
            TotalExploredCells = 0;
            LandExploredCells = 0;
            Array.Clear(_biomeExplored, 0, _biomeExplored.Length);
            TrackedQuestId = "Q_PrologueAwakening";
            _notifications.Clear();
            RevealAround(SitePos("OakhavenVillage", 0f, 0f), AetheriaWorld.UsesCourtyardLayout ? 55f : 95f, suppressToast: true);
            OnQuestOrMapUpdated?.Invoke();
        }

        public QuestDef FindQuest(string id) => Quests.Find(q => q.QuestId == id);
        public QuestDef GetTrackedQuest()
        {
            var tracked = FindQuest(TrackedQuestId);
            if (tracked != null && tracked.State != QuestState.Completed) return tracked;
            var next = Quests.Find(q => q.State == QuestState.Active || q.State == QuestState.ReadyToTurnIn)
                       ?? Quests.Find(q => q.State != QuestState.Completed);
            if (next != null) TrackedQuestId = next.QuestId;
            return next;
        }

        public QuestDef FindQuestForGiver(string giverId)
        {
            QuestDef best = null;
            int BestPri(QuestState s) => s == QuestState.ReadyToTurnIn ? 4 : s == QuestState.Available ? 3 : s == QuestState.Active ? 2 : 1;
            foreach (var q in Quests)
                if (q.GiverId == giverId && (best == null || BestPri(q.State) > BestPri(best.State)))
                    best = q;
            return best;
        }

        public void AcceptQuest(string id)
        {
            var q = FindQuest(id);
            if (q == null || q.State != QuestState.Available) return;
            q.State = q.Progress >= q.TargetCount ? QuestState.ReadyToTurnIn : QuestState.Active;
            TrackedQuestId = q.QuestId;
            PushNotification($"КВЕСТ ПРИНЯТ: {q.Title}");
            OnQuestOrMapUpdated?.Invoke();
        }

        public void AdvanceQuestProgress(string id, int delta = 1)
        {
            var q = FindQuest(id);
            if (q == null || q.State == QuestState.Completed) return;
            if (q.State == QuestState.Available) q.State = QuestState.Active;
            q.Progress = Mathf.Min(q.TargetCount, q.Progress + delta);
            if (q.Progress >= q.TargetCount)
            {
                q.State = QuestState.ReadyToTurnIn;
                if (q.GiverId == "World") CompleteWorldQuest(q);
                else PushNotification($"ЦЕЛЬ ВЫПОЛНЕНА: вернитесь к {q.GiverName} ({q.Title})");
            }
            else
            {
                PushNotification($"{q.Title}: {q.Progress}/{q.TargetCount}");
            }
            OnQuestOrMapUpdated?.Invoke();
        }

        /// <summary>Side quests have no speaker: rewards are granted the moment the goal is met.</summary>
        private void CompleteWorldQuest(QuestDef q)
        {
            q.State = QuestState.Completed;
            var hero = HeroController.Instance;
            if (hero != null)
            {
                hero.SkillTree?.AddGold(q.RewardGold);
                hero.SkillTree?.AddXP(q.RewardXP);
            }
            PushNotification($"ВЫПОЛНЕНО: {q.Title} (+{q.RewardGold}G · +{q.RewardXP}XP)");
            if (TrackedQuestId == q.QuestId)
            {
                var next = Quests.Find(x => x.State != QuestState.Completed && x.GiverId == "World");
                if (next != null) TrackedQuestId = next.QuestId;
            }
        }

        public bool TurnInQuest(string id, HeroController hero)
        {
            var q = FindQuest(id);
            if (q == null || q.State != QuestState.ReadyToTurnIn || q.GiverId == "World") return false;
            q.State = QuestState.Completed;
            if (hero != null)
            {
                hero.SkillTree?.AddGold(q.RewardGold);
                hero.SkillTree?.AddXP(q.RewardXP);
                if (q.UnlockOrbReward != OrbType.None) hero.Spells?.UnlockOrb(q.UnlockOrbReward);
                if (q.UnlocksDragonMount) hero.DragonUnlocked = true;
            }
            PushNotification($"КВЕСТ ВЫПОЛНЕН: {q.Title} (+{q.RewardGold}G · +{q.RewardXP}XP)");
            if (TrackedQuestId == q.QuestId)
            {
                var next = Quests.Find(x => x.State != QuestState.Completed && x.GiverId != "World");
                if (next != null) TrackedQuestId = next.QuestId;
            }
            OnQuestOrMapUpdated?.Invoke();
            return true;
        }

        public void PushNotification(string msg)
        {
            if (string.IsNullOrEmpty(msg)) return;
            if (_notifications.Count < 24) _notifications.Enqueue(msg);
        }

        public string PopNotification() => _notifications.Count > 0 ? _notifications.Dequeue() : null;
    }
}
