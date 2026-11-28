using System;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Computes derived RPG stats from Race passives, Attributes, Musculature,
    /// and Stature (1.10m tiny no-magic .. 3.00m Giant Titan) and updates the live 3D Hero rig.
    /// </summary>
    public class HeroCustomizationService : MonoBehaviour
    {
        public const float MinHeightMeters = 1.10f;
        public const float BaseHeightMeters = 1.85f;
        public const float SmallNoMagicMax = 1.45f;
        public const float GiantTitanMin = 2.26f;
        public const float MaxHeightMeters = 3.00f;
        public const int MinAttr = 5;
        public const int MaxAttr = 60;
        public const int DefaultStartPoints = 15;
        /// <summary>Default new-game hero: the CC-BY "Medieval Knight" (by__Rx) with its own PBR textures and a
        /// separate "Weapon_Sword" in the right hand (built by Aetheria/4c. Build Knight Hero).</summary>
        public const string KnightHeroResource = "Heroes/Hero_Knight";
        /// <summary>Last resort when a prefab is missing (e.g. 4c not run yet).</summary>
        public const string FallbackHeroResource = "Heroes/Hero_Kwang";

        public HeroCustomizationProfile Profile = new HeroCustomizationProfile();
        public HeroDerivedStats Derived { get; private set; }
        public event Action OnProfileChanged;

        public static readonly Color[] HairColors =
        {
            new Color(0.10f, 0.09f, 0.11f), // Obsidian Black
            new Color(0.32f, 0.21f, 0.14f), // Chestnut Umber
            new Color(0.66f, 0.50f, 0.24f), // Burnished Gold
            new Color(0.58f, 0.17f, 0.12f), // Crimson Flame
            new Color(0.84f, 0.86f, 0.90f), // Frost Silver
            new Color(0.95f, 0.94f, 0.88f), // Platinum White
            new Color(0.28f, 0.44f, 0.68f), // Sapphire Mist
            new Color(0.22f, 0.56f, 0.46f)  // Emerald Verdant
        };

        public static readonly string[] HairColorNames =
        {
            "Obsidian", "Chestnut", "Sun Gold", "Ember Red", "Frost Silver", "Platinum", "Sapphire", "Emerald"
        };

        public static readonly Color[] TattooColors =
        {
            new Color(0.12f, 0.86f, 0.98f), // Arcane Cyan
            new Color(0.98f, 0.34f, 0.12f), // Solar Ember
            new Color(0.76f, 0.14f, 0.18f), // Blood Crimson
            new Color(0.88f, 0.72f, 0.28f), // Sovereign Gold
            new Color(0.62f, 0.32f, 0.98f), // Void Violet
            new Color(0.14f, 0.14f, 0.18f)  // Ash Charcoal
        };

        public static readonly Color[] ArmorTints =
        {
            new Color(0.92f, 0.90f, 0.88f), // Forged Steel
            new Color(0.22f, 0.20f, 0.25f), // Obsidian Plate
            new Color(0.62f, 0.16f, 0.16f), // Crimson Sovereign
            new Color(0.74f, 0.58f, 0.24f), // Aurelian Gilt
            new Color(0.22f, 0.38f, 0.56f), // Sapphire Guard
            new Color(0.24f, 0.44f, 0.32f)  // Highland Verdant
        };

        public static Color GetEyeColor(HeroEyeStyle style) => style switch
        {
            HeroEyeStyle.SteelBlue    => new Color(0.45f, 0.72f, 0.98f),
            HeroEyeStyle.MoltenAmber  => new Color(1.00f, 0.62f, 0.14f),
            HeroEyeStyle.ArcaneCyan   => new Color(0.00f, 0.95f, 1.00f),
            HeroEyeStyle.EmeraldJade  => new Color(0.22f, 0.92f, 0.52f),
            HeroEyeStyle.CrimsonFlame => new Color(1.00f, 0.20f, 0.16f),
            HeroEyeStyle.VoidViolet   => new Color(0.72f, 0.30f, 1.00f),
            HeroEyeStyle.SilverOracle => new Color(0.86f, 0.92f, 0.98f),
            _                         => new Color(1.00f, 0.85f, 0.30f)
        };

        public static Color GetRaceSkinTint(HeroRace race) => race switch
        {
            HeroRace.Human        => new Color(0.96f, 0.89f, 0.83f),
            HeroRace.HighElf      => new Color(0.92f, 0.93f, 0.98f),
            HeroRace.HighlandNord => new Color(0.93f, 0.86f, 0.82f),
            HeroRace.AshDraconid  => new Color(0.76f, 0.58f, 0.54f),
            HeroRace.ShadowKin    => new Color(0.68f, 0.64f, 0.78f),
            HeroRace.Ironborn     => new Color(0.82f, 0.78f, 0.74f),
            _                     => Color.white
        };

        public static string GetRaceLabel(HeroRace r) => r switch
        {
            HeroRace.Human        => "Human (Valenarian)",
            HeroRace.HighElf      => "High Elf (Aetherian)",
            HeroRace.HighlandNord => "Highland Nord",
            HeroRace.AshDraconid  => "Ash Draconid",
            HeroRace.ShadowKin    => "Shadow Kin (Umbra)",
            HeroRace.Ironborn     => "Ironborn Runesmith",
            _                     => r.ToString()
        };

        public static string GetAttributeHint(HeroAttribute attr) => attr switch
        {
            HeroAttribute.Strength     => "+6% Melee · +12 HP · +4% Stagger per pt",
            HeroAttribute.Endurance    => "+10 Stamina · +8 HP · +1.2% Armor per pt",
            HeroAttribute.Agility      => "+2.8% AtkSpd · +0.14m/s Move · +1.5% Crit per pt",
            HeroAttribute.Intelligence => "+6.5% Spell Power · +14 Mana · -1.5% CD per pt",
            HeroAttribute.Spirit       => "+1.4 Mana/s · +0.5 HP/s · +4% Drake Fire per pt",
            HeroAttribute.Fortune      => "+4% Crit Dmg · +3% Gold/XP · +2.5ms Parry per pt",
            _                          => ""
        };

        public static HeroDerivedStats ComputeDerivedStats(
            HeroCustomizationProfile p, int heroLevel = 1, bool isNightTime = false, float hpRatio = 1f)
        {
            if (p == null) p = new HeroCustomizationProfile();
            HeroDerivedStats s = new HeroDerivedStats
            {
                MaxHP = 120f + Mathf.Max(0, heroLevel - 1) * 14f,
                MaxStamina = 100f,
                MaxMana = 120f + Mathf.Max(0, heroLevel - 1) * 10f,
                HPRegenPerSec = 0.5f,
                StaminaRegenPerSec = 22f,
                ManaRegenPerSec = 9f,
                MeleeDamageMultiplier = 1f,
                MeleeReachMultiplier = 1f,
                AttackSpeedMultiplier = 1f,
                MoveSpeedMetersPerSec = 6.2f,
                TurnRateDegPerSec = 560f,
                AccelMultiplier = 1f,
                StaggerMultiplier = 1f,
                DestructionDamageMultiplier = 1f,
                CanCastSpells = true,
                SpellPowerMultiplier = 1f,
                ExortBonusMultiplier = 1f,
                ManaCostMultiplier = 1f,
                SpellCooldownMultiplier = 1f,
                CritChance = 0.08f,
                CritDamageMultiplier = 1.50f,
                ArmorReduction = 0.05f,
                ParryStaggerBonusMult = 1f,
                BlockStaminaDrainMult = 1f,
                DodgeDistanceMultiplier = 1f,
                DodgeStaminaCostMultiplier = 1f,
                DragonSpeedMultiplier = 1f,
                DragonFireMultiplier = 1f,
                DragonFireDrainMultiplier = 1f,
                XPAndGoldMultiplier = 1f,
                MapRevealMultiplier = 1f
            };

            int str = p.Attributes.Strength;
            int end = p.Attributes.Endurance;
            int agi = p.Attributes.Agility;
            int intel = p.Attributes.Intelligence;
            int spi = p.Attributes.Spirit;
            int lck = p.Attributes.Fortune;

            switch (p.Race)
            {
                case HeroRace.Human:
                    str += 2; end += 2; agi += 2; intel += 2; spi += 2; lck += 2;
                    s.XPAndGoldMultiplier *= 1.15f;
                    s.MapRevealMultiplier *= 1.15f;
                    s.StaminaRegenPerSec *= 1.12f;
                    s.RacePassiveSummary = "Valenarian Versatility: +2 All Attrs · +15% XP & Gold · +15% Map Reveal · +12% Stamina Regen";
                    break;
                case HeroRace.HighElf:
                    intel += 5; spi += 4; agi += 3; end -= 2;
                    s.ManaCostMultiplier *= 0.80f;
                    s.SpellCooldownMultiplier *= 0.85f;
                    s.MoveSpeedMetersPerSec *= 1.12f;
                    s.AttackSpeedMultiplier *= 1.10f;
                    s.MaxHP *= 0.88f;
                    s.MapRevealMultiplier *= 1.35f;
                    s.RacePassiveSummary = "Aetherian Arcana: +5 INT +4 SPI +3 AGI -2 END · -20% Mana · -15% CD · +12% Move · -12% HP";
                    break;
                case HeroRace.HighlandNord:
                    str += 6; end += 5; intel -= 2; agi -= 1;
                    s.MaxHP *= 1.25f;
                    s.ColdSlowImmune = true;
                    if (hpRatio <= 0.45f)
                    {
                        s.BerserkLowHPActive = true;
                        s.MeleeDamageMultiplier *= 1.30f;
                        s.StaggerMultiplier *= 1.30f;
                    }
                    s.RacePassiveSummary = "Nordic Bloodrage: +6 STR +5 END -2 INT -1 AGI · +25% HP · Cold Immune · +30% Melee <=45% HP";
                    break;
                case HeroRace.AshDraconid:
                    str += 4; spi += 4; end += 3; intel += 2; agi -= 1;
                    s.DragonSpeedMultiplier *= 1.25f;
                    s.DragonFireMultiplier *= 1.35f;
                    s.DragonFireDrainMultiplier *= 0.75f;
                    s.FireResistance += 0.40f;
                    s.ExortBonusMultiplier *= 1.25f;
                    s.RacePassiveSummary = "Draconic Kinship: +4 STR +4 SPI +3 END +2 INT -1 AGI · +25% Drake Speed · +35% Breath · +25% Exort";
                    break;
                case HeroRace.ShadowKin:
                    agi += 6; lck += 4; intel += 2; end -= 3;
                    s.DodgeDistanceMultiplier *= 1.25f;
                    s.DodgeIFrameBonusSec += 0.08f;
                    s.DodgeStaminaCostMultiplier *= 0.65f;
                    s.CritChance += 0.12f;
                    if (isNightTime)
                    {
                        s.NightShadowCritActive = true;
                        s.CritDamageMultiplier += 0.40f;
                    }
                    s.RacePassiveSummary = "Umbral Veil: +6 AGI +4 LCK +2 INT -3 END · +25% Dodge · +80ms iFrames · +12% Crit (+40% Night)";
                    break;
                case HeroRace.Ironborn:
                    end += 5; str += 4; lck += 3; agi -= 3;
                    s.ArmorReduction += 0.18f;
                    s.KnockbackResistance += 0.50f;
                    s.DestructionDamageMultiplier *= 1.45f;
                    s.ParryStaggerBonusMult *= 2.0f;
                    s.XPAndGoldMultiplier *= 1.15f;
                    s.RacePassiveSummary = "Runesmith Bastion: +5 END +4 STR +3 LCK -3 AGI · +18% Armor · +50% Poise · 2x Parry Stagger";
                    break;
            }

            s.EffStrength = Mathf.Max(1, str);
            s.EffEndurance = Mathf.Max(1, end);
            s.EffAgility = Mathf.Max(1, agi);
            s.EffIntelligence = Mathf.Max(1, intel);
            s.EffSpirit = Mathf.Max(1, spi);
            s.EffFortune = Mathf.Max(1, lck);

            int dStr = s.EffStrength - 10;
            s.MeleeDamageMultiplier += dStr * 0.06f;
            s.MaxHP += dStr * 12f;
            s.StaggerMultiplier += dStr * 0.04f;

            int dEnd = s.EffEndurance - 10;
            s.MaxStamina += dEnd * 10f;
            s.StaminaRegenPerSec += dEnd * 1.5f;
            s.MaxHP += dEnd * 8f;
            s.ArmorReduction += Mathf.Max(0, dEnd) * 0.012f;
            s.BlockStaminaDrainMult *= Mathf.Clamp(1f - dEnd * 0.015f, 0.45f, 1.35f);

            int dAgi = s.EffAgility - 10;
            s.AttackSpeedMultiplier += dAgi * 0.028f;
            s.MoveSpeedMetersPerSec += dAgi * 0.14f;
            s.CritChance += dAgi * 0.015f;
            s.DodgeIFrameBonusSec += Mathf.Max(0, dAgi) * 0.003f;

            int dInt = s.EffIntelligence - 10;
            s.SpellPowerMultiplier += dInt * 0.065f;
            s.MaxMana += dInt * 14f;
            s.SpellCooldownMultiplier *= Mathf.Clamp(1f - dInt * 0.015f, 0.55f, 1.25f);

            int dSpi = s.EffSpirit - 10;
            s.ManaRegenPerSec += dSpi * 1.4f;
            s.HPRegenPerSec += Mathf.Max(0f, dSpi * 0.5f);
            s.DragonFireMultiplier += dSpi * 0.04f;

            int dLck = s.EffFortune - 10;
            s.CritDamageMultiplier += dLck * 0.04f;
            s.XPAndGoldMultiplier += dLck * 0.03f;
            s.ParryWindowBonusSec += Mathf.Max(0, dLck) * 0.0025f;

            // Physical Stature (1.10m .. 3.00m)
            float h = Mathf.Clamp(p.HeightMeters, MinHeightMeters, MaxHeightMeters);
            if (h < SmallNoMagicMax)
            {
                s.StatureClass = HeroStatureClass.SmallNoMagic;
                s.CanCastSpells = false;
                float t = Mathf.Clamp01((SmallNoMagicMax - h) / (SmallNoMagicMax - MinHeightMeters));
                s.MeleeDamageMultiplier *= Mathf.Lerp(0.72f, 0.52f, t);
                s.MeleeReachMultiplier *= Mathf.Lerp(0.84f, 0.72f, t);
                s.MaxHP *= Mathf.Lerp(0.85f, 0.70f, t);
                s.MoveSpeedMetersPerSec *= Mathf.Lerp(1.14f, 1.26f, t);
                s.TurnRateDegPerSec = Mathf.Lerp(720f, 860f, t);
                s.DodgeStaminaCostMultiplier *= 0.72f;
                s.DodgeIFrameBonusSec += 0.06f;
                s.StatureSummary = $"{h:F2}m · Small Stature (No Magic · +{((Mathf.Lerp(1.14f, 1.26f, t) - 1f) * 100f):F0}% Speed)";
            }
            else if (h > 2.25f)
            {
                s.StatureClass = HeroStatureClass.GiantTitan;
                s.CanCastSpells = true;
                float t = Mathf.Clamp01((h - 2.25f) / (MaxHeightMeters - 2.25f));
                s.MeleeDamageMultiplier *= Mathf.Lerp(1.25f, 1.78f, t);
                s.MeleeReachMultiplier *= Mathf.Lerp(1.18f, 1.48f, t);
                s.StaggerMultiplier *= Mathf.Lerp(1.30f, 1.90f, t);
                s.MaxHP *= Mathf.Lerp(1.22f, 1.68f, t);
                s.MoveSpeedMetersPerSec *= Mathf.Lerp(0.88f, 0.72f, t);
                s.AttackSpeedMultiplier *= Mathf.Lerp(0.88f, 0.70f, t);
                s.TurnRateDegPerSec = Mathf.Lerp(340f, 185f, t);
                s.AccelMultiplier = Mathf.Lerp(0.76f, 0.48f, t);
                s.DodgeDistanceMultiplier *= Mathf.Lerp(0.86f, 0.72f, t);
                s.DodgeStaminaCostMultiplier *= Mathf.Lerp(1.22f, 1.48f, t);
                s.SpellCooldownMultiplier *= Mathf.Lerp(1.06f, 1.18f, t);
                s.StatureSummary = $"{h:F2}m · Giant Titan (+{((Mathf.Lerp(1.25f, 1.78f, t) - 1f) * 100f):F0}% Melee · Heavy Slow Turn)";
            }
            else
            {
                s.StatureClass = HeroStatureClass.StandardBalanced;
                s.CanCastSpells = true;
                float nd = (h - BaseHeightMeters) / 0.40f;
                s.MeleeDamageMultiplier *= (1f + nd * 0.08f);
                s.MeleeReachMultiplier *= (1f + nd * 0.07f);
                s.MaxHP *= (1f + nd * 0.06f);
                s.MoveSpeedMetersPerSec *= (1f - nd * 0.04f);
                s.TurnRateDegPerSec = Mathf.Clamp(560f - nd * 90f, 430f, 680f);
                s.AccelMultiplier = Mathf.Clamp(1f - nd * 0.08f, 0.88f, 1.12f);
                s.StatureSummary = $"{h:F2}m · Standard Stature (Full Magic & Balanced Combat)";
            }

            float mDelta = Mathf.Clamp01(p.Musculature) - 0.5f;
            s.MeleeDamageMultiplier *= (1f + mDelta * 0.28f);
            s.MaxHP *= (1f + mDelta * 0.22f);
            s.MaxStamina *= (1f + mDelta * 0.14f);
            s.MoveSpeedMetersPerSec *= (1f - mDelta * 0.08f);

            float shDelta = Mathf.Clamp01(p.ShoulderWidth) - 0.5f;
            s.StaggerMultiplier *= (1f + shDelta * 0.10f);

            s.MaxHP = Mathf.Max(60f, s.MaxHP);
            s.MaxStamina = Mathf.Max(50f, s.MaxStamina);
            s.MaxMana = Mathf.Max(40f, s.MaxMana);
            s.HPRegenPerSec = Mathf.Max(0f, s.HPRegenPerSec);
            s.StaminaRegenPerSec = Mathf.Max(8f, s.StaminaRegenPerSec);
            s.ManaRegenPerSec = Mathf.Max(2f, s.ManaRegenPerSec);
            s.MeleeDamageMultiplier = Mathf.Clamp(s.MeleeDamageMultiplier, 0.35f, 4.00f);
            s.MeleeReachMultiplier = Mathf.Clamp(s.MeleeReachMultiplier, 0.65f, 1.75f);
            s.AttackSpeedMultiplier = Mathf.Clamp(s.AttackSpeedMultiplier, 0.55f, 1.85f);
            s.MoveSpeedMetersPerSec = Mathf.Clamp(s.MoveSpeedMetersPerSec, 3.6f, 10.2f);
            s.TurnRateDegPerSec = Mathf.Clamp(s.TurnRateDegPerSec, 160f, 920f);
            s.CritChance = Mathf.Clamp(s.CritChance, 0.01f, 0.75f);
            s.CritDamageMultiplier = Mathf.Clamp(s.CritDamageMultiplier, 1.20f, 3.50f);
            s.SpellCooldownMultiplier = Mathf.Clamp(s.SpellCooldownMultiplier, 0.45f, 1.50f);
            s.ManaCostMultiplier = Mathf.Clamp(s.ManaCostMultiplier, 0.50f, 1.40f);
            return s;
        }

        public void SetRace(HeroRace race, bool applyDefaults = true)
        {
            if (Profile == null) Profile = new HeroCustomizationProfile();
            Profile.Race = race;
            if (applyDefaults)
            {
                switch (race)
                {
                    case HeroRace.Human:        Profile.HeightMeters = 1.85f; Profile.Musculature = 0.52f; Profile.ShoulderWidth = 0.52f; break;
                    case HeroRace.HighElf:      Profile.HeightMeters = 1.98f; Profile.Musculature = 0.36f; Profile.ShoulderWidth = 0.44f; break;
                    case HeroRace.HighlandNord: Profile.HeightMeters = 2.42f; Profile.Musculature = 0.88f; Profile.ShoulderWidth = 0.86f; break;
                    case HeroRace.AshDraconid:  Profile.HeightMeters = 2.15f; Profile.Musculature = 0.76f; Profile.ShoulderWidth = 0.78f; break;
                    case HeroRace.ShadowKin:    Profile.HeightMeters = 1.36f; Profile.Musculature = 0.32f; Profile.ShoulderWidth = 0.38f; break;
                    case HeroRace.Ironborn:     Profile.HeightMeters = 1.66f; Profile.Musculature = 0.84f; Profile.ShoulderWidth = 0.86f; break;
                }
            }
            Recalculate();
        }

        public bool ModifyAttribute(HeroAttribute attr, int delta)
        {
            if (delta == 0) return false;
            if (Profile == null) Profile = new HeroCustomizationProfile();
            ref HeroAttributes a = ref Profile.Attributes;
            int val = GetBaseAttr(attr);
            if (delta > 0)
            {
                int add = Mathf.Min(delta, Mathf.Min(a.UnspentPoints, MaxAttr - val));
                if (add <= 0) return false;
                SetBaseAttr(attr, val + add);
                a.UnspentPoints -= add;
            }
            else
            {
                int sub = Mathf.Min(-delta, val - MinAttr);
                if (sub <= 0) return false;
                SetBaseAttr(attr, val - sub);
                a.UnspentPoints += sub;
            }
            Recalculate();
            return true;
        }

        public void GrantAttributePoints(int pts)
        {
            if (pts <= 0) return;
            Profile.Attributes.UnspentPoints = Mathf.Min(999, Profile.Attributes.UnspentPoints + pts);
            Recalculate();
        }

        public void ResetAttributes()
        {
            if (Profile == null) Profile = new HeroCustomizationProfile();
            ref HeroAttributes a = ref Profile.Attributes;
            int pool = a.Strength + a.Endurance + a.Agility + a.Intelligence + a.Spirit + a.Fortune + a.UnspentPoints;
            a.Strength = a.Endurance = a.Agility = a.Intelligence = a.Spirit = a.Fortune = 10;
            a.UnspentPoints = Mathf.Max(DefaultStartPoints, pool - 60);
            Recalculate();
        }

        public void ResetToDefault()
        {
            Profile = new HeroCustomizationProfile();
            Recalculate();
        }

        public int GetBaseAttr(HeroAttribute attr) => attr switch
        {
            HeroAttribute.Strength     => Profile.Attributes.Strength,
            HeroAttribute.Endurance    => Profile.Attributes.Endurance,
            HeroAttribute.Agility      => Profile.Attributes.Agility,
            HeroAttribute.Intelligence => Profile.Attributes.Intelligence,
            HeroAttribute.Spirit       => Profile.Attributes.Spirit,
            HeroAttribute.Fortune      => Profile.Attributes.Fortune,
            _                          => 10
        };

        private void SetBaseAttr(HeroAttribute attr, int v)
        {
            switch (attr)
            {
                case HeroAttribute.Strength:     Profile.Attributes.Strength = v; break;
                case HeroAttribute.Endurance:    Profile.Attributes.Endurance = v; break;
                case HeroAttribute.Agility:      Profile.Attributes.Agility = v; break;
                case HeroAttribute.Intelligence: Profile.Attributes.Intelligence = v; break;
                case HeroAttribute.Spirit:       Profile.Attributes.Spirit = v; break;
                case HeroAttribute.Fortune:      Profile.Attributes.Fortune = v; break;
            }
        }

        public void Recalculate()
        {
            if (Profile == null) Profile = new HeroCustomizationProfile();
            Profile.HeightMeters = Mathf.Clamp(Profile.HeightMeters, MinHeightMeters, MaxHeightMeters);
            Profile.Musculature = Mathf.Clamp01(Profile.Musculature);
            Profile.ShoulderWidth = Mathf.Clamp01(Profile.ShoulderWidth);
            int level = 1;
            float hpRatio = 1f;
            bool isNight = false;
            var hero = GetComponent<HeroController>();
            if (hero != null)
            {
                if (hero.SkillTree != null) level = Mathf.Max(1, hero.SkillTree.Level);
                if (hero.MaxHP > 1f) hpRatio = Mathf.Clamp01(hero.HP / hero.MaxHP);
            }
            var world = AetheriaWorldGenerator.Instance;
            if (world != null)
            {
                isNight = world.TimeOfDay < 0.22f || world.TimeOfDay > 0.80f;
            }
            Derived = ComputeDerivedStats(Profile, level, isNight, hpRatio);
            OnProfileChanged?.Invoke();
        }

        public static string GetHeroPrefabResourceName(HeroCustomizationProfile p)
        {
            if (p == null) return KnightHeroResource;
            if (p.Gender == HeroGender.Female)
            {
                return p.OutfitStyle switch
                {
                    HeroOutfitStyle.SovereignGilded  => "Heroes/Hero_SerathGold",
                    HeroOutfitStyle.ArchmageRobes    => "Heroes/Hero_MorigeshMystic",
                    HeroOutfitStyle.DraconidWarlock  => "Heroes/Hero_Morigesh",
                    HeroOutfitStyle.InquisitorCoat   => "Heroes/Hero_KallariLotus",
                    HeroOutfitStyle.HighlandPelt     => "Heroes/Hero_Kallari",
                    _                                => (p.Race == HeroRace.HighElf || p.Race == HeroRace.ShadowKin) ? "Heroes/Hero_SerathFrost" : "Heroes/Hero_Serath"
                };
            }

            if (p.HairStyle == HeroHairStyle.SamuraiManbun && p.OutfitStyle == HeroOutfitStyle.VanguardPlate)
                return "Heroes/Hero_KwangManbun";

            return p.OutfitStyle switch
            {
                HeroOutfitStyle.ArchmageRobes   => "Heroes/Hero_Gideon",
                HeroOutfitStyle.InquisitorCoat  => "Heroes/Hero_GideonInq",
                HeroOutfitStyle.DraconidWarlock => "Heroes/Hero_GideonMeph",
                HeroOutfitStyle.HighlandPelt    => p.Race == HeroRace.HighlandNord ? "Heroes/Hero_KhaimeraPelt" : "Heroes/Hero_Khaimera",
                HeroOutfitStyle.SovereignGilded => "Heroes/Hero_KwangSunrise",
                // Default look (Vanguard plate): a Human is the knight; the other races keep the Paragon bodies.
                _                               => p.Race == HeroRace.Human ? KnightHeroResource
                                                 : p.Race == HeroRace.HighElf ? "Heroes/Hero_KwangFrost"
                                                 : p.Race == HeroRace.ShadowKin ? "Heroes/Hero_KwangAlbino" : "Heroes/Hero_Kwang"
            };
        }

        public static string GetWeaponResourceName(HeroCustomizationProfile p)
        {
            if (p == null) return "Weapons/LongSword";
            return p.OutfitStyle switch
            {
                HeroOutfitStyle.ArchmageRobes   => "Weapons/ArcaneStaff",
                HeroOutfitStyle.DraconidWarlock => "Weapons/CrystalScepter",
                HeroOutfitStyle.InquisitorCoat  => "Weapons/DemonSword",
                HeroOutfitStyle.HighlandPelt    => "Weapons/Thornblade",
                _                               => "Weapons/LongSword"
            };
        }
    }
}
