// ============================================================================
// SpellSystemComponent.h — Quest-Gated 10-Spell Invoker System & Spell Actor
// Supports progressive orb unlocks (Exort -> Wex -> Quas : 1 -> 4 -> 10 spells),
// Classic Invoker & InstantCombo modes, multiplayer replication, physical AgX
// Lumen lighting, CHRONICLE_TRACE_PROJECTILE sweeps, and Chaos/physics impulse.
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "ChroniclesTypes.h"
#include "SpellSystemComponent.generated.h"

class ACharacter;
class AChronicleSpellActor;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraComponent;
class UParticleSystemComponent;
class UPointLightComponent;
class USceneComponent;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChronicleOrbUnlocked, EChronicleOrb, UnlockedOrb);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChronicleSpellStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChronicleSpellCast, EChronicleSpellId, SpellId, AChronicleSpellActor*, SpawnedSpell);

/**
 * Replicated runtime spell actor executing all 10 Invoker spells with procedural
 * emissive BasicShapes geometry, physical AgX Lumen point lighting, dedicated
 * CHRONICLE_TRACE_PROJECTILE sweeps, radial damage, and destruction impulse.
 */
UCLASS()
class AETHERIACHRONICLES_API AChronicleSpellActor : public AActor
{
	GENERATED_BODY()

public:
	AChronicleSpellActor();

	/** Must be called before FinishSpawning (or on server) to configure spell visuals & behavior. */
	void InitializeSpell(
		EChronicleSpellId InSpellId,
		const FVector& InDeliveryDirection,
		const FVector& InTargetWorldLocation,
		float InChargeT01,
		float InPowerScale,
		float InSpeedScale,
		float InControlScale);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Spell")
	EChronicleSpellId GetSpellId() const { return SpellId; }

protected:
	UFUNCTION()
	void OnRep_SpellConfig();

	void ConfigureSpellVisualsAndBehavior();
	void SpawnRealSpellVFXAndCastSFX(float ChargeScale);
	void PlaySpellImpactVFXAndSFX(const FVector& Origin);
	void UpdateEmissiveIntensity(float EmissiveScale);
	void UpdateLightFade(float DeltaSeconds);
	void ApplySpellBlastAndDestruction(
		const FVector& Origin,
		float Radius,
		float DamageAmount,
		float ImpulseStrength,
		bool bPullInward = false,
		float VerticalLaunch = 0.f);
	void TriggerFlashBoost(float TargetPeakLumens);
	FCollisionQueryParams BuildSpellQueryParams(const FName& TraceTag) const;
	bool SnapPointToGround(const FVector& ReferencePoint, FVector& OutGroundPoint) const;
	void ApplyTemporarySpeedBuff(ACharacter* TargetChar, float SpeedBonusRatio);
	void ClearTemporarySpeedBuff();

	// --- Scene & Visual Components (all UPROPERTY TObjectPtr for strict GC safety) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spell|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spell|Components")
	TObjectPtr<UStaticMeshComponent> PrimaryMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spell|Components")
	TObjectPtr<UStaticMeshComponent> SecondaryMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spell|Components")
	TObjectPtr<UPointLightComponent> SpellLight;

	// --- Cached Engine BasicShapes & Material Assets ---
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMeshAsset;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMeshAsset;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMeshAsset;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseEmissiveMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PrimaryMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SecondaryMID;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveNiagaraVFX;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystemComponent> ActiveCascadeVFX;

	// --- Replicated Spell Configuration ---
	UPROPERTY(ReplicatedUsing = OnRep_SpellConfig, BlueprintReadOnly, Category = "Spell")
	EChronicleSpellId SpellId = EChronicleSpellId::None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Spell")
	FVector DeliveryDirection = FVector::ForwardVector;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Spell")
	FVector TargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Spell")
	float ChargeT01 = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Spell")
	float PowerScale = 1.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Spell")
	float SpeedScale = 1.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Spell")
	float ControlScale = 1.f;

	// --- Runtime Execution State ---
	FChronicleSpellDef CachedDef;
	bool bConfigured = false;
	bool bPrimaryDetonated = false;
	bool bMeteorLanded = false;
	bool bProjectileImpacted = false;
	bool bImpactVFXPlayed = false;

	float ElapsedTime = 0.f;
	float TotalDuration = 2.0f;
	float PulseTimer = 0.f;
	float BaseLightLumens = 22000.f;
	float CurrentLightLumens = 22000.f;
	float PeakLightLumens = 30000.f;
	float BaseEmissiveStrength = 6.0f;
	float TravelSpeed = 1600.f;
	float EffectiveDamage = 65.f;
	float EffectiveRadius = 520.f;
	float EffectiveImpulse = 1600.f;

	FVector MeteorStartLocation = FVector::ZeroVector;
	FVector MeteorLandingLocation = FVector::ZeroVector;

	int32 ColdSnapRemainingProcs = 0;
	TWeakObjectPtr<AActor> ColdSnapTarget;
	TWeakObjectPtr<ACharacter> BuffedCharacter;
	float AppliedSpeedDelta = 0.f;

	TSet<TWeakObjectPtr<AActor>> SweptHitActors;
};

/**
 * Replicated Invoker spellcasting component with quest-gated primal orb unlocks,
 * 3-orb FIFO queue, D/F spell slots, ClassicInvoker & InstantCombo cast modes,
 * per-spell cooldowns, skill-tree scaling hooks, and mana regeneration.
 */
UCLASS(ClassGroup = (Chronicles), meta = (BlueprintSpawnableComponent))
class AETHERIACHRONICLES_API USpellSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellSystemComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Quest-Gated Orb Unlocks (Replicated) ---
	UPROPERTY(ReplicatedUsing = OnRep_SpellState, EditAnywhere, BlueprintReadOnly, Category = "Spell|Unlocks")
	bool bUnlockedExort = false;

	UPROPERTY(ReplicatedUsing = OnRep_SpellState, EditAnywhere, BlueprintReadOnly, Category = "Spell|Unlocks")
	bool bUnlockedWex = false;

	UPROPERTY(ReplicatedUsing = OnRep_SpellState, EditAnywhere, BlueprintReadOnly, Category = "Spell|Unlocks")
	bool bUnlockedQuas = false;

	UFUNCTION(BlueprintCallable, Category = "Spell|Unlocks")
	void UnlockOrb(EChronicleOrb Orb);

	UFUNCTION(BlueprintPure, Category = "Spell|Unlocks")
	bool IsOrbUnlocked(EChronicleOrb Orb) const;

	UFUNCTION(BlueprintPure, Category = "Spell|Unlocks")
	int32 GetUnlockedOrbCount() const;

	// --- Active 3-Orb Queue & Spell Slots (Replicated) ---
	UPROPERTY(ReplicatedUsing = OnRep_SpellState, BlueprintReadOnly, Category = "Spell|Invoker")
	TArray<EChronicleOrb> ActiveOrbs;

	UPROPERTY(ReplicatedUsing = OnRep_SpellState, BlueprintReadOnly, Category = "Spell|Invoker")
	EChronicleSpellId SlotD = EChronicleSpellId::None;

	UPROPERTY(ReplicatedUsing = OnRep_SpellState, BlueprintReadOnly, Category = "Spell|Invoker")
	EChronicleSpellId SlotF = EChronicleSpellId::None;

	UPROPERTY(ReplicatedUsing = OnRep_SpellState, EditAnywhere, BlueprintReadWrite, Category = "Spell|Invoker")
	EChronicleCastMode CastMode = EChronicleCastMode::ClassicInvoker;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Resources")
	float Mana = 120.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Resources")
	float MaxMana = 120.f;

	// --- External Skill-Tree Scaling Hooks ---
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Scaling")
	float ExortPowerScale = 1.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Scaling")
	float WexSpeedScale = 1.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Scaling")
	float QuasControlScale = 1.f;

	/** True when Hero stature < 1.45m — physically locks out casting and invoking spells! */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Scaling")
	bool bSpellCastingBlockedByStature = false;

	/** Multiplier applied to spell ManaCost (driven by Race passives e.g. HighElf 0.80x). */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Scaling")
	float ManaCostMultiplier = 1.f;

	/** Multiplier applied to spell cooldowns (driven by Intelligence + HighElf + SkillTree). */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Scaling")
	float SpellCooldownMultiplier = 1.f;

	/** Mana regenerated per second (driven by Spirit + SkillTree). */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Spell|Scaling")
	float ManaRegenPerSec = 9.f;

	// --- Cooldowns ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Cooldown")
	float InvokeCooldownDuration = 0.65f;

	UPROPERTY(BlueprintReadOnly, Category = "Spell|Cooldown")
	float InvokeCooldownRemaining = 0.f;

	UPROPERTY(Transient)
	TMap<EChronicleSpellId, float> CooldownRemaining;

	// --- Events ---
	UPROPERTY(BlueprintAssignable, Category = "Spell|Events")
	FOnChronicleOrbUnlocked OnOrbUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Spell|Events")
	FOnChronicleSpellStateChanged OnSpellStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Spell|Events")
	FOnChronicleSpellCast OnSpellCast;

	// --- Core Invoker Methods ---
	UFUNCTION(BlueprintCallable, Category = "Spell|Invoker")
	bool PushOrb(EChronicleOrb Orb);

	UFUNCTION(BlueprintPure, Category = "Spell|Invoker")
	EChronicleSpellId ResolveCurrentCombo() const;

	UFUNCTION(BlueprintCallable, Category = "Spell|Invoker")
	bool InvokeCurrentSpell();

	UFUNCTION(BlueprintCallable, Category = "Spell|Invoker")
	bool CastSpellSlot(
		bool bSlotF,
		const FVector& AimOrigin,
		const FVector& AimDir,
		const FVector& TargetPoint,
		float ChargeT01 = 0.f);

	// --- Spell Definition Registry & Unlock Queries ---
	static const TArray<FChronicleSpellDef>& GetAllSpellDefs();

	UFUNCTION(BlueprintPure, Category = "Spell|Data")
	static FChronicleSpellDef GetSpellDef(EChronicleSpellId SpellId);

	UFUNCTION(BlueprintPure, Category = "Spell|Unlocks")
	bool IsSpellUnlocked(EChronicleSpellId SpellId) const;

	UFUNCTION(BlueprintPure, Category = "Spell|Unlocks")
	int32 GetUnlockedSpellCount() const;

	UFUNCTION(BlueprintPure, Category = "Spell|Cooldown")
	float GetSpellCooldownRemaining(EChronicleSpellId SpellId) const;

	// --- Convenience Helpers ---
	UFUNCTION(BlueprintCallable, Category = "Spell|Resources")
	void RestoreMana(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Spell|Invoker")
	void SetCastMode(EChronicleCastMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Spell|Invoker")
	void ToggleCastMode();

	UFUNCTION(BlueprintPure, Category = "Spell|Invoker")
	FString GetActiveOrbsString() const;

	// --- God Mode (/godmode) Full Spell Access, Zero Cooldown & Infinite Mana ---
	UPROPERTY(ReplicatedUsing = OnRep_SpellState, BlueprintReadOnly, Category = "Spell|GodMode")
	bool bGodModeActive = false;

	UFUNCTION(BlueprintCallable, Category = "Spell|GodMode")
	void SetGodModeActive(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "Spell|GodMode")
	bool IsGodModeActive() const { return bGodModeActive; }

protected:
	UFUNCTION()
	void OnRep_SpellState();

	bool OwnerHasAuthority() const;
	void SetGodModeActive_Internal(bool bEnable);
	void UnlockOrb_Internal(EChronicleOrb Orb);
	bool PushOrb_Internal(EChronicleOrb Orb);
	bool InvokeCurrentSpell_Internal();
	bool CastSpellSlot_Internal(
		bool bSlotF,
		const FVector& AimOrigin,
		const FVector& AimDir,
		const FVector& TargetPoint,
		float ChargeT01);

	UFUNCTION(Server, Reliable)
	void ServerSetGodModeActive(bool bEnable);

	UFUNCTION(Server, Reliable)
	void ServerUnlockOrb(EChronicleOrb Orb);

	UFUNCTION(Server, Reliable)
	void ServerPushOrb(EChronicleOrb Orb);

	UFUNCTION(Server, Reliable)
	void ServerInvokeCurrentSpell();

	UFUNCTION(Server, Reliable)
	void ServerSetCastMode(EChronicleCastMode NewMode);

	UFUNCTION(Server, Reliable)
	void ServerCastSpellSlot(
		bool bSlotF,
		const FVector& AimOrigin,
		const FVector& AimDir,
		const FVector& TargetPoint,
		float ChargeT01);
};
