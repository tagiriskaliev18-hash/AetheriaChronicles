// ============================================================================
// AudioAndStyleSubsystem.h — Crimson Desert Visual Style Harmonizer & Dual-Deck
// Dynamic Music Subsystem for AetheriaChronicles (UE 5.8.4)
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "AudioAndStyleSubsystem.generated.h"

class AActor;
class UAudioComponent;
class USoundBase;
struct FPostProcessSettings;

UENUM(BlueprintType)
enum class EChronicleStyleCategory : uint8
{
	Character    UMETA(DisplayName = "Character"),
	Creature     UMETA(DisplayName = "Creature"),
	Architecture UMETA(DisplayName = "Architecture"),
	Prop         UMETA(DisplayName = "Prop")
};

UENUM(BlueprintType)
enum class EChronicleMusicState : uint8
{
	None    UMETA(DisplayName = "None"),
	Village UMETA(DisplayName = "Village"),
	Field   UMETA(DisplayName = "Field"),
	Battle  UMETA(DisplayName = "Battle"),
	Boss    UMETA(DisplayName = "Boss")
};

/**
 * Grounded AgX/Filmic post-process grading and PBR material harmonizer
 * enforcing cohesive Crimson Desert tonal, roughness, and specular budgets.
 */
struct AETHERIACHRONICLES_API FAetheriaStyleHarmonizer
{
	static void ApplyCrimsonDesertPostProcess(FPostProcessSettings& PP);
	static void HarmonizeActorMaterials(AActor* Actor, EChronicleStyleCategory Category);
};

/**
 * World subsystem driving dual-deck 2D music crossfades across
 * Village, Field, Battle, and Boss encounters with 2.5s combat hysteresis.
 */
UCLASS()
class AETHERIACHRONICLES_API UAetheriaMusicSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UAetheriaMusicSubsystem* Get(const UObject* WorldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Audio")
	void SetExplorationZone(bool bInVillage);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Audio")
	void NotifyCombatActivity();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Audio")
	void SetBossEncounterActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "Aetheria|Audio")
	EChronicleMusicState GetCurrentState() const { return CurrentState; }

private:
	void LoadCueForState(EChronicleMusicState State, const TCHAR* ObjectPath);
	void TransitionToState(EChronicleMusicState NewState);
	void ScheduleReturnToExploration(float DelaySeconds);
	EChronicleMusicState GetExplorationState() const;

	UPROPERTY()
	TObjectPtr<UAudioComponent> DeckA;

	UPROPERTY()
	TObjectPtr<UAudioComponent> DeckB;

	UPROPERTY()
	TObjectPtr<USoundBase> StateCues[5];

	int32 ActiveDeckIndex = 0;
	EChronicleMusicState CurrentState = EChronicleMusicState::None;
	bool bIsInVillage = false;
	bool bBossActive = false;
	FTimerHandle CombatDecayTimerHandle;
};

/**
 * Dilation-safe Black Myth: Wukong micro-hitstop subsystem.
 * Uses GetRealTimeSeconds() so actor CustomTimeDilation recovers accurately
 * without ever dilating the player camera manager.
 */
UCLASS()
class AETHERIACHRONICLES_API UHitStopSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UHitStopSubsystem* Get(const UObject* WorldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Combat")
	void RequestHitStop(AActor* ActorA, AActor* ActorB, float DurationSeconds = 0.055f, float Dilation = 0.06f);

private:
	struct FActiveHitStop
	{
		TWeakObjectPtr<AActor> Actor;
		double EndRealTime = 0.0;
	};

	TArray<FActiveHitStop> ActiveStops;
};
