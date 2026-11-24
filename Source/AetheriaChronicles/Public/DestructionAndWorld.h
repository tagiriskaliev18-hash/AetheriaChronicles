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
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
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
	void TriggerShatter(const FVector& ImpulseOrigin, const FVector& StrikeDirection);
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
	TObjectPtr<UMaterialInstanceDynamic> PropMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DebrisMID;

	FLinearColor AccentColor = FLinearColor(0.72f, 0.65f, 0.56f, 1.0f);
	bool bAccentEmissive = false;
	bool bHasCustomAccent = false;

	bool bShatterEffectsSpawned = false;
	bool bFlashActive = false;
	float FlashElapsed = 0.f;
	float FlashDuration = 0.42f;
	float FlashPeakLumens = 14000.f;
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

	/** Normalized 0..1 day/night clock. Default 0.72 = dramatic Crimson Desert Golden Sunset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aetheria|Atmosphere", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TimeOfDay = 0.72f;

	/** Cycles Golden Sunset (0.72) -> Starry Full Moon (0.02) -> Misty Dawn (0.26) -> Highland Noon (0.48). */
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

	UStaticMesh* LoadMeshSafe(const TCHAR* AssetPath, const TCHAR* FallbackPath = TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* LoadPBRMaterial(const TArray<const TCHAR*>& Candidates);
	UParticleSystem* LoadParticleSafe(const TCHAR* AssetPath);
	UMaterialInstanceDynamic* CreateColorMID(const FLinearColor& Color, bool bEmissive = false);
	static bool IsGreyboxOrMissingMaterial(const UMaterialInterface* Mat);

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
	TArray<TObjectPtr<UStaticMeshComponent>> SpawnedMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UParticleSystemComponent>> SpawnedParticles;

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

	float FallbackGroundZ = 0.f;
	float ElapsedAnimTime = 0.f;
	float LandscapePatchTimer = 0.f;
	int32 ActivePresetIndex = 0;
	bool bWorldBuilt = false;
	bool bRTXActiveCached = false;
};
