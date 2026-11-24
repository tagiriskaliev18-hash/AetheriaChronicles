// ============================================================================
// SpellSystemComponent.cpp — Quest-Gated 10-Spell Invoker System & Spell Actor
// ============================================================================
#include "SpellSystemComponent.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	struct FChronicleSpellAssetPaths
	{
		const TCHAR* NiagaraPath = nullptr;
		const TCHAR* CascadePath = nullptr;
		const TCHAR* ImpactCascadePath = nullptr;
		const TCHAR* CastSFXPath = nullptr;
		const TCHAR* ImpactSFXPath = nullptr;
		float BaseVFXScale = 1.5f;
	};

	template <typename TAsset>
	TAsset* LoadGuardedAsset(const TCHAR* ObjectPath)
	{
		if (!ObjectPath || !*ObjectPath)
		{
			return nullptr;
		}
		const FString PathStr(ObjectPath);
		const FString PackageName = FPackageName::ObjectPathToPackageName(PathStr);
		if (!FPackageName::DoesPackageExist(PackageName))
		{
			return nullptr;
		}
		return LoadObject<TAsset>(nullptr, *PathStr);
	}

	FChronicleSpellAssetPaths GetSpellAssetPaths(EChronicleSpellId SpellId)
	{
		switch (SpellId)
		{
		case EChronicleSpellId::SunStrike:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_fireStorm.P_ky_fireStorm"),
				nullptr,
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_GroundStrike_1.SW_MagicCast_GroundStrike_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Fire_1.SW_MagicImpact_Fire_1"),
				1.8f
			};
		case EChronicleSpellId::ChaosMeteor:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_shootingStar1.P_ky_shootingStar1"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_explosion.P_ky_explosion"),
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_ThrowHeavy_1.SW_MagicCast_ThrowHeavy_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Fire_1.SW_MagicImpact_Fire_1"),
				1.9f
			};
		case EChronicleSpellId::StormAlacrity:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Air.NS_Free_Spells_Buff_Air"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_magicCircle1.P_ky_magicCircle1"),
				nullptr,
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_Buff1_1.SW_MagicCast_Buff1_1"),
				nullptr,
				1.35f
			};
		case EChronicleSpellId::StormEMP:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Lightning.NS_Free_Spells_Aura_Lightning"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_thunderBall.P_ky_thunderBall"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_ThunderBallHit.P_ky_ThunderBallHit"),
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_ThrowLightningSpear_1.SW_MagicCast_ThrowLightningSpear_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_LightningStrikeMulti_1.SW_MagicImpact_LightningStrikeMulti_1"),
				1.75f
			};
		case EChronicleSpellId::ColdSnap:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_waterBall.P_ky_waterBall"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_waterBallHit.P_ky_waterBallHit"),
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_IceCrystal1_1.SW_MagicCast_IceCrystal1_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Crystal_1.SW_MagicImpact_Crystal_1"),
				1.25f
			};
		case EChronicleSpellId::ChronoBlink:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Soul.NS_Free_Spells_Aura_Soul"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_darkStorm.P_ky_darkStorm"),
				nullptr,
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_Buff1_1.SW_MagicCast_Buff1_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Water_1.SW_MagicImpact_Water_1"),
				1.45f
			};
		case EChronicleSpellId::IcePrismWall:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Circle.NS_Free_Spells_Circle"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_aquaStorm.P_ky_aquaStorm"),
				nullptr,
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_IceCrystal1_1.SW_MagicCast_IceCrystal1_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Crystal_1.SW_MagicImpact_Crystal_1"),
				1.65f
			};
		case EChronicleSpellId::GravityTornado:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Air.NS_Free_Spells_Aura_Air"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_storm.P_ky_storm"),
				nullptr,
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_CreatingBarrier1_1.SW_MagicCast_CreatingBarrier1_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Water_1.SW_MagicImpact_Water_1"),
				1.7f
			};
		case EChronicleSpellId::EmberConduit:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Fire.NS_Free_Spells_Aura_Fire"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_fireBall.P_ky_fireBall"),
				nullptr,
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_Buff1_1.SW_MagicCast_Buff1_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Barrier_1.SW_MagicImpact_Barrier_1"),
				1.6f
			};
		case EChronicleSpellId::PrimalBlast:
			return {
				TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave"),
				TEXT("/Game/FXVarietyPack/Particles/P_ky_shotShockwave.P_ky_shotShockwave"),
				nullptr,
				TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_GroundStrike_1.SW_MagicCast_GroundStrike_1"),
				TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Metallic_1.SW_MagicImpact_Metallic_1"),
				1.85f
			};
		default:
			return {};
		}
	}

	FString OrbToShortLetter(EChronicleOrb Orb)
	{
		switch (Orb)
		{
		case EChronicleOrb::Exort: return TEXT("E");
		case EChronicleOrb::Wex:   return TEXT("W");
		case EChronicleOrb::Quas:  return TEXT("Q");
		default:                   return TEXT("-");
		}
	}

	const TCHAR* OrbToDisplayName(EChronicleOrb Orb)
	{
		switch (Orb)
		{
		case EChronicleOrb::Exort: return TEXT("Exort (Solar Flame)");
		case EChronicleOrb::Wex:   return TEXT("Wex (Tempest Storm)");
		case EChronicleOrb::Quas:  return TEXT("Quas (Chrono Frost)");
		default:                   return TEXT("None");
		}
	}
}

// ============================================================================
// AChronicleSpellActor
// ============================================================================

AChronicleSpellActor::AChronicleSpellActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	InitialLifeSpan = 6.0f;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	PrimaryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrimaryMesh"));
	PrimaryMesh->SetupAttachment(RootScene);
	PrimaryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrimaryMesh->SetGenerateOverlapEvents(false);
	PrimaryMesh->SetCastShadow(false);

	SecondaryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SecondaryMesh"));
	SecondaryMesh->SetupAttachment(RootScene);
	SecondaryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SecondaryMesh->SetGenerateOverlapEvents(false);
	SecondaryMesh->SetCastShadow(false);

	// Physical AgX Lumen light — balanced transient spell flash (3,200 - 5,500 lm) so VFX read cleanly under EV100 exposure
	SpellLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SpellLight"));
	SpellLight->SetupAttachment(RootScene);
	SpellLight->SetIntensityUnits(ELightUnits::Lumens);
	SpellLight->SetIntensity(3600.f);
	SpellLight->SetAttenuationRadius(1400.f);
	SpellLight->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		SphereMeshAsset = SphereFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		CylinderMeshAsset = CylinderFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMeshAsset = CubeFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (EmissiveFinder.Succeeded())
	{
		BaseEmissiveMaterial = EmissiveFinder.Object;
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (BasicMatFinder.Succeeded())
		{
			BaseEmissiveMaterial = BasicMatFinder.Object;
		}
	}

	if (SphereMeshAsset)
	{
		PrimaryMesh->SetStaticMesh(SphereMeshAsset);
		SecondaryMesh->SetStaticMesh(SphereMeshAsset);
	}
}

void AChronicleSpellActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChronicleSpellActor, SpellId);
	DOREPLIFETIME(AChronicleSpellActor, DeliveryDirection);
	DOREPLIFETIME(AChronicleSpellActor, TargetWorldLocation);
	DOREPLIFETIME(AChronicleSpellActor, ChargeT01);
	DOREPLIFETIME(AChronicleSpellActor, PowerScale);
	DOREPLIFETIME(AChronicleSpellActor, SpeedScale);
	DOREPLIFETIME(AChronicleSpellActor, ControlScale);
}

void AChronicleSpellActor::InitializeSpell(
	EChronicleSpellId InSpellId,
	const FVector& InDeliveryDirection,
	const FVector& InTargetWorldLocation,
	float InChargeT01,
	float InPowerScale,
	float InSpeedScale,
	float InControlScale)
{
	SpellId = InSpellId;
	DeliveryDirection = InDeliveryDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	TargetWorldLocation = InTargetWorldLocation;
	ChargeT01 = FMath::Clamp(InChargeT01, 0.f, 1.f);
	PowerScale = FMath::Clamp(InPowerScale, 0.25f, 5.f);
	SpeedScale = FMath::Clamp(InSpeedScale, 0.25f, 4.f);
	ControlScale = FMath::Clamp(InControlScale, 0.25f, 4.f);
}

void AChronicleSpellActor::BeginPlay()
{
	Super::BeginPlay();

	if (SpellId != EChronicleSpellId::None && !bConfigured)
	{
		ConfigureSpellVisualsAndBehavior();
	}
}

void AChronicleSpellActor::OnRep_SpellConfig()
{
	if (SpellId != EChronicleSpellId::None && !bConfigured)
	{
		ConfigureSpellVisualsAndBehavior();
	}
}

void AChronicleSpellActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTemporarySpeedBuff();

	if (IsValid(ActiveNiagaraVFX))
	{
		ActiveNiagaraVFX->Deactivate();
	}
	ActiveNiagaraVFX = nullptr;

	if (IsValid(ActiveCascadeVFX))
	{
		ActiveCascadeVFX->DeactivateSystem();
	}
	ActiveCascadeVFX = nullptr;

	Super::EndPlay(EndPlayReason);
}

FCollisionQueryParams AChronicleSpellActor::BuildSpellQueryParams(const FName& TraceTag) const
{
	FCollisionQueryParams Params(TraceTag, false, this);
	if (const AActor* OwnerActor = GetOwner())
	{
		Params.AddIgnoredActor(OwnerActor);
	}
	if (const APawn* InstigatorPawn = GetInstigator())
	{
		Params.AddIgnoredActor(InstigatorPawn);
	}
	return Params;
}

bool AChronicleSpellActor::SnapPointToGround(const FVector& ReferencePoint, FVector& OutGroundPoint) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		OutGroundPoint = ReferencePoint;
		return false;
	}

	const FVector TraceStart = ReferencePoint + FVector(0.f, 0.f, 480.f);
	const FVector TraceEnd = ReferencePoint - FVector(0.f, 0.f, 1400.f);
	FHitResult GroundHit;
	const FCollisionQueryParams Params = BuildSpellQueryParams(TEXT("ChronicleSpellGroundSnap"));

	if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, CHRONICLE_TRACE_PROJECTILE, Params))
	{
		OutGroundPoint = GroundHit.ImpactPoint;
		return true;
	}

	OutGroundPoint = ReferencePoint;
	return false;
}

void AChronicleSpellActor::ApplyTemporarySpeedBuff(ACharacter* TargetChar, float SpeedBonusRatio)
{
	if (!HasAuthority() || !TargetChar)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = TargetChar->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	ClearTemporarySpeedBuff();
	const float Delta = FMath::Clamp(MoveComp->MaxWalkSpeed * SpeedBonusRatio, 60.f, 420.f);
	MoveComp->MaxWalkSpeed += Delta;
	AppliedSpeedDelta = Delta;
	BuffedCharacter = TargetChar;
}

void AChronicleSpellActor::ClearTemporarySpeedBuff()
{
	if (AppliedSpeedDelta <= 0.f)
	{
		return;
	}

	if (ACharacter* Char = BuffedCharacter.Get())
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = FMath::Max(150.f, MoveComp->MaxWalkSpeed - AppliedSpeedDelta);
		}
	}

	AppliedSpeedDelta = 0.f;
	BuffedCharacter.Reset();
}

void AChronicleSpellActor::UpdateEmissiveIntensity(float EmissiveScale)
{
	const float SafeScale = FMath::Max(0.f, EmissiveScale);
	const FLinearColor PrimaryHDR(
		CachedDef.PrimaryColor.R * SafeScale,
		CachedDef.PrimaryColor.G * SafeScale,
		CachedDef.PrimaryColor.B * SafeScale,
		1.f);

	const FLinearColor SecondaryTint(
		FMath::Lerp(CachedDef.PrimaryColor.R, 1.f, 0.35f) * SafeScale * 0.75f,
		FMath::Lerp(CachedDef.PrimaryColor.G, 1.f, 0.35f) * SafeScale * 0.75f,
		FMath::Lerp(CachedDef.PrimaryColor.B, 1.f, 0.35f) * SafeScale * 0.75f,
		1.f);

	if (PrimaryMID)
	{
		PrimaryMID->SetVectorParameterValue(TEXT("Color"), PrimaryHDR);
		PrimaryMID->SetVectorParameterValue(TEXT("EmissiveColor"), PrimaryHDR);
	}
	if (SecondaryMID)
	{
		SecondaryMID->SetVectorParameterValue(TEXT("Color"), SecondaryTint);
		SecondaryMID->SetVectorParameterValue(TEXT("EmissiveColor"), SecondaryTint);
	}
}

void AChronicleSpellActor::TriggerFlashBoost(float TargetPeakLumens)
{
	CurrentLightLumens = FMath::Clamp(TargetPeakLumens, 2600.f, 5500.f);
	if (SpellLight)
	{
		SpellLight->SetIntensity(CurrentLightLumens);
	}
}

void AChronicleSpellActor::UpdateLightFade(float DeltaSeconds)
{
	if (!SpellLight)
	{
		return;
	}

	const float LifeFrac = (TotalDuration > KINDA_SMALL_NUMBER)
		? FMath::Clamp(ElapsedTime / TotalDuration, 0.f, 1.f)
		: 1.f;

	// Smooth tail fade across final 35% of spell lifetime
	float FadeEnvelope = 1.f;
	if (LifeFrac > 0.65f)
	{
		const float TailAlpha = FMath::Clamp((1.f - LifeFrac) / 0.35f, 0.f, 1.f);
		FadeEnvelope = TailAlpha * TailAlpha * (3.f - 2.f * TailAlpha);
	}

	const float TargetLumens = BaseLightLumens * FadeEnvelope;
	CurrentLightLumens = FMath::FInterpTo(CurrentLightLumens, TargetLumens, DeltaSeconds, 5.5f);
	SpellLight->SetIntensity(FMath::Max(0.f, CurrentLightLumens));

	UpdateEmissiveIntensity(BaseEmissiveStrength * FMath::Max(0.05f, FadeEnvelope));
}

void AChronicleSpellActor::ConfigureSpellVisualsAndBehavior()
{
	bConfigured = true;
	CachedDef = USpellSystemComponent::GetSpellDef(SpellId);
	ElapsedTime = 0.f;
	PulseTimer = 0.f;
	bPrimaryDetonated = false;
	bMeteorLanded = false;
	bProjectileImpacted = false;
	bImpactVFXPlayed = false;
	SweptHitActors.Reset();

	const float ChargeScale = 1.f + 0.25f * ChargeT01;
	EffectiveDamage = CachedDef.BaseDamage * PowerScale * (1.f + 0.55f * ChargeT01);
	EffectiveRadius = CachedDef.BlastRadius * (1.f + 0.25f * ChargeT01);
	EffectiveImpulse = CachedDef.DestructionImpulse * PowerScale * (1.f + 0.45f * ChargeT01);

	// Create crisp emissive MIDs tinted by SpellDef.PrimaryColor
	if (BaseEmissiveMaterial)
	{
		PrimaryMID = UMaterialInstanceDynamic::Create(BaseEmissiveMaterial, this);
		SecondaryMID = UMaterialInstanceDynamic::Create(BaseEmissiveMaterial, this);
		if (PrimaryMesh && PrimaryMID)
		{
			PrimaryMesh->SetMaterial(0, PrimaryMID);
		}
		if (SecondaryMesh && SecondaryMID)
		{
			SecondaryMesh->SetMaterial(0, SecondaryMID);
		}
	}

	// Default physical AgX Lumen configuration (clamped to 3,200 - 5,500 lm for crisp EV100 VFX readability)
	BaseLightLumens = FMath::Clamp(3200.f + 1000.f * ChargeT01, 3200.f, 5500.f);
	PeakLightLumens = FMath::Clamp(4600.f + 900.f * ChargeT01, 3500.f, 5500.f);
	BaseEmissiveStrength = 6.5f + 2.5f * ChargeT01;

	PrimaryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SecondaryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrimaryMesh->SetRelativeLocation(FVector::ZeroVector);
	PrimaryMesh->SetRelativeRotation(FRotator::ZeroRotator);
	SecondaryMesh->SetRelativeLocation(FVector::ZeroVector);
	SecondaryMesh->SetRelativeRotation(FRotator::ZeroRotator);

	const FVector SafeDir = DeliveryDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	FVector FlatDir(SafeDir.X, SafeDir.Y, 0.f);
	if (!FlatDir.Normalize())
	{
		FlatDir = FVector::ForwardVector;
	}

	switch (SpellId)
	{
	case EChronicleSpellId::SunStrike:
	{
		FVector GroundLoc = TargetWorldLocation;
		SnapPointToGround(TargetWorldLocation, GroundLoc);
		SetActorLocation(GroundLoc);

		PrimaryMesh->SetStaticMesh(CylinderMeshAsset);
		SecondaryMesh->SetStaticMesh(CylinderMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(0.38f * ChargeScale, 0.38f * ChargeScale, 24.f));
		PrimaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 1200.f));
		SecondaryMesh->SetRelativeScale3D(FVector(3.8f * ChargeScale, 3.8f * ChargeScale, 0.08f));
		SecondaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 8.f));

		BaseLightLumens = FMath::Clamp(3600.f + 900.f * ChargeT01, 3200.f, 5500.f);
		PeakLightLumens = 5500.f;
		TotalDuration = FMath::Lerp(0.55f, 0.35f, ChargeT01) + 1.15f;
		break;
	}

	case EChronicleSpellId::ChaosMeteor:
	{
		FVector LandingGround = TargetWorldLocation;
		SnapPointToGround(TargetWorldLocation, LandingGround);
		MeteorLandingLocation = LandingGround;
		MeteorStartLocation = LandingGround - FlatDir * 440.f + FVector(0.f, 0.f, 1020.f);
		SetActorLocation(MeteorStartLocation);

		PrimaryMesh->SetStaticMesh(SphereMeshAsset);
		SecondaryMesh->SetStaticMesh(SphereMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(2.15f * ChargeScale));
		SecondaryMesh->SetRelativeScale3D(FVector(2.65f * ChargeScale));

		BaseLightLumens = FMath::Clamp(3800.f + 900.f * ChargeT01, 3200.f, 5500.f);
		PeakLightLumens = 5500.f;
		TravelSpeed = 740.f * SpeedScale;
		TotalDuration = 2.95f;
		break;
	}

	case EChronicleSpellId::StormAlacrity:
	{
		PrimaryMesh->SetStaticMesh(CylinderMeshAsset);
		SecondaryMesh->SetStaticMesh(SphereMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(1.85f * ChargeScale, 1.85f * ChargeScale, 0.08f));
		SecondaryMesh->SetRelativeScale3D(FVector(0.65f * ChargeScale));
		SecondaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 125.f));

		BaseLightLumens = FMath::Clamp(3200.f + 600.f * ChargeT01, 3200.f, 5500.f);
		TotalDuration = 5.5f * FMath::Clamp(ControlScale, 0.6f, 2.5f);

		if (HasAuthority())
		{
			ApplySpellBlastAndDestruction(GetActorLocation(), EffectiveRadius, EffectiveDamage, EffectiveImpulse, false, 160.f);
			ApplyTemporarySpeedBuff(Cast<ACharacter>(GetInstigator()), 0.30f + 0.15f * ChargeT01);
		}
		break;
	}

	case EChronicleSpellId::StormEMP:
	{
		FVector GroundLoc = TargetWorldLocation;
		SnapPointToGround(TargetWorldLocation, GroundLoc);
		SetActorLocation(GroundLoc + FVector(0.f, 0.f, 125.f));

		PrimaryMesh->SetStaticMesh(SphereMeshAsset);
		SecondaryMesh->SetStaticMesh(CylinderMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(1.25f * ChargeScale));
		SecondaryMesh->SetRelativeScale3D(FVector(3.2f * ChargeScale, 3.2f * ChargeScale, 0.10f));
		SecondaryMesh->SetRelativeRotation(FRotator(24.f, 0.f, 12.f));

		BaseLightLumens = FMath::Clamp(3400.f + 900.f * ChargeT01, 3200.f, 5500.f);
		PeakLightLumens = 5500.f;
		TotalDuration = FMath::Lerp(0.95f, 0.65f, ChargeT01) + 0.95f;
		break;
	}

	case EChronicleSpellId::ColdSnap:
	{
		PrimaryMesh->SetStaticMesh(CubeMeshAsset);
		SecondaryMesh->SetStaticMesh(SphereMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(1.65f * ChargeScale, 0.42f * ChargeScale, 0.42f * ChargeScale));
		SecondaryMesh->SetRelativeScale3D(FVector(0.95f * ChargeScale));
		SetActorRotation(SafeDir.Rotation());

		BaseLightLumens = FMath::Clamp(3200.f + 800.f * ChargeT01, 3200.f, 5500.f);
		PeakLightLumens = 4800.f;
		TravelSpeed = 2400.f * SpeedScale;
		TotalDuration = 2.4f;
		break;
	}

	case EChronicleSpellId::ChronoBlink:
	{
		PrimaryMesh->SetStaticMesh(CylinderMeshAsset);
		SecondaryMesh->SetStaticMesh(SphereMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(1.25f * ChargeScale, 1.25f * ChargeScale, 2.8f));
		PrimaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
		SecondaryMesh->SetRelativeScale3D(FVector(1.85f * ChargeScale));
		SecondaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 95.f));

		BaseLightLumens = FMath::Clamp(3300.f + 800.f * ChargeT01, 3200.f, 5500.f);
		TotalDuration = 1.45f * FMath::Clamp(ControlScale, 0.7f, 2.0f);

		if (HasAuthority())
		{
			const FVector DepartureLoc = GetActorLocation();
			if (APawn* InstigatorPawn = GetInstigator())
			{
				const UWorld* World = GetWorld();
				const float BlinkDist = (760.f + 220.f * ChargeT01) * FMath::Clamp(ControlScale, 0.7f, 1.8f);
				const FVector Start = InstigatorPawn->GetActorLocation();
				FVector Dest = Start + FlatDir * BlinkDist;

				if (World)
				{
					FHitResult BlinkHit;
					const FQuat CapsuleRot = FQuat::FindBetweenNormals(FVector::UpVector, FlatDir);
					const FCollisionQueryParams Params = BuildSpellQueryParams(TEXT("ChronicleChronoBlinkSweep"));
					if (World->SweepSingleByChannel(
							BlinkHit,
							Start,
							Dest,
							CapsuleRot,
							CHRONICLE_TRACE_PROJECTILE,
							FCollisionShape::MakeCapsule(42.f, 88.f),
							Params))
					{
						Dest = BlinkHit.Location - FlatDir * 55.f;
					}
				}

				FVector GroundDest = Dest;
				if (SnapPointToGround(Dest, GroundDest))
				{
					Dest.Z = GroundDest.Z + 96.f;
				}
				InstigatorPawn->TeleportTo(Dest, FlatDir.Rotation(), false, true);
				ApplyTemporarySpeedBuff(Cast<ACharacter>(InstigatorPawn), 0.25f);
			}

			// Departure cryo-rift nova chills enemies left behind
			ApplySpellBlastAndDestruction(DepartureLoc, EffectiveRadius, EffectiveDamage, EffectiveImpulse, false, 120.f);
		}
		break;
	}

	case EChronicleSpellId::IcePrismWall:
	{
		FVector GroundLoc = TargetWorldLocation;
		SnapPointToGround(TargetWorldLocation, GroundLoc);
		SetActorLocation(GroundLoc);
		SetActorRotation(FlatDir.Rotation());

		PrimaryMesh->SetStaticMesh(CubeMeshAsset);
		SecondaryMesh->SetStaticMesh(CubeMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(0.85f, 5.8f * ChargeScale, 2.75f * ChargeScale));
		PrimaryMesh->SetRelativeLocation(FVector(0.f, 0.f, -140.f)); // Rises up during initial 0.25s
		SecondaryMesh->SetRelativeScale3D(FVector(2.5f, 6.2f * ChargeScale, 0.14f));
		SecondaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 7.f));

		// Solid crystalline barrier that blocks enemies & projectiles while letting camera pass
		PrimaryMesh->bDisallowNanite = true;
		SecondaryMesh->bDisallowNanite = true;
		PrimaryMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PrimaryMesh->SetCollisionObjectType(ECC_WorldDynamic);
		PrimaryMesh->SetCollisionResponseToAllChannels(ECR_Block);
		PrimaryMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

		BaseLightLumens = FMath::Clamp(3300.f + 800.f * ChargeT01, 3200.f, 5500.f);
		TotalDuration = 4.8f * FMath::Clamp(ControlScale, 0.7f, 2.0f);

		if (HasAuthority())
		{
			ApplySpellBlastAndDestruction(GroundLoc + FVector(0.f, 0.f, 80.f), EffectiveRadius, EffectiveDamage, EffectiveImpulse, false, 180.f);
		}
		break;
	}

	case EChronicleSpellId::GravityTornado:
	{
		FVector GroundLoc = GetActorLocation();
		SnapPointToGround(GroundLoc, GroundLoc);
		SetActorLocation(GroundLoc);
		SetActorRotation(FlatDir.Rotation());

		PrimaryMesh->SetStaticMesh(CylinderMeshAsset);
		SecondaryMesh->SetStaticMesh(SphereMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(1.35f * ChargeScale, 1.35f * ChargeScale, 5.4f * ChargeScale));
		PrimaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
		SecondaryMesh->SetRelativeScale3D(FVector(2.8f * ChargeScale, 2.8f * ChargeScale, 0.85f * ChargeScale));
		SecondaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 470.f));

		BaseLightLumens = FMath::Clamp(3500.f + 800.f * ChargeT01, 3200.f, 5500.f);
		TravelSpeed = 1280.f * SpeedScale;
		TotalDuration = 2.6f * FMath::Clamp(ControlScale, 0.75f, 1.8f);
		break;
	}

	case EChronicleSpellId::EmberConduit:
	{
		FVector GroundLoc = TargetWorldLocation;
		SnapPointToGround(TargetWorldLocation, GroundLoc);
		SetActorLocation(GroundLoc);

		PrimaryMesh->SetStaticMesh(CubeMeshAsset);
		SecondaryMesh->SetStaticMesh(CylinderMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(1.05f * ChargeScale, 1.05f * ChargeScale, 3.2f * ChargeScale));
		PrimaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
		SecondaryMesh->SetRelativeScale3D(FVector(4.4f * ChargeScale, 4.4f * ChargeScale, 0.12f));
		SecondaryMesh->SetRelativeLocation(FVector(0.f, 0.f, 8.f));

		BaseLightLumens = FMath::Clamp(3700.f + 900.f * ChargeT01, 3200.f, 5500.f);
		PeakLightLumens = 5200.f;
		TotalDuration = 3.2f * FMath::Clamp(ControlScale, 0.75f, 1.8f);

		if (HasAuthority())
		{
			ApplySpellBlastAndDestruction(GroundLoc + FVector(0.f, 0.f, 60.f), EffectiveRadius, EffectiveDamage * 0.55f, EffectiveImpulse * 0.75f, false, 160.f);
		}
		break;
	}

	case EChronicleSpellId::PrimalBlast:
	default:
	{
		SetActorRotation(SafeDir.Rotation());
		PrimaryMesh->SetStaticMesh(CubeMeshAsset);
		SecondaryMesh->SetStaticMesh(SphereMeshAsset);
		PrimaryMesh->SetRelativeScale3D(FVector(0.9f, 2.2f * ChargeScale, 2.4f * ChargeScale));
		SecondaryMesh->SetRelativeScale3D(FVector(1.85f * ChargeScale));

		BaseLightLumens = FMath::Clamp(4200.f + 900.f * ChargeT01, 3200.f, 5500.f);
		PeakLightLumens = 5500.f;
		TravelSpeed = 1750.f * SpeedScale;
		TotalDuration = 1.15f;
		break;
	}
	}

	CurrentLightLumens = BaseLightLumens;
	if (SpellLight)
	{
		SpellLight->SetIntensityUnits(ELightUnits::Lumens);
		SpellLight->SetIntensity(CurrentLightLumens);
		SpellLight->SetAttenuationRadius(FMath::Max(1400.f, EffectiveRadius * 2.8f));
		SpellLight->SetLightColor(CachedDef.PrimaryColor, true);
	}

	UpdateEmissiveIntensity(BaseEmissiveStrength);
	SpawnRealSpellVFXAndCastSFX(ChargeScale);
	SetLifeSpan(TotalDuration + 0.1f);
}

void AChronicleSpellActor::SpawnRealSpellVFXAndCastSFX(float ChargeScale)
{
	if (IsValid(ActiveNiagaraVFX))
	{
		ActiveNiagaraVFX->Deactivate();
		ActiveNiagaraVFX = nullptr;
	}
	if (IsValid(ActiveCascadeVFX))
	{
		ActiveCascadeVFX->DeactivateSystem();
		ActiveCascadeVFX = nullptr;
	}

	if (IsRunningDedicatedServer() || !RootScene)
	{
		return;
	}

	const FChronicleSpellAssetPaths Paths = GetSpellAssetPaths(SpellId);
	const float VFXScale = FMath::Clamp(Paths.BaseVFXScale * ChargeScale, 1.2f, 2.4f);

	if (UNiagaraSystem* NiagaraAsset = LoadGuardedAsset<UNiagaraSystem>(Paths.NiagaraPath))
	{
		ActiveNiagaraVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraAsset,
			RootScene,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			FVector(VFXScale),
			EAttachLocation::KeepRelativeOffset,
			true,
			ENCPoolMethod::None,
			true,
			true);
	}

	if (UParticleSystem* CascadeAsset = LoadGuardedAsset<UParticleSystem>(Paths.CascadePath))
	{
		ActiveCascadeVFX = UGameplayStatics::SpawnEmitterAttached(
			CascadeAsset,
			RootScene,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			FVector(VFXScale),
			EAttachLocation::KeepRelativeOffset,
			true,
			EPSCPoolMethod::None,
			true);
	}

	if (USoundBase* CastSFX = LoadGuardedAsset<USoundBase>(Paths.CastSFXPath))
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastSFX, GetActorLocation());
	}

	// Hide ugly BasicShapes fallback meshes whenever real Niagara or Cascade loaded cleanly,
	// except IcePrismWall which relies on its solid crystalline barrier mesh (with bDisallowNanite=true).
	const bool bHasRealVFX = (ActiveNiagaraVFX != nullptr) || (ActiveCascadeVFX != nullptr);
	if (bHasRealVFX && SpellId != EChronicleSpellId::IcePrismWall)
	{
		if (PrimaryMesh)
		{
			PrimaryMesh->SetVisibility(false);
		}
		if (SecondaryMesh)
		{
			SecondaryMesh->SetVisibility(false);
		}
	}
}

void AChronicleSpellActor::PlaySpellImpactVFXAndSFX(const FVector& Origin)
{
	if (bImpactVFXPlayed || IsRunningDedicatedServer())
	{
		return;
	}

	bImpactVFXPlayed = true;
	const FChronicleSpellAssetPaths Paths = GetSpellAssetPaths(SpellId);
	const float ImpactScale = FMath::Clamp(Paths.BaseVFXScale * (1.f + 0.25f * ChargeT01), 1.2f, 2.4f);

	if (UWorld* World = GetWorld())
	{
		if (UParticleSystem* ImpactCascade = LoadGuardedAsset<UParticleSystem>(Paths.ImpactCascadePath))
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				ImpactCascade,
				Origin,
				GetActorRotation(),
				FVector(ImpactScale),
				true);
		}
	}

	if (USoundBase* ImpactSFX = LoadGuardedAsset<USoundBase>(Paths.ImpactSFXPath))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSFX, Origin);
	}
}

void AChronicleSpellActor::ApplySpellBlastAndDestruction(
	const FVector& Origin,
	float Radius,
	float DamageAmount,
	float ImpulseStrength,
	bool bPullInward,
	float VerticalLaunch)
{
	if (!HasAuthority())
	{
		return;
	}

	PlaySpellImpactVFXAndSFX(Origin);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float SafeRadius = FMath::Max(120.f, Radius);
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);
	if (AActor* OwnerActor = GetOwner())
	{
		IgnoredActors.AddUnique(OwnerActor);
	}
	if (APawn* InstigatorPawn = GetInstigator())
	{
		IgnoredActors.AddUnique(InstigatorPawn);
	}

	// 1) Apply radial damage first so dynamic actors & destructibles (e.g. ADestructiblePropActor)
	//    fracture immediately. Always pass CHRONICLE_TRACE_PROJECTILE — never default ECC_Visibility.
	if (DamageAmount > 0.f)
	{
		UGameplayStatics::ApplyRadialDamage(
			this,
			DamageAmount,
			Origin,
			SafeRadius,
			UDamageType::StaticClass(),
			IgnoredActors,
			this,
			GetInstigatorController(),
			false,
			CHRONICLE_TRACE_PROJECTILE);
	}

	const FCollisionQueryParams QueryParams = BuildSpellQueryParams(TEXT("ChronicleSpellBlastOverlap"));
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(SafeRadius);

	// 2) Channel overlap on CHRONICLE_TRACE_PROJECTILE: catches ECC_WorldStatic destructible pillars
	//    that ApplyRadialDamage's AllDynamicObjects query does not include, fracturing them before impulse.
	TArray<FOverlapResult> ChannelOverlaps;
	World->OverlapMultiByChannel(
		ChannelOverlaps,
		Origin,
		FQuat::Identity,
		CHRONICLE_TRACE_PROJECTILE,
		SphereShape,
		QueryParams);

	if (DamageAmount > 0.f)
	{
		TSet<AActor*> DamagedStaticActors;
		for (const FOverlapResult& Result : ChannelOverlaps)
		{
			UPrimitiveComponent* PrimComp = Result.GetComponent();
			AActor* HitActor = Result.GetActor();
			if (!PrimComp || !HitActor || IgnoredActors.Contains(HitActor) || DamagedStaticActors.Contains(HitActor))
			{
				continue;
			}
			if (PrimComp->GetCollisionObjectType() == ECC_WorldStatic && HitActor->CanBeDamaged())
			{
				DamagedStaticActors.Add(HitActor);
				const float Dist = FVector::Dist(Origin, HitActor->GetActorLocation());
				const float Falloff = FMath::Clamp(1.f - (Dist / SafeRadius) * 0.5f, 0.25f, 1.f);
				UGameplayStatics::ApplyDamage(
					HitActor,
					DamageAmount * Falloff,
					GetInstigatorController(),
					this,
					UDamageType::StaticClass());
			}
		}
	}

	// 3) Apply radial physics impulse to nearby simulating/destructible primitives (including freshly fractured debris)
	FCollisionObjectQueryParams PhysicsObjParams;
	PhysicsObjParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	PhysicsObjParams.AddObjectTypesToQuery(ECC_Destructible);
	PhysicsObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	PhysicsObjParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps = MoveTemp(ChannelOverlaps);
	TArray<FOverlapResult> PhysicsOverlaps;
	World->OverlapMultiByObjectType(
		PhysicsOverlaps,
		Origin,
		FQuat::Identity,
		PhysicsObjParams,
		SphereShape,
		QueryParams);

	Overlaps.Append(PhysicsOverlaps);

	TSet<UPrimitiveComponent*> PulsedPrimitives;
	TSet<AActor*> LaunchedActors;
	const float SignedVelImpulse = (bPullInward ? -1.f : 1.f) * FMath::Clamp(ImpulseStrength * 0.35f, 320.f, 1300.f);

	for (const FOverlapResult& Result : Overlaps)
	{
		UPrimitiveComponent* PrimComp = Result.GetComponent();
		AActor* HitActor = Result.GetActor();

		if (PrimComp && !PulsedPrimitives.Contains(PrimComp))
		{
			PulsedPrimitives.Add(PrimComp);
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->AddRadialImpulse(
					Origin,
					SafeRadius,
					SignedVelImpulse,
					ERadialImpulseFalloff::RIF_Linear,
					true);
			}
		}

		if (HitActor && !IgnoredActors.Contains(HitActor) && !LaunchedActors.Contains(HitActor))
		{
			if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
			{
				LaunchedActors.Add(HitActor);
				FVector PushDir = HitChar->GetActorLocation() - Origin;
				PushDir.Z = 0.f;
				if (!PushDir.Normalize())
				{
					PushDir = DeliveryDirection.GetSafeNormal2D();
				}
				if (bPullInward)
				{
					PushDir = -PushDir;
				}
				const float HorizontalSpeed = FMath::Clamp(ImpulseStrength * 0.22f, 180.f, 820.f);
				HitChar->LaunchCharacter(PushDir * HorizontalSpeed + FVector(0.f, 0.f, VerticalLaunch), false, VerticalLaunch > 0.f);
			}
		}
	}
}

void AChronicleSpellActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bConfigured)
	{
		if (SpellId != EChronicleSpellId::None)
		{
			ConfigureSpellVisualsAndBehavior();
		}
		else
		{
			return;
		}
	}

	ElapsedTime += DeltaSeconds;
	PulseTimer += DeltaSeconds;

	UWorld* World = GetWorld();
	const float ChargeScale = 1.f + 0.25f * ChargeT01;
	const FVector SafeDir = DeliveryDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	FVector FlatDir(SafeDir.X, SafeDir.Y, 0.f);
	if (!FlatDir.Normalize())
	{
		FlatDir = FVector::ForwardVector;
	}

	switch (SpellId)
	{
	case EChronicleSpellId::SunStrike:
	{
		const float TelegraphDuration = FMath::Lerp(0.55f, 0.35f, ChargeT01);
		if (!bPrimaryDetonated)
		{
			const float T = FMath::Clamp(ElapsedTime / FMath::Max(0.1f, TelegraphDuration), 0.f, 1.f);
			const float BeamWidth = FMath::Lerp(0.28f, 0.85f, T) * ChargeScale;
			PrimaryMesh->SetRelativeScale3D(FVector(BeamWidth, BeamWidth, 24.f));
			SecondaryMesh->AddLocalRotation(FRotator(0.f, 220.f * DeltaSeconds, 0.f));

			if (ElapsedTime >= TelegraphDuration)
			{
				bPrimaryDetonated = true;
				TriggerFlashBoost(PeakLightLumens);
				PlaySpellImpactVFXAndSFX(GetActorLocation());
				ApplySpellBlastAndDestruction(GetActorLocation(), EffectiveRadius, EffectiveDamage, EffectiveImpulse, false, 340.f);
			}
		}
		else
		{
			const float PostT = FMath::Clamp((ElapsedTime - TelegraphDuration) / FMath::Max(0.1f, TotalDuration - TelegraphDuration), 0.f, 1.f);
			const float BlastWidth = FMath::Lerp(4.2f, 0.35f, PostT) * ChargeScale;
			const float RingScale = FMath::Lerp(4.2f, 9.5f, PostT) * ChargeScale;
			PrimaryMesh->SetRelativeScale3D(FVector(BlastWidth, BlastWidth, 26.f));
			SecondaryMesh->SetRelativeScale3D(FVector(RingScale, RingScale, 0.08f));
		}
		break;
	}

	case EChronicleSpellId::ChaosMeteor:
	{
		const float DescentDuration = 0.48f;
		PrimaryMesh->AddLocalRotation(FRotator(-540.f * DeltaSeconds, 90.f * DeltaSeconds, 0.f));
		SecondaryMesh->AddLocalRotation(FRotator(240.f * DeltaSeconds, -300.f * DeltaSeconds, 120.f * DeltaSeconds));

		if (!bMeteorLanded)
		{
			const float Alpha = FMath::Clamp(ElapsedTime / DescentDuration, 0.f, 1.f);
			const FVector Pos = FMath::Lerp(MeteorStartLocation, MeteorLandingLocation + FVector(0.f, 0.f, 105.f), Alpha * Alpha);
			SetActorLocation(Pos);

			if (ElapsedTime >= DescentDuration)
			{
				bMeteorLanded = true;
				PulseTimer = 0.f;
				TriggerFlashBoost(PeakLightLumens);
				PlaySpellImpactVFXAndSFX(GetActorLocation());
				ApplySpellBlastAndDestruction(GetActorLocation(), EffectiveRadius, EffectiveDamage, EffectiveImpulse, false, 260.f);
			}
		}
		else
		{
			const FVector PrevLoc = GetActorLocation();
			FVector NextLoc = PrevLoc + FlatDir * (TravelSpeed * DeltaSeconds);

			if (World)
			{
				FHitResult RollHit;
				const FQuat CapsuleRot = FQuat::FindBetweenNormals(FVector::UpVector, FlatDir);
				const FCollisionQueryParams Params = BuildSpellQueryParams(TEXT("ChronicleMeteorRoll"));
				if (World->SweepSingleByChannel(
						RollHit,
						PrevLoc,
						NextLoc,
						CapsuleRot,
						CHRONICLE_TRACE_PROJECTILE,
						FCollisionShape::MakeCapsule(95.f, 115.f),
						Params))
				{
					// Blast destructible barrier in meteor's path
					if (HasAuthority() && PulseTimer >= 0.12f)
					{
						PulseTimer = 0.f;
						TriggerFlashBoost(5000.f);
						ApplySpellBlastAndDestruction(RollHit.ImpactPoint, EffectiveRadius * 0.8f, EffectiveDamage * 0.45f, EffectiveImpulse * 0.85f, false, 180.f);
					}
					NextLoc = RollHit.Location + RollHit.ImpactNormal * 12.f;
				}
			}

			FVector GroundLoc = NextLoc;
			if (SnapPointToGround(NextLoc, GroundLoc))
			{
				NextLoc.Z = GroundLoc.Z + 105.f;
			}
			SetActorLocation(NextLoc);

			if (PulseTimer >= 0.36f)
			{
				PulseTimer = 0.f;
				ApplySpellBlastAndDestruction(NextLoc, EffectiveRadius * 0.78f, EffectiveDamage * 0.38f, EffectiveImpulse * 0.7f, false, 160.f);
			}
		}
		break;
	}

	case EChronicleSpellId::StormAlacrity:
	{
		if (APawn* InstigatorPawn = GetInstigator())
		{
			SetActorLocation(InstigatorPawn->GetActorLocation());
		}
		PrimaryMesh->AddLocalRotation(FRotator(0.f, 380.f * DeltaSeconds, 0.f));
		const float BobZ = 125.f + FMath::Sin(ElapsedTime * 7.5f) * 14.f;
		SecondaryMesh->SetRelativeLocation(FVector(0.f, 0.f, BobZ));
		break;
	}

	case EChronicleSpellId::StormEMP:
	{
		const float ChargeDelay = FMath::Lerp(0.95f, 0.65f, ChargeT01);
		SecondaryMesh->AddLocalRotation(FRotator(0.f, 480.f * DeltaSeconds, 190.f * DeltaSeconds));

		if (!bPrimaryDetonated)
		{
			const float T = FMath::Clamp(ElapsedTime / FMath::Max(0.1f, ChargeDelay), 0.f, 1.f);
			const float PulseScale = (1.1f + 1.35f * T + 0.18f * FMath::Sin(ElapsedTime * 22.f)) * ChargeScale;
			PrimaryMesh->SetRelativeScale3D(FVector(PulseScale));

			if (ElapsedTime >= ChargeDelay)
			{
				bPrimaryDetonated = true;
				TriggerFlashBoost(PeakLightLumens);
				PlaySpellImpactVFXAndSFX(GetActorLocation());
				ApplySpellBlastAndDestruction(GetActorLocation(), EffectiveRadius, EffectiveDamage, EffectiveImpulse, false, 220.f);

				// Classic Invoker EMP mana siphon back to caster
				if (HasAuthority())
				{
					if (AActor* OwnerActor = GetOwner())
					{
						if (USpellSystemComponent* SpellComp = OwnerActor->FindComponentByClass<USpellSystemComponent>())
						{
							SpellComp->RestoreMana(24.f * SpeedScale);
						}
					}
				}
			}
		}
		else
		{
			const float PostT = FMath::Clamp((ElapsedTime - ChargeDelay) / FMath::Max(0.1f, TotalDuration - ChargeDelay), 0.f, 1.f);
			const float NovaScale = FMath::Lerp(2.4f, (EffectiveRadius / 55.f), PostT);
			PrimaryMesh->SetRelativeScale3D(FVector(NovaScale));
			SecondaryMesh->SetRelativeScale3D(FVector(NovaScale * 1.15f, NovaScale * 1.15f, 0.12f));
		}
		break;
	}

	case EChronicleSpellId::ColdSnap:
	{
		if (!bProjectileImpacted)
		{
			const FVector Start = GetActorLocation();
			const FVector End = Start + SafeDir * (TravelSpeed * DeltaSeconds);
			PrimaryMesh->AddLocalRotation(FRotator(0.f, 0.f, 520.f * DeltaSeconds));

			if (World)
			{
				FHitResult ImpactHit;
				const FQuat CapsuleRot = FQuat::FindBetweenNormals(FVector::UpVector, SafeDir);
				const FCollisionQueryParams Params = BuildSpellQueryParams(TEXT("ChronicleColdSnapSweep"));
				if (World->SweepSingleByChannel(
						ImpactHit,
						Start,
						End,
						CapsuleRot,
						CHRONICLE_TRACE_PROJECTILE,
						FCollisionShape::MakeCapsule(42.f, 64.f),
						Params))
				{
					bProjectileImpacted = true;
					SetActorLocation(ImpactHit.ImpactPoint);
					TriggerFlashBoost(PeakLightLumens);
					PlaySpellImpactVFXAndSFX(ImpactHit.ImpactPoint);

					PrimaryMesh->SetStaticMesh(SphereMeshAsset);
					PrimaryMesh->SetRelativeScale3D(FVector(2.2f * ChargeScale));

					if (HasAuthority())
					{
						if (AActor* StruckActor = ImpactHit.GetActor())
						{
							UGameplayStatics::ApplyDamage(
								StruckActor,
								EffectiveDamage,
								GetInstigatorController(),
								this,
								UDamageType::StaticClass());
							ColdSnapTarget = StruckActor;
							ColdSnapRemainingProcs = 2;
							PulseTimer = 0.f;
						}
						ApplySpellBlastAndDestruction(ImpactHit.ImpactPoint, EffectiveRadius, EffectiveDamage * 0.55f, EffectiveImpulse, false, 110.f);
					}

					TotalDuration = ElapsedTime + 1.1f;
					SetLifeSpan(1.2f);
				}
				else
				{
					SetActorLocation(End);
				}
			}
			else
			{
				SetActorLocation(End);
			}
		}
		else
		{
			// Follow-up signature Cold Snap frost ticks on struck target
			if (AActor* Target = ColdSnapTarget.Get())
			{
				SetActorLocation(Target->GetActorLocation() + FVector(0.f, 0.f, 60.f));
				if (HasAuthority() && ColdSnapRemainingProcs > 0 && PulseTimer >= 0.42f)
				{
					PulseTimer = 0.f;
					--ColdSnapRemainingProcs;
					TriggerFlashBoost(4200.f);
					UGameplayStatics::ApplyDamage(
						Target,
						EffectiveDamage * 0.35f,
						GetInstigatorController(),
						this,
						UDamageType::StaticClass());
				}
			}
			const float ShatterScale = FMath::Max(0.4f, PrimaryMesh->GetRelativeScale3D().X - DeltaSeconds * 1.4f);
			PrimaryMesh->SetRelativeScale3D(FVector(ShatterScale));
		}
		break;
	}

	case EChronicleSpellId::ChronoBlink:
	{
		PrimaryMesh->AddLocalRotation(FRotator(0.f, 260.f * DeltaSeconds, 0.f));
		const float LifeFrac = FMath::Clamp(ElapsedTime / FMath::Max(0.1f, TotalDuration), 0.f, 1.f);
		const float RingExpand = FMath::Lerp(1.85f, 4.6f, LifeFrac) * ChargeScale;
		SecondaryMesh->SetRelativeScale3D(FVector(RingExpand, RingExpand, 0.35f));
		break;
	}

	case EChronicleSpellId::IcePrismWall:
	{
		const float RiseAlpha = FMath::Clamp(ElapsedTime / 0.25f, 0.f, 1.f);
		const float SinkAlpha = (ElapsedTime > TotalDuration - 0.45f)
			? FMath::Clamp((TotalDuration - ElapsedTime) / 0.45f, 0.f, 1.f)
			: 1.f;
		const float TargetZ = FMath::Lerp(-140.f, 125.f * ChargeScale, RiseAlpha * SinkAlpha);
		PrimaryMesh->SetRelativeLocation(FVector(0.f, 0.f, TargetZ));

		if (HasAuthority() && PulseTimer >= 0.65f)
		{
			PulseTimer = 0.f;
			ApplySpellBlastAndDestruction(GetActorLocation() + FVector(0.f, 0.f, 75.f), EffectiveRadius * 0.85f, EffectiveDamage * 0.35f, EffectiveImpulse * 0.4f, false, 60.f);
		}
		break;
	}

	case EChronicleSpellId::GravityTornado:
	{
		PrimaryMesh->AddLocalRotation(FRotator(0.f, 760.f * DeltaSeconds, 0.f));
		SecondaryMesh->AddLocalRotation(FRotator(0.f, -540.f * DeltaSeconds, 0.f));

		const FVector PrevLoc = GetActorLocation();
		FVector NextLoc = PrevLoc + FlatDir * (TravelSpeed * DeltaSeconds);

		if (World)
		{
			FHitResult TornadoHit;
			const FQuat CapsuleRot = FQuat::FindBetweenNormals(FVector::UpVector, FlatDir);
			const FCollisionQueryParams Params = BuildSpellQueryParams(TEXT("ChronicleTornadoSweep"));
			if (World->SweepSingleByChannel(
					TornadoHit,
					PrevLoc + FVector(0.f, 0.f, 120.f),
					NextLoc + FVector(0.f, 0.f, 120.f),
					CapsuleRot,
					CHRONICLE_TRACE_PROJECTILE,
					FCollisionShape::MakeCapsule(90.f, 130.f),
					Params))
			{
				// Deflect slightly around immovable cliffs while still lifting nearby foes
				NextLoc = TornadoHit.Location - FVector(0.f, 0.f, 120.f) + TornadoHit.ImpactNormal * 20.f;
			}
		}

		FVector GroundLoc = NextLoc;
		if (SnapPointToGround(NextLoc, GroundLoc))
		{
			NextLoc.Z = GroundLoc.Z;
		}
		SetActorLocation(NextLoc);

		if (PulseTimer >= 0.24f)
		{
			PulseTimer = 0.f;
			ApplySpellBlastAndDestruction(NextLoc + FVector(0.f, 0.f, 140.f), EffectiveRadius, EffectiveDamage * 0.36f, EffectiveImpulse * 0.85f, true, 480.f);
		}
		break;
	}

	case EChronicleSpellId::EmberConduit:
	{
		PrimaryMesh->AddLocalRotation(FRotator(0.f, 145.f * DeltaSeconds, 0.f));
		const float HoverZ = 180.f + FMath::Sin(ElapsedTime * 4.5f) * 18.f;
		PrimaryMesh->SetRelativeLocation(FVector(0.f, 0.f, HoverZ));

		if (PulseTimer >= 0.68f)
		{
			PulseTimer = 0.f;
			TriggerFlashBoost(PeakLightLumens);
			ApplySpellBlastAndDestruction(GetActorLocation() + FVector(0.f, 0.f, 65.f), EffectiveRadius, EffectiveDamage * 0.45f, EffectiveImpulse * 0.75f, false, 180.f);
		}
		break;
	}

	case EChronicleSpellId::PrimalBlast:
	default:
	{
		const float LifeFrac = FMath::Clamp(ElapsedTime / FMath::Max(0.1f, TotalDuration), 0.f, 1.f);
		const float WaveWidthScale = FMath::Lerp(2.2f, 7.8f, LifeFrac) * ChargeScale;
		PrimaryMesh->SetRelativeScale3D(FVector(0.95f, WaveWidthScale, 2.4f * ChargeScale));
		SecondaryMesh->AddLocalRotation(FRotator(260.f * DeltaSeconds, 420.f * DeltaSeconds, 0.f));

		const FVector Start = GetActorLocation();
		const FVector End = Start + SafeDir * (TravelSpeed * DeltaSeconds);

		if (World && HasAuthority())
		{
			const float CapsuleRadius = FMath::Clamp(WaveWidthScale * 22.f, 75.f, 190.f);
			const float CapsuleHalfHeight = FMath::Max(CapsuleRadius + 15.f, WaveWidthScale * 36.f);
			const FQuat CapsuleRot = FQuat::FindBetweenNormals(FVector::UpVector, SafeDir);
			const FCollisionQueryParams Params = BuildSpellQueryParams(TEXT("ChroniclePrimalBlastSweep"));

			TArray<FHitResult> WaveHits;
			World->SweepMultiByChannel(
				WaveHits,
				Start,
				End,
				CapsuleRot,
				CHRONICLE_TRACE_PROJECTILE,
				FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
				Params);

			bool bBlastedDestructiblesThisStep = false;
			for (const FHitResult& Hit : WaveHits)
			{
				AActor* HitActor = Hit.GetActor();
				const TWeakObjectPtr<AActor> HitWeak(HitActor);
				if (!HitActor || SweptHitActors.Contains(HitWeak))
				{
					continue;
				}
				SweptHitActors.Add(HitWeak);

				UGameplayStatics::ApplyDamage(
					HitActor,
					EffectiveDamage,
					GetInstigatorController(),
					this,
					UDamageType::StaticClass());

				if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
				{
					HitChar->LaunchCharacter(FlatDir * 820.f + FVector(0.f, 0.f, 280.f), true, true);
				}

				if (!bBlastedDestructiblesThisStep)
				{
					bBlastedDestructiblesThisStep = true;
					TriggerFlashBoost(PeakLightLumens);
					ApplySpellBlastAndDestruction(Hit.ImpactPoint, EffectiveRadius * 0.72f, EffectiveDamage * 0.35f, EffectiveImpulse, false, 240.f);
				}
			}
		}

		SetActorLocation(End);
		break;
	}
	}

	UpdateLightFade(DeltaSeconds);
}

// ============================================================================
// USpellSystemComponent
// ============================================================================

USpellSystemComponent::USpellSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void USpellSystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USpellSystemComponent, bUnlockedExort);
	DOREPLIFETIME(USpellSystemComponent, bUnlockedWex);
	DOREPLIFETIME(USpellSystemComponent, bUnlockedQuas);
	DOREPLIFETIME(USpellSystemComponent, ActiveOrbs);
	DOREPLIFETIME(USpellSystemComponent, SlotD);
	DOREPLIFETIME(USpellSystemComponent, SlotF);
	DOREPLIFETIME(USpellSystemComponent, CastMode);
	DOREPLIFETIME(USpellSystemComponent, Mana);
	DOREPLIFETIME(USpellSystemComponent, MaxMana);
	DOREPLIFETIME(USpellSystemComponent, ExortPowerScale);
	DOREPLIFETIME(USpellSystemComponent, WexSpeedScale);
	DOREPLIFETIME(USpellSystemComponent, QuasControlScale);
	DOREPLIFETIME(USpellSystemComponent, bSpellCastingBlockedByStature);
	DOREPLIFETIME(USpellSystemComponent, ManaCostMultiplier);
	DOREPLIFETIME(USpellSystemComponent, SpellCooldownMultiplier);
	DOREPLIFETIME(USpellSystemComponent, ManaRegenPerSec);
}

void USpellSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime <= 0.f)
	{
		return;
	}

	// Regenerate Mana driven by ManaRegenPerSec (Spirit + SkillTree)
	if (Mana < MaxMana)
	{
		const float RegenRate = FMath::Max(0.f, ManaRegenPerSec);
		Mana = FMath::Min(MaxMana, Mana + RegenRate * DeltaTime);
	}

	// Tick invoke cooldown
	if (InvokeCooldownRemaining > 0.f)
	{
		InvokeCooldownRemaining = FMath::Max(0.f, InvokeCooldownRemaining - DeltaTime);
	}

	// Tick down per-spell cooldown map cleanly
	for (auto It = CooldownRemaining.CreateIterator(); It; ++It)
	{
		It.Value() -= DeltaTime;
		if (It.Value() <= 0.f)
		{
			It.RemoveCurrent();
		}
	}
}

void USpellSystemComponent::OnRep_SpellState()
{
	OnSpellStateChanged.Broadcast();
}

bool USpellSystemComponent::OwnerHasAuthority() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor != nullptr && OwnerActor->HasAuthority();
}

// --- Quest-Gated Orb Unlocks ---

void USpellSystemComponent::UnlockOrb(EChronicleOrb Orb)
{
	if (Orb == EChronicleOrb::None)
	{
		return;
	}

	if (!OwnerHasAuthority())
	{
		ServerUnlockOrb(Orb);
		return;
	}

	UnlockOrb_Internal(Orb);
}

void USpellSystemComponent::UnlockOrb_Internal(EChronicleOrb Orb)
{
	bool bNewlyUnlocked = false;
	switch (Orb)
	{
	case EChronicleOrb::Exort:
		if (!bUnlockedExort)
		{
			bUnlockedExort = true;
			bNewlyUnlocked = true;
		}
		break;
	case EChronicleOrb::Wex:
		if (!bUnlockedWex)
		{
			bUnlockedWex = true;
			bNewlyUnlocked = true;
		}
		break;
	case EChronicleOrb::Quas:
		if (!bUnlockedQuas)
		{
			bUnlockedQuas = true;
			bNewlyUnlocked = true;
		}
		break;
	default:
		break;
	}

	if (bNewlyUnlocked)
	{
		UE_LOG(LogChronicles, Display,
			TEXT("[SpellSystem] Primal Orb Unlocked: %s | Unlocked Orbs: %d/3 | Available Spells: %d/10"),
			OrbToDisplayName(Orb),
			GetUnlockedOrbCount(),
			GetUnlockedSpellCount());

		OnOrbUnlocked.Broadcast(Orb);
		OnSpellStateChanged.Broadcast();
	}
}

bool USpellSystemComponent::IsOrbUnlocked(EChronicleOrb Orb) const
{
	switch (Orb)
	{
	case EChronicleOrb::Exort: return bUnlockedExort;
	case EChronicleOrb::Wex:   return bUnlockedWex;
	case EChronicleOrb::Quas:  return bUnlockedQuas;
	default:                   return false;
	}
}

int32 USpellSystemComponent::GetUnlockedOrbCount() const
{
	return (bUnlockedExort ? 1 : 0) + (bUnlockedWex ? 1 : 0) + (bUnlockedQuas ? 1 : 0);
}

// --- 3-Orb Invoker Queue & Invocation ---

bool USpellSystemComponent::PushOrb(EChronicleOrb Orb)
{
	if (bSpellCastingBlockedByStature)
	{
		UE_LOG(LogChronicles, Verbose, TEXT("[SpellSystem] PushOrb blocked by small Hero stature (< 1.45m)."));
		return false;
	}

	if (Orb == EChronicleOrb::None || !IsOrbUnlocked(Orb))
	{
		UE_LOG(LogChronicles, Verbose, TEXT("[SpellSystem] Rejected locked/invalid orb push: %s"), OrbToDisplayName(Orb));
		return false;
	}

	if (!OwnerHasAuthority())
	{
		ServerPushOrb(Orb);
	}

	return PushOrb_Internal(Orb);
}

bool USpellSystemComponent::PushOrb_Internal(EChronicleOrb Orb)
{
	if (bSpellCastingBlockedByStature || Orb == EChronicleOrb::None || !IsOrbUnlocked(Orb))
	{
		return false;
	}

	ActiveOrbs.Add(Orb);
	while (ActiveOrbs.Num() > 3)
	{
		ActiveOrbs.RemoveAt(0);
	}

	if (CastMode == EChronicleCastMode::InstantCombo && ActiveOrbs.Num() == 3)
	{
		const EChronicleSpellId Resolved = ResolveCurrentCombo();
		if (Resolved != EChronicleSpellId::None)
		{
			if (Resolved == SlotF)
			{
				Swap(SlotD, SlotF);
			}
			else if (Resolved != SlotD)
			{
				SlotF = SlotD;
				SlotD = Resolved;
			}
		}
	}

	OnSpellStateChanged.Broadcast();
	return true;
}

EChronicleSpellId USpellSystemComponent::ResolveCurrentCombo() const
{
	if (ActiveOrbs.Num() != 3)
	{
		return EChronicleSpellId::None;
	}

	int32 NumE = 0;
	int32 NumW = 0;
	int32 NumQ = 0;

	for (const EChronicleOrb Orb : ActiveOrbs)
	{
		switch (Orb)
		{
		case EChronicleOrb::Exort: ++NumE; break;
		case EChronicleOrb::Wex:   ++NumW; break;
		case EChronicleOrb::Quas:  ++NumQ; break;
		default:                   return EChronicleSpellId::None;
		}
	}

	EChronicleSpellId Candidate = EChronicleSpellId::None;
	if (NumE == 3)                      { Candidate = EChronicleSpellId::SunStrike; }       // EEE
	else if (NumE == 2 && NumW == 1)    { Candidate = EChronicleSpellId::ChaosMeteor; }     // EEW
	else if (NumE == 1 && NumW == 2)    { Candidate = EChronicleSpellId::StormAlacrity; }   // EWW
	else if (NumW == 3)                 { Candidate = EChronicleSpellId::StormEMP; }        // WWW
	else if (NumQ == 3)                 { Candidate = EChronicleSpellId::ColdSnap; }        // QQQ
	else if (NumQ == 2 && NumW == 1)    { Candidate = EChronicleSpellId::ChronoBlink; }     // QQW
	else if (NumQ == 2 && NumE == 1)    { Candidate = EChronicleSpellId::IcePrismWall; }    // QQE
	else if (NumQ == 1 && NumW == 2)    { Candidate = EChronicleSpellId::GravityTornado; }  // QWW
	else if (NumQ == 1 && NumE == 2)    { Candidate = EChronicleSpellId::EmberConduit; }    // QEE
	else if (NumQ == 1 && NumW == 1 && NumE == 1) { Candidate = EChronicleSpellId::PrimalBlast; } // QWE

	if (Candidate == EChronicleSpellId::None || !IsSpellUnlocked(Candidate))
	{
		return EChronicleSpellId::None;
	}

	return Candidate;
}

bool USpellSystemComponent::InvokeCurrentSpell()
{
	if (bSpellCastingBlockedByStature || InvokeCooldownRemaining > 0.f)
	{
		return false;
	}

	if (ResolveCurrentCombo() == EChronicleSpellId::None)
	{
		return false;
	}

	if (!OwnerHasAuthority())
	{
		ServerInvokeCurrentSpell();
	}

	return InvokeCurrentSpell_Internal();
}

bool USpellSystemComponent::InvokeCurrentSpell_Internal()
{
	if (bSpellCastingBlockedByStature || InvokeCooldownRemaining > 0.f)
	{
		return false;
	}

	const EChronicleSpellId Resolved = ResolveCurrentCombo();
	if (Resolved == EChronicleSpellId::None)
	{
		return false;
	}

	if (Resolved == SlotF)
	{
		Swap(SlotD, SlotF);
	}
	else if (Resolved != SlotD)
	{
		SlotF = SlotD;
		SlotD = Resolved;
	}

	InvokeCooldownRemaining = FMath::Max(0.15f, InvokeCooldownDuration / FMath::Max(0.5f, WexSpeedScale));
	UE_LOG(LogChronicles, Log, TEXT("[SpellSystem] Invoked %s into SlotD (SlotF = %s)"),
		*GetSpellDef(SlotD).ComboKey,
		SlotF != EChronicleSpellId::None ? *GetSpellDef(SlotF).ComboKey : TEXT("None"));

	OnSpellStateChanged.Broadcast();
	return true;
}

// --- Spell Casting ---

bool USpellSystemComponent::CastSpellSlot(
	bool bSlotF,
	const FVector& AimOrigin,
	const FVector& AimDir,
	const FVector& TargetPoint,
	float ChargeT01)
{
	if (bSpellCastingBlockedByStature)
	{
		return false;
	}

	const EChronicleSpellId SpellId = bSlotF ? SlotF : SlotD;
	if (SpellId == EChronicleSpellId::None || !IsSpellUnlocked(SpellId))
	{
		return false;
	}

	if (GetSpellCooldownRemaining(SpellId) > 0.f)
	{
		return false;
	}

	const FChronicleSpellDef Def = GetSpellDef(SpellId);
	const float EffectiveManaCost = FMath::Max(1.f, Def.ManaCost * FMath::Clamp(ManaCostMultiplier, 0.25f, 2.0f));
	if (Mana < EffectiveManaCost)
	{
		return false;
	}

	const float ClampedCharge = FMath::Clamp(ChargeT01, 0.f, 1.f);

	if (!OwnerHasAuthority())
	{
		// Predict cooldown & mana locally for snappy client UX, then run authoritative cast on server
		Mana = FMath::Max(0.f, Mana - EffectiveManaCost);
		const float ScaledCooldown = FMath::Max(0.35f, Def.Cooldown * FMath::Clamp(SpellCooldownMultiplier, 0.25f, 2.0f));
		CooldownRemaining.Add(SpellId, ScaledCooldown);
		OnSpellStateChanged.Broadcast();

		ServerCastSpellSlot(bSlotF, AimOrigin, AimDir, TargetPoint, ClampedCharge);
		return true;
	}

	return CastSpellSlot_Internal(bSlotF, AimOrigin, AimDir, TargetPoint, ClampedCharge);
}

bool USpellSystemComponent::CastSpellSlot_Internal(
	bool bSlotF,
	const FVector& AimOrigin,
	const FVector& AimDir,
	const FVector& TargetPoint,
	float ChargeT01)
{
	if (bSpellCastingBlockedByStature)
	{
		return false;
	}

	const EChronicleSpellId SpellId = bSlotF ? SlotF : SlotD;
	if (SpellId == EChronicleSpellId::None || !IsSpellUnlocked(SpellId))
	{
		return false;
	}

	if (GetSpellCooldownRemaining(SpellId) > 0.f)
	{
		return false;
	}

	const FChronicleSpellDef Def = GetSpellDef(SpellId);
	const float EffectiveManaCost = FMath::Max(1.f, Def.ManaCost * FMath::Clamp(ManaCostMultiplier, 0.25f, 2.0f));
	if (Mana < EffectiveManaCost)
	{
		return false;
	}

	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		return false;
	}

	const float ClampedCharge = FMath::Clamp(ChargeT01, 0.f, 1.f);
	FVector SafeDir = AimDir.GetSafeNormal();
	if (SafeDir.IsNearlyZero())
	{
		SafeDir = OwnerActor->GetActorForwardVector().GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	}

	const FVector SafeOrigin = AimOrigin.IsNearlyZero() ? (OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 60.f)) : AimOrigin;
	FVector ResolvedTarget = TargetPoint;
	if ((TargetPoint - SafeOrigin).IsNearlyZero())
	{
		FHitResult AimHit;
		FCollisionQueryParams AimParams(TEXT("ChronicleSpellAimResolve"), false, OwnerActor);
		const FVector TraceEnd = SafeOrigin + SafeDir * 2200.f;
		if (World->LineTraceSingleByChannel(AimHit, SafeOrigin, TraceEnd, CHRONICLE_TRACE_PROJECTILE, AimParams))
		{
			ResolvedTarget = AimHit.ImpactPoint;
		}
		else
		{
			ResolvedTarget = SafeOrigin + SafeDir * 950.f;
		}
	}

	FVector SpawnLoc = SafeOrigin + SafeDir * 90.f;
	switch (Def.Delivery)
	{
	case EChronicleDelivery::OrbitalStrike:
	case EChronicleDelivery::BarrierWall:
		SpawnLoc = ResolvedTarget;
		break;
	case EChronicleDelivery::RadialNova:
		SpawnLoc = ResolvedTarget + FVector(0.f, 0.f, 80.f);
		break;
	case EChronicleDelivery::RollingMeteor:
		SpawnLoc = ResolvedTarget - SafeDir.GetSafeNormal2D() * 420.f + FVector(0.f, 0.f, 980.f);
		break;
	case EChronicleDelivery::SelfBuff:
		SpawnLoc = OwnerActor->GetActorLocation();
		break;
	case EChronicleDelivery::Projectile:
	case EChronicleDelivery::ConeWave:
	default:
		SpawnLoc = SafeOrigin + SafeDir * 95.f;
		break;
	}

	const FTransform SpawnTransform(SafeDir.Rotation(), SpawnLoc);
	AChronicleSpellActor* SpellActor = World->SpawnActorDeferred<AChronicleSpellActor>(
		AChronicleSpellActor::StaticClass(),
		SpawnTransform,
		OwnerActor,
		Cast<APawn>(OwnerActor),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!SpellActor)
	{
		return false;
	}

	// Deduct resources & start scaled cooldown
	Mana = FMath::Max(0.f, Mana - EffectiveManaCost);
	const float ScaledCooldown = FMath::Max(0.35f, Def.Cooldown * FMath::Clamp(SpellCooldownMultiplier, 0.25f, 2.0f));
	CooldownRemaining.Add(SpellId, ScaledCooldown);

	SpellActor->InitializeSpell(
		SpellId,
		SafeDir,
		ResolvedTarget,
		ClampedCharge,
		ExortPowerScale,
		WexSpeedScale,
		QuasControlScale);
	SpellActor->FinishSpawning(SpawnTransform);

	UE_LOG(LogChronicles, Log,
		TEXT("[SpellSystem] Cast %s (%s) | Mana=%.1f/%.1f | CD=%.2fs | Charge=%.2f"),
		*Def.ComboKey,
		*Def.DisplayName.ToString(),
		Mana,
		MaxMana,
		ScaledCooldown,
		ClampedCharge);

	OnSpellCast.Broadcast(SpellId, SpellActor);
	OnSpellStateChanged.Broadcast();
	return true;
}

// --- Spell Registry & Unlock Checks ---

const TArray<FChronicleSpellDef>& USpellSystemComponent::GetAllSpellDefs()
{
	static const TArray<FChronicleSpellDef> SpellDefs = []()
	{
		TArray<FChronicleSpellDef> List;
		List.Reserve(10);

		auto MakeDef = [](
			EChronicleSpellId Id,
			const TCHAR* Key,
			const TCHAR* Name,
			const TCHAR* Desc,
			EChronicleDelivery Delivery,
			bool bE, bool bW, bool bQ,
			float InMana, float InCD, float InDmg, float InRadius, float InImpulse,
			const FLinearColor& Color)
		{
			FChronicleSpellDef Def;
			Def.SpellId = Id;
			Def.ComboKey = Key;
			Def.DisplayName = FText::FromString(Name);
			Def.Description = FText::FromString(Desc);
			Def.Delivery = Delivery;
			Def.bNeedsExort = bE;
			Def.bNeedsWex = bW;
			Def.bNeedsQuas = bQ;
			Def.ManaCost = InMana;
			Def.Cooldown = InCD;
			Def.BaseDamage = InDmg;
			Def.BlastRadius = InRadius;
			Def.DestructionImpulse = InImpulse;
			Def.PrimaryColor = Color;
			return Def;
		};

		// 1) Exort-only (1 spell unlocked after Exort quest)
		List.Add(MakeDef(
			EChronicleSpellId::SunStrike,
			TEXT("EEE"),
			TEXT("Solar Cataclysm"),
			TEXT("Calls down a concentrated pillar of Exort solar plasma from the heavens, incinerating foes and shattering stone pillars."),
			EChronicleDelivery::OrbitalStrike,
			true, false, false,
			35.f, 5.0f, 125.f, 540.f, 2800.f,
			FLinearColor(1.00f, 0.42f, 0.06f, 1.0f)));

		// 2-4) Exort + Wex (4 spells total after Wex quest)
		List.Add(MakeDef(
			EChronicleSpellId::ChaosMeteor,
			TEXT("EEW"),
			TEXT("Chaos Meteor"),
			TEXT("Summons a blazing astral boulder that crashes down and rolls forward along the earth, crushing barriers in molten flame."),
			EChronicleDelivery::RollingMeteor,
			true, true, false,
			45.f, 7.5f, 95.f, 520.f, 3200.f,
			FLinearColor(1.00f, 0.22f, 0.04f, 1.0f)));

		List.Add(MakeDef(
			EChronicleSpellId::StormAlacrity,
			TEXT("EWW"),
			TEXT("Tempest Surge"),
			TEXT("Surrounds the caster in crackling solar-tempest rings, unleashing a repelling arc pulse and accelerating stride velocity."),
			EChronicleDelivery::SelfBuff,
			true, true, false,
			25.f, 6.0f, 35.f, 360.f, 1200.f,
			FLinearColor(0.95f, 0.78f, 0.18f, 1.0f)));

		List.Add(MakeDef(
			EChronicleSpellId::StormEMP,
			TEXT("WWW"),
			TEXT("Plasma EMP Nova"),
			TEXT("Charges a volatile violet tempest singularity that detonates in a wide shockwave, fracturing cover and siphoning mana back."),
			EChronicleDelivery::RadialNova,
			false, true, false,
			40.f, 7.0f, 105.f, 680.f, 2600.f,
			FLinearColor(0.76f, 0.24f, 1.00f, 1.0f)));

		// 5-10) Exort + Wex + Quas (all 10 Invoker spells after Quas quest)
		List.Add(MakeDef(
			EChronicleSpellId::ColdSnap,
			TEXT("QQQ"),
			TEXT("Glacial Cold Snap"),
			TEXT("Launches a piercing cryo-crystal shard that fractures on impact and triggers repeating glacial frost snaps on the target."),
			EChronicleDelivery::Projectile,
			false, false, true,
			22.f, 3.5f, 60.f, 260.f, 1400.f,
			FLinearColor(0.22f, 0.85f, 1.00f, 1.0f)));

		List.Add(MakeDef(
			EChronicleSpellId::ChronoBlink,
			TEXT("QQW"),
			TEXT("Chrono-Ghost Walk"),
			TEXT("Phase-steps swiftly through fractured time, leaving a detonating cryo-echo behind while accelerating spectral movement."),
			EChronicleDelivery::SelfBuff,
			false, true, true,
			28.f, 5.5f, 40.f, 420.f, 1500.f,
			FLinearColor(0.36f, 0.68f, 1.00f, 1.0f)));

		List.Add(MakeDef(
			EChronicleSpellId::IcePrismWall,
			TEXT("QQE"),
			TEXT("Cryo-Prism Wall"),
			TEXT("Erupts a solid crystalline barrier perpendicular to your aim that blocks enemies and pulses chilling frost energy."),
			EChronicleDelivery::BarrierWall,
			true, false, true,
			34.f, 6.5f, 50.f, 480.f, 1900.f,
			FLinearColor(0.48f, 0.94f, 1.00f, 1.0f)));

		List.Add(MakeDef(
			EChronicleSpellId::GravityTornado,
			TEXT("QWW"),
			TEXT("Tempest Tornado"),
			TEXT("Unleashes a roaring cyclone that sweeps across the terrain, pulling debris inward and launching adversaries skyward."),
			EChronicleDelivery::Projectile,
			false, true, true,
			38.f, 6.5f, 72.f, 460.f, 2400.f,
			FLinearColor(0.58f, 0.56f, 1.00f, 1.0f)));

		List.Add(MakeDef(
			EChronicleSpellId::EmberConduit,
			TEXT("QEE"),
			TEXT("Forge Lava Conduit"),
			TEXT("Manifests a searing crystalline magma spire that radiates successive thermal shockwaves to melt armor and crack masonry."),
			EChronicleDelivery::RadialNova,
			true, false, true,
			36.f, 6.0f, 68.f, 500.f, 2200.f,
			FLinearColor(1.00f, 0.30f, 0.14f, 1.0f)));

		List.Add(MakeDef(
			EChronicleSpellId::PrimalBlast,
			TEXT("QWE"),
			TEXT("Primal Deafening Blast"),
			TEXT("Unites Exort, Wex, and Quas into an expanding tri-elemental shockwave wall that hurls enemies back and demolishes obstacles."),
			EChronicleDelivery::ConeWave,
			true, true, true,
			50.f, 8.0f, 140.f, 640.f, 3600.f,
			FLinearColor(0.85f, 0.62f, 1.00f, 1.0f)));

		return List;
	}();

	return SpellDefs;
}

FChronicleSpellDef USpellSystemComponent::GetSpellDef(EChronicleSpellId SpellId)
{
	const TArray<FChronicleSpellDef>& AllDefs = GetAllSpellDefs();
	for (const FChronicleSpellDef& Def : AllDefs)
	{
		if (Def.SpellId == SpellId)
		{
			return Def;
		}
	}

	return FChronicleSpellDef();
}

bool USpellSystemComponent::IsSpellUnlocked(EChronicleSpellId SpellId) const
{
	if (SpellId == EChronicleSpellId::None)
	{
		return false;
	}

	const FChronicleSpellDef Def = GetSpellDef(SpellId);
	if (Def.SpellId == EChronicleSpellId::None)
	{
		return false;
	}

	return (!Def.bNeedsExort || bUnlockedExort)
		&& (!Def.bNeedsWex || bUnlockedWex)
		&& (!Def.bNeedsQuas || bUnlockedQuas);
}

int32 USpellSystemComponent::GetUnlockedSpellCount() const
{
	int32 Count = 0;
	for (const FChronicleSpellDef& Def : GetAllSpellDefs())
	{
		if (IsSpellUnlocked(Def.SpellId))
		{
			++Count;
		}
	}
	return Count;
}

float USpellSystemComponent::GetSpellCooldownRemaining(EChronicleSpellId SpellId) const
{
	if (const float* Found = CooldownRemaining.Find(SpellId))
	{
		return FMath::Max(0.f, *Found);
	}
	return 0.f;
}

void USpellSystemComponent::RestoreMana(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}
	Mana = FMath::Min(MaxMana, Mana + Amount);
}

void USpellSystemComponent::SetCastMode(EChronicleCastMode NewMode)
{
	if (CastMode == NewMode)
	{
		return;
	}

	CastMode = NewMode;
	if (!OwnerHasAuthority())
	{
		ServerSetCastMode(NewMode);
	}
	OnSpellStateChanged.Broadcast();
}

void USpellSystemComponent::ToggleCastMode()
{
	const EChronicleCastMode NextMode = (CastMode == EChronicleCastMode::ClassicInvoker)
		? EChronicleCastMode::InstantCombo
		: EChronicleCastMode::ClassicInvoker;
	SetCastMode(NextMode);
}

FString USpellSystemComponent::GetActiveOrbsString() const
{
	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		if (i > 0)
		{
			Result += TEXT(" ");
		}
		if (ActiveOrbs.IsValidIndex(i))
		{
			Result += OrbToShortLetter(ActiveOrbs[i]);
		}
		else
		{
			Result += TEXT("-");
		}
	}
	return Result;
}

// --- Server RPC Implementations ---

void USpellSystemComponent::ServerUnlockOrb_Implementation(EChronicleOrb Orb)
{
	UnlockOrb_Internal(Orb);
}

void USpellSystemComponent::ServerPushOrb_Implementation(EChronicleOrb Orb)
{
	PushOrb_Internal(Orb);
}

void USpellSystemComponent::ServerInvokeCurrentSpell_Implementation()
{
	InvokeCurrentSpell_Internal();
}

void USpellSystemComponent::ServerSetCastMode_Implementation(EChronicleCastMode NewMode)
{
	CastMode = NewMode;
	OnSpellStateChanged.Broadcast();
}

void USpellSystemComponent::ServerCastSpellSlot_Implementation(
	bool bSlotF,
	const FVector& AimOrigin,
	const FVector& AimDir,
	const FVector& TargetPoint,
	float ChargeT01)
{
	CastSpellSlot_Internal(bSlotF, AimOrigin, AimDir, TargetPoint, ChargeT01);
}
