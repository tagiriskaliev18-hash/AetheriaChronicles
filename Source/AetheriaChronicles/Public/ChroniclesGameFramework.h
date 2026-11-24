// ============================================================================
// ChroniclesGameFramework.h — GameState, GameMode, PlayerController (with
// Input Hygiene, Co-op Commands & -wizardsmoke Harness), Fog-of-War Slate Map
// Widget & Production Slate HUD / Interactive Overlays for AetheriaChronicles
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "ChroniclesTypes.h"
#include "ChroniclesGameFramework.generated.h"

class AAetheriaBossCharacter;
class AAetheriaDragonActor;
class AAetheriaEnemyCharacter;
class AAetheriaHeroCharacter;
class AAetheriaNPCActor;
class AAetheriaWorldBuilder;
class AChroniclesPlayerController;
class APlayerStart;
class SAetheriaChroniclesHUD;
class SBorder;
class SVerticalBox;
class SWidget;
class UFogOfWarMapSubsystem;
class UGraphicsRTXSubsystem;
class UQuestSubsystem;

/** Active fullscreen or modal Slate overlay managed by AChroniclesPlayerController. */
UENUM(BlueprintType)
enum class EChroniclesOverlayMode : uint8
{
	None,
	WorldMap,
	SkillTree,
	QuestLog,
	Grimoire,
	Dialogue,
	GraphicsRTX,
	HeroCustomizer
};

// ============================================================================
// 1. AChroniclesGameState — Replicated World Clock, Co-op Count & Banner
// ============================================================================
UCLASS()
class AETHERIACHRONICLES_API AChroniclesGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AChroniclesGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|World")
	float ReplicatedTimeOfDay = 0.72f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Aetheria|Multiplayer")
	int32 ConnectedPlayers = 1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|World")
	FString ActiveWorldBanner;
};

// ============================================================================
// 2. AChroniclesGameMode — Authoritative World, Dragon, NPC, Enemy & Boss Spawner
// ============================================================================
UCLASS()
class AETHERIACHRONICLES_API AChroniclesGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AChroniclesGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	AAetheriaWorldBuilder* GetWorldBuilder() const { return WorldBuilder.Get(); }

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	AAetheriaDragonActor* GetWorldDragon() const { return WorldDragon.Get(); }

	UFUNCTION(BlueprintPure, Category = "Aetheria|World")
	AAetheriaBossCharacter* GetWorldBoss() const { return WorldBoss.Get(); }

	/** Grounds a world XY coordinate using WorldBuilder or direct ECC_WorldStatic vertical trace. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|World")
	float SampleGroundZSafe(float X, float Y, float FallbackZ = 0.f) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|World")
	TObjectPtr<AAetheriaWorldBuilder> WorldBuilder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|World")
	TObjectPtr<AAetheriaDragonActor> WorldDragon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aetheria|World")
	TObjectPtr<AAetheriaBossCharacter> WorldBoss;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AAetheriaNPCActor>> SpawnedNPCs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AAetheriaEnemyCharacter>> SpawnedEnemies;

	UPROPERTY(Transient)
	TObjectPtr<APlayerStart> OakhavenPlayerStart;

private:
	void SpawnOpenWorldActors();
	void SpawnNamedQuestNPCs();
	void SpawnRoamingBiomeEnemies();
	void UpdateConnectedPlayerCount();

	bool bWorldActorsSpawned = false;
	int32 SpawnedHeroCounter = 0;
};

// ============================================================================
// 3. AChroniclesPlayerController — Input Hygiene, Combat/Invoker/Dragon Controls,
//    Interactive Overlays, Co-op Commands & Automated -wizardsmoke Harness
// ============================================================================
UCLASS()
class AETHERIACHRONICLES_API AChroniclesPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AChroniclesPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;

	// --- Mouse Capture & Modal UI Hygiene ---

	UPROPERTY(BlueprintReadOnly, Category = "Aetheria|Input")
	bool bMouseCaptured = true;

	UPROPERTY(BlueprintReadOnly, Category = "Aetheria|UI")
	EChroniclesOverlayMode ActiveOverlay = EChroniclesOverlayMode::None;

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Input")
	void ToggleMouseCapture();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Input")
	void SetUIMode(bool bOpenUI);

	UFUNCTION(BlueprintPure, Category = "Aetheria|UI")
	bool IsAnyModalOpen() const;

	UFUNCTION(BlueprintCallable, Category = "Aetheria|UI")
	void ToggleOverlay(EChroniclesOverlayMode TargetOverlay);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|UI")
	void CloseAllOverlays();

	// --- Gameplay & HUD Query Helpers ---

	/** Returns the controlled Hero, or the Hero currently riding a possessed Dragon. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Gameplay")
	AAetheriaHeroCharacter* GetActiveHero() const;

	/** Returns the possessed Dragon if currently flying, otherwise nullptr. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Gameplay")
	AAetheriaDragonActor* GetControlledDragon() const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Gameplay")
	bool IsRidingDragon() const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Gameplay")
	bool IsHeroDead() const;

	AAetheriaNPCActor* FindNearestInteractableNPC(float MaxDistance = 420.f) const;
	AAetheriaDragonActor* FindNearestDragon(float MaxDistance = 1400.f) const;
	AAetheriaBossCharacter* FindWorldBoss() const;
	AAetheriaWorldBuilder* FindWorldBuilder() const;

	AAetheriaNPCActor* GetDialogueNPC() const { return ActiveDialogueNPC.Get(); }

	// --- Gameplay Actions (callable from Keyboard or Slate Buttons) ---

	void HandleInteractOrExort();
	void OpenDialogueWithNPC(AAetheriaNPCActor* NPC);
	void HandleDialoguePrimaryAction();
	void TryToggleDragonMount();
	void RequestCycleTimeOfDay();
	void RequestRespawnHero();
	void RequestFastTravelToPin(FName PinId);
	void PushToastMessage(const FString& Message, float DurationSeconds = 4.2f);
	void AdjustCreatorCameraOrbit(float DeltaYawDeg);
	void SetCreatorCameraZoom(int32 NewZoomMode);
	int32 GetCreatorCameraZoom() const { return CreatorZoomMode; }
	float GetCreatorCameraOrbitYaw() const { return CreatorOrbitYaw; }

	const FString& GetActiveToastMessage() const { return ActiveToastText; }
	float GetActiveToastAlpha() const;

	// --- Multiplayer Console Commands ---

	UFUNCTION(Exec)
	void HostCoop();

	UFUNCTION(Exec)
	void JoinCoop(const FString& ServerIP);

protected:
	UFUNCTION(Server, Reliable)
	void ServerCycleTimeOfDay();

	UFUNCTION(Server, Reliable)
	void ServerToggleDragonMount();

	UFUNCTION(Server, Reliable)
	void ServerFastTravel(const FVector& TargetLocation);

	UFUNCTION(Server, Reliable)
	void ServerApplyDragonFlightInput(
		float ForwardAxis,
		float RightAxis,
		float UpAxis,
		float YawInput,
		float PitchInput,
		bool bFireHeld,
		float DeltaTime);

private:
	void MountChroniclesHUD();
	void UnmountChroniclesHUD();
	void HandleApplicationActivationChanged(bool bIsActive);

	void HandleGlobalAndOverlayKeys();
	void HandleHeroGameplayInput(AAetheriaHeroCharacter* Hero, float DeltaTime);
	void HandleDragonGameplayInput(AAetheriaDragonActor* Dragon, float DeltaTime);
	void UpdateToastNotifications(float DeltaTime);
	void UpdateDeathOverlayState();

	bool ComputeSpellAim(AAetheriaHeroCharacter* Hero, FVector& OutOrigin, FVector& OutDir, FVector& OutTargetPoint) const;

	// --- Automated -game -wizardsmoke Verification & -captureframes Visual Critic Harness ---
	void StartSmokeTestIfRequested();
	void RunSmokeTestTick(float DeltaTime);
	void AbortSmokeTestWithError(const FString& Reason);
	void StartCaptureFramesIfRequested();
	void RunCaptureFramesTick(float DeltaTime);

	TSharedPtr<SAetheriaChroniclesHUD> HUDWidget;
	TSharedPtr<SWidget> HUDViewportContainer;

	FDelegateHandle AppActivationHandle;
	TWeakObjectPtr<AAetheriaNPCActor> ActiveDialogueNPC;
	mutable TWeakObjectPtr<AAetheriaBossCharacter> CachedBossRef;
	mutable TWeakObjectPtr<AAetheriaWorldBuilder> CachedWorldBuilderRef;

	float MouseLookSensitivity = 1.85f;
	bool bLastBlockKeyDown = false;
	bool bLastKnownHeroDead = false;
	bool bRefocusClickSwallowed = false;

	// Toast notification display state
	FString ActiveToastText;
	float ActiveToastRemaining = 0.f;
	float ActiveToastDuration = 4.2f;

	// -wizardsmoke state machine (strictly DeltaSeconds-driven, zero frame counts)
	bool bSmokeTestActive = false;
	int32 SmokeStepIndex = 0;
	int32 SmokeSubPhase = 0;
	float SmokeStepElapsed = 0.f;
	float SmokeTotalElapsed = 0.f;
	FVector SmokeDragonStartLoc = FVector::ZeroVector;
	bool bSmokeObservedFireBreath = false;
	bool bSmokeBossEngaged = false;
	bool bSmokeBossTelegraphed = false;

	// Adaptive music zone check throttle
	float MusicZoneTimer = 0.f;

	// -captureframes RTX visual critic capture state machine
	bool bCaptureFramesActive = false;
	int32 CaptureStage = 0;
	bool bCaptureStageSetupDone = false;
	float CaptureStageElapsed = 0.f;

	// Live 3D Hero Customizer studio camera orbit & zoom state
	float CreatorOrbitYaw = 165.f;
	int32 CreatorZoomMode = 0;
};

// ============================================================================
// 4A. SFogOfWarMapWidget — Crisp 64x64 Fog-of-War & Landmark Slate Leaf Widget
// ============================================================================
class AETHERIACHRONICLES_API SFogOfWarMapWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFogOfWarMapWidget)
		: _OwnerPC(nullptr)
		, _MapSize(520.f)
	{}
		SLATE_ARGUMENT(TWeakObjectPtr<AChroniclesPlayerController>, OwnerPC)
		SLATE_ARGUMENT(float, MapSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	static FLinearColor GetBiomeMapColor(EChronicleBiome Biome);

	TWeakObjectPtr<AChroniclesPlayerController> OwnerPC;
	float DesiredMapSize = 520.f;
};

// ============================================================================
// 4B. SAetheriaChroniclesHUD — Production Slate HUD & Interactive Overlays
// ============================================================================
class AETHERIACHRONICLES_API SAetheriaChroniclesHUD : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAetheriaChroniclesHUD)
		: _OwnerPC(nullptr)
	{}
		SLATE_ARGUMENT(TWeakObjectPtr<AChroniclesPlayerController>, OwnerPC)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rebuilds dynamic overlay lists (Skill Tree columns, Quest Chronicle, Grimoire, Landmarks). */
	void RefreshDynamicPanels();

private:
	// --- Persistent HUD Regions ---
	TSharedRef<SWidget> BuildTopCenterCompassAndBossBar();
	TSharedRef<SWidget> BuildTopRightStatusAndHotkeys();
	TSharedRef<SWidget> BuildBottomLeftVitalsAndInvokerPanel();
	TSharedRef<SWidget> BuildBottomCenterPromptsAndToast();

	// --- Interactive Modal Overlays ---
	TSharedRef<SWidget> BuildWorldMapOverlay();
	TSharedRef<SWidget> BuildSkillTreeOverlay();
	TSharedRef<SWidget> BuildQuestLogOverlay();
	TSharedRef<SWidget> BuildGrimoireOverlay();
	TSharedRef<SWidget> BuildDialogueOverlay();
	TSharedRef<SWidget> BuildGraphicsRTXOverlay();
	TSharedRef<SWidget> BuildHeroCustomizerOverlay();
	TSharedRef<SWidget> BuildGameOverOverlay();

	// --- Dynamic Panel Builders ---
	void RebuildSkillTreeColumns();
	void RebuildQuestList();
	void RebuildGrimoireGrid();
	void RebuildMapLandmarksList();
	void RebuildHeroCustomizerPanels();

	// --- Shared Styling & Formatting Helpers ---
	TSharedRef<SWidget> BuildResourceBarRow(
		const FText& LabelText,
		const FLinearColor& FillColor,
		TAttribute<TOptional<float>> PercentAttr,
		TAttribute<FText> ValueTextAttr) const;

	TSharedRef<SWidget> BuildOrbBadgeWidget(EChronicleOrb Orb, const FString& KeyHint) const;
	TSharedRef<SWidget> BuildSpellSlotWidget(bool bIsSlotF) const;

	static FLinearColor GetOrbColor(EChronicleOrb Orb);
	static FString GetOrbLetter(EChronicleOrb Orb);
	static FString GetBossAttackWarningText(EBossAttackKind AttackKind);
	static FString GetQuestStateLabel(EQuestState State);
	static FLinearColor GetQuestStateColor(EQuestState State);

	// --- Safe Weak Accessor (ZERO raw UObject* pointers inside Slate!) ---
	TWeakObjectPtr<AChroniclesPlayerController> OwnerPC;

	TSharedPtr<SVerticalBox> SkillBranchBoxes[4];
	TSharedPtr<SVerticalBox> QuestListBox;
	TSharedPtr<SVerticalBox> GrimoireLeftCol;
	TSharedPtr<SVerticalBox> GrimoireRightCol;
	TSharedPtr<SVerticalBox> MapLandmarksBox;
	TSharedPtr<SVerticalBox> CreatorLeftContentBox;
	TSharedPtr<SVerticalBox> CreatorRightStatsBox;
	int32 CreatorActiveTab = 0;
};
