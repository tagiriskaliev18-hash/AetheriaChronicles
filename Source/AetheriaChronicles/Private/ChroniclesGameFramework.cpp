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

#include "Brushes/SlateDynamicImageBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/FileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Math/RotationMatrix.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
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
	// --- Authentic Dark-Fantasy Crimson Desert / Diablo IV Obsidian & Antique Gold Palette ---
	static const FLinearColor PanelBg(0.045f, 0.040f, 0.052f, 0.90f);
	static const FLinearColor PanelBgDeep(0.026f, 0.022f, 0.032f, 0.94f);
	static const FLinearColor RibbonBg(0.034f, 0.029f, 0.042f, 0.82f);
	static const FLinearColor ModalScrim(0.012f, 0.010f, 0.018f, 0.76f);
	static const FLinearColor CardBg(0.058f, 0.050f, 0.066f, 0.92f);
	static const FLinearColor CardBgLocked(0.036f, 0.033f, 0.044f, 0.78f);
	static const FLinearColor TrackRecessed(0.030f, 0.025f, 0.035f, 0.92f);

	static const FLinearColor HairlineGold(0.76f, 0.59f, 0.28f, 0.85f);
	static const FLinearColor HairlineGoldSubtle(0.76f, 0.59f, 0.28f, 0.65f);
	static const FLinearColor HairlineBronze(0.46f, 0.35f, 0.18f, 0.72f);
	static const FLinearColor HairlineCyan(0.16f, 0.82f, 0.96f, 0.72f);
	static const FLinearColor HairlineCrimson(0.86f, 0.16f, 0.18f, 0.88f);

	static const FLinearColor TextPrimary(0.95f, 0.92f, 0.86f, 1.0f);
	static const FLinearColor TextSecondary(0.72f, 0.68f, 0.63f, 1.0f);
	static const FLinearColor TextGold(0.96f, 0.80f, 0.42f, 1.0f);
	static const FLinearColor TextCyan(0.24f, 0.90f, 0.98f, 1.0f);
	static const FLinearColor TextCrimson(0.96f, 0.26f, 0.24f, 1.0f);
	static const FLinearColor TextEmerald(0.30f, 0.88f, 0.56f, 1.0f);
	static const FLinearColor TextAmber(0.98f, 0.72f, 0.24f, 1.0f);

	// Semantic Progress Bar Fills
	static const FLinearColor BarHP(0.722f, 0.114f, 0.141f, 1.0f);       // Deep Crimson #B81D24
	static const FLinearColor BarMana(0.118f, 0.431f, 0.910f, 1.0f);     // Sapphire Arcana #1E6EE8
	static const FLinearColor BarStamina(0.224f, 0.659f, 0.420f, 1.0f);  // Emerald-Gold #39A86B
	static const FLinearColor BarXP(0.831f, 0.659f, 0.263f, 1.0f);       // Burnished Gold #D4A843
	static const FLinearColor BarDragonFire(0.95f, 0.38f, 0.08f, 1.0f);  // Molten Drake Flame
	static const FLinearColor BarBossHP(0.76f, 0.09f, 0.12f, 1.0f);      // Blood-Crimson
	static const FLinearColor BarStagger(0.910f, 0.608f, 0.141f, 1.0f);  // Molten Amber #E89B24

	static const FLinearColor OrbExort(0.98f, 0.42f, 0.12f, 1.0f);
	static const FLinearColor OrbWex(0.68f, 0.36f, 1.00f, 1.0f);
	static const FLinearColor OrbQuas(0.20f, 0.86f, 1.00f, 1.0f);
	static const FLinearColor OrbLocked(0.28f, 0.26f, 0.32f, 0.78f);

	static const FSlateBrush* WhiteBrush()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}

	/** Lazily-initialized runtime cache of custom TTF fonts, PNG textures, sculpted brushes, buttons & bar styles. */
	struct FChroniclesUIStyleCache
	{
		bool bInitialized = false;
		bool bPreExitBound = false;

		TSharedPtr<const FCompositeFont> DisplayFontData;
		TSharedPtr<const FCompositeFont> SerifFontData;

		TArray<TWeakObjectPtr<UTexture2D>> RootedTextures;

		TSharedPtr<FSlateDynamicImageBrush> PanelObsidianDyn;
		TSharedPtr<FSlateDynamicImageBrush> BtnStoneNormDyn;
		TSharedPtr<FSlateDynamicImageBrush> BtnStoneHovDyn;
		TSharedPtr<FSlateDynamicImageBrush> BtnStonePressDyn;
		TSharedPtr<FSlateDynamicImageBrush> BtnCrimsonNormDyn;
		TSharedPtr<FSlateDynamicImageBrush> BtnCrimsonHovDyn;
		TSharedPtr<FSlateDynamicImageBrush> BtnCrimsonPressDyn;
		TSharedPtr<FSlateDynamicImageBrush> SocketNormDyn;
		TSharedPtr<FSlateDynamicImageBrush> SocketArmedDyn;
		TSharedPtr<FSlateDynamicImageBrush> OrbIgnisDyn;
		TSharedPtr<FSlateDynamicImageBrush> OrbTempestDyn;
		TSharedPtr<FSlateDynamicImageBrush> OrbGravitonDyn;
		TSharedPtr<FSlateDynamicImageBrush> CrestSkullDyn;
		TSharedPtr<FSlateDynamicImageBrush> MenuVignetteDyn;
		TSharedPtr<FSlateDynamicImageBrush> MinimapFrameDyn;

		// Sculpted FSlateRoundedBoxBrush fallbacks & card brushes
		TSharedPtr<FSlateRoundedBoxBrush> PanelObsidianFallback;
		TSharedPtr<FSlateRoundedBoxBrush> CardObsidianBrush;
		TSharedPtr<FSlateRoundedBoxBrush> CardObsidianLockedBrush;
		TSharedPtr<FSlateRoundedBoxBrush> RibbonObsidianBrush;
		TSharedPtr<FSlateRoundedBoxBrush> SocketNormFallback;
		TSharedPtr<FSlateRoundedBoxBrush> SocketArmedFallback;
		TSharedPtr<FSlateRoundedBoxBrush> MinimapDiscBrush;
		TSharedPtr<FSlateRoundedBoxBrush> MinimapInnerRingBrush;
		TSharedPtr<FSlateRoundedBoxBrush> BlipCircleBrush;
		TSharedPtr<FSlateRoundedBoxBrush> WorldPillBrush;

		FButtonStyle StoneBtnStyle;
		FButtonStyle CrimsonBtnStyle;
		FButtonStyle ActiveGoldBtnStyle;

		FProgressBarStyle RecessedBaseBarStyle;
		FProgressBarStyle HPBarStyle;
		FProgressBarStyle ManaBarStyle;
		FProgressBarStyle StaminaBarStyle;
		FProgressBarStyle XPBarStyle;
		FProgressBarStyle BossHPStyle;
		FProgressBarStyle BossStaggerStyle;
		FProgressBarStyle DragonFireStyle;

		UTexture2D* LoadPNGTexture(const FString& RelPath)
		{
			if (!FSlateApplication::IsInitialized() || IsEngineExitRequested() || IsRunningCommandlet())
			{
				return nullptr;
			}

			const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / RelPath);
			if (!IFileManager::Get().FileExists(*FullPath))
			{
				return nullptr;
			}

			UTexture2D* Tex = FImageUtils::ImportFileAsTexture2D(FullPath);
			if (!Tex)
			{
				// Secondary direct IImageWrapperModule decode fallback
				TArray<uint8> FileData;
				if (FFileHelper::LoadFileToArray(FileData, *FullPath) && !FileData.IsEmpty())
				{
					IImageWrapperModule& ImgMod = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName(TEXT("ImageWrapper")));
					TSharedPtr<IImageWrapper> Wrapper = ImgMod.CreateImageWrapper(EImageFormat::PNG);
					if (Wrapper.IsValid() && Wrapper->SetCompressed(FileData.GetData(), FileData.Num()))
					{
						TArray64<uint8> RawBGRA;
						if (Wrapper->GetRaw(ERGBFormat::BGRA, 8, RawBGRA) && Wrapper->GetWidth() > 0 && Wrapper->GetHeight() > 0)
						{
							Tex = UTexture2D::CreateTransient(Wrapper->GetWidth(), Wrapper->GetHeight(), PF_B8G8R8A8);
							if (Tex && Tex->GetPlatformData() && !Tex->GetPlatformData()->Mips.IsEmpty())
							{
								void* MipData = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
								if (MipData)
								{
									const int64 ByteCount = FMath::Min<int64>(RawBGRA.Num(), static_cast<int64>(Wrapper->GetWidth()) * Wrapper->GetHeight() * 4);
									FMemory::Memcpy(MipData, RawBGRA.GetData(), ByteCount);
									Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
								}
								Tex->SRGB = true;
								Tex->UpdateResource();
							}
						}
					}
				}
			}

			if (Tex)
			{
				Tex->LODGroup = TEXTUREGROUP_UI;
				Tex->NeverStream = true;
				Tex->Filter = TF_Bilinear;
				Tex->AddToRoot();
				RootedTextures.Add(Tex);
			}
			return Tex;
		}

		void LoadBrush(
			TSharedPtr<FSlateDynamicImageBrush>& OutBrush,
			const TCHAR* ResourceId,
			const TCHAR* RelPath,
			const FVector2f& Size,
			ESlateBrushDrawType::Type DrawAs = ESlateBrushDrawType::Image,
			const FMargin& Margin = FMargin(0.f))
		{
			if (UTexture2D* Tex = LoadPNGTexture(RelPath))
			{
				OutBrush = MakeShareable(new FSlateDynamicImageBrush(Tex, Size, FName(ResourceId)));
				OutBrush->DrawAs = DrawAs;
				OutBrush->Margin = Margin;
			}
		}

		static FProgressBarStyle BuildBarStyle(const FLinearColor& FillTint)
		{
			FProgressBarStyle Style = FProgressBarStyle::GetDefault();
			FSlateRoundedBoxBrush BgBrush(
				TrackRecessed,
				2.5f,
				FLinearColor(0.38f, 0.29f, 0.16f, 0.65f),
				1.0f);
			FSlateRoundedBoxBrush FillBrush(
				FillTint,
				2.0f,
				FLinearColor(FMath::Min(1.f, FillTint.R * 1.25f), FMath::Min(1.f, FillTint.G * 1.25f), FMath::Min(1.f, FillTint.B * 1.25f), 0.35f),
				0.5f);
			Style.SetBackgroundImage(BgBrush);
			Style.SetFillImage(FillBrush);
			Style.SetMarqueeImage(FillBrush);
			Style.SetEnableFillAnimation(false);
			return Style;
		}

		void ReleaseOnExit()
		{
			for (TWeakObjectPtr<UTexture2D>& WeakTex : RootedTextures)
			{
				if (UTexture2D* Tex = WeakTex.Get())
				{
					Tex->RemoveFromRoot();
				}
			}
			RootedTextures.Reset();
			PanelObsidianDyn.Reset();
			BtnStoneNormDyn.Reset();
			BtnStoneHovDyn.Reset();
			BtnStonePressDyn.Reset();
			BtnCrimsonNormDyn.Reset();
			BtnCrimsonHovDyn.Reset();
			BtnCrimsonPressDyn.Reset();
			SocketNormDyn.Reset();
			SocketArmedDyn.Reset();
			OrbIgnisDyn.Reset();
			OrbTempestDyn.Reset();
			OrbGravitonDyn.Reset();
			CrestSkullDyn.Reset();
			MenuVignetteDyn.Reset();
			MinimapFrameDyn.Reset();
			DisplayFontData.Reset();
			SerifFontData.Reset();
			bInitialized = false;
		}

		void EnsureInitialized()
		{
			if (bInitialized)
			{
				return;
			}
			bInitialized = true;

			if (!bPreExitBound)
			{
				bPreExitBound = true;
				FCoreDelegates::OnPreExit.AddLambda([this]()
				{
					ReleaseOnExit();
				});
			}

			// 1. Custom Dark-Fantasy Fonts (AetheriaDisplay-Bold.ttf & AetheriaSerif-SemiBold.ttf)
			const FString DisplayPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Fonts/AetheriaDisplay-Bold.ttf"));
			if (IFileManager::Get().FileExists(*DisplayPath))
			{
				DisplayFontData = MakeShared<FStandaloneCompositeFont>(
					FName(TEXT("AetheriaDisplayBold")), DisplayPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			}

			const FString SerifPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Fonts/AetheriaSerif-SemiBold.ttf"));
			if (IFileManager::Get().FileExists(*SerifPath))
			{
				SerifFontData = MakeShared<FStandaloneCompositeFont>(
					FName(TEXT("AetheriaSerifSemiBold")), SerifPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			}

			// 2. Sculpted Fallback Brushes
			PanelObsidianFallback = MakeShared<FSlateRoundedBoxBrush>(PanelBg, 4.f, HairlineGold, 1.5f);
			CardObsidianBrush = MakeShared<FSlateRoundedBoxBrush>(CardBg, 3.f, HairlineBronze, 1.0f);
			CardObsidianLockedBrush = MakeShared<FSlateRoundedBoxBrush>(CardBgLocked, 3.f, FLinearColor(0.24f, 0.22f, 0.27f, 0.55f), 1.0f);
			RibbonObsidianBrush = MakeShared<FSlateRoundedBoxBrush>(RibbonBg, 3.f, HairlineGoldSubtle, 1.0f);
			SocketNormFallback = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(0.032f, 0.028f, 0.040f, 0.94f), 3.f, HairlineBronze, 1.5f);
			SocketArmedFallback = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(0.048f, 0.036f, 0.028f, 0.96f), 3.f, HairlineGold, 2.0f);
			MinimapDiscBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(0.030f, 0.028f, 0.040f, 0.88f), 74.f, HairlineGoldSubtle, 1.5f);
			MinimapInnerRingBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(0.f, 0.f, 0.f, 0.f), 44.f, FLinearColor(0.76f, 0.59f, 0.28f, 0.24f), 1.0f);
			BlipCircleBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, 5.f, FLinearColor(0.02f, 0.018f, 0.028f, 0.90f), 1.0f);
			WorldPillBrush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor(0.032f, 0.028f, 0.042f, 0.80f), 4.f, FLinearColor(0.76f, 0.59f, 0.28f, 0.48f), 1.0f);

			// 3. Load Custom UI Art PNGs from RawAssets/UI/
			LoadBrush(PanelObsidianDyn,   TEXT("Chronicles_PanelObsidian"),    TEXT("RawAssets/UI/T_UI_PanelObsidian.png"),      FVector2f(512.f, 512.f), ESlateBrushDrawType::Box, FMargin(0.10f));
			LoadBrush(BtnStoneNormDyn,    TEXT("Chronicles_BtnStoneNorm"),     TEXT("RawAssets/UI/T_UI_BtnStone_Normal.png"),    FVector2f(384.f, 88.f),  ESlateBrushDrawType::Box, FMargin(0.12f, 0.22f));
			LoadBrush(BtnStoneHovDyn,     TEXT("Chronicles_BtnStoneHov"),      TEXT("RawAssets/UI/T_UI_BtnStone_Hovered.png"),   FVector2f(384.f, 88.f),  ESlateBrushDrawType::Box, FMargin(0.12f, 0.22f));
			LoadBrush(BtnStonePressDyn,   TEXT("Chronicles_BtnStonePress"),    TEXT("RawAssets/UI/T_UI_BtnStone_Pressed.png"),   FVector2f(384.f, 88.f),  ESlateBrushDrawType::Box, FMargin(0.12f, 0.22f));
			LoadBrush(BtnCrimsonNormDyn,  TEXT("Chronicles_BtnCrimsonNorm"),   TEXT("RawAssets/UI/T_UI_BtnCrimson_Normal.png"),  FVector2f(384.f, 88.f),  ESlateBrushDrawType::Box, FMargin(0.12f, 0.22f));
			LoadBrush(BtnCrimsonHovDyn,   TEXT("Chronicles_BtnCrimsonHov"),    TEXT("RawAssets/UI/T_UI_BtnCrimson_Hovered.png"), FVector2f(384.f, 88.f),  ESlateBrushDrawType::Box, FMargin(0.12f, 0.22f));
			LoadBrush(BtnCrimsonPressDyn, TEXT("Chronicles_BtnCrimsonPress"),  TEXT("RawAssets/UI/T_UI_BtnCrimson_Pressed.png"), FVector2f(384.f, 88.f),  ESlateBrushDrawType::Box, FMargin(0.12f, 0.22f));
			LoadBrush(SocketNormDyn,      TEXT("Chronicles_SocketNorm"),       TEXT("RawAssets/UI/T_UI_SkillSocket_Normal.png"), FVector2f(128.f, 128.f), ESlateBrushDrawType::Box, FMargin(0.18f));
			LoadBrush(SocketArmedDyn,     TEXT("Chronicles_SocketArmed"),      TEXT("RawAssets/UI/T_UI_SkillSocket_Armed.png"),  FVector2f(128.f, 128.f), ESlateBrushDrawType::Box, FMargin(0.18f));
			LoadBrush(OrbIgnisDyn,        TEXT("Chronicles_OrbIgnis"),         TEXT("RawAssets/UI/icon_orb_ignis.png"),          FVector2f(128.f, 128.f));
			LoadBrush(OrbTempestDyn,      TEXT("Chronicles_OrbTempest"),       TEXT("RawAssets/UI/icon_orb_tempest.png"),        FVector2f(128.f, 128.f));
			LoadBrush(OrbGravitonDyn,     TEXT("Chronicles_OrbGraviton"),      TEXT("RawAssets/UI/icon_orb_graviton.png"),       FVector2f(128.f, 128.f));
			LoadBrush(CrestSkullDyn,      TEXT("Chronicles_CrestSkull"),       TEXT("RawAssets/UI/Icons/T_UI_Crest_Skull.png"),  FVector2f(64.f, 64.f));
			if (!CrestSkullDyn.IsValid())
			{
				LoadBrush(CrestSkullDyn,  TEXT("Chronicles_CrestSkull"),       TEXT("RawAssets/UI/T_UI_Crest_Skull.png"),        FVector2f(64.f, 64.f));
			}
			LoadBrush(MenuVignetteDyn,    TEXT("Chronicles_MenuVignette"),     TEXT("RawAssets/UI/T_UI_MenuVignette.png"),       FVector2f(1024.f, 1024.f));
			LoadBrush(MinimapFrameDyn,    TEXT("Chronicles_MinimapFrame"),     TEXT("RawAssets/UI/hud_minimap_frame.png"),       FVector2f(172.f, 172.f));

			// 4. Sculpted Stone Button Style (Carved Obsidian + Antique Gold Hover + Warm Bronze Press)
			StoneBtnStyle = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("Button"));
			if (BtnStoneNormDyn.IsValid() && BtnStoneHovDyn.IsValid() && BtnStonePressDyn.IsValid())
			{
				StoneBtnStyle.SetNormal(*BtnStoneNormDyn);
				StoneBtnStyle.SetHovered(*BtnStoneHovDyn);
				StoneBtnStyle.SetPressed(*BtnStonePressDyn);
				FSlateBrush DisabledBrush = *BtnStoneNormDyn;
				DisabledBrush.TintColor = FSlateColor(FLinearColor(0.45f, 0.43f, 0.48f, 0.55f));
				StoneBtnStyle.SetDisabled(DisabledBrush);
			}
			else
			{
				StoneBtnStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.085f, 0.078f, 0.098f, 0.96f), 3.f, HairlineBronze, 1.2f));
				StoneBtnStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.145f, 0.125f, 0.110f, 0.98f), 3.f, HairlineGold, 1.5f));
				StoneBtnStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.180f, 0.135f, 0.085f, 0.98f), 3.f, TextGold, 1.5f));
				StoneBtnStyle.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.042f, 0.040f, 0.050f, 0.65f), 3.f, FLinearColor(0.22f, 0.20f, 0.24f, 0.45f), 1.0f));
			}
			StoneBtnStyle.SetNormalForeground(FSlateColor(TextPrimary));
			StoneBtnStyle.SetHoveredForeground(FSlateColor(TextGold));
			StoneBtnStyle.SetPressedForeground(FSlateColor(FLinearColor(1.0f, 0.88f, 0.56f, 1.0f)));
			StoneBtnStyle.SetDisabledForeground(FSlateColor(FLinearColor(0.46f, 0.44f, 0.44f, 0.72f)));
			StoneBtnStyle.SetNormalPadding(FMargin(10.f, 5.f));
			StoneBtnStyle.SetPressedPadding(FMargin(10.f, 6.f, 10.f, 4.f));

			// 5. Forged Crimson CTA Button Style (Primary Confirm / Save / Close / Respawn)
			CrimsonBtnStyle = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("Button"));
			if (BtnCrimsonNormDyn.IsValid() && BtnCrimsonHovDyn.IsValid() && BtnCrimsonPressDyn.IsValid())
			{
				CrimsonBtnStyle.SetNormal(*BtnCrimsonNormDyn);
				CrimsonBtnStyle.SetHovered(*BtnCrimsonHovDyn);
				CrimsonBtnStyle.SetPressed(*BtnCrimsonPressDyn);
				FSlateBrush DisabledCrimson = *BtnCrimsonNormDyn;
				DisabledCrimson.TintColor = FSlateColor(FLinearColor(0.42f, 0.34f, 0.36f, 0.55f));
				CrimsonBtnStyle.SetDisabled(DisabledCrimson);
			}
			else
			{
				CrimsonBtnStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.36f, 0.075f, 0.085f, 0.96f), 3.f, HairlineGold, 1.3f));
				CrimsonBtnStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.52f, 0.110f, 0.120f, 0.98f), 3.f, TextGold, 1.6f));
				CrimsonBtnStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.28f, 0.055f, 0.065f, 0.98f), 3.f, HairlineBronze, 1.5f));
				CrimsonBtnStyle.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.14f, 0.055f, 0.060f, 0.60f), 3.f, FLinearColor(0.28f, 0.22f, 0.20f, 0.45f), 1.0f));
			}
			CrimsonBtnStyle.SetNormalForeground(FSlateColor(FLinearColor(0.98f, 0.91f, 0.76f, 1.0f)));
			CrimsonBtnStyle.SetHoveredForeground(FSlateColor(FLinearColor(1.0f, 0.97f, 0.88f, 1.0f)));
			CrimsonBtnStyle.SetPressedForeground(FSlateColor(TextGold));
			CrimsonBtnStyle.SetDisabledForeground(FSlateColor(FLinearColor(0.55f, 0.48f, 0.46f, 0.70f)));
			CrimsonBtnStyle.SetNormalPadding(FMargin(12.f, 5.f));
			CrimsonBtnStyle.SetPressedPadding(FMargin(12.f, 6.f, 12.f, 4.f));

			// 6. Active Burnished-Gold Selected Button Style (Selected Tabs / Chips / Active Modes)
			ActiveGoldBtnStyle = StoneBtnStyle;
			ActiveGoldBtnStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.165f, 0.128f, 0.068f, 0.96f), 3.f, HairlineGold, 1.6f));
			ActiveGoldBtnStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.215f, 0.165f, 0.082f, 0.98f), 3.f, TextGold, 1.8f));
			ActiveGoldBtnStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.140f, 0.105f, 0.055f, 0.98f), 3.f, HairlineGold, 1.5f));
			ActiveGoldBtnStyle.SetNormalForeground(FSlateColor(TextGold));
			ActiveGoldBtnStyle.SetHoveredForeground(FSlateColor(FLinearColor(1.0f, 0.94f, 0.72f, 1.0f)));
			ActiveGoldBtnStyle.SetPressedForeground(FSlateColor(TextPrimary));

			// 7. Recessed Obsidian Progress Bar Styles
			RecessedBaseBarStyle = BuildBarStyle(FLinearColor::White);
			HPBarStyle           = BuildBarStyle(BarHP);
			ManaBarStyle         = BuildBarStyle(BarMana);
			StaminaBarStyle      = BuildBarStyle(BarStamina);
			XPBarStyle           = BuildBarStyle(BarXP);
			BossHPStyle          = BuildBarStyle(BarBossHP);
			BossStaggerStyle     = BuildBarStyle(BarStagger);
			DragonFireStyle      = BuildBarStyle(BarDragonFire);
		}
	};

	static FChroniclesUIStyleCache& GetStyles()
	{
		static FChroniclesUIStyleCache Cache;
		Cache.EnsureInitialized();
		return Cache;
	}

	static FSlateFontInfo FontBold(int32 Size)
	{
		FChroniclesUIStyleCache& Styles = GetStyles();
		const FFontOutlineSettings Outline(1, FLinearColor(0.02f, 0.015f, 0.02f, 0.88f));
		if (Styles.DisplayFontData.IsValid())
		{
			return FSlateFontInfo(Styles.DisplayFontData, static_cast<float>(Size), NAME_None, Outline);
		}
		FSlateFontInfo Fallback = FCoreStyle::GetDefaultFontStyle("Bold", Size);
		Fallback.OutlineSettings = Outline;
		return Fallback;
	}

	static FSlateFontInfo FontRegular(int32 Size)
	{
		FChroniclesUIStyleCache& Styles = GetStyles();
		const FFontOutlineSettings Outline(1, FLinearColor(0.02f, 0.015f, 0.02f, 0.88f));
		if (Styles.SerifFontData.IsValid())
		{
			return FSlateFontInfo(Styles.SerifFontData, static_cast<float>(Size), NAME_None, Outline);
		}
		FSlateFontInfo Fallback = FCoreStyle::GetDefaultFontStyle("Regular", Size);
		Fallback.OutlineSettings = Outline;
		return Fallback;
	}

	static const FSlateBrush* PanelObsidianBrush()
	{
		FChroniclesUIStyleCache& S = GetStyles();
		return S.PanelObsidianDyn.IsValid() ? static_cast<const FSlateBrush*>(S.PanelObsidianDyn.Get()) : S.PanelObsidianFallback.Get();
	}

	static const FSlateBrush* ObsidianCardBrush()
	{
		return GetStyles().CardObsidianBrush.Get();
	}

	static const FSlateBrush* ObsidianCardLockedBrush()
	{
		return GetStyles().CardObsidianLockedBrush.Get();
	}

	static const FSlateBrush* RibbonObsidianBrush()
	{
		return GetStyles().RibbonObsidianBrush.Get();
	}

	static const FSlateBrush* SocketNormalBrush()
	{
		FChroniclesUIStyleCache& S = GetStyles();
		return S.SocketNormDyn.IsValid() ? static_cast<const FSlateBrush*>(S.SocketNormDyn.Get()) : S.SocketNormFallback.Get();
	}

	static const FSlateBrush* SocketArmedBrush()
	{
		FChroniclesUIStyleCache& S = GetStyles();
		return S.SocketArmedDyn.IsValid() ? static_cast<const FSlateBrush*>(S.SocketArmedDyn.Get()) : S.SocketArmedFallback.Get();
	}

	static const FSlateBrush* OrbIconBrush(EChronicleOrb Orb)
	{
		FChroniclesUIStyleCache& S = GetStyles();
		switch (Orb)
		{
		case EChronicleOrb::Exort: return S.OrbIgnisDyn.IsValid() ? S.OrbIgnisDyn.Get() : nullptr;
		case EChronicleOrb::Wex:   return S.OrbGravitonDyn.IsValid() ? S.OrbGravitonDyn.Get() : nullptr;
		case EChronicleOrb::Quas:  return S.OrbTempestDyn.IsValid() ? S.OrbTempestDyn.Get() : nullptr;
		default:                   return nullptr;
		}
	}

	static const FSlateBrush* SkullCrestBrush()
	{
		FChroniclesUIStyleCache& S = GetStyles();
		return S.CrestSkullDyn.IsValid() ? S.CrestSkullDyn.Get() : nullptr;
	}

	static const FSlateBrush* MenuVignetteBrush()
	{
		FChroniclesUIStyleCache& S = GetStyles();
		return S.MenuVignetteDyn.IsValid() ? S.MenuVignetteDyn.Get() : nullptr;
	}

	static const FSlateBrush* MinimapFrameBrush()
	{
		FChroniclesUIStyleCache& S = GetStyles();
		return S.MinimapFrameDyn.IsValid() ? S.MinimapFrameDyn.Get() : nullptr;
	}

	static const FSlateBrush* MinimapDiscBrush()      { return GetStyles().MinimapDiscBrush.Get(); }
	static const FSlateBrush* MinimapInnerRingBrush() { return GetStyles().MinimapInnerRingBrush.Get(); }
	static const FSlateBrush* BlipCircleBrush()       { return GetStyles().BlipCircleBrush.Get(); }
	static const FSlateBrush* WorldPillBrush()        { return GetStyles().WorldPillBrush.Get(); }

	static const FButtonStyle& StoneButtonStyle()      { return GetStyles().StoneBtnStyle; }
	static const FButtonStyle& CrimsonButtonStyle()    { return GetStyles().CrimsonBtnStyle; }
	static const FButtonStyle& ActiveGoldButtonStyle() { return GetStyles().ActiveGoldBtnStyle; }

	static const FProgressBarStyle& RecessedBarBaseStyle() { return GetStyles().RecessedBaseBarStyle; }
	static const FProgressBarStyle& HealthBarStyle()       { return GetStyles().HPBarStyle; }
	static const FProgressBarStyle& ManaBarStyle()         { return GetStyles().ManaBarStyle; }
	static const FProgressBarStyle& StaminaBarStyle()      { return GetStyles().StaminaBarStyle; }
	static const FProgressBarStyle& XPBarStyle()           { return GetStyles().XPBarStyle; }
	static const FProgressBarStyle& BossHPBarStyle()       { return GetStyles().BossHPStyle; }
	static const FProgressBarStyle& BossStaggerBarStyle()  { return GetStyles().BossStaggerStyle; }
	static const FProgressBarStyle& DragonFireBarStyle()   { return GetStyles().DragonFireStyle; }

	static const FQuestDef* ResolveQuestForNPC(const UQuestSubsystem* QS, const AAetheriaNPCActor* NPC)
	{
		if (!QS || !NPC)
		{
			return nullptr;
		}
		if (const FQuestDef* Direct = QS->FindQuest(NPC->QuestId))
		{
			return Direct;
		}
		return QS->FindQuestForGiver(NPC->NpcId);
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

namespace ChroniclesStudioBg
{
	static TMap<TWeakObjectPtr<AChroniclesPlayerController>, TArray<TWeakObjectPtr<AActor>>>& HiddenActorsByPC()
	{
		static TMap<TWeakObjectPtr<AChroniclesPlayerController>, TArray<TWeakObjectPtr<AActor>>> Map;
		return Map;
	}

	static void RestoreHiddenActors(AChroniclesPlayerController* PC)
	{
		TMap<TWeakObjectPtr<AChroniclesPlayerController>, TArray<TWeakObjectPtr<AActor>>>& Map = HiddenActorsByPC();
		for (auto It = Map.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}

		if (!PC)
		{
			return;
		}
		const TWeakObjectPtr<AChroniclesPlayerController> Key(PC);
		if (TArray<TWeakObjectPtr<AActor>>* List = Map.Find(Key))
		{
			for (const TWeakObjectPtr<AActor>& WeakActor : *List)
			{
				if (AActor* Actor = WeakActor.Get())
				{
					Actor->SetActorHiddenInGame(false);
				}
			}
			Map.Remove(Key);
		}
	}

	static void SetNearbyBystandersHidden(AChroniclesPlayerController* PC, bool bHide, float RadiusCm = 4500.f)
	{
		if (!PC)
		{
			return;
		}
		RestoreHiddenActors(PC);
		if (!bHide)
		{
			return;
		}

		UWorld* World = PC->GetWorld();
		const AAetheriaHeroCharacter* Hero = PC->GetActiveHero();
		if (!World || !Hero)
		{
			return;
		}

		const FVector Center = Hero->GetActorLocation();
		const float RadiusSq = FMath::Square(FMath::Max(100.f, RadiusCm));
		TArray<TWeakObjectPtr<AActor>> NewlyHidden;

		for (TActorIterator<AAetheriaEnemyCharacter> It(World); It; ++It)
		{
			AAetheriaEnemyCharacter* Enemy = *It;
			if (IsValid(Enemy) && !Enemy->IsHidden() && FVector::DistSquared(Center, Enemy->GetActorLocation()) <= RadiusSq)
			{
				Enemy->SetActorHiddenInGame(true);
				NewlyHidden.Add(Enemy);
			}
		}

		for (TActorIterator<AAetheriaNPCActor> It(World); It; ++It)
		{
			AAetheriaNPCActor* NPC = *It;
			if (IsValid(NPC) && !NPC->IsHidden() && FVector::DistSquared(Center, NPC->GetActorLocation()) <= RadiusSq)
			{
				NPC->SetActorHiddenInGame(true);
				NewlyHidden.Add(NPC);
			}
		}

		if (!NewlyHidden.IsEmpty())
		{
			HiddenActorsByPC().Add(TWeakObjectPtr<AChroniclesPlayerController>(PC), MoveTemp(NewlyHidden));
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

	// Dedicated Prologue Lore Tutor NPC inside the Awakening Cave overlooking Mirror Lake
	{
		const FVector TutorFloor = WorldBuilder
			? WorldBuilder->GetPrologueTutorLocation()
			: FVector(-2920.f, -620.f, SampleGroundZSafe(-2920.f, -620.f, 1800.f) + 2.f);
		// AAetheriaNPCActor capsule half-height is 92 cm -> Floor + 92 places feet flush on cave stone
		const FVector TutorLoc = TutorFloor + FVector(0.f, 0.f, 92.f);
		const FRotator TutorRot(0.f, 165.f, 0.f);

		if (AAetheriaNPCActor* CaveTutor = World->SpawnActor<AAetheriaNPCActor>(AAetheriaNPCActor::StaticClass(), TutorLoc, TutorRot, SpawnParams))
		{
			CaveTutor->ConfigureNPC(
				FName(TEXT("Npc_CaveTutor")),
				FText::FromString(TEXT("Хранитель Этерии Вейлен")),
				FText::FromString(TEXT("Наставник Пробуждённых")),
				FName(TEXT("Q_PrologueAwakening")),
				0);
			SpawnedNPCs.Add(CaveTutor);
		}
	}

	UE_LOG(LogChronicles, Log, TEXT("Spawned %d Named Story Quest NPCs across Aetheria (including Cave Tutor)."), SpawnedNPCs.Num());
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
		// Disable raw engine UConsole interception on ` / ~ / ё so our Slate Developer Console owns the key cleanly
		if (UInputSettings* InputSettings = UInputSettings::GetInputSettings())
		{
			InputSettings->ConsoleKeys.Empty();
		}
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* Viewport = World->GetGameViewport())
			{
				Viewport->ViewportConsole = nullptr;
			}
		}

		if (ConsoleLogLines.Num() == 0)
		{
			AppendConsoleLogLine(
				TEXT("Aetheria Arcane Console — введите /godmode для включения Режима Бога (Бессмертие + 10/10 заклинаний) или /help"),
				ChroniclesHUDPalette::TextGold);
		}

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
			OpenMainMenuOverlay();
		}
	}
}

void AChroniclesPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ChroniclesStudioBg::RestoreHiddenActors(this);

	if (ACameraActor* CutsceneCam = CutsceneCameraActor.Get())
	{
		if (IsValid(CutsceneCam))
		{
			CutsceneCam->Destroy();
		}
		CutsceneCameraActor.Reset();
	}

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

	if (IsLocalController())
	{
		if (ActiveOverlay == EChroniclesOverlayMode::MainMenu && !bHasEnteredGameWorld)
		{
			ChroniclesStudioBg::SetNearbyBystandersHidden(this, true, 4500.f);
		}
		if (HUDWidget.IsValid())
		{
			HUDWidget->RefreshDynamicPanels();
		}
	}
}

void AChroniclesPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (IsLocalController())
	{
		if (ActiveOverlay == EChroniclesOverlayMode::MainMenu && !bHasEnteredGameWorld)
		{
			ChroniclesStudioBg::SetNearbyBystandersHidden(this, true, 4500.f);
		}
		if (HUDWidget.IsValid())
		{
			HUDWidget->RefreshDynamicPanels();
		}
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
	return bDevConsoleOpen || (ActiveOverlay != EChroniclesOverlayMode::None) || IsHeroDead();
}

void AChroniclesPlayerController::ToggleOverlay(EChroniclesOverlayMode TargetOverlay)
{
	if (TargetOverlay == EChroniclesOverlayMode::MainMenu)
	{
		if (ActiveOverlay == EChroniclesOverlayMode::MainMenu)
		{
			if (bHasEnteredGameWorld)
			{
				EnterGameFromTitleScreen(false);
			}
			return;
		}
		OpenMainMenuOverlay();
		return;
	}

	if (TargetOverlay == EChroniclesOverlayMode::None || ActiveOverlay == TargetOverlay)
	{
		CloseAllOverlays();
		return;
	}

	const bool bWasCreator = (ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer);
	const bool bWasMainMenu = (ActiveOverlay == EChroniclesOverlayMode::MainMenu);
	ActiveOverlay = TargetOverlay;
	if (TargetOverlay != EChroniclesOverlayMode::Dialogue)
	{
		ActiveDialogueNPC.Reset();
	}

	if ((bWasCreator || bWasMainMenu) && TargetOverlay != EChroniclesOverlayMode::HeroCustomizer)
	{
		ChroniclesStudioBg::RestoreHiddenActors(this);
	}

	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		if (TargetOverlay == EChroniclesOverlayMode::HeroCustomizer)
		{
			Hero->SetCreatorStudioCameraMode(true, CreatorOrbitYaw, CreatorZoomMode);
			ChroniclesStudioBg::SetNearbyBystandersHidden(this, true, 4500.f);
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
	if (bMainMenuReturnTarget && !bSmokeTestActive && !bCaptureFramesActive)
	{
		bMainMenuReturnTarget = false;
		bIsNewGameCreationFlow = false;
		if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(GetWorld()))
		{
			Music->PlayMenuClickSFX();
		}
		OpenMainMenuOverlay();
		return;
	}

	const bool bWasMainMenu = (ActiveOverlay == EChroniclesOverlayMode::MainMenu);
	ActiveOverlay = EChroniclesOverlayMode::None;
	ActiveDialogueNPC.Reset();
	bMainMenuReturnTarget = false;
	bIsNewGameCreationFlow = false;

	ChroniclesStudioBg::RestoreHiddenActors(this);

	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->SetCreatorStudioCameraMode(false);
	}

	if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(GetWorld()))
	{
		if (bWasMainMenu || Music->IsMenuMusicActive())
		{
			Music->SetMenuMusicActive(false);
		}
	}

	if (!IsHeroDead())
	{
		SetUIMode(false);
	}
}

void AChroniclesPlayerController::OpenMainMenuOverlay()
{
	const bool bWasCreator = (ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer);
	ActiveOverlay = EChroniclesOverlayMode::MainMenu;
	ActiveDialogueNPC.Reset();
	bMainMenuReturnTarget = false;
	bIsNewGameCreationFlow = false;

	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->SetBlocking(false);
		if (bWasCreator)
		{
			Hero->SetCreatorStudioCameraMode(false);
		}
	}
	bLastBlockKeyDown = false;

	if (!bHasEnteredGameWorld)
	{
		TitleCameraOrbitAngle = 0.f;
		ChroniclesStudioBg::SetNearbyBystandersHidden(this, true, 4500.f);
		SetControlRotation(FRotator(-8.f, 90.f, 0.f));
	}
	else if (bWasCreator)
	{
		ChroniclesStudioBg::RestoreHiddenActors(this);
	}

	if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(GetWorld()))
	{
		Music->SetMenuMusicActive(true);
	}

	if (HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}

	SetUIMode(true);
}

void AChroniclesPlayerController::EnterGameFromTitleScreen(bool bLoadSavedProgress)
{
	if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(GetWorld()))
	{
		Music->PlayMenuStartSFX();
		Music->SetMenuMusicActive(false);
	}

	const bool bFirstEntry = !bHasEnteredGameWorld;
	bHasEnteredGameWorld = true;
	bIsNewGameCreationFlow = false;
	bMainMenuReturnTarget = false;
	if (bLoadSavedProgress || bSmokeTestActive || bCaptureFramesActive)
	{
		PrologueStage = 4;
	}

	if (bLoadSavedProgress)
	{
		if (AAetheriaHeroCharacter* Hero = GetActiveHero())
		{
			if (Hero->SkillTree)
			{
				if (UGameplayStatics::DoesSaveGameExist(TEXT("AetheriaChroniclesSlot0"), 0))
				{
					Hero->SkillTree->LoadProgress(TEXT("AetheriaChroniclesSlot0"));
				}
				else if (UGameplayStatics::DoesSaveGameExist(TEXT("AetheriaChroniclesSlot"), 0))
				{
					Hero->SkillTree->LoadProgress(TEXT("AetheriaChroniclesSlot"));
				}
			}
		}
	}

	ActiveOverlay = EChroniclesOverlayMode::None;
	ActiveDialogueNPC.Reset();
	ChroniclesStudioBg::RestoreHiddenActors(this);

	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->SetCreatorStudioCameraMode(false);
		if (bFirstEntry)
		{
			SetControlRotation(FRotator(-10.f, 90.f, 0.f));
		}
	}

	if (!IsHeroDead())
	{
		SetUIMode(false);
	}

	if (HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}

	if (bFirstEntry && !bSmokeTestActive && !bCaptureFramesActive)
	{
		PushToastMessage(TEXT("Welcome to Aetheria: Chronicles of the Drake"), 5.0f);
	}
}

void AChroniclesPlayerController::StartNewGameCharacterCreation()
{
	if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(GetWorld()))
	{
		Music->PlayMenuClickSFX();
	}

	bIsNewGameCreationFlow = true;
	bMainMenuReturnTarget = true;

	// Place Hero temporarily in sunlit Oakhaven courtyard lighting so the 3D Character Creator portrait glows warmly
	AAetheriaWorldBuilder* Builder = FindWorldBuilder();
	if (Builder)
	{
		Builder->SetTimeOfDay(0.72f);
	}
	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		const float GZ = Builder ? Builder->SampleGroundZ(0.f, 3720.f, 120.f) : 120.f;
		Hero->SetActorLocationAndRotation(
			FVector(0.f, 3720.f, GZ + 105.f),
			FRotator(0.f, 145.f, 0.f),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		SetControlRotation(FRotator(-6.f, 145.f, 0.f));
	}
	CreatorZoomMode = 0;
	CreatorOrbitYaw = 162.f;

	ToggleOverlay(EChroniclesOverlayMode::HeroCustomizer);
}

void AChroniclesPlayerController::ConfirmNewHeroAndAwakenInCave()
{
	if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(GetWorld()))
	{
		Music->PlayMenuStartSFX();
		Music->SetMenuMusicActive(false);
	}

	bIsNewGameCreationFlow = false;
	bMainMenuReturnTarget = false;
	bHasEnteredGameWorld = true;
	PrologueStage = 0; // InCaveTalkToTutor

	ActiveOverlay = EChroniclesOverlayMode::None;
	ActiveDialogueNPC.Reset();
	ChroniclesStudioBg::RestoreHiddenActors(this);

	AAetheriaWorldBuilder* Builder = FindWorldBuilder();
	if (Builder)
	{
		Builder->SetTimeOfDay(0.72f);
	}

	const FVector CaveSpawnLoc = Builder
		? Builder->GetPrologueCaveSpawnLocation()
		: FVector(-3320.f, -800.f, 2098.f);

	if (AAetheriaHeroCharacter* Hero = GetActiveHero())
	{
		Hero->SetCreatorStudioCameraMode(false);
		Hero->SetBlocking(false);
		Hero->HP = Hero->MaxHP;
		Hero->Stamina = Hero->MaxStamina;
		if (Hero->SkillTree)
		{
			Hero->SkillTree->SaveProgress();
		}
		Hero->SetActorLocationAndRotation(
			CaveSpawnLoc,
			FRotator(0.f, 0.f, 0.f),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		SetControlRotation(FRotator(-4.f, 0.f, 0.f));
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQuestSubsystem* QuestSub = GI->GetSubsystem<UQuestSubsystem>())
		{
			QuestSub->SetTrackedQuest(FName(TEXT("Q_PrologueAwakening")));
		}
		if (UFogOfWarMapSubsystem* MapSub = GI->GetSubsystem<UFogOfWarMapSubsystem>())
		{
			MapSub->RevealAroundWorldLocation(CaveSpawnLoc, 5500.f);
		}
	}

	if (!IsHeroDead())
	{
		SetUIMode(false);
	}

	if (HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}

	PushToastMessage(TEXT("ПЕЩЕРА ПРОБУЖДЕНИЯ · Поговорите с Хранителем Вейленом [E]"), 6.0f);
}

void AChroniclesPlayerController::StartPrologueWaterfallCutscene()
{
	if (PrologueStage == 3 || bSmokeTestActive || bCaptureFramesActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	AAetheriaHeroCharacter* Hero = GetActiveHero();
	if (!World || !Hero)
	{
		PrologueStage = 4;
		return;
	}

	PrologueStage = 3;
	WaterfallCutsceneTimer = 0.0f;
	bWaterfallSplashSpawned = false;
	CutsceneStartHeroLoc = Hero->GetActorLocation();

	// Close any open console or modal & silence combat inputs cleanly during the 5.6s cinematic
	if (bDevConsoleOpen)
	{
		SetDeveloperConsoleOpen(false);
	}
	ActiveOverlay = EChroniclesOverlayMode::None;
	ActiveDialogueNPC.Reset();
	Hero->SetBlocking(false);
	Hero->InvulnTimer = 8.0f;
	if (UCharacterMovementComponent* MoveComp = Hero->GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(MOVE_Flying);
	}

	if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(World))
	{
		Music->PlayMenuStartSFX();
	}

	// Wind debris burst on leap from the cliff brink
	const FString WindPath(TEXT("/Game/Vefects/Wind_VFX/VFX/Particles/NS_Wind_Debris_Tree_Leaves_01_Colored_Closer.NS_Wind_Debris_Tree_Leaves_01_Colored_Closer"));
	if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(WindPath)))
	{
		if (UNiagaraSystem* WindNS = LoadObject<UNiagaraSystem>(nullptr, *WindPath))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, WindNS, CutsceneStartHeroLoc, FRotator::ZeroRotator, FVector(1.8f));
		}
	}

	if (ACameraActor* OldCam = CutsceneCameraActor.Get())
	{
		if (IsValid(OldCam))
		{
			OldCam->Destroy();
		}
		CutsceneCameraActor.Reset();
	}

	FActorSpawnParameters CamParams;
	CamParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ACameraActor* CineCam = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CutsceneStartHeroLoc + FVector(-300.f, -420.f, 180.f), FRotator(-14.f, 32.f, 0.f), CamParams))
	{
		if (UCameraComponent* CamComp = CineCam->GetCameraComponent())
		{
			CamComp->SetFieldOfView(72.f);
		}
		CutsceneCameraActor = CineCam;
		SetViewTargetWithBlend(CineCam, 0.25f, VTBlend_Cubic);
	}

	PushToastMessage(TEXT("ВОДОПАД ЗЕРКАЛЬНОГО ОЗЕРА"), 5.2f);
}

void AChroniclesPlayerController::EndPrologueWaterfallCutscene()
{
	AAetheriaHeroCharacter* Hero = GetActiveHero();
	AAetheriaWorldBuilder* Builder = FindWorldBuilder();

	const FVector ShoreLoc = Builder
		? Builder->GetPrologueShoreEmergenceLocation()
		: FVector(-480.f, 1680.f, 218.f);

	if (Hero)
	{
		Hero->SetActorLocationAndRotation(ShoreLoc, FRotator(0.f, 68.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* MoveComp = Hero->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->SetMovementMode(MOVE_Walking);
		}
		Hero->InvulnTimer = 2.0f;
		SetControlRotation(FRotator(-8.f, 68.f, 0.f));
		SetViewTargetWithBlend(Hero, 0.45f, VTBlend_Cubic);
	}

	if (ACameraActor* CineCam = CutsceneCameraActor.Get())
	{
		if (IsValid(CineCam))
		{
			CineCam->SetLifeSpan(0.55f);
		}
		CutsceneCameraActor.Reset();
	}

	PrologueStage = 4;
	WaterfallCutsceneTimer = 0.0f;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQuestSubsystem* QuestSub = GI->GetSubsystem<UQuestSubsystem>())
		{
			QuestSub->AdvanceQuestProgress(FName(TEXT("Q_PrologueAwakening")), 1);
			int32 G = 0;
			int32 X = 0;
			EChronicleOrb Orb = EChronicleOrb::None;
			bool bDrag = false;
			if (QuestSub->TurnInQuest(FName(TEXT("Q_PrologueAwakening")), G, X, Orb, bDrag) && Hero && Hero->SkillTree)
			{
				Hero->SkillTree->AddGold(G);
				Hero->SkillTree->AddXP(X);
			}
			QuestSub->SetTrackedQuest(FName(TEXT("Q_AwakenExort")));
		}
		if (UFogOfWarMapSubsystem* MapSub = GI->GetSubsystem<UFogOfWarMapSubsystem>())
		{
			MapSub->RevealAroundWorldLocation(ShoreLoc, 8500.f);
		}
	}

	if (!IsHeroDead())
	{
		SetUIMode(false);
	}

	if (HUDWidget.IsValid())
	{
		HUDWidget->RefreshDynamicPanels();
	}

	PushToastMessage(TEXT("Берег Зеркального озера — Ваше приключение начинается!"), 5.5f);
}

void AChroniclesPlayerController::UpdatePrologueAndWaterfallCutscene(float DeltaTime)
{
	if (bSmokeTestActive || bCaptureFramesActive || PrologueStage >= 4)
	{
		return;
	}

	AAetheriaHeroCharacter* Hero = GetActiveHero();
	UWorld* World = GetWorld();
	if (!Hero || !World)
	{
		return;
	}

	AAetheriaWorldBuilder* Builder = FindWorldBuilder();
	const FVector BrinkLoc = Builder ? Builder->GetPrologueWaterfallBrinkLocation() : FVector(-1980.f, -800.f, 2040.f);
	const FVector PlungeLoc = Builder ? Builder->GetPrologueLakePlungeLocation() : FVector(-1400.f, -800.f, 135.f);
	const FVector ShoreLoc = Builder ? Builder->GetPrologueShoreEmergenceLocation() : FVector(-480.f, 1680.f, 218.f);
	const float WaterZ = PlungeLoc.Z - 15.f;
	const FVector HeroLoc = Hero->GetActorLocation();

	// Stage 0/1 -> Stage 2: Exiting Cave onto Panoramic Cliff Overlook Ledge
	if (PrologueStage < 2)
	{
		if (HeroLoc.X > -2380.f && HeroLoc.Z > WaterZ + 900.f)
		{
			PrologueStage = 2;
			if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(World))
			{
				Music->PlayMenuStartSFX();
			}
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UFogOfWarMapSubsystem* MapSub = GI->GetSubsystem<UFogOfWarMapSubsystem>())
				{
					MapSub->RevealAroundWorldLocation(BrinkLoc, 18000.f);
				}
			}
			PushToastMessage(TEXT("ПРОСТОРЫ ЭТЕРИИ — Подойдите к краю водопада, чтобы спуститься в долину"), 6.0f);
		}
		return;
	}

	// Stage 2 -> Stage 3: Approaching Waterfall Brink triggers Epic 3-Shot Plunge & Shore Surfacing Cutscene
	if (PrologueStage == 2)
	{
		const float Dist2DToBrink = FVector::Dist2D(HeroLoc, BrinkLoc);
		if (Dist2DToBrink <= 290.f || (HeroLoc.X > -2050.f && HeroLoc.Z > WaterZ + 900.f))
		{
			StartPrologueWaterfallCutscene();
		}
		return;
	}

	// Stage 3: Driving the 5.6s 3-Shot Waterfall Plunge & Shore Surfacing Cutscene
	if (PrologueStage == 3)
	{
		const float SafeDT = FMath::Clamp(DeltaTime, 0.001f, 0.05f);
		WaterfallCutsceneTimer += SafeDT;
		const float T = WaterfallCutsceneTimer;

		const FVector LeapPeak = BrinkLoc + FVector(280.f, 0.f, 180.f);
		FVector TargetHeroPos = HeroLoc;
		FRotator TargetHeroRot = Hero->GetActorRotation();
		FVector TargetCamPos = BrinkLoc + FVector(-300.f, -440.f, 180.f);
		FVector TargetCamLook = BrinkLoc + FVector(400.f, 200.f, -250.f);

		if (T < 1.6f)
		{
			// Shot 1 (0.0s .. 1.6s): Heroic Leap at the Waterfall Brink overlooking Mirror Lake, Bridge & Oakhaven
			const float U = FMath::SmoothStep(0.f, 1.6f, T);
			TargetHeroPos = FMath::Lerp(CutsceneStartHeroLoc, LeapPeak, U);
			TargetHeroPos.Z += FMath::Sin(U * UE_PI) * 230.f;
			TargetHeroRot = FRotator(-10.f * U, 0.f, 0.f);

			TargetCamPos = FMath::Lerp(
				BrinkLoc + FVector(-340.f, -460.f, 190.f),
				BrinkLoc + FVector(-110.f, -380.f, 150.f),
				U);
			TargetCamLook = FMath::Lerp(TargetHeroPos + FVector(320.f, 180.f, -120.f), TargetHeroPos + FVector(160.f, 80.f, -80.f), U);
		}
		else if (T < 3.6f)
		{
			// Shot 2 (1.6s .. 3.6s): Parabolic Plunge Down the Roaring Cascade into Mirror Lake
			const float U = FMath::Clamp((T - 1.6f) / 2.0f, 0.f, 1.f);
			const float FallEase = U * U;
			TargetHeroPos.X = FMath::Lerp(LeapPeak.X, PlungeLoc.X, U);
			TargetHeroPos.Y = FMath::Lerp(LeapPeak.Y, PlungeLoc.Y, U);
			TargetHeroPos.Z = FMath::Lerp(LeapPeak.Z, PlungeLoc.Z - 35.f, FallEase);
			TargetHeroRot = FRotator(FMath::Lerp(-12.f, -42.f, U), 0.f, 0.f);

			TargetCamPos = FMath::Lerp(
				BrinkLoc + FVector(180.f, -520.f, -80.f),
				PlungeLoc + FVector(240.f, -460.f, 280.f),
				FMath::SmoothStep(0.f, 1.f, U));
			TargetCamLook = TargetHeroPos + FVector(0.f, 0.f, -40.f);

			if (T >= 3.4f && !bWaterfallSplashSpawned)
			{
				bWaterfallSplashSpawned = true;
				auto SpawnSplash = [&](const TCHAR* AssetPath, float Scale)
				{
					const FString PathStr(AssetPath);
					if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(PathStr)))
					{
						if (UParticleSystem* PS = LoadObject<UParticleSystem>(nullptr, *PathStr))
						{
							UGameplayStatics::SpawnEmitterAtLocation(World, PS, PlungeLoc + FVector(0.f, 0.f, 20.f), FRotator::ZeroRotator, FVector(Scale), true);
						}
					}
				};
				SpawnSplash(TEXT("/Game/FXVarietyPack/Particles/P_ky_aquaStorm.P_ky_aquaStorm"), 2.4f);
				SpawnSplash(TEXT("/Game/FXVarietyPack/Particles/P_ky_waterBallHit.P_ky_waterBallHit"), 2.2f);
			}
		}
		else if (T < 5.6f)
		{
			// Shot 3 (3.6s .. 5.6s): Surfacing Through Lake Mist & Rising Heroically onto Mirror Lake Shore
			const float U = FMath::SmoothStep(3.6f, 5.6f, T);
			const FVector SurfacePoint = FMath::Lerp(PlungeLoc, ShoreLoc, 0.38f) + FVector(0.f, 0.f, 18.f);
			if (U < 0.45f)
			{
				const float SubU = U / 0.45f;
				TargetHeroPos = FMath::Lerp(PlungeLoc - FVector(0.f, 0.f, 65.f), SurfacePoint, SubU);
			}
			else
			{
				const float SubU = (U - 0.45f) / 0.55f;
				TargetHeroPos = FMath::Lerp(SurfacePoint, ShoreLoc, SubU);
			}
			TargetHeroRot = FRotator(0.f, FMath::Lerp(42.f, 68.f, U), 0.f);

			TargetCamPos = FMath::Lerp(
				ShoreLoc + FVector(-340.f, -420.f, 155.f),
				ShoreLoc + FVector(-220.f, -340.f, 115.f),
				U);
			TargetCamLook = FMath::Lerp(TargetHeroPos + FVector(120.f, 240.f, 40.f), ShoreLoc + FVector(280.f, 650.f, 65.f), U);
		}
		else
		{
			EndPrologueWaterfallCutscene();
			return;
		}

		Hero->SetActorLocationAndRotation(TargetHeroPos, TargetHeroRot, false, nullptr, ETeleportType::TeleportPhysics);
		if (ACameraActor* CineCam = CutsceneCameraActor.Get())
		{
			if (IsValid(CineCam))
			{
				const FRotator CamLookRot = (TargetCamLook - TargetCamPos).GetSafeNormal().Rotation();
				CineCam->SetActorLocationAndRotation(TargetCamPos, CamLookRot);
			}
		}
	}
}

FVector AChroniclesPlayerController::GetActiveWaypointWorldLocation(FString& OutLabel) const
{
	OutLabel.Empty();

	const AAetheriaWorldBuilder* Builder = FindWorldBuilder();
	if (PrologueStage == 0)
	{
		OutLabel = TEXT("Хранитель Вейлен");
		return Builder ? (Builder->GetPrologueTutorLocation() + FVector(0.f, 0.f, 115.f)) : FVector(-2920.f, -620.f, 2115.f);
	}
	if (PrologueStage == 1)
	{
		OutLabel = TEXT("Выход из пещеры");
		const FVector Brink = Builder ? Builder->GetPrologueWaterfallBrinkLocation() : FVector(-1980.f, -800.f, 2040.f);
		return FVector(-2250.f, -800.f, Brink.Z + 50.f);
	}
	if (PrologueStage == 2)
	{
		OutLabel = TEXT("Прыжок у водопада");
		return Builder ? (Builder->GetPrologueWaterfallBrinkLocation() + FVector(0.f, 0.f, 65.f)) : FVector(-1980.f, -800.f, 2105.f);
	}

	const UGameInstance* GI = GetGameInstance();
	const UQuestSubsystem* QuestSub = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;
	if (const FQuestDef* Tracked = QuestSub ? QuestSub->GetTrackedQuest() : nullptr)
	{
		OutLabel = Tracked->Title.ToString();
		if (Tracked->State == EQuestState::Available || Tracked->State == EQuestState::ReadyToTurnIn)
		{
			if (UWorld* World = GetWorld())
			{
				for (TActorIterator<AAetheriaNPCActor> It(World); It; ++It)
				{
					if (IsValid(*It) && (*It)->NpcId == Tracked->GiverId)
					{
						return (*It)->GetActorLocation() + FVector(0.f, 0.f, 120.f);
					}
				}
			}
		}
		return Tracked->ObjectiveWorldLocation;
	}

	return FVector::ZeroVector;
}

void AChroniclesPlayerController::RequestQuitApplication()
{
	if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(GetWorld()))
	{
		Music->PlayMenuClickSFX();
	}
	UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
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

AAetheriaNPCActor* AChroniclesPlayerController::FindTalkableNPC(float MaxDistance) const
{
	const AAetheriaHeroCharacter* Hero = GetActiveHero();
	UWorld* World = GetWorld();
	if (!Hero || !World)
	{
		return nullptr;
	}

	const UGameInstance* GI = GetGameInstance();
	const UQuestSubsystem* QuestSub = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;

	const FVector HeroLoc = Hero->GetActorLocation();
	FVector LookDir = GetControlRotation().Vector().GetSafeNormal2D();
	if (LookDir.IsNearlyZero())
	{
		LookDir = Hero->GetActorForwardVector().GetSafeNormal2D();
	}

	const float SafeMaxDist = FMath::Max(60.f, MaxDistance);
	float BestScore = TNumericLimits<float>::Max();
	AAetheriaNPCActor* BestNPC = nullptr;

	for (TActorIterator<AAetheriaNPCActor> It(World); It; ++It)
	{
		AAetheriaNPCActor* NPC = *It;
		if (!IsValid(NPC))
		{
			continue;
		}

		const FVector ToNPC = NPC->GetActorLocation() - HeroLoc;
		const float Dist = ToNPC.Size();
		if (Dist > SafeMaxDist)
		{
			continue;
		}

		// Require an active quest/dialogue entry on the NPC so ambient actors don't hijack [E] Exort
		if (QuestSub && ChroniclesHUDPalette::ResolveQuestForNPC(QuestSub, NPC) == nullptr)
		{
			continue;
		}

		// Must be facing the NPC (within ~72 deg cone) OR standing right beside them (< 135 cm)
		const FVector ToNPCDir = ToNPC.GetSafeNormal2D();
		const float FacingDot = FVector::DotProduct(LookDir, ToNPCDir);
		if (Dist > 135.f && FacingDot < 0.30f)
		{
			continue;
		}

		// Favor closest NPC directly in front of the player's gaze
		const float Score = Dist - FMath::Max(0.f, FacingDot) * 85.f;
		if (Score < BestScore)
		{
			BestScore = Score;
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
				ChroniclesStudioBg::RestoreHiddenActors(this);
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

	if (ActiveOverlay == EChroniclesOverlayMode::MainMenu && !bHasEnteredGameWorld)
	{
		TitleCameraOrbitAngle += FMath::Clamp(DeltaTime, 0.001f, 0.1f) * 0.18f;
		const float SwayYaw = 90.f + FMath::Sin(TitleCameraOrbitAngle) * 14.f;
		SetControlRotation(FRotator(-8.f, SwayYaw, 0.f));
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

	// Deferred next-tick Slate focus confirmation so SEditableTextBox always receives focus cleanly after PrePass
	if (bDevConsoleOpen && bPendingConsoleFocus && HUDWidget.IsValid())
	{
		bPendingConsoleFocus = false;
		HUDWidget->SyncDeveloperConsoleVisibilityAndFocus(true, true);
	}

	// Allow ` (backtick / tilde / ё) Developer Console even during Prologue cutscene if desired, or handle before cutscene early-return
	if (WasConsoleToggleKeyJustPressed())
	{
		ToggleDeveloperConsole();
		return;
	}

	UpdatePrologueAndWaterfallCutscene(DeltaTime);
	if (IsPrologueCutsceneActive())
	{
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

bool AChroniclesPlayerController::WasConsoleToggleKeyJustPressed() const
{
	static const FKey KeyCyrillicYoLower(TEXT("ё"));
	static const FKey KeyCyrillicYoUpper(TEXT("Ё"));
	static const FKey KeyGraveAccent(TEXT("GraveAccent"));

	return WasInputKeyJustPressed(EKeys::Tilde)
		|| WasInputKeyJustPressed(KeyCyrillicYoLower)
		|| WasInputKeyJustPressed(KeyCyrillicYoUpper)
		|| WasInputKeyJustPressed(KeyGraveAccent);
}

void AChroniclesPlayerController::HandleGlobalAndOverlayKeys()
{
	if (bDevConsoleOpen)
	{
		if (WasInputKeyJustPressed(EKeys::Escape))
		{
			SetDeveloperConsoleOpen(false);
		}
		return;
	}

	if (WasInputKeyJustPressed(EKeys::Escape))
	{
		if (ActiveOverlay == EChroniclesOverlayMode::MainMenu)
		{
			if (bHasEnteredGameWorld)
			{
				EnterGameFromTitleScreen(false);
			}
			// Initial Launch Screen (!bHasEnteredGameWorld): [Esc] stays safely on Title Screen
		}
		else if (ActiveOverlay != EChroniclesOverlayMode::None)
		{
			CloseAllOverlays();
		}
		else if (!IsHeroDead())
		{
			OpenMainMenuOverlay();
		}
		return;
	}

	if (IsHeroDead() || ActiveOverlay == EChroniclesOverlayMode::MainMenu)
	{
		return;
	}

	// While browsing Creator / RTX sub-screens launched from the initial Title Screen, allow only closing keys
	if (!bHasEnteredGameWorld)
	{
		if (ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer && WasInputKeyJustPressed(EKeys::C))
		{
			CloseAllOverlays();
		}
		else if (ActiveOverlay == EChroniclesOverlayMode::GraphicsRTX && WasInputKeyJustPressed(EKeys::F10))
		{
			CloseAllOverlays();
		}
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
	// Only open NPC dialogue when within 280 cm AND facing/closest NPC with active dialogue;
	// otherwise cleanly push the Exort primal orb without fighting [E] key collisions!
	if (AAetheriaNPCActor* NearbyNPC = FindTalkableNPC(280.f))
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
		ChroniclesStudioBg::RestoreHiddenActors(this);
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

	// Prologue Cave Tutor Lore Interaction: advances PrologueStage 0 -> 1 cleanly toward Cave Exit Overlook
	if (NPC->NpcId == FName(TEXT("Npc_CaveTutor")))
	{
		if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(GetWorld()))
		{
			Music->PlayMenuClickSFX();
		}
		QuestSub->AcceptQuest(FName(TEXT("Q_PrologueAwakening")));
		QuestSub->SetTrackedQuest(FName(TEXT("Q_PrologueAwakening")));
		if (PrologueStage < 1)
		{
			PrologueStage = 1;
		}
		if (UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr)
		{
			MapSub->RevealAroundWorldLocation(NPC->GetActorLocation(), 9500.f);
		}
		PushToastMessage(TEXT("Выйдите из пещеры на утёс к солнечному свету Этерии"), 5.0f);
		if (HUDWidget.IsValid())
		{
			HUDWidget->RefreshDynamicPanels();
		}
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

FString AChroniclesPlayerController::ConsoleCommand(const FString& Command, bool bWriteToLog)
{
	const FString Trimmed = Command.TrimStartAndEnd();
	if (Trimmed.StartsWith(TEXT("/")) || Trimmed.Equals(TEXT("godmode"), ESearchCase::IgnoreCase))
	{
		ExecuteDeveloperConsoleCommand(Trimmed);
		return FString();
	}
	return Super::ConsoleCommand(Command, bWriteToLog);
}

void AChroniclesPlayerController::GodMode()
{
	ExecuteDeveloperConsoleCommand(TEXT("/godmode"));
}

void AChroniclesPlayerController::ToggleDeveloperConsole()
{
	const double NowReal = FPlatformTime::Seconds();
	if (NowReal - LastConsoleToggleRealTime < 0.12)
	{
		return;
	}
	SetDeveloperConsoleOpen(!bDevConsoleOpen);
}

void AChroniclesPlayerController::SetDeveloperConsoleOpen(bool bOpen)
{
	if (bDevConsoleOpen == bOpen)
	{
		if (bOpen && HUDWidget.IsValid())
		{
			HUDWidget->SyncDeveloperConsoleVisibilityAndFocus(true, true);
		}
		return;
	}

	LastConsoleToggleRealTime = FPlatformTime::Seconds();
	bDevConsoleOpen = bOpen;

	if (bOpen)
	{
		LastConsoleOpenRealTime = LastConsoleToggleRealTime;
		bPendingConsoleFocus = true;

		if (AAetheriaHeroCharacter* Hero = GetActiveHero())
		{
			Hero->SetBlocking(false);
			if (UCharacterMovementComponent* MoveComp = Hero->GetCharacterMovement())
			{
				MoveComp->StopMovementImmediately();
			}
		}
		bLastBlockKeyDown = false;

		SetUIMode(true);
		if (HUDWidget.IsValid())
		{
			HUDWidget->RebuildConsoleLogBox();
			HUDWidget->SyncDeveloperConsoleVisibilityAndFocus(true, true);
		}
	}
	else
	{
		bPendingConsoleFocus = false;
		if (HUDWidget.IsValid())
		{
			HUDWidget->SyncDeveloperConsoleVisibilityAndFocus(false, false);
		}

		// Restore gameplay mouse capture ONLY when no other modal overlay, prologue cutscene, or death screen is active underneath
		if (ActiveOverlay == EChroniclesOverlayMode::None && !IsHeroDead() && !IsPrologueCutsceneActive() && !bCaptureFramesActive)
		{
			SetUIMode(false);
		}
		else if (!IsPrologueCutsceneActive())
		{
			SetUIMode(true);
		}
	}
}

void AChroniclesPlayerController::AppendConsoleLogLine(const FString& Line, const FLinearColor& Color)
{
	if (Line.IsEmpty())
	{
		return;
	}

	ConsoleLogLines.Emplace(Line, Color);
	while (ConsoleLogLines.Num() > 40)
	{
		ConsoleLogLines.RemoveAt(0);
	}

	if (HUDWidget.IsValid())
	{
		HUDWidget->RebuildConsoleLogBox();
	}
}

bool AChroniclesPlayerController::ExecuteDeveloperConsoleCommand(const FString& RawCommand)
{
	FString Cleaned = RawCommand.TrimStartAndEnd();
	// Strip any accidental leading/trailing backtick, tilde, or Russian 'ё'/'Ё' from opening keystroke
	while (!Cleaned.IsEmpty())
	{
		const TCHAR First = Cleaned[0];
		if (First == TEXT('`') || First == TEXT('~') || First == 0x0451 || First == 0x0401)
		{
			Cleaned.RightChopInline(1, EAllowShrinking::No);
			Cleaned.TrimStartAndEndInline();
		}
		else
		{
			break;
		}
	}

	if (Cleaned.IsEmpty())
	{
		return false;
	}

	ConsoleCommandHistory.Remove(Cleaned);
	ConsoleCommandHistory.Add(Cleaned);
	while (ConsoleCommandHistory.Num() > 24)
	{
		ConsoleCommandHistory.RemoveAt(0);
	}

	AppendConsoleLogLine(FString::Printf(TEXT("> %s"), *Cleaned), ChroniclesHUDPalette::TextSecondary);

	FString CmdToken = Cleaned;
	FString ArgString;
	Cleaned.Split(TEXT(" "), &CmdToken, &ArgString);
	CmdToken = CmdToken.ToLower();
	ArgString = ArgString.TrimStartAndEnd();

	// --- /godmode (or godmode / /god) ---
	if (CmdToken == TEXT("/godmode") || CmdToken == TEXT("godmode") || CmdToken == TEXT("/god"))
	{
		AAetheriaHeroCharacter* Hero = GetActiveHero();
		if (!Hero)
		{
			AppendConsoleLogLine(TEXT("[Ошибка] Персонаж героя еще не создан в мире."), ChroniclesHUDPalette::TextCrimson);
			return false;
		}

		bool bTargetGodMode = !Hero->IsGodModeActive();
		if (!ArgString.IsEmpty())
		{
			if (ArgString.Equals(TEXT("on"), ESearchCase::IgnoreCase) || ArgString == TEXT("1") || ArgString.Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				bTargetGodMode = true;
			}
			else if (ArgString.Equals(TEXT("off"), ESearchCase::IgnoreCase) || ArgString == TEXT("0") || ArgString.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				bTargetGodMode = false;
			}
		}

		Hero->SetGodMode(bTargetGodMode);
		bLastKnownHeroDead = Hero->bDead;

		if (!bDevConsoleOpen && ActiveOverlay == EChroniclesOverlayMode::None && !Hero->bDead && !IsPrologueCutsceneActive() && !bCaptureFramesActive)
		{
			SetUIMode(false);
		}

		if (HUDWidget.IsValid())
		{
			HUDWidget->RefreshDynamicPanels();
		}

		if (bTargetGodMode)
		{
			const int32 UnlockedSpells = Hero->SpellSystem ? Hero->SpellSystem->GetUnlockedSpellCount() : 10;
			AppendConsoleLogLine(
				FString::Printf(
					TEXT("[GOD MODE: ON] Режим Бога ВКЛЮЧЕН — Бессмертие активно · Открыты все %d/10 заклинаний (Exort · Wex · Quas) · Бесконечная мана и 0с КД!"),
					UnlockedSpells),
				ChroniclesHUDPalette::TextGold);
			PushToastMessage(TEXT("РЕЖИМ БОГА ВКЛЮЧЕН — Бессмертие и все 10/10 заклинаний открыты!"), 5.0f);
		}
		else
		{
			AppendConsoleLogLine(
				TEXT("[GOD MODE: OFF] Режим Бога ВЫКЛЮЧЕН — Восстановлены обычные правила смертности и магии."),
				ChroniclesHUDPalette::TextAmber);
			PushToastMessage(TEXT("Режим Бога выключен"), 3.6f);
		}
		return true;
	}

	// --- /help ---
	if (CmdToken == TEXT("/help") || CmdToken == TEXT("help") || CmdToken == TEXT("/?"))
	{
		AppendConsoleLogLine(TEXT("Доступные команды консоли Aetheria:"), ChroniclesHUDPalette::TextGold);
		AppendConsoleLogLine(TEXT("  /godmode       — Вкл/Выкл Режим Бога (бессмертие + все 10 заклинаний + бесконечная мана)"), ChroniclesHUDPalette::TextPrimary);
		AppendConsoleLogLine(TEXT("  /restore       — Мгновенное магическое восстановление ландшафта и разрушенных объектов"), ChroniclesHUDPalette::TextPrimary);
		AppendConsoleLogLine(TEXT("  /heal          — Полностью восстановить HP, ману и выносливость"), ChroniclesHUDPalette::TextPrimary);
		AppendConsoleLogLine(TEXT("  /gold [сумма]  — Добавить золото (по умолчанию +500G)"), ChroniclesHUDPalette::TextPrimary);
		AppendConsoleLogLine(TEXT("  /time          — Сменить время суток (Закат / Полная Луна / Рассвет / Полдень)"), ChroniclesHUDPalette::TextPrimary);
		AppendConsoleLogLine(TEXT("  /clear         — Очистить журнал консоли"), ChroniclesHUDPalette::TextPrimary);
		return true;
	}

	// --- /restore ---
	if (CmdToken == TEXT("/restore") || CmdToken == TEXT("restore"))
	{
		int32 ScarsHealed = 0;
		int32 PropsRegrown = 0;
		if (AAetheriaWorldBuilder* Builder = FindWorldBuilder())
		{
			ScarsHealed = Builder->RestoreAllActiveTerrainScarsNow();
			for (ADestructiblePropActor* Prop : Builder->GetDestructibleProps())
			{
				if (IsValid(Prop) && Prop->IsBroken())
				{
					Prop->TriggerMagicalRestoration();
					++PropsRegrown;
				}
			}
		}
		AppendConsoleLogLine(
			FString::Printf(TEXT("[Магическое восстановление] Исцелено следов заклинаний: %d · Восстановлено объектов: %d"), ScarsHealed, PropsRegrown),
			ChroniclesHUDPalette::TextEmerald);
		return true;
	}

	// --- /heal ---
	if (CmdToken == TEXT("/heal") || CmdToken == TEXT("heal"))
	{
		if (AAetheriaHeroCharacter* Hero = GetActiveHero())
		{
			Hero->HP = Hero->MaxHP;
			Hero->Stamina = Hero->MaxStamina;
			if (Hero->SpellSystem)
			{
				Hero->SpellSystem->Mana = Hero->SpellSystem->MaxMana;
			}
			AppendConsoleLogLine(TEXT("[Исцеление] HP, мана и выносливость полностью восстановлены."), ChroniclesHUDPalette::TextEmerald);
			return true;
		}
		return false;
	}

	// --- /gold [amount] ---
	if (CmdToken == TEXT("/gold") || CmdToken == TEXT("gold"))
	{
		if (AAetheriaHeroCharacter* Hero = GetActiveHero())
		{
			if (Hero->SkillTree)
			{
				const int32 AddAmount = ArgString.IsNumeric() ? FMath::Clamp(FCString::Atoi(*ArgString), 1, 999999) : 500;
				Hero->SkillTree->AddGold(AddAmount);
				if (HUDWidget.IsValid())
				{
					HUDWidget->RefreshDynamicPanels();
				}
				AppendConsoleLogLine(FString::Printf(TEXT("[Золото] Получено +%d G (всего: %d G)."), AddAmount, Hero->SkillTree->Gold), ChroniclesHUDPalette::TextGold);
				return true;
			}
		}
		return false;
	}

	// --- /time ---
	if (CmdToken == TEXT("/time") || CmdToken == TEXT("time"))
	{
		RequestCycleTimeOfDay();
		const AAetheriaWorldBuilder* Builder = FindWorldBuilder();
		AppendConsoleLogLine(
			FString::Printf(TEXT("[Время суток] %s"), Builder ? *Builder->GetTimeOfDayLabel() : TEXT("Переключено")),
			ChroniclesHUDPalette::TextCyan);
		return true;
	}

	// --- /clear ---
	if (CmdToken == TEXT("/clear") || CmdToken == TEXT("clear") || CmdToken == TEXT("cls"))
	{
		ConsoleLogLines.Empty();
		if (HUDWidget.IsValid())
		{
			HUDWidget->RebuildConsoleLogBox();
		}
		return true;
	}

	// Unknown slash command feedback
	if (CmdToken.StartsWith(TEXT("/")))
	{
		AppendConsoleLogLine(
			FString::Printf(TEXT("Неизвестная команда '%s'. Введите /godmode или /help."), *Cleaned),
			ChroniclesHUDPalette::TextCrimson);
		return false;
	}

	// Forward non-slash engine/multiplayer commands (e.g. stat fps, HostCoop)
	const FString EngineOut = Super::ConsoleCommand(Cleaned, true);
	if (!EngineOut.IsEmpty())
	{
		AppendConsoleLogLine(EngineOut, ChroniclesHUDPalette::TextPrimary);
	}
	else
	{
		AppendConsoleLogLine(FString::Printf(TEXT("Выполнена команда движка: %s"), *Cleaned), ChroniclesHUDPalette::TextSecondary);
	}
	return true;
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
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFogOfWarMapSubsystem* MapSub = GI->GetSubsystem<UFogOfWarMapSubsystem>())
		{
			MapSub->RevealAroundWorldLocation(TargetLocation, 5500.f);
		}
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

		int32 LoadedMusicCues = 0;
		bool bMenuMusicOK = false;
		bool bBgMusicOK = false;
		if (UAetheriaMusicSubsystem* MusicSub = UAetheriaMusicSubsystem::Get(GetWorld()))
		{
			LoadedMusicCues = MusicSub->GetLoadedCueCount();
			MusicSub->SetMenuMusicActive(true);
			bMenuMusicOK = MusicSub->IsMenuMusicActive() && (MusicSub->GetCurrentState() == EChronicleMusicState::Menu);
			MusicSub->SetMenuMusicActive(false);
			MusicSub->SetExplorationZone(true);
			bBgMusicOK = !MusicSub->IsMenuMusicActive()
				&& (MusicSub->GetCurrentState() == EChronicleMusicState::Village
					|| MusicSub->GetCurrentState() == EChronicleMusicState::Field);
		}

		const AAetheriaWorldBuilder* WB = FindWorldBuilder();
		const bool bPrologueGeomOK = (WB != nullptr)
			&& (WB->GetPrologueCaveSpawnLocation().Z > WB->GetPrologueLakePlungeLocation().Z + 400.f);

		UE_LOG(LogWizardsmoke, Display,
			TEXT("STEP 1 OK | GPU='%s' | Tier=%s | RTXSupported=%d | RTXActive=%d | DLSSActive=%d | TexturePool=%dMB | MusicCues=%d/5 | MenuMusic=%d | BgMusic=%d | PrologueReady=%d"),
			*Tel.GPUAdapterName, TierStr,
			Tel.bHardwareRTXSupported ? 1 : 0,
			Tel.bHardwareRTXActive ? 1 : 0,
			Tel.bDLSSActive ? 1 : 0,
			Tel.TexturePoolMB,
			LoadedMusicCues,
			bMenuMusicOK ? 1 : 0,
			bBgMusicOK ? 1 : 0,
			bPrologueGeomOK ? 1 : 0);

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

			int32 RestoredScars = 0;
			bool bMagicalPropRegrowth = false;
			if (IsValid(PostGCWorld) && IsValid(PostGCHero))
			{
				// Ensure clean baseline, create a fresh 60s terrain scar, and verify 59.0s -> still scarred, 60.5s -> automatically restored via Tick!
				PostGCWorld->RestoreAllActiveTerrainScarsNow();
				const float SavedTOD = PostGCWorld->TimeOfDay;
				PostGCWorld->NotifySpellTerrainImpact(
					PostGCHero->GetActorLocation() + FVector(320.f, 0.f, 0.f),
					320.f,
					EChronicleOrb::Exort);
				const int32 ScarsAt0s = PostGCWorld->GetActiveTerrainScarCount();
				for (int32 StepIdx = 0; StepIdx < 590; ++StepIdx)
				{
					PostGCWorld->Tick(0.1f); // 59.0s elapsed (< 60.0s)
				}
				const int32 ScarsAt59s = PostGCWorld->GetActiveTerrainScarCount();
				for (int32 StepIdx = 0; StepIdx < 15; ++StepIdx)
				{
					PostGCWorld->Tick(0.1f); // 60.5s elapsed (> 60.0s -> auto-restores!)
				}
				const int32 ScarsAt60_5s = PostGCWorld->GetActiveTerrainScarCount();
				PostGCWorld->SetTimeOfDay(SavedTOD);
				if (ScarsAt0s >= 1 && ScarsAt59s >= 1 && ScarsAt60_5s == 0)
				{
					RestoredScars = ScarsAt0s;
				}

				ADestructiblePropActor* TestProp = nullptr;
				for (ADestructiblePropActor* Candidate : PostGCWorld->GetDestructibleProps())
				{
					if (IsValid(Candidate) && !Candidate->IsBroken())
					{
						TestProp = Candidate;
						break;
					}
				}
				if (IsValid(TestProp))
				{
					const bool bDefault60sConfig = FMath::IsNearlyEqual(TestProp->MagicalRestoreDelaySeconds, 60.f, 0.1f);
					TestProp->ForceShatter(PostGCHero->GetActorLocation());
					const bool bBrokenAt0s = TestProp->IsBroken();
					for (int32 StepIdx = 0; StepIdx < 590; ++StepIdx)
					{
						TestProp->Tick(0.1f); // 59.0s elapsed (< 60.0s)
					}
					const bool bStillBrokenAt59s = TestProp->IsBroken();
					for (int32 StepIdx = 0; StepIdx < 15; ++StepIdx)
					{
						TestProp->Tick(0.1f); // 60.5s elapsed (> 60.0s -> auto-restores via Tick!)
					}
					bMagicalPropRegrowth = bDefault60sConfig && bBrokenAt0s && bStillBrokenAt59s && !TestProp->IsBroken();
				}
			}

			// Verify in-game ` / ~ / ё Console + /godmode (Slate key-event toggle, 10/10 spells, stature bypass, 99999 dmg + FellOutOfWorld immortality, 0 mana/CD cast, toggle OFF)
			bool bGodModeConsoleVerified = false;
			if (IsValid(PostGCHero) && PostGCHero->SpellSystem && PostGCHero->Customization)
			{
				USpellSystemComponent* Spells = PostGCHero->SpellSystem;
				UHeroCustomizationComponent* Custom = PostGCHero->Customization;

				const bool bSavedE = Spells->bUnlockedExort;
				const bool bSavedW = Spells->bUnlockedWex;
				const bool bSavedQ = Spells->bUnlockedQuas;
				const FVector SavedHeroLoc = PostGCHero->GetActorLocation();

				// Lock out all orbs & set small stature (< 1.45m) before entering /godmode
				Spells->bUnlockedExort = false;
				Spells->bUnlockedWex = false;
				Spells->bUnlockedQuas = false;
				Spells->ActiveOrbs.Reset();
				Spells->SlotD = EChronicleSpellId::None;
				Spells->SlotF = EChronicleSpellId::None;
				Custom->SetHeightMeters(1.25f);

				// Test real Slate ` (VK_OEM_3 = 0xC0 / EKeys::Tilde) key event opening the console!
				SetDeveloperConsoleOpen(false);
				LastConsoleToggleRealTime = -10.0;
				bool bConsoleOpenedOK = false;
				if (HUDWidget.IsValid())
				{
					const FKeyEvent BacktickDownEvent(EKeys::Tilde, FModifierKeysState(), 0, false, TEXT('`'), 0xC0);
					HUDWidget->OnPreviewKeyDown(FGeometry(), BacktickDownEvent);
					bConsoleOpenedOK = IsDeveloperConsoleOpen();
				}
				else
				{
					SetDeveloperConsoleOpen(true);
					bConsoleOpenedOK = IsDeveloperConsoleOpen();
				}

				const bool bCmdExecOK = ExecuteDeveloperConsoleCommand(TEXT("/godmode"));

				const bool bGodActiveOK = PostGCHero->IsGodModeActive()
					&& (Spells->GetUnlockedOrbCount() == 3)
					&& (Spells->GetUnlockedSpellCount() == 10)
					&& (Spells->ActiveOrbs.Num() == 3)
					&& PostGCHero->CanCastSpellsByStature();

				// Verify true immortality against BOTH lethal 99,999 damage AND FellOutOfWorld (KillZ)!
				PostGCHero->InvulnTimer = 0.f;
				const float HPBeforeLethal = PostGCHero->HP;
				const float TakenDmg = UGameplayStatics::ApplyDamage(
					PostGCHero,
					99999.f,
					this,
					PostGCBoss,
					UDamageType::StaticClass());
				PostGCHero->FellOutOfWorld(*UDamageType::StaticClass()->GetDefaultObject<UDamageType>());
				PostGCHero->SetActorLocation(SavedHeroLoc, false, nullptr, ETeleportType::TeleportPhysics);
				const bool bImmortalOK = (TakenDmg == 0.f)
					&& IsValid(PostGCHero)
					&& !PostGCHero->bDead
					&& FMath::IsNearlyEqual(PostGCHero->HP, HPBeforeLethal, 0.1f);

				// Verify casting a God Mode spell costs 0 Mana & leaves 0s cooldown
				const FVector CastOrig = PostGCHero->GetActorLocation() + FVector(0.f, 0.f, 65.f);
				const FVector CastDir = PostGCHero->GetActorForwardVector().GetSafeNormal();
				const bool bGodCastOK = Spells->CastSpellSlot(false, CastOrig, CastDir, CastOrig + CastDir * 700.f, 0.5f)
					&& FMath::IsNearlyEqual(Spells->Mana, Spells->MaxMana, 0.1f)
					&& (Spells->GetSpellCooldownRemaining(Spells->SlotD) == 0.f);

				// Toggle /godmode OFF and verify Russian 'ё' (0x0451 / VK_OEM_3) key event closes the console cleanly!
				ExecuteDeveloperConsoleCommand(TEXT("/godmode"));
				LastConsoleToggleRealTime = -10.0;
				if (HUDWidget.IsValid())
				{
					const FKeyEvent RussianYoCloseEvent(FKey(TEXT("ё")), FModifierKeysState(), 0, false, 0x0451, 0xC0);
					HUDWidget->OnPreviewKeyDown(FGeometry(), RussianYoCloseEvent);
				}
				else
				{
					SetDeveloperConsoleOpen(false);
				}
				const bool bGodToggledOffOK = !PostGCHero->IsGodModeActive()
					&& !IsDeveloperConsoleOpen()
					&& (Spells->GetUnlockedSpellCount() == 0);

				// Restore post-smoke state
				Spells->bUnlockedExort = bSavedE;
				Spells->bUnlockedWex = bSavedW;
				Spells->bUnlockedQuas = bSavedQ;
				Custom->SetHeightMeters(1.85f);
				PostGCHero->HP = PostGCHero->MaxHP;

				bGodModeConsoleVerified = bConsoleOpenedOK && bCmdExecOK && bGodActiveOK && bImmortalOK && bGodCastOK && bGodToggledOffOK;
				if (!bGodModeConsoleVerified || RestoredScars < 1 || !bMagicalPropRegrowth)
				{
					AbortSmokeTestWithError(FString::Printf(
						TEXT("60sRestore/GodMode gate failed (Scars60s=%d Prop60s=%d Open=%d Cmd=%d Active10Spells=%d Immortal=%d Cast0CD=%d ToggleOff=%d)."),
						RestoredScars,
						bMagicalPropRegrowth ? 1 : 0,
						bConsoleOpenedOK ? 1 : 0,
						bCmdExecOK ? 1 : 0,
						bGodActiveOK ? 1 : 0,
						bImmortalOK ? 1 : 0,
						bGodCastOK ? 1 : 0,
						bGodToggledOffOK ? 1 : 0));
					return;
				}
			}

			UE_LOG(LogWizardsmoke, Display,
				TEXT("STEP 6 OK | BossEngaged=%d | CrimsonTelegraph=%d | CleanDisengage=%d | PostGCValid=%d | TerrainScarsRestored=%d | MagicalPropRegrowth=%d | GodModeConsole=%d"),
				bSmokeBossEngaged ? 1 : 0, bSmokeBossTelegraphed ? 1 : 0, bCleanDisengage ? 1 : 0, bPostGCValid ? 1 : 0,
				RestoredScars, bMagicalPropRegrowth ? 1 : 0, bGodModeConsoleVerified ? 1 : 0);

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
	case 0: // Stage 0: AAA Dark-Fantasy Launch / Title Screen over sunlit Oakhaven Abbey & Hero
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			if (Builder) { Builder->SetTimeOfDay(0.72f); }
			Hero->HP = Hero->MaxHP;
			Hero->InvulnTimer = 30.f;
			Hero->bDragonUnlocked = true;
			const float GZ = Builder ? Builder->SampleGroundZ(0.f, 3450.f, 120.f) : 120.f;
			Hero->SetActorLocationAndRotation(FVector(0.f, 3450.f, GZ + 105.f), FRotator(0.f, 90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			bHasEnteredGameWorld = false;
			OpenMainMenuOverlay();
		}
		else if (CaptureStageElapsed >= 1.15f)
		{
			// IMPORTANT: Keep MainMenu visible through frame end so RequestScreenshot captures the Launch Screen!
			SnapFrame(TEXT("00_launch_title_screen.png"));
			CaptureStage = 1;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 1: // Stage 1: Oakhaven Village Plaza + Full-Body Kwang Hero + 3 Orbs + Golden Sunset Vista
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			EnterGameFromTitleScreen(false);
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
		else if (CaptureStageElapsed >= 1.85f)
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
					TEXT("[CaptureFrames] Stage 1 Camera->Hero Trace: Obstructed=%d | HitActor=%s | HitComp=%s | Dist=%.1fcm"),
					bObstructed ? 1 : 0,
					(bObstructed && CamHit.GetActor()) ? *CamHit.GetActor()->GetName() : TEXT("None"),
					(bObstructed && CamHit.GetComponent()) ? *CamHit.GetComponent()->GetName() : TEXT("None"),
					bObstructed ? CamHit.Distance : FVector::Dist(CamLoc, HeroChest));
			}

			SnapFrame(TEXT("01_oakhaven_kwang_hero.png"));
			CaptureStage = 2;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 2: // Stage 2: Real Niagara + Cascade Invoker Spell (SunStrike 7.5m ahead in open sunlit plaza)
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
			CaptureStage = 3;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 3: // Stage 3: Crimson Red Dragon Ignis Full-Wingspan Mounted Flight + Blazing Fire Breath over Mirror Lake & Stone Bridge toward Oakhaven!
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			if (Builder) { Builder->SetTimeOfDay(0.72f); }
			if (Dragon)
			{
				const float LakeZ = Builder ? Builder->SampleGroundZ(-780.f, -1480.f, 120.f) : 120.f;
				const FVector FlyLoc(-780.f, -1480.f, LakeZ + 460.f);
				Dragon->SetActorLocationAndRotation(FlyLoc, FRotator(0.f, 58.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
				Dragon->MountRider(Hero, this);
				SetControlRotation(FRotator(-10.f, 58.f, 0.f));
				Dragon->FireEnergy = Dragon->MaxFireEnergy;
			}
		}
		else
		{
			if (Dragon && Dragon->bMounted)
			{
				Dragon->ApplyFlightInput(0.55f, 0.02f, 0.0f, 0.f, 0.f, true, SafeDT);
			}
			if (CaptureStageElapsed >= 0.85f)
			{
				// IMPORTANT: Never DismountRider on the same tick as SnapFrame — RequestScreenshot renders at frame end!
				SnapFrame(TEXT("03_dragon_ignis_flight.png"));
				CaptureStage = 4;
				bCaptureStageSetupDone = false;
				CaptureStageElapsed = 0.f;
			}
		}
		break;

	case 4: // Stage 4: Highkeep Citadel Courtyard facing towering SevarogBloodred Malakor Boss WITH active Crimson Telegraph Glow!
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
				const float ArenaGroundZ = Builder ? Builder->SampleGroundZ(BossLoc.X, BossLoc.Y + 880.f, BossLoc.Z) : BossLoc.Z;
				const FVector HeroPos(BossLoc.X, BossLoc.Y + 880.f, ArenaGroundZ + 104.f);
				Hero->InvulnTimer = 30.f;
				Hero->SetActorLocationAndRotation(HeroPos, FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
				SetControlRotation(FRotator(-6.f, -90.f, 0.f));
			}
		}
		else if (CaptureStageElapsed >= 1.45f)
		{
			SnapFrame(TEXT("04_highkeep_malakor_boss.png"));
			CaptureStage = 5;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 5: // Stage 5: Ash Caldera & Ember Crypt Moody Gothic Lava Hall (positioned 440 cm past North Doorway so CameraBoom stays 100% inside!)
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			const float GZ = Builder ? Builder->SampleGroundZ(9800.f, -2220.f, 120.f) : 120.f;
			Hero->SetActorLocationAndRotation(FVector(9800.f, -2220.f, GZ + 104.f), FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(FRotator(-10.f, -90.f, 0.f));
		}
		else if (CaptureStageElapsed >= 1.35f)
		{
			SnapFrame(TEXT("05_ember_crypt_lava.png"));
			CaptureStage = 6;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 6: // Stage 6: 4km x 4km Fog-of-War World Map Overlay
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UFogOfWarMapSubsystem* MapSub = GI->GetSubsystem<UFogOfWarMapSubsystem>())
				{
					// Reveal a broad multi-biome scenic swath across all 5 realms so the showcase atlas demonstrates rich cartography
					MapSub->RevealAroundWorldLocation(FVector(0.f, 4200.f, 0.f), 68000.f);         // Central Oakhaven & Heartland
					MapSub->RevealAroundWorldLocation(FVector(-32000.f, 52000.f, 0.f), 62000.f);   // Southern Oakhaven Valley & Dragon Aerie approaches
					MapSub->RevealAroundWorldLocation(FVector(74000.f, 44000.f, 0.f), 60000.f);    // Sapphire Whispering Woods
					MapSub->RevealAroundWorldLocation(FVector(0.f, -72000.f, 0.f), 66000.f);       // Highkeep Citadel Northern Spine
					MapSub->RevealAroundWorldLocation(FVector(78000.f, -34000.f, 0.f), 62000.f);   // Ash Caldera & Ember Crypt
					MapSub->RevealAroundWorldLocation(FVector(-76000.f, -48000.f, 0.f), 64000.f);  // Crystal Glacier Peaks
				}
			}
			ToggleOverlay(EChroniclesOverlayMode::WorldMap);
		}
		else if (CaptureStageElapsed >= 0.85f)
		{
			SnapFrame(TEXT("06_world_map_overlay.png"));
			CaptureStage = 7;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 7: // Stage 7: Live 3D Hero Customizer Studio Overlay facing sunlit Oakhaven shrine & courtyard
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			CloseAllOverlays();
			if (Builder) { Builder->SetTimeOfDay(0.72f); }
			const float GZ = Builder ? Builder->SampleGroundZ(0.f, 3720.f, 120.f) : 120.f;
			Hero->SetActorLocationAndRotation(FVector(0.f, 3720.f, GZ + 105.f), FRotator(0.f, 145.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(FRotator(-6.f, 145.f, 0.f));
			CreatorZoomMode = 0;
			CreatorOrbitYaw = 162.f;
			ToggleOverlay(EChroniclesOverlayMode::HeroCustomizer);
		}
		else if (CaptureStageElapsed >= 1.35f)
		{
			SnapFrame(TEXT("07_hero_customizer_studio.png"));
			CaptureStage = 8;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 8: // Stage 8: Prologue Panoramic Cliff Overlook Vista beside Roaring Waterfall over Mirror Lake, Stone Bridge, Meadows & Oakhaven!
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			CloseAllOverlays();
			if (Builder) { Builder->SetTimeOfDay(0.72f); }
			const FVector BrinkLoc = Builder
				? Builder->GetPrologueWaterfallBrinkLocation()
				: FVector(-1980.f, -800.f, 2040.f);
			// Stand on Natural Megascans Rocky Overlook Brink (-1985, -560, CliffZ + 68) looking East-Northeast (Yaw +18, Pitch -19)
			// across Gleaming Oval Mirror Lake, the 9-Arch Stone Bridge, Roaring Waterfall Mist, Lush Wildflower Meadows & Oakhaven Village!
			const FVector OverlookPos(BrinkLoc.X - 5.f, -560.f, BrinkLoc.Z + 68.f);
			Hero->SetActorLocationAndRotation(OverlookPos, FRotator(0.f, 18.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(FRotator(-19.f, 18.f, 0.f));
		}
		else if (CaptureStageElapsed >= 1.45f)
		{
			SnapFrame(TEXT("08_prologue_cliff_overlook_vista.png"));
			CaptureStage = 9;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 9: // Stage 9: In-Game ` Developer Console + /godmode Active (Immortal + 10/10 Spells + Meadow Spell Terrain Scar — 100% clear of tree canopies!)
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			CloseAllOverlays();
			if (Builder) { Builder->SetTimeOfDay(0.70f); }
			// Wide-open sunlit King's Road & grassy wildflower verge at (0, 2650) facing South (-90) toward Mirror Lake & Stone Bridge!
			const float GZ = Builder ? Builder->SampleGroundZ(0.f, 2650.f, 120.f) : 120.f;
			Hero->SetActorLocationAndRotation(FVector(0.f, 2650.f, GZ + 104.f), FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(FRotator(-9.f, -90.f, 0.f));

			Hero->SetGodMode(false);
			SetDeveloperConsoleOpen(true);
			ExecuteDeveloperConsoleCommand(TEXT("/godmode"));
			if (HUDWidget.IsValid() && HUDWidget->SupportsKeyboardFocus())
			{
				HUDWidget->SyncDeveloperConsoleVisibilityAndFocus(true, true);
			}

			if (Hero->SpellSystem)
			{
				Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->InvokeCurrentSpell();
				Hero->SpellSystem->PushOrb(EChronicleOrb::Exort);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Wex);
				Hero->SpellSystem->PushOrb(EChronicleOrb::Quas);
				const FVector Fwd = Hero->GetActorForwardVector();
				const FVector Target = Hero->GetActorLocation() + Fwd * 620.f + FVector(-220.f, 0.f, 0.f);
				Hero->SpellSystem->CastSpellSlot(false, Hero->GetActorLocation() + FVector(0.f, 0.f, 68.f), (Target - Hero->GetActorLocation()).GetSafeNormal(), Target, 0.85f);
			}
		}
		else if (CaptureStageElapsed >= 0.65f)
		{
			SnapFrame(TEXT("09_dev_console_godmode.png"));
			CaptureStage = 10;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	case 10: // Stage 10: Inside Warm Torchlit Rocky Prologue Awakening Cave facing Tutor Vaelen & Sunlit Cave Mouth!
		if (!bCaptureStageSetupDone)
		{
			bCaptureStageSetupDone = true;
			CaptureStageElapsed = 0.f;
			SetDeveloperConsoleOpen(false);
			SetUIMode(false);
			Hero->SetGodMode(false);
			PushToastMessage(TEXT("ПЕЩЕРА ПРОБУЖДЕНИЯ · Поговорите с Хранителем Вейленом [E]"), 5.0f);
			if (Builder) { Builder->SetTimeOfDay(0.72f); }
			const FVector CaveSpawnLoc = Builder
				? (Builder->GetPrologueCaveSpawnLocation() + FVector(0.f, 115.f, 0.f))
				: FVector(-3210.f, -685.f, 2098.f);
			Hero->SetActorLocationAndRotation(CaveSpawnLoc, FRotator(0.f, -12.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			SetControlRotation(FRotator(-4.f, -12.f, 0.f));
		}
		else if (CaptureStageElapsed >= 1.25f)
		{
			SnapFrame(TEXT("10_prologue_awakening_cave_tutor.png"));
			CaptureStage = 11;
			bCaptureStageSetupDone = false;
			CaptureStageElapsed = 0.f;
		}
		break;

	default: // Stage 11: Flush last PNG to disk then exit cleanly
		if (CaptureStageElapsed >= 0.90f)
		{
			SetDeveloperConsoleOpen(false);
			UE_LOG(LogWizardsmoke, Display, TEXT("[CaptureFrames] All 11 critic frames saved cleanly! Exiting."));
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
	// Rich muted dark-fantasy cartographic biome tones (crimson desert / antique parchment atlas)
	switch (Biome)
	{
	case EChronicleBiome::OakhavenValley:  return FLinearColor(0.34f, 0.36f, 0.20f, 0.96f); // Golden-olive valley
	case EChronicleBiome::BlueLeafWoods:   return FLinearColor(0.13f, 0.28f, 0.36f, 0.96f); // Deep twilight-teal woods
	case EChronicleBiome::HighkeepCitadel: return FLinearColor(0.32f, 0.14f, 0.18f, 0.96f); // Dark crimson-slate citadel
	case EChronicleBiome::AshCalderaCrypt: return FLinearColor(0.42f, 0.20f, 0.12f, 0.96f); // Volcanic ember-umber caldera
	case EChronicleBiome::CrystalGlacier:  return FLinearColor(0.30f, 0.44f, 0.56f, 0.96f); // Frost-slate glacier peaks
	default:                               return FLinearColor(0.24f, 0.23f, 0.26f, 0.96f);
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
	if (LocalSize.X <= 8.f || LocalSize.Y <= 8.f || !WhiteBrush)
	{
		return LayerId;
	}

	// 1. Deep weathered obsidian-parchment unexplored shroud
	const FLinearColor ShroudColor(0.045f, 0.042f, 0.052f, 1.0f);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(LocalSize, FSlateLayoutTransform(FVector2f::ZeroVector)),
		WhiteBrush,
		ESlateDrawEffect::None,
		ShroudColor);

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

	// 2. Smooth multi-biome cartographic territory with subtle elevation relief & feathered fog-of-war border
	const int32 GridLayer = LayerId + 1;
	const int32 FeatherLayer = GridLayer + 1;

	auto SampleNeighborCoverage = [MapSub](int32 CX, int32 CY) -> float
	{
		float Sum = 0.f;
		float WeightTotal = 0.f;
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			for (int32 dx = -1; dx <= 1; ++dx)
			{
				const float W = (dx == 0 && dy == 0) ? 2.2f : ((dx == 0 || dy == 0) ? 1.2f : 0.7f);
				WeightTotal += W;
				if (MapSub->IsCellExplored(CX + dx, CY + dy))
				{
					Sum += W;
				}
			}
		}
		return (WeightTotal > 0.f) ? (Sum / WeightTotal) : 0.f;
	};

	for (int32 Y = 0; Y < GridDim; ++Y)
	{
		const float WorldY = -Extent + (static_cast<float>(Y) + 0.5f) * CellWorld;
		const float ScreenY = static_cast<float>(Y) * CellH;

		for (int32 X = 0; X < GridDim; ++X)
		{
			const bool bExplored = MapSub->IsCellExplored(X, Y);
			const float Coverage = SampleNeighborCoverage(X, Y);
			if (!bExplored && Coverage <= 0.04f)
			{
				continue;
			}

			const float WorldX = -Extent + (static_cast<float>(X) + 0.5f) * CellWorld;
			const EChronicleBiome Biome = MapSub->GetBiomeAtLocation(FVector(WorldX, WorldY, 0.f));
			FLinearColor TileColor = GetBiomeMapColor(Biome);

			// Subtle cartographic relief contour modulation so explored realms feel hand-painted
			const float Relief = 0.86f
				+ 0.14f * FMath::Sin(static_cast<float>(X) * 0.34f) * FMath::Cos(static_cast<float>(Y) * 0.31f)
				+ 0.06f * FMath::Sin(static_cast<float>(X + Y) * 0.22f);
			TileColor.R = FMath::Clamp(TileColor.R * Relief, 0.f, 1.f);
			TileColor.G = FMath::Clamp(TileColor.G * Relief, 0.f, 1.f);
			TileColor.B = FMath::Clamp(TileColor.B * Relief, 0.f, 1.f);

			if (bExplored)
			{
				// Soften frontier cells into weathered parchment edge
				const float EdgeBlend = FMath::Clamp((Coverage - 0.28f) / 0.72f, 0.25f, 1.0f);
				FLinearColor FinalColor = FMath::Lerp(ShroudColor, TileColor, EdgeBlend);
				FinalColor.A = 1.0f;

				FSlateDrawElement::MakeBox(
					OutDrawElements,
					GridLayer,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(CellW + 1.2f, CellH + 1.2f),
						FSlateLayoutTransform(FVector2f(static_cast<float>(X) * CellW - 0.6f, ScreenY - 0.6f))),
					WhiteBrush,
					ESlateDrawEffect::None,
					FinalColor);
			}
			else
			{
				// Soft translucent mist feather along the fog-of-war boundary (eliminates blocky staircase edges)
				const float MistAlpha = FMath::Clamp(Coverage * 0.45f, 0.0f, 0.38f);
				const FLinearColor MistColor(
					FMath::Lerp(ShroudColor.R, TileColor.R * 0.75f, 0.6f),
					FMath::Lerp(ShroudColor.G, TileColor.G * 0.75f, 0.6f),
					FMath::Lerp(ShroudColor.B, TileColor.B * 0.75f, 0.6f),
					MistAlpha);

				FSlateDrawElement::MakeBox(
					OutDrawElements,
					FeatherLayer,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(CellW + 1.5f, CellH + 1.5f),
						FSlateLayoutTransform(FVector2f(static_cast<float>(X) * CellW - 0.75f, ScreenY - 0.75f))),
					WhiteBrush,
					ESlateDrawEffect::None,
					MistColor);
			}
		}
	}

	// 3. Subtle Antique Gold Latitude/Longitude Cartographic Grid (Alpha 0.14) & Meridians
	const int32 GridLineLayer = FeatherLayer + 1;
	const FLinearColor LatLonColor(0.76f, 0.62f, 0.34f, 0.14f);
	const FLinearColor MeridianColor(0.84f, 0.68f, 0.36f, 0.24f);
	for (int32 Step = 1; Step < 8; ++Step)
	{
		const float Frac = static_cast<float>(Step) / 8.f;
		const bool bCenterLine = (Step == 4);
		const FLinearColor LineCol = bCenterLine ? MeridianColor : LatLonColor;
		const float LineThick = bCenterLine ? 1.2f : 1.0f;

		FSlateDrawElement::MakeLines(
			OutDrawElements, GridLineLayer, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{ FVector2f(0.f, LocalSize.Y * Frac), FVector2f(LocalSize.X, LocalSize.Y * Frac) },
			ESlateDrawEffect::None, LineCol, true, LineThick);

		FSlateDrawElement::MakeLines(
			OutDrawElements, GridLineLayer, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{ FVector2f(LocalSize.X * Frac, 0.f), FVector2f(LocalSize.X * Frac, LocalSize.Y) },
			ESlateDrawEffect::None, LineCol, true, LineThick);
	}

	// 4. Subtle River & Cartographic Trade-Road Contours connecting the 5 realms
	const int32 RouteLayer = GridLineLayer + 1;
	const auto NormPt = [&LocalSize](float NX, float NY) -> FVector2f
	{
		return FVector2f(NX * LocalSize.X, NY * LocalSize.Y);
	};

	// Mirror Lake Sapphire River contour winding through the eastern valley
	const FLinearColor RiverColor(0.24f, 0.62f, 0.82f, 0.42f);
	FSlateDrawElement::MakeLines(
		OutDrawElements, RouteLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{
			NormPt(0.32f, 0.36f), NormPt(0.42f, 0.43f), NormPt(0.51f, 0.50f),
			NormPt(0.60f, 0.53f), NormPt(0.69f, 0.60f), NormPt(0.76f, 0.72f)
		},
		ESlateDrawEffect::None, RiverColor, true, 2.2f);

	// Ochre-Gold Imperial Roads: Oakhaven <-> Mirror Lake Bridge <-> Highkeep / Ember Crypt / Glacier / Woods
	const FLinearColor RoadColor(0.78f, 0.62f, 0.34f, 0.36f);
	// Spine road: Oakhaven -> Mirror Lake -> Highkeep Citadel
	FSlateDrawElement::MakeLines(
		OutDrawElements, RouteLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{
			NormPt(0.50f, 0.74f), NormPt(0.51f, 0.62f), NormPt(0.51f, 0.51f),
			NormPt(0.49f, 0.39f), NormPt(0.50f, 0.25f)
		},
		ESlateDrawEffect::None, RoadColor, true, 1.5f);
	// West spur: Oakhaven -> Dragon Aerie -> Crystal Glacier Peaks
	FSlateDrawElement::MakeLines(
		OutDrawElements, RouteLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{
			NormPt(0.50f, 0.62f), NormPt(0.38f, 0.56f), NormPt(0.24f, 0.52f),
			NormPt(0.22f, 0.41f), NormPt(0.23f, 0.30f)
		},
		ESlateDrawEffect::None, RoadColor, true, 1.3f);
	// East spurs: Mirror Lake -> Ash Caldera & Ember Crypt + Whispering Woods
	FSlateDrawElement::MakeLines(
		OutDrawElements, RouteLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{
			NormPt(0.51f, 0.51f), NormPt(0.65f, 0.44f), NormPt(0.78f, 0.35f)
		},
		ESlateDrawEffect::None, RoadColor, true, 1.3f);
	FSlateDrawElement::MakeLines(
		OutDrawElements, RouteLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{
			NormPt(0.51f, 0.51f), NormPt(0.64f, 0.57f), NormPt(0.76f, 0.64f)
		},
		ESlateDrawEffect::None, RoadColor, true, 1.3f);

	// 5. Ornate Antique Gold Compass Rose (lower-left corner)
	const int32 CompassLayer = RouteLayer + 1;
	const FVector2f RoseCenter(46.f, LocalSize.Y - 46.f);
	const float RoseR = 22.f;
	const FLinearColor RoseGold(0.82f, 0.66f, 0.34f, 0.72f);
	const FLinearColor RoseDim(0.76f, 0.59f, 0.28f, 0.34f);

	// Compass cardinal & diagonal rays
	FSlateDrawElement::MakeLines(
		OutDrawElements, CompassLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ RoseCenter + FVector2f(0.f, -RoseR), RoseCenter + FVector2f(0.f, RoseR) },
		ESlateDrawEffect::None, RoseGold, true, 1.4f);
	FSlateDrawElement::MakeLines(
		OutDrawElements, CompassLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ RoseCenter + FVector2f(-RoseR, 0.f), RoseCenter + FVector2f(RoseR, 0.f) },
		ESlateDrawEffect::None, RoseGold, true, 1.4f);
	const float DiagR = RoseR * 0.62f;
	FSlateDrawElement::MakeLines(
		OutDrawElements, CompassLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ RoseCenter + FVector2f(-DiagR, -DiagR), RoseCenter + FVector2f(DiagR, DiagR) },
		ESlateDrawEffect::None, RoseDim, true, 1.0f);
	FSlateDrawElement::MakeLines(
		OutDrawElements, CompassLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ RoseCenter + FVector2f(-DiagR, DiagR), RoseCenter + FVector2f(DiagR, -DiagR) },
		ESlateDrawEffect::None, RoseDim, true, 1.0f);

	// Compass octagon ring
	TArray<FVector2f> RingPts;
	RingPts.Reserve(17);
	for (int32 i = 0; i <= 16; ++i)
	{
		const float Ang = (static_cast<float>(i) / 16.f) * UE_TWO_PI;
		RingPts.Add(RoseCenter + FVector2f(FMath::Cos(Ang) * (RoseR * 0.72f), FMath::Sin(Ang) * (RoseR * 0.72f)));
	}
	FSlateDrawElement::MakeLines(
		OutDrawElements, CompassLayer, AllottedGeometry.ToPaintGeometry(),
		MoveTemp(RingPts), ESlateDrawEffect::None, RoseDim, true, 1.1f);

	const FSlateFontInfo CompassFont = ChroniclesHUDPalette::FontBold(8);
	FSlateDrawElement::MakeText(
		OutDrawElements, CompassLayer + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(16.f, 14.f), FSlateLayoutTransform(RoseCenter + FVector2f(-4.f, -RoseR - 12.f))),
		TEXT("N"), CompassFont, ESlateDrawEffect::None, ChroniclesHUDPalette::TextGold);
	FSlateDrawElement::MakeText(
		OutDrawElements, CompassLayer + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(16.f, 14.f), FSlateLayoutTransform(RoseCenter + FVector2f(-3.5f, RoseR + 2.f))),
		TEXT("S"), CompassFont, ESlateDrawEffect::None, RoseGold);
	FSlateDrawElement::MakeText(
		OutDrawElements, CompassLayer + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(16.f, 14.f), FSlateLayoutTransform(RoseCenter + FVector2f(RoseR + 4.f, -5.5f))),
		TEXT("E"), CompassFont, ESlateDrawEffect::None, RoseGold);
	FSlateDrawElement::MakeText(
		OutDrawElements, CompassLayer + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(16.f, 14.f), FSlateLayoutTransform(RoseCenter + FVector2f(-RoseR - 12.f, -5.5f))),
		TEXT("W"), CompassFont, ESlateDrawEffect::None, RoseGold);

	// 6. Landmark Pins & Crisp Non-Overlapping Regional Cartographic Labels
	const int32 PinLayer = CompassLayer + 2;
	const FSlateFontInfo LabelFont = ChroniclesHUDPalette::FontBold(9);
	const FSlateFontInfo SubFont = ChroniclesHUDPalette::FontRegular(8);

	struct FLandmarkCallout
	{
		const TCHAR* PinIdStr;
		const TCHAR* ShortLabel;
		float LabelNormX;
		float LabelNormY;
	};
	static const FLandmarkCallout Callouts[] = {
		{ TEXT("Pin_HighkeepCitadel"), TEXT("Highkeep Citadel"), 0.50f, 0.17f },
		{ TEXT("Pin_CrystalGlacier"),  TEXT("Glacier Peaks"),    0.20f, 0.27f },
		{ TEXT("Pin_EmberCrypt"),      TEXT("Ember Crypt"),      0.80f, 0.31f },
		{ TEXT("Pin_DragonAerie"),     TEXT("Dragon Aerie"),     0.21f, 0.53f },
		{ TEXT("Pin_MirrorLake"),      TEXT("Mirror Lake"),      0.77f, 0.50f },
		{ TEXT("Pin_BlueLeafWoods"),   TEXT("Whispering Woods"), 0.77f, 0.67f },
		{ TEXT("Pin_OakhavenVillage"), TEXT("Oakhaven"),         0.50f, 0.77f }
	};

	const TArray<FMapPinInfo>& Pins = MapSub->GetMapPins();
	for (const FMapPinInfo& Pin : Pins)
	{
		const FVector2D UV = MapSub->WorldToNormalizedUV(Pin.WorldLocation);
		const float PX = static_cast<float>(UV.X) * LocalSize.X;
		const float PY = static_cast<float>(UV.Y) * LocalSize.Y;
		const float PinRadius = Pin.bDiscovered ? (Pin.bFastTravel ? 4.5f : 5.0f) : 2.5f;
		const FLinearColor PinColor = Pin.bDiscovered
			? Pin.Color
			: FLinearColor(0.55f, 0.52f, 0.48f, 0.34f);

		// Soft outer glow halo for discovered landmarks
		if (Pin.bDiscovered)
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements, PinLayer,
				AllottedGeometry.ToPaintGeometry(
					FVector2f((PinRadius + 4.f) * 2.f, (PinRadius + 4.f) * 2.f),
					FSlateLayoutTransform(FVector2f(PX - PinRadius - 4.f, PY - PinRadius - 4.f))),
				WhiteBrush, ESlateDrawEffect::None, FLinearColor(PinColor.R, PinColor.G, PinColor.B, 0.26f));
		}

		// Dark obsidian pin border
		FSlateDrawElement::MakeBox(
			OutDrawElements, PinLayer + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f((PinRadius + 1.5f) * 2.f, (PinRadius + 1.5f) * 2.f),
				FSlateLayoutTransform(FVector2f(PX - PinRadius - 1.5f, PY - PinRadius - 1.5f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.02f, 0.018f, 0.028f, 0.94f));

		// Glowing pin jewel
		FSlateDrawElement::MakeBox(
			OutDrawElements, PinLayer + 2,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(PinRadius * 2.f, PinRadius * 2.f),
				FSlateLayoutTransform(FVector2f(PX - PinRadius, PY - PinRadius))),
			WhiteBrush, ESlateDrawEffect::None, PinColor);

		// Render crisp readable regional label badge + hairline gold leader line for major landmarks
		if (Pin.bDiscovered)
		{
			for (const FLandmarkCallout& Callout : Callouts)
			{
				if (Pin.PinId == FName(Callout.PinIdStr))
				{
					const FVector2f BadgeCenter = NormPt(Callout.LabelNormX, Callout.LabelNormY);
					const FString LabelStr(Callout.ShortLabel);
					const float PillW = FMath::Max(76.f, static_cast<float>(LabelStr.Len()) * 6.8f + 16.f);
					const float PillH = 19.f;
					const FVector2f PillPos(
						FMath::Clamp(BadgeCenter.X - PillW * 0.5f, 10.f, LocalSize.X - PillW - 10.f),
						FMath::Clamp(BadgeCenter.Y - PillH * 0.5f, 10.f, LocalSize.Y - PillH - 10.f));

					// Subtle cartographic gold leader line from true world pin to regional label pill
					FSlateDrawElement::MakeLines(
						OutDrawElements, PinLayer + 1, AllottedGeometry.ToPaintGeometry(),
						TArray<FVector2f>{ FVector2f(PX, PY), FVector2f(PillPos.X + PillW * 0.5f, PillPos.Y + PillH * 0.5f) },
						ESlateDrawEffect::None, FLinearColor(PinColor.R, PinColor.G, PinColor.B, 0.36f), true, 1.0f);

					// Dark obsidian pill + antique gold rim
					FSlateDrawElement::MakeBox(
						OutDrawElements, PinLayer + 3,
						AllottedGeometry.ToPaintGeometry(FVector2f(PillW + 2.f, PillH + 2.f), FSlateLayoutTransform(PillPos - FVector2f(1.f, 1.f))),
						WhiteBrush, ESlateDrawEffect::None, FLinearColor(PinColor.R, PinColor.G, PinColor.B, 0.58f));
					FSlateDrawElement::MakeBox(
						OutDrawElements, PinLayer + 4,
						AllottedGeometry.ToPaintGeometry(FVector2f(PillW, PillH), FSlateLayoutTransform(PillPos)),
						WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.035f, 0.030f, 0.044f, 0.88f));

					// Colored pin jewel inside pill + crisp label text
					FSlateDrawElement::MakeBox(
						OutDrawElements, PinLayer + 5,
						AllottedGeometry.ToPaintGeometry(FVector2f(5.f, 5.f), FSlateLayoutTransform(PillPos + FVector2f(5.f, 7.f))),
						WhiteBrush, ESlateDrawEffect::None, PinColor);

					FSlateDrawElement::MakeText(
						OutDrawElements, PinLayer + 5,
						AllottedGeometry.ToPaintGeometry(FVector2f(PillW - 14.f, PillH), FSlateLayoutTransform(PillPos + FVector2f(13.f, 2.5f))),
						LabelStr, LabelFont, ESlateDrawEffect::None, ChroniclesHUDPalette::TextPrimary);
					break;
				}
			}
		}
	}

	// 7. Malakor Red Skull Boss Marker at Highkeep Citadel (north spine)
	const int32 BossLayer = PinLayer + 6;
	{
		const FVector2f BossCenter = NormPt(0.50f, 0.25f);
		const float PulseTime = (PC && PC->GetWorld()) ? PC->GetWorld()->GetTimeSeconds() : 0.f;
		const float BossPulse = 0.65f + 0.35f * FMath::Abs(FMath::Sin(PulseTime * 3.5f));

		// Pulsing crimson threat ring
		FSlateDrawElement::MakeBox(
			OutDrawElements, BossLayer,
			AllottedGeometry.ToPaintGeometry(FVector2f(28.f, 28.f), FSlateLayoutTransform(BossCenter - FVector2f(14.f, 14.f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.88f, 0.12f, 0.14f, 0.34f * BossPulse));

		if (const FSlateBrush* SkullBrush = ChroniclesHUDPalette::SkullCrestBrush())
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements, BossLayer + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(24.f, 24.f), FSlateLayoutTransform(BossCenter - FVector2f(12.f, 12.f))),
				SkullBrush, ESlateDrawEffect::None, FLinearColor(1.0f, 0.42f, 0.38f, 0.98f));
		}
		else
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements, BossLayer + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(12.f, 12.f), FSlateLayoutTransform(BossCenter - FVector2f(6.f, 6.f))),
				WhiteBrush, ESlateDrawEffect::None, ChroniclesHUDPalette::TextCrimson);
		}

		FSlateDrawElement::MakeText(
			OutDrawElements, BossLayer + 2,
			AllottedGeometry.ToPaintGeometry(FVector2f(76.f, 14.f), FSlateLayoutTransform(BossCenter + FVector2f(-26.f, 13.f))),
			TEXT("MALAKOR"), SubFont, ESlateDrawEffect::None, ChroniclesHUDPalette::TextCrimson);
	}

	// 8. Pulsing Gold Hero Directional Marker
	const int32 PlayerLayer = BossLayer + 3;
	if (const APawn* ActivePawn = PC->GetPawn())
	{
		const FVector2D PlayerUV = MapSub->WorldToNormalizedUV(ActivePawn->GetActorLocation());
		const float UX = static_cast<float>(PlayerUV.X) * LocalSize.X;
		const float UY = static_cast<float>(PlayerUV.Y) * LocalSize.Y;
		const float Now = (PC && PC->GetWorld()) ? PC->GetWorld()->GetTimeSeconds() : 0.f;
		const float Pulse = 0.65f + 0.35f * FMath::Sin(Now * 4.5f);

		// Pulsing radiant gold halo
		const float HaloSize = 18.f + 4.f * Pulse;
		FSlateDrawElement::MakeBox(
			OutDrawElements, PlayerLayer,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(HaloSize, HaloSize),
				FSlateLayoutTransform(FVector2f(UX - HaloSize * 0.5f, UY - HaloSize * 0.5f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.98f, 0.78f, 0.28f, 0.34f * Pulse));

		// Dark obsidian core outline + bright burnished gold Hero jewel
		FSlateDrawElement::MakeBox(
			OutDrawElements, PlayerLayer + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(12.f, 12.f), FSlateLayoutTransform(FVector2f(UX - 6.f, UY - 6.f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.03f, 0.02f, 0.03f, 0.96f));
		FSlateDrawElement::MakeBox(
			OutDrawElements, PlayerLayer + 2,
			AllottedGeometry.ToPaintGeometry(FVector2f(8.f, 8.f), FSlateLayoutTransform(FVector2f(UX - 4.f, UY - 4.f))),
			WhiteBrush, ESlateDrawEffect::None, ChroniclesHUDPalette::TextGold);

		// Directional chevron along actor forward XY
		const FVector Fwd = ActivePawn->GetActorForwardVector().GetSafeNormal2D();
		const FVector2f Fwd2(static_cast<float>(Fwd.X), static_cast<float>(Fwd.Y));
		const FVector2f Right2(-Fwd2.Y, Fwd2.X);
		const FVector2f HeroPos(UX, UY);
		const FVector2f Tip = HeroPos + Fwd2 * 14.f;
		const FVector2f WingL = HeroPos + Fwd2 * 7.f - Right2 * 5.f;
		const FVector2f WingR = HeroPos + Fwd2 * 7.f + Right2 * 5.f;

		FSlateDrawElement::MakeLines(
			OutDrawElements, PlayerLayer + 3, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{ WingL, Tip, WingR },
			ESlateDrawEffect::None, FLinearColor(1.0f, 0.92f, 0.58f, 1.0f), true, 2.2f);
	}

	// 9. Ornate Double Antique Gold Frame around the Cartographic Canvas
	const int32 FrameLayer = PlayerLayer + 4;
	auto DrawRectFrame = [&](float Inset, const FLinearColor& Col, float Thick)
	{
		FSlateDrawElement::MakeLines(
			OutDrawElements, FrameLayer, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{
				FVector2f(Inset, Inset),
				FVector2f(LocalSize.X - Inset, Inset),
				FVector2f(LocalSize.X - Inset, LocalSize.Y - Inset),
				FVector2f(Inset, LocalSize.Y - Inset),
				FVector2f(Inset, Inset)
			},
			ESlateDrawEffect::None, Col, true, Thick);
	};
	DrawRectFrame(1.5f, ChroniclesHUDPalette::HairlineGold, 1.6f);
	DrawRectFrame(6.0f, FLinearColor(0.76f, 0.59f, 0.28f, 0.45f), 1.0f);

	return FrameLayer + 1;
}

// ============================================================================
// 4B. SChroniclesMinimapWidget — 172x172 Round Bottom-Right Radar Minimap
// ============================================================================

void SChroniclesMinimapWidget::Construct(const FArguments& InArgs)
{
	OwnerPC = InArgs._OwnerPC;
	DesiredSizePx = FMath::Max(120.f, InArgs._MinimapSize);
}

FVector2D SChroniclesMinimapWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(DesiredSizePx, DesiredSizePx);
}

int32 SChroniclesMinimapWidget::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
	const float Side = FMath::Min(LocalSize.X, LocalSize.Y);
	const FVector2f Center = LocalSize * 0.5f;
	const float DiscRadius = Side * 0.43f;  // ~74px at 172x172
	const float PlotRadius = DiscRadius - 11.f;
	const float RadarRangeCm = 4200.f;

	const FSlateBrush* DiscBrush = ChroniclesHUDPalette::MinimapDiscBrush();
	const FSlateBrush* BlipBrush = ChroniclesHUDPalette::BlipCircleBrush();

	// 1. Circular Dark Obsidian Radar Disc (GPU SDF Anti-Aliased Circle — pass dark obsidian tint explicitly because MakeBox ignores Brush->TintColor!)
	const float OuterRimR = DiscRadius + 5.5f;
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(OuterRimR * 2.f, OuterRimR * 2.f),
			FSlateLayoutTransform(Center - FVector2f(OuterRimR, OuterRimR))),
		DiscBrush, ESlateDrawEffect::None, FLinearColor(0.018f, 0.016f, 0.026f, 0.95f));

	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(DiscRadius * 2.f, DiscRadius * 2.f),
			FSlateLayoutTransform(Center - FVector2f(DiscRadius, DiscRadius))),
		DiscBrush, ESlateDrawEffect::None, FLinearColor(0.032f, 0.029f, 0.044f, 0.92f));

	const AChroniclesPlayerController* PC = OwnerPC.Get();
	const APawn* ActivePawn = PC ? PC->GetPawn() : nullptr;
	const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
	UWorld* World = PC ? PC->GetWorld() : nullptr;
	UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
	const UFogOfWarMapSubsystem* MapSub = GI ? GI->GetSubsystem<UFogOfWarMapSubsystem>() : nullptr;
	const UQuestSubsystem* QuestSub = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;

	// Subtle biome tint inside radar disc
	if (ActivePawn && MapSub)
	{
		const EChronicleBiome Biome = MapSub->GetBiomeAtLocation(ActivePawn->GetActorLocation());
		FLinearColor BiomeTint(0.24f, 0.30f, 0.16f, 0.14f);
		switch (Biome)
		{
		case EChronicleBiome::BlueLeafWoods:   BiomeTint = FLinearColor(0.12f, 0.30f, 0.38f, 0.16f); break;
		case EChronicleBiome::HighkeepCitadel: BiomeTint = FLinearColor(0.32f, 0.14f, 0.18f, 0.16f); break;
		case EChronicleBiome::AshCalderaCrypt: BiomeTint = FLinearColor(0.42f, 0.18f, 0.10f, 0.18f); break;
		case EChronicleBiome::CrystalGlacier:  BiomeTint = FLinearColor(0.22f, 0.36f, 0.48f, 0.16f); break;
		default: break;
		}
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(
				FVector2f((DiscRadius - 3.f) * 2.f, (DiscRadius - 3.f) * 2.f),
				FSlateLayoutTransform(Center - FVector2f(DiscRadius - 3.f, DiscRadius - 3.f))),
			DiscBrush, ESlateDrawEffect::None, BiomeTint);
	}

	// 2. Concentric Range Ring & Subtle Cardinal Crosshair Hairlines (drawn via antialiased MakeLines — zero white fill!)
	const int32 GridLayer = LayerId + 3;
	const float InnerR = DiscRadius * 0.56f;
	{
		TArray<FVector2f> InnerRingPts;
		InnerRingPts.Reserve(37);
		for (int32 i = 0; i <= 36; ++i)
		{
			const float Ang = (static_cast<float>(i) / 36.f) * UE_TWO_PI;
			InnerRingPts.Add(Center + FVector2f(FMath::Cos(Ang) * InnerR, FMath::Sin(Ang) * InnerR));
		}
		FSlateDrawElement::MakeLines(
			OutDrawElements, GridLayer, AllottedGeometry.ToPaintGeometry(),
			MoveTemp(InnerRingPts), ESlateDrawEffect::None, FLinearColor(0.76f, 0.59f, 0.28f, 0.24f), true, 1.0f);
	}

	const FLinearColor CrossCol(0.76f, 0.59f, 0.28f, 0.15f);
	const float CrossHalf = DiscRadius - 8.f;
	FSlateDrawElement::MakeLines(
		OutDrawElements, GridLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ Center + FVector2f(0.f, -CrossHalf), Center + FVector2f(0.f, CrossHalf) },
		ESlateDrawEffect::None, CrossCol, true, 1.0f);
	FSlateDrawElement::MakeLines(
		OutDrawElements, GridLayer, AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2f>{ Center + FVector2f(-CrossHalf, 0.f), Center + FVector2f(CrossHalf, 0.f) },
		ESlateDrawEffect::None, CrossCol, true, 1.0f);

	const int32 BlipLayer = GridLayer + 1;
	auto DrawDotBlip = [&](const FVector2f& Pos, float Radius, const FLinearColor& Col, int32 Layer)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(Radius * 2.f, Radius * 2.f),
				FSlateLayoutTransform(Pos - FVector2f(Radius, Radius))),
			BlipBrush, ESlateDrawEffect::None, Col);
	};

	auto DrawDiamondBlip = [&](const FVector2f& Pos, float HalfSize, const FLinearColor& OuterCol, const FLinearColor& CoreCol, int32 Layer)
	{
		TArray<FVector2f> Pts{
			Pos + FVector2f(0.f, -HalfSize),
			Pos + FVector2f(HalfSize, 0.f),
			Pos + FVector2f(0.f, HalfSize),
			Pos + FVector2f(-HalfSize, 0.f),
			Pos + FVector2f(0.f, -HalfSize)
		};
		FSlateDrawElement::MakeLines(
			OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(),
			MoveTemp(Pts), ESlateDrawEffect::None, OuterCol, true, 1.8f);
		DrawDotBlip(Pos, FMath::Max(2.0f, HalfSize * 0.42f), CoreCol, Layer + 1);
	};

	if (PC && ActivePawn && World)
	{
		const FVector Origin = ActivePawn->GetActorLocation();
		const float Pulse = 0.5f + 0.5f * FMath::Sin(World->GetTimeSeconds() * 4.5f);

		auto WorldToRadarPos = [&](const FVector& TargetLoc, bool bClampToRim, FVector2f& OutPos) -> bool
		{
			// North (-Y) is UP on screen, East (+X) is RIGHT — 100% aligned with Aetheria compass & Atlas Map
			FVector2f Offset(
				static_cast<float>((TargetLoc.X - Origin.X) / RadarRangeCm) * PlotRadius,
				static_cast<float>((TargetLoc.Y - Origin.Y) / RadarRangeCm) * PlotRadius);
			const float Len = Offset.Size();
			if (Len > PlotRadius)
			{
				if (!bClampToRim)
				{
					return false;
				}
				Offset = (Offset / FMath::Max(0.001f, Len)) * PlotRadius;
			}
			OutPos = Center + Offset;
			return true;
		};

		// 3A. Roaming Enemies (Crimson Dots) & Boss Malakor (Larger Pulsing Blood-Red Marker)
		for (TActorIterator<AAetheriaEnemyCharacter> It(World); It; ++It)
		{
			const AAetheriaEnemyCharacter* Enemy = *It;
			if (!IsValid(Enemy) || Enemy->bDead || Enemy->IsHidden())
			{
				continue;
			}
			const bool bIsBoss = Enemy->IsA<AAetheriaBossCharacter>();
			const float Dist2D = FVector::Dist2D(Origin, Enemy->GetActorLocation());
			if (bIsBoss)
			{
				if (Dist2D <= 15000.f)
				{
					FVector2f Pos;
					if (WorldToRadarPos(Enemy->GetActorLocation(), true, Pos))
					{
						DrawDotBlip(Pos, 6.5f + Pulse * 2.0f, FLinearColor(0.95f, 0.14f, 0.16f, 0.34f), BlipLayer);
						DrawDotBlip(Pos, 4.8f, FLinearColor(0.98f, 0.18f, 0.20f, 1.0f), BlipLayer + 1);
					}
				}
			}
			else if (Dist2D <= RadarRangeCm)
			{
				FVector2f Pos;
				if (WorldToRadarPos(Enemy->GetActorLocation(), false, Pos))
				{
					DrawDotBlip(Pos, 3.4f, ChroniclesHUDPalette::TextCrimson, BlipLayer);
				}
			}
		}

		// 3B. Villager Quest NPCs (Bright Emerald / Burnished Gold Dots)
		for (TActorIterator<AAetheriaNPCActor> It(World); It; ++It)
		{
			const AAetheriaNPCActor* NPC = *It;
			if (!IsValid(NPC) || NPC->IsHidden())
			{
				continue;
			}
			FVector2f Pos;
			if (WorldToRadarPos(NPC->GetActorLocation(), false, Pos))
			{
				const FQuestDef* Q = ChroniclesHUDPalette::ResolveQuestForNPC(QuestSub, NPC);
				const bool bHasImportantQuest = Q && (Q->State == EQuestState::Available || Q->State == EQuestState::ReadyToTurnIn);
				const FLinearColor DotCol = bHasImportantQuest
					? ChroniclesHUDPalette::TextGold
					: ChroniclesHUDPalette::TextEmerald;
				DrawDotBlip(Pos, bHasImportantQuest ? 4.2f : 3.5f, DotCol, BlipLayer + 1);
			}
		}

		// 3C. Dragon Ignis (Fiery Orange Drake Diamond)
		for (TActorIterator<AAetheriaDragonActor> It(World); It; ++It)
		{
			const AAetheriaDragonActor* Drag = *It;
			if (!IsValid(Drag) || Drag->IsHidden())
			{
				continue;
			}
			const bool bClampDragon = (Hero && Hero->bDragonUnlocked);
			const float Dist2D = FVector::Dist2D(Origin, Drag->GetActorLocation());
			if (bClampDragon || Dist2D <= 6500.f)
			{
				FVector2f Pos;
				if (WorldToRadarPos(Drag->GetActorLocation(), bClampDragon, Pos))
				{
					DrawDiamondBlip(Pos, 5.2f, FLinearColor(1.0f, 0.45f, 0.10f, 0.98f), FLinearColor(1.0f, 0.80f, 0.35f, 1.0f), BlipLayer + 1);
				}
			}
		}

		// 3D. Tracked Quest / Prologue Objective (Glowing Gold/Amber Vector Diamond, Clamped to Rim!)
		FString WaypointLabel;
		const FVector ObjectiveLoc = PC->GetActiveWaypointWorldLocation(WaypointLabel);
		if (!ObjectiveLoc.IsNearlyZero())
		{
			FVector2f Pos;
			if (WorldToRadarPos(ObjectiveLoc, true, Pos))
			{
				DrawDiamondBlip(
					Pos,
					5.5f + Pulse * 1.2f,
					FLinearColor(1.0f, 0.80f, 0.28f, 0.96f),
					FLinearColor(1.0f, 0.95f, 0.72f, 1.0f),
					BlipLayer + 2);
			}
		}

		// 3E. Center Hero Directional Gold Chevron (rotates smoothly with camera/facing Yaw)
		const float YawRad = FMath::DegreesToRadians(PC->GetControlRotation().Yaw);
		const FVector2f Fwd(FMath::Cos(YawRad), FMath::Sin(YawRad));
		const FVector2f Right(-Fwd.Y, Fwd.X);
		const FVector2f Tip = Center + Fwd * 9.5f;
		const FVector2f LeftWing = Center - Fwd * 6.0f - Right * 6.2f;
		const FVector2f Notch = Center - Fwd * 2.2f;
		const FVector2f RightWing = Center - Fwd * 6.0f + Right * 6.2f;

		FSlateDrawElement::MakeLines(
			OutDrawElements, BlipLayer + 3, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{ LeftWing, Tip, RightWing, Notch, LeftWing },
			ESlateDrawEffect::None, FLinearColor(0.02f, 0.015f, 0.025f, 0.92f), true, 4.0f);
		FSlateDrawElement::MakeLines(
			OutDrawElements, BlipLayer + 4, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>{ LeftWing, Tip, RightWing, Notch, LeftWing },
			ESlateDrawEffect::None, ChroniclesHUDPalette::TextGold, true, 2.0f);
	}

	// 4. Pure Circular Sculpted Antique-Gold Double Compass Bezel & Radial Degree Ticks (100% round — zero square frame corners!)
	const int32 FrameLayer = BlipLayer + 5;
	{
		TArray<FVector2f> OuterBezelRing;
		TArray<FVector2f> InnerBezelRing;
		OuterBezelRing.Reserve(65);
		InnerBezelRing.Reserve(65);
		for (int32 i = 0; i <= 64; ++i)
		{
			const float Ang = (static_cast<float>(i) / 64.f) * UE_TWO_PI;
			const FVector2f Dir(FMath::Cos(Ang), FMath::Sin(Ang));
			OuterBezelRing.Add(Center + Dir * (DiscRadius + 5.0f));
			InnerBezelRing.Add(Center + Dir * DiscRadius);
		}
		FSlateDrawElement::MakeLines(
			OutDrawElements, FrameLayer, AllottedGeometry.ToPaintGeometry(),
			MoveTemp(OuterBezelRing), ESlateDrawEffect::None, ChroniclesHUDPalette::HairlineBronze, true, 1.6f);
		FSlateDrawElement::MakeLines(
			OutDrawElements, FrameLayer, AllottedGeometry.ToPaintGeometry(),
			MoveTemp(InnerBezelRing), ESlateDrawEffect::None, ChroniclesHUDPalette::HairlineGold, true, 2.0f);

		// 24 subtle brass compass degree tick marks inside the circular bezel
		for (int32 TickIdx = 0; TickIdx < 24; ++TickIdx)
		{
			if ((TickIdx % 6) == 0) { continue; } // Cardinal slots reserved for N/E/S/W pips
			const float Ang = (static_cast<float>(TickIdx) / 24.f) * UE_TWO_PI;
			const FVector2f Dir(FMath::Cos(Ang), FMath::Sin(Ang));
			FSlateDrawElement::MakeLines(
				OutDrawElements, FrameLayer, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2f>{ Center + Dir * (DiscRadius + 0.8f), Center + Dir * (DiscRadius + 4.2f) },
				ESlateDrawEffect::None, FLinearColor(0.76f, 0.59f, 0.28f, 0.55f), true, 1.1f);
		}

		// East, South, West antique gold diamond pips
		DrawDiamondBlip(Center + FVector2f(DiscRadius + 2.5f, 0.f), 3.2f, ChroniclesHUDPalette::HairlineGold, ChroniclesHUDPalette::TextGold, FrameLayer);
		DrawDiamondBlip(Center + FVector2f(0.f, DiscRadius + 2.5f), 3.2f, ChroniclesHUDPalette::HairlineGold, ChroniclesHUDPalette::TextGold, FrameLayer);
		DrawDiamondBlip(Center + FVector2f(-DiscRadius - 2.5f, 0.f), 3.2f, ChroniclesHUDPalette::HairlineGold, ChroniclesHUDPalette::TextGold, FrameLayer);
	}

	// 5. Crisp Cardinal 'N' Burnished Gold Badge at Top of Rim
	const int32 NorthLayer = FrameLayer + 1;
	const FVector2f NorthPos(Center.X, Center.Y - DiscRadius + 1.f);
	DrawDotBlip(NorthPos, 9.5f, FLinearColor(0.035f, 0.030f, 0.045f, 0.95f), NorthLayer);
	FSlateDrawElement::MakeText(
		OutDrawElements, NorthLayer + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(16.f, 14.f), FSlateLayoutTransform(NorthPos - FVector2f(4.5f, 6.5f))),
		TEXT("N"), ChroniclesHUDPalette::FontBold(8), ESlateDrawEffect::None, ChroniclesHUDPalette::TextGold);

	return NorthLayer + 2;
}

// ============================================================================
// 4C. SChroniclesWorldSpaceOverlay — Overhead Villager Names, Enemy HP Bars,
//     Floating Golden Objective Diamond & Minimalist Center Dot Reticle
// ============================================================================

void SChroniclesWorldSpaceOverlay::Construct(const FArguments& InArgs)
{
	OwnerPC = InArgs._OwnerPC;
}

FVector2D SChroniclesWorldSpaceOverlay::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D::ZeroVector;
}

int32 SChroniclesWorldSpaceOverlay::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const AChroniclesPlayerController* PC = OwnerPC.Get();
	if (!PC || PC->ActiveOverlay != EChroniclesOverlayMode::None || PC->IsHeroDead() || PC->IsPrologueCutsceneActive())
	{
		return LayerId;
	}

	UWorld* World = PC->GetWorld();
	const APawn* ActivePawn = PC->GetPawn();
	if (!World || !ActivePawn)
	{
		return LayerId;
	}

	int32 ViewX = 0;
	int32 ViewY = 0;
	PC->GetViewportSize(ViewX, ViewY);
	const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
	if (ViewX <= 0 || ViewY <= 0 || LocalSize.X <= 1.f || LocalSize.Y <= 1.f)
	{
		return LayerId;
	}

	const float PixelToLocalX = LocalSize.X / static_cast<float>(ViewX);
	const float PixelToLocalY = LocalSize.Y / static_cast<float>(ViewY);

	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector CamFwd = CamRot.Vector();
	const FVector HeroLoc = ActivePawn->GetActorLocation();

	const FSlateBrush* WhiteBrush = ChroniclesHUDPalette::WhiteBrush();
	const FSlateBrush* PillBrush = ChroniclesHUDPalette::WorldPillBrush();
	const FSlateFontInfo NameFont = ChroniclesHUDPalette::FontBold(10);
	const FSlateFontInfo EnemyFont = ChroniclesHUDPalette::FontBold(9);
	const FSlateFontInfo DistFont = ChroniclesHUDPalette::FontBold(9);

	TSharedPtr<FSlateFontMeasure> FontMeasure;
	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
	{
		FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	}

	auto ProjectToLocal = [&](const FVector& WorldPos, float MarginPx, FVector2f& OutLocal) -> bool
	{
		const FVector ToTarget = WorldPos - CamLoc;
		if (FVector::DotProduct(ToTarget, CamFwd) <= 25.f)
		{
			return false;
		}
		FVector2D ScreenPx = FVector2D::ZeroVector;
		if (!PC->ProjectWorldLocationToScreen(WorldPos, ScreenPx, true))
		{
			return false;
		}
		const float LX = static_cast<float>(ScreenPx.X) * PixelToLocalX;
		const float LY = static_cast<float>(ScreenPx.Y) * PixelToLocalY;
		if (LX < MarginPx || LX > LocalSize.X - MarginPx || LY < MarginPx || LY > LocalSize.Y - MarginPx)
		{
			return false;
		}
		OutLocal = FVector2f(LX, LY);
		return true;
	};

	const UGameInstance* GI = PC->GetGameInstance();
	const UQuestSubsystem* QuestSub = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;

	// 1. Minimalist Crimson-Desert Center Dot Reticle (3x3 warm ivory/gold dot, Alpha 0.55)
	if (PC->bMouseCaptured)
	{
		const FVector2f Mid = LocalSize * 0.5f;
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(5.f, 5.f), FSlateLayoutTransform(Mid - FVector2f(2.5f, 2.5f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.02f, 0.015f, 0.025f, 0.42f));
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(3.f, 3.f), FSlateLayoutTransform(Mid - FVector2f(1.5f, 1.5f))),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.98f, 0.92f, 0.76f, 0.55f));
	}

	const int32 NPCLayer = LayerId + 2;

	// 2. Overhead Villager & Quest NPC Nameplates (within 2400 cm, same elevation band, and unobstructed line-of-sight)
	for (TActorIterator<AAetheriaNPCActor> It(World); It; ++It)
	{
		const AAetheriaNPCActor* NPC = *It;
		if (!IsValid(NPC) || NPC->IsHidden())
		{
			continue;
		}
		const FVector NPCLoc = NPC->GetActorLocation();
		const float Dist = FVector::Dist(HeroLoc, NPCLoc);
		if (Dist > 2400.f || FMath::Abs(HeroLoc.Z - NPCLoc.Z) > 520.f)
		{
			continue;
		}
		{
			FCollisionQueryParams LOSParams(SCENE_QUERY_STAT(NPCNameplateLOS), false, ActivePawn);
			LOSParams.AddIgnoredActor(NPC);
			if (const AAetheriaDragonActor* DragPawn = Cast<AAetheriaDragonActor>(ActivePawn))
			{
				if (DragPawn->GetRiderHero()) { LOSParams.AddIgnoredActor(DragPawn->GetRiderHero()); }
			}
			FHitResult LOSHit;
			if (World->LineTraceSingleByChannel(LOSHit, CamLoc, NPCLoc + FVector(0.f, 0.f, 85.f), ECC_Visibility, LOSParams))
			{
				continue;
			}
		}

		FVector2f ScreenPos;
		if (!ProjectToLocal(NPCLoc + FVector(0.f, 0.f, 115.f), 28.f, ScreenPos))
		{
			continue;
		}

		const FQuestDef* Q = ChroniclesHUDPalette::ResolveQuestForNPC(QuestSub, NPC);
		FString LabelStr = NPC->DisplayName.ToString();
		FLinearColor TextCol = ChroniclesHUDPalette::TextPrimary;
		if (Q && Q->State == EQuestState::Available)
		{
			LabelStr = FString::Printf(TEXT("[!] %s"), *LabelStr);
			TextCol = ChroniclesHUDPalette::TextGold;
		}
		else if (Q && Q->State == EQuestState::ReadyToTurnIn)
		{
			LabelStr = FString::Printf(TEXT("[?] %s"), *LabelStr);
			TextCol = ChroniclesHUDPalette::TextEmerald;
		}

		const FVector2D Measured = FontMeasure.IsValid()
			? FontMeasure->Measure(LabelStr, NameFont)
			: FVector2D(static_cast<double>(LabelStr.Len()) * 7.5, 16.0);
		const float TextW = static_cast<float>(Measured.X);
		const float TextH = static_cast<float>(Measured.Y);
		const float PillW = TextW + 16.f;
		const float PillH = TextH + 6.f;
		const FVector2f PillPos(ScreenPos.X - PillW * 0.5f, ScreenPos.Y - PillH);

		FSlateDrawElement::MakeBox(
			OutDrawElements, NPCLayer,
			AllottedGeometry.ToPaintGeometry(FVector2f(PillW, PillH), FSlateLayoutTransform(PillPos)),
			PillBrush, ESlateDrawEffect::None, FLinearColor(0.028f, 0.024f, 0.038f, 0.88f));
		FSlateDrawElement::MakeText(
			OutDrawElements, NPCLayer + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(TextW + 4.f, TextH + 2.f), FSlateLayoutTransform(PillPos + FVector2f(8.f, 3.f))),
			LabelStr, NameFont, ESlateDrawEffect::None, TextCol);
	}

	const int32 EnemyLayer = NPCLayer + 2;

	// 3. Overhead Enemy Health Bars (84x7 recessed obsidian track + crimson fill) & Names (within 2200 cm, non-boss)
	for (TActorIterator<AAetheriaEnemyCharacter> It(World); It; ++It)
	{
		const AAetheriaEnemyCharacter* Enemy = *It;
		if (!IsValid(Enemy) || Enemy->bDead || Enemy->IsHidden() || Enemy->IsA<AAetheriaBossCharacter>())
		{
			continue;
		}
		const float Dist = FVector::Dist(HeroLoc, Enemy->GetActorLocation());
		if (Dist > 2200.f)
		{
			continue;
		}

		const float HalfH = Enemy->GetCapsuleComponent() ? Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
		FVector2f ScreenPos;
		if (!ProjectToLocal(Enemy->GetActorLocation() + FVector(0.f, 0.f, HalfH + 32.f), 32.f, ScreenPos))
		{
			continue;
		}

		const float BarW = 84.f;
		const float BarH = 7.f;
		const float HPFrac = (Enemy->MaxHP > 0.f) ? FMath::Clamp(Enemy->HP / Enemy->MaxHP, 0.f, 1.f) : 0.f;
		const FVector2f BarPos(ScreenPos.X - BarW * 0.5f, ScreenPos.Y - BarH);

		// Name centered right above HP bar
		const FString EnemyName = Enemy->EnemyDisplayName.IsEmpty() ? TEXT("Foe") : Enemy->EnemyDisplayName.ToString();
		const FVector2D NameSize = FontMeasure.IsValid()
			? FontMeasure->Measure(EnemyName, EnemyFont)
			: FVector2D(static_cast<double>(EnemyName.Len()) * 6.5, 14.0);
		FSlateDrawElement::MakeText(
			OutDrawElements, EnemyLayer + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(static_cast<float>(NameSize.X) + 4.f, static_cast<float>(NameSize.Y) + 2.f),
				FSlateLayoutTransform(FVector2f(ScreenPos.X - static_cast<float>(NameSize.X) * 0.5f, BarPos.Y - static_cast<float>(NameSize.Y) - 3.f))),
			EnemyName, EnemyFont, ESlateDrawEffect::None, ChroniclesHUDPalette::TextPrimary);

		// Outer antique-bronze border + recessed obsidian track
		FSlateDrawElement::MakeBox(
			OutDrawElements, EnemyLayer,
			AllottedGeometry.ToPaintGeometry(FVector2f(BarW + 2.f, BarH + 2.f), FSlateLayoutTransform(BarPos - FVector2f(1.f, 1.f))),
			WhiteBrush, ESlateDrawEffect::None, ChroniclesHUDPalette::HairlineBronze);
		FSlateDrawElement::MakeBox(
			OutDrawElements, EnemyLayer + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(BarW, BarH), FSlateLayoutTransform(BarPos)),
			WhiteBrush, ESlateDrawEffect::None, ChroniclesHUDPalette::TrackRecessed);

		// Deep Crimson HP Fill + subtle highlight rim
		if (HPFrac > 0.005f)
		{
			const float FillW = FMath::Max(2.f, BarW * HPFrac);
			FSlateDrawElement::MakeBox(
				OutDrawElements, EnemyLayer + 2,
				AllottedGeometry.ToPaintGeometry(FVector2f(FillW, BarH), FSlateLayoutTransform(BarPos)),
				WhiteBrush, ESlateDrawEffect::None, ChroniclesHUDPalette::BarHP);
			FSlateDrawElement::MakeBox(
				OutDrawElements, EnemyLayer + 2,
				AllottedGeometry.ToPaintGeometry(FVector2f(FillW, 1.5f), FSlateLayoutTransform(BarPos)),
				WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.96f, 0.40f, 0.36f, 0.45f));
		}
	}

	// 4. Floating Golden Objective Waypoint Diamond (vector-drawn hollow diamond + distance in meters)
	const int32 WaypointLayer = EnemyLayer + 3;
	FString WaypointLabel;
	const FVector ObjectiveLoc = PC->GetActiveWaypointWorldLocation(WaypointLabel);
	if (!ObjectiveLoc.IsNearlyZero())
	{
		const float DistCm = FVector::Dist(HeroLoc, ObjectiveLoc);
		if (DistCm > 260.f)
		{
			FVector2f WayPos;
			if (ProjectToLocal(ObjectiveLoc + FVector(0.f, 0.f, 65.f), 44.f, WayPos))
			{
				const float Pulse = 0.5f + 0.5f * FMath::Sin(World->GetTimeSeconds() * 3.6f);
				const float OuterR = 9.0f + Pulse * 1.4f;
				const float InnerR = 4.2f;

				TArray<FVector2f> OuterDiamond{
					WayPos + FVector2f(0.f, -OuterR),
					WayPos + FVector2f(OuterR, 0.f),
					WayPos + FVector2f(0.f, OuterR),
					WayPos + FVector2f(-OuterR, 0.f),
					WayPos + FVector2f(0.f, -OuterR)
				};
				FSlateDrawElement::MakeLines(
					OutDrawElements, WaypointLayer, AllottedGeometry.ToPaintGeometry(),
					OuterDiamond, ESlateDrawEffect::None, FLinearColor(0.02f, 0.015f, 0.025f, 0.78f), true, 3.6f);
				FSlateDrawElement::MakeLines(
					OutDrawElements, WaypointLayer + 1, AllottedGeometry.ToPaintGeometry(),
					MoveTemp(OuterDiamond), ESlateDrawEffect::None, ChroniclesHUDPalette::TextGold, true, 1.8f);

				TArray<FVector2f> InnerDiamond{
					WayPos + FVector2f(0.f, -InnerR),
					WayPos + FVector2f(InnerR, 0.f),
					WayPos + FVector2f(0.f, InnerR),
					WayPos + FVector2f(-InnerR, 0.f),
					WayPos + FVector2f(0.f, -InnerR)
				};
				FSlateDrawElement::MakeLines(
					OutDrawElements, WaypointLayer + 2, AllottedGeometry.ToPaintGeometry(),
					MoveTemp(InnerDiamond), ESlateDrawEffect::None, FLinearColor(1.0f, 0.96f, 0.76f, 0.95f), true, 1.4f);

				const int32 DistMeters = FMath::Max(1, FMath::RoundToInt(DistCm / 100.f));
				const FString DistStr = (PC->PrologueStage < 3 && !WaypointLabel.IsEmpty())
					? FString::Printf(TEXT("%s · %dm"), *WaypointLabel, DistMeters)
					: FString::Printf(TEXT("%dm"), DistMeters);
				const FVector2D DistSize = FontMeasure.IsValid()
					? FontMeasure->Measure(DistStr, DistFont)
					: FVector2D(static_cast<double>(DistStr.Len()) * 6.5, 14.0);
				FSlateDrawElement::MakeText(
					OutDrawElements, WaypointLayer + 2,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(static_cast<float>(DistSize.X) + 4.f, static_cast<float>(DistSize.Y) + 2.f),
						FSlateLayoutTransform(FVector2f(WayPos.X - static_cast<float>(DistSize.X) * 0.5f, WayPos.Y + OuterR + 3.f))),
					DistStr, DistFont, ESlateDrawEffect::None, ChroniclesHUDPalette::TextGold);
			}
		}
	}

	return WaypointLayer + 3;
}

// ============================================================================
// 4D. SAetheriaChroniclesHUD — Production Slate HUD & Interactive Overlays
// ============================================================================

void SAetheriaChroniclesHUD::Construct(const FArguments& InArgs)
{
	OwnerPC = InArgs._OwnerPC;

	ChildSlot
	[
		SNew(SOverlay)

		// --- Layer 0: World-Space Overhead NPC Names, Enemy HP Bars, Waypoint Diamond & Center Reticle ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SChroniclesWorldSpaceOverlay)
			.OwnerPC(OwnerPC)
			.Visibility(EVisibility::HitTestInvisible)
		]

		// --- Layer 1: Top-Center Compass & Contextual Boss Health/Stagger Bar ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 16.f, 0.f, 0.f))
		[
			BuildTopCenterCompassAndBossBar()
		]

		// --- Layer 2: Top-Right Gold / Level / Exploration / RTX Tier & Hotkeys ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 16.f, 18.f, 0.f))
		[
			BuildTopRightStatusAndHotkeys()
		]

		// --- Layer 3: Bottom-Left Vitals, 3-Orb Queue, D/F Spells & Dragon Bar ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(20.f, 0.f, 0.f, 20.f))
		[
			BuildBottomLeftVitalsAndInvokerPanel()
		]

		// --- Layer 4: Bottom-Right Round Minimap Radar, Objective Distance & Burnished Gold Corner ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 24.f, 24.f))
		[
			BuildBottomRightMinimapAndGold()
		]

		// --- Layer 5: Bottom-Center Interaction Prompt & Toast Notifications ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 28.f))
		[
			BuildBottomCenterPromptsAndToast()
		]

		// --- Layer 6: Cinematic AAA Launch / Title & Pause Menu Overlay ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildMainMenuOverlay()
		]

		// --- Layer 5: Fullscreen Fog-of-War World Map [M] ---
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

		// --- Layer 12: Cinematic Cutscene Letterbox Bars (Prologue Waterfall Plunge) ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildCutsceneLetterboxOverlay()
		]

		// --- Layer 13: In-Game Arcane Developer Console (` / ~ / ё -> /godmode) ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			BuildDeveloperConsoleOverlay()
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
	auto BuildSkullEmblem = []() -> TSharedRef<SWidget>
	{
		if (const FSlateBrush* Skull = ChroniclesHUDPalette::SkullCrestBrush())
		{
			return SNew(SBox)
				.WidthOverride(28.f)
				.HeightOverride(28.f)
				[
					SNew(SImage)
					.Image(Skull)
					.ColorAndOpacity(FLinearColor(0.98f, 0.88f, 0.72f, 0.96f))
				];
		}
		return SNew(SBox)
			.WidthOverride(10.f)
			.HeightOverride(10.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineCrimson)
			];
	};

	return SNew(SVerticalBox)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && (PC->ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer || PC->ActiveOverlay == EChroniclesOverlayMode::MainMenu || PC->IsPrologueCutsceneActive()))
				? EVisibility::Collapsed
				: EVisibility::SelfHitTestInvisible;
		})

		// Sleek Translucent Dark Obsidian Compass & Tracked Quest Ribbon with 1px Antique Gold Top/Bottom Hairlines
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(1.f)
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGoldSubtle)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::RibbonBg)
				.Padding(FMargin(24.f, 7.f))
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
						.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
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
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(1.f)
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGoldSubtle)
				]
			]
		]

		// Monumental Dark-Fantasy Boss Frame (Malakor · Skull Crests, Blood-Crimson HP, Molten Amber Poise & Telegraph)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(FMargin(0.f, 12.f, 0.f, 0.f))
		[
			SNew(SBox)
			.WidthOverride(670.f)
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
							? FSlateColor(FLinearColor(1.00f, 0.702f, 0.278f, 0.96f))
							: FSlateColor(FLinearColor(1.00f, 0.165f, 0.165f, 0.96f));
					}
					return FSlateColor(ChroniclesHUDPalette::HairlineGold);
				})
				.Padding(FMargin(2.f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
					.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.96f))
					.Padding(FMargin(18.f, 12.f))
					[
						SNew(SVerticalBox)

						// Title & Phase Header flanked by Sculpted Skull Crests
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(FMargin(0.f, 0.f, 10.f, 0.f))
							[
								BuildSkullEmblem()
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(14))
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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
							.Padding(FMargin(8.f, 0.f, 10.f, 0.f))
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
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								BuildSkullEmblem()
							]
						]

						// Blood-Crimson Boss HP Bar
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 7.f, 0.f, 5.f))
						[
							SNew(SBox)
							.HeightOverride(15.f)
							[
								SNew(SProgressBar)
								.Style(&ChroniclesHUDPalette::BossHPBarStyle())
								.FillColorAndOpacity(FLinearColor::White)
								.Percent_Lambda([WeakPC = OwnerPC]() -> TOptional<float>
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
									return (Boss && Boss->MaxHP > 0.f) ? FMath::Clamp(Boss->HP / Boss->MaxHP, 0.f, 1.f) : 0.f;
								})
							]
						]

						// Molten Amber Boss Poise / Stagger Bar
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
								.ColorAndOpacity(ChroniclesHUDPalette::TextAmber)
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
								.HeightOverride(8.f)
								[
									SNew(SProgressBar)
									.Style(&ChroniclesHUDPalette::RecessedBarBaseStyle())
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

						// High-Contrast Glowing AMBER (PARRY RMB) vs CRIMSON (DODGE Q/Ctrl) Warning Banner
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(FMargin(0.f, 7.f, 0.f, 0.f))
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
								return (Boss && Boss->IsCurrentAttackParryable())
									? FSlateColor(FLinearColor(1.00f, 0.85f, 0.42f, 0.98f))
									: FSlateColor(FLinearColor(1.00f, 0.36f, 0.32f, 0.98f));
							})
							.Padding(FMargin(1.5f))
							.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
								return (Boss && Boss->bIsTelegraphing) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
									const float T = (PC && PC->GetWorld()) ? PC->GetWorld()->GetTimeSeconds() : 0.f;
									const float Pulse = 0.72f + 0.28f * FMath::Abs(FMath::Sin(T * 10.f));
									if (Boss && Boss->IsCurrentAttackParryable())
									{
										// Glowing Amber #FFB347
										return FSlateColor(FLinearColor(0.95f * Pulse, 0.62f * Pulse, 0.16f * Pulse, 0.96f));
									}
									// Glowing Blood-Crimson #FF2A2A
									return FSlateColor(FLinearColor(0.86f * Pulse, 0.12f * Pulse, 0.14f * Pulse, 0.96f));
								})
								.Padding(FMargin(16.f, 4.f))
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(11))
									.ColorAndOpacity(FLinearColor(0.99f, 0.96f, 0.88f, 1.0f))
									.Text_Lambda([WeakPC = OwnerPC]() -> FText
									{
										AChroniclesPlayerController* PC = WeakPC.Get();
										const AAetheriaBossCharacter* Boss = PC ? PC->FindWorldBoss() : nullptr;
										if (!Boss) return FText::GetEmpty();
										const FString AttackName = GetBossAttackWarningText(Boss->CurrentAttack);
										if (Boss->IsCurrentAttackParryable())
										{
											return FText::FromString(FString::Printf(
												TEXT("AMBER · PARRY [RMB]  —  %s (+2.5x STAGGER)"),
												*AttackName));
										}
										return FText::FromString(FString::Printf(
											TEXT("CRIMSON · DODGE [Q / Ctrl]  —  %s (UNBLOCKABLE)"),
											*AttackName));
									})
								]
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
		.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
		.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.92f))
		.Padding(FMargin(16.f, 11.f))
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && (PC->ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer || PC->ActiveOverlay == EChroniclesOverlayMode::MainMenu || PC->IsPrologueCutsceneActive()))
				? EVisibility::Collapsed
				: EVisibility::SelfHitTestInvisible;
		})
		[
			SNew(SVerticalBox)

			// Antique Gold Header: LV · GOLD · SP · MAP% · CO-OP
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(11))
				.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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
						TEXT("LV %d  ·  %d G  ·  SP %d  ·  MAP %.1f%%%s"),
						Lv, Gold, Pts, Explored,
						(Players > 1) ? *FString::Printf(TEXT("  ·  CO-OP %d"), Players) : TEXT("")));
				})
			]

			// Active Race + Stature Badge
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 4.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontRegular(9))
				.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
					const UHeroCustomizationComponent* Custom = Hero ? Hero->Customization.Get() : nullptr;
					if (!Custom)
					{
						return FSlateColor(ChroniclesHUDPalette::TextPrimary);
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
					return FSlateColor(ChroniclesHUDPalette::TextPrimary);
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
						? TEXT(" · NO MAGIC")
						: (H >= UHeroCustomizationComponent::GIANT_TITAN_MIN ? TEXT(" · GIANT") : TEXT(""));
					return FText::FromString(FString::Printf(TEXT("%s · %.2fm%s"),
						*UHeroCustomizationComponent::GetRaceLabel(Custom->Profile.Race), H, Tag));
				})
			]

			// Live RTX HWRT / NVIDIA DLSS 4.5 Status Line (Cyan / Emerald)
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
					if (RTXSub && RTXSub->IsRTXActive())
					{
						return RTXSub->IsDLSSActive()
							? FSlateColor(ChroniclesHUDPalette::TextEmerald)
							: FSlateColor(ChroniclesHUDPalette::TextCyan);
					}
					return FSlateColor(ChroniclesHUDPalette::TextGold);
				})
				.Text_Lambda([WeakPC = OwnerPC]() -> FText
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
					const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
					if (!RTXSub)
					{
						return LOCTEXT("RTXBadgeUnknown", "RENDERER · INITIALIZING");
					}
					const FGraphicsTelemetry& Tel = RTXSub->GetTelemetry();
					if (Tel.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
					{
						if (Tel.bDLSSActive)
						{
							const FString ModeLabel = RTXSub->GetDLSSModeLabel();
							const TCHAR* RRTag = RTXSub->IsDLSSRayReconstructionEnabled() ? TEXT("+RR") : TEXT("");
							const TCHAR* FGTag = RTXSub->IsDLSSFrameGenEnabled() ? TEXT("+FG") : TEXT("");
							return FText::FromString(FString::Printf(
								TEXT("RTX HWRT · DLSS 4.5 %s%s%s"),
								ModeLabel.IsEmpty() ? TEXT("Quality") : *ModeLabel,
								RRTag,
								FGTag));
						}
						return FText::FromString(FString::Printf(TEXT("RTX HWRT · TSR · %dMB"), Tel.TexturePoolMB));
					}
					return FText::FromString(FString::Printf(TEXT("Standard · %dMB"), Tel.TexturePoolMB));
				})
			]

			// Live God Mode Immortal Archmage Banner (visible when /godmode is active)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(FLinearColor(0.86f, 0.68f, 0.20f, 0.92f))
				.Padding(FMargin(1.f))
				.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
					return (Hero && Hero->IsGodModeActive()) ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
					.BorderBackgroundColor(FLinearColor::White)
					.Padding(FMargin(8.f, 3.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(9))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text(LOCTEXT("GodModeActiveBadge", "GOD MODE · БЕССМЕРТИЕ · 10/10 ЗАКЛИНАНИЙ"))
					]
				]
			]

			// Thin Antique Gold Divider + Compact Keybind Footer
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
			[
				SNew(SBox)
				.HeightOverride(1.f)
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::HairlineBronze)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontRegular(9))
				.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
				.Text(LOCTEXT("HotkeyHint", "[M] Map  [K] Skills  [C] Hero  [J] Quests  [Tab] Grimoire  [T] Time  [~] Console"))
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
			.WidthOverride(44.f)
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(9))
				.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
				.Text(LabelText)
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.Padding(FMargin(4.f, 0.f, 8.f, 0.f))
		[
			SNew(SBox)
			.HeightOverride(11.f)
			[
				SNew(SProgressBar)
				.Style(&ChroniclesHUDPalette::RecessedBarBaseStyle())
				.FillColorAndOpacity(FillColor)
				.Percent(PercentAttr)
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(78.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::ObsidianCardLockedBrush())
				.BorderBackgroundColor(FLinearColor::White)
				.Padding(FMargin(5.f, 1.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontRegular(9))
					.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
					.Justification(ETextJustify::Right)
					.Text(ValueTextAttr)
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildOrbBadgeWidget(EChronicleOrb Orb, const FString& KeyHint) const
{
	const FSlateBrush* IconBrush = ChroniclesHUDPalette::OrbIconBrush(Orb);

	TSharedRef<SHorizontalBox> InnerContent = SNew(SHorizontalBox);
	if (IconBrush)
	{
		InnerContent->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
		[
			SNew(SBox)
			.WidthOverride(22.f)
			.HeightOverride(22.f)
			[
				SNew(SImage)
				.Image(IconBrush)
				.ColorAndOpacity_Lambda([WeakPC = OwnerPC, Orb]() -> FSlateColor
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
					const bool bUnlocked = (Hero && Hero->SpellSystem && Hero->SpellSystem->IsOrbUnlocked(Orb));
					return bUnlocked
						? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
						: FSlateColor(FLinearColor(0.28f, 0.26f, 0.30f, 0.48f));
				})
			]
		];
	}

	InnerContent->AddSlot()
	.FillWidth(1.f)
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Font(ChroniclesHUDPalette::FontBold(9))
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
				: FString::Printf(TEXT("%s (Lock)"), *KeyHint));
		})
	];

	return SNew(SBorder)
		.BorderImage(ChroniclesHUDPalette::WhiteBrush())
		.BorderBackgroundColor_Lambda([WeakPC = OwnerPC, Orb]() -> FSlateColor
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
			const bool bUnlocked = (Hero && Hero->SpellSystem && Hero->SpellSystem->IsOrbUnlocked(Orb));
			if (bUnlocked)
			{
				const FLinearColor C = GetOrbColor(Orb);
				return FSlateColor(FLinearColor(C.R, C.G, C.B, 0.85f));
			}
			return FSlateColor(ChroniclesHUDPalette::OrbLocked);
		})
		.Padding(FMargin(1.5f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(FMargin(7.f, 4.f))
			[
				InnerContent
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildSpellSlotWidget(bool bIsSlotF) const
{
	const FString KeyBadge = bIsSlotF ? TEXT("[5/Z] SLOT F") : TEXT("[4/X] SLOT D");
	return SNew(SBox)
		.WidthOverride(180.f)
		[
			SNew(SOverlay)

			// Base carved dark obsidian backing so transparent socket center stays rich and readable
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
				.BorderBackgroundColor(FLinearColor::White)
			]

			// Sculpted T_UI_SkillSocket_Normal / Armed metallic frame overlay
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image_Lambda([WeakPC = OwnerPC, bIsSlotF]() -> const FSlateBrush*
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
					const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
					const EChronicleSpellId Id = Spells ? (bIsSlotF ? Spells->SlotF : Spells->SlotD) : EChronicleSpellId::None;
					return (Id != EChronicleSpellId::None)
						? ChroniclesHUDPalette::SocketArmedBrush()
						: ChroniclesHUDPalette::SocketNormalBrush();
				})
				.ColorAndOpacity_Lambda([WeakPC = OwnerPC, bIsSlotF]() -> FSlateColor
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
					const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
					const EChronicleSpellId Id = Spells ? (bIsSlotF ? Spells->SlotF : Spells->SlotD) : EChronicleSpellId::None;
					if (Id == EChronicleSpellId::None)
					{
						return FSlateColor(FLinearColor(0.75f, 0.72f, 0.68f, 0.85f));
					}
					const FLinearColor SpellCol = USpellSystemComponent::GetSpellDef(Id).PrimaryColor;
					return FSlateColor(FLinearColor(
						FMath::Lerp(1.f, SpellCol.R, 0.45f),
						FMath::Lerp(1.f, SpellCol.G, 0.45f),
						FMath::Lerp(1.f, SpellCol.B, 0.45f),
						0.98f));
				})
			]

			// Spell Slot Keybind & Invoked Name / Cooldown
			+ SOverlay::Slot()
			.Padding(FMargin(11.f, 8.f))
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
					.ColorAndOpacity_Lambda([WeakPC = OwnerPC, bIsSlotF]() -> FSlateColor
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
						const USpellSystemComponent* Spells = Hero ? Hero->SpellSystem.Get() : nullptr;
						const EChronicleSpellId Id = Spells ? (bIsSlotF ? Spells->SlotF : Spells->SlotD) : EChronicleSpellId::None;
						return (Id != EChronicleSpellId::None)
							? FSlateColor(ChroniclesHUDPalette::TextPrimary)
							: FSlateColor(ChroniclesHUDPalette::TextSecondary);
					})
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
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildBottomLeftVitalsAndInvokerPanel()
{
	return SNew(SVerticalBox)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && (PC->ActiveOverlay == EChroniclesOverlayMode::HeroCustomizer || PC->ActiveOverlay == EChroniclesOverlayMode::MainMenu || PC->IsPrologueCutsceneActive()))
				? EVisibility::Collapsed
				: EVisibility::SelfHitTestInvisible;
		})

		// Dragon Fire Breath Energy Bar (visible ONLY when riding Ignis!)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
		[
			SNew(SBox)
			.WidthOverride(396.f)
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
					.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
					.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.96f))
					.Padding(FMargin(14.f, 9.f))
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
						.Padding(FMargin(0.f, 5.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontRegular(9))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text(LOCTEXT("DragonFlightHint", "IGNIS: [WASD] Fly · [Space/Shift] Alt · [LMB] Flame"))
						]
					]
				]
			]
		]

		// Carved Obsidian Hero Vitals + Sculpted Invoker Orbs & Socket Cards Console
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.WidthOverride(396.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.94f))
				.Padding(FMargin(15.f, 13.f))
				[
					SNew(SVerticalBox)

					// HP (Deep Crimson #B81D24)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
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
								if (!Hero) return LOCTEXT("ZeroVal", "0 / 0");
								return Hero->IsGodModeActive()
									? FText::FromString(FString::Printf(TEXT("∞ %.0f"), Hero->MaxHP))
									: FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Hero->HP, Hero->MaxHP));
							}))
					]

					// Mana (Sapphire Arcana #1E6EE8)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
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
								if (!Spells) return LOCTEXT("ZeroVal", "0 / 0");
								return Spells->IsGodModeActive()
									? FText::FromString(FString::Printf(TEXT("∞ %.0f"), Spells->MaxMana))
									: FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Spells->Mana, Spells->MaxMana));
							}))
					]

					// Stamina (Emerald-Gold #39A86B)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
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

					// XP (Burnished Gold #D4A843)
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

					// Sculpted Elemental Orb Sockets (1:Exort / 2:Wex / 3:Quas)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 7.f))
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
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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

					// Invoked Spell Socket Cards ([4/X] D & [5/Z] F)
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
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildBottomRightMinimapAndGold()
{
	return SNew(SVerticalBox)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::None && !PC->IsHeroDead() && !PC->IsPrologueCutsceneActive())
				? EVisibility::SelfHitTestInvisible
				: EVisibility::Collapsed;
		})

		// 172x172 Round Minimap Radar Framed by hud_minimap_frame.png
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SChroniclesMinimapWidget)
			.OwnerPC(OwnerPC)
			.MinimapSize(172.f)
		]

		// Tracked Objective Distance Pill + Unmistakable Burnished Gold Corner Readout
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.92f))
			.Padding(FMargin(14.f, 7.f))
			[
				SNew(SVerticalBox)

				// Compact Tracked Objective Distance Readout (e.g. "◊ 84m · Oakhaven")
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontRegular(9))
					.ColorAndOpacity(ChroniclesHUDPalette::TextAmber)
					.Justification(ETextJustify::Center)
					.Text_Lambda([WeakPC = OwnerPC]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						const APawn* P = PC ? PC->GetPawn() : nullptr;
						if (!PC || !P) return FText::GetEmpty();

						FString Label;
						const FVector ObjLoc = PC->GetActiveWaypointWorldLocation(Label);
						if (ObjLoc.IsNearlyZero())
						{
							return LOCTEXT("MinimapRealmClear", "◊ Aetheria Free Roam");
						}
						const int32 DistM = FMath::Max(1, FMath::RoundToInt(FVector::Dist(P->GetActorLocation(), ObjLoc) / 100.f));
						return FText::FromString(FString::Printf(TEXT("◊ %dm"), DistM));
					})
				]

				// Thin Antique Gold Hairline
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
				[
					SNew(SBox)
					.HeightOverride(1.f)
					[
						SNew(SBorder)
						.BorderImage(ChroniclesHUDPalette::WhiteBrush())
						.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGoldSubtle)
					]
				]

				// Unmistakable Burnished Gold Corner Counter
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(11))
					.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
					.Justification(ETextJustify::Center)
					.Text_Lambda([WeakPC = OwnerPC]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
						const USkillTreeComponent* Skills = Hero ? Hero->SkillTree.Get() : nullptr;
						const int32 GoldVal = Skills ? Skills->Gold : 0;
						return FText::FromString(FString::Printf(TEXT("GOLD: %d G"), GoldVal));
					})
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildCutsceneLetterboxOverlay()
{
	return SNew(SOverlay)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->IsPrologueCutsceneActive())
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})

		// Top 64px Black Cinema Letterbox Bar + Subtle Gold Rim
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(64.f)
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(FLinearColor(0.01f, 0.008f, 0.014f, 0.98f))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(1.f)
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGoldSubtle)
				]
			]
		]

		// Bottom 64px Black Cinema Letterbox Bar + Subtle Gold Rim & Caption
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(1.f)
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGoldSubtle)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(64.f)
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(FLinearColor(0.01f, 0.008f, 0.014f, 0.98f))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(13))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Justification(ETextJustify::Center)
						.Text(LOCTEXT("WaterfallCutsceneCaption", "ВОДОПАД ЗЕРКАЛЬНОГО ОЗЕРА  ·  FALLS OF MIRROR LAKE"))
					]
				]
			]
		];
}

FReply SAetheriaChroniclesHUD::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	AChroniclesPlayerController* PC = OwnerPC.Get();
	if (!PC)
	{
		return SCompoundWidget::OnPreviewKeyDown(MyGeometry, InKeyEvent);
	}

	const FKey Key = InKeyEvent.GetKey();
	const uint32 CharCode = InKeyEvent.GetCharacter();
	const uint32 KeyCode = InKeyEvent.GetKeyCode();
	static const FKey KeyCyrillicYoLower(TEXT("ё"));
	static const FKey KeyCyrillicYoUpper(TEXT("Ё"));
	static const FKey KeyGraveAccent(TEXT("GraveAccent"));

	const bool bIsConsoleKey = (Key == EKeys::Tilde)
		|| (Key == KeyCyrillicYoLower)
		|| (Key == KeyCyrillicYoUpper)
		|| (Key == KeyGraveAccent)
		|| (KeyCode == 0xC0) // Physical VK_OEM_3 (` / ~ / ё) across all Windows keyboard layouts!
		|| (CharCode == TEXT('`'))
		|| (CharCode == TEXT('~'))
		|| (CharCode == 0x0451)
		|| (CharCode == 0x0401);

	if (bIsConsoleKey)
	{
		PC->ToggleDeveloperConsole();
		return FReply::Handled();
	}

	if (PC->IsDeveloperConsoleOpen())
	{
		if (Key == EKeys::Escape)
		{
			PC->SetDeveloperConsoleOpen(false);
			return FReply::Handled();
		}

		// Up/Down command history navigation inside the console input box
		if ((Key == EKeys::Up || Key == EKeys::Down) && ConsoleInputBox.IsValid())
		{
			const TArray<FString>& History = PC->GetConsoleCommandHistory();
			if (History.Num() == 0 && Key == EKeys::Up)
			{
				ConsoleInputBox->SetText(FText::FromString(TEXT("/godmode")));
				return FReply::Handled();
			}
			if (History.Num() > 0)
			{
				if (Key == EKeys::Up)
				{
					ConsoleHistoryBrowseIndex = (ConsoleHistoryBrowseIndex < 0)
						? (History.Num() - 1)
						: FMath::Max(0, ConsoleHistoryBrowseIndex - 1);
				}
				else
				{
					ConsoleHistoryBrowseIndex = (ConsoleHistoryBrowseIndex < 0)
						? (History.Num() - 1)
						: FMath::Min(History.Num() - 1, ConsoleHistoryBrowseIndex + 1);
				}
				if (History.IsValidIndex(ConsoleHistoryBrowseIndex))
				{
					ConsoleInputBox->SetText(FText::FromString(History[ConsoleHistoryBrowseIndex]));
				}
				return FReply::Handled();
			}
		}
	}

	return SCompoundWidget::OnPreviewKeyDown(MyGeometry, InKeyEvent);
}

void SAetheriaChroniclesHUD::RebuildConsoleLogBox()
{
	if (!ConsoleLogListBox.IsValid())
	{
		return;
	}

	ConsoleLogListBox->ClearChildren();
	AChroniclesPlayerController* PC = OwnerPC.Get();
	if (!PC)
	{
		return;
	}

	for (const TPair<FString, FLinearColor>& Entry : PC->GetConsoleLogLines())
	{
		ConsoleLogListBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(STextBlock)
			.Font(ChroniclesHUDPalette::FontRegular(10))
			.ColorAndOpacity(Entry.Value)
			.AutoWrapText(true)
			.Text(FText::FromString(Entry.Key))
		];
	}

	if (ConsoleScrollBox.IsValid())
	{
		ConsoleScrollBox->ScrollToEnd();
	}
}

void SAetheriaChroniclesHUD::SyncDeveloperConsoleVisibilityAndFocus(bool bOpen, bool bForceFocus)
{
	if (ConsoleOverlayContainer.IsValid())
	{
		ConsoleOverlayContainer->SetVisibility(bOpen ? EVisibility::Visible : EVisibility::Collapsed);
	}

	if (bOpen)
	{
		ConsoleHistoryBrowseIndex = -1;
		if (ConsoleInputBox.IsValid())
		{
			// Strip any stray opening ` / ~ / ё character that WM_CHAR might have queued
			FString CurStr = ConsoleInputBox->GetText().ToString();
			if (CurStr == TEXT("`") || CurStr == TEXT("~") || CurStr == TEXT("ё") || CurStr == TEXT("Ё"))
			{
				ConsoleInputBox->SetText(FText::GetEmpty());
			}
			else if (!bForceFocus)
			{
				ConsoleInputBox->SetText(FText::GetEmpty());
			}

			if (FSlateApplication::IsInitialized())
			{
				ConsoleOverlayContainer->SlatePrepass();
				FSlateApplication::Get().SetKeyboardFocus(ConsoleInputBox.ToSharedRef(), EFocusCause::SetDirectly);
				FSlateApplication::Get().SetUserFocus(0, ConsoleInputBox.ToSharedRef(), EFocusCause::SetDirectly);
			}
		}
	}
	else if (ConsoleInputBox.IsValid())
	{
		ConsoleInputBox->SetText(FText::GetEmpty());
	}
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildDeveloperConsoleOverlay()
{
	static const FEditableTextBoxStyle ConsoleBoxStyle = []()
	{
		FEditableTextBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
		if (const FSlateBrush* White = ChroniclesHUDPalette::WhiteBrush())
		{
			Style.SetBackgroundImageNormal(*White);
			Style.SetBackgroundImageHovered(*White);
			Style.SetBackgroundImageFocused(*White);
			Style.SetBackgroundImageReadOnly(*White);
		}
		Style.SetBackgroundColor(FLinearColor(0.016f, 0.014f, 0.022f, 0.98f));
		Style.SetForegroundColor(ChroniclesHUDPalette::TextPrimary);
		Style.SetFocusedForegroundColor(ChroniclesHUDPalette::TextGold);
		Style.SetPadding(FMargin(10.f, 7.f));
		Style.TextStyle.SetFont(ChroniclesHUDPalette::FontBold(11));
		Style.TextStyle.SetColorAndOpacity(ChroniclesHUDPalette::TextPrimary);
		return Style;
	}();

	SAssignNew(ConsoleLogListBox, SVerticalBox);
	SAssignNew(ConsoleScrollBox, SScrollBox)
	+ SScrollBox::Slot()
	[
		ConsoleLogListBox.ToSharedRef()
	];

	SAssignNew(ConsoleInputBox, SEditableTextBox)
		.Style(&ConsoleBoxStyle)
		.HintText(LOCTEXT("ConsoleInputHint", "Введите команду (например: /godmode или /help) и нажмите Enter..."))
		.ClearKeyboardFocusOnCommit(false)
		.SelectAllTextOnCommit(false)
		.OnTextChanged_Lambda([this, WeakPC = OwnerPC](const FText& NewText)
		{
			const FString Raw = NewText.ToString();
			FString Sanitized;
			Sanitized.Reserve(Raw.Len());
			const AChroniclesPlayerController* PC = WeakPC.Get();
			const bool bJustOpened = PC && ((FPlatformTime::Seconds() - PC->GetLastConsoleOpenRealTime()) < 0.20);

			for (int32 Idx = 0; Idx < Raw.Len(); ++Idx)
			{
				const TCHAR Ch = Raw[Idx];
				if (Ch == TEXT('`') || Ch == TEXT('~'))
				{
					continue;
				}
				if (bJustOpened && (Ch == 0x0451 || Ch == 0x0401))
				{
					continue;
				}
				Sanitized.AppendChar(Ch);
			}

			if (Sanitized != Raw && ConsoleInputBox.IsValid())
			{
				ConsoleInputBox->SetText(FText::FromString(Sanitized));
			}
		})
		.OnTextCommitted_Lambda([this, WeakPC = OwnerPC](const FText& CommittedText, ETextCommit::Type CommitType)
		{
			if (CommitType == ETextCommit::OnEnter)
			{
				if (AChroniclesPlayerController* PC = WeakPC.Get())
				{
					const FString Cmd = CommittedText.ToString();
					if (ConsoleInputBox.IsValid())
					{
						ConsoleInputBox->SetText(FText::GetEmpty());
					}
					ConsoleHistoryBrowseIndex = -1;
					if (!Cmd.TrimStartAndEnd().IsEmpty())
					{
						PC->ExecuteDeveloperConsoleCommand(Cmd);
					}
					if (PC->IsDeveloperConsoleOpen() && ConsoleInputBox.IsValid() && FSlateApplication::IsInitialized())
					{
						FSlateApplication::Get().SetKeyboardFocus(ConsoleInputBox.ToSharedRef(), EFocusCause::SetDirectly);
					}
				}
			}
		});

	SAssignNew(ConsoleOverlayContainer, SBox)
		.Visibility(EVisibility::Collapsed)
		.Padding(FMargin(28.f, 14.f, 28.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
			.Padding(FMargin(1.5f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.97f))
				.Padding(FMargin(18.f, 12.f))
				[
					SNew(SVerticalBox)

					// Console Header Bar
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(12))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text(LOCTEXT("DevConsoleTitle", "AETHERIA ARCANE CONSOLE  ·  [~ / Ё] КОНСОЛЬ РАЗРАБОТЧИКА"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(8.f, 0.f, 8.f, 0.f))
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor(FLinearColor(0.86f, 0.68f, 0.20f, 0.94f))
							.Padding(FMargin(1.f))
							.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const AAetheriaHeroCharacter* Hero = PC ? PC->GetActiveHero() : nullptr;
								return (Hero && Hero->IsGodModeActive()) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
								.BorderBackgroundColor(FLinearColor::White)
								.Padding(FMargin(8.f, 2.f))
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(9))
									.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
									.Text(LOCTEXT("ConsoleGodModePill", "GOD MODE ACTIVE · БЕССМЕРТИЕ · 10/10 SPELLS"))
								]
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(12.f, 0.f, 12.f, 0.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontRegular(9))
							.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
							.Text(LOCTEXT("DevConsoleHint", "[Enter] Выполнить  ·  [/godmode] Режим Бога  ·  [~ / Esc] Закрыть"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
							.ContentPadding(FMargin(10.f, 3.f))
							.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get())
								{
									PC->SetDeveloperConsoleOpen(false);
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(9))
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
								.Text(LOCTEXT("CloseConsoleBtn", "ЗАКРЫТЬ [~]"))
							]
						]
					]

					// Hairline Gold Divider
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
					[
						SNew(SBox)
						.HeightOverride(1.f)
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor(ChroniclesHUDPalette::HairlineBronze)
						]
					]

					// Scrollable Output Log
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
					[
						SNew(SBox)
						.HeightOverride(132.f)
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::ObsidianCardLockedBrush())
							.BorderBackgroundColor(FLinearColor::White)
							.Padding(FMargin(10.f, 6.f))
							[
								ConsoleScrollBox.ToSharedRef()
							]
						]
					]

					// Command Prompt Input Row
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(4.f, 0.f, 10.f, 0.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(13))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text(FText::FromString(TEXT(">")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGoldSubtle)
							.Padding(FMargin(1.f))
							[
								ConsoleInputBox.ToSharedRef()
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(8.f, 0.f, 0.f, 0.f))
						[
							SNew(SButton)
							.ButtonStyle(&ChroniclesHUDPalette::ActiveGoldButtonStyle())
							.ContentPadding(FMargin(14.f, 6.f))
							.OnClicked_Lambda([this, WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get())
								{
									if (ConsoleInputBox.IsValid())
									{
										const FString Cmd = ConsoleInputBox->GetText().ToString();
										ConsoleInputBox->SetText(FText::GetEmpty());
										if (!Cmd.TrimStartAndEnd().IsEmpty())
										{
											PC->ExecuteDeveloperConsoleCommand(Cmd);
										}
										else
										{
											PC->ExecuteDeveloperConsoleCommand(TEXT("/godmode"));
										}
										if (FSlateApplication::IsInitialized())
										{
											FSlateApplication::Get().SetKeyboardFocus(ConsoleInputBox.ToSharedRef(), EFocusCause::SetDirectly);
										}
									}
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(10))
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
								.Text(LOCTEXT("RunCmdBtn", "ENTER"))
							]
						]
					]
				]
			]
		];

	RebuildConsoleLogBox();
	return ConsoleOverlayContainer.ToSharedRef();
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildBottomCenterPromptsAndToast()
{
	return SNew(SVerticalBox)

		// Sculpted Obsidian & Antique Gold Toast Banner Notification
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
			.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				const float Alpha = PC ? PC->GetActiveToastAlpha() : 0.f;
				return FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.94f * Alpha));
			})
			.Padding(FMargin(24.f, 9.f))
			.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				return (PC && PC->ActiveOverlay != EChroniclesOverlayMode::MainMenu && !PC->IsPrologueCutsceneActive() && PC->GetActiveToastAlpha() > 0.01f)
					? EVisibility::HitTestInvisible
					: EVisibility::Collapsed;
			})
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(11))
				.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					const float Alpha = PC ? PC->GetActiveToastAlpha() : 0.f;
					return FSlateColor(FLinearColor(0.98f, 0.85f, 0.48f, Alpha));
				})
				.Text_Lambda([WeakPC = OwnerPC]() -> FText
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					return PC ? FText::FromString(PC->GetActiveToastMessage()) : FText::GetEmpty();
				})
			]
		]

		// Crisp Diegetic Interaction Prompt ([E] Поговорить: <NPC> / [F] Mount Ignis / Cursor Unlocked)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.95f))
			.Padding(FMargin(20.f, 9.f))
			.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				if (!PC || PC->IsAnyModalOpen() || PC->IsPrologueCutsceneActive())
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
				if (PC->FindTalkableNPC(280.f) != nullptr || PC->FindNearestDragon(950.f) != nullptr)
				{
					return EVisibility::HitTestInvisible;
				}
				return EVisibility::Collapsed;
			})
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontBold(11))
				.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
				.Text_Lambda([WeakPC = OwnerPC]() -> FText
				{
					AChroniclesPlayerController* PC = WeakPC.Get();
					if (!PC) return FText::GetEmpty();

					if (!PC->bMouseCaptured)
					{
						return LOCTEXT("CursorUnlockedPrompt", "Cursor Unlocked — Click Viewport or Press [Esc] to Resume Combat");
					}
					if (const AAetheriaNPCActor* NPC = PC->FindTalkableNPC(280.f))
					{
						return FText::FromString(FString::Printf(TEXT("[E] Поговорить: %s"),
							*NPC->DisplayName.ToString()));
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
		];
}

// ============================================================================
// Interactive Modal Overlays
// ============================================================================

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildMainMenuOverlay()
{
	auto HasSavedJourney = []() -> bool
	{
		return UGameplayStatics::DoesSaveGameExist(TEXT("AetheriaChroniclesSlot0"), 0)
			|| UGameplayStatics::DoesSaveGameExist(TEXT("AetheriaChroniclesSlot"), 0);
	};

	auto MakeMenuButton = [WeakPC = OwnerPC](
		TAttribute<FText> LabelAttr,
		bool bPrimaryCrimson,
		const FLinearColor& BezelColor,
		const FLinearColor& LabelColor,
		int32 FontSize,
		TFunction<void(AChroniclesPlayerController*)> OnClickFn) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(BezelColor)
			.Padding(FMargin(1.5f))
			[
				SNew(SButton)
				.ButtonStyle(bPrimaryCrimson ? &ChroniclesHUDPalette::CrimsonButtonStyle() : &ChroniclesHUDPalette::StoneButtonStyle())
				.ContentPadding(FMargin(18.f, 12.f))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked_Lambda([WeakPC, OnClickFn]() -> FReply
				{
					if (AChroniclesPlayerController* PC = WeakPC.Get())
					{
						OnClickFn(PC);
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(FontSize))
					.ColorAndOpacity(LabelColor)
					.Justification(ETextJustify::Center)
					.Text(MoveTemp(LabelAttr))
				]
			];
	};

	TSharedRef<SWidget> VignetteBackdrop = [&]() -> TSharedRef<SWidget>
	{
		if (const FSlateBrush* VignetteBrush = ChroniclesHUDPalette::MenuVignetteBrush())
		{
			return SNew(SImage)
				.Image(VignetteBrush)
				.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.78f))
				.Visibility(EVisibility::HitTestInvisible);
		}
		return SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.015f, 0.012f, 0.020f, 0.42f))
			.Visibility(EVisibility::HitTestInvisible);
	}();

	TSharedRef<SWidget> TopCrestWidget = [&]() -> TSharedRef<SWidget>
	{
		if (const FSlateBrush* Skull = ChroniclesHUDPalette::SkullCrestBrush())
		{
			return SNew(SBox)
				.WidthOverride(56.f)
				.HeightOverride(56.f)
				[
					SNew(SImage)
					.Image(Skull)
					.ColorAndOpacity(FLinearColor(0.98f, 0.88f, 0.68f, 0.96f))
				];
		}
		return SNew(SBox)
			.WidthOverride(28.f)
			.HeightOverride(28.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
			];
	}();

	return SNew(SOverlay)
		.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
		{
			AChroniclesPlayerController* PC = WeakPC.Get();
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::MainMenu)
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})

		// --- Layer 0: Atmospheric Dark-Fantasy Edge Vignette over Live 3D Oakhaven Sunset Vista ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			VignetteBackdrop
		]

		// --- Layer 1: Left-Aligned Sculpted Obsidian Title Column (480px) ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(64.f, 52.f, 0.f, 52.f))
		[
			SNew(SBox)
			.WidthOverride(480.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::WhiteBrush())
				.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
				.Padding(FMargin(1.5f))
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
					.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.94f))
					.Padding(FMargin(32.f, 26.f))
					[
						SNew(SVerticalBox)

						// Top Skull Crest Emblem
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
						[
							TopCrestWidget
						]

						// Monumental Burnished Gold Title
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(38))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Justification(ETextJustify::Center)
							.Text(LOCTEXT("MainMenuTitle", "AETHERIA"))
						]

						// Subtitle Lockup
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(FMargin(0.f, 2.f, 0.f, 14.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(14))
							.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
							.Justification(ETextJustify::Center)
							.Text(LOCTEXT("MainMenuSubtitle", "CHRONICLES OF THE DRAKE"))
						]

						// Gold 1px Hairline Divider
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(12.f, 0.f, 12.f, 10.f))
						[
							SNew(SBox)
							.HeightOverride(1.f)
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
							]
						]

						// Live Hardware & NVIDIA DLSS 4.5 Telemetry Strip
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontRegular(10))
							.Justification(ETextJustify::Center)
							.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
								const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
								if (RTXSub && RTXSub->IsRTXActive())
								{
									return FSlateColor(ChroniclesHUDPalette::TextCyan);
								}
								return FSlateColor(ChroniclesHUDPalette::TextSecondary);
							})
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
								const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
								if (!RTXSub)
								{
									return LOCTEXT("MenuHwFallback", "UE 5.8 · D3D12 SM6 · INITIALIZING");
								}
								const FGraphicsTelemetry& Tel = RTXSub->GetTelemetry();
								if (Tel.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
								{
									if (Tel.bDLSSActive)
									{
										const FString ModeLabel = RTXSub->GetDLSSModeLabel();
										const TCHAR* RRTag = RTXSub->IsDLSSRayReconstructionEnabled() ? TEXT("+RR") : TEXT("");
										const TCHAR* FGTag = RTXSub->IsDLSSFrameGenEnabled() ? TEXT("+FG") : TEXT("");
										return FText::FromString(FString::Printf(
											TEXT("UE 5.8 · RTX HWRT · NVIDIA DLSS 4.5 %s%s%s"),
											ModeLabel.IsEmpty() ? TEXT("Quality") : *ModeLabel,
											RRTag,
											FGTag));
									}
									return LOCTEXT("MenuHwRTXTSR", "UE 5.8 · RTX HWRT · LUMEN · TSR");
								}
								return LOCTEXT("MenuHwClassic", "UE 5.8 · D3D12 SM6 · CLASSIC MODE");
							})
						]

						// Upper Breathing Spacer
						+ SVerticalBox::Slot()
						.FillHeight(0.35f)
						[
							SNew(SSpacer)
						]

						// 1. Primary Forged Crimson CTA: New Journey (-> Hero Creator -> Cave Awakening) / Resume
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
						[
							MakeMenuButton(
								TAttribute<FText>::CreateLambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									if (PC && PC->bHasEnteredGameWorld)
									{
										return LOCTEXT("MainMenuResumeCTA", "ПРОДОЛЖИТЬ ИГРУ · RESUME");
									}
									return LOCTEXT("MainMenuStartCTA", "НАЧАТЬ ПРИКЛЮЧЕНИЕ · NEW JOURNEY");
								}),
								true,
								ChroniclesHUDPalette::HairlineGold,
								ChroniclesHUDPalette::TextPrimary,
								11,
								[](AChroniclesPlayerController* PC)
								{
									if (PC->bHasEnteredGameWorld)
									{
										PC->EnterGameFromTitleScreen(false);
									}
									else
									{
										PC->StartNewGameCharacterCreation();
									}
								})
						]

						// 2. Secondary Continue Saved Journey Button (visible only on initial launch if save exists)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
						[
							SNew(SBox)
							.Visibility_Lambda([WeakPC = OwnerPC, HasSavedJourney]() -> EVisibility
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								return (PC && !PC->bHasEnteredGameWorld && HasSavedJourney())
									? EVisibility::Visible
									: EVisibility::Collapsed;
							})
							[
								MakeMenuButton(
									LOCTEXT("MainMenuContinueCTA", "ЗАГРУЗИТЬ СОХРАНЕНИЕ · CONTINUE"),
									false,
									ChroniclesHUDPalette::HairlineGoldSubtle,
									ChroniclesHUDPalette::TextGold,
									10,
									[](AChroniclesPlayerController* PC)
									{
										PC->EnterGameFromTitleScreen(true);
									})
							]
						]

						// 3. Hero Creator Studio
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
						[
							MakeMenuButton(
								LOCTEXT("MainMenuCreatorCTA", "СОЗДАНИЕ ГЕРОЯ · HERO CREATOR"),
								false,
								ChroniclesHUDPalette::HairlineBronze,
								ChroniclesHUDPalette::TextPrimary,
								10,
								[](AChroniclesPlayerController* PC)
								{
									if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(PC))
									{
										Music->PlayMenuClickSFX();
									}
									PC->bMainMenuReturnTarget = !PC->bHasEnteredGameWorld;
									PC->ToggleOverlay(EChroniclesOverlayMode::HeroCustomizer);
								})
						]

						// 4. Graphics RTX / DLSS 4.5 & Audio
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
						[
							MakeMenuButton(
								LOCTEXT("MainMenuGraphicsCTA", "ГРАФИКА RTX / DLSS 4.5 И ЗВУК"),
								false,
								ChroniclesHUDPalette::HairlineBronze,
								ChroniclesHUDPalette::TextPrimary,
								10,
								[](AChroniclesPlayerController* PC)
								{
									if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(PC))
									{
										Music->PlayMenuClickSFX();
									}
									PC->bMainMenuReturnTarget = !PC->bHasEnteredGameWorld;
									PC->ToggleOverlay(EChroniclesOverlayMode::GraphicsRTX);
								})
						]

						// 5. Quit to Desktop
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeMenuButton(
								LOCTEXT("MainMenuQuitCTA", "ВЫХОД НА РАБОЧИЙ СТОЛ · QUIT"),
								false,
								ChroniclesHUDPalette::HairlineBronze,
								ChroniclesHUDPalette::TextSecondary,
								10,
								[](AChroniclesPlayerController* PC)
								{
									PC->RequestQuitApplication();
								})
						]

						// Lower Breathing Spacer
						+ SVerticalBox::Slot()
						.FillHeight(0.65f)
						[
							SNew(SSpacer)
						]

						// Gold Hairline Above Live Audio Strip
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 12.f, 0.f, 10.f))
						[
							SNew(SBox)
							.HeightOverride(1.f)
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGoldSubtle)
							]
						]

						// Live Soundtrack Track Readout
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontRegular(11))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.AutoWrapText(true)
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
								const FString TrackName = Music ? Music->GetCurrentTrackDisplayName() : TEXT("Aetheria Main Theme");
								return FText::FromString(FString::Printf(
									TEXT("OST · %s"),
									TrackName.IsEmpty() ? TEXT("Aetheria Main Theme") : *TrackName));
							})
						]

						// Inline Music Volume & Mute Controls
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor(ChroniclesHUDPalette::HairlineBronze)
								.Padding(FMargin(1.f))
								[
									SNew(SButton)
									.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
									.ContentPadding(FMargin(12.f, 5.f))
									.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
									{
										if (AChroniclesPlayerController* PC = WeakPC.Get())
										{
											if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(PC))
											{
												Music->SetMusicVolume(FMath::Clamp(Music->GetMusicVolume() - 0.10f, 0.f, 1.f));
												Music->PlayMenuClickSFX();
											}
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Font(ChroniclesHUDPalette::FontBold(11))
										.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
										.Text(LOCTEXT("MenuVolDown", "−"))
									]
								]
							]

							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FMargin(8.f, 0.f))
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(10))
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
									const int32 VolPct = Music ? FMath::RoundToInt(FMath::Clamp(Music->GetMusicVolume(), 0.f, 1.f) * 100.f) : 75;
									return FText::FromString(FString::Printf(TEXT("Музыка: %d%%"), VolPct));
								})
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor(ChroniclesHUDPalette::HairlineBronze)
								.Padding(FMargin(1.f))
								[
									SNew(SButton)
									.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
									.ContentPadding(FMargin(12.f, 5.f))
									.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
									{
										if (AChroniclesPlayerController* PC = WeakPC.Get())
										{
											if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(PC))
											{
												Music->SetMusicVolume(FMath::Clamp(Music->GetMusicVolume() + 0.10f, 0.f, 1.f));
												Music->PlayMenuClickSFX();
											}
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Font(ChroniclesHUDPalette::FontBold(11))
										.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
										.Text(LOCTEXT("MenuVolUp", "+"))
									]
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
									const bool bMuted = Music && Music->IsMusicMuted();
									return bMuted ? FSlateColor(ChroniclesHUDPalette::HairlineCrimson) : FSlateColor(ChroniclesHUDPalette::TextEmerald);
								})
								.Padding(FMargin(1.f))
								[
									SNew(SButton)
									.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
									.ContentPadding(FMargin(12.f, 5.f))
									.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
									{
										if (AChroniclesPlayerController* PC = WeakPC.Get())
										{
											if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(PC))
											{
												Music->SetMusicMuted(!Music->IsMusicMuted());
												Music->PlayMenuClickSFX();
											}
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Font(ChroniclesHUDPalette::FontBold(10))
										.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
										{
											AChroniclesPlayerController* PC = WeakPC.Get();
											const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
											const bool bMuted = Music && Music->IsMusicMuted();
											return bMuted ? FSlateColor(ChroniclesHUDPalette::TextCrimson) : FSlateColor(ChroniclesHUDPalette::TextEmerald);
										})
										.Text_Lambda([WeakPC = OwnerPC]() -> FText
										{
											AChroniclesPlayerController* PC = WeakPC.Get();
											const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
											const bool bMuted = Music && Music->IsMusicMuted();
											return bMuted
												? LOCTEXT("MenuSoundOff", "Звук: ВЫКЛ")
												: LOCTEXT("MenuSoundOn", "Звук: ВКЛ");
										})
									]
								]
							]
						]
					]
				]
			]
		]

		// --- Layer 2: Subtle Bottom-Right Version & Engine Watermark over the 3D Scene ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 28.f, 22.f))
		[
			SNew(STextBlock)
			.Font(ChroniclesHUDPalette::FontRegular(11))
			.ColorAndOpacity(FLinearColor(0.84f, 0.80f, 0.72f, 0.84f))
			.Text(LOCTEXT("MainMenuWatermark", "AETHERIA: CHRONICLES OF THE DRAKE  ·  UE 5.8.4  ·  D3D12 SM6 RTX"))
		];
}

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
				SNew(SBox).HeightOverride(7.f)
				[
					SNew(SProgressBar)
					.Style(&ChroniclesHUDPalette::RecessedBarBaseStyle())
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
			.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.96f))
			.Padding(FMargin(24.f, 20.f))
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
								TEXT("ATLAS OF AETHERIA  ·  4km x 4km REALM CHART  (%.1f%% EXPLORED)"), Total));
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
						.ContentPadding(FMargin(14.f, 5.f))
						.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
						{
							if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(10))
							.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
							.Text(LOCTEXT("CloseBtn", "Close [M / Esc]"))
						]
					]
				]

				// Main Map + Side Breakdown
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					// 64x64 Live Cartographic Fog-of-War Slate Canvas
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(0.f, 0.f, 18.f, 0.f))
					[
						SNew(SFogOfWarMapWidget)
						.OwnerPC(OwnerPC)
						.MapSize(526.f)
					]

					// Right Column: 5 Biomes % + Discovered Landmarks
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(336.f)
						.HeightOverride(526.f)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(11))
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
								.Text(LOCTEXT("BiomeBreakdownHdr", "BIOME EXPLORATION"))
							]

							+ SVerticalBox::Slot().AutoHeight()
							[ BuildBiomePctRow(EChronicleBiome::OakhavenValley,  FLinearColor(0.56f, 0.62f, 0.30f, 1.f)) ]
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildBiomePctRow(EChronicleBiome::BlueLeafWoods,   FLinearColor(0.24f, 0.56f, 0.72f, 1.f)) ]
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildBiomePctRow(EChronicleBiome::HighkeepCitadel, FLinearColor(0.76f, 0.22f, 0.26f, 1.f)) ]
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildBiomePctRow(EChronicleBiome::AshCalderaCrypt, FLinearColor(0.88f, 0.42f, 0.16f, 1.f)) ]
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildBiomePctRow(EChronicleBiome::CrystalGlacier,  FLinearColor(0.48f, 0.76f, 0.92f, 1.f)) ]

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
			.BorderImage(Pin.bDiscovered ? ChroniclesHUDPalette::ObsidianCardBrush() : ChroniclesHUDPalette::ObsidianCardLockedBrush())
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(FMargin(9.f, 5.f))
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
					.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
					.Visibility((Pin.bDiscovered && Pin.bFastTravel) ? EVisibility::Visible : EVisibility::Collapsed)
					.ContentPadding(FMargin(8.f, 3.f))
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
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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
			.BorderBackgroundColor(FLinearColor(BranchColors[i].R, BranchColors[i].G, BranchColors[i].B, 0.55f))
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
			.WidthOverride(1140.f)
			.HeightOverride(670.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.96f))
				.Padding(FMargin(24.f, 20.f))
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
							.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
							.ContentPadding(FMargin(14.f, 5.f))
							.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(10))
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
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

	auto FindPrereqTitle = [Skills](FName PrereqId) -> FString
	{
		if (!Skills || PrereqId.IsNone())
		{
			return FString();
		}
		for (const FSkillNodeDef& Candidate : Skills->Nodes)
		{
			if (Candidate.NodeId == PrereqId)
			{
				return Candidate.Title.ToString();
			}
		}
		return PrereqId.ToString();
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
			const FName PrereqId = Node.PrerequisiteNodeId;
			const FString PrereqTitle = FindPrereqTitle(PrereqId);
			const int32 MaxRank = Node.MaxRank;
			const int32 Tier = Node.Tier;

			SkillBranchBoxes[b]->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
				.BorderBackgroundColor(FLinearColor::White)
				.Padding(FMargin(9.f, 7.f))
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
					.Padding(FMargin(0.f, 3.f, 0.f, 4.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontRegular(8))
						.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
						.AutoWrapText(true)
						.Text(Node.Description)
					]

					// Clear Prerequisite Node Lock Hint for Locked Tier 2-4 Nodes
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 5.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(8))
						.ColorAndOpacity(ChroniclesHUDPalette::TextAmber)
						.AutoWrapText(true)
						.Visibility_Lambda([WeakPC = OwnerPC, PrereqId]() -> EVisibility
						{
							if (PrereqId.IsNone())
							{
								return EVisibility::Collapsed;
							}
							AChroniclesPlayerController* StrongPC = WeakPC.Get();
							const AAetheriaHeroCharacter* H = StrongPC ? StrongPC->GetActiveHero() : nullptr;
							const bool bMissingPrereq = (!H || !H->SkillTree || H->SkillTree->GetNodeRank(PrereqId) <= 0);
							return bMissingPrereq ? EVisibility::Visible : EVisibility::Collapsed;
						})
						.Text(FText::FromString(FString::Printf(TEXT("Requires: %s (Rank 1+)"), *PrereqTitle)))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					[
						SNew(SButton)
						.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
						.ContentPadding(FMargin(10.f, 3.f))
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
							.ColorAndOpacity_Lambda([WeakPC = OwnerPC, NodeId, PrereqId, MaxRank]() -> FSlateColor
							{
								AChroniclesPlayerController* StrongPC = WeakPC.Get();
								const AAetheriaHeroCharacter* H = StrongPC ? StrongPC->GetActiveHero() : nullptr;
								const int32 Rank = (H && H->SkillTree) ? H->SkillTree->GetNodeRank(NodeId) : 0;
								if (Rank >= MaxRank) return FSlateColor(ChroniclesHUDPalette::TextEmerald);
								if (!PrereqId.IsNone() && (!H || !H->SkillTree || H->SkillTree->GetNodeRank(PrereqId) <= 0))
								{
									return FSlateColor(ChroniclesHUDPalette::TextSecondary);
								}
								return FSlateColor(ChroniclesHUDPalette::TextGold);
							})
							.Text_Lambda([WeakPC = OwnerPC, NodeId, PrereqId, MaxRank]() -> FText
							{
								AChroniclesPlayerController* StrongPC = WeakPC.Get();
								const AAetheriaHeroCharacter* H = StrongPC ? StrongPC->GetActiveHero() : nullptr;
								const int32 Rank = (H && H->SkillTree) ? H->SkillTree->GetNodeRank(NodeId) : 0;
								if (Rank >= MaxRank) return LOCTEXT("SkillMaxed", "MAXED");
								if (!PrereqId.IsNone() && (!H || !H->SkillTree || H->SkillTree->GetNodeRank(PrereqId) <= 0))
								{
									return LOCTEXT("SkillLockedPrereq", "Locked");
								}
								return LOCTEXT("SkillUpgrade", "+ Upgrade");
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
			.WidthOverride(880.f)
			.HeightOverride(630.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.96f))
				.Padding(FMargin(24.f, 20.f))
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
							.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
							.ContentPadding(FMargin(14.f, 5.f))
							.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(10))
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
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
			.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
			.BorderBackgroundColor(FLinearColor::White)
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
						.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
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
							.ColorAndOpacity_Lambda([WeakPC = OwnerPC, QId]() -> FSlateColor
							{
								AChroniclesPlayerController* StrongPC = WeakPC.Get();
								UGameInstance* G = StrongPC ? StrongPC->GetGameInstance() : nullptr;
								const UQuestSubsystem* QS = G ? G->GetSubsystem<UQuestSubsystem>() : nullptr;
								const bool bTracked = (QS && QS->GetTrackedQuestId() == QId);
								return bTracked ? FSlateColor(ChroniclesHUDPalette::TextGold) : FSlateColor(ChroniclesHUDPalette::TextPrimary);
							})
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
			.WidthOverride(960.f)
			.HeightOverride(640.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.96f))
				.Padding(FMargin(24.f, 20.f))
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
							.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
							.ContentPadding(FMargin(12.f, 5.f))
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
								.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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
							.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
							.ContentPadding(FMargin(14.f, 5.f))
							.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(10))
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
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
			.WidthOverride(760.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.96f))
				.Padding(FMargin(26.f, 20.f))
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
							const FQuestDef* Q = ChroniclesHUDPalette::ResolveQuestForNPC(QS, NPC);
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
							const FQuestDef* Q = ChroniclesHUDPalette::ResolveQuestForNPC(QS, NPC);
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
								const FQuestDef* Q = ChroniclesHUDPalette::ResolveQuestForNPC(QS, NPC);
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
							.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
							.ContentPadding(FMargin(16.f, 6.f))
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
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const AAetheriaNPCActor* NPC = PC ? PC->GetDialogueNPC() : nullptr;
									if (NPC && NPC->NpcId == FName(TEXT("Npc_CaveTutor")))
									{
										return LOCTEXT("CaveTutorProceedBtn", "Выйти к свету Этерии · Step into the Light");
									}
									UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
									const UQuestSubsystem* QS = GI ? GI->GetSubsystem<UQuestSubsystem>() : nullptr;
									const FQuestDef* Q = ChroniclesHUDPalette::ResolveQuestForNPC(QS, NPC);
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
							.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
							.ContentPadding(FMargin(14.f, 6.f))
							.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontRegular(10))
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
								.Text(LOCTEXT("CloseDlgBtn", "Leave [Esc]"))
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAetheriaChroniclesHUD::BuildGraphicsRTXOverlay()
{
	auto MakeDLSSModeButton = [WeakPC = OwnerPC](int32 ModeIdx, const FText& LabelText) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor_Lambda([WeakPC, ModeIdx]() -> FSlateColor
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
				const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
				const bool bActive = (RTXSub && RTXSub->GetDLSSQualityMode() == ModeIdx);
				return bActive ? FSlateColor(ChroniclesHUDPalette::HairlineGold) : FSlateColor(ChroniclesHUDPalette::HairlineBronze);
			})
			.Padding(FMargin(1.5f))
			[
				SNew(SButton)
				.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
				.ContentPadding(FMargin(10.f, 6.f))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([WeakPC, ModeIdx]() -> FReply
				{
					if (AChroniclesPlayerController* PC = WeakPC.Get())
					{
						if (UGameInstance* GI = PC->GetGameInstance())
						{
							if (UGraphicsRTXSubsystem* RTXSub = GI->GetSubsystem<UGraphicsRTXSubsystem>())
							{
								RTXSub->SetDLSSQualityMode(ModeIdx);
								PC->PushToastMessage(FString::Printf(TEXT("Upscaler Mode: %s"), *RTXSub->GetDLSSModeLabel()), 3.0f);
							}
						}
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(9))
					.ColorAndOpacity_Lambda([WeakPC, ModeIdx]() -> FSlateColor
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
						const bool bActive = (RTXSub && RTXSub->GetDLSSQualityMode() == ModeIdx);
						return bActive ? FSlateColor(ChroniclesHUDPalette::TextGold) : FSlateColor(ChroniclesHUDPalette::TextPrimary);
					})
					.Text(LabelText)
				]
			];
	};

	auto MakeRTXFeatureToggle = [WeakPC = OwnerPC](
		const FString& FeatureName,
		TFunction<bool(const UGraphicsRTXSubsystem*)> GetterFn,
		TFunction<void(UGraphicsRTXSubsystem*, bool)> SetterFn) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor_Lambda([WeakPC, GetterFn]() -> FSlateColor
			{
				AChroniclesPlayerController* PC = WeakPC.Get();
				UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
				const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
				const bool bOn = (RTXSub && GetterFn(RTXSub));
				return bOn ? FSlateColor(ChroniclesHUDPalette::TextEmerald) : FSlateColor(ChroniclesHUDPalette::HairlineBronze);
			})
			.Padding(FMargin(1.5f))
			[
				SNew(SButton)
				.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
				.ContentPadding(FMargin(10.f, 7.f))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([WeakPC, FeatureName, GetterFn, SetterFn]() -> FReply
				{
					if (AChroniclesPlayerController* PC = WeakPC.Get())
					{
						if (UGameInstance* GI = PC->GetGameInstance())
						{
							if (UGraphicsRTXSubsystem* RTXSub = GI->GetSubsystem<UGraphicsRTXSubsystem>())
							{
								const bool bWantOn = !GetterFn(RTXSub);
								SetterFn(RTXSub, bWantOn);
								const bool bActualOn = GetterFn(RTXSub);
								PC->PushToastMessage(FString::Printf(TEXT("%s: %s"), *FeatureName, bActualOn ? TEXT("ENABLED") : TEXT("DISABLED")), 3.0f);
							}
						}
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(9))
					.ColorAndOpacity_Lambda([WeakPC, GetterFn]() -> FSlateColor
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
						const bool bOn = (RTXSub && GetterFn(RTXSub));
						return bOn ? FSlateColor(ChroniclesHUDPalette::TextEmerald) : FSlateColor(ChroniclesHUDPalette::TextSecondary);
					})
					.Text_Lambda([WeakPC, FeatureName, GetterFn]() -> FText
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
						const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
						const bool bOn = (RTXSub && GetterFn(RTXSub));
						return FText::FromString(FString::Printf(TEXT("%s: %s"), *FeatureName, bOn ? TEXT("ON") : TEXT("OFF")));
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
			return (PC && PC->ActiveOverlay == EChroniclesOverlayMode::GraphicsRTX) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.WidthOverride(740.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.97f))
				.Padding(FMargin(26.f, 22.f))
				[
					SNew(SVerticalBox)

					// Header
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
							.Text(LOCTEXT("RTXPanelHdr", "RTX HWRT  ·  NVIDIA DLSS 4.5  ·  AUDIO & TELEMETRY"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
							.ContentPadding(FMargin(14.f, 5.f))
							.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get()) { PC->CloseAllOverlays(); }
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(10))
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
								.Text_Lambda([WeakPC = OwnerPC]() -> FText
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									return (PC && PC->bMainMenuReturnTarget)
										? LOCTEXT("BackToMenuRTXBtn", "Back to Menu [Esc]")
										: LOCTEXT("CloseRTXBtn", "Close [F10 / Esc]");
								})
							]
						]
					]

					// Live Hardware & DLSS 4.5 Telemetry Card
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 14.f))
					[
						SNew(SBorder)
						.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
						.BorderBackgroundColor(FLinearColor::White)
						.Padding(FMargin(14.f, 10.f))
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontRegular(10))
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
									TEXT("GPU Adapter: %s\nActive Render Tier: %s\nHardware Ray Tracing: Supported=%s  |  Active=%s\nNVIDIA DLSS 4.5: %s  (Plugin=%s, Active=%s)\nRay Reconstruction: %s  |  Frame Generation: %s  |  NVIDIA Reflex: %s\nTexture Streaming Pool: %d MB (Frustum & Proximity Priority)\nBanner: %s"),
									*Tel.GPUAdapterName,
									TierLabel,
									Tel.bHardwareRTXSupported ? TEXT("YES") : TEXT("NO"),
									Tel.bHardwareRTXActive ? TEXT("ON") : TEXT("OFF"),
									*RTXSub->GetDLSSModeLabel(),
									Tel.bDLSSPluginDetected ? TEXT("YES") : TEXT("TSR Fallback"),
									Tel.bDLSSActive ? TEXT("ON") : TEXT("OFF"),
									RTXSub->IsDLSSRayReconstructionEnabled() ? TEXT("ON") : TEXT("OFF"),
									RTXSub->IsDLSSFrameGenEnabled() ? TEXT("ON") : TEXT("OFF"),
									RTXSub->IsReflexEnabled() ? TEXT("ON") : TEXT("OFF"),
									Tel.TexturePoolMB,
									*Tel.SummaryBanner));
							})
						]
					]

					// Section 1: Render Tier
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(10))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text(LOCTEXT("RTXTierSecHdr", "RENDER TIER"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
								const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
								const bool bActive = (RTXSub && RTXSub->GetTelemetry().ActiveTier == EChronicleRenderTier::CrimsonRTXUltra);
								return bActive ? FSlateColor(ChroniclesHUDPalette::HairlineGold) : FSlateColor(ChroniclesHUDPalette::HairlineBronze);
							})
							.Padding(FMargin(1.5f))
							[
								SNew(SButton)
								.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
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
												const EChronicleRenderTier ActualTier = RTXSub->GetTelemetry().ActiveTier;
												if (ActualTier == EChronicleRenderTier::CrimsonRTXUltra)
												{
													PC->PushToastMessage(TEXT("Active Render Tier: Crimson RTX Ultra (HWRT + Lumen)"), 3.2f);
												}
												else
												{
													PC->PushToastMessage(TEXT("GPU lacks Hardware RTX — Remaining on Classic Performance Mode"), 3.6f);
												}
											}
										}
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
									.Text(LOCTEXT("ApplyRTXBtn", "Crimson RTX Ultra"))
								]
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
								const UGraphicsRTXSubsystem* RTXSub = GI ? GI->GetSubsystem<UGraphicsRTXSubsystem>() : nullptr;
								const bool bActive = (RTXSub && RTXSub->GetTelemetry().ActiveTier == EChronicleRenderTier::ClassicPerformance);
								return bActive ? FSlateColor(ChroniclesHUDPalette::HairlineGold) : FSlateColor(ChroniclesHUDPalette::HairlineBronze);
							})
							.Padding(FMargin(1.5f))
							[
								SNew(SButton)
								.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
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
												const EChronicleRenderTier ActualTier = RTXSub->GetTelemetry().ActiveTier;
												PC->PushToastMessage(
													(ActualTier == EChronicleRenderTier::ClassicPerformance)
														? TEXT("Active Render Tier: Classic Performance Mode")
														: TEXT("Active Render Tier: Crimson RTX Ultra"),
													3.2f);
											}
										}
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
									.Text(LOCTEXT("ApplyClassicBtn", "Classic Performance"))
								]
							]
						]
					]

					// Section 2: NVIDIA DLSS 4.5 Quality Mode
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(10))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text(LOCTEXT("DLSSModeSecHdr", "NVIDIA DLSS 4.5 SUPER RESOLUTION MODE"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 12.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 6.f, 0.f))
						[ MakeDLSSModeButton(0, LOCTEXT("DLSSPerfBtn", "Performance")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(3.f, 0.f, 3.f, 0.f))
						[ MakeDLSSModeButton(1, LOCTEXT("DLSSBalBtn", "Balanced")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(3.f, 0.f, 3.f, 0.f))
						[ MakeDLSSModeButton(2, LOCTEXT("DLSSQualBtn", "Quality")) ]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(6.f, 0.f, 0.f, 0.f))
						[ MakeDLSSModeButton(3, LOCTEXT("DLSSDLAABtn", "DLAA")) ]
					]

					// Section 3: RTX Neural Features (Ray Reconstruction / Frame Gen / Reflex)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(10))
						.ColorAndOpacity(ChroniclesHUDPalette::TextCyan)
						.Text(LOCTEXT("RTXFeaturesSecHdr", "RTX FEATURES & LOW LATENCY"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 6.f, 0.f))
						[
							MakeRTXFeatureToggle(
								TEXT("Ray Reconstruction"),
								[](const UGraphicsRTXSubsystem* S) { return S->IsDLSSRayReconstructionEnabled(); },
								[](UGraphicsRTXSubsystem* S, bool bOn) { S->SetDLSSRayReconstructionEnabled(bOn); })
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(3.f, 0.f, 3.f, 0.f))
						[
							MakeRTXFeatureToggle(
								TEXT("Frame Generation"),
								[](const UGraphicsRTXSubsystem* S) { return S->IsDLSSFrameGenEnabled(); },
								[](UGraphicsRTXSubsystem* S, bool bOn) { S->SetDLSSFrameGenEnabled(bOn); })
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(6.f, 0.f, 0.f, 0.f))
						[
							MakeRTXFeatureToggle(
								TEXT("NVIDIA Reflex"),
								[](const UGraphicsRTXSubsystem* S) { return S->IsReflexEnabled(); },
								[](UGraphicsRTXSubsystem* S, bool bOn) { S->SetReflexEnabled(bOn); })
						]
					]

					// Hairline Divider
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 14.f, 0.f, 10.f))
					[
						SNew(SBox)
						.HeightOverride(1.f)
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGoldSubtle)
						]
					]

					// Section 4: Audio & Soundtrack Controls
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(10))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
						.Text(LOCTEXT("AudioSecHdr", "AUDIO & SOUNDTRACK  ·  МУЗЫКА И ЗВУК"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
						.BorderBackgroundColor(FLinearColor::White)
						.Padding(FMargin(14.f, 9.f))
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							.Padding(FMargin(0.f, 0.f, 12.f, 0.f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
									.Text_Lambda([WeakPC = OwnerPC]() -> FText
									{
										AChroniclesPlayerController* PC = WeakPC.Get();
										const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
										const FString Track = Music ? Music->GetCurrentTrackDisplayName() : TEXT("Aetheria Soundtrack");
										return FText::FromString(FString::Printf(
											TEXT("OST · %s"),
											Track.IsEmpty() ? TEXT("Aetheria Soundtrack") : *Track));
									})
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 2.f, 0.f, 0.f))
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontRegular(9))
									.ColorAndOpacity(ChroniclesHUDPalette::TextSecondary)
									.Text_Lambda([WeakPC = OwnerPC]() -> FText
									{
										AChroniclesPlayerController* PC = WeakPC.Get();
										const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
										const int32 VolPct = Music ? FMath::RoundToInt(FMath::Clamp(Music->GetMusicVolume(), 0.f, 1.f) * 100.f) : 75;
										const bool bMuted = Music && Music->IsMusicMuted();
										return FText::FromString(FString::Printf(
											TEXT("Громкость музыки: %d%%  ·  %s"),
											VolPct,
											bMuted ? TEXT("Заглушено (Muted)") : TEXT("Активно (Active)")));
									})
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor(ChroniclesHUDPalette::HairlineBronze)
								.Padding(FMargin(1.f))
								[
									SNew(SButton)
									.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
									.ContentPadding(FMargin(12.f, 6.f))
									.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
									{
										if (AChroniclesPlayerController* PC = WeakPC.Get())
										{
											if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(PC))
											{
												Music->SetMusicVolume(FMath::Clamp(Music->GetMusicVolume() - 0.10f, 0.f, 1.f));
												Music->PlayMenuClickSFX();
											}
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Font(ChroniclesHUDPalette::FontBold(9))
										.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
										.Text(LOCTEXT("RTXAudioVolDown", "− 10%"))
									]
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor(ChroniclesHUDPalette::HairlineBronze)
								.Padding(FMargin(1.f))
								[
									SNew(SButton)
									.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
									.ContentPadding(FMargin(12.f, 6.f))
									.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
									{
										if (AChroniclesPlayerController* PC = WeakPC.Get())
										{
											if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(PC))
											{
												Music->SetMusicVolume(FMath::Clamp(Music->GetMusicVolume() + 0.10f, 0.f, 1.f));
												Music->PlayMenuClickSFX();
											}
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Font(ChroniclesHUDPalette::FontBold(9))
										.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
										.Text(LOCTEXT("RTXAudioVolUp", "+ 10%"))
									]
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SBorder)
								.BorderImage(ChroniclesHUDPalette::WhiteBrush())
								.BorderBackgroundColor_Lambda([WeakPC = OwnerPC]() -> FSlateColor
								{
									AChroniclesPlayerController* PC = WeakPC.Get();
									const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
									const bool bMuted = Music && Music->IsMusicMuted();
									return bMuted ? FSlateColor(ChroniclesHUDPalette::HairlineCrimson) : FSlateColor(ChroniclesHUDPalette::TextEmerald);
								})
								.Padding(FMargin(1.f))
								[
									SNew(SButton)
									.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
									.ContentPadding(FMargin(12.f, 6.f))
									.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
									{
										if (AChroniclesPlayerController* PC = WeakPC.Get())
										{
											if (UAetheriaMusicSubsystem* Music = UAetheriaMusicSubsystem::Get(PC))
											{
												Music->SetMusicMuted(!Music->IsMusicMuted());
												Music->PlayMenuClickSFX();
											}
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Font(ChroniclesHUDPalette::FontBold(9))
										.ColorAndOpacity_Lambda([WeakPC = OwnerPC]() -> FSlateColor
										{
											AChroniclesPlayerController* PC = WeakPC.Get();
											const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
											const bool bMuted = Music && Music->IsMusicMuted();
											return bMuted ? FSlateColor(ChroniclesHUDPalette::TextCrimson) : FSlateColor(ChroniclesHUDPalette::TextEmerald);
										})
										.Text_Lambda([WeakPC = OwnerPC]() -> FText
										{
											AChroniclesPlayerController* PC = WeakPC.Get();
											const UAetheriaMusicSubsystem* Music = PC ? UAetheriaMusicSubsystem::Get(PC) : nullptr;
											const bool bMuted = Music && Music->IsMusicMuted();
											return bMuted
												? LOCTEXT("RTXAudioUnmute", "Unmute · ВКЛ")
												: LOCTEXT("RTXAudioMute", "Mute · ВЫКЛ");
										})
									]
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
	const FLinearColor StudioObsidianTint(1.f, 1.f, 1.f, 0.86f);
	TWeakPtr<SAetheriaChroniclesHUD> WeakHUD = DoesSharedInstanceExist()
		? TWeakPtr<SAetheriaChroniclesHUD>(SharedThis(this))
		: TWeakPtr<SAetheriaChroniclesHUD>();

	auto MakeStudioCamBtn = [WeakPC = OwnerPC](const FText& Label, TFunction<void(AChroniclesPlayerController*)> OnClick, TFunction<bool(const AChroniclesPlayerController*)> IsActive = nullptr, bool bIsPrimaryCrimson = false) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::WhiteBrush())
			.BorderBackgroundColor_Lambda([WeakPC, IsActive, bIsPrimaryCrimson]() -> FSlateColor
			{
				if (bIsPrimaryCrimson)
				{
					return FSlateColor(ChroniclesHUDPalette::HairlineGold);
				}
				const AChroniclesPlayerController* PC = WeakPC.Get();
				const bool bHot = (PC && IsActive && IsActive(PC));
				return bHot ? FSlateColor(ChroniclesHUDPalette::HairlineGold) : FSlateColor(ChroniclesHUDPalette::HairlineBronze);
			})
			.Padding(FMargin(1.f))
			[
				SNew(SButton)
				.ButtonStyle(bIsPrimaryCrimson ? &ChroniclesHUDPalette::CrimsonButtonStyle() : &ChroniclesHUDPalette::StoneButtonStyle())
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
					.ColorAndOpacity_Lambda([WeakPC, IsActive, bIsPrimaryCrimson]() -> FSlateColor
					{
						if (bIsPrimaryCrimson)
						{
							return FSlateColor(ChroniclesHUDPalette::TextPrimary);
						}
						const AChroniclesPlayerController* PC = WeakPC.Get();
						const bool bHot = (PC && IsActive && IsActive(PC));
						return bHot ? FSlateColor(ChroniclesHUDPalette::TextGold) : FSlateColor(ChroniclesHUDPalette::TextPrimary);
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
					? FSlateColor(ChroniclesHUDPalette::HairlineGold)
					: FSlateColor(ChroniclesHUDPalette::HairlineBronze);
			})
			.Padding(FMargin(1.5f))
			[
				SNew(SButton)
				.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
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
							? FSlateColor(ChroniclesHUDPalette::TextGold)
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

		// --- Top-Center Carved Obsidian Studio Camera Control Bar (Center 42%+ stays open for 3D Hero!) ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 14.f, 0.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.90f))
			.Padding(FMargin(14.f, 8.f))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.f, 0.f, 10.f, 0.f))
				[
					SNew(STextBlock)
					.Font(ChroniclesHUDPalette::FontBold(11))
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
				+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 6.f, 0.f))
				[
					SNew(SBox)
					.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
					{
						AChroniclesPlayerController* PC = WeakPC.Get();
						return (PC && PC->bIsNewGameCreationFlow) ? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						SNew(SBorder)
						.BorderImage(ChroniclesHUDPalette::WhiteBrush())
						.BorderBackgroundColor(ChroniclesHUDPalette::HairlineBronze)
						.Padding(FMargin(1.f))
						[
							SNew(SButton)
							.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
							.ContentPadding(FMargin(10.f, 4.f))
							.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
							{
								if (AChroniclesPlayerController* PC = WeakPC.Get())
								{
									PC->CloseAllOverlays();
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Font(ChroniclesHUDPalette::FontBold(9))
								.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
								.Text(LOCTEXT("CamBackTitleBtn", "< Title"))
							]
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBorder)
					.BorderImage(ChroniclesHUDPalette::WhiteBrush())
					.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
					.Padding(FMargin(1.f))
					[
						SNew(SButton)
						.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
						.ContentPadding(FMargin(12.f, 4.f))
						.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
						{
							if (AChroniclesPlayerController* PC = WeakPC.Get())
							{
								if (PC->bIsNewGameCreationFlow)
								{
									PC->ConfirmNewHeroAndAwakenInCave();
								}
								else
								{
									PC->CloseAllOverlays();
								}
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Font(ChroniclesHUDPalette::FontBold(9))
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text_Lambda([WeakPC = OwnerPC]() -> FText
							{
								AChroniclesPlayerController* PC = WeakPC.Get();
								if (PC && PC->bIsNewGameCreationFlow)
								{
									return LOCTEXT("CamBeginJourneyTopBtn", "ПРОБУДИТЬСЯ В ЭТЕРИИ · BEGIN JOURNEY");
								}
								return (PC && PC->bMainMenuReturnTarget)
									? LOCTEXT("CamBackMenuBtn", "Back to Menu [Esc]")
									: LOCTEXT("CamCloseBtn", "Close [C/Esc]");
							})
						]
					]
				]
			]
		]

		// --- Left Edge-Docked Translucent Obsidian Panel (Width 372px, Alpha 0.86): Appearance, Race, Physique ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(16.f, 68.f, 0.f, 18.f))
		[
			SNew(SBox)
			.WidthOverride(372.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(StudioObsidianTint)
				.Padding(FMargin(16.f, 14.f))
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

					// 3 Clickable Tabs with Gold Selection Highlights
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

		// --- Right Edge-Docked Translucent Obsidian Panel (Width 362px, Alpha 0.86): RPG Attributes & Live Combat Power ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(0.f, 68.f, 16.f, 18.f))
		[
			SNew(SBox)
			.WidthOverride(362.f)
			[
				SNew(SBorder)
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(StudioObsidianTint)
				.Padding(FMargin(16.f, 14.f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
					[
						SNew(STextBlock)
						.Font(ChroniclesHUDPalette::FontBold(13))
						.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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

					// Prominent Forged Crimson Bottom-Right CTA for New Game Creation -> Awakening Cave
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 10.f, 0.f, 0.f))
					[
						SNew(SBox)
						.Visibility_Lambda([WeakPC = OwnerPC]() -> EVisibility
						{
							AChroniclesPlayerController* PC = WeakPC.Get();
							return (PC && PC->bIsNewGameCreationFlow) ? EVisibility::Visible : EVisibility::Collapsed;
						})
						[
							SNew(SBorder)
							.BorderImage(ChroniclesHUDPalette::WhiteBrush())
							.BorderBackgroundColor(ChroniclesHUDPalette::HairlineGold)
							.Padding(FMargin(1.5f))
							[
								SNew(SButton)
								.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
								.ContentPadding(FMargin(12.f, 11.f))
								.HAlign(HAlign_Center)
								.OnClicked_Lambda([WeakPC = OwnerPC]() -> FReply
								{
									if (AChroniclesPlayerController* PC = WeakPC.Get())
									{
										PC->ConfirmNewHeroAndAwakenInCave();
									}
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Font(ChroniclesHUDPalette::FontBold(10))
									.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
									.Justification(ETextJustify::Center)
									.Text(LOCTEXT("CreatorBeginJourneyBottomCTA", "ПРОБУДИТЬСЯ В ЭТЕРИИ · BEGIN JOURNEY"))
								]
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
		.Padding(FMargin(0.f, 7.f, 0.f, 5.f))
		[
			SNew(STextBlock)
			.Font(ChroniclesHUDPalette::FontBold(10))
			.ColorAndOpacity(Color)
			.Text(FText::FromString(Title))
		];
	};

	auto MakeSelectableChip = [WeakPC = OwnerPC, WeakHUD](const FString& Label, bool bSelected, TFunction<void(UHeroCustomizationComponent*)> Action) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.ButtonStyle(bSelected ? &ChroniclesHUDPalette::ActiveGoldButtonStyle() : &ChroniclesHUDPalette::StoneButtonStyle())
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
				.ColorAndOpacity(bSelected ? ChroniclesHUDPalette::TextGold : ChroniclesHUDPalette::TextPrimary)
				.Text(FText::FromString(Label))
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
			.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
			.BorderBackgroundColor(FLinearColor::White)
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
						.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
						.ContentPadding(FMargin(9.f, 3.f))
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
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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
						.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
						.ContentPadding(FMargin(9.f, 3.f))
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
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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
			.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(FMargin(10.f, 8.f))
			[
				SNew(STextBlock)
				.Font(ChroniclesHUDPalette::FontRegular(9))
				.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
				.AutoWrapText(true)
				.Text(FText::FromString(UHeroCustomizationComponent::GetRaceLoreAndPassives(Profile.Race)))
			]
		];

		AddSectionHeader(CreatorLeftContentBox, TEXT("HERO DETAIL QUALITY"), ChroniclesHUDPalette::TextGold);
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

		AddSectionHeader(CreatorLeftContentBox, TEXT("EYES & WAR PAINT"), ChroniclesHUDPalette::TextGold);

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

		AddSectionHeader(CreatorLeftContentBox, TEXT("STATURE / HEIGHT (1.10m - 3.00m)"), ChroniclesHUDPalette::TextGold);

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
		.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
		.BorderBackgroundColor(FLinearColor::White)
		.Padding(FMargin(10.f, 7.f))
		[
			SNew(STextBlock)
			.Font(ChroniclesHUDPalette::FontBold(11))
			.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
			.Justification(ETextJustify::Center)
			.Text(FText::FromString(FString::Printf(TEXT("Unspent Attribute Points: %d"), Profile.Attributes.UnspentPoints)))
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
			.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(FMargin(9.f, 5.f))
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
						.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
						.ContentPadding(FMargin(8.f, 1.f))
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
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
							.Text(LOCTEXT("AttrMinusBtn", "-"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
						.ContentPadding(FMargin(8.f, 1.f))
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
							.ColorAndOpacity(ChroniclesHUDPalette::TextGold)
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

	// Reset Stats (Stone) & Save Hero (Forged Crimson CTA) Buttons
	CreatorRightStatsBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 4.f, 0.f, 8.f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 4.f, 0.f))
		[
			SNew(SButton)
			.ButtonStyle(&ChroniclesHUDPalette::StoneButtonStyle())
			.ContentPadding(FMargin(8.f, 6.f))
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
				.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
				.Text(LOCTEXT("ResetStatsBtn", "Reset Stats"))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(4.f, 0.f, 0.f, 0.f))
		[
			SNew(SButton)
			.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
			.ContentPadding(FMargin(8.f, 6.f))
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
			.BorderImage(ChroniclesHUDPalette::ObsidianCardBrush())
			.BorderBackgroundColor(FLinearColor::White)
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
		.BorderBackgroundColor(FLinearColor(0.05f, 0.008f, 0.012f, 0.82f))
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
				.BorderImage(ChroniclesHUDPalette::PanelObsidianBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.98f))
				.Padding(FMargin(46.f, 34.f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
					[
						SNew(SBox)
						.WidthOverride(56.f)
						.HeightOverride(56.f)
						.Visibility(ChroniclesHUDPalette::SkullCrestBrush() ? EVisibility::Visible : EVisibility::Collapsed)
						[
							SNew(SImage)
							.Image(ChroniclesHUDPalette::SkullCrestBrush())
							.ColorAndOpacity(FLinearColor(0.98f, 0.36f, 0.30f, 0.96f))
						]
					]

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
					.Padding(FMargin(0.f, 8.f, 0.f, 22.f))
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
						.ButtonStyle(&ChroniclesHUDPalette::CrimsonButtonStyle())
						.ContentPadding(FMargin(26.f, 10.f))
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
							.ColorAndOpacity(ChroniclesHUDPalette::TextPrimary)
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
