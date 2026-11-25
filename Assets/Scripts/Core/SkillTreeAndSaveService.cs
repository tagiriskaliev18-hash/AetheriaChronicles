using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace Aetheria
{
    [Serializable]
    public class AetheriaSavePayload
    {
        public int PlayerLevel = 1;
        public int CurrentXP = 0;
        public int UnspentSkillPoints = 1;
        public int Gold = 0;
        public bool UnlockedExort = false;
        public bool UnlockedWex = false;
        public bool UnlockedQuas = false;
        public bool UnlockedDragonMount = false;
        public List<string> SkillNodeIds = new List<string>();
        public List<int> SkillNodeRanks = new List<int>();
        public List<string> QuestIds = new List<string>();
        public List<int> QuestProgress = new List<int>();
        public List<int> QuestStates = new List<int>();
        public byte[] FogOfWarGrid = new byte[4096];
        public Vector3 LastHeroPosition;
        public HeroCustomizationProfile HeroProfile = new HeroCustomizationProfile();
    }

    /// <summary>Manages the 24-node 4-branch RPG Skill Tree, XP/leveling, Gold, and JSON save persistence.</summary>
    public class SkillTreeAndSaveService : MonoBehaviour
    {
        public int Level = 1;
        public int CurrentXP = 0;
        public int UnspentSkillPoints = 1;
        public int Gold = 0;
        public List<SkillNodeDef> Nodes = new List<SkillNodeDef>();

        public event Action OnSkillTreeChanged;
        public event Action<int> OnLevelUp;

        private static string SaveFilePath => Path.Combine(Application.persistentDataPath, "aetheria_save0.json");

        private void Awake()
        {
            if (Nodes.Count == 0) BuildDefaultNodes();
        }

        public void BuildDefaultNodes()
        {
            Nodes.Clear();
            void Add(string id, SkillBranch b, int tier, string title, string desc, string pre, int maxRank, float val)
            {
                Nodes.Add(new SkillNodeDef
                {
                    NodeId = id, Branch = b, Tier = tier, Title = title, Description = desc,
                    PrerequisiteNodeId = pre, MaxRank = maxRank, CurrentRank = 0, ValuePerRank = val
                });
            }

            // Branch 1: Path of Exort
            Add("exort_solar_core",      SkillBranch.ExortPath, 1, "Solar Core",      "+14% Exort spell damage per rank.",                       "",                   3, 0.14f);
            Add("exort_wildfire",        SkillBranch.ExortPath, 2, "Wildfire",        "Burn ticks spread Wildfire & +12% DoT per rank.",         "exort_solar_core",   3, 0.12f);
            Add("exort_armor_melt",      SkillBranch.ExortPath, 2, "Armor Melt",      "Fire hits melt Boss armor (+8% vulnerability/rank).",     "exort_solar_core",   3, 0.08f);
            Add("exort_chaos_shatter",   SkillBranch.ExortPath, 3, "Chaos Shatter",   "+35% Destruction impulse & debris radius per rank.",      "exort_wildfire",     2, 0.35f);
            Add("exort_meteor_mass",     SkillBranch.ExortPath, 3, "Meteor Mass",     "+22% Meteor & SunStrike blast radius per rank.",          "exort_armor_melt",   2, 0.22f);
            Add("exort_cataclysm_master",SkillBranch.ExortPath, 4, "Cataclysm Master","Capstone: +35% Solar critical burst.",                  "exort_meteor_mass",  1, 0.35f);

            // Branch 2: Path of Wex
            Add("wex_swift_invocation",  SkillBranch.WexPath, 1, "Swift Invocation", "-10% spell cooldowns & faster cast speed per rank.",      "",                      3, 0.10f);
            Add("wex_arc_conduit",       SkillBranch.WexPath, 2, "Arc Conduit",      "+15% Storm & Lightning damage per rank.",               "wex_swift_invocation",  3, 0.15f);
            Add("wex_phantom_step",      SkillBranch.WexPath, 2, "Phantom Step",     "+14% sprint & dodge-roll distance per rank.",           "wex_swift_invocation",  3, 0.14f);
            Add("wex_drake_gale",        SkillBranch.WexPath, 3, "Drake Gale",       "+20% Dragon flight boost speed & fire recharge/rank.",  "wex_phantom_step",      2, 0.20f);
            Add("wex_storm_mana",        SkillBranch.WexPath, 3, "Storm Mana",       "+20 Max Mana & +15% Mana regen per rank.",              "wex_arc_conduit",       2, 20.0f);
            Add("wex_tempest_lord",      SkillBranch.WexPath, 4, "Tempest Lord",     "Capstone: Superconduct reaction deals +45% shock.",     "wex_storm_mana",        1, 0.45f);

            // Branch 3: Path of Quas
            Add("quas_vital_frost",      SkillBranch.QuasPath, 1, "Vital Frost",      "+25 Max HP & +1.5 HP/s regen per rank.",                "",                     3, 25.0f);
            Add("quas_chrono_hold",      SkillBranch.QuasPath, 2, "Chrono Hold",      "+20% Freeze / Slow duration per rank.",                 "quas_vital_frost",     3, 0.20f);
            Add("quas_stagger_breaker",  SkillBranch.QuasPath, 2, "Stagger Breaker",  "+18% Boss StaggerGauge buildup per rank.",              "quas_vital_frost",     3, 0.18f);
            Add("quas_glacial_aegis",    SkillBranch.QuasPath, 3, "Glacial Aegis",    "Shield block reduces 10% more damage & chills/rank.",   "quas_chrono_hold",     2, 0.10f);
            Add("quas_thermal_shock",    SkillBranch.QuasPath, 3, "Thermal Shock",    "+30% Thermal Shock & Cryo-Shatter damage per rank.",    "quas_stagger_breaker", 2, 0.30f);
            Add("quas_timeless_sovereign",SkillBranch.QuasPath,4, "Timeless Sovereign","Capstone: +1.2s Boss Stagger Break window.",          "quas_thermal_shock",   1, 1.20f);

            // Branch 4: Way of Blade & Drake
            Add("blade_keen_edge",       SkillBranch.BladeAndDrake, 1, "Keen Edge",       "+15% Sword melee combo damage per rank.",            "",                   3, 0.15f);
            Add("blade_iron_stamina",    SkillBranch.BladeAndDrake, 2, "Iron Stamina",    "+20 Max Stamina & +18% Stamina regen per rank.",     "blade_keen_edge",    3, 20.0f);
            Add("blade_perfect_parry",   SkillBranch.BladeAndDrake, 2, "Perfect Parry",   "Timed parry restores 22 Mana/rank & staggers.",      "blade_keen_edge",    3, 22.0f);
            Add("blade_executioner",     SkillBranch.BladeAndDrake, 3, "Executioner",     "+28% melee finisher vs staggered foes per rank.",    "blade_perfect_parry",2, 0.28f);
            Add("blade_dragonfire_mastery",SkillBranch.BladeAndDrake,3,"Dragonfire Mastery","+25% Dragon Fire Breath DPS & range per rank.",    "blade_iron_stamina", 2, 0.25f);
            Add("blade_wukong_stance",   SkillBranch.BladeAndDrake, 4, "Wukong Stance",   "Capstone: +25% Dodge i-frames & riposte strike.",    "blade_executioner",  1, 0.25f);
        }

        public int GetXPForNextLevel() => Mathf.Max(1, Mathf.RoundToInt(120f * Mathf.Pow(Mathf.Max(1, Level), 1.6f)));

        public void AddXP(int amount)
        {
            if (amount <= 0) return;
            var cust = GetComponent<HeroCustomizationService>();
            float mult = cust != null ? Mathf.Max(0.1f, cust.Derived.XPAndGoldMultiplier) : 1f;
            CurrentXP += Mathf.Max(1, Mathf.RoundToInt(amount * mult));

            while (Level < 999)
            {
                int req = GetXPForNextLevel();
                if (CurrentXP < req) break;
                CurrentXP -= req;
                Level++;
                UnspentSkillPoints++;
                cust?.GrantAttributePoints(3);
                OnLevelUp?.Invoke(Level);
                QuestAndMapService.Instance?.PushNotification($"LEVEL UP! Level {Level} (+1 Skill Pt · +3 Attr Pts)");
            }
            cust?.Recalculate();
            OnSkillTreeChanged?.Invoke();
        }

        public void AddGold(int delta)
        {
            Gold = Mathf.Max(0, Gold + delta);
            OnSkillTreeChanged?.Invoke();
        }

        public int GetRank(string nodeId)
        {
            var n = Nodes.Find(x => x.NodeId == nodeId);
            return n != null ? n.CurrentRank : 0;
        }

        public bool CanUpgrade(string nodeId)
        {
            if (UnspentSkillPoints <= 0) return false;
            var n = Nodes.Find(x => x.NodeId == nodeId);
            if (n == null || n.CurrentRank >= n.MaxRank) return false;
            if (!string.IsNullOrEmpty(n.PrerequisiteNodeId) && GetRank(n.PrerequisiteNodeId) <= 0) return false;
            return true;
        }

        public bool UpgradeNode(string nodeId)
        {
            if (!CanUpgrade(nodeId)) return false;
            var n = Nodes.Find(x => x.NodeId == nodeId);
            n.CurrentRank++;
            UnspentSkillPoints--;
            OnSkillTreeChanged?.Invoke();
            return true;
        }

        // Computed Multipliers
        public float MaxHPBonus        => GetRank("quas_vital_frost") * 25f;
        public float HPRegenBonus      => GetRank("quas_vital_frost") * 1.5f;
        public float MaxManaBonus      => GetRank("wex_storm_mana") * 20f;
        public float ManaRegenMult     => 1f + GetRank("wex_storm_mana") * 0.15f;
        public float MaxStaminaBonus   => GetRank("blade_iron_stamina") * 20f;
        public float StaminaRegenMult  => 1f + GetRank("blade_iron_stamina") * 0.18f;
        public float ExortDamageMult   => 1f + GetRank("exort_solar_core") * 0.14f + GetRank("exort_cataclysm_master") * 0.35f;
        public float ExortRadiusMult   => 1f + GetRank("exort_meteor_mass") * 0.22f;
        public float WexDamageMult     => 1f + GetRank("wex_arc_conduit") * 0.15f;
        public float WexCooldownMult   => Mathf.Clamp(1f - GetRank("wex_swift_invocation") * 0.10f, 0.35f, 1f);
        public float QuasControlMult   => 1f + GetRank("quas_chrono_hold") * 0.20f;
        public float StaggerMult       => 1f + GetRank("quas_stagger_breaker") * 0.18f;
        public float StaggerWindowBonus=> GetRank("quas_timeless_sovereign") * 1.20f;
        public float ShieldMitigation  => GetRank("quas_glacial_aegis") * 0.10f;
        public float MeleeDamageMult   => 1f + GetRank("blade_keen_edge") * 0.15f;
        public float ExecutionerMult   => 1f + GetRank("blade_executioner") * 0.28f;
        public float ParryManaRestore  => GetRank("blade_perfect_parry") * 22f;
        public float DodgeIFrameMult   => 1f + GetRank("blade_wukong_stance") * 0.25f;
        public float MoveSpeedMult     => 1f + GetRank("wex_phantom_step") * 0.14f;
        public float DragonSpeedMult   => 1f + GetRank("wex_drake_gale") * 0.20f;
        public float DragonFireMult    => 1f + GetRank("blade_dragonfire_mastery") * 0.25f;
        public float DestructionMult   => 1f + GetRank("exort_chaos_shatter") * 0.35f;

        public static bool HasSaveFile() => File.Exists(SaveFilePath);

        public void SaveProgress(HeroController hero)
        {
            var payload = new AetheriaSavePayload
            {
                PlayerLevel = Level,
                CurrentXP = CurrentXP,
                UnspentSkillPoints = UnspentSkillPoints,
                Gold = Gold,
                LastHeroPosition = hero != null ? hero.transform.position : Vector3.zero
            };

            if (hero != null)
            {
                payload.UnlockedDragonMount = hero.DragonUnlocked;
                if (hero.Spells != null)
                {
                    payload.UnlockedExort = hero.Spells.UnlockedExort;
                    payload.UnlockedWex = hero.Spells.UnlockedWex;
                    payload.UnlockedQuas = hero.Spells.UnlockedQuas;
                }
                if (hero.Customization != null)
                    payload.HeroProfile = hero.Customization.Profile.Clone();
            }

            foreach (var n in Nodes)
            {
                payload.SkillNodeIds.Add(n.NodeId);
                payload.SkillNodeRanks.Add(n.CurrentRank);
            }

            if (QuestAndMapService.Instance != null)
            {
                foreach (var q in QuestAndMapService.Instance.Quests)
                {
                    payload.QuestIds.Add(q.QuestId);
                    payload.QuestProgress.Add(q.Progress);
                    payload.QuestStates.Add((int)q.State);
                }
                payload.FogOfWarGrid = (byte[])QuestAndMapService.Instance.FogGrid.Clone();
            }

            File.WriteAllText(SaveFilePath, JsonUtility.ToJson(payload, true));
        }

        public bool LoadProgress(HeroController hero)
        {
            if (!File.Exists(SaveFilePath)) return false;
            var payload = JsonUtility.FromJson<AetheriaSavePayload>(File.ReadAllText(SaveFilePath));
            if (payload == null) return false;

            Level = Mathf.Max(1, payload.PlayerLevel);
            CurrentXP = Mathf.Max(0, payload.CurrentXP);
            UnspentSkillPoints = Mathf.Max(0, payload.UnspentSkillPoints);
            Gold = Mathf.Max(0, payload.Gold);

            for (int i = 0; i < Mathf.Min(payload.SkillNodeIds.Count, payload.SkillNodeRanks.Count); i++)
            {
                var node = Nodes.Find(x => x.NodeId == payload.SkillNodeIds[i]);
                if (node != null) node.CurrentRank = Mathf.Clamp(payload.SkillNodeRanks[i], 0, node.MaxRank);
            }

            if (hero != null)
            {
                hero.DragonUnlocked = payload.UnlockedDragonMount;
                if (hero.Spells != null)
                {
                    if (payload.UnlockedExort) hero.Spells.UnlockOrb(OrbType.Exort);
                    if (payload.UnlockedWex)   hero.Spells.UnlockOrb(OrbType.Wex);
                    if (payload.UnlockedQuas)  hero.Spells.UnlockOrb(OrbType.Quas);
                }
                if (hero.Customization != null && payload.HeroProfile != null)
                {
                    hero.Customization.Profile = payload.HeroProfile.Clone();
                    hero.Customization.Recalculate();
                }
                if (payload.LastHeroPosition.sqrMagnitude > 1f)
                    hero.TeleportSafe(payload.LastHeroPosition);
            }

            if (QuestAndMapService.Instance != null)
            {
                for (int i = 0; i < Mathf.Min(payload.QuestIds.Count, payload.QuestStates.Count); i++)
                {
                    var q = QuestAndMapService.Instance.FindQuest(payload.QuestIds[i]);
                    if (q != null)
                    {
                        q.Progress = payload.QuestProgress[i];
                        q.State = (QuestState)payload.QuestStates[i];
                    }
                }
                if (payload.FogOfWarGrid != null && payload.FogOfWarGrid.Length == 4096)
                    QuestAndMapService.Instance.LoadFogGrid(payload.FogOfWarGrid);
            }

            OnSkillTreeChanged?.Invoke();
            return true;
        }
    }
}
