// ============================================================================
// ChroniclesTypes.h — Shared Enums, Structs & Constants for AetheriaChronicles
// Combines Aetheria (Multiplayer, Reactions, 10 Biomes), wizardEPIC (Chaos
// Destruction, Charged Casts), and Fantasy (Open World, Dragon, Telegraphed Boss)
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "ChroniclesTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogChronicles, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogWizardsmoke, Log, All);

// Dedicated combat projectile collision channel (configured in DefaultEngine.ini)
#define CHRONICLE_TRACE_PROJECTILE ECC_GameTraceChannel1

/** Progressive Invoker-style primal spheres unlocked via story quests: Exort -> Wex -> Quas */
UENUM(BlueprintType)
enum class EChronicleOrb : uint8
{
	None  UMETA(DisplayName = "None"),
	Exort UMETA(DisplayName = "E - Exort (Solar Flame)"),
	Wex   UMETA(DisplayName = "W - Wex (Tempest Storm)"),
	Quas  UMETA(DisplayName = "Q - Quas (Chrono Frost)")
};

UENUM(BlueprintType)
enum class EChronicleCastMode : uint8
{
	ClassicInvoker UMETA(DisplayName = "Classic Invoker (E/W/Q -> R -> D/F)"),
	InstantCombo   UMETA(DisplayName = "Instant Cast on 3rd Orb / R")
};

/** 10 Classic Invoker Spells gated dynamically by unlocked spheres (1 -> 4 -> 10) */
UENUM(BlueprintType)
enum class EChronicleSpellId : uint8
{
	None            UMETA(DisplayName = "None"),
	// Unlocked with Exort alone (1 spell):
	SunStrike       UMETA(DisplayName = "EEE — Solar Cataclysm"),
	// Unlocked when Wex is added (4 spells total):
	ChaosMeteor     UMETA(DisplayName = "EEW — Chaos Meteor"),
	StormAlacrity   UMETA(DisplayName = "EWW — Tempest Surge"),
	StormEMP        UMETA(DisplayName = "WWW — Plasma EMP Nova"),
	// Unlocked when Quas is added (all 10 spells):
	ColdSnap        UMETA(DisplayName = "QQQ — Glacial Cold Snap"),
	ChronoBlink     UMETA(DisplayName = "QQW — Chrono-Ghost Walk"),
	IcePrismWall    UMETA(DisplayName = "QQE — Cryo-Prism Wall"),
	GravityTornado  UMETA(DisplayName = "QWW — Tempest Tornado"),
	EmberConduit    UMETA(DisplayName = "QEE — Forge Lava Conduit"),
	PrimalBlast     UMETA(DisplayName = "QWE — Primal Deafening Blast")
};

UENUM(BlueprintType)
enum class EChronicleDelivery : uint8
{
	Projectile,
	OrbitalStrike,
	RollingMeteor,
	SelfBuff,
	RadialNova,
	BarrierWall,
	ConeWave
};

UENUM(BlueprintType)
enum class EChronicleRenderTier : uint8
{
	CrimsonRTXUltra    UMETA(DisplayName = "Crimson Desert RTX (HWRT + Lumen + DLSS/TSR)"),
	ClassicPerformance UMETA(DisplayName = "Standard Performance Mode (No RTX Hardware)")
};

UENUM(BlueprintType)
enum class ESkillBranch : uint8
{
	ExortPath      UMETA(DisplayName = "Path of Exort"),
	WexPath        UMETA(DisplayName = "Path of Wex"),
	QuasPath       UMETA(DisplayName = "Path of Quas"),
	BladeAndDrake  UMETA(DisplayName = "Way of Blade & Drake")
};

UENUM(BlueprintType)
enum class EChronicleBiome : uint8
{
	OakhavenValley  UMETA(DisplayName = "Golden Oakhaven Valley"),
	BlueLeafWoods   UMETA(DisplayName = "Sapphire Whispering Woods"),
	HighkeepCitadel UMETA(DisplayName = "Highkeep Citadel"),
	AshCalderaCrypt UMETA(DisplayName = "Ash Caldera & Ember Crypt"),
	CrystalGlacier  UMETA(DisplayName = "Crystal Glacier Peaks")
};

UENUM(BlueprintType)
enum class EQuestState : uint8
{
	Available,
	Active,
	ReadyToTurnIn,
	Completed
};

UENUM(BlueprintType)
enum class EDestructibleKind : uint8
{
	Stone,
	Wood,
	Crystal,
	Obsidian,
	Foliage
};

UENUM(BlueprintType)
enum class EBossAttackKind : uint8
{
	None,
	CrimsonCleave,
	SeismicSlam,
	AbyssalFireVolley,
	TempestLunge,
	ShatterSpin,
	ChronoCataclysm
};

USTRUCT(BlueprintType)
struct FChronicleSpellDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EChronicleSpellId SpellId = EChronicleSpellId::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString ComboKey; // Sorted canonical key e.g. "EEE", "EEW", "QWE"

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EChronicleDelivery Delivery = EChronicleDelivery::Projectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bNeedsExort = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bNeedsWex = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bNeedsQuas = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ManaCost = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Cooldown = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BaseDamage = 65.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BlastRadius = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DestructionImpulse = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor PrimaryColor = FLinearColor(1.0f, 0.35f, 0.08f, 1.0f);
};

USTRUCT(BlueprintType)
struct FSkillNodeDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESkillBranch Branch = ESkillBranch::ExortPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Tier = 1; // 1..4

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PrerequisiteNodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxRank = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CurrentRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ValuePerRank = 0.12f;
};

USTRUCT(BlueprintType)
struct FQuestDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName QuestId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName GiverId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText GiverName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Summary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText ObjectiveText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText OfferDialogue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText TurnInDialogue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EQuestState State = EQuestState::Available;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Progress = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TargetCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardGold = 150;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardXP = 220;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EChronicleOrb UnlockOrbReward = EChronicleOrb::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUnlocksDragonMount = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector ObjectiveWorldLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FMapPinInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PinId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EChronicleBiome Biome = EChronicleBiome::OakhavenValley;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor Color = FLinearColor(0.95f, 0.78f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDiscovered = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bFastTravel = false;
};

USTRUCT(BlueprintType)
struct FGraphicsTelemetry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString GPUAdapterName;

	UPROPERTY(BlueprintReadOnly)
	EChronicleRenderTier ActiveTier = EChronicleRenderTier::ClassicPerformance;

	UPROPERTY(BlueprintReadOnly)
	bool bHardwareRTXSupported = false;

	UPROPERTY(BlueprintReadOnly)
	bool bHardwareRTXActive = false;

	UPROPERTY(BlueprintReadOnly)
	bool bDLSSPluginDetected = false;

	UPROPERTY(BlueprintReadOnly)
	bool bDLSSActive = false;

	UPROPERTY(BlueprintReadOnly)
	bool bReflexActive = false;

	UPROPERTY(BlueprintReadOnly)
	int32 TexturePoolMB = 2048;

	UPROPERTY(BlueprintReadOnly)
	FString SummaryBanner;
};

// ============================================================================
// Detailed Hero Customization, Playable Races, Physique/Stature & RPG Stats
// ============================================================================

UENUM(BlueprintType)
enum class EHeroGender : uint8
{
	Male   UMETA(DisplayName = "Male"),
	Female UMETA(DisplayName = "Female")
};

UENUM(BlueprintType)
enum class EHeroRace : uint8
{
	Human        UMETA(DisplayName = "Human (Valenarian)"),
	HighElf      UMETA(DisplayName = "High Elf (Aetherian)"),
	HighlandNord UMETA(DisplayName = "Highland Nord"),
	AshDraconid  UMETA(DisplayName = "Ash Draconid"),
	ShadowKin    UMETA(DisplayName = "Shadow Kin (Umbra)"),
	Ironborn     UMETA(DisplayName = "Ironborn Runesmith")
};

UENUM(BlueprintType)
enum class EHeroHairStyle : uint8
{
	ShortKnight    UMETA(DisplayName = "Short Knight"),
	SamuraiManbun  UMETA(DisplayName = "Samurai Topknot"),
	LongFlowing    UMETA(DisplayName = "Long Flowing"),
	BraidedWarrior UMETA(DisplayName = "Braided Warrior"),
	BattleMohawk   UMETA(DisplayName = "Highland Crest"),
	ArchmageHood   UMETA(DisplayName = "Arcane Hood"),
	CrownCirclet   UMETA(DisplayName = "Sovereign Crown"),
	BaldShaved     UMETA(DisplayName = "Shaved / Bald")
};

UENUM(BlueprintType)
enum class EHeroMustacheStyle : uint8
{
	None           UMETA(DisplayName = "None"),
	NobleChevron   UMETA(DisplayName = "Noble Chevron"),
	Handlebar      UMETA(DisplayName = "Handlebar"),
	ImperialWalrus UMETA(DisplayName = "Imperial Walrus"),
	HighlandForked UMETA(DisplayName = "Highland Forked"),
	LightStubble   UMETA(DisplayName = "Light Stubble")
};

UENUM(BlueprintType)
enum class EHeroBeardStyle : uint8
{
	None          UMETA(DisplayName = "Clean Shaven"),
	Stubble       UMETA(DisplayName = "Rugged Stubble"),
	ShortBoxed    UMETA(DisplayName = "Short Boxed"),
	RangerFull    UMETA(DisplayName = "Ranger Full"),
	VikingBraided UMETA(DisplayName = "Nordic Braided"),
	ArchmageLong  UMETA(DisplayName = "Archmage Long"),
	Goatee        UMETA(DisplayName = "Royal Goatee")
};

UENUM(BlueprintType)
enum class EHeroEyeStyle : uint8
{
	SteelBlue    UMETA(DisplayName = "Steel Blue"),
	MoltenAmber  UMETA(DisplayName = "Molten Amber"),
	ArcaneCyan   UMETA(DisplayName = "Arcane Cyan"),
	EmeraldJade  UMETA(DisplayName = "Emerald Jade"),
	CrimsonFlame UMETA(DisplayName = "Crimson Flame"),
	VoidViolet   UMETA(DisplayName = "Void Violet"),
	SilverOracle UMETA(DisplayName = "Silver Oracle"),
	SolarGold    UMETA(DisplayName = "Solar Gold")
};

UENUM(BlueprintType)
enum class EHeroTattooStyle : uint8
{
	None          UMETA(DisplayName = "None"),
	NordicRunes   UMETA(DisplayName = "Nordic War Runes"),
	DragonScales  UMETA(DisplayName = "Draconid Scale Marks"),
	ArcaneSigils  UMETA(DisplayName = "Arcane Invocation Sigils"),
	CrimsonScar   UMETA(DisplayName = "Crimson Battle Scars"),
	TribalAsh     UMETA(DisplayName = "Ash Caldera Warpaint"),
	CelestialHalo UMETA(DisplayName = "Celestial Crown Marks")
};

UENUM(BlueprintType)
enum class EHeroOutfitStyle : uint8
{
	VanguardPlate   UMETA(DisplayName = "Vanguard Plate"),
	ArchmageRobes   UMETA(DisplayName = "Archmage Vestments"),
	InquisitorCoat  UMETA(DisplayName = "Royal Inquisitor"),
	DraconidWarlock UMETA(DisplayName = "Ash Draconid Mail"),
	HighlandPelt    UMETA(DisplayName = "Highland Berserker"),
	SovereignGilded UMETA(DisplayName = "Sovereign Gilded Armor")
};

UENUM(BlueprintType)
enum class EHeroDetailQuality : uint8
{
	Low          UMETA(DisplayName = "Low (Performance)"),
	Medium       UMETA(DisplayName = "Medium"),
	High         UMETA(DisplayName = "High"),
	Ultra        UMETA(DisplayName = "Ultra PBR"),
	CinematicRTX UMETA(DisplayName = "Cinematic RTX")
};

UENUM(BlueprintType)
enum class EHeroAttribute : uint8
{
	Strength     UMETA(DisplayName = "Strength"),
	Endurance    UMETA(DisplayName = "Endurance"),
	Agility      UMETA(DisplayName = "Agility"),
	Intelligence UMETA(DisplayName = "Intelligence"),
	Spirit       UMETA(DisplayName = "Spirit"),
	Fortune      UMETA(DisplayName = "Fortune")
};

UENUM(BlueprintType)
enum class EHeroStatureClass : uint8
{
	SmallNoMagic     UMETA(DisplayName = "Small (<1.45m · No Magic · Agile)"),
	StandardBalanced UMETA(DisplayName = "Standard (1.45m-2.25m · Balanced)"),
	GiantTitan       UMETA(DisplayName = "Giant (2.26m-3.00m · Titan Power · Slow Turn)")
};

USTRUCT(BlueprintType)
struct FHeroAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 Strength = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 Endurance = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 Agility = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 Intelligence = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 Spirit = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 Fortune = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 UnspentPoints = 15;
};

USTRUCT(BlueprintType)
struct FHeroCustomizationProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroGender Gender = EHeroGender::Male;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroRace Race = EHeroRace::Human;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroHairStyle HairStyle = EHeroHairStyle::ShortKnight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 HairColorIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroMustacheStyle MustacheStyle = EHeroMustacheStyle::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroBeardStyle BeardStyle = EHeroBeardStyle::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroEyeStyle EyeStyle = EHeroEyeStyle::MoltenAmber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float EyeGlowIntensity = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroTattooStyle TattooStyle = EHeroTattooStyle::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 TattooColorIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroOutfitStyle OutfitStyle = EHeroOutfitStyle::VanguardPlate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int32 ArmorTintIndex = 0;

	/** Physical stature in meters: 1.10m (tiny, no magic) -> 1.85m (standard) -> 3.00m (giant titan). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float HeightMeters = 1.85f;

	/** Musculature / body mass 0.0 (lean) .. 1.0 (titanic bulk). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float Musculature = 0.50f;

	/** Shoulder & upper frame breadth 0.0 .. 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float ShoulderWidth = 0.50f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	EHeroDetailQuality DetailQuality = EHeroDetailQuality::Ultra;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FHeroAttributes Attributes;
};

USTRUCT(BlueprintType)
struct FHeroDerivedStats
{
	GENERATED_BODY()

	// Effective attributes after Racial bonuses
	UPROPERTY(BlueprintReadOnly) int32 EffStrength = 10;
	UPROPERTY(BlueprintReadOnly) int32 EffEndurance = 10;
	UPROPERTY(BlueprintReadOnly) int32 EffAgility = 10;
	UPROPERTY(BlueprintReadOnly) int32 EffIntelligence = 10;
	UPROPERTY(BlueprintReadOnly) int32 EffSpirit = 10;
	UPROPERTY(BlueprintReadOnly) int32 EffFortune = 10;

	// Vitals & Regen
	UPROPERTY(BlueprintReadOnly) float MaxHP = 120.f;
	UPROPERTY(BlueprintReadOnly) float MaxStamina = 100.f;
	UPROPERTY(BlueprintReadOnly) float MaxMana = 120.f;
	UPROPERTY(BlueprintReadOnly) float HPRegenPerSec = 0.5f;
	UPROPERTY(BlueprintReadOnly) float StaminaRegenPerSec = 22.f;
	UPROPERTY(BlueprintReadOnly) float ManaRegenPerSec = 9.f;

	// Melee, Movement & Stature Physics
	UPROPERTY(BlueprintReadOnly) float MeleeDamageMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float MeleeReachMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float AttackSpeedMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float MoveSpeedCmPerSec = 580.f;
	UPROPERTY(BlueprintReadOnly) float TurnRateDegPerSec = 560.f;
	UPROPERTY(BlueprintReadOnly) float AccelMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float StaggerMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float DestructionDamageMultiplier = 1.0f;

	// Magic & Spells
	UPROPERTY(BlueprintReadOnly) bool bCanCastSpells = true;
	UPROPERTY(BlueprintReadOnly) float SpellPowerMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float ExortBonusMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float ManaCostMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float SpellCooldownMultiplier = 1.0f;

	// Crits, Defense, Parry & Dodge
	UPROPERTY(BlueprintReadOnly) float CritChance = 0.08f;
	UPROPERTY(BlueprintReadOnly) float CritDamageMultiplier = 1.50f;
	UPROPERTY(BlueprintReadOnly) float ArmorReduction = 0.05f;
	UPROPERTY(BlueprintReadOnly) float FireResistance = 0.0f;
	UPROPERTY(BlueprintReadOnly) float KnockbackResistance = 0.0f;
	UPROPERTY(BlueprintReadOnly) float ParryWindowBonusSec = 0.0f;
	UPROPERTY(BlueprintReadOnly) float ParryStaggerBonusMult = 1.0f;
	UPROPERTY(BlueprintReadOnly) float BlockStaminaDrainMult = 1.0f;
	UPROPERTY(BlueprintReadOnly) float DodgeDistanceMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float DodgeStaminaCostMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float DodgeIFrameBonusSec = 0.0f;

	// Dragon, World & Progression
	UPROPERTY(BlueprintReadOnly) float DragonSpeedMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float DragonFireMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float DragonFireDrainMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float XPAndGoldMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float MapRevealMultiplier = 1.0f;

	// Active Racial Flags & Classification
	UPROPERTY(BlueprintReadOnly) bool bColdSlowImmune = false;
	UPROPERTY(BlueprintReadOnly) bool bBerserkLowHPActive = false;
	UPROPERTY(BlueprintReadOnly) bool bNightShadowCritActive = false;
	UPROPERTY(BlueprintReadOnly) EHeroStatureClass StatureClass = EHeroStatureClass::StandardBalanced;
	UPROPERTY(BlueprintReadOnly) FString StatureSummary;
	UPROPERTY(BlueprintReadOnly) FString RacePassiveSummary;
};
