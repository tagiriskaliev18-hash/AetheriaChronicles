// ============================================================================
// ChroniclesGameFramework.cpp — Complete GameState, GameMode, PlayerController
// (Input Hygiene, Co-op Commands, Automated -wizardsmoke Harness), 64x64 Slate
// Fog-of-War Map Widget & Production Slate HUD / Interactive Overlays
// ============================================================================
#include "ChroniclesGameFramework.h"

#include "AudioAndStyleSubsystem.h"
#include "DestructionAndWorld.h"
#include "GraphicsRTXSubsystem.h"
#include "HeroCustomizationComponent.h"
#include "HeroDragonBoss.h"
#include "QuestAndMapSubsystem.h"
#include "SkillTreeComponent.h"
#include "SpellSystemComponent.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AetheriaChroniclesHUD"

namespace ChroniclesHUDPalette
{
	static const FLinearColor PanelBg(0.032f, 0.036f, 0.056f, 0.88f);
	static const FLinearColor PanelBgDeep(0.020f, 0.022f, 0.036f, 0.94f);
	static const FLinearColor ModalScrim(0.015f, 0.016f, 0.028f, 0.72f);
	static const FLinearColor CardBg(0.065f, 0.072f, 0.105f, 0.90f);
	static const FLinearColor CardBgLocked(0.042f, 0.044f, 0.060f, 0.72f);
	static const FLinearColor HairlineGold(0.72f, 0.58f, 0.34f, 0.52f);
	static const FLinearColor HairlineCyan(0.08f, 0.85f, 0.98f, 0.55f);
	static const FLinearColor HairlineCrimson(0.92f, 0.16f, 0.14f, 0.78f);

	static const FLinearColor TextPrimary(0.95f, 0.94f, 0.90f, 1.0f);
	static const FLinearColor TextSecondary(0.70f, 0.72f, 0.78f, 1.0f);
	static const FLinearColor TextGold(0.98f, 0.82f, 0.42f, 1.0f);
	static const FLinearColor TextCyan(0.18f, 0.92f, 1.00f, 1.0f);
	static const FLinearColor TextCrimson(1.00f, 0.28f, 0.24f, 1.0f);
	static const FLinearColor TextEmerald(0.28f, 0.92f, 0.54f, 1.0f);

	static const FLinearColor BarHP(0.84f, 0.16f, 0.18f, 1.0f);
	static const FLinearColor BarMana(0.16f, 0.52f, 0.98f, 1.0f);
	static const FLinearColor BarStamina(0.22f, 0.78f, 0.44f, 1.0f);
	static const FLinearColor BarXP(0.88f, 0.70f, 0.24f, 1.0f);
	static const FLinearColor BarDragonFire(1.00f, 0.42f, 0.08f, 1.0f);
	static const FLinearColor BarStagger(0.98f, 0.76f, 0.18f, 1.0f);

	static const FLinearColor OrbExort(1.00f, 0.40f, 0.10f, 1.0f);
	static const FLinearColor OrbWex(0.62f, 0.32f, 1.00f, 1.0f);
	static const FLinearColor OrbQuas(0.14f, 0.84f, 1.00f, 1.0f);
	static const FLinearColor OrbLocked(0.26f, 0.28f, 0.34f, 0.80f);

	static const FSlateBrush* WhiteBrush()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}

	static FSlateFontInfo FontBold(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	static FSlateFontInfo FontRegular(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", Size);
	}

	static FString CardinalFromYaw(float YawDegrees)
	{
		// In Aetheria world coordinates: -Y (Yaw -90) = North (Highkeep), +X (Yaw 0) = East, +Y (Yaw +90) = South (Oakhaven), -X (Yaw +-180) = West
		float Bearing = FRotator::ClampAxis(YawDegrees + 90.f);
		static const TCHAR* Directions[8] = {
			TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"),
			TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW")
		};
		const int32 Idx = FMath::FloorToInt((Bearing + 22.5f) / 45.f) % 8;
		return FString::Printf(TEXT("%s (%03d deg)"), Directions[Idx], FMath::RoundToInt(Bearing) % 360);
	}

	static FString RomanPhase(int32 Phase)
	{
		switch (Phase)
		{
		case 1:  return TEXT("PHASE I");
		case 2:  return TEXT("PHASE II");
		case 3:  return TEXT("PHASE III");
		default: return FString::Printf(TEXT("PHASE %d"), Phase);
		}
	}
}

// ============================================================================
// 1. AChroniclesGameState
// ============================================================================

AChroniclesGameState::AChroniclesGameState()
{
	bReplicates = true;
	ReplicatedTimeOfDay = 0.72f;
	ConnectedPlayers = 1;
	ActiveWorldBanner = TEXT("Aetheria Chronicles — Golden Oakhaven Valley");
}

void AChroniclesGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChroniclesGameState, ReplicatedTimeOfDay);
	DOREPLIFETIME(AChroniclesGameState, ConnectedPlayers);
	DOREPLIFETIME(AChroniclesGameState, ActiveWorldBanner);
}

// ============================================================================
// 2. AChroniclesGameMode
// ============================================================================

AChroniclesGameMode::AChroniclesGameMode()
{
	DefaultPawnClass = AAetheriaHeroCharacter::StaticClass();
	PlayerControllerClass = AChroniclesPlayerController::StaticClass();
	GameStateClass = AChroniclesGameState::StaticClass();
}

void AChroniclesGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnOpenWorldActors();
		UpdateConnectedPlayerCount();
	}
}

void AChroniclesGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UpdateConnectedPlayerCount();
}

void AChroniclesGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UpdateConnectedPlayerCount();
}

void AChroniclesGameMode::UpdateConnectedPlayerCount()
{
	if (AChroniclesGameState* GS = GetGameState<AChroniclesGameState>())
	{
		GS->ConnectedPlayers = FMath::Max(1, GetNumPlayers());
	}
}

float AChroniclesGameMode::SampleGroundZSafe(float X, float Y, float FallbackZ) const
{
	if (WorldBuilder)
	{
		return WorldBuilder->SampleGroundZ(X, Y, FallbackZ);
	}

	if (UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params(TEXT("ChroniclesGameModeGroundTrace"), false);
		if (World->LineTraceSingleByChannel(
				Hit,
				FVector(X, Y, 20000.f),
				FVector(X, Y, -10000.f),
				ECC_WorldStatic,
				Params))
		{
			return Hit.ImpactPoint.Z;
		}
	}

	return FallbackZ;
}

AActor* AChroniclesGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (UWorld* World = GetWorld())
	{
		const float GroundZ = SampleGroundZSafe(0.f, 4800.f, 0.f);
		const FVector SpawnLoc(0.f, 4800.f, GroundZ + 120.f);
		const FRotator SpawnRot(0.f, -90.f, 0.f);

		if (!IsValid(OakhavenPlayerStart))
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			OakhavenPlayerStart = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnLoc, SpawnRot, SpawnParams);
		}
		else
		{
			OakhavenPlayerStart->SetActorLocationAndRotation(SpawnLoc, SpawnRot);
		}

		if (OakhavenPlayerStart)
		{
			return OakhavenPlayerStart;
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AChroniclesGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	// Stagger co-op spawn offsets cleanly around Oakhaven Sanctuary (0, 4800)
	static const float CoopOffsetsX[6] = { 0.f, 180.f, -180.f, 340.f, -340.f, 500.f };
	const float OffsetX = CoopOffsetsX[SpawnedHeroCounter % 6];
	++SpawnedHeroCounter;

	const float GroundZ = SampleGroundZSafe(OffsetX, 4800.f, 0.f);
	const FVector SpawnLoc(OffsetX, 4800.f, GroundZ + 120.f);
	const FRotator SpawnRot(0.f, -90.f, 0.f);
	const FTransform SpawnTransform(SpawnRot, SpawnLoc);

	RestartPlayerAtTransform(NewPlayer, SpawnTransform);

	if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(NewPlayer->GetPawn()))
	{
		Hero->OakhavenSpawnLocation = SpawnLoc;
		NewPlayer->SetControlRotation(SpawnRot);

		UE_LOG(LogChronicles, Log, TEXT("AChroniclesGameMode spawned Hero cleanly at Oakhaven Sanctuary (%.1f, %.1f, %.1f)."),
			SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z);
	}
}

void AChroniclesGameMode::SpawnOpenWorldActors()
{
	if (bWorldActorsSpawned)
	{
		return;
	}
	bWorldActorsSpawned = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters AlwaysSpawn;
	AlwaysSpawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 1. Spawn or adopt AAetheriaWorldBuilder
	for (TActorIterator<AAetheriaWorldBuilder> It(World); It; ++It)
	{
		WorldBuilder = *It;
		break;
	}
	if (!WorldBuilder)
	{
		WorldBuilder = World->SpawnActor<AAetheriaWorldBuilder>(
			AAetheriaWorldBuilder::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			AlwaysSpawn);
	}

	if (AChroniclesGameState* GS = GetGameState<AChroniclesGameState>())
	{
		GS->ReplicatedTimeOfDay = WorldBuilder ? WorldBuilder->TimeOfDay : 0.72f;
		GS->ActiveWorldBanner = WorldBuilder
			? FString::Printf(TEXT("Aetheria Chronicles · %s"), *WorldBuilder->GetTimeOfDayLabel())
			: TEXT("Aetheria Chronicles · Golden Sunset");
	}

	// 2. Spawn Rideable Red Dragon (Ignis) at Dragon Aerie (-3400, 2200, GroundZ + 120)
	{
		const float DragonGroundZ = SampleGroundZSafe(-3400.f, 2200.f, 400.f);
		const FVector DragonLoc(-3400.f, 2200.f, DragonGroundZ + 120.f);
		const FRotator DragonRot(0.f, -35.f, 0.f);
		WorldDragon = World->SpawnActor<AAetheriaDragonActor>(
			AAetheriaDragonActor::StaticClass(),
			DragonLoc,
			DragonRot,
			AlwaysSpawn);

		UE_LOG(LogChronicles, Log, TEXT("Spawned Crimson Dragon Ignis at Dragon Aerie (%.0f, %.0f, %.0f)."),
			DragonLoc.X, DragonLoc.Y, DragonLoc.Z);
	}

	// 3. Spawn all 6 Named Quest NPCs
	SpawnNamedQuestNPCs();

	// 4. Spawn Roaming Biome Enemies
	SpawnRoamingBiomeEnemies();

	// 5. Spawn Multi-Phase Boss Malakor inside Highkeep Citadel (0, -10400, GroundZ + 120)
	{
		const float BossGroundZ = SampleGroundZSafe(0.f, -10400.f, 800.f);
		const FVector BossLoc(0.f, -10400.f, BossGroundZ + 120.f);
		const FRotator BossRot(0.f, 90.f, 0.f);
		WorldBoss = World->SpawnActor<AAetheriaBossCharacter>(
			AAetheriaBossCharacter::StaticClass(),
			BossLoc,
			BossRot,
			AlwaysSpawn);

		if (WorldBoss)
		{
			WorldBoss->QuestIdOnDeath = FName(TEXT("Q_FallOfSovereign"));
			UE_LOG(LogChronicles, Log, TEXT("Spawned Sovereign Boss Malakor inside Highkeep Citadel (%.0f, %.0f, %.0f)."),
				BossLoc.X, BossLoc.Y, BossLoc.Z);
		}
	}
}

void AChroniclesGameMode::SpawnNamedQuestNPCs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	struct FNpcSpawnSpec
	{
		const TCHAR* NpcId;
		const TCHAR* DisplayName;
		const TCHAR* RoleTitle;
		const TCHAR* QuestId;
		float X;
		float Y;
		float Yaw;
		int32 Variant;
	};

	static const FNpcSpawnSpec NpcSpecs[6] = {
		{ TEXT("ElderAldric"),     TEXT("Elder Aldric"),     TEXT("Oakhaven Village Elder"),           TEXT("Q_AwakenExort"),     350.f,   4400.f, -120.f, 0 },
		{ TEXT("ScholarLyra"),     TEXT("Scholar Lyra"),     TEXT("Arcane Scholar of Mirror Lake"),    TEXT("Q_TameDragon"),     3400.f,   1200.f, -150.f, 1 },
		{ TEXT("BlacksmithTorin"), TEXT("Blacksmith Torin"), TEXT("Master Runesmith of Oakhaven"),     TEXT("Q_StormSpires"),   -1400.f,   4800.f,  -45.f, 2 },
		{ TEXT("WardenKael"),      TEXT("Warden Kael"),      TEXT("Ash Caldera Crypt Sentinel"),       TEXT("Q_EmberCrypt"),     8900.f,  -2400.f,  180.f, 3 },
		{ TEXT("SeerVaelis"),      TEXT("Seer Vaelis"),      TEXT("Oracle of the Crystal Glacier"),    TEXT("Q_GlacierSilence"),-6800.f,  -5600.f,   45.f, 4 },
		{ TEXT("CaptainRowena"),   TEXT("Captain Rowena"),   TEXT("Highkeep Vanguard Commander"),      TEXT("Q_FallOfSovereign"), 450.f,  -7600.f,  105.f, 5 }
	};

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedNPCs.Reset(6);
	for (const FNpcSpawnSpec& Spec : NpcSpecs)
	{
		// AAetheriaNPCActor capsule half-height is 92 cm -> GroundZ + 92 places feet flush on terrain
		const float GroundZ = SampleGroundZSafe(Spec.X, Spec.Y, 0.f);
		const FVector Loc(Spec.X, Spec.Y, GroundZ + 92.f);
		const FRotator Rot(0.f, Spec.Yaw, 0.f);

		if (AAetheriaNPCActor* NPC = World->SpawnActor<AAetheriaNPCActor>(AAetheriaNPCActor::StaticClass(), Loc, Rot, SpawnParams))
		{
			NPC->ConfigureNPC(
				FName(Spec.NpcId),
				FText::FromString(Spec.DisplayName),
				FText::FromString(Spec.RoleTitle),
				FName(Spec.QuestId),
				Spec.Variant);
			SpawnedNPCs.Add(NPC);
		}
	}

	UE_LOG(LogChronicles, Log, TEXT("Spawned %d Named Story Quest NPCs across Aetheria."), SpawnedNPCs.Num());
}

void AChroniclesGameMode::SpawnRoamingBiomeEnemies()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	struct FEnemySpawnSpec
	{
		EChronicleEnemyArchetype Archetype;
		float X;
		float Y;
		const TCHAR* QuestIdOnDeath;
	};

	static const FEnemySpawnSpec EnemySpecs[] = {
		// Skeletons patrolling Stone Bridge & Ember Crypt
		{ EChronicleEnemyArchetype::Skeleton,        680.f,   1650.f, nullptr },
		{ EChronicleEnemyArchetype::Skeleton,       -520.f,   1350.f, nullptr },
		{ EChronicleEnemyArchetype::Skeleton,       9150.f,  -2050.f, nullptr },
		{ EChronicleEnemyArchetype::Skeleton,       9650.f,  -2750.f, nullptr },
		// Forest Trolls roaming Sapphire Whispering Woods (Blue-Leaf Woods)
		{ EChronicleEnemyArchetype::ForestTroll,    5150.f,   2550.f, nullptr },
		{ EChronicleEnemyArchetype::ForestTroll,    6950.f,   3850.f, nullptr },
		// Tempest Stalker guarding the Storm Spires monoliths
		{ EChronicleEnemyArchetype::TempestStalker, 6400.f,   3200.f, TEXT("Q_StormSpires") },
		// Frost Warden commanding the Crystal Glacier Peaks (Quest 5 -> unlocks Quas!)
		{ EChronicleEnemyArchetype::FrostWarden,   -8200.f,  -6800.f, TEXT("Q_GlacierSilence") }
	};

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedEnemies.Reset();
	for (const FEnemySpawnSpec& Spec : EnemySpecs)
	{
		const float GroundZ = SampleGroundZSafe(Spec.X, Spec.Y, 0.f);
		const FVector Loc(Spec.X, Spec.Y, GroundZ + 115.f);
		const FRotator Rot(0.f, FMath::FRandRange(-180.f, 180.f), 0.f);

		if (AAetheriaEnemyCharacter* Enemy = World->SpawnActor<AAetheriaEnemyCharacter>(AAetheriaEnemyCharacter::StaticClass(), Loc, Rot, SpawnParams))
		{
			const FName QuestName = (Spec.QuestIdOnDeath != nullptr) ? FName(Spec.QuestIdOnDeath) : NAME_None;
			Enemy->ConfigureArchetype(Spec.Archetype, QuestName);
			SpawnedEnemies.Add(Enemy);
		}
	}

	UE_LOG(LogChronicles, Log, TEXT("Spawned %d roaming biome enemies across 5 biomes."), SpawnedEnemies.Num());
}

// ============================================================================
// 3. AChroniclesPlayerController
// ============================================================================

AChroniclesPlayerController::AChroniclesPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AChroniclesPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		// Bind Alt-Tab / window activation state safely with TWeakObjectPtr (zero phantom clicks on refocus!)
		if (FSlateApplication::IsInitialized())
		{
			TWeakObjectPtr<AChroniclesPlayerController> WeakPC(this);
			AppActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddLambda(
				[WeakPC](bool bIsActive)
				{
					if (AChroniclesPlayerController* PC = WeakPC.Get())
					{
						PC->HandleApplicationActivationChanged(bIsActive);
					}
				});
		}

		MountChroniclesHUD();
		SetUIMode(false);
		StartSmokeTestIfRequested();
		if (!bSmokeTestActive)
		{
			StartCaptureFramesIfRequested();
		}

		if (!bSmokeTestActive && !bCaptureFramesActive)
		{
			PushToastMessage(TEXT("Welcome to Aetheria Chronicles — Speak to Elder Aldric [E] or open Map [M]"), 5.5f);
		}
	}
}

void AChroniclesPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AppActivationHandle.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(AppActivationHandle);
		AppActivationHandle.Reset();
	}

	UnmountChroniclesHUD();
	Super::EndPlay(EndPlayReason);
}

void AChroniclesPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AChroniclesPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalController() && HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}
}

void AChroniclesPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (IsLocalController() && HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}
}

void AChroniclesPlayerController::MountChroniclesHUD()
{
	if (!IsLocalController() || HUDWidget.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}

	HUDWidget = SNew(SAetheriaChroniclesHUD)
		.OwnerPC(TWeakObjectPtr<AChroniclesPlayerController>(this));

	HUDViewportContainer = SNew(SWeakWidget)
		.PossiblyNullContent(HUDWidget.ToSharedRef());

	Viewport->AddViewportWidgetContent(HUDViewportContainer.ToSharedRef(), 12);
	HUDWidget->RefreshDynamicPanels();
}

void AChroniclesPlayerController::UnmountChroniclesHUD()
{
	if (HUDViewportContainer.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* Viewport = World->GetGameViewport())
			{
				Viewport->RemoveViewportWidgetContent(HUDViewportContainer.ToSharedRef());
			}
		}
		HUDViewportContainer.Reset();
	}
	HUDWidget.Reset();
}

void AChroniclesPlayerController::HandleApplicationActivationChanged(bool bIsActive)
{
	if (!bIsActive)
	{
		// Clear capture immediately on Alt-Tab focus loss so returning never fires a phantom strike!
		bMouseCaptured = false;
		bShowMouseCursor = true;
		bRefocusClickSwallowed = false;

		if (AAetheriaHeroCharacter* Hero = GetActiveHero())
		{
			Hero->SetBlocking(false);
		}
		bLastBlockKeyDown = false;
	}
	else if (!bSmokeTestActive)
	{
		// Remain uncaptured until user clicks back into the viewport or presses Escape
		bMouseCaptured = false;
		bShowMouseCursor = true;
		bRefocusClickSwallowed = false;
	}
}

void AChroniclesPlayerController::ToggleMouseCapture()
{
	SetUIMode(bMouseCaptured);
}

void AChroniclesPlayerController::SetUIMode(bool bOpenUI)
{
	if (bOpenUI)
	{
		bMouseCaptured = false;
		bShowMouseCursor = true;

		if (AAetheriaHeroCharacter* Hero = GetActiveHero())
		{
			Hero->SetBlocking(false);
		}
		bLastBlockKeyDown = false;

		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
	}
	else
	{
		bMouseCaptured = true;
		bShowMouseCursor = false;

		FInputModeGameOnly Mode;
		SetInputMode(Mode);

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

bool AChroniclesPlayerController::IsAnyModalOpen() const
{
	return (ActiveOverlay != EChroniclesOverlayMode::None) || IsHeroDead();
}

void AChroniclesPlayerController::ToggleOverlay(EChroniclesOverlayMode TargetOverlay)
{
	if (TargetOverlay == EChroniclesOverlayMode::None || ActiveOverlay == TargetOverlay)
	{
		CloseAllOverlays();
		return;
	}

	const bool bWasCreator = (ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer);
	ActiveOverlay = TargetOverlay;
	if (TargetOverlay != EChroniclesOverlayMode::Dialogue)
	{
		ActiveDialogueNPC.Reset();
	}

	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		if (TargetOverlay == EChroniclesOverlayMode::HeroCustomizer)
		{
			Hero->SetCreatorStudioCameraMode(true, CreatorOrbitYaw, CreatorZoomMode);
		}
		else if (bWasCreator)
		{
			Hero->SetCreatorStudioCameraMode(false);
		}
	}

	if (HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}

	SetUIMode(true);
}

void AChroniclesPlayerController::CloseAllOverlays()
{
	ActiveOverlay = EChroniclesOverlayMode::None;
	ActiveDialogueNPC.Reset();

	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->SetCreatorStudioCameraMode(false);
	}

	if (!IsHeroDead())
	{
		SetUIMode(false);
	}
}

void AChroniclesPlayerController::AdjustCreatorCameraOrbit(float DeltaYawDeg)
{
	CreatorOrbitYaw = FRotator::ClampAxis(CreatorOrbitYaw + DeltaYawDeg);
	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->SetCreatorStudioCameraMode(true, CreatorOrbitYaw, CreatorZoomMode);
	}
}

void AChroniclesPlayerController::SetCreatorCameraZoom(int32 NewZoomMode)
{
	CreatorZoomMode = FMath::Clamp(NewZoomMode, 0, 2);
	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->SetCreatorStudioCameraMode(true, CreatorOrbitYaw, CreatorZoomMode);
	}
}

AAetheriaHeroCharacter* AChroniclesPlayerController::GetActiveHero() const
{
	if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(GetPawn()))
	{
		return Hero;
	}
	if (AAetheriaDragonActor* Dragon = Cast<AAetheriaDragonActor>(GetPawn()))
	{
		return Dragon->GetRiderHero();
	}
	return nullptr;
}

AAetheriaDragonActor* AChroniclesPlayerController::GetControlledDragon() const
{
	return Cast<AAetheriaDragonActor>(GetPawn());
}

bool AChroniclesPlayerController::IsRidingDragon() const
{
	return GetControlledDragon() != nullptr;
}

bool AChroniclesPlayerController::IsHeroDead() const
{
	const AAetheriaHeroCharacter* Hero = GetActiveHero();
	return Hero && Hero->bDead;
}

AAetheriaNPCActor* AChroniclesPlayerController::FindNearestInteractableNPC(float MaxDistance) const
{
	const AAetheriaHeroCharacter* Hero = GetActiveHero();
	UWorld* World = GetWorld();
	if (!Hero || !World)
	{
		return nullptr;
	}

	const FVector HeroLoc = Hero->GetActorLocation();
	float BestDistSq = FMath::Square(MaxDistance);
	AAetheriaNPCActor* BestNPC = nullptr;

	for (TActorIterator<AAetheriaNPCActor> It(World); It; ++It)
	{
		AAetheriaNPCActor* NPC = *It;
		if (!IsValid(NPC))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(HeroLoc, NPC->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestNPC = NPC;
		}
	}

	return BestNPC;
}

AAetheriaDragonActor* AChroniclesPlayerController::FindNearestDragon(float MaxDistance) const
{
	const APawn* CurrentPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!CurrentPawn || !World)
	{
		return nullptr;
	}

	const FVector RefLoc = CurrentPawn->GetActorLocation();
	float BestDistSq = FMath::Square(MaxDistance);
	AAetheriaDragonActor* BestDragon = nullptr;

	for (TActorIterator<AAetheriaDragonActor> It(World); It; ++It)
	{
		AAetheriaDragonActor* Dragon = *It;
		if (!IsValid(Dragon))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(RefLoc, Dragon->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestDragon = Dragon;
		}
	}

	return BestDragon;
}

AAetheriaBossCharacter* AChroniclesPlayerController::FindWorldBoss() const
{
	if (AAetheriaBossCharacter* Cached = CachedBossRef.Get())
	{
		return Cached;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AAetheriaBossCharacter> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				CachedBossRef = *It;
				return *It;
			}
		}
	}

	return nullptr;
}

AAetheriaWorldBuilder* AChroniclesPlayerController::FindWorldBuilder() const
{
	if (AAetheriaWorldBuilder* Cached = CachedWorldBuilderRef.Get())
	{
		return Cached;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AAetheriaWorldBuilder> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				CachedWorldBuilderRef = *It;
				return *It;
			}
		}
	}

	return nullptr;
}

void AChroniclesPlayerController::PushToastMessage(const FString& Message, float DurationSeconds)
{
	if (Message.IsEmpty())
	{
		return;
	}

	ActiveToastText = Message;
	ActiveToastDuration = FMath::Max(0.5f, DurationSeconds);
	ActiveToastRemaining = ActiveToastDuration;
}

float AChroniclesPlayerController::GetActiveToastAlpha() const
{
	if (ActiveToastRemaining <= 0.f || ActiveToastText.IsEmpty())
	{
		return 0.f;
	}

	if (ActiveToastRemaining < 0.65f)
	{
		return FMath::Clamp(ActiveToastRemaining / 0.65f, 0.f, 1.f);
	}
	return 1.f;
}

void AChroniclesPlayerController::UpdateToastNotifications(float DeltaTime)
{
	if (ActiveToastRemaining > 0.f)
	{
		ActiveToastRemaining = FMath::Max(0.f, ActiveToastRemaining - DeltaTime);
		if (ActiveToastRemaining <= 0.f)
		{
			ActiveToastText.Empty();
		}
	}

	if (ActiveToastRemaining <= 0.f)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQuestSubsystem* QuestSub = GI->GetSubsystem<UQuestSubsystem>())
			{
				if (QuestSub->HasPendingNotifications())
				{
					PushToastMessage(QuestSub->PopLatestNotification(), 4.5f);
				}
			}
		}
	}
}

void AChroniclesPlayerController::UpdateDeathOverlayState()
{
	const bool bNowDead = IsHeroDead();
	if (bNowDead != bLastKnownHeroDead)
	{
		bLastKnownHeroDead = bNowDead;
		if (bNowDead)
		{
			if (ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer)
			{
				if (AAetheriaHeroCharacter* Hero = GetActiveHero())
				{
					Hero->SetCreatorStudioCameraMode(false);
				}
			}
			ActiveOverlay = EChroniclesOverlayMode::None;
			ActiveDialogueNPC.Reset();
			SetUIMode(true);
		}
		else
		{
			SetUIMode(false);
		}
	}
}

void AChroniclesPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!IsLocalController())
	{
		return;
	}

	if (!HUDWidget.IsValid())
	{
		MountChroniclesHUD();
	}

	UpdateToastNotifications(DeltaTime);
	UpdateDeathOverlayState();

	MusicZoneTimer -= DeltaTime;
	if (MusicZoneTimer <= 0.f)
	{
		MusicZoneTimer = 0.65f;
		if (UAetheriaMusicSubsystem* MusicSub = UAetheriaMusicSubsystem::Get(this))
		{
			if (const APawn* ActivePawn = GetPawn())
			{
				const bool bInVillage = FVector::Dist2D(ActivePawn->GetActorLocation(), FVector(0.f, 4800.f, 0.f)) < 2900.f;
				MusicSub->SetExplorationZone(bInVillage);
			}
		}
	}

	if (bSmokeTestActive)
	{
		RunSmokeTestTick(DeltaTime);
		return;
	}

	if (bCaptureFramesActive)
	{
		RunCaptureFramesTick(DeltaTime);
		return;
	}

	HandleGlobalAndOverlayKeys();

	// If Hero died, allow [R] respawn hotkey while cursor remains unlocked for the UI button
	if (IsHeroDead())
	{
		if (WasInputKeyJustPressed(EKeys::R))
		{
			RequestRespawnHero();
		}
		return;
	}

	// Refocus click recapture: swallow the click that recaptures the cursor so zero phantom strikes occur!
	if (!bMouseCaptured && !IsAnyModalOpen())
	{
		if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			bRefocusClickSwallowed = true;
			SetUIMode(false);
		}
		return;
	}

	if (bRefocusClickSwallowed)
	{
		if (!IsInputKeyDown(EKeys::LeftMouseButton))
		{
			bRefocusClickSwallowed = false;
		}
		return;
	}

	// Strictly gate ALL combat & movement input on bMouseCaptured && !IsAnyModalOpen()
	if (!bMouseCaptured || IsAnyModalOpen())
	{
		return;
	}

	if (AAetheriaDragonActor* Dragon = GetControlledDragon())
	{
		HandleDragonGameplayInput(Dragon, DeltaTime);
	}
	else if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		HandleHeroGameplayInput(Hero, DeltaTime);
	}
}

void AChroniclesPlayerController::HandleGlobalAndOverlayKeys()
{
	if (WasInputKeyJustPressed(EKeys::Escape))
	{
		if (ActiveOverlay != EChroniclesOverlayMode::None)
		{
			CloseAllOverlays();
		}
		else if (!IsHeroDead())
		{
			ToggleMouseCapture();
		}
		return;
	}

	if (IsHeroDead())
	{
		return;
	}

	if (WasInputKeyJustPressed(EKeys::M))
	{
		ToggleOverlay(EChroniclesOverlayMode::WorldMap);
	}
	else if (WasInputKeyJustPressed(EKeys::K))
	{
		ToggleOverlay(EChroniclesOverlayMode::SkillTree);
	}
	else if (WasInputKeyJustPressed(EKeys::C))
	{
		ToggleOverlay(EChroniclesOverlayMode::HeroCustomizer);
	}
	else if (WasInputKeyJustPressed(EKeys::J))
	{
		ToggleOverlay(EChroniclesOverlayMode::QuestLog);
	}
	else if (WasInputKeyJustPressed(EKeys::Tab))
	{
		ToggleOverlay(EChroniclesOverlayMode::Grimoire);
	}
	else if (WasInputKeyJustPressed(EKeys::F10))
	{
		ToggleOverlay(EChroniclesOverlayMode::GraphicsRTX);
	}
	else if (WasInputKeyJustPressed(EKeys::T))
	{
		RequestCycleTimeOfDay();
	}
}

void AChroniclesPlayerController::HandleHeroGameplayInput(AAetheriaHeroCharacter* Hero, float DeltaTime)
{
	if (!Hero || Hero->bDead)
	{
		return;
	}

	// 1. Third-person Camera Look (clamped safely against gimbal flips)
	float MouseDX = 0.f;
	float MouseDY = 0.f;
	GetInputMouseDelta(MouseDX, MouseDY);

	if (!FMath::IsNearlyZero(MouseDX) || !FMath::IsNearlyZero(MouseDY))
	{
		FRotator CtrlRot = GetControlRotation();
		CtrlRot.Yaw += MouseDX * MouseLookSensitivity;
		const float UnwrappedPitch = FRotator::NormalizeAxis(CtrlRot.Pitch);
		CtrlRot.Pitch = FMath::Clamp(UnwrappedPitch + MouseDY * MouseLookSensitivity, -62.f, 52.f);
		CtrlRot.Roll = 0.f;
		SetControlRotation(CtrlRot);
	}

	// 2. WASD Third-Person Movement relative to Camera Yaw
	const float ForwardAxis = (IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
	const float RightAxis = (IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) ? 1.f : 0.f);

	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	if (!FMath::IsNearlyZero(ForwardAxis))
	{
		Hero->AddMovementInput(ForwardDir, ForwardAxis);
	}
	if (!FMath::IsNearlyZero(RightAxis))
	{
		Hero->AddMovementInput(RightDir, RightAxis);
	}

	// 3. Jump
	if (WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		Hero->Jump();
	}
	else if (WasInputKeyJustReleased(EKeys::SpaceBar))
	{
		Hero->StopJumping();
	}

	// 4. Sword 3-Hit Combo (LMB) & Shield Block / Timed Parry (RMB)
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		Hero->PerformMeleeAttack();
	}

	const bool bBlockDown = IsInputKeyDown(EKeys::RightMouseButton);
	if (bBlockDown != bLastBlockKeyDown)
	{
		bLastBlockKeyDown = bBlockDown;
		Hero->SetBlocking(bBlockDown);
	}

	// 5. Dodge Roll (Q, LeftControl, or V — zero conflict with WASD or Spell Slots!)
	if (WasInputKeyJustPressed(EKeys::Q) || WasInputKeyJustPressed(EKeys::LeftControl) || WasInputKeyJustPressed(EKeys::V))
	{
		FVector RollDir = (ForwardDir * ForwardAxis + RightDir * RightAxis).GetSafeNormal2D();
		if (RollDir.IsNearlyZero())
		{
			RollDir = Hero->GetActorForwardVector().GetSafeNormal2D();
		}
		Hero->PerformDodgeRoll(RollDir);
	}

	// 6. Smart Interact / Exort [E], Dedicated Primal Orbs [1/2/3], Invoke [R], Cast Slots [4/X] & [5/Z]
	USpellSystemComponent* Spells = Hero->SpellSystem;

	if (WasInputKeyJustPressed(EKeys::E))
	{
		HandleInteractOrExort();
	}

	if (Spells)
	{
		static const TCHAR* SmallStatureToast = TEXT("Stature < 1.45m: Too small to channel Aether — Spells locked! Open [C] Hero Creator to adjust height.");

		if (WasInputKeyJustPressed(EKeys::One))
		{
			if (!Hero->CanCastSpellsByStature())
			{
				PushToastMessage(SmallStatureToast);
			}
			else if (!Spells->PushOrb(EChronicleOrb::Exort))
			{
				PushToastMessage(TEXT("Exort Locked — Speak to Elder Aldric [E] in Oakhaven to awaken Solar Flame!"));
			}
		}
		if (WasInputKeyJustPressed(EKeys::Two))
		{
			if (!Hero->CanCastSpellsByStature())
			{
				PushToastMessage(SmallStatureToast);
			}
			else if (!Spells->PushOrb(EChronicleOrb::Wex))
			{
				PushToastMessage(TEXT("Wex Locked — Complete 'Spires of the Tempest' (Blacksmith Torin) to unlock Storm!"));
			}
		}
		if (WasInputKeyJustPressed(EKeys::Three))
		{
			if (!Hero->CanCastSpellsByStature())
			{
				PushToastMessage(SmallStatureToast);
			}
			else if (!Spells->PushOrb(EChronicleOrb::Quas))
			{
				PushToastMessage(TEXT("Quas Locked — Complete 'Silence of the Crystal Glacier' (Seer Vaelis) to unlock Chrono-Frost!"));
			}
		}

		if (WasInputKeyJustPressed(EKeys::R))
		{
			if (!Hero->CanCastSpellsByStature())
			{
				PushToastMessage(SmallStatureToast);
			}
			else if (Spells->InvokeCurrentSpell())
			{
				const FChronicleSpellDef Def = USpellSystemComponent::GetSpellDef(Spells->SlotD);
				PushToastMessage(FString::Printf(TEXT("Invoked [%s] %s -> Slot D [4/X]"),
					*Def.ComboKey, *Def.DisplayName.ToString()), 2.6f);
			}
			else if (Spells->GetUnlockedOrbCount() == 0)
			{
				PushToastMessage(TEXT("No Primal Spheres awakened yet — complete 'Ashes of the Hearth' in Oakhaven!"));
			}
		}

		const bool bCastSlotD = WasInputKeyJustPressed(EKeys::Four) || WasInputKeyJustPressed(EKeys::X);
		const bool bCastSlotF = WasInputKeyJustPressed(EKeys::Five) || WasInputKeyJustPressed(EKeys::Z);

		if (bCastSlotD || bCastSlotF)
		{
			if (!Hero->CanCastSpellsByStature())
			{
				PushToastMessage(SmallStatureToast);
			}
			else
			{
				FVector AimOrigin = FVector::ZeroVector;
				FVector AimDir = FVector::ForwardVector;
				FVector TargetPoint = FVector::ZeroVector;
				ComputeSpellAim(Hero, AimOrigin, AimDir, TargetPoint);
				Spells->CastSpellSlot(bCastSlotF, AimOrigin, AimDir, TargetPoint, 0.35f);
			}
		}
	}

	// 7. Mount Red Dragon Ignis [F]
	if (WasInputKeyJustPressed(EKeys::F))
	{
		TryToggleDragonMount();
	}
}

void AChroniclesPlayerController::HandleDragonGameplayInput(AAetheriaDragonActor* Dragon, float DeltaTime)
{
	if (!Dragon)
	{
		return;
	}

	if (WasInputKeyJustPressed(EKeys::F))
	{
		TryToggleDragonMount();
		return;
	}

	const float ForwardAxis = (IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
	const float RightAxis = (IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
	const float UpAxis = (IsInputKeyDown(EKeys::SpaceBar) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::LeftShift) ? 1.f : 0.f);

	float MouseDX = 0.f;
	float MouseDY = 0.f;
	GetInputMouseDelta(MouseDX, MouseDY);

	const bool bFireHeld = IsInputKeyDown(EKeys::LeftMouseButton);

	if (HasAuthority())
	{
		Dragon->ApplyFlightInput(ForwardAxis, RightAxis, UpAxis, MouseDX, MouseDY, bFireHeld, DeltaTime);
	}
	else
	{
		Dragon->ApplyFlightInput(ForwardAxis, RightAxis, UpAxis, MouseDX, MouseDY, bFireHeld, DeltaTime);
		ServerApplyDragonFlightInput(ForwardAxis, RightAxis, UpAxis, MouseDX, MouseDY, bFireHeld, DeltaTime);
	}
}

bool AChroniclesPlayerController::ComputeSpellAim(
	AAetheriaHeroCharacter* Hero,
	FVector& OutOrigin,
	FVector& OutDir,
	FVector& OutTargetPoint) const
{
	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	GetPlayerViewPoint(CamLoc, CamRot);

	OutDir = CamRot.Vector().GetSafeNormal();
	OutOrigin = Hero
		? (Hero->GetActorLocation() + FVector(0.f, 0.f, 58.f) + OutDir * 72.f)
		: (CamLoc + OutDir * 120.f);

	OutTargetPoint = CamLoc + OutDir * 2400.f;

	if (UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params(TEXT("ChroniclesSpellAimTrace"), false);
		if (Hero)
		{
			Params.AddIgnoredActor(Hero);
		}
		if (APawn* P = GetPawn())
		{
			Params.AddIgnoredActor(P);
		}

		// Use dedicated projectile channel (CHRONICLE_TRACE_PROJECTILE), NEVER ECC_Visibility!
		if (World->LineTraceSingleByChannel(Hit, CamLoc, CamLoc + OutDir * 8500.f, CHRONICLE_TRACE_PROJECTILE, Params))
		{
			OutTargetPoint = Hit.ImpactPoint;
			return true;
		}
	}

	return false;
}

void AChroniclesPlayerController::HandleInteractOrExort()
{
	if (AAetheriaNPCActor* NearbyNPC = FindNearestInteractableNPC(420.f))
	{
		OpenDialogueWithNPC(NearbyNPC);
		return;
	}

	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		if (!Hero->CanCastSpellsByStature())
		{
			PushToastMessage(TEXT("Stature < 1.45m: Too small to channel Aether — Spells locked! Open [C] Hero Creator to adjust height."));
			return;
		}

		if (Hero->SpellSystem)
		{
			if (!Hero->SpellSystem->PushOrb(EChronicleOrb::Exort))
			{
				PushToastMessage(TEXT("Exort Locked — Walk to Elder Aldric in Oakhaven and press [E] to speak!"));
			}
		}
	}
}

void AChroniclesPlayerController::OpenDialogueWithNPC(AAetheriaNPCActor* NPC)
{
	if (!IsValid(NPC))
	{
		return;
	}

	if (ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer)
	{
		if (AAetheriaHeroCharacter* Hero = GetActiveHero())
		{
			Hero->SetCreatorStudioCameraMode(false);
		}
	}

	ActiveDialogueNPC = NPC;
	ActiveOverlay = EChroniclesOverlayMode::Dialogue;
	if (HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}
	SetUIMode(true);
}

void AChroniclesPlayerController::HandleDialoguePrimaryAction()
{
	AAetheriaNPCActor* NPC = ActiveDialogueNPC.Get();
	AAetheriaHeroCharacter* Hero = GetActiveHero();
	UGameInstance* GI = GetGameInstance();
	UQuestSubsystem* QuestSub = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;

	if (!NPC || !QuestSub)
	{
		CloseAllOverlays();
		return;
	}

	const FQuestDef* Quest = QuestSub->FindQuest(NPC->QuestId);
	if (!Quest)
	{
		Quest = QuestSub->FindQuestForGiver(NPC->NpcId);
	}

	if (!Quest)
	{
		CloseAllOverlays();
		return;
	}

	const FName TargetQuestId = Quest->QuestId;

	switch (Quest->State)
	{
	case EQuestState::Available:
	{
		QuestSub->AcceptQuest(TargetQuestId);
		QuestSub->SetTrackedQuest(TargetQuestId);
		break;
	}
	case EQuestState::ReadyToTurnIn:
	{
		int32 GoldReward = 0;
		int32 XPReward = 0;
		EChronicleOrb UnlockedOrb = EChronicleOrb::None;
		bool bUnlockedDragon = false;

		if (QuestSub->TurnInQuest(TargetQuestId, GoldReward, XPReward, UnlockedOrb, bUnlockedDragon))
		{
			if (Hero)
			{
				if (Hero->SkillTree)
				{
					Hero->SkillTree->AddGold(GoldReward);
					Hero->SkillTree->AddXP(XPReward);
				}
				if (UnlockedOrb != EChronicleOrb::None && Hero->SpellSystem)
				{
					Hero->SpellSystem->UnlockOrb(UnlockedOrb);
				}
				if (bUnlockedDragon)
				{
					Hero->bDragonUnlocked = true;
				}
				if (Hero->SkillTree)
				{
					Hero->SkillTree->SaveProgress();
				}
			}
		}
		break;
	}
	case EQuestState::Active:
	{
		QuestSub->SetTrackedQuest(TargetQuestId);
		PushToastMessage(FString::Printf(TEXT("Tracking Quest: %s"), *Quest->Title.ToString()), 3.2f);
		break;
	}
	case EQuestState::Completed:
	default:
		break;
	}

	if (HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}
	CloseAllOverlays();
}

void AChroniclesPlayerController::TryToggleDragonMount()
{
	if (!HasAuthority())
	{
		ServerToggleDragonMount();
		return;
	}

	// Currently flying -> Dismount cleanly
	if (AAetheriaDragonActor* FlyingDragon = GetControlledDragon())
	{
		FlyingDragon->DismountRider(this);
		PushToastMessage(TEXT("Dismounted Ignis the Red Dragon."), 2.6f);
		return;
	}

	AAetheriaHeroCharacter* Hero = GetActiveHero();
	if (!Hero || Hero->bDead)
	{
		return;
	}

	// Check story unlock (allow mount if Dragon unlocked OR Q_TameDragon completed/ready)
	bool bCanRide = Hero->bDragonUnlocked || bSmokeTestActive;
	if (!bCanRide)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (const UQuestSubsystem* QuestSub = GI->GetSubsystem<UQuestSubsystem>())
			{
				if (const FQuestDef* TameQ = QuestSub->FindQuest(FName(TEXT("Q_TameDragon"))))
				{
					if (TameQ->State == EQuestState::Completed || TameQ->State == EQuestState::ReadyToTurnIn)
					{
						bCanRide = true;
						Hero->bDragonUnlocked = true;
					}
				}
			}
		}
	}

	if (!bCanRide)
	{
		PushToastMessage(TEXT("Ignis is bound at Dragon Aerie — Complete 'Taming the Crimson Wing' (Scholar Lyra)!"), 4.2f);
		return;
	}

	AAetheriaDragonActor* Dragon = FindNearestDragon(1600.f);
	if (!Dragon)
	{
		// Summon unlocked Ignis directly beside Hero so players are never stranded!
		Dragon = FindNearestDragon(500000.f);
		if (Dragon && !Dragon->bMounted)
		{
			const FVector SummonLoc = Hero->GetActorLocation() + Hero->GetActorForwardVector() * 420.f + FVector(0.f, 0.f, 160.f);
			Dragon->SetActorLocationAndRotation(SummonLoc, Hero->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
			PushToastMessage(TEXT("Whistled for Ignis — the Crimson Wing descends!"), 3.2f);
		}
	}

	if (Dragon && !Dragon->bMounted)
	{
		Dragon->MountRider(Hero, this);
		PushToastMessage(TEXT("Mounted Ignis! [WASD] Fly · [Space/Shift] Climb/Dive · [LMB] Fire Breath · [F] Land"), 4.5f);
	}
}

void AChroniclesPlayerController::RequestCycleTimeOfDay()
{
	if (!HasAuthority())
	{
		ServerCycleTimeOfDay();
		return;
	}

	if (AAetheriaWorldBuilder* Builder = FindWorldBuilder())
	{
		Builder->CycleTimeOfDayPreset();
		if (AChroniclesGameState* GS = GetWorld() ? GetWorld()->GetGameState<AChroniclesGameState>() : nullptr)
		{
			GS->ReplicatedTimeOfDay = Builder->TimeOfDay;
			GS->ActiveWorldBanner = FString::Printf(TEXT("Time of Day · %s"), *Builder->GetTimeOfDayLabel());
		}
		PushToastMessage(FString::Printf(TEXT("Atmosphere: %s"), *Builder->GetTimeOfDayLabel()), 3.0f);
	}
}

void AChroniclesPlayerController::RequestRespawnHero()
{
	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->RespawnAtOakhaven();
		bLastKnownHeroDead = false;
		CloseAllOverlays();
		PushToastMessage(TEXT("Returned to Oakhaven Sanctuary."), 3.5f);
	}
}

void AChroniclesPlayerController::RequestFastTravelToPin(FName PinId)
{
	UGameInstance* GI = GetGameInstance();
	UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;
	if (!MapSub)
	{
		return;
	}

	for (const FMapPinInfo& Pin : MapSub->GetMapPins())
	{
		if (Pin.PinId == PinId && Pin.bDiscovered && Pin.bFastTravel)
		{
			FVector Dest = Pin.WorldLocation;
			if (AAetheriaWorldBuilder* Builder = FindWorldBuilder())
			{
				Dest.Z = Builder->SampleGroundZ(Dest.X, Dest.Y, Dest.Z) + 120.f;
			}
			else
			{
				Dest.Z += 120.f;
			}

			if (HasAuthority())
			{
				if (AAetheriaHeroCharacter* Hero = GetActiveHero())
				{
					Hero->SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
					MapSub->RevealAroundWorldLocation(Dest, 5500.f);
				}
			}
			else
			{
				ServerFastTravel(Dest);
			}

			CloseAllOverlays();
			PushToastMessage(FString::Printf(TEXT("Fast Traveled to %s"), *Pin.Label.ToString()), 3.4f);
			return;
		}
	}
}

void AChroniclesPlayerController::HostCoop()
{
	if (UWorld* World = GetWorld())
	{
		PushToastMessage(TEXT("Opening Listen Server: OpenWorld?listen ..."), 4.f);
		UE_LOG(LogChronicles, Log, TEXT("[Co-op] HostCoop -> OpenLevel(OpenWorld?listen)"));
		UGameplayStatics::OpenLevel(World, FName(TEXT("OpenWorld")), true, TEXT("listen"));
	}
}

void AChroniclesPlayerController::JoinCoop(const FString& ServerIP)
{
	const FString Trimmed = ServerIP.TrimStartAndEnd();
	const FString TargetAddress = Trimmed.IsEmpty() ? TEXT("127.0.0.1") : Trimmed;
	PushToastMessage(FString::Printf(TEXT("Joining Co-op Host at %s ..."), *TargetAddress), 4.f);
	UE_LOG(LogChronicles, Log, TEXT("[Co-op] JoinCoop -> ClientTravel(%s)"), *TargetAddress);
	ClientTravel(TargetAddress, ETravelType::TRAVEL_Absolute);
}

void AChroniclesPlayerController::ServerCycleTimeOfDay_Implementation()
{
	RequestCycleTimeOfDay();
}

void AChroniclesPlayerController::ServerToggleDragonMount_Implementation()
{
	TryToggleDragonMount();
}

void AChroniclesPlayerController::ServerFastTravel_Implementation(const FVector& TargetLocation)
{
	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AChroniclesPlayerController::ServerApplyDragonFlightInput_Implementation(
	float ForwardAxis,
	float RightAxis,
	float UpAxis,
	float YawInput,
	float PitchInput,
	bool bFireHeld,
	float DeltaTime)
{
	if (AAetheriaDragonActor* Dragon = GetControlledDragon())
	{
		Dragon->ApplyFlightInput(ForwardAxis, RightAxis, UpAxis, YawInput, PitchInput, bFireHeld, FMath::Clamp(DeltaTime, 0.f, 0.1f));
	}
}

// ============================================================================
// Automated -game -wizardsmoke Verification Harness (strictly DeltaSeconds-driven)
// ============================================================================

void AChroniclesPlayerController::StartSmokeTestIfRequested()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("wizardsmoke")))
	{
		bSmokeTestActive = true;
		SmokeStepIndex = 0;
		SmokeSubPhase = 0;
		SmokeStepElapsed = 0.f;
		SmokeTotalElapsed = 0.f;
		bSmokeObservedFireBreath = false;
		bSmokeBossEngaged = false;
		bSmokeBossTelegraphed = false;

		UE_LOG(LogWizardsmoke, Display, TEXT("=================================================================="));
		UE_LOG(LogWizardsmoke, Display, TEXT("AETHERIA CHRONICLES -wizardsmoke VERIFICATION HARNESS INITIALIZED"));
		UE_LOG(LogWizardsmoke, Display, TEXT("=================================================================="));
	}
}

void AChroniclesPlayerController::AbortSmokeTestWithError(const FString& Reason)
{
	UE_LOG(LogWizardsmoke, Error, TEXT("WIZARDSMOKE FAILED at Step %d (SubPhase %d): %s"),
		SmokeStepIndex + 1, SmokeSubPhase, *Reason);
	bSmokeTestActive = false;
	FGenericPlatformMisc::RequestExit(false);
}

void AChroniclesPlayerController::RunSmokeTestTick(float DeltaTime)
{
	const float SafeDT = FMath::Clamp(DeltaTime, 0.001f, 0.1f);
	SmokeStepElapsed += SafeDT;
	SmokeTotalElapsed += SafeDT;

	// Hard watchdog timeout so CI/headless verification can never stall
	if (SmokeTotalElapsed > 40.f)
	{
		AbortSmokeTestWithError(TEXT("Watchdog timeout exceeded 40.0 seconds."));
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	switch (SmokeStepIndex)
	{
	// ------------------------------------------------------------------------
	// STEP 1: Verify UGraphicsRTXSubsystem hardware detection & render tier
	// ------------------------------------------------------------------------
	case 0:
	{
		if (SmokeStepElapsed < 0.35f)
		{
			return;
		}

		UGraphicsRTXSubsystem* RTXSub = GI->GetSubsystem<UGraphicsRTXSubsystem>();
		if (!RTXSub)
		{
			AbortSmokeTestWithError(TEXT("UGraphicsRTXSubsystem missing from GameInstance."));
			return;
		}

		if (RTXSub->GetTelemetry().GPUAdapterName.IsEmpty())
		{
			RTXSub->DetectAndConfigureRenderTier();
		}

		const FGraphicsTelemetry& Tel = RTXSub->GetTelemetry();
		const TCHAR* TierStr = (Tel.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
			? TEXT("CrimsonRTXUltra")
			: TEXT("ClassicPerformance");

		UE_LOG(LogWizardsmoke, Display,
			TEXT("STEP 1 OK | GPU='%s' | Tier=%s | RTXSupported=%d | RTXActive=%d | DLSSActive=%d | TexturePool=%dMB"),
			*Tel.GPUAdapterName, TierStr,
			Tel.bHardwareRTXSupported ? 1 : 0,
			Tel.bHardwareRTXActive ? 1 : 0,
			Tel.bDLSSActive ? 1 : 0,
			Tel.TexturePoolMB);

		SmokeStepIndex = 1;
		SmokeStepElapsed = 0.f;
		break;
	}

	// ------------------------------------------------------------------------
	// STEP 2: Verify Hero spawned + SkeletalMesh bound + Fog-of-War > 0%
	// ------------------------------------------------------------------------
	case 1:
	{
		if (SmokeStepElapsed < 0.35f)
		{
			return;
		}

		AAetheriaHeroCharacter* Hero = GetActiveHero();
		if (!Hero)
		{
			if (SmokeStepElapsed > 5.f)
			{
				AbortSmokeTestWithError(TEXT("Hero pawn failed to spawn within 5 seconds."));
			}
			return;
		}

		USkeletalMesh* BoundMesh = Hero->GetMesh() ? Hero->GetMesh()->GetSkeletalMeshAsset() : nullptr;
		if (!BoundMesh)
		{
			AbortSmokeTestWithError(TEXT("Hero SkeletalMeshComponent has no SkeletalMesh asset bound."));
			return;
		}

		UFogOfWarMapSubsystem* MapSub = GI->GetSubsystem<UFogOfWarMapSubsystem>();
		if (!MapSub)
		{
			AbortSmokeTestWithError(TEXT("UFogOfWarMapSubsystem missing."));
			return;
		}

		MapSub->RevealAroundWorldLocation(Hero->GetActorLocation(), 6500.f);
		const float ExploredPct = MapSub->GetTotalExplorationPercent();
		if (ExploredPct <= 0.f)
		{
			AbortSmokeTestWithError(TEXT("FogOfWarMapSubsystem exploration percent remained 0% after RevealAroundWorldLocation."));
			return;
		}

		// Verify RPG Hero Customizer, Attributes, Race, and Stature Physics
		if (!Hero->Customization || !Hero->SpellSystem)
		{
			AbortSmokeTestWithError(TEXT("Hero Customization or SpellSystem component missing in Step 2."));
			return;
		}

		UHeroCustomizationComponent* Custom = Hero->Customization;
		const bool bPrevExortUnlocked = Hero->SpellSystem->bUnlockedExort;
		Hero->SpellSystem->bUnlockedExort = true;
		const float BaselineMaxHP = Hero->MaxHP;

		// 1. Small stature (< 1.45m) spell lockout check
		Custom->SetHeightMeters(1.25f);
		const bool bSmallLockoutOK = !Hero->CanCastSpellsByStature() && !Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);

		// 2. Giant Titan (2.95m) + HighlandNord + Strength +5 check
		Custom->SetRace(EHeroRace::HighlandNord, false);
		Custom->SetHeightMeters(2.95f);
		Custom->ModifyAttribute(EHeroAttribute::Strength, 5);

		const float GiantMaxHP = Hero->MaxHP;
		const float GiantTurnYaw = Hero->GetCharacterMovement() ? Hero->GetCharacterMovement()->RotationRate.Yaw : 999.f;
		const float GiantMeleeMult = Custom->GetDerivedStats().MeleeDamageMultiplier;

		const bool bGiantSlowTurnOK = (GiantTurnYaw < 260.f);
		const bool bStrHPOK = (GiantMaxHP > BaselineMaxHP + 15.f) && (GiantMeleeMult > 1.7f);

		// 3. Reset cleanly back to Human, 1.85m, ResetAttributes()
		Hero->SpellSystem->bUnlockedExort = bPrevExortUnlocked;
		Custom->SetRace(EHeroRace::Human, true);
		Custom->SetHeightMeters(1.85f);
		Custom->ResetAttributes();
		Hero->HP = Hero->MaxHP;

		if (!bSmallLockoutOK || !bGiantSlowTurnOK || !bStrHPOK)
		{
			AbortSmokeTestWithError(FString::Printf(
				TEXT("HeroCustomizer verification failed (SmallLockout=%d GiantSlowTurn=%d[Yaw=%.1f] StrHP=%d[HP=%.1f->%.1f Melee=%.2f])."),
				bSmallLockoutOK ? 1 : 0,
				bGiantSlowTurnOK ? 1 : 0, GiantTurnYaw,
				bStrHPOK ? 1 : 0, BaselineMaxHP, GiantMaxHP, GiantMeleeMult));
			return;
		}

		UE_LOG(LogWizardsmoke, Display,
			TEXT("STEP 2 OK | Hero=%s | Mesh=%s | Loc=(%.0f,%.0f,%.0f) | Explored=%.2f%% | HeroCustomizer=OK (SmallLockout=1 GiantSlowTurn=1 StrHP=1)"),
			*Hero->GetName(), *BoundMesh->GetName(),
			Hero->GetActorLocation().X, Hero->GetActorLocation().Y, Hero->GetActorLocation().Z,
			ExploredPct);

		SmokeStepIndex = 2;
		SmokeStepElapsed = 0.f;
		break;
	}

	// ------------------------------------------------------------------------
	// STEP 3: Verify Quest 1->Exort (1 spell), Cast EEE, Quest 3->Wex (4), Quest 5->Quas (10)
	// ------------------------------------------------------------------------
	case 2:
	{
		if (SmokeStepElapsed < 0.35f)
		{
			return;
		}

		AAetheriaHeroCharacter* Hero = GetActiveHero();
		UQuestSubsystem* QuestSub = GI->GetSubsystem<UQuestSubsystem>();
		if (!Hero || !Hero->SpellSystem || !QuestSub)
		{
			AbortSmokeTestWithError(TEXT("Hero, SpellSystem, or QuestSubsystem missing in Step 3."));
			return;
		}

		USpellSystemComponent* Spells = Hero->SpellSystem;

		// Ensure deterministic baseline for smoke verification
		Spells->bUnlockedExort = false;
		Spells->bUnlockedWex = false;
		Spells->bUnlockedQuas = false;
		Spells->ActiveOrbs.Reset();
		Spells->SlotD = EChronicleSpellId::None;
		Spells->SlotF = EChronicleSpellId::None;
		Spells->Mana = Spells->MaxMana;

		// Quest 1: Q_AwakenExort -> Unlocks Exort (1 spell: EEE SunStrike)
		int32 GoldOut = 0, XPOut = 0;
		EChronicleOrb OrbOut = EChronicleOrb::None;
		bool bDragonOut = false;

		QuestSub->AdvanceQuestProgress(FName(TEXT("Q_AwakenExort")), 2);
		QuestSub->TurnInQuest(FName(TEXT("Q_AwakenExort")), GoldOut, XPOut, OrbOut, bDragonOut);
		if (OrbOut != EChronicleOrb::None)
		{
			Spells->UnlockOrb(OrbOut);
		}
		const int32 SpellsAfterExort = Spells->GetUnlockedSpellCount();

		// Push E-E-E, Invoke SunStrike, Cast Slot D, verify mana consumption & cooldown!
		const float ManaBeforeCast = Spells->Mana;
		const bool bPushed1 = Spells->PushOrb(EChronicleOrb::Exort);
		const bool bPushed2 = Spells->PushOrb(EChronicleOrb::Exort);
		const bool bPushed3 = Spells->PushOrb(EChronicleOrb::Exort);
		const bool bInvokedEEE = Spells->InvokeCurrentSpell();

		const FVector CastOrigin = Hero->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		const FVector CastDir = Hero->GetActorForwardVector().GetSafeNormal();
		const bool bCastEEE = Spells->CastSpellSlot(false, CastOrigin, CastDir, CastOrigin + CastDir * 850.f, 0.5f);
		const float ManaAfterCast = Spells->Mana;
		const float CooldownEEE = Spells->GetSpellCooldownRemaining(EChronicleSpellId::SunStrike);

		// Quest 2: Q_TameDragon -> Unlocks Dragon Mount for Step 5
		QuestSub->AdvanceQuestProgress(FName(TEXT("Q_TameDragon")), 2);
		QuestSub->TurnInQuest(FName(TEXT("Q_TameDragon")), GoldOut, XPOut, OrbOut, bDragonOut);
		if (bDragonOut)
		{
			Hero->bDragonUnlocked = true;
		}

		// Quest 3: Q_StormSpires -> Unlocks Wex (4 spells total)
		QuestSub->AdvanceQuestProgress(FName(TEXT("Q_StormSpires")), 3);
		QuestSub->TurnInQuest(FName(TEXT("Q_StormSpires")), GoldOut, XPOut, OrbOut, bDragonOut);
		if (OrbOut != EChronicleOrb::None)
		{
			Spells->UnlockOrb(OrbOut);
		}
		const int32 SpellsAfterWex = Spells->GetUnlockedSpellCount();

		// Quest 5: Q_GlacierSilence -> Unlocks Quas (all 10 spells total!)
		QuestSub->AdvanceQuestProgress(FName(TEXT("Q_GlacierSilence")), 1);
		QuestSub->TurnInQuest(FName(TEXT("Q_GlacierSilence")), GoldOut, XPOut, OrbOut, bDragonOut);
		if (OrbOut != EChronicleOrb::None)
		{
			Spells->UnlockOrb(OrbOut);
		}
		const int32 SpellsAfterQuas = Spells->GetUnlockedSpellCount();

		if (SpellsAfterExort != 1 || SpellsAfterWex != 4 || SpellsAfterQuas != 10)
		{
			AbortSmokeTestWithError(FString::Printf(
				TEXT("Expected progressive spell unlock counts 1->4->10, got %d->%d->%d."),
				SpellsAfterExort, SpellsAfterWex, SpellsAfterQuas));
			return;
		}

		if (!bPushed1 || !bPushed2 || !bPushed3 || !bInvokedEEE || !bCastEEE || !(ManaAfterCast < ManaBeforeCast) || !(CooldownEEE > 0.f))
		{
			AbortSmokeTestWithError(FString::Printf(
				TEXT("EEE Solar Cataclysm cast verification failed (Invoked=%d Cast=%d Mana=%.1f->%.1f CD=%.2f)."),
				bInvokedEEE ? 1 : 0, bCastEEE ? 1 : 0, ManaBeforeCast, ManaAfterCast, CooldownEEE));
			return;
		}

		UE_LOG(LogWizardsmoke, Display,
			TEXT("STEP 3 OK | ProgressiveSpells=%d->%d->%d | CastEEE=1 | Mana=%.1f->%.1f | Cooldown=%.2fs | DragonUnlocked=%d"),
			SpellsAfterExort, SpellsAfterWex, SpellsAfterQuas, ManaBeforeCast, ManaAfterCast, CooldownEEE,
			Hero->bDragonUnlocked ? 1 : 0);

		SmokeStepIndex = 3;
		SmokeStepElapsed = 0.f;
		break;
	}

	// ------------------------------------------------------------------------
	// STEP 4: Verify SkillTree XP Level-Up + Upgrade exort_solar_core + SaveProgress()
	// ------------------------------------------------------------------------
	case 3:
	{
		if (SmokeStepElapsed < 0.30f)
		{
			return;
		}

		AAetheriaHeroCharacter* Hero = GetActiveHero();
		if (!Hero || !Hero->SkillTree)
		{
			AbortSmokeTestWithError(TEXT("Hero or SkillTreeComponent missing in Step 4."));
			return;
		}

		USkillTreeComponent* Skills = Hero->SkillTree;
		const int32 LevelBefore = Skills->Level;
		Skills->AddXP(360);

		const bool bUpgraded = Skills->UpgradeNode(FName(TEXT("exort_solar_core")));
		const int32 NodeRank = Skills->GetNodeRank(FName(TEXT("exort_solar_core")));
		const bool bSaved = Skills->SaveProgress(TEXT("AetheriaChroniclesSmokeSlot"));

		if (Skills->Level <= LevelBefore || !bUpgraded || NodeRank < 1 || !bSaved)
		{
			AbortSmokeTestWithError(FString::Printf(
				TEXT("SkillTree verification failed (Level=%d->%d Upgraded=%d Rank=%d Saved=%d)."),
				LevelBefore, Skills->Level, bUpgraded ? 1 : 0, NodeRank, bSaved ? 1 : 0));
			return;
		}

		UE_LOG(LogWizardsmoke, Display,
			TEXT("STEP 4 OK | Level=%d->%d | exort_solar_core Rank=%d | ExortMult=%.2f | SaveProgress=%d"),
			LevelBefore, Skills->Level, NodeRank, Skills->GetExortDamageMultiplier(), bSaved ? 1 : 0);

		SmokeStepIndex = 4;
		SmokeSubPhase = 0;
		SmokeStepElapsed = 0.f;
		break;
	}

	// ------------------------------------------------------------------------
	// STEP 5: Verify Mounting Red Dragon, Aerial Flight + Fire Breath & Safe Dismount
	// ------------------------------------------------------------------------
	case 4:
	{
		AAetheriaDragonActor* Dragon = GetControlledDragon();
		if (!Dragon)
		{
			Dragon = FindNearestDragon(500000.f);
		}
		AAetheriaHeroCharacter* Hero = GetActiveHero();

		if (!Dragon || !Hero)
		{
			AbortSmokeTestWithError(TEXT("Dragon or Hero actor missing in Step 5."));
			return;
		}

		if (SmokeSubPhase == 0)
		{
			if (SmokeStepElapsed < 0.25f)
			{
				return;
			}

			Dragon->MountRider(Hero, this);
			if (!Dragon->bMounted || GetPawn() != Dragon)
			{
				AbortSmokeTestWithError(TEXT("Dragon->MountRider failed to mount and possess Dragon."));
				return;
			}

			SmokeDragonStartLoc = Dragon->GetActorLocation();
			bSmokeObservedFireBreath = false;
			SmokeSubPhase = 1;
			SmokeStepElapsed = 0.f;
			return;
		}

		if (SmokeSubPhase == 1)
		{
			// Fly forward & climb while breathing dragonfire for 0.65 seconds
			Dragon->ApplyFlightInput(1.0f, 0.25f, 0.40f, 0.35f, 0.0f, true, SafeDT);
			if (Dragon->bBreathingFire)
			{
				bSmokeObservedFireBreath = true;
			}

			if (SmokeStepElapsed < 0.65f)
			{
				return;
			}

			const float DistanceFlown = FVector::Dist(SmokeDragonStartLoc, Dragon->GetActorLocation());
			Dragon->DismountRider(this);

			if (Dragon->bMounted || GetPawn() != Hero || DistanceFlown < 50.f || !bSmokeObservedFireBreath)
			{
				AbortSmokeTestWithError(FString::Printf(
					TEXT("Dragon flight/dismount failed (Mounted=%d PossessedHero=%d Dist=%.1f Fire=%d)."),
					Dragon->bMounted ? 1 : 0, (GetPawn() == Hero) ? 1 : 0, DistanceFlown, bSmokeObservedFireBreath ? 1 : 0));
				return;
			}

			UE_LOG(LogWizardsmoke, Display,
				TEXT("STEP 5 OK | DragonFlown=%.0fcm | FireBreath=%d | DismountedCleanly=%d"),
				DistanceFlown, bSmokeObservedFireBreath ? 1 : 0, (!Dragon->bMounted && GetPawn() == Hero) ? 1 : 0);

			SmokeStepIndex = 5;
			SmokeSubPhase = 0;
			SmokeStepElapsed = 0.f;
		}
		break;
	}

	// ------------------------------------------------------------------------
	// STEP 6: Verify Boss Engagement, Crimson Telegraph, Clean Disengage + GC Safety
	// ------------------------------------------------------------------------
	case 5:
	{
		AAetheriaHeroCharacter* Hero = GetActiveHero();
		AAetheriaBossCharacter* Boss = FindWorldBoss();
		if (!Hero || !Boss)
		{
			AbortSmokeTestWithError(TEXT("Hero or Malakor Boss missing in Step 6."));
			return;
		}

		if (SmokeSubPhase == 0)
		{
			// Move Hero cleanly into Highkeep Citadel engagement range of Malakor
			const FVector EngageLoc = Boss->GetActorLocation() + FVector(0.f, 740.f, 25.f);
			Hero->HP = Hero->MaxHP;
			Hero->bDead = false;
			Hero->InvulnTimer = 5.0f;
			Hero->SetActorLocation(EngageLoc, false, nullptr, ETeleportType::TeleportPhysics);

			bSmokeBossEngaged = false;
			bSmokeBossTelegraphed = false;
			SmokeSubPhase = 1;
			SmokeStepElapsed = 0.f;
			return;
		}

		if (SmokeSubPhase == 1)
		{
			Hero->InvulnTimer = 3.0f;
			if (Boss->bBossEngaged)
			{
				bSmokeBossEngaged = true;
			}
			if (Boss->bIsTelegraphing)
			{
				bSmokeBossTelegraphed = true;
			}

			if ((bSmokeBossEngaged && bSmokeBossTelegraphed) || SmokeStepElapsed >= 2.6f)
			{
				// Retreat cleanly back to Oakhaven Sanctuary (> 5400 cm disengage radius)
				Hero->SetActorLocation(Hero->OakhavenSpawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
				SmokeSubPhase = 2;
				SmokeStepElapsed = 0.f;
			}
			return;
		}

		if (SmokeSubPhase == 2)
		{
			if (SmokeStepElapsed < 0.35f)
			{
				return;
			}

			// Schedule GC cleanly for the end-of-frame boundary (never call CollectGarbage inside World->Tick on World Partition!)
			if (GEngine)
			{
				GEngine->ForceGarbageCollection(true);
			}
			SmokeSubPhase = 3;
			SmokeStepElapsed = 0.f;
			return;
		}

		if (SmokeSubPhase == 3)
		{
			if (SmokeStepElapsed < 0.25f)
			{
				return;
			}

			const bool bCleanDisengage = !Boss->bBossEngaged;

			AAetheriaHeroCharacter* PostGCHero = GetActiveHero();
			AAetheriaBossCharacter* PostGCBoss = FindWorldBoss();
			AAetheriaWorldBuilder* PostGCWorld = FindWorldBuilder();
			const bool bPostGCValid =
				IsValid(PostGCHero) &&
				IsValid(PostGCHero->SpellSystem) &&
				IsValid(PostGCHero->SkillTree) &&
				PostGCHero->GetMesh() != nullptr &&
				PostGCHero->GetMesh()->GetSkeletalMeshAsset() != nullptr &&
				IsValid(PostGCBoss) &&
				IsValid(PostGCWorld);

			if (!bSmokeBossEngaged || !bSmokeBossTelegraphed || !bCleanDisengage || !bPostGCValid)
			{
				AbortSmokeTestWithError(FString::Printf(
					TEXT("Boss/GC gate failed (Engaged=%d Telegraph=%d Disengaged=%d PostGC=%d)."),
					bSmokeBossEngaged ? 1 : 0, bSmokeBossTelegraphed ? 1 : 0, bCleanDisengage ? 1 : 0, bPostGCValid ? 1 : 0));
				return;
			}

			UE_LOG(LogWizardsmoke, Display,
				TEXT("STEP 6 OK | BossEngaged=%d | CrimsonTelegraph=%d | CleanDisengage=%d | PostGCValid=%d"),
				bSmokeBossEngaged ? 1 : 0, bSmokeBossTelegraphed ? 1 : 0, bCleanDisengage ? 1 : 0, bPostGCValid ? 1 : 0);

			SmokeStepIndex = 6;
			SmokeSubPhase = 0;
			SmokeStepElapsed = 0.f;
		}
		break;
	}

	// ------------------------------------------------------------------------
	// STEP 7: Clean Exit
	// ------------------------------------------------------------------------
	case 6:
	default:
	{
		if (SmokeStepElapsed < 0.25f)
		{
			return;
		}

		UE_LOG(LogWizardsmoke, Display,
			TEXT("STEP 7 OK | All 7 AetheriaChronicles verification gates PASSED in %.2fs! Exiting cleanly."),
			SmokeTotalElapsed);

		bSmokeTestActive = false;
		FGenericPlatformMisc::RequestExit(false);
		break;
	}
	}
}

void AChroniclesPlayerController::StartCaptureFramesIfRequested()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("captureframes")))
	{
		bCaptureFramesActive = true;
		CaptureStage = 0;
		bCaptureStageSetupDone = false;
		CaptureStageElapsed = 0.f;
		UE_LOG(LogWizardsmoke, Display, TEXT("AETHERIA -captureframes VISUAL CRITIC HARNESS INITIALIZED"));
	}
}

void AChroniclesPlayerController::RunCaptureFramesTick(float DeltaTime)
{
	const float SafeDT = FMath::Clamp(DeltaTime, 0.001f, 0.1f);
	CaptureStageElapsed += SafeDT;

	AAetheriaHeroCharacter* Hero = GetActiveHero();
	AAetheriaDragonActor* Dragon = FindNearestDragon(500000.f);
	AAetheriaBossCharacter* Boss = FindWorldBoss();
	AAetheriaWorldBuilder* Builder = FindWorldBuilder();
	if (!Hero)
	{
		return;
	}

	const FString OutDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/CriticLoop"));
	auto SnapFrame = [&](const TCHAR* FileName)
	{
		const FString FullPath = OutDir / FileName;
		FScreenshotRequest::RequestScreenshot(FullPath, true, false);
		UE_LOG(LogWizardsmoke, Display, TEXT("[CaptureFrames] Requested screenshot -> %s"), *FullPath);
	};

	switch (CaptureStage)
	{
	case 0: // Stage 0: Oakhaven Village Plaza + Full-Body Kwang Hero + 3 Orbs + Golden Sunset Vista
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			CloseAllOverlays();
			if (Builder) { Builder->SetTimeOfDay(0.72f); }
			Hero->HP = Hero->MaxHP;
			Hero->InvulnTimer = 30.f;
			Hero->bDragonUnlocked = true;
			if (Hero->SpellSystem)
			{
				Hero->SpellSystem->UnlockOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->UnlockOrb(EChronicleOrb::Wex);
				Hero->SpellSystem->UnlockOrb(EChronicleOrb::Quas);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Wex);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Quas);
			}
			// Wide-open sunlit Oakhaven plaza framing Kwang full-body, Gothic statues, Runic Altar, village & golden sunset sky
			const float GZ = Builder ? Builder->SampleGroundZ(0.f, 3450.f, 120.f) : 120.f;
			Hero->SetActorLocationAndRotation(FVector(0.f, 3450.f, GZ + 105.f), FRotator(0.f, 90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(FRotator(-6.f, 90.f, 0.f));
		}
		else if (CaptureStageElapsed >= 2.2f)
		{
			if (UWorld* World = GetWorld())
			{
				FVector CamLoc = FVector::ZeroVector;
				FRotator CamRot = FRotator::ZeroRotator;
				GetPlayerViewPoint(CamLoc, CamRot);
				const FVector HeroChest = Hero->GetActorLocation() + FVector(0.f, 0.f, 55.f);
				FHitResult CamHit;
				FCollisionQueryParams CamParams(TEXT("CaptureCamObstructionCheck"), false);
				CamParams.AddIgnoredActor(Hero);
				const bool bObstructed = World->LineTraceSingleByChannel(CamHit, CamLoc, HeroChest, ECC_Visibility, CamParams);
				UE_LOG(LogWizardsmoke, Display,
					TEXT("[CaptureFrames] Stage 0 Camera->Hero Trace: Obstructed=%d | HitActor=%s | HitComp=%s | Dist=%.1fcm"),
					bObstructed ? 1 : 0,
					(bObstructed && CamHit.GetActor()) ? *CamHit.GetActor()->GetName() : TEXT("None"),
					(bObstructed && CamHit.GetComponent()) ? *CamHit.GetComponent()->GetName() : TEXT("None"),
					bObstructed ? CamHit.Distance : FVector::Dist(CamLoc, HeroChest));
			}

			SnapFrame(TEXT("01_oakhaven_kwang_hero.png"));
			CaptureStage = 1;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 1: // Stage 1: Real Niagara + Cascade Invoker Spell (SunStrike 7.5m ahead in open sunlit plaza)
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			if (Hero->SpellSystem)
			{
				Hero->SpellSystem->Mana = Hero->SpellSystem->MaxMana;
				Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->InvokeCurrentSpell();
				const FVector Fwd = Hero->GetActorForwardVector();
				const FVector Target = Hero->GetActorLocation() + Fwd * 750.f;
				Hero->SpellSystem->CastSpellSlot(false, Hero->GetActorLocation() + FVector(0.f, 0.f, 70.f), Fwd, Target, 0.85f);
			}
		}
		else if (CaptureStageElapsed >= 0.55f)
		{
			SnapFrame(TEXT("02_spell_sunstrike_niagara.png"));
			CaptureStage = 2;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 2: // Stage 2: DragonTheUsurperSK Full-Wingspan Mounted Flight + Blazing Fire Breath over Mirror Lake & Golden Sunset
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			if (Dragon)
			{
				const float LakeZ = Builder ? Builder->SampleGroundZ(-650.f, 1100.f, 120.f) : 120.f;
				const FVector FlyLoc(-650.f, 1100.f, LakeZ + 820.f);
				Dragon->SetActorLocationAndRotation(FlyLoc, FRotator(0.f, -80.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
				Dragon->MountRider(Hero, this);
				SetControlRotation(FRotator(-8.f, -80.f, 0.f));
				Dragon->FireEnergy = Dragon->MaxFireEnergy;
			}
		}
		else
		{
			if (Dragon && Dragon->bMounted)
			{
				Dragon->ApplyFlightInput(0.75f, 0.06f, 0.0f, 0.f, 0.f, true, SafeDT);
			}
			if (CaptureStageElapsed >= 1.00f)
			{
				// IMPORTANT: Never DismountRider on the same tick as SnapFrame — RequestScreenshot renders at frame end!
				SnapFrame(TEXT("03_dragon_ignis_flight.png"));
				CaptureStage = 3;
				bCaptureStageSetupDone = false;
				CaptureStageElapsed = 0.f;
			}
		}
		break;

	case 3: // Stage 3: Highkeep Citadel Open-Air Courtyard facing towering SevarogBloodred Malakor Boss
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			if (Dragon && Dragon->bMounted)
			{
				Dragon->DismountRider(this);
			}
			if (Boss)
			{
				const FVector BossLoc = Boss->GetActorLocation();
				const FVector HeroPos = BossLoc + FVector(0.f, 880.f, 45.f);
				Hero->InvulnTimer = 30.f;
				Hero->SetActorLocationAndRotation(HeroPos, FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
				SetControlRotation(FRotator(-6.f, -90.f, 0.f));
			}
		}
		else if (CaptureStageElapsed >= 1.45f)
		{
			SnapFrame(TEXT("04_highkeep_malakor_boss.png"));
			CaptureStage = 4;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 4: // Stage 4: Ash Caldera & Ember Crypt Moody Gothic Lava Hall (positioned cleanly INSIDE past the entrance gate)
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			const float GZ = Builder ? Builder->SampleGroundZ(9800.f, -2080.f, 120.f) : 120.f;
			Hero->SetActorLocationAndRotation(FVector(9800.f, -2080.f, GZ + 115.f), FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(FRotator(-10.f, -90.f, 0.f));
		}
		else if (CaptureStageElapsed >= 1.35f)
		{
			SnapFrame(TEXT("05_ember_crypt_lava.png"));
			CaptureStage = 5;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 5: // Stage 5: 4km x 4km Fog-of-War World Map Overlay
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UFogOfWarMapSubsystem* MapSub = GI->GetSubsystem<UFogOfWarMapSubsystem>())
				{
					MapSub->RevealAroundWorldLocation(FVector(0.f, 4200.f, 0.f), 42000.f);
					MapSub->RevealAroundWorldLocation(FVector(-2400.f, 1200.f, 0.f), 38000.f);
					MapSub->RevealAroundWorldLocation(FVector(0.f, -9800.f, 0.f), 36000.f);
					MapSub->RevealAroundWorldLocation(FVector(9800.f, -2400.f, 0.f), 32000.f);
				}
			}
			ToggleOverlay(EChroniclesOverlayMode::WorldMap);
		}
		else if (CaptureStageElapsed >= 0.85f)
		{
			SnapFrame(TEXT("06_world_map_overlay.png"));
			CaptureStage = 6;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 6: // Stage 6: Live 3D Hero Customizer Studio Overlay in Sunlit Oakhaven Plaza
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			CloseAllOverlays();
			if (Builder) { Builder->SetTimeOfDay(0.72f); }
			const float GZ = Builder ? Builder->SampleGroundZ(0.f, 3450.f, 120.f) : 120.f;
			Hero->SetActorLocationAndRotation(FVector(0.f, 3450.f, GZ + 105.f), FRotator(0.f, 90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(FRotator(-6.f, 90.f, 0.f));
			CreatorZoomMode = 0;
			CreatorOrbitYaw = 152.f;
			ToggleOverlay(EChroniclesOverlayMode::HeroCustomizer);
		}
		else if (CaptureStageElapsed >= 1.10f)
		{
			SnapFrame(TEXT("07_hero_customizer_studio.png"));
			CaptureStage = 7;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	default: // Stage 7: Flush last PNG to disk then exit cleanly
		if (CaptureStageElapsed >= 0.90f)
		{
			UE_LOG(LogWizardsmoke, Display, TEXT("[CaptureFrames] All 7 critic frames saved cleanly! Exiting."));
			bCaptureFramesActive = false;
			FGenericPlatformMisc::RequestExit(false);
		}
		break;
	}
}

// ============================================================================
// 4A. SFogOfWarMapWidget — 64x64 Fog-of-War & Landmark Slate Leaf Widget
// ============================================================================

void SFogOfWarMapWidget::Construct(const FArguments& InArgs)
{
	OwnerPC = InArgs._OwnerPC;
	DesiredMapSize = FMath::Max(240.f, InArgs._MapSize);
}

FVector2D SFogOfWarMapWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(DesiredMapSize, DesiredMapSize);
}

FLinearColor SFogOfWarMapWidget::GetBiomeMapColor(EChronicleBiome Biome)
{
	switch (Biome)
	{
	case EChronicleBiome::OakhavenValley:  return FLinearColor(0.32f, 0.44f, 0.22f, 0.96f);
	case EChronicleBiome::BlueLeafWoods:   return FLinearColor(0.14f, 0.34f, 0.56f, 0.96f);
	case EChronicleBiome::HighkeepCitadel: return FLinearColor(0.36f, 0.16f, 0.20f, 0.96f);
	case EChronicleBiome::AshCalderaCrypt: return FLinearColor(0.56f, 0.24f, 0.10f, 0.96f);
	case EChronicleBiome::CrystalGlacier:  return FLinearColor(0.42f, 0.68f, 0.84f, 0.96f);
	default:                               return FLinearColor(0.28f, 0.30f, 0.34f, 0.96f);
	}
}

int32 SFogOfWarMapWidget::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FSlateBrush* WhiteBrush = ChroniclesHUDPalette::WhiteBrush();
	const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= 4.f || LocalSize.Y <= 4.f || !WhiteBrush)
	{
		return LayerId;
	}

	// 1. Dark uncharted parchment backdrop
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(LocalSize, FSlateLayoutTransform(FVector2f::ZeroVector)),
		WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(0.025f, 0.030f, 0.048f, 1.0f));

	AChroniclesPlayerController* PC = OwnerPC.Get();
	UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
	const UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;

	if (!MapSub)
	{
		return LayerId + 1;
	}

	constexpr int32 GridDim = UFogOfWarMapSubsystem::GRID_SIZE;
	const float CellW = LocalSize.X / static_cast<float>(GridDim);
	const float CellH = LocalSize.Y / static_cast<float>(GridDim);
	const float Extent = UFogOfWarMapSubsystem::WORLD_EXTENT_UU;
	const float CellWorld = UFogOfWarMapSubsystem::CELL_SIZE_UU;

	// 2. Render 64x64 Fog-of-War territory cells
	const int32 GridLayer = LayerId + 1;
	for (int32 Y = 0; Y < GridDim; ++Y)
	{
		const float WorldY = -Extent + (static_cast<float>(Y) + 0.5f) * CellWorld;
		const float ScreenY = static_cast<float>(Y) * CellH;

		for (int32 X = 0; X < GridDim; ++X)
		{
			const bool bExplored = MapSub->IsCellExplored(X, Y);
			if (!bExplored)
			{
				// Subtle Crimson Desert dark parchment fog with faint cartographic latitude/longitude lines (no dev checkerboard!)
				if ((X % 8) == 0 || (Y % 8) == 0)
				{
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						GridLayer,
						AllottedGeometry.ToPaintGeometry(FVector2f(CellW, CellH), FSlateLayoutTransform(FVector2f(static_cast<float>(X) * CellW, ScreenY))),
						WhiteBrush,
						ESlateDrawEffect::None,
						FLinearColor(0.18f, 0.15f, 0.12f, 0.14f));
				}
				continue;
			}

			const float WorldX = -Extent + (static_cast<float>(X) + 0.5f) * CellWorld;
			const EChronicleBiome Biome = MapSub->GetBiomeAtLocation(FVector(WorldX, WorldY, 0.f));
			const FLinearColor TileColor = GetBiomeMapColor(Biome);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				GridLayer,
				AllottedGeometry.ToPaintGeometry(FVector2f(CellW + 0.5f, CellH + 0.5f), FSlateLayoutTransform(FVector2f(static_cast<float>(X) * CellW, ScreenY))),
				WhiteBrush,
				ESlateDrawEffect::None,
				TileColor);
		}
	}

	// 3. Subtle cartographic meridian crosshairs (N/S & E/W)
	const int32 LineLayer = GridLayer + 1;
	const FLinearColor MeridianColor(0.70f, 0.62f, 0.42f, 0.16f);
	FSlateDrawElement::MakeBox(
		OutDrawElements, LineLayer,
		AllottedGeometry.ToPaintGeometry(FVector2f(LocalSize.X, 1.f), FSlateLayoutTransform(FVector2f(0.f, LocalSize.Y * 0.5f))),
		WhiteBrush, ESlateDrawEffect::None, MeridianColor);
	FSlateDrawElement::MakeBox(
		OutDrawElements, LineLayer,
		AllottedGeometry.ToPaintGeometry(FVector2f(1.f, LocalSize.Y), FSlateLayoutTransform(FVector2f(LocalSize.X * 0.5f, 0.f))),
		WhiteBrush, ESlateDrawEffect::None, MeridianColor);

	// 4. Landmark Pins
	const int32 PinLayer = LineLayer + 1;
	for (const FMapPinInfo& Pin : MapSub->GetMapPins())
	{
		const FVector2D UV = MapSub->WorldToNormalizedUV(Pin.WorldLocation);
		const float PX = static_cast<float>(UV.X) * LocalSize.X;
		const float PY = static_cast<float>(UV.Y) * LocalSize.Y;
		const float PinRadius = Pin.bDiscovered ? 5.0f : 3.0f;
		const FLinearColor PinColor = Pin.bDiscovered
			? Pin.Color
			: FLinearColor(0.55f, 0.56f, 0.62f, 0.38f);

		// Dark outline behind pin for crisp contrast
		FSlateDrawElement::MakeBox(
			OutDrawElements, PinLayer,
			AllottedGeometry.ToPaintGeometry(
				FVector2f((PinRadius + 1.5f) * 2.f, (PinRadius + 1.5f) * 2.f),
				FSlateLayoutTransform(FVector2f(PX - PinRadius - 1.5f, PY - PinRadius - 1.5f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.02f, 0.02f, 0.04f, 0.90f));

		FSlateDrawElement::MakeBox(
			OutDrawElements, PinLayer + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(PinRadius * 2.f, PinRadius * 2.f),
				FSlateLayoutTransform(FVector2f(PX - PinRadius, PY - PinRadius))),
			WhiteBrush, ESlateDrawEffect::None, PinColor);
	}

	// 5. Active Player Marker + Facing Indicator
	if (const APawn* ActivePawn = PC->GetPawn())
	{
		const FVector2D PlayerUV = MapSub->WorldToNormalizedUV(ActivePawn->GetActorLocation());
		const float UX = static_cast<float>(PlayerUV.X) * LocalSize.X;
		const float UY = static_cast<float>(PlayerUV.Y) * LocalSize.Y;
		const int32 PlayerLayer = PinLayer + 2;

		// Outer cyan halo
		FSlateDrawElement::MakeBox(
			OutDrawElements, PlayerLayer,
			AllottedGeometry.ToPaintGeometry(FVector2f(16.f, 16.f), FSlateLayoutTransform(FVector2f(UX - 8.f, UY - 8.f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.0f, 0.96f, 1.0f, 0.35f));

		// Crisp white-cyan core
		FSlateDrawElement::MakeBox(
			OutDrawElements, PlayerLayer + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(9.f, 9.f), FSlateLayoutTransform(FVector2f(UX - 4.5f, UY - 4.5f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.95f, 1.0f, 1.0f, 1.0f));

		// Heading pip offset along actor forward XY
		const FVector Fwd = ActivePawn->GetActorForwardVector().GetSafeNormal2D();
		const float TipX = UX + static_cast<float>(Fwd.X) * 10.f;
		const float TipY = UY + static_cast<float>(Fwd.Y) * 10.f;
		FSlateDrawElement::MakeBox(
			OutDrawElements, PlayerLayer + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(5.f, 5.f), FSlateLayoutTransform(FVector2f(TipX - 2.5f, TipY - 2.5f))),
			WhiteBrush, ESlateDrawEffect::None, ChroniclesHUDPalette::TextGold);
	}

	return PinLayer + 4;
}

// ============================================================================
// 4B. SAetheriaChroniclesHUD — Production Slate HUD & Interactive Overlays
// ============================================================================

void SAetheriaChroniclesHUD::Construct(const FArguments& InArgs)
{
	OwnerPC = InArgs._OwnerPC;

	ChildSlot
	[
		SNew(SOverlay)

		// --- Layer 0: Top-Center Compass & Contextual Boss Health/Stagger Bar ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 16.f, 0.f, 0.f))
		[
			BuildTopCenterCompassAndBossBar()
		]

		// --- Layer 1: Top-Right Gold / Level / Exploration / RTX Tier & Hotkeys ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 16.f, 18.f, 0.f))
		[
			BuildTopRightStatusAndHotkeys()
		]

		// --- Layer 2: Bottom-Left Vitals, 3-Orb Queue, D/F Spells & Dragon Bar ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(20.f, 0.f, 0.f, 20.f))
		[
			BuildBottomLeftVitalsAndInvokerPanel()
		]

		// --- Layer 3: Bottom-Center Interaction Prompt & Toast Notifications ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 28.f))
		[
			BuildBottomCenterPromptsAndToast()
		]

		// --- Layer 4: Fullscreen Fog-of-War World Map [M] ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildWorldMapOverlay()
		]

		// --- Layer 5: 4-Branch Character Skill Tree [K] ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildSkillTreeOverlay()
		]

		// --- Layer 6: Quest Chronicle Log [J] ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildQuestLogOverlay()
		]

		// --- Layer 7: 10-Spell Invoker Grimoire [Tab] ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildGrimoireOverlay()
		]

		// --- Layer 8: NPC Dialogue Overlay [E] ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildDialogueOverlay()
		]

		// --- Layer 9: RTX / DLSS / Streaming Graphics Panel [F10] ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildGraphicsRTXOverlay()
		]

		// --- Layer 10: Edge-Docked Live 3D Hero Customizer Studio [C] ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildHeroCustomizerOverlay()
		]

		// --- Layer 11: Game Over Overlay ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildGameOverOverlay()
		]
	];

	RefreshDynamicPanels();
}

void SAetheriaChroniclesHUD::RefreshDynamicPanels()
{
	RebuildSkillTreeColumns();
	RebuildQuestList();
	RebuildGrimoireGrid();
	RebuildMapLandmarksList();
	RebuildHeroCustomizerPanels();
}

// ============================================================================
// Persistent HUD Regions
// ============================================================================

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildTopCenterCompassAndBossBar()
{
	return SNew(SVerticalBox)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer) ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible;
		})

		// Ornate Compass & Tracked Quest Strip
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::PanelBg)
				.Padding(FMargin(18.f, 7.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(11))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text_Lambda([WeakPC = OwnerPC]() -> FText
						{
							AChroniclesPlayerController* PC = WeakPC.Get();
							if (!PC) return FText::GetEmpty();

							const FString Heading = ChroniclesHUDPalette::CardinalFromYaw(PC->GetControlRotation().Yaw);
							FString BiomeName = TEXT("Golden Oakhaven Valley");
							FString TimeLabel = TEXT("Golden Sunset");

							if (UGameInstance* GI = PC->GetGameInstance())
							{
								if (const UFogOfWarMapSubsystem* MapSub = GI->GetSubsystem<UFogOfWarMapSubsystem>())
								{
									const FVector Loc = PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
									BiomeName = MapSub->GetBiomeDisplayName(MapSub->GetBiomeAtLocation(Loc)).ToString();
								}
							}
							if (const AAetheriaWorldBuilder* Builder = PC->FindWorldBuilder())
							{
								TimeLabel = Builder->GetTimeOfDayLabel();
							}

							return FText::FromString(FString::Printf(TEXT("%s   ·   %s   ·   %s"),
								*Heading, *BiomeName, *TimeLabel));
						})
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(FMargin(0.f, 2.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontRegular(10))
						.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
						.Text_Lambda([WeakPC = OwnerPC]() -> FText
						{
							AChroniclesPlayerController* PC = WeakPC.Get();
							UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
							const UQuestSubsystem* QuestSub = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;
							const FQuestDef* Tracked = QuestSub ? QuestSub->GetTrackedQuest() : nullptr;
							if (!Tracked)
							{
								return LOCTEXT("AllQuestsDone", "All Story Chronicles Completed — Explore Aetheria freely");
							}

							int32 DistMeters = 0;
							if (const APawn* P = PC->GetPawn())
							{
								DistMeters = FMath::RoundToInt(FVector::Dist2D(P->GetActorLocation(), Tracked->ObjectiveWorldLocation) / 100.f);
							}

							return FText::FromString(FString::Printf(TEXT("Tracked: %s (%d/%d) — %dm"),
								*Tracked->Title.ToString(), Tracked->Progress, Tracked->TargetCount, DistMeters));
						})
					]
				]
			]
		]

		// Contextual Multi-Phase Boss Bar (Malakor · Phase I/II/III, HP, StaggerGauge & Flashing Crimson Telegraph)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(FMargin(0.f, 12.f, 0.f, 0.f))
		[
			SNew(SBox)
			.WidthOverride(640.f)
			.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
				return (Boss && Boss->bBossEngaged && !Boss->bDead) ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
					if (Boss && Boss->bIsTelegraphing)
					{
						return Boss->IsCurrentAttackParryable()
							? FSlateColor(FLinearColor(1.00f, 0.702f, 0.278f, 0.90f))
							: FSlateColor(FLinearColor(1.00f, 0.165f, 0.165f, 0.90f));
					}
					return FSlateColor(ChroniclesHUDPalette::HairlineGold);
				})
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
					.Padding(FMargin(16.f, 10.f))
					[
						SNew(SVerticalBox)

						// Title & Phase Header
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(13))
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
									if (!Boss) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("%s  ·  %s"),
										*Boss->BossTitle, *ChroniclesHUDPalette::RomanPhase(Boss->BossPhase)));
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(11))
								.ColorAndOpacity(ChroniclesHUDPalette::TextCrimson)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
									if (!Boss) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Boss->HP, Boss->MaxHP));
								})
							]
						]

						// Boss HP Bar
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 6.f, 0.f, 4.f))
						[
							SNew(SBox)
							.HeightOverride(14.f)
							[
								SNew(SProgressBar)
								.BackgroundImage(ChroniclesHUDPalette::WhiteBrush())
								.FillImage(ChroniclesHUDPalette::WhiteBrush())
								.FillColorAndOpacity(ChroniclesHUDPalette::BarHP)
								.Percent_Lambda([WeakPC = OwnerPC]() -> TOptional<float>
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
									return (Boss && Boss->MaxHP > 0.f) ? FMath::Clamp(Boss->HP / Boss->MaxHP, 0.f, 1.f) : 0.f;
								})
							]
						]

						// Boss StaggerGauge Bar
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(9))
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
									return (Boss && Boss->bStaggerBroken)
										? LOCTEXT("StaggerBrokenLbl", "STAGGER BROKEN (+35% DMG)")
										: LOCTEXT("StaggerLbl", "POISE / STAGGER");
								})
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(SBox)
								.HeightOverride(7.f)
								[
									SNew(SProgressBar)
									.BackgroundImage(ChroniclesHUDPalette::WhiteBrush())
									.FillImage(ChroniclesHUDPalette::WhiteBrush())
									.FillColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
									{
										AChroniclesPlayerController* PC = WeakPC.Get();
										const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
										return (Boss && Boss->bStaggerBroken)
											? FSlateColor(ChroniclesHUDPalette::TextCyan)
											: FSlateColor(ChroniclesHUDPalette::BarStagger);
									})
									.Percent_Lambda([WeakPC = OwnerPC]() -> TOptional<float>
									{
										AChroniclesPlayerController* PC = WeakPC.Get();
										const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
										return (Boss && Boss->MaxStagger > 0.f) ? FMath::Clamp(Boss->StaggerGauge / Boss->MaxStagger, 0.f, 1.f) : 0.f;
									})
								]
							]
						]

						// Wukong Amber (#FFB347 Parryable) / Crimson (#FF2A2A Unblockable) Telegraph Warning Strip
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
								const float T = (PC && PC->GetWorld()) ? PC->GetWorld()->GetTimeSeconds() : 0.f;
								const float Pulse = 0.68f + 0.32f * FMath::Abs(FMath::Sin(T * 10.f));
								if (Boss && Boss->IsCurrentAttackParryable())
								{
									// Amber #FFB347
									return FSlateColor(FLinearColor(1.00f * Pulse, 0.702f * Pulse, 0.278f * Pulse, 0.95f));
								}
								// Crimson #FF2A2A
								return FSlateColor(FLinearColor(1.00f * Pulse, 0.165f * Pulse, 0.165f * Pulse, 0.95f));
							})
							.Padding(FMargin(14.f, 3.f))
							.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
								return (Boss && Boss->bIsTelegraphing) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(11))
								.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
									return (Boss && Boss->IsCurrentAttackParryable())
										? FSlateColor(FLinearColor(0.05f, 0.03f, 0.01f, 1.0f))
										: FSlateColor(ChroniclesHUDPalette::TextPrimary);
								})
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
									if (!Boss) return FText::GetEmpty();
									const FString AttackName = GetBossAttackWarningText(Boss->CurrentAttack);
									if (Boss->IsCurrentAttackParryable())
									{
										return FText::FromString(FString::Printf(
											TEXT("[AMBER · PARRY RMB] INCOMING: %s (+2.5x STAGGER)"),
											*AttackName));
									}
									return FText::FromString(FString::Printf(
										TEXT("[CRIMSON · DODGE Q/Ctrl] INCOMING: %s (UNBLOCKABLE)"),
										*AttackName));
								})
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildTopRightStatusAndHotkeys()
{
	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
		.Padding(FMargin(1.f))
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer) ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible;
		})
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::PanelBg)
			.Padding(FMargin(14.f, 10.f))
			[
				SNew(SVerticalBox)

				// Top Row: Level, Gold, Exploration %, Co-op Players
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(11))
					.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
					.Text_Lambda([WeakPC = OwnerPC]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
						const USkillTreeComponent* Skills = Hero ? Hero->SkillTree.Get() : nullptr;
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;
						const AChroniclesGameState* GS = (PC && PC->GetWorld()) ? PC->GetWorld()->GetGameState<AChroniclesGameState>() : nullptr;

						const int32 Lv = Skills ? Skills->Level : 1;
						const int32 Gold = Skills ? Skills->Gold : 0;
						const int32 Pts = Skills ? Skills->UnspentSkillPoints : 0;
						const float Explored = MapSub ? MapSub->GetTotalExplorationPercent() : 0.f;
						const int32 Players = GS ? GS->ConnectedPlayers : 1;

						return FText::FromString(FString::Printf(
							TEXT("LV %d  |  %d G  |  SP: %d  |  MAP %.1f%%%s"),
							Lv, Gold, Pts, Explored,
							(Players > 1) ? *FString::Printf(TEXT("  |  CO-OP %d"), Players) : TEXT("")));
					})
				]

				// Active Race + Stature Badge
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 4.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(9))
					.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
						const UHeroCustomizationComponent* Custom = Hero ? Hero->Customization.Get() : nullptr;
						if (!Custom)
						{
							return FSlateColor(ChroniclesHUDPalette::TextCyan);
						}
						const float H = Custom->Profile.HeightMeters;
						if (H < UHeroCustomizationComponent::SMALL_NO_MAGIC_MAX)
						{
							return FSlateColor(ChroniclesHUDPalette::TextCrimson);
						}
						if (H >= UHeroCustomizationComponent::GIANT_TITAN_MIN)
						{
							return FSlateColor(ChroniclesHUDPalette::TextGold);
						}
						return FSlateColor(ChroniclesHUDPalette::TextCyan);
					})
					.Text_Lambda([WeakPC = OwnerPC]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
						const UHeroCustomizationComponent* Custom = Hero ? Hero->Customization.Get() : nullptr;
						if (!Custom)
						{
							return LOCTEXT("DefaultRaceStature", "Human · 1.85m");
						}
						const float H = Custom->Profile.HeightMeters;
						const TCHAR* Tag = (H < UHeroCustomizationComponent::SMALL_NO_MAGIC_MAX)
							? TEXT(" NO MAGIC")
							: (H >= UHeroCustomizationComponent::GIANT_TITAN_MIN ? TEXT(" GIANT") : TEXT(""));
						return FText::FromString(FString::Printf(TEXT("%s · %.2fm%s"),
							*UHeroCustomizationComponent::GetRaceLabel(Custom->Profile.Race), H, Tag));
					})
				]

				// GPU RTX / Standard Tier Badge
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 4.f, 0.f, 6.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(9))
					.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
						return (RTXSub && RTXSub->IsRTXActive())
							? FSlateColor(ChroniclesHUDPalette::TextCyan)
							: FSlateColor(ChroniclesHUDPalette::TextGold);
					})
					.Text_Lambda([WeakPC = OwnerPC]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
						if (!RTXSub)
						{
							return LOCTEXT("RTXBadgeUnknown", "RENDERER: INITIALIZING");
						}
						const FGraphicsTelemetry& Tel = RTXSub->GetTelemetry();
						const TCHAR* TierName = (Tel.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
							? TEXT("CRIMSON RTX ULTRA")
							: TEXT("STANDARD PERF");
						const TCHAR* DLSSStr = Tel.bDLSSActive ? TEXT(" + DLSS") : TEXT(" + TSR");
						return FText::FromString(FString::Printf(TEXT("%s%s  (%dMB Pool)"), TierName, DLSSStr, Tel.TexturePoolMB));
					})
				]

				// Compact Hotkey Bar
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontRegular(9))
					.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
					.Text(LOCTEXT("HotkeyHint", "[M] Map  [K] Skills  [C] Hero  [J] Quests  [Tab] Grimoire  [T] Time  [F10] RTX"))
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildResourceBarRow(
	const FText& LabelText,
	const FLinearColor& FillColor,
	TAttribute<TOptional<float>> PercentAttr,
	TAttribute<FText> ValueTextAttr) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(42.f)
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(9))
				.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
				.Text(LabelText)
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.Padding(FMargin(4.f, 0.f, 8.f, 0.f))
		[
			SNew(SBox)
			.HeightOverride(10.f)
			[
				SNew(SProgressBar)
				.BackgroundImage(ChroniclesHUDPalette::WhiteBrush())
				.FillImage(ChroniclesHUDPalette::WhiteBrush())
				.FillColorAndOpacity(FillColor)
				.Percent(PercentAttr)
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(74.f)
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontRegular(9))
				.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
				.Justification(ETextJustify::Right)
				.Text(ValueTextAttr)
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildOrbBadgeWidget(EChronicleOrb Orb, const FString& KeyHint) const
{
	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor_Lambda([WeakPC = OwnerPC, Orb]() -> FSlateColor
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
			const bool bUnlocked = (Hero && Hero->SpellSystem && Hero->SpellSystem->IsOrbUnlocked(Orb));
			return bUnlocked ? FSlateColor(GetOrbColor(Orb)) : FSlateColor(ChroniclesHUDPalette::OrbLocked);
		})
		.Padding(FMargin(1.5f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
			.Padding(FMargin(10.f, 5.f))
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(10))
				.ColorAndOpacity_Lambda([WeakPC = OwnerPC, Orb]() -> FSlateColor
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
					const bool bUnlocked = (Hero && Hero->SpellSystem && Hero->SpellSystem->IsOrbUnlocked(Orb));
					return bUnlocked ? FSlateColor(GetOrbColor(Orb)) : FSlateColor(ChroniclesHUDPalette::TextSecondary);
				})
				.Text_Lambda([WeakPC = OwnerPC, Orb, KeyHint]() -> FText
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
					const bool bUnlocked = (Hero && Hero->SpellSystem && Hero->SpellSystem->IsOrbUnlocked(Orb));
					return FText::FromString(bUnlocked
						? KeyHint
						: FString::Printf(TEXT("%s [LOCKED]"), *KeyHint));
				})
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildSpellSlotWidget(bool bIsSlotF) const
{
	const FString KeyBadge = bIsSlotF ? TEXT("[5/Z] SLOT F") : TEXT("[4/X] SLOT D");
	return SNew(SBox)
		.WidthOverride(176.f)
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor_Lambda([WeakPC = OwnerPC, bIsSlotF]() -> FSlateColor
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
				const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
				const EChronicleSpellId Id = Spells ? (bIsSlotF ? Spells->SlotF : Spells->SlotD) : EChronicleSpellId::None;
				if (Id == EChronicleSpellId::None)
				{
					return FSlateColor(ChroniclesHUDPalette::OrbLocked);
				}
				return FSlateColor(USpellSystemComponent::GetSpellDef(Id).PrimaryColor);
			})
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::CardBg)
				.Padding(FMargin(8.f, 6.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(8))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text(FText::FromString(KeyBadge))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 2.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(10))
						.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
						.Text_Lambda([WeakPC = OwnerPC, bIsSlotF]() -> FText
						{
							AChroniclesPlayerController* PC = WeakPC.Get();
							const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
							const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
							const EChronicleSpellId Id = Spells ? (bIsSlotF ? Spells->SlotF : Spells->SlotD) : EChronicleSpellId::None;
							if (Id == EChronicleSpellId::None)
							{
								return LOCTEXT("EmptySpellSlot", "Empty — Press [R]");
							}
							const FChronicleSpellDef Def = USpellSystemComponent::GetSpellDef(Id);
							const float CD = Spells->GetSpellCooldownRemaining(Id);
							if (CD > 0.05f)
							{
								return FText::FromString(FString::Printf(TEXT("%s (%.1fs)"), *Def.DisplayName.ToString(), CD));
							}
							return Def.DisplayName;
						})
					]
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildBottomLeftVitalsAndInvokerPanel()
{
	return SNew(SVerticalBox)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer) ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible;
		})

		// Dragon Fire Breath Energy Bar (visible ONLY when riding Ignis!)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
		[
			SNew(SBox)
			.WidthOverride(372.f)
			.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				return (PC && PC->IsRidingDragon()) ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::BarDragonFire)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
					.Padding(FMargin(12.f, 8.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							BuildResourceBarRow(
								LOCTEXT("FireLbl", "FIRE"),
								ChroniclesHUDPalette::BarDragonFire,
								TAttribute<TOptional<float>>::CreateLambda([WeakPC = OwnerPC]() -> TOptional<float>
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaDragonActor* Drag = PC ? PC->GetControlledDragon() : nullptr;
									return (Drag && Drag->MaxFireEnergy > 0.f) ? FMath::Clamp(Drag->FireEnergy / Drag->MaxFireEnergy, 0.f, 1.f) : 0.f;
								}),
								TAttribute<FText>::CreateLambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaDragonActor* Drag = PC ? PC->GetControlledDragon() : nullptr;
									return Drag
										? FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Drag->FireEnergy, Drag->MaxFireEnergy))
										: FText::GetEmpty();
								}))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 4.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontRegular(9))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text(LOCTEXT("DragonFlightHint", "IGNIS: [WASD] Steer · [Space/Shift] Climb/Dive · [LMB] Fire Breath · [F] Land"))
						]
					]
				]
			]
		]

		// Hero Vitals + Invoker Spheres & Invoked Slots Card
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.WidthOverride(372.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::PanelBg)
					.Padding(FMargin(12.f, 10.f))
					[
						SNew(SVerticalBox)

						// HP
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
						[
							BuildResourceBarRow(
								LOCTEXT("HPLbl", "HP"),
								ChroniclesHUDPalette::BarHP,
								TAttribute<TOptional<float>>::CreateLambda([WeakPC = OwnerPC]() -> TOptional<float>
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									return (Hero && Hero->MaxHP > 0.f) ? FMath::Clamp(Hero->HP / Hero->MaxHP, 0.f, 1.f) : 0.f;
								}),
								TAttribute<FText>::CreateLambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									return Hero ? FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Hero->HP, Hero->MaxHP)) : LOCTEXT("ZeroVal", "0 / 0");
								}))
						]

						// Mana
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
						[
							BuildResourceBarRow(
								LOCTEXT("MPLbl", "MANA"),
								ChroniclesHUDPalette::BarMana,
								TAttribute<TOptional<float>>::CreateLambda([WeakPC = OwnerPC]() -> TOptional<float>
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
									return (Spells && Spells->MaxMana > 0.f) ? FMath::Clamp(Spells->Mana / Spells->MaxMana, 0.f, 1.f) : 0.f;
								}),
								TAttribute<FText>::CreateLambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
									return Spells ? FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Spells->Mana, Spells->MaxMana)) : LOCTEXT("ZeroVal", "0 / 0");
								}))
						]

						// Stamina
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
						[
							BuildResourceBarRow(
								LOCTEXT("STALbl", "STAM"),
								ChroniclesHUDPalette::BarStamina,
								TAttribute<TOptional<float>>::CreateLambda([WeakPC = OwnerPC]() -> TOptional<float>
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									return (Hero && Hero->MaxStamina > 0.f) ? FMath::Clamp(Hero->Stamina / Hero->MaxStamina, 0.f, 1.f) : 0.f;
								}),
								TAttribute<FText>::CreateLambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									return Hero ? FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Hero->Stamina, Hero->MaxStamina)) : LOCTEXT("ZeroVal", "0 / 0");
								}))
						]

						// XP
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
						[
							BuildResourceBarRow(
								LOCTEXT("XPLbl", "XP"),
								ChroniclesHUDPalette::BarXP,
								TAttribute<TOptional<float>>::CreateLambda([WeakPC = OwnerPC]() -> TOptional<float>
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									const USkillTreeComponent* Skills = Hero ? Hero->SkillTree.Get() : nullptr;
									const int32 Need = Skills ? FMath::Max(1, Skills->GetXPForNextLevel()) : 120;
									return Skills ? FMath::Clamp(static_cast<float>(Skills->CurrentXP) / static_cast<float>(Need), 0.f, 1.f) : 0.f;
								}),
								TAttribute<FText>::CreateLambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									const USkillTreeComponent* Skills = Hero ? Hero->SkillTree.Get() : nullptr;
									return Skills ? FText::FromString(FString::Printf(TEXT("%d / %d"), Skills->CurrentXP, Skills->GetXPForNextLevel())) : LOCTEXT("ZeroVal", "0 / 120");
								}))
						]

						// Primal Sphere Unlock Badges (1:Exort / 2:Wex / 3:Quas)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 4.f, 0.f))
							[ BuildOrbBadgeWidget(EChronicleOrb::Exort, TEXT("[1] Exort")) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(2.f, 0.f, 2.f, 0.f))
							[ BuildOrbBadgeWidget(EChronicleOrb::Wex, TEXT("[2] Wex")) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(4.f, 0.f, 0.f, 0.f))
							[ BuildOrbBadgeWidget(EChronicleOrb::Quas, TEXT("[3] Quas")) ]
						]

						// Active 3-Orb FIFO Queue & Invoke Preview
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(10))
							.ColorAndOpacity(ChroniclesHUDPalette::TextCyan)
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
								const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
								if (!Spells) return LOCTEXT("NoOrbs", "ORBS: [ - - - ]");

								const FString OrbsStr = Spells->GetActiveOrbsString();
								const EChronicleSpellId Preview = Spells->ResolveCurrentCombo();
								if (Preview != EChronicleSpellId::None)
								{
									const FChronicleSpellDef Def = USpellSystemComponent::GetSpellDef(Preview);
									return FText::FromString(FString::Printf(TEXT("ORBS: [ %s ]  ->  [R] %s"),
										*OrbsStr, *Def.DisplayName.ToString()));
								}
								return FText::FromString(FString::Printf(TEXT("ORBS: [ %s ]  (%d/10 Spells Unlocked)"),
									*OrbsStr, Spells->GetUnlockedSpellCount()));
							})
						]

						// Invoked Spell Slots [4/X] D & [5/Z] F
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
							[
								BuildSpellSlotWidget(false)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							[
								BuildSpellSlotWidget(true)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildBottomCenterPromptsAndToast()
{
	return SNew(SVerticalBox)

		// Toast Banner Notification
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				const float Alpha = PC ? PC->GetActiveToastAlpha() : 0.f;
				return FSlateColor(FLinearColor(0.72f, 0.58f, 0.34f, 0.65f * Alpha));
			})
			.Padding(FMargin(1.f))
			.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				return (PC && PC->GetActiveToastAlpha() > 0.01f) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const float Alpha = PC ? PC->GetActiveToastAlpha() : 0.f;
					return FSlateColor(FLinearColor(0.03f, 0.035f, 0.055f, 0.92f * Alpha));
				})
				.Padding(FMargin(20.f, 8.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(11))
					.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						const float Alpha = PC ? PC->GetActiveToastAlpha() : 0.f;
						return FSlateColor(FLinearColor(0.98f, 0.88f, 0.58f, Alpha));
					})
					.Text_Lambda([WeakPC = OwnerPC]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						return PC ? FText::FromString(PC->GetActiveToastMessage()) : FText::GetEmpty();
					})
				]
			]
		]

		// Contextual Interaction Prompt ([E] Talk / [F] Mount Ignis / Click to Resume)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::HairlineCyan)
			.Padding(FMargin(1.f))
			.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				if (!PC || PC->IsAnyModalOpen())
				{
					return EVisibility::Collapsed;
				}
				if (!PC->bMouseCaptured)
				{
					return EVisibility::HitTestInvisible;
				}
				if (PC->IsRidingDragon())
				{
					return EVisibility::Collapsed;
				}
				if (PC->FindNearestInteractableNPC(420.f) != nullptr || PC->FindNearestDragon(950.f) != nullptr)
				{
					return EVisibility::HitTestInvisible;
				}
				return EVisibility::Collapsed;
			})
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
				.Padding(FMargin(16.f, 7.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(11))
					.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
					.Text_Lambda([WeakPC = OwnerPC]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						if (!PC) return FText::GetEmpty();

						if (!PC->bMouseCaptured)
						{
							return LOCTEXT("CursorUnlockedPrompt", "Cursor Unlocked — Click Viewport or Press [Esc] to Resume Combat");
						}
						if (const AAetheriaNPCActor* NPC = PC->FindNearestInteractableNPC(420.f))
						{
							return FText::FromString(FString::Printf(TEXT("[E] Speak to %s (%s)"),
								*NPC->DisplayName.ToString(), *NPC->RoleTitle.ToString()));
						}
						if (PC->FindNearestDragon(950.f) != nullptr)
						{
							const AAetheriaHeroCharacter* Hero = PC->GetActiveHero();
							if (Hero && Hero->bDragonUnlocked)
							{
								return LOCTEXT("MountDragonPrompt", "[F] Mount Ignis the Red Dragon");
							}
							return LOCTEXT("DragonLockedPrompt", "Ignis the Red Dragon — Complete 'Taming the Crimson Wing' (Scholar Lyra) to Mount");
						}
						return FText::GetEmpty();
					})
				]
			]
		];
}

// ============================================================================
// Interactive Modal Overlays
// ============================================================================

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildWorldMapOverlay()
{
	auto BuildBiomePctRow = [this](EChronicleBiome Biome, const FLinearColor& SwatchColor) -> TSharedRef<SWidget>
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
				[
					SNew(SBox).WidthOverride(10.f).HeightOverride(10.f)
					[
						SNew(SBorder)
						.BorderImage(ChroniclesHUDPalette::WhiteBrush())
						.BorderBackgroundColor(SwatchColor)
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontRegular(10))
					.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
					.Text_Lambda([WeakPC = OwnerPC, Biome]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;
						return MapSub ? MapSub->GetBiomeDisplayName(Biome) : FText::GetEmpty();
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(10))
					.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
					.Text_Lambda([WeakPC = OwnerPC, Biome]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;
						const float Pct = MapSub ? MapSub->GetBiomeExplorationPercent(Biome) : 0.f;
						return FText::FromString(FString::Printf(TEXT("%.1f%%"), Pct));
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 3.f, 0.f, 8.f))
			[
				SNew(SBox).HeightOverride(6.f)
				[
					SNew(SProgressBar)
					.BackgroundImage(ChroniclesHUDPalette::WhiteBrush())
					.FillImage(ChroniclesHUDPalette::WhiteBrush())
					.FillColorAndOpacity(SwatchColor)
					.Percent_Lambda([WeakPC = OwnerPC, Biome]() -> TOptional<float>
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;
						return MapSub ? FMath::Clamp(MapSub->GetBiomeExplorationPercent(Biome) / 100.f, 0.f, 1.f) : 0.f;
					})
				]
			];
	};

	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(ChroniclesHUDPalette::ModalScrim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::WorldMap) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
				.Padding(FMargin(20.f))
				[
					SNew(SVerticalBox)

					// Header Row
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 14.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(16))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
								const UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;
								const float Total = MapSub ? MapSub->GetTotalExplorationPercent() : 0.f;
								return FText::FromString(FString::Printf(
									TEXT("ATLAS OF AETHERIA  |  4km x 4km FOG OF WAR  (%.1f%% EXPLORED)"), Total));
							})
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.ContentPadding(FMargin(12.f, 4.f))
							.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(10))
								.Text(LOCTEXT("CloseBtn", "Close [M / Esc]"))
							]
						]
					]

					// Main Map + Side Breakdown
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						// 64x64 Live Fog-of-War Slate Canvas
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(0.f, 0.f, 18.f, 0.f))
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
							.Padding(FMargin(1.5f))
							[
								SNew(SFogOfWarMapWidget)
								.OwnerPC(OwnerPC)
								.MapSize(520.f)
							]
						]

						// Right Column: 5 Biomes % + Discovered Landmarks
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(330.f)
							.HeightOverride(523.f)
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(11))
									.ColorAndOpacity(ChroniclesHUDPalette::TextCyan)
									.Text(LOCTEXT("BiomeBreakdownHdr", "BIOME EXPLORATION"))
								]

								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBiomePctRow(EChronicleBiome::OakhavenValley,  FLinearColor(0.45f, 0.65f, 0.30f, 1.f)) ]
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBiomePctRow(EChronicleBiome::BlueLeafWoods,   FLinearColor(0.24f, 0.56f, 0.92f, 1.f)) ]
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBiomePctRow(EChronicleBiome::HighkeepCitadel, FLinearColor(0.84f, 0.28f, 0.32f, 1.f)) ]
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBiomePctRow(EChronicleBiome::AshCalderaCrypt, FLinearColor(0.95f, 0.44f, 0.14f, 1.f)) ]
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBiomePctRow(EChronicleBiome::CrystalGlacier,  FLinearColor(0.54f, 0.86f, 0.98f, 1.f)) ]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 10.f, 0.f, 6.f))
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(11))
									.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
									.Text(LOCTEXT("LandmarksHdr", "LANDMARKS & FAST TRAVEL"))
								]

								+ SVerticalBox::Slot()
								.FillHeight(1.f)
								[
									SNew(SScrollBox)
									+ SScrollBox::Slot()
									[
										SAssignNew(MapLandmarksBox, SVerticalBox)
									]
								]
							]
						]
					]
				]
			]
		];
}

void SAetheriaChroniclesHUD::RebuildMapLandmarksList()
{
	if (!MapLandmarksBox.IsValid())
	{
		return;
	}
	MapLandmarksBox->ClearChildren();

	AChroniclesPlayerController* PC = OwnerPC.Get();
	UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
	const UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;
	if (!MapSub)
	{
		return;
	}

	for (const FMapPinInfo& Pin : MapSub->GetMapPins())
	{
		const FName PinId = Pin.PinId;
		MapLandmarksBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(Pin.bDiscovered ? ChroniclesHUDPalette::CardBg : ChroniclesHUDPalette::CardBgLocked)
			.Padding(FMargin(8.f, 5.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontRegular(10))
					.ColorAndOpacity(Pin.bDiscovered ? ChroniclesHUDPalette::TextPrimary : ChroniclesHUDPalette::TextSecondary)
					.Text(Pin.bDiscovered
						? Pin.Label
						: LOCTEXT("UnchartedLandmark", "??? (Uncharted Landmark)"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Visibility((Pin.bDiscovered && Pin.bFastTravel) ? EVisibility::Visible : EVisibility::Collapsed)
					.ContentPadding(FMargin(6.f, 2.f))
					.OnClicked_Lambda([WeakPC = OwnerPC, PinId]() -> FReply
					{
						if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
						{
							StrongPC->RequestFastTravelToPin(PinId);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(8))
						.Text(LOCTEXT("TravelBtn", "Travel"))
					]
				]
			]
		];
	}
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildSkillTreeOverlay()
{
	static const TCHAR* BranchTitles[4] = {
		TEXT("PATH OF EXORT"),
		TEXT("PATH OF WEX"),
		TEXT("PATH OF QUAS"),
		TEXT("BLADE & DRAKE")
	};
	static const FLinearColor BranchColors[4] = {
		ChroniclesHUDPalette::OrbExort,
		ChroniclesHUDPalette::OrbWex,
		ChroniclesHUDPalette::OrbQuas,
		ChroniclesHUDPalette::TextGold
	};

	TSharedRef<SHorizontalBox> ColumnsBox = SNew(SHorizontalBox);
	for (int32 i = 0; i < 4; ++i)
	{
		ColumnsBox->AddSlot()
		.FillWidth(1.f)
		.Padding(FMargin(i == 0 ? 0.f : 8.f, 0.f, 0.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(FLinearColor(BranchColors[i].R, BranchColors[i].G, BranchColors[i].B, 0.45f))
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::PanelBg)
				.Padding(FMargin(10.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(11))
						.ColorAndOpacity(BranchColors[i])
						.Text(FText::FromString(BranchTitles[i]))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(SkillBranchBoxes[i], SVerticalBox)
						]
					]
				]
			]
		];
	}

	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(ChroniclesHUDPalette::ModalScrim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::SkillTree) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.WidthOverride(1120.f)
			.HeightOverride(660.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
					.Padding(FMargin(20.f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(16))
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									const USkillTreeComponent* Skills = Hero ? Hero->SkillTree.Get() : nullptr;
									const int32 Lv = Skills ? Skills->Level : 1;
									const int32 SP = Skills ? Skills->UnspentSkillPoints : 0;
									const int32 Invested = Skills ? Skills->GetTotalInvestedRanks() : 0;
									return FText::FromString(FString::Printf(
										TEXT("ARCHMAGE SKILL TREE  ·  LEVEL %d  ·  UNSPENT SKILL POINTS: %d  (%d Invested)"),
										Lv, SP, Invested));
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.ContentPadding(FMargin(12.f, 4.f))
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.Text(LOCTEXT("CloseSkillsBtn", "Close [K / Esc]"))
								]
							]
						]

						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							ColumnsBox
						]
					]
				]
			]
		];
}

void SAetheriaChroniclesHUD::RebuildSkillTreeColumns()
{
	AChroniclesPlayerController* PC = OwnerPC.Get();
	const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
	const USkillTreeComponent* Skills = Hero ? Hero->SkillTree.Get() : nullptr;

	static const ESkillBranch Branches[4] = {
		ESkillBranch::ExortPath,
		ESkillBranch::WexPath,
		ESkillBranch::QuasPath,
		ESkillBranch::BladeAndDrake
	};

	for (int32 b = 0; b < 4; ++b)
	{
		if (!SkillBranchBoxes[b].IsValid())
		{
			continue;
		}
		SkillBranchBoxes[b]->ClearChildren();

		if (!Skills)
		{
			continue;
		}

		const TArray<FSkillNodeDef> BranchNodes = Skills->GetNodesForBranch(Branches[b]);
		for (const FSkillNodeDef& Node : BranchNodes)
		{
			const FName NodeId = Node.NodeId;
			const int32 MaxRank = Node.MaxRank;
			const int32 Tier = Node.Tier;

			SkillBranchBoxes[b]->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::CardBg)
				.Padding(FMargin(8.f, 7.f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(10))
							.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
							.Text(FText::FromString(FString::Printf(TEXT("T%d · %s"), Tier, *Node.Title.ToString())))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(10))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text_Lambda([WeakPC = OwnerPC, NodeId, MaxRank]() -> FText
							{
								AChroniclesPlayerController* StrongPC = WeakPC.Get();
								const AAetheriaHeroCharacter* H = StrongPC ? StrongPC->GetActiveHero() : nullptr;
								const int32 Rank = (H && H->SkillTree) ? H->SkillTree->GetNodeRank(NodeId) : 0;
								return FText::FromString(FString::Printf(TEXT("%d / %d"), Rank, MaxRank));
							})
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 3.f, 0.f, 6.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontRegular(8))
						.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
						.AutoWrapText(true)
						.Text(Node.Description)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					[
						SNew(SButton)
						.ContentPadding(FMargin(8.f, 3.f))
						.IsEnabled_Lambda([WeakPC = OwnerPC, NodeId]() -> bool
						{
							AChroniclesPlayerController* StrongPC = WeakPC.Get();
							const AAetheriaHeroCharacter* H = StrongPC ? StrongPC->GetActiveHero() : nullptr;
							return H && H->SkillTree && H->SkillTree->CanUpgradeNode(NodeId);
						})
						.OnClicked_Lambda([WeakPC = OwnerPC, NodeId]() -> FReply
						{
							if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
							{
								if (AAetheriaHeroCharacter* H = StrongPC->GetActiveHero())
								{
									if (H->SkillTree && H->SkillTree->UpgradeNode(NodeId))
									{
										H->SkillTree->SaveProgress();
										StrongPC->PushToastMessage(FString::Printf(TEXT("Skill Upgraded: %s"), *NodeId.ToString()), 2.4f);
									}
								}
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(9))
							.Text_Lambda([WeakPC = OwnerPC, NodeId, MaxRank]() -> FText
							{
								AChroniclesPlayerController* StrongPC = WeakPC.Get();
								const AAetheriaHeroCharacter* H = StrongPC ? StrongPC->GetActiveHero() : nullptr;
								const int32 Rank = (H && H->SkillTree) ? H->SkillTree->GetNodeRank(NodeId) : 0;
								return (Rank >= MaxRank) ? LOCTEXT("SkillMaxed", "MAXED") : LOCTEXT("SkillUpgrade", "+ Upgrade");
							})
						]
					]
				]
			];
		}
	}
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildQuestLogOverlay()
{
	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(ChroniclesHUDPalette::ModalScrim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::QuestLog) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.WidthOverride(860.f)
			.HeightOverride(620.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
					.Padding(FMargin(20.f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(16))
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
								.Text(LOCTEXT("QuestChronicleHdr", "CHRONICLE OF AETHERIA  ·  6 STORY QUESTS"))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.ContentPadding(FMargin(12.f, 4.f))
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.Text(LOCTEXT("CloseQuestsBtn", "Close [J / Esc]"))
								]
							]
						]

						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(QuestListBox, SVerticalBox)
							]
						]
					]
				]
			]
		];
}

void SAetheriaChroniclesHUD::RebuildQuestList()
{
	if (!QuestListBox.IsValid())
	{
		return;
	}
	QuestListBox->ClearChildren();

	AChroniclesPlayerController* PC = OwnerPC.Get();
	UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
	const UQuestSubsystem* QuestSub = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;
	if (!QuestSub)
	{
		return;
	}

	for (const FQuestDef& Q : QuestSub->GetAllQuests())
	{
		const FName QId = Q.QuestId;

		FString UnlockBadge;
		FLinearColor BadgeColor = ChroniclesHUDPalette::TextGold;
		if (Q.UnlockOrbReward == EChronicleOrb::Exort)      { UnlockBadge = TEXT("UNLOCKS EXORT");  BadgeColor = ChroniclesHUDPalette::OrbExort; }
		else if (Q.UnlockOrbReward == EChronicleOrb::Wex)   { UnlockBadge = TEXT("UNLOCKS WEX");    BadgeColor = ChroniclesHUDPalette::OrbWex; }
		else if (Q.UnlockOrbReward == EChronicleOrb::Quas)  { UnlockBadge = TEXT("UNLOCKS QUAS");   BadgeColor = ChroniclesHUDPalette::OrbQuas; }
		else if (Q.bUnlocksDragonMount)                     { UnlockBadge = TEXT("UNLOCKS DRAGON"); BadgeColor = ChroniclesHUDPalette::BarDragonFire; }

		QuestListBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::CardBg)
			.Padding(FMargin(14.f, 10.f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(12))
						.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
						.Text(FText::FromString(FString::Printf(TEXT("%s  —  %s"),
							*Q.Title.ToString(), *Q.GiverName.ToString())))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.f, 0.f, 10.f, 0.f))
					[
						SNew(STextBlock)
						.Visibility(UnlockBadge.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
						.Font(ChroniclesHUDPalette::FontBold(9))
						.ColorAndOpacity(BadgeColor)
						.Text(FText::FromString(UnlockBadge))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(10))
						.ColorAndOpacity_Lambda([WeakPC = OwnerPC, QId]() -> FSlateColor
						{
							AChroniclesPlayerController* StrongPC = WeakPC.Get();
							UGameInstance* G = StrongPC ? StrongPC->GetGameInstance() : nullptr;
							const UQuestSubsystem* QS = G ? G->GetSubsystem<UQuestSubsystem>() : nullptr;
							const FQuestDef* LiveQ = QS ? QS->FindQuest(QId) : nullptr;
							return LiveQ ? FSlateColor(GetQuestStateColor(LiveQ->State)) : FSlateColor(ChroniclesHUDPalette::TextSecondary);
						})
						.Text_Lambda([WeakPC = OwnerPC, QId]() -> FText
						{
							AChroniclesPlayerController* StrongPC = WeakPC.Get();
							UGameInstance* G = StrongPC ? StrongPC->GetGameInstance() : nullptr;
							const UQuestSubsystem* QS = G ? G->GetSubsystem<UQuestSubsystem>() : nullptr;
							const FQuestDef* LiveQ = QS ? QS->FindQuest(QId) : nullptr;
							if (!LiveQ) return FText::GetEmpty();
							return FText::FromString(FString::Printf(TEXT("[%s] %d/%d"),
								*GetQuestStateLabel(LiveQ->State), LiveQ->Progress, LiveQ->TargetCount));
						})
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 4.f, 0.f, 4.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontRegular(9))
					.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
					.AutoWrapText(true)
					.Text(Q.ObjectiveText)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(9))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text(FText::FromString(FString::Printf(TEXT("Rewards: %d Gold  ·  %d XP"), Q.RewardGold, Q.RewardXP)))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin(10.f, 3.f))
						.OnClicked_Lambda([WeakPC = OwnerPC, QId]() -> FReply
						{
							if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
							{
								if (UGameInstance* G = StrongPC->GetGameInstance())
								{
									if (UQuestSubsystem* QS = G->GetSubsystem<UQuestSubsystem>())
									{
										QS->SetTrackedQuest(QId);
										StrongPC->PushToastMessage(TEXT("Updated tracked quest on compass."), 2.4f);
									}
								}
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(9))
							.Text_Lambda([WeakPC = OwnerPC, QId]() -> FText
							{
								AChroniclesPlayerController* StrongPC = WeakPC.Get();
								UGameInstance* G = StrongPC ? StrongPC->GetGameInstance() : nullptr;
								const UQuestSubsystem* QS = G ? G->GetSubsystem<UQuestSubsystem>() : nullptr;
								const bool bTracked = (QS && QS->GetTrackedQuestId() == QId);
								return bTracked ? LOCTEXT("TrackedBtn", "Tracking") : LOCTEXT("TrackBtn", "Track");
							})
						]
					]
				]
			]
		];
	}
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildGrimoireOverlay()
{
	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(ChroniclesHUDPalette::ModalScrim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::Grimoire) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.WidthOverride(940.f)
			.HeightOverride(630.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineCyan)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
					.Padding(FMargin(20.f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(16))
								.ColorAndOpacity(ChroniclesHUDPalette::TextCyan)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
									const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
									const int32 Unlocked = Spells ? Spells->GetUnlockedSpellCount() : 0;
									return FText::FromString(FString::Printf(
										TEXT("INVOKER GRIMOIRE  ·  %d / 10 ARCHAIC SPELLS UNLOCKED"), Unlocked));
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
							[
								SNew(SButton)
								.ContentPadding(FMargin(10.f, 4.f))
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get())
									{
										if (AAetheriaHeroCharacter* H = PC->GetActiveHero())
										{
											if (H->SpellSystem)
											{
												H->SpellSystem->ToggleCastMode();
											}
										}
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(9))
									.Text_Lambda([WeakPC = OwnerPC]() -> FText
									{
										AChroniclesPlayerController* PC = WeakPC.Get();
										const AAetheriaHeroCharacter* H = PC ? PC->GetActiveHero() : nullptr;
										const bool bInstant = (H && H->SpellSystem && H->SpellSystem->CastMode == EChronicleCastMode::InstantCombo);
										return bInstant ? LOCTEXT("ModeInstant", "Mode: Instant Combo") : LOCTEXT("ModeClassic", "Mode: Classic Invoker");
									})
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.ContentPadding(FMargin(12.f, 4.f))
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.Text(LOCTEXT("CloseGrimoireBtn", "Close [Tab / Esc]"))
								]
							]
						]

						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
								[
									SAssignNew(GrimoireLeftCol, SVerticalBox)
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								[
									SAssignNew(GrimoireRightCol, SVerticalBox)
								]
							]
						]
					]
				]
			]
		];
}

void SAetheriaChroniclesHUD::RebuildGrimoireGrid()
{
	if (!GrimoireLeftCol.IsValid() || !GrimoireRightCol.IsValid())
	{
		return;
	}
	GrimoireLeftCol->ClearChildren();
	GrimoireRightCol->ClearChildren();

	const TArray<FChronicleSpellDef>& AllSpells = USpellSystemComponent::GetAllSpellDefs();
	for (int32 i = 0; i < AllSpells.Num(); ++i)
	{
		const FChronicleSpellDef& Def = AllSpells[i];
		const EChronicleSpellId SpellId = Def.SpellId;

		FString ReqText;
		if (Def.bNeedsExort) ReqText += TEXT("Exort ");
		if (Def.bNeedsWex)   ReqText += TEXT("Wex ");
		if (Def.bNeedsQuas)  ReqText += TEXT("Quas");
		ReqText = ReqText.TrimEnd();

		TSharedPtr<SVerticalBox>& TargetCol = (i % 2 == 0) ? GrimoireLeftCol : GrimoireRightCol;
		TargetCol->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor_Lambda([WeakPC = OwnerPC, SpellId, PrimaryCol = Def.PrimaryColor]() -> FSlateColor
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				const AAetheriaHeroCharacter* H = PC ? PC->GetActiveHero() : nullptr;
				const bool bUnlocked = (H && H->SpellSystem && H->SpellSystem->IsSpellUnlocked(SpellId));
				return bUnlocked
					? FSlateColor(FLinearColor(PrimaryCol.R, PrimaryCol.G, PrimaryCol.B, 0.72f))
					: FSlateColor(ChroniclesHUDPalette::OrbLocked);
			})
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor_Lambda([WeakPC = OwnerPC, SpellId]() -> FSlateColor
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaHeroCharacter* H = PC ? PC->GetActiveHero() : nullptr;
					const bool bUnlocked = (H && H->SpellSystem && H->SpellSystem->IsSpellUnlocked(SpellId));
					return bUnlocked ? FSlateColor(ChroniclesHUDPalette::CardBg) : FSlateColor(ChroniclesHUDPalette::CardBgLocked);
				})
				.Padding(FMargin(12.f, 8.f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(11))
							.ColorAndOpacity(Def.PrimaryColor)
							.Text(FText::FromString(FString::Printf(TEXT("[%s]"), *Def.ComboKey)))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(11))
							.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
							.Text(Def.DisplayName)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(9))
							.ColorAndOpacity_Lambda([WeakPC = OwnerPC, SpellId]() -> FSlateColor
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaHeroCharacter* H = PC ? PC->GetActiveHero() : nullptr;
								const bool bUnlocked = (H && H->SpellSystem && H->SpellSystem->IsSpellUnlocked(SpellId));
								return bUnlocked ? FSlateColor(ChroniclesHUDPalette::TextEmerald) : FSlateColor(ChroniclesHUDPalette::TextCrimson);
							})
							.Text_Lambda([WeakPC = OwnerPC, SpellId, ReqText]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaHeroCharacter* H = PC ? PC->GetActiveHero() : nullptr;
								const bool bUnlocked = (H && H->SpellSystem && H->SpellSystem->IsSpellUnlocked(SpellId));
								return bUnlocked
									? LOCTEXT("SpellUnlockedLbl", "UNLOCKED")
									: FText::FromString(FString::Printf(TEXT("LOCKED (%s)"), *ReqText));
							})
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 3.f, 0.f, 3.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontRegular(8))
						.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
						.AutoWrapText(true)
						.Text(Def.Description)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontRegular(8))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text(FText::FromString(FString::Printf(
							TEXT("Mana: %.0f  ·  Cooldown: %.1fs  ·  Damage: %.0f  ·  Radius: %.0fm"),
							Def.ManaCost, Def.Cooldown, Def.BaseDamage, Def.BlastRadius / 100.f)))
					]
				]
			]
		];
	}
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildDialogueOverlay()
{
	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(ChroniclesHUDPalette::ModalScrim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 70.f))
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::Dialogue && PC->GetDialogueNPC() != nullptr)
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.WidthOverride(740.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
					.Padding(FMargin(22.f, 18.f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(15))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaNPCActor* NPC = PC ? PC->GetDialogueNPC() : nullptr;
								if (!NPC) return FText::GetEmpty();
								return FText::FromString(FString::Printf(TEXT("%s  —  %s"),
									*NPC->DisplayName.ToString(), *NPC->RoleTitle.ToString()));
							})
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 4.f, 0.f, 10.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(11))
							.ColorAndOpacity(ChroniclesHUDPalette::TextCyan)
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaNPCActor* NPC = PC ? PC->GetDialogueNPC() : nullptr;
								UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
								const UQuestSubsystem* QS = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;
								const FQuestDef* Q = (NPC && QS) ? QS->FindQuest(NPC->QuestId) : nullptr;
								if (!Q) return FText::GetEmpty();
								return FText::FromString(FString::Printf(TEXT("Quest: %s  [%s]"),
									*Q->Title.ToString(), *GetQuestStateLabel(Q->State)));
							})
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 14.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontRegular(11))
							.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
							.AutoWrapText(true)
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaNPCActor* NPC = PC ? PC->GetDialogueNPC() : nullptr;
								UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
								const UQuestSubsystem* QS = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;
								const FQuestDef* Q = (NPC && QS) ? QS->FindQuest(NPC->QuestId) : nullptr;
								if (!Q) return LOCTEXT("DefaultGreeting", "May the embers light your road across Aetheria, traveler.");
								if (Q->State == EQuestState::ReadyToTurnIn || Q->State == EQuestState::Completed)
								{
									return Q->TurnInDialogue;
								}
								return Q->OfferDialogue;
							})
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(10))
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaNPCActor* NPC = PC ? PC->GetDialogueNPC() : nullptr;
									UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
									const UQuestSubsystem* QS = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;
									const FQuestDef* Q = (NPC && QS) ? QS->FindQuest(NPC->QuestId) : nullptr;
									if (!Q) return FText::GetEmpty();

									FString Extra;
									if (Q->UnlockOrbReward == EChronicleOrb::Exort) Extra = TEXT("  ·  UNLOCKS EXORT");
									else if (Q->UnlockOrbReward == EChronicleOrb::Wex) Extra = TEXT("  ·  UNLOCKS WEX");
									else if (Q->UnlockOrbReward == EChronicleOrb::Quas) Extra = TEXT("  ·  UNLOCKS QUAS");
									else if (Q->bUnlocksDragonMount) Extra = TEXT("  ·  UNLOCKS DRAGON MOUNT");

									return FText::FromString(FString::Printf(TEXT("Reward: %d Gold  ·  %d XP%s"),
										Q->RewardGold, Q->RewardXP, *Extra));
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
							[
								SNew(SButton)
								.ContentPadding(FMargin(14.f, 6.f))
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get())
									{
										PC->HandleDialoguePrimaryAction();
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.Text_Lambda([WeakPC = OwnerPC]() -> FText
									{
										AChroniclesPlayerController* PC = WeakPC.Get();
										const AAetheriaNPCActor* NPC = PC ? PC->GetDialogueNPC() : nullptr;
										UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
										const UQuestSubsystem* QS = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;
										const FQuestDef* Q = (NPC && QS) ? QS->FindQuest(NPC->QuestId) : nullptr;
										if (!Q) return LOCTEXT("FarewellBtn", "Farewell");
										switch (Q->State)
										{
										case EQuestState::Available:     return LOCTEXT("AcceptQuestBtn", "Accept Quest");
										case EQuestState::ReadyToTurnIn: return LOCTEXT("ClaimRewardBtn", "Claim Reward & Complete");
										case EQuestState::Active:        return LOCTEXT("TrackObjBtn", "Track Objective");
										default:                         return LOCTEXT("DoneQuestBtn", "Farewell");
										}
									})
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.ContentPadding(FMargin(12.f, 6.f))
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontRegular(10))
									.Text(LOCTEXT("CloseDlgBtn", "Leave [Esc]"))
								]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildGraphicsRTXOverlay()
{
	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(ChroniclesHUDPalette::ModalScrim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::GraphicsRTX) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.WidthOverride(640.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineCyan)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
					.Padding(FMargin(22.f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(15))
								.ColorAndOpacity(ChroniclesHUDPalette::TextCyan)
								.Text(LOCTEXT("RTXPanelHdr", "RTX · DLSS · STREAMING TELEMETRY"))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.ContentPadding(FMargin(10.f, 4.f))
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.Text(LOCTEXT("CloseRTXBtn", "Close [F10 / Esc]"))
								]
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 14.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontRegular(11))
							.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
							.AutoWrapText(true)
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
								const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
								if (!RTXSub) return LOCTEXT("NoRTXSub", "UGraphicsRTXSubsystem unavailable.");

								const FGraphicsTelemetry& Tel = RTXSub->GetTelemetry();
								const TCHAR* TierLabel = (Tel.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
									? TEXT("Crimson RTX Ultra (HWRT + Lumen + Volumetric Clouds)")
									: TEXT("Classic Performance (Simplified Non-RTX Fallback)");

								return FText::FromString(FString::Printf(
									TEXT("GPU Adapter: %s\nActive Render Tier: %s\nHardware Ray Tracing Supported: %s  |  Active: %s\nNVIDIA DLSS Detected: %s  |  Active: %s  |  Reflex: %s\nTexture Streaming Pool: %d MB (Frustum & Proximity Priority)\nStatus: %s"),
									*Tel.GPUAdapterName,
									TierLabel,
									Tel.bHardwareRTXSupported ? TEXT("YES") : TEXT("NO"),
									Tel.bHardwareRTXActive ? TEXT("YES") : TEXT("NO"),
									Tel.bDLSSPluginDetected ? TEXT("YES") : TEXT("NO (Using TSR)"),
									Tel.bDLSSActive ? TEXT("ON") : TEXT("OFF"),
									Tel.bReflexActive ? TEXT("ON") : TEXT("OFF"),
									Tel.TexturePoolMB,
									*Tel.SummaryBanner));
							})
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.Padding(FMargin(0.f, 0.f, 10.f, 0.f))
							[
								SNew(SButton)
								.ContentPadding(FMargin(12.f, 8.f))
								.HAlign(HAlign_Center)
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get())
									{
										if (UGameInstance* GI = PC->GetGameInstance())
										{
											if (UGraphicsRTXSubsystem* RTXSub = GI->GetSubsystem<UGraphicsRTXSubsystem>())
											{
												RTXSub->ApplyRenderTier(EChronicleRenderTier::CrimsonRTXUltra);
												PC->PushToastMessage(TEXT("Applied Render Tier: Crimson RTX Ultra"), 3.2f);
											}
										}
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.Text(LOCTEXT("ApplyRTXBtn", "Enable Crimson RTX Ultra"))
								]
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							[
								SNew(SButton)
								.ContentPadding(FMargin(12.f, 8.f))
								.HAlign(HAlign_Center)
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get())
									{
										if (UGameInstance* GI = PC->GetGameInstance())
										{
											if (UGraphicsRTXSubsystem* RTXSub = GI->GetSubsystem<UGraphicsRTXSubsystem>())
											{
												RTXSub->ApplyRenderTier(EChronicleRenderTier::ClassicPerformance);
												PC->PushToastMessage(TEXT("Applied Render Tier: Classic Performance Mode"), 3.2f);
											}
										}
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.Text(LOCTEXT("ApplyClassicBtn", "Enable Classic Performance"))
								]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildHeroCustomizerOverlay()
{
	const FLinearColor StudioGlassBg(0.031f, 0.035f, 0.059f, 0.86f);
	TWeakPtr<SAetheriaChroniclesHUD> WeakHUD = DoesSharedInstanceExist()
		? TWeakPtr<SAetheriaChroniclesHUD>(SharedThis(this))
		: TWeakPtr<SAetheriaChroniclesHUD>();

	auto MakeStudioCamBtn = [WeakPC = OwnerPC](const FText& Label, TFunction<void(AChroniclesPlayerController*)> OnClick, TFunction<bool(const AChroniclesPlayerController*)> IsActive = nullptr) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor_Lambda([WeakPC, IsActive]() -> FSlateColor
			{
				const AChroniclesPlayerController* PC = WeakPC.Get();
				const bool bHot = (PC && IsActive && IsActive(PC));
				return bHot ? FSlateColor(ChroniclesHUDPalette::HairlineCyan) : FSlateColor(ChroniclesHUDPalette::HairlineGold);
			})
			.Padding(FMargin(1.f))
			[
				SNew(SButton)
				.ContentPadding(FMargin(10.f, 4.f))
				.OnClicked_Lambda([WeakPC, OnClick]() -> FReply
				{
					if (AChroniclesPlayerController* PC = WeakPC.Get())
					{
						OnClick(PC);
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(9))
					.ColorAndOpacity_Lambda([WeakPC, IsActive]() -> FSlateColor
					{
						const AChroniclesPlayerController* PC = WeakPC.Get();
						const bool bHot = (PC && IsActive && IsActive(PC));
						return bHot ? FSlateColor(ChroniclesHUDPalette::TextCyan) : FSlateColor(ChroniclesHUDPalette::TextPrimary);
					})
					.Text(Label)
				]
			];
	};

	auto MakeTabBtn = [WeakHUD](int32 TabIndex, const FText& TabLabel) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor_Lambda([WeakHUD, TabIndex]() -> FSlateColor
			{
				const TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin();
				return (HUD && HUD->CreatorActiveTab == TabIndex)
					? FSlateColor(ChroniclesHUDPalette::HairlineCyan)
					: FSlateColor(ChroniclesHUDPalette::OrbLocked);
			})
			.Padding(FMargin(1.f))
			[
				SNew(SButton)
				.ContentPadding(FMargin(8.f, 5.f))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([WeakHUD, TabIndex]() -> FReply
				{
					if (TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin())
					{
						HUD->CreatorActiveTab = TabIndex;
						HUD->RebuildHeroCustomizerPanels();
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(9))
					.ColorAndOpacity_Lambda([WeakHUD, TabIndex]() -> FSlateColor
					{
						const TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin();
						return (HUD && HUD->CreatorActiveTab == TabIndex)
							? FSlateColor(ChroniclesHUDPalette::TextCyan)
							: FSlateColor(ChroniclesHUDPalette::TextSecondary);
					})
					.Text(TabLabel)
				]
			];
	};

	return SNew(SOverlay)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer)
				? EVisibility::SelfHitTestInvisible
				: EVisibility::Collapsed;
		})

		// --- Top-Center Sleek Studio Camera Control Bar (Center 42%+ stays open for 3D Hero!) ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 14.f, 0.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(StudioGlassBg)
				.Padding(FMargin(12.f, 7.f))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.f, 0.f, 10.f, 0.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(10))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text(LOCTEXT("CreatorStudioTitle", "HERO CREATOR"))
					]

					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 5.f, 0.f))
					[
						MakeStudioCamBtn(LOCTEXT("CamRotLeft", "< Rotate -30°"), [](AChroniclesPlayerController* PC) { PC->AdjustCreatorCameraOrbit(-30.f); })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 10.f, 0.f))
					[
						MakeStudioCamBtn(LOCTEXT("CamRotRight", "Rotate +30° >"), [](AChroniclesPlayerController* PC) { PC->AdjustCreatorCameraOrbit(30.f); })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 5.f, 0.f))
					[
						MakeStudioCamBtn(LOCTEXT("CamZoomFull", "Full Body"),
							[](AChroniclesPlayerController* PC) { PC->SetCreatorCameraZoom(0); },
							[](const AChroniclesPlayerController* PC) { return PC->GetCreatorCameraZoom() == 0; })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 5.f, 0.f))
					[
						MakeStudioCamBtn(LOCTEXT("CamZoomTorso", "Torso"),
							[](AChroniclesPlayerController* PC) { PC->SetCreatorCameraZoom(1); },
							[](const AChroniclesPlayerController* PC) { return PC->GetCreatorCameraZoom() == 1; })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 10.f, 0.f))
					[
						MakeStudioCamBtn(LOCTEXT("CamZoomFace", "Face"),
							[](AChroniclesPlayerController* PC) { PC->SetCreatorCameraZoom(2); },
							[](const AChroniclesPlayerController* PC) { return PC->GetCreatorCameraZoom() == 2; })
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						MakeStudioCamBtn(LOCTEXT("CamCloseBtn", "Close [C/Esc]"), [](AChroniclesPlayerController* PC) { PC->CloseAllOverlays(); })
					]
				]
			]
		]

		// --- Left Edge-Docked Panel (Width 370px): Appearance, Race, Physique & Detail Quality ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(16.f, 68.f, 0.f, 18.f))
		[
			SNew(SBox)
			.WidthOverride(370.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(StudioGlassBg)
					.Padding(FMargin(14.f, 12.f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(13))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text(LOCTEXT("CreatorLeftHdr", "HERO APPEARANCE & PHYSIQUE"))
						]

						// 3 Clickable Tabs
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 4.f, 0.f))
							[ MakeTabBtn(0, LOCTEXT("TabRaceGender", "Race")) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(2.f, 0.f, 2.f, 0.f))
							[ MakeTabBtn(1, LOCTEXT("TabFaceHair", "Face")) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(4.f, 0.f, 0.f, 0.f))
							[ MakeTabBtn(2, LOCTEXT("TabBodyOutfit", "Body")) ]
						]

						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(CreatorLeftContentBox, SVerticalBox)
							]
						]
					]
				]
			]
		]

		// --- Right Edge-Docked Panel (Width 360px): RPG Attributes & Live Combat Power ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(0.f, 68.f, 16.f, 18.f))
		[
			SNew(SBox)
			.WidthOverride(360.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineCyan)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(StudioGlassBg)
					.Padding(FMargin(14.f, 12.f))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(13))
							.ColorAndOpacity(ChroniclesHUDPalette::TextCyan)
							.Text(LOCTEXT("CreatorRightHdr", "RPG ATTRIBUTES & LIVE POWER"))
						]

						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(CreatorRightStatsBox, SVerticalBox)
							]
						]
					]
				]
			]
		];
}

void SAetheriaChroniclesHUD::RebuildHeroCustomizerPanels()
{
	if (!CreatorLeftContentBox.IsValid() || !CreatorRightStatsBox.IsValid())
	{
		return;
	}

	CreatorLeftContentBox->ClearChildren();
	CreatorRightStatsBox->ClearChildren();

	AChroniclesPlayerController* PC = OwnerPC.Get();
	AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
	UHeroCustomizationComponent* Custom = Hero ? Hero->Customization.Get() : nullptr;
	if (!PC || !Hero || !Custom)
	{
		CreatorLeftContentBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Font(ChroniclesHUDPalette::FontRegular(10))
			.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
			.Text(LOCTEXT("CreatorWaitingHero", "Awaiting Hero Customization Component..."))
		];
		return;
	}

	TWeakPtr<SAetheriaChroniclesHUD> WeakHUD = DoesSharedInstanceExist()
		? TWeakPtr<SAetheriaChroniclesHUD>(SharedThis(this))
		: TWeakPtr<SAetheriaChroniclesHUD>();

	const FHeroCustomizationProfile& Profile = Custom->Profile;
	const FHeroDerivedStats& Derived = Custom->GetDerivedStats();

	auto AddSectionHeader = [](TSharedPtr<SVerticalBox>& TargetBox, const FString& Title, const FLinearColor& Color)
	{
		TargetBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 6.f, 0.f, 5.f))
		[
			SNew(STextBlock)
			.Font(ChroniclesHUDPalette::FontBold(10))
			.ColorAndOpacity(Color)
			.Text(FText::FromString(Title))
		];
	};

	auto MakeSelectableChip = [WeakPC = OwnerPC, WeakHUD](const FString& Label, bool bSelected, TFunction<void(UHeroCustomizationComponent*)> Action) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(bSelected ? ChroniclesHUDPalette::HairlineCyan : ChroniclesHUDPalette::OrbLocked)
			.Padding(FMargin(1.f))
			[
				SNew(SButton)
				.ContentPadding(FMargin(8.f, 5.f))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([WeakPC, WeakHUD, Action]() -> FReply
				{
					if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
					{
						if (AAetheriaHeroCharacter* H = StrongPC->GetActiveHero())
						{
							if (UHeroCustomizationComponent* C = H->Customization.Get())
							{
								Action(C);
							}
						}
					}
					if (TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin())
					{
						HUD->RebuildHeroCustomizerPanels();
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(9))
					.ColorAndOpacity(bSelected ? ChroniclesHUDPalette::TextCyan : ChroniclesHUDPalette::TextPrimary)
					.Text(FText::FromString(Label))
				]
			];
	};

	auto AddCyclerRow = [WeakPC = OwnerPC, WeakHUD](
		TSharedPtr<SVerticalBox>& TargetBox,
		const FString& RowTitle,
		const FString& ValueText,
		const TOptional<FLinearColor>& SwatchColor,
		const FString& PrevText,
		const FString& NextText,
		TFunction<void(UHeroCustomizationComponent*, int32)> StepFn)
	{
		TSharedRef<SHorizontalBox> ValueBox = SNew(SHorizontalBox);
		if (SwatchColor.IsSet())
		{
			ValueBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
			[
				SNew(SBox)
				.WidthOverride(14.f)
				.HeightOverride(14.f)
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(ChroniclesHUDPalette::WhiteBrush())
						.BorderBackgroundColor(SwatchColor.GetValue())
					]
				]
			];
		}
		ValueBox->AddSlot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(ChroniclesHUDPalette::FontBold(9))
			.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
			.Justification(ETextJustify::Center)
			.Text(FText::FromString(ValueText))
		];

		TargetBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::CardBg)
			.Padding(FMargin(8.f, 6.f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(8))
					.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
					.Text(FText::FromString(RowTitle))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin(8.f, 3.f))
						.OnClicked_Lambda([WeakPC, WeakHUD, StepFn]() -> FReply
						{
							if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
							{
								if (AAetheriaHeroCharacter* H = StrongPC->GetActiveHero())
								{
									if (UHeroCustomizationComponent* C = H->Customization.Get())
									{
										StepFn(C, -1);
									}
								}
							}
							if (TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin())
							{
								HUD->RebuildHeroCustomizerPanels();
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(9))
							.Text(FText::FromString(PrevText))
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Padding(FMargin(6.f, 0.f))
					[
						ValueBox
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin(8.f, 3.f))
						.OnClicked_Lambda([WeakPC, WeakHUD, StepFn]() -> FReply
						{
							if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
							{
								if (AAetheriaHeroCharacter* H = StrongPC->GetActiveHero())
								{
									if (UHeroCustomizationComponent* C = H->Customization.Get())
									{
										StepFn(C, +1);
									}
								}
							}
							if (TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin())
							{
								HUD->RebuildHeroCustomizerPanels();
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(9))
							.Text(FText::FromString(NextText))
						]
					]
				]
			]
		];
	};

	// ========================================================================
	// LEFT PANEL CONTENT BY TAB
	// ========================================================================
	if (CreatorActiveTab == 0)
	{
		// Tab 0: Race & Gender + Detail Quality
		AddSectionHeader(CreatorLeftContentBox, TEXT("GENDER"), ChroniclesHUDPalette::TextGold);
		CreatorLeftContentBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 4.f, 0.f))
			[
				MakeSelectableChip(TEXT("Male"), Profile.Gender == EHeroGender::Male,
					[](UHeroCustomizationComponent* C) { C->SetGender(EHeroGender::Male); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(4.f, 0.f, 0.f, 0.f))
			[
				MakeSelectableChip(TEXT("Female"), Profile.Gender == EHeroGender::Female,
					[](UHeroCustomizationComponent* C) { C->SetGender(EHeroGender::Female); })
			]
		];

		AddSectionHeader(CreatorLeftContentBox, TEXT("PLAYABLE RACE (PASSIVES & STATURE)"), ChroniclesHUDPalette::TextGold);
		static const EHeroRace Races[6] = {
			EHeroRace::Human, EHeroRace::HighElf,
			EHeroRace::HighlandNord, EHeroRace::AshDraconid,
			EHeroRace::ShadowKin, EHeroRace::Ironborn
		};
		static const TCHAR* RaceShortNames[6] = {
			TEXT("Human"), TEXT("High Elf"),
			TEXT("Highland Nord"), TEXT("Ash Draconid"),
			TEXT("Shadow Kin"), TEXT("Ironborn")
		};
		for (int32 Row = 0; Row < 3; ++Row)
		{
			const int32 IdxA = Row * 2;
			const int32 IdxB = Row * 2 + 1;
			CreatorLeftContentBox->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 4.f, 0.f))
				[
					MakeSelectableChip(RaceShortNames[IdxA], Profile.Race == Races[IdxA],
						[R = Races[IdxA]](UHeroCustomizationComponent* C) { C->SetRace(R); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(4.f, 0.f, 0.f, 0.f))
				[
					MakeSelectableChip(RaceShortNames[IdxB], Profile.Race == Races[IdxB],
						[R = Races[IdxB]](UHeroCustomizationComponent* C) { C->SetRace(R); })
				]
			];
		}

		// Race Lore & Passives Rich Wrapped Card
		CreatorLeftContentBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 4.f, 0.f, 10.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::CardBg)
				.Padding(FMargin(10.f, 8.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontRegular(9))
					.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
					.AutoWrapText(true)
					.Text(FText::FromString(UHeroCustomizationComponent::GetRaceLoreAndPassives(Profile.Race)))
				]
			]
		];

		AddSectionHeader(CreatorLeftContentBox, TEXT("HERO DETAIL QUALITY"), ChroniclesHUDPalette::TextCyan);
		CreatorLeftContentBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 3.f, 0.f))
			[
				MakeSelectableChip(TEXT("Low"), Profile.DetailQuality == EHeroDetailQuality::Low,
					[](UHeroCustomizationComponent* C) { C->SetDetailQuality(EHeroDetailQuality::Low); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(2.f, 0.f, 2.f, 0.f))
			[
				MakeSelectableChip(TEXT("Medium"), Profile.DetailQuality == EHeroDetailQuality::Medium,
					[](UHeroCustomizationComponent* C) { C->SetDetailQuality(EHeroDetailQuality::Medium); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(3.f, 0.f, 0.f, 0.f))
			[
				MakeSelectableChip(TEXT("High"), Profile.DetailQuality == EHeroDetailQuality::High,
					[](UHeroCustomizationComponent* C) { C->SetDetailQuality(EHeroDetailQuality::High); })
			]
		];
		CreatorLeftContentBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 4.f, 0.f))
			[
				MakeSelectableChip(TEXT("Ultra"), Profile.DetailQuality == EHeroDetailQuality::Ultra,
					[](UHeroCustomizationComponent* C) { C->SetDetailQuality(EHeroDetailQuality::Ultra); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(4.f, 0.f, 0.f, 0.f))
			[
				MakeSelectableChip(TEXT("Cinematic RTX"), Profile.DetailQuality == EHeroDetailQuality::CinematicRTX,
					[](UHeroCustomizationComponent* C) { C->SetDetailQuality(EHeroDetailQuality::CinematicRTX); })
			]
		];
	}
	else if (CreatorActiveTab == 1)
	{
		// Tab 1: Face & Hair
		AddSectionHeader(CreatorLeftContentBox, TEXT("HAIR & FACIAL STYLING"), ChroniclesHUDPalette::TextGold);

		AddCyclerRow(CreatorLeftContentBox, TEXT("HAIRSTYLE"),
			UHeroCustomizationComponent::GetHairStyleLabel(Profile.HairStyle),
			TOptional<FLinearColor>(), TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				const int32 Cur = static_cast<int32>(C->Profile.HairStyle);
				C->SetHairStyle(static_cast<EHeroHairStyle>((Cur + Delta + 8) % 8));
			});

		AddCyclerRow(CreatorLeftContentBox, TEXT("HAIR COLOR"),
			UHeroCustomizationComponent::GetHairColorName(Profile.HairColorIndex),
			UHeroCustomizationComponent::GetHairColor(Profile.HairColorIndex),
			TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				constexpr int32 N = UHeroCustomizationComponent::NUM_HAIR_COLORS;
				C->SetHairColorIndex((C->Profile.HairColorIndex + Delta + N) % N);
			});

		AddCyclerRow(CreatorLeftContentBox, TEXT("MUSTACHE"),
			UHeroCustomizationComponent::GetMustacheStyleLabel(Profile.MustacheStyle),
			TOptional<FLinearColor>(), TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				const int32 Cur = static_cast<int32>(C->Profile.MustacheStyle);
				C->SetMustacheStyle(static_cast<EHeroMustacheStyle>((Cur + Delta + 6) % 6));
			});

		AddCyclerRow(CreatorLeftContentBox, TEXT("BEARD"),
			UHeroCustomizationComponent::GetBeardStyleLabel(Profile.BeardStyle),
			TOptional<FLinearColor>(), TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				const int32 Cur = static_cast<int32>(C->Profile.BeardStyle);
				C->SetBeardStyle(static_cast<EHeroBeardStyle>((Cur + Delta + 7) % 7));
			});

		AddSectionHeader(CreatorLeftContentBox, TEXT("EYES & WAR PAINT"), ChroniclesHUDPalette::TextCyan);

		AddCyclerRow(CreatorLeftContentBox, TEXT("EYE STYLE"),
			UHeroCustomizationComponent::GetEyeStyleLabel(Profile.EyeStyle),
			UHeroCustomizationComponent::GetEyeColor(Profile.EyeStyle),
			TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				const int32 Cur = static_cast<int32>(C->Profile.EyeStyle);
				C->SetEyeStyle(static_cast<EHeroEyeStyle>((Cur + Delta + 8) % 8));
			});

		AddCyclerRow(CreatorLeftContentBox, TEXT("EYE GLOW INTENSITY"),
			FString::Printf(TEXT("%.1fx"), Profile.EyeGlowIntensity),
			TOptional<FLinearColor>(), TEXT("-"), TEXT("+"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				C->AdjustEyeGlow(Delta * 0.3f);
			});

		AddCyclerRow(CreatorLeftContentBox, TEXT("TATTOO / WAR PAINT"),
			UHeroCustomizationComponent::GetTattooStyleLabel(Profile.TattooStyle),
			TOptional<FLinearColor>(), TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				const int32 Cur = static_cast<int32>(C->Profile.TattooStyle);
				C->SetTattooStyle(static_cast<EHeroTattooStyle>((Cur + Delta + 7) % 7));
			});

		AddCyclerRow(CreatorLeftContentBox, TEXT("TATTOO COLOR"),
			UHeroCustomizationComponent::GetTattooColorName(Profile.TattooColorIndex),
			UHeroCustomizationComponent::GetTattooColor(Profile.TattooColorIndex),
			TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				constexpr int32 N = UHeroCustomizationComponent::NUM_TATTOO_COLORS;
				C->SetTattooColorIndex((C->Profile.TattooColorIndex + Delta + N) % N);
			});
	}
	else
	{
		// Tab 2: Body & Outfit
		AddSectionHeader(CreatorLeftContentBox, TEXT("ARMOR & VESTMENTS"), ChroniclesHUDPalette::TextGold);

		AddCyclerRow(CreatorLeftContentBox, TEXT("OUTFIT / ARMOR"),
			UHeroCustomizationComponent::GetOutfitStyleLabel(Profile.OutfitStyle),
			TOptional<FLinearColor>(), TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				const int32 Cur = static_cast<int32>(C->Profile.OutfitStyle);
				C->SetOutfitStyle(static_cast<EHeroOutfitStyle>((Cur + Delta + 6) % 6));
			});

		AddCyclerRow(CreatorLeftContentBox, TEXT("ARMOR TINT"),
			UHeroCustomizationComponent::GetArmorTintName(Profile.ArmorTintIndex),
			UHeroCustomizationComponent::GetArmorTintColor(Profile.ArmorTintIndex),
			TEXT("<"), TEXT(">"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				constexpr int32 N = UHeroCustomizationComponent::NUM_ARMOR_TINTS;
				C->SetArmorTintIndex((C->Profile.ArmorTintIndex + Delta + N) % N);
			});

		AddSectionHeader(CreatorLeftContentBox, TEXT("STATURE / HEIGHT (1.10m - 3.00m)"), ChroniclesHUDPalette::TextCyan);

		AddCyclerRow(CreatorLeftContentBox, TEXT("HEIGHT / STATURE"),
			FString::Printf(TEXT("%.2f m"), Profile.HeightMeters),
			TOptional<FLinearColor>(), TEXT("-0.10m"), TEXT("+0.10m"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				C->SetHeightMeters(C->Profile.HeightMeters + Delta * 0.10f);
			});

		// 3 One-Click Stature Preset Buttons
		CreatorLeftContentBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 3.f, 0.f))
			[
				MakeSelectableChip(TEXT("Small 1.25m\n(No Magic)"), Profile.HeightMeters < UHeroCustomizationComponent::SMALL_NO_MAGIC_MAX,
					[](UHeroCustomizationComponent* C) { C->SetHeightMeters(1.25f); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(2.f, 0.f, 2.f, 0.f))
			[
				MakeSelectableChip(TEXT("Standard\n1.85m"), Profile.HeightMeters >= UHeroCustomizationComponent::SMALL_NO_MAGIC_MAX && Profile.HeightMeters < UHeroCustomizationComponent::GIANT_TITAN_MIN,
					[](UHeroCustomizationComponent* C) { C->SetHeightMeters(1.85f); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(3.f, 0.f, 0.f, 0.f))
			[
				MakeSelectableChip(TEXT("Giant 3.00m\n(Titan)"), Profile.HeightMeters >= UHeroCustomizationComponent::GIANT_TITAN_MIN,
					[](UHeroCustomizationComponent* C) { C->SetHeightMeters(3.00f); })
			]
		];

		// Live Color-Coded Stature Impact Banner
		FLinearColor StatureCol = ChroniclesHUDPalette::TextCyan;
		FString StatureMsg = TEXT("1.45m - 2.25m STANDARD: Full Aether Spellcasting, Balanced Melee DMG, Speed & Turn Rate.");
		if (Profile.HeightMeters < UHeroCustomizationComponent::SMALL_NO_MAGIC_MAX)
		{
			StatureCol = ChroniclesHUDPalette::TextCrimson;
			StatureMsg = TEXT("<1.45m SMALL STATURE: Low DMG, NO SPELLS (Aether Locked!), High Agility & Fast Dodge.");
		}
		else if (Profile.HeightMeters >= UHeroCustomizationComponent::GIANT_TITAN_MIN)
		{
			StatureCol = ChroniclesHUDPalette::TextGold;
			StatureMsg = TEXT(">2.25m GIANT TITAN: Huge Melee DMG, Reach & Max HP, Clumsy Slow Turn Rate.");
		}

		CreatorLeftContentBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(StatureCol)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
				.Padding(FMargin(9.f, 6.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(8))
					.ColorAndOpacity(StatureCol)
					.AutoWrapText(true)
					.Text(FText::FromString(StatureMsg))
				]
			]
		];

		AddSectionHeader(CreatorLeftContentBox, TEXT("PHYSIQUE PROPORTIONS"), ChroniclesHUDPalette::TextGold);

		AddCyclerRow(CreatorLeftContentBox, TEXT("MUSCULATURE"),
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Profile.Musculature * 100.f)),
			TOptional<FLinearColor>(), TEXT("-10%"), TEXT("+10%"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				C->SetMusculature(C->Profile.Musculature + Delta * 0.10f);
			});

		AddCyclerRow(CreatorLeftContentBox, TEXT("SHOULDER WIDTH"),
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Profile.ShoulderWidth * 100.f)),
			TOptional<FLinearColor>(), TEXT("-10%"), TEXT("+10%"),
			[](UHeroCustomizationComponent* C, int32 Delta)
			{
				C->SetShoulderWidth(C->Profile.ShoulderWidth + Delta * 0.10f);
			});
	}

	// ========================================================================
	// RIGHT PANEL CONTENT: RPG ATTRIBUTES & LIVE COMBAT POWER
	// ========================================================================
	CreatorRightStatsBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
	[
		SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::CardBg)
			.Padding(FMargin(10.f, 7.f))
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(11))
				.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
				.Justification(ETextJustify::Center)
				.Text(FText::FromString(FString::Printf(TEXT("Unspent Attribute Points: %d"), Profile.Attributes.UnspentPoints)))
			]
		]
	];

	struct FAttrRowSpec
	{
		EHeroAttribute Attr;
		int32 EffValue;
		const TCHAR* Hint;
	};
	const FAttrRowSpec AttrSpecs[6] = {
		{ EHeroAttribute::Strength,     Derived.EffStrength,     TEXT("+Melee Dmg & Max HP") },
		{ EHeroAttribute::Endurance,    Derived.EffEndurance,    TEXT("+Stamina, HP & Armor") },
		{ EHeroAttribute::Agility,      Derived.EffAgility,      TEXT("+Atk Spd, Move Spd & Crit") },
		{ EHeroAttribute::Intelligence, Derived.EffIntelligence, TEXT("+Spell Power, Mana & -CD") },
		{ EHeroAttribute::Spirit,       Derived.EffSpirit,       TEXT("+Regen & Dragon Fire") },
		{ EHeroAttribute::Fortune,      Derived.EffFortune,      TEXT("+Crit Dmg, XP & Parry") }
	};

	for (const FAttrRowSpec& Spec : AttrSpecs)
	{
		const EHeroAttribute Attr = Spec.Attr;
		const int32 BaseVal = Custom->GetBaseAttributeValue(Attr);
		const int32 RaceBonus = Spec.EffValue - BaseVal;
		const FString ValStr = (RaceBonus == 0)
			? FString::Printf(TEXT("%d = %d"), BaseVal, Spec.EffValue)
			: FString::Printf(TEXT("%d (%+d) = %d"), BaseVal, RaceBonus, Spec.EffValue);

		CreatorRightStatsBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::CardBg)
			.Padding(FMargin(8.f, 5.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(9))
						.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
						.Text(FText::FromString(UHeroCustomizationComponent::GetAttributeLabel(Attr)))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(9))
						.ColorAndOpacity(RaceBonus > 0 ? ChroniclesHUDPalette::TextCyan : (RaceBonus < 0 ? ChroniclesHUDPalette::TextCrimson : ChroniclesHUDPalette::TextGold))
						.Text(FText::FromString(ValStr))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
					[
						SNew(SButton)
						.ContentPadding(FMargin(7.f, 1.f))
						.OnClicked_Lambda([WeakPC = OwnerPC, WeakHUD, Attr]() -> FReply
						{
							if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
							{
								if (AAetheriaHeroCharacter* H = StrongPC->GetActiveHero())
								{
									if (UHeroCustomizationComponent* C = H->Customization.Get())
									{
										C->ModifyAttribute(Attr, -1);
									}
								}
							}
							if (TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin())
							{
								HUD->RebuildHeroCustomizerPanels();
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(9))
							.Text(LOCTEXT("AttrMinusBtn", "-"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin(7.f, 1.f))
						.OnClicked_Lambda([WeakPC = OwnerPC, WeakHUD, Attr]() -> FReply
						{
							if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
							{
								if (AAetheriaHeroCharacter* H = StrongPC->GetActiveHero())
								{
									if (UHeroCustomizationComponent* C = H->Customization.Get())
									{
										C->ModifyAttribute(Attr, +1);
									}
								}
							}
							if (TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin())
							{
								HUD->RebuildHeroCustomizerPanels();
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(9))
							.Text(LOCTEXT("AttrPlusBtn", "+"))
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 2.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontRegular(8))
					.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
					.Text(FText::FromString(Spec.Hint))
				]
			]
		];
	}

	// Reset Stats & Save Hero Buttons
	CreatorRightStatsBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 4.f, 0.f, 8.f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 4.f, 0.f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(8.f, 5.f))
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([WeakPC = OwnerPC, WeakHUD]() -> FReply
			{
				if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
				{
					if (AAetheriaHeroCharacter* H = StrongPC->GetActiveHero())
					{
						if (UHeroCustomizationComponent* C = H->Customization.Get())
						{
							C->ResetAttributes();
							StrongPC->PushToastMessage(TEXT("Attributes reset to base values."), 2.4f);
						}
					}
				}
				if (TSharedPtr<SAetheriaChroniclesHUD> HUD = WeakHUD.Pin())
				{
					HUD->RebuildHeroCustomizerPanels();
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(9))
				.Text(LOCTEXT("ResetStatsBtn", "Reset Stats"))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(4.f, 0.f, 0.f, 0.f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(8.f, 5.f))
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
			{
				if (AChroniclesPlayerController* StrongPC = WeakPC.Get())
				{
					if (AAetheriaHeroCharacter* H = StrongPC->GetActiveHero())
					{
						if (H->SkillTree)
						{
							H->SkillTree->SaveProgress();
							StrongPC->PushToastMessage(TEXT("Hero Appearance & RPG Attributes Saved!"), 2.8f);
						}
					}
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(9))
				.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
				.Text(LOCTEXT("SaveHeroBtn", "Save Hero"))
			]
		]
	];

	// Live Derived Combat Stats Readout
	AddSectionHeader(CreatorRightStatsBox, TEXT("LIVE DERIVED COMBAT POWER"), ChroniclesHUDPalette::TextGold);

	auto AddStatLine = [this](const FString& Label, const FString& Value, const FLinearColor& ValColor)
	{
		CreatorRightStatsBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontRegular(8))
				.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
				.Text(FText::FromString(Label))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(9))
				.ColorAndOpacity(ValColor)
				.Text(FText::FromString(Value))
			]
		];
	};

	AddStatLine(TEXT("HP"), FString::Printf(TEXT("%.0f (+%.1f/s)"), Derived.MaxHP, Derived.HPRegenPerSec), ChroniclesHUDPalette::TextPrimary);
	AddStatLine(TEXT("Stamina"), FString::Printf(TEXT("%.0f (+%.1f/s)"), Derived.MaxStamina, Derived.StaminaRegenPerSec), ChroniclesHUDPalette::TextPrimary);
	AddStatLine(TEXT("Mana"), FString::Printf(TEXT("%.0f (+%.1f/s)"), Derived.MaxMana, Derived.ManaRegenPerSec), ChroniclesHUDPalette::TextPrimary);
	AddStatLine(TEXT("Melee Dmg"), FString::Printf(TEXT("%.2fx (Reach %.2fx)"), Derived.MeleeDamageMultiplier, Derived.MeleeReachMultiplier), ChroniclesHUDPalette::TextGold);
	AddStatLine(TEXT("Attack Speed"), FString::Printf(TEXT("%.2fx"), Derived.AttackSpeedMultiplier), ChroniclesHUDPalette::TextPrimary);
	AddStatLine(TEXT("Move Speed"), FString::Printf(TEXT("%.0f cm/s"), Derived.MoveSpeedCmPerSec), ChroniclesHUDPalette::TextPrimary);
	AddStatLine(TEXT("Turn Rate"), FString::Printf(TEXT("%.0f °/s"), Derived.TurnRateDegPerSec),
		Derived.TurnRateDegPerSec < 280.f ? ChroniclesHUDPalette::TextGold : ChroniclesHUDPalette::TextPrimary);
	AddStatLine(TEXT("Spell Power"), FString::Printf(TEXT("%.2fx"), Derived.SpellPowerMultiplier), ChroniclesHUDPalette::TextCyan);
	AddStatLine(TEXT("Spell Access"),
		Derived.bCanCastSpells ? TEXT("ENABLED") : TEXT("LOCKED (<1.45m)"),
		Derived.bCanCastSpells ? ChroniclesHUDPalette::TextCyan : ChroniclesHUDPalette::TextCrimson);
	AddStatLine(TEXT("Crit"), FString::Printf(TEXT("%.1f%%  (%.2fx Dmg)"), Derived.CritChance * 100.f, Derived.CritDamageMultiplier), ChroniclesHUDPalette::TextGold);
	AddStatLine(TEXT("Armor"), FString::Printf(TEXT("%.1f%% Mitigation"), Derived.ArmorReduction * 100.f), ChroniclesHUDPalette::TextPrimary);
	AddStatLine(TEXT("Dragon Fire"), FString::Printf(TEXT("%.2fx Power"), Derived.DragonFireMultiplier), ChroniclesHUDPalette::BarDragonFire);

	if (!Derived.RacePassiveSummary.IsEmpty())
	{
		CreatorRightStatsBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::CardBg)
			.Padding(FMargin(8.f, 6.f))
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontRegular(8))
				.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
				.AutoWrapText(true)
				.Text(FText::FromString(FString::Printf(TEXT("Race Passives: %s"), *Derived.RacePassiveSummary)))
			]
		];
	}
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildGameOverOverlay()
{
	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor(FLinearColor(0.06f, 0.01f, 0.01f, 0.78f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->IsHeroDead()) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::HairlineCrimson)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::PanelBgDeep)
				.Padding(FMargin(44.f, 32.f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(26))
						.ColorAndOpacity(ChroniclesHUDPalette::TextCrimson)
						.Text(LOCTEXT("GameOverTitle", "YOU HAVE FALLEN"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(FMargin(0.f, 8.f, 0.f, 20.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontRegular(11))
						.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
						.Text_Lambda([WeakPC = OwnerPC]() -> FText
						{
							AChroniclesPlayerController* PC = WeakPC.Get();
							const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
							const int32 Slain = Hero ? Hero->EnemiesSlain : 0;
							return FText::FromString(FString::Printf(
								TEXT("Foes Vanquished: %d  ·  The embers of Oakhaven await your return."), Slain));
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SButton)
						.ContentPadding(FMargin(24.f, 10.f))
						.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
						{
							if (AChroniclesPlayerController* PC = WeakPC.Get())
							{
								PC->RequestRespawnHero();
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(12))
							.Text(LOCTEXT("RespawnBtn", "Respawn at Oakhaven [R]"))
						]
					]
				]
			]
		];
}

// ============================================================================
// Shared Static Helpers
// ============================================================================

FLinearColor SAetheriaChroniclesHUD::GetOrbColor(EChronicleOrb Orb)
{
	switch (Orb)
	{
	case EChronicleOrb::Exort: return ChroniclesHUDPalette::OrbExort;
	case EChronicleOrb::Wex:   return ChroniclesHUDPalette::OrbWex;
	case EChronicleOrb::Quas:  return ChroniclesHUDPalette::OrbQuas;
	default:                   return ChroniclesHUDPalette::OrbLocked;
	}
}

FString SAetheriaChroniclesHUD::GetOrbLetter(EChronicleOrb Orb)
{
	switch (Orb)
	{
	case EChronicleOrb::Exort: return TEXT("E");
	case EChronicleOrb::Wex:   return TEXT("W");
	case EChronicleOrb::Quas:  return TEXT("Q");
	default:                   return TEXT("-");
	}
}

FString SAetheriaChroniclesHUD::GetBossAttackWarningText(EBossAttackKind AttackKind)
{
	switch (AttackKind)
	{
	case EBossAttackKind::CrimsonCleave:     return TEXT("CRIMSON CLEAVE");
	case EBossAttackKind::SeismicSlam:       return TEXT("SEISMIC SLAM");
	case EBossAttackKind::AbyssalFireVolley: return TEXT("ABYSSAL FIRE VOLLEY");
	case EBossAttackKind::TempestLunge:      return TEXT("TEMPEST LUNGE");
	case EBossAttackKind::ShatterSpin:       return TEXT("SHATTER SPIN");
	case EBossAttackKind::ChronoCataclysm:   return TEXT("CHRONO CATACLYSM");
	default:                                 return TEXT("SOVEREIGN STRIKE");
	}
}

FString SAetheriaChroniclesHUD::GetQuestStateLabel(EQuestState State)
{
	switch (State)
	{
	case EQuestState::Available:     return TEXT("AVAILABLE");
	case EQuestState::Active:        return TEXT("ACTIVE");
	case EQuestState::ReadyToTurnIn: return TEXT("READY TO TURN IN");
	case EQuestState::Completed:     return TEXT("COMPLETED");
	default:                         return TEXT("UNKNOWN");
	}
}

FLinearColor SAetheriaChroniclesHUD::GetQuestStateColor(EQuestState State)
{
	switch (State)
	{
	case EQuestState::Available:     return ChroniclesHUDPalette::TextGold;
	case EQuestState::Active:        return ChroniclesHUDPalette::TextCyan;
	case EQuestState::ReadyToTurnIn: return ChroniclesHUDPalette::TextEmerald;
	case EQuestState::Completed:     return ChroniclesHUDPalette::TextSecondary;
	default:                         return ChroniclesHUDPalette::TextSecondary;
	}
}

#undef LOCTEXT_NAMESPACE
