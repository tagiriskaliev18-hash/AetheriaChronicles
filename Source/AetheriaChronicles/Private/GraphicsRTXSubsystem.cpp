// ============================================================================
// GraphicsRTXSubsystem.cpp — Hardware RTX Probe, Non-RTX Simplified Fallback,
// Auto-DLSS / Reflex & Invisible Proximity Texture Streaming
// ============================================================================
#include "GraphicsRTXSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RHI.h"
#include "RenderUtils.h"

void UGraphicsRTXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Suppress red dev on-screen warnings ("Lumen surface cache...", "Preparing Textures") in standalone -game mode
	if (!GIsEditor || FParse::Param(FCommandLine::Get(), TEXT("game")))
	{
		GAreScreenMessagesEnabled = false;
	}

	DetectAndConfigureRenderTier();

	// Re-apply once when the first Game/PIE world initializes (after DLSS & StreamlineCore modules finish StartupModule)
	bRefreshedAfterPluginInit = false;
	PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddWeakLambda(
		this,
		[this](UWorld* World, const UWorld::InitializationValues /*IVS*/)
		{
			if (bRefreshedAfterPluginInit || !World)
			{
				return;
			}
			if (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE)
			{
				bRefreshedAfterPluginInit = true;
				ApplyRenderTier(Telemetry.ActiveTier);
			}
		});
}

void UGraphicsRTXSubsystem::Deinitialize()
{
	if (PostWorldInitHandle.IsValid())
	{
		FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);
		PostWorldInitHandle.Reset();
	}

	Super::Deinitialize();
}

bool UGraphicsRTXSubsystem::HasCVar(const TCHAR* Name) const
{
	return IConsoleManager::Get().FindConsoleVariable(Name) != nullptr;
}

void UGraphicsRTXSubsystem::SetCVarInt(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		// ECVF_SetByCode cleanly overrides DefaultEngine.ini (ECVF_SetByProjectSetting) for live RTX <-> Non-RTX switching!
		Var->Set(Value, ECVF_SetByCode);
	}
}

void UGraphicsRTXSubsystem::SetCVarFloat(const TCHAR* Name, float Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		Var->Set(Value, ECVF_SetByCode);
	}
}

bool UGraphicsRTXSubsystem::DetectHardwareRTXCaps(FString& OutAdapterName) const
{
	OutAdapterName = GRHIAdapterName.IsEmpty() ? TEXT("Unknown GPU") : GRHIAdapterName;

	// Allow explicit testing of the non-RTX simplified version via `-nortx` command line flag
	if (FParse::Param(FCommandLine::Get(), TEXT("nortx")))
	{
		UE_LOG(LogChronicles, Log, TEXT("[GraphicsRTX] -nortx override detected; forcing ClassicPerformance mode."));
		return false;
	}

	// Headless / NullRHI commandlets cannot run hardware RT
	if (IsRunningCommandlet() || GDynamicRHI == nullptr)
	{
		return false;
	}

	const bool bRhiSupportsRT = GRHISupportsRayTracing && IsRayTracingEnabled();
	const FString Upper = OutAdapterName.ToUpper();
	const bool bKnownRTFamily =
		Upper.Contains(TEXT("RTX")) ||
		Upper.Contains(TEXT("RX 6")) ||
		Upper.Contains(TEXT("RX 7")) ||
		Upper.Contains(TEXT("RX 8")) ||
		Upper.Contains(TEXT("RX 9")) ||
		Upper.Contains(TEXT("INTEL")) && Upper.Contains(TEXT("ARC"));

	return bRhiSupportsRT && bKnownRTFamily;
}

void UGraphicsRTXSubsystem::DetectAndConfigureRenderTier()
{
	FString Adapter;
	Telemetry.bHardwareRTXSupported = DetectHardwareRTXCaps(Adapter);
	Telemetry.GPUAdapterName = Adapter;

	// Detect if official NVIDIA DLSS 4.5 / Streamline plugin CVars are loaded
	Telemetry.bDLSSPluginDetected =
		HasCVar(TEXT("r.NGX.DLSS.Enable")) ||
		HasCVar(TEXT("r.NGX.DLSS.Quality")) ||
		HasCVar(TEXT("r.NGX.DLSSD.Enable")) ||
		HasCVar(TEXT("r.NGX.DLSS.DenoiserMode")) ||
		HasCVar(TEXT("r.Streamline.DLSSG.Enable"));
	Telemetry.bReflexActive = false;

	if (Telemetry.bHardwareRTXSupported)
	{
		ApplyRenderTier(EChronicleRenderTier::CrimsonRTXUltra);
	}
	else
	{
		ApplyRenderTier(EChronicleRenderTier::ClassicPerformance);
	}
}

FString UGraphicsRTXSubsystem::GetDLSSModeLabel() const
{
	switch (FMath::Clamp(ActiveDLSSQualityMode, 0, 3))
	{
	case 0:  return TEXT("Performance");
	case 1:  return TEXT("Balanced");
	case 2:  return TEXT("Quality");
	case 3:  return TEXT("DLAA");
	default: return TEXT("Quality");
	}
}

void UGraphicsRTXSubsystem::SetDLSSQualityMode(int32 QualityMode)
{
	ActiveDLSSQualityMode = FMath::Clamp(QualityMode, 0, 3);

	if (Telemetry.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
	{
		if (Telemetry.bDLSSPluginDetected && (HasCVar(TEXT("r.NGX.DLSS.Enable")) || HasCVar(TEXT("r.NGX.DLSS.Quality"))))
		{
			// Exact UE 5.8 NVIDIA DLSS 4.5 v8.9.0 Super Resolution mapping:
			// ScreenPercentage drives Performance(50%), Balanced(58%), Quality(66.7%), DLAA(100%)
			static const float DlssScreenPercents[4] = { 50.0f, 58.0f, 66.7f, 100.0f };
			static const int32 LegacyNGXQualityMap[4] = { 1, 2, 3, 4 };
			SetCVarInt(TEXT("r.NGX.Enable"), 1);
			SetCVarInt(TEXT("r.NGX.DLSS.Enable"), 1);
			SetCVarInt(TEXT("r.NGX.DLSS.AutoExposure"), 1);
			SetCVarInt(TEXT("r.NGX.DLSS.Quality"), LegacyNGXQualityMap[ActiveDLSSQualityMode]); // Harmless legacy fallback
			SetCVarInt(TEXT("r.TemporalAA.Upscaler"), 1);
			SetCVarInt(TEXT("r.TemporalAA.Upsampling"), 1);
			SetCVarFloat(TEXT("r.ScreenPercentage"), DlssScreenPercents[ActiveDLSSQualityMode]);
			Telemetry.bDLSSActive = true;
		}
		else
		{
			// Crisp UE 5.8 TSR fallback mirroring the selected quality tier until DLSS plugin is loaded
			static const float TsrScreenPercents[4] = { 58.0f, 68.0f, 78.0f, 100.0f };
			SetCVarInt(TEXT("r.AntiAliasingMethod"), 4);
			SetCVarInt(TEXT("r.TemporalAA.Upsampling"), 1);
			SetCVarFloat(TEXT("r.ScreenPercentage"), TsrScreenPercents[ActiveDLSSQualityMode]);
			Telemetry.bDLSSActive = false;
		}
	}

	UpdateTelemetryBanner();
	UE_LOG(LogChronicles, Display, TEXT("[GraphicsRTX] Quality Mode -> %s (%s)"),
		*GetDLSSModeLabel(),
		Telemetry.bDLSSActive ? TEXT("DLSS 4.5") : TEXT("TSR Fallback"));
}

void UGraphicsRTXSubsystem::SetDLSSRayReconstructionEnabled(bool bEnable)
{
	const bool bSupportsRR = HasCVar(TEXT("r.NGX.DLSSD.Enable")) || HasCVar(TEXT("r.NGX.DLSS.DenoiserMode"));
	const bool bActivate = bEnable
		&& (Telemetry.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
		&& Telemetry.bDLSSPluginDetected
		&& bSupportsRR;

	if (bActivate)
	{
		// Exact UDLSSLibrary::EnableDLSSRR CVar set for UE 5.8 DLSS 4.5 v8.9.0
		SetCVarInt(TEXT("r.NGX.DLSS.DenoiserMode"), 1);
		SetCVarInt(TEXT("r.NGX.DLSSD.Enable"), 1);
		SetCVarInt(TEXT("r.Shadow.Denoiser"), 0);
		SetCVarInt(TEXT("r.Lumen.Reflections.ScreenSpaceReconstruction"), 0);
		SetCVarInt(TEXT("r.Lumen.Reflections.Temporal"), 0);
		SetCVarInt(TEXT("r.Lumen.Reflections.BilateralFilter"), 0);
		SetCVarInt(TEXT("r.Lumen.Reflections.ExportHitT"), 1);
		SetCVarInt(TEXT("r.TemporalAA.Upscaler"), 1);
	}
	else
	{
		SetCVarInt(TEXT("r.NGX.DLSS.DenoiserMode"), 0);
		SetCVarInt(TEXT("r.NGX.DLSSD.Enable"), 0);
		SetCVarInt(TEXT("r.Shadow.Denoiser"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.ScreenSpaceReconstruction"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.Temporal"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.BilateralFilter"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.ExportHitT"), 0);
	}
	bDLSSRayReconstructionActive = bActivate;

	UpdateTelemetryBanner();
}

void UGraphicsRTXSubsystem::SetDLSSFrameGenEnabled(bool bEnable)
{
	const FString UpperGPU = Telemetry.GPUAdapterName.ToUpper();
	const bool bSupportsFGHardware = UpperGPU.Contains(TEXT("RTX 40")) || UpperGPU.Contains(TEXT("RTX 50"));
	const bool bActivate = bEnable
		&& (Telemetry.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
		&& Telemetry.bDLSSPluginDetected
		&& bSupportsFGHardware
		&& HasCVar(TEXT("r.Streamline.DLSSG.Enable"));

	SetCVarInt(TEXT("r.Streamline.DLSSG.Enable"), bActivate ? 1 : 0);
	SetCVarInt(TEXT("r.Streamline.DLSSG.FramesToGenerate"), bActivate ? 1 : 0);
	bDLSSFrameGenActive = bActivate;

	if (bActivate)
	{
		// DLSS Frame Generation pairs with NVIDIA Reflex Low Latency
		SetReflexEnabled(true);
	}
	else
	{
		UpdateTelemetryBanner();
	}
}

void UGraphicsRTXSubsystem::SetReflexEnabled(bool bEnable)
{
	const bool bHasReflex = HasCVar(TEXT("t.Streamline.Reflex.Mode")) || HasCVar(TEXT("r.Streamline.Reflex.Enable"));
	const bool bActivate = bEnable && bHasReflex;

	SetCVarInt(TEXT("t.Streamline.Reflex.Mode"), bActivate ? 1 : 0);
	SetCVarInt(TEXT("r.Streamline.Reflex.Enable"), bActivate ? 1 : 0);
	Telemetry.bReflexActive = bActivate;

	UpdateTelemetryBanner();
}

void UGraphicsRTXSubsystem::UpdateTelemetryBanner()
{
	if (Telemetry.ActiveTier == EChronicleRenderTier::CrimsonRTXUltra)
	{
		if (Telemetry.bDLSSActive)
		{
			const FString ModeLabel = GetDLSSModeLabel();
			const TCHAR* RRTag = bDLSSRayReconstructionActive ? TEXT("+RR") : TEXT("");
			const TCHAR* FGTag = bDLSSFrameGenActive ? TEXT("+FG") : TEXT("");
			Telemetry.SummaryBanner = FString::Printf(
				TEXT("RTX ULTRA · %s · DLSS 4.5 %s%s%s · Lumen HWRT + Nanite"),
				*Telemetry.GPUAdapterName,
				*ModeLabel,
				RRTag,
				FGTag);
		}
		else
		{
			static const int32 TsrPercents[4] = { 58, 68, 78, 100 };
			const int32 SafeIdx = FMath::Clamp(ActiveDLSSQualityMode, 0, 3);
			Telemetry.SummaryBanner = FString::Printf(
				TEXT("RTX ULTRA · %s · TSR %d%% · Lumen HWRT + Nanite"),
				*Telemetry.GPUAdapterName,
				TsrPercents[SafeIdx]);
		}
	}
	else
	{
		Telemetry.SummaryBanner = FString::Printf(
			TEXT("STANDARD MODE (No RTX) · %s · Fast Shadows + Streaming 1.5GB"),
			*Telemetry.GPUAdapterName);
	}
}

void UGraphicsRTXSubsystem::ApplyRenderTier(EChronicleRenderTier Tier)
{
	// Guard against enabling HWRT on a GPU without hardware ray tracing
	if (Tier == EChronicleRenderTier::CrimsonRTXUltra && !Telemetry.bHardwareRTXSupported)
	{
		Tier = EChronicleRenderTier::ClassicPerformance;
	}

	Telemetry.ActiveTier = Tier;

	if (Tier == EChronicleRenderTier::CrimsonRTXUltra)
	{
		// === CRIMSON DESERT RTX ULTRA TIER (RTX 3070+ 8GB VRAM tuned) ===
		// Restore ALL scalability buckets so switching back from ClassicPerformance never leaks low settings
		SetCVarInt(TEXT("sg.EffectsQuality"), 3);
		SetCVarInt(TEXT("sg.PostProcessQuality"), 3);
		SetCVarInt(TEXT("sg.FoliageQuality"), 3);
		SetCVarInt(TEXT("sg.ShadowQuality"), 3);
		SetCVarInt(TEXT("sg.GlobalIlluminationQuality"), 3);
		SetCVarInt(TEXT("sg.ReflectionQuality"), 3);

		SetCVarInt(TEXT("r.Lumen.HardwareRayTracing"), 1);
		SetCVarInt(TEXT("r.RayTracing.Shadows"), 1);
		SetCVarInt(TEXT("r.Lumen.DiffuseIndirect.Allow"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.Allow"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.DownsampleFactor"), 2);
		SetCVarInt(TEXT("r.LumenScene.SurfaceCache.AtlasSize"), 8192);
		SetCVarInt(TEXT("r.LumenScene.SurfaceCache.CardMaxResolution"), 256);
		SetCVarInt(TEXT("r.Shadow.Virtual.Enable"), 1);
		SetCVarInt(TEXT("r.VolumetricFog"), 1);
		SetCVarInt(TEXT("r.VolumetricCloud"), 1);
		SetCVarInt(TEXT("r.Nanite"), 1);
		SetCVarInt(TEXT("r.ContactShadows"), 1);
		SetCVarFloat(TEXT("r.ToneMapper.Sharpen"), 0.48f);

		// Invisible proximity & camera-frustum texture streaming (3072 MB pool for 8GB VRAM)
		SetCVarInt(TEXT("r.Streaming.PoolSize"), 3072);
		SetCVarInt(TEXT("r.Streaming.LimitPoolSizeToVRAM"), 1);
		SetCVarInt(TEXT("r.Streaming.UsePerTextureBias"), 1);
		SetCVarInt(TEXT("r.Streaming.FullyLoadUsedTextures"), 0);
		SetCVarInt(TEXT("r.Streaming.FramesForFullUpdate"), 2);
		SetCVarFloat(TEXT("r.Streaming.Boost"), 1.0f);
		Telemetry.TexturePoolMB = 3072;
		Telemetry.bHardwareRTXActive = true;

		// Re-probe DLSS / Streamline plugin CVars in case plugins registered after early init
		Telemetry.bDLSSPluginDetected =
			HasCVar(TEXT("r.NGX.DLSS.Enable")) ||
			HasCVar(TEXT("r.NGX.DLSS.Quality")) ||
			HasCVar(TEXT("r.NGX.DLSSD.Enable")) ||
			HasCVar(TEXT("r.NGX.DLSS.DenoiserMode")) ||
			HasCVar(TEXT("r.Streamline.DLSSG.Enable"));

		if (Telemetry.bDLSSPluginDetected)
		{
			SetDLSSQualityMode(ActiveDLSSQualityMode);
			SetDLSSRayReconstructionEnabled(true);

			const FString UpperGPU = Telemetry.GPUAdapterName.ToUpper();
			const bool bSupportsFG = UpperGPU.Contains(TEXT("RTX 40")) || UpperGPU.Contains(TEXT("RTX 50"));
			SetDLSSFrameGenEnabled(bSupportsFG);
			SetReflexEnabled(true);
		}
		else
		{
			bDLSSRayReconstructionActive = false;
			bDLSSFrameGenActive = false;
			SetDLSSQualityMode(ActiveDLSSQualityMode);
			SetReflexEnabled(HasCVar(TEXT("t.Streamline.Reflex.Mode")) || HasCVar(TEXT("r.Streamline.Reflex.Enable")));
		}

		UpdateTelemetryBanner();
	}
	else
	{
		// === SIMPLIFIED NON-RTX PERFORMANCE VERSION (GTX / Integrated / Older GPUs) ===
		// Explicitly tear down DLSS / Ray Reconstruction / Frame Gen / Reflex so Ultra -> Classic never leaks state
		SetCVarInt(TEXT("r.NGX.DLSS.Enable"), 0);
		SetCVarInt(TEXT("r.NGX.DLSSD.Enable"), 0);
		SetCVarInt(TEXT("r.NGX.DLSS.DenoiserMode"), 0);
		SetCVarInt(TEXT("r.Streamline.DLSSG.Enable"), 0);
		SetCVarInt(TEXT("r.Streamline.DLSSG.FramesToGenerate"), 0);
		SetCVarInt(TEXT("t.Streamline.Reflex.Mode"), 0);
		SetCVarInt(TEXT("r.Streamline.Reflex.Enable"), 0);
		SetCVarInt(TEXT("r.Shadow.Denoiser"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.ScreenSpaceReconstruction"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.Temporal"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.BilateralFilter"), 1);
		SetCVarInt(TEXT("r.Lumen.Reflections.ExportHitT"), 0);

		SetCVarInt(TEXT("r.Lumen.HardwareRayTracing"), 0);
		SetCVarInt(TEXT("r.RayTracing.Shadows"), 0);
		SetCVarInt(TEXT("r.Lumen.DiffuseIndirect.Allow"), 0);
		SetCVarInt(TEXT("r.Lumen.Reflections.Allow"), 0);
		SetCVarInt(TEXT("r.Shadow.Virtual.Enable"), 0); // Classic lightweight CSM shadows
		SetCVarInt(TEXT("r.VolumetricFog"), 0);
		SetCVarInt(TEXT("r.VolumetricCloud"), 0);
		SetCVarInt(TEXT("r.ContactShadows"), 0);
		SetCVarInt(TEXT("r.AntiAliasingMethod"), 2); // Fast TAA
		SetCVarFloat(TEXT("r.ScreenPercentage"), 72.0f);
		SetCVarFloat(TEXT("r.ToneMapper.Sharpen"), 0.25f);
		SetCVarInt(TEXT("r.Streaming.PoolSize"), 1536);
		SetCVarInt(TEXT("r.Streaming.LimitPoolSizeToVRAM"), 1);
		SetCVarInt(TEXT("r.Streaming.UsePerTextureBias"), 1);
		SetCVarInt(TEXT("r.Streaming.FullyLoadUsedTextures"), 0);
		SetCVarInt(TEXT("sg.EffectsQuality"), 1);
		SetCVarInt(TEXT("sg.PostProcessQuality"), 1);
		SetCVarInt(TEXT("sg.FoliageQuality"), 1);
		SetCVarInt(TEXT("sg.ShadowQuality"), 1);
		SetCVarInt(TEXT("sg.GlobalIlluminationQuality"), 1);
		SetCVarInt(TEXT("sg.ReflectionQuality"), 1);

		Telemetry.TexturePoolMB = 1536;
		Telemetry.bHardwareRTXActive = false;
		Telemetry.bDLSSActive = false;
		Telemetry.bReflexActive = false;
		bDLSSRayReconstructionActive = false;
		bDLSSFrameGenActive = false;

		UpdateTelemetryBanner();
	}

	UE_LOG(LogChronicles, Display, TEXT("[GraphicsRTX] %s"), *Telemetry.SummaryBanner);
}
