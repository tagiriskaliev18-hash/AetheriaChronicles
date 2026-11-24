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

#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
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
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundAttenuation.h"
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

	// Verified AAA Landscape & Nature Cover Materials
	static constexpr TCHAR MatLandscapeMountain[]   = TEXT("/Game/MWLandscapeAutoMaterial/Materials/Landscape/MTL_MWAM_Landscape_MountainRangeExample.MTL_MWAM_Landscape_MountainRangeExample");
	static constexpr TCHAR MatLandscapeIsland[]     = TEXT("/Game/MWLandscapeAutoMaterial/Materials/Landscape/MTL_MWAM_Landscape_IslandExample.MTL_MWAM_Landscape_IslandExample");
	static constexpr TCHAR MatMWAMCoverRocks[]      = TEXT("/Game/MWLandscapeAutoMaterial/Materials/Cover/MTL_MWAM_CoverRocks.MTL_MWAM_CoverRocks");
	static constexpr TCHAR MatMWAMPlantsGrass[]     = TEXT("/Game/MWLandscapeAutoMaterial/Materials/Plants/MTL_MWAM_PlantsGrass.MTL_MWAM_PlantsGrass");

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
	static constexpr TCHAR MeshCryptDoorway[]       = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Crypt_Doorway.SM_Crypt_Doorway");
	static constexpr TCHAR MeshCryptCeilingArch[]   = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Crypt_Ceiling_Arch_A.SM_Crypt_Ceiling_Arch_A");
	static constexpr TCHAR MeshCryptCeilingFlat[]   = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Crypt_Ceiling_Flat.SM_Crypt_Ceiling_Flat");
	static constexpr TCHAR MeshMausoleum[]          = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Crypt/SM_Mausoleum.SM_Mausoleum");
	static constexpr TCHAR MeshMedievalAltar[]      = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Alter.SM_Alter");
	static constexpr TCHAR MeshStoneCoffin[]        = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Stone_Coffin.SM_Stone_Coffin");
	static constexpr TCHAR MeshMedievalPot[]        = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Pot_A_Complete.SM_Pot_A_Complete");
	static constexpr TCHAR MeshDirtMoundA[]         = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Dirt_Mound_A.SM_Dirt_Mound_A");
	static constexpr TCHAR MeshGothicBuilding09[]   = TEXT("/Game/Gothic_Environment/Meshes/Gothic_buildings/SM_Gothic_building_09.SM_Gothic_building_09");

	// Verified Closed 360-Degree Megascans-Style Boulders & Wind-Swaying Foliage (ZERO KiteDemo cliff slabs / SM_DeadTree!)
	static constexpr TCHAR MeshMWAMStoneA[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneA.SM_MWAM_StoneA");
	static constexpr TCHAR MeshMWAMStoneB[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneB.SM_MWAM_StoneB");
	static constexpr TCHAR MeshMWAMStoneC[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneC.SM_MWAM_StoneC");
	static constexpr TCHAR MeshMWAMStoneD[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneD.SM_MWAM_StoneD");
	static constexpr TCHAR MeshMWAMGrassA[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Plants/SM_MWAM_GrassA.SM_MWAM_GrassA");
	static constexpr TCHAR MeshMWAMGrassB[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Plants/SM_MWAM_GrassB.SM_MWAM_GrassB");
	static constexpr TCHAR MeshMWAMGrassC[]         = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Plants/SM_MWAM_GrassC.SM_MWAM_GrassC");
	static constexpr TCHAR MeshFlowerHeliophila[]   = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_flower_heliophila_large_LP.SM_flower_heliophila_large_LP");
	static constexpr TCHAR MeshFlowerGazania[]      = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_flower_gazania_a_LP.SM_flower_gazania_a_LP");
	static constexpr TCHAR MeshWildRooibosBush[]    = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_wild_rooibos_bush_a_LP.SM_wild_rooibos_bush_a_LP");
	static constexpr TCHAR MeshSearsiaLarge[]       = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_searsia_burchellii_large_LP.SM_searsia_burchellii_large_LP");
	static constexpr TCHAR MeshSearsiaMedium[]      = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_searsia_burchellii_medium_LP.SM_searsia_burchellii_medium_LP");
	static constexpr TCHAR MeshKiteFern01[]         = TEXT("/Game/KiteDemo/Environments/Foliage/Ferns/SM_Fern_01.SM_Fern_01");

	// Verified Namaqualand Micro-Detail: Fallen Twigs/Branches, Bark Curls, Pebbles & Biome Flora
	static constexpr TCHAR MeshTwigBranch01[]       = TEXT("/Game/Namaqualand/Meshes/General/LP/SM_dead_quiver_branch_01_LP.SM_dead_quiver_branch_01_LP");
	static constexpr TCHAR MeshTwigBranch02[]       = TEXT("/Game/Namaqualand/Meshes/General/LP/SM_dead_quiver_branch_02_LP.SM_dead_quiver_branch_02_LP");
	static constexpr TCHAR MeshBarkDebrisA[]        = TEXT("/Game/Namaqualand/Meshes/General/LP/SM_bark_debris_01_a_LP.SM_bark_debris_01_a_LP");
	static constexpr TCHAR MeshBarkDebrisB[]        = TEXT("/Game/Namaqualand/Meshes/General/LP/SM_bark_debris_01_b_LP.SM_bark_debris_01_b_LP");
	static constexpr TCHAR MeshDryLeafA[]           = TEXT("/Game/Namaqualand/Meshes/General/LP/SM_dry_quiver_leaf_a_LP.SM_dry_quiver_leaf_a_LP");
	static constexpr TCHAR MeshPebbleStonesA[]      = TEXT("/Game/Namaqualand/Meshes/General/LP/SM_namaqualand_stones_01_a_LP.SM_namaqualand_stones_01_a_LP");
	static constexpr TCHAR MeshPebbleStonesB[]      = TEXT("/Game/Namaqualand/Meshes/General/LP/SM_namaqualand_stones_01_b_LP.SM_namaqualand_stones_01_b_LP");
	static constexpr TCHAR MeshNamaqualandBoulderA[]= TEXT("/Game/Namaqualand/Meshes/General/LP/SM_namaqualand_boulders_01_a_LP.SM_namaqualand_boulders_01_a_LP");
	static constexpr TCHAR MeshNamaqualandBoulderB[]= TEXT("/Game/Namaqualand/Meshes/General/LP/SM_namaqualand_boulders_01_b_LP.SM_namaqualand_boulders_01_b_LP");
	static constexpr TCHAR MeshNamaqualandBoulder02[]= TEXT("/Game/Namaqualand/Meshes/General/LP/SM_namaqualand_boulder_02_LP.SM_namaqualand_boulder_02_LP");
	static constexpr TCHAR MeshNamaqualandBoulder03[]= TEXT("/Game/Namaqualand/Meshes/General/LP/SM_namaqualand_boulder_03_LP.SM_namaqualand_boulder_03_LP");
	static constexpr TCHAR MeshNamaqualandBoulder04[]= TEXT("/Game/Namaqualand/Meshes/General/LP/SM_namaqualand_boulder_04_LP.SM_namaqualand_boulder_04_LP");
	static constexpr TCHAR MeshFlowerUrsinia[]      = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_flower_ursinia_a_LP.SM_flower_ursinia_a_LP");
	static constexpr TCHAR MeshFlowerEmpodium[]     = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_flower_empodium_a_LP.SM_flower_empodium_a_LP");
	static constexpr TCHAR MeshCrystalIceplant[]    = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_crystalline_iceplant_a_LP.SM_crystalline_iceplant_a_LP");
	static constexpr TCHAR MeshSucculentRock[]      = TEXT("/Game/Namaqualand/Meshes/Foliage/LP/SM_cheiridopsis_succulent_a_LP.SM_cheiridopsis_succulent_a_LP");

	// Verified Cascade Fire & Waterfall VFX + Niagara Wind/Leaf/Petal/Restoration VFX + Ambient SFX
	static constexpr TCHAR ParticleTorchFire[]      = TEXT("/Game/MedievalDungeon/Particles/P_Torch_Fire.P_Torch_Fire");
	static constexpr TCHAR ParticlePitFire[]        = TEXT("/Game/MedievalDungeon/Particles/P_Pit_Fire.P_Pit_Fire");
	static constexpr TCHAR ParticleAncientFire[]    = TEXT("/Game/AncientTreasures/VFX/P_Fire_01a.P_Fire_01a");
	static constexpr TCHAR ParticleAquaStorm[]      = TEXT("/Game/FXVarietyPack/Particles/P_ky_aquaStorm.P_ky_aquaStorm");
	static constexpr TCHAR ParticleWaterBall[]      = TEXT("/Game/FXVarietyPack/Particles/P_ky_waterBall.P_ky_waterBall");
	static constexpr TCHAR ParticleHealAura[]       = TEXT("/Game/FXVarietyPack/Particles/P_ky_healAura.P_ky_healAura");
	static constexpr TCHAR ParticleMagicCircle[]    = TEXT("/Game/FXVarietyPack/Particles/P_ky_magicCircle1.P_ky_magicCircle1");
	static constexpr TCHAR NSWindFlows01[]          = TEXT("/Game/Vefects/Wind_VFX/VFX/Particles/NS_Wind_Flows_01.NS_Wind_Flows_01");
	static constexpr TCHAR NSWindPetals[]           = TEXT("/Game/Vefects/Wind_VFX/VFX/Particles/NS_Wind_Debris_Flower_Petals_01_Colored.NS_Wind_Debris_Flower_Petals_01_Colored");
	static constexpr TCHAR NSWindTreeLeavesExtreme[]= TEXT("/Game/Vefects/Wind_VFX/VFX/Particles/NS_Wind_Debris_Tree_Leaves_01_Colored_Extreme.NS_Wind_Debris_Tree_Leaves_01_Colored_Extreme");
	static constexpr TCHAR NSZapBlue[]              = TEXT("/Game/Vefects/Zap_VFX/VFX/Zap/Particles/NS_Zap_02_Blue.NS_Zap_02_Blue");
	static constexpr TCHAR NSMagicRestoreHeal[]     = TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing.NS_Free_Spells_Buff_Healing");
	static constexpr TCHAR NSMagicRestoreCircle[]   = TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Circle.NS_Free_Spells_Circle");
	static constexpr TCHAR SFXMagicalRestore[]      = TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_Buff1_1.SW_MagicCast_Buff1_1");
	static constexpr TCHAR SFXWaterfallLoop[]       = TEXT("/Game/Audio/SFX_Amb_Brazier_Loop.SFX_Amb_Brazier_Loop");
	static constexpr TCHAR SFXWoodBreak[]           = TEXT("/Game/Audio/SFX_Barrel_Break.SFX_Barrel_Break");

	// Verified Gothic_props_1 PBR Meshes
	static constexpr TCHAR MeshGothicSection1[]     = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Section1.SM_Section1");
	static constexpr TCHAR MeshGothicSection2[]     = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Section2.SM_Section2");
	static constexpr TCHAR MeshGothicBorder1[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Border1.SM_Border1");
	static constexpr TCHAR MeshGothicBlock1[]       = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Block1.SM_Block1");
	static constexpr TCHAR MeshGothicBlock2[]       = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Block2.SM_Block2");
	static constexpr TCHAR MeshGothicArc[]          = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Arc.SM_Arc");
	static constexpr TCHAR MeshGothicBridge1[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Bridge1.SM_Bridge1");
	static constexpr TCHAR MeshGothicBridge2[]      = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Bridge2.SM_Bridge2");
	static constexpr TCHAR MeshGothicHandrails1[]   = TEXT("/Game/Gothic_props_1/Static_meshes/SM_Handrails1.SM_Handrails1");
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
	static constexpr TCHAR MeshCitadelFortress[]    = TEXT("/Game/Meshes/SM_CitadelFortress.SM_CitadelFortress");
	static constexpr TCHAR MeshGothicArchSpan[]     = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Decorative_Arches.SM_Decorative_Arches");
	static constexpr TCHAR MeshGothicPillar[]       = TEXT("/Game/Meshes/SM_GothicPillar.SM_GothicPillar");
	static constexpr TCHAR MeshRuinedArch[]         = TEXT("/Game/Meshes/SM_RuinedArch.SM_RuinedArch");
	static constexpr TCHAR MeshDungeonDais[]        = TEXT("/Game/TA_Sample_Statue/Models/SM_TA_Sample_Pedestal_01.SM_TA_Sample_Pedestal_01");
	static constexpr TCHAR MeshDungeonSlab[]        = TEXT("/Game/MedievalDungeon/Meshes/Architecture/Dungeon/SM_Floor.SM_Floor");
	static constexpr TCHAR MeshGothicBarrel[]       = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Pot_A_Complete.SM_Pot_A_Complete");
	static constexpr TCHAR MeshTreasureChest[]      = TEXT("/Game/Meshes/SM_TreasureChest.SM_TreasureChest");
	static constexpr TCHAR MeshCryptSarcophagus[]   = TEXT("/Game/MedievalDungeon/Meshes/Props/SM_Stone_Coffin.SM_Stone_Coffin");
	static constexpr TCHAR MeshCliffBoulder[]       = TEXT("/Game/MWLandscapeAutoMaterial/Meshes/Cover/SM_MWAM_StoneA.SM_MWAM_StoneA");
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

void ADestructiblePropActor::BeginPlay()
{
	Super::BeginPlay();
	CaptureInitialTransformIfNeeded();
}

void ADestructiblePropActor::CaptureInitialTransformIfNeeded()
{
	if (!bInitialTransformCaptured)
	{
		InitialActorLocation = GetActorLocation();
		InitialActorRotation = GetActorRotation();
		if (IntactMesh)
		{
			InitialMeshScale = IntactMesh->GetRelativeScale3D();
			InitialMeshRelRotation = IntactMesh->GetRelativeRotation();
		}
		bInitialTransformCaptured = true;
	}
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
	InitialActorLocation = GetActorLocation();
	InitialActorRotation = GetActorRotation();
	InitialMeshScale = Scale;
	InitialMeshRelRotation = FRotator::ZeroRotator;
	bInitialTransformCaptured = true;

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
		IntactMesh->SetCastShadow(true);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		IntactMesh->SetCollisionResponseToAllChannels(ECR_Block);
		IntactMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		IntactMesh->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);

		if (bIsSliceableTree)
		{
			// Preserve photoreal Namaqualand bark & foliage materials and NEVER attach a rune crystal on sliceable meadow trees
			return;
		}

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
					IntactMesh->bDisallowNanite = true;
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
	if (bIsSliceableTree)
	{
		return;
	}

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
				IntactMesh->bDisallowNanite = true;
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

	CaptureInitialTransformIfNeeded();
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

	CaptureInitialTransformIfNeeded();

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
	else if (!bBroken && bShatterEffectsSpawned)
	{
		TriggerMagicalRestoration();
	}
}

void ADestructiblePropActor::CleanupDebrisChunks()
{
	for (TObjectPtr<UStaticMeshComponent>& Chunk : DebrisChunks)
	{
		if (IsValid(Chunk))
		{
			Chunk->SetSimulatePhysics(false);
			Chunk->DestroyComponent();
		}
	}
	DebrisChunks.Empty();
}

void ADestructiblePropActor::TriggerMagicalRestoration()
{
	CaptureInitialTransformIfNeeded();
	CleanupDebrisChunks();

	if (IsValid(TreeStumpMesh))
	{
		TreeStumpMesh->SetVisibility(false, false);
		TreeStumpMesh->DestroyComponent();
		TreeStumpMesh = nullptr;
	}

	bBroken = false;
	bShatterEffectsSpawned = false;
	bTreeToppling = false;
	bAwaitingMagicalRestore = false;
	MagicalRestoreRemaining = 0.f;
	DebrisCleanupRemaining = 0.f;
	HP = MaxHP;

	SetActorLocationAndRotation(InitialActorLocation, InitialActorRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (IntactMesh)
	{
		IntactMesh->SetWorldRotation(InitialActorRotation);
		IntactMesh->SetRelativeRotation(InitialMeshRelRotation);
		IntactMesh->SetRelativeScale3D(InitialMeshScale * 0.22f);
		IntactMesh->SetVisibility(true, false);
		IntactMesh->SetCastShadow(true);
		IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		IntactMesh->SetCollisionResponseToAllChannels(ECR_Block);
		IntactMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		IntactMesh->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);
	}

	if (!bIsSliceableTree)
	{
		if (RuneAccentMesh)
		{
			RuneAccentMesh->SetVisibility(true, false);
		}
		if (RuneAccentLight)
		{
			RuneAccentLight->SetIntensity(280.f);
			RuneAccentLight->SetVisibility(true);
		}
	}

	bMagicalRegrowthActive = true;
	MagicalRegrowthElapsed = 0.f;
	MagicalRegrowthDuration = 1.15f;

	const FVector RestoreOrigin = InitialActorLocation + FVector(0.f, 0.f, 32.f);

	// Emerald-gold magical restoration flash light
	if (!IsValid(ShatterFlashLight))
	{
		const FName LightName = MakeUniqueObjectName(this, UPointLightComponent::StaticClass(), TEXT("RestoreFlashLight"));
		ShatterFlashLight = NewObject<UPointLightComponent>(this, LightName);
		if (ShatterFlashLight)
		{
			ShatterFlashLight->SetUsingAbsoluteLocation(true);
			ShatterFlashLight->SetMobility(EComponentMobility::Movable);
			ShatterFlashLight->SetIntensityUnits(ELightUnits::Lumens);
			ShatterFlashLight->SetAttenuationRadius(1100.f);
			ShatterFlashLight->SetSourceRadius(28.f);
			ShatterFlashLight->SetCastShadows(false);
			ShatterFlashLight->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
			ShatterFlashLight->RegisterComponent();
		}
	}
	if (ShatterFlashLight)
	{
		FlashPeakLumens = 9200.f;
		FlashDuration = 0.85f;
		FlashElapsed = 0.f;
		bFlashActive = true;
		ShatterFlashLight->SetWorldLocation(RestoreOrigin + FVector(0.f, 0.f, 36.f));
		ShatterFlashLight->SetLightColor(FLinearColor(0.28f, 1.0f, 0.62f, 1.0f));
		ShatterFlashLight->SetIntensity(FlashPeakLumens);
		ShatterFlashLight->SetVisibility(true);
	}

	if (!IsRunningDedicatedServer() && GetWorld())
	{
		UWorld* World = GetWorld();
		const FString HealPkg = FPackageName::ObjectPathToPackageName(FString(AetheriaWorldConst::NSMagicRestoreHeal));
		if (!HealPkg.IsEmpty() && FPackageName::DoesPackageExist(HealPkg))
		{
			if (UNiagaraSystem* HealNS = LoadObject<UNiagaraSystem>(nullptr, AetheriaWorldConst::NSMagicRestoreHeal))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HealNS, RestoreOrigin, FRotator::ZeroRotator, FVector(1.35f), true, true);
			}
		}

		const TCHAR* SecondaryNSPath = bIsSliceableTree
			? AetheriaWorldConst::NSWindPetals
			: AetheriaWorldConst::NSMagicRestoreCircle;
		const FString SecPkg = FPackageName::ObjectPathToPackageName(FString(SecondaryNSPath));
		if (!SecPkg.IsEmpty() && FPackageName::DoesPackageExist(SecPkg))
		{
			if (UNiagaraSystem* SecNS = LoadObject<UNiagaraSystem>(nullptr, SecondaryNSPath))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SecNS, RestoreOrigin, FRotator::ZeroRotator, FVector(1.15f), true, true);
			}
		}

		const FString AuraPkg = FPackageName::ObjectPathToPackageName(FString(AetheriaWorldConst::ParticleHealAura));
		if (!AuraPkg.IsEmpty() && FPackageName::DoesPackageExist(AuraPkg))
		{
			if (UParticleSystem* HealAuraPS = LoadObject<UParticleSystem>(nullptr, AetheriaWorldConst::ParticleHealAura))
			{
				UGameplayStatics::SpawnEmitterAtLocation(World, HealAuraPS, RestoreOrigin, FRotator::ZeroRotator, FVector(1.25f), true);
			}
		}

		// Play subtle magical restoration chime only if local player is nearby
		if (APlayerController* LocalPC = World->GetFirstPlayerController())
		{
			FVector ViewLoc = FVector::ZeroVector;
			FRotator ViewRot = FRotator::ZeroRotator;
			LocalPC->GetPlayerViewPoint(ViewLoc, ViewRot);
			if (FVector::DistSquared(ViewLoc, RestoreOrigin) <= FMath::Square(4200.f))
			{
				const FString SfxPkg = FPackageName::ObjectPathToPackageName(FString(AetheriaWorldConst::SFXMagicalRestore));
				if (!SfxPkg.IsEmpty() && FPackageName::DoesPackageExist(SfxPkg))
				{
					if (USoundBase* RestoreSfx = LoadObject<USoundBase>(nullptr, AetheriaWorldConst::SFXMagicalRestore))
					{
						UGameplayStatics::PlaySoundAtLocation(this, RestoreSfx, RestoreOrigin, 0.55f, FMath::FRandRange(1.04f, 1.16f));
					}
				}
			}
		}
	}

	SetActorTickEnabled(true);
	UE_LOG(LogChronicles, Log, TEXT("[Destruction] Prop '%s' magically restored with emerald-gold VFX."), *GetName());
}

void ADestructiblePropActor::TriggerShatter(const FVector& ImpulseOrigin, const FVector& StrikeDirection)
{
	if (bShatterEffectsSpawned)
	{
		return;
	}
	CaptureInitialTransformIfNeeded();
	bShatterEffectsSpawned = true;
	bMagicalRegrowthActive = false;
	bAwaitingMagicalRestore = true;
	MagicalRestoreRemaining = FMath::Max(5.f, MagicalRestoreDelaySeconds);
	DebrisCleanupRemaining = 5.5f;

	FVector Origin = GetActorLocation();
	FVector BoxExtent(60.f, 60.f, 90.f);

	if (IntactMesh)
	{
		const FBoxSphereBounds Bounds = IntactMesh->Bounds;
		Origin = Bounds.Origin;
		BoxExtent = Bounds.BoxExtent.ComponentMax(FVector(35.f, 35.f, 45.f)).ComponentMin(FVector(180.f, 180.f, 260.f));

		if (!bIsSliceableTree)
		{
			IntactMesh->SetVisibility(false, false);
		}
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

	// Interactive Sliceable Meadow Tree mode: Niagara extreme leaf burst + grounded wooden stump + smooth topple
	if (bIsSliceableTree)
	{
		if (!IsRunningDedicatedServer())
		{
			const FString LeafPkg = FPackageName::ObjectPathToPackageName(FString(AetheriaWorldConst::NSWindTreeLeavesExtreme));
			if (!LeafPkg.IsEmpty() && FPackageName::DoesPackageExist(LeafPkg))
			{
				if (UNiagaraSystem* LeafBurstNS = LoadObject<UNiagaraSystem>(nullptr, AetheriaWorldConst::NSWindTreeLeavesExtreme))
				{
					const FVector CanopyPos = Origin + FVector(0.f, 0.f, BoxExtent.Z * 0.45f);
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						LeafBurstNS,
						CanopyPos,
						FRotator::ZeroRotator,
						FVector(1.35f),
						true,
						true);
				}
			}

			const FString WoodPkg = FPackageName::ObjectPathToPackageName(FString(AetheriaWorldConst::SFXWoodBreak));
			if (!WoodPkg.IsEmpty() && FPackageName::DoesPackageExist(WoodPkg))
			{
				if (USoundBase* WoodSfx = LoadObject<USoundBase>(nullptr, AetheriaWorldConst::SFXWoodBreak))
				{
					UGameplayStatics::PlaySoundAtLocation(this, WoodSfx, Origin, 0.90f, FMath::FRandRange(0.82f, 0.98f));
				}
			}
		}

		// Spawn clean wooden stump cylinder anchored at ground level
		if (!TreeStumpMesh)
		{
			if (UStaticMesh* CylMesh = LoadObject<UStaticMesh>(nullptr, AetheriaWorldConst::MeshCylinder))
			{
				const FName StumpName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("SlicedTreeStump"));
				TreeStumpMesh = NewObject<UStaticMeshComponent>(this, StumpName);
				if (TreeStumpMesh)
				{
					TreeStumpMesh->SetStaticMesh(CylMesh);
					if (UMaterialInterface* WoodMat = AetheriaWorldHelpers::LoadFirstExistingMaterial({
						AetheriaWorldConst::MatMedievalWood,
						AetheriaWorldConst::MatDirtMounds,
						AetheriaWorldConst::MatStoneWall }))
					{
						TreeStumpMesh->SetMaterial(0, WoodMat);
					}
					TreeStumpMesh->SetUsingAbsoluteLocation(true);
					TreeStumpMesh->SetUsingAbsoluteRotation(true);
					TreeStumpMesh->SetUsingAbsoluteScale(true);
					TreeStumpMesh->SetMobility(EComponentMobility::Movable);
					TreeStumpMesh->SetWorldLocationAndRotation(
						GetActorLocation() + FVector(0.f, 0.f, 22.f),
						FRotator(0.f, GetActorRotation().Yaw, 0.f));
					TreeStumpMesh->SetWorldScale3D(FVector(0.42f, 0.42f, 0.48f));
					TreeStumpMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					TreeStumpMesh->SetCastShadow(true);
					TreeStumpMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
					TreeStumpMesh->RegisterComponent();
				}
			}
		}

		FVector FallDir2D(StrikeDirection.X, StrikeDirection.Y, 0.f);
		if (FallDir2D.SizeSquared() < 0.01f)
		{
			FallDir2D = GetActorForwardVector();
			FallDir2D.Z = 0.f;
		}
		FallDir2D = FallDir2D.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		TreeFallAxis = FVector::CrossProduct(FVector::UpVector, FallDir2D).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
		TreeStartQuat = IntactMesh ? IntactMesh->GetComponentQuat() : GetActorQuat();
		TreeToppleElapsed = 0.f;
		TreeToppleDuration = 1.35f;
		bTreeToppling = true;
		SetActorTickEnabled(true);

		UE_LOG(LogChronicles, Log, TEXT("[Destruction] Sliceable meadow tree '%s' toppled cleanly with leaf burst."), *GetName());
		return;
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
	if (!IsValid(ShatterFlashLight))
	{
		const FName LightName = MakeUniqueObjectName(this, UPointLightComponent::StaticClass(), TEXT("ShatterFlashLight"));
		ShatterFlashLight = NewObject<UPointLightComponent>(this, LightName);
		if (ShatterFlashLight)
		{
			ShatterFlashLight->SetUsingAbsoluteLocation(true);
			ShatterFlashLight->SetMobility(EComponentMobility::Movable);
			ShatterFlashLight->SetIntensityUnits(ELightUnits::Lumens);
			ShatterFlashLight->SetAttenuationRadius(1250.f);
			ShatterFlashLight->SetSourceRadius(32.f);
			ShatterFlashLight->SetCastShadows(false);
			ShatterFlashLight->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
			ShatterFlashLight->RegisterComponent();
		}
	}
	if (ShatterFlashLight)
	{
		FlashPeakLumens = 14000.f;
		FlashDuration = 0.42f;
		FlashElapsed = 0.f;
		bFlashActive = true;
		ShatterFlashLight->SetWorldLocation(Origin + FVector(0.f, 0.f, 35.f));
		ShatterFlashLight->SetIntensity(FlashPeakLumens);
		ShatterFlashLight->SetLightColor(GetFlashColor(MaterialKind));
		ShatterFlashLight->SetVisibility(true);
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

	const float SafeDelta = FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);

	if (bTreeToppling && IntactMesh)
	{
		TreeToppleElapsed += SafeDelta;
		const float SafeDur = FMath::Max(0.1f, TreeToppleDuration);
		if (TreeToppleElapsed <= SafeDur)
		{
			const float Alpha = FMath::Clamp(TreeToppleElapsed / SafeDur, 0.f, 1.f);
			const float FallCurve = FMath::Pow(Alpha, 2.15f);
			const FQuat FallDelta(TreeFallAxis, FMath::DegreesToRadians(84.f * FallCurve));
			IntactMesh->SetWorldRotation(FallDelta * TreeStartQuat);
		}
		else if (TreeToppleElapsed > SafeDur + 1.4f)
		{
			const FVector CurScale = IntactMesh->GetComponentScale();
			const FVector NextScale = FMath::VInterpTo(CurScale, FVector::ZeroVector, SafeDelta, 2.8f);
			IntactMesh->SetWorldScale3D(NextScale);

			if (TreeToppleElapsed >= SafeDur + 3.1f || NextScale.GetMax() < 0.03f)
			{
				IntactMesh->SetVisibility(false, false);
				bTreeToppling = false;
			}
		}
	}

	if (bFlashActive && ShatterFlashLight)
	{
		FlashElapsed += SafeDelta;
		const float Alpha = FMath::Clamp(1.f - (FlashElapsed / FMath::Max(0.05f, FlashDuration)), 0.f, 1.f);
		const float SmoothCurve = Alpha * Alpha * Alpha;
		ShatterFlashLight->SetIntensity(FlashPeakLumens * SmoothCurve);

		if (FlashElapsed >= FlashDuration)
		{
			ShatterFlashLight->SetIntensity(0.f);
			ShatterFlashLight->SetVisibility(false);
			bFlashActive = false;
		}
	}
	else
	{
		bFlashActive = false;
	}

	if (DebrisChunks.Num() > 0 && DebrisCleanupRemaining > 0.f)
	{
		DebrisCleanupRemaining -= SafeDelta;
		if (DebrisCleanupRemaining <= 0.f)
		{
			CleanupDebrisChunks();
		}
	}

	if (bMagicalRegrowthActive && IntactMesh)
	{
		MagicalRegrowthElapsed += SafeDelta;
		const float Alpha = FMath::Clamp(MagicalRegrowthElapsed / FMath::Max(0.1f, MagicalRegrowthDuration), 0.f, 1.f);
		const float Smooth = FMath::SmoothStep(0.f, 1.f, Alpha);
		IntactMesh->SetRelativeScale3D(FMath::Lerp(InitialMeshScale * 0.22f, InitialMeshScale, Smooth));
		if (Alpha >= 1.f)
		{
			IntactMesh->SetRelativeScale3D(InitialMeshScale);
			bMagicalRegrowthActive = false;
		}
	}

	if (bAwaitingMagicalRestore)
	{
		MagicalRestoreRemaining -= SafeDelta;
		if (MagicalRestoreRemaining <= 0.f)
		{
			if (HasAuthority())
			{
				TriggerMagicalRestoration();
			}
			else
			{
				bAwaitingMagicalRestore = false;
			}
		}
	}

	if (!bFlashActive && !bTreeToppling && !bAwaitingMagicalRestore && !bMagicalRegrowthActive && DebrisChunks.Num() == 0)
	{
		SetActorTickEnabled(false);
	}
}

void ADestructiblePropActor::NotifyQuestProgress()
{
	if (LinkedQuestId == NAME_None || bQuestProgressNotified)
	{
		return;
	}
	bQuestProgressNotified = true;

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

	// Pulse Ember Crypt molten lava fissures subtly (deep volcanic orange-red glow, NEVER blown-out white)
	if (LavaFloorMID)
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(ElapsedAnimTime * 2.2f);
		const FLinearColor LavaGlow(
			FMath::Lerp(0.95f, 1.28f, Pulse),
			FMath::Lerp(0.18f, 0.30f, Pulse),
			FMath::Lerp(0.010f, 0.022f, Pulse),
			1.0f);
		LavaFloorMID->SetVectorParameterValue(TEXT("Color"), LavaGlow);
		LavaFloorMID->SetVectorParameterValue(TEXT("AccentColor"), LavaGlow);
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

	// Update active spell-impact landscape scars & 60s magical meadow restoration
	UpdateTerrainScarsAndMagicalRestoration(SafeDelta);
}

void AAetheriaWorldBuilder::CycleTimeOfDayPreset()
{
	// Golden Sunset (0.68) -> Starry Full Moon (0.02) -> Misty Dawn (0.26) -> Highland Noon (0.48)
	static constexpr float Presets[4] = { 0.68f, 0.02f, 0.26f, 0.48f };
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

float AAetheriaWorldBuilder::ComputeMirrorLakeWaterZ() const
{
	// Sample across the entire oval lake basin so MirrorLake_WaterSurface always gleams ABOVE the meadow terrain!
	float MaxBasinZ = SampleGroundZ(0.f, -800.f, FallbackGroundZ);
	MaxBasinZ = FMath::Max(MaxBasinZ, SampleGroundZ(-1500.f, -800.f, FallbackGroundZ));
	MaxBasinZ = FMath::Max(MaxBasinZ, SampleGroundZ( 1500.f, -800.f, FallbackGroundZ));
	MaxBasinZ = FMath::Max(MaxBasinZ, SampleGroundZ(0.f,  600.f, FallbackGroundZ));
	MaxBasinZ = FMath::Max(MaxBasinZ, SampleGroundZ(0.f, -2200.f, FallbackGroundZ));
	return MaxBasinZ + 28.f;
}

FVector AAetheriaWorldBuilder::GetPrologueCaveSpawnLocation() const
{
	const float CliffZ = ComputeMirrorLakeWaterZ() + 2200.f;
	return FVector(-3210.f, -800.f, CliffZ + 98.f);
}

FVector AAetheriaWorldBuilder::GetPrologueTutorLocation() const
{
	const float CliffZ = ComputeMirrorLakeWaterZ() + 2200.f;
	return FVector(-2840.f, -860.f, CliffZ + 2.f);
}

FVector AAetheriaWorldBuilder::GetPrologueWaterfallBrinkLocation() const
{
	const float CliffZ = ComputeMirrorLakeWaterZ() + 2200.f;
	return FVector(-1980.f, -800.f, CliffZ + 40.f);
}

FVector AAetheriaWorldBuilder::GetPrologueLakePlungeLocation() const
{
	const float WaterZ = ComputeMirrorLakeWaterZ();
	return FVector(-1400.f, -800.f, WaterZ + 15.f);
}

FVector AAetheriaWorldBuilder::GetPrologueShoreEmergenceLocation() const
{
	return FVector(-480.f, 1680.f, SampleGroundZ(-480.f, 1680.f, FallbackGroundZ) + 98.f);
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
			SunComp->SetEnableLightShaftBloom(true);
			SunComp->SetBloomScale(0.42f);
			SunComp->SetBloomThreshold(0.85f);
			SunComp->SetVolumetricScatteringIntensity(2.6f);
			SunComp->SetIntensity(5.5f);
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
		// 0.02 — Starry Full Moon (crisp silver-blue 7400K moonlight at 1.6 lux)
		{
			0.02f, -32.f, 138.f, 7400.f, 1.6f, 0.58f, 0.0056f,
			FLinearColor(0.62f, 0.74f, 1.00f),
			FLinearColor(0.07f, 0.12f, 0.24f),
			FLinearColor(0.26f, 0.42f, 0.72f),
			FColor(130, 155, 205)
		},
		// 0.26 — Misty Dawn (soft rose-amber highland sunrise)
		{
			0.26f, -13.f, 54.f, 3700.f, 5.2f, 1.02f, 0.0054f,
			FLinearColor(1.00f, 0.81f, 0.69f),
			FLinearColor(0.48f, 0.40f, 0.44f),
			FLinearColor(0.96f, 0.64f, 0.44f),
			FColor(218, 198, 196)
		},
		// 0.48 — Highland Noon (crisp alpine daylight)
		{
			0.48f, -58.f, -14.f, 5900.f, 8.8f, 1.22f, 0.0038f,
			FLinearColor(1.00f, 0.98f, 0.94f),
			FLinearColor(0.38f, 0.54f, 0.74f),
			FLinearColor(0.78f, 0.86f, 0.98f),
			FColor(225, 232, 240)
		},
		// 0.68 — Golden Sunset / Golden Hour (warm golden 3950K sun rays through trees at 5.5 lux + soft valley fog)
		{
			0.68f, -14.f, -44.f, 3950.f, 5.5f, 1.45f, 0.0048f,
			FLinearColor(1.00f, 0.80f, 0.56f),
			FLinearColor(0.56f, 0.43f, 0.34f),
			FLinearColor(1.00f, 0.64f, 0.28f),
			FColor(228, 200, 170)
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

	UpdateNightSkyVisibility(T);
}

void AAetheriaWorldBuilder::UpdateNightSkyVisibility(float NormalizedTime)
{
	const float T = FMath::Fmod(NormalizedTime + 1.0f, 1.0f);

	// Dusk/Night window: reveal Full Moon & Starry Dome for T >= 0.78 or T <= 0.20
	float NightAlpha = 0.0f;
	if (T >= 0.85f || T <= 0.14f)
	{
		NightAlpha = 1.0f;
	}
	else if (T >= 0.76f && T < 0.85f)
	{
		NightAlpha = FMath::SmoothStep(0.76f, 0.85f, T);
	}
	else if (T > 0.14f && T <= 0.22f)
	{
		NightAlpha = 1.0f - FMath::SmoothStep(0.14f, 0.22f, T);
	}

	const bool bVisible = (NightAlpha > 0.02f);

	if (NightFullMoonMesh)
	{
		NightFullMoonMesh->SetVisibility(bVisible, false);
		NightFullMoonMesh->SetHiddenInGame(!bVisible, false);
		if (MoonMID && bVisible)
		{
			const float MoonRad = 2.2f * NightAlpha;
			const FLinearColor MoonColor(0.92f * MoonRad, 0.96f * MoonRad, 1.00f * MoonRad, 1.0f);
			MoonMID->SetVectorParameterValue(TEXT("Color"), MoonColor);
			MoonMID->SetVectorParameterValue(TEXT("EmissiveColor"), MoonColor);
		}
	}

	if (NightStarsISM)
	{
		NightStarsISM->SetVisibility(bVisible, false);
		NightStarsISM->SetHiddenInGame(!bVisible, false);
		if (StarMID && bVisible)
		{
			const float Twinkle = 0.88f + 0.12f * FMath::Sin(ElapsedAnimTime * 2.8f);
			const float StarRad = 1.9f * NightAlpha * Twinkle;
			const FLinearColor StarColor(0.84f * StarRad, 0.91f * StarRad, 1.00f * StarRad, 1.0f);
			StarMID->SetVectorParameterValue(TEXT("Color"), StarColor);
			StarMID->SetVectorParameterValue(TEXT("EmissiveColor"), StarColor);
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

UNiagaraSystem* AAetheriaWorldBuilder::LoadNiagaraSafe(const TCHAR* AssetPath)
{
	if (!AssetPath || *AssetPath == 0)
	{
		return nullptr;
	}

	const FString Key(AssetPath);
	if (const TObjectPtr<UNiagaraSystem>* Found = NiagaraCache.Find(Key))
	{
		if (*Found)
		{
			return Found->Get();
		}
	}

	const FString PkgName = FPackageName::ObjectPathToPackageName(Key);
	if (!PkgName.IsEmpty() && FPackageName::DoesPackageExist(PkgName))
	{
		if (UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, AssetPath))
		{
			NiagaraCache.Add(Key, NS);
			return NS;
		}
	}
	return nullptr;
}

bool AAetheriaWorldBuilder::IsInsideNoGrassZone(float X, float Y) const
{
	const FVector2D P(X, Y);

	// 1. Oakhaven Village stone plaza & Grand Abbey footprint
	if (FMath::Abs(X) < 1920.f && Y > 2920.f && Y < 5520.f)
	{
		return true;
	}
	if (FMath::Abs(X) < 1250.f && Y >= 5520.f && Y < 6950.f)
	{
		return true;
	}

	// 2. Mirror Lake water basin & arched Stone Bridge deck (keep northern grassy shore at (-480, 1680) lush!)
	const float LakeNormX = X / 2650.f;
	const float LakeNormY = (Y + 800.f) / 2150.f;
	if ((LakeNormX * LakeNormX + LakeNormY * LakeNormY) < 1.0f)
	{
		return true;
	}
	if (FMath::Abs(X) < 420.f && Y > -3350.f && Y < 1720.f)
	{
		return true;
	}

	// 3. Prologue Western Cliff Escarpment, Awakening Cave & Waterfall Plunge Pool
	if (X > -3950.f && X < -1180.f && Y > -1620.f && Y < 20.f)
	{
		return true;
	}

	// 4. Dragon Aerie Lakeside Stone Pavilion
	if (FVector2D::Distance(P, FVector2D(-3400.f, 2200.f)) < 880.f)
	{
		return true;
	}

	// 5. Highkeep Citadel walled fortress courtyard & keep
	if (FMath::Abs(X) < 2280.f && Y > -13350.f && Y < -7820.f)
	{
		return true;
	}

	// 6. Ember Crypt enclosed dungeon & caldera floor
	if (FVector2D::Distance(P, FVector2D(9800.f, -2400.f)) < 1750.f)
	{
		return true;
	}

	// 7. Crystal Glacier Peaks sanctuary clearing
	if (FVector2D::Distance(P, FVector2D(-6800.f, -6400.f)) < 1850.f)
	{
		return true;
	}

	// 8. King's Road stone-curbed lanes (Oakhaven->Bridge, Bridge->Highkeep, Oakhaven->Dragon Aerie)
	if (FMath::Abs(X) < 280.f && Y >= 1620.f && Y <= 3020.f)
	{
		return true;
	}
	if (FMath::Abs(X) < 280.f && Y >= -8150.f && Y <= -3220.f)
	{
		return true;
	}
	{
		const FVector2D SegA(-1750.f, 3400.f);
		const FVector2D SegB(-2780.f, 2350.f);
		const FVector2D AB = SegB - SegA;
		const float LenSq = FMath::Max(1.f, AB.SizeSquared());
		const float Proj = FMath::Clamp(FVector2D::DotProduct(P - SegA, AB) / LenSq, 0.f, 1.f);
		if (FVector2D::Distance(P, SegA + AB * Proj) < 240.f)
		{
			return true;
		}
	}

	return false;
}

UHierarchicalInstancedStaticMeshComponent* AAetheriaWorldBuilder::CreateFoliageHISM(
	const FString& CompName,
	const TCHAR* MeshPath,
	UMaterialInterface* OverrideMat,
	bool bCastShadow,
	int32 StartCullDist,
	int32 EndCullDist,
	bool bEnableCollision)
{
	UStaticMesh* Mesh = LoadMeshSafe(MeshPath, nullptr);
	if (!Mesh)
	{
		return nullptr;
	}

	const FName UniqueName = MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), FName(*CompName));
	UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, UniqueName);
	if (!HISM)
	{
		return nullptr;
	}

	HISM->SetStaticMesh(Mesh);
	HISM->SetMobility(EComponentMobility::Static);
	HISM->SetCastShadow(bCastShadow);
	HISM->bCastDynamicShadow = bCastShadow;
	HISM->SetCullDistances(StartCullDist, EndCullDist);
	HISM->bAffectDistanceFieldLighting = bCastShadow;

	if (OverrideMat)
	{
		const int32 NumMats = FMath::Max(1, HISM->GetNumMaterials());
		for (int32 Idx = 0; Idx < NumMats; ++Idx)
		{
			HISM->SetMaterial(Idx, OverrideMat);
		}
	}

	if (bEnableCollision)
	{
		HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HISM->SetCollisionObjectType(ECC_WorldStatic);
		HISM->SetCollisionResponseToAllChannels(ECR_Block);
		HISM->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		HISM->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);
	}
	else
	{
		HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HISM->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	HISM->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
	HISM->RegisterComponent();
	FoliageHISMs.Add(HISM);
	return HISM;
}

UNiagaraComponent* AAetheriaWorldBuilder::SpawnAmbientWindNiagara(
	const FString& CompName,
	const TCHAR* NiagaraPath,
	const FVector& WorldLoc,
	const FVector& WorldScale,
	const FRotator& WorldRot)
{
	UNiagaraSystem* System = LoadNiagaraSafe(NiagaraPath);
	if (!System)
	{
		return nullptr;
	}

	const FName UniqueName = MakeUniqueObjectName(this, UNiagaraComponent::StaticClass(), FName(*CompName));
	UNiagaraComponent* NiagaraComp = NewObject<UNiagaraComponent>(this, UniqueName);
	if (!NiagaraComp)
	{
		return nullptr;
	}

	NiagaraComp->SetAsset(System);
	NiagaraComp->SetAutoActivate(true);
	NiagaraComp->SetMobility(EComponentMobility::Movable);
	NiagaraComp->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
	NiagaraComp->SetWorldLocationAndRotation(WorldLoc, WorldRot);
	NiagaraComp->SetWorldScale3D(WorldScale);
	NiagaraComp->RegisterComponent();
	NiagaraComp->Activate(true);

	SpawnedNiagaraSystems.Add(NiagaraComp);
	return NiagaraComp;
}

void AAetheriaWorldBuilder::AddCurbedFloorBorder(
	const FString& Prefix,
	float CenterX,
	float CenterY,
	float HalfExtentX,
	float HalfExtentY,
	float FixedBaseZ,
	bool bFollowTerrain,
	bool bOpenNorth,
	bool bOpenSouth,
	bool bOpenEast,
	bool bOpenWest)
{
	UStaticMesh* BorderMesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicBorder1, AetheriaWorldConst::MeshEdgeStones);
	UStaticMesh* StepsMesh  = LoadMeshSafe(AetheriaWorldConst::MeshGothicSteps, AetheriaWorldConst::MeshEdgeStones);
	UStaticMesh* DirtMesh   = LoadMeshSafe(AetheriaWorldConst::MeshDirtMoundA, AetheriaWorldConst::MeshMWAMStoneA);

	UMaterialInterface* CurbMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatEdgeStonesInst,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* DirtMat = LoadPBRMaterial({
		AetheriaWorldConst::MatDirtMounds,
		AetheriaWorldConst::MatMWAMCoverRocks,
		AetheriaWorldConst::MatStoneAltar
	});

	const FBox BorderBox = AetheriaWorldHelpers::GetSafeBoundingBox(BorderMesh);
	const FVector BorderSize = BorderBox.GetSize();
	const float BorderScaleZ = FMath::Clamp(38.f / FMath::Max(8.f, BorderSize.Z), 0.25f, 1.8f);
	const float BorderSpan = FMath::Clamp(FMath::Max(BorderSize.X, BorderSize.Y) * BorderScaleZ, 180.f, 440.f);
	const FVector BorderScale(
		(BorderSpan * 1.04f) / FMath::Max(10.f, FMath::Max(BorderSize.X, BorderSize.Y)),
		FMath::Clamp(65.f / FMath::Max(10.f, FMath::Min(BorderSize.X, BorderSize.Y)), 0.35f, 1.8f),
		BorderScaleZ);

	const float StepsScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StepsMesh, 42.f, 0.2f, 1.8f);
	const float DirtScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(DirtMesh, 48.f, 0.25f, 2.2f);
	const float OpenGapHalf = 420.f;

	auto SampleEdgeZ = [&](float EX, float EY) -> float
	{
		return bFollowTerrain ? SampleGroundZ(EX, EY, FixedBaseZ) : FixedBaseZ;
	};

	auto PlaceBorderEdge = [&](const FString& EdgeTag, const FVector2D& Start, const FVector2D& End, float YawDeg, bool bHasOpening, const FVector2D& OutwardNormal)
	{
		const float EdgeLen = FVector2D::Distance(Start, End);
		const int32 NumSegs = FMath::Max(2, FMath::CeilToInt(EdgeLen / BorderSpan));
		for (int32 Idx = 0; Idx < NumSegs; ++Idx)
		{
			const float Alpha = (static_cast<float>(Idx) + 0.5f) / static_cast<float>(NumSegs);
			const FVector2D Pos = FMath::Lerp(Start, End, Alpha);
			const float DistFromMid = FMath::Abs(Alpha - 0.5f) * EdgeLen;

			if (bHasOpening && DistFromMid < OpenGapHalf)
			{
				continue;
			}

			const float GZ = SampleEdgeZ(Pos.X, Pos.Y);
			const FRotator CurbRot(0.f, YawDeg, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("%s_%s_Curb_%d"), *Prefix, *EdgeTag, Idx),
				AetheriaWorldConst::MeshGothicBorder1,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(BorderMesh, Pos.X, Pos.Y, GZ, CurbRot, BorderScale, 14.f),
				CurbRot,
				BorderScale,
				CurbMat,
				AetheriaWorldConst::MeshEdgeStones,
				true);

			// Soft sunken earth transition mound feathering stone edge cleanly into surrounding meadow
			if ((Idx % 2 == 0) && DirtMesh)
			{
				const FVector2D MoundPos = Pos + OutwardNormal * 85.f;
				const float MGZ = SampleEdgeZ(MoundPos.X, MoundPos.Y);
				const FRotator MoundRot(0.f, YawDeg + ((Idx % 4 == 0) ? 12.f : -12.f), 0.f);
				const FVector MoundScale(DirtScale * 1.65f, DirtScale * 1.15f, DirtScale * 0.65f);
				AddWorldMesh(
					FString::Printf(TEXT("%s_%s_EarthBlend_%d"), *Prefix, *EdgeTag, Idx),
					AetheriaWorldConst::MeshDirtMoundA,
					AetheriaWorldHelpers::ComputeGroundedCenterLoc(DirtMesh, MoundPos.X, MoundPos.Y, MGZ, MoundRot, MoundScale, 18.f),
					MoundRot,
					MoundScale,
					DirtMat,
					AetheriaWorldConst::MeshMWAMStoneA,
					false);
			}
		}

		// Place welcoming stone steps at open approach entrances
		if (bHasOpening && StepsMesh)
		{
			const FVector2D Mid = 0.5f * (Start + End) + OutwardNormal * 42.f;
			const float SGZ = SampleEdgeZ(Mid.X, Mid.Y);
			const FRotator StepRot(0.f, YawDeg, 0.f);
			const FVector StepScale(StepsScale * 1.85f, StepsScale * 1.25f, StepsScale * 0.72f);
			AddWorldMesh(
				FString::Printf(TEXT("%s_%s_Steps"), *Prefix, *EdgeTag),
				AetheriaWorldConst::MeshGothicSteps,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(StepsMesh, Mid.X, Mid.Y, SGZ, StepRot, StepScale, 12.f),
				StepRot,
				StepScale,
				CurbMat,
				AetheriaWorldConst::MeshEdgeStones,
				true);
		}
	};

	const float MinX = CenterX - HalfExtentX;
	const float MaxX = CenterX + HalfExtentX;
	const float MinY = CenterY - HalfExtentY;
	const float MaxY = CenterY + HalfExtentY;

	PlaceBorderEdge(TEXT("North"), FVector2D(MinX, MaxY), FVector2D(MaxX, MaxY),   0.f, bOpenNorth, FVector2D( 0.f,  1.f));
	PlaceBorderEdge(TEXT("South"), FVector2D(MinX, MinY), FVector2D(MaxX, MinY), 180.f, bOpenSouth, FVector2D( 0.f, -1.f));
	PlaceBorderEdge(TEXT("East"),  FVector2D(MaxX, MinY), FVector2D(MaxX, MaxY),  90.f, bOpenEast,  FVector2D( 1.f,  0.f));
	PlaceBorderEdge(TEXT("West"),  FVector2D(MinX, MinY), FVector2D(MinX, MaxY), -90.f, bOpenWest,  FVector2D(-1.f,  0.f));
}

void AAetheriaWorldBuilder::AddBoulderCluster(
	const FString& Prefix,
	float CenterX,
	float CenterY,
	int32 Count,
	float TargetHeightMin,
	float TargetHeightMax,
	FRandomStream& Rng,
	bool bWithBush)
{
	static const TCHAR* StoneMeshes[4] = {
		AetheriaWorldConst::MeshMWAMStoneA,
		AetheriaWorldConst::MeshMWAMStoneB,
		AetheriaWorldConst::MeshMWAMStoneC,
		AetheriaWorldConst::MeshMWAMStoneD
	};

	UMaterialInterface* RockMat = LoadPBRMaterial({
		AetheriaWorldConst::MatMWAMCoverRocks,
		AetheriaWorldConst::MatNamaqualandCliff,
		AetheriaWorldConst::MatStoneWall
	});

	const int32 ClampedCount = FMath::Clamp(Count, 1, 5);
	for (int32 Idx = 0; Idx < ClampedCount; ++Idx)
	{
		const TCHAR* StonePath = StoneMeshes[Rng.RandRange(0, 3)];
		UStaticMesh* StoneMesh = LoadMeshSafe(StonePath, AetheriaWorldConst::MeshMWAMStoneA);
		if (!StoneMesh)
		{
			continue;
		}

		// Primary boulder anchored near cluster center; companion stones nestled tightly around it
		const float Dist = (Idx == 0) ? Rng.FRandRange(0.f, 45.f) : Rng.FRandRange(95.f, 230.f);
		const float Angle = Rng.FRandRange(0.f, UE_TWO_PI);
		const float BX = CenterX + FMath::Cos(Angle) * Dist;
		const float BY = CenterY + FMath::Sin(Angle) * Dist;
		const float GZ = SampleGroundZ(BX, BY, FallbackGroundZ);

		const float SizeMult = (Idx == 0) ? 1.0f : Rng.FRandRange(0.48f, 0.82f);
		const float TargetH = Rng.FRandRange(TargetHeightMin, TargetHeightMax) * SizeMult;
		const float UnitScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StoneMesh, TargetH, 0.15f, 3.2f);
		const FVector StoneScale(
			UnitScale * Rng.FRandRange(0.88f, 1.22f),
			UnitScale * Rng.FRandRange(0.88f, 1.22f),
			UnitScale * Rng.FRandRange(0.82f, 1.08f));
		const FRotator StoneRot(
			Rng.FRandRange(-9.f, 9.f),
			Rng.FRandRange(0.f, 360.f),
			Rng.FRandRange(-9.f, 9.f));

		// Always sink 35% of scaled boulder height into terrain so zero rocks sit flat on dirt
		const float Sink35Cm = TargetH * 0.35f;
		AddWorldMesh(
			FString::Printf(TEXT("%s_Rock_%d"), *Prefix, Idx),
			StonePath,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(StoneMesh, BX, BY, GZ, StoneRot, StoneScale, Sink35Cm),
			StoneRot,
			StoneScale,
			RockMat,
			AetheriaWorldConst::MeshMWAMStoneA,
			true);
	}

	if (bWithBush)
	{
		if (UStaticMesh* BushMesh = LoadMeshSafe(AetheriaWorldConst::MeshWildRooibosBush, nullptr))
		{
			const int32 BushCount = Rng.RandRange(1, 2);
			for (int32 BIdx = 0; BIdx < BushCount; ++BIdx)
			{
				const float BAngle = Rng.FRandRange(0.f, UE_TWO_PI);
				const float BDist = Rng.FRandRange(125.f, 250.f);
				const float GX = CenterX + FMath::Cos(BAngle) * BDist;
				const float GY = CenterY + FMath::Sin(BAngle) * BDist;
				const float BGZ = SampleGroundZ(GX, GY, FallbackGroundZ);
				const float BushScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BushMesh, Rng.FRandRange(85.f, 135.f), 0.2f, 2.2f);
				const FRotator BushRot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);
				if (UStaticMeshComponent* BushComp = AddWorldMesh(
					FString::Printf(TEXT("%s_Bush_%d"), *Prefix, BIdx),
					AetheriaWorldConst::MeshWildRooibosBush,
					AetheriaWorldHelpers::ComputeGroundedCenterLoc(BushMesh, GX, GY, BGZ, BushRot, FVector(BushScale), 10.f),
					BushRot,
					FVector(BushScale),
					nullptr,
					nullptr,
					false))
				{
					BushComp->SetCastShadow(false);
				}
			}
		}
	}
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
		// Always disable Nanite when overriding material slots so non-Nanite MI assets (e.g. M_StoneWall_Inst, M_Pillar_Inst)
		// never emit "missing usage flag Nanite! Default Material will be used in game" or fall back to grey WorldGridMaterial!
		Comp->bDisallowNanite = true;
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
					Comp->bDisallowNanite = true;
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

	const float HalfSpan = TileStep * 0.52f;
	const float SafeLift = FMath::Max(LiftCm, 10.f);

	for (int32 GX = -HalfCountX; GX <= HalfCountX; ++GX)
	{
		for (int32 GY = -HalfCountY; GY <= HalfCountY; ++GY)
		{
			const float TX = CenterX + static_cast<float>(GX) * TileStep;
			const float TY = CenterY + static_cast<float>(GY) * TileStep;
			float BaseZ = FixedBaseZ;
			if (bFollowTerrain)
			{
				// Sample all 9 footprint points (center + 4 corners + 4 edge midpoints) so local terrain humps NEVER poke through stone courtyard tiles!
				float MaxFootprintZ = SampleGroundZ(TX, TY, CenterTerrainZ);
				for (int32 sy = -1; sy <= 1; ++sy)
				{
					for (int32 sx = -1; sx <= 1; ++sx)
					{
						if (sx == 0 && sy == 0) { continue; }
						MaxFootprintZ = FMath::Max(MaxFootprintZ, SampleGroundZ(TX + sx * HalfSpan, TY + sy * HalfSpan, CenterTerrainZ));
					}
				}
				BaseZ = FMath::Max(MaxFootprintZ, CenterTerrainZ - 14.f);
			}

			// Deterministic 90-degree tile rotation breaks up repeating stone grid patterns
			const int32 QuarterTurn = ((GX * 7 + GY * 13) & 3);
			const FRotator TileRot(0.f, static_cast<float>(QuarterTurn) * 90.f, 0.f);
			const FVector LocalTopCenter(RawCenter.X * ScaleX, RawCenter.Y * ScaleY, FloorBox.Max.Z * ScaleZ);
			const FVector RotatedAnchor = TileRot.RotateVector(LocalTopCenter);
			const FVector TileLoc(TX - RotatedAnchor.X, TY - RotatedAnchor.Y, (BaseZ + SafeLift) - RotatedAnchor.Z);

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
	BuildKingsRoadAndLanterns();
	BuildLushMeadowFoliageAndWind();
	BuildMicroDetailTwigsBarkAndPebbles();
	BuildNightSkyMoonAndStars();
	UpdateNightSkyVisibility(TimeOfDay);

	int32 TotalFoliageInstances = 0;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& HISM : FoliageHISMs)
	{
		if (HISM)
		{
			TotalFoliageInstances += HISM->GetInstanceCount();
		}
	}

	UE_LOG(LogChronicles, Display,
		TEXT("[WorldBuilder] Built cohesive lush 5-biome world + Prologue Cliff/Cave/Waterfall: %d meshes, %d HISM foliage instances (%d layers), %d Niagara wind systems, %d Cascade VFX, %d lights, %d destructibles."),
		SpawnedMeshes.Num(),
		TotalFoliageInstances,
		FoliageHISMs.Num(),
		SpawnedNiagaraSystems.Num(),
		SpawnedParticles.Num(),
		AnimatedLights.Num(),
		SpawnedDestructibles.Num());
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

	// 1. Flush Gothic Flagstone Courtyard Floor (BaseZ + 2.0f cm — ZERO floating slab edges!) + Curbs, Steps & Earth Blends
	BuildTiledStoneFloor(
		TEXT("Oakhaven_CourtyardFloor"),
		0.f,
		4200.f,
		1500.f,
		1000.f,
		500.f,
		2.f,
		AetheriaWorldConst::MeshDungeonFloor,
		StoneAltarMat,
		BrickFloorMat,
		620.f,
		true,
		FallbackGroundZ);

	AddCurbedFloorBorder(
		TEXT("Oakhaven_PlazaCurb"),
		0.f,
		4200.f,
		1755.f,
		1255.f,
		FallbackGroundZ,
		true,
		false,
		true,
		false,
		true);

	// Natural boulder clusters nestled with bushes framing the Abbey garden edges
	{
		FRandomStream OakRockRng(42019);
		AddBoulderCluster(TEXT("Oakhaven_GardenRocksW"), -1980.f, 5620.f, 3, 130.f, 220.f, OakRockRng, true);
		AddBoulderCluster(TEXT("Oakhaven_GardenRocksE"),  1980.f, 5620.f, 3, 130.f, 220.f, OakRockRng, true);
		AddBoulderCluster(TEXT("Oakhaven_GardenRocksN"), -1420.f, 6550.f, 2, 160.f, 250.f, OakRockRng, true);
	}

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

	auto SampleBuildingMinFootprintZ = [&](float BX, float BY, float HalfX, float HalfY, float FallbackZ) -> float
	{
		float MinZ = SampleGroundZ(BX, BY, FallbackZ);
		MinZ = FMath::Min(MinZ, SampleGroundZ(BX - HalfX, BY - HalfY, FallbackZ));
		MinZ = FMath::Min(MinZ, SampleGroundZ(BX + HalfX, BY - HalfY, FallbackZ));
		MinZ = FMath::Min(MinZ, SampleGroundZ(BX - HalfX, BY + HalfY, FallbackZ));
		MinZ = FMath::Min(MinZ, SampleGroundZ(BX + HalfX, BY + HalfY, FallbackZ));
		return MinZ;
	};

	// Grand Gothic Sanctuary Abbey backdrop behind the plaza at (0, 6250) — grounded to lowest corner so zero foundation corners float!
	{
		const float AbbeyGZ = SampleBuildingMinFootprintZ(0.f, 6250.f, 760.f, 420.f, FallbackGroundZ);
		const FRotator AbbeyRot(0.f, 180.f, 0.f);
		AddWorldMesh(
			TEXT("Oakhaven_GrandAbbeyBackdrop"),
			AetheriaWorldConst::MeshGothicBuilding09,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(GothicBuildingMesh, 0.f, 6250.f, AbbeyGZ, AbbeyRot, FVector(AbbeyScale), 148.f),
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
		const float MGZ = SampleBuildingMinFootprintZ(MX, MY, 420.f, 620.f, FallbackGroundZ);
		const FRotator MRot(0.f, ManorYaws[Idx], 0.f);
		AddWorldMesh(
			FString::Printf(TEXT("Oakhaven_GothicManor_%d"), Idx),
			AetheriaWorldConst::MeshGothicBuilding09,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(GothicBuildingMesh, MX, MY, MGZ, MRot, FVector(ManorScale), 32.f),
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
	const float SouthBankZ = SampleGroundZ(0.f, -3200.f, FallbackGroundZ);
	const float WaterZ     = ComputeMirrorLakeWaterZ();

	// Build the Prologue Rocky Awakening Cave, Panoramic High Cliff Overlook & Colossal Waterfall directly over Mirror Lake!
	BuildPrologueCaveAndWaterfall(WaterZ);

	// Luminous dusk-sapphire & golden-hour reflective oval lake surface (seated cleanly above meadow basin!)
	MirrorWaterMID = CreateColorMID(FLinearColor(0.055f, 0.26f, 0.38f, 1.0f), false);
	if (MirrorWaterMID)
	{
		MirrorWaterMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.03f, 0.14f, 0.22f, 1.0f));
		MirrorWaterMID->SetScalarParameterValue(TEXT("Roughness"), 0.06f);
		MirrorWaterMID->SetScalarParameterValue(TEXT("Metallic"), 0.18f);
		MirrorWaterMID->SetScalarParameterValue(TEXT("Specular"), 0.85f);
	}

	AddWorldMesh(
		TEXT("MirrorLake_WaterSurface"),
		AetheriaWorldConst::MeshCylinder,
		FVector(0.f, -800.f, WaterZ),
		FRotator::ZeroRotator,
		FVector(53.f, 43.f, 0.06f),
		MirrorWaterMID,
		AetheriaWorldConst::MeshPlane,
		false);

	// Natural Mossy Shoreline Boulders framing the oval Mirror Lake banks
	{
		UStaticMesh* ShoreStoneMesh = LoadMeshSafe(AetheriaWorldConst::MeshNamaqualandBoulder02, AetheriaWorldConst::MeshMWAMStoneA);
		UMaterialInterface* ShoreRockMat = LoadPBRMaterial({ AetheriaWorldConst::MatMWAMCoverRocks, AetheriaWorldConst::MatStoneWall });
		if (ShoreStoneMesh)
		{
			const float ShoreUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ShoreStoneMesh, 115.f, 0.20f, 4.5f);
			for (int32 SIdx = 0; SIdx < 24; ++SIdx)
			{
				const float Ang = (static_cast<float>(SIdx) / 24.f) * UE_TWO_PI;
				const float SX = FMath::Cos(Ang) * 2660.f;
				const float SY = -800.f + FMath::Sin(Ang) * 2160.f;
				if (FMath::Abs(SX) < 480.f || (SX < -1300.f && FMath::Abs(SY + 800.f) < 620.f))
				{
					continue; // Leave Stone Bridge ramps & Waterfall plunge pool clear!
				}
				const float SGZ = FMath::Max(SampleGroundZ(SX, SY, WaterZ - 20.f), WaterZ - 35.f);
				const FRotator SRot(0.f, SIdx * 47.f, 0.f);
				const FVector SScale(ShoreUnit * 1.45f, ShoreUnit * 1.15f, ShoreUnit * 0.72f);
				const bool bFallback = ShoreStoneMesh->GetPathName().Contains(TEXT("SM_MWAM_Stone"));
				AddWorldMesh(
					FString::Printf(TEXT("MirrorLake_ShoreRock_%d"), SIdx),
					AetheriaWorldConst::MeshNamaqualandBoulder02,
					AetheriaWorldHelpers::ComputeGroundedCenterLoc(ShoreStoneMesh, SX, SY, SGZ, SRot, SScale, 12.f),
					SRot,
					SScale,
					bFallback ? ShoreRockMat : nullptr,
					AetheriaWorldConst::MeshMWAMStoneA,
					false);
			}
		}
	}

	// Arched Flagstone Bridge spanning Y: 1600 -> -3200 (4800 cm total length, 9 smooth arch spans with SM_Bridge1/2 & SM_Handrails1)
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
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatDecorativeArches,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* ParapetMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatEdgeStonesInst
	});

	UStaticMesh* DeckMesh     = LoadMeshSafe(AetheriaWorldConst::MeshDungeonFloor, AetheriaWorldConst::MeshCryptFloor);
	UStaticMesh* Bridge1Mesh  = LoadMeshSafe(AetheriaWorldConst::MeshGothicBridge1, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* Bridge2Mesh  = LoadMeshSafe(AetheriaWorldConst::MeshGothicBridge2, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* HandrailMesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicHandrails1, AetheriaWorldConst::MeshDungeonWallHalf);
	UStaticMesh* StepsMesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicSteps, AetheriaWorldConst::MeshEdgeStones);

	const FBox DeckBox = AetheriaWorldHelpers::GetSafeBoundingBox(DeckMesh);
	const FVector DeckRawSize = DeckBox.GetSize();
	const FVector DeckRawCenter = DeckBox.GetCenter();
	const float Bridge1Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Bridge1Mesh, 260.f, 0.2f, 2.6f);
	const float Bridge2Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Bridge2Mesh, 260.f, 0.2f, 2.6f);
	const float HandrailScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(HandrailMesh, 118.f, 0.2f, 2.5f);
	const float RampStepsScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StepsMesh, 95.f, 0.25f, 2.4f);

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

		// Solid walkable medieval flagstone deck aligned along parabolic arch tangent
		const FVector DeckScale(
			620.f / FMath::Max(10.f, DeckRawSize.X),
			(SegLen + 48.f) / FMath::Max(10.f, DeckRawSize.Y),
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

		// Sculpted Gothic Bridge stone arch piers underneath the bridge deck (SM_Bridge1 / SM_Bridge2)
		if (Idx > 0 && Idx < NumSpans - 1)
		{
			const bool bUseB1 = (Idx % 2 == 1);
			UStaticMesh* SpanMesh = bUseB1 ? Bridge1Mesh : Bridge2Mesh;
			const TCHAR* SpanPath = bUseB1 ? AetheriaWorldConst::MeshGothicBridge1 : AetheriaWorldConst::MeshGothicBridge2;
			const float SpanScale = (bUseB1 ? Bridge1Scale : Bridge2Scale) * 1.15f;
			const FRotator ArchRot(0.f, 90.f, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("MirrorBridge_Span_%d"), Idx),
				SpanPath,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(SpanMesh, 0.f, Mid.Y, WaterZ - 35.f, ArchRot, FVector(SpanScale), 12.f),
				ArchRot,
				FVector(SpanScale),
				BridgeArchMat);
		}

		// Left & right carved gothic stone handrails (SM_Handrails1)
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const FRotator RailRot(0.f, Side > 0 ? 90.f : -90.f, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("MirrorBridge_Handrail_%d_%d"), Idx, Side > 0 ? 1 : 0),
				AetheriaWorldConst::MeshGothicHandrails1,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(HandrailMesh, Side * 305.f, Mid.Y, Mid.Z, RailRot, FVector(HandrailScale), 8.f),
				RailRot,
				FVector(HandrailScale),
				ParapetMat);
		}
	}

	// Sloped stone approach ramps seamlessly joining Mirror Lake Bridge to the King's Road at both ends
	{
		const FVector NorthEnd = EvalBridgePoint(0.f);
		const FRotator NorthRampRot(0.f, 0.f, 0.f);
		const FVector NorthRampScale(RampStepsScale * 2.1f, RampStepsScale * 1.45f, RampStepsScale * 0.75f);
		AddWorldMesh(
			TEXT("MirrorBridge_NorthApproachRamp"),
			AetheriaWorldConst::MeshGothicSteps,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(StepsMesh, 0.f, StartY + 135.f, NorthBankZ, NorthRampRot, NorthRampScale, 12.f),
			NorthRampRot,
			NorthRampScale,
			ParapetMat,
			AetheriaWorldConst::MeshEdgeStones,
			true);

		const FVector SouthEnd = EvalBridgePoint(1.f);
		const FRotator SouthRampRot(0.f, 180.f, 0.f);
		const FVector SouthRampScale(RampStepsScale * 2.1f, RampStepsScale * 1.45f, RampStepsScale * 0.75f);
		AddWorldMesh(
			TEXT("MirrorBridge_SouthApproachRamp"),
			AetheriaWorldConst::MeshGothicSteps,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(StepsMesh, 0.f, EndY - 135.f, SouthBankZ, SouthRampRot, SouthRampScale, 12.f),
			SouthRampRot,
			SouthRampScale,
			ParapetMat,
			AetheriaWorldConst::MeshEdgeStones,
			true);
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

void AAetheriaWorldBuilder::BuildPrologueCaveAndWaterfall(float WaterZ)
{
	// Shared Prologue Geometry (matched 1:1 with Track C Prologue Cutscene & Spawn):
	// Western Cliff Escarpment & Enclosed Awakening Cavern at X = -3650..-1850, Y = -800, CliffZ = WaterZ + 2200.f facing East (+X)
	const float CliffZ = WaterZ + 2200.f;
	const float CenterY = -800.f;

	static const TCHAR* CliffBoulders[5] = {
		AetheriaWorldConst::MeshNamaqualandBoulderA,
		AetheriaWorldConst::MeshNamaqualandBoulderB,
		AetheriaWorldConst::MeshNamaqualandBoulder02,
		AetheriaWorldConst::MeshNamaqualandBoulder03,
		AetheriaWorldConst::MeshNamaqualandBoulder04
	};

	UMaterialInterface* StoneAltarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* CliffRockMat = LoadPBRMaterial({
		AetheriaWorldConst::MatMWAMCoverRocks,
		AetheriaWorldConst::MatNamaqualandCliff,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* PillarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatPillarInst,
		AetheriaWorldConst::MatStoneWall,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* PrismCrystalMat = LoadPBRMaterial({
		AetheriaWorldConst::MatAetheriaPrismWall
	});
	if (!PrismCrystalMat)
	{
		PrismCrystalMat = CreateColorMID(FLinearColor(0.28f, 0.88f, 1.45f, 1.0f), true);
	}

	UStaticMesh* FloorMesh  = LoadMeshSafe(AetheriaWorldConst::MeshCryptFloor, AetheriaWorldConst::MeshDungeonFloor);
	UStaticMesh* ArcMesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* ColumnMesh = LoadMeshSafe(AetheriaWorldConst::MeshGothicColumn1, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* AltarMesh  = LoadMeshSafe(AetheriaWorldConst::MeshMedievalAltar, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* CrystalMesh= LoadMeshSafe(AetheriaWorldConst::MeshCoreCrystal, AetheriaWorldConst::MeshEarthSpikeSingle);
	UStaticMesh* FernMesh   = LoadMeshSafe(AetheriaWorldConst::MeshKiteFern01, nullptr);

	FRandomStream CliffRng(77188);

	// 1. Walkable Solid Colliding Cavern Floor (GX = 0..2 strictly inside the cave X = -3610..-2360; outdoor overlook uses natural flat cliff boulders!)
	if (FloorMesh)
	{
		const FBox FloorBox = AetheriaWorldHelpers::GetSafeBoundingBox(FloorMesh);
		const FVector FloorSize = FloorBox.GetSize();
		const FVector FloorCenter = FloorBox.GetCenter();
		const float TileW = 425.f;
		const float TileD = 380.f;
		const float ScaleX = (TileW * 1.03f) / FMath::Max(10.f, FloorSize.X);
		const float ScaleY = (TileD * 1.03f) / FMath::Max(10.f, FloorSize.Y);
		const float ScaleZ = FMath::Clamp(56.f / FMath::Max(6.f, FloorSize.Z), 0.45f, 2.8f);
		const FVector TileScale(ScaleX, ScaleY, ScaleZ);

		for (int32 GX = 0; GX < 3; ++GX)
		{
			for (int32 GY = 0; GY < 3; ++GY)
			{
				const float TX = -3395.f + static_cast<float>(GX) * 415.f; // covers [-3610 .. -2360] inside cavern
				const float TY = -1170.f + static_cast<float>(GY) * 370.f; // covers [-1360 .. -240]
				const FRotator TileRot(0.f, static_cast<float>(((GX * 3 + GY * 5) & 3) * 90), 0.f);
				const FVector LocalTopCenter(FloorCenter.X * ScaleX, FloorCenter.Y * ScaleY, FloorBox.Max.Z * ScaleZ);
				const FVector TileLoc = FVector(TX, TY, CliffZ) - TileRot.RotateVector(LocalTopCenter);

				AddWorldMesh(
					FString::Printf(TEXT("Prologue_CliffFloor_%d_%d"), GX, GY),
					AetheriaWorldConst::MeshCryptFloor,
					TileLoc,
					TileRot,
					TileScale,
					StoneAltarMat,
					AetheriaWorldConst::MeshDungeonFloor,
					true);
			}
		}
	}

	// Inset Opaque Rocky Mountain Headland Core (tucked strictly INSIDE the outer 3D Namaqualand Megascans boulders so zero flat box faces ever show!)
	AddWorldMesh(
		TEXT("Prologue_MountainCoreUpper"),
		AetheriaWorldConst::MeshCube,
		FVector(-2940.f, CenterY, WaterZ + 940.f),
		FRotator::ZeroRotator,
		FVector(14.2f, 9.6f, 22.8f),
		CliffRockMat,
		AetheriaWorldConst::MeshCube,
		true);
	AddWorldMesh(
		TEXT("Prologue_MountainCoreBaseSkirt"),
		AetheriaWorldConst::MeshCube,
		FVector(-2980.f, CenterY, WaterZ + 410.f),
		FRotator::ZeroRotator,
		FVector(17.4f, 12.2f, 13.0f),
		CliffRockMat,
		AetheriaWorldConst::MeshCube,
		true);
	// Inset Opaque Cavern Light-Blocking Shell (blocks ECC_Camera so 3rd-person SpringArm NEVER clips behind cave walls into pitch darkness!)
	auto AddCameraBlockingShell = [&](const FString& ShellName, const FVector& Loc, const FVector& Scale)
	{
		if (UStaticMeshComponent* ShellComp = AddWorldMesh(
			ShellName,
			AetheriaWorldConst::MeshCube,
			Loc,
			FRotator::ZeroRotator,
			Scale,
			CliffRockMat,
			AetheriaWorldConst::MeshCube,
			true))
		{
			ShellComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
		}
	};
	AddCameraBlockingShell(TEXT("Prologue_CaveOuterShellNorth"), FVector(-3055.f, CenterY - 545.f, CliffZ + 275.f), FVector(11.2f, 0.75f, 5.4f));
	AddCameraBlockingShell(TEXT("Prologue_CaveOuterShellSouth"), FVector(-3055.f, CenterY + 545.f, CliffZ + 275.f), FVector(11.2f, 0.75f, 5.4f));
	AddCameraBlockingShell(TEXT("Prologue_CaveOuterShellWest"),  FVector(-3615.f, CenterY, CliffZ + 275.f),         FVector(0.85f, 11.4f, 5.4f));
	AddCameraBlockingShell(TEXT("Prologue_CaveOuterShellRoof"),  FVector(-3055.f, CenterY, CliffZ + 555.f),         FVector(11.2f, 11.4f, 0.75f));

	// Helper to spawn a stacked massive closed 360-degree Megascans Namaqualand cliff boulder
	auto PlaceCliffRock = [&](const FString& Name, float X, float Y, float BottomZ, float TargetH, float WidthMult, const FRotator& Rot, bool bCollide)
	{
		const TCHAR* StonePath = CliffBoulders[CliffRng.RandRange(0, 4)];
		UStaticMesh* StoneMesh = LoadMeshSafe(StonePath, AetheriaWorldConst::MeshMWAMStoneA);
		if (!StoneMesh)
		{
			return;
		}
		const bool bUsingFallback = (StoneMesh->GetPathName().Contains(TEXT("SM_MWAM_Stone")));
		const float Unit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StoneMesh, TargetH, 0.25f, 24.0f);
		const FVector Scale(Unit * WidthMult, Unit * WidthMult * CliffRng.FRandRange(0.88f, 1.15f), Unit);
		AddWorldMesh(
			Name,
			StonePath,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(StoneMesh, X, Y, BottomZ, Rot, Scale, 0.f),
			Rot,
			Scale,
			bUsingFallback ? CliffRockMat : nullptr,
			AetheriaWorldConst::MeshMWAMStoneA,
			bCollide);
	};

	// 2. Massive Stacked Mountain Cliff Escarpment Supporting Platform (Z = WaterZ - 120 .. CliffZ)
	const float TierHeights[4] = { WaterZ - 120.f, WaterZ + 480.f, WaterZ + 1080.f, WaterZ + 1620.f };
	const float TierRockH[4]   = { 860.f, 820.f, 760.f, 640.f };
	const float TierSpread[4]  = { 380.f, 250.f, 130.f, 20.f };

	for (int32 Tier = 0; Tier < 4; ++Tier)
	{
		const float BZ = TierHeights[Tier];
		const float RH = TierRockH[Tier];
		const float Spread = TierSpread[Tier];

		// Sheer Eastern Waterfall Cliff Face (stepping slightly eastward toward the lake plunge)
		for (int32 Row = -2; Row <= 2; ++Row)
		{
			const float RY = CenterY + static_cast<float>(Row) * (310.f + Spread * 0.22f);
			const float RX = -2050.f + Spread * 0.75f + CliffRng.FRandRange(-85.f, 65.f);
			PlaceCliffRock(
				FString::Printf(TEXT("Prologue_CliffEastFace_T%d_%d"), Tier, Row + 2),
				RX, RY, BZ, RH, 1.35f,
				FRotator(CliffRng.FRandRange(-8.f, 8.f), CliffRng.FRandRange(0.f, 360.f), CliffRng.FRandRange(-8.f, 8.f)),
				true);
		}

		// North & South Cliff Flank Walls (keep Tier 3 wide open so cavern interior & Keeper Vaelen have 100% clearance!)
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			for (int32 Col = 0; Col < 4; ++Col)
			{
				const float RX = -3650.f + static_cast<float>(Col) * 480.f;
				const float FlankDist = (Tier == 3) ? 940.f : (640.f + Spread * 0.85f);
				const float RY = CenterY + static_cast<float>(Side) * FlankDist;
				PlaceCliffRock(
					FString::Printf(TEXT("Prologue_CliffFlank_T%d_%d_%d"), Tier, Side > 0 ? 1 : 0, Col),
					RX, RY, BZ, RH * 1.05f, (Tier == 3) ? 1.15f : 1.45f,
					FRotator(CliffRng.FRandRange(-10.f, 10.f), CliffRng.FRandRange(0.f, 360.f), CliffRng.FRandRange(-10.f, 10.f)),
					true);
			}
		}

		// Western Mountain Spine backing the cliff mass
		for (int32 Row = -1; Row <= 1; ++Row)
		{
			const float RX = -3780.f - Spread * 0.75f;
			const float RY = CenterY + static_cast<float>(Row) * (460.f + Spread * 0.35f);
			PlaceCliffRock(
				FString::Printf(TEXT("Prologue_CliffWestSpine_T%d_%d"), Tier, Row + 1),
				RX, RY, BZ, RH * 1.15f, 1.55f,
				FRotator(CliffRng.FRandRange(-8.f, 8.f), CliffRng.FRandRange(0.f, 360.f), CliffRng.FRandRange(-8.f, 8.f)),
				false);
		}
	}

	// 3. Enclosed Torchlit Awakening Cave (X = -3600 .. -2420, Y = -1350 .. -250, Z = CliffZ .. CliffZ + 620)
	// Rear Cavern Wall (visual crag rock behind Prologue_CaveOuterShellWest; bCollide=false so boulder bounds never block camera LOS inside cavern!)
	for (int32 Row = -2; Row <= 2; ++Row)
	{
		const float RY = CenterY + static_cast<float>(Row) * 265.f;
		PlaceCliffRock(
			FString::Printf(TEXT("Prologue_CaveBackWall_%d"), Row + 2),
			-3890.f + CliffRng.FRandRange(-30.f, 20.f),
			RY,
			CliffZ - 90.f,
			760.f,
			1.10f,
			FRotator(CliffRng.FRandRange(-6.f, 6.f), CliffRng.FRandRange(0.f, 360.f), CliffRng.FRandRange(-6.f, 6.f)),
			false);
	}

	// North & South Cavern Side Walls (offset outward to CenterY ± 760 so the 960 cm cavern walkway & sunlit mouth stay wide open!)
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		for (int32 Col = 0; Col < 5; ++Col)
		{
			const float RX = -3540.f + static_cast<float>(Col) * 275.f;
			const float RY = CenterY + static_cast<float>(Side) * 760.f;
			PlaceCliffRock(
				FString::Printf(TEXT("Prologue_CaveSideWall_%d_%d"), Side > 0 ? 1 : 0, Col),
				RX, RY, CliffZ - 80.f, 710.f, 1.05f,
				FRotator(CliffRng.FRandRange(-8.f, 8.f), CliffRng.FRandRange(0.f, 360.f), CliffRng.FRandRange(-8.f, 8.f)),
				true);
		}
	}

	// Arched Rocky Cavern Roof & Craggy Mountain Summit Crest (X = -3600 .. -2560, leaving wide sunlit Cave Mouth at X = -2420!)
	for (int32 Col = 0; Col < 4; ++Col)
	{
		const float RX = -3520.f + static_cast<float>(Col) * 305.f;
		for (int32 Row = -1; Row <= 1; ++Row)
		{
			const float RY = CenterY + static_cast<float>(Row) * 360.f;
			const float ArchLift = (Row == 0) ? 545.f : 465.f;
			PlaceCliffRock(
				FString::Printf(TEXT("Prologue_CaveRoof_%d_%d"), Col, Row + 1),
				RX, RY, CliffZ + ArchLift, 420.f, 1.85f,
				FRotator(CliffRng.FRandRange(-6.f, 6.f), CliffRng.FRandRange(0.f, 360.f), CliffRng.FRandRange(-6.f, 6.f)),
				false);
		}
	}

	// Natural Craggy Cliff-Brow Lip Boulders concealing the Prologue_CliffFloor edges (eliminates diving-board slab edges!)
	for (int32 LipIdx = 0; LipIdx < 6; ++LipIdx)
	{
		const float LY = -1340.f + static_cast<float>(LipIdx) * 216.f;
		// Leave the central stream channel (-920 .. -680) recessed slightly so water pours naturally over the brink
		const float LipDrop = (FMath::Abs(LY - CenterY) < 160.f) ? 265.f : 215.f;
		PlaceCliffRock(
			FString::Printf(TEXT("Prologue_CliffBrowLip_%d"), LipIdx),
			-1955.f + CliffRng.FRandRange(-25.f, 25.f),
			LY,
			CliffZ - LipDrop,
			260.f,
			1.55f,
			FRotator(CliffRng.FRandRange(-5.f, 5.f), CliffRng.FRandRange(0.f, 360.f), CliffRng.FRandRange(-5.f, 5.f)),
			false);
	}

	// Natural Walkable Megascans Rocky Cliff Overlook Ledge (X = -2420 .. -1940, Y = -1280 .. -320, TopZ = CliffZ — ZERO dungeon floor tiles outdoors!)
	for (int32 OX = 0; OX < 2; ++OX)
	{
		for (int32 OY = -2; OY <= 2; ++OY)
		{
			const float LedgeX = -2320.f + static_cast<float>(OX) * 240.f;
			const float LedgeY = CenterY + static_cast<float>(OY) * 220.f;
			PlaceCliffRock(
				FString::Printf(TEXT("Prologue_NaturalOverlookRock_%d_%d"), OX, OY + 2),
				LedgeX, LedgeY, CliffZ - 380.f, 382.f, 1.85f,
				FRotator(0.f, (OX * 73 + (OY + 2) * 41) % 360, 0.f),
				true);
		}
	}

	// Ancient Shrine Flank Columns along Cavern Walls (ZERO center bulkheads blocking Tutor Vaelen or the sunlit Cave Mouth!)
	const float RibColScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ColumnMesh, 490.f, 0.25f, 3.0f);
	const float RibXs[3] = { -3320.f, -2960.f, -2620.f };
	for (int32 Idx = 0; Idx < 3; ++Idx)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddWorldMesh(
				FString::Printf(TEXT("Prologue_CaveRibCol_%d_%d"), Idx, Side > 0 ? 1 : 0),
				AetheriaWorldConst::MeshGothicColumn1,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(ColumnMesh, RibXs[Idx], CenterY + Side * 475.f, CliffZ, FRotator::ZeroRotator, FVector(RibColScale), 8.f),
				FRotator::ZeroRotator,
				FVector(RibColScale),
				PillarMat);
		}
	}

	// Cave Interior Dressing: Ancient Rune Altar behind Awakening Pedestal, Warm Torches, Glowing Crystals & Mouth Ferns
	{
		const float AltarScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(AltarMesh, 118.f, 0.25f, 2.0f);
		const FRotator AltarRot(0.f, 90.f, 0.f);
		AddWorldMesh(
			TEXT("Prologue_AwakeningRuneAltar"),
			AetheriaWorldConst::MeshMedievalAltar,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(AltarMesh, -3465.f, CenterY, CliffZ, AltarRot, FVector(AltarScale), 4.f),
			AltarRot,
			FVector(AltarScale),
			StoneAltarMat);

		// Four warm physical Lumen P_Torch_Fire braziers illuminating the Awakening Cavern & Tutor Vaelen against sunlit exposure!
		AddBrazierAssembly(TEXT("Prologue_CaveBrazierL"), FVector(-3210.f, CenterY - 250.f, CliffZ), 4200.f, 1550.f, AetheriaWorldConst::MeshFirePit, 0.95f, 0.3f);
		AddBrazierAssembly(TEXT("Prologue_CaveBrazierR"), FVector(-3210.f, CenterY + 250.f, CliffZ), 4200.f, 1550.f, AetheriaWorldConst::MeshFirePit, 0.95f, 1.9f);
		AddBrazierAssembly(TEXT("Prologue_MouthBrazierL"), FVector(-2520.f, CenterY - 310.f, CliffZ), 3600.f, 1450.f, AetheriaWorldConst::MeshFirePit, 0.92f, 3.1f);
		AddBrazierAssembly(TEXT("Prologue_MouthBrazierR"), FVector(-2520.f, CenterY + 310.f, CliffZ), 3600.f, 1450.f, AetheriaWorldConst::MeshFirePit, 0.92f, 4.4f);
		AddAmbientFireLight(
			TEXT("Prologue_CaveWarmVaultFill"),
			FVector(-2980.f, CenterY, CliffZ + 290.f),
			4600.f,
			2700.f,
			1650.f,
			FLinearColor(1.0f, 0.76f, 0.44f, 1.0f),
			0.05f,
			1.4f,
			false,
			0.12f);

		// Glowing crystal accents inside cavern walls
		const float CrystalUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CrystalMesh, 135.f, 0.15f, 2.0f);
		const FVector CrystalCoords[4] = {
			FVector(-3470.f, CenterY - 320.f, CliffZ),
			FVector(-3470.f, CenterY + 320.f, CliffZ),
			FVector(-2840.f, CenterY - 415.f, CliffZ),
			FVector(-2840.f, CenterY + 415.f, CliffZ)
		};
		for (int32 Idx = 0; Idx < 4; ++Idx)
		{
			const FRotator CryRot(CliffRng.FRandRange(-12.f, 12.f), Idx * 82.f, CliffRng.FRandRange(-12.f, 12.f));
			const float CS = CrystalUnit * CliffRng.FRandRange(0.75f, 1.15f);
			AddWorldMesh(
				FString::Printf(TEXT("Prologue_CaveCrystal_%d"), Idx),
				AetheriaWorldConst::MeshCoreCrystal,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(CrystalMesh, CrystalCoords[Idx].X, CrystalCoords[Idx].Y, CrystalCoords[Idx].Z, CryRot, FVector(CS), 14.f),
				CryRot,
				FVector(CS),
				PrismCrystalMat,
				AetheriaWorldConst::MeshEarthSpikeSingle,
				false);
		}

		AddAmbientFireLight(
			TEXT("Prologue_CaveCrystalFill"),
			FVector(-3340.f, CenterY, CliffZ + 175.f),
			360.f,
			7500.f,
			900.f,
			FLinearColor(0.32f, 0.80f, 1.0f, 1.0f),
			0.08f,
			0.7f,
			false,
			0.10f);

		// Ferns clustered naturally around the sunlit cave mouth
		if (FernMesh)
		{
			const float FernUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(FernMesh, 78.f, 0.25f, 2.5f);
			const FVector2D MouthFerns[6] = {
				FVector2D(-2560.f, CenterY - 380.f),
				FVector2D(-2450.f, CenterY - 350.f),
				FVector2D(-2360.f, CenterY - 410.f),
				FVector2D(-2560.f, CenterY + 380.f),
				FVector2D(-2450.f, CenterY + 350.f),
				FVector2D(-2360.f, CenterY + 410.f)
			};
			for (int32 Idx = 0; Idx < 6; ++Idx)
			{
				const float FS = FernUnit * CliffRng.FRandRange(0.85f, 1.35f);
				const FRotator FRot(0.f, CliffRng.FRandRange(0.f, 360.f), 0.f);
				if (UStaticMeshComponent* FernComp = AddWorldMesh(
					FString::Printf(TEXT("Prologue_MouthFern_%d"), Idx),
					AetheriaWorldConst::MeshKiteFern01,
					AetheriaWorldHelpers::ComputeGroundedCenterLoc(FernMesh, MouthFerns[Idx].X, MouthFerns[Idx].Y, CliffZ, FRot, FVector(FS), 4.f),
					FRot,
					FVector(FS),
					nullptr,
					nullptr,
					false))
				{
					FernComp->SetCastShadow(false);
				}
			}
		}
	}

	// 4. Panoramic Cliff Overlook Ledge (X = -2420 .. -1950, Y = -1250 .. -350, Z = CliffZ)
	// Low weathered border rocks on North & South overlook edges, leaving Waterfall Brink (-1980, -800, CliffZ) open!
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		for (int32 Step = 0; Step < 3; ++Step)
		{
			const float BX = -2360.f + static_cast<float>(Step) * 185.f;
			const float BY = CenterY + static_cast<float>(Side) * (465.f - static_cast<float>(Step) * 28.f);
			PlaceCliffRock(
				FString::Printf(TEXT("Prologue_OverlookParapet_%d_%d"), Side > 0 ? 1 : 0, Step),
				BX, BY, CliffZ - 45.f, 145.f, 1.15f,
				FRotator(CliffRng.FRandRange(-8.f, 8.f), CliffRng.FRandRange(0.f, 360.f), CliffRng.FRandRange(-8.f, 8.f)),
				true);
		}
	}

	// 5. Colossal Roaring Cliffside Waterfall (X = -1940 -> -1420, Y = -800, plunging from CliffZ down to WaterZ)
	// Pure multi-tier real Cascade foaming water spray & mist down wet Megascans cliff rocks (ZERO primitive Cylinder or Cube bars!)
	UMaterialInstanceDynamic* FoamWaterMID = CreateColorMID(FLinearColor(0.32f, 0.72f, 0.86f, 1.0f), false);
	if (FoamWaterMID)
	{
		FoamWaterMID->SetScalarParameterValue(TEXT("Roughness"), 0.08f);
		FoamWaterMID->SetScalarParameterValue(TEXT("Specular"), 0.88f);
	}
	UMaterialInterface* WaterCascadeMat = FoamWaterMID;

	const int32 NumTiers = 8;
	for (int32 Tier = 0; Tier < NumTiers; ++Tier)
	{
		const float Alpha = static_cast<float>(Tier) / static_cast<float>(NumTiers - 1);
		const float WX = FMath::Lerp(-1930.f, -1420.f, Alpha * Alpha * 0.45f + Alpha * 0.55f);
		const float WZ = FMath::Lerp(CliffZ - 45.f, WaterZ + 35.f, Alpha);
		const float SprayScale = FMath::Lerp(1.05f, 1.75f, Alpha);
		SpawnFireParticle(
			FString::Printf(TEXT("Prologue_WaterfallCascadeTier_%d_C"), Tier),
			AetheriaWorldConst::ParticleWaterBall,
			FVector(WX, CenterY, WZ),
			FVector(SprayScale));
		if ((Tier % 2) == 1)
		{
			SpawnFireParticle(
				FString::Printf(TEXT("Prologue_WaterfallCascadeTier_%d_L"), Tier),
				AetheriaWorldConst::ParticleWaterBall,
				FVector(WX - 25.f, CenterY - 120.f, WZ),
				FVector(SprayScale * 0.82f));
			SpawnFireParticle(
				FString::Printf(TEXT("Prologue_WaterfallCascadeTier_%d_R"), Tier),
				AetheriaWorldConst::ParticleWaterBall,
				FVector(WX - 25.f, CenterY + 120.f, WZ),
				FVector(SprayScale * 0.82f));
		}
	}

	// Churning white-water plunge pool foam disc at (-1420, -800, WaterZ + 8)
	AddWorldMesh(
		TEXT("Prologue_PlungePoolFoam"),
		AetheriaWorldConst::MeshCylinder,
		FVector(-1420.f, CenterY, WaterZ + 6.f),
		FRotator::ZeroRotator,
		FVector(7.2f, 8.8f, 0.08f),
		WaterCascadeMat,
		AetheriaWorldConst::MeshPlane,
		false);

	// Natural Compact Waterfall Foam & Spray Emitters (NO P_ky_aquaStorm giant cyclone blocking the cliff vista!)
	SpawnFireParticle(TEXT("Prologue_WaterfallBrinkSpray"), AetheriaWorldConst::ParticleWaterBall, FVector(-1905.f, CenterY, CliffZ - 110.f),     FVector(0.85f));
	SpawnFireParticle(TEXT("Prologue_WaterfallUpperSpray"), AetheriaWorldConst::ParticleWaterBall, FVector(-1790.f, CenterY, WaterZ + 1580.f),    FVector(1.05f));
	SpawnFireParticle(TEXT("Prologue_WaterfallMidSpray"),   AetheriaWorldConst::ParticleWaterBall, FVector(-1665.f, CenterY, WaterZ + 1040.f),    FVector(1.25f));
	SpawnFireParticle(TEXT("Prologue_WaterfallLowerSpray"), AetheriaWorldConst::ParticleWaterBall, FVector(-1535.f, CenterY, WaterZ + 480.f),     FVector(1.35f));
	SpawnFireParticle(TEXT("Prologue_PlungePoolSplash"),    AetheriaWorldConst::ParticleWaterBall, FVector(-1420.f, CenterY, WaterZ + 28.f),      FVector(1.65f));
	SpawnFireParticle(TEXT("Prologue_PlungePoolSplashL"),   AetheriaWorldConst::ParticleWaterBall, FVector(-1380.f, CenterY - 165.f, WaterZ + 18.f), FVector(1.15f));
	SpawnFireParticle(TEXT("Prologue_PlungePoolSplashR"),   AetheriaWorldConst::ParticleWaterBall, FVector(-1380.f, CenterY + 165.f, WaterZ + 18.f), FVector(1.15f));
	SpawnAmbientWindNiagara(TEXT("Prologue_PlungePoolMistDrift"), AetheriaWorldConst::NSWindFlows01, FVector(-1360.f, CenterY, WaterZ + 95.f), FVector(1.25f));

	// Soft aquatic mist illumination at the waterfall plunge pool
	AddAmbientFireLight(
		TEXT("Prologue_PlungePoolMistGlow"),
		FVector(-1420.f, CenterY, WaterZ + 240.f),
		580.f,
		8200.f,
		1800.f,
		FLinearColor(0.62f, 0.88f, 1.0f, 1.0f),
		0.10f,
		1.2f,
		true,
		0.14f);

	// Roaring Waterfall Ambient Spatial Audio Loop (owned by WorldBuilder component list)
	if (!IsRunningDedicatedServer())
	{
		const FString AudioPkg = FPackageName::ObjectPathToPackageName(FString(AetheriaWorldConst::SFXWaterfallLoop));
		if (!AudioPkg.IsEmpty() && FPackageName::DoesPackageExist(AudioPkg))
		{
			if (USoundBase* RoarSound = LoadObject<USoundBase>(nullptr, AetheriaWorldConst::SFXWaterfallLoop))
			{
				const FName AudioName = MakeUniqueObjectName(this, UAudioComponent::StaticClass(), TEXT("Prologue_WaterfallRoarAudio"));
				if (UAudioComponent* AudioComp = NewObject<UAudioComponent>(this, AudioName))
				{
					AudioComp->SetSound(RoarSound);
					AudioComp->bAutoActivate = true;
					AudioComp->bAutoDestroy = false;
					AudioComp->SetVolumeMultiplier(0.52f);
					AudioComp->SetPitchMultiplier(0.62f);
					AudioComp->bOverrideAttenuation = true;
					AudioComp->AttenuationOverrides.bAttenuate = true;
					AudioComp->AttenuationOverrides.bSpatialize = true;
					AudioComp->AttenuationOverrides.AttenuationShapeExtents = FVector(950.f, 0.f, 0.f);
					AudioComp->AttenuationOverrides.FalloffDistance = 4200.f;
					AudioComp->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
					AudioComp->SetWorldLocation(FVector(-1620.f, CenterY, WaterZ + 680.f));
					AudioComp->RegisterComponent();
					AudioComp->Play();
				}
			}
		}
	}
}

void AAetheriaWorldBuilder::BuildDragonAerie()
{
	// Flush curbed lakeside Dragon Aerie pavilion at (-3400, 2200) — right beside Mirror Lake & connected via King's Road!
	const float CenterX = -3400.f;
	const float CenterY =  2200.f;
	const float BaseZ   = SampleGroundZ(CenterX, CenterY, FallbackGroundZ);

	UMaterialInterface* StoneAltarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatAetheriaGothicStone
	});
	UMaterialInterface* PillarMat = LoadPBRMaterial({
		AetheriaWorldConst::MatPillarInst,
		AetheriaWorldConst::MatStoneWall
	});

	// Flush Gothic flagstone pavilion floor (BaseZ + 2.0f — ZERO abrupt floating slabs!) + stone curbs, steps & earth blends
	BuildTiledStoneFloor(
		TEXT("DragonAerie_DaisFloor"),
		CenterX,
		CenterY,
		420.f,
		420.f,
		420.f,
		2.f,
		AetheriaWorldConst::MeshDungeonFloor,
		StoneAltarMat,
		nullptr,
		0.f,
		false,
		BaseZ);

	AddCurbedFloorBorder(
		TEXT("DragonAerie_PlazaCurb"),
		CenterX,
		CenterY,
		635.f,
		635.f,
		BaseZ,
		false,
		false,
		true,
		true,
		false);

	UStaticMesh* AeriePedMesh   = LoadMeshSafe(AetheriaWorldConst::MeshStatuePedestal, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* AerieAltarMesh = LoadMeshSafe(AetheriaWorldConst::MeshMedievalAltar, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* SolidArcMesh   = LoadMeshSafe(AetheriaWorldConst::MeshGothicArc, AetheriaWorldConst::MeshRuinedArch);
	UStaticMesh* PillarMesh     = LoadMeshSafe(AetheriaWorldConst::MeshDungeonPillar, AetheriaWorldConst::MeshGothicPillar);

	const float AeriePedScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(AeriePedMesh, 72.f, 0.1f, 3.0f);
	const float AerieAltarScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(AerieAltarMesh, 120.f, 0.2f, 2.2f);
	const FVector PedLoc = AetheriaWorldHelpers::ComputeGroundedCenterLoc(
		AeriePedMesh, CenterX, CenterY, BaseZ + 2.f, FRotator::ZeroRotator, FVector(AeriePedScale), 4.f);
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

	// Cohesive ring of 6 grounded full-3D gothic arches & pillars forming the lakeside pavilion
	const float AerieArchScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SolidArcMesh, 410.f, 0.2f, 3.0f);
	const float AeriePillarScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PillarMesh, 390.f, 0.2f, 3.0f);
	for (int32 Idx = 0; Idx < 6; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 6.f) * UE_TWO_PI;
		const float PX = CenterX + FMath::Cos(Angle) * 560.f;
		const float PY = CenterY + FMath::Sin(Angle) * 560.f;
		const float PZ = SampleGroundZ(PX, PY, BaseZ) + 2.f;
		const FRotator RingRot(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f);
		const bool bUseArch = (Idx % 2 == 0);
		UStaticMesh* PieceMesh = bUseArch ? SolidArcMesh : PillarMesh;
		const float PieceScale = bUseArch ? AerieArchScale : AeriePillarScale;

		AddWorldMesh(
			FString::Printf(TEXT("DragonAerie_Arch_%d"), Idx),
			bUseArch ? AetheriaWorldConst::MeshGothicArc : AetheriaWorldConst::MeshDungeonPillar,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(PieceMesh, PX, PY, PZ, RingRot, FVector(PieceScale), 10.f),
			RingRot,
			FVector(PieceScale),
			bUseArch ? nullptr : PillarMat);
	}

	// 2 Beacon Braziers at the Aerie pavilion (SM_Fire_Pit + P_Torch_Fire + 850 lm)
	AddBrazierAssembly(TEXT("DragonAerie_BrazierL"), FVector(CenterX - 340.f, CenterY + 420.f, BaseZ + 2.f), 850.f, 1350.f, AetheriaWorldConst::MeshFirePit, 1.05f, 0.4f);
	AddBrazierAssembly(TEXT("DragonAerie_BrazierR"), FVector(CenterX + 340.f, CenterY + 420.f, BaseZ + 2.f), 850.f, 1350.f, AetheriaWorldConst::MeshFirePit, 1.05f, 2.2f);

	// 2 Destructible Runic Bind-Monoliths (LinkedQuestId = "Q_TameDragon") — grounded SM_Pillar
	const FLinearColor AzureBindGlow(0.22f, 0.95f, 1.35f, 1.0f);
	const FVector2D BindCoords[2] = {
		FVector2D(CenterX - 320.f, CenterY - 180.f),
		FVector2D(CenterX + 320.f, CenterY - 180.f)
	};
	const float BindScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PillarMesh, 250.f, 0.25f, 2.5f);
	for (int32 Idx = 0; Idx < 2; ++Idx)
	{
		const float BX = BindCoords[Idx].X;
		const float BY = BindCoords[Idx].Y;
		const float BZ = AetheriaWorldHelpers::ComputeGroundedZ(PillarMesh, BaseZ + 2.f, BindScale, 6.f);
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

	// Natural closed 360-degree lakeside boulder clusters sunk 35% with nestled bushes (ZERO KiteDemo cliff blocks or lone ruins!)
	FRandomStream LakeRockRng(34022);
	const FVector2D ShorelineClusters[] = {
		FVector2D(-2550.f,  1580.f),
		FVector2D(-1750.f,  1760.f),
		FVector2D( 1950.f,  1480.f),
		FVector2D( 2520.f,  -450.f),
		FVector2D( 2180.f, -2380.f),
		FVector2D(-1650.f, -2850.f)
	};
	for (int32 Idx = 0; Idx < UE_ARRAY_COUNT(ShorelineClusters); ++Idx)
	{
		AddBoulderCluster(
			FString::Printf(TEXT("MirrorLake_ShoreRocks_%d"), Idx),
			ShorelineClusters[Idx].X,
			ShorelineClusters[Idx].Y,
			LakeRockRng.RandRange(2, 4),
			145.f,
			290.f,
			LakeRockRng,
			true);
	}
}

void AAetheriaWorldBuilder::BuildSapphireWhisperingWoods()
{
	// X: -10800..-4200 (West) and 4200..10800 (East), Y: -7400..6400
	// Lush Blue-Leaf Full-Canopy Trees (glowing sapphire-cyan canopy MID!) + natural closed MWAM boulder clusters sunk 35% (ZERO orphan pillars or SM_DeadTree!)
	UMaterialInterface* CoverRockMat = LoadPBRMaterial({
		AetheriaWorldConst::MatMWAMCoverRocks,
		AetheriaWorldConst::MatNamaqualandCliff,
		AetheriaWorldConst::MatStoneWall
	});

	UStaticMesh* ClosedStoneMesh = LoadMeshSafe(AetheriaWorldConst::MeshMWAMStoneA, AetheriaWorldConst::MeshMWAMStoneB);
	if (MossyBouldersISM && ClosedStoneMesh)
	{
		MossyBouldersISM->SetStaticMesh(ClosedStoneMesh);
		if (CoverRockMat)
		{
			const int32 NumRockMats = FMath::Max(1, MossyBouldersISM->GetNumMaterials());
			for (int32 Idx = 0; Idx < NumRockMats; ++Idx)
			{
				MossyBouldersISM->SetMaterial(Idx, CoverRockMat);
			}
		}
	}

	// Build glowing Sapphire-Cyan Blue-Leaf HISM trees (large & medium Searsia full-canopy trees casting real shadows!)
	auto ApplyBlueLeafTintToHISM = [&](UHierarchicalInstancedStaticMeshComponent* TreeHISM)
	{
		if (!TreeHISM)
		{
			return;
		}
		const int32 NumMats = TreeHISM->GetNumMaterials();
		for (int32 SlotIdx = 0; SlotIdx < NumMats; ++SlotIdx)
		{
			UMaterialInterface* OrigMat = TreeHISM->GetMaterial(SlotIdx);
			if (!OrigMat)
			{
				continue;
			}
			const FString MatName = OrigMat->GetName();
			if (MatName.Contains(TEXT("Bark"), ESearchCase::IgnoreCase) ||
				MatName.Contains(TEXT("Trunk"), ESearchCase::IgnoreCase) ||
				MatName.Contains(TEXT("Wood"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			if (UMaterialInstanceDynamic* BlueLeafMID = UMaterialInstanceDynamic::Create(OrigMat, this))
			{
				const FLinearColor SapphireTint(0.16f, 0.56f, 1.00f, 1.0f);
				const FLinearColor CyanSubsurface(0.24f, 0.76f, 1.00f, 1.0f);

				// Apply known Namaqualand MM_MasterMat_Foliage_01a parameters + dynamic parameter scan for complete shader coverage
				BlueLeafMID->SetVectorParameterValue(TEXT("Tint"), SapphireTint);
				BlueLeafMID->SetVectorParameterValue(TEXT("Base Colour"), SapphireTint);
				BlueLeafMID->SetVectorParameterValue(TEXT("BaseColor"), SapphireTint);
				BlueLeafMID->SetVectorParameterValue(TEXT("SubsurfaceColor"), CyanSubsurface);
				BlueLeafMID->SetScalarParameterValue(TEXT("Hue Shift"), 0.28f);
				BlueLeafMID->SetScalarParameterValue(TEXT("Tint Hue Shift"), 0.28f);
				BlueLeafMID->SetScalarParameterValue(TEXT("Hue Shift Percentage"), 0.28f);

				TArray<FMaterialParameterInfo> VecInfos;
				TArray<FGuid> VecIds;
				OrigMat->GetAllVectorParameterInfo(VecInfos, VecIds);
				for (const FMaterialParameterInfo& Info : VecInfos)
				{
					const FString PName = Info.Name.ToString();
					if (PName.Contains(TEXT("Subsurface"), ESearchCase::IgnoreCase))
					{
						BlueLeafMID->SetVectorParameterValue(Info.Name, CyanSubsurface);
					}
					else if (PName.Contains(TEXT("Tint"), ESearchCase::IgnoreCase) ||
						PName.Contains(TEXT("Color"), ESearchCase::IgnoreCase) ||
						PName.Contains(TEXT("Colour"), ESearchCase::IgnoreCase))
					{
						BlueLeafMID->SetVectorParameterValue(Info.Name, SapphireTint);
					}
				}

				TArray<FMaterialParameterInfo> ScalarInfos;
				TArray<FGuid> ScalarIds;
				OrigMat->GetAllScalarParameterInfo(ScalarInfos, ScalarIds);
				for (const FMaterialParameterInfo& Info : ScalarInfos)
				{
					const FString PName = Info.Name.ToString();
					if (PName.Contains(TEXT("Hue"), ESearchCase::IgnoreCase))
					{
						BlueLeafMID->SetScalarParameterValue(Info.Name, 0.28f);
					}
				}

				DynamicMaterials.Add(BlueLeafMID);
				TreeHISM->SetMaterial(SlotIdx, BlueLeafMID);
			}
		}
	};

	UHierarchicalInstancedStaticMeshComponent* BlueTreesLargeHISM = CreateFoliageHISM(
		TEXT("BlueLeafWoods_TreesLargeHISM"),
		AetheriaWorldConst::MeshSearsiaLarge,
		nullptr,
		true,
		15000,
		24000,
		true);
	UHierarchicalInstancedStaticMeshComponent* BlueTreesMedHISM = CreateFoliageHISM(
		TEXT("BlueLeafWoods_TreesMedHISM"),
		AetheriaWorldConst::MeshSearsiaMedium,
		nullptr,
		true,
		13000,
		21000,
		true);
	UHierarchicalInstancedStaticMeshComponent* BlueFlowersHISM = CreateFoliageHISM(
		TEXT("BlueLeafWoods_FlowersHISM"),
		AetheriaWorldConst::MeshFlowerHeliophila,
		nullptr,
		false,
		7500,
		11500,
		false);
	UHierarchicalInstancedStaticMeshComponent* BlueBushesHISM = CreateFoliageHISM(
		TEXT("BlueLeafWoods_BushesHISM"),
		AetheriaWorldConst::MeshWildRooibosBush,
		nullptr,
		false,
		7500,
		11500,
		false);

	ApplyBlueLeafTintToHISM(BlueTreesLargeHISM);
	ApplyBlueLeafTintToHISM(BlueTreesMedHISM);

	UStaticMesh* LargeTreeMesh = LoadMeshSafe(AetheriaWorldConst::MeshSearsiaLarge, nullptr);
	UStaticMesh* MedTreeMesh   = LoadMeshSafe(AetheriaWorldConst::MeshSearsiaMedium, nullptr);
	UStaticMesh* FlowerMesh    = LoadMeshSafe(AetheriaWorldConst::MeshFlowerHeliophila, nullptr);
	UStaticMesh* BushMesh      = LoadMeshSafe(AetheriaWorldConst::MeshWildRooibosBush, nullptr);

	const float RockUnit      = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ClosedStoneMesh, 190.f, 0.15f, 2.4f);
	const float LargeTreeUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(LargeTreeMesh, 760.f, 0.25f, 2.6f);
	const float MedTreeUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(MedTreeMesh, 540.f, 0.25f, 2.6f);
	const float FlowerUnit    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(FlowerMesh, 52.f, 0.2f, 2.4f);
	const float BushUnit      = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BushMesh, 115.f, 0.2f, 2.4f);

	FRandomStream Rng(133742);

	auto IsExcludedWoodsClearing = [&](float X, float Y) -> bool
	{
		if (IsInsideNoGrassZone(X, Y))
		{
			return true;
		}
		const FVector2D P(X, Y);
		if (FVector2D::Distance(P, FVector2D(-6200.f,  1200.f)) < 580.f) return true;
		if (FVector2D::Distance(P, FVector2D( 6400.f,  3200.f)) < 580.f) return true;
		if (FVector2D::Distance(P, FVector2D( 7200.f, -1400.f)) < 580.f) return true;
		return false;
	};

	// 1. Natural clusters of closed 360-degree boulders (sunk 35% into terrain) nestled with bushes
	TArray<FTransform> BoulderTransforms;
	TArray<FTransform> BushTransforms;
	BoulderTransforms.Reserve(110);
	BushTransforms.Reserve(180);

	int32 ClustersPlaced = 0;
	int32 Attempts = 0;
	while (ClustersPlaced < 34 && Attempts < 450)
	{
		++Attempts;
		const bool bWest = (ClustersPlaced % 2 == 0);
		const float CX = bWest ? Rng.FRandRange(-10400.f, -4300.f) : Rng.FRandRange(4300.f, 10400.f);
		const float CY = Rng.FRandRange(-7000.f, 6000.f);
		if (IsExcludedWoodsClearing(CX, CY))
		{
			continue;
		}

		++ClustersPlaced;
		const int32 RocksInCluster = Rng.RandRange(2, 4);
		for (int32 RIdx = 0; RIdx < RocksInCluster; ++RIdx)
		{
			const float Dist = (RIdx == 0) ? 0.f : Rng.FRandRange(95.f, 240.f);
			const float Angle = Rng.FRandRange(0.f, UE_TWO_PI);
			const float X = CX + FMath::Cos(Angle) * Dist;
			const float Y = CY + FMath::Sin(Angle) * Dist;
			const float SizeMult = (RIdx == 0) ? Rng.FRandRange(0.95f, 1.45f) : Rng.FRandRange(0.48f, 0.85f);
			const float S = RockUnit * SizeMult;
			const float SZ = S * Rng.FRandRange(0.82f, 1.08f);
			const float TargetH = 190.f * SizeMult;
			const float GZ = AetheriaWorldHelpers::ComputeGroundedZ(ClosedStoneMesh, SampleGroundZ(X, Y, FallbackGroundZ), SZ, TargetH * 0.35f);
			const FRotator Rot(Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-8.f, 8.f));
			BoulderTransforms.Emplace(Rot, FVector(X, Y, GZ), FVector(S, S * Rng.FRandRange(0.85f, 1.20f), SZ));
		}

		// Bush nestled right against the boulder cluster
		if (BushMesh)
		{
			const float BAngle = Rng.FRandRange(0.f, UE_TWO_PI);
			const float BX = CX + FMath::Cos(BAngle) * Rng.FRandRange(140.f, 260.f);
			const float BY = CY + FMath::Sin(BAngle) * Rng.FRandRange(140.f, 260.f);
			const float BS = BushUnit * Rng.FRandRange(0.85f, 1.35f);
			const float BGZ = AetheriaWorldHelpers::ComputeGroundedZ(BushMesh, SampleGroundZ(BX, BY, FallbackGroundZ), BS, 10.f);
			BushTransforms.Emplace(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(BX, BY, BGZ), FVector(BS));
		}
	}

	// 2. 170 Glowing Sapphire Blue-Leaf Full-Canopy Trees + Wildflower Understory
	TArray<FTransform> LargeTreeTransforms;
	TArray<FTransform> MedTreeTransforms;
	TArray<FTransform> FlowerTransforms;
	LargeTreeTransforms.Reserve(90);
	MedTreeTransforms.Reserve(90);
	FlowerTransforms.Reserve(260);

	Attempts = 0;
	while ((LargeTreeTransforms.Num() + MedTreeTransforms.Num()) < 170 && Attempts < 1400)
	{
		++Attempts;
		const bool bWest = ((LargeTreeTransforms.Num() + MedTreeTransforms.Num()) % 2 == 0);
		const float X = bWest ? Rng.FRandRange(-10600.f, -4150.f) : Rng.FRandRange(4150.f, 10600.f);
		const float Y = Rng.FRandRange(-7200.f, 6200.f);
		if (IsExcludedWoodsClearing(X, Y))
		{
			continue;
		}

		const float TerrainZ = SampleGroundZ(X, Y, FallbackGroundZ);
		const bool bLarge = (Rng.FRand() < 0.52f);
		UStaticMesh* TreeMesh = bLarge ? LargeTreeMesh : MedTreeMesh;
		if (!TreeMesh)
		{
			continue;
		}

		const float BaseUnit = bLarge ? LargeTreeUnit : MedTreeUnit;
		const float TS = BaseUnit * Rng.FRandRange(0.82f, 1.32f);
		const float TSZ = TS * Rng.FRandRange(0.90f, 1.18f);
		const float TZ = AetheriaWorldHelpers::ComputeGroundedZ(TreeMesh, TerrainZ, TSZ, 18.f);
		const FRotator TRot(Rng.FRandRange(-3.f, 3.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-3.f, 3.f));

		if (bLarge)
		{
			LargeTreeTransforms.Emplace(TRot, FVector(X, Y, TZ), FVector(TS, TS, TSZ));
		}
		else
		{
			MedTreeTransforms.Emplace(TRot, FVector(X, Y, TZ), FVector(TS, TS, TSZ));
		}

		// Understory violet-blue wildflowers & occasional bush around tree base
		if (FlowerMesh && FlowerTransforms.Num() < 260)
		{
			for (int32 FIdx = 0; FIdx < 2; ++FIdx)
			{
				const float FAngle = Rng.FRandRange(0.f, UE_TWO_PI);
				const float FDist = Rng.FRandRange(120.f, 310.f);
				const float FX = X + FMath::Cos(FAngle) * FDist;
				const float FY = Y + FMath::Sin(FAngle) * FDist;
				const float FS = FlowerUnit * Rng.FRandRange(0.80f, 1.35f);
				const float FZ = AetheriaWorldHelpers::ComputeGroundedZ(FlowerMesh, SampleGroundZ(FX, FY, TerrainZ), FS, 4.f);
				FlowerTransforms.Emplace(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(FX, FY, FZ), FVector(FS));
			}
		}
		if (BushMesh && BushTransforms.Num() < 180 && Rng.FRand() < 0.45f)
		{
			const float BAngle = Rng.FRandRange(0.f, UE_TWO_PI);
			const float BX = X + FMath::Cos(BAngle) * Rng.FRandRange(145.f, 290.f);
			const float BY = Y + FMath::Sin(BAngle) * Rng.FRandRange(145.f, 290.f);
			const float BS = BushUnit * Rng.FRandRange(0.80f, 1.25f);
			const float BZ = AetheriaWorldHelpers::ComputeGroundedZ(BushMesh, SampleGroundZ(BX, BY, TerrainZ), BS, 10.f);
			BushTransforms.Emplace(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(BX, BY, BZ), FVector(BS));
		}
	}

	if (MossyBouldersISM && BoulderTransforms.Num() > 0)
	{
		MossyBouldersISM->AddInstances(BoulderTransforms, false, true, false);
	}
	if (BlueTreesLargeHISM && LargeTreeTransforms.Num() > 0)
	{
		BlueTreesLargeHISM->AddInstances(LargeTreeTransforms, false, true, false);
	}
	if (BlueTreesMedHISM && MedTreeTransforms.Num() > 0)
	{
		BlueTreesMedHISM->AddInstances(MedTreeTransforms, false, true, false);
	}
	if (BlueFlowersHISM && FlowerTransforms.Num() > 0)
	{
		BlueFlowersHISM->AddInstances(FlowerTransforms, false, true, false);
	}
	if (BlueBushesHISM && BushTransforms.Num() > 0)
	{
		BlueBushesHISM->AddInstances(BushTransforms, false, true, false);
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
	// Sample max ground height across the entire 3000x3000 courtyard so Highkeep_Floor is 100% laser-flat AND zero terrain triangles poke through!
	const float CX = 0.f;
	const float CY = -9800.f;
	float ArenaZ = SampleGroundZ(CX, CY, FallbackGroundZ);
	for (int32 sy = -2; sy <= 2; ++sy)
	{
		for (int32 sx = -2; sx <= 2; ++sx)
		{
			ArenaZ = FMath::Max(ArenaZ, SampleGroundZ(CX + sx * 750.f, CY + sy * 750.f, FallbackGroundZ));
		}
	}
	ArenaZ += 8.f;

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
	UStaticMesh* Section1Mesh   = LoadMeshSafe(AetheriaWorldConst::MeshGothicSection1, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* Column1Mesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicColumn1, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* BallistaMesh   = LoadMeshSafe(AetheriaWorldConst::MeshGothicBallista, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* StepsMesh      = LoadMeshSafe(AetheriaWorldConst::MeshGothicSteps, AetheriaWorldConst::MeshEdgeStones);
	UStaticMesh* Column3Mesh    = LoadMeshSafe(AetheriaWorldConst::MeshGothicColumn3, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* KeepMesh       = LoadMeshSafe(AetheriaWorldConst::MeshGothicBuilding09, AetheriaWorldConst::MeshCitadelFortress);

	const float GateScale      = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GateMesh, 680.f, 0.2f, 3.5f);
	const float PedestalScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PedestalMesh, 165.f, 0.1f, 3.5f);
	const float StatueScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StatueMesh, 320.f, 0.1f, 3.5f);
	const float Section1Scale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Section1Mesh, 560.f, 0.2f, 3.2f);
	const float Column1Scale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Column1Mesh, 580.f, 0.2f, 3.2f);
	const float BallistaScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BallistaMesh, 185.f, 0.1f, 3.5f);
	const float StepsScale     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StepsMesh, 115.f, 0.1f, 3.0f);
	const float Column3Scale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Column3Mesh, 520.f, 0.2f, 3.2f);

	// 1. Laser-Flat Flush Gothic Cobblestone Boss Arena floor (bFollowTerrain=false at MaxArenaZ -> zero stepped tile facets & zero terrain poke-through!)
	BuildTiledStoneFloor(
		TEXT("Highkeep_Floor"),
		CX,
		CY,
		1500.f,
		1500.f,
		500.f,
		10.f,
		AetheriaWorldConst::MeshDungeonFloor,
		BrickFloorMat,
		StoneAltarMat,
		640.f,
		false,
		ArenaZ);

	AddCurbedFloorBorder(
		TEXT("Highkeep_ArenaCurb"),
		CX,
		CY,
		1755.f,
		1755.f,
		ArenaZ,
		true,
		true,
		false,
		false,
		false);

	// 2. Grounded Gothic Castle Keep (SM_Gothic_building_09) behind Malakor (grounded across all 4 corners so zero edges float!)
	const float KeepY = -12450.f;
	float KeepGZ = SampleGroundZ(CX, KeepY, ArenaZ);
	KeepGZ = FMath::Min(KeepGZ, SampleGroundZ(CX - 820.f, KeepY - 460.f, ArenaZ));
	KeepGZ = FMath::Min(KeepGZ, SampleGroundZ(CX + 820.f, KeepY - 460.f, ArenaZ));
	KeepGZ = FMath::Min(KeepGZ, SampleGroundZ(CX - 820.f, KeepY + 460.f, ArenaZ));
	KeepGZ = FMath::Min(KeepGZ, SampleGroundZ(CX + 820.f, KeepY + 460.f, ArenaZ));
	const float KeepUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(KeepMesh, 1340.f, 0.25f, 1.35f);
	const FVector KeepScale(KeepUnit * 1.18f, KeepUnit * 1.18f, FMath::Min(KeepUnit, 1.35f));
	AddWorldMesh(
		TEXT("Highkeep_CastleKeep"),
		AetheriaWorldConst::MeshGothicBuilding09,
		AetheriaWorldHelpers::ComputeGroundedCenterLoc(KeepMesh, CX, KeepY, KeepGZ, FRotator::ZeroRotator, KeepScale, 36.f),
		FRotator::ZeroRotator,
		KeepScale);

	// 3. Grand Northern Gothic Castle Gate at Y = -8050 facing King's Road & Mirror Lake
	const float GateY = -8050.f;
	const float GateGroundZ = SampleGroundZ(CX, GateY, ArenaZ) + 2.f;
	AddWorldMesh(
		TEXT("Highkeep_GothicGate"),
		AetheriaWorldConst::MeshGothicGate,
		FVector(CX, GateY, AetheriaWorldHelpers::ComputeGroundedZ(GateMesh, GateGroundZ, GateScale, 10.f)),
		FRotator(0.f, 0.f, 0.f),
		FVector(GateScale));

	// Approach Steps at North Gate & South Keep Terrace
	const float NorthStepsZ = SampleGroundZ(CX, -7820.f, GateGroundZ) + 2.f;
	AddWorldMesh(
		TEXT("Highkeep_NorthSteps"),
		AetheriaWorldConst::MeshGothicSteps,
		FVector(CX, -7820.f, AetheriaWorldHelpers::ComputeGroundedZ(StepsMesh, NorthStepsZ, StepsScale, 8.f)),
		FRotator(0.f, 0.f, 0.f),
		FVector(StepsScale * 1.4f, StepsScale * 1.2f, StepsScale),
		StoneAltarMat);

	const float SouthStepsZ = SampleGroundZ(CX, CY - 1450.f, ArenaZ) + 2.f;
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
		const float SY = -8180.f;
		const float GroundZ = SampleGroundZ(SX, SY, ArenaZ) + 2.f;
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
		const float BGZ = SampleGroundZ(BX, BY, ArenaZ) + 2.f;
		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_Ballista_%d"), Idx),
			AetheriaWorldConst::MeshGothicBallista,
			FVector(BX, BY, AetheriaWorldHelpers::ComputeGroundedZ(BallistaMesh, BGZ, BallistaScale, 6.f)),
			FRotator(0.f, BallistaYaw[Idx], 0.f),
			FVector(BallistaScale));
	}

	// 4. Enclosed Castle Courtyard Curtain Walls (SM_Section1 + SM_Column1 bastions framing the castle courtyard!)
	const float WallHalf = 1755.f;
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		// West & East fortress walls (4 sections + 5 columns per flank)
		for (int32 Seg = 0; Seg < 4; ++Seg)
		{
			const float WY = (CY - 1320.f) + static_cast<float>(Seg) * 880.f;
			const float WX = CX + static_cast<float>(Side) * WallHalf;
			const float WZ = SampleGroundZ(WX, WY, ArenaZ) + 2.f;
			const FRotator WallRot(0.f, Side > 0 ? 90.f : -90.f, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("Highkeep_CurtainWall_%d_%d"), Side > 0 ? 1 : 0, Seg),
				AetheriaWorldConst::MeshGothicSection1,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(Section1Mesh, WX, WY, WZ, WallRot, FVector(Section1Scale), 12.f),
				WallRot,
				FVector(Section1Scale),
				StoneWallMat);
		}
		for (int32 Col = 0; Col < 5; ++Col)
		{
			const float CYPos = (CY - 1755.f) + static_cast<float>(Col) * 877.5f;
			const float CXPos = CX + static_cast<float>(Side) * WallHalf;
			const float CZPos = SampleGroundZ(CXPos, CYPos, ArenaZ) + 2.f;
			AddWorldMesh(
				FString::Printf(TEXT("Highkeep_BastionCol_%d_%d"), Side > 0 ? 1 : 0, Col),
				AetheriaWorldConst::MeshGothicColumn1,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(Column1Mesh, CXPos, CYPos, CZPos, FRotator::ZeroRotator, FVector(Column1Scale), 12.f),
				FRotator::ZeroRotator,
				FVector(Column1Scale),
				PillarMat);
		}

		// North curtain wings flanking SM_Gate
		const float WingX = CX + static_cast<float>(Side) * 1160.f;
		const float WingZ = SampleGroundZ(WingX, GateY, ArenaZ) + 2.f;
		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_NorthWing_%d"), Side > 0 ? 1 : 0),
			AetheriaWorldConst::MeshGothicSection1,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(Section1Mesh, WingX, GateY, WingZ, FRotator::ZeroRotator, FVector(Section1Scale), 12.f),
			FRotator::ZeroRotator,
			FVector(Section1Scale),
			StoneWallMat);

		// South curtain wings flanking the Castle Keep
		const float SouthWingY = CY - WallHalf;
		const float SouthWingZ = SampleGroundZ(WingX, SouthWingY, ArenaZ) + 2.f;
		AddWorldMesh(
			FString::Printf(TEXT("Highkeep_SouthWing_%d"), Side > 0 ? 1 : 0),
			AetheriaWorldConst::MeshGothicSection1,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(Section1Mesh, WingX, SouthWingY, SouthWingZ, FRotator(0.f, 180.f, 0.f), FVector(Section1Scale), 12.f),
			FRotator(0.f, 180.f, 0.f),
			FVector(Section1Scale),
			StoneWallMat);
	}

	// 5. 6 Perimeter Citadel War Braziers + 2 Heroic Flanking Boss Braziers framing Malakor (P_Torch_Fire + SpecularScale=0.14)
	for (int32 Idx = 0; Idx < 6; ++Idx)
	{
		const float Angle = (static_cast<float>(Idx) / 6.f) * UE_TWO_PI + 0.52f;
		const float BX = CX + FMath::Cos(Angle) * 1380.f;
		const float BY = CY + FMath::Sin(Angle) * 1380.f;
		const float BZ = SampleGroundZ(BX, BY, ArenaZ) + 2.f;

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
		const float BZ = SampleGroundZ(BX, BY, ArenaZ) + 2.f;
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
		const float PZ = SampleGroundZ(PX, PY, ArenaZ) + 2.f;

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
	// Dark Fully Enclosed Torchlit Gothic Dungeon ("Ember Crypt") at (9800, -2400) with Narrow Recessed Molten Lava Cracks
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

	UStaticMesh* CryptFloorMesh  = LoadMeshSafe(AetheriaWorldConst::MeshCryptFloor, AetheriaWorldConst::MeshDungeonFloor);
	UStaticMesh* CryptWallMesh   = LoadMeshSafe(AetheriaWorldConst::MeshCryptWallA, AetheriaWorldConst::MeshDungeonWallA);
	UStaticMesh* CryptDoorMesh   = LoadMeshSafe(AetheriaWorldConst::MeshCryptDoorway, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* CryptPillarMesh = LoadMeshSafe(AetheriaWorldConst::MeshCryptPillar, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* Pillar2Mesh     = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar2, AetheriaWorldConst::MeshCryptPillar);
	UStaticMesh* BowlMesh        = LoadMeshSafe(AetheriaWorldConst::MeshFirePit, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* VaultArchMesh   = LoadMeshSafe(AetheriaWorldConst::MeshCryptCeilingArch, AetheriaWorldConst::MeshGothicArc);
	UStaticMesh* RoofFlatMesh    = LoadMeshSafe(AetheriaWorldConst::MeshCryptCeilingFlat, AetheriaWorldConst::MeshCryptFloor);
	UStaticMesh* AltarMesh       = LoadMeshSafe(AetheriaWorldConst::MeshMedievalAltar, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* CoffinMesh      = LoadMeshSafe(AetheriaWorldConst::MeshStoneCoffin, AetheriaWorldConst::MeshMedievalAltar);

	const float WallSecScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CryptWallMesh, 580.f, 0.2f, 3.5f);
	const float DoorwayScale  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CryptDoorMesh, 580.f, 0.2f, 3.5f);
	const float PillarScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CryptPillarMesh, 600.f, 0.2f, 3.5f);
	const float SconceP2Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Pillar2Mesh, 210.f, 0.1f, 3.2f);
	const float BowlScale     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BowlMesh, 32.f, 0.1f, 2.5f);
	const float VaultScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(VaultArchMesh, 420.f, 0.2f, 3.5f);
	const float AltarScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(AltarMesh, 128.f, 0.2f, 2.0f);
	const float CoffinScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CoffinMesh, 115.f, 0.2f, 2.5f);

	// 1. Calibrated Churning Volcanic Magma Sub-Floor (M_AetheriaFireball shader visible ONLY through narrow irregular recessed basalt fissures!)
	LavaFloorMID = nullptr;
	if (UMaterialInterface* FireballShader = LoadPBRMaterial({ AetheriaWorldConst::MatAetheriaFireball }))
	{
		LavaFloorMID = UMaterialInstanceDynamic::Create(FireballShader, this);
		if (LavaFloorMID)
		{
			LavaFloorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.88f, 0.14f, 0.015f, 1.0f));
			LavaFloorMID->SetVectorParameterValue(TEXT("AccentColor"), FLinearColor(1.00f, 0.42f, 0.045f, 1.0f));
			LavaFloorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.95f, 0.22f, 0.02f, 1.0f));
			LavaFloorMID->SetScalarParameterValue(TEXT("EmissiveBoost"), 1.55f);
			DynamicMaterials.Add(LavaFloorMID);
		}
	}
	if (!LavaFloorMID)
	{
		LavaFloorMID = CreateColorMID(FLinearColor(1.65f, 0.18f, 0.012f, 1.0f), true);
	}
	UMaterialInterface* ActiveLavaMat = LavaFloorMID;
	if (UStaticMeshComponent* LavaPlaneComp = AddWorldMesh(
		TEXT("EmberCrypt_MoltenCrackSubFloor"),
		AetheriaWorldConst::MeshPlane,
		FVector(CX, CY, BaseZ + 4.f),
		FRotator::ZeroRotator,
		FVector(18.5f, 22.0f, 1.f),
		ActiveLavaMat,
		AetheriaWorldConst::MeshPlane,
		true))
	{
		LavaPlaneComp->bAffectDynamicIndirectLighting = false;
	}

	// 2. Staggered Running-Bond Dark Basalt Dungeon Floor Slabs with Narrow Irregular Molten Fissures & Volcanic Rubble!
	const float WalkwayTopZ = BaseZ + 22.f;
	if (CryptFloorMesh)
	{
		const FBox SlabBox = AetheriaWorldHelpers::GetSafeBoundingBox(CryptFloorMesh);
		const FVector SlabSize = SlabBox.GetSize();
		const FVector SlabCenter = SlabBox.GetCenter();
		const float GridPitchX = 365.f;
		const float GridPitchY = 412.f;

		for (int32 GY = -2; GY <= 2; ++GY)
		{
			const float RowStaggerX = ((GY & 1) != 0) ? 92.f : -64.f;
			for (int32 GX = -3; GX <= 3; ++GX)
			{
				const float TX = CX + static_cast<float>(GX) * GridPitchX + RowStaggerX;
				if (TX < CX - 960.f || TX > CX + 960.f)
				{
					continue;
				}
				const float TY = CY + static_cast<float>(GY) * GridPitchY;
				// Vary crack widths organically (some joints tight/closed, selected diagonal veins 16..22 cm open!)
				const int32 Hash = FMath::Abs(GX * 17 + GY * 31);
				const float GapX = ((Hash % 3) == 0) ? 20.f : 4.f;
				const float GapY = (((Hash + 1) % 3) == 0) ? 20.f : 5.f;
				const float ScaleX = (GridPitchX - GapX) / FMath::Max(10.f, SlabSize.X);
				const float ScaleY = (GridPitchY - GapY) / FMath::Max(10.f, SlabSize.Y);
				const float ScaleZ = FMath::Clamp(24.f / FMath::Max(6.f, SlabSize.Z), 0.30f, 1.8f);
				const FVector SlabScale(ScaleX, ScaleY, ScaleZ);

				const FRotator SlabRot(0.f, static_cast<float>(((GX * 5 + GY * 9) & 3) * 90), 0.f);
				const FVector LocalTopCenter(SlabCenter.X * ScaleX, SlabCenter.Y * ScaleY, SlabBox.Max.Z * ScaleZ);
				const FVector SlabLoc = FVector(TX, TY, WalkwayTopZ) - SlabRot.RotateVector(LocalTopCenter);
				UMaterialInterface* SlabMat = ((Hash & 1) == 0) ? CryptWallMat : StoneAltarMat;

				AddWorldMesh(
					FString::Printf(TEXT("EmberCrypt_BasaltSlab_%d_%d"), GX + 3, GY + 2),
					AetheriaWorldConst::MeshCryptFloor,
					SlabLoc,
					SlabRot,
					SlabScale,
					SlabMat,
					AetheriaWorldConst::MeshDungeonFloor,
					true);
			}
		}
	}

	// 3. Fully Enclosed Inward-Facing Dark Crypt Perimeter Walls (tight pitch blocks all outside sunlight!)
	const float HalfW = 860.f;
	const float HalfL = 1040.f;

	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		for (int32 Seg = -2; Seg <= 2; ++Seg)
		{
			const float WX = CX + static_cast<float>(Side) * HalfW;
			const float WY = CY + static_cast<float>(Seg) * 425.f;
			// SM_Crypt_Wall_Decorative_A local sculpted front normal is -Y -> Yaw=-90 faces -X (inward from +X), Yaw=+90 faces +X (inward from -X)
			const FRotator WallRot(0.f, Side > 0 ? -90.f : 90.f, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_SideWall_%d_%d"), Side > 0 ? 1 : 0, Seg + 2),
				AetheriaWorldConst::MeshCryptWallA,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(CryptWallMesh, WX, WY, BaseZ, WallRot, FVector(WallSecScale * 1.12f, WallSecScale, WallSecScale), 14.f),
				WallRot,
				FVector(WallSecScale * 1.12f, WallSecScale, WallSecScale),
				CryptWallMat);
		}

		// Heavy corner & bay pilasters framing torch sconces
		for (int32 ColIdx = -2; ColIdx <= 2; ++ColIdx)
		{
			const float ColX = CX + static_cast<float>(Side) * (HalfW - 42.f);
			const float ColY = CY + static_cast<float>(ColIdx) * 510.f;
			const FRotator ColRot(0.f, ColIdx * 45.f, 0.f);
			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_Pilaster_%d_%d"), Side > 0 ? 1 : 0, ColIdx + 2),
				AetheriaWorldConst::MeshCryptPillar,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(CryptPillarMesh, ColX, ColY, BaseZ, ColRot, FVector(PillarScale), 12.f),
				ColRot,
				FVector(PillarScale),
				PillarMat);
		}
	}

	// Solid Enclosed South Rear Apse Wall (Yaw=180 faces +Y inward toward player)
	for (int32 Seg = 0; Seg < 4; ++Seg)
	{
		const float WX = CX - 645.f + static_cast<float>(Seg) * 430.f;
		const FRotator ApseRot(0.f, 180.f, 0.f);
		AddWorldMesh(
			FString::Printf(TEXT("EmberCrypt_SouthWall_%d"), Seg),
			AetheriaWorldConst::MeshCryptWallA,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(CryptWallMesh, WX, CY - HalfL, BaseZ, ApseRot, FVector(WallSecScale * 1.12f, WallSecScale, WallSecScale), 14.f),
			ApseRot,
			FVector(WallSecScale * 1.12f, WallSecScale, WallSecScale),
			CryptWallMat);
	}

	// North Entrance Wall with Grand SM_Crypt_Doorway Arch (Yaw=0 faces -Y inward into the dark dungeon)
	{
		const FRotator NorthRot(0.f, 0.f, 0.f);
		const float WingXs[2] = { CX - 610.f, CX + 610.f };
		for (int32 WIdx = 0; WIdx < 2; ++WIdx)
		{
			AddWorldMesh(
				FString::Printf(TEXT("EmberCrypt_NorthWall_%d"), WIdx),
				AetheriaWorldConst::MeshCryptWallA,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(CryptWallMesh, WingXs[WIdx], CY + HalfL, BaseZ, NorthRot, FVector(WallSecScale * 1.25f, WallSecScale, WallSecScale), 14.f),
				NorthRot,
				FVector(WallSecScale * 1.25f, WallSecScale, WallSecScale),
				CryptWallMat);
		}
		AddWorldMesh(
			TEXT("EmberCrypt_GrandDoorway"),
			AetheriaWorldConst::MeshCryptDoorway,
			AetheriaWorldHelpers::ComputeGroundedCenterLoc(CryptDoorMesh, CX, CY + HalfL, BaseZ, NorthRot, FVector(DoorwayScale), 12.f),
			NorthRot,
			FVector(DoorwayScale),
			CryptWallMat);
	}

	// Inset Opaque Basalt Light-Blocking Liner (blocks ECC_Camera!) + Outward-Cleared Volcanic Caldera Crag Exterior (zero boulders bulging inside!)
	{
		const float ShellHeight = 570.f;
		const float ShellMidZ = BaseZ + ShellHeight * 0.5f - 10.f;
		auto AddCryptShell = [&](const FString& ShellName, const FVector& Loc, const FVector& Scale)
		{
			if (UStaticMeshComponent* ShellComp = AddWorldMesh(
				ShellName,
				AetheriaWorldConst::MeshCube,
				Loc,
				FRotator::ZeroRotator,
				Scale,
				CryptWallMat,
				AetheriaWorldConst::MeshCube,
				true))
			{
				ShellComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
			}
		};
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddCryptShell(
				FString::Printf(TEXT("EmberCrypt_OuterShellSide_%d"), Side > 0 ? 1 : 0),
				FVector(CX + static_cast<float>(Side) * (HalfW + 32.f), CY, ShellMidZ),
				FVector(0.45f, (HalfL * 2.f + 90.f) / 100.f, ShellHeight / 100.f));
		}
		AddCryptShell(
			TEXT("EmberCrypt_OuterShellSouth"),
			FVector(CX, CY - HalfL - 32.f, ShellMidZ),
			FVector((HalfW * 2.f + 90.f) / 100.f, 0.45f, ShellHeight / 100.f));
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddCryptShell(
				FString::Printf(TEXT("EmberCrypt_OuterShellNorthWing_%d"), Side > 0 ? 1 : 0),
				FVector(CX + static_cast<float>(Side) * 605.f, CY + HalfL + 30.f, ShellMidZ),
				FVector(6.2f, 0.45f, ShellHeight / 100.f));
		}
		AddCryptShell(
			TEXT("EmberCrypt_OuterShellRoofCap"),
			FVector(CX, CY, BaseZ + 568.f),
			FVector((HalfW * 2.f + 80.f) / 100.f, (HalfL * 2.f + 80.f) / 100.f, 0.45f));

		// Clad the crypt exterior & roof in jagged 3D Megascans volcanic caldera crag boulders offset strictly OUTSIDE the interior hall!
		static const TCHAR* CalderaBoulders[5] = {
			AetheriaWorldConst::MeshNamaqualandBoulderA,
			AetheriaWorldConst::MeshNamaqualandBoulderB,
			AetheriaWorldConst::MeshNamaqualandBoulder02,
			AetheriaWorldConst::MeshNamaqualandBoulder03,
			AetheriaWorldConst::MeshNamaqualandBoulder04
		};
		UMaterialInterface* BasaltRockMat = LoadPBRMaterial({
			AetheriaWorldConst::MatMWAMCoverRocks,
			AetheriaWorldConst::MatCryptWallA,
			AetheriaWorldConst::MatStoneWall
		});
		FRandomStream CragRng(90421);
		auto PlaceCalderaCrag = [&](const FString& Name, float X, float Y, float BottomZ, float TargetH, float WidthMult, bool bCollide)
		{
			const TCHAR* RockPath = CalderaBoulders[CragRng.RandRange(0, 4)];
			UStaticMesh* RockMesh = LoadMeshSafe(RockPath, AetheriaWorldConst::MeshMWAMStoneA);
			if (!RockMesh) { return; }
			const float Unit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(RockMesh, TargetH, 0.25f, 22.0f);
			const FVector RScale(Unit * WidthMult, Unit * WidthMult * CragRng.FRandRange(0.86f, 1.08f), Unit);
			const FRotator RRot(CragRng.FRandRange(-8.f, 8.f), CragRng.FRandRange(0.f, 360.f), CragRng.FRandRange(-8.f, 8.f));
			const bool bFallback = RockMesh->GetPathName().Contains(TEXT("SM_MWAM_Stone"));
			AddWorldMesh(
				Name,
				RockPath,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(RockMesh, X, Y, BottomZ, RRot, RScale, 0.f),
				RRot,
				RScale,
				bFallback ? BasaltRockMat : nullptr,
				AetheriaWorldConst::MeshMWAMStoneA,
				bCollide);
		};

		// West & East volcanic crag flanks (offset HalfW + 690 so boulder inner faces stay >150 cm outside HalfW!)
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			for (int32 Seg = -2; Seg <= 2; ++Seg)
			{
				const float RX = CX + static_cast<float>(Side) * (HalfW + 690.f + CragRng.FRandRange(-30.f, 60.f));
				const float RY = CY + static_cast<float>(Seg) * 490.f;
				PlaceCalderaCrag(
					FString::Printf(TEXT("EmberCrypt_CragSide_%d_%d"), Side > 0 ? 1 : 0, Seg + 2),
					RX, RY, SampleGroundZ(RX, RY, BaseZ) - 90.f, CragRng.FRandRange(780.f, 960.f), 1.06f, true);
			}
		}
		// South volcanic crag spine (offset HalfL + 690 outside south wall!)
		for (int32 Seg = -2; Seg <= 2; ++Seg)
		{
			const float RX = CX + static_cast<float>(Seg) * 460.f;
			const float RY = CY - HalfL - 690.f;
			PlaceCalderaCrag(
				FString::Printf(TEXT("EmberCrypt_CragSouth_%d"), Seg + 2),
				RX, RY, SampleGroundZ(RX, RY, BaseZ) - 90.f, CragRng.FRandRange(820.f, 980.f), 1.08f, true);
		}
		// North doorway flanking crag buttresses (wide clear approach to Grand Doorway!)
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float RX = CX + static_cast<float>(Side) * 980.f;
			const float RY = CY + HalfL + 480.f;
			PlaceCalderaCrag(
				FString::Printf(TEXT("EmberCrypt_CragNorthPortal_%d"), Side > 0 ? 1 : 0),
				RX, RY, SampleGroundZ(RX, RY, BaseZ) - 80.f, 780.f, 1.05f, true);
		}
		// Jagged volcanic caldera dome crowning the crypt roof (strictly above BaseZ + 565 ceiling!)
		for (int32 RXIdx = -1; RXIdx <= 1; ++RXIdx)
		{
			for (int32 RYIdx = -1; RYIdx <= 1; ++RYIdx)
			{
				const float RX = CX + static_cast<float>(RXIdx) * 560.f;
				const float RY = CY + static_cast<float>(RYIdx) * 640.f;
				const float PeakH = (RXIdx == 0 && RYIdx == 0) ? 580.f : 420.f;
				PlaceCalderaCrag(
					FString::Printf(TEXT("EmberCrypt_CragRoofDome_%d_%d"), RXIdx + 1, RYIdx + 1),
					RX, RY, BaseZ + 575.f, PeakH, 1.75f, false);
			}
		}
	}

	// 4. Enclosed Flat Stone Ceiling Canopy Blocking All Sunlight (ZERO center partition wall arches obstructing the crypt nave!)
	if (RoofFlatMesh)
	{
		const FBox RoofBox = AetheriaWorldHelpers::GetSafeBoundingBox(RoofFlatMesh);
		const FVector RoofSize = RoofBox.GetSize();
		const FVector RoofCenter = RoofBox.GetCenter();
		const float RoofTileW = 640.f;
		const float RoofTileL = 760.f;
		const FVector RoofScale(
			(RoofTileW * 1.06f) / FMath::Max(10.f, RoofSize.X),
			(RoofTileL * 1.06f) / FMath::Max(10.f, RoofSize.Y),
			FMath::Clamp(48.f / FMath::Max(6.f, RoofSize.Z), 0.45f, 2.5f));

		for (int32 RX = -1; RX <= 1; ++RX)
		{
			for (int32 RY = -1; RY <= 1; ++RY)
			{
				const float TX = CX + static_cast<float>(RX) * RoofTileW;
				const float TY = CY + static_cast<float>(RY) * RoofTileL;
				const FVector LocalBotCenter(RoofCenter.X * RoofScale.X, RoofCenter.Y * RoofScale.Y, RoofBox.Min.Z * RoofScale.Z);
				const FVector RoofLoc = FVector(TX, TY, BaseZ + 565.f) - LocalBotCenter;

				if (UStaticMeshComponent* RoofComp = AddWorldMesh(
					FString::Printf(TEXT("EmberCrypt_EnclosedRoof_%d_%d"), RX + 1, RY + 1),
					AetheriaWorldConst::MeshCryptCeilingFlat,
					RoofLoc,
					FRotator::ZeroRotator,
					RoofScale,
					CryptWallMat,
					AetheriaWorldConst::MeshCryptFloor,
					true))
				{
					RoofComp->SetCastShadow(true);
				}
			}
		}
	}

	// 5. 8 Flickering Wall Torch Sconces (SM_Pillar2 + SM_Fire_Pit + REAL P_Torch_Fire + warm 560 lm flickering torchlight)
	const float TorchYs[4] = { CY - 740.f, CY - 240.f, CY + 260.f, CY + 760.f };
	for (int32 Row = 0; Row < 4; ++Row)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const int32 TorchIdx = Row * 2 + (Side > 0 ? 1 : 0);
			const float TX = CX + static_cast<float>(Side) * (HalfW - 140.f);
			const float TY = TorchYs[Row];
			const FRotator PostRot(0.f, Side > 0 ? 180.f : 0.f, 0.f);
			const FVector PostLoc = AetheriaWorldHelpers::ComputeGroundedCenterLoc(Pillar2Mesh, TX, TY, WalkwayTopZ, PostRot, FVector(SconceP2Scale), 6.f);
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
				FVector(0.84f));

			AddAmbientFireLight(
				FString::Printf(TEXT("EmberCrypt_WallTorchLight_%d"), TorchIdx),
				FVector(TX - static_cast<float>(Side) * 42.f, TY, FireTopZ + 48.f),
				560.f,
				2050.f,
				1120.f,
				FLinearColor(1.0f, 0.46f, 0.12f, 1.0f),
				0.20f,
				TorchIdx * 1.1f,
				false,
				0.14f);
		}
	}

	// 6. Moody Underfloor Volcanic Crack Glow Bounce (subtle warm red-orange contrast in the dark enclosed dungeon)
	AddAmbientFireLight(
		TEXT("EmberCrypt_LavaCrackBounceN"),
		FVector(CX, CY + 260.f, BaseZ + 95.f),
		480.f,
		1850.f,
		1450.f,
		FLinearColor(1.0f, 0.24f, 0.03f, 1.0f),
		0.14f,
		0.4f,
		false,
		0.10f);
	AddAmbientFireLight(
		TEXT("EmberCrypt_LavaCrackBounceS"),
		FVector(CX, CY - 420.f, BaseZ + 95.f),
		480.f,
		1850.f,
		1450.f,
		FLinearColor(1.0f, 0.24f, 0.03f, 1.0f),
		0.14f,
		2.1f,
		false,
		0.10f);

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
		AetheriaWorldConst::MatMWAMCoverRocks,
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

	const float ShrineScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(ShrineMesh, 420.f, 0.2f, 3.0f);
	const FRotator ShrineRot(0.f, 35.f, 0.f);
	AddWorldMesh(
		TEXT("Glacier_SanctuaryShrine"),
		AetheriaWorldConst::MeshMausoleum,
		AetheriaWorldHelpers::ComputeGroundedCenterLoc(ShrineMesh, CX, CY, CenterZ, ShrineRot, FVector(ShrineScale), 10.f),
		ShrineRot,
		FVector(ShrineScale),
		GlacierStoneMat);

	// 12 Grounded Faceted Frost Crystalline Spires encircling the glacier basin + closed 360-degree sunken boulder clusters
	FRandomStream GlacierRng(99107);
	const float SpireBaseUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SpireMesh, 420.f, 0.2f, 3.2f);

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
			AddBoulderCluster(
				FString::Printf(TEXT("Glacier_BoulderCluster_%d"), Idx),
				SX + 165.f,
				SY - 120.f,
				GlacierRng.RandRange(2, 3),
				155.f,
				275.f,
				GlacierRng,
				false);
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
		{ FVector2D(-3800.f,  3200.f), 4800.f,  -28.f, FLinearColor(0.25f, 1.45f, 2.10f, 1.0f), false },
		{ FVector2D(    0.f, -3800.f), 5000.f,    0.f, FLinearColor(1.90f, 1.05f, 0.22f, 1.0f), true  },
		{ FVector2D(    0.f, -7200.f), 5200.f,    0.f, FLinearColor(1.15f, 0.42f, 2.05f, 1.0f), false }
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

void AAetheriaWorldBuilder::BuildKingsRoadAndLanterns()
{
	// Scenic stone-curbed cobble King's Road & warm torch lanterns every ~1800 cm connecting all major landmarks:
	// Oakhaven Village -> Mirror Lake Bridge -> Dragon Aerie Lakeside Pavilion -> Blue-Leaf Woods -> Highkeep Castle -> Ember Crypt Trail
	UStaticMesh* RoadSlabMesh = LoadMeshSafe(AetheriaWorldConst::MeshDungeonFloor, AetheriaWorldConst::MeshCryptFloor);
	UStaticMesh* CurbMesh     = LoadMeshSafe(AetheriaWorldConst::MeshGothicBorder1, AetheriaWorldConst::MeshEdgeStones);
	UStaticMesh* Pillar2Mesh  = LoadMeshSafe(AetheriaWorldConst::MeshGothicPillar2, AetheriaWorldConst::MeshDungeonPillar);
	UStaticMesh* BowlMesh     = LoadMeshSafe(AetheriaWorldConst::MeshFirePit, AetheriaWorldConst::MeshDungeonPillar);

	UMaterialInterface* CobbleMat = LoadPBRMaterial({
		AetheriaWorldConst::MatBrickFloorClean,
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* CurbMat = LoadPBRMaterial({
		AetheriaWorldConst::MatStoneAltar,
		AetheriaWorldConst::MatEdgeStonesInst,
		AetheriaWorldConst::MatStoneWall
	});
	UMaterialInterface* FirePitMat = LoadPBRMaterial({
		AetheriaWorldConst::MatFirePitInst,
		AetheriaWorldConst::MatStoneWall
	});

	if (!RoadSlabMesh)
	{
		return;
	}

	const FBox SlabBox = AetheriaWorldHelpers::GetSafeBoundingBox(RoadSlabMesh);
	const FVector SlabSize = SlabBox.GetSize();
	const FVector SlabCenter = SlabBox.GetCenter();
	const FBox CurbBox = AetheriaWorldHelpers::GetSafeBoundingBox(CurbMesh);
	const FVector CurbSize = CurbBox.GetSize();
	const float CurbScaleZ = FMath::Clamp(28.f / FMath::Max(6.f, CurbSize.Z), 0.20f, 1.5f);
	const float Pillar2Scale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Pillar2Mesh, 195.f, 0.1f, 3.2f);
	const float BowlScale    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BowlMesh, 30.f, 0.1f, 2.4f);

	int32 LanternGlobalIdx = 0;

	auto PlaceRoadLantern = [&](float LX, float LY, float YawDeg)
	{
		const float GZ = SampleGroundZ(LX, LY, FallbackGroundZ);
		const FRotator PostRot(0.f, YawDeg, 0.f);
		const FVector PostLoc = AetheriaWorldHelpers::ComputeGroundedCenterLoc(Pillar2Mesh, LX, LY, GZ, PostRot, FVector(Pillar2Scale), 6.f);
		const float BowlZ = AetheriaWorldHelpers::ComputeStackedZ(Pillar2Mesh, PostLoc.Z, Pillar2Scale, BowlMesh, BowlScale, 4.f);
		const FBox BowlBox = AetheriaWorldHelpers::GetSafeBoundingBox(BowlMesh);
		const FVector BowlCenter = BowlBox.GetCenter();
		const FVector BowlLoc(LX - BowlCenter.X * BowlScale, LY - BowlCenter.Y * BowlScale, BowlZ);
		const float FireTopZ = BowlZ + BowlBox.Max.Z * BowlScale - 4.f;

		AddWorldMesh(
			FString::Printf(TEXT("KingsRoad_LanternPost_%d"), LanternGlobalIdx),
			AetheriaWorldConst::MeshGothicPillar2,
			PostLoc,
			PostRot,
			FVector(Pillar2Scale));
		AddWorldMesh(
			FString::Printf(TEXT("KingsRoad_LanternBowl_%d"), LanternGlobalIdx),
			AetheriaWorldConst::MeshFirePit,
			BowlLoc,
			FRotator::ZeroRotator,
			FVector(BowlScale),
			FirePitMat);
		SpawnFireParticle(
			FString::Printf(TEXT("KingsRoad_LanternFire_%d"), LanternGlobalIdx),
			AetheriaWorldConst::ParticleTorchFire,
			FVector(LX, LY, FireTopZ),
			FVector(0.80f));
		AddAmbientFireLight(
			FString::Printf(TEXT("KingsRoad_LanternLight_%d"), LanternGlobalIdx),
			FVector(LX, LY, FireTopZ + 44.f),
			520.f,
			2350.f,
			1050.f,
			FLinearColor(1.0f, 0.68f, 0.30f, 1.0f),
			0.11f,
			LanternGlobalIdx * 0.95f,
			true,
			0.14f);
		++LanternGlobalIdx;
	};

	auto BuildCurbedRoadSegment = [&](const FString& RoadTag, const FVector2D& Start, const FVector2D& End, float RoadWidth, bool bPlaceCobbles, float LanternInterval)
	{
		const FVector2D Delta = End - Start;
		const float TotalLen = Delta.Size();
		if (TotalLen < 120.f)
		{
			return;
		}
		const FVector2D Dir = Delta / TotalLen;
		const FVector2D Right(-Dir.Y, Dir.X);
		const float RoadYaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));

		if (bPlaceCobbles)
		{
			const float SegLen = 420.f;
			const int32 NumSegs = FMath::Max(1, FMath::CeilToInt(TotalLen / SegLen));
			const float ActualStep = TotalLen / static_cast<float>(NumSegs);
			const FVector RoadScale(
				(ActualStep * 1.04f) / FMath::Max(10.f, SlabSize.X),
				RoadWidth / FMath::Max(10.f, SlabSize.Y),
				FMath::Clamp(26.f / FMath::Max(6.f, SlabSize.Z), 0.25f, 1.8f));

			const FVector CurbScale(
				(ActualStep * 1.02f) / FMath::Max(10.f, FMath::Max(CurbSize.X, CurbSize.Y)),
				FMath::Clamp(46.f / FMath::Max(10.f, FMath::Min(CurbSize.X, CurbSize.Y)), 0.25f, 1.5f),
				CurbScaleZ);

			for (int32 Idx = 0; Idx < NumSegs; ++Idx)
			{
				const float Alpha = (static_cast<float>(Idx) + 0.5f) / static_cast<float>(NumSegs);
				const FVector2D P0 = FMath::Lerp(Start, End, static_cast<float>(Idx) / static_cast<float>(NumSegs));
				const FVector2D P1 = FMath::Lerp(Start, End, static_cast<float>(Idx + 1) / static_cast<float>(NumSegs));
				const FVector2D Mid = FMath::Lerp(Start, End, Alpha);

				const float Z0 = SampleGroundZ(P0.X, P0.Y, FallbackGroundZ);
				const float Z1 = SampleGroundZ(P1.X, P1.Y, FallbackGroundZ);
				const float MidZ = FMath::Max(SampleGroundZ(Mid.X, Mid.Y, 0.5f * (Z0 + Z1)), 0.5f * (Z0 + Z1));
				const float PitchDeg = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Z1 - Z0, ActualStep)), -16.f, 16.f);
				const FRotator SegRot(PitchDeg, RoadYaw, 0.f);
				const FVector LocalTop(SlabCenter.X * RoadScale.X, SlabCenter.Y * RoadScale.Y, SlabBox.Max.Z * RoadScale.Z);
				const FVector SlabLoc = FVector(Mid.X, Mid.Y, MidZ + 1.5f) - SegRot.RotateVector(LocalTop);

				AddWorldMesh(
					FString::Printf(TEXT("%s_Slab_%d"), *RoadTag, Idx),
					AetheriaWorldConst::MeshDungeonFloor,
					SlabLoc,
					SegRot,
					RoadScale,
					CobbleMat,
					AetheriaWorldConst::MeshCryptFloor,
					true);

				// Flush stone curbs along both road edges so the cobble lane blends neatly into meadow grass
				if (CurbMesh)
				{
					for (int32 Side = -1; Side <= 1; Side += 2)
					{
						const FVector2D CurbPos = Mid + Right * (static_cast<float>(Side) * (RoadWidth * 0.5f + 16.f));
						const float CurbGZ = SampleGroundZ(CurbPos.X, CurbPos.Y, MidZ);
						const FRotator CurbRot(PitchDeg * 0.5f, RoadYaw + (Side > 0 ? 0.f : 180.f), 0.f);
						AddWorldMesh(
							FString::Printf(TEXT("%s_Curb_%d_%d"), *RoadTag, Idx, Side > 0 ? 1 : 0),
							AetheriaWorldConst::MeshGothicBorder1,
							AetheriaWorldHelpers::ComputeGroundedCenterLoc(CurbMesh, CurbPos.X, CurbPos.Y, CurbGZ, CurbRot, CurbScale, 12.f),
							CurbRot,
							CurbScale,
							CurbMat,
							AetheriaWorldConst::MeshEdgeStones,
							false);
					}
				}
			}
		}

		// Warm torch lanterns spaced every ~1800 cm along alternating sides of the King's Road
		const int32 NumLanterns = FMath::Max(1, FMath::FloorToInt(TotalLen / FMath::Max(600.f, LanternInterval)));
		for (int32 LIdx = 0; LIdx < NumLanterns; ++LIdx)
		{
			const float Alpha = (static_cast<float>(LIdx) + 0.5f) / static_cast<float>(NumLanterns);
			const FVector2D Pos = FMath::Lerp(Start, End, Alpha);
			const float SideSign = ((LIdx + LanternGlobalIdx) % 2 == 0) ? 1.f : -1.f;
			const FVector2D LPos = Pos + Right * (SideSign * (RoadWidth * 0.5f + 115.f));
			PlaceRoadLantern(LPos.X, LPos.Y, RoadYaw + SideSign * 90.f);
		}
	};

	// 1. Oakhaven Village South Gate -> Mirror Lake North Bridge Ramp
	BuildCurbedRoadSegment(TEXT("KingsRoad_OakhavenToBridge"), FVector2D(0.f, 2945.f), FVector2D(0.f, 1735.f), 460.f, true, 1200.f);

	// 2. Mirror Lake South Bridge Ramp -> Highkeep Castle North Steps
	BuildCurbedRoadSegment(TEXT("KingsRoad_BridgeToHighkeep"), FVector2D(0.f, -3335.f), FVector2D(0.f, -7860.f), 480.f, true, 1800.f);

	// 3. Oakhaven West Exit -> Dragon Aerie Lakeside Pavilion
	BuildCurbedRoadSegment(TEXT("KingsRoad_OakhavenToAerie"), FVector2D(-1755.f, 3480.f), FVector2D(-2765.f, 2320.f), 380.f, true, 1600.f);

	// 4. Scenic Lanterned Eastern Trail from Oakhaven -> Blue-Leaf Woods -> Ember Crypt
	BuildCurbedRoadSegment(TEXT("KingsRoad_EastTrailA"), FVector2D(1950.f, 3250.f), FVector2D(5400.f, 1150.f), 360.f, false, 1800.f);
	BuildCurbedRoadSegment(TEXT("KingsRoad_EastTrailB"), FVector2D(5400.f, 1150.f), FVector2D(9350.f, -1150.f), 360.f, false, 1800.f);
}

void AAetheriaWorldBuilder::BuildLushMeadowFoliageAndWind()
{
	// GPU-Efficient HISM Meadow Grass (wind sway), Pink/Violet/Gold Wildflowers, Bushes, Sunlit Green Full-Canopy Trees,
	// Natural Sunken Boulder Clusters, Ambient Niagara Wind/Petals, and 12 Interactive Sliceable Meadow Trees!
	UMaterialInterface* GrassWindMat = LoadPBRMaterial({
		AetheriaWorldConst::MatMWAMPlantsGrass
	});

	// 1. Wind-Swaying Green Meadow Grass HISMs (CastShadow=false for fast RTX HWRT FPS, Cull 7500..11500)
	UHierarchicalInstancedStaticMeshComponent* GrassAHISM = CreateFoliageHISM(
		TEXT("Meadow_WindGrassA_HISM"), AetheriaWorldConst::MeshMWAMGrassA, GrassWindMat, false, 7500, 11500, false);
	UHierarchicalInstancedStaticMeshComponent* GrassBHISM = CreateFoliageHISM(
		TEXT("Meadow_WindGrassB_HISM"), AetheriaWorldConst::MeshMWAMGrassB, GrassWindMat, false, 7500, 11500, false);
	UHierarchicalInstancedStaticMeshComponent* GrassCHISM = CreateFoliageHISM(
		TEXT("Meadow_WindGrassC_HISM"), AetheriaWorldConst::MeshMWAMGrassC, GrassWindMat, false, 7500, 11500, false);

	// 2. Wildflower & Bush HISMs (CastShadow=false, Cull 7500..11500)
	UHierarchicalInstancedStaticMeshComponent* FlowerVioletHISM = CreateFoliageHISM(
		TEXT("Meadow_FlowerHeliophila_HISM"), AetheriaWorldConst::MeshFlowerHeliophila, nullptr, false, 7500, 11500, false);
	UHierarchicalInstancedStaticMeshComponent* FlowerGoldHISM = CreateFoliageHISM(
		TEXT("Meadow_FlowerGazania_HISM"), AetheriaWorldConst::MeshFlowerGazania, nullptr, false, 7500, 11500, false);
	UHierarchicalInstancedStaticMeshComponent* BushHISM = CreateFoliageHISM(
		TEXT("Meadow_WildRooibosBush_HISM"), AetheriaWorldConst::MeshWildRooibosBush, nullptr, false, 7500, 11500, false);

	// 3. Full-Canopy Sunlit Green Meadow & Hillside Trees (CastShadow=true for real volumetric sun rays through leaves!)
	UHierarchicalInstancedStaticMeshComponent* GreenTreesLargeHISM = CreateFoliageHISM(
		TEXT("Meadow_GreenTreesLarge_HISM"), AetheriaWorldConst::MeshSearsiaLarge, nullptr, true, 14500, 23000, true);
	UHierarchicalInstancedStaticMeshComponent* GreenTreesMedHISM = CreateFoliageHISM(
		TEXT("Meadow_GreenTreesMed_HISM"), AetheriaWorldConst::MeshSearsiaMedium, nullptr, true, 13000, 20000, true);

	UStaticMesh* GrassAMesh   = LoadMeshSafe(AetheriaWorldConst::MeshMWAMGrassA, nullptr);
	UStaticMesh* GrassBMesh   = LoadMeshSafe(AetheriaWorldConst::MeshMWAMGrassB, nullptr);
	UStaticMesh* GrassCMesh   = LoadMeshSafe(AetheriaWorldConst::MeshMWAMGrassC, nullptr);
	UStaticMesh* VioletMesh   = LoadMeshSafe(AetheriaWorldConst::MeshFlowerHeliophila, nullptr);
	UStaticMesh* GoldMesh     = LoadMeshSafe(AetheriaWorldConst::MeshFlowerGazania, nullptr);
	UStaticMesh* BushMesh     = LoadMeshSafe(AetheriaWorldConst::MeshWildRooibosBush, nullptr);
	UStaticMesh* TreeLargeMesh= LoadMeshSafe(AetheriaWorldConst::MeshSearsiaLarge, nullptr);
	UStaticMesh* TreeMedMesh  = LoadMeshSafe(AetheriaWorldConst::MeshSearsiaMedium, nullptr);

	const float GrassAUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GrassAMesh, 62.f, 0.25f, 3.0f);
	const float GrassBUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GrassBMesh, 58.f, 0.25f, 3.0f);
	const float GrassCUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GrassCMesh, 66.f, 0.25f, 3.0f);
	const float VioletUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(VioletMesh, 46.f, 0.20f, 2.6f);
	const float GoldUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(GoldMesh, 42.f, 0.20f, 2.6f);
	const float BushUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BushMesh, 110.f, 0.20f, 2.4f);
	const float TreeLgUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(TreeLargeMesh, 720.f, 0.25f, 2.5f);
	const float TreeMdUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(TreeMedMesh, 510.f, 0.25f, 2.5f);

	FRandomStream MeadowRng(582026);

	// Populate ~3,400 Wind-Swaying Grass Instances across Oakhaven Valley, Lakeside Shores, Road Verges & Hills
	TArray<FTransform> GrassATransforms;
	TArray<FTransform> GrassBTransforms;
	TArray<FTransform> GrassCTransforms;
	GrassATransforms.Reserve(1200);
	GrassBTransforms.Reserve(1200);
	GrassCTransforms.Reserve(1200);

	// 1a. Dense hero-proximity lush valley & lakeside shore grass (~2,200 blades)
	int32 Attempts = 0;
	while ((GrassATransforms.Num() + GrassBTransforms.Num() + GrassCTransforms.Num()) < 2200 && Attempts < 7500)
	{
		++Attempts;
		const float X = MeadowRng.FRandRange(-4200.f, 4200.f);
		const float Y = MeadowRng.FRandRange(-4800.f, 6200.f);
		if (IsInsideNoGrassZone(X, Y))
		{
			continue;
		}
		const float GZ = SampleGroundZ(X, Y, FallbackGroundZ);
		const int32 Variant = MeadowRng.RandRange(0, 2);
		UStaticMesh* GMesh = (Variant == 0) ? GrassAMesh : ((Variant == 1) ? GrassBMesh : GrassCMesh);
		if (!GMesh)
		{
			continue;
		}
		const float BaseU = (Variant == 0) ? GrassAUnit : ((Variant == 1) ? GrassBUnit : GrassCUnit);
		const float S = BaseU * MeadowRng.FRandRange(0.78f, 1.55f);
		const float SZ = S * MeadowRng.FRandRange(0.85f, 1.32f);
		const float Z = AetheriaWorldHelpers::ComputeGroundedZ(GMesh, GZ, SZ, 4.f);
		const FRotator Rot(MeadowRng.FRandRange(-4.f, 4.f), MeadowRng.FRandRange(0.f, 360.f), MeadowRng.FRandRange(-4.f, 4.f));

		if (Variant == 0)      GrassATransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S, S, SZ));
		else if (Variant == 1) GrassBTransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S, S, SZ));
		else                   GrassCTransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S, S, SZ));
	}

	// 1b. Rolling hillside & outer meadow grass (~1,200 blades reaching out to +/-6800)
	Attempts = 0;
	while ((GrassATransforms.Num() + GrassBTransforms.Num() + GrassCTransforms.Num()) < 3400 && Attempts < 4500)
	{
		++Attempts;
		const float X = MeadowRng.FRandRange(-6800.f, 6800.f);
		const float Y = MeadowRng.FRandRange(-7400.f, 6800.f);
		if (IsInsideNoGrassZone(X, Y))
		{
			continue;
		}
		const float GZ = SampleGroundZ(X, Y, FallbackGroundZ);
		const int32 Variant = MeadowRng.RandRange(0, 2);
		UStaticMesh* GMesh = (Variant == 0) ? GrassAMesh : ((Variant == 1) ? GrassBMesh : GrassCMesh);
		if (!GMesh)
		{
			continue;
		}
		const float BaseU = (Variant == 0) ? GrassAUnit : ((Variant == 1) ? GrassBUnit : GrassCUnit);
		const float S = BaseU * MeadowRng.FRandRange(0.85f, 1.65f);
		const float SZ = S * MeadowRng.FRandRange(0.85f, 1.35f);
		const float Z = AetheriaWorldHelpers::ComputeGroundedZ(GMesh, GZ, SZ, 4.f);
		const FRotator Rot(MeadowRng.FRandRange(-5.f, 5.f), MeadowRng.FRandRange(0.f, 360.f), MeadowRng.FRandRange(-5.f, 5.f));

		if (Variant == 0)      GrassATransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S, S, SZ));
		else if (Variant == 1) GrassBTransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S, S, SZ));
		else                   GrassCTransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S, S, SZ));
	}

	if (GrassAHISM && GrassATransforms.Num() > 0) GrassAHISM->AddInstances(GrassATransforms, false, true, false);
	if (GrassBHISM && GrassBTransforms.Num() > 0) GrassBHISM->AddInstances(GrassBTransforms, false, true, false);
	if (GrassCHISM && GrassCTransforms.Num() > 0) GrassCHISM->AddInstances(GrassCTransforms, false, true, false);

	// Populate ~660 Pink/Violet & Gold Wildflowers in 26 Organic Meadow Patches + ~240 Meadow Bushes
	TArray<FTransform> VioletTransforms;
	TArray<FTransform> GoldTransforms;
	TArray<FTransform> BushTransforms;
	VioletTransforms.Reserve(360);
	GoldTransforms.Reserve(340);
	BushTransforms.Reserve(250);

	int32 PatchCount = 0;
	Attempts = 0;
	while (PatchCount < 26 && Attempts < 600)
	{
		++Attempts;
		const float PX = MeadowRng.FRandRange(-4200.f, 4200.f);
		const float PY = MeadowRng.FRandRange(-5400.f, 5900.f);
		if (IsInsideNoGrassZone(PX, PY))
		{
			continue;
		}
		++PatchCount;

		const bool bPreferViolet = (PatchCount % 2 == 0);
		const int32 FlowersInPatch = MeadowRng.RandRange(22, 28);
		for (int32 FIdx = 0; FIdx < FlowersInPatch; ++FIdx)
		{
			const float Angle = MeadowRng.FRandRange(0.f, UE_TWO_PI);
			const float Dist = FMath::Sqrt(MeadowRng.FRand()) * MeadowRng.FRandRange(180.f, 520.f);
			const float FX = PX + FMath::Cos(Angle) * Dist;
			const float FY = PY + FMath::Sin(Angle) * Dist;
			if (IsInsideNoGrassZone(FX, FY))
			{
				continue;
			}
			const bool bUseViolet = (MeadowRng.FRand() < (bPreferViolet ? 0.72f : 0.28f));
			UStaticMesh* FMesh = bUseViolet ? VioletMesh : GoldMesh;
			if (!FMesh)
			{
				continue;
			}
			const float Unit = bUseViolet ? VioletUnit : GoldUnit;
			const float FS = Unit * MeadowRng.FRandRange(0.75f, 1.42f);
			const float FZ = AetheriaWorldHelpers::ComputeGroundedZ(FMesh, SampleGroundZ(FX, FY, FallbackGroundZ), FS, 3.f);
			const FRotator FRot(MeadowRng.FRandRange(-5.f, 5.f), MeadowRng.FRandRange(0.f, 360.f), MeadowRng.FRandRange(-5.f, 5.f));
			if (bUseViolet) VioletTransforms.Emplace(FRot, FVector(FX, FY, FZ), FVector(FS));
			else            GoldTransforms.Emplace(FRot, FVector(FX, FY, FZ), FVector(FS));
		}
	}

	Attempts = 0;
	while (BushTransforms.Num() < 240 && Attempts < 1600)
	{
		++Attempts;
		const float BX = MeadowRng.FRandRange(-4600.f, 4600.f);
		const float BY = MeadowRng.FRandRange(-6200.f, 6400.f);
		if (IsInsideNoGrassZone(BX, BY) || !BushMesh)
		{
			continue;
		}
		const float BS = BushUnit * MeadowRng.FRandRange(0.72f, 1.45f);
		const float BZ = AetheriaWorldHelpers::ComputeGroundedZ(BushMesh, SampleGroundZ(BX, BY, FallbackGroundZ), BS, 10.f);
		BushTransforms.Emplace(FRotator(0.f, MeadowRng.FRandRange(0.f, 360.f), 0.f), FVector(BX, BY, BZ), FVector(BS, BS, BS * MeadowRng.FRandRange(0.85f, 1.15f)));
	}

	if (FlowerVioletHISM && VioletTransforms.Num() > 0) FlowerVioletHISM->AddInstances(VioletTransforms, false, true, false);
	if (FlowerGoldHISM && GoldTransforms.Num() > 0)     FlowerGoldHISM->AddInstances(GoldTransforms, false, true, false);
	if (BushHISM && BushTransforms.Num() > 0)           BushHISM->AddInstances(BushTransforms, false, true, false);

	// Populate ~145 Sunlit Green Full-Canopy Meadow & Hillside Trees (casting golden-hour volumetric sun rays!)
	TArray<FTransform> GreenLargeTransforms;
	TArray<FTransform> GreenMedTransforms;
	GreenLargeTransforms.Reserve(80);
	GreenMedTransforms.Reserve(75);

	Attempts = 0;
	while ((GreenLargeTransforms.Num() + GreenMedTransforms.Num()) < 145 && Attempts < 1800)
	{
		++Attempts;
		const float TX = MeadowRng.FRandRange(-4400.f, 4400.f);
		const float TY = MeadowRng.FRandRange(-7400.f, 6800.f);
		if (IsInsideNoGrassZone(TX, TY))
		{
			continue;
		}
		// Keep hero's immediate Oakhaven->Lake vista corridor & northern sunny shore emergence point airy and framed rather than cluttered
		if (FMath::Abs(TX) < 580.f && TY > 1600.f && TY < 3100.f)
		{
			continue;
		}
		if (FVector2D::Distance(FVector2D(TX, TY), FVector2D(-480.f, 1680.f)) < 380.f)
		{
			continue;
		}

		const bool bLarge = (MeadowRng.FRand() < 0.54f);
		UStaticMesh* TMesh = bLarge ? TreeLargeMesh : TreeMedMesh;
		if (!TMesh)
		{
			continue;
		}
		const float Unit = bLarge ? TreeLgUnit : TreeMdUnit;
		const float TS = Unit * MeadowRng.FRandRange(0.82f, 1.28f);
		const float TSZ = TS * MeadowRng.FRandRange(0.90f, 1.18f);
		const float TZ = AetheriaWorldHelpers::ComputeGroundedZ(TMesh, SampleGroundZ(TX, TY, FallbackGroundZ), TSZ, 18.f);
		const FRotator TRot(MeadowRng.FRandRange(-3.f, 3.f), MeadowRng.FRandRange(0.f, 360.f), MeadowRng.FRandRange(-3.f, 3.f));

		if (bLarge) GreenLargeTransforms.Emplace(TRot, FVector(TX, TY, TZ), FVector(TS, TS, TSZ));
		else        GreenMedTransforms.Emplace(TRot, FVector(TX, TY, TZ), FVector(TS, TS, TSZ));
	}

	if (GreenTreesLargeHISM && GreenLargeTransforms.Num() > 0) GreenTreesLargeHISM->AddInstances(GreenLargeTransforms, false, true, false);
	if (GreenTreesMedHISM && GreenMedTransforms.Num() > 0)     GreenTreesMedHISM->AddInstances(GreenMedTransforms, false, true, false);

	// 10 Curated Natural Closed Boulder Clusters (2-4 rocks sunk 35% + nestled bushes) across Meadow Hills
	const FVector2D MeadowRockClusters[] = {
		FVector2D(-2680.f,  4650.f),
		FVector2D( 2720.f,  4520.f),
		FVector2D(-1420.f,  2480.f),
		FVector2D( 1520.f,  2350.f),
		FVector2D(-2180.f, -3850.f),
		FVector2D( 2050.f, -4120.f),
		FVector2D(-1620.f, -5950.f),
		FVector2D( 1740.f, -6180.f),
		FVector2D( 3250.f,  1750.f),
		FVector2D(-3150.f,  3550.f)
	};
	for (int32 Idx = 0; Idx < UE_ARRAY_COUNT(MeadowRockClusters); ++Idx)
	{
		AddBoulderCluster(
			FString::Printf(TEXT("Meadow_RockCluster_%d"), Idx),
			MeadowRockClusters[Idx].X,
			MeadowRockClusters[Idx].Y,
			MeadowRng.RandRange(2, 4),
			135.f,
			250.f,
			MeadowRng,
			true);
	}

	// 8 Ambient Niagara Wind Streams & Floating Flower Petals drifting across Oakhaven Meadow, Lakeside & Blue-Leaf Woods
	const FVector2D WindFlowSpots[4] = {
		FVector2D( -780.f,  2550.f),
		FVector2D(  820.f,  2280.f),
		FVector2D(-5200.f,  1800.f),
		FVector2D( 5100.f,  2400.f)
	};
	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		const float GZ = SampleGroundZ(WindFlowSpots[Idx].X, WindFlowSpots[Idx].Y, FallbackGroundZ);
		SpawnAmbientWindNiagara(
			FString::Printf(TEXT("Ambient_WindFlow_%d"), Idx),
			AetheriaWorldConst::NSWindFlows01,
			FVector(WindFlowSpots[Idx].X, WindFlowSpots[Idx].Y, GZ + 185.f),
			FVector(1.55f),
			FRotator(0.f, -25.f + Idx * 18.f, 0.f));
	}

	const FVector2D PetalSpots[4] = {
		FVector2D( -950.f,  3350.f),
		FVector2D(  980.f,  3120.f),
		FVector2D( -540.f,  1780.f),
		FVector2D( 4850.f,   450.f)
	};
	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		const float GZ = SampleGroundZ(PetalSpots[Idx].X, PetalSpots[Idx].Y, FallbackGroundZ);
		SpawnAmbientWindNiagara(
			FString::Printf(TEXT("Ambient_WindPetals_%d"), Idx),
			AetheriaWorldConst::NSWindPetals,
			FVector(PetalSpots[Idx].X, PetalSpots[Idx].Y, GZ + 220.f),
			FVector(1.45f),
			FRotator(0.f, 15.f + Idx * 22.f, 0.f));
	}

	// 12 Interactive Sliceable Meadow Trees along Oakhaven-to-Lake meadow (Crimson Desert reel style — NO rune crystals!)
	if (HasAuthority() && GetWorld() && TreeLargeMesh)
	{
		const FVector2D SliceableCoords[12] = {
			FVector2D( -620.f, 2720.f),
			FVector2D(  640.f, 2680.f),
			FVector2D( -880.f, 2320.f),
			FVector2D(  910.f, 2240.f),
			FVector2D( -680.f, 1980.f),
			FVector2D(  680.f, 1920.f),
			FVector2D(-1180.f, 2760.f),
			FVector2D( 1220.f, 2740.f),
			FVector2D(-1350.f, 2060.f),
			FVector2D( 1340.f, 1980.f),
			FVector2D(-2180.f, 2980.f),
			FVector2D( 1980.f, 3080.f)
		};

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		for (int32 Idx = 0; Idx < 12; ++Idx)
		{
			const float SX = SliceableCoords[Idx].X;
			const float SY = SliceableCoords[Idx].Y;
			const float SGZ = SampleGroundZ(SX, SY, FallbackGroundZ);
			const float TScale = TreeLgUnit * MeadowRng.FRandRange(0.86f, 1.12f);
			const float TZ = AetheriaWorldHelpers::ComputeGroundedZ(TreeLargeMesh, SGZ, TScale, 14.f);

			if (ADestructiblePropActor* TreeActor = GetWorld()->SpawnActor<ADestructiblePropActor>(
				ADestructiblePropActor::StaticClass(),
				FVector(SX, SY, TZ),
				FRotator(0.f, MeadowRng.FRandRange(0.f, 360.f), 0.f),
				SpawnParams))
			{
				TreeActor->bIsSliceableTree = true;
				TreeActor->ConfigureProp(
					EDestructibleKind::Wood,
					55.f,
					AetheriaWorldConst::MeshSearsiaLarge,
					FVector(TScale),
					NAME_None);
				SpawnedDestructibles.Add(TreeActor);
			}
		}
	}
}

void AAetheriaWorldBuilder::BuildNightSkyMoonAndStars()
{
	// 1. Sky-Distant Luminous Full Moon Sphere at (-28000, 24000, 22000), scale 28.0, soft silver-white emissive 2.2
	MoonMID = CreateColorMID(FLinearColor(0.92f * 2.2f, 0.96f * 2.2f, 1.00f * 2.2f, 1.0f), true);
	NightFullMoonMesh = AddWorldMesh(
		TEXT("NightSky_FullMoon"),
		AetheriaWorldConst::MeshSphere,
		FVector(-28000.f, 24000.f, 22000.f),
		FRotator::ZeroRotator,
		FVector(28.0f),
		MoonMID,
		AetheriaWorldConst::MeshSphere,
		false);

	if (NightFullMoonMesh)
	{
		NightFullMoonMesh->SetCastShadow(false);
		NightFullMoonMesh->bAffectDynamicIndirectLighting = false;
		NightFullMoonMesh->bAffectDistanceFieldLighting = false;
		NightFullMoonMesh->bDisallowNanite = true;
	}

	// 2. Overhead Starry Night Dome (84 subtle distant twinkling stars visible only during Dusk/Night!)
	UStaticMesh* StarMesh = LoadMeshSafe(AetheriaWorldConst::MeshSphere, nullptr);
	StarMID = CreateColorMID(FLinearColor(0.86f * 1.9f, 0.92f * 1.9f, 1.00f * 1.9f, 1.0f), true);

	if (StarMesh)
	{
		const FName StarsName = MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(), TEXT("NightSky_StarsISM"));
		NightStarsISM = NewObject<UInstancedStaticMeshComponent>(this, StarsName);
		if (NightStarsISM)
		{
			NightStarsISM->SetStaticMesh(StarMesh);
			if (StarMID)
			{
				NightStarsISM->SetMaterial(0, StarMID);
			}
			NightStarsISM->SetMobility(EComponentMobility::Static);
			NightStarsISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			NightStarsISM->SetCastShadow(false);
			NightStarsISM->bAffectDynamicIndirectLighting = false;
			NightStarsISM->bAffectDistanceFieldLighting = false;
			NightStarsISM->bDisallowNanite = true;
			NightStarsISM->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
			NightStarsISM->RegisterComponent();

			FRandomStream StarRng(90210);
			TArray<FTransform> StarTransforms;
			StarTransforms.Reserve(84);

			for (int32 Idx = 0; Idx < 84; ++Idx)
			{
				const float AzimuthRad = StarRng.FRandRange(0.f, UE_TWO_PI);
				const float ElevRad = FMath::DegreesToRadians(StarRng.FRandRange(22.f, 84.f));
				const float Radius = StarRng.FRandRange(31000.f, 38000.f);
				const float Horiz = FMath::Cos(ElevRad) * Radius;
				const FVector StarPos(
					FMath::Cos(AzimuthRad) * Horiz,
					FMath::Sin(AzimuthRad) * Horiz - 1200.f,
					FMath::Sin(ElevRad) * Radius + FallbackGroundZ);

				// Keep stars clear of the Full Moon disc
				if (FVector::Dist(StarPos, FVector(-28000.f, 24000.f, 22000.f)) < 4800.f)
				{
					continue;
				}

				const float StarScale = StarRng.FRandRange(0.55f, 1.30f);
				StarTransforms.Emplace(FRotator::ZeroRotator, StarPos, FVector(StarScale));
			}

			if (StarTransforms.Num() > 0)
			{
				NightStarsISM->AddInstances(StarTransforms, false, true, false);
			}
		}
	}
}

// ============================================================================
// Handcrafted Micro-Detail Across All 5 Biomes (Twigs, Bark, Pebbles, Flora & Light Destructibles)
// ============================================================================

void AAetheriaWorldBuilder::BuildMicroDetailTwigsBarkAndPebbles()
{
	// GPU-Instanced micro-detail layers with tight cull distances (zero shadow cost, high close-up richness!)
	UHierarchicalInstancedStaticMeshComponent* Twig01HISM = CreateFoliageHISM(
		TEXT("Micro_TwigBranch01_HISM"), AetheriaWorldConst::MeshTwigBranch01, nullptr, false, 4200, 7000, false);
	UHierarchicalInstancedStaticMeshComponent* Twig02HISM = CreateFoliageHISM(
		TEXT("Micro_TwigBranch02_HISM"), AetheriaWorldConst::MeshTwigBranch02, nullptr, false, 4200, 7000, false);
	UHierarchicalInstancedStaticMeshComponent* BarkAHISM = CreateFoliageHISM(
		TEXT("Micro_BarkDebrisA_HISM"), AetheriaWorldConst::MeshBarkDebrisA, nullptr, false, 3600, 6200, false);
	UHierarchicalInstancedStaticMeshComponent* BarkBHISM = CreateFoliageHISM(
		TEXT("Micro_BarkDebrisB_HISM"), AetheriaWorldConst::MeshBarkDebrisB, nullptr, false, 3600, 6200, false);
	UHierarchicalInstancedStaticMeshComponent* DryLeafHISM = CreateFoliageHISM(
		TEXT("Micro_DryLeafA_HISM"), AetheriaWorldConst::MeshDryLeafA, nullptr, false, 3500, 5800, false);
	UHierarchicalInstancedStaticMeshComponent* PebblesAHISM = CreateFoliageHISM(
		TEXT("Micro_PebbleStonesA_HISM"), AetheriaWorldConst::MeshPebbleStonesA, nullptr, false, 4200, 7200, false);
	UHierarchicalInstancedStaticMeshComponent* PebblesBHISM = CreateFoliageHISM(
		TEXT("Micro_PebbleStonesB_HISM"), AetheriaWorldConst::MeshPebbleStonesB, nullptr, false, 4200, 7200, false);
	UHierarchicalInstancedStaticMeshComponent* UrsiniaHISM = CreateFoliageHISM(
		TEXT("Micro_FlowerUrsinia_HISM"), AetheriaWorldConst::MeshFlowerUrsinia, nullptr, false, 6200, 9800, false);
	UHierarchicalInstancedStaticMeshComponent* EmpodiumHISM = CreateFoliageHISM(
		TEXT("Micro_FlowerEmpodium_HISM"), AetheriaWorldConst::MeshFlowerEmpodium, nullptr, false, 6200, 9800, false);
	UHierarchicalInstancedStaticMeshComponent* FernHISM = CreateFoliageHISM(
		TEXT("Micro_ForestFern_HISM"), AetheriaWorldConst::MeshKiteFern01, nullptr, false, 5800, 9200, false);
	UHierarchicalInstancedStaticMeshComponent* IceplantHISM = CreateFoliageHISM(
		TEXT("Micro_GlacierIceplant_HISM"), AetheriaWorldConst::MeshCrystalIceplant, nullptr, false, 5600, 8800, false);
	UHierarchicalInstancedStaticMeshComponent* SucculentHISM = CreateFoliageHISM(
		TEXT("Micro_RockSucculent_HISM"), AetheriaWorldConst::MeshSucculentRock, nullptr, false, 5000, 8200, false);

	UStaticMesh* Twig01Mesh    = LoadMeshSafe(AetheriaWorldConst::MeshTwigBranch01, nullptr);
	UStaticMesh* Twig02Mesh    = LoadMeshSafe(AetheriaWorldConst::MeshTwigBranch02, nullptr);
	UStaticMesh* BarkAMesh     = LoadMeshSafe(AetheriaWorldConst::MeshBarkDebrisA, nullptr);
	UStaticMesh* BarkBMesh     = LoadMeshSafe(AetheriaWorldConst::MeshBarkDebrisB, nullptr);
	UStaticMesh* DryLeafMesh   = LoadMeshSafe(AetheriaWorldConst::MeshDryLeafA, nullptr);
	UStaticMesh* PebblesAMesh  = LoadMeshSafe(AetheriaWorldConst::MeshPebbleStonesA, nullptr);
	UStaticMesh* PebblesBMesh  = LoadMeshSafe(AetheriaWorldConst::MeshPebbleStonesB, nullptr);
	UStaticMesh* UrsiniaMesh   = LoadMeshSafe(AetheriaWorldConst::MeshFlowerUrsinia, nullptr);
	UStaticMesh* EmpodiumMesh  = LoadMeshSafe(AetheriaWorldConst::MeshFlowerEmpodium, nullptr);
	UStaticMesh* FernMesh      = LoadMeshSafe(AetheriaWorldConst::MeshKiteFern01, nullptr);
	UStaticMesh* IceplantMesh  = LoadMeshSafe(AetheriaWorldConst::MeshCrystalIceplant, nullptr);
	UStaticMesh* SucculentMesh = LoadMeshSafe(AetheriaWorldConst::MeshSucculentRock, nullptr);

	const float Twig01Unit    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Twig01Mesh, 34.f, 0.18f, 1.8f);
	const float Twig02Unit    = AetheriaWorldHelpers::ComputeUniformScaleForHeight(Twig02Mesh, 30.f, 0.18f, 1.8f);
	const float BarkAUnit     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BarkAMesh, 16.f, 0.18f, 1.6f);
	const float BarkBUnit     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(BarkBMesh, 15.f, 0.18f, 1.6f);
	const float DryLeafUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(DryLeafMesh, 22.f, 0.18f, 1.6f);
	const float PebbleAUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PebblesAMesh, 20.f, 0.18f, 1.8f);
	const float PebbleBUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PebblesBMesh, 18.f, 0.18f, 1.8f);
	const float UrsiniaUnit   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(UrsiniaMesh, 44.f, 0.20f, 2.4f);
	const float EmpodiumUnit  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(EmpodiumMesh, 38.f, 0.20f, 2.4f);
	const float FernUnit      = AetheriaWorldHelpers::ComputeUniformScaleForHeight(FernMesh, 68.f, 0.22f, 2.2f);
	const float IceplantUnit  = AetheriaWorldHelpers::ComputeUniformScaleForHeight(IceplantMesh, 34.f, 0.20f, 2.2f);
	const float SucculentUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(SucculentMesh, 30.f, 0.20f, 2.0f);

	FRandomStream DetailRng(718293);

	TArray<FTransform> Twig01Transforms, Twig02Transforms;
	TArray<FTransform> BarkATransforms, BarkBTransforms, DryLeafTransforms;
	TArray<FTransform> PebbleATransforms, PebbleBTransforms;
	TArray<FTransform> UrsiniaTransforms, EmpodiumTransforms;
	TArray<FTransform> FernTransforms, IceplantTransforms, SucculentTransforms;

	Twig01Transforms.Reserve(260);
	Twig02Transforms.Reserve(260);
	BarkATransforms.Reserve(240);
	BarkBTransforms.Reserve(240);
	DryLeafTransforms.Reserve(220);
	PebbleATransforms.Reserve(320);
	PebbleBTransforms.Reserve(320);
	UrsiniaTransforms.Reserve(280);
	EmpodiumTransforms.Reserve(280);
	FernTransforms.Reserve(220);
	IceplantTransforms.Reserve(160);
	SucculentTransforms.Reserve(140);

	// 1. Place fallen branches, twigs, curled bark & woodland ferns around every HISM tree trunk across Oakhaven Valley & Blue-Leaf Woods!
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& ExistingHISM : FoliageHISMs)
	{
		if (!ExistingHISM)
		{
			continue;
		}
		const FString HISMName = ExistingHISM->GetName();
		if (!HISMName.Contains(TEXT("Trees"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		const int32 NumTrees = ExistingHISM->GetInstanceCount();
		for (int32 TIdx = 0; TIdx < NumTrees; ++TIdx)
		{
			FTransform TreeXf;
			if (!ExistingHISM->GetInstanceTransform(TIdx, TreeXf, true))
			{
				continue;
			}
			const FVector TreeLoc = TreeXf.GetLocation();

			// 1-2 fallen branches/twigs per tree root zone
			const int32 NumTwigs = DetailRng.RandRange(1, 2);
			for (int32 K = 0; K < NumTwigs; ++K)
			{
				const float Angle = DetailRng.FRandRange(0.f, UE_TWO_PI);
				const float Dist = DetailRng.FRandRange(85.f, 240.f);
				const float X = TreeLoc.X + FMath::Cos(Angle) * Dist;
				const float Y = TreeLoc.Y + FMath::Sin(Angle) * Dist;
				const float GZ = SampleGroundZ(X, Y, TreeLoc.Z);
				const bool bVar2 = (DetailRng.FRand() < 0.5f);
				UStaticMesh* M = bVar2 ? Twig02Mesh : Twig01Mesh;
				if (M)
				{
					const float S = (bVar2 ? Twig02Unit : Twig01Unit) * DetailRng.FRandRange(0.72f, 1.35f);
					const float Z = AetheriaWorldHelpers::ComputeGroundedZ(M, GZ, S, 3.f);
					const FRotator Rot(DetailRng.FRandRange(-7.f, 7.f), DetailRng.FRandRange(0.f, 360.f), DetailRng.FRandRange(-7.f, 7.f));
					if (bVar2) Twig02Transforms.Emplace(Rot, FVector(X, Y, Z), FVector(S));
					else       Twig01Transforms.Emplace(Rot, FVector(X, Y, Z), FVector(S));
				}
			}

			// Curled bark debris & fallen leaf cluster near trunk base
			if (DetailRng.FRand() < 0.80f)
			{
				const float Angle = DetailRng.FRandRange(0.f, UE_TWO_PI);
				const float Dist = DetailRng.FRandRange(55.f, 155.f);
				const float X = TreeLoc.X + FMath::Cos(Angle) * Dist;
				const float Y = TreeLoc.Y + FMath::Sin(Angle) * Dist;
				const float GZ = SampleGroundZ(X, Y, TreeLoc.Z);
				const bool bVarB = (DetailRng.FRand() < 0.5f);
				UStaticMesh* BM = bVarB ? BarkBMesh : BarkAMesh;
				if (BM)
				{
					const float S = (bVarB ? BarkBUnit : BarkAUnit) * DetailRng.FRandRange(0.80f, 1.45f);
					const float Z = AetheriaWorldHelpers::ComputeGroundedZ(BM, GZ, S, 2.f);
					const FRotator Rot(DetailRng.FRandRange(-5.f, 5.f), DetailRng.FRandRange(0.f, 360.f), DetailRng.FRandRange(-5.f, 5.f));
					if (bVarB) BarkBTransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S));
					else       BarkATransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S));
				}
			}

			if (DryLeafMesh && DetailRng.FRand() < 0.55f)
			{
				const float Angle = DetailRng.FRandRange(0.f, UE_TWO_PI);
				const float Dist = DetailRng.FRandRange(70.f, 195.f);
				const float X = TreeLoc.X + FMath::Cos(Angle) * Dist;
				const float Y = TreeLoc.Y + FMath::Sin(Angle) * Dist;
				const float GZ = SampleGroundZ(X, Y, TreeLoc.Z);
				const float S = DryLeafUnit * DetailRng.FRandRange(0.75f, 1.30f);
				const float Z = AetheriaWorldHelpers::ComputeGroundedZ(DryLeafMesh, GZ, S, 2.f);
				DryLeafTransforms.Emplace(FRotator(0.f, DetailRng.FRandRange(0.f, 360.f), 0.f), FVector(X, Y, Z), FVector(S));
			}

			// Shaded forest fern tucked beneath tree canopy
			if (FernMesh && DetailRng.FRand() < 0.62f)
			{
				const float Angle = DetailRng.FRandRange(0.f, UE_TWO_PI);
				const float Dist = DetailRng.FRandRange(95.f, 210.f);
				const float X = TreeLoc.X + FMath::Cos(Angle) * Dist;
				const float Y = TreeLoc.Y + FMath::Sin(Angle) * Dist;
				const float GZ = SampleGroundZ(X, Y, TreeLoc.Z);
				const float S = FernUnit * DetailRng.FRandRange(0.78f, 1.28f);
				const float Z = AetheriaWorldHelpers::ComputeGroundedZ(FernMesh, GZ, S, 4.f);
				FernTransforms.Emplace(FRotator(0.f, DetailRng.FRandRange(0.f, 360.f), 0.f), FVector(X, Y, Z), FVector(S));
			}
		}
	}

	// 2. Meadow Golden Ursinia & White-Gold Empodium WildflowerRibbons + King's Road & Mirror Lake Pebble Scatter
	int32 Attempts = 0;
	while ((UrsiniaTransforms.Num() + EmpodiumTransforms.Num()) < 520 && Attempts < 2400)
	{
		++Attempts;
		const float X = DetailRng.FRandRange(-4600.f, 4600.f);
		const float Y = DetailRng.FRandRange(-6400.f, 6200.f);
		if (IsInsideNoGrassZone(X, Y))
		{
			continue;
		}
		const float GZ = SampleGroundZ(X, Y, FallbackGroundZ);
		const bool bEmpodium = (DetailRng.FRand() < 0.48f);
		UStaticMesh* FM = bEmpodium ? EmpodiumMesh : UrsiniaMesh;
		if (!FM)
		{
			continue;
		}
		const float S = (bEmpodium ? EmpodiumUnit : UrsiniaUnit) * DetailRng.FRandRange(0.75f, 1.38f);
		const float Z = AetheriaWorldHelpers::ComputeGroundedZ(FM, GZ, S, 3.f);
		const FRotator Rot(DetailRng.FRandRange(-4.f, 4.f), DetailRng.FRandRange(0.f, 360.f), DetailRng.FRandRange(-4.f, 4.f));
		if (bEmpodium) EmpodiumTransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S));
		else           UrsiniaTransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S));
	}

	// Natural river pebbles & road-verge stones along Mirror Lake banks, Prologue Overlook & King's Road curbs
	for (int32 PIdx = 0; PIdx < 480; ++PIdx)
	{
		float X = 0.f;
		float Y = 0.f;
		const int32 Zone = PIdx % 4;
		if (Zone == 0)
		{
			// Mirror Lake shoreline pebble ring
			const float Angle = DetailRng.FRandRange(0.f, UE_TWO_PI);
			X = FMath::Cos(Angle) * DetailRng.FRandRange(2450.f, 2980.f);
			Y = -800.f + FMath::Sin(Angle) * DetailRng.FRandRange(1950.f, 2450.f);
		}
		else if (Zone == 1)
		{
			// King's Road stone curb verges (Oakhaven -> Bridge & Bridge -> Highkeep)
			const float Side = (DetailRng.FRand() < 0.5f) ? -1.f : 1.f;
			X = Side * DetailRng.FRandRange(265.f, 430.f);
			Y = (DetailRng.FRand() < 0.35f)
				? DetailRng.FRandRange(1700.f, 2960.f)
				: DetailRng.FRandRange(-7850.f, -3320.f);
		}
		else if (Zone == 2)
		{
			// Ash Caldera & Eastern Trail volcanic gravel scatter
			X = DetailRng.FRandRange(4800.f, 9200.f);
			Y = DetailRng.FRandRange(-3600.f, 1600.f);
		}
		else
		{
			// Highkeep Citadel outer rocky approach & Oakhaven hillsides
			X = DetailRng.FRandRange(-3600.f, 3600.f);
			Y = DetailRng.FRandRange(-8200.f, -6800.f);
		}

		const float GZ = SampleGroundZ(X, Y, FallbackGroundZ);
		const bool bVarB = (DetailRng.FRand() < 0.5f);
		UStaticMesh* PM = bVarB ? PebblesBMesh : PebblesAMesh;
		if (!PM)
		{
			continue;
		}
		const float S = (bVarB ? PebbleBUnit : PebbleAUnit) * DetailRng.FRandRange(0.70f, 1.45f);
		const float Z = AetheriaWorldHelpers::ComputeGroundedZ(PM, GZ, S, 4.f);
		const FRotator Rot(DetailRng.FRandRange(-8.f, 8.f), DetailRng.FRandRange(0.f, 360.f), DetailRng.FRandRange(-8.f, 8.f));
		if (bVarB) PebbleBTransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S));
		else       PebbleATransforms.Emplace(Rot, FVector(X, Y, Z), FVector(S));
	}

	// Prologue Cliff Overlook Ledge pebbles & weathered twigs framing the dramatic waterfall brink
	{
		const float CliffZ = ComputeMirrorLakeWaterZ() + 2200.f;
		for (int32 CIdx = 0; CIdx < 28; ++CIdx)
		{
			const float Side = (CIdx % 2 == 0) ? -1.f : 1.f;
			const float X = DetailRng.FRandRange(-2440.f, -2010.f);
			const float Y = -800.f + Side * DetailRng.FRandRange(220.f, 430.f);
			if (PebblesAMesh)
			{
				const float S = PebbleAUnit * DetailRng.FRandRange(0.75f, 1.30f);
				const float Z = AetheriaWorldHelpers::ComputeGroundedZ(PebblesAMesh, CliffZ, S, 3.f);
				PebbleATransforms.Emplace(FRotator(0.f, DetailRng.FRandRange(0.f, 360.f), 0.f), FVector(X, Y, Z), FVector(S));
			}
			if (Twig01Mesh && (CIdx % 3 == 0))
			{
				const float TS = Twig01Unit * DetailRng.FRandRange(0.70f, 1.15f);
				const float TZ = AetheriaWorldHelpers::ComputeGroundedZ(Twig01Mesh, CliffZ, TS, 2.f);
				Twig01Transforms.Emplace(FRotator(0.f, DetailRng.FRandRange(0.f, 360.f), 0.f), FVector(X + 24.f, Y, TZ), FVector(TS));
			}
		}
	}

	// 3. Crystal Glacier Peaks Crystalline Iceplants & Ash Caldera Rim Succulents
	for (int32 GIdx = 0; GIdx < 150; ++GIdx)
	{
		const float Angle = DetailRng.FRandRange(0.f, UE_TWO_PI);
		const float Dist = DetailRng.FRandRange(520.f, 2950.f);
		const float X = -6800.f + FMath::Cos(Angle) * Dist;
		const float Y = -6400.f + FMath::Sin(Angle) * Dist;
		const float GZ = SampleGroundZ(X, Y, FallbackGroundZ);
		if (IceplantMesh)
		{
			const float S = IceplantUnit * DetailRng.FRandRange(0.80f, 1.45f);
			const float Z = AetheriaWorldHelpers::ComputeGroundedZ(IceplantMesh, GZ, S, 3.f);
			IceplantTransforms.Emplace(FRotator(0.f, DetailRng.FRandRange(0.f, 360.f), 0.f), FVector(X, Y, Z), FVector(S));
		}
	}

	for (int32 SIdx = 0; SIdx < 120; ++SIdx)
	{
		const float Angle = DetailRng.FRandRange(0.f, UE_TWO_PI);
		const float Dist = DetailRng.FRandRange(1350.f, 2900.f);
		const float X = 9800.f + FMath::Cos(Angle) * Dist;
		const float Y = -2400.f + FMath::Sin(Angle) * Dist;
		const float GZ = SampleGroundZ(X, Y, FallbackGroundZ);
		if (SucculentMesh)
		{
			const float S = SucculentUnit * DetailRng.FRandRange(0.80f, 1.40f);
			const float Z = AetheriaWorldHelpers::ComputeGroundedZ(SucculentMesh, GZ, S, 3.f);
			SucculentTransforms.Emplace(FRotator(0.f, DetailRng.FRandRange(0.f, 360.f), 0.f), FVector(X, Y, Z), FVector(S));
		}
	}

	if (Twig01HISM && Twig01Transforms.Num() > 0)       Twig01HISM->AddInstances(Twig01Transforms, false, true, false);
	if (Twig02HISM && Twig02Transforms.Num() > 0)       Twig02HISM->AddInstances(Twig02Transforms, false, true, false);
	if (BarkAHISM && BarkATransforms.Num() > 0)         BarkAHISM->AddInstances(BarkATransforms, false, true, false);
	if (BarkBHISM && BarkBTransforms.Num() > 0)         BarkBHISM->AddInstances(BarkBTransforms, false, true, false);
	if (DryLeafHISM && DryLeafTransforms.Num() > 0)     DryLeafHISM->AddInstances(DryLeafTransforms, false, true, false);
	if (PebblesAHISM && PebbleATransforms.Num() > 0)    PebblesAHISM->AddInstances(PebbleATransforms, false, true, false);
	if (PebblesBHISM && PebbleBTransforms.Num() > 0)    PebblesBHISM->AddInstances(PebbleBTransforms, false, true, false);
	if (UrsiniaHISM && UrsiniaTransforms.Num() > 0)     UrsiniaHISM->AddInstances(UrsiniaTransforms, false, true, false);
	if (EmpodiumHISM && EmpodiumTransforms.Num() > 0)   EmpodiumHISM->AddInstances(EmpodiumTransforms, false, true, false);
	if (FernHISM && FernTransforms.Num() > 0)           FernHISM->AddInstances(FernTransforms, false, true, false);
	if (IceplantHISM && IceplantTransforms.Num() > 0)   IceplantHISM->AddInstances(IceplantTransforms, false, true, false);
	if (SucculentHISM && SucculentTransforms.Num() > 0) SucculentHISM->AddInstances(SucculentTransforms, false, true, false);

	// 4. Biome-Wide Light World Destructibles (roadside vessels, glacier frost shards, woodland stone cairns — all regrow after 60s!)
	if (HasAuthority())
	{
		UStaticMesh* PotMesh     = LoadMeshSafe(AetheriaWorldConst::MeshMedievalPot, AetheriaWorldConst::MeshDungeonPillar);
		UStaticMesh* CrystalMesh = LoadMeshSafe(AetheriaWorldConst::MeshCoreCrystal, AetheriaWorldConst::MeshDungeonPillar);
		UStaticMesh* StoneMesh   = LoadMeshSafe(AetheriaWorldConst::MeshMWAMStoneB, AetheriaWorldConst::MeshDungeonPillar);

		const float PotScale     = AetheriaWorldHelpers::ComputeUniformScaleForHeight(PotMesh, 96.f, 0.2f, 2.4f);
		const float CrystalScale = AetheriaWorldHelpers::ComputeUniformScaleForHeight(CrystalMesh, 195.f, 0.2f, 2.2f);
		const float CairnScale   = AetheriaWorldHelpers::ComputeUniformScaleForHeight(StoneMesh, 145.f, 0.2f, 2.0f);
		const FLinearColor FrostGlow(0.28f, 0.88f, 1.35f, 1.0f);

		const FVector2D RoadsidePots[4] = {
			FVector2D(-340.f,  2380.f),
			FVector2D( 340.f,  2120.f),
			FVector2D(-360.f, -4450.f),
			FVector2D( 360.f, -6120.f)
		};
		for (int32 Idx = 0; Idx < 4; ++Idx)
		{
			const float GZ = SampleGroundZ(RoadsidePots[Idx].X, RoadsidePots[Idx].Y, FallbackGroundZ);
			SpawnDestructibleProp(
				FVector(RoadsidePots[Idx].X, RoadsidePots[Idx].Y, AetheriaWorldHelpers::ComputeGroundedZ(PotMesh, GZ, PotScale, 4.f)),
				FRotator(0.f, Idx * 67.f, 0.f),
				EDestructibleKind::Stone,
				45.f,
				AetheriaWorldConst::MeshMedievalPot,
				FVector(PotScale),
				NAME_None);
		}

		const FVector2D GlacierShards[3] = {
			FVector2D(-6120.f, -5920.f),
			FVector2D(-7380.f, -6080.f),
			FVector2D(-6650.f, -7120.f)
		};
		for (int32 Idx = 0; Idx < 3; ++Idx)
		{
			const float GZ = SampleGroundZ(GlacierShards[Idx].X, GlacierShards[Idx].Y, FallbackGroundZ);
			SpawnDestructibleProp(
				FVector(GlacierShards[Idx].X, GlacierShards[Idx].Y, AetheriaWorldHelpers::ComputeGroundedZ(CrystalMesh, GZ, CrystalScale, 12.f)),
				FRotator(0.f, Idx * 115.f, 0.f),
				EDestructibleKind::Crystal,
				65.f,
				AetheriaWorldConst::MeshCoreCrystal,
				FVector(CrystalScale),
				NAME_None,
				&FrostGlow,
				true);
		}

		const FVector2D WoodlandCairns[3] = {
			FVector2D(-4850.f,  1950.f),
			FVector2D( 4920.f,  1680.f),
			FVector2D( 8450.f, -1580.f)
		};
		for (int32 Idx = 0; Idx < 3; ++Idx)
		{
			const float GZ = SampleGroundZ(WoodlandCairns[Idx].X, WoodlandCairns[Idx].Y, FallbackGroundZ);
			SpawnDestructibleProp(
				FVector(WoodlandCairns[Idx].X, WoodlandCairns[Idx].Y, AetheriaWorldHelpers::ComputeGroundedZ(StoneMesh, GZ, CairnScale, 28.f)),
				FRotator(0.f, Idx * 92.f, 0.f),
				EDestructibleKind::Stone,
				60.f,
				AetheriaWorldConst::MeshMWAMStoneB,
				FVector(CairnScale),
				NAME_None);
		}
	}
}

// ============================================================================
// Spell-Reactive Landscape Scarring, Foliage Flattening & 60s Magical Restoration
// ============================================================================

void AAetheriaWorldBuilder::NotifySpellTerrainImpact(
	const FVector& WorldLocation,
	float ImpactRadius,
	EChronicleOrb PrimaryElement)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float ClampedRadius = FMath::Clamp(ImpactRadius, 180.f, 680.f);
	const float GroundZ = SampleGroundZ(WorldLocation.X, WorldLocation.Y, FallbackGroundZ);

	// Ignore sky-high impacts far above the terrain surface
	if (WorldLocation.Z > GroundZ + 1350.f)
	{
		return;
	}

	// Skip submerged Mirror Lake basin floor except along shorelines
	const float LakeNormX = WorldLocation.X / 2350.f;
	const float LakeNormY = (WorldLocation.Y + 800.f) / 1850.f;
	if ((LakeNormX * LakeNormX + LakeNormY * LakeNormY) < 0.88f && FMath::Abs(WorldLocation.X) > 360.f)
	{
		return;
	}

	const FVector GroundCenter(WorldLocation.X, WorldLocation.Y, GroundZ);

	// Refresh timer if an existing scar already occupies almost the exact same spot (e.g. continuous Dragonfire sweep)
	for (FActiveTerrainScar& Existing : ActiveTerrainScars)
	{
		if (FVector::DistSquared2D(Existing.WorldCenter, GroundCenter) <= FMath::Square(145.f))
		{
			Existing.TimeRemaining = 60.f;
			if (Existing.ScarLight)
			{
				Existing.ScarLight->SetIntensity(Existing.BaseLightLumens * 1.35f);
			}
			return;
		}
	}

	// Cap simultaneous active scars so extended battles remain 60+ FPS
	while (ActiveTerrainScars.Num() >= 36)
	{
		RestoreSingleTerrainScar(0, false);
	}

	FActiveTerrainScar Scar;
	Scar.WorldCenter = GroundCenter;
	Scar.Radius = ClampedRadius;
	Scar.Element = PrimaryElement;
	Scar.TimeRemaining = 60.f; // Restores magically after 1 minute (60 seconds)!

	// 1. Flatten & radially part nearby non-colliding HISM meadow grass, wildflowers, bushes, ferns & twigs
	const float FlattenRadius = ClampedRadius * 0.92f;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& HISM : FoliageHISMs)
	{
		if (!IsValid(HISM) || HISM->IsCollisionEnabled())
		{
			continue;
		}

		const TArray<int32> Overlapping = HISM->GetInstancesOverlappingSphere(GroundCenter, FlattenRadius, true);
		if (Overlapping.Num() == 0)
		{
			continue;
		}

		const uint64 CompKeyPrefix = (static_cast<uint64>(HISM->GetUniqueID()) << 32);
		bool bAnyUpdated = false;

		for (const int32 InstIdx : Overlapping)
		{
			FTransform CurXf;
			if (!HISM->GetInstanceTransform(InstIdx, CurXf, true))
			{
				continue;
			}

			const uint64 FoliageKey = CompKeyPrefix | static_cast<uint32>(InstIdx);
			if (!OriginalFoliageTransformMap.Contains(FoliageKey))
			{
				OriginalFoliageTransformMap.Add(FoliageKey, CurXf);
			}
			const FTransform OrigXf = OriginalFoliageTransformMap.FindRef(FoliageKey);

			const FVector OrigPos = OrigXf.GetLocation();
			const FVector OrigScale = OrigXf.GetScale3D();
			const FVector OutDir = (OrigPos - GroundCenter).GetSafeNormal2D(UE_SMALL_NUMBER, FVector::ForwardVector);
			const FQuat TiltQuat(FVector::CrossProduct(FVector::UpVector, OutDir).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector), FMath::DegreesToRadians(62.f));

			FTransform FlattenedXf = OrigXf;
			FlattenedXf.SetLocation(OrigPos + OutDir * 14.f - FVector(0.f, 0.f, 8.f));
			FlattenedXf.SetRotation(TiltQuat * OrigXf.GetRotation());
			FlattenedXf.SetScale3D(FVector(OrigScale.X * 0.85f, OrigScale.Y * 0.85f, OrigScale.Z * 0.14f));

			HISM->UpdateInstanceTransform(InstIdx, FlattenedXf, true, false, true);
			bAnyUpdated = true;

			FFlattenedFoliageRecord Rec;
			Rec.HISM = HISM;
			Rec.InstanceIndex = InstIdx;
			Rec.FoliageKey = FoliageKey;
			Scar.FlattenedFoliage.Add(Rec);
		}

		if (bAnyUpdated)
		{
			HISM->MarkRenderStateDirty();
		}
	}

	// 2. Spawn Elemental Landscape Reaction (charred crater / frost rime / storm fulgurite + displaced rim clods)
	if (!IsRunningDedicatedServer())
	{
		auto SpawnTempMesh = [&](const TCHAR* MeshPath, const FVector& Loc, const FRotator& Rot, const FVector& Scale, UMaterialInterface* Mat) -> UStaticMeshComponent*
		{
			UStaticMesh* Mesh = LoadMeshSafe(MeshPath, AetheriaWorldConst::MeshCylinder);
			if (!Mesh)
			{
				return nullptr;
			}
			const FName UniqueName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("SpellTerrainScarMesh"));
			UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, UniqueName);
			if (!Comp)
			{
				return nullptr;
			}
			Comp->SetStaticMesh(Mesh);
			Comp->SetMobility(EComponentMobility::Movable);
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetCastShadow(false);
			Comp->bDisallowNanite = true;
			if (Mat)
			{
				const int32 NumMats = FMath::Max(1, Comp->GetNumMaterials());
				for (int32 MIdx = 0; MIdx < NumMats; ++MIdx)
				{
					Comp->SetMaterial(MIdx, Mat);
				}
			}
			Comp->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
			Comp->SetWorldLocationAndRotation(Loc, Rot);
			Comp->SetWorldScale3D(Scale);
			Comp->RegisterComponent();
			Scar.ScarMeshes.Add(Comp);
			return Comp;
		};

		const float DiscScaleXY = FMath::Clamp(ClampedRadius / 82.f, 1.8f, 5.2f);
		UMaterialInterface* RimDirtMat = LoadPBRMaterial({
			AetheriaWorldConst::MatDirtMounds,
			AetheriaWorldConst::MatMWAMCoverRocks,
			AetheriaWorldConst::MatStoneWall
		});

		FLinearColor ScarLightColor(1.0f, 0.48f, 0.12f, 1.0f);
		UMaterialInterface* CraterCenterMat = RimDirtMat;

		if (PrimaryElement == EChronicleOrb::Quas)
		{
			ScarLightColor = FLinearColor(0.30f, 0.86f, 1.0f, 1.0f);
			CraterCenterMat = LoadPBRMaterial({ AetheriaWorldConst::MatAetheriaPrismWall });
			if (!CraterCenterMat)
			{
				CraterCenterMat = CreateColorMID(FLinearColor(0.32f, 0.90f, 1.35f, 1.0f), true);
			}
		}
		else if (PrimaryElement == EChronicleOrb::Wex)
		{
			ScarLightColor = FLinearColor(0.62f, 0.42f, 1.0f, 1.0f);
			CraterCenterMat = LoadPBRMaterial({ AetheriaWorldConst::MatFirePitInst, AetheriaWorldConst::MatDirtMounds });
		}
		else
		{
			ScarLightColor = FLinearColor(1.0f, 0.44f, 0.10f, 1.0f);
			CraterCenterMat = LoadPBRMaterial({ AetheriaWorldConst::MatFirePitInst, AetheriaWorldConst::MatDirtMounds });
		}

		// Ground-hugging elemental scorch / frost rime patch
		SpawnTempMesh(
			AetheriaWorldConst::MeshCylinder,
			GroundCenter + FVector(0.f, 0.f, 1.5f),
			FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f),
			FVector(DiscScaleXY, DiscScaleXY, 0.035f),
			CraterCenterMat);

		// 4 Displaced earth/stone/crystal chunks upturned around the blast rim
		const TCHAR* RimMeshPath = (PrimaryElement == EChronicleOrb::Quas)
			? AetheriaWorldConst::MeshCoreCrystal
			: AetheriaWorldConst::MeshDirtMoundA;
		UStaticMesh* RimRefMesh = LoadMeshSafe(RimMeshPath, AetheriaWorldConst::MeshMWAMStoneA);
		const float RimUnit = AetheriaWorldHelpers::ComputeUniformScaleForHeight(
			RimRefMesh,
			(PrimaryElement == EChronicleOrb::Quas) ? 68.f : 36.f,
			0.12f,
			1.6f);

		for (int32 RIdx = 0; RIdx < 4; ++RIdx)
		{
			const float Angle = (static_cast<float>(RIdx) / 4.f) * UE_TWO_PI + FMath::FRandRange(-0.28f, 0.28f);
			const float RimDist = ClampedRadius * FMath::FRandRange(0.45f, 0.72f);
			const float RX = GroundCenter.X + FMath::Cos(Angle) * RimDist;
			const float RY = GroundCenter.Y + FMath::Sin(Angle) * RimDist;
			const float RGZ = SampleGroundZ(RX, RY, GroundZ);
			const FRotator RimRot(FMath::FRandRange(-16.f, 16.f), FMath::RadiansToDegrees(Angle) + 90.f, FMath::FRandRange(-12.f, 12.f));
			const FVector RimScale = (PrimaryElement == EChronicleOrb::Quas)
				? FVector(RimUnit * 0.85f, RimUnit * 0.85f, RimUnit * FMath::FRandRange(0.9f, 1.35f))
				: FVector(RimUnit * 1.45f, RimUnit * 1.15f, RimUnit * 0.68f);

			SpawnTempMesh(
				RimMeshPath,
				AetheriaWorldHelpers::ComputeGroundedCenterLoc(RimRefMesh, RX, RY, RGZ, RimRot, RimScale, 10.f),
				RimRot,
				RimScale,
				(PrimaryElement == EChronicleOrb::Quas) ? CraterCenterMat : RimDirtMat);
		}

		// Subtle residual elemental particle wisp (smoldering embers / static sparks / frost mist)
		if (PrimaryElement == EChronicleOrb::Wex)
		{
			if (UNiagaraSystem* ZapNS = LoadNiagaraSafe(AetheriaWorldConst::NSZapBlue))
			{
				const FName ZapName = MakeUniqueObjectName(this, UNiagaraComponent::StaticClass(), TEXT("ScarZapNiagara"));
				if (UNiagaraComponent* NC = NewObject<UNiagaraComponent>(this, ZapName))
				{
					NC->SetAsset(ZapNS);
					NC->SetMobility(EComponentMobility::Movable);
					NC->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
					NC->SetWorldLocation(GroundCenter + FVector(0.f, 0.f, 18.f));
					NC->SetWorldScale3D(FVector(0.55f));
					NC->RegisterComponent();
					NC->Activate(true);
					Scar.ScarNiagara.Add(NC);
				}
			}
		}
		else
		{
			const TCHAR* WispPath = (PrimaryElement == EChronicleOrb::Quas)
				? AetheriaWorldConst::ParticleWaterBall
				: AetheriaWorldConst::ParticlePitFire;
			if (UParticleSystem* WispPS = LoadParticleSafe(WispPath))
			{
				const FName WispName = MakeUniqueObjectName(this, UParticleSystemComponent::StaticClass(), TEXT("ScarWispParticle"));
				if (UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(this, WispName))
				{
					PSC->SetTemplate(WispPS);
					PSC->bAutoActivate = true;
					PSC->bAutoDestroy = false;
					PSC->SetMobility(EComponentMobility::Movable);
					PSC->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
					PSC->SetWorldLocation(GroundCenter + FVector(0.f, 0.f, 8.f));
					PSC->SetWorldScale3D(FVector(0.52f));
					PSC->RegisterComponent();
					PSC->ActivateSystem(true);
					Scar.ScarParticles.Add(PSC);
				}
			}
		}

		// Subtle physical AgX residual crater glow
		const FName ScarLightName = MakeUniqueObjectName(this, UPointLightComponent::StaticClass(), TEXT("ScarResidualLight"));
		if (UPointLightComponent* Light = NewObject<UPointLightComponent>(this, ScarLightName))
		{
			Scar.BaseLightLumens = 380.f;
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetIntensityUnits(ELightUnits::Lumens);
			Light->SetIntensity(Scar.BaseLightLumens);
			Light->SetAttenuationRadius(ClampedRadius * 2.1f);
			Light->SetSourceRadius(22.f);
			Light->SetSpecularScale(0.12f);
			Light->SetLightColor(ScarLightColor);
			Light->SetCastShadows(false);
			Light->AttachToComponent(WorldRoot, FAttachmentTransformRules::KeepWorldTransform);
			Light->SetWorldLocation(GroundCenter + FVector(0.f, 0.f, 36.f));
			Light->RegisterComponent();
			Scar.ScarLight = Light;
		}
	}

	ActiveTerrainScars.Add(MoveTemp(Scar));
}

void AAetheriaWorldBuilder::UpdateTerrainScarsAndMagicalRestoration(float DeltaTime)
{
	if (ActiveTerrainScars.Num() == 0)
	{
		return;
	}

	for (int32 Idx = ActiveTerrainScars.Num() - 1; Idx >= 0; --Idx)
	{
		FActiveTerrainScar& Scar = ActiveTerrainScars[Idx];
		Scar.TimeRemaining -= DeltaTime;

		if (Scar.TimeRemaining <= 0.f)
		{
			RestoreSingleTerrainScar(Idx, true);
			continue;
		}

		// Pre-restoration emerald shimmer during final 2.0 seconds before magical regrowth
		if (Scar.ScarLight)
		{
			if (Scar.TimeRemaining <= 2.0f)
			{
				const float HealAlpha = 1.f - (Scar.TimeRemaining / 2.0f);
				Scar.ScarLight->SetLightColor(FMath::Lerp(Scar.ScarLight->GetLightColor(), FLinearColor(0.28f, 1.0f, 0.62f, 1.0f), HealAlpha));
				Scar.ScarLight->SetIntensity(FMath::Lerp(Scar.BaseLightLumens, 1450.f, HealAlpha));
			}
			else
			{
				const float Pulse = 0.88f + 0.12f * FMath::Sin(ElapsedAnimTime * 4.2f + static_cast<float>(Idx));
				Scar.ScarLight->SetIntensity(Scar.BaseLightLumens * Pulse);
			}
		}
	}
}

void AAetheriaWorldBuilder::RestoreSingleTerrainScar(int32 ScarIndex, bool bSpawnMagicalVFX)
{
	if (!ActiveTerrainScars.IsValidIndex(ScarIndex))
	{
		return;
	}

	FActiveTerrainScar ScarToRestore = MoveTemp(ActiveTerrainScars[ScarIndex]);
	ActiveTerrainScars.RemoveAt(ScarIndex);

	// 1. Restore flattened meadow grass, wildflowers, bushes & twigs (unless still covered by another active scar)
	TSet<UHierarchicalInstancedStaticMeshComponent*> DirtyHISMs;
	for (const FFlattenedFoliageRecord& Rec : ScarToRestore.FlattenedFoliage)
	{
		UHierarchicalInstancedStaticMeshComponent* HISM = Rec.HISM.Get();
		if (!IsValid(HISM) || Rec.InstanceIndex == INDEX_NONE)
		{
			continue;
		}

		bool bStillCoveredByOtherScar = false;
		for (const FActiveTerrainScar& Other : ActiveTerrainScars)
		{
			for (const FFlattenedFoliageRecord& OtherRec : Other.FlattenedFoliage)
			{
				if (OtherRec.FoliageKey == Rec.FoliageKey)
				{
					bStillCoveredByOtherScar = true;
					break;
				}
			}
			if (bStillCoveredByOtherScar)
			{
				break;
			}
		}

		if (!bStillCoveredByOtherScar)
		{
			if (const FTransform* OrigXf = OriginalFoliageTransformMap.Find(Rec.FoliageKey))
			{
				HISM->UpdateInstanceTransform(Rec.InstanceIndex, *OrigXf, true, false, true);
				OriginalFoliageTransformMap.Remove(Rec.FoliageKey);
				DirtyHISMs.Add(HISM);
			}
		}
	}

	for (UHierarchicalInstancedStaticMeshComponent* HISM : DirtyHISMs)
	{
		if (IsValid(HISM))
		{
			HISM->MarkRenderStateDirty();
		}
	}

	// 2. Clean up temporary scar visual components
	for (TObjectPtr<UStaticMeshComponent>& Comp : ScarToRestore.ScarMeshes)
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
	}
	for (TObjectPtr<UParticleSystemComponent>& PSC : ScarToRestore.ScarParticles)
	{
		if (IsValid(PSC))
		{
			PSC->DeactivateSystem();
			PSC->DestroyComponent();
		}
	}
	for (TObjectPtr<UNiagaraComponent>& NC : ScarToRestore.ScarNiagara)
	{
		if (IsValid(NC))
		{
			NC->Deactivate();
			NC->DestroyComponent();
		}
	}
	if (IsValid(ScarToRestore.ScarLight))
	{
		ScarToRestore.ScarLight->DestroyComponent();
	}

	// 3. Spawn glowing emerald-gold Magical Restoration VFX & petals over the healed terrain
	if (bSpawnMagicalVFX && !IsRunningDedicatedServer() && GetWorld())
	{
		UWorld* World = GetWorld();
		const FVector HealOrigin = ScarToRestore.WorldCenter + FVector(0.f, 0.f, 18.f);

		if (UNiagaraSystem* HealNS = LoadNiagaraSafe(AetheriaWorldConst::NSMagicRestoreHeal))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HealNS, HealOrigin, FRotator::ZeroRotator, FVector(1.20f), true, true);
		}
		if (UNiagaraSystem* CircleNS = LoadNiagaraSafe(AetheriaWorldConst::NSMagicRestoreCircle))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, CircleNS, HealOrigin, FRotator::ZeroRotator, FVector(1.10f), true, true);
		}
		if (UNiagaraSystem* PetalsNS = LoadNiagaraSafe(AetheriaWorldConst::NSWindPetals))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, PetalsNS, HealOrigin + FVector(0.f, 0.f, 35.f), FRotator::ZeroRotator, FVector(0.95f), true, true);
		}
		if (UParticleSystem* HealAuraPS = LoadParticleSafe(AetheriaWorldConst::ParticleHealAura))
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, HealAuraPS, HealOrigin, FRotator::ZeroRotator, FVector(1.15f), true);
		}
	}
}

int32 AAetheriaWorldBuilder::RestoreAllActiveTerrainScarsNow()
{
	const int32 TotalToRestore = ActiveTerrainScars.Num();
	for (int32 Idx = TotalToRestore - 1; Idx >= 0; --Idx)
	{
		RestoreSingleTerrainScar(Idx, true);
	}
	OriginalFoliageTransformMap.Empty();
	return TotalToRestore;
}
