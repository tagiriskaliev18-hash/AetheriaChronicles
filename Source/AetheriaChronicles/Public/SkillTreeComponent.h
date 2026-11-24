// ============================================================================
// SkillTreeComponent.h — 24-Node 4-Branch RPG Skill Tree, XP/Leveling,
// Replicated Progression Stats & Persistent UChroniclesSaveGame
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "ChroniclesTypes.h"
#include "SkillTreeComponent.generated.h"

/**
 * Persistent save game payload for AetheriaChronicles.
 * Stores progression, unlocked primal spheres, dragon mount state, skill ranks,
 * quest state/progress, 64x64 fog-of-war grid (4096 bytes), and last player location.
 */
UCLASS(BlueprintType)
class AETHERIACHRONICLES_API UChroniclesSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 FogOfWarGridDim = 64;
	static constexpr int32 FogOfWarGridSize = FogOfWarGridDim * FogOfWarGridDim; // 4096 bytes

	UChroniclesSaveGame();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	int32 PlayerLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	int32 CurrentXP = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	int32 UnspentSkillPoints = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	int32 Gold = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	bool bUnlockedExort = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	bool bUnlockedWex = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	bool bUnlockedQuas = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	bool bUnlockedDragonMount = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	TMap<FName, int32> SkillRanks;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	TMap<FName, int32> QuestProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	TMap<FName, uint8> QuestStates;

	/** 64x64 world fog-of-war exploration mask (4096 bytes). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	TArray<uint8> FogOfWarGrid;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	FVector LastPlayerLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	bool bHasSavedHeroProfile = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "Aetheria|Save")
	FHeroCustomizationProfile SavedHeroProfile;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillTreeLevelUp, int32, NewLevel, int32, NewUnspentSkillPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillNodeUpgraded, FName, NodeId, int32, NewRank);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillTreeChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChroniclesSaveSync, UChroniclesSaveGame*);

/**
 * Replicated RPG Skill Tree & Progression Component.
 * Manages 24 handcrafted skill nodes across 4 branches (ExortPath, WexPath, QuasPath, BladeAndDrake),
 * multi-level XP progression, gold economy, computed combat/movement/dragon multipliers,
 * and SaveGame persistence.
 */
UCLASS(ClassGroup = (Aetheria), meta = (BlueprintSpawnableComponent))
class AETHERIACHRONICLES_API USkillTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillTreeComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---- Replicated Progression State ----

	UPROPERTY(ReplicatedUsing = OnRep_Progression, EditAnywhere, BlueprintReadOnly, Category = "Aetheria|Progression")
	int32 Level = 1;

	UPROPERTY(ReplicatedUsing = OnRep_Progression, EditAnywhere, BlueprintReadOnly, Category = "Aetheria|Progression")
	int32 CurrentXP = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Progression, EditAnywhere, BlueprintReadOnly, Category = "Aetheria|Progression")
	int32 UnspentSkillPoints = 1;

	UPROPERTY(ReplicatedUsing = OnRep_Progression, EditAnywhere, BlueprintReadOnly, Category = "Aetheria|Progression")
	int32 Gold = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Nodes, EditAnywhere, BlueprintReadOnly, Category = "Aetheria|SkillTree")
	TArray<FSkillNodeDef> Nodes;

	// ---- Events / Delegates ----

	UPROPERTY(BlueprintAssignable, Category = "Aetheria|Progression")
	FOnSkillTreeLevelUp OnLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Aetheria|SkillTree")
	FOnSkillNodeUpgraded OnSkillNodeUpgraded;

	UPROPERTY(BlueprintAssignable, Category = "Aetheria|SkillTree")
	FOnSkillTreeChanged OnSkillTreeChanged;

	/** Native hook fired during SaveProgress so character/quest/map systems can write into UChroniclesSaveGame. */
	mutable FOnChroniclesSaveSync OnPopulateSaveGame;

	/** Native hook fired during LoadProgress so character/quest/map systems can restore from UChroniclesSaveGame. */
	FOnChroniclesSaveSync OnApplySaveGame;

	// ---- Progression & Skill Tree API ----

	/** XP required to advance from current Level to Level + 1: round(120 * Level^1.6). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Progression")
	int32 GetXPForNextLevel() const;

	/** Adds XP and processes any number of level-ups, granting +1 UnspentSkillPoints per level. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Progression")
	void AddXP(int32 Amount);

	/** Adds (or subtracts, clamped >= 0) Gold. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Progression")
	void AddGold(int32 Amount);

	/** Returns true if UnspentSkillPoints > 0, node exists, rank < MaxRank, and prerequisite has >= 1 rank. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|SkillTree")
	bool CanUpgradeNode(FName NodeId) const;

	/** Invests 1 skill point into NodeId if valid. Replicates cleanly from client via ServerUpgradeNode. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|SkillTree")
	bool UpgradeNode(FName NodeId);

	/** Returns current rank of NodeId, or 0 if not found. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|SkillTree")
	int32 GetNodeRank(FName NodeId) const;

	/** Returns all 6 skill nodes belonging to the requested branch. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|SkillTree")
	TArray<FSkillNodeDef> GetNodesForBranch(ESkillBranch Branch) const;

	/** Returns total skill points invested across all 24 nodes. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|SkillTree")
	int32 GetTotalInvestedRanks() const;

	// ---- Computed Stat Getters (Core 12 + Specialized Node Accessors) ----

	/** Flat Max HP bonus from quas_vital_frost (+25/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetMaxHPBonus() const;

	/** Flat Max Mana bonus from wex_storm_mana (+20/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetMaxManaBonus() const;

	/** Flat Max Stamina bonus from blade_iron_stamina (+20/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetMaxStaminaBonus() const;

	/** Exort damage multiplier: 1 + 0.14*exort_solar_core + 0.35*exort_cataclysm_master. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetExortDamageMultiplier() const;

	/** Wex spell cooldown multiplier: clamp(1 - 0.10*wex_swift_invocation, 0.35, 1.0). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetWexCooldownMultiplier() const;

	/** Quas Freeze/Slow control duration multiplier: 1 + 0.20*quas_chrono_hold. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetQuasControlMultiplier() const;

	/** Boss StaggerGauge buildup multiplier: 1 + 0.18*quas_stagger_breaker. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetStaggerMultiplier() const;

	/** Sword melee combo damage multiplier: 1 + 0.15*blade_keen_edge. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetMeleeDamageMultiplier() const;

	/** Dragon flight boost speed & fire recharge multiplier: 1 + 0.20*wex_drake_gale. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetDragonSpeedMultiplier() const;

	/** Dragon fire breath DPS & range multiplier: 1 + 0.25*blade_dragonfire_mastery. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetDragonFireMultiplier() const;

	/** Chaos Destruction impulse & debris radius multiplier: 1 + 0.35*exort_chaos_shatter. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetChaosDestructionMultiplier() const;

	/** Sprint speed & dodge-roll distance multiplier: 1 + 0.14*wex_phantom_step. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetMoveSpeedMultiplier() const;

	// ---- Additional Per-Node Computed Getters ----

	/** HP/s regeneration bonus from quas_vital_frost (+1.5/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetHPRegenBonus() const;

	/** Mana regen multiplier from wex_storm_mana (1 + 0.15/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetManaRegenMultiplier() const;

	/** Stamina regen multiplier from blade_iron_stamina (1 + 0.18/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetStaminaRegenMultiplier() const;

	/** Wildfire DoT damage multiplier from exort_wildfire (1 + 0.12/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetExortWildfireMultiplier() const;

	/** Boss fire armor melt vulnerability bonus from exort_armor_melt (+0.08/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetArmorMeltBonus() const;

	/** Meteor & SunStrike blast radius multiplier from exort_meteor_mass (1 + 0.22/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetExortBlastRadiusMultiplier() const;

	/** Storm & Lightning damage multiplier from wex_arc_conduit (1 + 0.15/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetWexDamageMultiplier() const;

	/** Superconduct shock reaction multiplier from wex_tempest_lord (1 + 0.45/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetSuperconductMultiplier() const;

	/** Additional shield block mitigation from quas_glacial_aegis (+0.10/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetShieldMitigationBonus() const;

	/** Thermal Shock & Cryo-Shatter reaction multiplier from quas_thermal_shock (1 + 0.30/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetThermalShockMultiplier() const;

	/** Extra Boss Stagger Break vulnerability duration from quas_timeless_sovereign (+1.2s/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetStaggerWindowBonusSeconds() const;

	/** Mana restored on timed parry from blade_perfect_parry (+22/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetParryManaRestore() const;

	/** Melee finisher multiplier vs staggered foes from blade_executioner (1 + 0.28/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetExecutionerMultiplier() const;

	/** Dodge i-frame & riposte multiplier from blade_wukong_stance (1 + 0.25/rank). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Stats")
	float GetDodgeIFrameMultiplier() const;

	// ---- Persistence API ----

	/** Writes SkillTreeComponent fields into an existing UChroniclesSaveGame object. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Save")
	void PopulateSaveGame(UChroniclesSaveGame* SaveObj) const;

	/** Restores SkillTreeComponent fields from an existing UChroniclesSaveGame object. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Save")
	void ApplySaveGame(const UChroniclesSaveGame* SaveObj);

	/** Saves progression to the specified SaveGame slot (preserving non-skill fields if slot exists). */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Save")
	bool SaveProgress(const FString& SlotName = TEXT("AetheriaChroniclesSlot0")) const;

	/** Loads progression from the specified SaveGame slot. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Save")
	bool LoadProgress(const FString& SlotName = TEXT("AetheriaChroniclesSlot0"));

protected:
	UFUNCTION()
	void OnRep_Progression();

	UFUNCTION()
	void OnRep_Nodes();

	UFUNCTION(Server, Reliable)
	void ServerUpgradeNode(FName NodeId);

private:
	void BuildDefaultNodes();
	const FSkillNodeDef* FindNode(FName NodeId) const;
	FSkillNodeDef* FindNodeMutable(FName NodeId);
};
