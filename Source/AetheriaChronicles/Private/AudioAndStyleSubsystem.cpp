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
#include "Kismet/GameplayStatics.h"
#include "MaterialTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
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
	// Slight lift over the old 0.90: Crimson Desert keeps rich earth tones and saturated
	// magic while staying grounded — 0.90 was bleaching the whole frame into grey.
	PP.ColorSaturation = FVector4(0.97f, 0.97f, 0.97f, 1.0f);

	PP.bOverride_ColorContrast = true;
	// Punchier contrast separates silhouettes from the fog plane and gives the frame depth.
	PP.ColorContrast = FVector4(1.14f, 1.14f, 1.14f, 1.0f);

	PP.bOverride_ColorGamma = true;
	PP.ColorGamma = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

	PP.bOverride_ColorGainHighlights = true;
	PP.ColorGainHighlights = FVector4(1.06f, 1.00f, 0.93f, 1.0f);

	PP.bOverride_ColorGainShadows = true;
	// Cooler, deeper shadows (was a barely-there 0.94/0.97/1.04) for that cold-blue bounce
	// against warm sunlit stone — the signature Crimson Desert split-tone.
	PP.ColorGainShadows = FVector4(0.88f, 0.93f, 1.06f, 1.0f);

	PP.bOverride_ColorOffset = true;
	PP.ColorOffset = FVector4(0.0f, 0.001f, 0.004f, 0.0f);

	PP.bOverride_FilmSlope = true;
	PP.FilmSlope = 0.89f;

	PP.bOverride_FilmToe = true;
	PP.FilmToe = 0.44f;

	PP.bOverride_FilmShoulder = true;
	PP.FilmShoulder = 0.28f;

	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.28f;

	PP.bOverride_BloomThreshold = true;
	PP.BloomThreshold = 1.35f;

	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.28f;

	PP.bOverride_MotionBlurAmount = true;
	PP.MotionBlurAmount = 0.0f;

	// Film-like local contrast: lifts micro-detail in stone/armor without crushing the sky,
	// which is what separates a "filmic" frame from a flat unlit-looking render.
	PP.bOverride_LocalExposureMethod = true;
	PP.LocalExposureMethod = ELocalExposureMethod::Bilateral;
	PP.bOverride_LocalExposureHighlightContrastScale = true;
	PP.LocalExposureHighlightContrastScale = 1.05f;
	PP.bOverride_LocalExposureShadowContrastScale = true;
	PP.LocalExposureShadowContrastScale = 1.10f;
	PP.bOverride_LocalExposureDetailStrength = true;
	PP.LocalExposureDetailStrength = 0.85f;

	// Whisper of grain so large flat surfaces (sky, stone) never read as sterile CG.
	PP.bOverride_FilmGrainIntensity = true;
	PP.FilmGrainIntensity = 0.035f;
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

	// Load orchestral music cues with multi-candidate fallbacks (CUE -> looping WAV -> Gothic BGM)
	LoadCueForState(EChronicleMusicState::Menu, {
		TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_Menu_Loop.CUE_Menu_Loop"),
		TEXT("/Game/PathOfAdventure/Loop/WAV/WAV_Menu_Loop.WAV_Menu_Loop"),
		TEXT("/Game/Audio/BGM_Sanctuary_Gothic.BGM_Sanctuary_Gothic")
	});
	LoadCueForState(EChronicleMusicState::Village, {
		TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_Village_Loop.CUE_Village_Loop"),
		TEXT("/Game/PathOfAdventure/Loop/WAV/WAV_Village_Loop.WAV_Village_Loop"),
		TEXT("/Game/Audio/BGM_Sanctuary_Gothic.BGM_Sanctuary_Gothic")
	});
	LoadCueForState(EChronicleMusicState::Field, {
		TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_Field_Loop.CUE_Field_Loop"),
		TEXT("/Game/PathOfAdventure/Loop/WAV/WAV_Field_Loop.WAV_Field_Loop"),
		TEXT("/Game/Audio/BGM_Abyssal_Expedition.BGM_Abyssal_Expedition")
	});
	LoadCueForState(EChronicleMusicState::Battle, {
		TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_Battle_Loop.CUE_Battle_Loop"),
		TEXT("/Game/PathOfAdventure/Loop/WAV/WAV_Battle_Loop.WAV_Battle_Loop"),
		TEXT("/Game/Audio/BGM_Abyssal_Expedition.BGM_Abyssal_Expedition")
	});
	LoadCueForState(EChronicleMusicState::Boss, {
		TEXT("/Game/PathOfAdventure/Loop/CUE/CUE_BossBattle_Loop.CUE_BossBattle_Loop"),
		TEXT("/Game/PathOfAdventure/Loop/WAV/WAV_BossBattle_Loop.WAV_BossBattle_Loop"),
		TEXT("/Game/Audio/BGM_Boss_Dread.BGM_Boss_Dread")
	});

	AmbientLoopSound = SafeLoadSound(TEXT("/Game/Audio/SFX_Amb_Brazier_Loop.SFX_Amb_Brazier_Loop"), true);
	MenuClickSound   = SafeLoadSound(TEXT("/Game/Audio/SFX_UIClick.SFX_UIClick"), false);
	MenuStartSound   = SafeLoadSound(TEXT("/Game/Audio/SFX_Banner_Notify.SFX_Banner_Notify"), false);

	UE_LOG(LogChronicles, Display, TEXT("[Music] Loaded %d/5 orchestral cues (Menu/Village/Field/Battle/Boss)"), GetLoadedCueCount());

	// UI-safe 2D audio decks: bIsUISound = true & bAllowSpatialization = false guarantees playback in menus, UI-only focus, and gameplay!
	auto CreateDeck = [this, &InWorld]() -> UAudioComponent*
	{
		UAudioComponent* Deck = NewObject<UAudioComponent>(this);
		if (Deck)
		{
			Deck->bAutoDestroy = false;
			Deck->bAllowSpatialization = false;
			Deck->bIsUISound = true;
			Deck->bAutoActivate = false;
			Deck->bIgnoreForFlushing = true;
			Deck->RegisterComponentWithWorld(&InWorld);
		}
		return Deck;
	};

	DeckA = CreateDeck();
	DeckB = CreateDeck();
	AmbientDeck = CreateDeck();
	ActiveDeckIndex = 0;

	if (IsValid(AmbientDeck) && IsValid(AmbientLoopSound))
	{
		AmbientDeck->SetSound(AmbientLoopSound);
		const float AmbVol = bMusicMuted ? 0.0f : (0.18f * MusicVolumeMaster);
		if (AmbVol > KINDA_SMALL_NUMBER)
		{
			AmbientDeck->FadeIn(1.5f, AmbVol, 0.0f, EAudioFaderCurve::Logarithmic);
		}
	}

	const bool bAutomatedRun = FParse::Param(FCommandLine::Get(), TEXT("wizardsmoke"))
		|| FParse::Param(FCommandLine::Get(), TEXT("captureframes"));
	if (!bAutomatedRun)
	{
		bMenuMusicActive = true;
		TransitionToState(EChronicleMusicState::Menu, 1.0f, 1.0f);
	}
	else
	{
		bMenuMusicActive = false;
		bIsInVillage = true;
		TransitionToState(EChronicleMusicState::Village, 1.0f, 1.0f);
	}
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
	ReleaseDeck(AmbientDeck);

	for (TObjectPtr<USoundBase>& CueSlot : StateCues)
	{
		CueSlot = nullptr;
	}
	AmbientLoopSound = nullptr;
	MenuClickSound = nullptr;
	MenuStartSound = nullptr;

	CurrentState = EChronicleMusicState::None;
	bMenuMusicActive = false;
	bBossActive = false;

	Super::Deinitialize();
}

void UAetheriaMusicSubsystem::SetMenuMusicActive(bool bInMenu)
{
	bMenuMusicActive = bInMenu;

	if (bMenuMusicActive)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CombatDecayTimerHandle);
		}
		TransitionToState(EChronicleMusicState::Menu, 1.0f, 1.0f);
	}
	else
	{
		const EChronicleMusicState TargetState = bBossActive ? EChronicleMusicState::Boss : GetExplorationState();
		TransitionToState(TargetState, 1.4f, 1.4f);
	}
}

void UAetheriaMusicSubsystem::SetMusicVolume(float NewVolume01)
{
	MusicVolumeMaster = FMath::Clamp(NewVolume01, 0.0f, 1.0f);
	ApplyLiveVolumes();
}

void UAetheriaMusicSubsystem::SetMusicMuted(bool bMute)
{
	bMusicMuted = bMute;
	ApplyLiveVolumes();
}

FString UAetheriaMusicSubsystem::GetCurrentTrackDisplayName() const
{
	switch (CurrentState)
	{
	case EChronicleMusicState::Menu:    return TEXT("Main Theme — Chronicles of the Drake");
	case EChronicleMusicState::Village: return TEXT("Golden Oakhaven Sanctuary");
	case EChronicleMusicState::Field:   return TEXT("Highland Frontier");
	case EChronicleMusicState::Battle:  return TEXT("Crimson Skirmish");
	case EChronicleMusicState::Boss:    return TEXT("Malakor, Sovereign of the Seal");
	case EChronicleMusicState::None:
	default:                            return TEXT("Silence");
	}
}

void UAetheriaMusicSubsystem::PlayMenuClickSFX()
{
	if (bMusicMuted || IsRunningDedicatedServer() || !IsValid(MenuClickSound))
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		const float Vol = FMath::Clamp(0.65f * FMath::Max(0.35f, MusicVolumeMaster), 0.25f, 0.85f);
		UGameplayStatics::PlaySound2D(World, MenuClickSound, Vol);
	}
}

void UAetheriaMusicSubsystem::PlayMenuStartSFX()
{
	if (bMusicMuted || IsRunningDedicatedServer() || !IsValid(MenuStartSound))
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		const float Vol = FMath::Clamp(0.80f * FMath::Max(0.35f, MusicVolumeMaster), 0.30f, 0.95f);
		UGameplayStatics::PlaySound2D(World, MenuStartSound, Vol);
	}
}

int32 UAetheriaMusicSubsystem::GetLoadedCueCount() const
{
	int32 Count = 0;
	for (int32 Idx = static_cast<int32>(EChronicleMusicState::Menu); Idx <= static_cast<int32>(EChronicleMusicState::Boss); ++Idx)
	{
		if (StateCues[Idx] != nullptr)
		{
			++Count;
		}
	}
	return Count;
}

void UAetheriaMusicSubsystem::SetExplorationZone(bool bInVillage)
{
	bIsInVillage = bInVillage;

	if (bMenuMusicActive || bBossActive || CurrentState == EChronicleMusicState::Boss || CurrentState == EChronicleMusicState::Battle)
	{
		return;
	}

	TransitionToState(GetExplorationState());
}

void UAetheriaMusicSubsystem::NotifyCombatActivity()
{
	if (bMenuMusicActive || bBossActive || CurrentState == EChronicleMusicState::Boss)
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

	if (bMenuMusicActive)
	{
		return;
	}

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

USoundBase* UAetheriaMusicSubsystem::SafeLoadSound(const TCHAR* ObjectPath, bool bForceLooping)
{
	if (!ObjectPath || ObjectPath[0] == TEXT('\0'))
	{
		return nullptr;
	}

	const FString PackageName = FPackageName::ObjectPathToPackageName(FString(ObjectPath));
	if (PackageName.IsEmpty() || !FPackageName::DoesPackageExist(PackageName))
	{
		return nullptr;
	}

	USoundBase* Loaded = LoadObject<USoundBase>(nullptr, ObjectPath);
	if (Loaded && bForceLooping)
	{
		if (USoundWave* Wave = Cast<USoundWave>(Loaded))
		{
			Wave->bLooping = true;
		}
	}
	return Loaded;
}

void UAetheriaMusicSubsystem::LoadCueForState(EChronicleMusicState State, const TArray<const TCHAR*>& CandidatePaths)
{
	const int32 StateIndex = static_cast<int32>(State);
	if (StateIndex < 0 || StateIndex >= UE_ARRAY_COUNT(StateCues))
	{
		return;
	}

	for (const TCHAR* CandidatePath : CandidatePaths)
	{
		if (USoundBase* Loaded = SafeLoadSound(CandidatePath, true))
		{
			StateCues[StateIndex] = Loaded;
			return;
		}
	}

	UE_LOG(LogChronicles, Warning, TEXT("UAetheriaMusicSubsystem: No staged music asset found for state %d."), StateIndex);
}

float UAetheriaMusicSubsystem::GetTargetVolumeForState(EChronicleMusicState State) const
{
	if (bMusicMuted)
	{
		return 0.0f;
	}

	float BaseVol = 0.62f;
	switch (State)
	{
	case EChronicleMusicState::Menu:    BaseVol = 0.70f; break;
	case EChronicleMusicState::Village: BaseVol = 0.62f; break;
	case EChronicleMusicState::Field:   BaseVol = 0.62f; break;
	case EChronicleMusicState::Battle:  BaseVol = 0.68f; break;
	case EChronicleMusicState::Boss:    BaseVol = 0.78f; break;
	case EChronicleMusicState::None:
	default:                            BaseVol = 0.0f;  break;
	}

	return BaseVol * FMath::Clamp(MusicVolumeMaster, 0.0f, 1.0f);
}

void UAetheriaMusicSubsystem::ApplyLiveVolumes()
{
	const float TargetVol = GetTargetVolumeForState(CurrentState);
	UAudioComponent* ActiveDeck = (ActiveDeckIndex == 0) ? DeckA.Get() : DeckB.Get();
	if (IsValid(ActiveDeck))
	{
		if (!bMusicMuted && TargetVol > KINDA_SMALL_NUMBER && !ActiveDeck->IsPlaying() && CurrentState != EChronicleMusicState::None)
		{
			const int32 StateIndex = static_cast<int32>(CurrentState);
			USoundBase* ActiveCue = (StateIndex >= 0 && StateIndex < UE_ARRAY_COUNT(StateCues)) ? StateCues[StateIndex].Get() : nullptr;
			if (IsValid(ActiveCue))
			{
				ActiveDeck->SetSound(ActiveCue);
				ActiveDeck->FadeIn(0.6f, TargetVol, 0.0f, EAudioFaderCurve::Logarithmic);
			}
		}
		else
		{
			ActiveDeck->SetVolumeMultiplier(TargetVol);
		}
	}

	if (IsValid(AmbientDeck) && IsValid(AmbientLoopSound))
	{
		const float AmbVol = bMusicMuted ? 0.0f : (0.18f * FMath::Clamp(MusicVolumeMaster, 0.0f, 1.0f));
		if (!bMusicMuted && AmbVol > KINDA_SMALL_NUMBER && !AmbientDeck->IsPlaying())
		{
			AmbientDeck->SetSound(AmbientLoopSound);
			AmbientDeck->FadeIn(0.8f, AmbVol, 0.0f, EAudioFaderCurve::Logarithmic);
		}
		else
		{
			AmbientDeck->SetVolumeMultiplier(AmbVol);
		}
	}
}

void UAetheriaMusicSubsystem::TransitionToState(EChronicleMusicState NewState, float OverrideFadeIn, float OverrideFadeOut)
{
	if (NewState == CurrentState)
	{
		return;
	}

	const bool bIsCombat = (NewState == EChronicleMusicState::Battle || NewState == EChronicleMusicState::Boss);
	const float InFade = (OverrideFadeIn >= 0.0f) ? OverrideFadeIn : (bIsCombat ? 0.85f : 1.75f);
	const float OutFade = (OverrideFadeOut >= 0.0f) ? OverrideFadeOut : (bIsCombat ? 1.15f : 2.00f);
	const float TargetVolume = GetTargetVolumeForState(NewState);

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
		if (!bMusicMuted && TargetVolume > KINDA_SMALL_NUMBER)
		{
			IncomingDeck->FadeIn(InFade, TargetVolume, 0.0f, EAudioFaderCurve::Logarithmic);
		}
		else
		{
			IncomingDeck->SetVolumeMultiplier(0.0f);
		}
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
				if (!StrongThis->bBossActive && !StrongThis->bMenuMusicActive)
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
