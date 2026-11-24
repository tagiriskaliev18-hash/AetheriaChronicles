// ============================================================================
// GraphicsRTXSubsystem.cpp — Hardware RTX Probe, Non-RTX Simplified Fallback,
// Auto-DLSS / Reflex & Invisible Proximity Texture Streaming
// ============================================================================
#include "GraphicsRTXSubsystem.h"
#include "Engine/Engine.h"
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

	// Detect if official NVIDIA DLSS / Streamline plugin CVars are loaded
	Telemetry.bDLSSPluginDetected = HasCVar(TEXT("r.NGX.DLSS.Enable")) || HasCVar(TEXT("r.Streamline.DLSSG.Enable"));
	Telemetry.bReflexActive = HasCVar(TEXT("r.Streamline.Reflex.Enable")) || HasCVar(TEXT("t.Streamline.Reflex.Mode"));

	if (Telemetry.bHardwareRTXSupported)
	{
		ApplyRenderTier(EChronicleRenderTier::CrimsonRTXUltra);
	}
	else
	{
		ApplyRenderTier(EChronicleRenderTier::ClassicPerformance);
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

		// Auto-activate NVIDIA DLSS + Ray Reconstruction + Reflex if plugin present; else TSR Quality
		if (Telemetry.bDLSSPluginDetected)
		{
			SetCVarInt(TEXT("r.NGX.Enable"), 1);
			SetCVarInt(TEXT("r.NGX.DLSS.Enable"), 1);
			SetCVarInt(TEXT("r.NGX.DLSS.Quality"), 2); // Balanced/Quality sweet spot for 1440p RTX 3070
			SetCVarInt(TEXT("r.NGX.DLSS.DenoiserMode"), 1); // Ray Reconstruction when supported
			SetCVarInt(TEXT("r.Streamline.Reflex.Enable"), 1);
			Telemetry.bDLSSActive = true;
		}
		else
		{
			// Crisp UE 5.8 TSR upscaling at 78% internal res (nearest visual parity until DLSS zip is dropped in)
			SetCVarInt(TEXT("r.AntiAliasingMethod"), 4);
			SetCVarInt(TEXT("r.TemporalAA.Upsampling"), 1);
			SetCVarFloat(TEXT("r.ScreenPercentage"), 78.0f);
			Telemetry.bDLSSActive = false;
		}

		Telemetry.SummaryBanner = FString::Printf(
			TEXT("RTX ACTIVE · %s · %s · Lumen HWRT + Nanite + Streaming 3GB"),
			*Telemetry.GPUAdapterName,
			Telemetry.bDLSSActive ? TEXT("DLSS Quality ON") : TEXT("TSR 78% (DLSS Ready)"));
	}
	else
	{
		// === SIMPLIFIED NON-RTX PERFORMANCE VERSION (GTX / Integrated / Older GPUs) ===
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
		SetCVarInt(TEXT("r.Streaming.PoolSize"), 1536);
		SetCVarInt(TEXT("r.Streaming.LimitPoolSizeToVRAM"), 1);
		SetCVarInt(TEXT("r.Streaming.UsePerTextureBias"), 1);
		SetCVarInt(TEXT("r.Streaming.FullyLoadUsedTextures"), 0);
		SetCVarInt(TEXT("sg.EffectsQuality"), 1);
		SetCVarInt(TEXT("sg.PostProcessQuality"), 1);
		SetCVarInt(TEXT("sg.FoliageQuality"), 1);
		SetCVarInt(TEXT("sg.ShadowQuality"), 1);

		Telemetry.TexturePoolMB = 1536;
		Telemetry.bHardwareRTXActive = false;
		Telemetry.bDLSSActive = false;

		Telemetry.SummaryBanner = FString::Printf(
			TEXT("STANDARD MODE (No RTX) · %s · Fast Shadows + Streaming 1.5GB"),
			*Telemetry.GPUAdapterName);
	}

	UE_LOG(LogChronicles, Display, TEXT("[GraphicsRTX] %s"), *Telemetry.SummaryBanner);
}
