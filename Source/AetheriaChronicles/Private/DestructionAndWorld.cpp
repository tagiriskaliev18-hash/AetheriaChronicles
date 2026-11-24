// ============================================================================
// DestructionAndWorld.cpp — Replicated Physics-Shattering Destructible Props
// & Crimson Desert-Inspired 5-Biome Open World / Dynamic Day-Night Builder
// ============================================================================
#include "DestructionAndWorld.h"
#include "AudioAndStyleSubsystem.h"
#include "GraphicsRTXSubsystem.h"
#include "QuestAndMapSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"

namespace AetheriaWorldConst
{
	static constexpr TCHAR MeshCube[]               = TEXT("/Engine/BasicShapes/Cube.Cube");
	static constexpr TCHAR MeshCylinder[]           = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	static constexpr TCHAR MeshSphere[]             = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	static constexpr TCHAR MeshCone[]               = TEXT("/Engine/BasicShapes/Cone.Cone");
	static constexpr TCHAR MeshPlane[]              = TEXT("/Engine/BasicShapes/Plane.Plane");
	static constexpr TCHAR MatBasicShape[]          = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	static constexpr TCHAR MatEmissive[]            = TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");
	static constexpr TCHAR MatSimpleCloud[]         = TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst");

	// Verified Gothic_props_1 PBR Meshes
	static constexpr TCHAR MeshGothicSection1[]     = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Section1.SM_Section1");
	static constexpr TCHAR MeshGothicSection2[]     = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Section2.SM_Section2");
	static constexpr TCHAR MeshGothicBlock1[]       = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Block1.SM_Block1");
	static constexpr TCHAR MeshGothicBlock2[]       = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Block2.SM_Block2");
	static constexpr TCHAR MeshGothicArc[]          = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Arc.SM_Arc");
	static constexpr TCHAR MeshGothicBridge1[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Bridge1.SM_Bridge1");
	static constexpr TCHAR MeshGothicBridge2[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Bridge2.SM_Bridge2");
	static constexpr TCHAR MeshGothicColumn1[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Column1.SM_Column1");
	static constexpr TCHAR MeshGothicColumn3[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Column3.SM_Column3");
	static constexpr TCHAR MeshGothicGate[]         = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Gate.SM_Gate");
	static constexpr TCHAR MeshGothicPillar1[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Pillar1.SM_Pillar1");
	static constexpr TCHAR MeshGothicPillar2[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Pillar2.SM_Pillar2");
	static constexpr TCHAR MeshGothicSteps[]        = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Steps.SM_Steps");
	static constexpr TCHAR MeshGothicBallista[]     = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Ballista.SM_Ballista");

	// Verified TA_Sample_Statue PBR Meshes
	static constexpr TCHAR MeshStatue01[]           = TEXT("/Game/TA_Sample_Statue/Models/SM_TA_Sample_Statue_01.SM_TA_Sample_Statue_01");
	static constexpr TCHAR MeshStatuePedestal[]     = TEXT("/Game/TA_Sample_Statue/Models/SM_TA_Sample_Pedestal_01.SM_TA_Sample_Pedestal_01");

	// Verified /Game/Meshes/ PBR Meshes (replaced broken FortWall/FortPlaster/RockMoss/DeadTreeTrunk & floating chandeliers)
	static constexpr TCHAR MeshCathedralLow[]       = TEXT("/Game/Meshes/SM_CathedralWall_LowCutaway.SM_CathedralWall_LowCutaway");
	static constexpr TCHAR MeshCathedralArched[]    = TEXT("/Game/Meshes/SM_CathedralWall_Arched.SM_CathedralWall_Arched");
	static constexpr TCHAR MeshFortWall[]           = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Section1.SM_Section1");
	static constexpr TCHAR MeshFortPlaster[]        = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Section2.SM_Section2");
	static constexpr TCHAR MeshSanctuaryShrine[]    = TEXT("/Game/Meshes/SM_SanctuaryShrine.SM_SanctuaryShrine");
	static constexpr TCHAR MeshRunicAltar[]         = TEXT("/Game/Meshes/SM_RunicAltar.SM_RunicAltar");
	static constexpr TCHAR MeshBrazierFlame[]       = TEXT("/Game/Meshes/SM_BrazierFlame.SM_BrazierFlame");
	static constexpr TCHAR MeshStoneFirePit[]       = TEXT("/Game/Meshes/SM_StoneFirePit.SM_StoneFirePit");
	static constexpr TCHAR MeshDeadTree[]           = TEXT("/Game/Meshes/SM_DeadTree.SM_DeadTree");
	static constexpr TCHAR MeshCitadelFortress[]    = TEXT("/Game/Meshes/SM_CitadelFortress.SM_CitadelFortress");
	static constexpr TCHAR MeshGothicArchSpan[]     = TEXT("/Game/Meshes/SM_GothicArch_Span.SM_GothicArch_Span");
	static constexpr TCHAR MeshGothicPillar[]       = TEXT("/Game/Meshes/SM_GothicPillar.SM_GothicPillar");
	static constexpr TCHAR MeshRuinedArch[]         = TEXT("/Game/Meshes/SM_RuinedArch.SM_RuinedArch");
	// Replaced oversized SM_DungeonFloor_Dais/Slab4x4 (whose raw bounds spanned ~80m!) with grounded PBR pedestal & flagstone
	static constexpr TCHAR MeshDungeonDais[]        = TEXT("/Game/TA_Sample_Statue/Models/SM_TA_Sample_Pedestal_01.SM_TA_Sample_Pedestal_01");
	static constexpr TCHAR MeshDungeonSlab[]        = TEXT("/Engine/BasicShapes/Cube.Cube");
	static constexpr TCHAR MeshGothicBarrel[]       = TEXT("/Game/Meshes/SM_GothicBarrel.SM_GothicBarrel");
	static constexpr TCHAR MeshTreasureChest[]      = TEXT("/Game/Meshes/SM_TreasureChest.SM_TreasureChest");
	static constexpr TCHAR MeshCryptSarcophagus[]   = TEXT("/Game/Meshes/SM_CryptSarcophagus.SM_CryptSarcophagus");
	static constexpr TCHAR MeshCliffBoulder[]       = TEXT("/Game/Meshes/SM_CliffBoulder.SM_CliffBoulder");
}

namespace AetheriaWorldHelpers
{
	static FBox GetSafeBoundingBox(const UStaticMesh* Mesh)
	{
		if (Mesh)
		{
			const FBox Box = Mesh->GetBoundingBox();
			if (Box.IsValid)
			{
				return Box;
			}
		}
		return FBox(FVector(-50.f, -50.f, -50.f), FVector(50.f, 50.f, 50.f));
	}

	static float ComputeUniformScaleForHeight(const UStaticMesh* Mesh, float TargetHeightCm, float MinScale = 0.05f, float MaxScale = 3.5f)
	{
		const FBox Box = GetSafeBoundingBox(Mesh);
		const FVector Size = Box.GetSize();
		const float RawHeight = FMath::Max(10.f, Size.Z);
		const float RawMaxDim = FMath::Max(10.f, Size.GetMax());
		// Bound both vertical height AND maximum horizontal span so wide/rotated Gothic pieces never balloon into 40m walls
		const float ScaleByH = TargetHeightCm / RawHeight;
		const float ScaleByMax = (TargetHeightCm * 1.65f) / RawMaxDim;
		return FMath::Clamp(FMath::Min(ScaleByH, ScaleByMax), MinScale, FMath::Min(MaxScale, 3.5f));
	}

	static float ComputeGroundedZ(const UStaticMesh* Mesh, float TerrainZ, float ScaleZ, float SinkCm = 4.f)
	{
		const FBox Box = GetSafeBoundingBox(Mesh);
		return TerrainZ - SinkCm - Box.Min.Z * ScaleZ;
	}

	static float ComputeStackedZ(const UStaticMesh* BaseMesh, float BaseWorldZ, float BaseScaleZ, const UStaticMesh* TopMesh, float TopScaleZ, float SinkCm = 2.f)
	{
		const FBox BaseBox = GetSafeBoundingBox(BaseMesh);
		const FBox TopBox = GetSafeBoundingBox(TopMesh);
		const float BaseTopZ = BaseWorldZ + BaseBox.Max.Z * BaseScaleZ;
		return BaseTopZ - SinkCm - TopBox.Min.Z * TopScaleZ;
	}
}

// ============================================================================
// ADestructiblePropActor
// ============================================================================

ADestructiblePropActor::ADestructiblePropActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	IntactMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntactMesh"));
	RootComponent = IntactMesh;

	IntactMesh->SetMobility(EComponentMobility::Movable);
	IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	IntactMesh->SetCollisionObjectType(ECC_WorldDynamic);
	IntactMesh->SetCollisionResponseToAllChannels(ECR_Block);
	IntactMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	IntactMesh->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);
	IntactMesh->SetGenerateOverlapEvents(true);
	IntactMesh->SetCanEverAffectNavigation(false);
}

void ADestructiblePropActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADestructiblePropActor, HP);
	DOREPLIFETIME(ADestructiblePropActor, bBroken);
}

void ADestructiblePropActor::ConfigureProp(
	EDestructibleKind InKind,
	float InMaxHP,
	const FString& OptionalMeshPath,
	const FVector& Scale,
	FName InQuestId)
{
	MaterialKind = InKind;
	MaxHP = FMath::Max(1.f, InMaxHP);
	HP = MaxHP;
	bBroken = false;
	bShatterEffectsSpawned = false;
	LinkedQuestId = InQuestId;

	if (!bHasCustomAccent)
	{
		AccentColor = GetDefaultAccentColor(InKind);
		bAccentEmissive = (InKind == EDestructibleKind::Crystal);
	}

	UStaticMesh* LoadedMesh = nullptr;
	if (!OptionalMeshPath.IsEmpty())
	{
		const FString PkgName = FPackageName::ObjectPathToPackageName(OptionalMeshPath);
		if (OptionalMeshPath.StartsWith(TEXT("/Engine/")) || (!PkgName.IsEmpty() && FPackageName::DoesPackageExist(PkgName)))
		{
			LoadedMesh = LoadObject<UStaticMesh>(nullptr, *OptionalMeshPath);
		}
	}

	bool bUsedPrimitiveFallback = false;
	if (!LoadedMesh)
	{
		const TCHAR* FallbackPath =
			(InKind == EDestructibleKind::Crystal || InKind == EDestructibleKind::Wood)
				? AetheriaWorldConst::MeshCylinder
				: AetheriaWorldConst::MeshCube;
		LoadedMesh = LoadObject<UStaticMesh>(nullptr, FallbackPath);
		bUsedPrimitiveFallback = true;
	}

	if (IntactMesh)
	{
		if (LoadedMesh)
		{
			IntactMesh->SetStaticMesh(LoadedMesh);
		}
		IntactMesh->SetRelativeScale3D(Scale);
		IntactMesh->SetVisibility(true, false);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		IntactMesh->SetCollisionResponseToAllChannels(ECR_Block);
		IntactMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		IntactMesh->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);

		if (bUsedPrimitiveFallback || bHasCustomAccent || InKind == EDestructibleKind::Crystal)
		{
			ApplyAccentGlow(AccentColor, bAccentEmissive);
		}
	}
}

void ADestructiblePropActor::ApplyAccentGlow(const FLinearColor& GlowColor, bool bUseEmissive)
{
	AccentColor = GlowColor;
	bAccentEmissive = bUseEmissive;
	bHasCustomAccent = true;

	const TCHAR* MatPath = bUseEmissive ? AetheriaWorldConst::MatEmissive : AetheriaWorldConst::MatBasicShape;
	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, MatPath))
	{
		PropMID = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (PropMID)
		{
			PropMID->SetVectorParameterValue(TEXT("Color"), GlowColor);
			PropMID->SetVectorParameterValue(TEXT("BaseColor"), GlowColor);
			PropMID->SetVectorParameterValue(TEXT("EmissiveColor"), GlowColor);
			if (IntactMesh)
			{
				if (bUseEmissive)
				{
					IntactMesh->bDisallowNanite = true;
				}
				const int32 NumMats = FMath::Max(1, IntactMesh->GetNumMaterials());
				for (int32 Idx = 0; Idx < NumMats; ++Idx)
				{
					IntactMesh->SetMaterial(Idx, PropMID);
				}
			}
		}
	}
}

void ADestructiblePropActor::ForceShatter(const FVector& ImpulseOrigin)
{
	if (bBroken)
	{
		return;
	}

	HP = 0.f;
	bBroken = true;

	const FVector PropCenter = IntactMesh ? IntactMesh->Bounds.Origin : GetActorLocation();
	const FVector EffectiveOrigin = ImpulseOrigin.IsNearlyZero() ? PropCenter : ImpulseOrigin;
	const FVector StrikeDir = (PropCenter - EffectiveOrigin).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);

	TriggerShatter(EffectiveOrigin, StrikeDir);
	if (HasAuthority())
	{
		NotifyQuestProgress();
	}
	SetLifeSpan(5.0f);
}

float ADestructiblePropActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bBroken || ActualDamage <= 0.f)
	{
		return 0.f;
	}

	const FVector PropCenter = IntactMesh ? IntactMesh->Bounds.Origin : GetActorLocation();
	FVector ImpulseOrigin = PropCenter;
	FVector StrikeDir = FVector::UpVector;

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointEvt = static_cast<const FPointDamageEvent*>(&DamageEvent);
		ImpulseOrigin = PointEvt->HitInfo.bBlockingHit ? FVector(PointEvt->HitInfo.ImpactPoint) : PropCenter;
		StrikeDir = PointEvt->ShotDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		const FRadialDamageEvent* RadialEvt = static_cast<const FRadialDamageEvent*>(&DamageEvent);
		ImpulseOrigin = RadialEvt->Origin;
		StrikeDir = (PropCenter - ImpulseOrigin).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	}
	else if (DamageCauser)
	{
		ImpulseOrigin = DamageCauser->GetActorLocation();
		StrikeDir = (PropCenter - ImpulseOrigin).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	}

	HP = FMath::Max(0.f, HP - ActualDamage);

	if (HP <= 0.f && !bBroken)
	{
		bBroken = true;
		TriggerShatter(ImpulseOrigin, StrikeDir);
		if (HasAuthority())
		{
			NotifyQuestProgress();
		}
		SetLifeSpan(5.0f);
	}

	return ActualDamage;
}

void ADestructiblePropActor::OnRep_Broken()
{
	if (bBroken && !bShatterEffectsSpawned)
	{
		const FVector PropCenter = IntactMesh ? IntactMesh->Bounds.Origin : GetActorLocation();
		TriggerShatter(PropCenter, FVector::UpVector);
	}
}

void ADestructiblePropActor::TriggerShatter(const FVector& ImpulseOrigin, const FVector& StrikeDirection)
{
	if (bShatterEffectsSpawned)
	{
		return;
	}
	bShatterEffectsSpawned = true;

	FVector Origin = GetActorLocation();
	FVector BoxExtent(60.f, 60.f, 90.f);

	if (IntactMesh)
	{
		const FBoxSphereBounds Bounds = IntactMesh->Bounds;
		Origin = Bounds.Origin;
		BoxExtent = Bounds.BoxExtent.ComponentMax(FVector(35.f, 35.f, 45.f)).ComponentMin(FVector(180.f, 180.f, 260.f));

		IntactMesh->SetVisibility(false, false);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IntactMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	// Select debris chunk mesh & matching material (faceted cones for crystal shards, fractured blocks otherwise)
	const TCHAR* DebrisMeshPath = (MaterialKind == EDestructibleKind::Crystal)
		? AetheriaWorldConst::MeshCone
		: AetheriaWorldConst::MeshCube;
	UStaticMesh* ChunkMesh = LoadObject<UStaticMesh>(nullptr, DebrisMeshPath);

	const TCHAR* DebrisMatPath = (bAccentEmissive || MaterialKind == EDestructibleKind::Crystal)
		? AetheriaWorldConst::MatEmissive
		: AetheriaWorldConst::MatBasicShape;

	if (UMaterialInterface* BaseDebrisMat = LoadObject<UMaterialInterface>(nullptr, DebrisMatPath))
	{
		DebrisMID = UMaterialInstanceDynamic::Create(BaseDebrisMat, this);
		if (DebrisMID)
		{
			const FLinearColor ChunkTint = bHasCustomAccent ? AccentColor : GetDefaultAccentColor(MaterialKind);
			DebrisMID->SetVectorParameterValue(TEXT("Color"), ChunkTint);
			DebrisMID->SetVectorParameterValue(TEXT("BaseColor"), ChunkTint);
			DebrisMID->SetVectorParameterValue(TEXT("EmissiveColor"), ChunkTint);
		}
	}

	const int32 NumChunks = FMath::RandRange(6, 8);
	DebrisChunks.Reserve(NumChunks);

	const float BaseChunkScale = FMath::Clamp(BoxExtent.GetMax() / 220.f, 0.24f, 0.72f);

	for (int32 Idx = 0; Idx < NumChunks; ++Idx)
	{
		const FName ChunkName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("DebrisChunk"));
		UStaticMeshComponent* Chunk = NewObject<UStaticMeshComponent>(this, ChunkName);
		if (!Chunk)
		{
			continue;
		}

		if (ChunkMesh)
		{
			Chunk->SetStaticMesh(ChunkMesh);
		}
		if (DebrisMID)
		{
			if (bAccentEmissive || MaterialKind == EDestructibleKind::Crystal)
			{
				Chunk->bDisallowNanite = true;
			}
			Chunk->SetMaterial(0, DebrisMID);
		}

		const float AngleRad = (static_cast<float>(Idx) / static_cast<float>(NumChunks)) * UE_TWO_PI
			+ FMath::FRandRange(-0.35f, 0.35f);
		const float RadialDist = FMath::FRandRange(18.f, FMath::Max(32.f, BoxExtent.GetMin() * 0.65f));
		const float HeightOffset = FMath::FRandRange(-BoxExtent.Z * 0.45f, BoxExtent.Z * 0.65f);

		const FVector ChunkOffset(
			FMath::Cos(AngleRad) * RadialDist,
			FMath::Sin(AngleRad) * RadialDist,
			HeightOffset);
		const FVector ChunkPos = Origin + ChunkOffset;
		const FRotator ChunkRot(
			FMath::FRandRange(0.f, 360.f),
			FMath::FRandRange(0.f, 360.f),
			FMath::FRandRange(0.f, 360.f));

		// Varied fractured shard proportions
		const FVector ChunkScale(
			BaseChunkScale * FMath::FRandRange(0.55f, 1.15f),
			BaseChunkScale * FMath::FRandRange(0.45f, 0.95f),
			BaseChunkScale * FMath::FRandRange(0.50f, 1.35f));

		Chunk->SetUsingAbsoluteLocation(true);
		Chunk->SetUsingAbsoluteRotation(true);
		Chunk->SetUsingAbsoluteScale(true);
		Chunk->SetMobility(EComponentMobility::Movable);
		Chunk->SetWorldLocationAndRotation(ChunkPos, ChunkRot);
		Chunk->SetWorldScale3D(ChunkScale);

		Chunk->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Chunk->SetCollisionObjectType(ECC_PhysicsBody);
		Chunk->SetCollisionResponseToAllChannels(ECR_Block);
		Chunk->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Chunk->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Chunk->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Ignore);
		Chunk->SetCanEverAffectNavigation(false);
		Chunk->SetCastShadow(true);

		Chunk->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
		Chunk->RegisterComponent();

		Chunk->SetSimulatePhysics(true);
		Chunk->SetEnableGravity(true);

		FVector OutwardDir = (ChunkPos - ImpulseOrigin);
		OutwardDir.Z = FMath::Abs(OutwardDir.Z) + 0.45f;
		OutwardDir = (OutwardDir.GetSafeNormal() + StrikeDirection * 0.45f).GetSafeNormal();

		const float Speed = FMath::FRandRange(440.f, 880.f);
		const FVector VelocityImpulse = OutwardDir * Speed + FVector(0.f, 0.f, FMath::FRandRange(220.f, 480.f));
		Chunk->AddImpulse(VelocityImpulse, NAME_None, true);

		const FVector SpinDeg(
			FMath::FRandRange(-420.f, 420.f),
			FMath::FRandRange(-420.f, 420.f),
			FMath::FRandRange(-420.f, 420.f));
		Chunk->SetPhysicsAngularVelocityInDegrees(SpinDeg);

		DebrisChunks.Add(Chunk);
	}

	// Physical AgX 14,000 lm shatter flash point light (fades smoothly in Tick)
	const FName LightName = MakeUniqueObjectName(this, UPointLightComponent::StaticClass(), TEXT("ShatterFlashLight"));
	ShatterFlashLight = NewObject<UPointLightComponent>(this, LightName);
	if (ShatterFlashLight)
	{
		ShatterFlashLight->SetUsingAbsoluteLocation(true);
		ShatterFlashLight->SetMobility(EComponentMobility::Movable);
		ShatterFlashLight->SetWorldLocation(Origin + FVector(0.f, 0.f, 35.f));
		ShatterFlashLight->SetIntensityUnits(ELightUnits::Lumens);
		ShatterFlashLight->SetIntensity(FlashPeakLumens);
		ShatterFlashLight->SetAttenuationRadius(1250.f);
		ShatterFlashLight->SetSourceRadius(32.f);
		ShatterFlashLight->SetLightColor(GetFlashColor(MaterialKind));
		ShatterFlashLight->SetCastShadows(false);
		ShatterFlashLight->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
		ShatterFlashLight->RegisterComponent();

		FlashElapsed = 0.f;
		bFlashActive = true;
		SetActorTickEnabled(true);
	}

	if (!IsRunningDedicatedServer())
	{
		const TCHAR* ShatterSfxPath = (MaterialKind == EDestructibleKind::Crystal)
			? TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Crystal_1.SW_MagicImpact_Crystal_1")
			: TEXT("/Game/FreeModularMagicSFX/E_Impact/Mono/SW_MagicImpact_Metallic_1.SW_MagicImpact_Metallic_1");
		const FString PkgName = FPackageName::ObjectPathToPackageName(FString(ShatterSfxPath));
		if (!PkgName.IsEmpty() && FPackageName::DoesPackageExist(PkgName))
		{
			if (USoundBase* Sfx = LoadObject<USoundBase>(nullptr, ShatterSfxPath))
			{
				UGameplayStatics::PlaySoundAtLocation(this, Sfx, Origin, 0.85f, FMath::FRandRange(0.92f, 1.08f));
			}
		}
	}

	UE_LOG(LogChronicles, Log, TEXT("[Destruction] Prop '%s' shattered into %d physics chunks (Quest=%s)."),
		*GetName(), DebrisChunks.Num(), *LinkedQuestId.ToString());
}

void ADestructiblePropActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFlashActive || !ShatterFlashLight)
	{
		SetActorTickEnabled(false);
		return;
	}

	FlashElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(1.f - (FlashElapsed / FMath::Max(0.05f, FlashDuration)), 0.f, 1.f);
	const float SmoothCurve = Alpha * Alpha * Alpha;
	ShatterFlashLight->SetIntensity(FlashPeakLumens * SmoothCurve);

	if (FlashElapsed >= FlashDuration)
	{
		ShatterFlashLight->SetIntensity(0.f);
		ShatterFlashLight->SetVisibility(false);
		bFlashActive = false;
		SetActorTickEnabled(false);
	}
}

void ADestructiblePropActor::NotifyQuestProgress()
{
	if (LinkedQuestId == NAME_None)
	{
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (!GI)
	{
		return;
	}

	if (UQuestSubsystem* QuestSub = GI->GetSubsystem<UQuestSubsystem>())
	{
		QuestSub->AdvanceQuestProgress(LinkedQuestId, 1);
		UE_LOG(LogChronicles, Log, TEXT("[Destruction] Notified UQuestSubsystem::AdvanceQuestProgress(%s, 1)."),
			*LinkedQuestId.ToString());
	}
}

FLinearColor ADestructiblePropActor::GetDefaultAccentColor(EDestructibleKind Kind) const
{
	switch (Kind)
	{
	case EDestructibleKind::Wood:
		return FLinearColor(0.38f, 0.22f, 0.10f, 1.0f);
	case EDestructibleKind::Crystal:
		return FLinearColor(0.25f, 1.65f, 2.50f, 1.0f);
	case EDestructibleKind::Obsidian:
		return FLinearColor(0.16f, 0.05f, 0.03f, 1.0f);
	case EDestructibleKind::Foliage:
		return FLinearColor(0.12f, 0.42f, 0.56f, 1.0f);
	case EDestructibleKind::Stone:
	default:
		return FLinearColor(0.52f, 0.48f, 0.43f, 1.0f);
	}
}

FLinearColor ADestructiblePropActor::GetFlashColor(EDestructibleKind Kind) const
{
	if (bHasCustomAccent && bAccentEmissive)
	{
		return FLinearColor(
			FMath::Clamp(AccentColor.R, 0.15f, 1.0f),
			FMath::Clamp(AccentColor.G, 0.15f, 1.0f),
			FMath::Clamp(AccentColor.B, 0.15f, 1.0f),
			1.0f);
	}

	switch (Kind)
	{
	case EDestructibleKind::Crystal:
		return FLinearColor(0.38f, 0.88f, 1.0f, 1.0f);
	case EDestructibleKind::Obsidian:
		return FLinearColor(1.0f, 0.30f, 0.06f, 1.0f);
	case EDestructibleKind::Wood:
		return FLinearColor(1.0f, 0.60f, 0.22f, 1.0f);
	case EDestructibleKind::Foliage:
		return FLinearColor(0.45f, 0.85f, 1.0f, 1.0f);
	case EDestructibleKind::Stone:
	default:
		return FLinearColor(1.0f, 0.72f, 0.36f, 1.0f);
	}
}

// ============================================================================
// AAetheriaWorldBuilder
// ============================================================================

AAetheriaWorldBuilder::AAetheriaWorldBuilder()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	WorldRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WorldRoot"));
	WorldRoot->SetMobility(EComponentMobility::Static);
	RootComponent = WorldRoot;

	MossyBouldersISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MossyBouldersISM"));
	MossyBouldersISM->SetupAttachment(WorldRoot);
	MossyBouldersISM->SetMobility(EComponentMobility::Static);
	MossyBouldersISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MossyBouldersISM->SetCollisionResponseToAllChannels(ECR_Block);
	MossyBouldersISM->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MossyBouldersISM->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);

	TreeTrunksISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TreeTrunksISM"));
	TreeTrunksISM->SetupAttachment(WorldRoot);
	TreeTrunksISM->SetMobility(EComponentMobility::Static);
	TreeTrunksISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TreeTrunksISM->SetCollisionResponseToAllChannels(ECR_Block);
	TreeTrunksISM->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	TreeTrunksISM->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);

	TreeCanopiesISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TreeCanopiesISM"));
	TreeCanopiesISM->SetupAttachment(WorldRoot);
	TreeCanopiesISM->SetMobility(EComponentMobility::Static);
	TreeCanopiesISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TreeCanopiesISM->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	TreeCanopiesISM->SetCastShadow(true);
}

void AAetheriaWorldBuilder::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGraphicsRTXSubsystem* RTXSub = GI->GetSubsystem<UGraphicsRTXSubsystem>())
		{
			bRTXActiveCached = RTXSub->IsRTXActive();
		}
	}

	FallbackGroundZ = SampleGroundZ(0.f, 0.f, 0.f);

	SetupAtmosphereAndLighting();
	UpdateAtmosphereForTimeOfDay(TimeOfDay);
	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			SkyComp->RecaptureSky();
		}
	}
	BuildAllBiomes();
	FAetheriaStyleHarmonizer::HarmonizeActorMaterials(this, EChronicleStyleCategory::Architecture);
}

void AAetheriaWorldBuilder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float SafeDelta = FMath::Clamp(DeltaTime, 0.0f, 0.1f);
	ElapsedAnimTime += SafeDelta;

	// Slowly advance Day/Night clock (1 full cycle = 360 seconds)
	TimeOfDay = FMath::Fmod(TimeOfDay + (SafeDelta / 360.f), 1.0f);
	UpdateAtmosphereForTimeOfDay(TimeOfDay);

	// Dusk/Night warmth boost for village lanterns, bridge braziers, and citadel fires
	const float SunElevationProxy = FMath::Sin((TimeOfDay - 0.25f) * UE_TWO_PI); // +1 noon, -1 midnight
	const float NightFactor = FMath::Clamp((0.25f - SunElevationProxy) / 1.15f, 0.0f, 1.0f);

	for (const FWorldAnimatedLight& Anim : AnimatedLights)
	{
		if (!Anim.Light)
		{
			continue;
		}
		const float WaveA = FMath::Sin(ElapsedAnimTime * Anim.FlickerSpeed + Anim.PhaseOffset);
		const float WaveB = FMath::Sin(ElapsedAnimTime * (Anim.FlickerSpeed * 1.73f) + Anim.PhaseOffset * 2.1f);
		const float Flicker = 1.0f + Anim.FlickerAmplitude * (0.65f * WaveA + 0.35f * WaveB);
		const float NightScale = Anim.bNightBoost ? FMath::Lerp(0.85f, 1.65f, NightFactor) : 1.0f;
		Anim.Light->SetIntensity(Anim.BaseLumens * Flicker * NightScale);
	}

	// Pulse Ember Crypt molten lava cracks (controlled volcanic smolder around 1.4, 0.28, 0.03)
	if (LavaFloorMID)
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(ElapsedAnimTime * 2.4f);
		const FLinearColor LavaGlow(
			FMath::Lerp(1.05f, 1.42f, Pulse),
			FMath::Lerp(0.20f, 0.29f, Pulse),
			FMath::Lerp(0.02f, 0.032f, Pulse),
			1.0f);
		LavaFloorMID->SetVectorParameterValue(TEXT("Color"), LavaGlow);
		LavaFloorMID->SetVectorParameterValue(TEXT("EmissiveColor"), LavaGlow);
	}

	// Slowly rotate Dragon Sky Rune Rings
	for (int32 Idx = 0; Idx < SkyRuneRingPivots.Num(); ++Idx)
	{
		if (USceneComponent* Pivot = SkyRuneRingPivots[Idx])
		{
			const float RollSpeed = (Idx % 2 == 0) ? 14.f : -12.f;
			Pivot->AddLocalRotation(FRotator(0.f, 0.f, RollSpeed * SafeDelta));
		}
	}
}

void AAetheriaWorldBuilder::CycleTimeOfDayPreset()
{
	// Golden Sunset (0.72) -> Starry Full Moon (0.02) -> Misty Dawn (0.26) -> Highland Noon (0.48)
	static constexpr float Presets[4] = { 0.72f, 0.02f, 0.26f, 0.48f };
	ActivePresetIndex = (ActivePresetIndex + 1) % 4;
	TimeOfDay = Presets[ActivePresetIndex];
	UpdateAtmosphereForTimeOfDay(TimeOfDay);
	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			SkyComp->RecaptureSky();
		}
	}

	UE_LOG(LogChronicles, Display, TEXT("[WorldBuilder] TimeOfDay preset switched -> %s (%.2f)"),
		*GetTimeOfDayLabel(), TimeOfDay);
}

void AAetheriaWorldBuilder::SetTimeOfDay(float NewTimeOfDay)
{
	TimeOfDay = FMath::Fmod(FMath::Abs(NewTimeOfDay), 1.0f);
	UpdateAtmosphereForTimeOfDay(TimeOfDay);
	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			SkyComp->RecaptureSky();
		}
	}
}

FString AAetheriaWorldBuilder::GetTimeOfDayLabel() const
{
	if (TimeOfDay >= 0.62f && TimeOfDay < 0.84f)
	{
		return TEXT("Golden Sunset");
	}
	if (TimeOfDay >= 0.84f || TimeOfDay < 0.18f)
	{
		return TEXT("Starry Full Moon");
	}
	if (TimeOfDay >= 0.18f && TimeOfDay < 0.36f)
	{
		return TEXT("Misty Dawn");
	}
	return TEXT("Highland Noon");
}

float AAetheriaWorldBuilder::SampleGroundZ(float X, float Y, float FallbackZ) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FallbackZ;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldBuilderSampleGroundZ), false, this);
	Params.AddIgnoredActor(this);

	const FVector TraceStart(X, Y, 20000.f);
	const FVector TraceEnd(X, Y, -10000.f);

	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params) && Hit.bBlockingHit)
	{
		return Hit.ImpactPoint.Z;
	}
	return FallbackZ;
}

EChronicleBiome AAetheriaWorldBuilder::GetBiomeAtLocation(const FVector& WorldLoc) const
{
	if (FVector::Dist2D(WorldLoc, FVector(0.f, -9800.f, 0.f)) < 3600.f)
	{
		return EChronicleBiome::HighkeepCitadel;
	}
	if (FVector::Dist2D(WorldLoc, FVector(9800.f, -2400.f, 0.f)) < 3200.f)
	{
		return EChronicleBiome::AshCalderaCrypt;
	}
	if (FVector::Dist2D(WorldLoc, FVector(-6800.f, -6400.f, 0.f)) < 3400.f)
	{
		return EChronicleBiome::CrystalGlacier;
	}
	if (FMath::Abs(WorldLoc.X) >= 4000.f)
	{
		return EChronicleBiome::BlueLeafWoods;
	}
	return EChronicleBiome::OakhavenValley;
}

TArray<ADestructiblePropActor*> AAetheriaWorldBuilder::GetDestructibleProps() const
{
	TArray<ADestructiblePropActor*> Result;
	Result.Reserve(SpawnedDestructibles.Num());
	for (const TObjectPtr<ADestructiblePropActor>& Prop : SpawnedDestructibles)
	{
		if (Prop)
		{
			Result.Add(Prop.Get());
		}
	}
	return Result;
}

// ============================================================================
// Atmosphere & Dynamic Lighting Setup
// ============================================================================

void AAetheriaWorldBuilder::SetupAtmosphereAndLighting()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Inspect & disable any pre-existing SkySphere / SkyDome static mesh actors in OpenWorld
	// (OpenWorld's SM_SkySphere defaults to CastShadow=1 at 400x scale, occluding DirectionalLight & SkyAtmosphere!)
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		UStaticMeshComponent* SMC = It->GetStaticMeshComponent();
		const UStaticMesh* SM = SMC ? SMC->GetStaticMesh() : nullptr;
		if (SM)
		{
			const FString MeshName = SM->GetName();
			if (MeshName.Contains(TEXT("SkySphere"), ESearchCase::IgnoreCase) ||
				MeshName.Contains(TEXT("SkyDome"), ESearchCase::IgnoreCase))
			{
				UE_LOG(LogChronicles, Display,
					TEXT("[AtmosphereInspect] Disabling blocking SkyMeshActor='%s' Mesh='%s' Scale=%s (was CastShadow=%d Visible=%d)"),
					*It->GetName(),
					*MeshName,
					*It->GetActorScale3D().ToCompactString(),
					SMC->CastShadow ? 1 : 0,
					SMC->IsVisible() ? 1 : 0);

				SMC->SetCastShadow(false);
				SMC->bAffectDynamicIndirectLighting = false;
				SMC->bAffectDistanceFieldLighting = false;
				SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				SMC->SetVisibility(false, true);
				SMC->SetHiddenInGame(true, true);
				SMC->MarkRenderStateDirty();
				It->SetActorHiddenInGame(true);
				It->SetActorEnableCollision(false);
			}
		}
	}

	// 1. Directional Sun / Moon Light (adopt existing OpenWorld sun light or spawn cleanly)
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		const UDirectionalLightComponent* Comp = It->GetComponent();
		UE_LOG(LogChronicles, Display,
			TEXT("[AtmosphereInspect] DirectionalLight='%s' Visible=%d HiddenInGame=%d Mobility=%d Intensity=%.2f AtmoSun=%d"),
			*It->GetName(),
			Comp ? (Comp->IsVisible() ? 1 : 0) : -1,
			It->IsHidden() ? 1 : 0,
			Comp ? static_cast<int32>(Comp->Mobility.GetValue()) : -1,
			Comp ? Comp->Intensity : -1.f,
			Comp ? (Comp->IsUsedAsAtmosphereSunLight() ? 1 : 0) : -1);

		if (!SunLight)
		{
			SunLight = *It;
		}
		else if (UDirectionalLightComponent* ExtraComp = It->GetComponent())
		{
			// Prevent duplicate OpenWorld directional lights from doubling illumination
			ExtraComp->SetIntensity(0.f);
			ExtraComp->SetAtmosphereSunLight(false);
			ExtraComp->MarkRenderStateDirty();
		}
	}
	if (!SunLight)
	{
		SunLight = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(),
			FVector(0.f, 0.f, 1500.f),
			FRotator(-16.f, -42.f, 0.f),
			SpawnParams);
	}
	if (SunLight)
	{
		SunLight->SetActorHiddenInGame(false);
		if (UDirectionalLightComponent* SunComp = SunLight->GetComponent())
		{
			SunComp->SetMobility(EComponentMobility::Movable);
			SunComp->SetVisibility(true);
			SunComp->SetHiddenInGame(false);
			SunComp->bAffectsWorld = true;
			SunComp->SetAtmosphereSunLight(true);
			SunComp->SetAtmosphereSunLightIndex(0);
			SunComp->SetUseTemperature(true);
			SunComp->SetCastShadows(true);
			SunComp->SetCastVolumetricShadow(true);
			SunComp->SetVolumetricScatteringIntensity(1.35f);
			SunComp->SetIntensity(11.5f);
			SunComp->MarkRenderStateDirty();
		}
	}

	// 2. SkyAtmosphere
	for (TActorIterator<ASkyAtmosphere> It(World); It; ++It)
	{
		const USkyAtmosphereComponent* Comp = It->GetComponent();
		UE_LOG(LogChronicles, Display,
			TEXT("[AtmosphereInspect] SkyAtmosphere='%s' Visible=%d HiddenInGame=%d TransformMode=%d MultiScatter=%.3f"),
			*It->GetName(),
			Comp ? (Comp->IsVisible() ? 1 : 0) : -1,
			It->IsHidden() ? 1 : 0,
			Comp ? static_cast<int32>(Comp->TransformMode) : -1,
			Comp ? Comp->MultiScatteringFactor : -1.f);

		if (!SkyAtmosphere)
		{
			SkyAtmosphere = *It;
		}
	}
	if (!SkyAtmosphere)
	{
		SkyAtmosphere = World->SpawnActor<ASkyAtmosphere>(
			ASkyAtmosphere::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams);
	}
	if (SkyAtmosphere)
	{
		SkyAtmosphere->SetActorHiddenInGame(false);
		if (USkyAtmosphereComponent* AtmoComp = SkyAtmosphere->GetComponent())
		{
			AtmoComp->SetVisibility(true);
			AtmoComp->SetHiddenInGame(false);
			AtmoComp->TransformMode = ESkyAtmosphereTransformMode::PlanetTopAtAbsoluteWorldOrigin;
			AtmoComp->SetMultiScatteringFactor(1.25f);
			AtmoComp->SetRayleighScatteringScale(0.0331f);
			AtmoComp->SetMieScatteringScale(0.0045f);
			AtmoComp->SetMieAnisotropy(0.80f);
			AtmoComp->SetAerialPespectiveViewDistanceScale(1.15f);
			AtmoComp->MarkRenderStateDirty();
		}
	}

	// 3. SkyLight with real-time atmosphere capture (RecaptureSky is invoked in BeginPlay after UpdateAtmosphereForTimeOfDay)
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		const USkyLightComponent* Comp = It->GetLightComponent();
		UE_LOG(LogChronicles, Display,
			TEXT("[AtmosphereInspect] SkyLight='%s' Visible=%d HiddenInGame=%d Intensity=%.2f SourceType=%d RealTimeCapture=%d"),
			*It->GetName(),
			Comp ? (Comp->IsVisible() ? 1 : 0) : -1,
			It->IsHidden() ? 1 : 0,
			Comp ? Comp->Intensity : -1.f,
			Comp ? static_cast<int32>(Comp->SourceType.GetValue()) : -1,
			Comp ? (Comp->IsRealTimeCaptureEnabled() ? 1 : 0) : -1);

		if (!SkyLight)
		{
			SkyLight = *It;
		}
	}
	if (!SkyLight)
	{
		SkyLight = World->SpawnActor<ASkyLight>(
			ASkyLight::StaticClass(),
			FVector(0.f, 0.f, 1200.f),
			FRotator::ZeroRotator,
			SpawnParams);
	}
	if (SkyLight)
	{
		SkyLight->SetActorHiddenInGame(false);
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			SkyComp->SetMobility(EComponentMobility::Movable);
			SkyComp->SetVisibility(true);
			SkyComp->SetHiddenInGame(false);
			SkyComp->bAffectsWorld = true;
			SkyComp->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			SkyComp->SetRealTimeCapture(true);
			SkyComp->SetIntensity(1.55f);
			SkyComp->MarkRenderStateDirty();
		}
	}

	// 4. ExponentialHeightFog (Volumetric Fog enabled when RTX active, clean horizon & golden sky)
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		const UExponentialHeightFogComponent* Comp = It->GetComponent();
		UE_LOG(LogChronicles, Display,
			TEXT("[AtmosphereInspect] ExponentialHeightFog='%s' Visible=%d HiddenInGame=%d Density=%.5f MaxOpacity=%.2f StartDist=%.1f"),
			*It->GetName(),
			Comp ? (Comp->IsVisible() ? 1 : 0) : -1,
			It->IsHidden() ? 1 : 0,
			Comp ? Comp->FogDensity : -1.f,
			Comp ? Comp->FogMaxOpacity : -1.f,
			Comp ? Comp->StartDistance : -1.f);

		if (!HeightFog)
		{
			HeightFog = *It;
		}
	}
	if (!HeightFog)
	{
		HeightFog = World->SpawnActor<AExponentialHeightFog>(
			AExponentialHeightFog::StaticClass(),
			FVector(0.f, 0.f, FallbackGroundZ + 350.f),
			FRotator::ZeroRotator,
			SpawnParams);
	}
	if (HeightFog)
	{
		HeightFog->SetActorHiddenInGame(false);
		if (UExponentialHeightFogComponent* FogComp = HeightFog->GetComponent())
		{
			FogComp->SetVisibility(true);
			FogComp->SetHiddenInGame(false);
			FogComp->SetFogDensity(0.0038f);
			FogComp->SetFogHeightFalloff(0.22f);
			FogComp->SetFogMaxOpacity(0.55f);
			FogComp->SetStartDistance(1200.f);
			FogComp->SetVolumetricFog(bRTXActiveCached);
			FogComp->SetVolumetricFogScatteringDistribution(0.45f);
			FogComp->SetVolumetricFogExtinctionScale(0.65f);
			FogComp->SetVolumetricFogDistance(7500.f);
			FogComp->MarkRenderStateDirty();
		}
	}

	// 5. VolumetricCloud (Crimson Desert rolling highland clouds)
	for (TActorIterator<AVolumetricCloud> It(World); It; ++It)
	{
		const UVolumetricCloudComponent* Comp = It->FindComponentByClass<UVolumetricCloudComponent>();
		UE_LOG(LogChronicles, Display,
			TEXT("[AtmosphereInspect] VolumetricCloud='%s' Visible=%d HiddenInGame=%d BottomAlt=%.2f LayerHeight=%.2f"),
			*It->GetName(),
			Comp ? (Comp->IsVisible() ? 1 : 0) : -1,
			It->IsHidden() ? 1 : 0,
			Comp ? Comp->LayerBottomAltitude : -1.f,
			Comp ? Comp->LayerHeight : -1.f);

		if (!VolumetricCloud)
		{
			VolumetricCloud = *It;
		}
	}
	if (!VolumetricCloud)
	{
		VolumetricCloud = World->SpawnActor<AVolumetricCloud>(
			AVolumetricCloud::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams);
	}
	if (VolumetricCloud)
	{
		VolumetricCloud->SetActorHiddenInGame(false);
		if (UVolumetricCloudComponent* CloudComp = VolumetricCloud->FindComponentByClass<UVolumetricCloudComponent>())
		{
			CloudComp->SetVisibility(true);
			CloudComp->SetHiddenInGame(false);
			CloudComp->SetLayerBottomAltitude(3.8f);
			CloudComp->SetLayerHeight(6.5f);
			CloudComp->SetTracingStartMaxDistance(350.f);
			if (UMaterialInterface* CloudMat = LoadObject<UMaterialInterface>(nullptr, AetheriaWorldConst::MatSimpleCloud))
			{
				CloudComp->SetMaterial(CloudMat);
			}
			CloudComp->MarkRenderStateDirty();
		}
	}

	// 6. Unbound PostProcessVolume (AgX EV100 exposure range + Crimson Desert grading)
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		if (It->bUnbound)
		{
			PostProcessVolume = *It;
			break;
		}
	}
	if (!PostProcessVolume)
	{
		PostProcessVolume = World->SpawnActor<APostProcessVolume>(
			APostProcessVolume::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams);
	}
	if (PostProcessVolume)
	{
		PostProcessVolume->bUnbound = true;
		PostProcessVolume->Priority = 10.f;

		FPostProcessSettings& PP = PostProcessVolume->Settings;

		// EV100 luminance-range exposure (DefaultEngine.ini ExtendDefaultLuminanceRange=True)
		PP.bOverride_AutoExposureMethod = true;
		PP.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
		PP.bOverride_AutoExposureMinBrightness = true;
		PP.AutoExposureMinBrightness = -0.25f;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMaxBrightness = 1.65f;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = 0.25f;

		// Unified Crimson Desert AgX/Filmic highland grading (kills patchwork across mixed Fab packs)
		FAetheriaStyleHarmonizer::ApplyCrimsonDesertPostProcess(PP);
	}
}

void AAetheriaWorldBuilder::UpdateAtmosphereForTimeOfDay(float NormalizedTime)
{
	struct FAtmosphereKeyframe
	{
		float T;
		float PitchDeg;
		float YawDeg;
		float TemperatureK;
		float SunIntensity;
		float SkyIntensity;
		float FogDensity;
		FLinearColor SunColor;
		FLinearColor FogInscatter;
		FLinearColor DirectionalFog;
		FColor VolumetricAlbedo;
	};

	// 4 cyclical cardinal keyframes matching CycleTimeOfDayPreset()
	static const FAtmosphereKeyframe Keys[4] = {
		// 0.02 — Starry Full Moon (silver-blue 7200K moonlight)
		{
			0.02f, -34.f, 132.f, 7200.f, 2.2f, 0.65f, 0.0055f,
			FLinearColor(0.68f, 0.82f, 1.00f),
			FLinearColor(0.08f, 0.14f, 0.27f),
			FLinearColor(0.28f, 0.44f, 0.72f),
			FColor(140, 165, 210)
		},
		// 0.26 — Misty Dawn (soft rose-amber highland sunrise)
		{
			0.26f, -13.f, 54.f, 3600.f, 6.8f, 0.98f, 0.0048f,
			FLinearColor(1.00f, 0.81f, 0.69f),
			FLinearColor(0.46f, 0.38f, 0.44f),
			FLinearColor(0.95f, 0.62f, 0.44f),
			FColor(218, 198, 196)
		},
		// 0.48 — Highland Noon (crisp alpine daylight)
		{
			0.48f, -58.f, -14.f, 5900.f, 11.2f, 1.24f, 0.0032f,
			FLinearColor(1.00f, 0.98f, 0.94f),
			FLinearColor(0.36f, 0.54f, 0.76f),
			FLinearColor(0.75f, 0.86f, 0.98f),
			FColor(225, 232, 240)
		},
		// 0.72 — Golden Sunset (default Crimson Desert 4200K golden-hour spectacle with rich sky fill)
		{
			0.72f, -16.f, -42.f, 4200.f, 11.5f, 1.55f, 0.0035f,
			FLinearColor(1.00f, 0.85f, 0.66f),
			FLinearColor(0.56f, 0.44f, 0.38f),
			FLinearColor(1.00f, 0.62f, 0.30f),
			FColor(220, 202, 184)
		}
	};

	const float T = FMath::Fmod(NormalizedTime + 1.0f, 1.0f);

	int32 IndexA = 3;
	int32 IndexB = 0;
	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		const int32 NextIdx = (Idx + 1) % 4;
		const float TA = Keys[Idx].T;
		const float TB = Keys[NextIdx].T;
		if (TA < TB)
		{
			if (T >= TA && T < TB)
			{
				IndexA = Idx;
				IndexB = NextIdx;
				break;
			}
		}
		else
		{
			if (T >= TA || T < TB)
			{
				IndexA = Idx;
				IndexB = NextIdx;
				break;
			}
		}
	}

	const FAtmosphereKeyframe& A = Keys[IndexA];
	const FAtmosphereKeyframe& B = Keys[IndexB];

	const float Span = (B.T >= A.T) ? (B.T - A.T) : (1.0f - A.T + B.T);
	const float Dist = (T >= A.T) ? (T - A.T) : (1.0f - A.T + T);
	const float RawAlpha = (Span > KINDA_SMALL_NUMBER) ? FMath::Clamp(Dist / Span, 0.f, 1.f) : 0.f;
	const float Alpha = FMath::SmoothStep(0.f, 1.f, RawAlpha);

	const float Pitch = FMath::Lerp(A.PitchDeg, B.PitchDeg, Alpha);
	const float Yaw = FRotator::NormalizeAxis(A.YawDeg + FRotator::NormalizeAxis(B.YawDeg - A.YawDeg) * Alpha);
	const float TempK = FMath::Lerp(A.TemperatureK, B.TemperatureK, Alpha);
	const float SunInt = FMath::Lerp(A.SunIntensity, B.SunIntensity, Alpha);
	const float SkyInt = FMath::Lerp(A.SkyIntensity, B.SkyIntensity, Alpha);
	const float FogDen = FMath::Lerp(A.FogDensity, B.FogDensity, Alpha);
	const FLinearColor SunCol = FMath::Lerp(A.SunColor, B.SunColor, Alpha);
	const FLinearColor FogCol = FMath::Lerp(A.FogInscatter, B.FogInscatter, Alpha);
	const FLinearColor DirFogCol = FMath::Lerp(A.DirectionalFog, B.DirectionalFog, Alpha);

	if (SunLight)
	{
		SunLight->SetActorRotation(FRotator(Pitch, Yaw, 0.f));
		if (UDirectionalLightComponent* SunComp = SunLight->GetComponent())
		{
			SunComp->SetTemperature(TempK);
			SunComp->SetIntensity(SunInt);
			SunComp->SetLightColor(SunCol);
		}
	}

	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			SkyComp->SetIntensity(SkyInt);
		}
	}

	if (HeightFog)
	{
		if (UExponentialHeightFogComponent* FogComp = HeightFog->GetComponent())
		{
			FogComp->SetFogDensity(FogDen);
			FogComp->SetFogInscatteringColor(FogCol);
			FogComp->SetDirectionalInscatteringColor(DirFogCol);

			const FColor Albedo(
				static_cast<uint8>(FMath::Lerp(static_cast<float>(A.VolumetricAlbedo.R), static_cast<float>(B.VolumetricAlbedo.R), Alpha)),
				static_cast<uint8>(FMath::Lerp(static_cast<float>(A.VolumetricAlbedo.G), static_cast<float>(B.VolumetricAlbedo.G), Alpha)),
				static_cast<uint8>(FMath::Lerp(static_cast<float>(A.VolumetricAlbedo.B), static_cast<float>(B.VolumetricAlbedo.B), Alpha)),
				255);
			FogComp->SetVolumetricFogAlbedo(Albedo);
		}
	}
}

// ============================================================================
// Asset Loading & Placement Helpers
// ============================================================================

UStaticMesh* AAetheriaWorldBuilder::LoadMeshSafe(const TCHAR* AssetPath, const TCHAR* FallbackPath)
{
	if (!AssetPath || *AssetPath == 0)
	{
		return FallbackPath ? LoadMeshSafe(FallbackPath, nullptr) : nullptr;
	}

	const FString Key(AssetPath);
	if (const TObjectPtr<UStaticMesh>* Found = MeshCache.Find(Key))
	{
		if (*Found)
		{
			return Found->Get();
		}
	}

	UStaticMesh* Mesh = nullptr;
	const bool bIsEngineAsset = Key.StartsWith(TEXT("/Engine/"));
	const FString PkgName = bIsEngineAsset ? FString() : FPackageName::ObjectPathToPackageName(Key);
	if (bIsEngineAsset || (!PkgName.IsEmpty() && FPackageName::DoesPackageExist(PkgName)))
	{
		Mesh = LoadObject<UStaticMesh>(nullptr, AssetPath);
	}
	else
	{
		UE_LOG(LogChronicles, Warning, TEXT("[WorldBuilder] Package missing on disk for '%s' — using fallback."), *Key);
	}

	if (!Mesh && FallbackPath)
	{
		Mesh = LoadObject<UStaticMesh>(nullptr, FallbackPath);
	}
	if (Mesh)
	{
		MeshCache.Add(Key, Mesh);
		if (!bIsEngineAsset)
		{
			const FBox Box = Mesh->GetBoundingBox();
			UE_LOG(LogChronicles, Verbose, TEXT("[WorldBuilder] Loaded '%s' BoundsMin=%s BoundsMax=%s Size=%s"),
				*Mesh->GetName(), *Box.Min.ToCompactString(), *Box.Max.ToCompactString(), *Box.GetSize().ToCompactString());
		}
	}
	return Mesh;
}

UMaterialInstanceDynamic* AAetheriaWorldBuilder::CreateColorMID(const FLinearColor& Color, bool bEmissive)
{
	const TCHAR* MatPath = bEmissive ? AetheriaWorldConst::MatEmissive : AetheriaWorldConst::MatBasicShape;
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, MatPath);
	if (!BaseMat)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Color);
		DynamicMaterials.Add(MID);
	}
	return MID;
}

UStaticMeshComponent* AAetheriaWorldBuilder::AddWorldMesh(
	const FString& CompName,
	const TCHAR* MeshPath,
	const FVector& WorldLoc,
	const FRotator& WorldRot,
	const FVector& WorldScale,
	UMaterialInterface* OverrideMat,
	const TCHAR* FallbackMesh,
	bool bEnableCollision,
	USceneComponent* AttachParent)
{
	UStaticMesh* Mesh = LoadMeshSafe(MeshPath, FallbackMesh);
	if (!Mesh)
	{
		return nullptr;
	}

	const FName UniqueName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), FName(*CompName));
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, UniqueName);
	if (!Comp)
	{
		return nullptr;
	}

	USceneComponent* Parent = AttachParent ? AttachParent : WorldRoot.Get();
	Comp->SetStaticMesh(Mesh);
	Comp->SetMobility(AttachParent ? EComponentMobility::Movable : EComponentMobility::Static);

	if (OverrideMat)
	{
		const EBlendMode BlendMode = OverrideMat->GetBlendMode();
		const UMaterial* BaseMat = OverrideMat->GetBaseMaterial();
		const bool bIsEmissive = BaseMat && BaseMat->GetName().Contains(TEXT("Emissive"), ESearchCase::IgnoreCase);
		if (bIsEmissive || (BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked))
		{
			Comp->bDisallowNanite = true;
		}
		const int32 NumMats = FMath::Max(1, Comp->GetNumMaterials());
		for (int32 Idx = 0; Idx < NumMats; ++Idx)
		{
			Comp->SetMaterial(Idx, OverrideMat);
		}
	}

	if (bEnableCollision)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCollisionObjectType(ECC_WorldStatic);
		Comp->SetCollisionResponseToAllChannels(ECR_Block);
		Comp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Comp->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);
	}
	else
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	Comp->AttachToComponent(Parent, FAttachmentTransformRules::KeepWorldTransform);
	Comp->SetWorldLocationAndRotation(WorldLoc, WorldRot);
	Comp->SetWorldScale3D(WorldScale);
	Comp->RegisterComponent();

	SpawnedMeshes.Add(Comp);
	return Comp;
}

UPointLightComponent* AAetheriaWorldBuilder::AddAmbientFireLight(
	const FString& CompName,
	const FVector& WorldLoc,
	float BaseLumens,
	float TempKelvin,
	float AttenuationRadius,
	const FLinearColor& LightColor,
	float FlickerAmplitude,
	float PhaseOffset,
	bool bNightBoost)
{
	const FName UniqueName = MakeUniqueObjectName(this, UPointLightComponent::StaticClass(), FName(*CompName));
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this, UniqueName);
	if (!Light)
	{
		return nullptr;
	}

	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Lumens);
	Light->SetIntensity(BaseLumens);
	Light->SetUseTemperature(true);
	Light->SetTemperature(TempKelvin);
	Light->SetLightColor(LightColor);
	Light->SetAttenuationRadius(AttenuationRadius);
	Light->SetSourceRadius(18.f);
	Light->SetCastShadows(false);

	Light->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
	Light->SetWorldLocation(WorldLoc);
	Light->RegisterComponent();

	FWorldAnimatedLight Entry;
	Entry.Light = Light;
	Entry.BaseLumens = BaseLumens;
	Entry.FlickerAmplitude = FlickerAmplitude;
	Entry.FlickerSpeed = FMath::FRandRange(5.2f, 7.8f);
	Entry.PhaseOffset = PhaseOffset;
	Entry.bNightBoost = bNightBoost;
	AnimatedLights.Add(Entry);

	return Light;
}

ADestructiblePropActor* AAetheriaWorldBuilder::SpawnDestructibleProp(
	const FVector& WorldLoc,
	const FRotator& WorldRot,
	EDestructibleKind Kind,
	float InMaxHP,
	const FString& MeshPath,
	const FVector& Scale,
	FName QuestId,
	const FLinearColor* OptionalGlow,
	bool bEmissiveGlow)
{
	// Only authority spawns replicated destructible actors so multiplayer clients do not duplicate them
	if (!HasAuthority())
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADestructiblePropActor* Prop = World->SpawnActor<ADestructiblePropActor>(
		ADestructiblePropActor::StaticClass(),
		WorldLoc,
		WorldRot,
		SpawnParams);
	if (Prop)
	{
		Prop->ConfigureProp(Kind, InMaxHP, MeshPath, Scale, QuestId);
		if (OptionalGlow)
		{
			Prop->ApplyAccentGlow(*OptionalGlow, bEmissiveGlow);
		}
		SpawnedDestructibles.Add(Prop);
	}
	return Prop;
}

void AAetheriaWorldBuilder::AddBrazierAssembly(
	const FString& Prefix,
	const FVector& GroundLoc,
	float Lumens,
	float AttenuationRadius,
	const TCHAR* BrazierMeshPath,
	float ScaleFactor,
	float PhaseOffset)
{
	// Never use floating chandelier SM_IronBrazier — always ground with SM_StoneFirePit
	const TCHAR* SafeBrazierPath =
		(!BrazierMeshPath || FCString::Stristr(BrazierMeshPath, TEXT("SM_IronBrazier")) != nullptr)
			? AetheriaWorldConst::MeshStoneFirePit
			: BrazierMeshPath;

	UStaticMesh* PitMesh = LoadMeshSafe(SafeBrazierPath, AetheriaWorldConst::MeshCylinder);
	const float PitScale = ScaleFactor * AetheriaWorldHelpers::ComputeUniformScaleForHeight(PitMesh, 52.f, 0.25f, 40.f);
	const float PitZ = AetheriaWorldHelpers::ComputeGroundedZ(PitMesh, GroundLoc.Z, PitScale, 4.f);

	AddWorldMesh(
		Prefix + TEXT("_Base"),
		SafeBrazierPath,
		FVector(GroundLoc.X, GroundLoc.Y, PitZ),
		FRotator::ZeroRotator,
		FVector(PitScale),
		nullptr,
		AetheriaWorldConst::MeshCylinder,
		true);

	UStaticMesh* FlameMesh = LoadMeshSafe(AetheriaWorldConst::MeshBrazierFlame, AetheriaWorldConst::MeshCone);
	const float FlameScale = ScaleFactor * AetheriaWorldHelpers::ComputeUniformScaleForHeight(FlameMesh, 68.f, 0.2f, 40.f);
	const float FlameZ = AetheriaWorldHelpers::ComputeStackedZ(PitMesh, PitZ, PitScale, FlameMesh, FlameScale, 14.f);

	UMaterialInstanceDynamic* FlameMID = CreateColorMID(FLinearColor(2.4f, 0.88f, 0.12f, 1.0f), true);
	AddWorldMesh(
		Prefix + TEXT("_Flame"),
		AetheriaWorldConst::MeshBrazierFlame,
		FVector(GroundLoc.X, GroundLoc.Y, FlameZ),
		FRotator::ZeroRotator,
		FVector(FlameScale),
		FlameMID,
		AetheriaWorldConst::MeshCone,
		false);

	AddAmbientFireLight(
		Prefix + TEXT("_Light"),
		GroundLoc + FVector(0.f, 0.f, 115.f * ScaleFactor),
		Lumens,
		2300.f,
		AttenuationRadius,
		FLinearColor(1.0f, 0.62f, 0.24f, 1.0f),
		0.16f,
		PhaseOffset,
		true);
}

// ============================================================================
// 5-Biome Open World Construction
// ============================================================================

void AAetheriaWorldBuilder::BuildAllBiomes()
{
	if (bWorldBuilt)
	{
		return;
	}
	bWorldBuilt = true;

	BuildOakhavenVillage();
	BuildMirrorLakeAndBridge();
	BuildDragonAerie();
	BuildSapphireWhisperingWoods();
	BuildHighkeepCitadel();
	BuildAshCalderaAndEmberCrypt();
	BuildCrystalGlacierPeaks();
	BuildSkyRuneRings();

	UE_LOG(LogChronicles, Display,
		TEXT("[WorldBuilder] Built 5-biome Crimson Desert world: %d static meshes, %d animated lights, %d destructibles, %d boulders, %d trees."),
		SpawnedMeshes.Num(),
		AnimatedLights.Num(),
		SpawnedDestructibles.Num(),
		MossyBouldersISM ? MossyBouldersISM->GetInstanceCount() : 0,
		TreeTrunksISM ? TreeTrunksISM->GetInstanceCount() : 0);
}

void AAetheriaWorldBuilder::BuildOakhavenVillage()
{
	// Centered around (0, 4200); keep central plaza sightline X in [-350..+350], Y in [2800..4000] 100% open!
	UStaticMesh* Section1Mesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicSection1, AetheriaWorldConst::MeshCube);
	UStaticMesh* Block1Mesh   = LoadMeshSafe(AetheriaWorldConst::MeshGothicBlock1, AetheriaWorldConst::MeshCube);
	UStaticMesh* ArcMesh      = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshGothicArchSpan);
	UStaticMesh* StepsMesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicSteps, AetheriaWorldConst::MeshCube);

	const float SectionUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Section1Mesh, 380.f, 0.2f, 80.f);
	const float BlockUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Block1Mesh, 320.f, 0.2f, 80.f);
	const float ArcUnit     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ArcMesh, 360.f, 0.2f, 80.f);
	const float StepsUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StepsMesh, 95.f, 0.2f, 80.f);

	struct FCottageSpec
	{
		FVector2D Pos;
		float Yaw;
		float Scale;
	};

	const FCottageSpec Cottages[] = {
		{ FVector2D(-1750.f, 4680.f),  25.f, 1.05f },
		{ FVector2D( 1750.f, 4680.f), -25.f, 1.05f },
		{ FVector2D(-1950.f, 3820.f),  68.f, 1.00f },
		{ FVector2D( 1950.f, 3820.f), -68.f, 1.00f },
		{ FVector2D(-1250.f, 5450.f),  10.f, 1.10f },
		{ FVector2D( 1250.f, 5450.f), -10.f, 1.10f }
	};

	for (int32 Idx = 0; Idx < UE_ARRAY_COUNT(Cottages); ++Idx)
	{
		const FVector2D P = Cottages[Idx].Pos;
		const FRotator Rot(0.f, Cottages[Idx].Yaw, 0.f);
		const float S = Cottages[Idx].Scale;
		const FVector Forward = Rot.Vector();
		const FVector Right = Rot.RotateVector(FVector::RightVector);

		const float SecScale = SectionUnit * S;
		const float SecGroundZ = SampleGroundZ(P.X, P.Y, FallbackGroundZ);
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_CottageSection_%d"), Idx),
			AetheriaWorldConst::MeshGothicSection1,
			FVector(P.X, P.Y, AetheriaWorldHelpers::ComputeGroundedZ(Section1Mesh, SecGroundZ, SecScale, 12.f)),
			Rot,
			FVector(SecScale));

		const FVector BlockPos2D = FVector(P.X, P.Y, 0.f) - Forward * (140.f * S) + Right * (190.f * S);
		const float BlkScale = BlockUnit * S;
		const float BlkGroundZ = SampleGroundZ(BlockPos2D.X, BlockPos2D.Y, SecGroundZ);
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_CottageBlock_%d"), Idx),
			AetheriaWorldConst::MeshGothicBlock1,
			FVector(BlockPos2D.X, BlockPos2D.Y, AetheriaWorldHelpers::ComputeGroundedZ(Block1Mesh, BlkGroundZ, BlkScale, 12.f)),
			Rot,
			FVector(BlkScale));

		const FVector ArcPos2D = FVector(P.X, P.Y, 0.f) + Forward * (250.f * S);
		const float ArcScale = ArcUnit * S;
		const float ArcGroundZ = SampleGroundZ(ArcPos2D.X, ArcPos2D.Y, SecGroundZ);
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_CottageArc_%d"), Idx),
			AetheriaWorldConst::MeshGothicArc,
			FVector(ArcPos2D.X, ArcPos2D.Y, AetheriaWorldHelpers::ComputeGroundedZ(ArcMesh, ArcGroundZ, ArcScale, 10.f)),
			Rot,
			FVector(ArcScale));

		const FVector StepsPos2D = FVector(P.X, P.Y, 0.f) + Forward * (360.f * S);
		const float StpScale = StepsUnit * S;
		const float StpGroundZ = SampleGroundZ(StepsPos2D.X, StepsPos2D.Y, SecGroundZ);
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_CottageSteps_%d"), Idx),
			AetheriaWorldConst::MeshGothicSteps,
			FVector(StepsPos2D.X, StepsPos2D.Y, AetheriaWorldHelpers::ComputeGroundedZ(StepsMesh, StpGroundZ, StpScale, 8.f)),
			Rot,
			FVector(StpScale));

		const FVector BarrelPos2D = FVector(P.X, P.Y, 0.f) + Forward * (210.f * S) - Right * (165.f * S);
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_Barrel_%d"), Idx),
			AetheriaWorldConst::MeshGothicBarrel,
			FVector(BarrelPos2D.X, BarrelPos2D.Y, SampleGroundZ(BarrelPos2D.X, BarrelPos2D.Y, SecGroundZ)),
			FRotator(0.f, Idx * 47.f, 0.f),
			FVector(1.0f));
	}

	// Village Sanctuary Shrine & Central Runic Altar (north of central plaza Y=2800..4000)
	const float ShrineZ = SampleGroundZ(0.f, 4780.f, FallbackGroundZ) - 10.f;
	AddWorldMesh(
		TEXT("Oakhaven_SanctuaryShrine"),
		AetheriaWorldConst::MeshSanctuaryShrine,
		FVector(0.f, 4780.f, ShrineZ),
		FRotator(0.f, 180.f, 0.f),
		FVector(1.55f));

	const float AltarZ = SampleGroundZ(0.f, 4320.f, FallbackGroundZ) - 8.f;
	AddWorldMesh(
		TEXT("Oakhaven_RunicAltar"),
		AetheriaWorldConst::MeshRunicAltar,
		FVector(0.f, 4320.f, AltarZ),
		FRotator::ZeroRotator,
		FVector(1.35f));

	AddWorldMesh(
		TEXT("Oakhaven_TreasureChest"),
		AetheriaWorldConst::MeshTreasureChest,
		FVector(260.f, 4620.f, SampleGroundZ(260.f, 4620.f, FallbackGroundZ)),
		FRotator(0.f, -35.f, 0.f),
		FVector(1.2f));

	// 2 Photoreal TA_Sample_Statue_01 monuments on SM_TA_Sample_Pedestal_01 overlooking the central plaza
	UStaticMesh* PedestalMesh = LoadMeshSafe(AetheriaWorldConst::MeshStatuePedestal, AetheriaWorldConst::MeshCylinder);
	UStaticMesh* StatueMesh   = LoadMeshSafe(AetheriaWorldConst::MeshStatue01, AetheriaWorldConst::MeshGothicPillar);
	const float PedestalScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PedestalMesh, 135.f, 0.1f, 120.f);
	const float StatueScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StatueMesh, 240.f, 0.1f, 120.f);

	const FVector2D PlazaStatueCoords[2] = { FVector2D(-360.f, 4180.f), FVector2D(360.f, 4180.f) };
	const float PlazaStatueYaws[2] = { -65.f, -115.f };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float SX = PlazaStatueCoords[Idx].X;
		const float SY = PlazaStatueCoords[Idx].Y;
		const float GroundZ = SampleGroundZ(SX, SY, FallbackGroundZ);
		const float PedestalZ = AetheriaWorldHelpers::ComputeGroundedZ(PedestalMesh, GroundZ, PedestalScale, 6.f);
		const float StatueZ = AetheriaWorldHelpers::ComputeStackedZ(PedestalMesh, PedestalZ, PedestalScale, StatueMesh, StatueScale, 3.f);

		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_PlazaPedestal_%d"), Idx),
			AetheriaWorldConst::MeshStatuePedestal,
			FVector(SX, SY, PedestalZ),
			FRotator(0.f, PlazaStatueYaws[Idx], 0.f),
			FVector(PedestalScale));

		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_PlazaStatue_%d"), Idx),
			AetheriaWorldConst::MeshStatue01,
			FVector(SX, SY, StatueZ),
			FRotator(0.f, PlazaStatueYaws[Idx], 0.f),
			FVector(StatueScale));
	}

	// 6 Grounded Street Lanterns (SM_Pillar2 + SM_StoneFirePit flame bowl + 650 lm warm 2400K)
	UStaticMesh* Pillar2Mesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar2, AetheriaWorldConst::MeshGothicPillar);
	UStaticMesh* BowlMesh    = LoadMeshSafe(AetheriaWorldConst::MeshStoneFirePit, AetheriaWorldConst::MeshCylinder);
	UStaticMesh* FlameMesh   = LoadMeshSafe(AetheriaWorldConst::MeshBrazierFlame, AetheriaWorldConst::MeshCone);
	const float Pillar2Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Pillar2Mesh, 205.f, 0.1f, 100.f);
	const float BowlScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BowlMesh, 28.f, 0.1f, 50.f);
	const float FlameScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(FlameMesh, 44.f, 0.1f, 50.f);
	UMaterialInstanceDynamic* LanternFlameMID = CreateColorMID(FLinearColor(2.2f, 0.82f, 0.14f, 1.0f), true);

	const FVector2D LanternCoords[] = {
		FVector2D(-520.f, 4420.f),
		FVector2D( 520.f, 4420.f),
		FVector2D(-560.f, 3780.f),
		FVector2D( 560.f, 3780.f),
		FVector2D(-420.f, 3050.f),
		FVector2D( 420.f, 3050.f)
	};

	for (int32 Idx = 0; Idx < UE_ARRAY_COUNT(LanternCoords); ++Idx)
	{
		const float LX = LanternCoords[Idx].X;
		const float LY = LanternCoords[Idx].Y;
		const float LZ = SampleGroundZ(LX, LY, FallbackGroundZ);
		const float PostZ  = AetheriaWorldHelpers::ComputeGroundedZ(Pillar2Mesh, LZ, Pillar2Scale, 6.f);
		const float BowlZ  = AetheriaWorldHelpers::ComputeStackedZ(Pillar2Mesh, PostZ, Pillar2Scale, BowlMesh, BowlScale, 4.f);
		const float FlameZ = AetheriaWorldHelpers::ComputeStackedZ(BowlMesh, BowlZ, BowlScale, FlameMesh, FlameScale, 8.f);

		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_LanternPost_%d"), Idx),
			AetheriaWorldConst::MeshGothicPillar2,
			FVector(LX, LY, PostZ),
			FRotator(0.f, Idx * 60.f, 0.f),
			FVector(Pillar2Scale));

		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_LanternBowl_%d"), Idx),
			AetheriaWorldConst::MeshStoneFirePit,
			FVector(LX, LY, BowlZ),
			FRotator::ZeroRotator,
			FVector(BowlScale));

		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_LanternFlame_%d"), Idx),
			AetheriaWorldConst::MeshBrazierFlame,
			FVector(LX, LY, FlameZ),
			FRotator::ZeroRotator,
			FVector(FlameScale),
			LanternFlameMID,
			AetheriaWorldConst::MeshCone,
			false);

		AddAmbientFireLight(
			FString::Printf(TEXT("Oakhaven_LanternLight_%d"), Idx),
			FVector(LX, LY, LZ + 235.f),
			650.f,
			2400.f,
			1100.f,
			FLinearColor(1.0f, 0.72f, 0.36f, 1.0f),
			0.09f,
			Idx * 1.15f,
			true);
	}

	// 2 Destructible Barrier Crystals flanking south village approach (LinkedQuestId = "Q_AwakenExort")
	// Placed outside X in [-350..+350] with SM_GothicPillar so the central plaza sightline stays 100% open
	UStaticMesh* ExortPillarMesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar, AetheriaWorldConst::MeshCylinder);
	const FLinearColor ExortAmberGlow(1.35f, 0.36f, 0.04f, 1.0f);
	const FVector2D CrystalCoords[2] = { FVector2D(-560.f, 3250.f), FVector2D(560.f, 3250.f) };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float CX = CrystalCoords[Idx].X;
		const float CY = CrystalCoords[Idx].Y;
		const float CZ = SampleGroundZ(CX, CY, FallbackGroundZ);
		const float GroundedCrystalZ = AetheriaWorldHelpers::ComputeGroundedZ(ExortPillarMesh, CZ, 1.2f, 6.f);
		SpawnDestructibleProp(
			FVector(CX, CY, GroundedCrystalZ),
			FRotator(0.f, Idx * 45.f, 0.f),
			EDestructibleKind::Crystal,
			90.f,
			AetheriaWorldConst::MeshGothicPillar,
			FVector(0.7f, 0.7f, 1.2f),
			FName(TEXT("Q_AwakenExort")),
			&ExortAmberGlow,
			true);
	}
}

void AAetheriaWorldBuilder::BuildMirrorLakeAndBridge()
{
	// Mirror Lake centered around (0, -800), arched stone bridge spanning Y: 1600 -> -3200
	const float NorthBankZ = SampleGroundZ(0.f, 1600.f, FallbackGroundZ);
	const float CenterZ    = SampleGroundZ(0.f, -800.f, FallbackGroundZ);
	const float SouthBankZ = SampleGroundZ(0.f, -3200.f, FallbackGroundZ);
	const float BankAvgZ   = 0.5f * (NorthBankZ + SouthBankZ);
	const float WaterZ     = FMath::Min(CenterZ + 12.f, BankAvgZ - 18.f);

	// Glossy reflective water plane across Mirror Lake
	MirrorWaterMID = CreateColorMID(FLinearColor(0.025f, 0.13f, 0.21f, 1.0f), false);
	if (MirrorWaterMID)
	{
		MirrorWaterMID->SetScalarParameterValue(TEXT("Roughness"), 0.05f);
		MirrorWaterMID->SetScalarParameterValue(TEXT("Metallic"), 0.18f);
	}

	AddWorldMesh(
		TEXT("MirrorLake_WaterSurface"),
		AetheriaWorldConst::MeshPlane,
		FVector(0.f, -800.f, WaterZ),
		FRotator::ZeroRotator,
		FVector(68.f, 52.f, 1.f),
		MirrorWaterMID,
		AetheriaWorldConst::MeshPlane,
		false);

	// Arched Stone Bridge spanning Y: 1600 -> -3200 (4800 cm total length, 9 smooth arch spans)
	const int32 NumSpans = 9;
	const float StartY = 1600.f;
	const float EndY   = -3200.f;
	const float ArchApexLift = 210.f;

	UMaterialInstanceDynamic* BridgeStoneMID = CreateColorMID(FLinearColor(0.42f, 0.39f, 0.35f, 1.0f), false);

	auto EvalBridgePoint = [&](float Alpha) -> FVector
	{
		const float Y = FMath::Lerp(StartY, EndY, Alpha);
		const float ParabolicLift = 4.f * Alpha * (1.f - Alpha) * ArchApexLift;
		const float BaseZ = FMath::Lerp(NorthBankZ, SouthBankZ, Alpha);
		const float DeckZ = FMath::Max(BaseZ + 18.f, WaterZ + 75.f) + ParabolicLift;
		return FVector(0.f, Y, DeckZ);
	};

	for (int32 Idx = 0; Idx < NumSpans; ++Idx)
	{
		const float Alpha0 = static_cast<float>(Idx) / static_cast<float>(NumSpans);
		const float Alpha1 = static_cast<float>(Idx + 1) / static_cast<float>(NumSpans);
		const FVector P0 = EvalBridgePoint(Alpha0);
		const FVector P1 = EvalBridgePoint(Alpha1);
		const FVector Mid = 0.5f * (P0 + P1);
		const float SegLen = FVector::Dist(P0, P1);
		const FRotator DeckRot = FRotationMatrix::MakeFromY((P1 - P0).GetSafeNormal()).Rotator();

		// Solid walkable stone bridge deck slab aligned exactly along the parabolic arch tangent
		AddWorldMesh(
			FString::Printf(TEXT("MirrorBridge_Deck_%d"), Idx),
			AetheriaWorldConst::MeshCube,
			Mid - FVector(0.f, 0.f, 28.f),
			DeckRot,
			FVector(6.2f, (SegLen / 100.f) + 0.35f, 0.56f),
			BridgeStoneMID,
			AetheriaWorldConst::MeshCube,
			true);

		// Supporting gothic arches underneath the bridge deck
		if (Idx > 0 && Idx < NumSpans - 1)
		{
			AddWorldMesh(
				FString::Printf(TEXT("MirrorBridge_Arch_%d"), Idx),
				AetheriaWorldConst::MeshGothicArchSpan,
				FVector(0.f, Mid.Y, WaterZ - 20.f),
				FRotator(0.f, 90.f, 0.f),
				FVector(1.35f, 1.55f, 1.25f));
		}

		// Left & right carved stone parapet walls
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddWorldMesh(
				FString::Printf(TEXT("MirrorBridge_Parapet_%d_%d"), Idx, Side > 0 ? 1 : 0),
				AetheriaWorldConst::MeshCathedralLow,
				FVector(Side * 310.f, Mid.Y, Mid.Z - 6.f),
				FRotator(0.f, 90.f, 0.f),
				FVector(0.95f, 0.65f, 0.68f));
		}
	}

	// 6 Flaming Bridge Braziers (SM_StoneFirePit + 1,100 lm warm fire light)
	const float BrazierYs[3] = { 1420.f, -800.f, -3020.f };
	for (int32 Row = 0; Row < 3; ++Row)
	{
		const float Alpha = (StartY - BrazierYs[Row]) / (StartY - EndY);
		const float Lift = 4.f * Alpha * (1.f - Alpha) * ArchApexLift;
		const float BaseZ = FMath::Lerp(NorthBankZ, SouthBankZ, Alpha);
		const float DeckZ = FMath::Max(BaseZ + 18.f, WaterZ + 75.f) + Lift;

		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const int32 BIdx = Row * 2 + (Side > 0 ? 1 : 0);
			AddBrazierAssembly(
				FString::Printf(TEXT("MirrorBridge_Brazier_%d"), BIdx),
				FVector(Side * 285.f, BrazierYs[Row], DeckZ + 4.f),
				1100.f,
				1450.f,
				AetheriaWorldConst::MeshStoneFirePit,
				1.15f,
				BIdx * 0.9f);
		}
	}
}

void AAetheriaWorldBuilder::BuildDragonAerie()
{
	// Elevated runic stone dais at (-3400, 2200)
	const float CenterX = -3400.f;
	const float CenterY =  2200.f;
	const float BaseZ   = SampleGroundZ(CenterX, CenterY, FallbackGroundZ);

	// Raised stone foundation plinth + Runic Dais + Altar
	UMaterialInstanceDynamic* DaisStoneMID = CreateColorMID(FLinearColor(0.36f, 0.35f, 0.38f, 1.0f), false);
	AddWorldMesh(
		TEXT("DragonAerie_Plinth"),
		AetheriaWorldConst::MeshCylinder,
		FVector(CenterX, CenterY, BaseZ + 35.f),
		FRotator::ZeroRotator,
		FVector(12.5f, 12.5f, 1.4f),
		DaisStoneMID,
		AetheriaWorldConst::MeshCylinder,
		true);

	UStaticMesh* AeriePedMesh = LoadMeshSafe(AetheriaWorldConst::MeshDungeonDais, AetheriaWorldConst::MeshCylinder);
	const float AeriePedScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(AeriePedMesh, 95.f, 0.1f, 3.0f);
	AddWorldMesh(
		TEXT("DragonAerie_Dais"),
		AetheriaWorldConst::MeshDungeonDais,
		FVector(CenterX, CenterY, AetheriaWorldHelpers::ComputeGroundedZ(AeriePedMesh, BaseZ + 65.f, AeriePedScale, 4.f)),
		FRotator::ZeroRotator,
		FVector(AeriePedScale));

	AddWorldMesh(
		TEXT("DragonAerie_Altar"),
		AetheriaWorldConst::MeshRunicAltar,
		FVector(CenterX, CenterY, BaseZ + 115.f),
		FRotator(0.f, -35.f, 0.f),
		FVector(1.45f));

	// Ring of 6 weathered gothic pillars & ruined arches overlooking Mirror Lake
	for (int32 Idx = 0; Idx < 6; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 6.f) * UE_TWO_PI;
		const float PX = CenterX + FMath::Cos(Angle) * 620.f;
		const float PY = CenterY + FMath::Sin(Angle) * 620.f;
		const float PZ = SampleGroundZ(PX, PY, BaseZ) - 18.f;
		const float YawDeg = FMath::RadiansToDegrees(Angle) + 90.f;

		AddWorldMesh(
			FString::Printf(TEXT("DragonAerie_Arch_%d"), Idx),
			(Idx % 2 == 0) ? AetheriaWorldConst::MeshRuinedArch : AetheriaWorldConst::MeshGothicPillar,
			FVector(PX, PY, PZ),
			FRotator(0.f, YawDeg, 0.f),
			FVector(1.4f));
	}

	// 2 Beacon Braziers at the Aerie stairs (SM_StoneFirePit + 1,100 lm)
	AddBrazierAssembly(TEXT("DragonAerie_BrazierL"), FVector(CenterX - 360.f, CenterY + 460.f, BaseZ + 102.f), 1100.f, 1450.f, AetheriaWorldConst::MeshStoneFirePit, 1.1f, 0.4f);
	AddBrazierAssembly(TEXT("DragonAerie_BrazierR"), FVector(CenterX + 360.f, CenterY + 460.f, BaseZ + 102.f), 1100.f, 1450.f, AetheriaWorldConst::MeshStoneFirePit, 1.1f, 2.2f);

	// 2 Destructible Bind-Crystals (LinkedQuestId = "Q_TameDragon")
	const FLinearColor AzureBindGlow(0.18f, 1.55f, 3.0f, 1.0f);
	const FVector2D BindCoords[2] = {
		FVector2D(CenterX - 340.f, CenterY - 190.f),
		FVector2D(CenterX + 340.f, CenterY - 190.f)
	};

	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		SpawnDestructibleProp(
			FVector(BindCoords[Idx].X, BindCoords[Idx].Y, BaseZ + 190.f),
			FRotator(0.f, Idx * 60.f, Idx == 0 ? 10.f : -10.f),
			EDestructibleKind::Crystal,
			110.f,
			FString(),
			FVector(0.9f, 0.9f, 2.5f),
			FName(TEXT("Q_TameDragon")),
			&AzureBindGlow,
			true);
	}
}

void AAetheriaWorldBuilder::BuildSapphireWhisperingWoods()
{
	// X: -11000..-4200 (West) and 4200..11000 (East), Y: -7600..6600
	UStaticMesh* RockMesh   = LoadMeshSafe(AetheriaWorldConst::MeshCliffBoulder, AetheriaWorldConst::MeshSphere);
	UStaticMesh* TrunkMesh  = LoadMeshSafe(AetheriaWorldConst::MeshDeadTree, AetheriaWorldConst::MeshCylinder);
	UStaticMesh* CanopyMesh = LoadMeshSafe(AetheriaWorldConst::MeshSphere, AetheriaWorldConst::MeshSphere);

	if (MossyBouldersISM && RockMesh)
	{
		MossyBouldersISM->SetStaticMesh(RockMesh);
	}
	if (TreeTrunksISM && TrunkMesh)
	{
		TreeTrunksISM->SetStaticMesh(TrunkMesh);
	}
	if (TreeCanopiesISM && CanopyMesh)
	{
		TreeCanopiesISM->SetStaticMesh(CanopyMesh);
		if (UMaterialInstanceDynamic* BlueLeafMID = CreateColorMID(FLinearColor(0.07f, 0.30f, 0.48f, 1.0f), false))
		{
			TreeCanopiesISM->SetMaterial(0, BlueLeafMID);
		}
	}

	FRandomStream Rng(133742);

	auto IsExcludedLandmarkClearing = [](float X, float Y) -> bool
	{
		const FVector2D P(X, Y);
		// Keep clearings around Dragon Aerie, Ember Crypt, and Crystal Glacier Peaks
		if (FVector2D::Distance(P, FVector2D(-3400.f,  2200.f)) < 1300.f) return true;
		if (FVector2D::Distance(P, FVector2D( 9800.f, -2400.f)) < 2100.f) return true;
		if (FVector2D::Distance(P, FVector2D(-6800.f, -6400.f)) < 2100.f) return true;
		return false;
	};

	// ~160 instanced mossy boulders (rock_moss_set_01)
	TArray<FTransform> BoulderTransforms;
	BoulderTransforms.Reserve(160);
	int32 Attempts = 0;
	while (BoulderTransforms.Num() < 160 && Attempts < 1200)
	{
		++Attempts;
		const bool bWest = (BoulderTransforms.Num() % 2 == 0);
		const float X = bWest ? Rng.FRandRange(-11000.f, -4200.f) : Rng.FRandRange(4200.f, 11000.f);
		const float Y = Rng.FRandRange(-7400.f, 6400.f);
		if (IsExcludedLandmarkClearing(X, Y))
		{
			continue;
		}

		const float GZ = SampleGroundZ(X, Y, FallbackGroundZ) - 18.f;
		const FRotator Rot(Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-8.f, 8.f));
		const float S = Rng.FRandRange(1.1f, 3.1f);
		BoulderTransforms.Emplace(Rot, FVector(X, Y, GZ), FVector(S, S * Rng.FRandRange(0.85f, 1.25f), S * Rng.FRandRange(0.75f, 1.2f)));
	}
	if (MossyBouldersISM && BoulderTransforms.Num() > 0)
	{
		MossyBouldersISM->AddInstances(BoulderTransforms, false, true, false);
	}

	// ~140 stylized highland/blue-leaf trees (dead_tree_trunk_02 + layered canopy clusters)
	TArray<FTransform> TrunkTransforms;
	TArray<FTransform> CanopyTransforms;
	TrunkTransforms.Reserve(140);
	CanopyTransforms.Reserve(360);

	Attempts = 0;
	while (TrunkTransforms.Num() < 140 && Attempts < 1200)
	{
		++Attempts;
		const bool bWest = (TrunkTransforms.Num() % 2 == 0);
		const float X = bWest ? Rng.FRandRange(-10800.f, -4300.f) : Rng.FRandRange(4300.f, 10800.f);
		const float Y = Rng.FRandRange(-7200.f, 6200.f);
		if (IsExcludedLandmarkClearing(X, Y))
		{
			continue;
		}

		const float GZ = SampleGroundZ(X, Y, FallbackGroundZ) - 22.f;
		const float TreeScale = Rng.FRandRange(1.45f, 2.75f);
		const float Yaw = Rng.FRandRange(0.f, 360.f);
		const FRotator TrunkRot(Rng.FRandRange(-4.f, 4.f), Yaw, Rng.FRandRange(-4.f, 4.f));

		TrunkTransforms.Emplace(TrunkRot, FVector(X, Y, GZ), FVector(TreeScale, TreeScale, TreeScale * Rng.FRandRange(0.95f, 1.30f)));

		// 2-3 layered asymmetrical sapphire foliage crowns per tree
		const float CrownBaseZ = GZ + 280.f * TreeScale;
		const int32 CrownLayers = Rng.RandRange(2, 3);
		for (int32 Layer = 0; Layer < CrownLayers; ++Layer)
		{
			const float LayerAngle = Yaw + Layer * 125.f;
			const float OffsetR = (Layer == 0) ? 0.f : Rng.FRandRange(65.f, 130.f) * (TreeScale * 0.55f);
			const FVector CanopyPos(
				X + FMath::Cos(FMath::DegreesToRadians(LayerAngle)) * OffsetR,
				Y + FMath::Sin(FMath::DegreesToRadians(LayerAngle)) * OffsetR,
				CrownBaseZ + Layer * (75.f * TreeScale * 0.55f));
			const float CrownRad = TreeScale * Rng.FRandRange(1.55f, 2.25f) * (1.0f - Layer * 0.14f);
			CanopyTransforms.Emplace(
				FRotator(0.f, LayerAngle, 0.f),
				CanopyPos,
				FVector(CrownRad, CrownRad * Rng.FRandRange(0.85f, 1.15f), CrownRad * 0.62f));
		}
	}

	if (TreeTrunksISM && TrunkTransforms.Num() > 0)
	{
		TreeTrunksISM->AddInstances(TrunkTransforms, false, true, false);
	}
	if (TreeCanopiesISM && CanopyTransforms.Num() > 0)
	{
		TreeCanopiesISM->AddInstances(CanopyTransforms, false, true, false);
	}

	// 3 Storm Conductor Monoliths (LinkedQuestId = "Q_StormSpires")
	const FLinearColor WexStormGlow(0.75f, 0.38f, 3.0f, 1.0f);
	const FVector2D MonolithCoords[3] = {
		FVector2D(-6200.f,  1200.f),
		FVector2D( 6400.f,  3200.f),
		FVector2D( 7200.f, -1400.f)
	};

	for (int32 Idx = 0; Idx < 3; ++Idx)
	{
		const float MX = MonolithCoords[Idx].X;
		const float MY = MonolithCoords[Idx].Y;
		const float MZ = SampleGroundZ(MX, MY, FallbackGroundZ);

		// Grounded stone pedestal beneath each monolith
		UStaticMesh* MonoPedMesh = LoadMeshSafe(AetheriaWorldConst::MeshDungeonDais, AetheriaWorldConst::MeshCylinder);
		const float MonoPedScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(MonoPedMesh, 85.f, 0.1f, 3.0f);
		AddWorldMesh(
			FString::Printf(TEXT("StormMonolith_Pedestal_%d"), Idx),
			AetheriaWorldConst::MeshDungeonDais,
			FVector(MX, MY, AetheriaWorldHelpers::ComputeGroundedZ(MonoPedMesh, MZ, MonoPedScale, 6.f)),
			FRotator(0.f, Idx * 40.f, 0.f),
			FVector(MonoPedScale));

		SpawnDestructibleProp(
			FVector(MX, MY, MZ + 35.f),
			FRotator(0.f, Idx * 55.f, 0.f),
			EDestructibleKind::Crystal,
			130.f,
			AetheriaWorldConst::MeshGothicPillar,
			FVector(1.35f, 1.35f, 1.65f),
			FName(TEXT("Q_StormSpires")),
			&WexStormGlow,
			true);

		AddAmbientFireLight(
			FString::Printf(TEXT("StormMonolith_Glow_%d"), Idx),
			FVector(MX, MY, MZ + 310.f),
			1200.f,
			8000.f,
			1300.f,
			FLinearColor(0.62f, 0.42f, 1.0f, 1.0f),
			0.22f,
			Idx * 1.7f,
			true);
	}
}

void AAetheriaWorldBuilder::BuildHighkeepCitadel()
{
	// Monumental fortress courtyard & boss arena centered at (0, -9800); Malakor stands at (0, -10400)
	const float CX = 0.f;
	const float CY = -9800.f;
	const float ArenaZ = SampleGroundZ(CX, CY, FallbackGroundZ);

	UStaticMesh* GateMesh     = LoadMeshSafe(AetheriaWorldConst::MeshGothicGate, AetheriaWorldConst::MeshGothicArchSpan);
	UStaticMesh* PedestalMesh = LoadMeshSafe(AetheriaWorldConst::MeshStatuePedestal, AetheriaWorldConst::MeshCylinder);
	UStaticMesh* StatueMesh   = LoadMeshSafe(AetheriaWorldConst::MeshStatue01, AetheriaWorldConst::MeshGothicPillar);
	UStaticMesh* ArcMesh      = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshGothicArchSpan);
	UStaticMesh* Section1Mesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicSection1, AetheriaWorldConst::MeshCathedralArched);
	UStaticMesh* BallistaMesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicBallista, AetheriaWorldConst::MeshCube);
	UStaticMesh* StepsMesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicSteps, AetheriaWorldConst::MeshCube);
	UStaticMesh* Column3Mesh  = LoadMeshSafe(AetheriaWorldConst::MeshGothicColumn3, AetheriaWorldConst::MeshGothicPillar);

	const float GateScale     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GateMesh, 680.f, 0.2f, 120.f);
	const float PedestalScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PedestalMesh, 165.f, 0.1f, 120.f);
	const float StatueScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StatueMesh, 320.f, 0.1f, 120.f);
	const float ArcScale      = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ArcMesh, 650.f, 0.2f, 120.f);
	const float Section1Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Section1Mesh, 650.f, 0.2f, 120.f);
	const float BallistaScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BallistaMesh, 185.f, 0.1f, 100.f);
	const float StepsScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StepsMesh, 125.f, 0.1f, 100.f);
	const float Column3Scale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Column3Mesh, 540.f, 0.2f, 120.f);

	// Towering Citadel Fortress Keep moved far back to (0, -12400) so it NEVER engulfs Malakor or the courtyard
	const float KeepY = -12400.f;
	AddWorldMesh(
		TEXT("Highkeep_CitadelFortress"),
		AetheriaWorldConst::MeshCitadelFortress,
		FVector(CX, KeepY, SampleGroundZ(CX, KeepY, ArenaZ) - 25.f),
		FRotator(0.f, 0.f, 0.f),
		FVector(2.25f));

	// Grand Northern Gothic Gate at Y = -8250 facing Mirror Lake
	const float GateY = -8250.f;
	const float GateGroundZ = SampleGroundZ(CX, GateY, ArenaZ);
	AddWorldMesh(
		TEXT("Highkeep_GothicGate"),
		AetheriaWorldConst::MeshGothicGate,
		FVector(CX, GateY, AetheriaWorldHelpers::ComputeGroundedZ(GateMesh, GateGroundZ, GateScale, 10.f)),
		FRotator(0.f, 0.f, 0.f),
		FVector(GateScale));

	// Approach Steps at North Gate & South Citadel Terrace
	const float NorthStepsZ = SampleGroundZ(CX, -8020.f, GateGroundZ);
	AddWorldMesh(
		TEXT("Highkeep_NorthSteps"),
		AetheriaWorldConst::MeshGothicSteps,
		FVector(CX, -8020.f, AetheriaWorldHelpers::ComputeGroundedZ(StepsMesh, NorthStepsZ, StepsScale, 8.f)),
		FRotator(0.f, 0.f, 0.f),
		FVector(StepsScale * 1.4f, StepsScale * 1.2f, StepsScale));

	const float SouthStepsZ = SampleGroundZ(CX, CY - 1450.f, ArenaZ);
	AddWorldMesh(
		TEXT("Highkeep_SouthSteps"),
		AetheriaWorldConst::MeshGothicSteps,
		FVector(CX, CY - 1450.f, AetheriaWorldHelpers::ComputeGroundedZ(StepsMesh, SouthStepsZ, StepsScale, 8.f)),
		FRotator(0.f, 180.f, 0.f),
		FVector(StepsScale * 1.6f, StepsScale * 1.2f, StepsScale));

	// Sentinel TA_Sample_Statue_01 monuments on pedestals flanking the north gate
	const float StatueCoordsX[2] = { CX - 540.f, CX + 540.f };
	const float StatueYaws[2]    = { 65.f, 115.f };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float SX = StatueCoordsX[Idx];
		const float SY = -8320.f;
		const float GroundZ = SampleGroundZ(SX, SY, ArenaZ);
		const float PedZ = AetheriaWorldHelpers::ComputeGroundedZ(PedestalMesh, GroundZ, PedestalScale, 8.f);
		const float StatZ = AetheriaWorldHelpers::ComputeStackedZ(PedestalMesh, PedZ, PedestalScale, StatueMesh, StatueScale, 3.f);

		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_GatePedestal_%d"), Idx),
			AetheriaWorldConst::MeshStatuePedestal,
			FVector(SX, SY, PedZ),
			FRotator(0.f, StatueYaws[Idx], 0.f),
			FVector(PedestalScale));

		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_GateStatue_%d"), Idx),
			AetheriaWorldConst::MeshStatue01,
			FVector(SX, SY, StatZ),
			FRotator(0.f, StatueYaws[Idx], 0.f),
			FVector(StatueScale));
	}

	// 2 War Ballistas flanking the southern courtyard
	const float BallistaX[2]   = { CX - 1180.f, CX + 1180.f };
	const float BallistaYaw[2] = { 65.f, 115.f };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float BX = BallistaX[Idx];
		const float BY = CY - 950.f;
		const float BGZ = SampleGroundZ(BX, BY, ArenaZ);
		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_Ballista_%d"), Idx),
			AetheriaWorldConst::MeshGothicBallista,
			FVector(BX, BY, AetheriaWorldHelpers::ComputeGroundedZ(BallistaMesh, BGZ, BallistaScale, 6.f)),
			FRotator(0.f, BallistaYaw[Idx], 0.f),
			FVector(BallistaScale));
	}

	// Flagstone Boss Arena Courtyard floor slabs (3x3 flat dark-slate tiles, zero height overshoot)
	UMaterialInstanceDynamic* ArenaFlagstoneMID = CreateColorMID(FLinearColor(0.14f, 0.13f, 0.13f, 1.0f), false);
	for (int32 GX = -1; GX <= 1; ++GX)
	{
		for (int32 GY = -1; GY <= 1; ++GY)
		{
			AddWorldMesh(
				FString::Printf(TEXT("Highkeep_Floor_%d_%d"), GX + 1, GY + 1),
				AetheriaWorldConst::MeshDungeonSlab,
				FVector(CX + GX * 900.f, CY + GY * 900.f, ArenaZ - 4.f),
				FRotator::ZeroRotator,
				FVector(8.8f, 8.8f, 0.12f),
				ArenaFlagstoneMID);
		}
	}

	// Perimeter Gothic SM_Arc & SM_Section1 walls enclosing the open-air courtyard
	const int32 PerimeterSegments = 10;
	const float PerimeterRadius = 1750.f;
	for (int32 Idx = 0; Idx < PerimeterSegments; ++Idx)
	{
		// Leave north entrance open for SM_Gate (Angle = +90 deg)
		const float AngleDeg = Idx * (360.f / static_cast<float>(PerimeterSegments));
		if (FMath::IsNearlyEqual(AngleDeg, 90.f, 20.f))
		{
			continue;
		}
		const float Rad = FMath::DegreesToRadians(AngleDeg);
		const float WX = CX + FMath::Cos(Rad) * PerimeterRadius;
		const float WY = CY + FMath::Sin(Rad) * PerimeterRadius;
		const float WZ = SampleGroundZ(WX, WY, ArenaZ);
		const bool bUseArc = (Idx % 2 == 0);
		const float SegScale = bUseArc ? ArcScale : Section1Scale;
		UStaticMesh* SegMesh = bUseArc ? ArcMesh : Section1Mesh;

		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_PerimeterWall_%d"), Idx),
			bUseArc ? AetheriaWorldConst::MeshGothicArc : AetheriaWorldConst::MeshGothicSection1,
			FVector(WX, WY, AetheriaWorldHelpers::ComputeGroundedZ(SegMesh, WZ, SegScale, 14.f)),
			FRotator(0.f, AngleDeg + 90.f, 0.f),
			FVector(SegScale));
	}

	// 6 Flaming Citadel War Braziers (SM_StoneFirePit + 1,200 lm warm firelight)
	for (int32 Idx = 0; Idx < 6; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 6.f) * UE_TWO_PI + 0.52f;
		const float BX = CX + FMath::Cos(Angle) * 1380.f;
		const float BY = CY + FMath::Sin(Angle) * 1380.f;
		const float BZ = SampleGroundZ(BX, BY, ArenaZ);

		AddBrazierAssembly(
			FString::Printf(TEXT("Highkeep_Brazier_%d"), Idx),
			FVector(BX, BY, BZ),
			1200.f,
			1550.f,
			AetheriaWorldConst::MeshStoneFirePit,
			1.35f,
			Idx * 1.05f);
	}

	// 6 Chaos-Destructible Boss Arena Columns (SM_Column3) shattered during Malakor's attacks
	for (int32 Idx = 0; Idx < 6; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 6.f) * UE_TWO_PI;
		const float PX = CX + FMath::Cos(Angle) * 1020.f;
		const float PY = CY + FMath::Sin(Angle) * 1020.f;
		const float PZ = SampleGroundZ(PX, PY, ArenaZ);

		SpawnDestructibleProp(
			FVector(PX, PY, AetheriaWorldHelpers::ComputeGroundedZ(Column3Mesh, PZ, Column3Scale, 8.f)),
			FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f),
			EDestructibleKind::Stone,
			180.f,
			AetheriaWorldConst::MeshGothicColumn3,
			FVector(Column3Scale),
			NAME_None);
	}
}

void AAetheriaWorldBuilder::BuildAshCalderaAndEmberCrypt()
{
	// Open-vaulted gothic lava dungeon ("Ember Crypt") at (9800, -2400)
	const float CX = 9800.f;
	const float CY = -2400.f;
	const float BaseZ = SampleGroundZ(CX, CY, FallbackGroundZ);

	UStaticMesh* Section1Mesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicSection1, AetheriaWorldConst::MeshCathedralArched);
	UStaticMesh* ArcMesh      = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshGothicArchSpan);
	UStaticMesh* Column3Mesh  = LoadMeshSafe(AetheriaWorldConst::MeshGothicColumn3, AetheriaWorldConst::MeshGothicPillar);
	UStaticMesh* Pillar2Mesh  = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar2, AetheriaWorldConst::MeshGothicPillar);
	UStaticMesh* BowlMesh     = LoadMeshSafe(AetheriaWorldConst::MeshStoneFirePit, AetheriaWorldConst::MeshCylinder);
	UStaticMesh* FlameMesh    = LoadMeshSafe(AetheriaWorldConst::MeshBrazierFlame, AetheriaWorldConst::MeshCone);

	const float WallSecScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Section1Mesh, 620.f, 0.2f, 120.f);
	const float WallArcScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ArcMesh, 620.f, 0.2f, 120.f);
	const float Column3Scale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Column3Mesh, 640.f, 0.2f, 120.f);
	const float SconceP2Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Pillar2Mesh, 220.f, 0.1f, 100.f);
	const float BowlScale     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BowlMesh, 28.f, 0.1f, 50.f);
	const float FlameScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(FlameMesh, 46.f, 0.1f, 50.f);

	// Controlled glowing molten-lava-cracked floor slab under raised PBR basalt walkways
	LavaFloorMID = CreateColorMID(FLinearColor(1.4f, 0.28f, 0.03f, 1.0f), true);
	AddWorldMesh(
		TEXT("EmberCrypt_MoltenLavaFloor"),
		AetheriaWorldConst::MeshPlane,
		FVector(CX, CY, BaseZ + 6.f),
		FRotator::ZeroRotator,
		FVector(18.5f, 22.5f, 1.f),
		LavaFloorMID,
		AetheriaWorldConst::MeshPlane,
		false);

	// Raised volcanic basalt walkway slabs (keep real PBR stone material — NOT BasicShapeMaterial)
	for (int32 Row = -1; Row <= 1; ++Row)
	{
		AddWorldMesh(
			FString::Printf(TEXT("EmberCrypt_BasaltSlab_%d"), Row + 1),
			AetheriaWorldConst::MeshDungeonSlab,
			FVector(CX, CY + Row * 640.f, BaseZ + 6.f),
			FRotator::ZeroRotator,
			FVector(6.2f, 5.8f, 0.16f),
			CreateColorMID(FLinearColor(0.09f, 0.08f, 0.09f, 1.0f), false));
	}

	// Gothic basalt crypt perimeter walls from SM_Section1 / SM_Arc / SM_Column3 (real PBR Gothic materials)
	const float HalfW = 860.f;
	const float HalfL = 1040.f;

	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		for (int32 Seg = -1; Seg <= 1; ++Seg)
		{
			const bool bUseArc = (Seg == 0);
			UStaticMesh* SegMesh = bUseArc ? ArcMesh : Section1Mesh;
			const float SegScale = bUseArc ? WallArcScale : WallSecScale;
			const float WX = CX + Side * HalfW;
			const float WY = CY + Seg * 680.f;

			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_SideWall_%d_%d"), Side > 0 ? 1 : 0, Seg + 1),
				bUseArc ? AetheriaWorldConst::MeshGothicArc : AetheriaWorldConst::MeshGothicSection1,
				FVector(WX, WY, AetheriaWorldHelpers::ComputeGroundedZ(SegMesh, BaseZ, SegScale, 16.f)),
				FRotator(0.f, Side > 0 ? 180.f : 0.f, 0.f),
				FVector(SegScale));
		}

		// Corner & mid-bay SM_Column3 pilasters
		for (int32 ColIdx = -1; ColIdx <= 1; ++ColIdx)
		{
			const float ColX = CX + Side * (HalfW - 45.f);
			const float ColY = CY + ColIdx * 920.f;
			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_Pilaster_%d_%d"), Side > 0 ? 1 : 0, ColIdx + 1),
				AetheriaWorldConst::MeshGothicColumn3,
				FVector(ColX, ColY, AetheriaWorldHelpers::ComputeGroundedZ(Column3Mesh, BaseZ, Column3Scale, 12.f)),
				FRotator(0.f, ColIdx * 45.f, 0.f),
				FVector(Column3Scale));
		}
	}

	// South rear apse wall (SM_Arc bays + SM_Section1 corners)
	for (int32 Seg = -1; Seg <= 1; ++Seg)
	{
		const bool bCenterArc = (Seg == 0);
		UStaticMesh* SegMesh = bCenterArc ? ArcMesh : Section1Mesh;
		const float SegScale = bCenterArc ? WallArcScale : WallSecScale;
		AddWorldMesh(
			FString::Printf(TEXT("EmberCrypt_SouthWall_%d"), Seg + 1),
			bCenterArc ? AetheriaWorldConst::MeshGothicArc : AetheriaWorldConst::MeshGothicSection1,
			FVector(CX + Seg * 560.f, CY - HalfL, AetheriaWorldHelpers::ComputeGroundedZ(SegMesh, BaseZ, SegScale, 16.f)),
			FRotator(0.f, 90.f, 0.f),
			FVector(SegScale));
	}

	// North entrance flanking bastions + entrance arch (leaving central doorway for Q_EmberCrypt)
	AddWorldMesh(
		TEXT("EmberCrypt_NorthBastionL"),
		AetheriaWorldConst::MeshGothicSection1,
		FVector(CX - 590.f, CY + HalfL, AetheriaWorldHelpers::ComputeGroundedZ(Section1Mesh, BaseZ, WallSecScale, 16.f)),
		FRotator(0.f, -90.f, 0.f),
		FVector(WallSecScale));
	AddWorldMesh(
		TEXT("EmberCrypt_NorthBastionR"),
		AetheriaWorldConst::MeshGothicSection1,
		FVector(CX + 590.f, CY + HalfL, AetheriaWorldHelpers::ComputeGroundedZ(Section1Mesh, BaseZ, WallSecScale, 16.f)),
		FRotator(0.f, -90.f, 0.f),
		FVector(WallSecScale));
	AddWorldMesh(
		TEXT("EmberCrypt_EntranceArch"),
		AetheriaWorldConst::MeshGothicArc,
		FVector(CX, CY + HalfL, AetheriaWorldHelpers::ComputeGroundedZ(ArcMesh, BaseZ, WallArcScale, 14.f)),
		FRotator(0.f, -90.f, 0.f),
		FVector(WallArcScale));

	// Open gothic vaulted ceiling arcs (replaces flat BasicShape cube roof so sky light shafts slice across the lava!)
	const float VaultYs[4] = { CY - 680.f, CY - 220.f, CY + 240.f, CY + 700.f };
	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		AddWorldMesh(
			FString::Printf(TEXT("EmberCrypt_VaultArch_%d"), Idx),
			AetheriaWorldConst::MeshGothicArchSpan,
			FVector(CX, VaultYs[Idx], BaseZ + 440.f),
			FRotator(0.f, 0.f, 0.f),
			FVector(2.85f, 1.35f, 1.65f));
	}

	// 6 Grounded Wall Torch Sconces on SM_Pillar2 + flame bowl (750 lm, Atten 1150)
	UMaterialInstanceDynamic* CryptFlameMID = CreateColorMID(FLinearColor(2.4f, 0.72f, 0.10f, 1.0f), true);
	for (int32 Row = -1; Row <= 1; ++Row)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const int32 TorchIdx = (Row + 1) * 2 + (Side > 0 ? 1 : 0);
			const float TX = CX + Side * (HalfW - 140.f);
			const float TY = CY + Row * 620.f;
			const float PostZ  = AetheriaWorldHelpers::ComputeGroundedZ(Pillar2Mesh, BaseZ + 18.f, SconceP2Scale, 6.f);
			const float BowlZ  = AetheriaWorldHelpers::ComputeStackedZ(Pillar2Mesh, PostZ, SconceP2Scale, BowlMesh, BowlScale, 4.f);
			const float FlameZ = AetheriaWorldHelpers::ComputeStackedZ(BowlMesh, BowlZ, BowlScale, FlameMesh, FlameScale, 8.f);

			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_TorchPost_%d"), TorchIdx),
				AetheriaWorldConst::MeshGothicPillar2,
				FVector(TX, TY, PostZ),
				FRotator(0.f, Side > 0 ? 180.f : 0.f, 0.f),
				FVector(SconceP2Scale));

			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_TorchBowl_%d"), TorchIdx),
				AetheriaWorldConst::MeshStoneFirePit,
				FVector(TX, TY, BowlZ),
				FRotator::ZeroRotator,
				FVector(BowlScale));

			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_TorchFlame_%d"), TorchIdx),
				AetheriaWorldConst::MeshBrazierFlame,
				FVector(TX, TY, FlameZ),
				FRotator::ZeroRotator,
				FVector(FlameScale),
				CryptFlameMID,
				AetheriaWorldConst::MeshCone,
				false);

			AddAmbientFireLight(
				FString::Printf(TEXT("EmberCrypt_WallTorchLight_%d"), TorchIdx),
				FVector(TX - Side * 35.f, TY, BaseZ + 275.f),
				750.f,
				2100.f,
				1150.f,
				FLinearColor(1.0f, 0.48f, 0.12f, 1.0f),
				0.20f,
				TorchIdx * 1.1f,
				false);
		}
	}

	// Controlled molten lava bounce illumination (1,850 lm, Atten 1,750)
	AddAmbientFireLight(
		TEXT("EmberCrypt_LavaBounce"),
		FVector(CX, CY, BaseZ + 110.f),
		1850.f,
		1900.f,
		1750.f,
		FLinearColor(1.0f, 0.28f, 0.05f, 1.0f),
		0.12f,
		0.5f,
		false);

	// 1 Destructible Obsidian Seal Wall blocking the inner sanctum doorway (LinkedQuestId = "Q_EmberCrypt")
	SpawnDestructibleProp(
		FVector(CX, CY + HalfL - 40.f, AetheriaWorldHelpers::ComputeGroundedZ(Section1Mesh, BaseZ, WallSecScale, 12.f)),
		FRotator(0.f, -90.f, 0.f),
		EDestructibleKind::Obsidian,
		160.f,
		AetheriaWorldConst::MeshGothicSection1,
		FVector(WallSecScale * 0.85f),
		FName(TEXT("Q_EmberCrypt")));

	// Glowing Sunstone Altar in the deepest apse + flanking Sarcophagi
	const FVector AltarPos(CX, CY - 740.f, BaseZ + 22.f);
	AddWorldMesh(
		TEXT("EmberCrypt_SunstoneAltar"),
		AetheriaWorldConst::MeshRunicAltar,
		AltarPos,
		FRotator::ZeroRotator,
		FVector(1.65f));

	UMaterialInstanceDynamic* SunstoneCoreMID = CreateColorMID(FLinearColor(2.8f, 1.35f, 0.28f, 1.0f), true);
	AddWorldMesh(
		TEXT("EmberCrypt_SunstoneCore"),
		AetheriaWorldConst::MeshSphere,
		AltarPos + FVector(0.f, 0.f, 185.f),
		FRotator::ZeroRotator,
		FVector(0.95f),
		SunstoneCoreMID,
		AetheriaWorldConst::MeshSphere,
		false);

	AddAmbientFireLight(
		TEXT("EmberCrypt_SunstoneGlow"),
		AltarPos + FVector(0.f, 0.f, 220.f),
		1400.f,
		3200.f,
		1250.f,
		FLinearColor(1.0f, 0.78f, 0.34f, 1.0f),
		0.08f,
		2.7f,
		false);

	AddWorldMesh(
		TEXT("EmberCrypt_SarcophagusL"),
		AetheriaWorldConst::MeshCryptSarcophagus,
		FVector(CX - 430.f, CY - 580.f, BaseZ + 22.f),
		FRotator(0.f, 20.f, 0.f),
		FVector(1.35f));
	AddWorldMesh(
		TEXT("EmberCrypt_SarcophagusR"),
		AetheriaWorldConst::MeshCryptSarcophagus,
		FVector(CX + 430.f, CY - 580.f, BaseZ + 22.f),
		FRotator(0.f, -20.f, 0.f),
		FVector(1.35f));
}

void AAetheriaWorldBuilder::BuildCrystalGlacierPeaks()
{
	// Frost crystalline spires & warming sanctuary braziers (SM_StoneFirePit) at (-6800, -6400)
	const float CX = -6800.f;
	const float CY = -6400.f;
	const float CenterZ = SampleGroundZ(CX, CY, FallbackGroundZ);

	UMaterialInstanceDynamic* IceCrystalMID = CreateColorMID(FLinearColor(0.32f, 0.92f, 1.65f, 1.0f), true);
	UMaterialInstanceDynamic* FrostedStoneMID = CreateColorMID(FLinearColor(0.58f, 0.74f, 0.86f, 1.0f), false);

	// Central Quas Frost Sanctuary Shrine
	AddWorldMesh(
		TEXT("Glacier_SanctuaryShrine"),
		AetheriaWorldConst::MeshSanctuaryShrine,
		FVector(CX, CY, CenterZ - 10.f),
		FRotator(0.f, 35.f, 0.f),
		FVector(1.65f),
		FrostedStoneMID);

	// 12 Tall Faceted Frost Crystalline Spires encircling the glacier basin
	FRandomStream GlacierRng(99107);
	for (int32 Idx = 0; Idx < 12; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 12.f) * UE_TWO_PI + GlacierRng.FRandRange(-0.15f, 0.15f);
		const float Dist = GlacierRng.FRandRange(620.f, 1650.f);
		const float SX = CX + FMath::Cos(Angle) * Dist;
		const float SY = CY + FMath::Sin(Angle) * Dist;
		const float SZ = SampleGroundZ(SX, SY, CenterZ) - 24.f;
		const float SpireHeight = GlacierRng.FRandRange(3.2f, 7.2f);
		const float SpireRadius = GlacierRng.FRandRange(1.1f, 2.2f);

		AddWorldMesh(
			FString::Printf(TEXT("Glacier_FrostSpire_%d"), Idx),
			AetheriaWorldConst::MeshCone,
			FVector(SX, SY, SZ + SpireHeight * 42.f),
			FRotator(GlacierRng.FRandRange(-12.f, 12.f), GlacierRng.FRandRange(0.f, 360.f), GlacierRng.FRandRange(-12.f, 12.f)),
			FVector(SpireRadius, SpireRadius, SpireHeight),
			IceCrystalMID,
			AetheriaWorldConst::MeshCone,
			true);

		if (Idx % 3 == 0)
		{
			AddWorldMesh(
				FString::Printf(TEXT("Glacier_IceBoulder_%d"), Idx),
				AetheriaWorldConst::MeshCliffBoulder,
				FVector(SX + 140.f, SY - 110.f, SZ),
				FRotator(0.f, GlacierRng.FRandRange(0.f, 360.f), 0.f),
				FVector(GlacierRng.FRandRange(1.4f, 2.4f)),
				FrostedStoneMID);
		}
	}

	// 3 Warming Sanctuary Braziers (SM_StoneFirePit + 1,100 lm warm orange contrast against glacier ice)
	for (int32 Idx = 0; Idx < 3; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 3.f) * UE_TWO_PI + 0.45f;
		const float FX = CX + FMath::Cos(Angle) * 480.f;
		const float FY = CY + FMath::Sin(Angle) * 480.f;
		const float FZ = SampleGroundZ(FX, FY, CenterZ);

		AddBrazierAssembly(
			FString::Printf(TEXT("Glacier_FirePit_%d"), Idx),
			FVector(FX, FY, FZ),
			1100.f,
			1450.f,
			AetheriaWorldConst::MeshStoneFirePit,
			1.25f,
			Idx * 1.4f);
	}
}

void AAetheriaWorldBuilder::BuildSkyRuneRings()
{
	// 3 Sky Rune Rings suspended high above Mirror Lake & Highkeep Citadel for Dragon aerial traversal
	struct FSkyRingSpec
	{
		FVector2D Pos;
		float AltitudeAboveGround;
		float YawDeg;
		FLinearColor Color;
	};

	const FSkyRingSpec Rings[3] = {
		{ FVector2D(-1800.f,  -400.f), 3200.f,  -28.f, FLinearColor(0.25f, 1.65f, 2.40f, 1.0f) },
		{ FVector2D(    0.f, -2600.f), 3600.f,    0.f, FLinearColor(2.20f, 1.25f, 0.25f, 1.0f) },
		{ FVector2D(    0.f, -6600.f), 4000.f,    0.f, FLinearColor(1.25f, 0.45f, 2.40f, 1.0f) }
	};

	const float RingRadius = 480.f;
	const int32 NumSegments = 12;

	for (int32 RingIdx = 0; RingIdx < 3; ++RingIdx)
	{
		const FSkyRingSpec& Spec = Rings[RingIdx];
		const float GZ = SampleGroundZ(Spec.Pos.X, Spec.Pos.Y, FallbackGroundZ);
		const FVector RingCenter(Spec.Pos.X, Spec.Pos.Y, GZ + Spec.AltitudeAboveGround);

		const FName PivotName = MakeUniqueObjectName(this, USceneComponent::StaticClass(), FName(*FString::Printf(TEXT("SkyRuneRingPivot_%d"), RingIdx)));
		USceneComponent* Pivot = NewObject<USceneComponent>(this, PivotName);
		if (!Pivot)
		{
			continue;
		}
		Pivot->SetMobility(EComponentMobility::Movable);
		Pivot->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
		Pivot->SetWorldLocationAndRotation(RingCenter, FRotator(0.f, Spec.YawDeg, 0.f));
		Pivot->RegisterComponent();
		SkyRuneRingPivots.Add(Pivot);

		UMaterialInstanceDynamic* RingMID = CreateColorMID(Spec.Color, true);

		for (int32 Seg = 0; Seg < NumSegments; ++Seg)
		{
			const float Theta = (static_cast<float>(Seg) / static_cast<float>(NumSegments)) * UE_TWO_PI;
			const FVector LocalOffset(0.f, FMath::Cos(Theta) * RingRadius, FMath::Sin(Theta) * RingRadius);
			const FVector WorldPos = Pivot->GetComponentTransform().TransformPosition(LocalOffset);
			const FRotator SegRot = Pivot->GetComponentTransform().TransformRotation(
				FRotator(0.f, 0.f, FMath::RadiansToDegrees(Theta) + 90.f).Quaternion()).Rotator();

			AddWorldMesh(
				FString::Printf(TEXT("SkyRuneRing_%d_Seg_%d"), RingIdx, Seg),
				AetheriaWorldConst::MeshCube,
				WorldPos,
				SegRot,
				FVector(0.45f, 1.95f, 0.52f),
				RingMID,
				AetheriaWorldConst::MeshCube,
				false,
				Pivot);
		}

		// Aerial guidance beacon light (2,200 lm)
		AddAmbientFireLight(
			FString::Printf(TEXT("SkyRuneRing_Beacon_%d"), RingIdx),
			RingCenter,
			2200.f,
			6500.f,
			2600.f,
			FLinearColor(
				FMath::Clamp(Spec.Color.R, 0.15f, 1.0f),
				FMath::Clamp(Spec.Color.G, 0.15f, 1.0f),
				FMath::Clamp(Spec.Color.B, 0.15f, 1.0f),
				1.0f),
			0.10f,
			RingIdx * 2.1f,
			false);
	}
}
