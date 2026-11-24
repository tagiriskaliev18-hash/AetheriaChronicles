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
	virtual void Deinitialize() override;

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

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	void SetDLSSQualityMode(int32 QualityMode); // 0=Performance, 1=Balanced, 2=Quality, 3=DLAA

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	int32 GetDLSSQualityMode() const { return ActiveDLSSQualityMode; }

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	void SetDLSSFrameGenEnabled(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	bool IsDLSSFrameGenEnabled() const { return bDLSSFrameGenActive; }

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	void SetDLSSRayReconstructionEnabled(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	bool IsDLSSRayReconstructionEnabled() const { return bDLSSRayReconstructionActive; }

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	void SetReflexEnabled(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	bool IsReflexEnabled() const { return Telemetry.bReflexActive; }

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Graphics")
	FString GetDLSSModeLabel() const;

private:
	void SetCVarInt(const TCHAR* Name, int32 Value);
	void SetCVarFloat(const TCHAR* Name, float Value);
	bool HasCVar(const TCHAR* Name) const;
	bool DetectHardwareRTXCaps(FString& OutAdapterName) const;
	void UpdateTelemetryBanner();

	UPROPERTY()
	FGraphicsTelemetry Telemetry;

	int32 ActiveDLSSQualityMode = 2; // Quality default
	bool bDLSSFrameGenActive = false;
	bool bDLSSRayReconstructionActive = false;

	FDelegateHandle PostWorldInitHandle;
	bool bRefreshedAfterPluginInit = false;
};
