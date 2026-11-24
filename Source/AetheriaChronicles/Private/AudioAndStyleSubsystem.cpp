// ============================================================================
// AudioAndStyleSubsystem.cpp — Crimson Desert Visual Style Harmonizer &
// Dual-Deck Dynamic Music Subsystem for AetheriaChronicles (UE 5.8.4)
// ============================================================================

#include "AudioAndStyleSubsystem.h"

#include "ChroniclesTypes.h"
#include "Components/AudioComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MaterialTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace AetheriaStyleInternal
{
	struct FRoughnessBudget
	{
		float MinRoughness = 0.48f;
		float MaxRoughness = 0.96f;
		bool bClampSpecular = true;
		float MaxSpecular = 0.38f;
	};

	static FRoughnessBudget GetCategoryBudget(EChronicleStyleCategory Category)
	{
		switch (Category)
		{
		case EChronicleStyleCategory::Character:
			return { 0.38f, 0.94f, true, 0.38f };
		case EChronicleStyleCategory::Creature:
			return { 0.42f, 0.96f, true, 0.38f };
		case EChronicleStyleCategory::Architecture:
			return { 0.58f, 1.00f, false, 0.38f };
		case EChronicleStyleCategory::Prop:
		default:
			return { 0.48f, 0.96f, true, 0.38f };
		}
	}

	static bool ShouldSkipMaterialByName(const FString& MatName)
	{
		static const TCHAR* ExcludedTokens[] =
		{
			TEXT("FX"),
			TEXT("VFX"),
			TEXT("Emissive"),
			TEXT("Glow"),
			TEXT("Fire"),
			TEXT("Flame"),
			TEXT("Magic"),
			TEXT("Aura"),
			TEXT("Lava"),
			TEXT("Crystal"),
			TEXT("Orb"),
			TEXT("Eye"),
			TEXT("Hair"),
			TEXT("Beam"),
			TEXT("Smoke")
		};

		for (const TCHAR* Token : ExcludedTokens)
		{
			if (MatName.Contains(Token, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	static bool IsSafeTintVectorParam(const FString& ParamName)
	{
		const bool bHasTintToken =
			ParamName.Contains(TEXT("Tint"), ESearchCase::IgnoreCase) ||
			ParamName.Contains(TEXT("AlbedoTint"), ESearchCase::IgnoreCase) ||
			ParamName.Contains(TEXT("BaseColorTint"), ESearchCase::IgnoreCase);

		if (!bHasTintToken)
		{
			return false;
		}

		return !ParamName.Contains(TEXT("Emissive"), ESearchCase::IgnoreCase)
			&& !ParamName.Contains(TEXT("Glow"), ESearchCase::IgnoreCase)
			&& !ParamName.Contains(TEXT("Fire"), ESearchCase::IgnoreCase)
			&& !ParamName.Contains(TEXT("Skin"), ESearchCase::IgnoreCase);
	}
}

void FAetheriaStyleHarmonizer::ApplyCrimsonDesertPostProcess(FPostProcessSettings& PP)
{
	// Grounded Crimson Desert AgX/Filmic grading — intentionally leaves AutoExposureMethod/Min/Max/Bias untouched.
	PP.bOverride_WhiteTemp = true;
	PP.WhiteTemp = 6800.f;

	PP.bOverride_WhiteTint = true;
	PP.WhiteTint = 0.0f;

	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(0.90f, 0.90f, 0.90f, 1.0f);

	PP.bOverride_ColorContrast = true;
	PP.ColorContrast = FVector4(1.08f, 1.08f, 1.08f, 1.0f);

	PP.bOverride_ColorGamma = true;
	PP.ColorGamma = FVector4(0.97f, 0.97f, 0.97f, 1.0f);

	PP.bOverride_ColorGainHighlights = true;
	PP.ColorGainHighlights = FVector4(1.05f, 0.99f, 0.92f, 1.0f);

	PP.bOverride_ColorGainShadows = true;
	PP.ColorGainShadows = FVector4(0.91f, 0.95f, 1.04f, 1.0f);

	PP.bOverride_ColorOffset = true;
	PP.ColorOffset = FVector4(0.002f, 0.004f, 0.008f, 0.0f);

	PP.bOverride_FilmSlope = true;
	PP.FilmSlope = 0.89f;

	PP.bOverride_FilmToe = true;
	PP.FilmToe = 0.51f;

	PP.bOverride_FilmShoulder = true;
	PP.FilmShoulder = 0.28f;

	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.32f;

	PP.bOverride_BloomThreshold = true;
	PP.BloomThreshold = 1.18f;

	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.34f;

	PP.bOverride_MotionBlurAmount = true;
	PP.MotionBlurAmount = 0.0f;
}

void FAetheriaStyleHarmonizer::HarmonizeActorMaterials(AActor* Actor, EChronicleStyleCategory Category)
{
	if (!IsValid(Actor))
	{
		return;
	}

	const AetheriaStyleInternal::FRoughnessBudget Budget = AetheriaStyleInternal::GetCategoryBudget(Category);
	const FLinearColor HighlandWarmth(1.02f, 0.995f, 0.965f, 1.0f);

	TInlineComponentArray<UMeshComponent*> MeshComponents;
	Actor->GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* MeshComp : MeshComponents)
	{
		if (!IsValid(MeshComp))
		{
			continue;
		}

		const int32 NumMaterials = MeshComp->GetNumMaterials();
		for (int32 SlotIdx = 0; SlotIdx < NumMaterials; ++SlotIdx)
		{
			UMaterialInterface* Mat = MeshComp->GetMaterial(SlotIdx);
			if (!IsValid(Mat))
			{
				continue;
			}

			// Idempotent: never re-wrap an existing MID.
			if (Mat->IsA<UMaterialInstanceDynamic>())
			{
				continue;
			}

			// Never touch translucent, additive, or modulated VFX materials.
			const EBlendMode BlendMode = Mat->GetBlendMode();
			if (BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked)
			{
				continue;
			}

			if (AetheriaStyleInternal::ShouldSkipMaterialByName(Mat->GetName()))
			{
				continue;
			}

			TArray<FMaterialParameterInfo> ScalarInfos;
			TArray<FGuid> ScalarGuids;
			Mat->GetAllScalarParameterInfo(ScalarInfos, ScalarGuids);

			TArray<FMaterialParameterInfo> VecInfos;
			TArray<FGuid> VecGuids;
			Mat->GetAllVectorParameterInfo(VecInfos, VecGuids);

			TArray<TPair<FMaterialParameterInfo, float>> PendingScalars;
			TArray<TPair<FMaterialParameterInfo, FLinearColor>> PendingVectors;

			for (const FMaterialParameterInfo& Info : ScalarInfos)
			{
				const FString ParamName = Info.Name.ToString();

				if (ParamName.Contains(TEXT("Rough"), ESearchCase::IgnoreCase)
					&& !ParamName.Contains(TEXT("Offset"), ESearchCase::IgnoreCase)
					&& !ParamName.Contains(TEXT("Blend"), ESearchCase::IgnoreCase))
				{
					float CurVal = 0.0f;
					if (Mat->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), CurVal))
					{
						if (CurVal < Budget.MinRoughness || CurVal > Budget.MaxRoughness)
						{
							PendingScalars.Emplace(Info, FMath::Clamp(CurVal, Budget.MinRoughness, Budget.MaxRoughness));
						}
					}
				}
				else if (Budget.bClampSpecular
					&& ParamName.Contains(TEXT("Specular"), ESearchCase::IgnoreCase)
					&& !ParamName.Contains(TEXT("Color"), ESearchCase::IgnoreCase))
				{
					float CurVal = 0.0f;
					if (Mat->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), CurVal))
					{
						if (CurVal > Budget.MaxSpecular)
						{
							PendingScalars.Emplace(Info, Budget.MaxSpecular);
						}
					}
				}
			}

			for (const FMaterialParameterInfo& Info : VecInfos)
			{
				const FString ParamName = Info.Name.ToString();
				if (!AetheriaStyleInternal::IsSafeTintVectorParam(ParamName))
				{
					continue;
				}

				FLinearColor CurColor = FLinearColor::White;
				if (Mat->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), CurColor))
				{
					const float Luminance = CurColor.GetLuminance();
					const float DesatR = FMath::Lerp(CurColor.R, Luminance, 0.06f);
					const float DesatG = FMath::Lerp(CurColor.G, Luminance, 0.06f);
					const float DesatB = FMath::Lerp(CurColor.B, Luminance, 0.06f);

					const FLinearColor HarmonizedColor(
						DesatR * HighlandWarmth.R,
						DesatG * HighlandWarmth.G,
						DesatB * HighlandWarmth.B,
						CurColor.A);

					if (!HarmonizedColor.Equals(CurColor, 1e-4f))
					{
						PendingVectors.Emplace(Info, HarmonizedColor);
					}
				}
			}

			if (PendingScalars.IsEmpty() && PendingVectors.IsEmpty())
			{
				continue;
			}

			UMaterialInstanceDynamic* MID = MeshComp->CreateDynamicMaterialInstance(SlotIdx, Mat);
			if (!IsValid(MID))
			{
				continue;
			}

			for (const TPair<FMaterialParameterInfo, float>& ScalarPair : PendingScalars)
			{
				MID->SetScalarParameterValueByInfo(ScalarPair.Key, ScalarPair.Value);
			}

			for (const TPair<FMaterialParameterInfo, FLinearColor>& VectorPair : PendingVectors)
			{
				MID->SetVectorParameterValueByInfo(VectorPair.Key, VectorPair.Value);
			}
		}
	}
}

UAetheriaMusicSubsystem* UAetheriaMusicSubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext || !GEngine)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UAetheriaMusicSubsystem>() : nullptr;
}

bool UAetheriaMusicSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
}

void UAetheriaMusicSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (IsRunningDedicatedServer())
	{
		return;
	}

	for (TObjectPtr<USoundBase>& CueSlot : StateCues)
	{
		CueSlot = nullptr;
	}

	LoadCueForState(EChronicleMusicState::Village, TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_Village_Loop.CUE_Village_Loop"));
	LoadCueForState(EChronicleMusicState::Field,   TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_Field_Loop.CUE_Field_Loop"));
	LoadCueForState(EChronicleMusicState::Battle,  TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_Battle_Loop.CUE_Battle_Loop"));
	LoadCueForState(EChronicleMusicState::Boss,    TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_BossBattle_Loop.CUE_BossBattle_Loop"));

	auto CreateDeck = [this, &InWorld]() -> UAudioComponent*
	{
		UAudioComponent* Deck = NewObject<UAudioComponent>(this);
		if (Deck)
		{
			Deck->bAutoDestroy = false;
			Deck->bAllowSpatialization = false;
			Deck->bIsUISound = false;
			Deck->bAutoActivate = false;
			Deck->RegisterComponentWithWorld(&InWorld);
		}
		return Deck;
	};

	DeckA = CreateDeck();
	DeckB = CreateDeck();
	ActiveDeckIndex = 0;

	TransitionToState(GetExplorationState());
}

void UAetheriaMusicSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CombatDecayTimerHandle);
	}

	auto ReleaseDeck = [](TObjectPtr<UAudioComponent>& Deck)
	{
		if (IsValid(Deck))
		{
			Deck->Stop();
			if (Deck->IsRegistered())
			{
				Deck->UnregisterComponent();
			}
		}
		Deck = nullptr;
	};

	ReleaseDeck(DeckA);
	ReleaseDeck(DeckB);

	for (TObjectPtr<USoundBase>& CueSlot : StateCues)
	{
		CueSlot = nullptr;
	}

	CurrentState = EChronicleMusicState::None;
	bBossActive = false;

	Super::Deinitialize();
}

void UAetheriaMusicSubsystem::SetExplorationZone(bool bInVillage)
{
	bIsInVillage = bInVillage;

	if (bBossActive || CurrentState == EChronicleMusicState::Boss || CurrentState == EChronicleMusicState::Battle)
	{
		return;
	}

	TransitionToState(GetExplorationState());
}

void UAetheriaMusicSubsystem::NotifyCombatActivity()
{
	if (bBossActive || CurrentState == EChronicleMusicState::Boss)
	{
		return;
	}

	if (CurrentState != EChronicleMusicState::Battle)
	{
		TransitionToState(EChronicleMusicState::Battle);
	}

	ScheduleReturnToExploration(2.5f);
}

void UAetheriaMusicSubsystem::SetBossEncounterActive(bool bActive)
{
	bBossActive = bActive;

	if (bBossActive)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CombatDecayTimerHandle);
		}
		TransitionToState(EChronicleMusicState::Boss);
	}
	else if (CurrentState == EChronicleMusicState::Boss)
	{
		ScheduleReturnToExploration(2.5f);
	}
}

void UAetheriaMusicSubsystem::LoadCueForState(EChronicleMusicState State, const TCHAR* ObjectPath)
{
	const int32 StateIndex = static_cast<int32>(State);
	if (StateIndex < 0 || StateIndex >= UE_ARRAY_COUNT(StateCues) || !ObjectPath)
	{
		return;
	}

	const FString PackageName = FPackageName::ObjectPathToPackageName(FString(ObjectPath));
	if (!FPackageName::DoesPackageExist(PackageName))
	{
		UE_LOG(LogChronicles, Verbose, TEXT("UAetheriaMusicSubsystem: Music package not staged, skipping [%s]."), ObjectPath);
		return;
	}

	StateCues[StateIndex] = LoadObject<USoundBase>(nullptr, ObjectPath);
	if (!StateCues[StateIndex])
	{
		UE_LOG(LogChronicles, Warning, TEXT("UAetheriaMusicSubsystem: Failed to load SoundBase [%s]."), ObjectPath);
	}
}

void UAetheriaMusicSubsystem::TransitionToState(EChronicleMusicState NewState)
{
	if (NewState == CurrentState)
	{
		return;
	}

	const bool bIsCombat = (NewState == EChronicleMusicState::Battle || NewState == EChronicleMusicState::Boss);
	const float InFade = bIsCombat ? 0.85f : 1.75f;
	const float OutFade = bIsCombat ? 1.15f : 2.00f;
	const float TargetVolume = (NewState == EChronicleMusicState::Boss) ? 0.72f : 0.62f;

	UAudioComponent* OutgoingDeck = (ActiveDeckIndex == 0) ? DeckA.Get() : DeckB.Get();
	if (IsValid(OutgoingDeck) && OutgoingDeck->IsPlaying())
	{
		OutgoingDeck->FadeOut(OutFade, 0.0f, EAudioFaderCurve::Logarithmic);
	}

	ActiveDeckIndex = (ActiveDeckIndex == 0) ? 1 : 0;
	UAudioComponent* IncomingDeck = (ActiveDeckIndex == 0) ? DeckA.Get() : DeckB.Get();

	const int32 StateIndex = static_cast<int32>(NewState);
	USoundBase* NextCue = (StateIndex >= 0 && StateIndex < UE_ARRAY_COUNT(StateCues)) ? StateCues[StateIndex].Get() : nullptr;

	if (IsValid(IncomingDeck) && IsValid(NextCue))
	{
		IncomingDeck->SetSound(NextCue);
		IncomingDeck->FadeIn(InFade, TargetVolume, 0.0f, EAudioFaderCurve::Logarithmic);
	}

	CurrentState = NewState;
}

void UAetheriaMusicSubsystem::ScheduleReturnToExploration(float DelaySeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TWeakObjectPtr<UAetheriaMusicSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimer(
		CombatDecayTimerHandle,
		FTimerDelegate::CreateLambda([WeakThis]()
		{
			if (UAetheriaMusicSubsystem* StrongThis = WeakThis.Get())
			{
				if (!StrongThis->bBossActive)
				{
					StrongThis->TransitionToState(StrongThis->GetExplorationState());
				}
			}
		}),
		DelaySeconds,
		false);
}

EChronicleMusicState UAetheriaMusicSubsystem::GetExplorationState() const
{
	return bIsInVillage ? EChronicleMusicState::Village : EChronicleMusicState::Field;
}

// ============================================================================
// UHitStopSubsystem — Real-Time Wukong Micro-Hitstop (Never Dilates Camera)
// ============================================================================

UHitStopSubsystem* UHitStopSubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext || !GEngine)
	{
		return nullptr;
	}
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UHitStopSubsystem>() : nullptr;
}

bool UHitStopSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
}

TStatId UHitStopSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UHitStopSubsystem, STATGROUP_Tickables);
}

void UHitStopSubsystem::Deinitialize()
{
	for (const FActiveHitStop& Stop : ActiveStops)
	{
		if (AActor* Act = Stop.Actor.Get())
		{
			Act->CustomTimeDilation = 1.0f;
		}
	}
	ActiveStops.Reset();
	Super::Deinitialize();
}

void UHitStopSubsystem::RequestHitStop(AActor* ActorA, AActor* ActorB, float DurationSeconds, float Dilation)
{
	UWorld* World = GetWorld();
	if (!World || DurationSeconds <= 0.005f)
	{
		return;
	}

	const double EndTime = static_cast<double>(World->GetRealTimeSeconds()) + static_cast<double>(FMath::Clamp(DurationSeconds, 0.015f, 0.16f));
	const float SafeDil = FMath::Clamp(Dilation, 0.02f, 0.45f);

	auto RegisterActor = [&](AActor* Target)
	{
		if (!IsValid(Target))
		{
			return;
		}
		Target->CustomTimeDilation = SafeDil;
		for (FActiveHitStop& Existing : ActiveStops)
		{
			if (Existing.Actor.Get() == Target)
			{
				Existing.EndRealTime = FMath::Max(Existing.EndRealTime, EndTime);
				return;
			}
		}
		ActiveStops.Add({ Target, EndTime });
	};

	RegisterActor(ActorA);
	RegisterActor(ActorB);
}

void UHitStopSubsystem::Tick(float DeltaTime)
{
	if (ActiveStops.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double Now = static_cast<double>(World->GetRealTimeSeconds());
	for (int32 Idx = ActiveStops.Num() - 1; Idx >= 0; --Idx)
	{
		if (Now >= ActiveStops[Idx].EndRealTime || !ActiveStops[Idx].Actor.IsValid())
		{
			if (AActor* Act = ActiveStops[Idx].Actor.Get())
			{
				Act->CustomTimeDilation = 1.0f;
			}
			ActiveStops.RemoveAtSwap(Idx);
		}
	}
}
