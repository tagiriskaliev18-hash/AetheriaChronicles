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
#include "Components/MeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
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
#include "GameFramework/Pawn.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/UnrealType.h"

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

	// Verified AAA Landscape Auto-Materials
	static constexpr TCHAR MatLandscapeMountain[]   = TEXT("/Game/MWLandscapeAutoMaterial/Materials/Landscape/MTL_MWAM_Landscape_MountainRangeExample.MTL_MWAM_Landscape_MountainRangeExample");
	static constexpr TCHAR MatLandscapeIsland[]     = TEXT("/Game/MWLandscapeAutoMaterial/Materials/Landscape/MTL_MWAM_Landscape_IslandExample.MTL_MWAM_Landscape_IslandExample");

	// Verified AAA Stone Floor, Masonry, Obsidian, Iron & Element Materials
	static constexpr TCHAR MatStoneAltar[]          = TEXT("/Game/MedievalDungeon/Materials/M_Stone_Altar_Inst.M_Stone_Altar_Inst");
	static constexpr TCHAR MatBrickFloorClean[]     = TEXT("/Game/MedievalDungeon/Materials/M_BrickFloor_Clean_Inst.M_BrickFloor_Clean_Inst");
	static constexpr TCHAR MatStoneWall[]           = TEXT("/Game/MedievalDungeon/Materials/M_StoneWall_Inst.M_StoneWall_Inst");
	static constexpr TCHAR MatCryptWallA[]          = TEXT("/Game/MedievalDungeon/Materials/M_Crypt_Wall_Decorative_A_Inst.M_Crypt_Wall_Decorative_A_Inst");
	static constexpr TCHAR MatDecorativeArches[]    = TEXT("/Game/MedievalDungeon/Materials/M_Decorative_Arches_Inst.M_Decorative_Arches_Inst");
	static constexpr TCHAR MatPillarInst[]          = TEXT("/Game/MedievalDungeon/Materials/M_Pillar_Inst.M_Pillar_Inst");
	static constexpr TCHAR MatFirePitInst[]         = TEXT("/Game/MedievalDungeon/Materials/M_Fire_Pit_Inst.M_Fire_Pit_Inst");
	static constexpr TCHAR MatEdgeStonesInst[]      = TEXT("/Game/MedievalDungeon/Materials/M_Edge_Stones_Inst.M_Edge_Stones_Inst");
	static constexpr TCHAR MatStoneCoffin[]         = TEXT("/Game/MedievalDungeon/Materials/M_Stone_Coffin_Inst.M_Stone_Coffin_Inst");
	static constexpr TCHAR MatDirtMounds[]          = TEXT("/Game/MedievalDungeon/Materials/M_Dirt_Mounds_Inst.M_Dirt_Mounds_Inst");
	static constexpr TCHAR MatMedievalWood[]        = TEXT("/Game/MedievalDungeon/Materials/M_Wood_Inst.M_Wood_Inst");
	static constexpr TCHAR MatNamaqualandCliff[]    = TEXT("/Game/Namaqualand/Materials/MI_Cliff_Side.MI_Cliff_Side");
	static constexpr TCHAR MatAetheriaMasterPBR[]   = TEXT("/Game/Materials/M_AetheriaMasterPBR.M_AetheriaMasterPBR");
	static constexpr TCHAR MatAetheriaGothicStone[] = TEXT("/Game/Materials/M_AetheriaGothicStone.M_AetheriaGothicStone");
	static constexpr TCHAR MatAetheriaObsidian[]    = TEXT("/Game/Materials/M_AetheriaObsidian.M_AetheriaObsidian");
	static constexpr TCHAR MatAetheriaDarkIron[]    = TEXT("/Game/Materials/M_AetheriaDarkIron.M_AetheriaDarkIron");
	static constexpr TCHAR MatAetheriaFireball[]    = TEXT("/Game/Materials/M_AetheriaFireball.M_AetheriaFireball");
	static constexpr TCHAR MatAetheriaPrismWall[]   = TEXT("/Game/Materials/M_AetheriaPrismWall.M_AetheriaPrismWall");

	// Verified MedievalDungeon Architecture & Props Meshes
	static constexpr TCHAR MeshDungeonFloor[]       = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Floor.SM_Floor");
	static constexpr TCHAR MeshCryptFloor[]         = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Crypt_Floor.SM_Crypt_Floor");
	static constexpr TCHAR MeshEdgeStones[]         = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Edge_Stones.SM_Edge_Stones");
	static constexpr TCHAR MeshDungeonWallA[]       = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Dungeon_Wall_Decorative_A.SM_Dungeon_Wall_Decorative_A");
	static constexpr TCHAR MeshDungeonWallHalf[]    = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Dungeon_Wall_Half.SM_Dungeon_Wall_Half");
	static constexpr TCHAR MeshDecorativeArches[]   = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Decorative_Arches.SM_Decorative_Arches");
	static constexpr TCHAR MeshDungeonPillar[]      = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Pillar.SM_Pillar");
	static constexpr TCHAR MeshHalfArch[]           = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Half_Arch.SM_Half_Arch");
	static constexpr TCHAR MeshFirePit[]            = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Fire_Pit.SM_Fire_Pit");
	static constexpr TCHAR MeshDoorWay[]            = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_DoorWay.SM_DoorWay");
	static constexpr TCHAR MeshCryptWallA[]         = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Crypt_Wall_Decorative_A.SM_Crypt_Wall_Decorative_A");
	static constexpr TCHAR MeshCryptPillar[]        = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Crypt_Pillar.SM_Crypt_Pillar");
	static constexpr TCHAR MeshCryptCeilingArch[]   = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Crypt_Ceiling_Arch_A.SM_Crypt_Ceiling_Arch_A");
	static constexpr TCHAR MeshMausoleum[]          = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Mausoleum.SM_Mausoleum");
	static constexpr TCHAR MeshMedievalAltar[]      = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Alter.SM_Alter");
	static constexpr TCHAR MeshStoneCoffin[]        = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Stone_Coffin.SM_Stone_Coffin");
	static constexpr TCHAR MeshMedievalPot[]        = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Pot_A_Complete.SM_Pot_A_Complete");
	static constexpr TCHAR MeshGothicBuilding09[]   = TEXT("/Game/Gothic_Environment/Meshes/Gothic_buildings/SM_Gothic_building_09.SM_Gothic_building_09");

	// Verified KiteDemo & MWLandscapeAutoMaterial Nature Meshes
	static constexpr TCHAR MeshKiteCliff01[]        = TEXT("/Game/KiteDemo/Environments/Cliffs/Cliff01/SM_Cliff01.SM_Cliff01");
	static constexpr TCHAR MeshKiteMountainRock[]   = TEXT("/Game/KiteDemo/Environments/Rocks/Mountain_RockFace_002/SM_MountainRock_Closed.SM_MountainRock_Closed");
	static constexpr TCHAR MeshKiteRiverRock[]      = TEXT("/Game/KiteDemo/Environments/Rocks/River_Rock_01/SM_River_Rock_01.SM_River_Rock_01");
	static constexpr TCHAR MeshKiteFern01[]         = TEXT("/Game/KiteDemo/Environments/Foliage/Ferns/SM_Fern_01.SM_Fern_01");
	static constexpr TCHAR MeshKiteFieldGrass01[]   = TEXT("/Game/KiteDemo/Environments/Foliage/Grass/FieldGrass/SM_FieldGrass_01.SM_FieldGrass_01");
	static constexpr TCHAR MeshMWAMStoneA[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneA.SM_MWAM_StoneA");
	static constexpr TCHAR MeshMWAMStoneB[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneB.SM_MWAM_StoneB");
	static constexpr TCHAR MeshMWAMStoneC[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneC.SM_MWAM_StoneC");
	static constexpr TCHAR MeshMWAMStoneD[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneD.SM_MWAM_StoneD");

	// Verified Cascade Fire VFX
	static constexpr TCHAR ParticleTorchFire[]      = TEXT("/Game/MedievalDungeon/Particles/P_Torch_Fire.P_Torch_Fire");
	static constexpr TCHAR ParticlePitFire[]        = TEXT("/Game/MedievalDungeon/Particles/P_Pit_Fire.P_Pit_Fire");
	static constexpr TCHAR ParticleAncientFire[]    = TEXT("/Game/AncientTreasures/VFX/P_Fire_01a.P_Fire_01a");

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

	// Verified /Game/Meshes/ & MedievalDungeon PBR replacements (zero missing assets!)
	static constexpr TCHAR MeshCathedralLow[]       = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Dungeon_Wall_Half.SM_Dungeon_Wall_Half");
	static constexpr TCHAR MeshCathedralArched[]    = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Decorative_Arches.SM_Decorative_Arches");
	static constexpr TCHAR MeshFortWall[]           = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Dungeon_Wall_Decorative_A.SM_Dungeon_Wall_Decorative_A");
	static constexpr TCHAR MeshFortPlaster[]        = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Crypt_Wall_Decorative_A.SM_Crypt_Wall_Decorative_A");
	static constexpr TCHAR MeshSanctuaryShrine[]    = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Alter.SM_Alter");
	static constexpr TCHAR MeshRunicAltar[]         = TEXT("/Game/Meshes/SM_RunicAltar.SM_RunicAltar");
	static constexpr TCHAR MeshStoneFirePit[]       = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Fire_Pit.SM_Fire_Pit");
	static constexpr TCHAR MeshDeadTree[]           = TEXT("/Game/Meshes/SM_DeadTree.SM_DeadTree");
	static constexpr TCHAR MeshCitadelFortress[]    = TEXT("/Game/Meshes/SM_CitadelFortress.SM_CitadelFortress");
	static constexpr TCHAR MeshGothicArchSpan[]     = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Decorative_Arches.SM_Decorative_Arches");
	static constexpr TCHAR MeshGothicPillar[]       = TEXT("/Game/Meshes/SM_GothicPillar.SM_GothicPillar");
	static constexpr TCHAR MeshRuinedArch[]         = TEXT("/Game/Meshes/SM_RuinedArch.SM_RuinedArch");
	static constexpr TCHAR MeshDungeonDais[]        = TEXT("/Game/TA_Sample_Statue/Models/SM_TA_Sample_Pedestal_01.SM_TA_Sample_Pedestal_01");
	static constexpr TCHAR MeshDungeonSlab[]        = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Floor.SM_Floor");
	static constexpr TCHAR MeshGothicBarrel[]       = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Pot_A_Complete.SM_Pot_A_Complete");
	static constexpr TCHAR MeshTreasureChest[]      = TEXT("/Game/Meshes/SM_TreasureChest.SM_TreasureChest");
	static constexpr TCHAR MeshCryptSarcophagus[]   = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Stone_Coffin.SM_Stone_Coffin");
	static constexpr TCHAR MeshCliffBoulder[]       = TEXT("/Game/Meshes/SM_CliffBoulder.SM_CliffBoulder");
	static constexpr TCHAR MeshCoreCrystal[]        = TEXT("/Game/Meshes/SM_CoreCrystal.SM_CoreCrystal");
	static constexpr TCHAR MeshEarthSpikeSingle[]   = TEXT("/Game/Meshes/SM_Earth_Spike_Single.SM_Earth_Spike_Single");
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

	static FVector ComputeGroundedCenterLoc(
		const UStaticMesh* Mesh,
		float TargetX,
		float TargetY,
		float TerrainZ,
		const FRotator& Rot,
		const FVector& Scale,
		float SinkCm = 6.f)
	{
		const FBox Box = GetSafeBoundingBox(Mesh);
		const FVector RawCenter = Box.GetCenter();
		const FVector LocalAnchor(RawCenter.X * Scale.X, RawCenter.Y * Scale.Y, Box.Min.Z * Scale.Z);
		const FVector RotatedAnchor = Rot.RotateVector(LocalAnchor);
		return FVector(TargetX - RotatedAnchor.X, TargetY - RotatedAnchor.Y, TerrainZ - SinkCm - RotatedAnchor.Z);
	}

	static float ComputeStackedZ(const UStaticMesh* BaseMesh, float BaseWorldZ, float BaseScaleZ, const UStaticMesh* TopMesh, float TopScaleZ, float SinkCm = 2.f)
	{
		const FBox BaseBox = GetSafeBoundingBox(BaseMesh);
		const FBox TopBox = GetSafeBoundingBox(TopMesh);
		const float BaseTopZ = BaseWorldZ + BaseBox.Max.Z * BaseScaleZ;
		return BaseTopZ - SinkCm - TopBox.Min.Z * TopScaleZ;
	}

	static bool IsGreyboxMaterialPtr(const UMaterialInterface* Mat)
	{
		if (!Mat)
		{
			return true;
		}
		const FString Name = Mat->GetName();
		if (Name.Contains(TEXT("WorldGrid"), ESearchCase::IgnoreCase) ||
			Name.Contains(TEXT("BasicShapeMaterial"), ESearchCase::IgnoreCase) ||
			Name.Contains(TEXT("DefaultMaterial"), ESearchCase::IgnoreCase) ||
			Name.Contains(TEXT("DefaultDiffuse"), ESearchCase::IgnoreCase) ||
			Name.Contains(TEXT("ProcGrid"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		const FString Path = Mat->GetPathName();
		return Path.StartsWith(TEXT("/Engine/EngineMaterials/WorldGrid")) ||
			Path.StartsWith(TEXT("/Engine/BasicShapes/"));
	}

	static UMaterialInterface* LoadFirstExistingMaterial(const TArray<const TCHAR*>& Candidates)
	{
		for (const TCHAR* Candidate : Candidates)
		{
			if (!Candidate || *Candidate == 0)
			{
				continue;
			}
			const FString Key(Candidate);
			const bool bIsEngine = Key.StartsWith(TEXT("/Engine/"));
			const FString Pkg = bIsEngine ? FString() : FPackageName::ObjectPathToPackageName(Key);
			if (bIsEngine || (!Pkg.IsEmpty() && FPackageName::DoesPackageExist(Pkg)))
			{
				if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, Candidate))
				{
					return Mat;
				}
			}
		}
		return nullptr;
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
		// Prefer real sculpted gothic pillar instead of untextured BasicShapes primitives
		const FString PillarPkg = FPackageName::ObjectPathToPackageName(FString(AetheriaWorldConst::MeshDungeonPillar));
		if (!PillarPkg.IsEmpty() && FPackageName::DoesPackageExist(PillarPkg))
		{
			LoadedMesh = LoadObject<UStaticMesh>(nullptr, AetheriaWorldConst::MeshDungeonPillar);
		}
		if (!LoadedMesh)
		{
			const TCHAR* FallbackPath =
				(InKind == EDestructibleKind::Crystal || InKind == EDestructibleKind::Wood)
					? AetheriaWorldConst::MeshCylinder
					: AetheriaWorldConst::MeshCube;
			LoadedMesh = LoadObject<UStaticMesh>(nullptr, FallbackPath);
			bUsedPrimitiveFallback = true;
		}
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

		// Ensure destructible monoliths ALWAYS carry real PBR gothic stone/obsidian masonry (zero greybox!)
		UMaterialInterface* StoneMat = nullptr;
		if (InKind == EDestructibleKind::Obsidian)
		{
			StoneMat = AetheriaWorldHelpers::LoadFirstExistingMaterial({
				AetheriaWorldConst::MatCryptWallA,
				AetheriaWorldConst::MatStoneWall,
				AetheriaWorldConst::MatAetheriaGothicStone
			});
		}
		else
		{
			StoneMat = AetheriaWorldHelpers::LoadFirstExistingMaterial({
				AetheriaWorldConst::MatPillarInst,
				AetheriaWorldConst::MatAetheriaGothicStone,
				AetheriaWorldConst::MatStoneWall
			});
		}

		const FString MeshName = LoadedMesh ? LoadedMesh->GetName() : FString();
		const bool bForceStoneMat = bUsedPrimitiveFallback
			|| InKind == EDestructibleKind::Obsidian
			|| MeshName.Contains(TEXT("GothicPillar"), ESearchCase::IgnoreCase);

		if (StoneMat)
		{
			const int32 NumMats = FMath::Max(1, IntactMesh->GetNumMaterials());
			for (int32 Idx = 0; Idx < NumMats; ++Idx)
			{
				if (bForceStoneMat || AetheriaWorldHelpers::IsGreyboxMaterialPtr(IntactMesh->GetMaterial(Idx)))
				{
					IntactMesh->SetMaterial(Idx, StoneMat);
				}
			}
		}

		if (bHasCustomAccent || InKind == EDestructibleKind::Crystal)
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

	if (!IntactMesh)
	{
		return;
	}

	// NEVER overwrite the entire gothic pillar with solid neon EmissiveMeshMaterial!
	// Keep dark weathered gothic stone masonry on the body so destructible quest monoliths
	// look like ancient carved stone pillars instead of blinding yellow studio blocks.
	UMaterialInterface* PillarStoneMat = AetheriaWorldHelpers::LoadFirstExistingMaterial(
		(MaterialKind == EDestructibleKind::Obsidian)
			? TArray<const TCHAR*>{ AetheriaWorldConst::MatCryptWallA, AetheriaWorldConst::MatPillarInst, AetheriaWorldConst::MatStoneWall }
			: TArray<const TCHAR*>{ AetheriaWorldConst::MatPillarInst, AetheriaWorldConst::MatAetheriaGothicStone, AetheriaWorldConst::MatStoneWall });

	if (PillarStoneMat)
	{
		const int32 NumMats = FMath::Max(1, IntactMesh->GetNumMaterials());
		for (int32 Idx = 0; Idx < NumMats; ++Idx)
		{
			UMaterialInterface* CurMat = IntactMesh->GetMaterial(Idx);
			const bool bIsOldEmissive = CurMat && CurMat->GetName().Contains(TEXT("Emissive"), ESearchCase::IgnoreCase);
			if (bIsOldEmissive || AetheriaWorldHelpers::IsGreyboxMaterialPtr(CurMat) ||
				(IntactMesh->GetStaticMesh() && IntactMesh->GetStaticMesh()->GetName().Contains(TEXT("GothicPillar"), ESearchCase::IgnoreCase)))
			{
				IntactMesh->SetMaterial(Idx, PillarStoneMat);
			}
		}
	}

	if (!bUseEmissive)
	{
		return;
	}

	// Seat a subtle, grounded amber-gold rune crystal directly on top of the pillar's bounds (never floating off to the side or blinding white)
	const FBox LocalBox = AetheriaWorldHelpers::GetSafeBoundingBox(IntactMesh->GetStaticMesh());
	const FVector LocalCenter = LocalBox.GetCenter();

	UStaticMesh* CrystalMesh = nullptr;
	const FString CrystalPkg = FPackageName::ObjectPathToPackageName(FString(AetheriaWorldConst::MeshCoreCrystal));
	if (!CrystalPkg.IsEmpty() && FPackageName::DoesPackageExist(CrystalPkg))
	{
		CrystalMesh = LoadObject<UStaticMesh>(nullptr, AetheriaWorldConst::MeshCoreCrystal);
	}
	if (!CrystalMesh)
	{
		CrystalMesh = LoadObject<UStaticMesh>(nullptr, AetheriaWorldConst::MeshSphere);
	}

	if (!RuneAccentMesh && CrystalMesh)
	{
		const FName AccentName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("RuneAccentCrown"));
		RuneAccentMesh = NewObject<UStaticMeshComponent>(this, AccentName);
		if (RuneAccentMesh)
		{
			RuneAccentMesh->SetStaticMesh(CrystalMesh);
			RuneAccentMesh->SetMobility(EComponentMobility::Movable);
			RuneAccentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			RuneAccentMesh->SetCastShadow(false);
			RuneAccentMesh->bDisallowNanite = true;
			RuneAccentMesh->AttachToComponent(IntactMesh, FAttachmentTransformRules::KeepRelativeTransform);
			RuneAccentMesh->RegisterComponent();
		}
	}

	FVector LocalCrownPos(LocalCenter.X, LocalCenter.Y, LocalBox.Max.Z);

	if (RuneAccentMesh && CrystalMesh)
	{
		const FVector ParentScale = IntactMesh->GetRelativeScale3D().ComponentMax(FVector(0.15f));
		const FBox CrystalBox = AetheriaWorldHelpers::GetSafeBoundingBox(CrystalMesh);
		const FVector CrystalCenter = CrystalBox.GetCenter();
		const FVector RelScale(
			0.22f / ParentScale.X,
			0.22f / ParentScale.Y,
			0.22f / ParentScale.Z);

		// Seat crystal base squarely into the pillar capital center (no floating gap or horizontal offset)
		LocalCrownPos = FVector(
			LocalCenter.X - CrystalCenter.X * RelScale.X,
			LocalCenter.Y - CrystalCenter.Y * RelScale.Y,
			LocalBox.Max.Z - CrystalBox.Min.Z * RelScale.Z - (4.f / ParentScale.Z));

		RuneAccentMesh->SetRelativeLocation(LocalCrownPos);
		RuneAccentMesh->SetRelativeRotation(FRotator(0.f, 45.f, 0.f));
		RuneAccentMesh->SetRelativeScale3D(RelScale);
		RuneAccentMesh->SetVisibility(true, false);

		const bool bWarmRune = (GlowColor.R >= GlowColor.B * 0.95f);
		const FLinearColor SubtleRuneColor = bWarmRune
			? FLinearColor(1.0f, 0.45f, 0.08f, 1.0f)
			: FLinearColor(
				FMath::Clamp(GlowColor.R, 0.08f, 0.95f),
				FMath::Clamp(GlowColor.G, 0.12f, 0.85f),
				FMath::Clamp(GlowColor.B, 0.25f, 1.0f),
				1.0f);

		UMaterialInterface* BaseCrystalMat = AetheriaWorldHelpers::LoadFirstExistingMaterial({
			AetheriaWorldConst::MatAetheriaMasterPBR,
			AetheriaWorldConst::MatEmissive
		});

		UMaterialInterface* RuneMat = nullptr;
		if (BaseCrystalMat)
		{
			PropMID = UMaterialInstanceDynamic::Create(BaseCrystalMat, this);
			if (PropMID)
			{
				PropMID->SetVectorParameterValue(TEXT("BaseColor"), SubtleRuneColor);
				PropMID->SetVectorParameterValue(TEXT("Color"), SubtleRuneColor);
				PropMID->SetVectorParameterValue(TEXT("AccentColor"), SubtleRuneColor);
				PropMID->SetVectorParameterValue(TEXT("EmissiveColor"), SubtleRuneColor);
				PropMID->SetScalarParameterValue(TEXT("EmissiveBoost"), 1.6f);
				PropMID->SetScalarParameterValue(TEXT("EmissiveStrength"), 1.6f);
				PropMID->SetScalarParameterValue(TEXT("Roughness"), 0.22f);
				RuneMat = PropMID;
			}
		}

		if (RuneMat)
		{
			const int32 NumMats = FMath::Max(1, RuneAccentMesh->GetNumMaterials());
			for (int32 Idx = 0; Idx < NumMats; ++Idx)
			{
				RuneAccentMesh->SetMaterial(Idx, RuneMat);
			}
		}
	}

	if (!RuneAccentLight)
	{
		const FName LightName = MakeUniqueObjectName(this, UPointLightComponent::StaticClass(), TEXT("RuneAccentLight"));
		RuneAccentLight = NewObject<UPointLightComponent>(this, LightName);
		if (RuneAccentLight)
		{
			RuneAccentLight->SetMobility(EComponentMobility::Movable);
			RuneAccentLight->SetIntensityUnits(ELightUnits::Lumens);
			RuneAccentLight->SetCastShadows(false);
			RuneAccentLight->AttachToComponent(IntactMesh, FAttachmentTransformRules::KeepRelativeTransform);
			RuneAccentLight->RegisterComponent();
		}
	}

	if (RuneAccentLight)
	{
		const FLinearColor ClampedLightColor(
			FMath::Clamp(GlowColor.R, 0.15f, 1.0f),
			FMath::Clamp(GlowColor.G, 0.15f, 0.65f),
			FMath::Clamp(GlowColor.B, 0.08f, 1.0f),
			1.0f);
		RuneAccentLight->SetRelativeLocation(FVector(LocalCenter.X, LocalCenter.Y, LocalBox.Max.Z + 14.f));
		RuneAccentLight->SetIntensity(280.f);
		RuneAccentLight->SetAttenuationRadius(420.f);
		RuneAccentLight->SetSourceRadius(16.f);
		RuneAccentLight->SetSpecularScale(0.14f);
		RuneAccentLight->SetLightColor(ClampedLightColor);
		RuneAccentLight->SetVisibility(true);
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
	if (RuneAccentMesh)
	{
		RuneAccentMesh->SetVisibility(false, false);
		RuneAccentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (RuneAccentLight)
	{
		RuneAccentLight->SetIntensity(0.f);
		RuneAccentLight->SetVisibility(false);
	}

	// Select debris chunk mesh & real PBR stone/crystal material
	const TCHAR* DebrisMeshPath = (MaterialKind == EDestructibleKind::Crystal)
		? AetheriaWorldConst::MeshCone
		: AetheriaWorldConst::MeshCube;
	UStaticMesh* ChunkMesh = LoadObject<UStaticMesh>(nullptr, DebrisMeshPath);

	UMaterialInterface* ChunkPBRMat = nullptr;
	if (MaterialKind == EDestructibleKind::Crystal)
	{
		ChunkPBRMat = AetheriaWorldHelpers::LoadFirstExistingMaterial({
			AetheriaWorldConst::MatAetheriaPrismWall,
			AetheriaWorldConst::MatPillarInst
		});
	}
	else if (MaterialKind == EDestructibleKind::Obsidian)
	{
		ChunkPBRMat = AetheriaWorldHelpers::LoadFirstExistingMaterial({
			AetheriaWorldConst::MatCryptWallA,
			AetheriaWorldConst::MatStoneWall
		});
	}
	else
	{
		ChunkPBRMat = AetheriaWorldHelpers::LoadFirstExistingMaterial({
			AetheriaWorldConst::MatPillarInst,
			AetheriaWorldConst::MatAetheriaGothicStone,
			AetheriaWorldConst::MatStoneWall
		});
	}

	if (!ChunkPBRMat)
	{
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
				ChunkPBRMat = DebrisMID;
			}
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
		if (ChunkPBRMat)
		{
			if (bAccentEmissive || MaterialKind == EDestructibleKind::Crystal || ChunkPBRMat->GetBlendMode() != BLEND_Opaque)
			{
				Chunk->bDisallowNanite = true;
			}
			Chunk->SetMaterial(0, ChunkPBRMat);
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
			SunComp->bEnableLightShaftOcclusion = true;
			SunComp->bEnableLightShaftBloom = true;
			SunComp->SetVolumetricScatteringIntensity(2.2f);
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
			SkyComp->bLowerHemisphereIsBlack = false;
			SkyComp->SetLowerHemisphereColor(FLinearColor(0.16f, 0.14f, 0.12f, 1.0f));
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
			FogComp->SetFogDensity(0.0052f);
			FogComp->SetFogHeightFalloff(0.20f);
			FogComp->SetFogMaxOpacity(0.78f);
			FogComp->SetStartDistance(350.0f);
			FogComp->SetVolumetricFog(true);
			FogComp->SetVolumetricFogScatteringDistribution(0.68f);
			FogComp->SetVolumetricFogExtinctionScale(0.75f);
			FogComp->SetVolumetricFogDistance(8500.f);
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
		PP.AutoExposureMinBrightness = 0.35f;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMaxBrightness = 2.0f;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = -0.15f;

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
			0.02f, -34.f, 132.f, 7200.f, 2.4f, 0.72f, 0.0062f,
			FLinearColor(0.68f, 0.82f, 1.00f),
			FLinearColor(0.11f, 0.17f, 0.30f),
			FLinearColor(0.30f, 0.46f, 0.74f),
			FColor(145, 170, 215)
		},
		// 0.26 — Misty Dawn (soft rose-amber highland sunrise)
		{
			0.26f, -13.f, 54.f, 3600.f, 6.8f, 1.02f, 0.0056f,
			FLinearColor(1.00f, 0.81f, 0.69f),
			FLinearColor(0.48f, 0.40f, 0.44f),
			FLinearColor(0.96f, 0.64f, 0.44f),
			FColor(218, 198, 196)
		},
		// 0.48 — Highland Noon (crisp alpine daylight)
		{
			0.48f, -58.f, -14.f, 5900.f, 11.2f, 1.24f, 0.0042f,
			FLinearColor(1.00f, 0.98f, 0.94f),
			FLinearColor(0.38f, 0.54f, 0.74f),
			FLinearColor(0.78f, 0.86f, 0.98f),
			FColor(225, 232, 240)
		},
		// 0.72 — Golden Sunset (Crimson Desert 4200K golden-hour spectacle with warm dusk golden-slate aerial perspective)
		{
			0.72f, -16.f, -42.f, 4200.f, 11.5f, 1.55f, 0.0052f,
			FLinearColor(1.00f, 0.85f, 0.66f),
			FLinearColor(0.54f, 0.43f, 0.36f),
			FLinearColor(1.00f, 0.66f, 0.34f),
			FColor(222, 200, 178)
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
// Landscape & OpenWorld Greybox Material Patching + Asset Helpers
// ============================================================================

bool AAetheriaWorldBuilder::IsGreyboxOrMissingMaterial(const UMaterialInterface* Mat)
{
	return AetheriaWorldHelpers::IsGreyboxMaterialPtr(Mat);
}

UMaterialInterface* AAetheriaWorldBuilder::LoadPBRMaterial(const TArray<const TCHAR*>& Candidates)
{
	for (const TCHAR* Candidate : Candidates)
	{
		if (!Candidate || *Candidate == 0)
		{
			continue;
		}
		const FString Key(Candidate);
		if (const TObjectPtr<UMaterialInterface>* Found = MaterialCache.Find(Key))
		{
			if (*Found)
			{
				return Found->Get();
			}
		}

		const bool bIsEngineAsset = Key.StartsWith(TEXT("/Engine/"));
		const FString PkgName = bIsEngineAsset ? FString() : FPackageName::ObjectPathToPackageName(Key);
		if (bIsEngineAsset || (!PkgName.IsEmpty() && FPackageName::DoesPackageExist(PkgName)))
		{
			if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, Candidate))
			{
				MaterialCache.Add(Key, Mat);
				return Mat;
			}
		}
	}
	return nullptr;
}

UParticleSystem* AAetheriaWorldBuilder::LoadParticleSafe(const TCHAR* AssetPath)
{
	if (!AssetPath || *AssetPath == 0)
	{
		return nullptr;
	}

	const FString Key(AssetPath);
	if (const TObjectPtr<UParticleSystem>* Found = ParticleCache.Find(Key))
	{
		if (*Found)
		{
			return Found->Get();
		}
	}

	const FString PkgName = FPackageName::ObjectPathToPackageName(Key);
	if (!PkgName.IsEmpty() && FPackageName::DoesPackageExist(PkgName))
	{
		if (UParticleSystem* PS = LoadObject<UParticleSystem>(nullptr, AssetPath))
		{
			ParticleCache.Add(Key, PS);
			return PS;
		}
	}
	return nullptr;
}

void AAetheriaWorldBuilder::PatchOpenWorldLandscapeMaterials()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMaterialInterface* FloorPatchMat = LoadPBRMaterial({
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall
	});
	if (!FloorPatchMat)
	{
		return;
	}

	int32 PatchedWorldSlots = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor == this || Actor->IsA<APawn>() || Actor->IsA<ADestructiblePropActor>())
		{
			continue;
		}

		// NEVER mutate ALandscapeProxy or ULandscapeComponent at runtime (in -game mode GIsEditor==0,
		// mutating Landscape materials triggers SetParentEditorOnly() assertion on World Partition cell stream).
		// Landscape auto-materials are baked directly onto the map asset in Editor mode.
		const FString ClassName = Actor->GetClass()->GetName();
		if (ClassName.Contains(TEXT("Landscape"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		const bool bIsLevelFloor = Actor->GetName().Contains(TEXT("Floor"), ESearchCase::IgnoreCase);
		TInlineComponentArray<UStaticMeshComponent*> StaticMeshComps;
		Actor->GetComponents<UStaticMeshComponent>(StaticMeshComps);
		for (UStaticMeshComponent* MeshComp : StaticMeshComps)
		{
			if (!IsValid(MeshComp) || !MeshComp->IsVisible() || MeshComp->bHiddenInGame)
			{
				continue;
			}

			bool bModified = false;
			const int32 NumSlots = FMath::Max(1, MeshComp->GetNumMaterials());
			for (int32 SlotIdx = 0; SlotIdx < NumSlots; ++SlotIdx)
			{
				UMaterialInterface* CurMat = MeshComp->GetMaterial(SlotIdx);
				const bool bIsEngineFloorMat = bIsLevelFloor && CurMat
					&& CurMat->GetPathName().StartsWith(TEXT("/Engine/"));
				if ((IsGreyboxOrMissingMaterial(CurMat) || bIsEngineFloorMat) && CurMat != FloorPatchMat)
				{
					MeshComp->SetMaterial(SlotIdx, FloorPatchMat);
					bModified = true;
					++PatchedWorldSlots;
				}
			}
			if (bModified)
			{
				MeshComp->MarkRenderStateDirty();
			}
		}
	}

	if (PatchedWorldSlots > 0)
	{
		UE_LOG(LogChronicles, Display,
			TEXT("[WorldBuilder] Patched OpenWorld greybox static mesh slots: %d."),
			PatchedWorldSlots);
	}
}

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
		UE_LOG(LogChronicles, Warning, TEXT("[WorldBuilder] Package missing on disk for '%s' — using AAA gothic fallback."), *Key);
	}

	// Prefer sculpted MedievalDungeon architecture instead of Cube.Cube when a /Game/ mesh is missing
	if (!Mesh && !bIsEngineAsset)
	{
		const bool bFallbackIsCube = (!FallbackPath || FCString::Stricmp(FallbackPath, AetheriaWorldConst::MeshCube) == 0);
		if (bFallbackIsCube)
		{
			const TCHAR* ArchitecturalFallbacks[2] = {
				AetheriaWorldConst::MeshDungeonWallA,
				AetheriaWorldConst::MeshDungeonPillar
			};
			for (const TCHAR* ArchFallback : ArchitecturalFallbacks)
			{
				const FString ArchPkg = FPackageName::ObjectPathToPackageName(FString(ArchFallback));
				if (!ArchPkg.IsEmpty() && FPackageName::DoesPackageExist(ArchPkg))
				{
					Mesh = LoadObject<UStaticMesh>(nullptr, ArchFallback);
					if (Mesh)
					{
						break;
					}
				}
			}
		}
	}

	if (!Mesh && FallbackPath)
	{
		const FString FbKey(FallbackPath);
		const bool bFbEngine = FbKey.StartsWith(TEXT("/Engine/"));
		const FString FbPkg = bFbEngine ? FString() : FPackageName::ObjectPathToPackageName(FbKey);
		if (bFbEngine || (!FbPkg.IsEmpty() && FPackageName::DoesPackageExist(FbPkg)))
		{
			Mesh = LoadObject<UStaticMesh>(nullptr, FallbackPath);
		}
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
		const FString MatName = OverrideMat->GetName();
		const bool bIsSpecialShader =
			(BaseMat && BaseMat->GetName().Contains(TEXT("Emissive"), ESearchCase::IgnoreCase)) ||
			MatName.Contains(TEXT("Fireball"), ESearchCase::IgnoreCase) ||
			MatName.Contains(TEXT("PrismWall"), ESearchCase::IgnoreCase);
		if (bIsSpecialShader || (BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked))
		{
			Comp->bDisallowNanite = true;
		}
		const int32 NumMats = FMath::Max(1, Comp->GetNumMaterials());
		for (int32 Idx = 0; Idx < NumMats; ++Idx)
		{
			Comp->SetMaterial(Idx, OverrideMat);
		}
	}
	else
	{
		// Zero-greybox safety net: replace any null / WorldGrid / BasicShape / DefaultMaterial slot or Cube fallback with real PBR stone
		const bool bFellBackToCube = Mesh->GetPathName().Contains(TEXT("BasicShapes/Cube"), ESearchCase::IgnoreCase);
		UMaterialInterface* FallbackStoneMat = nullptr;
		const int32 NumMats = FMath::Max(1, Comp->GetNumMaterials());
		for (int32 Idx = 0; Idx < NumMats; ++Idx)
		{
			UMaterialInterface* SlotMat = Comp->GetMaterial(Idx);
			if (bFellBackToCube || IsGreyboxOrMissingMaterial(SlotMat))
			{
				if (!FallbackStoneMat)
				{
					FallbackStoneMat = LoadPBRMaterial({
						AetheriaWorldConst::MatStoneWall,
						AetheriaWorldConst::MatAetheriaGothicStone,
						AetheriaWorldConst::MatCryptWallA,
						AetheriaWorldConst::MatStoneAltar
					});
				}
				if (FallbackStoneMat)
				{
					Comp->SetMaterial(Idx, FallbackStoneMat);
				}
			}
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

UParticleSystemComponent* AAetheriaWorldBuilder::SpawnFireParticle(
	const FString& CompName,
	const TCHAR* ParticlePath,
	const FVector& WorldLoc,
	const FVector& WorldScale,
	const FRotator& WorldRot)
{
	UParticleSystem* Template = LoadParticleSafe(ParticlePath);
	if (!Template)
	{
		Template = LoadParticleSafe(AetheriaWorldConst::ParticleTorchFire);
	}
	if (!Template)
	{
		Template = LoadParticleSafe(AetheriaWorldConst::ParticlePitFire);
	}
	if (!Template)
	{
		Template = LoadParticleSafe(AetheriaWorldConst::ParticleAncientFire);
	}
	if (!Template)
	{
		return nullptr;
	}

	const FName UniqueName = MakeUniqueObjectName(this, UParticleSystemComponent::StaticClass(), FName(*CompName));
	UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(this, UniqueName);
	if (!PSC)
	{
		return nullptr;
	}

	PSC->SetTemplate(Template);
	PSC->bAutoActivate = true;
	PSC->bAutoDestroy = false;
	PSC->SetMobility(EComponentMobility::Movable);
	PSC->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
	PSC->SetWorldLocationAndRotation(WorldLoc, WorldRot);
	PSC->SetWorldScale3D(WorldScale);
	PSC->RegisterComponent();
	PSC->ActivateSystem(true);

	SpawnedParticles.Add(PSC);
	return PSC;
}

void AAetheriaWorldBuilder::BuildTiledStoneFloor(
	const FString& Prefix,
	float CenterX,
	float CenterY,
	float HalfExtentX,
	float HalfExtentY,
	float TileStep,
	float LiftCm,
	const TCHAR* FloorMeshPath,
	UMaterialInterface* PrimaryMat,
	UMaterialInterface* AccentMat,
	float AccentHalfWidthX,
	bool bFollowTerrain,
	float FixedBaseZ)
{
	UStaticMesh* FloorMesh = LoadMeshSafe(FloorMeshPath, AetheriaWorldConst::MeshDungeonFloor);
	if (!FloorMesh)
	{
		return;
	}

	const FBox FloorBox = AetheriaWorldHelpers::GetSafeBoundingBox(FloorMesh);
	const FVector FloorSize = FloorBox.GetSize();
	const FVector RawCenter = FloorBox.GetCenter();

	// 3% overlap eliminates hairline cracks; pivot-compensated placement handles corner-pivoted MedievalDungeon tiles
	const float ScaleX = (TileStep * 1.03f) / FMath::Max(10.f, FloorSize.X);
	const float ScaleY = (TileStep * 1.03f) / FMath::Max(10.f, FloorSize.Y);
	const float ScaleZ = FMath::Clamp(34.f / FMath::Max(6.f, FloorSize.Z), 0.35f, 2.2f);
	const FVector TileScale(ScaleX, ScaleY, ScaleZ);

	const int32 HalfCountX = FMath::Max(0, FMath::CeilToInt(HalfExtentX / TileStep));
	const int32 HalfCountY = FMath::Max(0, FMath::CeilToInt(HalfExtentY / TileStep));
	const float CenterTerrainZ = bFollowTerrain ? SampleGroundZ(CenterX, CenterY, FallbackGroundZ) : FixedBaseZ;

	for (int32 GX = -HalfCountX; GX <= HalfCountX; ++GX)
	{
		for (int32 GY = -HalfCountY; GY <= HalfCountY; ++GY)
		{
			const float TX = CenterX + static_cast<float>(GX) * TileStep;
			const float TY = CenterY + static_cast<float>(GY) * TileStep;
			const float BaseZ = bFollowTerrain
				? FMath::Max(SampleGroundZ(TX, TY, CenterTerrainZ), CenterTerrainZ - 14.f)
				: FixedBaseZ;

			// Deterministic 90-degree tile rotation breaks up repeating stone grid patterns
			const int32 QuarterTurn = ((GX * 7 + GY * 13) & 3);
			const FRotator TileRot(0.f, static_cast<float>(QuarterTurn) * 90.f, 0.f);
			const FVector LocalTopCenter(RawCenter.X * ScaleX, RawCenter.Y * ScaleY, FloorBox.Max.Z * ScaleZ);
			const FVector RotatedAnchor = TileRot.RotateVector(LocalTopCenter);
			const FVector TileLoc(TX - RotatedAnchor.X, TY - RotatedAnchor.Y, (BaseZ + LiftCm) - RotatedAnchor.Z);

			const bool bUseAccent = (AccentMat != nullptr && AccentHalfWidthX > 0.f && FMath::Abs(TX - CenterX) <= AccentHalfWidthX);
			UMaterialInterface* TileMat = bUseAccent ? AccentMat : PrimaryMat;

			AddWorldMesh(
				FString::Printf(TEXT("%s_%d_%d"), *Prefix, GX + HalfCountX, GY + HalfCountY),
				FloorMeshPath,
				TileLoc,
				TileRot,
				TileScale,
				TileMat,
				AetheriaWorldConst::MeshDungeonFloor,
				true);
		}
	}
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
	bool bNightBoost,
	float SpecularScale)
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
	Light->SetSourceRadius(24.f);
	Light->SetSpecularScale(SpecularScale);
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
	// Always ground with real MedievalDungeon SM_Fire_Pit + real Cascade P_Torch_Fire (zero bright cones!)
	const TCHAR* SafeBrazierPath =
		(!BrazierMeshPath || FCString::Stristr(BrazierMeshPath, TEXT("SM_IronBrazier")) != nullptr || FCString::Stristr(BrazierMeshPath, TEXT("SM_StoneFirePit")) != nullptr)
			? AetheriaWorldConst::MeshFirePit
			: BrazierMeshPath;

	UStaticMesh* PitMesh = LoadMeshSafe(SafeBrazierPath, AetheriaWorldConst::MeshFirePit);
	const float PitScale = ScaleFactor * AetheriaWorldHelpers::ComputeUniformScaleForHeight(PitMesh, 56.f, 0.25f, 3.5f);
	const FVector PitLoc = AetheriaWorldHelpers::ComputeGroundedCenterLoc(
		PitMesh, GroundLoc.X, GroundLoc.Y, GroundLoc.Z, FRotator::ZeroRotator, FVector(PitScale), 4.f);

	UMaterialInterface* PitMat = LoadPBRMaterial({
		AetheriaWorldConst::MatFirePitInst,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone
	});

	AddWorldMesh(
		Prefix + TEXT("_Base"),
		SafeBrazierPath,
		PitLoc,
		FRotator::ZeroRotator,
		FVector(PitScale),
		PitMat,
		AetheriaWorldConst::MeshFirePit,
		true);

	const FBox PitBox = AetheriaWorldHelpers::GetSafeBoundingBox(PitMesh);
	const float FireTopZ = PitLoc.Z + PitBox.Max.Z * PitScale - 6.f;
	const float FireScale = FMath::Clamp(0.85f * ScaleFactor, 0.55f, 1.35f);

	SpawnFireParticle(
		Prefix + TEXT("_FireVFX"),
		AetheriaWorldConst::ParticleTorchFire,
		FVector(GroundLoc.X, GroundLoc.Y, FireTopZ),
		FVector(FireScale));

	AddAmbientFireLight(
		Prefix + TEXT("_Light"),
		FVector(GroundLoc.X, GroundLoc.Y, FireTopZ + 58.f * ScaleFactor),
		FMath::Clamp(Lumens, 350.f, 950.f),
		2350.f,
		AttenuationRadius,
		FLinearColor(1.0f, 0.62f, 0.24f, 1.0f),
		0.14f,
		PhaseOffset,
		true,
		0.14f);
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

	// 1. Immediately eliminate WorldGridMaterial on OpenWorld Landscape & default level slabs
	PatchOpenWorldLandscapeMaterials();

	// 2. Deep distant highland bedrock skirt under the 4km world so horizon/valley edges NEVER show a black void
	UMaterialInterface* BedrockMat = LoadPBRMaterial({
		AetheriaWorldConst::MatNamaqualandCliff,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatBrickFloorClean
	});
	AddWorldMesh(
		TEXT("World_HighlandBedrockSkirt"),
		AetheriaWorldConst::MeshPlane,
		FVector(0.f, -2000.f, FallbackGroundZ - 1650.f),
		FRotator::ZeroRotator,
		FVector(4200.f, 4200.f, 1.f),
		BedrockMat,
		AetheriaWorldConst::MeshPlane,
		false);

	BuildOakhavenVillage();
	BuildMirrorLakeAndBridge();
	BuildDragonAerie();
	BuildSapphireWhisperingWoods();
	BuildHighkeepCitadel();
	BuildAshCalderaAndEmberCrypt();
	BuildCrystalGlacierPeaks();
	BuildSkyRuneRings();

	UE_LOG(LogChronicles, Display,
		TEXT("[WorldBuilder] Built 5-biome Crimson Desert world: %d static meshes, %d fire VFX, %d animated lights, %d destructibles, %d boulders, %d trees."),
		SpawnedMeshes.Num(),
		SpawnedParticles.Num(),
		AnimatedLights.Num(),
		SpawnedDestructibles.Num(),
		MossyBouldersISM ? MossyBouldersISM->GetInstanceCount() : 0,
		TreeTrunksISM ? TreeTrunksISM->GetInstanceCount() : 0);
}

void AAetheriaWorldBuilder::BuildOakhavenVillage()
{
	// Centered around (0, 4200); keep central plaza sightline X in [-420..+420], Y in [2600..5000] 100% open to Grand Abbey!
	UMaterialInterface* StoneAltarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* BrickFloorMat = LoadPBRMaterial({
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* StoneWallMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatCryptWallA,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* PillarStoneMat = LoadPBRMaterial({
		AetheriaWorldConst::MatPillarInst,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* FirePitMat = LoadPBRMaterial({
		AetheriaWorldConst::MatFirePitInst,
		AetheriaWorldConst::MatStoneWall
	});

	// 1. Monumental Gothic Flagstone Courtyard Floor across X in [-2700..+2700], Y in [2300..5900]
	BuildTiledStoneFloor(
		TEXT("Oakhaven_CourtyardFloor"),
		0.f,
		4100.f,
		2400.f,
		1500.f,
		600.f,
		4.f,
		AetheriaWorldConst::MeshDungeonFloor,
		StoneAltarMat,
		BrickFloorMat,
		750.f,
		true,
		FallbackGroundZ);

	// 2. Authentic Full-3D Gothic Village Manor Halls & Colonnades (ZERO single-sided wall screens showing flat backs!)
	UStaticMesh* GothicBuildingMesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicBuilding09, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* SolidArcMesh       = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* PillarMesh         = LoadMeshSafe(AetheriaWorldConst::MeshDungeonPillar, AetheriaWorldConst::MeshGothicPillar);
	UStaticMesh* DoorWayMesh        = LoadMeshSafe(AetheriaWorldConst::MeshDoorWay, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* StepsMesh          = LoadMeshSafe(AetheriaWorldConst::MeshGothicSteps, AetheriaWorldConst::MeshEdgeStones);
	UStaticMesh* PotMesh            = LoadMeshSafe(AetheriaWorldConst::MeshMedievalPot, AetheriaWorldConst::MeshDungeonPillar);

	const float AbbeyScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GothicBuildingMesh, 1180.f, 0.2f, 2.4f);
	const float ManorScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GothicBuildingMesh, 880.f, 0.2f, 2.2f);
	const float SolidArcUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SolidArcMesh, 440.f, 0.2f, 3.2f);
	const float PillarScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PillarMesh, 430.f, 0.2f, 3.2f);
	const float DoorScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(DoorWayMesh, 410.f, 0.2f, 3.0f);
	const float StepsScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StepsMesh, 90.f, 0.2f, 3.0f);
	const float PotScale     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PotMesh, 82.f, 0.2f, 3.0f);

	// Grand Gothic Sanctuary Abbey backdrop behind the plaza at (0, 6250)
	{
		const float AbbeyGZ = SampleGroundZ(0.f, 6250.f, FallbackGroundZ);
		const FRotator AbbeyRot(0.f, 180.f, 0.f);
		AddWorldMesh(
			TEXT("Oakhaven_GrandAbbeyBackdrop"),
			AetheriaWorldConst::MeshGothicBuilding09,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(GothicBuildingMesh, 0.f, 6250.f, AbbeyGZ, AbbeyRot, FVector(AbbeyScale), 18.f),
			AbbeyRot,
			FVector(AbbeyScale));

		// Full 3D Gothic colonnade pillars & solid arches flanking the Abbey & Statues (no single-sided screens!)
		const float ColonnadeX[4] = { -1020.f, -640.f, 640.f, 1020.f };
		for (int32 Idx = 0; Idx < 4; ++Idx)
		{
			const float CX = ColonnadeX[Idx];
			const float CY = 5240.f;
			const float CGZ = SampleGroundZ(CX, CY, FallbackGroundZ) + 4.f;
			const FRotator ColRot(0.f, (CX < 0.f) ? -15.f : 15.f, 0.f);

			AddWorldMesh(
				FString::Printf(TEXT("Oakhaven_SanctuaryPillar_%d"), Idx),
				AetheriaWorldConst::MeshDungeonPillar,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(PillarMesh, CX, CY, CGZ, ColRot, FVector(PillarScale), 6.f),
				ColRot,
				FVector(PillarScale),
				PillarStoneMat);

			if (Idx == 0 || Idx == 3)
			{
				const float AX = CX + (CX < 0.f ? 190.f : -190.f);
				AddWorldMesh(
					FString::Printf(TEXT("Oakhaven_SanctuaryArch_%d"), Idx),
					AetheriaWorldConst::MeshGothicArc,
					AetheriaWorldHelpers::ComputeGroundedCenterLoc(SolidArcMesh, AX, CY + 60.f, CGZ, ColRot, FVector(SolidArcUnit), 6.f),
					ColRot,
					FVector(SolidArcUnit));
			}
		}
	}

	// Flanking Gothic Manor Halls on West & East sides of the courtyard
	const FVector2D ManorCoords[2] = { FVector2D(-2350.f, 4900.f), FVector2D(2350.f, 4900.f) };
	const float ManorYaws[2]       = { -75.f, 75.f };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float MX = ManorCoords[Idx].X;
		const float MY = ManorCoords[Idx].Y;
		const float MGZ = SampleGroundZ(MX, MY, FallbackGroundZ);
		const FRotator MRot(0.f, ManorYaws[Idx], 0.f);
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_GothicManor_%d"), Idx),
			AetheriaWorldConst::MeshGothicBuilding09,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(GothicBuildingMesh, MX, MY, MGZ, MRot, FVector(ManorScale), 16.f),
			MRot,
			FVector(ManorScale));
	}

	// 4 Sculpted Full-3D Gothic Courtyard Pavilions (West/East mid-plaza + South flanks framing Studio camera backdrop!)
	struct FPavilionSpec
	{
		FVector2D Pos;
		float Yaw;
		float Scale;
	};

	const FPavilionSpec Pavilions[] = {
		{ FVector2D(-1750.f, 3720.f),   90.f, 1.00f },
		{ FVector2D( 1750.f, 3720.f),  -90.f, 1.00f },
		{ FVector2D(-1380.f, 2760.f),   25.f, 0.96f },
		{ FVector2D( 1380.f, 2760.f),  -25.f, 0.96f }
	};

	for (int32 Idx = 0; Idx < UE_ARRAY_COUNT(Pavilions); ++Idx)
	{
		const FVector2D P = Pavilions[Idx].Pos;
		const FRotator Rot(0.f, Pavilions[Idx].Yaw, 0.f);
		const float S = Pavilions[Idx].Scale;
		const FVector Forward = Rot.Vector();
		const FVector Right = Rot.RotateVector(FVector::RightVector);

		const float BaseGZ = SampleGroundZ(P.X, P.Y, FallbackGroundZ) + 4.f;
		const float AScale = SolidArcUnit * S;
		const float PScale = PillarScale * S;
		const float DScale = DoorScale * S;

		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_PavilionArchMain_%d"), Idx),
			AetheriaWorldConst::MeshGothicArc,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(SolidArcMesh, P.X, P.Y, BaseGZ, Rot, FVector(AScale), 8.f),
			Rot,
			FVector(AScale));

		const FVector DoorPos = FVector(P.X, P.Y, 0.f) - Right * (240.f * S);
		const float DoorGZ = SampleGroundZ(DoorPos.X, DoorPos.Y, BaseGZ) + 4.f;
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_PavilionDoor_%d"), Idx),
			AetheriaWorldConst::MeshDoorWay,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(DoorWayMesh, DoorPos.X, DoorPos.Y, DoorGZ, Rot, FVector(DScale), 8.f),
			Rot,
			FVector(DScale),
			StoneWallMat);

		const FVector PillarPosL = FVector(P.X, P.Y, 0.f) + Forward * (190.f * S) - Right * (150.f * S);
		const float PillarGZL = SampleGroundZ(PillarPosL.X, PillarPosL.Y, BaseGZ) + 4.f;
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_PavilionPillarL_%d"), Idx),
			AetheriaWorldConst::MeshDungeonPillar,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(PillarMesh, PillarPosL.X, PillarPosL.Y, PillarGZL, Rot, FVector(PScale), 6.f),
			Rot,
			FVector(PScale),
			PillarStoneMat);

		const FVector PillarPosR = FVector(P.X, P.Y, 0.f) + Forward * (190.f * S) + Right * (150.f * S);
		const float PillarGZR = SampleGroundZ(PillarPosR.X, PillarPosR.Y, BaseGZ) + 4.f;
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_PavilionPillarR_%d"), Idx),
			AetheriaWorldConst::MeshDungeonPillar,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(PillarMesh, PillarPosR.X, PillarPosR.Y, PillarGZR, Rot, FVector(PScale), 6.f),
			Rot,
			FVector(PScale),
			PillarStoneMat);

		const FVector StepsPos = FVector(P.X, P.Y, 0.f) + Forward * (240.f * S);
		const float StepsGZ = SampleGroundZ(StepsPos.X, StepsPos.Y, BaseGZ) + 4.f;
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_PavilionSteps_%d"), Idx),
			AetheriaWorldConst::MeshGothicSteps,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(StepsMesh, StepsPos.X, StepsPos.Y, StepsGZ, Rot, FVector(StepsScale * S), 6.f),
			Rot,
			FVector(StepsScale * S),
			StoneAltarMat);

		const FVector PotPos = FVector(P.X, P.Y, 0.f) + Forward * (175.f * S) + Right * (225.f * S);
		const float PotGZ = SampleGroundZ(PotPos.X, PotPos.Y, BaseGZ) + 4.f;
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_Vessel_%d"), Idx),
			AetheriaWorldConst::MeshMedievalPot,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(PotMesh, PotPos.X, PotPos.Y, PotGZ, FRotator(0.f, Idx * 47.f, 0.f), FVector(PotScale), 2.f),
			FRotator(0.f, Idx * 47.f, 0.f),
			FVector(PotScale));
	}

	// 3. Clean Waist-High Carved Sanctuary Altar at (0, 5150) & Grounded Chest — ZERO giant center obelisk blocking the Grand Abbey!
	{
		UStaticMesh* ShrineMesh = LoadMeshSafe(AetheriaWorldConst::MeshMedievalAltar, AetheriaWorldConst::MeshDungeonPillar);
		const float ShrineScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ShrineMesh, 118.f, 0.2f, 1.8f);
		const float ShrineGZ = SampleGroundZ(0.f, 5150.f, FallbackGroundZ) + 4.f;
		const FRotator ShrineRot(0.f, 180.f, 0.f);
		AddWorldMesh(
			TEXT("Oakhaven_SanctuaryShrine"),
			AetheriaWorldConst::MeshMedievalAltar,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(ShrineMesh, 0.f, 5150.f, ShrineGZ, ShrineRot, FVector(ShrineScale, ShrineScale, ShrineScale * 0.9f), 4.f),
			ShrineRot,
			FVector(ShrineScale, ShrineScale, ShrineScale * 0.9f),
			StoneAltarMat);

		UStaticMesh* ChestMesh = LoadMeshSafe(AetheriaWorldConst::MeshTreasureChest, AetheriaWorldConst::MeshMedievalPot);
		const float ChestGZ = SampleGroundZ(290.f, 4620.f, FallbackGroundZ) + 4.f;
		const FRotator ChestRot(0.f, -35.f, 0.f);
		AddWorldMesh(
			TEXT("Oakhaven_TreasureChest"),
			AetheriaWorldConst::MeshTreasureChest,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(ChestMesh, 290.f, 4620.f, ChestGZ, ChestRot, FVector(1.2f), 2.f),
			ChestRot,
			FVector(1.2f));
	}

	// 4. 2 Photoreal TA_Sample_Statue_01 monuments on SM_TA_Sample_Pedestal_01 overlooking the central plaza
	UStaticMesh* PedestalMesh = LoadMeshSafe(AetheriaWorldConst::MeshStatuePedestal, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* StatueMesh   = LoadMeshSafe(AetheriaWorldConst::MeshStatue01, AetheriaWorldConst::MeshGothicPillar);
	const float PedestalScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PedestalMesh, 135.f, 0.1f, 3.5f);
	const float StatueScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StatueMesh, 240.f, 0.1f, 3.5f);

	const FVector2D PlazaStatueCoords[2] = { FVector2D(-360.f, 4180.f), FVector2D(360.f, 4180.f) };
	const float PlazaStatueYaws[2] = { -65.f, -115.f };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float SX = PlazaStatueCoords[Idx].X;
		const float SY = PlazaStatueCoords[Idx].Y;
		const float GroundZ = SampleGroundZ(SX, SY, FallbackGroundZ) + 4.f;
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

	// 5. 6 Grounded Street Braziers / Lanterns (SM_Pillar2 + SM_Fire_Pit bowl + REAL Cascade P_Torch_Fire + 550 lm SpecularScale=0.14)
	UStaticMesh* Pillar2Mesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar2, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* BowlMesh    = LoadMeshSafe(AetheriaWorldConst::MeshFirePit, AetheriaWorldConst::MeshDungeonPillar);
	const float Pillar2Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Pillar2Mesh, 195.f, 0.1f, 3.5f);
	const float BowlScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BowlMesh, 32.f, 0.1f, 2.5f);

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
		const float LZ = SampleGroundZ(LX, LY, FallbackGroundZ) + 4.f;
		const FRotator PostRot(0.f, Idx * 60.f, 0.f);
		const FVector PostLoc = AetheriaWorldHelpers::ComputeGroundedCenterLoc(Pillar2Mesh, LX, LY, LZ, PostRot, FVector(Pillar2Scale), 6.f);
		const float BowlZ = AetheriaWorldHelpers::ComputeStackedZ(Pillar2Mesh, PostLoc.Z, Pillar2Scale, BowlMesh, BowlScale, 4.f);
		const FBox BowlBox = AetheriaWorldHelpers::GetSafeBoundingBox(BowlMesh);
		const FVector BowlCenter = BowlBox.GetCenter();
		const FVector BowlLoc(LX - BowlCenter.X * BowlScale, LY - BowlCenter.Y * BowlScale, BowlZ);
		const float FireTopZ = BowlZ + BowlBox.Max.Z * BowlScale - 4.f;

		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_LanternPost_%d"), Idx),
			AetheriaWorldConst::MeshGothicPillar2,
			PostLoc,
			PostRot,
			FVector(Pillar2Scale));

		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_LanternBowl_%d"), Idx),
			AetheriaWorldConst::MeshFirePit,
			BowlLoc,
			FRotator::ZeroRotator,
			FVector(BowlScale),
			FirePitMat);

		SpawnFireParticle(
			FString::Printf(TEXT("Oakhaven_LanternFireVFX_%d"), Idx),
			AetheriaWorldConst::ParticleTorchFire,
			FVector(LX, LY, FireTopZ),
			FVector(0.85f));

		AddAmbientFireLight(
			FString::Printf(TEXT("Oakhaven_LanternLight_%d"), Idx),
			FVector(LX, LY, FireTopZ + 45.f),
			550.f,
			2400.f,
			980.f,
			FLinearColor(1.0f, 0.70f, 0.34f, 1.0f),
			0.09f,
			Idx * 1.15f,
			true,
			0.14f);
	}

	// 6. 2 Destructible Runic Stone Pillars flanking south village approach (LinkedQuestId = "Q_AwakenExort")
	// Dark gothic stone M_Pillar_Inst + compact glowing ember rune crystal crown (NEVER a giant solid neon yellow block!)
	UStaticMesh* ExortPillarMesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar, AetheriaWorldConst::MeshDungeonPillar);
	const FLinearColor ExortAmberGlow(1.15f, 0.42f, 0.08f, 1.0f);
	const FVector2D CrystalCoords[2] = { FVector2D(-560.f, 3250.f), FVector2D(560.f, 3250.f) };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float CX = CrystalCoords[Idx].X;
		const float CY = CrystalCoords[Idx].Y;
		const float CZ = SampleGroundZ(CX, CY, FallbackGroundZ) + 4.f;
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

	// Luminous dusk-teal reflective water surface (never crushes to pitch black beneath Ignis!)
	MirrorWaterMID = CreateColorMID(FLinearColor(0.06f, 0.22f, 0.30f, 1.0f), false);
	if (MirrorWaterMID)
	{
		MirrorWaterMID->SetScalarParameterValue(TEXT("Roughness"), 0.14f);
		MirrorWaterMID->SetScalarParameterValue(TEXT("Metallic"), 0.06f);
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

	// Arched Flagstone Bridge spanning Y: 1600 -> -3200 (4800 cm total length, 9 smooth arch spans)
	const int32 NumSpans = 9;
	const float StartY = 1600.f;
	const float EndY   = -3200.f;
	const float ArchApexLift = 210.f;

	UMaterialInterface* BridgeDeckMat = LoadPBRMaterial({
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* BridgeArchMat = LoadPBRMaterial({
		AetheriaWorldConst::MatDecorativeArches,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* ParapetMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatEdgeStonesInst,
		AetheriaWorldConst::MatAetheriaGothicStone
	});

	UStaticMesh* DeckMesh    = LoadMeshSafe(AetheriaWorldConst::MeshDungeonFloor, AetheriaWorldConst::MeshCryptFloor);
	UStaticMesh* ArchMesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* ParapetMesh = LoadMeshSafe(AetheriaWorldConst::MeshDungeonWallHalf, AetheriaWorldConst::MeshEdgeStones);

	const FBox DeckBox = AetheriaWorldHelpers::GetSafeBoundingBox(DeckMesh);
	const FVector DeckRawSize = DeckBox.GetSize();
	const FVector DeckRawCenter = DeckBox.GetCenter();
	const float ArchScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ArchMesh, 260.f, 0.2f, 2.8f);
	const float ParapetScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ParapetMesh, 110.f, 0.2f, 2.5f);

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

		// Solid walkable medieval flagstone deck aligned along parabolic arch tangent (replaces Cube.Cube + BasicShapeMaterial!)
		const FVector DeckScale(
			620.f / FMath::Max(10.f, DeckRawSize.X),
			(SegLen + 45.f) / FMath::Max(10.f, DeckRawSize.Y),
			FMath::Clamp(48.f / FMath::Max(6.f, DeckRawSize.Z), 0.4f, 2.4f));
		const FVector LocalDeckTop(DeckRawCenter.X * DeckScale.X, DeckRawCenter.Y * DeckScale.Y, DeckBox.Max.Z * DeckScale.Z);
		const FVector DeckWorldLoc = Mid - DeckRot.RotateVector(LocalDeckTop);

		AddWorldMesh(
			FString::Printf(TEXT("MirrorBridge_Deck_%d"), Idx),
			AetheriaWorldConst::MeshDungeonFloor,
			DeckWorldLoc,
			DeckRot,
			DeckScale,
			BridgeDeckMat,
			AetheriaWorldConst::MeshDungeonFloor,
			true);

		// Supporting full-3D gothic stone arches underneath the bridge deck
		if (Idx > 0 && Idx < NumSpans - 1)
		{
			const FRotator ArchRot(0.f, 90.f, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("MirrorBridge_Arch_%d"), Idx),
				AetheriaWorldConst::MeshGothicArc,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(ArchMesh, 0.f, Mid.Y, WaterZ - 35.f, ArchRot, FVector(ArchScale * 1.15f), 8.f),
				ArchRot,
				FVector(ArchScale * 1.15f),
				BridgeArchMat);
		}

		// Left & right carved low stone parapet walls
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const FRotator ParaRot(0.f, Side > 0 ? 90.f : -90.f, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("MirrorBridge_Parapet_%d_%d"), Idx, Side > 0 ? 1 : 0),
				AetheriaWorldConst::MeshDungeonWallHalf,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(ParapetMesh, Side * 305.f, Mid.Y, Mid.Z, ParaRot, FVector(ParapetScale), 8.f),
				ParaRot,
				FVector(ParapetScale),
				ParapetMat);
		}
	}

	// 6 Flaming Bridge Braziers (SM_Fire_Pit + P_Torch_Fire + 850 lm warm firelight, SpecularScale=0.14)
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
				FVector(Side * 275.f, BrazierYs[Row], DeckZ + 2.f),
				850.f,
				1350.f,
				AetheriaWorldConst::MeshFirePit,
				1.05f,
				BIdx * 0.9f);
		}
	}
}

void AAetheriaWorldBuilder::BuildDragonAerie()
{
	// Elevated runic stone dais at (-3400, 2200) + dramatic textured highland cliffs & 3D stone ruins below Ignis's flight corridor
	const float CenterX = -3400.f;
	const float CenterY =  2200.f;
	const float BaseZ   = SampleGroundZ(CenterX, CenterY, FallbackGroundZ);

	UMaterialInterface* StoneAltarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* StoneWallMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* PillarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatPillarInst,
		AetheriaWorldConst::MatStoneWall
	});
	// SM6-safe Namaqualand cliff rock material (replaces KiteDemo M_Photoscan_Master which fails PCD3D_SM6!)
	UMaterialInterface* CliffRockMat = LoadPBRMaterial({
		AetheriaWorldConst::MatNamaqualandCliff,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatDirtMounds
	});

	// Grounded Gothic flagstone sanctuary platform
	BuildTiledStoneFloor(
		TEXT("DragonAerie_DaisFloor"),
		CenterX,
		CenterY,
		620.f,
		620.f,
		420.f,
		44.f,
		AetheriaWorldConst::MeshDungeonFloor,
		StoneAltarMat,
		nullptr,
		0.f,
		false,
		BaseZ);

	UStaticMesh* AeriePedMesh   = LoadMeshSafe(AetheriaWorldConst::MeshStatuePedestal, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* AerieAltarMesh = LoadMeshSafe(AetheriaWorldConst::MeshMedievalAltar, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* SolidArcMesh   = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshRuinedArch);
	UStaticMesh* PillarMesh     = LoadMeshSafe(AetheriaWorldConst::MeshDungeonPillar, AetheriaWorldConst::MeshGothicPillar);

	const float AeriePedScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(AeriePedMesh, 78.f, 0.1f, 3.0f);
	const float AerieAltarScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(AerieAltarMesh, 125.f, 0.2f, 2.2f);
	const FVector PedLoc = AetheriaWorldHelpers::ComputeGroundedCenterLoc(
		AeriePedMesh, CenterX, CenterY, BaseZ + 44.f, FRotator::ZeroRotator, FVector(AeriePedScale), 4.f);
	AddWorldMesh(
		TEXT("DragonAerie_Dais"),
		AetheriaWorldConst::MeshStatuePedestal,
		PedLoc,
		FRotator::ZeroRotator,
		FVector(AeriePedScale));

	const float AltarTopZ = PedLoc.Z + AetheriaWorldHelpers::GetSafeBoundingBox(AeriePedMesh).Max.Z * AeriePedScale;
	const FRotator AltarRot(0.f, -35.f, 0.f);
	AddWorldMesh(
		TEXT("DragonAerie_Altar"),
		AetheriaWorldConst::MeshMedievalAltar,
		AetheriaWorldHelpers::ComputeGroundedCenterLoc(AerieAltarMesh, CenterX, CenterY, AltarTopZ, AltarRot, FVector(AerieAltarScale), 4.f),
		AltarRot,
		FVector(AerieAltarScale),
		StoneAltarMat);

	// Ring of 6 grounded full-3D gothic arches & pillars overlooking Mirror Lake
	const float AerieArchScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SolidArcMesh, 420.f, 0.2f, 3.0f);
	const float AeriePillarScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PillarMesh, 400.f, 0.2f, 3.0f);
	for (int32 Idx = 0; Idx < 6; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 6.f) * UE_TWO_PI;
		const float PX = CenterX + FMath::Cos(Angle) * 620.f;
		const float PY = CenterY + FMath::Sin(Angle) * 620.f;
		const float PZ = SampleGroundZ(PX, PY, BaseZ);
		const FRotator RingRot(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f);
		const bool bUseArch = (Idx % 2 == 0);
		UStaticMesh* PieceMesh = bUseArch ? SolidArcMesh : PillarMesh;
		const float PieceScale = bUseArch ? AerieArchScale : AeriePillarScale;

		AddWorldMesh(
			FString::Printf(TEXT("DragonAerie_Arch_%d"), Idx),
			bUseArch ? AetheriaWorldConst::MeshGothicArc : AetheriaWorldConst::MeshDungeonPillar,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(PieceMesh, PX, PY, PZ, RingRot, FVector(PieceScale), 12.f),
			RingRot,
			FVector(PieceScale),
			bUseArch ? nullptr : PillarMat);
	}

	// 2 Beacon Braziers at the Aerie terrace (SM_Fire_Pit + P_Torch_Fire + 850 lm)
	AddBrazierAssembly(TEXT("DragonAerie_BrazierL"), FVector(CenterX - 360.f, CenterY + 460.f, BaseZ + 44.f), 850.f, 1350.f, AetheriaWorldConst::MeshFirePit, 1.05f, 0.4f);
	AddBrazierAssembly(TEXT("DragonAerie_BrazierR"), FVector(CenterX + 360.f, CenterY + 460.f, BaseZ + 44.f), 850.f, 1350.f, AetheriaWorldConst::MeshFirePit, 1.05f, 2.2f);

	// 2 Destructible Runic Bind-Monoliths (LinkedQuestId = "Q_TameDragon") — grounded SM_Pillar
	const FLinearColor AzureBindGlow(0.22f, 0.95f, 1.35f, 1.0f);
	const FVector2D BindCoords[2] = {
		FVector2D(CenterX - 340.f, CenterY - 190.f),
		FVector2D(CenterX + 340.f, CenterY - 190.f)
	};
	const float BindScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PillarMesh, 260.f, 0.25f, 2.5f);
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float BX = BindCoords[Idx].X;
		const float BY = BindCoords[Idx].Y;
		const float BZ = AetheriaWorldHelpers::ComputeGroundedZ(PillarMesh, BaseZ + 44.f, BindScale, 6.f);
		SpawnDestructibleProp(
			FVector(BX, BY, BZ),
			FRotator(0.f, Idx * 60.f, 0.f),
			EDestructibleKind::Crystal,
			110.f,
			AetheriaWorldConst::MeshDungeonPillar,
			FVector(BindScale),
			FName(TEXT("Q_TameDragon")),
			&AzureBindGlow,
			true);
	}

	// Dress the Valley Floor & Mirror Lake Highlands beneath Dragon Ignis's flight path (-1800..1200, -2400..3000)
	// Explicitly override KiteDemo cliffs/rocks with SM6-safe MI_Cliff_Side so ZERO rocks fall back to DefaultMaterial!
	UStaticMesh* CliffMesh    = LoadMeshSafe(AetheriaWorldConst::MeshKiteCliff01, AetheriaWorldConst::MeshKiteMountainRock);
	UStaticMesh* MtnRockMesh  = LoadMeshSafe(AetheriaWorldConst::MeshKiteMountainRock, AetheriaWorldConst::MeshCliffBoulder);
	UStaticMesh* RiverRockMesh= LoadMeshSafe(AetheriaWorldConst::MeshKiteRiverRock, AetheriaWorldConst::MeshMWAMStoneA);

	struct FCliffSpec
	{
		FVector2D Pos;
		float Yaw;
		float TargetHeight;
	};

	const FCliffSpec ValleyCliffs[] = {
		{ FVector2D(-2150.f,   400.f),   35.f, 760.f },
		{ FVector2D(-2350.f, -1550.f),   72.f, 880.f },
		{ FVector2D( 2100.f,   250.f), -125.f, 780.f },
		{ FVector2D( 2250.f, -1800.f), -160.f, 920.f },
		{ FVector2D(-1750.f,  2350.f),   18.f, 620.f },
		{ FVector2D( 1850.f,  1950.f), -145.f, 640.f }
	};

	for (int32 Idx = 0; Idx < UE_ARRAY_COUNT(ValleyCliffs); ++Idx)
	{
		const FCliffSpec& Spec = ValleyCliffs[Idx];
		const float GZ = SampleGroundZ(Spec.Pos.X, Spec.Pos.Y, FallbackGroundZ);
		const float CScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CliffMesh, Spec.TargetHeight, 0.12f, 3.2f);
		const FRotator CRot(0.f, Spec.Yaw, 0.f);
		AddWorldMesh(
			FString::Printf(TEXT("Valley_KiteCliff_%d"), Idx),
			AetheriaWorldConst::MeshKiteCliff01,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(CliffMesh, Spec.Pos.X, Spec.Pos.Y, GZ, CRot, FVector(CScale), 35.f),
			CRot,
			FVector(CScale),
			CliffRockMat);
	}

	// Rocky lake islets & highland outcrops beneath Ignis's flight corridor (clearing Mirror Bridge |X|<480)
	const FCliffSpec ValleyRocks[] = {
		{ FVector2D(-1120.f,  1650.f),  24.f, 260.f },
		{ FVector2D(-1380.f,   620.f), 110.f, 310.f },
		{ FVector2D( -960.f,  -420.f),  65.f, 240.f },
		{ FVector2D(-1280.f, -1480.f), 195.f, 340.f },
		{ FVector2D( -840.f, -2450.f), 280.f, 270.f },
		{ FVector2D(  920.f,  1480.f), -42.f, 250.f },
		{ FVector2D( 1180.f,   280.f), -95.f, 290.f },
		{ FVector2D(  880.f,  -880.f), 145.f, 230.f },
		{ FVector2D( 1260.f, -1960.f), 215.f, 320.f },
		{ FVector2D( -680.f,  2480.f),  48.f, 210.f },
		{ FVector2D(  760.f,  2380.f), -70.f, 220.f },
		{ FVector2D( -540.f, -3420.f),  15.f, 280.f },
		{ FVector2D(  620.f, -3520.f), -35.f, 280.f },
		{ FVector2D(-1650.f,  -750.f),  88.f, 360.f }
	};

	for (int32 Idx = 0; Idx < UE_ARRAY_COUNT(ValleyRocks); ++Idx)
	{
		const FCliffSpec& Spec = ValleyRocks[Idx];
		const float GZ = SampleGroundZ(Spec.Pos.X, Spec.Pos.Y, FallbackGroundZ);
		const bool bUseRiverRock = (Idx % 3 == 2);
		UStaticMesh* RMesh = bUseRiverRock ? RiverRockMesh : MtnRockMesh;
		const TCHAR* RPath = bUseRiverRock ? AetheriaWorldConst::MeshKiteRiverRock : AetheriaWorldConst::MeshKiteMountainRock;
		const float RScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(RMesh, Spec.TargetHeight, 0.15f, 3.2f);
		const FRotator RRot(0.f, Spec.Yaw, 0.f);
		AddWorldMesh(
			FString::Printf(TEXT("Valley_HighlandRock_%d"), Idx),
			RPath,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(RMesh, Spec.Pos.X, Spec.Pos.Y, GZ, RRot, FVector(RScale), 24.f),
			RRot,
			FVector(RScale),
			CliffRockMat);
	}

	// Ancient full-3D lakeside stone ruin arches visible directly below Dragon Ignis in flight
	const FVector2D LakeRuinCoords[4] = {
		FVector2D( -960.f,  -420.f),
		FVector2D(  880.f,  -880.f),
		FVector2D(-1280.f, -1480.f),
		FVector2D( 1180.f,   280.f)
	};
	const float RuinArchScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SolidArcMesh, 380.f, 0.2f, 2.8f);
	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		const float RX = LakeRuinCoords[Idx].X;
		const float RY = LakeRuinCoords[Idx].Y;
		const float RGZ = SampleGroundZ(RX, RY, FallbackGroundZ);
		const FRotator RRot(0.f, Idx * 55.f + 20.f, 0.f);
		AddWorldMesh(
			FString::Printf(TEXT("Valley_LakesideRuin_%d"), Idx),
			AetheriaWorldConst::MeshGothicArc,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(SolidArcMesh, RX, RY, RGZ, RRot, FVector(RuinArchScale), 12.f),
			RRot,
			FVector(RuinArchScale),
			StoneWallMat);
	}
}

void AAetheriaWorldBuilder::BuildSapphireWhisperingWoods()
{
	// X: -11000..-4200 (West) and 4200..11000 (East), Y: -7600..6600
	// Replace spiky SM_DeadTree wire-cages & broken KiteDemo photoscan shaders with SM6-safe MI_Cliff_Side rocks + weathered SM_Pillar ruins!
	UStaticMesh* RockMesh   = LoadMeshSafe(AetheriaWorldConst::MeshKiteMountainRock, AetheriaWorldConst::MeshMWAMStoneA);
	UStaticMesh* RuinMesh   = LoadMeshSafe(AetheriaWorldConst::MeshDungeonPillar, AetheriaWorldConst::MeshGothicPillar);
	UStaticMesh* FernMesh   = LoadMeshSafe(AetheriaWorldConst::MeshKiteFern01, AetheriaWorldConst::MeshKiteFieldGrass01);

	UMaterialInterface* CliffRockMat = LoadPBRMaterial({
		AetheriaWorldConst::MatNamaqualandCliff,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatDirtMounds
	});
	UMaterialInterface* RuinStoneMat = LoadPBRMaterial({
		AetheriaWorldConst::MatPillarInst,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone
	});

	if (MossyBouldersISM && RockMesh)
	{
		MossyBouldersISM->SetStaticMesh(RockMesh);
		if (CliffRockMat)
		{
			const int32 NumRockMats = FMath::Max(1, MossyBouldersISM->GetNumMaterials());
			for (int32 Idx = 0; Idx < NumRockMats; ++Idx)
			{
				MossyBouldersISM->SetMaterial(Idx, CliffRockMat);
			}
		}
	}
	if (TreeTrunksISM && RuinMesh)
	{
		TreeTrunksISM->SetStaticMesh(RuinMesh);
		if (RuinStoneMat)
		{
			const int32 NumRuinMats = FMath::Max(1, TreeTrunksISM->GetNumMaterials());
			for (int32 Idx = 0; Idx < NumRuinMats; ++Idx)
			{
				TreeTrunksISM->SetMaterial(Idx, RuinStoneMat);
			}
		}
	}
	if (TreeCanopiesISM && FernMesh)
	{
		TreeCanopiesISM->SetStaticMesh(FernMesh);
	}

	const float RockUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(RockMesh, 210.f, 0.12f, 2.6f);
	const float RuinUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(RuinMesh, 380.f, 0.25f, 2.8f);
	const float FernUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(FernMesh, 95.f, 0.25f, 3.2f);

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

	// ~180 instanced highland rock outcrops (SM_MountainRock_Closed + MI_Cliff_Side)
	TArray<FTransform> BoulderTransforms;
	BoulderTransforms.Reserve(180);
	int32 Attempts = 0;
	while (BoulderTransforms.Num() < 180 && Attempts < 1400)
	{
		++Attempts;
		const bool bWest = (BoulderTransforms.Num() % 2 == 0);
		const float X = bWest ? Rng.FRandRange(-11000.f, -4200.f) : Rng.FRandRange(4200.f, 11000.f);
		const float Y = Rng.FRandRange(-7400.f, 6400.f);
		if (IsExcludedLandmarkClearing(X, Y))
		{
			continue;
		}

		const float S = RockUnit * Rng.FRandRange(0.75f, 1.95f);
		const float SZ = S * Rng.FRandRange(0.75f, 1.20f);
		const float GZ = AetheriaWorldHelpers::ComputeGroundedZ(RockMesh, SampleGroundZ(X, Y, FallbackGroundZ), SZ, 20.f);
		const FRotator Rot(Rng.FRandRange(-6.f, 6.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-6.f, 6.f));
		BoulderTransforms.Emplace(Rot, FVector(X, Y, GZ), FVector(S, S * Rng.FRandRange(0.85f, 1.25f), SZ));
	}
	if (MossyBouldersISM && BoulderTransforms.Num() > 0)
	{
		MossyBouldersISM->AddInstances(BoulderTransforms, false, true, false);
	}

	// ~90 weathered highland ruin pillars + ~320 grounded fern undergrowth clusters (zero wire-cage SM_DeadTree spheres!)
	TArray<FTransform> RuinTransforms;
	TArray<FTransform> FernTransforms;
	RuinTransforms.Reserve(90);
	FernTransforms.Reserve(320);

	Attempts = 0;
	while (RuinTransforms.Num() < 90 && Attempts < 1200)
	{
		++Attempts;
		const bool bWest = (RuinTransforms.Num() % 2 == 0);
		const float X = bWest ? Rng.FRandRange(-10800.f, -4400.f) : Rng.FRandRange(4400.f, 10800.f);
		const float Y = Rng.FRandRange(-7200.f, 6200.f);
		if (IsExcludedLandmarkClearing(X, Y))
		{
			continue;
		}

		const float TerrainZ = SampleGroundZ(X, Y, FallbackGroundZ);
		const float RuinScale = RuinUnit * Rng.FRandRange(0.72f, 1.28f);
		const float RuinScaleZ = RuinScale * Rng.FRandRange(0.78f, 1.18f);
		const float RuinZ = AetheriaWorldHelpers::ComputeGroundedZ(RuinMesh, TerrainZ, RuinScaleZ, 18.f);
		const float Yaw = Rng.FRandRange(0.f, 360.f);
		const FRotator RuinRot(Rng.FRandRange(-7.f, 7.f), Yaw, Rng.FRandRange(-7.f, 7.f));

		RuinTransforms.Emplace(RuinRot, FVector(X, Y, RuinZ), FVector(RuinScale, RuinScale, RuinScaleZ));

		// 2-3 lush KiteDemo fern clumps grounded around the base of each weathered ruin pillar
		const int32 FernCount = Rng.RandRange(2, 3);
		for (int32 FIdx = 0; FIdx < FernCount; ++FIdx)
		{
			const float FAngle = Yaw + FIdx * 125.f + Rng.FRandRange(-20.f, 20.f);
			const float FDist = Rng.FRandRange(95.f, 240.f);
			const float FX = X + FMath::Cos(FMath::DegreesToRadians(FAngle)) * FDist;
			const float FY = Y + FMath::Sin(FMath::DegreesToRadians(FAngle)) * FDist;
			const float FTerrainZ = SampleGroundZ(FX, FY, TerrainZ);
			const float FScale = FernUnit * Rng.FRandRange(0.85f, 1.55f);
			const float FZ = AetheriaWorldHelpers::ComputeGroundedZ(FernMesh, FTerrainZ, FScale, 4.f);
			FernTransforms.Emplace(
				FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f),
				FVector(FX, FY, FZ),
				FVector(FScale, FScale * Rng.FRandRange(0.85f, 1.15f), FScale));
		}
	}

	if (TreeTrunksISM && RuinTransforms.Num() > 0)
	{
		TreeTrunksISM->AddInstances(RuinTransforms, false, true, false);
	}
	if (TreeCanopiesISM && FernTransforms.Num() > 0)
	{
		TreeCanopiesISM->AddInstances(FernTransforms, false, true, false);
	}

	// 3 Storm Conductor Monoliths (LinkedQuestId = "Q_StormSpires") grounded cleanly onto stone pedestals
	UStaticMesh* MonoPedMesh    = LoadMeshSafe(AetheriaWorldConst::MeshStatuePedestal, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* MonoPillarMesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar, AetheriaWorldConst::MeshDungeonPillar);
	const float MonoPedScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(MonoPedMesh, 85.f, 0.1f, 3.0f);
	const FLinearColor WexStormGlow(0.68f, 0.38f, 1.35f, 1.0f);
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
		const float PedZ = AetheriaWorldHelpers::ComputeGroundedZ(MonoPedMesh, MZ, MonoPedScale, 6.f);

		AddWorldMesh(
			FString::Printf(TEXT("StormMonolith_Pedestal_%d"), Idx),
			AetheriaWorldConst::MeshStatuePedestal,
			FVector(MX, MY, PedZ),
			FRotator(0.f, Idx * 40.f, 0.f),
			FVector(MonoPedScale));

		const float PillarZ = AetheriaWorldHelpers::ComputeStackedZ(MonoPedMesh, PedZ, MonoPedScale, MonoPillarMesh, 1.65f, 4.f);
		SpawnDestructibleProp(
			FVector(MX, MY, PillarZ),
			FRotator(0.f, Idx * 55.f, 0.f),
			EDestructibleKind::Crystal,
			130.f,
			AetheriaWorldConst::MeshGothicPillar,
			FVector(1.25f, 1.25f, 1.65f),
			FName(TEXT("Q_StormSpires")),
			&WexStormGlow,
			true);

		AddAmbientFireLight(
			FString::Printf(TEXT("StormMonolith_Glow_%d"), Idx),
			FVector(MX, MY, MZ + 310.f),
			680.f,
			8000.f,
			1200.f,
			FLinearColor(0.62f, 0.42f, 1.0f, 1.0f),
			0.18f,
			Idx * 1.7f,
			true,
			0.14f);
	}
}

void AAetheriaWorldBuilder::BuildHighkeepCitadel()
{
	// Monumental fortress courtyard & boss arena centered at (0, -9800); Malakor stands at (0, -10400)
	const float CX = 0.f;
	const float CY = -9800.f;
	const float ArenaZ = SampleGroundZ(CX, CY, FallbackGroundZ);

	// Use SM6-verified M_BrickFloor_Clean_Inst & M_Stone_Altar_Inst (removed M_AetheriaSanctumFloor which failed PCD3D_SM6!)
	UMaterialInterface* BrickFloorMat = LoadPBRMaterial({
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* StoneAltarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* StoneWallMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone,
		AetheriaWorldConst::MatCryptWallA
	});
	UMaterialInterface* PillarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatPillarInst,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone
	});

	UStaticMesh* GateMesh       = LoadMeshSafe(AetheriaWorldConst::MeshGothicGate, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* PedestalMesh   = LoadMeshSafe(AetheriaWorldConst::MeshStatuePedestal, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* StatueMesh     = LoadMeshSafe(AetheriaWorldConst::MeshStatue01, AetheriaWorldConst::MeshGothicPillar);
	UStaticMesh* SolidArcMesh   = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* PillarMesh     = LoadMeshSafe(AetheriaWorldConst::MeshDungeonPillar, AetheriaWorldConst::MeshGothicPillar);
	UStaticMesh* BallistaMesh   = LoadMeshSafe(AetheriaWorldConst::MeshGothicBallista, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* StepsMesh      = LoadMeshSafe(AetheriaWorldConst::MeshGothicSteps, AetheriaWorldConst::MeshEdgeStones);
	UStaticMesh* Column3Mesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicColumn3, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* CitadelMesh    = LoadMeshSafe(AetheriaWorldConst::MeshCitadelFortress, AetheriaWorldConst::MeshGothicBuilding09);

	const float GateScale      = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GateMesh, 680.f, 0.2f, 3.5f);
	const float PedestalScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PedestalMesh, 165.f, 0.1f, 3.5f);
	const float StatueScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StatueMesh, 320.f, 0.1f, 3.5f);
	const float SolidArcScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SolidArcMesh, 560.f, 0.2f, 3.5f);
	const float PillarScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PillarMesh, 540.f, 0.2f, 3.5f);
	const float BallistaScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BallistaMesh, 185.f, 0.1f, 3.5f);
	const float StepsScale     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StepsMesh, 125.f, 0.1f, 3.5f);
	const float Column3Scale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Column3Mesh, 540.f, 0.2f, 3.5f);

	// 1. Dark Gothic Cobblestone Boss Arena floor (M_BrickFloor_Clean_Inst + M_Stone_Altar_Inst — zero smooth grey patch!)
	BuildTiledStoneFloor(
		TEXT("Highkeep_Floor"),
		CX,
		CY,
		1950.f,
		1950.f,
		650.f,
		4.f,
		AetheriaWorldConst::MeshDungeonFloor,
		BrickFloorMat,
		StoneAltarMat,
		700.f,
		true,
		ArenaZ);

	// 2. Grounded Gothic Fortress Keep behind Malakor (modest Z-scale <= 1.25f so it reads as a fortress keep, NOT a skyscraper!)
	const float KeepY = -12650.f;
	const float KeepGZ = SampleGroundZ(CX, KeepY, ArenaZ);
	const float KeepScaleXY = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CitadelMesh, 960.f, 0.25f, 1.35f);
	const FVector KeepScale(KeepScaleXY * 1.15f, KeepScaleXY * 1.15f, FMath::Min(KeepScaleXY, 1.25f));
	AddWorldMesh(
		TEXT("Highkeep_CitadelFortress"),
		AetheriaWorldConst::MeshCitadelFortress,
		AetheriaWorldHelpers::ComputeGroundedCenterLoc(CitadelMesh, CX, KeepY, KeepGZ, FRotator::ZeroRotator, KeepScale, 20.f),
		FRotator::ZeroRotator,
		KeepScale,
		StoneWallMat);

	// 3. Grand Northern Gothic Gate at Y = -8250 facing Mirror Lake
	const float GateY = -8250.f;
	const float GateGroundZ = SampleGroundZ(CX, GateY, ArenaZ) + 4.f;
	AddWorldMesh(
		TEXT("Highkeep_GothicGate"),
		AetheriaWorldConst::MeshGothicGate,
		FVector(CX, GateY, AetheriaWorldHelpers::ComputeGroundedZ(GateMesh, GateGroundZ, GateScale, 10.f)),
		FRotator(0.f, 0.f, 0.f),
		FVector(GateScale));

	// Approach Steps at North Gate & South Citadel Terrace
	const float NorthStepsZ = SampleGroundZ(CX, -8020.f, GateGroundZ) + 4.f;
	AddWorldMesh(
		TEXT("Highkeep_NorthSteps"),
		AetheriaWorldConst::MeshGothicSteps,
		FVector(CX, -8020.f, AetheriaWorldHelpers::ComputeGroundedZ(StepsMesh, NorthStepsZ, StepsScale, 8.f)),
		FRotator(0.f, 0.f, 0.f),
		FVector(StepsScale * 1.4f, StepsScale * 1.2f, StepsScale),
		StoneAltarMat);

	const float SouthStepsZ = SampleGroundZ(CX, CY - 1450.f, ArenaZ) + 4.f;
	AddWorldMesh(
		TEXT("Highkeep_SouthSteps"),
		AetheriaWorldConst::MeshGothicSteps,
		FVector(CX, CY - 1450.f, AetheriaWorldHelpers::ComputeGroundedZ(StepsMesh, SouthStepsZ, StepsScale, 8.f)),
		FRotator(0.f, 180.f, 0.f),
		FVector(StepsScale * 1.6f, StepsScale * 1.2f, StepsScale),
		StoneAltarMat);

	// Sentinel TA_Sample_Statue_01 monuments on pedestals flanking the north gate
	const float StatueCoordsX[2] = { CX - 540.f, CX + 540.f };
	const float StatueYaws[2]    = { 65.f, 115.f };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float SX = StatueCoordsX[Idx];
		const float SY = -8320.f;
		const float GroundZ = SampleGroundZ(SX, SY, ArenaZ) + 4.f;
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
		const float BGZ = SampleGroundZ(BX, BY, ArenaZ) + 4.f;
		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_Ballista_%d"), Idx),
			AetheriaWorldConst::MeshGothicBallista,
			FVector(BX, BY, AetheriaWorldHelpers::ComputeGroundedZ(BallistaMesh, BGZ, BallistaScale, 6.f)),
			FRotator(0.f, BallistaYaw[Idx], 0.f),
			FVector(BallistaScale));
	}

	// 4. Sculpted Full-3D Gothic Perimeter (SM_Arc & SM_Pillar — zero single-sided wall backfaces!)
	const int32 PerimeterSegments = 12;
	const float PerimeterRadius = 1780.f;
	for (int32 Idx = 0; Idx < PerimeterSegments; ++Idx)
	{
		// Leave north entrance open for SM_Gate (+90 deg) and south backdrop open for Citadel Keep (-90 / 270 deg)
		const float AngleDeg = Idx * (360.f / static_cast<float>(PerimeterSegments));
		if (FMath::IsNearlyEqual(AngleDeg, 90.f, 22.f) || FMath::IsNearlyEqual(AngleDeg, 270.f, 22.f))
		{
			continue;
		}
		const float Rad = FMath::DegreesToRadians(AngleDeg);
		const float WX = CX + FMath::Cos(Rad) * PerimeterRadius;
		const float WY = CY + FMath::Sin(Rad) * PerimeterRadius;
		const float WZ = SampleGroundZ(WX, WY, ArenaZ) + 4.f;
		const FRotator WallRot(0.f, AngleDeg + 90.f, 0.f);

		const bool bUseArc = (Idx % 2 == 0);
		UStaticMesh* SegMesh = bUseArc ? SolidArcMesh : PillarMesh;
		const TCHAR* SegPath = bUseArc ? AetheriaWorldConst::MeshGothicArc : AetheriaWorldConst::MeshDungeonPillar;
		const float SegScale = bUseArc ? SolidArcScale : PillarScale;
		UMaterialInterface* SegMat = bUseArc ? nullptr : PillarMat;

		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_PerimeterWall_%d"), Idx),
			SegPath,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(SegMesh, WX, WY, WZ, WallRot, FVector(SegScale), 10.f),
			WallRot,
			FVector(SegScale),
			SegMat);
	}

	// 5. 6 Perimeter Citadel War Braziers + 2 Heroic Flanking Boss Braziers framing Malakor (P_Torch_Fire + SpecularScale=0.14)
	for (int32 Idx = 0; Idx < 6; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 6.f) * UE_TWO_PI + 0.52f;
		const float BX = CX + FMath::Cos(Angle) * 1380.f;
		const float BY = CY + FMath::Sin(Angle) * 1380.f;
		const float BZ = SampleGroundZ(BX, BY, ArenaZ) + 4.f;

		AddBrazierAssembly(
			FString::Printf(TEXT("Highkeep_Brazier_%d"), Idx),
			FVector(BX, BY, BZ),
			860.f,
			1450.f,
			AetheriaWorldConst::MeshFirePit,
			1.20f,
			Idx * 1.05f);
	}

	const float MalakorFlankX[2] = { CX - 380.f, CX + 380.f };
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float BX = MalakorFlankX[Idx];
		const float BY = -10180.f;
		const float BZ = SampleGroundZ(BX, BY, ArenaZ) + 4.f;
		AddBrazierAssembly(
			FString::Printf(TEXT("Highkeep_MalakorFlankBrazier_%d"), Idx),
			FVector(BX, BY, BZ),
			780.f,
			1250.f,
			AetheriaWorldConst::MeshFirePit,
			1.05f,
			Idx * 1.6f + 0.3f);
	}

	// 6. 6 Chaos-Destructible Boss Arena Columns (SM_Column3) shattered during Malakor's attacks
	for (int32 Idx = 0; Idx < 6; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 6.f) * UE_TWO_PI;
		const float PX = CX + FMath::Cos(Angle) * 1020.f;
		const float PY = CY + FMath::Sin(Angle) * 1020.f;
		const float PZ = SampleGroundZ(PX, PY, ArenaZ) + 4.f;

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

	UMaterialInterface* BaseLavaShader = LoadPBRMaterial({
		AetheriaWorldConst::MatAetheriaFireball,
		AetheriaWorldConst::MatAetheriaMasterPBR
	});
	UMaterialInterface* BrickFloorMat = LoadPBRMaterial({
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* StoneAltarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* StoneCoffinMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneCoffin,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* CryptWallMat = LoadPBRMaterial({
		AetheriaWorldConst::MatCryptWallA,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* PillarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatPillarInst,
		AetheriaWorldConst::MatCryptWallA,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* FirePitMat = LoadPBRMaterial({
		AetheriaWorldConst::MatFirePitInst,
		AetheriaWorldConst::MatStoneWall
	});

	UStaticMesh* CryptWallMesh   = LoadMeshSafe(AetheriaWorldConst::MeshCryptWallA, AetheriaWorldConst::MeshDungeonWallA);
	UStaticMesh* SolidArcMesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshCryptPillar);
	UStaticMesh* CryptPillarMesh = LoadMeshSafe(AetheriaWorldConst::MeshCryptPillar, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* Pillar2Mesh     = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar2, AetheriaWorldConst::MeshCryptPillar);
	UStaticMesh* BowlMesh        = LoadMeshSafe(AetheriaWorldConst::MeshFirePit, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* VaultArchMesh   = LoadMeshSafe(AetheriaWorldConst::MeshCryptCeilingArch, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* AltarMesh       = LoadMeshSafe(AetheriaWorldConst::MeshMedievalAltar, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* CoffinMesh      = LoadMeshSafe(AetheriaWorldConst::MeshStoneCoffin, AetheriaWorldConst::MeshMedievalAltar);

	const float WallSecScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CryptWallMesh, 560.f, 0.2f, 3.5f);
	const float WallArcScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SolidArcMesh, 560.f, 0.2f, 3.5f);
	const float PillarScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CryptPillarMesh, 580.f, 0.2f, 3.5f);
	const float SconceP2Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Pillar2Mesh, 210.f, 0.1f, 3.2f);
	const float BowlScale     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BowlMesh, 32.f, 0.1f, 2.5f);
	const float VaultScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(VaultArchMesh, 420.f, 0.2f, 3.5f);
	const float AltarScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(AltarMesh, 128.f, 0.2f, 2.0f);
	const float CoffinScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CoffinMesh, 115.f, 0.2f, 2.5f);

	// 1. Calibrated Volcanic Molten Lava MID on M_AetheriaFireball (fixes EmissiveBoost=18 bright purple-white blowout!)
	LavaFloorMID = nullptr;
	UMaterialInterface* ActiveLavaMat = nullptr;
	if (BaseLavaShader)
	{
		if (UMaterialInstanceDynamic* EmberLavaMID = UMaterialInstanceDynamic::Create(BaseLavaShader, this))
		{
			EmberLavaMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.24f, 0.025f, 0.004f, 1.0f));
			EmberLavaMID->SetVectorParameterValue(TEXT("AccentColor"), FLinearColor(1.0f, 0.22f, 0.015f, 1.0f));
			EmberLavaMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(1.0f, 0.24f, 0.02f, 1.0f));
			EmberLavaMID->SetScalarParameterValue(TEXT("EmissiveBoost"), 2.4f);
			EmberLavaMID->SetScalarParameterValue(TEXT("EmissiveStrength"), 2.4f);
			EmberLavaMID->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
			DynamicMaterials.Add(EmberLavaMID);
			ActiveLavaMat = EmberLavaMID;
		}
	}
	if (!ActiveLavaMat)
	{
		LavaFloorMID = CreateColorMID(FLinearColor(1.25f, 0.24f, 0.03f, 1.0f), true);
		ActiveLavaMat = LavaFloorMID;
	}
	AddWorldMesh(
		TEXT("EmberCrypt_MoltenLavaFloor"),
		AetheriaWorldConst::MeshPlane,
		FVector(CX, CY, BaseZ + 4.f),
		FRotator::ZeroRotator,
		FVector(19.0f, 23.0f, 1.f),
		ActiveLavaMat,
		AetheriaWorldConst::MeshPlane,
		false);

	// 2. Elevated Ancient Stone Flagstone Causeways at BaseZ + 26.0f (M_BrickFloor_Clean_Inst & M_Stone_Altar_Inst — zero blue tile!)
	const float WalkwayTopZ = BaseZ + 26.f;
	BuildTiledStoneFloor(
		TEXT("EmberCrypt_Causeway"),
		CX,
		CY + 140.f,
		0.f,
		840.f,
		560.f,
		26.f,
		AetheriaWorldConst::MeshCryptFloor,
		BrickFloorMat,
		StoneAltarMat,
		280.f,
		false,
		BaseZ);

	// Wider elevated apse sanctuary platform beneath Sunstone Altar & flanking Stone Coffins (warm ancient stone, zero blue!)
	BuildTiledStoneFloor(
		TEXT("EmberCrypt_ApseDais"),
		CX,
		CY - 680.f,
		420.f,
		0.f,
		460.f,
		26.f,
		AetheriaWorldConst::MeshCryptFloor,
		StoneAltarMat,
		BrickFloorMat,
		220.f,
		false,
		BaseZ);

	// 3. Inward-Facing Gothic Crypt Perimeter Walls & Pilasters (sculpted faces pointed INWARD toward hall — zero flat backfaces!)
	const float HalfW = 860.f;
	const float HalfL = 1040.f;

	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		for (int32 Seg = -1; Seg <= 1; ++Seg)
		{
			const bool bUseArc = (Seg == 0);
			UStaticMesh* SegMesh = bUseArc ? SolidArcMesh : CryptWallMesh;
			const float SegScale = bUseArc ? WallArcScale : WallSecScale;
			const float WX = CX + Side * HalfW;
			const float WY = CY + Seg * 680.f;
			// SM_Crypt_Wall_Decorative_A local front normal is -Y: Yaw=-90 faces -X (inward from +X wall), Yaw=+90 faces +X (inward from -X wall)
			const FRotator WallRot(0.f, Side > 0 ? -90.f : 90.f, 0.f);

			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_SideWall_%d_%d"), Side > 0 ? 1 : 0, Seg + 1),
				bUseArc ? AetheriaWorldConst::MeshGothicArc : AetheriaWorldConst::MeshCryptWallA,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(SegMesh, WX, WY, BaseZ, WallRot, FVector(SegScale), 14.f),
				WallRot,
				FVector(SegScale),
				bUseArc ? nullptr : CryptWallMat);
		}

		// Corner & mid-bay SM_Crypt_Pillar pilasters
		for (int32 ColIdx = -1; ColIdx <= 1; ++ColIdx)
		{
			const float ColX = CX + Side * (HalfW - 45.f);
			const float ColY = CY + ColIdx * 920.f;
			const FRotator ColRot(0.f, ColIdx * 45.f, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_Pilaster_%d_%d"), Side > 0 ? 1 : 0, ColIdx + 1),
				AetheriaWorldConst::MeshCryptPillar,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(CryptPillarMesh, ColX, ColY, BaseZ, ColRot, FVector(PillarScale), 12.f),
				ColRot,
				FVector(PillarScale),
				PillarMat);
		}
	}

	// South rear apse wall (Yaw=180 rotates -Y front to +Y facing the player camera + solid 3D center SM_Arc!)
	for (int32 Seg = -1; Seg <= 1; ++Seg)
	{
		const bool bCenterArc = (Seg == 0);
		UStaticMesh* SegMesh = bCenterArc ? SolidArcMesh : CryptWallMesh;
		const float SegScale = bCenterArc ? WallArcScale : WallSecScale;
		const FRotator ApseRot(0.f, 180.f, 0.f);
		AddWorldMesh(
			FString::Printf(TEXT("EmberCrypt_SouthWall_%d"), Seg + 1),
			bCenterArc ? AetheriaWorldConst::MeshGothicArc : AetheriaWorldConst::MeshCryptWallA,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(SegMesh, CX + Seg * 560.f, CY - HalfL, BaseZ, ApseRot, FVector(SegScale), 14.f),
			ApseRot,
			FVector(SegScale),
			bCenterArc ? nullptr : CryptWallMat);
	}

	// North entrance flanking bastions + solid 3D entrance arch (Yaw=0 faces -Y inward into the hall)
	{
		const FRotator NorthRot(0.f, 0.f, 0.f);
		AddWorldMesh(
			TEXT("EmberCrypt_NorthBastionL"),
			AetheriaWorldConst::MeshCryptWallA,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(CryptWallMesh, CX - 590.f, CY + HalfL, BaseZ, NorthRot, FVector(WallSecScale), 14.f),
			NorthRot,
			FVector(WallSecScale),
			CryptWallMat);
		AddWorldMesh(
			TEXT("EmberCrypt_NorthBastionR"),
			AetheriaWorldConst::MeshCryptWallA,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(CryptWallMesh, CX + 590.f, CY + HalfL, BaseZ, NorthRot, FVector(WallSecScale), 14.f),
			NorthRot,
			FVector(WallSecScale),
			CryptWallMat);
		AddWorldMesh(
			TEXT("EmberCrypt_EntranceArch"),
			AetheriaWorldConst::MeshGothicArc,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(SolidArcMesh, CX, CY + HalfL, BaseZ, NorthRot, FVector(WallArcScale), 12.f),
			NorthRot,
			FVector(WallArcScale));
	}

	// 4. Real Gothic Crypt Vaulted Ceiling Ribs (non-colliding high overhead)
	const float VaultYs[4] = { CY - 680.f, CY - 220.f, CY + 240.f, CY + 700.f };
	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		AddWorldMesh(
			FString::Printf(TEXT("EmberCrypt_VaultArch_%d"), Idx),
			AetheriaWorldConst::MeshCryptCeilingArch,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(VaultArchMesh, CX, VaultYs[Idx], BaseZ + 430.f, FRotator::ZeroRotator, FVector(VaultScale * 1.4f, VaultScale, VaultScale), 0.f),
			FRotator::ZeroRotator,
			FVector(VaultScale * 1.4f, VaultScale, VaultScale),
			CryptWallMat,
			AetheriaWorldConst::MeshGothicArc,
			false);
	}

	// 5. 6 Grounded Wall Torch Sconces (SM_Pillar2 + SM_Fire_Pit + REAL P_Torch_Fire Cascade + calibrated 520 lm SpecularScale=0.14)
	for (int32 Row = -1; Row <= 1; ++Row)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const int32 TorchIdx = (Row + 1) * 2 + (Side > 0 ? 1 : 0);
			const float TX = CX + Side * (HalfW - 140.f);
			const float TY = CY + Row * 620.f;
			const FRotator PostRot(0.f, Side > 0 ? 180.f : 0.f, 0.f);
			const FVector PostLoc = AetheriaWorldHelpers::ComputeGroundedCenterLoc(Pillar2Mesh, TX, TY, BaseZ + 18.f, PostRot, FVector(SconceP2Scale), 6.f);
			const float BowlZ = AetheriaWorldHelpers::ComputeStackedZ(Pillar2Mesh, PostLoc.Z, SconceP2Scale, BowlMesh, BowlScale, 4.f);
			const FBox BowlBox = AetheriaWorldHelpers::GetSafeBoundingBox(BowlMesh);
			const FVector BowlCenter = BowlBox.GetCenter();
			const FVector BowlLoc(TX - BowlCenter.X * BowlScale, TY - BowlCenter.Y * BowlScale, BowlZ);
			const float FireTopZ = BowlZ + BowlBox.Max.Z * BowlScale - 4.f;

			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_TorchPost_%d"), TorchIdx),
				AetheriaWorldConst::MeshGothicPillar2,
				PostLoc,
				PostRot,
				FVector(SconceP2Scale),
				PillarMat);

			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_TorchBowl_%d"), TorchIdx),
				AetheriaWorldConst::MeshFirePit,
				BowlLoc,
				FRotator::ZeroRotator,
				FVector(BowlScale),
				FirePitMat);

			SpawnFireParticle(
				FString::Printf(TEXT("EmberCrypt_TorchFireVFX_%d"), TorchIdx),
				AetheriaWorldConst::ParticleTorchFire,
				FVector(TX, TY, FireTopZ),
				FVector(0.80f));

			AddAmbientFireLight(
				FString::Printf(TEXT("EmberCrypt_WallTorchLight_%d"), TorchIdx),
				FVector(TX - Side * 35.f, TY, FireTopZ + 48.f),
				520.f,
				2100.f,
				1050.f,
				FLinearColor(1.0f, 0.48f, 0.14f, 1.0f),
				0.18f,
				TorchIdx * 1.1f,
				false,
				0.14f);
		}
	}

	// 6. Moody Volcanic Lava Bounce Illumination (750 lm, SpecularScale=0.12 for rich dark stone contrast)
	AddAmbientFireLight(
		TEXT("EmberCrypt_LavaBounce"),
		FVector(CX, CY, BaseZ + 115.f),
		750.f,
		1900.f,
		1650.f,
		FLinearColor(1.0f, 0.30f, 0.06f, 1.0f),
		0.12f,
		0.5f,
		false,
		0.12f);

	// 7. 1 Destructible Obsidian Seal Wall blocking the inner sanctum doorway (LinkedQuestId = "Q_EmberCrypt")
	SpawnDestructibleProp(
		FVector(CX, CY + HalfL - 40.f, AetheriaWorldHelpers::ComputeGroundedZ(CryptWallMesh, BaseZ, WallSecScale * 0.85f, 12.f)),
		FRotator(0.f, 0.f, 0.f),
		EDestructibleKind::Obsidian,
		160.f,
		AetheriaWorldConst::MeshCryptWallA,
		FVector(WallSecScale * 0.85f),
		FName(TEXT("Q_EmberCrypt")));

	// 8. Waist-High Carved Sunstone Altar (SM_Alter + M_Stone_Altar_Inst), Volcanic Sunstone Core & M_Stone_Coffin_Inst Sarcophagi
	const FVector AltarLoc = AetheriaWorldHelpers::ComputeGroundedCenterLoc(
		AltarMesh, CX, CY - 740.f, WalkwayTopZ, FRotator::ZeroRotator, FVector(AltarScale), 4.f);
	AddWorldMesh(
		TEXT("EmberCrypt_SunstoneAltar"),
		AetheriaWorldConst::MeshMedievalAltar,
		AltarLoc,
		FRotator::ZeroRotator,
		FVector(AltarScale),
		StoneAltarMat);

	UStaticMesh* SphereMesh = LoadMeshSafe(AetheriaWorldConst::MeshSphere, AetheriaWorldConst::MeshSphere);
	const float CoreScale = 0.68f;
	const float CoreZ = AetheriaWorldHelpers::ComputeStackedZ(AltarMesh, AltarLoc.Z, AltarScale, SphereMesh, CoreScale, -18.f);
	const FVector CoreWorldPos(CX, CY - 740.f, CoreZ);

	AddWorldMesh(
		TEXT("EmberCrypt_SunstoneCore"),
		AetheriaWorldConst::MeshSphere,
		CoreWorldPos,
		FRotator::ZeroRotator,
		FVector(CoreScale),
		ActiveLavaMat,
		AetheriaWorldConst::MeshSphere,
		false);

	SpawnFireParticle(
		TEXT("EmberCrypt_SunstoneEmbersVFX"),
		AetheriaWorldConst::ParticlePitFire,
		CoreWorldPos - FVector(0.f, 0.f, 18.f),
		FVector(0.85f));

	AddAmbientFireLight(
		TEXT("EmberCrypt_SunstoneGlow"),
		CoreWorldPos + FVector(0.f, 0.f, 40.f),
		650.f,
		2600.f,
		1100.f,
		FLinearColor(1.0f, 0.52f, 0.16f, 1.0f),
		0.10f,
		2.7f,
		false,
		0.14f);

	const FRotator CoffinRotL(0.f, 20.f, 0.f);
	const FRotator CoffinRotR(0.f, -20.f, 0.f);
	AddWorldMesh(
		TEXT("EmberCrypt_SarcophagusL"),
		AetheriaWorldConst::MeshStoneCoffin,
		AetheriaWorldHelpers::ComputeGroundedCenterLoc(CoffinMesh, CX - 430.f, CY - 580.f, WalkwayTopZ, CoffinRotL, FVector(CoffinScale), 4.f),
		CoffinRotL,
		FVector(CoffinScale),
		StoneCoffinMat);
	AddWorldMesh(
		TEXT("EmberCrypt_SarcophagusR"),
		AetheriaWorldConst::MeshStoneCoffin,
		AetheriaWorldHelpers::ComputeGroundedCenterLoc(CoffinMesh, CX + 430.f, CY - 580.f, WalkwayTopZ, CoffinRotR, FVector(CoffinScale), 4.f),
		CoffinRotR,
		FVector(CoffinScale),
		StoneCoffinMat);
}

void AAetheriaWorldBuilder::BuildCrystalGlacierPeaks()
{
	// Frost crystalline spires & warming sanctuary braziers at (-6800, -6400)
	const float CX = -6800.f;
	const float CY = -6400.f;
	const float CenterZ = SampleGroundZ(CX, CY, FallbackGroundZ);

	UMaterialInterface* PrismCrystalMat = LoadPBRMaterial({
		AetheriaWorldConst::MatAetheriaPrismWall
	});
	UMaterialInterface* GlacierStoneMat = LoadPBRMaterial({
		AetheriaWorldConst::MatNamaqualandCliff,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall
	});

	if (!PrismCrystalMat)
	{
		PrismCrystalMat = CreateColorMID(FLinearColor(0.28f, 0.85f, 1.45f, 1.0f), true);
	}

	// Grounded Central Quas Frost Sanctuary Shrine (SM_Mausoleum + textured stone)
	UStaticMesh* ShrineMesh  = LoadMeshSafe(AetheriaWorldConst::MeshMausoleum, AetheriaWorldConst::MeshMedievalAltar);
	UStaticMesh* SpireMesh   = LoadMeshSafe(AetheriaWorldConst::MeshCoreCrystal, AetheriaWorldConst::MeshEarthSpikeSingle);
	UStaticMesh* RockMesh    = LoadMeshSafe(AetheriaWorldConst::MeshKiteMountainRock, AetheriaWorldConst::MeshCliffBoulder);

	const float ShrineScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ShrineMesh, 420.f, 0.2f, 3.0f);
	const FRotator ShrineRot(0.f, 35.f, 0.f);
	AddWorldMesh(
		TEXT("Glacier_SanctuaryShrine"),
		AetheriaWorldConst::MeshMausoleum,
		AetheriaWorldHelpers::ComputeGroundedCenterLoc(ShrineMesh, CX, CY, CenterZ, ShrineRot, FVector(ShrineScale), 10.f),
		ShrineRot,
		FVector(ShrineScale),
		GlacierStoneMat);

	// 12 Grounded Faceted Frost Crystalline Spires encircling the glacier basin (M_AetheriaPrismWall + ComputeGroundedZ)
	FRandomStream GlacierRng(99107);
	const float SpireBaseUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SpireMesh, 420.f, 0.2f, 3.2f);
	const float BoulderBaseUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(RockMesh, 220.f, 0.15f, 3.0f);

	for (int32 Idx = 0; Idx < 12; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 12.f) * UE_TWO_PI + GlacierRng.FRandRange(-0.15f, 0.15f);
		const float Dist = GlacierRng.FRandRange(620.f, 1650.f);
		const float SX = CX + FMath::Cos(Angle) * Dist;
		const float SY = CY + FMath::Sin(Angle) * Dist;
		const float SZ = SampleGroundZ(SX, SY, CenterZ);
		const float HeightMult = GlacierRng.FRandRange(0.85f, 1.75f);
		const float WidthMult = GlacierRng.FRandRange(0.75f, 1.25f);
		const FVector SpireScale(SpireBaseUnit * WidthMult, SpireBaseUnit * WidthMult, SpireBaseUnit * HeightMult);
		const FRotator SpireRot(GlacierRng.FRandRange(-8.f, 8.f), GlacierRng.FRandRange(0.f, 360.f), GlacierRng.FRandRange(-8.f, 8.f));

		AddWorldMesh(
			FString::Printf(TEXT("Glacier_FrostSpire_%d"), Idx),
			AetheriaWorldConst::MeshCoreCrystal,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(SpireMesh, SX, SY, SZ, SpireRot, SpireScale, 24.f),
			SpireRot,
			SpireScale,
			PrismCrystalMat,
			AetheriaWorldConst::MeshEarthSpikeSingle,
			true);

		if (Idx % 3 == 0)
		{
			const float BX = SX + 140.f;
			const float BY = SY - 110.f;
			const float BGZ = SampleGroundZ(BX, BY, SZ);
			const float BScale = BoulderBaseUnit * GlacierRng.FRandRange(0.95f, 1.55f);
			const FRotator BRot(0.f, GlacierRng.FRandRange(0.f, 360.f), 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("Glacier_IceBoulder_%d"), Idx),
				AetheriaWorldConst::MeshKiteMountainRock,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(RockMesh, BX, BY, BGZ, BRot, FVector(BScale), 18.f),
				BRot,
				FVector(BScale),
				GlacierStoneMat);
		}
	}

	// 3 Warming Sanctuary Braziers (SM_Fire_Pit + P_Torch_Fire + 850 lm warm contrast against glacier ice)
	for (int32 Idx = 0; Idx < 3; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 3.f) * UE_TWO_PI + 0.45f;
		const float FX = CX + FMath::Cos(Angle) * 480.f;
		const float FY = CY + FMath::Sin(Angle) * 480.f;
		const float FZ = SampleGroundZ(FX, FY, CenterZ);

		AddBrazierAssembly(
			FString::Printf(TEXT("Glacier_FirePit_%d"), Idx),
			FVector(FX, FY, FZ),
			850.f,
			1350.f,
			AetheriaWorldConst::MeshFirePit,
			1.15f,
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
		bool bUseFireShader;
	};

	const FSkyRingSpec Rings[3] = {
		{ FVector2D(-1800.f,  -400.f), 3200.f,  -28.f, FLinearColor(0.25f, 1.45f, 2.10f, 1.0f), false },
		{ FVector2D(    0.f, -2600.f), 3600.f,    0.f, FLinearColor(1.90f, 1.05f, 0.22f, 1.0f), true  },
		{ FVector2D(    0.f, -6600.f), 4000.f,    0.f, FLinearColor(1.15f, 0.42f, 2.05f, 1.0f), false }
	};

	UMaterialInterface* PrismMat = LoadPBRMaterial({ AetheriaWorldConst::MatAetheriaPrismWall });
	UMaterialInterface* FireMat  = nullptr;
	if (UMaterialInterface* BaseFire = LoadPBRMaterial({ AetheriaWorldConst::MatAetheriaFireball, AetheriaWorldConst::MatAetheriaMasterPBR }))
	{
		if (UMaterialInstanceDynamic* RingFireMID = UMaterialInstanceDynamic::Create(BaseFire, this))
		{
			RingFireMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.32f, 0.08f, 0.01f, 1.0f));
			RingFireMID->SetVectorParameterValue(TEXT("AccentColor"), FLinearColor(1.0f, 0.35f, 0.04f, 1.0f));
			RingFireMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(1.0f, 0.40f, 0.05f, 1.0f));
			RingFireMID->SetScalarParameterValue(TEXT("EmissiveBoost"), 2.4f);
			RingFireMID->SetScalarParameterValue(TEXT("EmissiveStrength"), 2.4f);
			DynamicMaterials.Add(RingFireMID);
			FireMat = RingFireMID;
		}
	}

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

		UMaterialInterface* RingMat = Spec.bUseFireShader ? FireMat : PrismMat;
		if (!RingMat)
		{
			RingMat = CreateColorMID(Spec.Color, true);
		}

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
				RingMat,
				AetheriaWorldConst::MeshCube,
				false,
				Pivot);
		}

		// Aerial guidance beacon light
		AddAmbientFireLight(
			FString::Printf(TEXT("SkyRuneRing_Beacon_%d"), RingIdx),
			RingCenter,
			1600.f,
			6500.f,
			2400.f,
			FLinearColor(
				FMath::Clamp(Spec.Color.R, 0.15f, 1.0f),
				FMath::Clamp(Spec.Color.G, 0.15f, 1.0f),
				FMath::Clamp(Spec.Color.B, 0.15f, 1.0f),
				1.0f),
			0.10f,
			RingIdx * 2.1f,
			false,
			0.16f);
	}
}
