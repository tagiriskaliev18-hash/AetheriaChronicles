// ============================================================================
// GraphicsRTXSubsystem.h — Pre-Launch RTX Hardware Detection, Automatic
// Simplified Non-RTX Fallback, NVIDIA DLSS + Reflex Auto-Launch & Invisible
// Frustum/Proximity Texture Streaming Controller (Crimson Desert Visual Spec)
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ChroniclesTypes.h"
#include "GraphicsRTXSubsystem.generated.h"

UCLASS()
class AETHERIACHRONICLES_API UGraphicsRTXSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Runs hardware RTX capability probe and configures either CrimsonRTXUltra or ClassicPerformance automatically. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	void DetectAndConfigureRenderTier();

	/** Manually switch between CrimsonRTXUltra (if GPU supports RTX) and ClassicPerformance. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	void ApplyRenderTier(EChronicleRenderTier Tier);

	UFUNCTION(BlueprintPure, Category = "Aetheria|Graphics")
	const FGraphicsTelemetry& GetTelemetry() const { return Telemetry; }

	UFUNCTION(BlueprintPure, Category = "Aetheria|Graphics")
	bool IsRTXActive() const { return Telemetry.bHardwareRTXActive; }

	UFUNCTION(BlueprintPure, Category = "Aetheria|Graphics")
	bool IsDLSSActive() const { return Telemetry.bDLSSActive; }

private:
	void SetCVarInt(const TCHAR* Name, int32 Value);
	void SetCVarFloat(const TCHAR* Name, float Value);
	bool HasCVar(const TCHAR* Name) const;
	bool DetectHardwareRTXCaps(FString& OutAdapterName) const;

	UPROPERTY()
	FGraphicsTelemetry Telemetry;
};
