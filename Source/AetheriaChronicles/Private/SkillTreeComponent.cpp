// ============================================================================
// SkillTreeComponent.cpp — 24-Node 4-Branch RPG Skill Tree, Multi-Level XP,
// Replicated Progression & UChroniclesSaveGame Persistence
// ============================================================================
#include "SkillTreeComponent.h"

#include "HeroCustomizationComponent.h"
#include "HeroDragonBoss.h"

#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "AetheriaSkillTree"

namespace ChroniclesSkillNodes
{
	// Function-local static FNames avoid pre-engine DLL static initialization hazards.
	static const FName& ExortSolarCore()        { static const FName Id(TEXT("exort_solar_core"));        return Id; }
	static const FName& ExortWildfire()         { static const FName Id(TEXT("exort_wildfire"));          return Id; }
	static const FName& ExortArmorMelt()        { static const FName Id(TEXT("exort_armor_melt"));        return Id; }
	static const FName& ExortChaosShatter()     { static const FName Id(TEXT("exort_chaos_shatter"));     return Id; }
	static const FName& ExortMeteorMass()       { static const FName Id(TEXT("exort_meteor_mass"));       return Id; }
	static const FName& ExortCataclysmMaster()  { static const FName Id(TEXT("exort_cataclysm_master"));  return Id; }

	static const FName& WexSwiftInvocation()    { static const FName Id(TEXT("wex_swift_invocation"));    return Id; }
	static const FName& WexArcConduit()         { static const FName Id(TEXT("wex_arc_conduit"));         return Id; }
	static const FName& WexPhantomStep()        { static const FName Id(TEXT("wex_phantom_step"));        return Id; }
	static const FName& WexDrakeGale()          { static const FName Id(TEXT("wex_drake_gale"));          return Id; }
	static const FName& WexStormMana()          { static const FName Id(TEXT("wex_storm_mana"));          return Id; }
	static const FName& WexTempestLord()        { static const FName Id(TEXT("wex_tempest_lord"));        return Id; }

	static const FName& QuasVitalFrost()        { static const FName Id(TEXT("quas_vital_frost"));        return Id; }
	static const FName& QuasChronoHold()        { static const FName Id(TEXT("quas_chrono_hold"));        return Id; }
	static const FName& QuasStaggerBreaker()    { static const FName Id(TEXT("quas_stagger_breaker"));    return Id; }
	static const FName& QuasGlacialAegis()      { static const FName Id(TEXT("quas_glacial_aegis"));      return Id; }
	static const FName& QuasThermalShock()      { static const FName Id(TEXT("quas_thermal_shock"));      return Id; }
	static const FName& QuasTimelessSovereign() { static const FName Id(TEXT("quas_timeless_sovereign")); return Id; }

	static const FName& BladeKeenEdge()         { static const FName Id(TEXT("blade_keen_edge"));         return Id; }
	static const FName& BladeIronStamina()      { static const FName Id(TEXT("blade_iron_stamina"));      return Id; }
	static const FName& BladePerfectParry()     { static const FName Id(TEXT("blade_perfect_parry"));     return Id; }
	static const FName& BladeExecutioner()      { static const FName Id(TEXT("blade_executioner"));       return Id; }
	static const FName& BladeDragonfireMastery(){ static const FName Id(TEXT("blade_dragonfire_mastery"));return Id; }
	static const FName& BladeWukongStance()     { static const FName Id(TEXT("blade_wukong_stance"));     return Id; }

	static FSkillNodeDef MakeNode(
		const FName& NodeId,
		ESkillBranch Branch,
		int32 Tier,
		const FText& Title,
		const FText& Description,
		const FName& PrerequisiteNodeId,
		int32 MaxRank,
		float ValuePerRank)
	{
		FSkillNodeDef Def;
		Def.NodeId = NodeId;
		Def.Branch = Branch;
		Def.Tier = Tier;
		Def.Title = Title;
		Def.Description = Description;
		Def.PrerequisiteNodeId = PrerequisiteNodeId;
		Def.MaxRank = MaxRank;
		Def.CurrentRank = 0;
		Def.ValuePerRank = ValuePerRank;
		return Def;
	}
}

// ============================================================================
// UChroniclesSaveGame
// ============================================================================

UChroniclesSaveGame::UChroniclesSaveGame()
{
	FogOfWarGrid.Init(0, FogOfWarGridSize);
}

// ============================================================================
// USkillTreeComponent
// ============================================================================

USkillTreeComponent::USkillTreeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	BuildDefaultNodes();
}

void USkillTreeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Nodes.IsEmpty())
	{
		BuildDefaultNodes();
	}
}

void USkillTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USkillTreeComponent, Level);
	DOREPLIFETIME(USkillTreeComponent, CurrentXP);
	DOREPLIFETIME(USkillTreeComponent, UnspentSkillPoints);
	DOREPLIFETIME(USkillTreeComponent, Gold);
	DOREPLIFETIME(USkillTreeComponent, Nodes);
}

void USkillTreeComponent::BuildDefaultNodes()
{
	using namespace ChroniclesSkillNodes;

	Nodes.Reset(24);

	// ------------------------------------------------------------------------
	// Branch 1: Path of Exort (6 nodes across Tiers 1..4)
	// ------------------------------------------------------------------------
	Nodes.Add(MakeNode(
		ExortSolarCore(),
		ESkillBranch::ExortPath,
		1,
		LOCTEXT("ExortSolarCoreTitle", "Solar Core"),
		LOCTEXT("ExortSolarCoreDesc", "+14% Exort spell damage per rank."),
		NAME_None,
		3,
		0.14f));

	Nodes.Add(MakeNode(
		ExortWildfire(),
		ESkillBranch::ExortPath,
		2,
		LOCTEXT("ExortWildfireTitle", "Wildfire"),
		LOCTEXT("ExortWildfireDesc", "Burn ticks spread Wildfire & +12% DoT per rank."),
		ExortSolarCore(),
		3,
		0.12f));

	Nodes.Add(MakeNode(
		ExortArmorMelt(),
		ESkillBranch::ExortPath,
		2,
		LOCTEXT("ExortArmorMeltTitle", "Armor Melt"),
		LOCTEXT("ExortArmorMeltDesc", "Fire hits melt Boss armor (+8% vulnerability per rank)."),
		ExortSolarCore(),
		3,
		0.08f));

	Nodes.Add(MakeNode(
		ExortChaosShatter(),
		ESkillBranch::ExortPath,
		3,
		LOCTEXT("ExortChaosShatterTitle", "Chaos Shatter"),
		LOCTEXT("ExortChaosShatterDesc", "+35% Chaos Destruction impulse & debris radius per rank."),
		ExortWildfire(),
		2,
		0.35f));

	Nodes.Add(MakeNode(
		ExortMeteorMass(),
		ESkillBranch::ExortPath,
		3,
		LOCTEXT("ExortMeteorMassTitle", "Meteor Mass"),
		LOCTEXT("ExortMeteorMassDesc", "+22% Meteor & SunStrike blast radius per rank."),
		ExortArmorMelt(),
		2,
		0.22f));

	Nodes.Add(MakeNode(
		ExortCataclysmMaster(),
		ESkillBranch::ExortPath,
		4,
		LOCTEXT("ExortCataclysmMasterTitle", "Cataclysm Master"),
		LOCTEXT("ExortCataclysmMasterDesc", "Capstone: +35% Solar critical burst."),
		ExortMeteorMass(),
		1,
		0.35f));

	// ------------------------------------------------------------------------
	// Branch 2: Path of Wex (6 nodes across Tiers 1..4)
	// ------------------------------------------------------------------------
	Nodes.Add(MakeNode(
		WexSwiftInvocation(),
		ESkillBranch::WexPath,
		1,
		LOCTEXT("WexSwiftInvocationTitle", "Swift Invocation"),
		LOCTEXT("WexSwiftInvocationDesc", "-10% spell cooldowns & faster cast speed per rank."),
		NAME_None,
		3,
		0.10f));

	Nodes.Add(MakeNode(
		WexArcConduit(),
		ESkillBranch::WexPath,
		2,
		LOCTEXT("WexArcConduitTitle", "Arc Conduit"),
		LOCTEXT("WexArcConduitDesc", "+15% Storm & Lightning damage per rank."),
		WexSwiftInvocation(),
		3,
		0.15f));

	Nodes.Add(MakeNode(
		WexPhantomStep(),
		ESkillBranch::WexPath,
		2,
		LOCTEXT("WexPhantomStepTitle", "Phantom Step"),
		LOCTEXT("WexPhantomStepDesc", "+14% sprint & dodge-roll distance per rank."),
		WexSwiftInvocation(),
		3,
		0.14f));

	Nodes.Add(MakeNode(
		WexDrakeGale(),
		ESkillBranch::WexPath,
		3,
		LOCTEXT("WexDrakeGaleTitle", "Drake Gale"),
		LOCTEXT("WexDrakeGaleDesc", "+20% Dragon flight boost speed & fire recharge per rank."),
		WexPhantomStep(),
		2,
		0.20f));

	Nodes.Add(MakeNode(
		WexStormMana(),
		ESkillBranch::WexPath,
		3,
		LOCTEXT("WexStormManaTitle", "Storm Mana"),
		LOCTEXT("WexStormManaDesc", "+20 Max Mana & +15% Mana regen per rank."),
		WexArcConduit(),
		2,
		20.0f));

	Nodes.Add(MakeNode(
		WexTempestLord(),
		ESkillBranch::WexPath,
		4,
		LOCTEXT("WexTempestLordTitle", "Tempest Lord"),
		LOCTEXT("WexTempestLordDesc", "Capstone: Superconduct reaction deals +45% shock."),
		WexStormMana(),
		1,
		0.45f));

	// ------------------------------------------------------------------------
	// Branch 3: Path of Quas (6 nodes across Tiers 1..4)
	// ------------------------------------------------------------------------
	Nodes.Add(MakeNode(
		QuasVitalFrost(),
		ESkillBranch::QuasPath,
		1,
		LOCTEXT("QuasVitalFrostTitle", "Vital Frost"),
		LOCTEXT("QuasVitalFrostDesc", "+25 Max HP & +1.5 HP/s regen per rank."),
		NAME_None,
		3,
		25.0f));

	Nodes.Add(MakeNode(
		QuasChronoHold(),
		ESkillBranch::QuasPath,
		2,
		LOCTEXT("QuasChronoHoldTitle", "Chrono Hold"),
		LOCTEXT("QuasChronoHoldDesc", "+20% Freeze / Slow duration per rank."),
		QuasVitalFrost(),
		3,
		0.20f));

	Nodes.Add(MakeNode(
		QuasStaggerBreaker(),
		ESkillBranch::QuasPath,
		2,
		LOCTEXT("QuasStaggerBreakerTitle", "Stagger Breaker"),
		LOCTEXT("QuasStaggerBreakerDesc", "+18% Boss StaggerGauge buildup per rank."),
		QuasVitalFrost(),
		3,
		0.18f));

	Nodes.Add(MakeNode(
		QuasGlacialAegis(),
		ESkillBranch::QuasPath,
		3,
		LOCTEXT("QuasGlacialAegisTitle", "Glacial Aegis"),
		LOCTEXT("QuasGlacialAegisDesc", "Shield block reduces 10% more damage per rank & chills attackers."),
		QuasChronoHold(),
		2,
		0.10f));

	Nodes.Add(MakeNode(
		QuasThermalShock(),
		ESkillBranch::QuasPath,
		3,
		LOCTEXT("QuasThermalShockTitle", "Thermal Shock"),
		LOCTEXT("QuasThermalShockDesc", "+30% Thermal Shock & Cryo-Shatter damage per rank."),
		QuasStaggerBreaker(),
		2,
		0.30f));

	Nodes.Add(MakeNode(
		QuasTimelessSovereign(),
		ESkillBranch::QuasPath,
		4,
		LOCTEXT("QuasTimelessSovereignTitle", "Timeless Sovereign"),
		LOCTEXT("QuasTimelessSovereignDesc", "Capstone: +1.2s Boss Stagger Break vulnerability window."),
		QuasThermalShock(),
		1,
		1.2f));

	// ------------------------------------------------------------------------
	// Branch 4: Way of Blade & Drake (6 nodes across Tiers 1..4)
	// ------------------------------------------------------------------------
	Nodes.Add(MakeNode(
		BladeKeenEdge(),
		ESkillBranch::BladeAndDrake,
		1,
		LOCTEXT("BladeKeenEdgeTitle", "Keen Edge"),
		LOCTEXT("BladeKeenEdgeDesc", "+15% Sword melee combo damage per rank."),
		NAME_None,
		3,
		0.15f));

	Nodes.Add(MakeNode(
		BladeIronStamina(),
		ESkillBranch::BladeAndDrake,
		2,
		LOCTEXT("BladeIronStaminaTitle", "Iron Stamina"),
		LOCTEXT("BladeIronStaminaDesc", "+20 Max Stamina & +18% Stamina regen per rank."),
		BladeKeenEdge(),
		3,
		20.0f));

	Nodes.Add(MakeNode(
		BladePerfectParry(),
		ESkillBranch::BladeAndDrake,
		2,
		LOCTEXT("BladePerfectParryTitle", "Perfect Parry"),
		LOCTEXT("BladePerfectParryDesc", "Timed parry restores 22 Mana per rank & staggers enemies."),
		BladeKeenEdge(),
		3,
		22.0f));

	Nodes.Add(MakeNode(
		BladeExecutioner(),
		ESkillBranch::BladeAndDrake,
		3,
		LOCTEXT("BladeExecutionerTitle", "Executioner"),
		LOCTEXT("BladeExecutionerDesc", "+28% melee finisher damage vs staggered foes per rank."),
		BladePerfectParry(),
		2,
		0.28f));

	Nodes.Add(MakeNode(
		BladeDragonfireMastery(),
		ESkillBranch::BladeAndDrake,
		3,
		LOCTEXT("BladeDragonfireMasteryTitle", "Dragonfire Mastery"),
		LOCTEXT("BladeDragonfireMasteryDesc", "+25% Dragon Fire Breath DPS & range per rank."),
		BladeIronStamina(),
		2,
		0.25f));

	Nodes.Add(MakeNode(
		BladeWukongStance(),
		ESkillBranch::BladeAndDrake,
		4,
		LOCTEXT("BladeWukongStanceTitle", "Wukong Stance"),
		LOCTEXT("BladeWukongStanceDesc", "Capstone: +25% Dodge i-frames & critical riposte strike."),
		BladeExecutioner(),
		1,
		0.25f));
}

// ============================================================================
// Progression & Skill Node Mutations
// ============================================================================

int32 USkillTreeComponent::GetXPForNextLevel() const
{
	const float SafeLevel = static_cast<float>(FMath::Max(1, Level));
	return FMath::Max(1, FMath::RoundToInt(120.f * FMath::Pow(SafeLevel, 1.6f)));
}

void USkillTreeComponent::AddXP(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	if (const AActor* Owner = GetOwner())
	{
		if (!Owner->HasAuthority())
		{
			return;
		}
	}

	AAetheriaHeroCharacter* HeroOwner = Cast<AAetheriaHeroCharacter>(GetOwner());
	int32 EffectiveAmount = Amount;
	if (HeroOwner && HeroOwner->Customization)
	{
		const float XPMult = FMath::Max(0.1f, HeroOwner->Customization->GetDerivedStats().XPAndGoldMultiplier);
		EffectiveAmount = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Amount) * XPMult));
	}

	CurrentXP += EffectiveAmount;

	int32 LevelsGained = 0;
	constexpr int32 MaxPlayerLevel = 999;

	while (Level < MaxPlayerLevel)
	{
		const int32 RequiredXP = GetXPForNextLevel();
		if (CurrentXP < RequiredXP)
		{
			break;
		}

		CurrentXP -= RequiredXP;
		++Level;
		++UnspentSkillPoints;
		++LevelsGained;

		if (HeroOwner && HeroOwner->Customization)
		{
			HeroOwner->Customization->GrantAttributePoints(3);
		}

		UE_LOG(LogChronicles, Log, TEXT("[SkillTree] Level Up! Level=%d UnspentSkillPoints=%d (+3 Attribute Points) RemainingXP=%d/%d"),
			Level, UnspentSkillPoints, CurrentXP, GetXPForNextLevel());

		OnLevelUp.Broadcast(Level, UnspentSkillPoints);
	}

	if (LevelsGained == 0)
	{
		UE_LOG(LogChronicles, Verbose, TEXT("[SkillTree] +%d XP (%d/%d, Level %d)"),
			EffectiveAmount, CurrentXP, GetXPForNextLevel(), Level);
	}

	OnSkillTreeChanged.Broadcast();
}

void USkillTreeComponent::AddGold(int32 Amount)
{
	if (Amount == 0)
	{
		return;
	}

	if (const AActor* Owner = GetOwner())
	{
		if (!Owner->HasAuthority())
		{
			return;
		}
	}

	int32 EffectiveAmount = Amount;
	if (Amount > 0)
	{
		if (const AAetheriaHeroCharacter* HeroOwner = Cast<AAetheriaHeroCharacter>(GetOwner()))
		{
			if (HeroOwner->Customization)
			{
				const float GoldMult = FMath::Max(0.1f, HeroOwner->Customization->GetDerivedStats().XPAndGoldMultiplier);
				EffectiveAmount = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Amount) * GoldMult));
			}
		}
	}

	Gold = FMath::Max(0, Gold + EffectiveAmount);
	OnSkillTreeChanged.Broadcast();
}

bool USkillTreeComponent::CanUpgradeNode(FName NodeId) const
{
	if (UnspentSkillPoints <= 0 || NodeId.IsNone())
	{
		return false;
	}

	const FSkillNodeDef* Node = FindNode(NodeId);
	if (!Node)
	{
		return false;
	}

	if (Node->CurrentRank >= Node->MaxRank)
	{
		return false;
	}

	if (!Node->PrerequisiteNodeId.IsNone())
	{
		if (GetNodeRank(Node->PrerequisiteNodeId) <= 0)
		{
			return false;
		}
	}

	return true;
}

bool USkillTreeComponent::UpgradeNode(FName NodeId)
{
	if (!CanUpgradeNode(NodeId))
	{
		return false;
	}

	if (const AActor* Owner = GetOwner())
	{
		if (!Owner->HasAuthority())
		{
			ServerUpgradeNode(NodeId);
			return true;
		}
	}

	FSkillNodeDef* Node = FindNodeMutable(NodeId);
	if (!Node)
	{
		return false;
	}

	++Node->CurrentRank;
	--UnspentSkillPoints;

	UE_LOG(LogChronicles, Log, TEXT("[SkillTree] Upgraded '%s' to Rank %d/%d (Points left: %d)"),
		*Node->NodeId.ToString(), Node->CurrentRank, Node->MaxRank, UnspentSkillPoints);

	OnSkillNodeUpgraded.Broadcast(Node->NodeId, Node->CurrentRank);
	OnSkillTreeChanged.Broadcast();
	return true;
}

void USkillTreeComponent::ServerUpgradeNode_Implementation(FName NodeId)
{
	UpgradeNode(NodeId);
}

int32 USkillTreeComponent::GetNodeRank(FName NodeId) const
{
	if (const FSkillNodeDef* Node = FindNode(NodeId))
	{
		return Node->CurrentRank;
	}
	return 0;
}

TArray<FSkillNodeDef> USkillTreeComponent::GetNodesForBranch(ESkillBranch Branch) const
{
	TArray<FSkillNodeDef> BranchNodes;
	BranchNodes.Reserve(6);

	for (const FSkillNodeDef& Node : Nodes)
	{
		if (Node.Branch == Branch)
		{
			BranchNodes.Add(Node);
		}
	}

	return BranchNodes;
}

int32 USkillTreeComponent::GetTotalInvestedRanks() const
{
	int32 Total = 0;
	for (const FSkillNodeDef& Node : Nodes)
	{
		Total += Node.CurrentRank;
	}
	return Total;
}

// ============================================================================
// Computed Stat Getters
// ============================================================================

float USkillTreeComponent::GetMaxHPBonus() const
{
	return static_cast<float>(GetNodeRank(ChroniclesSkillNodes::QuasVitalFrost())) * 25.0f;
}

float USkillTreeComponent::GetMaxManaBonus() const
{
	return static_cast<float>(GetNodeRank(ChroniclesSkillNodes::WexStormMana())) * 20.0f;
}

float USkillTreeComponent::GetMaxStaminaBonus() const
{
	return static_cast<float>(GetNodeRank(ChroniclesSkillNodes::BladeIronStamina())) * 20.0f;
}

float USkillTreeComponent::GetExortDamageMultiplier() const
{
	const float SolarCoreBonus = static_cast<float>(GetNodeRank(ChroniclesSkillNodes::ExortSolarCore())) * 0.14f;
	const float CataclysmBonus = static_cast<float>(GetNodeRank(ChroniclesSkillNodes::ExortCataclysmMaster())) * 0.35f;
	return 1.0f + SolarCoreBonus + CataclysmBonus;
}

float USkillTreeComponent::GetWexCooldownMultiplier() const
{
	const float Reduction = static_cast<float>(GetNodeRank(ChroniclesSkillNodes::WexSwiftInvocation())) * 0.10f;
	return FMath::Clamp(1.0f - Reduction, 0.35f, 1.0f);
}

float USkillTreeComponent::GetQuasControlMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::QuasChronoHold())) * 0.20f;
}

float USkillTreeComponent::GetStaggerMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::QuasStaggerBreaker())) * 0.18f;
}

float USkillTreeComponent::GetMeleeDamageMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::BladeKeenEdge())) * 0.15f;
}

float USkillTreeComponent::GetDragonSpeedMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::WexDrakeGale())) * 0.20f;
}

float USkillTreeComponent::GetDragonFireMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::BladeDragonfireMastery())) * 0.25f;
}

float USkillTreeComponent::GetChaosDestructionMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::ExortChaosShatter())) * 0.35f;
}

float USkillTreeComponent::GetMoveSpeedMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::WexPhantomStep())) * 0.14f;
}

float USkillTreeComponent::GetHPRegenBonus() const
{
	return static_cast<float>(GetNodeRank(ChroniclesSkillNodes::QuasVitalFrost())) * 1.5f;
}

float USkillTreeComponent::GetManaRegenMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::WexStormMana())) * 0.15f;
}

float USkillTreeComponent::GetStaminaRegenMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::BladeIronStamina())) * 0.18f;
}

float USkillTreeComponent::GetExortWildfireMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::ExortWildfire())) * 0.12f;
}

float USkillTreeComponent::GetArmorMeltBonus() const
{
	return static_cast<float>(GetNodeRank(ChroniclesSkillNodes::ExortArmorMelt())) * 0.08f;
}

float USkillTreeComponent::GetExortBlastRadiusMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::ExortMeteorMass())) * 0.22f;
}

float USkillTreeComponent::GetWexDamageMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::WexArcConduit())) * 0.15f;
}

float USkillTreeComponent::GetSuperconductMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::WexTempestLord())) * 0.45f;
}

float USkillTreeComponent::GetShieldMitigationBonus() const
{
	return static_cast<float>(GetNodeRank(ChroniclesSkillNodes::QuasGlacialAegis())) * 0.10f;
}

float USkillTreeComponent::GetThermalShockMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::QuasThermalShock())) * 0.30f;
}

float USkillTreeComponent::GetStaggerWindowBonusSeconds() const
{
	return static_cast<float>(GetNodeRank(ChroniclesSkillNodes::QuasTimelessSovereign())) * 1.2f;
}

float USkillTreeComponent::GetParryManaRestore() const
{
	return static_cast<float>(GetNodeRank(ChroniclesSkillNodes::BladePerfectParry())) * 22.0f;
}

float USkillTreeComponent::GetExecutionerMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::BladeExecutioner())) * 0.28f;
}

float USkillTreeComponent::GetDodgeIFrameMultiplier() const
{
	return 1.0f + static_cast<float>(GetNodeRank(ChroniclesSkillNodes::BladeWukongStance())) * 0.25f;
}

// ============================================================================
// SaveGame Persistence
// ============================================================================

void USkillTreeComponent::PopulateSaveGame(UChroniclesSaveGame* SaveObj) const
{
	if (!SaveObj)
	{
		return;
	}

	SaveObj->PlayerLevel = Level;
	SaveObj->CurrentXP = CurrentXP;
	SaveObj->UnspentSkillPoints = UnspentSkillPoints;
	SaveObj->Gold = Gold;

	SaveObj->SkillRanks.Reset();
	for (const FSkillNodeDef& Node : Nodes)
	{
		SaveObj->SkillRanks.Add(Node.NodeId, Node.CurrentRank);
	}

	if (SaveObj->FogOfWarGrid.Num() != UChroniclesSaveGame::FogOfWarGridSize)
	{
		SaveObj->FogOfWarGrid.SetNumZeroed(UChroniclesSaveGame::FogOfWarGridSize);
	}

	if (const AActor* Owner = GetOwner())
	{
		SaveObj->LastPlayerLocation = Owner->GetActorLocation();
	}
}

void USkillTreeComponent::ApplySaveGame(const UChroniclesSaveGame* SaveObj)
{
	if (!SaveObj)
	{
		return;
	}

	if (Nodes.IsEmpty())
	{
		BuildDefaultNodes();
	}

	Level = FMath::Max(1, SaveObj->PlayerLevel);
	CurrentXP = FMath::Max(0, SaveObj->CurrentXP);
	UnspentSkillPoints = FMath::Max(0, SaveObj->UnspentSkillPoints);
	Gold = FMath::Max(0, SaveObj->Gold);

	for (FSkillNodeDef& Node : Nodes)
	{
		if (const int32* SavedRank = SaveObj->SkillRanks.Find(Node.NodeId))
		{
			Node.CurrentRank = FMath::Clamp(*SavedRank, 0, Node.MaxRank);
		}
		else
		{
			Node.CurrentRank = 0;
		}
	}
}

bool USkillTreeComponent::SaveProgress(const FString& SlotName) const
{
	if (SlotName.IsEmpty())
	{
		UE_LOG(LogChronicles, Warning, TEXT("[SkillTree] SaveProgress failed: empty SlotName."));
		return false;
	}

	UChroniclesSaveGame* SaveObj = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		SaveObj = Cast<UChroniclesSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}

	if (!SaveObj)
	{
		SaveObj = Cast<UChroniclesSaveGame>(UGameplayStatics::CreateSaveGameObject(UChroniclesSaveGame::StaticClass()));
	}

	if (!SaveObj)
	{
		UE_LOG(LogChronicles, Error, TEXT("[SkillTree] SaveProgress failed: could not create UChroniclesSaveGame."));
		return false;
	}

	PopulateSaveGame(SaveObj);
	OnPopulateSaveGame.Broadcast(SaveObj);

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, 0);
	if (bSaved)
	{
		UE_LOG(LogChronicles, Log, TEXT("[SkillTree] Saved progress to slot '%s' (Level=%d, XP=%d, Points=%d, Gold=%d, Ranks=%d)"),
			*SlotName, Level, CurrentXP, UnspentSkillPoints, Gold, GetTotalInvestedRanks());
	}
	else
	{
		UE_LOG(LogChronicles, Error, TEXT("[SkillTree] Failed to write SaveGame slot '%s'."), *SlotName);
	}

	return bSaved;
}

bool USkillTreeComponent::LoadProgress(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		UE_LOG(LogChronicles, Warning, TEXT("[SkillTree] LoadProgress failed: empty SlotName."));
		return false;
	}

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UE_LOG(LogChronicles, Log, TEXT("[SkillTree] LoadProgress skipped: slot '%s' does not exist."), *SlotName);
		return false;
	}

	UChroniclesSaveGame* LoadedObj = Cast<UChroniclesSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!LoadedObj)
	{
		UE_LOG(LogChronicles, Warning, TEXT("[SkillTree] LoadProgress failed: slot '%s' is not a UChroniclesSaveGame."), *SlotName);
		return false;
	}

	ApplySaveGame(LoadedObj);
	OnApplySaveGame.Broadcast(LoadedObj);
	OnSkillTreeChanged.Broadcast();

	UE_LOG(LogChronicles, Log, TEXT("[SkillTree] Loaded progress from slot '%s' (Level=%d, XP=%d, Points=%d, Gold=%d, Ranks=%d)"),
		*SlotName, Level, CurrentXP, UnspentSkillPoints, Gold, GetTotalInvestedRanks());

	return true;
}

// ============================================================================
// Replication & Internal Helpers
// ============================================================================

void USkillTreeComponent::OnRep_Progression()
{
	OnSkillTreeChanged.Broadcast();
}

void USkillTreeComponent::OnRep_Nodes()
{
	OnSkillTreeChanged.Broadcast();
}

const FSkillNodeDef* USkillTreeComponent::FindNode(FName NodeId) const
{
	if (NodeId.IsNone())
	{
		return nullptr;
	}

	for (const FSkillNodeDef& Node : Nodes)
	{
		if (Node.NodeId == NodeId)
		{
			return &Node;
		}
	}

	return nullptr;
}

FSkillNodeDef* USkillTreeComponent::FindNodeMutable(FName NodeId)
{
	if (NodeId.IsNone())
	{
		return nullptr;
	}

	for (FSkillNodeDef& Node : Nodes)
	{
		if (Node.NodeId == NodeId)
		{
			return &Node;
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
