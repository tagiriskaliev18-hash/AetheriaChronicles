// ============================================================================
// HeroCustomizationComponent.h — Detailed Hero Appearance, Race, Physique/Stature
// (1.10m Small No-Magic -> 3.00m Giant Titan) & Full RPG Attribute System
// Replicated over Multiplayer + SaveGame Persisted + Live 3D Visual Swapping
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChroniclesTypes.h"
#include "HeroCustomizationComponent.generated.h"

class AAetheriaHeroCharacter;
class UChroniclesSaveGame;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeroCustomizationChanged);

/**
 * Replicated Hero Customization, Playable Race, Stature/Physique & RPG Attribute Component.
 * - Controls Gender, Race (6 impactful playable races), Hair, Mustache, Beard, Eyes,
 *   Tattoos, Outfits, Detail Quality, Height (1.10m..3.00m), Musculature, ShoulderWidth.
 * - Controls 6 primary RPG Attributes (STR, END, AGI, INT, SPI, LCK) + unspent points.
 * - Computes live FHeroDerivedStats driving real combat, movement, turn rate, spells,
 *   dragon flight, parry/dodge, and enforcing small-stature (<1.45m) spell lockout.
 */
UCLASS(ClassGroup = (Aetheria), meta = (BlueprintSpawnableComponent))
class AETHERIACHRONICLES_API UHeroCustomizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeroCustomizationComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static constexpr float MIN_HEIGHT_METERS   = 1.10f;
	static constexpr float BASE_HEIGHT_METERS  = 1.85f;
	static constexpr float SMALL_NO_MAGIC_MAX  = 1.45f;
	static constexpr float GIANT_TITAN_MIN     = 2.26f;
	static constexpr float MAX_HEIGHT_METERS   = 3.00f;
	static constexpr int32 MIN_ATTRIBUTE_VAL   = 5;
	static constexpr int32 MAX_ATTRIBUTE_VAL   = 60;
	static constexpr int32 DEFAULT_START_PTS   = 15;

	UPROPERTY(ReplicatedUsing = OnRep_Profile, EditAnywhere, BlueprintReadWrite, Category = "Aetheria|HeroCreator")
	FHeroCustomizationProfile Profile;

	UPROPERTY(BlueprintReadOnly, Category = "Aetheria|HeroCreator")
	FHeroDerivedStats CachedDerivedStats;

	UPROPERTY(BlueprintAssignable, Category = "Aetheria|HeroCreator")
	FOnHeroCustomizationChanged OnCustomizationChanged;

	// ---- Live Computation & Queries ----

	UFUNCTION(BlueprintPure, Category = "Aetheria|HeroCreator")
	static FHeroDerivedStats ComputeDerivedStats(
		const FHeroCustomizationProfile& InProfile,
		int32 HeroLevel = 1,
		bool bIsNightTime = false,
		float CurrentHPRatio = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Aetheria|HeroCreator")
	const FHeroDerivedStats& GetDerivedStats() const { return CachedDerivedStats; }

	/** Recomputes CachedDerivedStats (taking live HeroLevel, Day/Night, and HP% into account) and applies visuals. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void RecalculateAndApply(bool bApplyVisuals = true);

	// ---- Full Profile Mutation API (Auto-replicated from Client -> Server) ----

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetCustomizationProfile(const FHeroCustomizationProfile& NewProfile);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetGender(EHeroGender NewGender);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetRace(EHeroRace NewRace, bool bApplyRaceDefaultStature = true);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetHairStyle(EHeroHairStyle NewStyle);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetHairColorIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetMustacheStyle(EHeroMustacheStyle NewStyle);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetBeardStyle(EHeroBeardStyle NewStyle);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetEyeStyle(EHeroEyeStyle NewStyle);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void AdjustEyeGlow(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetTattooStyle(EHeroTattooStyle NewStyle);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetTattooColorIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetOutfitStyle(EHeroOutfitStyle NewStyle);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetArmorTintIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetHeightMeters(float NewHeightMeters);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetMusculature(float NewMusculature01);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetShoulderWidth(float NewShoulderWidth01);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|HeroCreator")
	void SetDetailQuality(EHeroDetailQuality NewQuality);

	// ---- Attribute Allocation API ----

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Attributes")
	bool ModifyAttribute(EHeroAttribute Attribute, int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Attributes")
	void GrantAttributePoints(int32 PointsToAdd);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Attributes")
	void ResetAttributes();

	UFUNCTION(BlueprintPure, Category = "Aetheria|Attributes")
	int32 GetBaseAttributeValue(EHeroAttribute Attribute) const;

	// ---- Visual & Persistence Helpers ----

	void ApplyVisualsToOwnerHero();
	void ApplyDetailQualityCVarsAndLOD();
	void PopulateSaveGame(UChroniclesSaveGame* SaveObj) const;
	void ApplySaveGame(const UChroniclesSaveGame* SaveObj);

	// ---- UI Display Label & Palette Helpers ----

	static FString GetGenderLabel(EHeroGender Gender);
	static FString GetRaceLabel(EHeroRace Race);
	static FString GetRaceLoreAndPassives(EHeroRace Race);
	static FString GetHairStyleLabel(EHeroHairStyle Style);
	static FString GetMustacheStyleLabel(EHeroMustacheStyle Style);
	static FString GetBeardStyleLabel(EHeroBeardStyle Style);
	static FString GetEyeStyleLabel(EHeroEyeStyle Style);
	static FString GetTattooStyleLabel(EHeroTattooStyle Style);
	static FString GetOutfitStyleLabel(EHeroOutfitStyle Style);
	static FString GetDetailQualityLabel(EHeroDetailQuality Quality);
	static FString GetAttributeLabel(EHeroAttribute Attr);
	static FString GetAttributeEffectHint(EHeroAttribute Attr);

	static FLinearColor GetHairColor(int32 Index);
	static FString GetHairColorName(int32 Index);
	static FLinearColor GetEyeColor(EHeroEyeStyle Style);
	static FLinearColor GetTattooColor(int32 Index);
	static FString GetTattooColorName(int32 Index);
	static FLinearColor GetArmorTintColor(int32 Index);
	static FString GetArmorTintName(int32 Index);
	static FLinearColor GetRaceSkinTint(EHeroRace Race);

	static constexpr int32 NUM_HAIR_COLORS   = 8;
	static constexpr int32 NUM_TATTOO_COLORS = 6;
	static constexpr int32 NUM_ARMOR_TINTS   = 6;

protected:
	UFUNCTION()
	void OnRep_Profile();

	UFUNCTION(Server, Reliable)
	void ServerSetProfile(const FHeroCustomizationProfile& NewProfile);

private:
	void CommitProfileChange(const FHeroCustomizationProfile& SanitizedProfile);
	static FHeroCustomizationProfile SanitizeProfile(const FHeroCustomizationProfile& InProfile);
	void EnsureAttachmentComponents(AAetheriaHeroCharacter* Hero);
	void UpdateHeadAndFacialAttachments(AAetheriaHeroCharacter* Hero);
	void UpdateTattooAndMaterialTints(AAetheriaHeroCharacter* Hero);
	void UpdateHeroMeshForGenderAndOutfit(AAetheriaHeroCharacter* Hero);
	FName ResolveHeadBoneName(const USkeletalMeshComponent* MeshComp) const;
	FName ResolveChestBoneName(const USkeletalMeshComponent* MeshComp) const;

	// Live 3D Cosmetic Socket Attachments (GC-safe UPROPERTY TObjectPtr!)
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> HairAttachMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> HairSecondaryMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MustacheLeftMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MustacheRightMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BeardMainMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BeardLowerMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> EyeLeftMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> EyeRightMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> TattooBrowMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> TattooChestMesh;

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> HairMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> EyeMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> TattooMID;

	FString LastAppliedSkeletalMeshPath;
	bool bApplyingVisualsGuard = false;
};
