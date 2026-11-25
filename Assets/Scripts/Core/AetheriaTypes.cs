using System;
using UnityEngine;

namespace Aetheria
{
    /// <summary>Primal Invoker spheres unlocked progressively through story quests: Exort -> Wex -> Quas.</summary>
    public enum OrbType
    {
        None = 0,
        Exort = 1, // E - Solar Flame
        Wex   = 2, // W - Tempest Storm
        Quas  = 3  // Q - Chrono Frost
    }

    public enum CastMode
    {
        ClassicInvoker = 0, // E/W/Q -> R -> D/F (and quick 1..0)
        InstantCombo   = 1  // Automatically casts on 3rd orb or R
    }

    /// <summary>10 Classic Invoker Spells gated dynamically by unlocked spheres (1 -> 4 -> 10).</summary>
    public enum SpellId
    {
        None = 0,
        SunStrike      = 1,  // EEE
        ChaosMeteor    = 2,  // EEW
        StormAlacrity  = 3,  // EWW
        StormEMP       = 4,  // WWW
        ColdSnap       = 5,  // QQQ
        ChronoBlink    = 6,  // QQW
        IcePrismWall   = 7,  // QQE
        GravityTornado = 8,  // QWW
        EmberConduit   = 9,  // QEE
        PrimalBlast    = 10  // QWE
    }

    public enum SpellDelivery
    {
        Projectile,
        OrbitalStrike,
        RollingMeteor,
        SelfBuff,
        RadialNova,
        BarrierWall,
        ConeWave
    }

    public enum RenderTier
    {
        CrimsonHdrpUltra   = 0,
        ClassicPerformance = 1
    }

    public enum SkillBranch
    {
        ExortPath     = 0,
        WexPath       = 1,
        QuasPath      = 2,
        BladeAndDrake = 3
    }

    // BiomeId lives in AetheriaWorld.cs: it is the single source of truth for the world layout.

    public enum QuestState
    {
        Available     = 0,
        Active        = 1,
        ReadyToTurnIn = 2,
        Completed     = 3
    }

    public enum DestructibleKind
    {
        Stone,
        Wood,
        Crystal,
        Obsidian,
        Foliage
    }

    public enum EnemyArchetype
    {
        Skeleton,
        ForestTroll,
        TempestStalker,
        FrostWarden
    }

    public enum BossAttackKind
    {
        None,
        CrimsonCleave,     // Amber Parryable!
        SeismicSlam,       // Crimson Dodge-Only
        AbyssalFireVolley, // Crimson Dodge-Only
        TempestLunge,      // Amber Parryable!
        ShatterSpin,       // Amber Parryable!
        ChronoCataclysm    // Crimson Dodge-Only
    }

    public enum OverlayMode
    {
        None,
        MainMenu,
        WorldMap,
        SkillTree,
        QuestLog,
        Grimoire,
        Dialogue,
        HeroCustomizer,
        DevConsole,
        PauseMenu
    }

    public enum HeroGender { Male = 0, Female = 1 }

    public enum HeroRace
    {
        Human        = 0,
        HighElf      = 1,
        HighlandNord = 2,
        AshDraconid  = 3,
        ShadowKin    = 4,
        Ironborn     = 5
    }

    public enum HeroHairStyle
    {
        ShortKnight,
        SamuraiManbun,
        LongFlowing,
        BraidedWarrior,
        BattleMohawk,
        ArchmageHood,
        CrownCirclet,
        BaldShaved
    }

    public enum HeroMustacheStyle
    {
        None,
        NobleChevron,
        Handlebar,
        ImperialWalrus,
        HighlandForked,
        LightStubble
    }

    public enum HeroBeardStyle
    {
        None,
        Stubble,
        ShortBoxed,
        RangerFull,
        VikingBraided,
        ArchmageLong,
        Goatee
    }

    public enum HeroEyeStyle
    {
        SteelBlue,
        MoltenAmber,
        ArcaneCyan,
        EmeraldJade,
        CrimsonFlame,
        VoidViolet,
        SilverOracle,
        SolarGold
    }

    public enum HeroTattooStyle
    {
        None,
        NordicRunes,
        DragonScales,
        ArcaneSigils,
        CrimsonScar,
        TribalAsh,
        CelestialHalo
    }

    public enum HeroOutfitStyle
    {
        VanguardPlate,
        ArchmageRobes,
        InquisitorCoat,
        DraconidWarlock,
        HighlandPelt,
        SovereignGilded
    }

    public enum HeroDetailQuality
    {
        Low = 0,
        Medium = 1,
        High = 2,
        Ultra = 3,
        Cinematic = 4
    }

    public enum HeroAttribute
    {
        Strength,
        Endurance,
        Agility,
        Intelligence,
        Spirit,
        Fortune
    }

    public enum HeroStatureClass
    {
        SmallNoMagic,     // < 1.45m
        StandardBalanced, // 1.45m .. 2.25m
        GiantTitan        // > 2.25m .. 3.00m
    }

    [Serializable]
    public class SpellDef
    {
        public SpellId Id;
        public string ComboKey;
        public string DisplayName;
        public string Description;
        public SpellDelivery Delivery;
        public bool NeedsExort;
        public bool NeedsWex;
        public bool NeedsQuas;
        public float ManaCost;
        public float Cooldown;
        public float BaseDamage;
        public float BlastRadius;
        public float DestructionImpulse;
        public Color PrimaryColor;
    }

    [Serializable]
    public class SkillNodeDef
    {
        public string NodeId;
        public SkillBranch Branch;
        public int Tier;
        public string Title;
        public string Description;
        public string PrerequisiteNodeId;
        public int MaxRank;
        public int CurrentRank;
        public float ValuePerRank;
    }

    [Serializable]
    public class QuestDef
    {
        public string QuestId;
        public string GiverId;
        public string GiverName;
        public string Title;
        public string Summary;
        public string ObjectiveText;
        public string OfferDialogue;
        public string TurnInDialogue;
        public QuestState State;
        public int Progress;
        public int TargetCount;
        public int RewardGold;
        public int RewardXP;
        public OrbType UnlockOrbReward;
        public bool UnlocksDragonMount;
        public Vector3 ObjectiveWorldLocation;
    }

    [Serializable]
    public class MapPinInfo
    {
        public string PinId;
        public string Label;
        public Vector3 WorldLocation;
        public BiomeId Biome;
        public Color Color;
        public bool Discovered;
        public bool FastTravel;
    }

    [Serializable]
    public class HeroAttributes
    {
        public int Strength = 10;
        public int Endurance = 10;
        public int Agility = 10;
        public int Intelligence = 10;
        public int Spirit = 10;
        public int Fortune = 10;
        public int UnspentPoints = 15;
    }

    [Serializable]
    public class HeroCustomizationProfile
    {
        public HeroGender Gender = HeroGender.Male;
        public HeroRace Race = HeroRace.Human;
        public HeroHairStyle HairStyle = HeroHairStyle.ShortKnight;
        public int HairColorIndex = 0;
        public HeroMustacheStyle MustacheStyle = HeroMustacheStyle.None;
        public HeroBeardStyle BeardStyle = HeroBeardStyle.None;
        public HeroEyeStyle EyeStyle = HeroEyeStyle.MoltenAmber;
        public float EyeGlowIntensity = 0.8f;
        public HeroTattooStyle TattooStyle = HeroTattooStyle.None;
        public int TattooColorIndex = 0;
        public HeroOutfitStyle OutfitStyle = HeroOutfitStyle.VanguardPlate;
        public int ArmorTintIndex = 0;
        public float HeightMeters = 1.85f;
        public float Musculature = 0.50f;
        public float ShoulderWidth = 0.50f;
        public HeroDetailQuality DetailQuality = HeroDetailQuality.Ultra;
        public HeroAttributes Attributes = new HeroAttributes();

        public HeroCustomizationProfile Clone()
        {
            return new HeroCustomizationProfile
            {
                Gender = Gender,
                Race = Race,
                HairStyle = HairStyle,
                HairColorIndex = HairColorIndex,
                MustacheStyle = MustacheStyle,
                BeardStyle = BeardStyle,
                EyeStyle = EyeStyle,
                EyeGlowIntensity = EyeGlowIntensity,
                TattooStyle = TattooStyle,
                TattooColorIndex = TattooColorIndex,
                OutfitStyle = OutfitStyle,
                ArmorTintIndex = ArmorTintIndex,
                HeightMeters = HeightMeters,
                Musculature = Musculature,
                ShoulderWidth = ShoulderWidth,
                DetailQuality = DetailQuality,
                Attributes = new HeroAttributes
                {
                    Strength = Attributes.Strength,
                    Endurance = Attributes.Endurance,
                    Agility = Attributes.Agility,
                    Intelligence = Attributes.Intelligence,
                    Spirit = Attributes.Spirit,
                    Fortune = Attributes.Fortune,
                    UnspentPoints = Attributes.UnspentPoints
                }
            };
        }
    }

    [Serializable]
    public struct HeroDerivedStats
    {
        public int EffStrength, EffEndurance, EffAgility, EffIntelligence, EffSpirit, EffFortune;
        public float MaxHP, MaxStamina, MaxMana;
        public float HPRegenPerSec, StaminaRegenPerSec, ManaRegenPerSec;
        public float MeleeDamageMultiplier, MeleeReachMultiplier, AttackSpeedMultiplier;
        public float MoveSpeedMetersPerSec, TurnRateDegPerSec, AccelMultiplier;
        public float StaggerMultiplier, DestructionDamageMultiplier;
        public bool CanCastSpells;
        public float SpellPowerMultiplier, ExortBonusMultiplier, ManaCostMultiplier, SpellCooldownMultiplier;
        public float CritChance, CritDamageMultiplier, ArmorReduction, FireResistance, KnockbackResistance;
        public float ParryWindowBonusSec, ParryStaggerBonusMult, BlockStaminaDrainMult;
        public float DodgeDistanceMultiplier, DodgeStaminaCostMultiplier, DodgeIFrameBonusSec;
        public float DragonSpeedMultiplier, DragonFireMultiplier, DragonFireDrainMultiplier;
        public float XPAndGoldMultiplier, MapRevealMultiplier;
        public bool ColdSlowImmune, BerserkLowHPActive, NightShadowCritActive;
        public HeroStatureClass StatureClass;
        public string StatureSummary;
        public string RacePassiveSummary;
    }

    public static class AetheriaPalette
    {
        public static readonly Color PanelBg        = new Color(0.045f, 0.040f, 0.055f, 0.90f);
        public static readonly Color PanelBgDeep    = new Color(0.026f, 0.022f, 0.032f, 0.95f);
        public static readonly Color CardBg         = new Color(0.062f, 0.054f, 0.074f, 0.92f);
        public static readonly Color HairlineGold   = new Color(0.76f, 0.59f, 0.28f, 0.85f);
        public static readonly Color HairlineCyan   = new Color(0.00f, 0.96f, 1.00f, 0.80f);
        public static readonly Color HairlineCrimson= new Color(0.86f, 0.16f, 0.18f, 0.88f);
        public static readonly Color TextPrimary    = new Color(0.95f, 0.92f, 0.86f, 1f);
        public static readonly Color TextSecondary  = new Color(0.72f, 0.68f, 0.63f, 1f);
        public static readonly Color TextGold       = new Color(0.96f, 0.80f, 0.42f, 1f);
        public static readonly Color TextCyan       = new Color(0.00f, 0.96f, 1.00f, 1f);
        public static readonly Color TextCrimson    = new Color(0.96f, 0.26f, 0.24f, 1f);
        public static readonly Color TextAmber      = new Color(0.98f, 0.72f, 0.24f, 1f);
        public static readonly Color TextEmerald    = new Color(0.30f, 0.88f, 0.56f, 1f);
        public static readonly Color BarHP          = new Color(0.722f, 0.114f, 0.141f, 1f);
        public static readonly Color BarMana        = new Color(0.118f, 0.431f, 0.910f, 1f);
        public static readonly Color BarStamina     = new Color(0.224f, 0.659f, 0.420f, 1f);
        public static readonly Color BarXP          = new Color(0.831f, 0.659f, 0.263f, 1f);
        public static readonly Color BarBossHP      = new Color(0.76f, 0.09f, 0.12f, 1f);
        public static readonly Color BarStagger     = new Color(0.910f, 0.608f, 0.141f, 1f);
        public static readonly Color OrbExort       = new Color(0.98f, 0.42f, 0.12f, 1f);
        public static readonly Color OrbWex         = new Color(0.68f, 0.36f, 1.00f, 1f);
        public static readonly Color OrbQuas        = new Color(0.00f, 0.92f, 1.00f, 1f);
        public static readonly Color OrbLocked      = new Color(0.28f, 0.26f, 0.32f, 0.78f);

        public static Color GetOrbColor(OrbType orb) => orb switch
        {
            OrbType.Exort => OrbExort,
            OrbType.Wex   => OrbWex,
            OrbType.Quas  => OrbQuas,
            _             => OrbLocked
        };

        private static Material _litTemplate;
        private static Material _particleTemplate;

        public static Material GetLitMaterial(Color baseCol, Color? emissive = null)
        {
            if (_litTemplate == null) _litTemplate = Resources.Load<Material>("Materials/URP_Lit_Template");
            Material m;
            if (_litTemplate != null) m = new Material(_litTemplate);
            else
            {
                var sh = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard") ?? Shader.Find("Diffuse");
                m = new Material(sh);
            }
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", baseCol);
            m.color = baseCol;
            if (emissive.HasValue)
            {
                m.EnableKeyword("_EMISSION");
                m.SetColor("_EmissionColor", emissive.Value);
            }
            return m;
        }

        public static Material GetParticleMaterial(Color col)
        {
            if (_particleTemplate == null) _particleTemplate = Resources.Load<Material>("Materials/URP_Particles_Unlit_Template");
            Material m;
            if (_particleTemplate != null) m = new Material(_particleTemplate);
            else
            {
                var sh = Shader.Find("Universal Render Pipeline/Particles/Unlit") ?? Shader.Find("Particles/Standard Unlit") ?? Shader.Find("Sprites/Default");
                m = new Material(sh);
            }
            if (m.HasProperty("_BaseColor")) m.SetColor("_BaseColor", col);
            m.color = col;
            return m;
        }

        public static void RelaxHumanoidArms(GameObject root)
        {
            if (root == null) return;
            var anim = root.GetComponentInChildren<Animator>();
            if (anim != null && anim.runtimeAnimatorController == null) anim.enabled = false;

            Transform torso = FindBoneAny(root.transform, "spine_02", "spine_01", "pelvis", "Spine");
            Vector3 center = torso != null ? torso.position : root.transform.position + Vector3.up * 1.1f;

            void RelaxSide(string[] upperNames, string[] lowerNames, string[] handNames)
            {
                Transform upper = FindBoneAny(root.transform, upperNames);
                Transform lower = FindBoneAny(upper != null ? upper : root.transform, lowerNames);
                Transform hand = FindBoneAny(lower != null ? lower : root.transform, handNames);
                if (upper == null || lower == null) return;

                Vector3 curDir = lower.position - upper.position;
                if (curDir.sqrMagnitude < 0.0001f) return;
                curDir.Normalize();

                Vector3 outSide = upper.position - center;
                outSide.y = 0f;
                outSide = outSide.sqrMagnitude > 0.0001f ? outSide.normalized : root.transform.right;

                Vector3 targetDir = (Vector3.down * 0.90f + outSide * 0.22f + root.transform.forward * 0.14f).normalized;
                if (Vector3.Angle(curDir, targetDir) > 4f)
                    upper.rotation = Quaternion.FromToRotation(curDir, targetDir) * upper.rotation;

                if (hand != null)
                {
                    Vector3 fCur = hand.position - lower.position;
                    if (fCur.sqrMagnitude > 0.0001f)
                    {
                        fCur.Normalize();
                        Vector3 fTarget = (Vector3.down * 0.72f + root.transform.forward * 0.56f + outSide * 0.08f).normalized;
                        if (Vector3.Angle(fCur, fTarget) > 4f)
                            lower.rotation = Quaternion.FromToRotation(fCur, fTarget) * lower.rotation;
                    }
                }
            }

            RelaxSide(
                new[] { "upperarm_l", "UpperArm_L", "Bip001_L_UpperArm", "mixamorig:LeftArm" },
                new[] { "lowerarm_l", "LowerArm_L", "Bip001_L_Forearm", "mixamorig:LeftForeArm" },
                new[] { "hand_l", "Hand_L", "Bip001_L_Hand", "mixamorig:LeftHand" });
            RelaxSide(
                new[] { "upperarm_r", "UpperArm_R", "Bip001_R_UpperArm", "mixamorig:RightArm" },
                new[] { "lowerarm_r", "LowerArm_R", "Bip001_R_Forearm", "mixamorig:RightForeArm" },
                new[] { "hand_r", "Hand_R", "Bip001_R_Hand", "mixamorig:RightHand" });
        }

        public static Transform FindBoneAny(Transform root, params string[] names)
        {
            if (root == null) return null;
            foreach (var n in names)
                if (root.name.Equals(n, StringComparison.OrdinalIgnoreCase)) return root;
            for (int i = 0; i < root.childCount; i++)
            {
                var hit = FindBoneAny(root.GetChild(i), names);
                if (hit != null) return hit;
            }
            return null;
        }
    }
}
