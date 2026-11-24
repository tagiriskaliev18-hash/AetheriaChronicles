// ============================================================================
// HeroCustomizationComponent.cpp — Detailed Hero Appearance, Race, Physique/Stature
// (1.10m Small No-Magic -> 3.00m Giant Titan) & Full RPG Attribute System
// ============================================================================

#include "HeroCustomizationComponent.h"

#include "AudioAndStyleSubsystem.h"
#include "ChroniclesGameFramework.h"
#include "HeroDragonBoss.h"
#include "SkillTreeComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "MaterialTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "Misc/PackageName.h"
#include "Net/UnrealNetwork.h"

namespace HeroCustomizationUtil
{
	static constexpr TCHAR PathSphere[]      = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	static constexpr TCHAR PathCylinder[]    = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	static constexpr TCHAR PathCone[]        = TEXT("/Engine/BasicShapes/Cone.Cone");
	static constexpr TCHAR PathCube[]        = TEXT("/Engine/BasicShapes/Cube.Cube");
	static constexpr TCHAR PathMatBasic[]    = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	static constexpr TCHAR PathMatEmissive[] = TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");

	// Male Paragon Rigs (Verified Staged Paths)
	static constexpr TCHAR PathKwangGDC[]         = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Meshes/Kwang_GDC.Kwang_GDC");
	static constexpr TCHAR PathKwangManbun[]      = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Skins/Tier2/Kwang_Manban/Meshes/KwangManbun.KwangManbun");
	static constexpr TCHAR PathKwangAlbino[]      = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Meshes/KwangAlbino.KwangAlbino");
	static constexpr TCHAR PathKwangFrostwalker[] = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Skins/Tier2/Kwang_Manbun_Zombie/Meshes/KwangFrostwalker.KwangFrostwalker");
	static constexpr TCHAR PathKwangSunrise[]     = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Meshes/KwangSunrise.KwangSunrise");
	static constexpr TCHAR PathKwangRosewood[]    = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Meshes/KwangRosewood.KwangRosewood");
	static constexpr TCHAR PathKwangABP[]         = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Kwang_AnimBlueprint.Kwang_AnimBlueprint_C");
	static constexpr TCHAR PathKwangAttackA[]     = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_A_Slow_Montage.PrimaryAttack_A_Slow_Montage");
	static constexpr TCHAR PathKwangAttackB[]     = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_B_Slow_Montage.PrimaryAttack_B_Slow_Montage");
	static constexpr TCHAR PathKwangAttackC[]     = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_C_Slow_Montage.PrimaryAttack_C_Slow_Montage");
	static constexpr TCHAR PathKwangAttackD[]     = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_D_Slow_Montage.PrimaryAttack_D_Slow_Montage");
	static constexpr TCHAR PathKwangHitReact[]    = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Hitreact_Fwd.Hitreact_Fwd");
	static constexpr TCHAR PathKwangDeath[]       = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Death_Bwd.Death_Bwd");

	static constexpr TCHAR PathGideonBase[]       = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Meshes/Gideon.Gideon");
	static constexpr TCHAR PathGideonInquisitor[] = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Skins/Inquisitor/Meshes/Gideon_Inquisitor.Gideon_Inquisitor");
	static constexpr TCHAR PathGideonMephisto[]   = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Skins/Mephisto/Meshes/Gideon_Mephisto.Gideon_Mephisto");
	static constexpr TCHAR PathGideonTough[]      = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Skins/Tough/Meshes/Gideon_Tough.Gideon_Tough");
	static constexpr TCHAR PathGideonABP[]        = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Gideon_AnimBlueprint.Gideon_AnimBlueprint_C");
	static constexpr TCHAR PathGideonAttackA[]    = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_A_Medium_Montage.Primary_Attack_A_Medium_Montage");
	static constexpr TCHAR PathGideonAttackB[]    = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_B_Medium_Montage.Primary_Attack_B_Medium_Montage");
	static constexpr TCHAR PathGideonAttackC[]    = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_C_Medium_Montage.Primary_Attack_C_Medium_Montage");
	static constexpr TCHAR PathGideonHitReact[]   = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathGideonDeath[]      = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Death_Fwd.Death_Fwd");

	static constexpr TCHAR PathKhaimeraBase[]     = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Meshes/Khaimera.Khaimera");
	static constexpr TCHAR PathKhaimeraGruxPelt[] = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Skins/Tier2/GruxPelt/Meshes/Khaimera_GruxPelt.Khaimera_GruxPelt");
	static constexpr TCHAR PathKhaimeraABP[]      = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Khaimera_AnimBlueprint.Khaimera_AnimBlueprint_C");
	static constexpr TCHAR PathKhaimeraAttackA[]  = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_A_Montage.Melee_A_Montage");
	static constexpr TCHAR PathKhaimeraAttackB[]  = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_B_Montage.Melee_B_Montage");
	static constexpr TCHAR PathKhaimeraAttackC[]  = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_C_Montage.Melee_C_Montage");
	static constexpr TCHAR PathKhaimeraHitReact[] = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathKhaimeraDeath[]    = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Death_A.Death_A");

	static constexpr TCHAR PathGruxBase[]         = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Meshes/Grux.Grux");
	static constexpr TCHAR PathGruxWarchief[]     = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Skins/Tier_1/War_Chief/Meshes/GruxWarchief.GruxWarchief");
	static constexpr TCHAR PathGruxChestplate[]   = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Skins/Tier_1_5/Chestplate/Meshes/GruxChestplate.GruxChestplate");
	static constexpr TCHAR PathGruxHalloween[]    = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Skins/Tier_1/Halloween/Meshes/GruxHalloween.GruxHalloween");
	static constexpr TCHAR PathGruxABP[]          = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Grux_AnimBlueprint.Grux_AnimBlueprint_C");
	static constexpr TCHAR PathGruxAttackA[]      = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/PrimaryAttack_LA_Montage.PrimaryAttack_LA_Montage");
	static constexpr TCHAR PathGruxAttackB[]      = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/PrimaryAttack_RA_Montage.PrimaryAttack_RA_Montage");
	static constexpr TCHAR PathGruxAttackC[]      = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/PrimaryAttack_FourStrikes_Montage.PrimaryAttack_FourStrikes_Montage");
	static constexpr TCHAR PathGruxHitReact[]     = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathGruxDeath[]        = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/Death_A.Death_A");

	// Female Paragon Rigs (Serath, Kallari, Aurora & Morigesh — Verified Staged Paths)
	static constexpr TCHAR PathSerathBase[]       = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Meshes/Serath.Serath");
	static constexpr TCHAR PathSerathGold[]       = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Skins/Tier1/Serath_Gold/Meshes/SerathGold.SerathGold");
	static constexpr TCHAR PathSerathFrost[]      = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Skins/Tier1/Serath_Frost/Meshes/SerathFrost.SerathFrost");
	static constexpr TCHAR PathSerathValentine[]  = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Skins/Tier1/Serath_Val/Meshes/SerathValentine.SerathValentine");
	static constexpr TCHAR PathSerathFuture[]     = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Skins/Tier1/Serath_Future/Meshes/Serath_Future.Serath_Future");
	static constexpr TCHAR PathSerathABP[]        = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Serath_AnimBlueprint.Serath_AnimBlueprint_C");
	static constexpr TCHAR PathSerathAttackA[]    = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Animations/Primary_Attack_A_Medium_Montage.Primary_Attack_A_Medium_Montage");
	static constexpr TCHAR PathSerathAttackB[]    = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Animations/Primary_Attack_B_Medium_Montage.Primary_Attack_B_Medium_Montage");
	static constexpr TCHAR PathSerathAttackC[]    = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Animations/Primary_Attack_C_Medium_120fps_Montage.Primary_Attack_C_Medium_120fps_Montage");
	static constexpr TCHAR PathSerathAttackD[]    = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Animations/Primary_Attack_D_Medium_Montage.Primary_Attack_D_Medium_Montage");
	static constexpr TCHAR PathSerathHitReact[]   = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathSerathDeath[]      = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Animations/Death_Fwd.Death_Fwd");

	static constexpr TCHAR PathKallariBase[]        = TEXT("/Game/ParagonKallari/Characters/Heroes/Kallari/Meshes/Kallari.Kallari");
	static constexpr TCHAR PathKallariDeathLotus[]  = TEXT("/Game/ParagonKallari/Characters/Heroes/Kallari/Skins/DeathLotus/Meshes/Kallari_DeathLotus.Kallari_DeathLotus");
	static constexpr TCHAR PathKallariRedDeath[]    = TEXT("/Game/ParagonKallari/Characters/Heroes/Kallari/Skins/RedDeath/Meshes/Kallari_RedDeath.Kallari_RedDeath");
	static constexpr TCHAR PathKallariRogue[]       = TEXT("/Game/ParagonKallari/Characters/Heroes/Kallari/Skins/Rogue/Meshes/Kallari_Rogue.Kallari_Rogue");
	static constexpr TCHAR PathKallariABP[]         = TEXT("/Game/ParagonKallari/Characters/Heroes/Kallari/Kallari_AnimBlueprint.Kallari_AnimBlueprint_C");
	static constexpr TCHAR PathKallariAttackA[]     = TEXT("/Game/ParagonKallari/Characters/Heroes/Kallari/Animations/Attack_Melee_A_Montage.Attack_Melee_A_Montage");
	static constexpr TCHAR PathKallariAttackB[]     = TEXT("/Game/ParagonKallari/Characters/Heroes/Kallari/Animations/Attack_Melee_B_Montage.Attack_Melee_B_Montage");
	static constexpr TCHAR PathKallariAttackC[]     = TEXT("/Game/ParagonKallari/Characters/Heroes/Kallari/Animations/Attack_Melee_C_Montage.Attack_Melee_C_Montage");

	static constexpr TCHAR PathAuroraBase[]           = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora.Aurora");
	static constexpr TCHAR PathAuroraFrozenHearth[]   = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Skins/FrozenHearth/Meshes/Aurora_FrozenHearth.Aurora_FrozenHearth");
	static constexpr TCHAR PathAuroraGlacialEmpress[] = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Skins/GlacialEmpress/Meshes/Aurora_GlacialEmpress.Aurora_GlacialEmpress");
	static constexpr TCHAR PathAuroraABP[]            = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Aurora_AnimBlueprint.Aurora_AnimBlueprint_C");
	static constexpr TCHAR PathAuroraAttackA[]        = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_A_Montage.Primary_Attack_A_Montage");
	static constexpr TCHAR PathAuroraAttackB[]        = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_B_Montage.Primary_Attack_B_Montage");
	static constexpr TCHAR PathAuroraAttackC[]        = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_C_Montage.Primary_Attack_C_Montage");
	static constexpr TCHAR PathAuroraHitReact[]       = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathAuroraDeath[]          = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Death.Death");

	static constexpr TCHAR PathMorigeshBase[]       = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Meshes/Morigesh.Morigesh");
	static constexpr TCHAR PathMorigeshDarkHeart[]  = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Skins/DarkHeart/Meshes/Morigesh_DarkHeart.Morigesh_DarkHeart");
	static constexpr TCHAR PathMorigeshMystic[]     = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Skins/NorthernMystic/Meshes/Morigesh_NorthernMystic.Morigesh_NorthernMystic");
	static constexpr TCHAR PathMorigeshABP[]        = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Morigesh_AnimBlueprint.Morigesh_AnimBlueprint_C");
	static constexpr TCHAR PathMorigeshAttackA[]    = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/PrimaryAttack_A_Slow_Montage.PrimaryAttack_A_Slow_Montage");
	static constexpr TCHAR PathMorigeshHitReact[]   = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathMorigeshDeath[]      = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/Death.Death");

	static FLinearColor ClampGroundedTint(const FLinearColor& InColor, float MaxLuminance = 0.88f)
	{
		FLinearColor Clamped(
			FMath::Clamp(InColor.R, 0.0f, MaxLuminance),
			FMath::Clamp(InColor.G, 0.0f, MaxLuminance),
			FMath::Clamp(InColor.B, 0.0f, MaxLuminance),
			InColor.A);
		const float Lum = Clamped.GetLuminance();
		if (Lum > MaxLuminance && Lum > KINDA_SMALL_NUMBER)
		{
			const float Scale = MaxLuminance / Lum;
			Clamped.R *= Scale;
			Clamped.G *= Scale;
			Clamped.B *= Scale;
		}
		return Clamped;
	}

	static void ResolveRigAnimationsForMeshPath(
		const FString& MeshPath,
		FString& OutABP,
		TArray<FString>& OutAttacks,
		FString& OutHitReact,
		FString& OutDeath)
	{
		// Leave OutABP empty for Paragon rigs: UE 5.8 fails compiling legacy Paragon *_AnimBlueprint graphs,
		// so AAetheriaHeroCharacter drives Paragon heroes cleanly via AnimationSingleNode (Idle + Jog_Fwd + combat clips).
		OutABP.Reset();
		OutAttacks.Reset();
		OutHitReact.Reset();
		OutDeath.Reset();

		if (MeshPath.Contains(TEXT("ParagonKwang")))
		{
			OutAttacks = { FString(PathKwangAttackA), FString(PathKwangAttackB), FString(PathKwangAttackC), FString(PathKwangAttackD) };
			OutHitReact = PathKwangHitReact;
			OutDeath = PathKwangDeath;
		}
		else if (MeshPath.Contains(TEXT("ParagonGideon")))
		{
			OutAttacks = { FString(PathGideonAttackA), FString(PathGideonAttackB), FString(PathGideonAttackC) };
			OutHitReact = PathGideonHitReact;
			OutDeath = PathGideonDeath;
		}
		else if (MeshPath.Contains(TEXT("ParagonKhaimera")))
		{
			OutAttacks = { FString(PathKhaimeraAttackA), FString(PathKhaimeraAttackB), FString(PathKhaimeraAttackC) };
			OutHitReact = PathKhaimeraHitReact;
			OutDeath = PathKhaimeraDeath;
		}
		else if (MeshPath.Contains(TEXT("ParagonGrux")))
		{
			OutAttacks = { FString(PathGruxAttackA), FString(PathGruxAttackB), FString(PathGruxAttackC) };
			OutHitReact = PathGruxHitReact;
			OutDeath = PathGruxDeath;
		}
		else if (MeshPath.Contains(TEXT("ParagonSerath")))
		{
			OutAttacks = { FString(PathSerathAttackA), FString(PathSerathAttackB), FString(PathSerathAttackC), FString(PathSerathAttackD) };
			OutHitReact = PathSerathHitReact;
			OutDeath = PathSerathDeath;
		}
		else if (MeshPath.Contains(TEXT("ParagonKallari")))
		{
			OutAttacks = { FString(PathKallariAttackA), FString(PathKallariAttackB), FString(PathKallariAttackC) };
		}
		else if (MeshPath.Contains(TEXT("ParagonAurora")))
		{
			OutAttacks = { FString(PathAuroraAttackA), FString(PathAuroraAttackB), FString(PathAuroraAttackC) };
			OutHitReact = PathAuroraHitReact;
			OutDeath = PathAuroraDeath;
		}
		else if (MeshPath.Contains(TEXT("ParagonMorigesh")))
		{
			OutAttacks = { FString(PathMorigeshAttackA) };
			OutHitReact = PathMorigeshHitReact;
			OutDeath = PathMorigeshDeath;
		}
	}

	static bool DoesObjectPackageExist(const FString& ObjectPath)
	{
		if (ObjectPath.IsEmpty())
		{
			return false;
		}
		const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
		return !PackageName.IsEmpty() && FPackageName::DoesPackageExist(PackageName);
	}

	static FString PickFirstExistingPath(const TArray<const TCHAR*>& Candidates)
	{
		for (const TCHAR* Candidate : Candidates)
		{
			if (Candidate && DoesObjectPackageExist(FString(Candidate)))
			{
				return FString(Candidate);
			}
		}
		return FString();
	}

	static UStaticMesh* LoadShapeMesh(const TCHAR* ShapePath)
	{
		return LoadObject<UStaticMesh>(nullptr, ShapePath);
	}

	static UMaterialInstanceDynamic* CreateCosmeticMID(
		UObject* Outer,
		const FLinearColor& Color,
		bool bEmissive,
		float Metallic,
		float Roughness)
	{
		const TCHAR* MatPath = bEmissive ? PathMatEmissive : PathMatBasic;
		UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, MatPath);
		if (!BaseMat)
		{
			BaseMat = LoadObject<UMaterialInterface>(nullptr, PathMatBasic);
		}
		if (!BaseMat)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, Outer);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
			MID->SetVectorParameterValue(TEXT("EmissiveColor"), Color);
			MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
			MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		}
		return MID;
	}

	static void UpdateCosmeticMID(
		UMaterialInstanceDynamic* MID,
		const FLinearColor& Color,
		float Metallic,
		float Roughness)
	{
		if (!MID)
		{
			return;
		}
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Color);
		MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	}

	static UStaticMeshComponent* CreateCosmeticMesh(AActor* OwnerActor, const FName& CompName)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(OwnerActor, CompName);
		if (!Comp)
		{
			return nullptr;
		}
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
		Comp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCastShadow(false);
		Comp->bDisallowNanite = true;
		Comp->SetVisibility(false);
		Comp->RegisterComponent();
		return Comp;
	}

	static void ConfigureAttachment(
		UStaticMeshComponent* Comp,
		USkeletalMeshComponent* ParentSkel,
		const FName& BoneName,
		UStaticMesh* MeshAsset,
		UMaterialInterface* Mat,
		const FVector& RelLoc,
		const FRotator& RelRot,
		const FVector& RelScale,
		bool bVisible)
	{
		if (!Comp || !ParentSkel)
		{
			return;
		}

		if (MeshAsset && Comp->GetStaticMesh() != MeshAsset)
		{
			Comp->SetStaticMesh(MeshAsset);
		}
		if (Mat && Comp->GetMaterial(0) != Mat)
		{
			Comp->SetMaterial(0, Mat);
		}

		Comp->SetVisibility(bVisible);
		if (!bVisible)
		{
			return;
		}

		if (Comp->GetAttachParent() != ParentSkel || Comp->GetAttachSocketName() != BoneName)
		{
			Comp->AttachToComponent(ParentSkel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, BoneName);
		}
		Comp->SetRelativeLocationAndRotation(RelLoc, RelRot);
		Comp->SetRelativeScale3D(RelScale);
	}
}

// ============================================================================
// UHeroCustomizationComponent Lifecycle & Replication
// ============================================================================

UHeroCustomizationComponent::UHeroCustomizationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	Profile = SanitizeProfile(Profile);
	CachedDerivedStats = ComputeDerivedStats(Profile, 1, false, 1.0f);
}

void UHeroCustomizationComponent::BeginPlay()
{
	Super::BeginPlay();

	Profile = SanitizeProfile(Profile);
	RecalculateAndApply(true);

	if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(GetOwner()))
	{
		Hero->RefreshAllHeroStats();
	}
}

void UHeroCustomizationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHeroCustomizationComponent, Profile);
}

// ============================================================================
// 1. ComputeDerivedStats — Races, RPG Attributes, Stature Physics & Clamps
// ============================================================================

FHeroDerivedStats UHeroCustomizationComponent::ComputeDerivedStats(
	const FHeroCustomizationProfile& InProfile,
	int32 HeroLevel,
	bool bIsNightTime,
	float CurrentHPRatio)
{
	const FHeroCustomizationProfile P = SanitizeProfile(InProfile);
	const int32 ClampedLevel = FMath::Max(1, HeroLevel);
	const float HPRatio = FMath::Clamp(CurrentHPRatio, 0.0f, 1.0f);

	FHeroDerivedStats Stats;

	// Subtle baseline hero level scaling (0 extra at level 1)
	const int32 LevelDelta = ClampedLevel - 1;
	Stats.MaxHP += static_cast<float>(LevelDelta) * 8.f;
	Stats.MaxMana += static_cast<float>(LevelDelta) * 6.f;
	Stats.MaxStamina += static_cast<float>(LevelDelta) * 4.f;

	// Base RPG attributes before racial offsets
	int32 EffSTR = P.Attributes.Strength;
	int32 EffEND = P.Attributes.Endurance;
	int32 EffAGI = P.Attributes.Agility;
	int32 EffINT = P.Attributes.Intelligence;
	int32 EffSPI = P.Attributes.Spirit;
	int32 EffLCK = P.Attributes.Fortune;

	// ---- Playable Race Base Attribute Modifiers & Gameplay Passives ----
	switch (P.Race)
	{
	case EHeroRace::Human:
		EffSTR += 2;
		EffEND += 2;
		EffAGI += 2;
		EffINT += 2;
		EffSPI += 2;
		EffLCK += 2;
		Stats.XPAndGoldMultiplier *= 1.20f;
		Stats.ParryWindowBonusSec += 0.045f;
		Stats.BlockStaminaDrainMult *= 0.85f;
		Stats.RacePassiveSummary = TEXT("Valenarian Versatility: +2 All Attributes · +20% XP & Gold · +45ms Parry Window · -15% Block Drain");
		break;

	case EHeroRace::HighElf:
		EffINT += 5;
		EffSPI += 4;
		EffAGI += 3;
		EffEND -= 2;
		Stats.ManaCostMultiplier *= 0.80f;
		Stats.SpellCooldownMultiplier *= 0.85f;
		Stats.MoveSpeedCmPerSec *= 1.12f;
		Stats.AttackSpeedMultiplier *= 1.10f;
		Stats.MaxHP *= 0.88f;
		Stats.MapRevealMultiplier *= 1.35f;
		Stats.RacePassiveSummary = TEXT("Aetherian Arcana: +5 INT +4 SPI +3 AGI -2 END · -20% Mana · -15% CD · +12% Move · +10% AtkSpd · -12% HP · +35% Map");
		break;

	case EHeroRace::HighlandNord:
		EffSTR += 6;
		EffEND += 5;
		EffINT -= 2;
		EffAGI -= 1;
		Stats.MaxHP *= 1.25f;
		Stats.bColdSlowImmune = true;
		if (HPRatio <= 0.45f)
		{
			Stats.bBerserkLowHPActive = true;
			Stats.MeleeDamageMultiplier *= 1.30f;
			Stats.StaggerMultiplier *= 1.30f;
		}
		Stats.RacePassiveSummary = TEXT("Nordic Bloodrage: +6 STR +5 END -2 INT -1 AGI · +25% HP · Cold Slow Immune · +30% Melee & Stagger at <=45% HP");
		break;

	case EHeroRace::AshDraconid:
		EffSTR += 4;
		EffSPI += 4;
		EffEND += 3;
		EffINT += 2;
		EffAGI -= 1;
		Stats.DragonSpeedMultiplier *= 1.25f;
		Stats.DragonFireMultiplier *= 1.35f;
		Stats.DragonFireDrainMultiplier *= 0.75f;
		Stats.FireResistance += 0.40f;
		Stats.ExortBonusMultiplier *= 1.25f;
		Stats.RacePassiveSummary = TEXT("Draconic Kinship: +4 STR +4 SPI +3 END +2 INT -1 AGI · +25% Drake Speed · +35% Breath · -25% Breath Drain · +40% Fire Res · +25% Exort");
		break;

	case EHeroRace::ShadowKin:
		EffAGI += 6;
		EffLCK += 4;
		EffINT += 2;
		EffEND -= 3;
		Stats.DodgeDistanceMultiplier *= 1.25f;
		Stats.DodgeIFrameBonusSec += 0.08f;
		Stats.DodgeStaminaCostMultiplier *= 0.65f;
		Stats.CritChance += 0.12f;
		if (bIsNightTime)
		{
			Stats.bNightShadowCritActive = true;
			Stats.CritDamageMultiplier += 0.40f;
		}
		Stats.RacePassiveSummary = TEXT("Umbral Veil: +6 AGI +4 LCK +2 INT -3 END · +25% Dodge · +80ms i-Frames · -35% Dodge Cost · +12% Crit · +40% Night Crit Dmg");
		break;

	case EHeroRace::Ironborn:
		EffEND += 5;
		EffSTR += 4;
		EffLCK += 3;
		EffAGI -= 3;
		Stats.ArmorReduction += 0.18f;
		Stats.KnockbackResistance += 0.50f;
		Stats.DestructionDamageMultiplier *= 1.45f;
		Stats.ParryStaggerBonusMult *= 2.0f;
		Stats.XPAndGoldMultiplier *= 1.15f;
		Stats.RacePassiveSummary = TEXT("Runesmith Bastion: +5 END +4 STR +3 LCK -3 AGI · +18% Armor · +50% Poise · +45% Destruction · 2.0x Parry Stagger · +15% XP/Gold");
		break;

	default:
		break;
	}

	Stats.EffStrength     = FMath::Max(1, EffSTR);
	Stats.EffEndurance    = FMath::Max(1, EffEND);
	Stats.EffAgility      = FMath::Max(1, EffAGI);
	Stats.EffIntelligence = FMath::Max(1, EffINT);
	Stats.EffSpirit       = FMath::Max(1, EffSPI);
	Stats.EffFortune      = FMath::Max(1, EffLCK);

	// ---- Primary Attributes Math (Delta from baseline 10) ----
	const int32 DeltaSTR = Stats.EffStrength - 10;
	Stats.MeleeDamageMultiplier += static_cast<float>(DeltaSTR) * 0.06f;
	Stats.MaxHP                 += static_cast<float>(DeltaSTR) * 12.f;
	Stats.StaggerMultiplier     += static_cast<float>(DeltaSTR) * 0.04f;

	const int32 DeltaEND = Stats.EffEndurance - 10;
	Stats.MaxStamina            += static_cast<float>(DeltaEND) * 10.f;
	Stats.StaminaRegenPerSec    += static_cast<float>(DeltaEND) * 1.5f;
	Stats.MaxHP                 += static_cast<float>(DeltaEND) * 8.f;
	Stats.ArmorReduction        += static_cast<float>(FMath::Max(0, DeltaEND)) * 0.012f;
	Stats.BlockStaminaDrainMult *= FMath::Clamp(1.f - static_cast<float>(DeltaEND) * 0.015f, 0.45f, 1.35f);

	const int32 DeltaAGI = Stats.EffAgility - 10;
	Stats.AttackSpeedMultiplier += static_cast<float>(DeltaAGI) * 0.028f;
	Stats.MoveSpeedCmPerSec     += static_cast<float>(DeltaAGI) * 14.f;
	Stats.CritChance            += static_cast<float>(DeltaAGI) * 0.015f;
	Stats.DodgeIFrameBonusSec   += static_cast<float>(FMath::Max(0, DeltaAGI)) * 0.003f;

	const int32 DeltaINT = Stats.EffIntelligence - 10;
	Stats.SpellPowerMultiplier  += static_cast<float>(DeltaINT) * 0.065f;
	Stats.MaxMana               += static_cast<float>(DeltaINT) * 14.f;
	Stats.SpellCooldownMultiplier *= FMath::Clamp(1.f - static_cast<float>(DeltaINT) * 0.015f, 0.55f, 1.25f);

	const int32 DeltaSPI = Stats.EffSpirit - 10;
	Stats.ManaRegenPerSec       += static_cast<float>(DeltaSPI) * 1.4f;
	Stats.HPRegenPerSec         += FMath::Max(0.f, static_cast<float>(DeltaSPI) * 0.5f);
	Stats.DragonFireMultiplier  += static_cast<float>(DeltaSPI) * 0.04f;

	const int32 DeltaLCK = Stats.EffFortune - 10;
	Stats.CritDamageMultiplier  += static_cast<float>(DeltaLCK) * 0.04f;
	Stats.XPAndGoldMultiplier   += static_cast<float>(DeltaLCK) * 0.03f;
	Stats.ParryWindowBonusSec   += static_cast<float>(FMath::Max(0, DeltaLCK)) * 0.0025f;

	// ---- Stature / Height Physics (1.10m Small No-Magic -> 3.00m Giant Titan) ----
	const float Height = FMath::Clamp(P.HeightMeters, MIN_HEIGHT_METERS, MAX_HEIGHT_METERS);
	if (Height < SMALL_NO_MAGIC_MAX)
	{
		Stats.StatureClass = EHeroStatureClass::SmallNoMagic;
		Stats.bCanCastSpells = false; // Small stature (<1.45m) cannot channel primal magic

		const float SmallT = FMath::Clamp((SMALL_NO_MAGIC_MAX - Height) / (SMALL_NO_MAGIC_MAX - MIN_HEIGHT_METERS), 0.f, 1.f);
		Stats.MeleeDamageMultiplier      *= FMath::Lerp(0.72f, 0.52f, SmallT);
		Stats.MeleeReachMultiplier       *= FMath::Lerp(0.84f, 0.72f, SmallT);
		Stats.MaxHP                      *= FMath::Lerp(0.85f, 0.70f, SmallT);
		Stats.MoveSpeedCmPerSec          *= FMath::Lerp(1.14f, 1.26f, SmallT);
		Stats.TurnRateDegPerSec           = FMath::Lerp(720.f, 860.f, SmallT);
		Stats.DodgeStaminaCostMultiplier *= 0.72f;
		Stats.DodgeIFrameBonusSec        += 0.06f;

		Stats.StatureSummary = FString::Printf(
			TEXT("%.2fm · Small Stature (No Magic · +%.0f%% Speed · Rapid Turn & Dodge)"),
			Height,
			(FMath::Lerp(1.14f, 1.26f, SmallT) - 1.f) * 100.f);
	}
	else if (Height > 2.25f)
	{
		Stats.StatureClass = EHeroStatureClass::GiantTitan;
		Stats.bCanCastSpells = true;

		const float GiantT = FMath::Clamp((Height - 2.25f) / (MAX_HEIGHT_METERS - 2.25f), 0.f, 1.f);
		Stats.MeleeDamageMultiplier      *= FMath::Lerp(1.25f, 1.78f, GiantT);
		Stats.MeleeReachMultiplier       *= FMath::Lerp(1.18f, 1.48f, GiantT);
		Stats.StaggerMultiplier          *= FMath::Lerp(1.30f, 1.90f, GiantT);
		Stats.MaxHP                      *= FMath::Lerp(1.22f, 1.68f, GiantT);
		Stats.MoveSpeedCmPerSec          *= FMath::Lerp(0.88f, 0.72f, GiantT);
		Stats.AttackSpeedMultiplier      *= FMath::Lerp(0.88f, 0.70f, GiantT);
		Stats.TurnRateDegPerSec           = FMath::Lerp(340.f, 185.f, GiantT);
		Stats.AccelMultiplier             = FMath::Lerp(0.76f, 0.48f, GiantT);
		Stats.DodgeDistanceMultiplier    *= FMath::Lerp(0.86f, 0.72f, GiantT);
		Stats.DodgeStaminaCostMultiplier *= FMath::Lerp(1.22f, 1.48f, GiantT);
		Stats.SpellCooldownMultiplier    *= FMath::Lerp(1.06f, 1.18f, GiantT);

		Stats.StatureSummary = FString::Printf(
			TEXT("%.2fm · Giant Titan (+%.0f%% Melee · +%.0f%% Reach · Slow Heavy Turn)"),
			Height,
			(FMath::Lerp(1.25f, 1.78f, GiantT) - 1.f) * 100.f,
			(FMath::Lerp(1.18f, 1.48f, GiantT) - 1.f) * 100.f);
	}
	else
	{
		Stats.StatureClass = EHeroStatureClass::StandardBalanced;
		Stats.bCanCastSpells = true;

		const float NormDelta = (Height - BASE_HEIGHT_METERS) / 0.40f; // [-1 .. +1] across 1.45m..2.25m
		Stats.MeleeDamageMultiplier *= (1.0f + NormDelta * 0.08f);
		Stats.MeleeReachMultiplier  *= (1.0f + NormDelta * 0.07f);
		Stats.MaxHP                 *= (1.0f + NormDelta * 0.06f);
		Stats.MoveSpeedCmPerSec     *= (1.0f - NormDelta * 0.04f);
		Stats.TurnRateDegPerSec      = FMath::Clamp(560.f - NormDelta * 90.f, 430.f, 680.f);
		Stats.AccelMultiplier        = FMath::Clamp(1.0f - NormDelta * 0.08f, 0.88f, 1.12f);

		Stats.StatureSummary = FString::Printf(
			TEXT("%.2fm · Standard Stature (Full Magic & Balanced Combat)"),
			Height);
	}

	// ---- Musculature & Shoulder Breadth Physics ----
	const float MuscDelta = FMath::Clamp(P.Musculature, 0.f, 1.f) - 0.5f;
	Stats.MeleeDamageMultiplier *= (1.f + MuscDelta * 0.28f);
	Stats.MaxHP                 *= (1.f + MuscDelta * 0.22f);
	Stats.MaxStamina            *= (1.f + MuscDelta * 0.14f);
	Stats.MoveSpeedCmPerSec     *= (1.f - MuscDelta * 0.08f);

	const float ShoulderDelta = FMath::Clamp(P.ShoulderWidth, 0.f, 1.f) - 0.5f;
	Stats.StaggerMultiplier     *= (1.f + ShoulderDelta * 0.10f);

	// ---- Safe Final Clamps ----
	Stats.MaxHP                      = FMath::Max(60.f, Stats.MaxHP);
	Stats.MaxStamina                 = FMath::Max(50.f, Stats.MaxStamina);
	Stats.MaxMana                    = FMath::Max(40.f, Stats.MaxMana);
	Stats.HPRegenPerSec              = FMath::Max(0.f, Stats.HPRegenPerSec);
	Stats.StaminaRegenPerSec         = FMath::Max(8.f, Stats.StaminaRegenPerSec);
	Stats.ManaRegenPerSec            = FMath::Max(2.f, Stats.ManaRegenPerSec);
	Stats.MeleeDamageMultiplier      = FMath::Clamp(Stats.MeleeDamageMultiplier, 0.35f, 4.00f);
	Stats.MeleeReachMultiplier       = FMath::Clamp(Stats.MeleeReachMultiplier, 0.65f, 1.75f);
	Stats.AttackSpeedMultiplier      = FMath::Clamp(Stats.AttackSpeedMultiplier, 0.55f, 1.85f);
	Stats.MoveSpeedCmPerSec          = FMath::Clamp(Stats.MoveSpeedCmPerSec, 340.f, 980.f);
	Stats.TurnRateDegPerSec          = FMath::Clamp(Stats.TurnRateDegPerSec, 160.f, 920.f);
	Stats.AccelMultiplier            = FMath::Clamp(Stats.AccelMultiplier, 0.40f, 1.40f);
	Stats.StaggerMultiplier          = FMath::Clamp(Stats.StaggerMultiplier, 0.50f, 3.50f);
	Stats.DestructionDamageMultiplier= FMath::Clamp(Stats.DestructionDamageMultiplier, 0.50f, 3.50f);
	Stats.ArmorReduction             = FMath::Clamp(Stats.ArmorReduction, 0.0f, 0.60f);
	Stats.FireResistance             = FMath::Clamp(Stats.FireResistance, 0.0f, 0.75f);
	Stats.KnockbackResistance        = FMath::Clamp(Stats.KnockbackResistance, 0.0f, 0.85f);
	Stats.CritChance                 = FMath::Clamp(Stats.CritChance, 0.01f, 0.75f);
	Stats.CritDamageMultiplier       = FMath::Clamp(Stats.CritDamageMultiplier, 1.20f, 3.50f);
	Stats.SpellCooldownMultiplier    = FMath::Clamp(Stats.SpellCooldownMultiplier, 0.45f, 1.50f);
	Stats.ManaCostMultiplier         = FMath::Clamp(Stats.ManaCostMultiplier, 0.50f, 1.40f);
	Stats.DodgeDistanceMultiplier    = FMath::Clamp(Stats.DodgeDistanceMultiplier, 0.60f, 1.75f);
	Stats.DodgeStaminaCostMultiplier = FMath::Clamp(Stats.DodgeStaminaCostMultiplier, 0.45f, 1.75f);

	return Stats;
}

void UHeroCustomizationComponent::RecalculateAndApply(bool bApplyVisuals)
{
	int32 HeroLevel = 1;
	float HPRatio = 1.0f;
	bool bIsNightTime = false;

	if (const AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(GetOwner()))
	{
		if (Hero->SkillTree)
		{
			HeroLevel = FMath::Max(1, Hero->SkillTree->Level);
		}
		if (Hero->MaxHP > KINDA_SMALL_NUMBER)
		{
			HPRatio = FMath::Clamp(Hero->HP / Hero->MaxHP, 0.0f, 1.0f);
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const AChroniclesGameState* GS = World->GetGameState<AChroniclesGameState>())
		{
			const float TOD = GS->ReplicatedTimeOfDay;
			bIsNightTime = (TOD >= 0.84f || TOD < 0.18f);
		}
	}

	CachedDerivedStats = ComputeDerivedStats(Profile, HeroLevel, bIsNightTime, HPRatio);

	if (bApplyVisuals)
	{
		ApplyVisualsToOwnerHero();
	}
}

// ============================================================================
// Profile Mutation API
// ============================================================================

FHeroCustomizationProfile UHeroCustomizationComponent::SanitizeProfile(const FHeroCustomizationProfile& InProfile)
{
	FHeroCustomizationProfile Out = InProfile;
	Out.HairColorIndex   = FMath::Clamp(Out.HairColorIndex, 0, NUM_HAIR_COLORS - 1);
	Out.TattooColorIndex = FMath::Clamp(Out.TattooColorIndex, 0, NUM_TATTOO_COLORS - 1);
	Out.ArmorTintIndex   = FMath::Clamp(Out.ArmorTintIndex, 0, NUM_ARMOR_TINTS - 1);
	Out.EyeGlowIntensity = FMath::Clamp(Out.EyeGlowIntensity, 0.25f, 4.00f);
	Out.HeightMeters     = FMath::Clamp(Out.HeightMeters, MIN_HEIGHT_METERS, MAX_HEIGHT_METERS);
	Out.Musculature      = FMath::Clamp(Out.Musculature, 0.0f, 1.0f);
	Out.ShoulderWidth    = FMath::Clamp(Out.ShoulderWidth, 0.0f, 1.0f);

	Out.Attributes.Strength     = FMath::Clamp(Out.Attributes.Strength, MIN_ATTRIBUTE_VAL, MAX_ATTRIBUTE_VAL);
	Out.Attributes.Endurance    = FMath::Clamp(Out.Attributes.Endurance, MIN_ATTRIBUTE_VAL, MAX_ATTRIBUTE_VAL);
	Out.Attributes.Agility      = FMath::Clamp(Out.Attributes.Agility, MIN_ATTRIBUTE_VAL, MAX_ATTRIBUTE_VAL);
	Out.Attributes.Intelligence = FMath::Clamp(Out.Attributes.Intelligence, MIN_ATTRIBUTE_VAL, MAX_ATTRIBUTE_VAL);
	Out.Attributes.Spirit       = FMath::Clamp(Out.Attributes.Spirit, MIN_ATTRIBUTE_VAL, MAX_ATTRIBUTE_VAL);
	Out.Attributes.Fortune      = FMath::Clamp(Out.Attributes.Fortune, MIN_ATTRIBUTE_VAL, MAX_ATTRIBUTE_VAL);
	Out.Attributes.UnspentPoints = FMath::Clamp(Out.Attributes.UnspentPoints, 0, 999);

	return Out;
}

void UHeroCustomizationComponent::CommitProfileChange(const FHeroCustomizationProfile& SanitizedProfile)
{
	Profile = SanitizedProfile;
	RecalculateAndApply(true);
	OnCustomizationChanged.Broadcast();

	if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(GetOwner()))
	{
		Hero->RefreshAllHeroStats();

		if (!Hero->HasAuthority() && Hero->IsLocallyControlled())
		{
			ServerSetProfile(Profile);
		}
	}
}

void UHeroCustomizationComponent::OnRep_Profile()
{
	Profile = SanitizeProfile(Profile);
	RecalculateAndApply(true);
	OnCustomizationChanged.Broadcast();

	if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(GetOwner()))
	{
		Hero->RefreshAllHeroStats();
	}
}

void UHeroCustomizationComponent::ServerSetProfile_Implementation(const FHeroCustomizationProfile& NewProfile)
{
	Profile = SanitizeProfile(NewProfile);
	RecalculateAndApply(true);
	OnCustomizationChanged.Broadcast();

	if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(GetOwner()))
	{
		Hero->RefreshAllHeroStats();
	}
}

void UHeroCustomizationComponent::SetCustomizationProfile(const FHeroCustomizationProfile& NewProfile)
{
	CommitProfileChange(SanitizeProfile(NewProfile));
}

void UHeroCustomizationComponent::SetGender(EHeroGender NewGender)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.Gender = NewGender;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetRace(EHeroRace NewRace, bool bApplyRaceDefaultStature)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.Race = NewRace;

	if (bApplyRaceDefaultStature)
	{
		switch (NewRace)
		{
		case EHeroRace::Human:
			Updated.HeightMeters  = 1.85f;
			Updated.Musculature   = 0.50f;
			Updated.ShoulderWidth = 0.50f;
			break;
		case EHeroRace::HighElf:
			Updated.HeightMeters  = 1.94f;
			Updated.Musculature   = 0.34f;
			Updated.ShoulderWidth = 0.42f;
			break;
		case EHeroRace::HighlandNord:
			Updated.HeightMeters  = 2.38f;
			Updated.Musculature   = 0.86f;
			Updated.ShoulderWidth = 0.82f;
			break;
		case EHeroRace::AshDraconid:
			Updated.HeightMeters  = 2.08f;
			Updated.Musculature   = 0.72f;
			Updated.ShoulderWidth = 0.74f;
			break;
		case EHeroRace::ShadowKin:
			Updated.HeightMeters  = 1.72f;
			Updated.Musculature   = 0.32f;
			Updated.ShoulderWidth = 0.38f;
			break;
		case EHeroRace::Ironborn:
			Updated.HeightMeters  = 1.66f;
			Updated.Musculature   = 0.84f;
			Updated.ShoulderWidth = 0.86f;
			break;
		default:
			break;
		}
	}

	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetHairStyle(EHeroHairStyle NewStyle)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.HairStyle = NewStyle;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetHairColorIndex(int32 NewIndex)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.HairColorIndex = ((NewIndex % NUM_HAIR_COLORS) + NUM_HAIR_COLORS) % NUM_HAIR_COLORS;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetMustacheStyle(EHeroMustacheStyle NewStyle)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.MustacheStyle = NewStyle;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetBeardStyle(EHeroBeardStyle NewStyle)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.BeardStyle = NewStyle;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetEyeStyle(EHeroEyeStyle NewStyle)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.EyeStyle = NewStyle;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::AdjustEyeGlow(float Delta)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.EyeGlowIntensity = FMath::Clamp(Updated.EyeGlowIntensity + Delta, 0.25f, 4.0f);
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetTattooStyle(EHeroTattooStyle NewStyle)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.TattooStyle = NewStyle;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetTattooColorIndex(int32 NewIndex)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.TattooColorIndex = ((NewIndex % NUM_TATTOO_COLORS) + NUM_TATTOO_COLORS) % NUM_TATTOO_COLORS;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetOutfitStyle(EHeroOutfitStyle NewStyle)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.OutfitStyle = NewStyle;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetArmorTintIndex(int32 NewIndex)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.ArmorTintIndex = ((NewIndex % NUM_ARMOR_TINTS) + NUM_ARMOR_TINTS) % NUM_ARMOR_TINTS;
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetHeightMeters(float NewHeightMeters)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.HeightMeters = FMath::Clamp(NewHeightMeters, MIN_HEIGHT_METERS, MAX_HEIGHT_METERS);
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetMusculature(float NewMusculature01)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.Musculature = FMath::Clamp(NewMusculature01, 0.f, 1.f);
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetShoulderWidth(float NewShoulderWidth01)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.ShoulderWidth = FMath::Clamp(NewShoulderWidth01, 0.f, 1.f);
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::SetDetailQuality(EHeroDetailQuality NewQuality)
{
	FHeroCustomizationProfile Updated = Profile;
	Updated.DetailQuality = NewQuality;
	CommitProfileChange(SanitizeProfile(Updated));
}

// ============================================================================
// Attribute Allocation & Persistence API
// ============================================================================

int32 UHeroCustomizationComponent::GetBaseAttributeValue(EHeroAttribute Attribute) const
{
	switch (Attribute)
	{
	case EHeroAttribute::Strength:     return Profile.Attributes.Strength;
	case EHeroAttribute::Endurance:    return Profile.Attributes.Endurance;
	case EHeroAttribute::Agility:      return Profile.Attributes.Agility;
	case EHeroAttribute::Intelligence: return Profile.Attributes.Intelligence;
	case EHeroAttribute::Spirit:       return Profile.Attributes.Spirit;
	case EHeroAttribute::Fortune:      return Profile.Attributes.Fortune;
	default:                           return 10;
	}
}

bool UHeroCustomizationComponent::ModifyAttribute(EHeroAttribute Attribute, int32 Delta)
{
	if (Delta == 0)
	{
		return false;
	}

	FHeroCustomizationProfile Updated = Profile;
	int32* TargetAttr = nullptr;
	switch (Attribute)
	{
	case EHeroAttribute::Strength:     TargetAttr = &Updated.Attributes.Strength; break;
	case EHeroAttribute::Endurance:    TargetAttr = &Updated.Attributes.Endurance; break;
	case EHeroAttribute::Agility:      TargetAttr = &Updated.Attributes.Agility; break;
	case EHeroAttribute::Intelligence: TargetAttr = &Updated.Attributes.Intelligence; break;
	case EHeroAttribute::Spirit:       TargetAttr = &Updated.Attributes.Spirit; break;
	case EHeroAttribute::Fortune:      TargetAttr = &Updated.Attributes.Fortune; break;
	default:                           return false;
	}

	if (Delta > 0)
	{
		const int32 MaxAdd = FMath::Min(Updated.Attributes.UnspentPoints, MAX_ATTRIBUTE_VAL - *TargetAttr);
		const int32 ActualAdd = FMath::Min(Delta, MaxAdd);
		if (ActualAdd <= 0)
		{
			return false;
		}
		*TargetAttr += ActualAdd;
		Updated.Attributes.UnspentPoints -= ActualAdd;
	}
	else
	{
		const int32 MaxSub = *TargetAttr - MIN_ATTRIBUTE_VAL;
		const int32 ActualSub = FMath::Min(-Delta, MaxSub);
		if (ActualSub <= 0)
		{
			return false;
		}
		*TargetAttr -= ActualSub;
		Updated.Attributes.UnspentPoints += ActualSub;
	}

	CommitProfileChange(SanitizeProfile(Updated));
	return true;
}

void UHeroCustomizationComponent::GrantAttributePoints(int32 PointsToAdd)
{
	if (PointsToAdd <= 0)
	{
		return;
	}

	FHeroCustomizationProfile Updated = Profile;
	Updated.Attributes.UnspentPoints = FMath::Clamp(Updated.Attributes.UnspentPoints + PointsToAdd, 0, 999);
	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::ResetAttributes()
{
	FHeroCustomizationProfile Updated = Profile;
	const int32 TotalPool =
		Updated.Attributes.Strength +
		Updated.Attributes.Endurance +
		Updated.Attributes.Agility +
		Updated.Attributes.Intelligence +
		Updated.Attributes.Spirit +
		Updated.Attributes.Fortune +
		Updated.Attributes.UnspentPoints;

	Updated.Attributes.Strength      = 10;
	Updated.Attributes.Endurance     = 10;
	Updated.Attributes.Agility       = 10;
	Updated.Attributes.Intelligence  = 10;
	Updated.Attributes.Spirit        = 10;
	Updated.Attributes.Fortune       = 10;
	Updated.Attributes.UnspentPoints = FMath::Max(DEFAULT_START_PTS, TotalPool - 60);

	CommitProfileChange(SanitizeProfile(Updated));
}

void UHeroCustomizationComponent::PopulateSaveGame(UChroniclesSaveGame* SaveObj) const
{
	if (!SaveObj)
	{
		return;
	}

	SaveObj->bHasSavedHeroProfile = true;
	SaveObj->SavedHeroProfile = SanitizeProfile(Profile);
}

void UHeroCustomizationComponent::ApplySaveGame(const UChroniclesSaveGame* SaveObj)
{
	if (!SaveObj || !SaveObj->bHasSavedHeroProfile)
	{
		return;
	}

	CommitProfileChange(SanitizeProfile(SaveObj->SavedHeroProfile));
}

// ============================================================================
// 2. Live 3D Visual Swapping, Scaling, Facial Attachments & LOD
// ============================================================================

void UHeroCustomizationComponent::ApplyVisualsToOwnerHero()
{
	if (bApplyingVisualsGuard)
	{
		return;
	}

	AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(GetOwner());
	if (!Hero)
	{
		return;
	}

	bApplyingVisualsGuard = true;

	// 1. Update AAA Paragon Skeletal Rig if Gender/Outfit/Race variant changed
	UpdateHeroMeshForGenderAndOutfit(Hero);

	// 2. Physical Height / Musculature / ShoulderWidth Scaling on Mesh & Capsule
	const float HeightScale = FMath::Clamp(Profile.HeightMeters / BASE_HEIGHT_METERS, 0.59f, 1.62f);
	const float BulkXY = HeightScale
		* FMath::Lerp(0.84f, 1.24f, Profile.Musculature)
		* FMath::Lerp(0.92f, 1.12f, Profile.ShoulderWidth);

	if (USkeletalMeshComponent* MeshComp = Hero->GetMesh())
	{
		MeshComp->SetRelativeScale3D(FVector(BulkXY, BulkXY, HeightScale));
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -92.f * HeightScale));
	}

	if (UCapsuleComponent* CapsuleComp = Hero->GetCapsuleComponent())
	{
		CapsuleComp->SetCapsuleSize(
			42.f * FMath::Clamp(BulkXY, 0.72f, 1.45f),
			92.f * HeightScale,
			true);
	}

	// 3. Live 3D Facial/Head Attachments, Tattoos & Armor Tint Harmonization
	EnsureAttachmentComponents(Hero);
	UpdateHeadAndFacialAttachments(Hero);
	UpdateTattooAndMaterialTints(Hero);

	// 4. Detail Quality & Skeletal Mesh LOD Bias
	ApplyDetailQualityCVarsAndLOD();

	bApplyingVisualsGuard = false;
}

void UHeroCustomizationComponent::UpdateHeroMeshForGenderAndOutfit(AAetheriaHeroCharacter* Hero)
{
	using namespace HeroCustomizationUtil;

	if (!Hero)
	{
		return;
	}

	FString DesiredMeshPath;
	FString AnimBPPath;
	TArray<FString> AttackMontagePaths;
	FString HitReactPath;
	FString DeathPath;

	if (Profile.Gender == EHeroGender::Female)
	{
		switch (Profile.OutfitStyle)
		{
		case EHeroOutfitStyle::SovereignGilded:
			DesiredMeshPath = PickFirstExistingPath({
				PathSerathGold, PathSerathBase,
				PathAuroraGlacialEmpress, PathAuroraFrozenHearth, PathAuroraBase,
				PathMorigeshMystic, PathMorigeshBase
			});
			break;
		case EHeroOutfitStyle::ArchmageRobes:
			DesiredMeshPath = PickFirstExistingPath({
				PathSerathFrost, PathSerathBase,
				PathAuroraGlacialEmpress, PathAuroraBase,
				PathMorigeshMystic, PathMorigeshBase
			});
			break;
		case EHeroOutfitStyle::DraconidWarlock:
			DesiredMeshPath = PickFirstExistingPath({
				PathKallariRedDeath, PathSerathValentine, PathSerathBase,
				PathMorigeshDarkHeart, PathAuroraGlacialEmpress, PathAuroraBase, PathMorigeshBase
			});
			break;
		case EHeroOutfitStyle::InquisitorCoat:
			DesiredMeshPath = PickFirstExistingPath({
				PathKallariDeathLotus, PathKallariRogue, PathKallariBase, PathSerathFuture, PathSerathBase,
				PathAuroraFrozenHearth, PathAuroraBase, PathMorigeshDarkHeart, PathMorigeshBase
			});
			break;
		case EHeroOutfitStyle::HighlandPelt:
			DesiredMeshPath = PickFirstExistingPath({
				PathMorigeshMystic, PathMorigeshDarkHeart, PathMorigeshBase,
				PathSerathFuture, PathSerathBase, PathAuroraFrozenHearth, PathAuroraBase
			});
			break;
		case EHeroOutfitStyle::VanguardPlate:
		default:
			if (Profile.Race == EHeroRace::ShadowKin)
			{
				DesiredMeshPath = PickFirstExistingPath({
					PathKallariBase, PathSerathBase,
					PathMorigeshDarkHeart, PathAuroraBase, PathMorigeshBase
				});
			}
			else
			{
				DesiredMeshPath = PickFirstExistingPath({
					PathSerathBase, PathAuroraBase, PathAuroraFrozenHearth,
					PathAuroraGlacialEmpress, PathMorigeshBase
				});
			}
			break;
		}
	}
	else
	{
		switch (Profile.OutfitStyle)
		{
		case EHeroOutfitStyle::ArchmageRobes:
			DesiredMeshPath = PickFirstExistingPath({
				PathGideonBase, PathGideonInquisitor, PathGideonTough,
				PathKhaimeraBase, PathGruxBase
			});
			break;

		case EHeroOutfitStyle::InquisitorCoat:
			DesiredMeshPath = PickFirstExistingPath({
				PathGideonInquisitor, PathGideonTough, PathGideonBase,
				PathKhaimeraBase, PathGruxChestplate
			});
			break;

		case EHeroOutfitStyle::DraconidWarlock:
			DesiredMeshPath = PickFirstExistingPath({
				PathGideonMephisto, PathGruxWarchief, PathGideonBase,
				PathKhaimeraGruxPelt, PathGruxBase
			});
			break;

		case EHeroOutfitStyle::HighlandPelt:
			DesiredMeshPath = PickFirstExistingPath({
				PathKhaimeraGruxPelt, PathKhaimeraBase, PathGruxWarchief,
				PathGruxBase, PathGideonTough, PathGideonBase
			});
			break;

		case EHeroOutfitStyle::SovereignGilded:
			DesiredMeshPath = PickFirstExistingPath({
				PathKwangSunrise, PathKwangRosewood, PathKwangGDC,
				PathGruxChestplate, PathGideonTough, PathGideonInquisitor,
				PathGruxWarchief, PathGideonBase
			});
			break;

		case EHeroOutfitStyle::VanguardPlate:
		default:
			if (Profile.HairStyle == EHeroHairStyle::SamuraiManbun)
			{
				DesiredMeshPath = PickFirstExistingPath({
					PathKwangManbun, PathKwangGDC,
					PathGideonTough, PathGideonBase, PathKhaimeraBase, PathGruxChestplate
				});
			}
			else if (Profile.Race == EHeroRace::HighElf)
			{
				DesiredMeshPath = PickFirstExistingPath({
					PathKwangAlbino, PathKwangGDC,
					PathGideonInquisitor, PathGideonBase, PathKhaimeraBase, PathGruxBase
				});
			}
			else if (Profile.Race == EHeroRace::AshDraconid)
			{
				DesiredMeshPath = PickFirstExistingPath({
					PathKwangFrostwalker, PathKwangGDC,
					PathGideonMephisto, PathGruxWarchief, PathGideonBase, PathKhaimeraBase
				});
			}
			else
			{
				DesiredMeshPath = PickFirstExistingPath({
					PathKwangGDC, PathGideonTough, PathGideonBase,
					PathKhaimeraBase, PathGruxChestplate, PathGruxBase
				});
			}
			break;
		}
	}

	if (DesiredMeshPath.IsEmpty() || DesiredMeshPath == LastAppliedSkeletalMeshPath)
	{
		return;
	}

	if (!DoesObjectPackageExist(DesiredMeshPath))
	{
		return;
	}

	ResolveRigAnimationsForMeshPath(DesiredMeshPath, AnimBPPath, AttackMontagePaths, HitReactPath, DeathPath);

	if (Hero->ApplyCustomSkeletalRig(
		DesiredMeshPath,
		AnimBPPath,
		AttackMontagePaths,
		HitReactPath,
		DeathPath,
		FName(TEXT("FullBody"))))
	{
		LastAppliedSkeletalMeshPath = DesiredMeshPath;
	}
}

void UHeroCustomizationComponent::EnsureAttachmentComponents(AAetheriaHeroCharacter* Hero)
{
	using namespace HeroCustomizationUtil;

	if (!Hero)
	{
		return;
	}

	if (!HairAttachMesh)    { HairAttachMesh    = CreateCosmeticMesh(Hero, TEXT("HeroHairAttachMesh")); }
	if (!HairSecondaryMesh) { HairSecondaryMesh = CreateCosmeticMesh(Hero, TEXT("HeroHairSecondaryMesh")); }
	if (!MustacheLeftMesh)  { MustacheLeftMesh  = CreateCosmeticMesh(Hero, TEXT("HeroMustacheLeftMesh")); }
	if (!MustacheRightMesh) { MustacheRightMesh = CreateCosmeticMesh(Hero, TEXT("HeroMustacheRightMesh")); }
	if (!BeardMainMesh)     { BeardMainMesh     = CreateCosmeticMesh(Hero, TEXT("HeroBeardMainMesh")); }
	if (!BeardLowerMesh)    { BeardLowerMesh    = CreateCosmeticMesh(Hero, TEXT("HeroBeardLowerMesh")); }
	if (!EyeLeftMesh)       { EyeLeftMesh       = CreateCosmeticMesh(Hero, TEXT("HeroEyeLeftMesh")); }
	if (!EyeRightMesh)      { EyeRightMesh      = CreateCosmeticMesh(Hero, TEXT("HeroEyeRightMesh")); }
	if (!TattooBrowMesh)    { TattooBrowMesh    = CreateCosmeticMesh(Hero, TEXT("HeroTattooBrowMesh")); }
	if (!TattooChestMesh)   { TattooChestMesh   = CreateCosmeticMesh(Hero, TEXT("HeroTattooChestMesh")); }

	if (!HairMID)
	{
		HairMID = CreateCosmeticMID(this, GetHairColor(Profile.HairColorIndex), false, 0.08f, 0.56f);
	}
	if (!EyeMID)
	{
		const FLinearColor EyeCol = GetEyeColor(Profile.EyeStyle) * (Profile.EyeGlowIntensity * 3.5f);
		EyeMID = CreateCosmeticMID(this, EyeCol, true, 0.0f, 0.25f);
	}
	if (!TattooMID)
	{
		const FLinearColor TatCol = GetTattooColor(Profile.TattooColorIndex) * 2.4f;
		TattooMID = CreateCosmeticMID(this, TatCol, true, 0.05f, 0.35f);
	}
}

FName UHeroCustomizationComponent::ResolveHeadBoneName(const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return NAME_None;
	}

	static const FName HeadCandidates[] = {
		FName(TEXT("head")),
		FName(TEXT("Head")),
		FName(TEXT("b_Head")),
		FName(TEXT("neck_01")),
		FName(TEXT("Neck")),
		FName(TEXT("spine_03"))
	};

	for (const FName& Candidate : HeadCandidates)
	{
		if (MeshComp->GetBoneIndex(Candidate) != INDEX_NONE)
		{
			return Candidate;
		}
	}
	return NAME_None;
}

FName UHeroCustomizationComponent::ResolveChestBoneName(const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return NAME_None;
	}

	static const FName ChestCandidates[] = {
		FName(TEXT("spine_03")),
		FName(TEXT("spine_02")),
		FName(TEXT("Spine2")),
		FName(TEXT("Spine1")),
		FName(TEXT("chest")),
		FName(TEXT("Chest")),
		FName(TEXT("spine_01"))
	};

	for (const FName& Candidate : ChestCandidates)
	{
		if (MeshComp->GetBoneIndex(Candidate) != INDEX_NONE)
		{
			return Candidate;
		}
	}
	return NAME_None;
}

void UHeroCustomizationComponent::UpdateHeadAndFacialAttachments(AAetheriaHeroCharacter* Hero)
{
	using namespace HeroCustomizationUtil;

	if (!Hero)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = Hero->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	const FName HeadBone = ResolveHeadBoneName(MeshComp);
	const FLinearColor HairCol = GetHairColor(Profile.HairColorIndex);
	UpdateCosmeticMID(HairMID, HairCol, Profile.HairStyle == EHeroHairStyle::CrownCirclet ? 0.85f : 0.08f,
		Profile.HairStyle == EHeroHairStyle::CrownCirclet ? 0.25f : 0.56f);

	const FLinearColor EyeCol = GetEyeColor(Profile.EyeStyle) * (Profile.EyeGlowIntensity * 3.6f);
	UpdateCosmeticMID(EyeMID, EyeCol, 0.0f, 0.20f);

	UStaticMesh* SphereMesh   = LoadShapeMesh(PathSphere);
	UStaticMesh* CylinderMesh = LoadShapeMesh(PathCylinder);
	UStaticMesh* ConeMesh     = LoadShapeMesh(PathCone);
	UStaticMesh* CubeMesh     = LoadShapeMesh(PathCube);

	// ---- Hair / Crown / Hood Silhouette ----
	bool bShowPrimaryHair = true;
	bool bShowSecondaryHair = false;
	UStaticMesh* HairPrimShape = SphereMesh;
	UStaticMesh* HairSecShape = CylinderMesh;
	FVector HairPrimLoc(13.f, 0.f, -3.f);
	FRotator HairPrimRot(0.f, 0.f, 0.f);
	FVector HairPrimScale(0.11f, 0.11f, 0.11f);
	FVector HairSecLoc(11.f, 0.f, -5.f);
	FRotator HairSecRot(0.f, 0.f, 0.f);
	FVector HairSecScale(0.06f, 0.06f, 0.04f);

	switch (Profile.HairStyle)
	{
	case EHeroHairStyle::ShortKnight:
	case EHeroHairStyle::BaldShaved:
		bShowPrimaryHair = false;
		bShowSecondaryHair = false;
		break;

	case EHeroHairStyle::SamuraiManbun:
		HairPrimShape = SphereMesh;
		HairPrimLoc = FVector(15.f, 0.f, -5.f);
		HairPrimScale = FVector(0.085f, 0.085f, 0.095f);
		bShowSecondaryHair = true;
		HairSecShape = CylinderMesh;
		HairSecLoc = FVector(12.5f, 0.f, -4.f);
		HairSecScale = FVector(0.045f, 0.045f, 0.025f);
		break;

	case EHeroHairStyle::LongFlowing:
		HairPrimShape = ConeMesh;
		HairPrimLoc = FVector(4.f, 0.f, -10.f);
		HairPrimRot = FRotator(165.f, 0.f, 0.f);
		HairPrimScale = FVector(0.17f, 0.15f, 0.24f);
		bShowSecondaryHair = true;
		HairSecShape = SphereMesh;
		HairSecLoc = FVector(10.f, 0.f, -3.f);
		HairSecScale = FVector(0.15f, 0.14f, 0.12f);
		break;

	case EHeroHairStyle::BraidedWarrior:
		HairPrimShape = CylinderMesh;
		HairPrimLoc = FVector(1.f, 0.f, -11.f);
		HairPrimRot = FRotator(22.f, 0.f, 0.f);
		HairPrimScale = FVector(0.055f, 0.055f, 0.24f);
		bShowSecondaryHair = true;
		HairSecShape = CylinderMesh;
		HairSecLoc = FVector(12.f, 0.f, 0.f);
		HairSecScale = FVector(0.135f, 0.135f, 0.028f);
		break;

	case EHeroHairStyle::BattleMohawk:
		HairPrimShape = CubeMesh;
		HairPrimLoc = FVector(16.f, 0.f, 2.f);
		HairPrimRot = FRotator(-15.f, 0.f, 0.f);
		HairPrimScale = FVector(0.16f, 0.038f, 0.14f);
		bShowSecondaryHair = true;
		HairSecShape = ConeMesh;
		HairSecLoc = FVector(12.f, 0.f, -7.f);
		HairSecRot = FRotator(-110.f, 0.f, 0.f);
		HairSecScale = FVector(0.06f, 0.04f, 0.16f);
		break;

	case EHeroHairStyle::ArchmageHood:
		HairPrimShape = SphereMesh;
		HairPrimLoc = FVector(9.f, 0.f, -2.f);
		HairPrimScale = FVector(0.22f, 0.21f, 0.23f);
		bShowSecondaryHair = true;
		HairSecShape = ConeMesh;
		HairSecLoc = FVector(14.f, 0.f, -8.f);
		HairSecRot = FRotator(-105.f, 0.f, 0.f);
		HairSecScale = FVector(0.16f, 0.16f, 0.18f);
		break;

	case EHeroHairStyle::CrownCirclet:
		HairPrimShape = CylinderMesh;
		HairPrimLoc = FVector(13.5f, 0.f, 0.f);
		HairPrimRot = FRotator(-8.f, 0.f, 0.f);
		HairPrimScale = FVector(0.145f, 0.145f, 0.022f);
		bShowSecondaryHair = true;
		HairSecShape = ConeMesh;
		HairSecLoc = FVector(16.f, 0.f, 7.f);
		HairSecRot = FRotator(12.f, 0.f, 0.f);
		HairSecScale = FVector(0.035f, 0.035f, 0.065f);
		break;
	}

	ConfigureAttachment(HairAttachMesh, MeshComp, HeadBone, HairPrimShape, HairMID, HairPrimLoc, HairPrimRot, HairPrimScale, bShowPrimaryHair);
	ConfigureAttachment(HairSecondaryMesh, MeshComp, HeadBone, HairSecShape, HairMID, HairSecLoc, HairSecRot, HairSecScale, bShowSecondaryHair);

	// ---- Mustache (Left & Right Symmetrical) ----
	bool bShowMustache = (Profile.MustacheStyle != EHeroMustacheStyle::None);
	UStaticMesh* MustacheShape = ConeMesh;
	FVector MustacheLoc(4.2f, 2.2f, 8.8f);
	FRotator MustacheRot(0.f, 0.f, 78.f);
	FVector MustacheScale(0.018f, 0.018f, 0.048f);

	switch (Profile.MustacheStyle)
	{
	case EHeroMustacheStyle::NobleChevron:
		MustacheShape = CubeMesh;
		MustacheLoc = FVector(4.4f, 1.8f, 8.6f);
		MustacheRot = FRotator(0.f, 0.f, 16.f);
		MustacheScale = FVector(0.012f, 0.036f, 0.010f);
		break;
	case EHeroMustacheStyle::Handlebar:
		MustacheShape = ConeMesh;
		MustacheLoc = FVector(4.4f, 2.6f, 8.5f);
		MustacheRot = FRotator(0.f, 0.f, 102.f);
		MustacheScale = FVector(0.015f, 0.015f, 0.058f);
		break;
	case EHeroMustacheStyle::ImperialWalrus:
		MustacheShape = SphereMesh;
		MustacheLoc = FVector(4.0f, 2.0f, 8.9f);
		MustacheRot = FRotator(0.f, 0.f, 12.f);
		MustacheScale = FVector(0.024f, 0.044f, 0.022f);
		break;
	case EHeroMustacheStyle::HighlandForked:
		MustacheShape = ConeMesh;
		MustacheLoc = FVector(3.4f, 2.5f, 8.8f);
		MustacheRot = FRotator(145.f, 0.f, 28.f);
		MustacheScale = FVector(0.020f, 0.020f, 0.068f);
		break;
	case EHeroMustacheStyle::LightStubble:
		MustacheShape = CubeMesh;
		MustacheLoc = FVector(4.3f, 1.4f, 8.4f);
		MustacheRot = FRotator(0.f, 0.f, 8.f);
		MustacheScale = FVector(0.008f, 0.028f, 0.006f);
		break;
	case EHeroMustacheStyle::None:
	default:
		bShowMustache = false;
		break;
	}

	ConfigureAttachment(MustacheLeftMesh, MeshComp, HeadBone, MustacheShape, HairMID,
		MustacheLoc, MustacheRot, MustacheScale, bShowMustache);
	ConfigureAttachment(MustacheRightMesh, MeshComp, HeadBone, MustacheShape, HairMID,
		FVector(MustacheLoc.X, -MustacheLoc.Y, MustacheLoc.Z),
		FRotator(MustacheRot.Pitch, -MustacheRot.Yaw, -MustacheRot.Roll),
		MustacheScale, bShowMustache);

	// ---- Beard (Chin & Lower Jaw) ----
	bool bShowBeardMain = (Profile.BeardStyle != EHeroBeardStyle::None);
	bool bShowBeardLower = false;
	UStaticMesh* BeardMainShape = SphereMesh;
	UStaticMesh* BeardLowerShape = ConeMesh;
	FVector BeardMainLoc(1.2f, 0.f, 7.6f);
	FRotator BeardMainRot(0.f, 0.f, 0.f);
	FVector BeardMainScale(0.050f, 0.068f, 0.052f);
	FVector BeardLowerLoc(-3.5f, 0.f, 7.8f);
	FRotator BeardLowerRot(175.f, 0.f, 0.f);
	FVector BeardLowerScale(0.042f, 0.048f, 0.090f);

	switch (Profile.BeardStyle)
	{
	case EHeroBeardStyle::Stubble:
		BeardMainShape = SphereMesh;
		BeardMainLoc = FVector(1.5f, 0.f, 7.2f);
		BeardMainScale = FVector(0.036f, 0.062f, 0.042f);
		break;
	case EHeroBeardStyle::ShortBoxed:
		BeardMainShape = CubeMesh;
		BeardMainLoc = FVector(0.6f, 0.f, 7.4f);
		BeardMainScale = FVector(0.056f, 0.068f, 0.054f);
		break;
	case EHeroBeardStyle::RangerFull:
		BeardMainShape = SphereMesh;
		BeardMainLoc = FVector(-0.8f, 0.f, 7.5f);
		BeardMainScale = FVector(0.072f, 0.078f, 0.068f);
		bShowBeardLower = true;
		BeardLowerShape = ConeMesh;
		BeardLowerLoc = FVector(-4.2f, 0.f, 7.7f);
		BeardLowerScale = FVector(0.055f, 0.055f, 0.095f);
		break;
	case EHeroBeardStyle::VikingBraided:
		BeardMainShape = SphereMesh;
		BeardMainLoc = FVector(-0.5f, 0.f, 7.4f);
		BeardMainScale = FVector(0.068f, 0.076f, 0.064f);
		bShowBeardLower = true;
		BeardLowerShape = CylinderMesh;
		BeardLowerLoc = FVector(-5.2f, 0.f, 8.0f);
		BeardLowerRot = FRotator(168.f, 0.f, 0.f);
		BeardLowerScale = FVector(0.034f, 0.034f, 0.135f);
		break;
	case EHeroBeardStyle::ArchmageLong:
		BeardMainShape = ConeMesh;
		BeardMainLoc = FVector(-1.8f, 0.f, 7.4f);
		BeardMainRot = FRotator(175.f, 0.f, 0.f);
		BeardMainScale = FVector(0.076f, 0.080f, 0.140f);
		bShowBeardLower = true;
		BeardLowerShape = ConeMesh;
		BeardLowerLoc = FVector(-7.8f, 0.f, 8.2f);
		BeardLowerRot = FRotator(175.f, 0.f, 0.f);
		BeardLowerScale = FVector(0.048f, 0.048f, 0.160f);
		break;
	case EHeroBeardStyle::Goatee:
		BeardMainShape = ConeMesh;
		BeardMainLoc = FVector(0.2f, 0.f, 8.0f);
		BeardMainRot = FRotator(175.f, 0.f, 0.f);
		BeardMainScale = FVector(0.034f, 0.034f, 0.060f);
		break;
	case EHeroBeardStyle::None:
	default:
		bShowBeardMain = false;
		bShowBeardLower = false;
		break;
	}

	ConfigureAttachment(BeardMainMesh, MeshComp, HeadBone, BeardMainShape, HairMID,
		BeardMainLoc, BeardMainRot, BeardMainScale, bShowBeardMain);
	ConfigureAttachment(BeardLowerMesh, MeshComp, HeadBone, BeardLowerShape, HairMID,
		BeardLowerLoc, BeardLowerRot, BeardLowerScale, bShowBeardLower);

	// ---- Glowing Irises (only spawn arcane eye glow orbs when boosted > 1.4 or supernatural race!) ----
	const bool bShowArcaneEyeGlow = (Profile.EyeGlowIntensity >= 1.45f)
		|| (Profile.Race == EHeroRace::AshDraconid)
		|| (Profile.Race == EHeroRace::ShadowKin)
		|| (Profile.EyeStyle == EHeroEyeStyle::ArcaneCyan)
		|| (Profile.EyeStyle == EHeroEyeStyle::CrimsonFlame);
	const FVector EyeScale(0.011f, 0.011f, 0.011f);
	ConfigureAttachment(EyeLeftMesh, MeshComp, HeadBone, SphereMesh, EyeMID,
		FVector(6.8f, 2.8f, 8.6f), FRotator::ZeroRotator, EyeScale, bShowArcaneEyeGlow);
	ConfigureAttachment(EyeRightMesh, MeshComp, HeadBone, SphereMesh, EyeMID,
		FVector(6.8f, -2.8f, 8.6f), FRotator::ZeroRotator, EyeScale, bShowArcaneEyeGlow);
}

void UHeroCustomizationComponent::UpdateTattooAndMaterialTints(AAetheriaHeroCharacter* Hero)
{
	using namespace HeroCustomizationUtil;

	if (!Hero)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = Hero->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	const FName HeadBone  = ResolveHeadBoneName(MeshComp);
	const FName ChestBone = ResolveChestBoneName(MeshComp);
	const FLinearColor TattooCol = GetTattooColor(Profile.TattooColorIndex) * 2.5f;
	UpdateCosmeticMID(TattooMID, TattooCol, 0.08f, 0.32f);

	UStaticMesh* CubeMesh     = LoadShapeMesh(PathCube);
	UStaticMesh* CylinderMesh = LoadShapeMesh(PathCylinder);
	UStaticMesh* SphereMesh   = LoadShapeMesh(PathSphere);
	UStaticMesh* ConeMesh     = LoadShapeMesh(PathCone);

	bool bShowTattoo = (Profile.TattooStyle != EHeroTattooStyle::None);
	UStaticMesh* BrowShape  = CubeMesh;
	UStaticMesh* ChestShape = CylinderMesh;
	FVector BrowLoc(9.5f, 0.f, 8.7f);
	FRotator BrowRot(0.f, 0.f, 0.f);
	FVector BrowScale(0.014f, 0.056f, 0.010f);

	FVector ChestLoc(0.f, 0.f, 12.f);
	FRotator ChestRot(0.f, 0.f, 0.f);
	FVector ChestScale(0.11f, 0.14f, 0.016f);

	switch (Profile.TattooStyle)
	{
	case EHeroTattooStyle::NordicRunes:
		BrowShape = CubeMesh;
		BrowScale = FVector(0.018f, 0.068f, 0.012f);
		ChestShape = CubeMesh;
		ChestScale = FVector(0.13f, 0.12f, 0.014f);
		break;
	case EHeroTattooStyle::DragonScales:
		BrowShape = ConeMesh;
		BrowLoc = FVector(9.6f, 2.2f, 8.4f);
		BrowRot = FRotator(0.f, 0.f, 35.f);
		BrowScale = FVector(0.016f, 0.016f, 0.044f);
		ChestShape = SphereMesh;
		ChestScale = FVector(0.10f, 0.16f, 0.018f);
		break;
	case EHeroTattooStyle::ArcaneSigils:
		BrowShape = CylinderMesh;
		BrowRot = FRotator(90.f, 0.f, 0.f);
		BrowScale = FVector(0.022f, 0.022f, 0.008f);
		ChestShape = CylinderMesh;
		ChestRot = FRotator(90.f, 0.f, 0.f);
		ChestScale = FVector(0.14f, 0.14f, 0.010f);
		break;
	case EHeroTattooStyle::CrimsonScar:
		BrowShape = CubeMesh;
		BrowLoc = FVector(8.5f, 2.6f, 8.4f);
		BrowRot = FRotator(0.f, 0.f, 22.f);
		BrowScale = FVector(0.045f, 0.008f, 0.008f);
		ChestShape = CubeMesh;
		ChestRot = FRotator(0.f, 0.f, 35.f);
		ChestScale = FVector(0.22f, 0.014f, 0.014f);
		break;
	case EHeroTattooStyle::TribalAsh:
		BrowShape = CubeMesh;
		BrowScale = FVector(0.012f, 0.088f, 0.014f);
		ChestShape = CubeMesh;
		ChestScale = FVector(0.16f, 0.22f, 0.012f);
		break;
	case EHeroTattooStyle::CelestialHalo:
		BrowShape = CylinderMesh;
		BrowLoc = FVector(13.8f, 0.f, 0.f);
		BrowScale = FVector(0.165f, 0.165f, 0.008f);
		ChestShape = SphereMesh;
		ChestScale = FVector(0.08f, 0.08f, 0.016f);
		break;
	case EHeroTattooStyle::None:
	default:
		bShowTattoo = false;
		break;
	}

	ConfigureAttachment(TattooBrowMesh, MeshComp, HeadBone, BrowShape, TattooMID,
		BrowLoc, BrowRot, BrowScale, bShowTattoo);
	ConfigureAttachment(TattooChestMesh, MeshComp, ChestBone, ChestShape, TattooMID,
		ChestLoc, ChestRot, ChestScale, bShowTattoo);

	// Only override skin/armor/tattoo material parameters when the player actually picked a NON-default tint.
	// Preserves crisp hand-painted Paragon PBR skin, metal, and leather detail without pure-white washout!
	const bool bHasCustomSkinTint   = (Profile.Race != EHeroRace::Human);
	const bool bHasCustomArmorTint  = (Profile.ArmorTintIndex != 0);
	const bool bHasCustomTattooTint = (Profile.TattooStyle != EHeroTattooStyle::None) || (Profile.TattooColorIndex != 0);
	const bool bAnyCustomTint       = bHasCustomSkinTint || bHasCustomArmorTint || bHasCustomTattooTint;

	const FLinearColor ArmorTint  = ClampGroundedTint(GetArmorTintColor(Profile.ArmorTintIndex), 0.88f);
	const FLinearColor SkinTint   = ClampGroundedTint(GetRaceSkinTint(Profile.Race), 0.88f);
	const FLinearColor TattooTint = ClampGroundedTint(GetTattooColor(Profile.TattooColorIndex), 0.88f);

	const int32 NumMats = MeshComp->GetNumMaterials();
	for (int32 SlotIdx = 0; SlotIdx < NumMats; ++SlotIdx)
	{
		UMaterialInterface* Mat = MeshComp->GetMaterial(SlotIdx);
		if (!Mat)
		{
			continue;
		}

		UMaterialInstanceDynamic* ExistingMID = Cast<UMaterialInstanceDynamic>(Mat);
		// Fast path: untouched default Paragon material with no custom tints requested -> preserve authored PBR as-is!
		if (!bAnyCustomTint && !ExistingMID)
		{
			continue;
		}

		TArray<FMaterialParameterInfo> VecInfos;
		TArray<FGuid> VecGuids;
		Mat->GetAllVectorParameterInfo(VecInfos, VecGuids);

		bool bNeedsSkinOverride   = false;
		bool bNeedsArmorOverride  = false;
		bool bNeedsTattooOverride = false;
		bool bHasAnyTargetParam   = false;
		for (const FMaterialParameterInfo& Info : VecInfos)
		{
			const FString ParamStr = Info.Name.ToString();
			if (ParamStr.Contains(TEXT("Emissive"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			if (ParamStr.Contains(TEXT("Skin"), ESearchCase::IgnoreCase))
			{
				bHasAnyTargetParam = true;
				if (bHasCustomSkinTint) { bNeedsSkinOverride = true; }
			}
			else if (ParamStr.Contains(TEXT("Tattoo"), ESearchCase::IgnoreCase)
				|| ParamStr.Contains(TEXT("Warpaint"), ESearchCase::IgnoreCase))
			{
				bHasAnyTargetParam = true;
				if (bHasCustomTattooTint) { bNeedsTattooOverride = true; }
			}
			else if (ParamStr.Contains(TEXT("Tint"), ESearchCase::IgnoreCase)
				|| ParamStr.Contains(TEXT("Armor"), ESearchCase::IgnoreCase))
			{
				bHasAnyTargetParam = true;
				if (bHasCustomArmorTint) { bNeedsArmorOverride = true; }
			}
		}

		if (!bHasAnyTargetParam)
		{
			continue;
		}

		// Create MID only if we actually need to apply a non-default tint; otherwise restore parent defaults if MID exists
		UMaterialInstanceDynamic* SlotMID = ExistingMID;
		if (!SlotMID && (bNeedsSkinOverride || bNeedsArmorOverride || bNeedsTattooOverride))
		{
			SlotMID = MeshComp->CreateAndSetMaterialInstanceDynamic(SlotIdx);
		}
		if (!SlotMID)
		{
			continue;
		}

		for (const FMaterialParameterInfo& Info : VecInfos)
		{
			const FString ParamStr = Info.Name.ToString();
			if (ParamStr.Contains(TEXT("Emissive"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			auto RestoreParentVector = [SlotMID, &Info]()
			{
				if (SlotMID->Parent)
				{
					FLinearColor ParentVal = FLinearColor(0.82f, 0.82f, 0.82f, 1.0f);
					if (SlotMID->Parent->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), ParentVal))
					{
						SlotMID->SetVectorParameterValue(Info.Name, ParentVal);
					}
				}
			};

			if (ParamStr.Contains(TEXT("Skin"), ESearchCase::IgnoreCase))
			{
				if (bHasCustomSkinTint)
				{
					SlotMID->SetVectorParameterValue(Info.Name, SkinTint);
				}
				else
				{
					RestoreParentVector();
				}
			}
			else if (ParamStr.Contains(TEXT("Tattoo"), ESearchCase::IgnoreCase)
				|| ParamStr.Contains(TEXT("Warpaint"), ESearchCase::IgnoreCase))
			{
				if (bHasCustomTattooTint)
				{
					SlotMID->SetVectorParameterValue(Info.Name, TattooTint);
				}
				else
				{
					RestoreParentVector();
				}
			}
			else if (ParamStr.Contains(TEXT("Tint"), ESearchCase::IgnoreCase)
				|| ParamStr.Contains(TEXT("Armor"), ESearchCase::IgnoreCase))
			{
				if (bHasCustomArmorTint)
				{
					SlotMID->SetVectorParameterValue(Info.Name, ArmorTint);
				}
				else
				{
					RestoreParentVector();
				}
			}
		}
	}

	FAetheriaStyleHarmonizer::HarmonizeActorMaterials(Hero, EChronicleStyleCategory::Character);
}

void UHeroCustomizationComponent::ApplyDetailQualityCVarsAndLOD()
{
	int32 ForcedLOD = 0;
	int32 LODBias = 0;

	switch (Profile.DetailQuality)
	{
	case EHeroDetailQuality::Low:
		ForcedLOD = 3;
		LODBias = 2;
		break;
	case EHeroDetailQuality::Medium:
		ForcedLOD = 2;
		LODBias = 1;
		break;
	case EHeroDetailQuality::High:
		ForcedLOD = 1;
		LODBias = 0;
		break;
	case EHeroDetailQuality::Ultra:
		ForcedLOD = 0;
		LODBias = 0;
		break;
	case EHeroDetailQuality::CinematicRTX:
	default:
		ForcedLOD = 0;
		LODBias = -1;
		break;
	}

	if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* MeshComp = Hero->GetMesh())
		{
			MeshComp->SetForcedLOD(ForcedLOD);
		}
	}

	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SkeletalMeshLODBias")))
	{
		CVar->Set(LODBias, ECVF_SetByCode);
	}
}

// ============================================================================
// UI Display Label & Palette Helpers
// ============================================================================

FString UHeroCustomizationComponent::GetGenderLabel(EHeroGender Gender)
{
	switch (Gender)
	{
	case EHeroGender::Male:   return TEXT("Male");
	case EHeroGender::Female: return TEXT("Female");
	default:                  return TEXT("Male");
	}
}

FString UHeroCustomizationComponent::GetRaceLabel(EHeroRace Race)
{
	switch (Race)
	{
	case EHeroRace::Human:        return TEXT("Human (Valenarian)");
	case EHeroRace::HighElf:      return TEXT("High Elf (Aetherian)");
	case EHeroRace::HighlandNord: return TEXT("Highland Nord");
	case EHeroRace::AshDraconid:  return TEXT("Ash Draconid");
	case EHeroRace::ShadowKin:    return TEXT("Shadow Kin (Umbra)");
	case EHeroRace::Ironborn:     return TEXT("Ironborn Runesmith");
	default:                      return TEXT("Human");
	}
}

FString UHeroCustomizationComponent::GetRaceLoreAndPassives(EHeroRace Race)
{
	switch (Race)
	{
	case EHeroRace::Human:
		return TEXT("Adaptable Valenarian knights & battlemages. +2 All Stats · +20% XP & Gold · +45ms Parry Window · -15% Block Stamina Drain.");
	case EHeroRace::HighElf:
		return TEXT("Aetherian arcanists attuned to the astral leylines. +5 INT +4 SPI +3 AGI -2 END · -20% Mana Cost · -15% Cooldown · +12% Speed · +35% Map Reveal.");
	case EHeroRace::HighlandNord:
		return TEXT("Towering northern champions forged in blizzard warfare. +6 STR +5 END -2 INT -1 AGI · +25% HP · Cold Slow Immune · +30% Melee & Stagger <=45% HP.");
	case EHeroRace::AshDraconid:
		return TEXT("Ember-blooded drake riders of the Ash Caldera. +4 STR +4 SPI +3 END +2 INT -1 AGI · +25% Drake Speed · +35% Fire Breath · +40% Fire Res · +25% Exort.");
	case EHeroRace::ShadowKin:
		return TEXT("Twilight assassins of the Sapphire Canopy. +6 AGI +4 LCK +2 INT -3 END · +25% Dodge · +80ms i-Frames · -35% Dodge Cost · +12% Crit · +40% Night Crit Dmg.");
	case EHeroRace::Ironborn:
		return TEXT("Stout citadel runesmiths & siege-breakers. +5 END +4 STR +3 LCK -3 AGI · +18% Armor · +50% Poise · +45% Destruction · 2.0x Parry Stagger · +15% XP & Gold.");
	default:
		return TEXT("");
	}
}

FString UHeroCustomizationComponent::GetHairStyleLabel(EHeroHairStyle Style)
{
	switch (Style)
	{
	case EHeroHairStyle::ShortKnight:    return TEXT("Short Knight");
	case EHeroHairStyle::SamuraiManbun:  return TEXT("Samurai Topknot");
	case EHeroHairStyle::LongFlowing:    return TEXT("Long Flowing");
	case EHeroHairStyle::BraidedWarrior: return TEXT("Braided Warrior");
	case EHeroHairStyle::BattleMohawk:   return TEXT("Highland Crest");
	case EHeroHairStyle::ArchmageHood:   return TEXT("Arcane Hood");
	case EHeroHairStyle::CrownCirclet:   return TEXT("Sovereign Crown");
	case EHeroHairStyle::BaldShaved:     return TEXT("Shaved / Bald");
	default:                             return TEXT("Short Knight");
	}
}

FString UHeroCustomizationComponent::GetMustacheStyleLabel(EHeroMustacheStyle Style)
{
	switch (Style)
	{
	case EHeroMustacheStyle::None:           return TEXT("None");
	case EHeroMustacheStyle::NobleChevron:   return TEXT("Noble Chevron");
	case EHeroMustacheStyle::Handlebar:      return TEXT("Handlebar");
	case EHeroMustacheStyle::ImperialWalrus: return TEXT("Imperial Walrus");
	case EHeroMustacheStyle::HighlandForked: return TEXT("Highland Forked");
	case EHeroMustacheStyle::LightStubble:   return TEXT("Light Stubble");
	default:                                 return TEXT("None");
	}
}

FString UHeroCustomizationComponent::GetBeardStyleLabel(EHeroBeardStyle Style)
{
	switch (Style)
	{
	case EHeroBeardStyle::None:          return TEXT("Clean Shaven");
	case EHeroBeardStyle::Stubble:       return TEXT("Rugged Stubble");
	case EHeroBeardStyle::ShortBoxed:    return TEXT("Short Boxed");
	case EHeroBeardStyle::RangerFull:    return TEXT("Ranger Full");
	case EHeroBeardStyle::VikingBraided: return TEXT("Nordic Braided");
	case EHeroBeardStyle::ArchmageLong:  return TEXT("Archmage Long");
	case EHeroBeardStyle::Goatee:        return TEXT("Royal Goatee");
	default:                             return TEXT("Short Boxed");
	}
}

FString UHeroCustomizationComponent::GetEyeStyleLabel(EHeroEyeStyle Style)
{
	switch (Style)
	{
	case EHeroEyeStyle::SteelBlue:    return TEXT("Steel Blue");
	case EHeroEyeStyle::MoltenAmber:  return TEXT("Molten Amber");
	case EHeroEyeStyle::ArcaneCyan:   return TEXT("Arcane Cyan");
	case EHeroEyeStyle::EmeraldJade:  return TEXT("Emerald Jade");
	case EHeroEyeStyle::CrimsonFlame: return TEXT("Crimson Flame");
	case EHeroEyeStyle::VoidViolet:   return TEXT("Void Violet");
	case EHeroEyeStyle::SilverOracle: return TEXT("Silver Oracle");
	case EHeroEyeStyle::SolarGold:    return TEXT("Solar Gold");
	default:                          return TEXT("Molten Amber");
	}
}

FString UHeroCustomizationComponent::GetTattooStyleLabel(EHeroTattooStyle Style)
{
	switch (Style)
	{
	case EHeroTattooStyle::None:          return TEXT("None");
	case EHeroTattooStyle::NordicRunes:   return TEXT("Nordic War Runes");
	case EHeroTattooStyle::DragonScales:  return TEXT("Draconid Scale Marks");
	case EHeroTattooStyle::ArcaneSigils:  return TEXT("Arcane Invocation Sigils");
	case EHeroTattooStyle::CrimsonScar:   return TEXT("Crimson Battle Scars");
	case EHeroTattooStyle::TribalAsh:     return TEXT("Ash Caldera Warpaint");
	case EHeroTattooStyle::CelestialHalo: return TEXT("Celestial Crown Marks");
	default:                              return TEXT("None");
	}
}

FString UHeroCustomizationComponent::GetOutfitStyleLabel(EHeroOutfitStyle Style)
{
	switch (Style)
	{
	case EHeroOutfitStyle::VanguardPlate:   return TEXT("Vanguard Plate");
	case EHeroOutfitStyle::ArchmageRobes:   return TEXT("Archmage Vestments");
	case EHeroOutfitStyle::InquisitorCoat:  return TEXT("Royal Inquisitor");
	case EHeroOutfitStyle::DraconidWarlock: return TEXT("Ash Draconid Mail");
	case EHeroOutfitStyle::HighlandPelt:    return TEXT("Highland Berserker");
	case EHeroOutfitStyle::SovereignGilded: return TEXT("Sovereign Gilded Armor");
	default:                                return TEXT("Vanguard Plate");
	}
}

FString UHeroCustomizationComponent::GetDetailQualityLabel(EHeroDetailQuality Quality)
{
	switch (Quality)
	{
	case EHeroDetailQuality::Low:          return TEXT("Low (Performance)");
	case EHeroDetailQuality::Medium:       return TEXT("Medium");
	case EHeroDetailQuality::High:         return TEXT("High");
	case EHeroDetailQuality::Ultra:        return TEXT("Ultra PBR");
	case EHeroDetailQuality::CinematicRTX: return TEXT("Cinematic RTX");
	default:                               return TEXT("Ultra PBR");
	}
}

FString UHeroCustomizationComponent::GetAttributeLabel(EHeroAttribute Attr)
{
	switch (Attr)
	{
	case EHeroAttribute::Strength:     return TEXT("Strength (STR)");
	case EHeroAttribute::Endurance:    return TEXT("Endurance (END)");
	case EHeroAttribute::Agility:      return TEXT("Agility (AGI)");
	case EHeroAttribute::Intelligence: return TEXT("Intelligence (INT)");
	case EHeroAttribute::Spirit:       return TEXT("Spirit (SPI)");
	case EHeroAttribute::Fortune:      return TEXT("Fortune (LCK)");
	default:                           return TEXT("Attribute");
	}
}

FString UHeroCustomizationComponent::GetAttributeEffectHint(EHeroAttribute Attr)
{
	switch (Attr)
	{
	case EHeroAttribute::Strength:     return TEXT("+6% Melee Dmg · +12 Max HP · +4% Stagger per point");
	case EHeroAttribute::Endurance:    return TEXT("+10 Stamina · +1.5 Stam/s · +8 HP · +1.2% Armor · -1.5% Block Drain");
	case EHeroAttribute::Agility:      return TEXT("+2.8% Atk Spd · +14 Move Spd · +1.5% Crit · +3ms Dodge i-Frame");
	case EHeroAttribute::Intelligence: return TEXT("+6.5% Spell Power · +14 Max Mana · -1.5% Spell Cooldown");
	case EHeroAttribute::Spirit:       return TEXT("+1.4 Mana/s · +0.5 HP/s · +4% Dragonfire Power");
	case EHeroAttribute::Fortune:      return TEXT("+4% Crit Damage · +3% XP & Gold · +2.5ms Parry Window");
	default:                           return TEXT("");
	}
}

FLinearColor UHeroCustomizationComponent::GetHairColor(int32 Index)
{
	static const FLinearColor Palette[NUM_HAIR_COLORS] = {
		FLinearColor(0.05f, 0.04f, 0.04f, 1.0f), // Obsidian Black
		FLinearColor(0.24f, 0.14f, 0.08f, 1.0f), // Burnished Chestnut
		FLinearColor(0.68f, 0.52f, 0.24f, 1.0f), // Valenarian Gold
		FLinearColor(0.82f, 0.84f, 0.88f, 1.0f), // Aetherian Silver
		FLinearColor(0.56f, 0.14f, 0.07f, 1.0f), // Crimson Copper
		FLinearColor(0.32f, 0.34f, 0.38f, 1.0f), // Ash Iron
		FLinearColor(0.14f, 0.38f, 0.62f, 1.0f), // Tempest Sapphire
		FLinearColor(0.92f, 0.90f, 0.84f, 1.0f)  // Frost Platinum
	};
	const int32 SafeIdx = ((Index % NUM_HAIR_COLORS) + NUM_HAIR_COLORS) % NUM_HAIR_COLORS;
	return Palette[SafeIdx];
}

FString UHeroCustomizationComponent::GetHairColorName(int32 Index)
{
	static const TCHAR* Names[NUM_HAIR_COLORS] = {
		TEXT("Obsidian Black"),
		TEXT("Burnished Chestnut"),
		TEXT("Valenarian Gold"),
		TEXT("Aetherian Silver"),
		TEXT("Crimson Copper"),
		TEXT("Ash Iron"),
		TEXT("Tempest Sapphire"),
		TEXT("Frost Platinum")
	};
	const int32 SafeIdx = ((Index % NUM_HAIR_COLORS) + NUM_HAIR_COLORS) % NUM_HAIR_COLORS;
	return Names[SafeIdx];
}

FLinearColor UHeroCustomizationComponent::GetEyeColor(EHeroEyeStyle Style)
{
	switch (Style)
	{
	case EHeroEyeStyle::SteelBlue:    return FLinearColor(0.22f, 0.58f, 0.96f, 1.0f);
	case EHeroEyeStyle::MoltenAmber:  return FLinearColor(1.00f, 0.56f, 0.08f, 1.0f);
	case EHeroEyeStyle::ArcaneCyan:   return FLinearColor(0.08f, 0.94f, 1.00f, 1.0f);
	case EHeroEyeStyle::EmeraldJade:  return FLinearColor(0.16f, 0.92f, 0.45f, 1.0f);
	case EHeroEyeStyle::CrimsonFlame: return FLinearColor(1.00f, 0.14f, 0.08f, 1.0f);
	case EHeroEyeStyle::VoidViolet:   return FLinearColor(0.68f, 0.22f, 1.00f, 1.0f);
	case EHeroEyeStyle::SilverOracle: return FLinearColor(0.86f, 0.92f, 1.00f, 1.0f);
	case EHeroEyeStyle::SolarGold:    return FLinearColor(1.00f, 0.82f, 0.20f, 1.0f);
	default:                          return FLinearColor(1.00f, 0.56f, 0.08f, 1.0f);
	}
}

FLinearColor UHeroCustomizationComponent::GetTattooColor(int32 Index)
{
	static const FLinearColor Palette[NUM_TATTOO_COLORS] = {
		FLinearColor(0.06f, 0.88f, 1.00f, 1.0f), // Arcane Cyan
		FLinearColor(1.00f, 0.24f, 0.06f, 1.0f), // Exort Ember
		FLinearColor(0.95f, 0.78f, 0.22f, 1.0f), // Sovereign Gold
		FLinearColor(0.62f, 0.24f, 1.00f, 1.0f), // Umbral Violet
		FLinearColor(0.18f, 0.88f, 0.52f, 1.0f), // Jade Rune
		FLinearColor(0.12f, 0.12f, 0.14f, 1.0f)  // Charcoal Ink
	};
	const int32 SafeIdx = ((Index % NUM_TATTOO_COLORS) + NUM_TATTOO_COLORS) % NUM_TATTOO_COLORS;
	return Palette[SafeIdx];
}

FString UHeroCustomizationComponent::GetTattooColorName(int32 Index)
{
	static const TCHAR* Names[NUM_TATTOO_COLORS] = {
		TEXT("Arcane Cyan"),
		TEXT("Exort Ember"),
		TEXT("Sovereign Gold"),
		TEXT("Umbral Violet"),
		TEXT("Jade Rune"),
		TEXT("Charcoal Ink")
	};
	const int32 SafeIdx = ((Index % NUM_TATTOO_COLORS) + NUM_TATTOO_COLORS) % NUM_TATTOO_COLORS;
	return Names[SafeIdx];
}

FLinearColor UHeroCustomizationComponent::GetArmorTintColor(int32 Index)
{
	// Grounded PBR tint palette (luminance <= 0.88, never pure 1.0 white blasting over albedo)
	static const FLinearColor Palette[NUM_ARMOR_TINTS] = {
		FLinearColor(0.84f, 0.84f, 0.84f, 1.0f), // Sovereign Natural
		FLinearColor(0.72f, 0.16f, 0.12f, 1.0f), // Crimson Legion
		FLinearColor(0.16f, 0.38f, 0.78f, 1.0f), // Aetherian Cobalt
		FLinearColor(0.82f, 0.66f, 0.25f, 1.0f), // Gilded Sunsteel
		FLinearColor(0.20f, 0.22f, 0.26f, 1.0f), // Obsidian Steel
		FLinearColor(0.20f, 0.50f, 0.34f, 1.0f)  // Emerald Warden
	};
	const int32 SafeIdx = ((Index % NUM_ARMOR_TINTS) + NUM_ARMOR_TINTS) % NUM_ARMOR_TINTS;
	return Palette[SafeIdx];
}

FString UHeroCustomizationComponent::GetArmorTintName(int32 Index)
{
	static const TCHAR* Names[NUM_ARMOR_TINTS] = {
		TEXT("Sovereign Natural"),
		TEXT("Crimson Legion"),
		TEXT("Aetherian Cobalt"),
		TEXT("Gilded Sunsteel"),
		TEXT("Obsidian Steel"),
		TEXT("Emerald Warden")
	};
	const int32 SafeIdx = ((Index % NUM_ARMOR_TINTS) + NUM_ARMOR_TINTS) % NUM_ARMOR_TINTS;
	return Names[SafeIdx];
}

FLinearColor UHeroCustomizationComponent::GetRaceSkinTint(EHeroRace Race)
{
	// Grounded race skin tints (luminance <= 0.88, preserving hand-painted PBR pore/subsurface detail)
	switch (Race)
	{
	case EHeroRace::Human:        return FLinearColor(0.86f, 0.82f, 0.78f, 1.0f);
	case EHeroRace::HighElf:      return FLinearColor(0.80f, 0.83f, 0.88f, 1.0f);
	case EHeroRace::HighlandNord: return FLinearColor(0.86f, 0.79f, 0.75f, 1.0f);
	case EHeroRace::AshDraconid:  return FLinearColor(0.74f, 0.63f, 0.59f, 1.0f);
	case EHeroRace::ShadowKin:    return FLinearColor(0.62f, 0.64f, 0.76f, 1.0f);
	case EHeroRace::Ironborn:     return FLinearColor(0.80f, 0.76f, 0.72f, 1.0f);
	default:                      return FLinearColor(0.84f, 0.84f, 0.84f, 1.0f);
	}
}
