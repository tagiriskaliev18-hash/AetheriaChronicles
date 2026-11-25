// ============================================================================
// HeroDragonBoss.h — Core Replicated Gameplay Actors for AetheriaChronicles
// 1. AAetheriaDragonActor   (Rideable Fire-Breathing Red Dragon — APawn)
// 2. AAetheriaHeroCharacter (Wukong-Grade High-Bone Hero — ACharacter)
// 3. AAetheriaEnemyCharacter & AAetheriaBossCharacter (Malakor Multi-Phase Boss)
// 4. AAetheriaNPCActor      (Named Story Quest Givers)
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "ChroniclesTypes.h"
#include "HeroDragonBoss.generated.h"

class AAetheriaBossCharacter;
class AAetheriaDragonActor;
class AAetheriaEnemyCharacter;
class AAetheriaHeroCharacter;
class AController;
class APlayerController;
class UAnimMontage;
class UAnimSequence;
class UCameraComponent;
class UCapsuleComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMotionWarpingComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UParticleSystem;
class UParticleSystemComponent;
class UPointLightComponent;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class USkillTreeComponent;
class USoundBase;
class USpellSystemComponent;
class USpringArmComponent;
class UHeroCustomizationComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FDamageEvent;

/** Archetypes for roaming & quest enemies across Aetheria's 5 biomes. */
UENUM(BlueprintType)
enum class EChronicleEnemyArchetype : uint8
{
	Skeleton       UMETA(DisplayName = "Risen Skeleton"),
	ForestTroll    UMETA(DisplayName = "Highland Forest Troll"),
	TempestStalker UMETA(DisplayName = "Tempest Stalker"),
	FrostWarden    UMETA(DisplayName = "Glacier Frost Warden")
};

// ============================================================================
// 1. AAetheriaDragonActor — Rideable Fire-Breathing Red Dragon (Replicated)
// ============================================================================
UCLASS(Blueprintable)
class AETHERIACHRONICLES_API AAetheriaDragonActor : public APawn
{
	GENERATED_BODY()

public:
	AAetheriaDragonActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- Public Flight & Rider State ---

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Dragon")
	bool bMounted = false;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Dragon")
	float FireEnergy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Dragon")
	float MaxFireEnergy = 100.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Dragon")
	float FlightSpeed = 1600.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Dragon")
	bool bBreathingFire = false;

	TWeakObjectPtr<AAetheriaHeroCharacter> RiderHero;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Dragon")
	AAetheriaHeroCharacter* GetRiderHero() const { return RiderHero.Get(); }

	// --- Flight Camera Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Camera")
	TObjectPtr<USpringArmComponent> FlightArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Camera")
	TObjectPtr<UCameraComponent> FlightCamera;

	// --- Visual & Articulation Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> DragonRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USkeletalMeshComponent> ImportedDragonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> ProceduralDragonRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> NeckPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> HeadPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> LowerJawPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> LeftWingPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> RightWingPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> SaddleSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<USceneComponent> MouthSocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<UStaticMeshComponent> FireBreathConeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<UStaticMeshComponent> FireBreathCoreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Dragon|Visuals")
	TObjectPtr<UPointLightComponent> MouthFireLight;

	// --- Rider & Flight API ---

	/** Attaches Hero cleanly to SaddleSocket, possesses dragon via PC, and launches upward. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Dragon")
	void MountRider(AAetheriaHeroCharacter* Hero, APlayerController* PC);

	/** Traces safe ground below dragon via ECC_WorldStatic, lands gently, detaches Hero to side, restores possession. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Dragon")
	void DismountRider(APlayerController* PC);

	/** Drives banking flight, boost/cruise throttle, climb/dive, dragonfire breath DPS & Fog-of-War aerial reveal. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Dragon")
	void ApplyFlightInput(
		float ForwardAxis,
		float RightAxis,
		float UpAxis,
		float YawInput,
		float PitchInput,
		bool bFireHeld,
		float DeltaTime);

private:
	bool TryBindImportedDragonMesh();
	void BuildProceduralRedDragon();
	UStaticMeshComponent* AddProceduralPart(
		const FString& PartName,
		UStaticMesh* MeshAsset,
		USceneComponent* ParentComp,
		const FVector& RelLoc,
		const FRotator& RelRot,
		const FVector& RelScale,
		UMaterialInterface* Mat);
	void ApplyDragonFireSweep(float DeltaDamage, float FireMult);
	void UpdateDragonAnimation(float DeltaTime);

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> TailSegmentPivots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ProceduralParts;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMeshAsset;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMeshAsset;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ConeMeshAsset;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMeshAsset;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CrimsonScaleMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CrimsonBellyMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HornObsidianMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EyeGlowMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FireOuterMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FireCoreMID;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> DragonIdleAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> DragonTakeOffAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> DragonFlyAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> DragonGlideAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> DragonFlameAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> DragonLandAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveDragonAnim;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystemComponent> DragonFireStormPSC;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystemComponent> DragonFireBallPSC;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> DragonFireSFX;

	FName DragonMouthBoneName = NAME_None;
	FName DragonSaddleBoneName = NAME_None;
	bool bUsingImportedSkeletalMesh = false;
	bool bHasRealDragonFireFX = false;
	bool bWasBreathingFire = false;
	float WingBeatPhase = 0.f;
	float WingBeatSpeed = 3.2f;
	float TailWavePhase = 0.f;
	float CurrentBankRoll = 0.f;
	float CurrentPitchDeg = 0.f;
	float TakeoffBoostTimer = 0.f;
	float LandingAnimTimer = 0.f;
	float FireDamageTickAccum = 0.f;
	float MapRevealCooldown = 0.f;
	float CachedUpAxis = 0.f;
	float CachedForwardAxis = 0.f;
};

// ============================================================================
// 2. AAetheriaHeroCharacter — Wukong-Grade High-Bone Hero (Replicated)
// ============================================================================
UCLASS(Blueprintable)
class AETHERIACHRONICLES_API AAetheriaHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAetheriaHeroCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false) override;

	// --- Core Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Systems")
	TObjectPtr<USpellSystemComponent> SpellSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Systems")
	TObjectPtr<USkillTreeComponent> SkillTree;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|HeroCreator")
	TObjectPtr<UHeroCustomizationComponent> Customization;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Combat")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	/** 3 orbiting glowing 3D sphere meshes behind hero shoulders mirroring SpellSystem->ActiveOrbs. */
	UPROPERTY(VisibleAnywhere, Category = "Aetheria|Orbs")
	TObjectPtr<UStaticMeshComponent> OrbitingOrbMeshes[3];

	/** Per-orb point light so each primal orb actually casts coloured bounce onto armor & ground. */
	UPROPERTY(VisibleAnywhere, Category = "Aetheria|Orbs")
	TObjectPtr<UPointLightComponent> OrbitingOrbLights[3];

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Combat")
	TObjectPtr<UPointLightComponent> CombatFlashLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Combat")
	TObjectPtr<UStaticMeshComponent> HeroWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|VFX")
	TObjectPtr<UNiagaraComponent> SprintFootAuraComp;

	// --- Replicated Combat & Progression Stats ---

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Combat")
	float HP = 120.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Combat")
	float MaxHP = 120.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Combat")
	float Stamina = 100.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Combat")
	float MaxStamina = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Combat")
	bool bDead = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Combat")
	bool bBlocking = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Combat")
	int32 ComboIndex = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Combat")
	float RollTimer = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Combat")
	float InvulnTimer = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Progression")
	int32 EnemiesSlain = 0;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Progression")
	bool bDragonUnlocked = false;

	/** True when God Mode (/godmode) is active — Hero is completely immortal/invulnerable and commands all 10 spells! */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|GodMode")
	bool bGodMode = false;

	/** Safe respawn anchor in Oakhaven Sanctuary (grounded dynamically via ECC_WorldStatic). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|World")
	FVector OakhavenSpawnLocation = FVector(0.f, 4200.f, 450.f);

	// --- Combat & Lifecycle API ---

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Combat")
	void PerformMeleeAttack();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Combat")
	void SetBlocking(bool bNewBlocking);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Combat")
	void PerformDodgeRoll(const FVector& WorldMoveDir);

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Combat")
	void RespawnAtOakhaven();

	UFUNCTION(BlueprintPure, Category = "Aetheria|Orbs")
	UStaticMeshComponent* GetOrbitingOrbMesh(int32 Index) const;

	/** Swaps the Hero's active AAA Paragon skeletal mesh, AnimBP, and combat montages cleanly at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	bool ApplyCustomSkeletalRig(
		const FString& MeshPath,
		const FString& AnimBPPath,
		const TArray<FString>& AttackMontagePaths,
		const FString& HitReactPath,
		const FString& DeathPath,
		FName SlotName = FName(TEXT("FullBody")));

	/** Re-syncs all RPG Attributes, Race Passives, Stature/Height physics, and SkillTree multipliers. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void RefreshAllHeroStats();

	/** True when Hero stature >= 1.45m (small stature < 1.45m cannot channel spells!). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|HeroCreator")
	bool CanCastSpellsByStature() const;

	/** Switches CameraBoom smoothly between normal gameplay and live 3D front-portrait Hero Customizer studio mode. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetCreatorStudioCameraMode(bool bEnable, float OrbitYawDeg = 165.f, int32 ZoomMode = 0);

	/** Enables or disables Immortal Archmage God Mode (invulnerability + all 10 spells + infinite mana/stamina). */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|GodMode")
	void SetGodMode(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|GodMode")
	bool ToggleGodMode();

	UFUNCTION(BlueprintPure, Category = "Aetheria|GodMode")
	bool IsGodModeActive() const { return bGodMode; }

protected:
	UFUNCTION(Server, Reliable)
	void ServerSetGodMode(bool bEnable);

	UFUNCTION(Server, Reliable)
	void ServerPerformMeleeAttack();

	UFUNCTION(Server, Reliable)
	void ServerSetBlocking(bool bNewBlocking);

	UFUNCTION(Server, Reliable)
	void ServerPerformDodgeRoll(const FVector& WorldMoveDir);

	UFUNCTION(Server, Reliable)
	void ServerRespawnAtOakhaven();

private:
	void SetupHeroSkeletalMeshAndAnims();
	bool BindParagonHeroSingleNodeAnims(USkeletalMesh* SkelMesh);
	void SetupOrbitingOrbs();
	void ExecuteMeleeAttack_Internal();
	void ExecuteDodgeRoll_Internal(const FVector& WorldMoveDir);
	void ExecuteRespawn_Internal();
	void ExecuteSetGodMode_Internal(bool bEnable);
	void PlayHeroMeleeVisualsAndAudio(int32 Step);
	void SpawnMartialComboVFXAndEffects(int32 Step, const FVector& AttackDir, const FVector& ImpactCenter);
	void PlayCombatAnimSequence(UAnimSequence* Seq, float PlayRate = 1.f);
	void TriggerCombatFlash(const FLinearColor& Color, float PeakLumens, float Duration = 0.24f);
	void SyncSkillTreeMultipliers();
	void UpdateOrbitingOrbs(float DeltaTime);
	void SpawnLevelUpSpectacle();
	void HandlePopulateSaveGame(class UChroniclesSaveGame* SaveObj);
	void HandleApplySaveGame(class UChroniclesSaveGame* SaveObj);

	UFUNCTION()
	void HandleOrbUnlocked(EChronicleOrb UnlockedOrb);

	UFUNCTION()
	void HandleSkillTreeChanged();

	UFUNCTION()
	void HandleCustomizationChanged();

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> CreatorStudioFillLight;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> CreatorStudioRimLight;

	bool bCreatorStudioCameraActive = false;
	float CreatorStudioOrbitYaw = 165.f;
	int32 CreatorStudioZoomMode = 0;
	float LastDynamicRefreshTimer = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OrbitingOrbMIDs[3];

	int32 OrbitingOrbMatMode = 0;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> AnimAttack[3];

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> HeroAttackMontages[4];

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> AnimDash;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> AnimHitReact;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> AnimDeath;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> AnimCast;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveIdleAnimSeq;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveRunAnimSeq;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveSprintAnimSeq;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveJumpStartSeq;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveJumpLoopSeq;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveJumpLandSeq;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveRollAnimSeq;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> MartialHit1FX;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> MartialFireArcFX;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> MartialShockwaveFX;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> MartialExplosionFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> MartialShockwaveNS;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> MartialWindLeavesNS;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> DodgeWindFlowsNS;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> SprintFootAuraNS;

	float MeleeMoveLockUntilTime = 0.0f;
	float RollBrakeUntilTime = 0.0f;
	float JumpLandAnimUntilTime = 0.0f;
	float CachedMoveInputTimestamp = -10.0f;
	float CombatAnimLockUntilTime = 0.f;
	bool bWasFallingLastTick = false;
	bool bPlayingRunAnim = false;
	bool bPlayingSprintAnim = false;
	bool bPlayingJumpAnim = false;
	bool bPlayingCombatOneShot = false;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> KwangPrimaryImpactFX;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> KwangLightStrikeBurstFX;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> KwangAttackEffortSFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> OrbUnlockAuraFireNS;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> OrbUnlockAuraLightningNS;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> OrbUnlockAuraSoulNS;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> HeroLevelUpNS;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> MagicBuffSFX;

	FName HeroAnimSlotName = FName(TEXT("DefaultSlot"));
	bool bUsingKwangHero = false;
	int32 LastObservedHeroLevel = 1;

	float ParryWindowTimer = 0.f;
	float MeleeCooldownTimer = 0.f;
	float ComboResetTimer = 0.f;
	float OrbOrbitAngle = 0.f;
	float FlashTimer = 0.f;
	float FlashDuration = 0.24f;
	float FlashPeakLumens = 6500.f;
	float MapRevealCooldown = 0.f;
	float BaseWalkSpeed = 580.f;
	float LastSyncedWalkSpeed = 580.f;
	FVector ActiveRollDir = FVector::ForwardVector;
};

// ============================================================================
// 3A. AAetheriaEnemyCharacter — Elemental-Reaction Biome Enemies (Replicated)
// ============================================================================
UCLASS(Blueprintable)
class AETHERIACHRONICLES_API AAetheriaEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAetheriaEnemyCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|Enemy")
	EChronicleEnemyArchetype Archetype = EChronicleEnemyArchetype::Skeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|Enemy")
	FText EnemyDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|Enemy")
	float HP = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|Enemy")
	float MaxHP = 120.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Enemy")
	bool bDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Enemy")
	int32 XPReward = 70;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Enemy")
	int32 GoldReward = 35;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Enemy")
	FName QuestIdOnDeath = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Enemy")
	float AggroRadius = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Enemy")
	float AttackRange = 225.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Enemy")
	float AttackDamage = 14.f;

	// --- Elemental Reaction States (Replicated) ---

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Reactions")
	int32 BurnStacks = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Reactions")
	bool bFrozen = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Reactions")
	bool bShocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Enemy")
	TObjectPtr<UPointLightComponent> StatusFlashLight;

	/** Configures archetype stats, skeletal mesh, scale, and linked quest objective. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Enemy")
	void ConfigureArchetype(EChronicleEnemyArchetype InArchetype, FName InQuestIdOnDeath = NAME_None);

	/** Applies Exort/Wex/Quas aura and triggers Thermal Shock, Superconduct, or Cryo-Shatter bursts. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Reactions")
	void ApplyElementalStatus(EChronicleOrb Element, AActor* InstigatorActor);

	/** Brief stagger interruption (e.g. triggered by hero timed parry). */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Enemy")
	virtual void ApplyStagger(float Duration);

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

protected:
	virtual void BindEnemyMeshAndAnims();
	virtual void UpdateEnemyAI(float DeltaTime);
	virtual void HandleDeath(AController* KillerController, AActor* DamageCauser);

	AAetheriaHeroCharacter* FindNearestAliveHero(float MaxDistance, float& OutDistance) const;
	void PlayEnemyAnimSequence(UAnimSequence* Seq, float PlayRate = 1.f);
	void PlayEnemyAttackAnim(float PlayRate = 1.f);
	void PlayEnemyMontage(UAnimMontage* Montage, float PlayRate = 1.f);
	void PlayEnemySingleNodeAnim(UAnimSequence* Seq, bool bLooping, float PlayRate = 1.f, bool bForceRestart = false);
	void UpdateEnemySingleNodeAnimation(float DeltaTime);
	bool BindRampageAnimsAndMontages(USkeletalMeshComponent* MeshComp);
	bool BindParagonEnemySingleNodeAnims(
		USkeletalMeshComponent* MeshComp,
		const TCHAR* IdlePrimary,
		const TCHAR* IdleFallback,
		const TCHAR* RunPrimary,
		const TCHAR* RunFallback1,
		const TCHAR* RunFallback2,
		const TCHAR* AttackAPath,
		const TCHAR* AttackBPath,
		const TCHAR* AttackCPath,
		const TCHAR* HitReactPath,
		const TCHAR* DeathPath);
	bool BindParagonEnemyAnimsAndMontages(
		USkeletalMeshComponent* MeshComp,
		const TCHAR* ABPPath,
		const TCHAR* MeleeAPath,
		const TCHAR* MeleeBPath,
		const TCHAR* MeleeCPath,
		const TCHAR* HitReactPath,
		const TCHAR* DeathPath,
		FName SlotName = FName(TEXT("FullBody")));
	void TriggerStatusFlash(const FLinearColor& Color, float PeakLumens, float Duration = 0.30f);
	void UpdateElementalStatuses(float DeltaTime);
	void InferAndApplyElementFromDamageCauser(AActor* DamageCauser, AActor* InstigatorActor);

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> EnemyIdleAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> EnemyRunAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> EnemyAttackAnims[3];

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentEnemyAnim;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimMontage>> EnemyAttackMontages;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> EnemyAttackAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> EnemyHitReactAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> EnemyDeathAnim;

	FName EnemyAnimSlotName = FName(TEXT("DefaultSlot"));
	bool bUsingSingleNodeAnims = false;
	bool bDeathAnimPlayed = false;
	float OneShotAnimTimer = 0.f;
	int32 EnemyAttackAnimCycle = 0;

	float BaseMoveSpeed = 380.f;
	float BurnTimer = 0.f;
	float BurnDotAccumulator = 0.f;
	float FreezeTimer = 0.f;
	float ShockTimer = 0.f;
	float ArmorMeltVulnerability = 0.f;
	float StaggerTimer = 0.f;

	bool bWindupActive = false;
	float WindupTimer = 0.f;
	float AttackCooldownTimer = 0.f;

	float StatusFlashTimer = 0.f;
	float StatusFlashDuration = 0.30f;
	float StatusFlashPeakLumens = 8000.f;

	bool bArchetypeConfigured = false;
	TWeakObjectPtr<AActor> LastElementInstigator;
};

// ============================================================================
// 3B. AAetheriaBossVolleyOrb — Molten Projectile for Boss AbyssalFireVolley
// ============================================================================
UCLASS()
class AETHERIACHRONICLES_API AAetheriaBossVolleyOrb : public AActor
{
	GENERATED_BODY()

public:
	AAetheriaBossVolleyOrb();

	virtual void Tick(float DeltaTime) override;

	void InitializeVolleyOrb(const FVector& InDirection, float InSpeed, float InDamage);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Boss")
	TObjectPtr<USceneComponent> OrbRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Boss")
	TObjectPtr<UStaticMeshComponent> OrbMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Boss")
	TObjectPtr<UPointLightComponent> OrbLight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OrbMID;

private:
	FVector FlightDir = FVector::ForwardVector;
	float Speed = 1450.f;
	float Damage = 24.f;
	float Elapsed = 0.f;
	bool bImpacted = false;
};

// ============================================================================
// 3C. AAetheriaBossCharacter — Malakor, Sovereign of the Shattered Seal
// Multi-Phase Wukong-Style Telegraphed Cinematic Boss (Replicated)
// ============================================================================
UCLASS(Blueprintable)
class AETHERIACHRONICLES_API AAetheriaBossCharacter : public AAetheriaEnemyCharacter
{
	GENERATED_BODY()

public:
	AAetheriaBossCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- Multi-Phase Wukong Boss State (Replicated) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|Boss")
	FString BossTitle = TEXT("Malakor, Sovereign of the Shattered Seal");

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Boss")
	int32 BossPhase = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Boss")
	float StaggerGauge = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Boss")
	float MaxStagger = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Boss")
	bool bStaggerBroken = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Boss")
	bool bBossEngaged = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Boss")
	EBossAttackKind CurrentAttack = EBossAttackKind::None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Boss")
	bool bIsTelegraphing = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Boss")
	float TelegraphTimer = 0.f;

	// --- Boss MotionWarping & Crimson Telegraph Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Boss")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Boss")
	TObjectPtr<UPointLightComponent> TelegraphCrimsonLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Boss")
	TObjectPtr<UStaticMeshComponent> TelegraphRingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Boss")
	TObjectPtr<UStaticMeshComponent> TelegraphAuraCoreMesh;

	// --- Boss API ---

	/** Adds poise/stagger damage; triggers 2.4s Stagger Break (+35% incoming dmg) at 100. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Boss")
	void AddStagger(float Amount, float BonusBreakSeconds = 0.f);

	virtual void ApplyStagger(float Duration) override;

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	/** Cleanly resets phase, HP, stagger, telegraphs, and home anchor on player respawn if alive. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Boss")
	void ResetBossEncounter();

	/** Wukong Color Law: true = Amber (Parryable for +2.5x Stagger), false = Crimson (Dodge-Only). */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Boss")
	bool IsCurrentAttackParryable() const;

protected:
	virtual void BindEnemyMeshAndAnims() override;
	virtual void UpdateEnemyAI(float DeltaTime) override;
	virtual void HandleDeath(AController* KillerController, AActor* DamageCauser) override;

private:
	void UpdateBossPhase();
	EBossAttackKind ChooseNextBossAttack(float DistToHero);
	void BeginTelegraphedAttack(EBossAttackKind AttackKind, AAetheriaHeroCharacter* TargetHero);
	void ExecuteCurrentBossAttack();
	void ExecuteCrimsonCleave();
	void ExecuteSeismicSlam();
	void ExecuteAbyssalFireVolley();
	void ExecuteTempestLunge();
	void ExecuteShatterSpin();
	void ExecuteChronoCataclysm();
	void ApplyBossRadialShockwave(
		const FVector& Origin,
		float Radius,
		float DamageAmount,
		float LaunchZ,
		bool bShatterArenaColumns);
	void UpdateTelegraphVisuals(float DeltaTime);
	void PlayBossAttackVisualAnim(int32 PreferredMontageIdx, UAnimSequence* FallbackSeq, float PlayRate = 1.f);
	static FString GetBossAttackDisplayName(EBossAttackKind AttackKind);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TelegraphCrimsonMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TelegraphAuraShellMID;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> TelegraphAuraNiagara;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> BossHeavyAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> BossRoarAnim;

	FVector BossHomeLocation = FVector::ZeroVector;
	FVector LungeTargetLocation = FVector::ZeroVector;
	TWeakObjectPtr<AAetheriaHeroCharacter> LockedTargetHero;

	static constexpr float TELEGRAPH_DURATION = 0.85f;
	static constexpr float ENGAGE_RADIUS = 3200.f;
	static constexpr float DISENGAGE_RADIUS = 5400.f;

	float StaggerBreakTimer = 0.f;
	float StaggerBreakTotalDuration = 2.4f;
	float CataclysmStillnessTimer = 0.f;
	bool bCataclysmStillnessPhase = false;
	float BossAttackCooldown = 1.6f;
	int32 AttackCycleCounter = 0;
	float ImpactLightDecayTimer = 0.f;
	float ImpactLightPeakLumens = 32000.f;
};

// ============================================================================
// 4. AAetheriaNPCActor — Named Story Quest Givers (Replicated)
// ============================================================================
UCLASS(Blueprintable)
class AETHERIACHRONICLES_API AAetheriaNPCActor : public AActor
{
	GENERATED_BODY()

public:
	AAetheriaNPCActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|NPC")
	FName NpcId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|NPC")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|NPC")
	FText RoleTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|NPC")
	FName QuestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|NPC")
	int32 VisualVariantIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|NPC")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|NPC")
	TObjectPtr<USkeletalMeshComponent> NPCMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|NPC")
	TObjectPtr<UStaticMeshComponent> QuestBeaconMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|NPC")
	TObjectPtr<UPointLightComponent> BeaconLight;

	UFUNCTION(BlueprintCallable, Category = "Aetheria|NPC")
	void ConfigureNPC(
		FName InNpcId,
		const FText& InDisplayName,
		const FText& InRoleTitle,
		FName InQuestId,
		int32 InVisualVariantIndex = 0);

private:
	void BindNPCMeshAndAnim();
	void UpdateQuestBeacon(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BeaconMID;

	float BeaconBobTime = 0.f;
	float QuestCheckTimer = 0.f;
	bool bMeshBound = false;
};

// ============================================================================
// 5. AAetheriaEarthWedgeActor — Hit-3 Martial Finisher Rising Stone Wedge
// ============================================================================
UCLASS()
class AETHERIACHRONICLES_API AAetheriaEarthWedgeActor : public AActor
{
	GENERATED_BODY()

public:
	AAetheriaEarthWedgeActor();

	virtual void Tick(float DeltaTime) override;

	void InitializeWedge(const FVector& GroundImpactPoint, const FVector& ForwardDir);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> WedgeRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> WedgeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> ShatterExplosionFX;

	FVector StartLoc = FVector::ZeroVector;
	FVector PeakLoc = FVector::ZeroVector;
	float Elapsed = 0.f;
	bool bShattered = false;
};

// ============================================================================
// 6. AAetheriaLootOrbActor — Glowing Golden Loot Pickup Dropped on Enemy Death
// ============================================================================
UCLASS()
class AETHERIACHRONICLES_API AAetheriaLootOrbActor : public AActor
{
	GENERATED_BODY()

public:
	AAetheriaLootOrbActor();

	virtual void Tick(float DeltaTime) override;

	void InitializeLoot(int32 InGold, int32 InXP, float InHealHP);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> OrbRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> OrbMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> OrbLight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OrbMID;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> CollectBurstFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> CollectBurstNS;

	FVector SpawnOrigin = FVector::ZeroVector;
	int32 GoldValue = 35;
	int32 XPValue = 45;
	float HealHP = 25.f;
	float Elapsed = 0.f;
	bool bCollected = false;
};
