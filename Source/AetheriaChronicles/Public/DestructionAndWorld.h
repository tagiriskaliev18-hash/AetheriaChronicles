// ============================================================================
// DestructionAndWorld.h — Replicated Physics-Shattering Destructible Props
// & Crimson Desert-Inspired 5-Biome Open World / Dynamic Day-Night Builder
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChroniclesTypes.h"
#include "DestructionAndWorld.generated.h"

class AController;
class ADirectionalLight;
class AExponentialHeightFog;
class APostProcessVolume;
class ASkyAtmosphere;
class ASkyLight;
class AVolumetricCloud;
class UAudioComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class UParticleSystem;
class UParticleSystemComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FDamageEvent;

/**
 * Replicated destructible world prop (stone columns, barrier crystals,
 * storm monoliths, dragon bind-crystals, obsidian seal walls, timber crates).
 * Shatters into 6-8 physics-simulating debris chunks with a physical AgX
 * 14,000 lm point-light flash and optional quest-progress notification.
 */
UCLASS(Blueprintable)
class AETHERIACHRONICLES_API ADestructiblePropActor : public AActor
{
	GENERATED_BODY()

public:
	ADestructiblePropActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Destruction")
	EDestructibleKind MaterialKind = EDestructibleKind::Stone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Aetheria|Destruction")
	float HP = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Destruction")
	float MaxHP = 120.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Broken, Category = "Aetheria|Destruction")
	bool bBroken = false;

	/** If non-None, breaking this prop calls UQuestSubsystem::AdvanceQuestProgress(LinkedQuestId, 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Destruction")
	FName LinkedQuestId = NAME_None;

	/** When true, prop acts as a sliceable meadow tree (Niagara leaf burst + wood stump + smooth topple, no rune crystal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Destruction")
	bool bIsSliceableTree = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Destruction")
	TObjectPtr<UStaticMeshComponent> IntactMesh;

	/** Configures mesh, scale, hit points, material kind, and optional linked quest id. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Destruction")
	void ConfigureProp(
		EDestructibleKind InKind,
		float InMaxHP,
		const FString& OptionalMeshPath,
		const FVector& Scale,
		FName InQuestId = NAME_None);

	/** Applies an optional emissive or tinted dynamic material accent to the intact prop and its debris. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Destruction")
	void ApplyAccentGlow(const FLinearColor& GlowColor, bool bUseEmissive = true);

	/** Immediately shatters the prop (used by lethal damage, boss slam triggers, or smoke verification). */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Destruction")
	void ForceShatter(const FVector& ImpulseOrigin = FVector::ZeroVector);

	/** Magically restores a shattered prop or sliced tree back to intact state with glowing restoration VFX. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Destruction")
	void TriggerMagicalRestoration();

	/** Time in seconds before a broken world prop or sliced tree magically restores itself (default 60s = 1 minute). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Destruction")
	float MagicalRestoreDelaySeconds = 60.f;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Destruction")
	bool IsBroken() const { return bBroken; }

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

protected:
	UFUNCTION()
	void OnRep_Broken();

private:
	void CaptureInitialTransformIfNeeded();
	void TriggerShatter(const FVector& ImpulseOrigin, const FVector& StrikeDirection);
	void CleanupDebrisChunks();
	void NotifyQuestProgress();
	FLinearColor GetDefaultAccentColor(EDestructibleKind Kind) const;
	FLinearColor GetFlashColor(EDestructibleKind Kind) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> DebrisChunks;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> ShatterFlashLight;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> RuneAccentMesh;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> RuneAccentLight;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TreeStumpMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PropMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DebrisMID;

	FLinearColor AccentColor = FLinearColor(0.72f, 0.65f, 0.56f, 1.0f);
	bool bAccentEmissive = false;
	bool bHasCustomAccent = false;

	FVector InitialActorLocation = FVector::ZeroVector;
	FRotator InitialActorRotation = FRotator::ZeroRotator;
	FVector InitialMeshScale = FVector::OneVector;
	FRotator InitialMeshRelRotation = FRotator::ZeroRotator;
	bool bInitialTransformCaptured = false;
	bool bQuestProgressNotified = false;

	bool bShatterEffectsSpawned = false;
	bool bFlashActive = false;
	float FlashElapsed = 0.f;
	float FlashDuration = 0.42f;
	float FlashPeakLumens = 14000.f;

	bool bTreeToppling = false;
	float TreeToppleElapsed = 0.f;
	float TreeToppleDuration = 1.35f;
	FQuat TreeStartQuat = FQuat::Identity;
	FVector TreeFallAxis = FVector::RightVector;

	float DebrisCleanupRemaining = 0.f;
	bool bAwaitingMagicalRestore = false;
	float MagicalRestoreRemaining = 0.f;
	bool bMagicalRegrowthActive = false;
	float MagicalRegrowthElapsed = 0.f;
	float MagicalRegrowthDuration = 1.15f;
};

/** Tracks a flattened HISM meadow/twig/flower instance inside an active spell terrain scar. */
USTRUCT()
struct FFlattenedFoliageRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISM = nullptr;

	int32 InstanceIndex = INDEX_NONE;
	uint64 FoliageKey = 0;
};

/** Tracks a live spell-impact terrain scar & flattened foliage patch awaiting 60-second magical restoration. */
USTRUCT()
struct FActiveTerrainScar
{
	GENERATED_BODY()

	FVector WorldCenter = FVector::ZeroVector;
	float Radius = 320.f;
	EChronicleOrb Element = EChronicleOrb::Exort;
	float TimeRemaining = 60.f;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> ScarMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UParticleSystemComponent>> ScarParticles;

	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> ScarNiagara;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> ScarLight = nullptr;

	float BaseLightLumens = 420.f;
	TArray<FFlattenedFoliageRecord> FlattenedFoliage;
};

/** Tracks a world fire/lantern PointLight for dynamic flicker & dusk/night intensity boost. */
USTRUCT()
struct FWorldAnimatedLight
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light = nullptr;

	float BaseLumens = 4000.f;
	float FlickerAmplitude = 0.14f;
	float FlickerSpeed = 6.5f;
	float PhaseOffset = 0.f;
	bool bNightBoost = true;
};

/**
 * Runtime Crimson Desert-inspired 5-Biome Open World & Dynamic Day/Night Builder.
 * Grounds every landmark onto the real UE5 OpenWorld landscape via ECC_WorldStatic
 * line traces, configures AgX/Lumen atmosphere & volumetric clouds, and spawns
 * all quest destructibles, instanced highland forests/boulders, boss arena columns,
 * the dark lava Ember Crypt, Crystal Glacier Peaks, and Dragon Sky Rune Rings.
 */
UCLASS(Blueprintable)
class AETHERIACHRONICLES_API AAetheriaWorldBuilder : public AActor
{
	GENERATED_BODY()

public:
	AAetheriaWorldBuilder();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Normalized 0..1 day/night clock. Default 0.68 = dramatic Crimson Desert Golden Hour with volumetric sun rays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Atmosphere", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TimeOfDay = 0.68f;

	/** Cycles Golden Sunset (0.68) -> Starry Full Moon (0.02) -> Misty Dawn (0.26) -> Highland Noon (0.48). */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Atmosphere")
	void CycleTimeOfDayPreset();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Atmosphere")
	void SetTimeOfDay(float NewTimeOfDay);

	UFUNCTION(BlueprintPure, Category = "Aetheria|Atmosphere")
	FString GetTimeOfDayLabel() const;

	/** Traces vertically from Z = +20000 down to -10000 against ECC_WorldStatic to ground props cleanly. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|World")
	float SampleGroundZ(float X, float Y, float FallbackZ = 0.f) const;

	/** Determines which of the 5 major biomes a world coordinate belongs to. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	EChronicleBiome GetBiomeAtLocation(const FVector& WorldLoc) const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	int32 GetDestructiblePropCount() const { return SpawnedDestructibles.Num(); }

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	TArray<ADestructiblePropActor*> GetDestructibleProps() const;

	// --- Spell-Reactive Landscape Scarring & 60s Magical Restoration ---
	UFUNCTION(BlueprintCallable, Category = "Aetheria|World")
	void NotifySpellTerrainImpact(const FVector& WorldLocation, float ImpactRadius, EChronicleOrb PrimaryElement);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|World")
	int32 RestoreAllActiveTerrainScarsNow();

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	int32 GetActiveTerrainScarCount() const { return ActiveTerrainScars.Num(); }

	// --- Prologue Awakening Cave, Cliff Overlook & Waterfall Shared Coordinates ---
	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	FVector GetPrologueCaveSpawnLocation() const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	FVector GetPrologueTutorLocation() const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	FVector GetPrologueWaterfallBrinkLocation() const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	FVector GetPrologueLakePlungeLocation() const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	FVector GetPrologueShoreEmergenceLocation() const;

	// --- Managed Atmosphere & Lighting Actors ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Atmosphere")
	TObjectPtr<ADirectionalLight> SunLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Atmosphere")
	TObjectPtr<ASkyAtmosphere> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Atmosphere")
	TObjectPtr<ASkyLight> SkyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Atmosphere")
	TObjectPtr<AExponentialHeightFog> HeightFog;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Atmosphere")
	TObjectPtr<AVolumetricCloud> VolumetricCloud;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|Atmosphere")
	TObjectPtr<APostProcessVolume> PostProcessVolume;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|World")
	TObjectPtr<USceneComponent> WorldRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|World")
	TObjectPtr<UInstancedStaticMeshComponent> MossyBouldersISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|World")
	TObjectPtr<UInstancedStaticMeshComponent> TreeTrunksISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|World")
	TObjectPtr<UInstancedStaticMeshComponent> TreeCanopiesISM;

private:
	void SetupAtmosphereAndLighting();
	void UpdateAtmosphereForTimeOfDay(float NormalizedTime);
	void UpdateNightSkyVisibility(float NormalizedTime);
	void PatchOpenWorldLandscapeMaterials();

	void BuildAllBiomes();
	void BuildOakhavenVillage();
	void BuildMirrorLakeAndBridge();
	void BuildDragonAerie();
	void BuildSapphireWhisperingWoods();
	void BuildHighkeepCitadel();
	void BuildAshCalderaAndEmberCrypt();
	void BuildCrystalGlacierPeaks();
	void BuildSkyRuneRings();
	void BuildKingsRoadAndLanterns();
	void BuildLushMeadowFoliageAndWind();
	void BuildMicroDetailTwigsBarkAndPebbles();
	void BuildNightSkyMoonAndStars();
	void BuildPrologueCaveAndWaterfall(float WaterZ);

	void UpdateTerrainScarsAndMagicalRestoration(float DeltaTime);
	void RestoreSingleTerrainScar(int32 ScarIndex, bool bSpawnMagicalVFX);

	float ComputeMirrorLakeWaterZ() const;

	UStaticMesh* LoadMeshSafe(const TCHAR* AssetPath, const TCHAR* FallbackPath = TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* LoadPBRMaterial(const TArray<const TCHAR*>& Candidates);
	UParticleSystem* LoadParticleSafe(const TCHAR* AssetPath);
	UNiagaraSystem* LoadNiagaraSafe(const TCHAR* AssetPath);
	UMaterialInstanceDynamic* CreateColorMID(const FLinearColor& Color, bool bEmissive = false);
	static bool IsGreyboxOrMissingMaterial(const UMaterialInterface* Mat);
	bool IsInsideNoGrassZone(float X, float Y) const;

	UHierarchicalInstancedStaticMeshComponent* CreateFoliageHISM(
		const FString& CompName,
		const TCHAR* MeshPath,
		UMaterialInterface* OverrideMat,
		bool bCastShadow,
		int32 StartCullDist,
		int32 EndCullDist,
		bool bEnableCollision = false);

	UNiagaraComponent* SpawnAmbientWindNiagara(
		const FString& CompName,
		const TCHAR* NiagaraPath,
		const FVector& WorldLoc,
		const FVector& WorldScale,
		const FRotator& WorldRot = FRotator::ZeroRotator);

	void AddCurbedFloorBorder(
		const FString& Prefix,
		float CenterX,
		float CenterY,
		float HalfExtentX,
		float HalfExtentY,
		float FixedBaseZ,
		bool bFollowTerrain,
		bool bOpenNorth = false,
		bool bOpenSouth = false,
		bool bOpenEast = false,
		bool bOpenWest = false);

	void AddBoulderCluster(
		const FString& Prefix,
		float CenterX,
		float CenterY,
		int32 Count,
		float TargetHeightMin,
		float TargetHeightMax,
		FRandomStream& Rng,
		bool bWithBush = true);

	UStaticMeshComponent* AddWorldMesh(
		const FString& CompName,
		const TCHAR* MeshPath,
		const FVector& WorldLoc,
		const FRotator& WorldRot,
		const FVector& WorldScale,
		UMaterialInterface* OverrideMat = nullptr,
		const TCHAR* FallbackMesh = TEXT("/Engine/BasicShapes/Cube.Cube"),
		bool bEnableCollision = true,
		USceneComponent* AttachParent = nullptr);

	UParticleSystemComponent* SpawnFireParticle(
		const FString& CompName,
		const TCHAR* ParticlePath,
		const FVector& WorldLoc,
		const FVector& WorldScale,
		const FRotator& WorldRot = FRotator::ZeroRotator);

	void BuildTiledStoneFloor(
		const FString& Prefix,
		float CenterX,
		float CenterY,
		float HalfExtentX,
		float HalfExtentY,
		float TileStep,
		float LiftCm,
		const TCHAR* FloorMeshPath,
		UMaterialInterface* PrimaryMat,
		UMaterialInterface* AccentMat = nullptr,
		float AccentHalfWidthX = 0.f,
		bool bFollowTerrain = true,
		float FixedBaseZ = 0.f);

	UPointLightComponent* AddAmbientFireLight(
		const FString& CompName,
		const FVector& WorldLoc,
		float BaseLumens,
		float TempKelvin,
		float AttenuationRadius,
		const FLinearColor& LightColor,
		float FlickerAmplitude = 0.14f,
		float PhaseOffset = 0.f,
		bool bNightBoost = true,
		float SpecularScale = 0.14f);

	ADestructiblePropActor* SpawnDestructibleProp(
		const FVector& WorldLoc,
		const FRotator& WorldRot,
		EDestructibleKind Kind,
		float InMaxHP,
		const FString& MeshPath,
		const FVector& Scale,
		FName QuestId,
		const FLinearColor* OptionalGlow = nullptr,
		bool bEmissiveGlow = false);

	void AddBrazierAssembly(
		const FString& Prefix,
		const FVector& GroundLoc,
		float Lumens,
		float AttenuationRadius,
		const TCHAR* BrazierMeshPath = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Fire_Pit.SM_Fire_Pit"),
		float ScaleFactor = 1.0f,
		float PhaseOffset = 0.f);

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UStaticMesh>> MeshCache;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMaterialInterface>> MaterialCache;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UParticleSystem>> ParticleCache;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UNiagaraSystem>> NiagaraCache;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SpawnedMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> FoliageHISMs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UParticleSystemComponent>> SpawnedParticles;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> SpawnedNiagaraSystems;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> NightFullMoonMesh;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> NightStarsISM;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MoonMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StarMID;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LavaFloorMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MirrorWaterMID;

	UPROPERTY(Transient)
	TArray<FWorldAnimatedLight> AnimatedLights;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ADestructiblePropActor>> SpawnedDestructibles;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> SkyRuneRingPivots;

	UPROPERTY(Transient)
	TArray<FActiveTerrainScar> ActiveTerrainScars;

	TMap<uint64, FTransform> OriginalFoliageTransformMap;

	float FallbackGroundZ = 0.f;
	float ElapsedAnimTime = 0.f;
	float LandscapePatchTimer = 0.f;
	int32 ActivePresetIndex = 0;
	bool bWorldBuilt = false;
	bool bRTXActiveCached = false;
};
