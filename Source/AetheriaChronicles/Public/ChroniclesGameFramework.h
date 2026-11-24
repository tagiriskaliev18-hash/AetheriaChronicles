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
class ACameraActor;
class AChroniclesPlayerController;
class APlayerStart;
class SAetheriaChroniclesHUD;
class SBorder;
class SEditableTextBox;
class SScrollBox;
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
	MainMenu,
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

	UPROPERTY(BlueprintReadOnly, Category = "Aetheria|UI")
	bool bHasEnteredGameWorld = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aetheria|UI")
	bool bIsNewGameCreationFlow = false;

	/** 0=InCaveTalkToTutor, 1=WalkOutToOverlook, 2=ApproachWaterfall, 3=PlayingWaterfallCutscene, 4=CompletedAdventureActive */
	UPROPERTY(BlueprintReadOnly, Category = "Aetheria|Prologue")
	uint8 PrologueStage = 4;

	bool bMainMenuReturnTarget = false;

	UFUNCTION(BlueprintCallable, Category = "Aetheria|UI")
	void EnterGameFromTitleScreen(bool bLoadSavedProgress = false);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Prologue")
	void StartNewGameCharacterCreation();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Prologue")
	void ConfirmNewHeroAndAwakenInCave();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Prologue")
	void StartPrologueWaterfallCutscene();

	UFUNCTION(BlueprintPure, Category = "Aetheria|Prologue")
	bool IsPrologueCutsceneActive() const { return PrologueStage == 3; }

	/** Returns the live tracked waypoint world coordinate (Prologue stage target or tracked quest objective). */
	FVector GetActiveWaypointWorldLocation(FString& OutLabel) const;

	UFUNCTION(BlueprintCallable, Category = "Aetheria|UI")
	void OpenMainMenuOverlay();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|UI")
	void RequestQuitApplication();

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
	AAetheriaNPCActor* FindTalkableNPC(float MaxDistance = 280.f) const;
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

	// --- Multiplayer & Developer Console Commands (` key -> /godmode) ---

	virtual FString ConsoleCommand(const FString& Command, bool bWriteToLog = true) override;

	UFUNCTION(Exec)
	void GodMode();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Console")
	void ToggleDeveloperConsole();

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Console")
	void SetDeveloperConsoleOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Aetheria|Console")
	bool IsDeveloperConsoleOpen() const { return bDevConsoleOpen; }

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Console")
	bool ExecuteDeveloperConsoleCommand(const FString& RawCommand);

	void AppendConsoleLogLine(const FString& Line, const FLinearColor& Color = FLinearColor(0.88f, 0.86f, 0.82f, 1.0f));
	const TArray<TPair<FString, FLinearColor>>& GetConsoleLogLines() const { return ConsoleLogLines; }
	const TArray<FString>& GetConsoleCommandHistory() const { return ConsoleCommandHistory; }
	double GetLastConsoleOpenRealTime() const { return LastConsoleOpenRealTime; }

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
	bool WasConsoleToggleKeyJustPressed() const;
	void HandleHeroGameplayInput(AAetheriaHeroCharacter* Hero, float DeltaTime);
	void HandleDragonGameplayInput(AAetheriaDragonActor* Dragon, float DeltaTime);
	void UpdateToastNotifications(float DeltaTime);
	void UpdateDeathOverlayState();
	void UpdatePrologueAndWaterfallCutscene(float DeltaTime);
	void EndPrologueWaterfallCutscene();

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

	// Gentle cinematic camera sway angle on the Launch / Title Screen
	float TitleCameraOrbitAngle = 0.0f;

	// Prologue Waterfall Plunge & Shore Surfacing 3-shot cutscene state
	float WaterfallCutsceneTimer = 0.0f;
	bool bWaterfallSplashSpawned = false;
	FVector CutsceneStartHeroLoc = FVector::ZeroVector;
	TWeakObjectPtr<ACameraActor> CutsceneCameraActor;

	// In-game Developer Console (` / ~ / ё) state & history
	bool bDevConsoleOpen = false;
	bool bPendingConsoleFocus = false;
	double LastConsoleToggleRealTime = -10.0;
	double LastConsoleOpenRealTime = -10.0;
	TArray<TPair<FString, FLinearColor>> ConsoleLogLines;
	TArray<FString> ConsoleCommandHistory;
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
// 4B. SChroniclesMinimapWidget — 172x172 Round Bottom-Right Radar Minimap
// ============================================================================
class AETHERIACHRONICLES_API SChroniclesMinimapWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SChroniclesMinimapWidget)
		: _OwnerPC(nullptr)
		, _MinimapSize(172.f)
	{}
		SLATE_ARGUMENT(TWeakObjectPtr<AChroniclesPlayerController>, OwnerPC)
		SLATE_ARGUMENT(float, MinimapSize)
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
	TWeakObjectPtr<AChroniclesPlayerController> OwnerPC;
	float DesiredSizePx = 172.f;
};

// ============================================================================
// 4C. SChroniclesWorldSpaceOverlay — Overhead NPC Names, Enemy HP Bars,
//     Floating Golden Objective Diamond & Minimalist Center Dot Reticle
// ============================================================================
class AETHERIACHRONICLES_API SChroniclesWorldSpaceOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SChroniclesWorldSpaceOverlay)
		: _OwnerPC(nullptr)
	{}
		SLATE_ARGUMENT(TWeakObjectPtr<AChroniclesPlayerController>, OwnerPC)
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
	TWeakObjectPtr<AChroniclesPlayerController> OwnerPC;
};

// ============================================================================
// 4D. SAetheriaChroniclesHUD — Production Slate HUD & Interactive Overlays
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

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	/** Rebuilds dynamic overlay lists (Skill Tree columns, Quest Chronicle, Grimoire, Landmarks). */
	void RefreshDynamicPanels();
	void SyncDeveloperConsoleVisibilityAndFocus(bool bOpen, bool bForceFocus = false);
	void RebuildConsoleLogBox();

private:
	// --- Persistent HUD Regions ---
	TSharedRef<SWidget> BuildTopCenterCompassAndBossBar();
	TSharedRef<SWidget> BuildTopRightStatusAndHotkeys();
	TSharedRef<SWidget> BuildBottomLeftVitalsAndInvokerPanel();
	TSharedRef<SWidget> BuildBottomRightMinimapAndGold();
	TSharedRef<SWidget> BuildBottomCenterPromptsAndToast();
	TSharedRef<SWidget> BuildCutsceneLetterboxOverlay();
	TSharedRef<SWidget> BuildDeveloperConsoleOverlay();

	// --- Interactive Modal Overlays ---
	TSharedRef<SWidget> BuildMainMenuOverlay();
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
	TSharedPtr<SWidget> ConsoleOverlayContainer;
	TSharedPtr<SEditableTextBox> ConsoleInputBox;
	TSharedPtr<SScrollBox> ConsoleScrollBox;
	TSharedPtr<SVerticalBox> ConsoleLogListBox;
	int32 ConsoleHistoryBrowseIndex = -1;
	int32 CreatorActiveTab = 0;
};
