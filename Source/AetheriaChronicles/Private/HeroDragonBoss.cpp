// ============================================================================
// HeroDragonBoss.cpp — Replicated Dragon, Wukong-Grade Hero, Elemental Enemies,
// Malakor Multi-Phase Telegraphed Boss & Named Quest Giver NPCs (UE 5.8.4)
// ============================================================================
#include "HeroDragonBoss.h"

#include "AudioAndStyleSubsystem.h"
#include "DestructionAndWorld.h"
#include "HeroCustomizationComponent.h"
#include "QuestAndMapSubsystem.h"
#include "SkillTreeComponent.h"
#include "SpellSystemComponent.h"

#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "MotionWarpingComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace AetheriaActorsUtil
{
	static constexpr TCHAR PathSphere[]        = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	static constexpr TCHAR PathCylinder[]      = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	static constexpr TCHAR PathCone[]          = TEXT("/Engine/BasicShapes/Cone.Cone");
	static constexpr TCHAR PathCube[]          = TEXT("/Engine/BasicShapes/Cube.Cube");
	static constexpr TCHAR PathMatBasic[]        = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	static constexpr TCHAR PathMatEmissive[]     = TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");
	static constexpr TCHAR PathMatMasterPBR[]    = TEXT("/Game/Materials/M_AetheriaMasterPBR.M_AetheriaMasterPBR");
	static constexpr TCHAR PathMatObsidian[]     = TEXT("/Game/Materials/M_AetheriaObsidian.M_AetheriaObsidian");
	static constexpr TCHAR PathMatDarkIron[]     = TEXT("/Game/Materials/M_AetheriaDarkIron.M_AetheriaDarkIron");
	static constexpr TCHAR PathMatEmissiveCore[] = TEXT("/Game/Materials/M_AetheriaEmissiveCore.M_AetheriaEmissiveCore");
	static constexpr TCHAR PathMatFireball[]     = TEXT("/Game/Materials/M_AetheriaFireball.M_AetheriaFireball");

	static constexpr TCHAR PathABPCombat[]     = TEXT("/Game/Variant_Combat/Anims/ABP_Manny_Combat.ABP_Manny_Combat_C");
	static constexpr TCHAR PathABPUnarmed[]    = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");

	static constexpr TCHAR PathAnimAttack1[]   = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01");
	static constexpr TCHAR PathAnimAttack2[]   = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02");
	static constexpr TCHAR PathAnimAttack3[]   = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03");
	static constexpr TCHAR PathAnimCharged[]   = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack");
	static constexpr TCHAR PathAnimDash[]      = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash.MM_Dash");
	static constexpr TCHAR PathAnimHitReact[]  = TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01");
	static constexpr TCHAR PathAnimDeath[]     = TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01");

	static constexpr TCHAR PathHeroFallback0[] = TEXT("/Game/Characters/Heroes/SKM_Hero_00.SKM_Hero_00");
	static constexpr TCHAR PathMannySimple[]   = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	static constexpr TCHAR PathQuinnSimple[]   = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple");
	static constexpr TCHAR PathMobSwarm[]      = TEXT("/Game/Characters/Mobs/SKM_Mob_Swarm.SKM_Mob_Swarm");
	static constexpr TCHAR PathMobBruiser[]    = TEXT("/Game/Characters/Mobs/SKM_Mob_Bruiser.SKM_Mob_Bruiser");
	static constexpr TCHAR PathMobRanged[]     = TEXT("/Game/Characters/Mobs/SKM_Mob_Ranged.SKM_Mob_Ranged");
	static constexpr TCHAR PathBossMain[]      = TEXT("/Game/Characters/Mobs/SKM_Boss_Main.SKM_Boss_Main");
	static constexpr TCHAR PathStaffArcane[]   = TEXT("/Game/Meshes/SM_Staff_Arcane01.SM_Staff_Arcane01");
	static constexpr TCHAR PathLongswordMesh[] = TEXT("/Game/Longsword/Longsword/SM_Longsword.SM_Longsword");
	static constexpr TCHAR PathSwordMesh[]     = TEXT("/Game/Sword/Sword/SM_Sword.SM_Sword");

	// Staged Fab / VaultCache High-Poly Skeletal Packs: FourEvilDragonsHP (DragonTheUsurper)
	static constexpr TCHAR PathDragonUsurperMesh[]    = TEXT("/Game/FourEvilDragonsHP/Meshes/DragonTheUsurper/DragonTheUsurperSK.DragonTheUsurperSK");
	static constexpr TCHAR PathDragonUsurperRedMat[]  = TEXT("/Game/FourEvilDragonsHP/Materials/DragonTheUsurper/RedMat.RedMat");
	static constexpr TCHAR PathDragonUsurperIdle[]    = TEXT("/Game/FourEvilDragonsHP/Animations/DragonTheUsurper/Idle01Anim.Idle01Anim");
	static constexpr TCHAR PathDragonUsurperTakeOff[] = TEXT("/Game/FourEvilDragonsHP/Animations/DragonTheUsurper/TakeOffAnim.TakeOffAnim");
	static constexpr TCHAR PathDragonUsurperFly[]     = TEXT("/Game/FourEvilDragonsHP/Animations/DragonTheUsurper/FlyForwardAnim.FlyForwardAnim");
	static constexpr TCHAR PathDragonUsurperGlide[]   = TEXT("/Game/FourEvilDragonsHP/Animations/DragonTheUsurper/FlyGlideAnim.FlyGlideAnim");
	static constexpr TCHAR PathDragonUsurperFlame[]   = TEXT("/Game/FourEvilDragonsHP/Animations/DragonTheUsurper/FlyFlameAnim.FlyFlameAnim");
	static constexpr TCHAR PathDragonUsurperLand[]    = TEXT("/Game/FourEvilDragonsHP/Animations/DragonTheUsurper/LandAnim.LandAnim");

	// Staged Fab Skeletal Creature: Whisper (Crypt Wraith / Tempest Stalker)
	static constexpr TCHAR PathWhisperMesh[]     = TEXT("/Game/Whisper/Base_Mesh/SK_Whisper.SK_Whisper");
	static constexpr TCHAR PathWhisperIdle[]     = TEXT("/Game/Whisper/Animations/Anim_Whisper_Idle1.Anim_Whisper_Idle1");
	static constexpr TCHAR PathWhisperRun[]      = TEXT("/Game/Whisper/Animations/Anim_Whisper_Run.Anim_Whisper_Run");
	static constexpr TCHAR PathWhisperAttack1[]  = TEXT("/Game/Whisper/Animations/Anim_Whisper_Attack1.Anim_Whisper_Attack1");
	static constexpr TCHAR PathWhisperAttack2[]  = TEXT("/Game/Whisper/Animations/Anim_Whisper_Attack2.Anim_Whisper_Attack2");
	static constexpr TCHAR PathWhisperAttack3[]  = TEXT("/Game/Whisper/Animations/Anim_Whisper_Attack3.Anim_Whisper_Attack3");
	static constexpr TCHAR PathWhisperHitReact[] = TEXT("/Game/Whisper/Animations/Anim_Whisper_Taking_Damage1.Anim_Whisper_Taking_Damage1");
	static constexpr TCHAR PathWhisperDeath[]    = TEXT("/Game/Whisper/Animations/Anim_Whisper_Death.Anim_Whisper_Death");

	// Staged Paragon Rampage (ForestTroll & Boss Malakor)
	static constexpr TCHAR PathRampageMesh[]     = TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Meshes/Rampage.Rampage");
	static constexpr TCHAR PathRampageABP[]      = TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Rampage_AnimBlueprint.Rampage_AnimBlueprint_C");
	static constexpr TCHAR PathRampageMeleeA[]   = TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/Attack_Biped_Melee_A_Montage.Attack_Biped_Melee_A_Montage");
	static constexpr TCHAR PathRampageMeleeB[]   = TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/Attack_Biped_Melee_B_Montage.Attack_Biped_Melee_B_Montage");
	static constexpr TCHAR PathRampageMeleeC[]   = TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/Attack_Biped_Melee_C_Montage.Attack_Biped_Melee_C_Montage");
	static constexpr TCHAR PathRampageRoar[]     = TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/Emote_Master_Roar_T3.Emote_Master_Roar_T3");
	static constexpr TCHAR PathRampageHitReact[] = TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathRampageDeath[]    = TEXT("/Game/ParagonRampage/Characters/Heroes/Rampage/Animations/Death_A.Death_A");

	// Paragon Sevarog (Boss Malakor priority) & Paragon Heroes (Auto-upgrade when staged)
	static constexpr TCHAR PathSevarogBloodredMesh[]  = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Skins/Tier_1/Sevarog_Red/Meshes/SevarogBloodred.SevarogBloodred");
	static constexpr TCHAR PathSevarogMesh[]          = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog.Sevarog");
	static constexpr TCHAR PathSevarogABP[]           = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sevarog_AnimBlueprint.Sevarog_AnimBlueprint_C");
	static constexpr TCHAR PathSevarogSwing1Montage[] = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Swing1_Medium_Montage.Swing1_Medium_Montage");
	static constexpr TCHAR PathSevarogSwing2Montage[] = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Swing2_Medium_Montage.Swing2_Medium_Montage");
	static constexpr TCHAR PathSevarogSwing3Seq[]     = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Swing3_Medium.Swing3_Medium");
	static constexpr TCHAR PathSevarogUltSwingSeq[]   = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Ultimate_Swing_120fps.Ultimate_Swing_120fps");
	static constexpr TCHAR PathSevarogHitReact[]      = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Hitreact_Front.Hitreact_Front");
	static constexpr TCHAR PathSevarogDeath[]         = TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Death_front.Death_front");
	static constexpr TCHAR PathBossExplosionFX[]      = TEXT("/Game/FXVarietyPack/Particles/P_ky_explosion.P_ky_explosion");
	static constexpr TCHAR PathBossShockwaveNS[]      = TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave.NS_Free_Spells_Shockwave");
	static constexpr TCHAR PathGideonMesh[]         = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Meshes/Gideon.Gideon");
	static constexpr TCHAR PathGideonToughMesh[]    = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Skins/Tough/Meshes/Gideon_Tough.Gideon_Tough");
	static constexpr TCHAR PathGideonABP[]          = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Gideon_AnimBlueprint.Gideon_AnimBlueprint_C");
	static constexpr TCHAR PathGideonPedestalIdle[] = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/HeroPedestal_Idle.HeroPedestal_Idle");
	static constexpr TCHAR PathGideonIdle[]         = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Idle.Idle");
	static constexpr TCHAR PathGideonRun[]          = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Jog_Fwd.Jog_Fwd");
	static constexpr TCHAR PathGideonCast[]         = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Cast.Cast");
	static constexpr TCHAR PathGideonAttackASeq[]   = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_A_Medium.Primary_Attack_A_Medium");
	static constexpr TCHAR PathGideonAttackBSeq[]   = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_B_Medium.Primary_Attack_B_Medium");
	static constexpr TCHAR PathGideonAttackCSeq[]   = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Primary_Attack_C_Medium.Primary_Attack_C_Medium");
	static constexpr TCHAR PathGideonHitReact[]     = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathGideonDeath[]        = TEXT("/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Death_Fwd.Death_Fwd");

	static constexpr TCHAR PathAuroraMesh[]         = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora.Aurora");
	static constexpr TCHAR PathAuroraABP[]          = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Aurora_AnimBlueprint.Aurora_AnimBlueprint_C");
	static constexpr TCHAR PathAuroraIdle[]         = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Idle.Idle");
	static constexpr TCHAR PathAuroraRun[]          = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Fwd.Jog_Fwd");
	static constexpr TCHAR PathAuroraRunBwd[]       = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Bwd.Jog_Bwd");
	static constexpr TCHAR PathAuroraCast[]         = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Cast.Cast");
	static constexpr TCHAR PathAuroraAttackASeq[]   = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_A.Primary_Attack_A");
	static constexpr TCHAR PathAuroraAttackBSeq[]   = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_B.Primary_Attack_B");
	static constexpr TCHAR PathAuroraAttackCSeq[]   = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_C.Primary_Attack_C");
	static constexpr TCHAR PathAuroraHitReact[]     = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathAuroraDeath[]        = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Death.Death");

	static constexpr TCHAR PathGruxMesh[]           = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Meshes/Grux.Grux");
	static constexpr TCHAR PathGruxABP[]            = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Grux_AnimBlueprint.Grux_AnimBlueprint_C");
	static constexpr TCHAR PathGruxIdle[]           = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/Idle.Idle");
	static constexpr TCHAR PathGruxRun[]            = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/Jog_Fwd.Jog_Fwd");
	static constexpr TCHAR PathGruxCast[]           = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/Cast.Cast");
	static constexpr TCHAR PathGruxAttackASeq[]     = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/PrimaryAttack_LA.PrimaryAttack_LA");
	static constexpr TCHAR PathGruxAttackBSeq[]     = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/PrimaryAttack_RA.PrimaryAttack_RA");
	static constexpr TCHAR PathGruxAttackCSeq[]     = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/PrimaryAttack_FourStrikes.PrimaryAttack_FourStrikes");
	static constexpr TCHAR PathGruxHitReact[]       = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathGruxDeath[]          = TEXT("/Game/ParagonGrux/Characters/Heroes/Grux/Animations/Death_A.Death_A");

	static constexpr TCHAR PathMorigeshMesh[]       = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Meshes/Morigesh.Morigesh");
	static constexpr TCHAR PathMorigeshABP[]        = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Morigesh_AnimBlueprint.Morigesh_AnimBlueprint_C");
	static constexpr TCHAR PathMorigeshIdle[]       = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/Idle.Idle");
	static constexpr TCHAR PathMorigeshRun[]        = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/Jog_Fwd.Jog_Fwd");
	static constexpr TCHAR PathMorigeshCast[]       = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/Cast.Cast");
	static constexpr TCHAR PathMorigeshAttackASeq[] = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/PrimaryAttack_A_Slow.PrimaryAttack_A_Slow");
	static constexpr TCHAR PathMorigeshAttackBSeq[] = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/PrimaryAttack_B_Slow.PrimaryAttack_B_Slow");
	static constexpr TCHAR PathMorigeshAttackCSeq[] = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/PrimaryAttack_C_Slow.PrimaryAttack_C_Slow");
	static constexpr TCHAR PathMorigeshHitReact[]   = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathMorigeshDeath[]      = TEXT("/Game/ParagonMorigesh/Characters/Heroes/Morigesh/Animations/Death.Death");
	static constexpr TCHAR PathKwangMesh[]     = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Meshes/Kwang_GDC.Kwang_GDC");
	static constexpr TCHAR PathKwangABP[]      = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Kwang_AnimBlueprint.Kwang_AnimBlueprint_C");
	static constexpr TCHAR PathKwangMeleeA[]   = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_A_Slow_Montage.PrimaryAttack_A_Slow_Montage");
	static constexpr TCHAR PathKwangMeleeB[]   = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_B_Slow_Montage.PrimaryAttack_B_Slow_Montage");
	static constexpr TCHAR PathKwangMeleeC[]   = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_C_Slow_Montage.PrimaryAttack_C_Slow_Montage");
	static constexpr TCHAR PathKwangMeleeD[]   = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_D_Slow_Montage.PrimaryAttack_D_Slow_Montage");
	static constexpr TCHAR PathKwangMeleeASeq[] = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_A_Slow.PrimaryAttack_A_Slow");
	static constexpr TCHAR PathKwangMeleeBSeq[] = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_B_Slow.PrimaryAttack_B_Slow");
	static constexpr TCHAR PathKwangMeleeCSeq[] = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/PrimaryAttack_C_Slow.PrimaryAttack_C_Slow");
	static constexpr TCHAR PathKwangHitReact[] = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Hitreact_Fwd.Hitreact_Fwd");
	static constexpr TCHAR PathKwangDeath[]    = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Death_Bwd.Death_Bwd");
	static constexpr TCHAR PathKwangCast[]     = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Animations/Cast.Cast");
	static constexpr TCHAR PathKwangPrimaryImpactFX[]    = TEXT("/Game/ParagonKwang/FX/Particles/Abilities/Primary/FX/P_Kwang_Primary_Impact.P_Kwang_Primary_Impact");
	static constexpr TCHAR PathKwangLightStrikeBurstFX[] = TEXT("/Game/ParagonKwang/FX/Particles/Abilities/LightStrike/FX/P_Kwang_LightStrike_Burst.P_Kwang_LightStrike_Burst");
	static constexpr TCHAR PathKwangEffortPrimarySFX[]   = TEXT("/Game/ParagonKwang/Characters/Heroes/Kwang/Sounds/SoundCues/Kwang_Effort_Ability_Primary.Kwang_Effort_Ability_Primary");
	static constexpr TCHAR PathSerathMesh[]    = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Meshes/Serath.Serath");
	static constexpr TCHAR PathSerathABP[]     = TEXT("/Game/ParagonSerath/Characters/Heroes/Serath/Serath_AnimBlueprint.Serath_AnimBlueprint_C");
	static constexpr TCHAR PathGreystoneMesh[] = TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone.Greystone");
	static constexpr TCHAR PathGreystoneABP[]  = TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Greystone_AnimBlueprint.Greystone_AnimBlueprint_C");

	// Staged Paragon Khaimera (FrostWarden / Highland Berserker)
	static constexpr TCHAR PathKhaimeraMesh[]       = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Meshes/Khaimera.Khaimera");
	static constexpr TCHAR PathKhaimeraABP[]        = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Khaimera_AnimBlueprint.Khaimera_AnimBlueprint_C");
	static constexpr TCHAR PathKhaimeraIdle[]       = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Idle.Idle");
	static constexpr TCHAR PathKhaimeraRun[]        = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Jog_Fwd.Jog_Fwd");
	static constexpr TCHAR PathKhaimeraCast[]       = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Cast.Cast");
	static constexpr TCHAR PathKhaimeraAttackASeq[] = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_A.Melee_A");
	static constexpr TCHAR PathKhaimeraAttackBSeq[] = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_B.Melee_B");
	static constexpr TCHAR PathKhaimeraAttackCSeq[] = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_C.Melee_C");
	static constexpr TCHAR PathKhaimeraMeleeA[]     = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_A_Montage.Melee_A_Montage");
	static constexpr TCHAR PathKhaimeraMeleeB[]     = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_B_Montage.Melee_B_Montage");
	static constexpr TCHAR PathKhaimeraMeleeC[]     = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Melee_C_Montage.Melee_C_Montage");
	static constexpr TCHAR PathKhaimeraHitReact[]   = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/HitReact_Front.HitReact_Front");
	static constexpr TCHAR PathKhaimeraDeath[]      = TEXT("/Game/ParagonKhaimera/Characters/Heroes/Khaimera/Animations/Death_A.Death_A");

	// Staged Dragon Fire FX, Free_Spells Niagara & FreeModularMagicSFX
	static constexpr TCHAR PathDragonFireStormFX[]   = TEXT("/Game/FXVarietyPack/Particles/P_ky_fireStorm.P_ky_fireStorm");
	static constexpr TCHAR PathDragonFireBallFX[]    = TEXT("/Game/FXVarietyPack/Particles/P_ky_fireBall.P_ky_fireBall");
	static constexpr TCHAR PathSFXFireBall[]         = TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_FireBall1_1.SW_MagicCast_FireBall1_1");
	static constexpr TCHAR PathNSAuraFire[]          = TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Fire.NS_Free_Spells_Aura_Fire");
	static constexpr TCHAR PathNSAuraLightning[]     = TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Lightning.NS_Free_Spells_Aura_Lightning");
	static constexpr TCHAR PathNSAuraSoul[]          = TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Soul.NS_Free_Spells_Aura_Soul");
	static constexpr TCHAR PathNSLevelUp[]           = TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_LevelUp.NS_Free_Spells_LevelUp");
	static constexpr TCHAR PathSFXMagicBuff[]        = TEXT("/Game/FreeModularMagicSFX/C_Cast/Mono/SW_MagicCast_Buff1_1.SW_MagicCast_Buff1_1");

	static bool DoesObjectPackageExist(const TCHAR* ObjectPath)
	{
		if (!ObjectPath || ObjectPath[0] == TEXT('\0'))
		{
			return false;
		}
		const FString PackageName = FPackageName::ObjectPathToPackageName(FString(ObjectPath));
		return !PackageName.IsEmpty() && FPackageName::DoesPackageExist(PackageName);
	}

	template <typename T>
	static T* SafeLoadObjectIfPresent(const TCHAR* ObjectPath)
	{
		if (!DoesObjectPackageExist(ObjectPath))
		{
			return nullptr;
		}
		return LoadObject<T>(nullptr, ObjectPath);
	}

	template <typename T>
	static UClass* SafeLoadClassIfPresent(const TCHAR* ClassPath)
	{
		if (!DoesObjectPackageExist(ClassPath))
		{
			return nullptr;
		}
		return LoadClass<T>(nullptr, ClassPath);
	}

	static FQuat MakeCapsuleSweepQuat(const FVector& Direction)
	{
		const FVector Dir = Direction.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		return FQuat::FindBetweenNormals(FVector::UpVector, Dir);
	}

	static USkeletalMesh* FindFirstSkeletalMeshInPaths(const TArray<FString>& PackagePaths)
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
		{
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		}

		FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if (!AssetRegistryModule)
		{
			return nullptr;
		}

		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		const FTopLevelAssetPath SkelMeshClassPath = USkeletalMesh::StaticClass()->GetClassPathName();

		for (const FString& RawPath : PackagePaths)
		{
			FString CleanPath = RawPath;
			while (CleanPath.EndsWith(TEXT("/")))
			{
				CleanPath.LeftChopInline(1);
			}
			if (CleanPath.IsEmpty())
			{
				continue;
			}

			FARFilter Filter;
			Filter.PackagePaths.Add(FName(*CleanPath));
			Filter.bRecursivePaths = true;
			Filter.ClassPaths.Add(SkelMeshClassPath);

			TArray<FAssetData> FoundAssets;
			AssetRegistry.GetAssets(Filter, FoundAssets);

			for (const FAssetData& AssetData : FoundAssets)
			{
				const FString AssetNameStr = AssetData.AssetName.ToString();
				if (AssetNameStr.Contains(TEXT("Retarget")) || AssetNameStr.Contains(TEXT("Shadow")) || AssetNameStr.Contains(TEXT("Extent")))
				{
					continue;
				}
				if (USkeletalMesh* LoadedMesh = Cast<USkeletalMesh>(AssetData.GetAsset()))
				{
					if (LoadedMesh->GetSkeleton() != nullptr)
					{
						return LoadedMesh;
					}
				}
			}
		}

		return nullptr;
	}

	static bool IsSkeletonCompatible(USkeletalMesh* SkelMesh, const USkeleton* TargetSkeleton)
	{
		if (!SkelMesh || !TargetSkeleton)
		{
			return false;
		}
		const USkeleton* MeshSkeleton = SkelMesh->GetSkeleton();
		if (!MeshSkeleton)
		{
			return false;
		}
		return (MeshSkeleton == TargetSkeleton) || TargetSkeleton->IsCompatibleMesh(SkelMesh);
	}

	static bool TryBindCompatibleAnimBPFromList(
		USkeletalMeshComponent* MeshComp,
		const TArray<const TCHAR*>& CandidatePaths)
	{
		if (!MeshComp)
		{
			return false;
		}

		USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset();
		if (!SkelMesh || !SkelMesh->GetSkeleton())
		{
			return false;
		}

		for (const TCHAR* Path : CandidatePaths)
		{
			if (!Path || Path[0] == TEXT('\0'))
			{
				continue;
			}

			UClass* AnimClass = SafeLoadClassIfPresent<UAnimInstance>(Path);
			if (!AnimClass)
			{
				continue;
			}

			const UAnimBlueprintGeneratedClass* AnimBPClass = Cast<UAnimBlueprintGeneratedClass>(AnimClass);
			if (AnimBPClass)
			{
				const USkeleton* TargetSkel = AnimBPClass->GetTargetSkeleton();
				if (TargetSkel && !IsSkeletonCompatible(SkelMesh, TargetSkel))
				{
					continue;
				}
			}

			MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			MeshComp->SetAnimInstanceClass(AnimClass);
			return true;
		}

		return false;
	}

	static bool TryBindCompatibleAnimBP(
		USkeletalMeshComponent* MeshComp,
		const TCHAR* PrimaryABPPath,
		const TCHAR* FallbackABPPath = nullptr)
	{
		TArray<const TCHAR*> Candidates;
		if (PrimaryABPPath && PrimaryABPPath[0] != TEXT('\0'))
		{
			Candidates.Add(PrimaryABPPath);
		}
		if (FallbackABPPath && FallbackABPPath[0] != TEXT('\0'))
		{
			Candidates.Add(FallbackABPPath);
		}
		return TryBindCompatibleAnimBPFromList(MeshComp, Candidates);
	}

	static void PlaySafeDynamicMontage(
		USkeletalMeshComponent* MeshComp,
		UAnimSequence* Seq,
		float PlayRate = 1.f,
		float BlendIn = 0.08f,
		float BlendOut = 0.16f,
		FName SlotName = FName(TEXT("DefaultSlot")))
	{
		if (!MeshComp || !Seq)
		{
			return;
		}

		USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset();
		const USkeleton* SeqSkeleton = Seq->GetSkeleton();
		if (!SkelMesh || !SeqSkeleton || !IsSkeletonCompatible(SkelMesh, SeqSkeleton))
		{
			return;
		}

		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			AnimInst->PlaySlotAnimationAsDynamicMontage(
				Seq,
				SlotName,
				BlendIn,
				BlendOut,
				PlayRate,
				1);
		}
	}

	static UMaterialInstanceDynamic* CreateColorMID(
		UObject* Outer,
		const FLinearColor& Color,
		bool bEmissive,
		float Metallic = 0.f,
		float Roughness = 0.45f)
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

	static bool TraceGroundZ(
		const UWorld* World,
		float X,
		float Y,
		float StartZ,
		float EndZ,
		const TArray<const AActor*>& IgnoredActors,
		float& OutGroundZ)
	{
		if (!World)
		{
			return false;
		}

		FCollisionQueryParams Params(TEXT("AetheriaGroundTrace"), false);
		for (const AActor* Ignored : IgnoredActors)
		{
			if (Ignored)
			{
				Params.AddIgnoredActor(Ignored);
			}
		}

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(
				Hit,
				FVector(X, Y, StartZ),
				FVector(X, Y, EndZ),
				ECC_WorldStatic,
				Params))
		{
			OutGroundZ = Hit.ImpactPoint.Z;
			return true;
		}

		return false;
	}

	static UMaterialInstanceDynamic* CreateDragonPBRMID(
		UObject* Outer,
		const TCHAR* PrimaryMatPath,
		const TCHAR* SecondaryMatPath,
		const FLinearColor& BaseColor,
		float Metallic,
		float Roughness,
		float Specular,
		const FLinearColor& RimAccentColor,
		float EmissiveBoost = 0.0f)
	{
		UMaterialInterface* BaseMat = SafeLoadObjectIfPresent<UMaterialInterface>(PrimaryMatPath);
		if (!BaseMat && SecondaryMatPath)
		{
			BaseMat = SafeLoadObjectIfPresent<UMaterialInterface>(SecondaryMatPath);
		}
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
			MID->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
			MID->SetVectorParameterValue(TEXT("Color"), BaseColor);
			MID->SetVectorParameterValue(TEXT("AccentColor"), RimAccentColor);
			MID->SetVectorParameterValue(TEXT("EmissiveColor"), RimAccentColor);
			MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
			MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
			MID->SetScalarParameterValue(TEXT("Specular"), Specular);
			MID->SetScalarParameterValue(TEXT("EmissiveBoost"), EmissiveBoost);
			MID->SetScalarParameterValue(TEXT("ClothWind"), 0.0f);
		}
		return MID;
	}

	static FLinearColor GetOrbVisualColor(EChronicleOrb Orb)
	{
		switch (Orb)
		{
		case EChronicleOrb::Exort:
			return FLinearColor(1.00f, 0.28f, 0.04f, 1.0f); // Rich deep molten orange
		case EChronicleOrb::Wex:
			return FLinearColor(0.52f, 0.18f, 1.00f, 1.0f); // Royal arcane violet
		case EChronicleOrb::Quas:
			return FLinearColor(0.12f, 0.68f, 1.00f, 1.0f); // Frost sapphire
		default:
			return FLinearColor(0.35f, 0.35f, 0.40f, 1.0f);
		}
	}

	static AAetheriaHeroCharacter* ResolveHeroFromInstigator(AController* InstigatorCtrl, AActor* DamageCauser)
	{
		if (InstigatorCtrl)
		{
			if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(InstigatorCtrl->GetPawn()))
			{
				return Hero;
			}
			if (AAetheriaDragonActor* Dragon = Cast<AAetheriaDragonActor>(InstigatorCtrl->GetPawn()))
			{
				return Dragon->RiderHero.Get();
			}
		}

		if (DamageCauser)
		{
			if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(DamageCauser))
			{
				return Hero;
			}
			if (AAetheriaHeroCharacter* OwnerHero = Cast<AAetheriaHeroCharacter>(DamageCauser->GetOwner()))
			{
				return OwnerHero;
			}
			if (AAetheriaDragonActor* Dragon = Cast<AAetheriaDragonActor>(DamageCauser))
			{
				return Dragon->RiderHero.Get();
			}
		}

		return nullptr;
	}

	static void SpawnBossShockwaveAndExplosionFX(UWorld* World, const FVector& Origin, float Scale)
	{
		if (!World || IsRunningDedicatedServer())
		{
			return;
		}
		if (UNiagaraSystem* ShockwaveNS = SafeLoadObjectIfPresent<UNiagaraSystem>(PathBossShockwaveNS))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ShockwaveNS, Origin, FRotator::ZeroRotator, FVector(Scale), true);
		}
		if (UParticleSystem* ExplosionFX = SafeLoadObjectIfPresent<UParticleSystem>(PathBossExplosionFX))
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, ExplosionFX, Origin, FRotator::ZeroRotator, FVector(Scale * 0.85f), true);
		}
	}
}

// ============================================================================
// 1. AAetheriaDragonActor — Rideable Fire-Breathing Red Dragon
// ============================================================================

AAetheriaDragonActor::AAetheriaDragonActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	bAlwaysRelevant = true;

	DragonRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DragonRoot"));
	SetRootComponent(DragonRoot);

	ImportedDragonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ImportedDragonMesh"));
	ImportedDragonMesh->SetupAttachment(DragonRoot);
	ImportedDragonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImportedDragonMesh->SetVisibility(false, true);

	ProceduralDragonRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ProceduralDragonRoot"));
	ProceduralDragonRoot->SetupAttachment(DragonRoot);

	NeckPivot = CreateDefaultSubobject<USceneComponent>(TEXT("NeckPivot"));
	NeckPivot->SetupAttachment(ProceduralDragonRoot);
	NeckPivot->SetRelativeLocation(FVector(145.f, 0.f, 35.f));

	HeadPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivot"));
	HeadPivot->SetupAttachment(NeckPivot);
	HeadPivot->SetRelativeLocation(FVector(120.f, 0.f, 52.f));

	LowerJawPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LowerJawPivot"));
	LowerJawPivot->SetupAttachment(HeadPivot);
	LowerJawPivot->SetRelativeLocation(FVector(25.f, 0.f, -14.f));

	LeftWingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LeftWingPivot"));
	LeftWingPivot->SetupAttachment(ProceduralDragonRoot);
	LeftWingPivot->SetRelativeLocation(FVector(35.f, -62.f, 44.f));

	RightWingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("RightWingPivot"));
	RightWingPivot->SetupAttachment(ProceduralDragonRoot);
	RightWingPivot->SetRelativeLocation(FVector(35.f, 62.f, 44.f));

	SaddleSocket = CreateDefaultSubobject<USceneComponent>(TEXT("SaddleSocket"));
	SaddleSocket->SetupAttachment(DragonRoot);
	SaddleSocket->SetRelativeLocation(FVector(22.f, 0.f, 108.f));

	MouthSocket = CreateDefaultSubobject<USceneComponent>(TEXT("MouthSocket"));
	MouthSocket->SetupAttachment(HeadPivot);
	MouthSocket->SetRelativeLocation(FVector(118.f, 0.f, -4.f));

	// Cache engine basic shapes safely in constructor
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(AetheriaActorsUtil::PathSphere);
	if (SphereFinder.Succeeded())
	{
		SphereMeshAsset = SphereFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(AetheriaActorsUtil::PathCylinder);
	if (CylinderFinder.Succeeded())
	{
		CylinderMeshAsset = CylinderFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(AetheriaActorsUtil::PathCone);
	if (ConeFinder.Succeeded())
	{
		ConeMeshAsset = ConeFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(AetheriaActorsUtil::PathCube);
	if (CubeFinder.Succeeded())
	{
		CubeMeshAsset = CubeFinder.Object;
	}

	// Blazing dragonfire breath outer & inner cone meshes attached to MouthSocket
	FireBreathConeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireBreathConeMesh"));
	FireBreathConeMesh->SetupAttachment(MouthSocket);
	FireBreathConeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireBreathConeMesh->SetCastShadow(false);
	FireBreathConeMesh->SetRelativeLocation(FVector(620.f, 0.f, 0.f));
	FireBreathConeMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	FireBreathConeMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 12.5f));
	FireBreathConeMesh->SetVisibility(false);
	if (ConeMeshAsset)
	{
		FireBreathConeMesh->SetStaticMesh(ConeMeshAsset);
	}

	FireBreathCoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireBreathCoreMesh"));
	FireBreathCoreMesh->SetupAttachment(MouthSocket);
	FireBreathCoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireBreathCoreMesh->SetCastShadow(false);
	FireBreathCoreMesh->SetRelativeLocation(FVector(460.f, 0.f, 0.f));
	FireBreathCoreMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	FireBreathCoreMesh->SetRelativeScale3D(FVector(1.05f, 1.05f, 9.2f));
	FireBreathCoreMesh->SetVisibility(false);
	if (ConeMeshAsset)
	{
		FireBreathCoreMesh->SetStaticMesh(ConeMeshAsset);
	}

	// Physical AgX Lumen mouth fire light (subtle 450 lm idle -> 3,200 lm fire breath so sunset sky never clips)
	MouthFireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MouthFireLight"));
	MouthFireLight->SetupAttachment(MouthSocket);
	MouthFireLight->SetIntensityUnits(ELightUnits::Lumens);
	MouthFireLight->SetIntensity(450.f);
	MouthFireLight->SetLightColor(FLinearColor(1.0f, 0.40f, 0.08f, 1.0f));
	MouthFireLight->SetAttenuationRadius(2600.f);
	MouthFireLight->SetSourceRadius(32.f);
	MouthFireLight->SetSpecularScale(0.45f);
	MouthFireLight->SetCastShadows(false);

	// Sweeping cinematic dragon flight camera boom (unobstructed full-wingspan Crimson Desert framing)
	FlightArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("FlightArm"));
	FlightArm->SetupAttachment(DragonRoot);
	FlightArm->TargetArmLength = 1520.f;
	FlightArm->SocketOffset = FVector(0.f, 0.f, 380.f);
	FlightArm->SetRelativeRotation(FRotator(-14.f, 0.f, 0.f));
	FlightArm->bUsePawnControlRotation = false;
	FlightArm->bInheritPitch = true;
	FlightArm->bInheritYaw = true;
	FlightArm->bInheritRoll = false;
	FlightArm->bEnableCameraLag = true;
	FlightArm->CameraLagSpeed = 7.5f;
	FlightArm->bEnableCameraRotationLag = true;
	FlightArm->CameraRotationLagSpeed = 6.5f;
	FlightArm->bDoCollisionTest = false;

	FlightCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FlightCamera"));
	FlightCamera->SetupAttachment(FlightArm, USpringArmComponent::SocketName);
	FlightCamera->bUsePawnControlRotation = false;
	FlightCamera->FieldOfView = 94.f;
}

void AAetheriaDragonActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAetheriaDragonActor, bMounted);
	DOREPLIFETIME(AAetheriaDragonActor, FireEnergy);
	DOREPLIFETIME(AAetheriaDragonActor, FlightSpeed);
	DOREPLIFETIME(AAetheriaDragonActor, bBreathingFire);
}

void AAetheriaDragonActor::BeginPlay()
{
	Super::BeginPlay();

	// Emissive fire cone dynamic materials (prefer M_AetheriaFireball magma shader when fallback cones are active)
	if (UMaterialInterface* FireballMat = AetheriaActorsUtil::SafeLoadObjectIfPresent<UMaterialInterface>(AetheriaActorsUtil::PathMatFireball))
	{
		FireOuterMID = UMaterialInstanceDynamic::Create(FireballMat, this);
		if (FireOuterMID)
		{
			FireOuterMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.26f, 0.03f, 1.0f));
			FireOuterMID->SetVectorParameterValue(TEXT("AccentColor"), FLinearColor(1.0f, 0.58f, 0.08f, 1.0f));
			FireOuterMID->SetScalarParameterValue(TEXT("EmissiveBoost"), 3.2f);
		}
		FireCoreMID = UMaterialInstanceDynamic::Create(FireballMat, this);
		if (FireCoreMID)
		{
			FireCoreMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.68f, 0.14f, 1.0f));
			FireCoreMID->SetVectorParameterValue(TEXT("AccentColor"), FLinearColor(1.0f, 0.90f, 0.35f, 1.0f));
			FireCoreMID->SetScalarParameterValue(TEXT("EmissiveBoost"), 4.2f);
		}
	}
	else
	{
		FireOuterMID = AetheriaActorsUtil::CreateColorMID(this, FLinearColor(5.5f, 1.5f, 0.18f, 1.0f), true);
		FireCoreMID = AetheriaActorsUtil::CreateColorMID(this, FLinearColor(8.0f, 4.8f, 1.1f, 1.0f), true);
	}

	if (FireBreathConeMesh && FireOuterMID)
	{
		FireBreathConeMesh->SetMaterial(0, FireOuterMID);
	}
	if (FireBreathCoreMesh && FireCoreMID)
	{
		FireBreathCoreMesh->SetMaterial(0, FireCoreMID);
	}

	bUsingImportedSkeletalMesh = TryBindImportedDragonMesh();
	if (!bUsingImportedSkeletalMesh)
	{
		BuildProceduralRedDragon();
	}

	// Safely bind real Cascade dragon fire FX onto MouthSocket (after MouthSocket reparenting)
	auto CreateMouthFirePSC = [this](UParticleSystem* Template, const FVector& RelLoc, const FVector& RelScale) -> UParticleSystemComponent*
	{
		if (!Template || !MouthSocket)
		{
			return nullptr;
		}
		UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(this);
		if (!PSC)
		{
			return nullptr;
		}
		PSC->bAutoActivate = false;
		PSC->bAutoDestroy = false;
		PSC->SetTemplate(Template);
		PSC->SetupAttachment(MouthSocket);
		PSC->SetRelativeLocationAndRotation(RelLoc, FRotator::ZeroRotator);
		PSC->SetRelativeScale3D(RelScale);
		PSC->RegisterComponent();
		PSC->DeactivateSystem();
		return PSC;
	};

	UParticleSystem* FireStormFX = AetheriaActorsUtil::SafeLoadObjectIfPresent<UParticleSystem>(AetheriaActorsUtil::PathDragonFireStormFX);
	UParticleSystem* FireBallFX  = AetheriaActorsUtil::SafeLoadObjectIfPresent<UParticleSystem>(AetheriaActorsUtil::PathDragonFireBallFX);
	DragonFireSFX = AetheriaActorsUtil::SafeLoadObjectIfPresent<USoundBase>(AetheriaActorsUtil::PathSFXFireBall);

	DragonFireStormPSC = CreateMouthFirePSC(FireStormFX, FVector(80.f, 0.f, 0.f), FVector(2.15f));
	DragonFireBallPSC  = CreateMouthFirePSC(FireBallFX,  FVector(140.f, 0.f, 0.f), FVector(1.85f));
	if (DragonFireStormPSC)
	{
		DragonFireStormPSC->SetVisibility(true);
		DragonFireStormPSC->bSuppressSpawning = false;
	}
	if (DragonFireBallPSC)
	{
		DragonFireBallPSC->SetVisibility(true);
		DragonFireBallPSC->bSuppressSpawning = false;
	}
	bHasRealDragonFireFX = (DragonFireStormPSC != nullptr) || (DragonFireBallPSC != nullptr);
	bWasBreathingFire = false;

	if (bHasRealDragonFireFX)
	{
		if (FireBreathConeMesh) { FireBreathConeMesh->SetVisibility(false); }
		if (FireBreathCoreMesh) { FireBreathCoreMesh->SetVisibility(false); }
	}

	FAetheriaStyleHarmonizer::HarmonizeActorMaterials(this, EChronicleStyleCategory::Creature);
}

bool AAetheriaDragonActor::TryBindImportedDragonMesh()
{
	if (!ImportedDragonMesh)
	{
		return false;
	}

	// Priority 1: Custom FabImports dragon folder
	TArray<FString> SearchPaths;
	SearchPaths.Add(TEXT("/Game/FabImports/Dragon"));

	USkeletalMesh* FoundDragonMesh = AetheriaActorsUtil::FindFirstSkeletalMeshInPaths(SearchPaths);
	bool bUsingUsurperMesh = false;

	// Priority 2: Staged FourEvilDragonsHP high-poly skeletal DragonTheUsurperSK
	if (!FoundDragonMesh)
	{
		FoundDragonMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathDragonUsurperMesh);
		bUsingUsurperMesh = (FoundDragonMesh != nullptr);
	}

	if (!FoundDragonMesh)
	{
		return false;
	}

	// IMPORTANT: Reparent SaddleSocket & MouthSocket onto DragonRoot BEFORE hiding ProceduralDragonRoot
	// (MouthSocket defaults under HeadPivot -> ProceduralDragonRoot, so recursive hiding would hide MouthFireLight & fire cones!)
	if (SaddleSocket)
	{
		SaddleSocket->AttachToComponent(DragonRoot, FAttachmentTransformRules::KeepRelativeTransform);
		SaddleSocket->SetRelativeLocationAndRotation(FVector(25.f, 0.f, 105.f), FRotator::ZeroRotator);
		SaddleSocket->SetVisibility(true, false);
	}
	if (MouthSocket)
	{
		MouthSocket->AttachToComponent(DragonRoot, FAttachmentTransformRules::KeepRelativeTransform);
		MouthSocket->SetRelativeLocationAndRotation(FVector(350.f, 0.f, 75.f), FRotator::ZeroRotator);
		MouthSocket->SetVisibility(true, false);
	}
	if (MouthFireLight)
	{
		MouthFireLight->SetVisibility(true);
	}

	ImportedDragonMesh->SetSkeletalMesh(FoundDragonMesh);
	ImportedDragonMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ImportedDragonMesh->SetRelativeLocation(FVector(0.f, 0.f, -120.f));
	ImportedDragonMesh->SetRelativeRotation(FRotator::ZeroRotator);
	ImportedDragonMesh->SetRelativeScale3D(FVector(0.85f));
	ImportedDragonMesh->SetCastShadow(true);
	ImportedDragonMesh->SetVisibility(true, true);

	if (bUsingUsurperMesh)
	{
		if (UMaterialInterface* RedMat = AetheriaActorsUtil::SafeLoadObjectIfPresent<UMaterialInterface>(AetheriaActorsUtil::PathDragonUsurperRedMat))
		{
			ImportedDragonMesh->SetMaterial(0, RedMat);
		}
	}

	if (ProceduralDragonRoot)
	{
		ProceduralDragonRoot->SetVisibility(false, true);
	}

	// Resolve real skeletal bones/sockets on DragonTheUsurperSK or custom dragon for live snout & saddle tracking
	DragonSaddleBoneName = NAME_None;
	static const FName SaddleBoneCandidates[] = {
		FName(TEXT("Saddle")),
		FName(TEXT("Spine02")),
		FName(TEXT("Chest")),
		FName(TEXT("spine_03"))
	};
	for (const FName& BoneName : SaddleBoneCandidates)
	{
		if (ImportedDragonMesh->DoesSocketExist(BoneName))
		{
			DragonSaddleBoneName = BoneName;
			break;
		}
	}

	DragonMouthBoneName = NAME_None;
	static const FName MouthBoneCandidates[] = {
		FName(TEXT("Mouth")),
		FName(TEXT("UpperMouth01")),
		FName(TEXT("Head"))
	};
	for (const FName& BoneName : MouthBoneCandidates)
	{
		if (ImportedDragonMesh->DoesSocketExist(BoneName))
		{
			DragonMouthBoneName = BoneName;
			break;
		}
	}

	// Pre-load all 6 DragonTheUsurper animations into GC-safe UPROPERTY slots
	DragonIdleAnim    = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathDragonUsurperIdle);
	DragonTakeOffAnim = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathDragonUsurperTakeOff);
	DragonFlyAnim     = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathDragonUsurperFly);
	DragonGlideAnim   = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathDragonUsurperGlide);
	DragonFlameAnim   = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathDragonUsurperFlame);
	DragonLandAnim    = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathDragonUsurperLand);
	ActiveDragonAnim  = nullptr;

	// Kick off perched idle immediately so Ignis is alive and breathing from frame 1
	if (DragonIdleAnim && AetheriaActorsUtil::IsSkeletonCompatible(FoundDragonMesh, DragonIdleAnim->GetSkeleton()))
	{
		ImportedDragonMesh->PlayAnimation(DragonIdleAnim, true);
		ActiveDragonAnim = DragonIdleAnim;
	}

	// Mathematically align ImportedDragonMesh so Saddle->Mouth vector faces +X (DragonRoot forward)
	ImportedDragonMesh->RefreshBoneTransforms();
	float AutoAlignedYaw = 0.f;
	if (DragonMouthBoneName != NAME_None && DragonSaddleBoneName != NAME_None)
	{
		const FVector MouthCS = ImportedDragonMesh->GetBoneLocation(DragonMouthBoneName, EBoneSpaces::ComponentSpace);
		const FVector SaddleCS = ImportedDragonMesh->GetBoneLocation(DragonSaddleBoneName, EBoneSpaces::ComponentSpace);
		const FVector MouthDir = (MouthCS - SaddleCS).GetSafeNormal2D();
		if (!MouthDir.IsNearlyZero(0.01f))
		{
			const float NativeForwardYaw = FMath::RadiansToDegrees(FMath::Atan2(MouthDir.Y, MouthDir.X));
			AutoAlignedYaw = FRotator::NormalizeAxis(-NativeForwardYaw);
		}
	}
	ImportedDragonMesh->SetRelativeRotation(FRotator(0.f, AutoAlignedYaw, 0.f));

	UE_LOG(LogChronicles, Log, TEXT("AAetheriaDragonActor bound SkeletalMesh: %s (MouthBone=%s, SaddleBone=%s, AutoAlignedYaw=%.1f deg)"),
		*FoundDragonMesh->GetName(),
		*DragonMouthBoneName.ToString(),
		*DragonSaddleBoneName.ToString(),
		AutoAlignedYaw);
	return true;
}

UStaticMeshComponent* AAetheriaDragonActor::AddProceduralPart(
	const FString& PartName,
	UStaticMesh* MeshAsset,
	USceneComponent* ParentComp,
	const FVector& RelLoc,
	const FRotator& RelRot,
	const FVector& RelScale,
	UMaterialInterface* Mat)
{
	if (!MeshAsset || !ParentComp)
	{
		return nullptr;
	}

	const FName UniqueName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), FName(*PartName));
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, UniqueName);
	if (!Comp)
	{
		return nullptr;
	}

	Comp->SetStaticMesh(MeshAsset);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetGenerateOverlapEvents(false);
	Comp->SetCastShadow(true);

	if (Mat)
	{
		const int32 NumMats = FMath::Max(1, Comp->GetNumMaterials());
		for (int32 i = 0; i < NumMats; ++i)
		{
			Comp->SetMaterial(i, Mat);
		}
	}

	Comp->AttachToComponent(ParentComp, FAttachmentTransformRules::KeepRelativeTransform);
	Comp->SetRelativeLocationAndRotation(RelLoc, RelRot);
	Comp->SetRelativeScale3D(RelScale);
	Comp->RegisterComponent();

	ProceduralParts.Add(Comp);
	return Comp;
}

void AAetheriaDragonActor::BuildProceduralRedDragon()
{
	if (!ProceduralDragonRoot)
	{
		return;
	}

	// Richly shaded Aetheria PBR dragon materials (deep crimson-obsidian scales, burnished bronze-crimson membranes,
	// volcanic obsidian horns, and churning golden-ember M_AetheriaFireball eyes & chest furnace)
	CrimsonScaleMID = AetheriaActorsUtil::CreateDragonPBRMID(
		this,
		AetheriaActorsUtil::PathMatMasterPBR,
		AetheriaActorsUtil::PathMatObsidian,
		FLinearColor(0.58f, 0.055f, 0.038f, 1.0f),
		0.42f,
		0.48f,
		0.36f,
		FLinearColor(0.88f, 0.18f, 0.03f, 1.0f),
		0.22f);

	CrimsonBellyMID = AetheriaActorsUtil::CreateDragonPBRMID(
		this,
		AetheriaActorsUtil::PathMatMasterPBR,
		AetheriaActorsUtil::PathMatDarkIron,
		FLinearColor(0.56f, 0.18f, 0.068f, 1.0f),
		0.28f,
		0.58f,
		0.30f,
		FLinearColor(0.82f, 0.24f, 0.05f, 1.0f),
		0.14f);

	HornObsidianMID = AetheriaActorsUtil::CreateDragonPBRMID(
		this,
		AetheriaActorsUtil::PathMatObsidian,
		AetheriaActorsUtil::PathMatDarkIron,
		FLinearColor(0.055f, 0.045f, 0.052f, 1.0f),
		0.38f,
		0.42f,
		0.38f,
		FLinearColor(0.28f, 0.08f, 0.02f, 1.0f),
		0.0f);

	if (UMaterialInterface* FireballMat = AetheriaActorsUtil::SafeLoadObjectIfPresent<UMaterialInterface>(AetheriaActorsUtil::PathMatFireball))
	{
		EyeGlowMID = UMaterialInstanceDynamic::Create(FireballMat, this);
		if (EyeGlowMID)
		{
			EyeGlowMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.36f, 0.04f, 1.0f));
			EyeGlowMID->SetVectorParameterValue(TEXT("AccentColor"), FLinearColor(1.0f, 0.72f, 0.14f, 1.0f));
			EyeGlowMID->SetScalarParameterValue(TEXT("EmissiveBoost"), 2.8f);
		}
	}
	else if (UMaterialInterface* EmissiveCoreMat = AetheriaActorsUtil::SafeLoadObjectIfPresent<UMaterialInterface>(AetheriaActorsUtil::PathMatEmissiveCore))
	{
		EyeGlowMID = UMaterialInstanceDynamic::Create(EmissiveCoreMat, this);
		if (EyeGlowMID)
		{
			EyeGlowMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(1.0f, 0.52f, 0.08f, 1.0f));
			EyeGlowMID->SetScalarParameterValue(TEXT("EmissiveBoost"), 3.2f);
			EyeGlowMID->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
		}
	}
	else
	{
		EyeGlowMID = AetheriaActorsUtil::CreateColorMID(this, FLinearColor(3.8f, 1.9f, 0.25f, 1.0f), true);
	}

	// --- Torso, Armored Chest Keel, Ember Furnace Core & Muscular Haunches ---
	AddProceduralPart(TEXT("DragonTorso"), SphereMeshAsset, ProceduralDragonRoot,
		FVector(0.f, 0.f, 15.f), FRotator(0.f, 0.f, 0.f), FVector(3.2f, 1.35f, 1.25f), CrimsonScaleMID);
	AddProceduralPart(TEXT("DragonChest"), SphereMeshAsset, ProceduralDragonRoot,
		FVector(95.f, 0.f, 24.f), FRotator(10.f, 0.f, 0.f), FVector(1.85f, 1.45f, 1.38f), CrimsonScaleMID);
	AddProceduralPart(TEXT("DragonChestFurnace"), SphereMeshAsset, ProceduralDragonRoot,
		FVector(128.f, 0.f, -2.f), FRotator(12.f, 0.f, 0.f), FVector(0.58f, 0.52f, 0.44f), EyeGlowMID);
	AddProceduralPart(TEXT("DragonBellyKeel"), SphereMeshAsset, ProceduralDragonRoot,
		FVector(40.f, 0.f, -26.f), FRotator(0.f, 0.f, 0.f), FVector(2.55f, 0.98f, 0.70f), CrimsonBellyMID);
	AddProceduralPart(TEXT("DragonHaunchL"), SphereMeshAsset, ProceduralDragonRoot,
		FVector(-95.f, -58.f, -15.f), FRotator(-18.f, 0.f, 0.f), FVector(1.15f, 0.68f, 1.05f), CrimsonScaleMID);
	AddProceduralPart(TEXT("DragonHaunchR"), SphereMeshAsset, ProceduralDragonRoot,
		FVector(-95.f, 58.f, -15.f), FRotator(-18.f, 0.f, 0.f), FVector(1.15f, 0.68f, 1.05f), CrimsonScaleMID);

	// Dorsal spine crest ridge along the back
	for (int32 i = 0; i < 6; ++i)
	{
		const float X = -100.f + i * 44.f;
		const float H = (i == 2 || i == 3) ? 0.58f : 0.44f;
		AddProceduralPart(FString::Printf(TEXT("DragonSpine_%d"), i), ConeMeshAsset, ProceduralDragonRoot,
			FVector(X, 0.f, 84.f), FRotator(-24.f, 0.f, 0.f), FVector(0.24f, 0.16f, H), HornObsidianMID);
	}

	// Ornate leather & bronze war-saddle at SaddleSocket
	AddProceduralPart(TEXT("DragonSaddleBase"), CubeMeshAsset, SaddleSocket,
		FVector(0.f, 0.f, -18.f), FRotator::ZeroRotator, FVector(0.95f, 0.85f, 0.24f), HornObsidianMID);
	AddProceduralPart(TEXT("DragonSaddlePommel"), ConeMeshAsset, SaddleSocket,
		FVector(42.f, 0.f, 2.f), FRotator(14.f, 0.f, 0.f), FVector(0.28f, 0.45f, 0.42f), CrimsonBellyMID);
	AddProceduralPart(TEXT("DragonSaddleCantle"), CubeMeshAsset, SaddleSocket,
		FVector(-44.f, 0.f, 4.f), FRotator(-18.f, 0.f, 0.f), FVector(0.22f, 0.78f, 0.46f), CrimsonBellyMID);

	// --- Articulated Neck, Sculpted Head, Horns & Hinged Jaw ---
	AddProceduralPart(TEXT("DragonNeckSeg1"), CylinderMeshAsset, NeckPivot,
		FVector(36.f, 0.f, 16.f), FRotator(-64.f, 0.f, 0.f), FVector(0.76f, 0.72f, 1.10f), CrimsonScaleMID);
	AddProceduralPart(TEXT("DragonNeckSeg2"), CylinderMeshAsset, NeckPivot,
		FVector(88.f, 0.f, 38.f), FRotator(-68.f, 0.f, 0.f), FVector(0.64f, 0.60f, 1.02f), CrimsonScaleMID);
	AddProceduralPart(TEXT("DragonNeckSpine1"), ConeMeshAsset, NeckPivot,
		FVector(55.f, 0.f, 56.f), FRotator(-32.f, 0.f, 0.f), FVector(0.22f, 0.14f, 0.48f), HornObsidianMID);

	// Head & Upper Snout
	AddProceduralPart(TEXT("DragonCranium"), SphereMeshAsset, HeadPivot,
		FVector(16.f, 0.f, 6.f), FRotator::ZeroRotator, FVector(1.05f, 0.78f, 0.72f), CrimsonScaleMID);
	AddProceduralPart(TEXT("DragonUpperSnout"), CubeMeshAsset, HeadPivot,
		FVector(78.f, 0.f, 2.f), FRotator(-4.f, 0.f, 0.f), FVector(0.95f, 0.56f, 0.34f), CrimsonScaleMID);
	AddProceduralPart(TEXT("DragonNoseHorn"), ConeMeshAsset, HeadPivot,
		FVector(114.f, 0.f, 26.f), FRotator(-22.f, 0.f, 0.f), FVector(0.16f, 0.14f, 0.38f), HornObsidianMID);

	// Swept Dorsal Cranial Horns (Left & Right)
	AddProceduralPart(TEXT("DragonHornL"), ConeMeshAsset, HeadPivot,
		FVector(-42.f, -34.f, 44.f), FRotator(-58.f, -20.f, -14.f), FVector(0.22f, 0.22f, 1.25f), HornObsidianMID);
	AddProceduralPart(TEXT("DragonHornR"), ConeMeshAsset, HeadPivot,
		FVector(-42.f, 34.f, 44.f), FRotator(-58.f, 20.f, 14.f), FVector(0.22f, 0.22f, 1.25f), HornObsidianMID);

	// Molten Emissive Dragon Eyes
	AddProceduralPart(TEXT("DragonEyeL"), SphereMeshAsset, HeadPivot,
		FVector(46.f, -30.f, 18.f), FRotator::ZeroRotator, FVector(0.20f, 0.14f, 0.16f), EyeGlowMID);
	AddProceduralPart(TEXT("DragonEyeR"), SphereMeshAsset, HeadPivot,
		FVector(46.f, 30.f, 18.f), FRotator::ZeroRotator, FVector(0.20f, 0.14f, 0.16f), EyeGlowMID);

	// Hinged Lower Jaw
	AddProceduralPart(TEXT("DragonLowerJaw"), CubeMeshAsset, LowerJawPivot,
		FVector(48.f, 0.f, -6.f), FRotator(4.f, 0.f, 0.f), FVector(0.86f, 0.48f, 0.22f), CrimsonBellyMID);

	// --- Flapping Left & Right Articulated Wings (~600 cm wingspan) ---
	auto BuildWing = [this](USceneComponent* WingPivot, float SideSign, const TCHAR* Prefix)
	{
		// Inner leading wing bone
		AddProceduralPart(FString::Printf(TEXT("%sArmInner"), Prefix), CylinderMeshAsset, WingPivot,
			FVector(16.f, SideSign * 98.f, 28.f), FRotator(0.f, 0.f, SideSign * 78.f), FVector(0.26f, 0.26f, 2.05f), CrimsonScaleMID);
		// Outer leading wing spar
		AddProceduralPart(FString::Printf(TEXT("%sArmOuter"), Prefix), CylinderMeshAsset, WingPivot,
			FVector(-12.f, SideSign * 265.f, 42.f), FRotator(0.f, SideSign * -12.f, SideSign * 85.f), FVector(0.20f, 0.20f, 2.55f), CrimsonScaleMID);
		// Obsidian wing-thumb claw
		AddProceduralPart(FString::Printf(TEXT("%sClaw"), Prefix), ConeMeshAsset, WingPivot,
			FVector(44.f, SideSign * 185.f, 52.f), FRotator(58.f, 0.f, SideSign * 12.f), FVector(0.18f, 0.18f, 0.52f), HornObsidianMID);
		// Crimson-gold stretched wing membrane inner & outer panels
		AddProceduralPart(FString::Printf(TEXT("%sMembraneInner"), Prefix), CubeMeshAsset, WingPivot,
			FVector(-48.f, SideSign * 112.f, 16.f), FRotator(4.f, SideSign * -6.f, SideSign * 8.f), FVector(1.65f, 1.85f, 0.06f), CrimsonBellyMID);
		AddProceduralPart(FString::Printf(TEXT("%sMembraneOuter"), Prefix), CubeMeshAsset, WingPivot,
			FVector(-72.f, SideSign * 272.f, 26.f), FRotator(6.f, SideSign * -18.f, SideSign * 4.f), FVector(1.45f, 2.15f, 0.05f), CrimsonBellyMID);
	};

	BuildWing(LeftWingPivot, -1.f, TEXT("LeftWing"));
	BuildWing(RightWingPivot, 1.f, TEXT("RightWing"));

	// --- 5-Segment Articulated Waving Tail ---
	TailSegmentPivots.Reset();
	USceneComponent* PrevTailParent = ProceduralDragonRoot;
	for (int32 Seg = 0; Seg < 5; ++Seg)
	{
		const FName PivotName = MakeUniqueObjectName(this, USceneComponent::StaticClass(), FName(*FString::Printf(TEXT("TailPivot_%d"), Seg)));
		USceneComponent* SegPivot = NewObject<USceneComponent>(this, PivotName);
		SegPivot->AttachToComponent(PrevTailParent, FAttachmentTransformRules::KeepRelativeTransform);
		SegPivot->SetRelativeLocation(Seg == 0 ? FVector(-145.f, 0.f, 8.f) : FVector(-82.f, 0.f, -2.f));
		SegPivot->RegisterComponent();
		TailSegmentPivots.Add(SegPivot);

		const float Taper = FMath::Lerp(0.76f, 0.24f, static_cast<float>(Seg) / 4.f);
		AddProceduralPart(FString::Printf(TEXT("TailSegMesh_%d"), Seg), CylinderMeshAsset, SegPivot,
			FVector(-42.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(Taper, Taper, 0.92f), CrimsonScaleMID);
		AddProceduralPart(FString::Printf(TEXT("TailSpine_%d"), Seg), ConeMeshAsset, SegPivot,
			FVector(-40.f, 0.f, 34.f * Taper), FRotator(-28.f, 0.f, 0.f), FVector(0.18f * Taper, 0.12f * Taper, 0.42f * Taper), HornObsidianMID);

		if (Seg == 4)
		{
			// Obsidian diamond tail-spade blade
			AddProceduralPart(TEXT("TailSpadeBlade"), ConeMeshAsset, SegPivot,
				FVector(-108.f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f), FVector(0.14f, 0.62f, 0.95f), HornObsidianMID);
		}

		PrevTailParent = SegPivot;
	}
}

void AAetheriaDragonActor::MountRider(AAetheriaHeroCharacter* Hero, APlayerController* PC)
{
	if (!Hero || Hero->bDead)
	{
		return;
	}

	RiderHero = Hero;
	bMounted = true;
	bBreathingFire = false;
	FlightSpeed = 1950.f;
	CurrentPitchDeg = 14.f;
	CurrentBankRoll = 0.f;
	TakeoffBoostTimer = 0.85f;
	LandingAnimTimer = 0.f;

	// Cleanly suspend Hero movement & capsule collision while riding dragon saddle
	if (UCharacterMovementComponent* HeroMove = Hero->GetCharacterMovement())
	{
		HeroMove->StopMovementImmediately();
		HeroMove->SetMovementMode(MOVE_None);
	}
	if (UCapsuleComponent* HeroCapsule = Hero->GetCapsuleComponent())
	{
		HeroCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (SaddleSocket)
	{
		Hero->AttachToComponent(SaddleSocket, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Hero->SetActorRelativeLocation(FVector(0.f, 0.f, 74.f));
		Hero->SetActorRelativeRotation(FRotator::ZeroRotator);
	}

	// Lift cleanly off perch on mount so takeoff feels heroic and immediate
	SetActorLocation(GetActorLocation() + FVector(0.f, 0.f, 240.f), false, nullptr, ETeleportType::TeleportPhysics);

	if (PC)
	{
		PC->Possess(this);
		PC->SetControlRotation(GetActorRotation());
	}

	UE_LOG(LogChronicles, Log, TEXT("Hero mounted Crimson Dragon Ignis and launched into the skies!"));
}

void AAetheriaDragonActor::DismountRider(APlayerController* PC)
{
	bBreathingFire = false;
	TakeoffBoostTimer = 0.f;
	LandingAnimTimer = 1.15f;

	AAetheriaHeroCharacter* Hero = RiderHero.Get();

	TArray<const AActor*> Ignored;
	Ignored.Add(this);
	if (Hero)
	{
		Ignored.Add(Hero);
	}

	// Trace safe ground directly beneath dragon via ECC_WorldStatic
	const FVector CurrentLoc = GetActorLocation();
	float GroundZ = CurrentLoc.Z - 185.f;
	AetheriaActorsUtil::TraceGroundZ(GetWorld(), CurrentLoc.X, CurrentLoc.Y, CurrentLoc.Z + 350.f, CurrentLoc.Z - 24000.f, Ignored, GroundZ);

	const FRotator LevelRot(0.f, GetActorRotation().Yaw, 0.f);
	const FVector LandedDragonLoc(CurrentLoc.X, CurrentLoc.Y, GroundZ + 185.f);
	SetActorLocationAndRotation(LandedDragonLoc, LevelRot, false, nullptr, ETeleportType::TeleportPhysics);

	CurrentBankRoll = 0.f;
	CurrentPitchDeg = 0.f;
	FlightSpeed = 1600.f;
	bMounted = false;
	RiderHero.Reset();

	if (Hero)
	{
		Hero->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		const FVector SideOffset = GetActorRightVector() * 320.f;
		const FVector CandidateSide = LandedDragonLoc + SideOffset;
		float SideGroundZ = GroundZ;
		AetheriaActorsUtil::TraceGroundZ(GetWorld(), CandidateSide.X, CandidateSide.Y, CandidateSide.Z + 500.f, CandidateSide.Z - 4000.f, Ignored, SideGroundZ);

		const float HalfH = Hero->GetCapsuleComponent() ? Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
		Hero->SetActorLocationAndRotation(
			FVector(CandidateSide.X, CandidateSide.Y, SideGroundZ + HalfH + 14.f),
			LevelRot,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);

		if (UCapsuleComponent* HeroCapsule = Hero->GetCapsuleComponent())
		{
			HeroCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		if (UCharacterMovementComponent* HeroMove = Hero->GetCharacterMovement())
		{
			HeroMove->SetMovementMode(MOVE_Walking);
		}

		if (PC)
		{
			PC->Possess(Hero);
			PC->SetControlRotation(LevelRot);
		}
	}

	UE_LOG(LogChronicles, Log, TEXT("Hero safely dismounted Crimson Dragon at Z=%.1f."), GroundZ);
}

void AAetheriaDragonActor::ApplyFlightInput(
	float ForwardAxis,
	float RightAxis,
	float UpAxis,
	float YawInput,
	float PitchInput,
	bool bFireHeld,
	float DeltaTime)
{
	if ((!bMounted && !GetController()) || DeltaTime <= 0.f)
	{
		return;
	}

	CachedUpAxis = UpAxis;
	CachedForwardAxis = ForwardAxis;

	// Pull live RPG Skill Tree + Hero Customization multipliers from rider if mounted
	float DragonSpeedMult = 1.f;
	float DragonFireMult = 1.f;
	float DragonFireDrainMult = 1.f;
	if (AAetheriaHeroCharacter* Hero = RiderHero.Get())
	{
		if (Hero->SkillTree)
		{
			DragonSpeedMult *= Hero->SkillTree->GetDragonSpeedMultiplier();
			DragonFireMult *= Hero->SkillTree->GetDragonFireMultiplier();
		}
		if (Hero->Customization)
		{
			const FHeroDerivedStats& RiderStats = Hero->Customization->GetDerivedStats();
			DragonSpeedMult *= RiderStats.DragonSpeedMultiplier;
			DragonFireMult *= RiderStats.DragonFireMultiplier;
			DragonFireDrainMult *= RiderStats.DragonFireDrainMultiplier;
		}
	}

	// Smooth turning, pitch clamping, and dramatic banking roll into turns
	const float TurnRateDegPerSec = (RightAxis * 82.f) + (YawInput * 115.f);
	const float NewYaw = FRotator::NormalizeAxis(GetActorRotation().Yaw + TurnRateDegPerSec * DeltaTime);

	const float PitchDelta = (PitchInput * 70.f) + (UpAxis * 22.f);
	CurrentPitchDeg = FMath::Clamp(FMath::FInterpTo(CurrentPitchDeg, CurrentPitchDeg + PitchDelta * DeltaTime, DeltaTime, 7.5f), -48.f, 52.f);
	if (FMath::Abs(PitchInput) < 0.05f && FMath::Abs(UpAxis) < 0.05f)
	{
		CurrentPitchDeg = FMath::FInterpTo(CurrentPitchDeg, 0.f, DeltaTime, 1.4f);
	}

	const float TargetBankRoll = FMath::Clamp((RightAxis * 34.f) + (YawInput * 28.f), -42.f, 42.f);
	CurrentBankRoll = FMath::FInterpTo(CurrentBankRoll, TargetBankRoll, DeltaTime, 5.2f);

	SetActorRotation(FRotator(CurrentPitchDeg, NewYaw, CurrentBankRoll));

	// Throttle: W boosts up to 3800 cm/s, S slows to 1100 cm/s cruise, Shift dive accelerates
	float TargetSpeed = 1650.f * DragonSpeedMult;
	if (ForwardAxis > 0.1f)
	{
		TargetSpeed = FMath::Lerp(1650.f, 3800.f, FMath::Clamp(ForwardAxis, 0.f, 1.f)) * DragonSpeedMult;
	}
	else if (ForwardAxis < -0.1f)
	{
		TargetSpeed = FMath::Lerp(1650.f, 1100.f, FMath::Clamp(-ForwardAxis, 0.f, 1.f));
	}
	if (UpAxis < -0.1f)
	{
		// Diving converts altitude into exhilarating forward surge
		TargetSpeed += 650.f * FMath::Clamp(-UpAxis, 0.f, 1.f) * DragonSpeedMult;
	}

	FlightSpeed = FMath::FInterpTo(FlightSpeed, TargetSpeed, DeltaTime, 3.2f);

	// Compose 3D flight velocity (Forward + Space climb / Shift dive + Takeoff surge)
	FVector Velocity = GetActorForwardVector() * FlightSpeed;
	Velocity.Z += UpAxis * 1150.f * DragonSpeedMult;
	if (TakeoffBoostTimer > 0.f)
	{
		Velocity.Z += 720.f * (TakeoffBoostTimer / 0.85f);
	}

	FVector NextLocation = GetActorLocation() + Velocity * DeltaTime;

	// Terrain clearance safety trace (ECC_WorldStatic) so dragon glides cleanly over terrain
	TArray<const AActor*> Ignored;
	Ignored.Add(this);
	if (AAetheriaHeroCharacter* Hero = RiderHero.Get())
	{
		Ignored.Add(Hero);
	}

	float GroundZ = -100000.f;
	if (AetheriaActorsUtil::TraceGroundZ(GetWorld(), NextLocation.X, NextLocation.Y, NextLocation.Z + 60.f, NextLocation.Z - 4000.f, Ignored, GroundZ))
	{
		const float MinAltitude = GroundZ + 175.f;
		if (NextLocation.Z < MinAltitude)
		{
			NextLocation.Z = MinAltitude;
			CurrentPitchDeg = FMath::Max(CurrentPitchDeg, 2.f);
		}
	}

	// Keep inside the 4km x 4km seamless world bounds gracefully (clamped below sky ceiling)
	const float MaxWorldCoord = UFogOfWarMapSubsystem::WORLD_EXTENT_UU - 2500.f;
	constexpr float MaxAltitude = 8500.f;
	NextLocation.X = FMath::Clamp(NextLocation.X, -MaxWorldCoord, MaxWorldCoord);
	NextLocation.Y = FMath::Clamp(NextLocation.Y, -MaxWorldCoord, MaxWorldCoord);
	NextLocation.Z = FMath::Clamp(NextLocation.Z, -2000.f, MaxAltitude);

	SetActorLocation(NextLocation, false);

	// Dynamic wingbeat cadence: climb & boost flap vigorously, dive glides gracefully
	if (UpAxis < -0.25f)
	{
		WingBeatSpeed = FMath::FInterpTo(WingBeatSpeed, 1.4f, DeltaTime, 4.0f);
	}
	else if (UpAxis > 0.25f || ForwardAxis > 0.5f)
	{
		WingBeatSpeed = FMath::FInterpTo(WingBeatSpeed, 6.2f, DeltaTime, 5.0f);
	}
	else
	{
		WingBeatSpeed = FMath::FInterpTo(WingBeatSpeed, 3.8f, DeltaTime, 3.5f);
	}

	// Dragonfire Breath Cone (CHRONICLE_TRACE_PROJECTILE sweep, 65 * DragonFireMult DPS)
	bBreathingFire = bFireHeld && (FireEnergy > 2.f);
	if (bBreathingFire)
	{
		FireEnergy = FMath::Max(0.f, FireEnergy - 16.5f * DragonFireDrainMult * DeltaTime);
		FireDamageTickAccum += DeltaTime;

		constexpr float TickStep = 0.10f;
		if (FireDamageTickAccum >= TickStep)
		{
			const float SliceDuration = FireDamageTickAccum;
			FireDamageTickAccum = 0.f;
			ApplyDragonFireSweep(65.f * DragonFireMult * SliceDuration, DragonFireMult);
		}
	}
	else
	{
		FireDamageTickAccum = 0.f;
		FireEnergy = FMath::Min(MaxFireEnergy, FireEnergy + 18.f * DragonSpeedMult * DeltaTime);
	}

	// Aerial Fog-of-War revelation (11,000 UU radius while soaring!)
	MapRevealCooldown -= DeltaTime;
	if (MapRevealCooldown <= 0.f)
	{
		MapRevealCooldown = 0.20f;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFogOfWarMapSubsystem* MapSys = GI->GetSubsystem<UFogOfWarMapSubsystem>())
			{
				MapSys->RevealAroundWorldLocation(GetActorLocation(), 11000.f);
			}
		}
	}
}

void AAetheriaDragonActor::ApplyDragonFireSweep(float DeltaDamage, float FireMult)
{
	UWorld* World = GetWorld();
	if (!World || DeltaDamage <= 0.f)
	{
		return;
	}

	const FVector FireOrigin = MouthSocket ? MouthSocket->GetComponentLocation() : (GetActorLocation() + GetActorForwardVector() * 260.f);
	const FVector FireDir = GetActorForwardVector().GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const float Range = 2200.f * FMath::Lerp(1.f, FireMult, 0.45f);
	const FVector FireEnd = FireOrigin + FireDir * Range;

	FCollisionQueryParams Params(TEXT("DragonFireBreathSweep"), false, this);
	Params.AddIgnoredActor(this);
	if (AAetheriaHeroCharacter* Hero = RiderHero.Get())
	{
		Params.AddIgnoredActor(Hero);
	}

	TArray<FHitResult> Hits;
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(220.f, 260.f);
	const FQuat SweepQuat = AetheriaActorsUtil::MakeCapsuleSweepQuat(FireDir);

	World->SweepMultiByChannel(Hits, FireOrigin, FireEnd, SweepQuat, CHRONICLE_TRACE_PROJECTILE, CapsuleShape, Params);

	TSet<AActor*> DamagedSet;
	AController* InstigatorCtrl = GetController();
	AActor* HeroInstigator = RiderHero.Get() ? static_cast<AActor*>(RiderHero.Get()) : static_cast<AActor*>(this);

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == this || HitActor == RiderHero.Get() || DamagedSet.Contains(HitActor))
		{
			continue;
		}

		DamagedSet.Add(HitActor);

		if (ADestructiblePropActor* Destructible = Cast<ADestructiblePropActor>(HitActor))
		{
			// Dragonfire rapidly shatters destructible barriers and arena columns
			UGameplayStatics::ApplyPointDamage(Destructible, DeltaDamage * 3.2f, FireDir, Hit, InstigatorCtrl, this, UDamageType::StaticClass());
		}
		else if (AAetheriaEnemyCharacter* Enemy = Cast<AAetheriaEnemyCharacter>(HitActor))
		{
			UGameplayStatics::ApplyPointDamage(Enemy, DeltaDamage, FireDir, Hit, InstigatorCtrl, this, UDamageType::StaticClass());
			Enemy->ApplyElementalStatus(EChronicleOrb::Exort, HeroInstigator);
		}
		else
		{
			UGameplayStatics::ApplyPointDamage(HitActor, DeltaDamage, FireDir, Hit, InstigatorCtrl, this, UDamageType::StaticClass());
		}
	}
}

void AAetheriaDragonActor::UpdateDragonAnimation(float DeltaTime)
{
	const float ActiveBeatRate = bMounted ? WingBeatSpeed : 1.15f;
	WingBeatPhase += DeltaTime * ActiveBeatRate;
	TailWavePhase += DeltaTime * (bMounted ? 3.8f : 1.7f);

	// High-poly SkeletalMesh animation state machine (DragonTheUsurperSK)
	if (bUsingImportedSkeletalMesh && ImportedDragonMesh)
	{
		UAnimSequence* DesiredAnim = nullptr;
		bool bShouldLoop = true;

		if (!bMounted)
		{
			if (LandingAnimTimer > 0.f && DragonLandAnim)
			{
				DesiredAnim = DragonLandAnim.Get();
				bShouldLoop = false;
			}
			else
			{
				DesiredAnim = DragonIdleAnim.Get();
			}
		}
		else if (bBreathingFire)
		{
			DesiredAnim = DragonFlameAnim ? DragonFlameAnim.Get() : DragonFlyAnim.Get();
		}
		else if (TakeoffBoostTimer > 0.f)
		{
			DesiredAnim = DragonTakeOffAnim ? DragonTakeOffAnim.Get() : DragonFlyAnim.Get();
		}
		else if (CachedUpAxis < -0.15f)
		{
			DesiredAnim = DragonGlideAnim ? DragonGlideAnim.Get() : DragonFlyAnim.Get();
		}
		else
		{
			DesiredAnim = DragonFlyAnim.Get();
		}

		if (DesiredAnim && DesiredAnim != ActiveDragonAnim.Get())
		{
			USkeletalMesh* SkelMesh = ImportedDragonMesh->GetSkeletalMeshAsset();
			if (SkelMesh && AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, DesiredAnim->GetSkeleton()))
			{
				ImportedDragonMesh->PlayAnimation(DesiredAnim, bShouldLoop);
				ActiveDragonAnim = DesiredAnim;
			}
		}

		// Track live snout & back bone positions while preserving actor-forward alignment so
		// FireBreathConeMesh shoots straight ahead and the rider always sits upright!
		const FRotator ActorRot = GetActorRotation();
		if (MouthSocket && !DragonMouthBoneName.IsNone())
		{
			const FVector SnoutLoc = ImportedDragonMesh->GetSocketLocation(DragonMouthBoneName);
			MouthSocket->SetWorldLocationAndRotation(SnoutLoc + GetActorForwardVector() * 40.f, ActorRot);
		}
		if (SaddleSocket && !DragonSaddleBoneName.IsNone())
		{
			const FVector BackLoc = ImportedDragonMesh->GetSocketLocation(DragonSaddleBoneName);
			SaddleSocket->SetWorldLocationAndRotation(BackLoc + GetActorUpVector() * 48.f, ActorRot);
		}
	}
	else if (!bUsingImportedSkeletalMesh)
	{
		const bool bDivingGlide = bMounted && (CachedUpAxis < -0.25f);
		const float WingAmp = bMounted ? (bDivingGlide ? 11.f : 42.f) : 7.f;
		const float WingBias = bMounted ? (bDivingGlide ? 16.f : 6.f) : -24.f;
		const float FlapAngle = FMath::Sin(WingBeatPhase) * WingAmp + WingBias;

		if (LeftWingPivot)
		{
			LeftWingPivot->SetRelativeRotation(FRotator(bDivingGlide ? -14.f : 0.f, bDivingGlide ? -18.f : 0.f, FlapAngle));
		}
		if (RightWingPivot)
		{
			RightWingPivot->SetRelativeRotation(FRotator(bDivingGlide ? -14.f : 0.f, bDivingGlide ? 18.f : 0.f, -FlapAngle));
		}

		// Neck & head poise + jaw opening wide when breathing blazing dragonfire
		if (NeckPivot)
		{
			const float NeckBreath = FMath::Sin(TailWavePhase * 0.85f) * 3.5f;
			NeckPivot->SetRelativeRotation(FRotator(NeckBreath + (bBreathingFire ? 6.f : 0.f), -CurrentBankRoll * 0.25f, 0.f));
		}
		if (LowerJawPivot)
		{
			const float TargetJawPitch = bBreathingFire ? -34.f : -3.f;
			const FRotator CurJaw = LowerJawPivot->GetRelativeRotation();
			LowerJawPivot->SetRelativeRotation(FRotator(FMath::FInterpTo(CurJaw.Pitch, TargetJawPitch, DeltaTime, 12.f), 0.f, 0.f));
		}

		// 5-segment serpentine waving tail
		for (int32 Seg = 0; Seg < TailSegmentPivots.Num(); ++Seg)
		{
			if (USceneComponent* SegPivot = TailSegmentPivots[Seg])
			{
				const float Phase = TailWavePhase - static_cast<float>(Seg) * 0.72f;
				const float YawWave = FMath::Sin(Phase) * (9.f + Seg * 3.2f) - (CurrentBankRoll * 0.18f);
				const float PitchWave = FMath::Cos(Phase * 0.7f) * (3.5f + Seg * 1.1f);
				SegPivot->SetRelativeRotation(FRotator(PitchWave, YawWave, 0.f));
			}
		}
	}

	// Real Dragon Fire Particles (or fallback BasicShapes cone meshes) & physical AgX Lumen mouth light (up to 24,000 lm!)
	if (bHasRealDragonFireFX)
	{
		if (FireBreathConeMesh) { FireBreathConeMesh->SetVisibility(false); }
		if (FireBreathCoreMesh) { FireBreathCoreMesh->SetVisibility(false); }

		if (bBreathingFire != bWasBreathingFire)
		{
			if (bBreathingFire)
			{
				if (DragonFireStormPSC) { DragonFireStormPSC->ActivateSystem(true); }
				if (DragonFireBallPSC)  { DragonFireBallPSC->ActivateSystem(true); }
				if (DragonFireSFX && MouthSocket)
				{
					UGameplayStatics::PlaySoundAtLocation(this, DragonFireSFX, MouthSocket->GetComponentLocation());
				}
			}
			else
			{
				if (DragonFireStormPSC) { DragonFireStormPSC->DeactivateSystem(); }
				if (DragonFireBallPSC)  { DragonFireBallPSC->DeactivateSystem(); }
			}
			bWasBreathingFire = bBreathingFire;
		}
	}
	else
	{
		if (bBreathingFire && !bWasBreathingFire && DragonFireSFX && MouthSocket)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DragonFireSFX, MouthSocket->GetComponentLocation());
		}
		bWasBreathingFire = bBreathingFire;

		if (FireBreathConeMesh)
		{
			FireBreathConeMesh->SetVisibility(bBreathingFire);
			if (bBreathingFire)
			{
				const float Pulse = 1.0f + 0.16f * FMath::Sin(WingBeatPhase * 9.5f);
				FireBreathConeMesh->SetRelativeScale3D(FVector(2.2f * Pulse, 2.2f * Pulse, 12.5f * FMath::Lerp(0.92f, 1.08f, Pulse - 0.84f)));
				FireBreathConeMesh->AddRelativeRotation(FRotator(0.f, 0.f, DeltaTime * 360.f));
			}
		}
		if (FireBreathCoreMesh)
		{
			FireBreathCoreMesh->SetVisibility(bBreathingFire);
			if (bBreathingFire)
			{
				const float CorePulse = 1.0f + 0.22f * FMath::Cos(WingBeatPhase * 12.0f);
				FireBreathCoreMesh->SetRelativeScale3D(FVector(1.05f * CorePulse, 1.05f * CorePulse, 9.2f));
			}
		}
	}

	if (MouthFireLight)
	{
		if (bBreathingFire)
		{
			const float Flicker = 2850.f + 350.f * FMath::Sin(WingBeatPhase * 14.f);
			MouthFireLight->SetIntensity(Flicker);
			MouthFireLight->SetAttenuationRadius(2400.f);
		}
		else
		{
			// Subtle draconic ember glow when mounted/perched (450 lm physical AgX)
			const float AmbientEmber = bMounted ? 520.f : 450.f;
			MouthFireLight->SetIntensity(FMath::FInterpTo(MouthFireLight->Intensity, AmbientEmber, DeltaTime, 6.f));
			MouthFireLight->SetAttenuationRadius(1200.f);
		}
	}
}

void AAetheriaDragonActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TakeoffBoostTimer > 0.f)
	{
		TakeoffBoostTimer = FMath::Max(0.f, TakeoffBoostTimer - DeltaTime);
	}
	if (LandingAnimTimer > 0.f)
	{
		LandingAnimTimer = FMath::Max(0.f, LandingAnimTimer - DeltaTime);
	}

	if (!bMounted)
	{
		bBreathingFire = false;
		FireEnergy = FMath::Min(MaxFireEnergy, FireEnergy + 14.f * DeltaTime);
	}

	UpdateDragonAnimation(DeltaTime);
}

// ============================================================================
// 2. AAetheriaHeroCharacter — Wukong-Grade High-Bone Hero
// ============================================================================

AAetheriaHeroCharacter::AAetheriaHeroCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	// ONE rotation owner per mind-ue5-bug-hunter §5C: CharacterMovement owns actor yaw
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 560.f, 0.f);
		MoveComp->MaxWalkSpeed = BaseWalkSpeed;
		MoveComp->JumpZVelocity = 560.f;
		MoveComp->AirControl = 0.35f;
		MoveComp->BrakingDecelerationWalking = 2100.f;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(42.f, 92.f);
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -92.f));
		MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	// Over-the-shoulder Crimson Desert / Wukong camera framing (never clips inside cape or world shrines!)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 470.f;
	CameraBoom->SocketOffset = FVector(0.f, 48.f, 96.f);
	CameraBoom->ProbeSize = 6.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	CameraBoom->bEnableCameraRotationLag = false;
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 78.0f;

	// Core RPG & Combat Components
	SpellSystem = CreateDefaultSubobject<USpellSystemComponent>(TEXT("SpellSystem"));
	SkillTree = CreateDefaultSubobject<USkillTreeComponent>(TEXT("SkillTree"));
	Customization = CreateDefaultSubobject<UHeroCustomizationComponent>(TEXT("HeroCustomization"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	// Transient combat slash / parry / hit physical AgX Lumen light
	CombatFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CombatFlashLight"));
	CombatFlashLight->SetupAttachment(RootComponent);
	CombatFlashLight->SetRelativeLocation(FVector(65.f, 0.f, 38.f));
	CombatFlashLight->SetIntensityUnits(ELightUnits::Lumens);
	CombatFlashLight->SetIntensity(0.f);
	CombatFlashLight->SetAttenuationRadius(1100.f);
	CombatFlashLight->SetCastShadows(false);

	// Creator Studio warm 3/4 portrait key light (0 lm & hidden until Creator Studio camera mode activates)
	CreatorStudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CreatorStudioFillLight"));
	CreatorStudioFillLight->SetupAttachment(RootComponent);
	CreatorStudioFillLight->SetRelativeLocation(FVector(120.f, 110.f, 115.f));
	CreatorStudioFillLight->SetIntensityUnits(ELightUnits::Lumens);
	CreatorStudioFillLight->SetIntensity(0.f);
	CreatorStudioFillLight->SetLightColor(FLinearColor(1.0f, 0.90f, 0.78f, 1.0f));
	CreatorStudioFillLight->SetAttenuationRadius(480.f);
	CreatorStudioFillLight->SetSpecularScale(0.22f);
	CreatorStudioFillLight->SetSourceRadius(28.f);
	CreatorStudioFillLight->SetCastShadows(false);
	CreatorStudioFillLight->SetVisibility(false);

	// Creator Studio cool silhouette rim light placed behind-opposite camera (0 lm & hidden outside studio)
	CreatorStudioRimLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CreatorStudioRimLight"));
	CreatorStudioRimLight->SetupAttachment(RootComponent);
	CreatorStudioRimLight->SetRelativeLocation(FVector(-120.f, -85.f, 105.f));
	CreatorStudioRimLight->SetIntensityUnits(ELightUnits::Lumens);
	CreatorStudioRimLight->SetIntensity(0.f);
	CreatorStudioRimLight->SetLightColor(FLinearColor(0.58f, 0.78f, 1.0f, 1.0f));
	CreatorStudioRimLight->SetAttenuationRadius(460.f);
	CreatorStudioRimLight->SetSpecularScale(0.20f);
	CreatorStudioRimLight->SetSourceRadius(24.f);
	CreatorStudioRimLight->SetCastShadows(false);
	CreatorStudioRimLight->SetVisibility(false);

	// 3 Orbiting Invoker Primal Spheres hovering behind shoulders (jewel-scale 0.105)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(AetheriaActorsUtil::PathSphere);
	UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;

	for (int32 i = 0; i < 3; ++i)
	{
		const FName OrbCompName(*FString::Printf(TEXT("OrbitingOrbMesh_%d"), i));
		OrbitingOrbMeshes[i] = CreateDefaultSubobject<UStaticMeshComponent>(OrbCompName);
		OrbitingOrbMeshes[i]->SetupAttachment(RootComponent);
		OrbitingOrbMeshes[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OrbitingOrbMeshes[i]->SetGenerateOverlapEvents(false);
		OrbitingOrbMeshes[i]->SetCastShadow(false);
		OrbitingOrbMeshes[i]->SetRelativeScale3D(FVector(0.105f));
		OrbitingOrbMeshes[i]->bDisallowNanite = true;
		OrbitingOrbMeshes[i]->SetVisibility(false);
		if (SphereMesh)
		{
			OrbitingOrbMeshes[i]->SetStaticMesh(SphereMesh);
		}
	}

	// Optional arcane staff/blade mesh attached to hand socket in BeginPlay if present
	HeroWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeroWeaponMesh"));
	HeroWeaponMesh->SetupAttachment(RootComponent);
	HeroWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeroWeaponMesh->SetCastShadow(true);
	HeroWeaponMesh->SetVisibility(false);
}

void AAetheriaHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAetheriaHeroCharacter, HP);
	DOREPLIFETIME(AAetheriaHeroCharacter, MaxHP);
	DOREPLIFETIME(AAetheriaHeroCharacter, Stamina);
	DOREPLIFETIME(AAetheriaHeroCharacter, MaxStamina);
	DOREPLIFETIME(AAetheriaHeroCharacter, bDead);
	DOREPLIFETIME(AAetheriaHeroCharacter, bBlocking);
	DOREPLIFETIME(AAetheriaHeroCharacter, ComboIndex);
	DOREPLIFETIME(AAetheriaHeroCharacter, RollTimer);
	DOREPLIFETIME(AAetheriaHeroCharacter, InvulnTimer);
	DOREPLIFETIME(AAetheriaHeroCharacter, EnemiesSlain);
	DOREPLIFETIME(AAetheriaHeroCharacter, bDragonUnlocked);
}

void AAetheriaHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Only fall back to SetupHeroSkeletalMeshAndAnims() if Customization::BeginPlay() did not already bind a Paragon rig
	if (!GetMesh() || !GetMesh()->GetSkeletalMeshAsset())
	{
		SetupHeroSkeletalMeshAndAnims();
	}
	SetupOrbitingOrbs();

	if (Customization)
	{
		Customization->OnCustomizationChanged.AddDynamic(this, &AAetheriaHeroCharacter::HandleCustomizationChanged);
	}
	RefreshAllHeroStats();

	// Preload Free_Spells Niagara & Buff SFX for Orb Unlocks & Level-Up celebrations
	OrbUnlockAuraFireNS      = AetheriaActorsUtil::SafeLoadObjectIfPresent<UNiagaraSystem>(AetheriaActorsUtil::PathNSAuraFire);
	OrbUnlockAuraLightningNS = AetheriaActorsUtil::SafeLoadObjectIfPresent<UNiagaraSystem>(AetheriaActorsUtil::PathNSAuraLightning);
	OrbUnlockAuraSoulNS      = AetheriaActorsUtil::SafeLoadObjectIfPresent<UNiagaraSystem>(AetheriaActorsUtil::PathNSAuraSoul);
	HeroLevelUpNS            = AetheriaActorsUtil::SafeLoadObjectIfPresent<UNiagaraSystem>(AetheriaActorsUtil::PathNSLevelUp);
	MagicBuffSFX             = AetheriaActorsUtil::SafeLoadObjectIfPresent<USoundBase>(AetheriaActorsUtil::PathSFXMagicBuff);

	if (SpellSystem)
	{
		SpellSystem->OnOrbUnlocked.AddDynamic(this, &AAetheriaHeroCharacter::HandleOrbUnlocked);
	}

	// Hook Save/Load persistence cleanly with TWeakObjectPtr + SkillTree Level-Up FX
	if (SkillTree)
	{
		LastObservedHeroLevel = SkillTree->Level;
		SkillTree->OnSkillTreeChanged.AddDynamic(this, &AAetheriaHeroCharacter::HandleSkillTreeChanged);

		TWeakObjectPtr<AAetheriaHeroCharacter> WeakHero(this);
		SkillTree->OnPopulateSaveGame.AddLambda([WeakHero](UChroniclesSaveGame* SaveObj)
		{
			if (AAetheriaHeroCharacter* StrongHero = WeakHero.Get())
			{
				StrongHero->HandlePopulateSaveGame(SaveObj);
			}
		});
		SkillTree->OnApplySaveGame.AddLambda([WeakHero](UChroniclesSaveGame* SaveObj)
		{
			if (AAetheriaHeroCharacter* StrongHero = WeakHero.Get())
			{
				StrongHero->HandleApplySaveGame(SaveObj);
			}
		});
	}

	FAetheriaStyleHarmonizer::HarmonizeActorMaterials(this, EChronicleStyleCategory::Character);
}

bool AAetheriaHeroCharacter::BindParagonHeroSingleNodeAnims(USkeletalMesh* SkelMesh)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	ActiveIdleAnimSeq = nullptr;
	ActiveRunAnimSeq = nullptr;
	CombatAnimLockUntilTime = 0.f;
	bPlayingRunAnim = false;
	bPlayingCombatOneShot = false;

	if (!MeshComp || !SkelMesh || !SkelMesh->GetSkeleton())
	{
		return false;
	}

	const FString MeshId = SkelMesh->GetPathName() + TEXT(" ") + SkelMesh->GetName();

	const TCHAR* IdlePrimary   = nullptr;
	const TCHAR* IdleFallback  = nullptr;
	const TCHAR* RunPrimary    = nullptr;
	const TCHAR* RunFallback   = nullptr;
	const TCHAR* CastPath      = nullptr;
	const TCHAR* AttackSeqs[3] = { nullptr, nullptr, nullptr };
	const TCHAR* HitPath       = nullptr;
	const TCHAR* DeathPath     = nullptr;

	if (MeshId.Contains(TEXT("Gideon"), ESearchCase::IgnoreCase))
	{
		IdlePrimary   = AetheriaActorsUtil::PathGideonPedestalIdle;
		IdleFallback  = AetheriaActorsUtil::PathGideonIdle;
		RunPrimary    = AetheriaActorsUtil::PathGideonRun;
		CastPath      = AetheriaActorsUtil::PathGideonCast;
		AttackSeqs[0] = AetheriaActorsUtil::PathGideonAttackASeq;
		AttackSeqs[1] = AetheriaActorsUtil::PathGideonAttackBSeq;
		AttackSeqs[2] = AetheriaActorsUtil::PathGideonAttackCSeq;
		HitPath       = AetheriaActorsUtil::PathGideonHitReact;
		DeathPath     = AetheriaActorsUtil::PathGideonDeath;
	}
	else if (MeshId.Contains(TEXT("Aurora"), ESearchCase::IgnoreCase))
	{
		IdlePrimary   = AetheriaActorsUtil::PathAuroraIdle;
		RunPrimary    = AetheriaActorsUtil::PathAuroraRun;
		RunFallback   = AetheriaActorsUtil::PathAuroraRunBwd;
		CastPath      = AetheriaActorsUtil::PathAuroraCast;
		AttackSeqs[0] = AetheriaActorsUtil::PathAuroraAttackASeq;
		AttackSeqs[1] = AetheriaActorsUtil::PathAuroraAttackBSeq;
		AttackSeqs[2] = AetheriaActorsUtil::PathAuroraAttackCSeq;
		HitPath       = AetheriaActorsUtil::PathAuroraHitReact;
		DeathPath     = AetheriaActorsUtil::PathAuroraDeath;
	}
	else if (MeshId.Contains(TEXT("Khaimera"), ESearchCase::IgnoreCase))
	{
		IdlePrimary   = AetheriaActorsUtil::PathKhaimeraIdle;
		RunPrimary    = AetheriaActorsUtil::PathKhaimeraRun;
		CastPath      = AetheriaActorsUtil::PathKhaimeraCast;
		AttackSeqs[0] = AetheriaActorsUtil::PathKhaimeraAttackASeq;
		AttackSeqs[1] = AetheriaActorsUtil::PathKhaimeraAttackBSeq;
		AttackSeqs[2] = AetheriaActorsUtil::PathKhaimeraAttackCSeq;
		HitPath       = AetheriaActorsUtil::PathKhaimeraHitReact;
		DeathPath     = AetheriaActorsUtil::PathKhaimeraDeath;
	}
	else if (MeshId.Contains(TEXT("Grux"), ESearchCase::IgnoreCase))
	{
		IdlePrimary   = AetheriaActorsUtil::PathGruxIdle;
		RunPrimary    = AetheriaActorsUtil::PathGruxRun;
		CastPath      = AetheriaActorsUtil::PathGruxCast;
		AttackSeqs[0] = AetheriaActorsUtil::PathGruxAttackASeq;
		AttackSeqs[1] = AetheriaActorsUtil::PathGruxAttackBSeq;
		AttackSeqs[2] = AetheriaActorsUtil::PathGruxAttackCSeq;
		HitPath       = AetheriaActorsUtil::PathGruxHitReact;
		DeathPath     = AetheriaActorsUtil::PathGruxDeath;
	}
	else if (MeshId.Contains(TEXT("Morigesh"), ESearchCase::IgnoreCase))
	{
		IdlePrimary   = AetheriaActorsUtil::PathMorigeshIdle;
		RunPrimary    = AetheriaActorsUtil::PathMorigeshRun;
		CastPath      = AetheriaActorsUtil::PathMorigeshCast;
		AttackSeqs[0] = AetheriaActorsUtil::PathMorigeshAttackASeq;
		AttackSeqs[1] = AetheriaActorsUtil::PathMorigeshAttackBSeq;
		AttackSeqs[2] = AetheriaActorsUtil::PathMorigeshAttackCSeq;
		HitPath       = AetheriaActorsUtil::PathMorigeshHitReact;
		DeathPath     = AetheriaActorsUtil::PathMorigeshDeath;
	}
	else
	{
		return false;
	}

	auto LoadCompatibleSeq = [SkelMesh](const TCHAR* PrimaryPath, const TCHAR* FallbackPath = nullptr) -> UAnimSequence*
	{
		if (PrimaryPath)
		{
			if (UAnimSequence* Seq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(PrimaryPath))
			{
				if (AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, Seq->GetSkeleton()))
				{
					return Seq;
				}
			}
		}
		if (FallbackPath)
		{
			if (UAnimSequence* Seq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(FallbackPath))
			{
				if (AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, Seq->GetSkeleton()))
				{
					return Seq;
				}
			}
		}
		return nullptr;
	};

	ActiveIdleAnimSeq = LoadCompatibleSeq(IdlePrimary, IdleFallback);
	ActiveRunAnimSeq  = LoadCompatibleSeq(RunPrimary, RunFallback);
	if (UAnimSequence* CastSeq = LoadCompatibleSeq(CastPath))
	{
		AnimCast = CastSeq;
	}
	for (int32 i = 0; i < 3; ++i)
	{
		if (UAnimSequence* AtkSeq = LoadCompatibleSeq(AttackSeqs[i]))
		{
			AnimAttack[i] = AtkSeq;
		}
	}
	if (UAnimSequence* HitSeq = LoadCompatibleSeq(HitPath))
	{
		AnimHitReact = HitSeq;
	}
	if (UAnimSequence* DeathSeq = LoadCompatibleSeq(DeathPath))
	{
		AnimDeath = DeathSeq;
	}

	if (!ActiveIdleAnimSeq)
	{
		return false;
	}

	// SingleNode mode bypasses legacy Paragon *_AnimBlueprint assets that fail UE 5.8 BP compilation (eliminates A-pose!)
	for (int32 i = 0; i < 4; ++i)
	{
		HeroAttackMontages[i] = nullptr;
	}
	MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeshComp->PlayAnimation(ActiveIdleAnimSeq, true);
	MeshComp->SetPlayRate(1.0f);
	bPlayingRunAnim = false;
	bPlayingCombatOneShot = false;

	UE_LOG(LogChronicles, Log, TEXT("AAetheriaHeroCharacter bound SingleNode Paragon anims for %s (Idle=%s, Run=%s)"),
		*SkelMesh->GetName(),
		*ActiveIdleAnimSeq->GetName(),
		ActiveRunAnimSeq ? *ActiveRunAnimSeq->GetName() : TEXT("None"));
	return true;
}

void AAetheriaHeroCharacter::SetupHeroSkeletalMeshAndAnims()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// Silky Crimson Desert 3rd-person camera framing & lag
	if (FollowCamera && !bCreatorStudioCameraActive)
	{
		FollowCamera->SetFieldOfView(78.0f);
	}
	if (CameraBoom && !bCreatorStudioCameraActive)
	{
		CameraBoom->bEnableCameraLag = true;
		CameraBoom->CameraLagSpeed = 12.0f;
	}

	// Auto-upgrade priority:
	// /Game/FabImports/Hero -> ParagonGideon -> ParagonKwang -> ParagonKhaimera -> ParagonGrux
	// -> ParagonAurora -> ParagonMorigesh -> ParagonSerath -> ParagonGreystone
	// -> SKM_Hero_00 -> SKM_Manny_Simple
	USkeletalMesh* ChosenMesh = nullptr;
	const TCHAR* PreferredParagonABP = nullptr;

	// 1. Custom FabImports/Hero override
	{
		TArray<FString> FabHeroDirs;
		FabHeroDirs.Add(TEXT("/Game/FabImports/Hero"));
		ChosenMesh = AetheriaActorsUtil::FindFirstSkeletalMeshInPaths(FabHeroDirs);
	}

	// 2. Paragon Hero packs (explicit primary mesh first so Proto_Retarget is never selected, then folder fallback)
	struct FParagonHeroSpec
	{
		const TCHAR* FolderPath;
		const TCHAR* PrimaryMeshPath;
		const TCHAR* AnimBPPath;
	};
	static const FParagonHeroSpec ParagonHeroSpecs[] = {
		{ TEXT("/Game/ParagonGideon"),    AetheriaActorsUtil::PathGideonToughMesh, AetheriaActorsUtil::PathGideonABP },
		{ TEXT("/Game/ParagonGideon"),    AetheriaActorsUtil::PathGideonMesh,      AetheriaActorsUtil::PathGideonABP },
		{ TEXT("/Game/ParagonKwang"),     AetheriaActorsUtil::PathKwangMesh,       AetheriaActorsUtil::PathKwangABP },
		{ TEXT("/Game/ParagonKhaimera"),  AetheriaActorsUtil::PathKhaimeraMesh,    AetheriaActorsUtil::PathKhaimeraABP },
		{ TEXT("/Game/ParagonGrux"),      AetheriaActorsUtil::PathGruxMesh,        AetheriaActorsUtil::PathGruxABP },
		{ TEXT("/Game/ParagonAurora"),    AetheriaActorsUtil::PathAuroraMesh,      AetheriaActorsUtil::PathAuroraABP },
		{ TEXT("/Game/ParagonMorigesh"),  AetheriaActorsUtil::PathMorigeshMesh,    AetheriaActorsUtil::PathMorigeshABP },
		{ TEXT("/Game/ParagonSerath"),    AetheriaActorsUtil::PathSerathMesh,      AetheriaActorsUtil::PathSerathABP },
		{ TEXT("/Game/ParagonGreystone"), AetheriaActorsUtil::PathGreystoneMesh,   AetheriaActorsUtil::PathGreystoneABP }
	};

	for (const FParagonHeroSpec& Spec : ParagonHeroSpecs)
	{
		if (ChosenMesh)
		{
			break;
		}
		ChosenMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(Spec.PrimaryMeshPath);
		if (!ChosenMesh)
		{
			TArray<FString> SingleDir;
			SingleDir.Add(Spec.FolderPath);
			ChosenMesh = AetheriaActorsUtil::FindFirstSkeletalMeshInPaths(SingleDir);
		}
		if (ChosenMesh)
		{
			PreferredParagonABP = Spec.AnimBPPath;
		}
	}

	// 3. Staged SKM_Hero_00 -> SKM_Manny_Simple fallback
	if (!ChosenMesh)
	{
		ChosenMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathHeroFallback0);
	}
	if (!ChosenMesh)
	{
		ChosenMesh = LoadObject<USkeletalMesh>(nullptr, AetheriaActorsUtil::PathMannySimple);
	}

	// Pre-load default Manny combat UAnimSequence clips into GC-tracked UPROPERTY slots as baseline fallback
	AnimAttack[0] = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimAttack1);
	AnimAttack[1] = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimAttack2);
	AnimAttack[2] = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimAttack3);
	AnimDash      = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimDash);
	AnimHitReact  = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimHitReact);
	AnimDeath     = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimDeath);
	AnimCast      = nullptr;
	for (int32 i = 0; i < 4; ++i)
	{
		HeroAttackMontages[i] = nullptr;
	}
	bUsingKwangHero = false;
	HeroAnimSlotName = FName(TEXT("DefaultSlot"));

	if (ChosenMesh)
	{
		MeshComp->SetSkeletalMesh(ChosenMesh);

		// Drive staged Paragon heroes via AnimationSingleNode (Idle + Jog_Fwd + combat sequences) so broken legacy
		// Paragon *_AnimBlueprint graphs never freeze the Hero in ref A-pose!
		const bool bBoundSingleNodeParagon = BindParagonHeroSingleNodeAnims(ChosenMesh);
		if (!bBoundSingleNodeParagon)
		{
			TArray<const TCHAR*> CandidateABPs;
			if (PreferredParagonABP && !FString(PreferredParagonABP).Contains(TEXT("/Game/Paragon")))
			{
				CandidateABPs.Add(PreferredParagonABP);
			}
			CandidateABPs.Add(AetheriaActorsUtil::PathABPCombat);
			CandidateABPs.Add(AetheriaActorsUtil::PathABPUnarmed);
			AetheriaActorsUtil::TryBindCompatibleAnimBPFromList(MeshComp, CandidateABPs);
		}

		const bool bIsParagonHero = ChosenMesh->GetPathName().Contains(TEXT("/Game/Paragon"));
		if (bIsParagonHero)
		{
			HeroAnimSlotName = FName(TEXT("FullBody"));
			// Paragon heroes carry native sculpted weapons / astral casting rings
			if (HeroWeaponMesh)
			{
				HeroWeaponMesh->SetVisibility(false);
			}
		}
		else if (HeroWeaponMesh)
		{
			// Attach real PBR sword (/Game/Longsword or /Game/Sword) -> fallback arcane staff for non-Paragon meshes
			const FName WeaponSocket = MeshComp->DoesSocketExist(TEXT("weapon_r")) ? FName(TEXT("weapon_r"))
				: (MeshComp->DoesSocketExist(TEXT("hand_r")) ? FName(TEXT("hand_r")) : NAME_None);
			if (!WeaponSocket.IsNone())
			{
				UStaticMesh* WeaponMeshAsset = AetheriaActorsUtil::SafeLoadObjectIfPresent<UStaticMesh>(AetheriaActorsUtil::PathLongswordMesh);
				if (!WeaponMeshAsset)
				{
					WeaponMeshAsset = AetheriaActorsUtil::SafeLoadObjectIfPresent<UStaticMesh>(AetheriaActorsUtil::PathSwordMesh);
				}
				if (!WeaponMeshAsset)
				{
					WeaponMeshAsset = AetheriaActorsUtil::SafeLoadObjectIfPresent<UStaticMesh>(AetheriaActorsUtil::PathStaffArcane);
				}
				if (WeaponMeshAsset)
				{
					HeroWeaponMesh->SetStaticMesh(WeaponMeshAsset);
					HeroWeaponMesh->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
					HeroWeaponMesh->SetRelativeScale3D(FVector(0.92f));
					HeroWeaponMesh->SetVisibility(true);
				}
			}
		}

		if (ChosenMesh->GetPathName().Contains(TEXT("/Game/ParagonKwang")) || ChosenMesh->GetName().Contains(TEXT("Kwang")))
		{
			bUsingKwangHero = true;

			static const TCHAR* KwangMontagePaths[4] = {
				AetheriaActorsUtil::PathKwangMeleeA,
				AetheriaActorsUtil::PathKwangMeleeB,
				AetheriaActorsUtil::PathKwangMeleeC,
				AetheriaActorsUtil::PathKwangMeleeD
			};
			for (int32 i = 0; i < 4; ++i)
			{
				if (UAnimMontage* M = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimMontage>(KwangMontagePaths[i]))
				{
					if (AetheriaActorsUtil::IsSkeletonCompatible(ChosenMesh, M->GetSkeleton()))
					{
						HeroAttackMontages[i] = M;
					}
				}
			}

			static const TCHAR* KwangSeqPaths[3] = {
				AetheriaActorsUtil::PathKwangMeleeASeq,
				AetheriaActorsUtil::PathKwangMeleeBSeq,
				AetheriaActorsUtil::PathKwangMeleeCSeq
			};
			for (int32 i = 0; i < 3; ++i)
			{
				if (UAnimSequence* Seq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(KwangSeqPaths[i]))
				{
					if (AetheriaActorsUtil::IsSkeletonCompatible(ChosenMesh, Seq->GetSkeleton()))
					{
						AnimAttack[i] = Seq;
					}
				}
			}

			if (UAnimSequence* HitSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathKwangHitReact))
			{
				if (AetheriaActorsUtil::IsSkeletonCompatible(ChosenMesh, HitSeq->GetSkeleton()))
				{
					AnimHitReact = HitSeq;
				}
			}
			if (UAnimSequence* DeathSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathKwangDeath))
			{
				if (AetheriaActorsUtil::IsSkeletonCompatible(ChosenMesh, DeathSeq->GetSkeleton()))
				{
					AnimDeath = DeathSeq;
				}
			}
			if (UAnimSequence* CastSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathKwangCast))
			{
				if (AetheriaActorsUtil::IsSkeletonCompatible(ChosenMesh, CastSeq->GetSkeleton()))
				{
					AnimCast = CastSeq;
				}
			}

			KwangPrimaryImpactFX    = AetheriaActorsUtil::SafeLoadObjectIfPresent<UParticleSystem>(AetheriaActorsUtil::PathKwangPrimaryImpactFX);
			KwangLightStrikeBurstFX = AetheriaActorsUtil::SafeLoadObjectIfPresent<UParticleSystem>(AetheriaActorsUtil::PathKwangLightStrikeBurstFX);
			KwangAttackEffortSFX    = AetheriaActorsUtil::SafeLoadObjectIfPresent<USoundBase>(AetheriaActorsUtil::PathKwangEffortPrimarySFX);
		}

		UE_LOG(LogChronicles, Log, TEXT("AAetheriaHeroCharacter bound SkeletalMesh: %s (Kwang=%d)"), *ChosenMesh->GetName(), bUsingKwangHero ? 1 : 0);
	}
}

bool AAetheriaHeroCharacter::ApplyCustomSkeletalRig(
	const FString& MeshPath,
	const FString& AnimBPPath,
	const TArray<FString>& AttackMontagePaths,
	const FString& HitReactPath,
	const FString& DeathPath,
	FName SlotName)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || MeshPath.IsEmpty() || !AetheriaActorsUtil::DoesObjectPackageExist(*MeshPath))
	{
		return false;
	}

	USkeletalMesh* NewMesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath);
	if (!NewMesh || !NewMesh->GetSkeleton())
	{
		return false;
	}

	MeshComp->SetSkeletalMesh(NewMesh);

	for (int32 i = 0; i < 4; ++i)
	{
		HeroAttackMontages[i] = nullptr;
	}
	AnimHitReact = nullptr;
	AnimDeath = nullptr;

	// Drive staged Paragon heroes via AnimationSingleNode (Idle + Jog_Fwd + combat sequences) instead of broken Paragon AnimBPs
	const bool bBoundSingleNodeParagon = BindParagonHeroSingleNodeAnims(NewMesh);
	if (!bBoundSingleNodeParagon)
	{
		TArray<const TCHAR*> CandidateABPs;
		if (!AnimBPPath.IsEmpty() && !AnimBPPath.Contains(TEXT("/Game/Paragon")))
		{
			CandidateABPs.Add(*AnimBPPath);
		}
		CandidateABPs.Add(AetheriaActorsUtil::PathABPCombat);
		CandidateABPs.Add(AetheriaActorsUtil::PathABPUnarmed);
		AetheriaActorsUtil::TryBindCompatibleAnimBPFromList(MeshComp, CandidateABPs);

		const int32 NumMontages = FMath::Min(4, AttackMontagePaths.Num());
		for (int32 i = 0; i < NumMontages; ++i)
		{
			if (UAnimMontage* Montage = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimMontage>(*AttackMontagePaths[i]))
			{
				if (Montage->GetSkeleton() && AetheriaActorsUtil::IsSkeletonCompatible(NewMesh, Montage->GetSkeleton()))
				{
					HeroAttackMontages[i] = Montage;
				}
			}
		}
	}

	if (!AnimHitReact && !HitReactPath.IsEmpty())
	{
		if (UAnimSequence* HitSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(*HitReactPath))
		{
			if (HitSeq->GetSkeleton() && AetheriaActorsUtil::IsSkeletonCompatible(NewMesh, HitSeq->GetSkeleton()))
			{
				AnimHitReact = HitSeq;
			}
		}
	}

	if (!AnimDeath && !DeathPath.IsEmpty())
	{
		if (UAnimSequence* DeathSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(*DeathPath))
		{
			if (DeathSeq->GetSkeleton() && AetheriaActorsUtil::IsSkeletonCompatible(NewMesh, DeathSeq->GetSkeleton()))
			{
				AnimDeath = DeathSeq;
			}
		}
	}

	HeroAnimSlotName = SlotName.IsNone() ? FName(TEXT("FullBody")) : SlotName;
	bUsingKwangHero = MeshPath.Contains(TEXT("Kwang")) || NewMesh->GetName().Contains(TEXT("Kwang"));

	if (HeroWeaponMesh)
	{
		HeroWeaponMesh->SetVisibility(false);
	}

	FAetheriaStyleHarmonizer::HarmonizeActorMaterials(this, EChronicleStyleCategory::Character);
	UE_LOG(LogChronicles, Log, TEXT("AAetheriaHeroCharacter::ApplyCustomSkeletalRig bound %s (Slot=%s)"),
		*NewMesh->GetName(), *HeroAnimSlotName.ToString());
	return true;
}

bool AAetheriaHeroCharacter::CanCastSpellsByStature() const
{
	return Customization ? Customization->GetDerivedStats().bCanCastSpells : true;
}

void AAetheriaHeroCharacter::SetCreatorStudioCameraMode(bool bEnable, float OrbitYawDeg, int32 ZoomMode)
{
	bCreatorStudioCameraActive = bEnable;
	CreatorStudioOrbitYaw = OrbitYawDeg;
	CreatorStudioZoomMode = FMath::Clamp(ZoomMode, 0, 2);

	const float HeightScale = Customization
		? (Customization->Profile.HeightMeters / UHeroCustomizationComponent::BASE_HEIGHT_METERS)
		: 1.0f;

	if (CameraBoom)
	{
		CameraBoom->bUsePawnControlRotation = !bEnable;
		CameraBoom->bDoCollisionTest = !bEnable;
		CameraBoom->bEnableCameraLag = !bEnable;
		CameraBoom->CameraLagSpeed = 12.0f;
	}
	if (FollowCamera)
	{
		// 54 deg portrait lens in Creator Studio vs 78 deg Crimson Desert 3rd-person lens
		FollowCamera->SetFieldOfView(bEnable ? 54.0f : 78.0f);
	}

	if (bEnable)
	{
		// Immediately pose Hero in crisp heroic breathing idle inside Creator Studio (never A-pose!)
		if (ActiveIdleAnimSeq)
		{
			if (USkeletalMeshComponent* MeshComp = GetMesh())
			{
				MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				MeshComp->PlayAnimation(ActiveIdleAnimSeq, true);
				MeshComp->SetPlayRate(1.0f);
				bPlayingRunAnim = false;
				bPlayingCombatOneShot = false;
				CombatAnimLockUntilTime = 0.f;
			}
		}

		// Hide orbiting element orbs cleanly in Hero Customizer Studio
		for (int32 i = 0; i < 3; ++i)
		{
			if (OrbitingOrbMeshes[i])
			{
				OrbitingOrbMeshes[i]->SetVisibility(false);
			}
		}

		// Position calibrated 3-point studio lighting immediately (warm 3/4 key + cool rim)
		const float CamYaw = CreatorStudioOrbitYaw + 180.f;
		if (CreatorStudioFillLight)
		{
			const FVector KeyDir = FRotator(25.f, CamYaw + 45.f, 0.f).Vector();
			CreatorStudioFillLight->SetRelativeLocation(KeyDir * (175.f * HeightScale) + FVector(0.f, 0.f, 45.f * HeightScale));
			CreatorStudioFillLight->SetLightColor(FLinearColor(1.0f, 0.90f, 0.78f, 1.0f));
			CreatorStudioFillLight->SetAttenuationRadius(480.f);
			CreatorStudioFillLight->SetSpecularScale(0.22f);
			CreatorStudioFillLight->SetIntensity(145.0f);
			CreatorStudioFillLight->SetVisibility(true);
		}
		if (CreatorStudioRimLight)
		{
			const FVector RimDir = FRotator(28.f, CreatorStudioOrbitYaw - 35.f, 0.f).Vector();
			CreatorStudioRimLight->SetRelativeLocation(RimDir * (165.f * HeightScale) + FVector(0.f, 0.f, 55.f * HeightScale));
			CreatorStudioRimLight->SetLightColor(FLinearColor(0.58f, 0.78f, 1.0f, 1.0f));
			CreatorStudioRimLight->SetAttenuationRadius(460.f);
			CreatorStudioRimLight->SetSpecularScale(0.20f);
			CreatorStudioRimLight->SetIntensity(95.0f);
			CreatorStudioRimLight->SetVisibility(true);
		}
	}
	else
	{
		if (CreatorStudioFillLight)
		{
			CreatorStudioFillLight->SetIntensity(0.f);
			CreatorStudioFillLight->SetVisibility(false);
		}
		if (CreatorStudioRimLight)
		{
			CreatorStudioRimLight->SetIntensity(0.f);
			CreatorStudioRimLight->SetVisibility(false);
		}

		// Restore active orbiting element orbs immediately upon exiting studio mode
		UpdateOrbitingOrbs(0.f);
	}
}

void AAetheriaHeroCharacter::SetupOrbitingOrbs()
{
	// Prefer M_AetheriaEmissiveCore -> M_AetheriaFireball -> Engine EmissiveMeshMaterial at calibrated 2.2x intensity
	UMaterialInterface* OrbBaseMat = AetheriaActorsUtil::SafeLoadObjectIfPresent<UMaterialInterface>(AetheriaActorsUtil::PathMatEmissiveCore);
	if (OrbBaseMat)
	{
		OrbitingOrbMatMode = 1;
	}
	else if (UMaterialInterface* FireballMat = AetheriaActorsUtil::SafeLoadObjectIfPresent<UMaterialInterface>(AetheriaActorsUtil::PathMatFireball))
	{
		OrbBaseMat = FireballMat;
		OrbitingOrbMatMode = 2;
	}
	else
	{
		OrbitingOrbMatMode = 0;
	}

	const FLinearColor DefaultCol = AetheriaActorsUtil::GetOrbVisualColor(EChronicleOrb::Exort);
	for (int32 i = 0; i < 3; ++i)
	{
		if (OrbBaseMat && OrbitingOrbMatMode != 0)
		{
			OrbitingOrbMIDs[i] = UMaterialInstanceDynamic::Create(OrbBaseMat, this);
			if (OrbitingOrbMIDs[i])
			{
				if (OrbitingOrbMatMode == 1)
				{
					OrbitingOrbMIDs[i]->SetVectorParameterValue(TEXT("EmissiveColor"), DefaultCol);
					OrbitingOrbMIDs[i]->SetScalarParameterValue(TEXT("EmissiveBoost"), 2.2f);
					OrbitingOrbMIDs[i]->SetScalarParameterValue(TEXT("Opacity"), 0.92f);
				}
				else
				{
					OrbitingOrbMIDs[i]->SetVectorParameterValue(TEXT("BaseColor"), DefaultCol);
					OrbitingOrbMIDs[i]->SetVectorParameterValue(TEXT("AccentColor"), DefaultCol * 1.10f);
					OrbitingOrbMIDs[i]->SetScalarParameterValue(TEXT("EmissiveBoost"), 2.2f);
				}
			}
		}
		else
		{
			OrbitingOrbMIDs[i] = AetheriaActorsUtil::CreateColorMID(this, DefaultCol * 2.2f, true);
		}

		if (OrbitingOrbMeshes[i])
		{
			OrbitingOrbMeshes[i]->SetRelativeScale3D(FVector(0.105f));
			if (OrbitingOrbMIDs[i])
			{
				OrbitingOrbMeshes[i]->SetMaterial(0, OrbitingOrbMIDs[i]);
			}
		}
	}
}

UStaticMeshComponent* AAetheriaHeroCharacter::GetOrbitingOrbMesh(int32 Index) const
{
	return (Index >= 0 && Index < 3) ? OrbitingOrbMeshes[Index].Get() : nullptr;
}

void AAetheriaHeroCharacter::PlayCombatAnimSequence(UAnimSequence* Seq, float PlayRate)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !Seq)
	{
		return;
	}

	if (ActiveIdleAnimSeq || MeshComp->GetAnimationMode() == EAnimationMode::AnimationSingleNode)
	{
		USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset();
		const USkeleton* SeqSkeleton = Seq->GetSkeleton();
		if (!SkelMesh || !SeqSkeleton || !AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, SeqSkeleton))
		{
			return;
		}

		const float SafeRate = FMath::Max(PlayRate, 0.2f);
		MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComp->PlayAnimation(Seq, false);
		MeshComp->SetPlayRate(SafeRate);
		bPlayingRunAnim = false;
		bPlayingCombatOneShot = true;

		const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		CombatAnimLockUntilTime = WorldTime + FMath::Clamp(Seq->GetPlayLength() / SafeRate, 0.35f, 1.4f);
		return;
	}

	AetheriaActorsUtil::PlaySafeDynamicMontage(MeshComp, Seq, PlayRate, 0.08f, 0.16f, HeroAnimSlotName);
}

void AAetheriaHeroCharacter::PlayHeroMeleeVisualsAndAudio(int32 Step)
{
	const int32 ClampedStep = FMath::Clamp(Step, 0, 3);
	const int32 MaxComboSteps = (HeroAttackMontages[3] != nullptr) ? 4 : 3;
	const bool bFinisher = (ClampedStep == (MaxComboSteps - 1));
	const float AtkSpeedMult = Customization ? FMath::Max(0.4f, Customization->GetDerivedStats().AttackSpeedMultiplier) : 1.0f;
	const float PlayRate = (bFinisher ? 1.12f : 1.05f) * AtkSpeedMult;

	bool bPlayedNativeMontage = false;
	USkeletalMeshComponent* MeshComp = GetMesh();
	const bool bUsingSingleNode = (ActiveIdleAnimSeq != nullptr)
		|| (MeshComp && MeshComp->GetAnimationMode() == EAnimationMode::AnimationSingleNode);
	UAnimMontage* NativeMontage = HeroAttackMontages[ClampedStep].Get();
	if (!bUsingSingleNode && MeshComp && NativeMontage)
	{
		USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset();
		UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
		if (SkelMesh && AnimInst && NativeMontage->GetSkeleton() && AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, NativeMontage->GetSkeleton()))
		{
			AnimInst->Montage_Play(NativeMontage, PlayRate);
			bPlayedNativeMontage = true;
		}
	}

	if (!bPlayedNativeMontage)
	{
		PlayCombatAnimSequence(AnimAttack[FMath::Clamp(ClampedStep, 0, 2)], PlayRate);
	}

	if (KwangAttackEffortSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, KwangAttackEffortSFX, GetActorLocation());
	}
}

void AAetheriaHeroCharacter::TriggerCombatFlash(const FLinearColor& Color, float PeakLumens, float Duration)
{
	if (!CombatFlashLight)
	{
		return;
	}
	FlashPeakLumens = PeakLumens;
	FlashDuration = FMath::Max(0.06f, Duration);
	FlashTimer = FlashDuration;
	CombatFlashLight->SetLightColor(Color);
	CombatFlashLight->SetIntensity(PeakLumens);
}

void AAetheriaHeroCharacter::PerformMeleeAttack()
{
	if (!HasAuthority())
	{
		ServerPerformMeleeAttack();
		// Local responsive VFX & anim preview
		if (!bDead && RollTimer <= 0.f && MeleeCooldownTimer <= 0.f)
		{
			const int32 MaxComboSteps = (HeroAttackMontages[3] != nullptr) ? 4 : 3;
			const int32 PreviewIdx = FMath::Clamp(ComboIndex, 0, MaxComboSteps - 1);
			const bool bFinisher = (PreviewIdx == (MaxComboSteps - 1));
			PlayHeroMeleeVisualsAndAudio(PreviewIdx);
			TriggerCombatFlash(bFinisher ? FLinearColor(1.0f, 0.42f, 0.12f) : FLinearColor(1.0f, 0.72f, 0.30f),
				bFinisher ? 9500.f : 6500.f, 0.22f);
		}
		return;
	}

	ExecuteMeleeAttack_Internal();
}

void AAetheriaHeroCharacter::ServerPerformMeleeAttack_Implementation()
{
	ExecuteMeleeAttack_Internal();
}

void AAetheriaHeroCharacter::ExecuteMeleeAttack_Internal()
{
	if (bDead || RollTimer > 0.f || MeleeCooldownTimer > 0.f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ComboResetTimer <= 0.f)
	{
		ComboIndex = 0;
	}

	const FHeroDerivedStats Stats = Customization ? Customization->GetDerivedStats() : FHeroDerivedStats();
	const float SafeAtkSpeed = FMath::Max(0.4f, Stats.AttackSpeedMultiplier);

	const int32 MaxComboSteps = (HeroAttackMontages[3] != nullptr) ? 4 : 3;
	const int32 FinisherStep = MaxComboSteps - 1;
	const int32 CurrentStep = FMath::Clamp(ComboIndex, 0, FinisherStep);
	ComboIndex = (CurrentStep + 1) % MaxComboSteps;
	ComboResetTimer = 1.45f;
	MeleeCooldownTimer = ((CurrentStep == FinisherStep) ? 0.50f : 0.34f) / SafeAtkSpeed;
	bBlocking = false;

	// Play 4-hit native montage (A -> B -> C -> D) or sequence fallback + effort SFX
	PlayHeroMeleeVisualsAndAudio(CurrentStep);

	// MotionWarping: gently lock warp target onto nearest enemy in front melee arc
	AAetheriaEnemyCharacter* WarpEnemy = nullptr;
	float BestDistSq = FMath::Square(390.f * FMath::Max(0.75f, Stats.MeleeReachMultiplier));
	const FVector MyLoc = GetActorLocation();
	const FVector MyFwd = GetActorForwardVector();

	for (TActorIterator<AAetheriaEnemyCharacter> It(World); It; ++It)
	{
		AAetheriaEnemyCharacter* Enemy = *It;
		if (!Enemy || Enemy->bDead)
		{
			continue;
		}
		const FVector ToEnemy = Enemy->GetActorLocation() - MyLoc;
		const float DistSq = ToEnemy.SizeSquared();
		if (DistSq < BestDistSq && FVector::DotProduct(MyFwd, ToEnemy.GetSafeNormal()) > 0.25f)
		{
			BestDistSq = DistSq;
			WarpEnemy = Enemy;
		}
	}

	if (MotionWarping && WarpEnemy)
	{
		const FVector ToTarget = (WarpEnemy->GetActorLocation() - MyLoc).GetSafeNormal2D();
		const FVector WarpPos = WarpEnemy->GetActorLocation() - ToTarget * 125.f;
		MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("MeleeTarget"), WarpPos, ToTarget.Rotation());
	}

	// SkillTree + Customization combat scaling + Critical Hit roll
	const float MeleeMult = SkillTree ? SkillTree->GetMeleeDamageMultiplier() : 1.f;
	const float StaggerMult = (SkillTree ? SkillTree->GetStaggerMultiplier() : 1.f) * Stats.StaggerMultiplier;
	const float ExecMult = SkillTree ? SkillTree->GetExecutionerMultiplier() : 1.f;
	const float StaggerBonusSec = SkillTree ? SkillTree->GetStaggerWindowBonusSeconds() : 0.f;

	static const float ComboBaseDmg4[4]     = { 26.f, 34.f, 44.f, 58.f };
	static const float ComboBaseStagger4[4] = { 14.f, 18.f, 22.f, 32.f };
	static const float ComboBaseDmg3[3]     = { 26.f, 34.f, 52.f };
	static const float ComboBaseStagger3[3] = { 14.f, 18.f, 30.f };

	const float BaseDmg = (MaxComboSteps == 4) ? ComboBaseDmg4[CurrentStep] : ComboBaseDmg3[CurrentStep];
	const float BaseStagger = (MaxComboSteps == 4) ? ComboBaseStagger4[CurrentStep] : ComboBaseStagger3[CurrentStep];

	const bool bIsCrit = (FMath::FRand() < Stats.CritChance);
	const float CritMult = bIsCrit ? FMath::Max(1.0f, Stats.CritDamageMultiplier) : 1.0f;
	const float DamageToDeal = BaseDmg * MeleeMult * Stats.MeleeDamageMultiplier * CritMult;
	const float StaggerToDeal = BaseStagger * StaggerMult;

	// Physical AgX Lumen melee slash flash (Golden crit flash on crit, else warm blade slash)
	const bool bIsFinisherHit = (CurrentStep == FinisherStep);
	if (bIsCrit)
	{
		TriggerCombatFlash(FLinearColor(1.0f, 0.86f, 0.16f), 11500.f, 0.28f);
	}
	else
	{
		TriggerCombatFlash(
			bIsFinisherHit ? FLinearColor(1.0f, 0.38f, 0.10f) : FLinearColor(1.0f, 0.72f, 0.28f),
			bIsFinisherHit ? 9500.f : 6500.f,
			0.22f);
	}

	// CHRONICLE_TRACE_PROJECTILE arc sweep (capsule rotated from +Z via FindBetweenNormals)
	const float ReachMult = FMath::Max(0.6f, Stats.MeleeReachMultiplier);
	const FVector SweepDir = MyFwd.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector SweepStart = MyLoc + FVector(0.f, 0.f, 45.f) + SweepDir * 45.f;
	const FVector SweepEnd = SweepStart + SweepDir * ((bIsFinisherHit ? 295.f : 255.f) * ReachMult);

	FCollisionQueryParams Params(TEXT("HeroMeleeSweep"), false, this);
	Params.AddIgnoredActor(this);

	const float CapsuleRadius = (bIsFinisherHit ? 135.f : 108.f) * FMath::Lerp(1.0f, ReachMult, 0.5f);
	const FCollisionShape SweepShape = FCollisionShape::MakeCapsule(CapsuleRadius, 120.f);
	const FQuat SweepQuat = AetheriaActorsUtil::MakeCapsuleSweepQuat(SweepDir);

	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(Hits, SweepStart, SweepEnd, SweepQuat, CHRONICLE_TRACE_PROJECTILE, SweepShape, Params);

	TSet<AActor*> HitActors;
	int32 EnemiesStruck = 0;

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == this || HitActors.Contains(HitActor))
		{
			continue;
		}
		HitActors.Add(HitActor);

		const FVector ImpactLoc = Hit.ImpactPoint.IsNearlyZero() ? HitActor->GetActorLocation() : FVector(Hit.ImpactPoint);

		if (AAetheriaBossCharacter* Boss = Cast<AAetheriaBossCharacter>(HitActor))
		{
			const float FinalBossDmg = Boss->bStaggerBroken ? (DamageToDeal * ExecMult) : DamageToDeal;
			UGameplayStatics::ApplyPointDamage(Boss, FinalBossDmg, SweepDir, Hit, GetController(), this, UDamageType::StaticClass());
			Boss->AddStagger(StaggerToDeal, StaggerBonusSec);
			++EnemiesStruck;

			if (UHitStopSubsystem* HitStop = UHitStopSubsystem::Get(this))
			{
				HitStop->RequestHitStop(this, HitActor, (CurrentStep >= 2 ? 0.055f : 0.035f), 0.07f);
			}

			if (KwangPrimaryImpactFX)
			{
				UGameplayStatics::SpawnEmitterAtLocation(World, KwangPrimaryImpactFX, ImpactLoc, SweepDir.Rotation(), FVector(1.0f), true);
			}
			if (CurrentStep >= 2 && KwangLightStrikeBurstFX)
			{
				UGameplayStatics::SpawnEmitterAtLocation(World, KwangLightStrikeBurstFX, ImpactLoc, FRotator::ZeroRotator, FVector(1.0f), true);
			}
		}
		else if (AAetheriaEnemyCharacter* Enemy = Cast<AAetheriaEnemyCharacter>(HitActor))
		{
			UGameplayStatics::ApplyPointDamage(Enemy, DamageToDeal, SweepDir, Hit, GetController(), this, UDamageType::StaticClass());
			++EnemiesStruck;

			if (UHitStopSubsystem* HitStop = UHitStopSubsystem::Get(this))
			{
				HitStop->RequestHitStop(this, HitActor, (CurrentStep >= 2 ? 0.055f : 0.035f), 0.07f);
			}

			if (KwangPrimaryImpactFX)
			{
				UGameplayStatics::SpawnEmitterAtLocation(World, KwangPrimaryImpactFX, ImpactLoc, SweepDir.Rotation(), FVector(1.0f), true);
			}
			if (CurrentStep >= 2 && KwangLightStrikeBurstFX)
			{
				UGameplayStatics::SpawnEmitterAtLocation(World, KwangLightStrikeBurstFX, ImpactLoc, FRotator::ZeroRotator, FVector(1.0f), true);
			}
		}
		else if (ADestructiblePropActor* Destructible = Cast<ADestructiblePropActor>(HitActor))
		{
			UGameplayStatics::ApplyPointDamage(Destructible, DamageToDeal * 1.45f * Stats.DestructionDamageMultiplier, SweepDir, Hit, GetController(), this, UDamageType::StaticClass());
		}
	}

	if (EnemiesStruck > 0)
	{
		if (UAetheriaMusicSubsystem* MusicSys = UAetheriaMusicSubsystem::Get(this))
		{
			MusicSys->NotifyCombatActivity();
		}

		// Reward aggressive melee-spell weaving with small mana siphon per enemy struck
		if (SpellSystem)
		{
			SpellSystem->RestoreMana(4.5f * static_cast<float>(EnemiesStruck));
		}
	}
}

void AAetheriaHeroCharacter::SetBlocking(bool bNewBlocking)
{
	if (!HasAuthority())
	{
		ServerSetBlocking(bNewBlocking);
		return;
	}

	if (bDead)
	{
		bBlocking = false;
		ParryWindowTimer = 0.f;
		return;
	}

	if (!bBlocking && bNewBlocking)
	{
		// Open 0.25s (+ ParryWindowBonusSec) Wukong-style timed parry window on block entry
		const float ParryBonus = Customization ? Customization->GetDerivedStats().ParryWindowBonusSec : 0.f;
		ParryWindowTimer = 0.25f + ParryBonus;
	}
	bBlocking = bNewBlocking;
}

void AAetheriaHeroCharacter::ServerSetBlocking_Implementation(bool bNewBlocking)
{
	SetBlocking(bNewBlocking);
}

void AAetheriaHeroCharacter::PerformDodgeRoll(const FVector& WorldMoveDir)
{
	const float DodgeCostMult = Customization ? FMath::Max(0.25f, Customization->GetDerivedStats().DodgeStaminaCostMultiplier) : 1.0f;
	const float StamCost = 20.f * DodgeCostMult;

	if (!HasAuthority())
	{
		ServerPerformDodgeRoll(WorldMoveDir);
		if (!bDead && RollTimer <= 0.f && Stamina >= StamCost)
		{
			PlayCombatAnimSequence(AnimDash, 1.15f);
		}
		return;
	}

	ExecuteDodgeRoll_Internal(WorldMoveDir);
}

void AAetheriaHeroCharacter::ServerPerformDodgeRoll_Implementation(const FVector& WorldMoveDir)
{
	ExecuteDodgeRoll_Internal(WorldMoveDir);
}

void AAetheriaHeroCharacter::ExecuteDodgeRoll_Internal(const FVector& WorldMoveDir)
{
	const FHeroDerivedStats Stats = Customization ? Customization->GetDerivedStats() : FHeroDerivedStats();
	const float StamCost = 20.f * FMath::Max(0.25f, Stats.DodgeStaminaCostMultiplier);
	if (bDead || RollTimer > 0.f || Stamina < StamCost)
	{
		return;
	}

	Stamina = FMath::Max(0.f, Stamina - StamCost);
	bBlocking = false;
	ParryWindowTimer = 0.f;

	const float IFrameMult = SkillTree ? SkillTree->GetDodgeIFrameMultiplier() : 1.f;
	const float MoveMult = SkillTree ? SkillTree->GetMoveSpeedMultiplier() : 1.f;

	RollTimer = 0.42f;
	InvulnTimer = 0.34f * IFrameMult + Stats.DodgeIFrameBonusSec;

	FVector Dir2D(WorldMoveDir.X, WorldMoveDir.Y, 0.f);
	if (Dir2D.IsNearlyZero())
	{
		Dir2D = GetActorForwardVector().GetSafeNormal2D();
	}
	else
	{
		Dir2D.Normalize();
	}
	ActiveRollDir = Dir2D;

	LaunchCharacter(ActiveRollDir * (980.f * MoveMult * Stats.DodgeDistanceMultiplier) + FVector(0.f, 0.f, 110.f), true, false);
	PlayCombatAnimSequence(AnimDash, 1.15f);
}

float AAetheriaHeroCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bDead || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	if (UAetheriaMusicSubsystem* MusicSys = UAetheriaMusicSubsystem::Get(this))
	{
		MusicSys->NotifyCombatActivity();
	}

	// Perfect dodge i-frames
	if (InvulnTimer > 0.f)
	{
		TriggerCombatFlash(FLinearColor(0.28f, 0.88f, 1.0f), 7000.f, 0.20f);
		return 0.f;
	}

	const FHeroDerivedStats Stats = Customization ? Customization->GetDerivedStats() : FHeroDerivedStats();
	float FinalDamage = DamageAmount * FMath::Clamp(1.0f - Stats.ArmorReduction, 0.15f, 1.0f);

	bool bIsFireDamage = (Cast<AAetheriaBossVolleyOrb>(DamageCauser) != nullptr) || (Cast<AAetheriaDragonActor>(DamageCauser) != nullptr);
	if (const AChronicleSpellActor* SpellActor = Cast<AChronicleSpellActor>(DamageCauser))
	{
		const EChronicleSpellId Sid = SpellActor->GetSpellId();
		bIsFireDamage = (Sid == EChronicleSpellId::SunStrike || Sid == EChronicleSpellId::ChaosMeteor || Sid == EChronicleSpellId::EmberConduit || Sid == EChronicleSpellId::PrimalBlast);
	}
	if (bIsFireDamage && Stats.FireResistance > 0.f)
	{
		FinalDamage *= FMath::Clamp(1.0f - Stats.FireResistance, 0.15f, 1.0f);
	}

	if (bBlocking)
	{
		if (ParryWindowTimer > 0.f)
		{
			// Timed Perfect Parry: zero damage, +22 base Mana (+ skill tree parry bonus), HitStop + staggers attacker!
			ParryWindowTimer = 0.f;
			const float ExtraParryMana = SkillTree ? SkillTree->GetParryManaRestore() : 0.f;
			if (SpellSystem)
			{
				SpellSystem->RestoreMana(22.f + ExtraParryMana);
			}

			TriggerCombatFlash(FLinearColor(0.35f, 0.85f, 1.0f), 14000.f, 0.32f);

			AActor* Attacker = DamageCauser ? DamageCauser : (EventInstigator ? EventInstigator->GetPawn() : nullptr);
			if (UHitStopSubsystem* HitStop = UHitStopSubsystem::Get(this))
			{
				HitStop->RequestHitStop(this, Attacker, 0.070f, 0.05f);
			}

			if (AAetheriaBossCharacter* BossAttacker = Cast<AAetheriaBossCharacter>(Attacker))
			{
				const float BonusSec = SkillTree ? SkillTree->GetStaggerWindowBonusSeconds() : 0.f;
				const float ParryBurstMult = BossAttacker->IsCurrentAttackParryable() ? 2.5f : 1.0f;
				BossAttacker->AddStagger(40.f * ParryBurstMult * Stats.ParryStaggerBonusMult, BonusSec);
			}
			else if (AAetheriaEnemyCharacter* EnemyAttacker = Cast<AAetheriaEnemyCharacter>(Attacker))
			{
				EnemyAttacker->ApplyStagger(1.5f);
			}

			UE_LOG(LogChronicles, Log, TEXT("Hero executed a Timed Parry! Restored %.0f Mana and staggered attacker."), 22.f + ExtraParryMana);
			return 0.f;
		}

		// Standard Block: 80% base mitigation + Quas Glacial Aegis bonus (clamped 95%)
		const float ExtraBlock = SkillTree ? SkillTree->GetShieldMitigationBonus() : 0.f;
		const float Mitigation = FMath::Clamp(0.80f + ExtraBlock, 0.80f, 0.95f);
		FinalDamage *= (1.f - Mitigation);
		Stamina = FMath::Max(0.f, Stamina - 12.f * FMath::Clamp(Stats.BlockStaminaDrainMult, 0.2f, 2.0f));
		TriggerCombatFlash(FLinearColor(0.45f, 0.72f, 1.0f), 5500.f, 0.18f);
	}
	else
	{
		TriggerCombatFlash(FLinearColor(1.0f, 0.14f, 0.10f), 7200.f, 0.25f);
		PlayCombatAnimSequence(AnimHitReact, 1.0f);
	}

	const float Applied = Super::TakeDamage(FinalDamage, DamageEvent, EventInstigator, DamageCauser);
	HP = FMath::Max(0.f, HP - Applied);

	if (HP <= 0.f && !bDead)
	{
		bDead = true;
		bBlocking = false;
		RollTimer = 0.f;
		InvulnTimer = 0.f;
		PlayCombatAnimSequence(AnimDeath, 1.0f);

		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->DisableMovement();
		}

		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQuestSubsystem* QuestSys = GI->GetSubsystem<UQuestSubsystem>())
			{
				QuestSys->PushNotification(TEXT("You have fallen — press Respawn to return to Oakhaven Sanctuary."));
			}
		}
	}

	return Applied;
}

void AAetheriaHeroCharacter::RespawnAtOakhaven()
{
	if (!HasAuthority())
	{
		ServerRespawnAtOakhaven();
		return;
	}

	ExecuteRespawn_Internal();
}

void AAetheriaHeroCharacter::ServerRespawnAtOakhaven_Implementation()
{
	ExecuteRespawn_Internal();
}

void AAetheriaHeroCharacter::ExecuteRespawn_Internal()
{
	SyncSkillTreeMultipliers();

	bDead = false;
	bBlocking = false;
	ComboIndex = 0;
	RollTimer = 0.f;
	ParryWindowTimer = 0.f;
	MeleeCooldownTimer = 0.f;
	InvulnTimer = 1.5f;

	HP = MaxHP;
	Stamina = MaxStamina;
	if (SpellSystem)
	{
		SpellSystem->Mana = SpellSystem->MaxMana;
	}

	// Ground safely at Oakhaven Sanctuary via ECC_WorldStatic
	TArray<const AActor*> Ignored;
	Ignored.Add(this);
	float GroundZ = OakhavenSpawnLocation.Z;
	AetheriaActorsUtil::TraceGroundZ(GetWorld(), OakhavenSpawnLocation.X, OakhavenSpawnLocation.Y, 20000.f, -10000.f, Ignored, GroundZ);

	const float HalfH = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 92.f;
	const FVector SafeTeleportLoc(OakhavenSpawnLocation.X, OakhavenSpawnLocation.Y, GroundZ + HalfH + 14.f);

	SetActorLocation(SafeTeleportLoc, false, nullptr, ETeleportType::TeleportPhysics);

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	CombatAnimLockUntilTime = 0.f;
	bPlayingRunAnim = false;
	bPlayingCombatOneShot = false;
	if (ActiveIdleAnimSeq)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			MeshComp->PlayAnimation(ActiveIdleAnimSeq, true);
			MeshComp->SetPlayRate(1.0f);
		}
	}

	// Cleanly reset any alive Malakor boss encounter on player respawn
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AAetheriaBossCharacter> It(World); It; ++It)
		{
			if (AAetheriaBossCharacter* Boss = *It)
			{
				if (!Boss->bDead)
				{
					Boss->ResetBossEncounter();
				}
			}
		}
	}

	TriggerCombatFlash(FLinearColor(1.0f, 0.85f, 0.42f), 11000.f, 0.45f);
	UE_LOG(LogChronicles, Log, TEXT("Hero respawned cleanly at Oakhaven Sanctuary (Z=%.1f)."), SafeTeleportLoc.Z);
}

void AAetheriaHeroCharacter::HandleCustomizationChanged()
{
	RefreshAllHeroStats();
}

void AAetheriaHeroCharacter::RefreshAllHeroStats()
{
	if (Customization)
	{
		bool bIsNight = false;
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AAetheriaWorldBuilder> It(World); It; ++It)
			{
				if (const AAetheriaWorldBuilder* WorldBuilder = *It)
				{
					bIsNight = (WorldBuilder->TimeOfDay >= 0.84f || WorldBuilder->TimeOfDay < 0.18f);
					break;
				}
			}
		}

		const float HPRatio = (MaxHP > KINDA_SMALL_NUMBER) ? FMath::Clamp(HP / MaxHP, 0.f, 1.f) : 1.f;
		const int32 HeroLevel = SkillTree ? SkillTree->Level : 1;
		Customization->CachedDerivedStats = UHeroCustomizationComponent::ComputeDerivedStats(
			Customization->Profile,
			HeroLevel,
			bIsNight,
			HPRatio);
	}

	SyncSkillTreeMultipliers();
}

void AAetheriaHeroCharacter::SyncSkillTreeMultipliers()
{
	const FHeroDerivedStats Stats = Customization ? Customization->GetDerivedStats() : FHeroDerivedStats();
	const float SkillTreeHPBonus = SkillTree ? SkillTree->GetMaxHPBonus() : 0.f;
	const float SkillTreeStamBonus = SkillTree ? SkillTree->GetMaxStaminaBonus() : 0.f;
	const float SkillTreeMoveMult = SkillTree ? SkillTree->GetMoveSpeedMultiplier() : 1.f;
	const float SkillTreeManaBonus = SkillTree ? SkillTree->GetMaxManaBonus() : 0.f;
	const float SkillTreeExortMult = SkillTree ? SkillTree->GetExortDamageMultiplier() : 1.f;
	const float SkillTreeWexCDMult = SkillTree ? SkillTree->GetWexCooldownMultiplier() : 1.f;
	const float SkillTreeManaRegenMult = SkillTree ? SkillTree->GetManaRegenMultiplier() : 1.f;

	const float NewMaxHP = Stats.MaxHP + SkillTreeHPBonus;
	if (NewMaxHP > MaxHP + KINDA_SMALL_NUMBER && !bDead)
	{
		const float OldRatio = (MaxHP > KINDA_SMALL_NUMBER) ? FMath::Clamp(HP / MaxHP, 0.f, 1.f) : 1.f;
		HP = FMath::Min(NewMaxHP, OldRatio * NewMaxHP);
	}
	MaxHP = NewMaxHP;
	HP = FMath::Clamp(HP, 0.f, MaxHP);

	MaxStamina = Stats.MaxStamina + SkillTreeStamBonus;
	Stamina = FMath::Clamp(Stamina, 0.f, MaxStamina);

	BaseWalkSpeed = Stats.MoveSpeedCmPerSec * SkillTreeMoveMult;

	int32 ActiveWexOrbs = 0;
	if (SpellSystem)
	{
		const float NewMaxMana = Stats.MaxMana + SkillTreeManaBonus;
		if (NewMaxMana > SpellSystem->MaxMana + KINDA_SMALL_NUMBER)
		{
			const float OldManaRatio = (SpellSystem->MaxMana > KINDA_SMALL_NUMBER) ? FMath::Clamp(SpellSystem->Mana / SpellSystem->MaxMana, 0.f, 1.f) : 1.f;
			SpellSystem->Mana = FMath::Min(NewMaxMana, OldManaRatio * NewMaxMana);
		}
		SpellSystem->MaxMana = NewMaxMana;
		SpellSystem->Mana = FMath::Clamp(SpellSystem->Mana, 0.f, SpellSystem->MaxMana);
		SpellSystem->ExortPowerScale = Stats.SpellPowerMultiplier * Stats.ExortBonusMultiplier * SkillTreeExortMult;
		SpellSystem->WexSpeedScale = 1.f / FMath::Max(0.35f, SkillTreeWexCDMult);
		SpellSystem->QuasControlScale = SkillTree ? SkillTree->GetQuasControlMultiplier() : 1.f;
		SpellSystem->bSpellCastingBlockedByStature = !Stats.bCanCastSpells;
		SpellSystem->ManaCostMultiplier = Stats.ManaCostMultiplier;
		SpellSystem->SpellCooldownMultiplier = Stats.SpellCooldownMultiplier * SkillTreeWexCDMult;
		SpellSystem->ManaRegenPerSec = Stats.ManaRegenPerSec * SkillTreeManaRegenMult;

		for (EChronicleOrb Orb : SpellSystem->ActiveOrbs)
		{
			if (Orb == EChronicleOrb::Wex)
			{
				++ActiveWexOrbs;
			}
		}
	}

	// Delta-sync MaxWalkSpeed so external buffs (StormAlacrity) are never overwritten,
	// while driving TurnRateDegPerSec & AccelMultiplier directly from Hero Stature/Physique!
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->RotationRate = FRotator(0.f, Stats.TurnRateDegPerSec, 0.f);
		MoveComp->MaxAcceleration = 2048.f * Stats.AccelMultiplier;
		MoveComp->BrakingDecelerationWalking = 2100.f * Stats.AccelMultiplier;

		const float BlockScale = bBlocking ? 0.55f : 1.0f;
		const float WexBonus = 1.0f + 0.045f * static_cast<float>(ActiveWexOrbs);
		const float DesiredBaseSpeed = BaseWalkSpeed * BlockScale * WexBonus;
		const float Delta = DesiredBaseSpeed - LastSyncedWalkSpeed;
		if (!FMath::IsNearlyZero(Delta))
		{
			MoveComp->MaxWalkSpeed = FMath::Max(160.f, MoveComp->MaxWalkSpeed + Delta);
			LastSyncedWalkSpeed = DesiredBaseSpeed;
		}
	}
}

void AAetheriaHeroCharacter::UpdateOrbitingOrbs(float DeltaTime)
{
	OrbOrbitAngle += DeltaTime * 2.15f;

	const int32 ActiveCount = SpellSystem ? SpellSystem->ActiveOrbs.Num() : 0;

	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* OrbMesh = OrbitingOrbMeshes[i];
		if (!OrbMesh)
		{
			continue;
		}

		const bool bShow = (!bCreatorStudioCameraActive) && (!bDead) && (i < ActiveCount) && (SpellSystem->ActiveOrbs[i] != EChronicleOrb::None);
		OrbMesh->SetVisibility(bShow);
		if (!bShow)
		{
			continue;
		}

		const float Angle = OrbOrbitAngle + (static_cast<float>(i) * (2.f * PI / 3.f));
		const float OrbitRadius = 56.f;
		const float BobZ = FMath::Sin(OrbOrbitAngle * 1.9f + static_cast<float>(i) * 1.4f) * 8.f;
		// Gracefully hover behind & above hero shoulders in local component space
		const FVector LocalOffset(
			-34.f + FMath::Cos(Angle) * (OrbitRadius * 0.45f),
			FMath::Sin(Angle) * OrbitRadius,
			58.f + BobZ + FMath::Cos(Angle * 0.5f) * 10.f);

		OrbMesh->SetRelativeLocation(LocalOffset);

		if (UMaterialInstanceDynamic* MID = OrbitingOrbMIDs[i])
		{
			const FLinearColor BaseCol = AetheriaActorsUtil::GetOrbVisualColor(SpellSystem->ActiveOrbs[i]);
			if (OrbitingOrbMatMode == 1)
			{
				// M_AetheriaEmissiveCore: EmissiveColor * EmissiveBoost -> rich jewel orb at 2.2x
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), BaseCol);
				MID->SetScalarParameterValue(TEXT("EmissiveBoost"), 2.2f);
				MID->SetScalarParameterValue(TEXT("Opacity"), 0.92f);
			}
			else if (OrbitingOrbMatMode == 2)
			{
				// M_AetheriaFireball: churning elemental core at 2.2x
				MID->SetVectorParameterValue(TEXT("BaseColor"), BaseCol);
				MID->SetVectorParameterValue(TEXT("AccentColor"), BaseCol * 1.10f);
				MID->SetScalarParameterValue(TEXT("EmissiveBoost"), 2.2f);
			}
			else
			{
				const FLinearColor EmissiveCol = BaseCol * 2.2f;
				MID->SetVectorParameterValue(TEXT("Color"), EmissiveCol);
				MID->SetVectorParameterValue(TEXT("BaseColor"), BaseCol);
				MID->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveCol);
			}
		}
	}
}

void AAetheriaHeroCharacter::HandlePopulateSaveGame(UChroniclesSaveGame* SaveObj)
{
	if (!SaveObj)
	{
		return;
	}

	SaveObj->LastPlayerLocation = GetActorLocation();
	SaveObj->bUnlockedDragonMount = bDragonUnlocked;
	if (SpellSystem)
	{
		SaveObj->bUnlockedExort = SpellSystem->bUnlockedExort;
		SaveObj->bUnlockedWex = SpellSystem->bUnlockedWex;
		SaveObj->bUnlockedQuas = SpellSystem->bUnlockedQuas;
	}
	if (Customization)
	{
		Customization->PopulateSaveGame(SaveObj);
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFogOfWarMapSubsystem* MapSys = GI->GetSubsystem<UFogOfWarMapSubsystem>())
		{
			SaveObj->FogOfWarGrid = MapSys->GetRawFogGrid();
		}
	}
}

void AAetheriaHeroCharacter::HandleApplySaveGame(UChroniclesSaveGame* SaveObj)
{
	if (!SaveObj)
	{
		return;
	}

	bDragonUnlocked = SaveObj->bUnlockedDragonMount;
	if (SpellSystem)
	{
		if (SaveObj->bUnlockedExort) { SpellSystem->UnlockOrb(EChronicleOrb::Exort); }
		if (SaveObj->bUnlockedWex)   { SpellSystem->UnlockOrb(EChronicleOrb::Wex); }
		if (SaveObj->bUnlockedQuas)  { SpellSystem->UnlockOrb(EChronicleOrb::Quas); }
	}
	if (Customization)
	{
		Customization->ApplySaveGame(SaveObj);
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFogOfWarMapSubsystem* MapSys = GI->GetSubsystem<UFogOfWarMapSubsystem>())
		{
			if (SaveObj->FogOfWarGrid.Num() == UChroniclesSaveGame::FogOfWarGridSize)
			{
				MapSys->LoadRawFogGrid(SaveObj->FogOfWarGrid);
			}
		}
	}
	if (SkillTree)
	{
		LastObservedHeroLevel = SkillTree->Level;
	}
	RefreshAllHeroStats();
}

void AAetheriaHeroCharacter::HandleOrbUnlocked(EChronicleOrb UnlockedOrb)
{
	UNiagaraSystem* AuraNS = nullptr;
	switch (UnlockedOrb)
	{
	case EChronicleOrb::Exort: AuraNS = OrbUnlockAuraFireNS.Get(); break;
	case EChronicleOrb::Wex:   AuraNS = OrbUnlockAuraLightningNS.Get(); break;
	case EChronicleOrb::Quas:  AuraNS = OrbUnlockAuraSoulNS.Get(); break;
	default: break;
	}

	if (AuraNS && !IsRunningDedicatedServer())
	{
		UNiagaraComponent* SpawnedBurst = nullptr;
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			SpawnedBurst = UNiagaraFunctionLibrary::SpawnSystemAttached(
				AuraNS,
				MeshComp,
				NAME_None,
				FVector(0.f, 0.f, 10.f),
				FRotator::ZeroRotator,
				FVector(0.55f),
				EAttachLocation::KeepRelativeOffset,
				true,
				ENCPoolMethod::None);
		}
		else
		{
			SpawnedBurst = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, AuraNS, GetActorLocation() - FVector(0.f, 0.f, 80.f), FRotator::ZeroRotator, FVector(0.55f), true);
		}

		// Free_Spells_Aura_* systems loop indefinitely — deactivate cleanly after 1.35s so they celebrate unlock without blinding the camera!
		if (SpawnedBurst)
		{
			if (UWorld* World = GetWorld())
			{
				TWeakObjectPtr<UNiagaraComponent> WeakBurst(SpawnedBurst);
				FTimerHandle BurstStopHandle;
				World->GetTimerManager().SetTimer(
					BurstStopHandle,
					FTimerDelegate::CreateLambda([WeakBurst]()
					{
						if (UNiagaraComponent* Comp = WeakBurst.Get())
						{
							Comp->Deactivate();
							Comp->DestroyComponent();
						}
					}),
					1.35f,
					false);
			}
		}
	}

	if (MagicBuffSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, MagicBuffSFX, GetActorLocation());
	}

	TriggerCombatFlash(AetheriaActorsUtil::GetOrbVisualColor(UnlockedOrb), 12000.f, 0.40f);
}

void AAetheriaHeroCharacter::HandleSkillTreeChanged()
{
	if (SkillTree && SkillTree->Level > LastObservedHeroLevel)
	{
		LastObservedHeroLevel = SkillTree->Level;
		SpawnLevelUpSpectacle();
	}
	RefreshAllHeroStats();
}

void AAetheriaHeroCharacter::SpawnLevelUpSpectacle()
{
	if (HeroLevelUpNS)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				HeroLevelUpNS,
				MeshComp,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true);
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HeroLevelUpNS, GetActorLocation(), FRotator::ZeroRotator, FVector::OneVector, true);
		}
	}

	if (MagicBuffSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, MagicBuffSFX, GetActorLocation());
	}

	TriggerCombatFlash(FLinearColor(1.0f, 0.85f, 0.30f), 14000.f, 0.45f);
}

void AAetheriaHeroCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SkillTree && SkillTree->Level > LastObservedHeroLevel)
	{
		LastObservedHeroLevel = SkillTree->Level;
		SpawnLevelUpSpectacle();
	}

	// Timers
	if (RollTimer > 0.f)          { RollTimer = FMath::Max(0.f, RollTimer - DeltaTime); }
	if (InvulnTimer > 0.f)        { InvulnTimer = FMath::Max(0.f, InvulnTimer - DeltaTime); }
	if (ParryWindowTimer > 0.f)   { ParryWindowTimer = FMath::Max(0.f, ParryWindowTimer - DeltaTime); }
	if (MeleeCooldownTimer > 0.f) { MeleeCooldownTimer = FMath::Max(0.f, MeleeCooldownTimer - DeltaTime); }
	if (ComboResetTimer > 0.f)    { ComboResetTimer = FMath::Max(0.f, ComboResetTimer - DeltaTime); }

	// Decay transient combat flash light cleanly
	if (FlashTimer > 0.f && CombatFlashLight)
	{
		FlashTimer = FMath::Max(0.f, FlashTimer - DeltaTime);
		const float Alpha = (FlashDuration > KINDA_SMALL_NUMBER) ? (FlashTimer / FlashDuration) : 0.f;
		CombatFlashLight->SetIntensity(FlashPeakLumens * Alpha * Alpha);
	}

	// Refresh dynamic conditional stats (bBerserkLowHPActive, bNightShadowCritActive) every 0.5s without touching meshes
	LastDynamicRefreshTimer += DeltaTime;
	if (LastDynamicRefreshTimer >= 0.5f)
	{
		LastDynamicRefreshTimer = 0.f;
		RefreshAllHeroStats();
	}
	else
	{
		SyncSkillTreeMultipliers();
	}

	const FHeroDerivedStats Stats = Customization ? Customization->GetDerivedStats() : FHeroDerivedStats();
	const float HeightScale = Customization
		? (Customization->Profile.HeightMeters / UHeroCustomizationComponent::BASE_HEIGHT_METERS)
		: 1.0f;

	if (!bDead)
	{
		// Stamina regeneration when not rolling or blocking (driven by Stats.StaminaRegenPerSec * SkillTree)
		if (RollTimer <= 0.f && !bBlocking)
		{
			const float StamRegenMult = SkillTree ? SkillTree->GetStaminaRegenMultiplier() : 1.f;
			Stamina = FMath::Min(MaxStamina, Stamina + Stats.StaminaRegenPerSec * StamRegenMult * DeltaTime);
		}

		// Quas primal orb, SkillTree & Customization passive HP regeneration
		int32 ActiveQuasOrbs = 0;
		if (SpellSystem)
		{
			for (EChronicleOrb Orb : SpellSystem->ActiveOrbs)
			{
				if (Orb == EChronicleOrb::Quas)
				{
					++ActiveQuasOrbs;
				}
			}
		}
		const float TotalHPRegenPerSec = Stats.HPRegenPerSec
			+ (SkillTree ? SkillTree->GetHPRegenBonus() : 0.f)
			+ (static_cast<float>(ActiveQuasOrbs) * 1.4f);
		if (TotalHPRegenPerSec > 0.f && HP < MaxHP)
		{
			HP = FMath::Min(MaxHP, HP + TotalHPRegenPerSec * DeltaTime);
		}
	}

	// SingleNode Paragon locomotion state machine (Idle <-> Jog_Fwd; bypasses broken Paragon *_AnimBlueprint A-pose!)
	if (ActiveIdleAnimSeq && !bDead)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			const bool bIsMountedOnDragon = (Cast<AAetheriaDragonActor>(GetAttachParentActor()) != nullptr);
			const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
			if (!bIsMountedOnDragon && WorldTime >= CombatAnimLockUntilTime)
			{
				const bool bShouldRun = (!bCreatorStudioCameraActive) && (GetVelocity().Size2D() > 45.f) && (ActiveRunAnimSeq != nullptr);
				if (bShouldRun)
				{
					if (!bPlayingRunAnim || bPlayingCombatOneShot || !MeshComp->IsPlaying())
					{
						MeshComp->PlayAnimation(ActiveRunAnimSeq, true);
						MeshComp->SetPlayRate(1.0f);
						bPlayingRunAnim = true;
						bPlayingCombatOneShot = false;
					}
				}
				else
				{
					if (bPlayingRunAnim || bPlayingCombatOneShot || !MeshComp->IsPlaying())
					{
						MeshComp->PlayAnimation(ActiveIdleAnimSeq, true);
						MeshComp->SetPlayRate(1.0f);
						bPlayingRunAnim = false;
						bPlayingCombatOneShot = false;
					}
				}
			}
			else if (bIsMountedOnDragon && (bPlayingRunAnim || bPlayingCombatOneShot || !MeshComp->IsPlaying()))
			{
				MeshComp->PlayAnimation(ActiveIdleAnimSeq, true);
				MeshComp->SetPlayRate(1.0f);
				bPlayingRunAnim = false;
				bPlayingCombatOneShot = false;
			}
		}
	}

	// Smoothly frame Creator Studio portrait camera (54 deg lens + 3-point lighting) or Crimson Desert gameplay camera (78 deg)
	if (CameraBoom)
	{
		if (bCreatorStudioCameraActive)
		{
			// Calibrated for 54 deg portrait lens so full-body / torso / face portraits frame cleanly
			float StudioArm = 440.f * HeightScale;
			float StudioZ = 12.f * HeightScale;
			if (CreatorStudioZoomMode == 1)
			{
				StudioArm = 245.f * HeightScale;
				StudioZ = 48.f * HeightScale;
			}
			else if (CreatorStudioZoomMode == 2)
			{
				StudioArm = 135.f * HeightScale;
				StudioZ = 68.f * HeightScale;
			}

			CameraBoom->bUsePawnControlRotation = false;
			CameraBoom->bDoCollisionTest = false;
			CameraBoom->bEnableCameraLag = false;
			CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, StudioArm, DeltaTime, 10.f);
			CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, FVector(0.f, 0.f, StudioZ), DeltaTime, 10.f);
			const FRotator TargetRelRot(-8.f, CreatorStudioOrbitYaw, 0.f);
			CameraBoom->SetRelativeRotation(FMath::RInterpTo(CameraBoom->GetRelativeRotation(), TargetRelRot, DeltaTime, 10.f));

			if (FollowCamera && !FMath::IsNearlyEqual(FollowCamera->FieldOfView, 54.0f, 0.1f))
			{
				FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, 54.0f, DeltaTime, 12.f));
			}

			// Calibrated 3-point Crimson Desert studio lighting: warm 3/4 key (+45 deg side, +25 deg above) + cool silhouette rim
			const float CamYaw = CreatorStudioOrbitYaw + 180.f;
			if (CreatorStudioFillLight)
			{
				const FVector KeyDir = FRotator(25.f, CamYaw + 45.f, 0.f).Vector();
				CreatorStudioFillLight->SetRelativeLocation(KeyDir * (175.f * HeightScale) + FVector(0.f, 0.f, 45.f * HeightScale));
				CreatorStudioFillLight->SetIntensity(145.0f);
				CreatorStudioFillLight->SetVisibility(true);
			}
			if (CreatorStudioRimLight)
			{
				const FVector RimDir = FRotator(28.f, CreatorStudioOrbitYaw - 35.f, 0.f).Vector();
				CreatorStudioRimLight->SetRelativeLocation(RimDir * (165.f * HeightScale) + FVector(0.f, 0.f, 55.f * HeightScale));
				CreatorStudioRimLight->SetIntensity(95.0f);
				CreatorStudioRimLight->SetVisibility(true);
			}
		}
		else
		{
			CameraBoom->bUsePawnControlRotation = true;
			CameraBoom->bDoCollisionTest = true;
			CameraBoom->bEnableCameraLag = true;
			CameraBoom->CameraLagSpeed = 12.0f;
			CameraBoom->TargetArmLength = FMath::FInterpTo(
				CameraBoom->TargetArmLength,
				470.f * FMath::Clamp(HeightScale, 0.85f, 1.45f),
				DeltaTime,
				10.f);
			CameraBoom->SocketOffset = FMath::VInterpTo(
				CameraBoom->SocketOffset,
				FVector(0.f, 48.f, 96.f * HeightScale),
				DeltaTime,
				10.f);

			if (FollowCamera && !FMath::IsNearlyEqual(FollowCamera->FieldOfView, 78.0f, 0.1f))
			{
				FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, 78.0f, DeltaTime, 12.f));
			}

			if (CreatorStudioFillLight)
			{
				CreatorStudioFillLight->SetIntensity(0.f);
				CreatorStudioFillLight->SetVisibility(false);
			}
			if (CreatorStudioRimLight)
			{
				CreatorStudioRimLight->SetIntensity(0.f);
				CreatorStudioRimLight->SetVisibility(false);
			}
		}
	}

	// Smoothly orbit Exort/Wex/Quas 3D spheres behind shoulders
	UpdateOrbitingOrbs(DeltaTime);

	// Ground exploration Fog-of-War reveal (5,800 UU * MapRevealMultiplier)
	MapRevealCooldown -= DeltaTime;
	if (MapRevealCooldown <= 0.f)
	{
		MapRevealCooldown = 0.25f;
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFogOfWarMapSubsystem* MapSys = GI->GetSubsystem<UFogOfWarMapSubsystem>())
			{
				MapSys->RevealAroundWorldLocation(GetActorLocation(), 5800.f * Stats.MapRevealMultiplier);
			}
		}
	}
}

// ============================================================================
// 3A. AAetheriaEnemyCharacter — Elemental-Reaction Biome Enemies
// ============================================================================

AAetheriaEnemyCharacter::AAetheriaEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 420.f, 0.f);
		MoveComp->MaxWalkSpeed = BaseMoveSpeed;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(44.f, 92.f);
		Capsule->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Block);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -92.f));
		MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}

	// Physical AgX Lumen light for elemental reactions & attack telegraphs
	StatusFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusFlashLight"));
	StatusFlashLight->SetupAttachment(RootComponent);
	StatusFlashLight->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
	StatusFlashLight->SetIntensityUnits(ELightUnits::Lumens);
	StatusFlashLight->SetIntensity(0.f);
	StatusFlashLight->SetAttenuationRadius(1250.f);
	StatusFlashLight->SetCastShadows(false);
}

void AAetheriaEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAetheriaEnemyCharacter, Archetype);
	DOREPLIFETIME(AAetheriaEnemyCharacter, EnemyDisplayName);
	DOREPLIFETIME(AAetheriaEnemyCharacter, HP);
	DOREPLIFETIME(AAetheriaEnemyCharacter, MaxHP);
	DOREPLIFETIME(AAetheriaEnemyCharacter, bDead);
	DOREPLIFETIME(AAetheriaEnemyCharacter, BurnStacks);
	DOREPLIFETIME(AAetheriaEnemyCharacter, bFrozen);
	DOREPLIFETIME(AAetheriaEnemyCharacter, bShocked);
}

void AAetheriaEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	EnemyAttackAnim   = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimAttack1);
	EnemyHitReactAnim = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimHitReact);
	EnemyDeathAnim    = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimDeath);

	if (!bArchetypeConfigured)
	{
		ConfigureArchetype(Archetype, QuestIdOnDeath);
	}
	else
	{
		BindEnemyMeshAndAnims();
	}

	FAetheriaStyleHarmonizer::HarmonizeActorMaterials(this, EChronicleStyleCategory::Creature);
}

void AAetheriaEnemyCharacter::ConfigureArchetype(EChronicleEnemyArchetype InArchetype, FName InQuestIdOnDeath)
{
	Archetype = InArchetype;
	if (!InQuestIdOnDeath.IsNone())
	{
		QuestIdOnDeath = InQuestIdOnDeath;
	}
	bArchetypeConfigured = true;

	switch (Archetype)
	{
	case EChronicleEnemyArchetype::Skeleton:
		EnemyDisplayName = NSLOCTEXT("AetheriaEnemies", "Skeleton", "Risen Skeleton");
		MaxHP = 110.f;
		AttackDamage = 13.f;
		XPReward = 65;
		GoldReward = 32;
		BaseMoveSpeed = 390.f;
		SetActorScale3D(FVector(1.0f));
		break;

	case EChronicleEnemyArchetype::ForestTroll:
		EnemyDisplayName = NSLOCTEXT("AetheriaEnemies", "ForestTroll", "Highland Forest Troll");
		MaxHP = 230.f;
		AttackDamage = 22.f;
		XPReward = 115;
		GoldReward = 65;
		BaseMoveSpeed = 325.f;
		SetActorScale3D(FVector(1.22f));
		break;

	case EChronicleEnemyArchetype::TempestStalker:
		EnemyDisplayName = NSLOCTEXT("AetheriaEnemies", "TempestStalker", "Tempest Stalker");
		MaxHP = 195.f;
		AttackDamage = 19.f;
		XPReward = 155;
		GoldReward = 90;
		BaseMoveSpeed = 450.f;
		SetActorScale3D(FVector(1.10f));
		break;

	case EChronicleEnemyArchetype::FrostWarden:
		EnemyDisplayName = NSLOCTEXT("AetheriaEnemies", "FrostWarden", "Glacier Frost Warden");
		MaxHP = 290.f;
		AttackDamage = 25.f;
		XPReward = 220;
		GoldReward = 130;
		BaseMoveSpeed = 355.f;
		SetActorScale3D(FVector(1.28f));
		break;
	}

	HP = MaxHP;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = BaseMoveSpeed;
	}

	BindEnemyMeshAndAnims();
	if (HasActorBegunPlay())
	{
		FAetheriaStyleHarmonizer::HarmonizeActorMaterials(this, EChronicleStyleCategory::Creature);
	}
}

void AAetheriaEnemyCharacter::BindEnemyMeshAndAnims()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// Cleanly reset skeletal animation caches before binding the active archetype
	bUsingSingleNodeAnims = false;
	bDeathAnimPlayed = false;
	OneShotAnimTimer = 0.f;
	EnemyAttackAnimCycle = 0;
	CurrentEnemyAnim = nullptr;
	EnemyIdleAnim = nullptr;
	EnemyRunAnim = nullptr;
	EnemyAttackAnims[0] = nullptr;
	EnemyAttackAnims[1] = nullptr;
	EnemyAttackAnims[2] = nullptr;
	EnemyAttackAnim = nullptr;
	EnemyHitReactAnim = nullptr;
	EnemyDeathAnim = nullptr;
	EnemyAttackMontages.Reset();
	EnemyAnimSlotName = FName(TEXT("DefaultSlot"));

	// 1. TempestStalker & Skeleton (Crypt Wraith): prefer REAL Fab SK_Whisper + 7 SingleNode clips
	if (Archetype == EChronicleEnemyArchetype::TempestStalker || Archetype == EChronicleEnemyArchetype::Skeleton)
	{
		if (USkeletalMesh* WhisperMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathWhisperMesh))
		{
			MeshComp->SetSkeletalMesh(WhisperMesh);
			MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);

			EnemyIdleAnim       = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathWhisperIdle);
			EnemyRunAnim        = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathWhisperRun);
			EnemyAttackAnims[0] = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathWhisperAttack1);
			EnemyAttackAnims[1] = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathWhisperAttack2);
			EnemyAttackAnims[2] = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathWhisperAttack3);
			EnemyAttackAnim     = EnemyAttackAnims[0];
			EnemyHitReactAnim   = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathWhisperHitReact);
			EnemyDeathAnim      = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathWhisperDeath);
			bUsingSingleNodeAnims = true;

			if (EnemyIdleAnim)
			{
				PlayEnemySingleNodeAnim(EnemyIdleAnim, true, 1.0f, true);
			}

			UE_LOG(LogChronicles, Log, TEXT("AAetheriaEnemyCharacter (%s) bound Whisper SkeletalMesh & SingleNode animations."), *EnemyDisplayName.ToString());
			return;
		}
	}

	// 2. ForestTroll: prefer REAL Paragon Rampage + Rampage_AnimBlueprint + Attack_Biped_Melee_A/B/C montages
	if (Archetype == EChronicleEnemyArchetype::ForestTroll)
	{
		if (USkeletalMesh* RampageMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathRampageMesh))
		{
			MeshComp->SetSkeletalMesh(RampageMesh);
			BindRampageAnimsAndMontages(MeshComp);
			UE_LOG(LogChronicles, Log, TEXT("AAetheriaEnemyCharacter (%s) bound Paragon Rampage SkeletalMesh & %d Montages."),
				*EnemyDisplayName.ToString(), EnemyAttackMontages.Num());
			return;
		}
	}

	// 2B. FrostWarden: prefer REAL Paragon Khaimera + Khaimera_AnimBlueprint + Melee_A/B/C montages (scale 1.18f)
	if (Archetype == EChronicleEnemyArchetype::FrostWarden)
	{
		if (USkeletalMesh* KhaimeraMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathKhaimeraMesh))
		{
			SetActorScale3D(FVector(1.18f));
			MeshComp->SetSkeletalMesh(KhaimeraMesh);
			BindParagonEnemyAnimsAndMontages(
				MeshComp,
				AetheriaActorsUtil::PathKhaimeraABP,
				AetheriaActorsUtil::PathKhaimeraMeleeA,
				AetheriaActorsUtil::PathKhaimeraMeleeB,
				AetheriaActorsUtil::PathKhaimeraMeleeC,
				AetheriaActorsUtil::PathKhaimeraHitReact,
				AetheriaActorsUtil::PathKhaimeraDeath,
				FName(TEXT("FullBody")));
			UE_LOG(LogChronicles, Log, TEXT("AAetheriaEnemyCharacter (%s) bound Paragon Khaimera SkeletalMesh (scale 1.18) & %d Montages."),
				*EnemyDisplayName.ToString(), EnemyAttackMontages.Num());
			return;
		}
	}

	// 3. Fallback to staged SKM_Mob_* -> SKM_Manny_Simple + Manny Combat/Unarmed ABP
	const TCHAR* PrimaryPath = AetheriaActorsUtil::PathMobSwarm;
	switch (Archetype)
	{
	case EChronicleEnemyArchetype::Skeleton:       PrimaryPath = AetheriaActorsUtil::PathMobSwarm; break;
	case EChronicleEnemyArchetype::ForestTroll:    PrimaryPath = AetheriaActorsUtil::PathMobBruiser; break;
	case EChronicleEnemyArchetype::TempestStalker: PrimaryPath = AetheriaActorsUtil::PathMobRanged; break;
	case EChronicleEnemyArchetype::FrostWarden:    PrimaryPath = AetheriaActorsUtil::PathMobBruiser; break;
	}

	USkeletalMesh* LoadedMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(PrimaryPath);
	if (!LoadedMesh)
	{
		LoadedMesh = LoadObject<USkeletalMesh>(nullptr, AetheriaActorsUtil::PathMannySimple);
	}

	if (LoadedMesh)
	{
		MeshComp->SetSkeletalMesh(LoadedMesh);
		AetheriaActorsUtil::TryBindCompatibleAnimBP(MeshComp, AetheriaActorsUtil::PathABPCombat, AetheriaActorsUtil::PathABPUnarmed);
		EnemyAttackAnim   = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimAttack1);
		EnemyHitReactAnim = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimHitReact);
		EnemyDeathAnim    = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimDeath);
	}
}

bool AAetheriaEnemyCharacter::BindRampageAnimsAndMontages(USkeletalMeshComponent* MeshComp)
{
	return BindParagonEnemyAnimsAndMontages(
		MeshComp,
		AetheriaActorsUtil::PathRampageABP,
		AetheriaActorsUtil::PathRampageMeleeA,
		AetheriaActorsUtil::PathRampageMeleeB,
		AetheriaActorsUtil::PathRampageMeleeC,
		AetheriaActorsUtil::PathRampageHitReact,
		AetheriaActorsUtil::PathRampageDeath,
		FName(TEXT("FullBody")));
}

bool AAetheriaEnemyCharacter::BindParagonEnemyAnimsAndMontages(
	USkeletalMeshComponent* MeshComp,
	const TCHAR* ABPPath,
	const TCHAR* MeleeAPath,
	const TCHAR* MeleeBPath,
	const TCHAR* MeleeCPath,
	const TCHAR* HitReactPath,
	const TCHAR* DeathPath,
	FName SlotName)
{
	if (!MeshComp)
	{
		return false;
	}

	USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset();
	if (!SkelMesh || !SkelMesh->GetSkeleton())
	{
		return false;
	}

	const bool bBoundABP = AetheriaActorsUtil::TryBindCompatibleAnimBP(MeshComp, ABPPath, nullptr);
	EnemyAnimSlotName = SlotName;

	EnemyAttackMontages.Reset();
	const TCHAR* MontagePaths[3] = { MeleeAPath, MeleeBPath, MeleeCPath };
	for (const TCHAR* MontagePath : MontagePaths)
	{
		if (UAnimMontage* Montage = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimMontage>(MontagePath))
		{
			if (AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, Montage->GetSkeleton()))
			{
				EnemyAttackMontages.Add(Montage);
			}
		}
	}

	if (UAnimSequence* HitSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(HitReactPath))
	{
		if (AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, HitSeq->GetSkeleton()))
		{
			EnemyHitReactAnim = HitSeq;
		}
	}
	if (UAnimSequence* DeathSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(DeathPath))
	{
		if (AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, DeathSeq->GetSkeleton()))
		{
			EnemyDeathAnim = DeathSeq;
		}
	}

	return bBoundABP || (EnemyAttackMontages.Num() > 0);
}

void AAetheriaEnemyCharacter::PlayEnemySingleNodeAnim(UAnimSequence* Seq, bool bLooping, float PlayRate, bool bForceRestart)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !Seq)
	{
		return;
	}

	USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset();
	if (!SkelMesh || !Seq->GetSkeleton() || !AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, Seq->GetSkeleton()))
	{
		return;
	}

	if (!bForceRestart && CurrentEnemyAnim.Get() == Seq)
	{
		return;
	}

	const float SafeRate = FMath::Max(0.15f, PlayRate);
	MeshComp->PlayAnimation(Seq, bLooping);
	MeshComp->SetPlayRate(SafeRate);
	CurrentEnemyAnim = Seq;

	if (!bLooping)
	{
		OneShotAnimTimer = FMath::Clamp(Seq->GetPlayLength() / SafeRate, 0.25f, 3.5f);
	}
}

void AAetheriaEnemyCharacter::PlayEnemyMontage(UAnimMontage* Montage, float PlayRate)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !Montage)
	{
		return;
	}

	USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset();
	if (!SkelMesh || !Montage->GetSkeleton() || !AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, Montage->GetSkeleton()))
	{
		return;
	}

	if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
	{
		AnimInst->Montage_Play(Montage, FMath::Max(0.15f, PlayRate));
	}
}

void AAetheriaEnemyCharacter::PlayEnemyAttackAnim(float PlayRate)
{
	if (bDead)
	{
		return;
	}

	if (bUsingSingleNodeAnims)
	{
		TArray<UAnimSequence*> ValidClips;
		for (int32 i = 0; i < 3; ++i)
		{
			if (EnemyAttackAnims[i])
			{
				ValidClips.Add(EnemyAttackAnims[i].Get());
			}
		}
		if (ValidClips.Num() > 0)
		{
			UAnimSequence* ChosenClip = ValidClips[EnemyAttackAnimCycle % ValidClips.Num()];
			++EnemyAttackAnimCycle;
			PlayEnemySingleNodeAnim(ChosenClip, false, PlayRate, true);
			return;
		}
	}

	if (EnemyAttackMontages.Num() > 0)
	{
		UAnimMontage* ChosenMontage = EnemyAttackMontages[EnemyAttackAnimCycle % EnemyAttackMontages.Num()].Get();
		++EnemyAttackAnimCycle;
		PlayEnemyMontage(ChosenMontage, PlayRate);
		return;
	}

	PlayEnemyAnimSequence(EnemyAttackAnim, PlayRate);
}

void AAetheriaEnemyCharacter::PlayEnemyAnimSequence(UAnimSequence* Seq, float PlayRate)
{
	if (!Seq)
	{
		return;
	}

	const bool bIsDeath = (Seq == EnemyDeathAnim.Get());
	if (bDead && !bIsDeath)
	{
		return;
	}

	// Play death as non-looping SingleNode clip so fallen corpses stay collapsed until despawn
	if (bIsDeath)
	{
		if (!bDeathAnimPlayed)
		{
			PlayEnemySingleNodeAnim(Seq, false, PlayRate, true);
			bDeathAnimPlayed = true;
			OneShotAnimTimer = 999.f;
		}
		return;
	}

	if (bUsingSingleNodeAnims)
	{
		PlayEnemySingleNodeAnim(Seq, false, PlayRate, true);
		return;
	}

	AetheriaActorsUtil::PlaySafeDynamicMontage(GetMesh(), Seq, PlayRate, 0.08f, 0.16f, EnemyAnimSlotName);
}

void AAetheriaEnemyCharacter::UpdateEnemySingleNodeAnimation(float DeltaTime)
{
	if (bDead)
	{
		if (!bDeathAnimPlayed && EnemyDeathAnim)
		{
			PlayEnemySingleNodeAnim(EnemyDeathAnim, false, 1.0f, true);
			bDeathAnimPlayed = true;
			OneShotAnimTimer = 999.f;
		}
		return;
	}

	if (!bUsingSingleNodeAnims)
	{
		return;
	}

	if (OneShotAnimTimer > 0.f)
	{
		OneShotAnimTimer = FMath::Max(0.f, OneShotAnimTimer - DeltaTime);
		if (OneShotAnimTimer > 0.f)
		{
			return;
		}
	}

	const float Speed2D = GetVelocity().Size2D();
	UAnimSequence* DesiredLocomotion = (Speed2D > 25.f && EnemyRunAnim) ? EnemyRunAnim.Get() : EnemyIdleAnim.Get();
	if (DesiredLocomotion)
	{
		PlayEnemySingleNodeAnim(DesiredLocomotion, true, 1.0f, false);
	}
}

void AAetheriaEnemyCharacter::TriggerStatusFlash(const FLinearColor& Color, float PeakLumens, float Duration)
{
	if (!StatusFlashLight)
	{
		return;
	}
	StatusFlashPeakLumens = PeakLumens;
	StatusFlashDuration = FMath::Max(0.08f, Duration);
	StatusFlashTimer = StatusFlashDuration;
	StatusFlashLight->SetLightColor(Color);
	StatusFlashLight->SetIntensity(PeakLumens);
}

void AAetheriaEnemyCharacter::ApplyStagger(float Duration)
{
	if (bDead)
	{
		return;
	}
	StaggerTimer = FMath::Max(StaggerTimer, Duration);
	bWindupActive = false;
	PlayEnemyAnimSequence(EnemyHitReactAnim, 0.9f);
	TriggerStatusFlash(FLinearColor(1.0f, 0.86f, 0.35f), 9000.f, 0.35f);
}

void AAetheriaEnemyCharacter::ApplyElementalStatus(EChronicleOrb Element, AActor* InstigatorActor)
{
	if (bDead || Element == EChronicleOrb::None)
	{
		return;
	}

	LastElementInstigator = InstigatorActor;

	float ThermalShockMult = 1.f;
	float SuperconductMult = 1.f;
	float ArmorMeltBonus = 0.f;
	float QuasControlMult = 1.f;
	AController* InstigatorCtrl = nullptr;

	if (AAetheriaHeroCharacter* Hero = AetheriaActorsUtil::ResolveHeroFromInstigator(nullptr, InstigatorActor))
	{
		InstigatorCtrl = Hero->GetController();
		if (Hero->SkillTree)
		{
			ThermalShockMult = Hero->SkillTree->GetThermalShockMultiplier();
			SuperconductMult = Hero->SkillTree->GetSuperconductMultiplier();
			ArmorMeltBonus = Hero->SkillTree->GetArmorMeltBonus();
			QuasControlMult = Hero->SkillTree->GetQuasControlMultiplier();
		}
	}

	float ReactionBonusDamage = 0.f;

	if (Element == EChronicleOrb::Exort)
	{
		if (bFrozen)
		{
			// THERMAL SHOCK (Fire + Ice)
			bFrozen = false;
			FreezeTimer = 0.f;
			ReactionBonusDamage = 52.f * ThermalShockMult;
			TriggerStatusFlash(FLinearColor(1.0f, 0.56f, 0.18f), 19000.f, 0.42f);
			UE_LOG(LogChronicles, Log, TEXT("Elemental Reaction: THERMAL SHOCK on %s (%.1f bonus dmg)!"), *GetName(), ReactionBonusDamage);
		}
		else if (bShocked)
		{
			// OVERLOAD (Fire + Storm)
			bShocked = false;
			ShockTimer = 0.f;
			ReactionBonusDamage = 44.f * SuperconductMult;
			TriggerStatusFlash(FLinearColor(1.0f, 0.25f, 0.72f), 17500.f, 0.38f);
		}
		else
		{
			BurnStacks = FMath::Min(5, BurnStacks + 1);
			BurnTimer = 4.5f;
			ArmorMeltVulnerability = FMath::Max(ArmorMeltVulnerability, ArmorMeltBonus);
			TriggerStatusFlash(FLinearColor(1.0f, 0.34f, 0.06f), 8500.f, 0.26f);
		}
	}
	else if (Element == EChronicleOrb::Wex)
	{
		if (bFrozen)
		{
			// SUPERCONDUCT (Storm + Ice)
			bFrozen = false;
			FreezeTimer = 0.f;
			ArmorMeltVulnerability = FMath::Max(ArmorMeltVulnerability, 0.22f);
			ReactionBonusDamage = 48.f * SuperconductMult;
			TriggerStatusFlash(FLinearColor(0.45f, 0.72f, 1.0f), 18500.f, 0.40f);
			UE_LOG(LogChronicles, Log, TEXT("Elemental Reaction: SUPERCONDUCT on %s (%.1f bonus dmg)!"), *GetName(), ReactionBonusDamage);
		}
		else
		{
			bShocked = true;
			ShockTimer = 4.5f;
			TriggerStatusFlash(FLinearColor(0.58f, 0.28f, 1.0f), 9000.f, 0.26f);
		}
	}
	else if (Element == EChronicleOrb::Quas)
	{
		if (BurnStacks > 0)
		{
			// CRYO-SHATTER / THERMAL SHOCK (Ice + Burn)
			ReactionBonusDamage = (34.f + 11.f * static_cast<float>(BurnStacks)) * ThermalShockMult;
			BurnStacks = 0;
			BurnTimer = 0.f;
			bFrozen = true;
			FreezeTimer = 2.0f * QuasControlMult;
			TriggerStatusFlash(FLinearColor(0.28f, 0.86f, 1.0f), 18000.f, 0.42f);
			UE_LOG(LogChronicles, Log, TEXT("Elemental Reaction: CRYO-SHATTER on %s (%.1f bonus dmg)!"), *GetName(), ReactionBonusDamage);
		}
		else if (bShocked)
		{
			// SUPERCONDUCT (Ice + Shock)
			bShocked = false;
			ShockTimer = 0.f;
			bFrozen = true;
			FreezeTimer = 2.4f * QuasControlMult;
			ArmorMeltVulnerability = FMath::Max(ArmorMeltVulnerability, 0.22f);
			ReactionBonusDamage = 46.f * SuperconductMult;
			TriggerStatusFlash(FLinearColor(0.40f, 0.76f, 1.0f), 18000.f, 0.40f);
		}
		else
		{
			bFrozen = true;
			FreezeTimer = 2.8f * QuasControlMult;
			TriggerStatusFlash(FLinearColor(0.16f, 0.74f, 1.0f), 8500.f, 0.30f);
		}
	}

	if (ReactionBonusDamage > 0.f && HasAuthority())
	{
		HP = FMath::Max(0.f, HP - ReactionBonusDamage);
		if (HP <= 0.f && !bDead)
		{
			HandleDeath(InstigatorCtrl, InstigatorActor);
		}
	}
}

void AAetheriaEnemyCharacter::InferAndApplyElementFromDamageCauser(AActor* DamageCauser, AActor* InstigatorActor)
{
	if (!DamageCauser)
	{
		return;
	}

	if (const AChronicleSpellActor* SpellActor = Cast<AChronicleSpellActor>(DamageCauser))
	{
		switch (SpellActor->GetSpellId())
		{
		case EChronicleSpellId::SunStrike:
		case EChronicleSpellId::ChaosMeteor:
		case EChronicleSpellId::EmberConduit:
			ApplyElementalStatus(EChronicleOrb::Exort, InstigatorActor);
			break;
		case EChronicleSpellId::StormAlacrity:
		case EChronicleSpellId::StormEMP:
		case EChronicleSpellId::GravityTornado:
			ApplyElementalStatus(EChronicleOrb::Wex, InstigatorActor);
			break;
		case EChronicleSpellId::ColdSnap:
		case EChronicleSpellId::ChronoBlink:
		case EChronicleSpellId::IcePrismWall:
			ApplyElementalStatus(EChronicleOrb::Quas, InstigatorActor);
			break;
		case EChronicleSpellId::PrimalBlast:
			ApplyElementalStatus(EChronicleOrb::Exort, InstigatorActor);
			ApplyElementalStatus(EChronicleOrb::Quas, InstigatorActor);
			break;
		default:
			break;
		}
	}
}

float AAetheriaEnemyCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bDead || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	float ModifiedDamage = DamageAmount;
	if (bShocked)
	{
		ModifiedDamage *= 1.15f;
	}
	if (ArmorMeltVulnerability > 0.f)
	{
		ModifiedDamage *= (1.f + ArmorMeltVulnerability);
	}

	const float Applied = Super::TakeDamage(ModifiedDamage, DamageEvent, EventInstigator, DamageCauser);
	HP = FMath::Max(0.f, HP - Applied);

	AActor* InstigatorActor = (EventInstigator && EventInstigator->GetPawn()) ? static_cast<AActor*>(EventInstigator->GetPawn()) : DamageCauser;
	InferAndApplyElementFromDamageCauser(DamageCauser, InstigatorActor);

	if (HP <= 0.f && !bDead)
	{
		HandleDeath(EventInstigator, DamageCauser);
	}
	else
	{
		PlayEnemyAnimSequence(EnemyHitReactAnim, 1.0f);
		TriggerStatusFlash(FLinearColor(1.0f, 0.32f, 0.14f), 6500.f, 0.20f);
	}

	return Applied;
}

void AAetheriaEnemyCharacter::HandleDeath(AController* KillerController, AActor* DamageCauser)
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	bWindupActive = false;
	BurnStacks = 0;
	bFrozen = false;
	bShocked = false;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	PlayEnemyAnimSequence(EnemyDeathAnim, 1.0f);
	TriggerStatusFlash(FLinearColor(1.0f, 0.55f, 0.18f), 11000.f, 0.45f);

	// Award XP & Gold to killer Hero's SkillTreeComponent and increment EnemiesSlain
	if (AAetheriaHeroCharacter* KillerHero = AetheriaActorsUtil::ResolveHeroFromInstigator(KillerController, DamageCauser))
	{
		KillerHero->EnemiesSlain += 1;
		if (KillerHero->SkillTree)
		{
			KillerHero->SkillTree->AddXP(XPReward);
			KillerHero->SkillTree->AddGold(GoldReward);
		}
	}

	// Advance linked quest objective automatically on death
	if (!QuestIdOnDeath.IsNone())
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQuestSubsystem* QuestSys = GI->GetSubsystem<UQuestSubsystem>())
			{
				QuestSys->AdvanceQuestProgress(QuestIdOnDeath, 1);
			}
		}
	}

	SetLifeSpan(4.0f);
}

AAetheriaHeroCharacter* AAetheriaEnemyCharacter::FindNearestAliveHero(float MaxDistance, float& OutDistance) const
{
	OutDistance = MaxDistance;
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AAetheriaHeroCharacter* BestHero = nullptr;
	float BestDistSq = FMath::Square(MaxDistance);
	const FVector MyLoc = GetActorLocation();

	for (TActorIterator<AAetheriaHeroCharacter> It(World); It; ++It)
	{
		AAetheriaHeroCharacter* Hero = *It;
		if (!Hero || Hero->bDead)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(MyLoc, Hero->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestHero = Hero;
		}
	}

	if (BestHero)
	{
		OutDistance = FMath::Sqrt(BestDistSq);
	}
	return BestHero;
}

void AAetheriaEnemyCharacter::UpdateElementalStatuses(float DeltaTime)
{
	if (StatusFlashTimer > 0.f && StatusFlashLight)
	{
		StatusFlashTimer = FMath::Max(0.f, StatusFlashTimer - DeltaTime);
		const float Alpha = (StatusFlashDuration > KINDA_SMALL_NUMBER) ? (StatusFlashTimer / StatusFlashDuration) : 0.f;
		StatusFlashLight->SetIntensity(StatusFlashPeakLumens * Alpha * Alpha);
	}

	if (bDead)
	{
		return;
	}

	if (StaggerTimer > 0.f)
	{
		StaggerTimer = FMath::Max(0.f, StaggerTimer - DeltaTime);
	}

	if (FreezeTimer > 0.f)
	{
		FreezeTimer = FMath::Max(0.f, FreezeTimer - DeltaTime);
		if (FreezeTimer <= 0.f)
		{
			bFrozen = false;
		}
	}

	if (ShockTimer > 0.f)
	{
		ShockTimer = FMath::Max(0.f, ShockTimer - DeltaTime);
		if (ShockTimer <= 0.f)
		{
			bShocked = false;
		}
	}

	if (BurnTimer > 0.f && BurnStacks > 0)
	{
		BurnTimer = FMath::Max(0.f, BurnTimer - DeltaTime);
		BurnDotAccumulator += DeltaTime;

		if (BurnDotAccumulator >= 0.5f)
		{
			BurnDotAccumulator = 0.f;
			if (HasAuthority())
			{
				float WildfireMult = 1.f;
				AController* BurnCtrl = nullptr;
				if (AAetheriaHeroCharacter* Hero = AetheriaActorsUtil::ResolveHeroFromInstigator(nullptr, LastElementInstigator.Get()))
				{
					BurnCtrl = Hero->GetController();
					if (Hero->SkillTree)
					{
						WildfireMult = Hero->SkillTree->GetExortWildfireMultiplier();
					}
				}
				const float TickBurn = 4.5f * static_cast<float>(BurnStacks) * WildfireMult;
				HP = FMath::Max(0.f, HP - TickBurn);
				TriggerStatusFlash(FLinearColor(1.0f, 0.36f, 0.08f), 5500.f, 0.16f);

				if (HP <= 0.f && !bDead)
				{
					HandleDeath(BurnCtrl, LastElementInstigator.Get());
					return;
				}
			}
		}

		if (BurnTimer <= 0.f)
		{
			BurnStacks = 0;
			ArmorMeltVulnerability = 0.f;
		}
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = bFrozen ? (BaseMoveSpeed * 0.25f) : BaseMoveSpeed;
	}
}

void AAetheriaEnemyCharacter::UpdateEnemyAI(float DeltaTime)
{
	if (!HasAuthority() || bDead || bFrozen || StaggerTimer > 0.f)
	{
		return;
	}

	if (AttackCooldownTimer > 0.f)
	{
		AttackCooldownTimer = FMath::Max(0.f, AttackCooldownTimer - DeltaTime);
	}

	float DistToHero = 0.f;
	AAetheriaHeroCharacter* TargetHero = FindNearestAliveHero(AggroRadius, DistToHero);
	if (!TargetHero)
	{
		bWindupActive = false;
		return;
	}

	const FVector ToHero = (TargetHero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

	if (bWindupActive)
	{
		WindupTimer -= DeltaTime;
		if (WindupTimer <= 0.f)
		{
			bWindupActive = false;
			AttackCooldownTimer = 1.65f;
			PlayEnemyAttackAnim(1.0f);

			// Melee sweep using CHRONICLE_TRACE_PROJECTILE
			if (UWorld* World = GetWorld())
			{
				const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 40.f);
				const FVector End = Start + ToHero * (AttackRange + 45.f);
				FCollisionQueryParams Params(TEXT("EnemyMeleeSwing"), false, this);
				Params.AddIgnoredActor(this);

				TArray<FHitResult> Hits;
				const FCollisionShape Shape = FCollisionShape::MakeCapsule(75.f, 95.f);
				const FQuat SweepQuat = AetheriaActorsUtil::MakeCapsuleSweepQuat(ToHero);
				World->SweepMultiByChannel(Hits, Start, End, SweepQuat, CHRONICLE_TRACE_PROJECTILE, Shape, Params);

				TSet<AActor*> HitHeroes;
				for (const FHitResult& Hit : Hits)
				{
					if (AAetheriaHeroCharacter* HitHero = Cast<AAetheriaHeroCharacter>(Hit.GetActor()))
					{
						if (!HitHeroes.Contains(HitHero))
						{
							HitHeroes.Add(HitHero);
							UGameplayStatics::ApplyPointDamage(HitHero, AttackDamage, ToHero, Hit, GetController(), this, UDamageType::StaticClass());
						}
					}
				}
			}
		}
		return;
	}

	if (DistToHero > AttackRange * 0.88f)
	{
		AddMovementInput(ToHero, 1.0f);
	}
	else if (AttackCooldownTimer <= 0.f)
	{
		bWindupActive = true;
		WindupTimer = 0.55f;
		TriggerStatusFlash(FLinearColor(1.0f, 0.16f, 0.10f), 7500.f, 0.50f);
	}
}

void AAetheriaEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateElementalStatuses(DeltaTime);
	UpdateEnemyAI(DeltaTime);
	UpdateEnemySingleNodeAnimation(DeltaTime);
}

// ============================================================================
// 3B. AAetheriaBossVolleyOrb — Molten Projectile for Boss AbyssalFireVolley
// ============================================================================

AAetheriaBossVolleyOrb::AAetheriaBossVolleyOrb()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	InitialLifeSpan = 4.5f;

	OrbRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OrbRoot"));
	SetRootComponent(OrbRoot);

	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	OrbMesh->SetupAttachment(OrbRoot);
	OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OrbMesh->SetCastShadow(false);
	OrbMesh->SetRelativeScale3D(FVector(0.68f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(AetheriaActorsUtil::PathSphere);
	if (SphereFinder.Succeeded())
	{
		OrbMesh->SetStaticMesh(SphereFinder.Object);
	}

	OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
	OrbLight->SetupAttachment(OrbRoot);
	OrbLight->SetIntensityUnits(ELightUnits::Lumens);
	OrbLight->SetIntensity(14000.f);
	OrbLight->SetLightColor(FLinearColor(1.0f, 0.22f, 0.06f, 1.0f));
	OrbLight->SetAttenuationRadius(1500.f);
	OrbLight->SetCastShadows(false);
}

void AAetheriaBossVolleyOrb::InitializeVolleyOrb(const FVector& InDirection, float InSpeed, float InDamage)
{
	FlightDir = InDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	Speed = FMath::Max(400.f, InSpeed);
	Damage = FMath::Max(1.f, InDamage);

	OrbMID = AetheriaActorsUtil::CreateColorMID(this, FLinearColor(16.0f, 2.2f, 0.4f, 1.0f), true);
	if (OrbMesh && OrbMID)
	{
		OrbMesh->SetMaterial(0, OrbMID);
	}
}

void AAetheriaBossVolleyOrb::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bImpacted)
	{
		return;
	}

	Elapsed += DeltaTime;
	const FVector Start = GetActorLocation();
	const FVector Step = FlightDir * Speed * DeltaTime;
	const FVector End = Start + Step;

	if (OrbMesh)
	{
		const float Pulse = 0.68f + 0.10f * FMath::Sin(Elapsed * 22.f);
		OrbMesh->SetRelativeScale3D(FVector(Pulse));
	}

	if (UWorld* World = GetWorld())
	{
		FCollisionQueryParams Params(TEXT("BossVolleyOrbSweep"), false, this);
		Params.AddIgnoredActor(this);
		if (AActor* OwnerActor = GetOwner())
		{
			Params.AddIgnoredActor(OwnerActor);
		}

		FHitResult Hit;
		const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(42.f, 46.f);
		const FQuat SweepQuat = AetheriaActorsUtil::MakeCapsuleSweepQuat(FlightDir);

		if (World->SweepSingleByChannel(Hit, Start, End, SweepQuat, CHRONICLE_TRACE_PROJECTILE, CapsuleShape, Params))
		{
			bImpacted = true;
			SetActorLocation(Hit.ImpactPoint);

			if (HasAuthority())
			{
				if (AActor* HitActor = Hit.GetActor())
				{
					if (ADestructiblePropActor* Destructible = Cast<ADestructiblePropActor>(HitActor))
					{
						Destructible->ForceShatter(Hit.ImpactPoint);
					}
					else
					{
						UGameplayStatics::ApplyPointDamage(HitActor, Damage, FlightDir, Hit, GetInstigatorController(), this, UDamageType::StaticClass());
					}
				}
			}

			Destroy();
			return;
		}
	}

	SetActorLocation(End);
}

// ============================================================================
// 3C. AAetheriaBossCharacter — Malakor, Sovereign of the Shattered Seal
// ============================================================================

AAetheriaBossCharacter::AAetheriaBossCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	bAlwaysRelevant = true;

	EnemyDisplayName = NSLOCTEXT("AetheriaBoss", "Malakor", "Malakor, Sovereign of the Shattered Seal");
	MaxHP = 1800.f;
	HP = 1800.f;
	XPReward = 650;
	GoldReward = 450;
	QuestIdOnDeath = FName(TEXT("Q_FallOfSovereign"));
	AggroRadius = ENGAGE_RADIUS;
	AttackRange = 420.f;
	AttackDamage = 30.f;
	BaseMoveSpeed = 420.f;
	bArchetypeConfigured = true;

	// Towering Wukong-scale cinematic presence (1.65x)
	SetActorScale3D(FVector(1.65f));

	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("BossMotionWarping"));

	// Physical AgX Crimson Telegraph PointLight (22,000 lm telegraph -> 34,000 lm strike!)
	TelegraphCrimsonLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TelegraphCrimsonLight"));
	TelegraphCrimsonLight->SetupAttachment(RootComponent);
	TelegraphCrimsonLight->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	TelegraphCrimsonLight->SetIntensityUnits(ELightUnits::Lumens);
	TelegraphCrimsonLight->SetIntensity(0.f);
	TelegraphCrimsonLight->SetLightColor(FLinearColor(1.0f, 0.06f, 0.04f, 1.0f));
	TelegraphCrimsonLight->SetAttenuationRadius(2400.f);
	TelegraphCrimsonLight->SetSourceRadius(42.f);
	TelegraphCrimsonLight->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(AetheriaActorsUtil::PathCylinder);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(AetheriaActorsUtil::PathSphere);

	// Glowing crimson ground danger ring showing exact AoE footprint during telegraphs
	TelegraphRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TelegraphRingMesh"));
	TelegraphRingMesh->SetupAttachment(RootComponent);
	TelegraphRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TelegraphRingMesh->SetCastShadow(false);
	TelegraphRingMesh->SetRelativeLocation(FVector(0.f, 0.f, -86.f));
	TelegraphRingMesh->SetRelativeScale3D(FVector(6.0f, 6.0f, 0.05f));
	TelegraphRingMesh->SetVisibility(false);
	if (CylinderFinder.Succeeded())
	{
		TelegraphRingMesh->SetStaticMesh(CylinderFinder.Object);
	}

	// Sculpted pulsing crimson boss aura core during telegraphs
	TelegraphAuraCoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TelegraphAuraCoreMesh"));
	TelegraphAuraCoreMesh->SetupAttachment(RootComponent);
	TelegraphAuraCoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TelegraphAuraCoreMesh->SetCastShadow(false);
	TelegraphAuraCoreMesh->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	TelegraphAuraCoreMesh->SetRelativeScale3D(FVector(1.4f));
	TelegraphAuraCoreMesh->SetVisibility(false);
	if (SphereFinder.Succeeded())
	{
		TelegraphAuraCoreMesh->SetStaticMesh(SphereFinder.Object);
	}
}

void AAetheriaBossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAetheriaBossCharacter, BossTitle);
	DOREPLIFETIME(AAetheriaBossCharacter, BossPhase);
	DOREPLIFETIME(AAetheriaBossCharacter, StaggerGauge);
	DOREPLIFETIME(AAetheriaBossCharacter, bStaggerBroken);
	DOREPLIFETIME(AAetheriaBossCharacter, bBossEngaged);
	DOREPLIFETIME(AAetheriaBossCharacter, CurrentAttack);
	DOREPLIFETIME(AAetheriaBossCharacter, bIsTelegraphing);
	DOREPLIFETIME(AAetheriaBossCharacter, TelegraphTimer);
}

void AAetheriaBossCharacter::BeginPlay()
{
	bArchetypeConfigured = true;
	Super::BeginPlay();

	BossHomeLocation = GetActorLocation();
	if (!BossHeavyAnim)
	{
		if (UAnimSequence* ChargedSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathAnimCharged))
		{
			USkeletalMeshComponent* MeshComp = GetMesh();
			USkeletalMesh* SkelMesh = MeshComp ? MeshComp->GetSkeletalMeshAsset() : nullptr;
			if (SkelMesh && AetheriaActorsUtil::IsSkeletonCompatible(SkelMesh, ChargedSeq->GetSkeleton()))
			{
				BossHeavyAnim = ChargedSeq;
			}
		}
	}

	TelegraphCrimsonMID = AetheriaActorsUtil::CreateColorMID(this, FLinearColor(14.0f, 0.45f, 0.30f, 1.0f), true);
	if (TelegraphRingMesh && TelegraphCrimsonMID)
	{
		TelegraphRingMesh->SetMaterial(0, TelegraphCrimsonMID);
	}
	if (TelegraphAuraCoreMesh && TelegraphCrimsonMID)
	{
		TelegraphAuraCoreMesh->SetMaterial(0, TelegraphCrimsonMID);
	}
}

void AAetheriaBossCharacter::BindEnemyMeshAndAnims()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	bUsingSingleNodeAnims = false;
	bDeathAnimPlayed = false;
	OneShotAnimTimer = 0.f;
	EnemyAttackAnimCycle = 0;
	CurrentEnemyAnim = nullptr;
	EnemyAttackAnim = nullptr;
	EnemyHitReactAnim = nullptr;
	EnemyDeathAnim = nullptr;
	EnemyAttackMontages.Reset();
	BossHeavyAnim = nullptr;
	BossRoarAnim = nullptr;
	EnemyAnimSlotName = FName(TEXT("DefaultSlot"));

	// Boss mesh priority:
	// 1. /Game/FabImports/Bosses
	// 2. /Game/ParagonSevarog/Characters/Heroes/Sevarog/Skins/Tier_1/Sevarog_Red/Meshes/SevarogBloodred.SevarogBloodred
	//    (fallback Sevarog.Sevarog, then /Game/ParagonSevarog scan)
	// 3. /Game/ParagonRampage/Characters/Heroes/Rampage/Meshes/Rampage.Rampage (scaled 1.55f + Rampage ABP/Montages/Roar)
	// 4. Fallback: /Game/Characters/Mobs/SKM_Boss_Main -> SKM_Mob_Bruiser -> SKM_Manny_Simple
	USkeletalMesh* ChosenMesh = nullptr;
	bool bUsingSevarogBoss = false;
	bool bUsingRampageBoss = false;

	// Priority 1: /Game/FabImports/Bosses
	{
		TArray<FString> FabBossDirs;
		FabBossDirs.Add(TEXT("/Game/FabImports/Bosses"));
		ChosenMesh = AetheriaActorsUtil::FindFirstSkeletalMeshInPaths(FabBossDirs);
	}

	// Priority 2: Paragon Sevarog Bloodred -> Sevarog -> folder scan
	if (!ChosenMesh)
	{
		ChosenMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathSevarogBloodredMesh);
		bUsingSevarogBoss = (ChosenMesh != nullptr);
	}
	if (!ChosenMesh)
	{
		ChosenMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathSevarogMesh);
		bUsingSevarogBoss = (ChosenMesh != nullptr);
	}
	if (!ChosenMesh)
	{
		TArray<FString> SevarogDirs;
		SevarogDirs.Add(TEXT("/Game/ParagonSevarog"));
		ChosenMesh = AetheriaActorsUtil::FindFirstSkeletalMeshInPaths(SevarogDirs);
		bUsingSevarogBoss = (ChosenMesh != nullptr);
	}

	// Priority 3: Paragon Rampage
	if (!ChosenMesh)
	{
		ChosenMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathRampageMesh);
		bUsingRampageBoss = (ChosenMesh != nullptr);
	}

	// Priority 4: Staged SKM_Boss_Main -> SKM_Mob_Bruiser -> SKM_Manny_Simple fallback
	if (!ChosenMesh)
	{
		ChosenMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathBossMain);
	}
	if (!ChosenMesh)
	{
		ChosenMesh = AetheriaActorsUtil::SafeLoadObjectIfPresent<USkeletalMesh>(AetheriaActorsUtil::PathMobBruiser);
	}
	if (!ChosenMesh)
	{
		ChosenMesh = LoadObject<USkeletalMesh>(nullptr, AetheriaActorsUtil::PathMannySimple);
	}

	if (!ChosenMesh)
	{
		return;
	}

	MeshComp->SetSkeletalMesh(ChosenMesh);

	if (bUsingSevarogBoss || ChosenMesh->GetPathName().Contains(TEXT("/Game/ParagonSevarog")))
	{
		SetActorScale3D(FVector(1.60f));
		BindParagonEnemyAnimsAndMontages(
			MeshComp,
			AetheriaActorsUtil::PathSevarogABP,
			AetheriaActorsUtil::PathSevarogSwing1Montage,
			AetheriaActorsUtil::PathSevarogSwing2Montage,
			nullptr,
			AetheriaActorsUtil::PathSevarogHitReact,
			AetheriaActorsUtil::PathSevarogDeath,
			FName(TEXT("FullBody")));

		if (UAnimSequence* Swing3Seq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathSevarogSwing3Seq))
		{
			if (AetheriaActorsUtil::IsSkeletonCompatible(ChosenMesh, Swing3Seq->GetSkeleton()))
			{
				EnemyAttackAnim = Swing3Seq;
			}
		}
		if (UAnimSequence* UltSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathSevarogUltSwingSeq))
		{
			if (AetheriaActorsUtil::IsSkeletonCompatible(ChosenMesh, UltSeq->GetSkeleton()))
			{
				BossHeavyAnim = UltSeq;
			}
		}

		UE_LOG(LogChronicles, Log, TEXT("AAetheriaBossCharacter (%s) bound Paragon Sevarog (%s, scale 1.60) + AnimBP + %d Montages."),
			*BossTitle, *ChosenMesh->GetName(), EnemyAttackMontages.Num());
		return;
	}

	if (bUsingRampageBoss)
	{
		SetActorScale3D(FVector(1.55f));
		BindRampageAnimsAndMontages(MeshComp);

		if (UAnimSequence* RoarSeq = AetheriaActorsUtil::SafeLoadObjectIfPresent<UAnimSequence>(AetheriaActorsUtil::PathRampageRoar))
		{
			if (AetheriaActorsUtil::IsSkeletonCompatible(ChosenMesh, RoarSeq->GetSkeleton()))
			{
				BossRoarAnim = RoarSeq;
				BossHeavyAnim = RoarSeq;
			}
		}

		UE_LOG(LogChronicles, Log, TEXT("AAetheriaBossCharacter (%s) bound Paragon Rampage (scale 1.55) + AnimBP + %d Montages + Roar."),
			*BossTitle, EnemyAttackMontages.Num());
		return;
	}

	TArray<const TCHAR*> CandidateABPs;
	CandidateABPs.Add(AetheriaActorsUtil::PathSevarogABP);
	CandidateABPs.Add(AetheriaActorsUtil::PathABPCombat);
	CandidateABPs.Add(AetheriaActorsUtil::PathABPUnarmed);
	AetheriaActorsUtil::TryBindCompatibleAnimBPFromList(MeshComp, CandidateABPs);

	EnemyAttackAnim   = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimAttack1);
	EnemyHitReactAnim = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimHitReact);
	EnemyDeathAnim    = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimDeath);
	BossHeavyAnim     = LoadObject<UAnimSequence>(nullptr, AetheriaActorsUtil::PathAnimCharged);

	UE_LOG(LogChronicles, Log, TEXT("AAetheriaBossCharacter (%s) bound SkeletalMesh: %s"), *BossTitle, *ChosenMesh->GetName());
}

bool AAetheriaBossCharacter::IsCurrentAttackParryable() const
{
	return (CurrentAttack == EBossAttackKind::CrimsonCleave)
		|| (CurrentAttack == EBossAttackKind::TempestLunge);
}

void AAetheriaBossCharacter::AddStagger(float Amount, float BonusBreakSeconds)
{
	if (bDead || bStaggerBroken || Amount <= 0.f)
	{
		return;
	}

	StaggerGauge = FMath::Clamp(StaggerGauge + Amount, 0.f, MaxStagger);
	if (StaggerGauge >= MaxStagger)
	{
		// STAGGER BREAK: cancels active telegraph + opens 2.4s (+skill bonus) +35% vulnerability window!
		bStaggerBroken = true;
		bIsTelegraphing = false;
		bCataclysmStillnessPhase = false;
		CurrentAttack = EBossAttackKind::None;
		TelegraphTimer = 0.f;
		StaggerBreakTotalDuration = 2.4f + FMath::Max(0.f, BonusBreakSeconds);
		StaggerBreakTimer = StaggerBreakTotalDuration;

		PlayEnemyAnimSequence(EnemyHitReactAnim, 0.72f);
		TriggerStatusFlash(FLinearColor(1.0f, 0.88f, 0.32f), 26000.f, 0.65f);

		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQuestSubsystem* QuestSys = GI->GetSubsystem<UQuestSubsystem>())
			{
				QuestSys->PushNotification(TEXT("STAGGER BREAK! Malakor's guard is shattered (+35% Damage)!"));
			}
		}

		UE_LOG(LogChronicles, Log, TEXT("Malakor STAGGER BROKEN for %.2fs!"), StaggerBreakTotalDuration);
	}
}

void AAetheriaBossCharacter::ApplyStagger(float Duration)
{
	AddStagger(35.f, FMath::Max(0.f, Duration - 1.0f));
}

float AAetheriaBossCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bDead || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	// +35% incoming damage during Stagger Break!
	float AdjustedDamage = bStaggerBroken ? (DamageAmount * 1.35f) : DamageAmount;
	const float Applied = Super::TakeDamage(AdjustedDamage, DamageEvent, EventInstigator, DamageCauser);

	if (!bDead && Applied > 0.f)
	{
		if (!bBossEngaged)
		{
			bBossEngaged = true;
			if (UAetheriaMusicSubsystem* MusicSys = UAetheriaMusicSubsystem::Get(this))
			{
				MusicSys->SetBossEncounterActive(true);
			}
		}

		// Spells & heavy strikes naturally chip Boss StaggerGauge
		if (!bStaggerBroken)
		{
			float StaggerMult = 1.f;
			float BonusSec = 0.f;
			if (AAetheriaHeroCharacter* Hero = AetheriaActorsUtil::ResolveHeroFromInstigator(EventInstigator, DamageCauser))
			{
				if (Hero->SkillTree)
				{
					StaggerMult = Hero->SkillTree->GetStaggerMultiplier();
					BonusSec = Hero->SkillTree->GetStaggerWindowBonusSeconds();
				}
			}
			AddStagger(FMath::Clamp(Applied * 0.22f * StaggerMult, 2.f, 26.f), BonusSec);
		}

		UpdateBossPhase();
	}

	return Applied;
}

void AAetheriaBossCharacter::UpdateBossPhase()
{
	if (MaxHP <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float HPRatio = HP / MaxHP;
	const int32 TargetPhase = (HPRatio <= 0.33f) ? 3 : ((HPRatio <= 0.66f) ? 2 : 1);

	if (TargetPhase > BossPhase)
	{
		BossPhase = TargetPhase;

		if (BossRoarAnim)
		{
			PlayEnemyAnimSequence(BossRoarAnim, 1.0f);
		}

		AetheriaActorsUtil::SpawnBossShockwaveAndExplosionFX(GetWorld(), GetActorLocation(), 1.55f);

		if (TelegraphCrimsonLight)
		{
			ImpactLightPeakLumens = 3800.f;
			ImpactLightDecayTimer = 0.75f;
			TelegraphCrimsonLight->SetLightColor(FLinearColor(1.0f, 0.08f, 0.05f));
			TelegraphCrimsonLight->SetIntensity(ImpactLightPeakLumens);
		}

		if (UGameInstance* GI = GetGameInstance())
		{
			if (UQuestSubsystem* QuestSys = GI->GetSubsystem<UQuestSubsystem>())
			{
				if (BossPhase == 2)
				{
					QuestSys->PushNotification(TEXT("Malakor enters PHASE II — Tempest Lunge & Shatter Spin unlocked!"));
				}
				else if (BossPhase == 3)
				{
					QuestSys->PushNotification(TEXT("Malakor enters PHASE III — Beware CHRONO CATACLYSM!"));
				}
			}
		}

		UE_LOG(LogChronicles, Log, TEXT("%s transitioned to Boss Phase %d (HP=%.1f/%.1f)."), *BossTitle, BossPhase, HP, MaxHP);
	}
}

void AAetheriaBossCharacter::ResetBossEncounter()
{
	if (bDead)
	{
		return;
	}

	const bool bWasEngaged = bBossEngaged;
	HP = MaxHP;
	BossPhase = 1;
	StaggerGauge = 0.f;
	bStaggerBroken = false;
	StaggerBreakTimer = 0.f;
	bBossEngaged = false;
	if (bWasEngaged)
	{
		if (UAetheriaMusicSubsystem* MusicSys = UAetheriaMusicSubsystem::Get(this))
		{
			MusicSys->SetBossEncounterActive(false);
		}
	}
	bIsTelegraphing = false;
	bCataclysmStillnessPhase = false;
	CataclysmStillnessTimer = 0.f;
	TelegraphTimer = 0.f;
	CurrentAttack = EBossAttackKind::None;
	BossAttackCooldown = 1.6f;
	AttackCycleCounter = 0;

	BurnStacks = 0;
	bFrozen = false;
	bShocked = false;
	ArmorMeltVulnerability = 0.f;
	LockedTargetHero.Reset();

	if (!BossHomeLocation.IsNearlyZero())
	{
		SetActorLocation(BossHomeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (TelegraphRingMesh)     { TelegraphRingMesh->SetVisibility(false); }
	if (TelegraphAuraCoreMesh) { TelegraphAuraCoreMesh->SetVisibility(false); }
	if (TelegraphCrimsonLight) { TelegraphCrimsonLight->SetIntensity(0.f); }

	UE_LOG(LogChronicles, Log, TEXT("Boss encounter (%s) cleanly reset to home sanctuary."), *BossTitle);
}

void AAetheriaBossCharacter::HandleDeath(AController* KillerController, AActor* DamageCauser)
{
	bBossEngaged = false;
	if (UAetheriaMusicSubsystem* MusicSys = UAetheriaMusicSubsystem::Get(this))
	{
		MusicSys->SetBossEncounterActive(false);
	}
	bIsTelegraphing = false;
	bCataclysmStillnessPhase = false;
	CurrentAttack = EBossAttackKind::None;
	bStaggerBroken = false;

	if (TelegraphRingMesh)     { TelegraphRingMesh->SetVisibility(false); }
	if (TelegraphAuraCoreMesh) { TelegraphAuraCoreMesh->SetVisibility(false); }

	if (TelegraphCrimsonLight)
	{
		ImpactLightPeakLumens = 4200.f;
		ImpactLightDecayTimer = 1.1f;
		TelegraphCrimsonLight->SetLightColor(FLinearColor(1.0f, 0.72f, 0.25f));
		TelegraphCrimsonLight->SetIntensity(ImpactLightPeakLumens);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UQuestSubsystem* QuestSys = GI->GetSubsystem<UQuestSubsystem>())
		{
			QuestSys->PushNotification(TEXT("VICTORY! Malakor, Sovereign of the Shattered Seal, has been vanquished!"));
		}
	}

	Super::HandleDeath(KillerController, DamageCauser);
	SetLifeSpan(10.0f);
}

EBossAttackKind AAetheriaBossCharacter::ChooseNextBossAttack(float DistToHero)
{
	// Phase-gated Wukong Boss Arsenal:
	// Phase 1: CrimsonCleave, SeismicSlam, AbyssalFireVolley
	// Phase 2: + TempestLunge, ShatterSpin
	// Phase 3: + ChronoCataclysm
	TArray<EBossAttackKind> Pool;
	Pool.Add(EBossAttackKind::CrimsonCleave);
	Pool.Add(EBossAttackKind::SeismicSlam);
	Pool.Add(EBossAttackKind::AbyssalFireVolley);

	if (BossPhase >= 2)
	{
		Pool.Add(EBossAttackKind::TempestLunge);
		Pool.Add(EBossAttackKind::ShatterSpin);
	}
	if (BossPhase >= 3)
	{
		Pool.Add(EBossAttackKind::ChronoCataclysm);
	}

	// Smart distance bias: if hero is far, favor gap-closer or molten volley
	if (DistToHero > 820.f)
	{
		if (BossPhase >= 2 && (AttackCycleCounter % 2 == 0))
		{
			++AttackCycleCounter;
			return EBossAttackKind::TempestLunge;
		}
		++AttackCycleCounter;
		return EBossAttackKind::AbyssalFireVolley;
	}

	const EBossAttackKind Chosen = Pool[AttackCycleCounter % Pool.Num()];
	++AttackCycleCounter;
	return Chosen;
}

void AAetheriaBossCharacter::BeginTelegraphedAttack(EBossAttackKind AttackKind, AAetheriaHeroCharacter* TargetHero)
{
	CurrentAttack = AttackKind;
	bIsTelegraphing = true;
	bCataclysmStillnessPhase = false;
	TelegraphTimer = TELEGRAPH_DURATION; // Unmistakable 0.95s Crimson Glow telegraph!
	LockedTargetHero = TargetHero;

	if (TargetHero)
	{
		LungeTargetLocation = TargetHero->GetActorLocation();
		const FVector ToHero = (LungeTargetLocation - GetActorLocation()).GetSafeNormal2D();
		if (!ToHero.IsNearlyZero())
		{
			SetActorRotation(ToHero.Rotation());
			if (MotionWarping)
			{
				const FVector WarpLoc = LungeTargetLocation - ToHero * 165.f;
				MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("BossAttackTarget"), WarpLoc, ToHero.Rotation());
			}
		}
	}

	// Scale glowing crimson ground danger ring to match the telegraphed attack's exact danger radius
	if (TelegraphRingMesh)
	{
		float WorldRadius = 460.f;
		switch (AttackKind)
		{
		case EBossAttackKind::CrimsonCleave:     WorldRadius = 480.f; break;
		case EBossAttackKind::SeismicSlam:       WorldRadius = 780.f; break;
		case EBossAttackKind::AbyssalFireVolley: WorldRadius = 400.f; break;
		case EBossAttackKind::TempestLunge:      WorldRadius = 560.f; break;
		case EBossAttackKind::ShatterSpin:       WorldRadius = 640.f; break;
		case EBossAttackKind::ChronoCataclysm:   WorldRadius = 1250.f; break;
		default: break;
		}

		// Compensate for Boss 1.65x actor scale (BasicShapes Cylinder radius = 50 UU)
		const float ActorScale = FMath::Max(0.5f, GetActorScale3D().X);
		const float LocalScaleXY = (WorldRadius / 50.f) / ActorScale;
		TelegraphRingMesh->SetRelativeScale3D(FVector(LocalScaleXY, LocalScaleXY, 0.04f));
		TelegraphRingMesh->SetVisibility(true);
	}

	if (TelegraphAuraCoreMesh)
	{
		TelegraphAuraCoreMesh->SetVisibility(true);
	}
}

void AAetheriaBossCharacter::ExecuteCurrentBossAttack()
{
	const EBossAttackKind AttackToRun = CurrentAttack;
	const bool bWasParryable = IsCurrentAttackParryable();
	bIsTelegraphing = false;

	if (TelegraphRingMesh)     { TelegraphRingMesh->SetVisibility(false); }
	if (TelegraphAuraCoreMesh) { TelegraphAuraCoreMesh->SetVisibility(false); }

	// Clean EV100 AgX strike flash (Amber on parryable strikes, Crimson on dodge-only)
	if (TelegraphCrimsonLight)
	{
		ImpactLightPeakLumens = (AttackToRun == EBossAttackKind::ChronoCataclysm) ? 4500.f : 3600.f;
		ImpactLightDecayTimer = 0.55f;
		TelegraphCrimsonLight->SetLightColor(bWasParryable
			? FLinearColor(1.0f, 0.68f, 0.16f)
			: FLinearColor(1.0f, 0.08f, 0.05f));
		TelegraphCrimsonLight->SetIntensity(ImpactLightPeakLumens);
	}

	// Keep CurrentAttack intact during strike execution so Hero::TakeDamage() can query IsCurrentAttackParryable()
	switch (AttackToRun)
	{
	case EBossAttackKind::CrimsonCleave:     ExecuteCrimsonCleave(); break;
	case EBossAttackKind::SeismicSlam:       ExecuteSeismicSlam(); break;
	case EBossAttackKind::AbyssalFireVolley: ExecuteAbyssalFireVolley(); break;
	case EBossAttackKind::TempestLunge:      ExecuteTempestLunge(); break;
	case EBossAttackKind::ShatterSpin:       ExecuteShatterSpin(); break;
	case EBossAttackKind::ChronoCataclysm:   ExecuteChronoCataclysm(); break;
	default: break;
	}

	CurrentAttack = EBossAttackKind::None;

	// Faster attack rhythm in higher phases
	BossAttackCooldown = (BossPhase == 3) ? 1.35f : ((BossPhase == 2) ? 1.70f : 2.05f);
}

void AAetheriaBossCharacter::PlayBossAttackVisualAnim(int32 PreferredMontageIdx, UAnimSequence* FallbackSeq, float PlayRate)
{
	if (PreferredMontageIdx >= 0 && EnemyAttackMontages.IsValidIndex(PreferredMontageIdx) && EnemyAttackMontages[PreferredMontageIdx])
	{
		PlayEnemyMontage(EnemyAttackMontages[PreferredMontageIdx].Get(), PlayRate);
		return;
	}
	if (FallbackSeq)
	{
		PlayEnemyAnimSequence(FallbackSeq, PlayRate);
		return;
	}
	PlayEnemyAttackAnim(PlayRate);
}

void AAetheriaBossCharacter::ExecuteCrimsonCleave()
{
	PlayBossAttackVisualAnim(0, EnemyAttackAnim.Get(), 0.95f);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin = GetActorLocation() + FVector(0.f, 0.f, 55.f);
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector End = Origin + Forward * 470.f;

	FCollisionQueryParams Params(TEXT("BossCrimsonCleave"), false, this);
	Params.AddIgnoredActor(this);

	TArray<FHitResult> Hits;
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(220.f, 240.f);
	const FQuat SweepQuat = AetheriaActorsUtil::MakeCapsuleSweepQuat(Forward);
	World->SweepMultiByChannel(Hits, Origin, End, SweepQuat, CHRONICLE_TRACE_PROJECTILE, Shape, Params);

	TSet<AActor*> HitSet;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == this || HitSet.Contains(HitActor))
		{
			continue;
		}
		HitSet.Add(HitActor);

		if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(HitActor))
		{
			UGameplayStatics::ApplyPointDamage(Hero, 28.f, Forward, Hit, GetController(), this, UDamageType::StaticClass());
		}
		else if (ADestructiblePropActor* Prop = Cast<ADestructiblePropActor>(HitActor))
		{
			UGameplayStatics::ApplyPointDamage(Prop, 140.f, Forward, Hit, GetController(), this, UDamageType::StaticClass());
		}
	}
}

void AAetheriaBossCharacter::ExecuteSeismicSlam()
{
	PlayBossAttackVisualAnim(1, BossHeavyAnim ? BossHeavyAnim.Get() : EnemyAttackAnim.Get(), 0.90f);
	AetheriaActorsUtil::SpawnBossShockwaveAndExplosionFX(GetWorld(), GetActorLocation(), 1.50f);
	ApplyBossRadialShockwave(GetActorLocation(), 780.f, 32.f, 380.f, true);
}

void AAetheriaBossCharacter::ExecuteAbyssalFireVolley()
{
	PlayBossAttackVisualAnim(-1, BossRoarAnim ? BossRoarAnim.Get() : EnemyAttackAnim.Get(), 1.08f);

	UWorld* World = GetWorld();
	if (!World || !HasAuthority())
	{
		return;
	}

	FVector AimDir = GetActorForwardVector().GetSafeNormal();
	const FVector SpawnBase = GetActorLocation() + AimDir * 165.f + FVector(0.f, 0.f, 95.f);
	if (AAetheriaHeroCharacter* Target = LockedTargetHero.Get())
	{
		AimDir = (Target->GetActorLocation() + FVector(0.f, 0.f, 35.f) - SpawnBase).GetSafeNormal(UE_SMALL_NUMBER, AimDir);
	}

	// Spawn 3 blazing molten orbs in a crisp spread fan (-14°, 0°, +14°)
	static const float YawOffsets[3] = { -14.f, 0.f, 14.f };
	for (int32 i = 0; i < 3; ++i)
	{
		const FVector OrbDir = AimDir.RotateAngleAxis(YawOffsets[i], FVector::UpVector).GetSafeNormal();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AAetheriaBossVolleyOrb* Orb = World->SpawnActor<AAetheriaBossVolleyOrb>(
				AAetheriaBossVolleyOrb::StaticClass(),
				SpawnBase + OrbDir * 40.f,
				OrbDir.Rotation(),
				SpawnParams))
		{
			Orb->InitializeVolleyOrb(OrbDir, 1520.f, 24.f);
		}
	}
}

void AAetheriaBossCharacter::ExecuteTempestLunge()
{
	PlayBossAttackVisualAnim(2, BossHeavyAnim ? BossHeavyAnim.Get() : EnemyAttackAnim.Get(), 1.15f);

	FVector TargetPos = LungeTargetLocation;
	if (AAetheriaHeroCharacter* Target = LockedTargetHero.Get())
	{
		TargetPos = Target->GetActorLocation();
	}

	const FVector ToTarget = TargetPos - GetActorLocation();
	const FVector LungeDir = ToTarget.GetSafeNormal2D();
	const float Dist = FMath::Min(ToTarget.Size2D(), 1080.f);

	if (!LungeDir.IsNearlyZero())
	{
		SetActorRotation(LungeDir.Rotation());
		// High-speed gap-closing rush lunge with MotionWarping target alignment
		const FVector LungeLanding = GetActorLocation() + LungeDir * FMath::Max(0.f, Dist - 145.f);
		SetActorLocation(LungeLanding, true);
		LaunchCharacter(LungeDir * 680.f + FVector(0.f, 0.f, 120.f), true, false);
	}

	ApplyBossRadialShockwave(GetActorLocation() + LungeDir * 120.f, 440.f, 34.f, 260.f, true);
}

void AAetheriaBossCharacter::ExecuteShatterSpin()
{
	PlayBossAttackVisualAnim(2, EnemyAttackAnim.Get(), 1.25f);
	ApplyBossRadialShockwave(GetActorLocation(), 640.f, 36.f, 320.f, true);
}

void AAetheriaBossCharacter::ExecuteChronoCataclysm()
{
	UAnimSequence* CataclysmSeq = BossRoarAnim ? BossRoarAnim.Get() : (BossHeavyAnim ? BossHeavyAnim.Get() : EnemyAttackAnim.Get());
	PlayBossAttackVisualAnim(-1, CataclysmSeq, 0.88f);
	AetheriaActorsUtil::SpawnBossShockwaveAndExplosionFX(GetWorld(), GetActorLocation(), 2.30f);
	ApplyBossRadialShockwave(GetActorLocation(), 1250.f, 48.f, 520.f, true);
}

void AAetheriaBossCharacter::ApplyBossRadialShockwave(
	const FVector& Origin,
	float Radius,
	float DamageAmount,
	float LaunchZ,
	bool bShatterArenaColumns)
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority())
	{
		return;
	}

	FCollisionQueryParams Params(TEXT("BossRadialShockwave"), false, this);
	Params.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps,
		Origin,
		FQuat::Identity,
		CHRONICLE_TRACE_PROJECTILE,
		FCollisionShape::MakeSphere(Radius),
		Params);

	TSet<AActor*> AffectedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == this || AffectedActors.Contains(Actor))
		{
			continue;
		}
		AffectedActors.Add(Actor);

		if (AAetheriaHeroCharacter* Hero = Cast<AAetheriaHeroCharacter>(Actor))
		{
			const FVector PushDir = (Hero->GetActorLocation() - Origin).GetSafeNormal2D();
			const float Taken = UGameplayStatics::ApplyDamage(Hero, DamageAmount, GetController(), this, UDamageType::StaticClass());
			if (Taken > 0.f && !Hero->bDead)
			{
				const float KBResist = Hero->Customization ? FMath::Clamp(Hero->Customization->GetDerivedStats().KnockbackResistance, 0.f, 0.85f) : 0.f;
				const float KBMult = 1.0f - KBResist;
				Hero->LaunchCharacter((PushDir * 540.f + FVector(0.f, 0.f, LaunchZ)) * KBMult, false, false);
			}
		}
		else if (bShatterArenaColumns)
		{
			if (ADestructiblePropActor* Destructible = Cast<ADestructiblePropActor>(Actor))
			{
				// Chaos Destruction impulse fracturing nearby Highkeep arena columns!
				Destructible->ForceShatter(Origin);
			}
		}
	}
}

void AAetheriaBossCharacter::UpdateTelegraphVisuals(float DeltaTime)
{
	if (bIsTelegraphing || bCataclysmStillnessPhase)
	{
		// Wukong Color Law Telegraph: Amber (#FFB347) for Parryable attacks, Crimson (#FF2A2A) for Dodge-Only!
		const bool bParryable = IsCurrentAttackParryable();
		const FLinearColor TelegraphColor = bParryable
			? FLinearColor(1.0f, 0.68f, 0.16f, 1.0f)
			: FLinearColor(1.0f, 0.08f, 0.05f, 1.0f);

		const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		const float Progress = bCataclysmStillnessPhase
			? 1.0f
			: FMath::Clamp(1.0f - (TelegraphTimer / TELEGRAPH_DURATION), 0.f, 1.f);
		const float PulseFreq = bCataclysmStillnessPhase ? 28.f : 14.f;
		const float Pulse = 0.85f + 0.15f * FMath::Sin(TimeSec * PulseFreq);

		if (TelegraphRingMesh)
		{
			float WorldRadius = 480.f;
			switch (CurrentAttack)
			{
			case EBossAttackKind::CrimsonCleave:     WorldRadius = 480.f; break;
			case EBossAttackKind::SeismicSlam:       WorldRadius = 780.f; break;
			case EBossAttackKind::AbyssalFireVolley: WorldRadius = 400.f; break;
			case EBossAttackKind::TempestLunge:      WorldRadius = 560.f; break;
			case EBossAttackKind::ShatterSpin:       WorldRadius = 640.f; break;
			case EBossAttackKind::ChronoCataclysm:   WorldRadius = 1250.f; break;
			default: break;
			}
			const float ActorScale = FMath::Max(0.5f, GetActorScale3D().X);
			const float LocalScaleXY = (WorldRadius / 50.f) / ActorScale;
			TelegraphRingMesh->SetRelativeScale3D(FVector(LocalScaleXY, LocalScaleXY, 0.025f));
			TelegraphRingMesh->SetVisibility(true);
		}

		if (TelegraphCrimsonLight)
		{
			TelegraphCrimsonLight->SetLightColor(TelegraphColor);
			TelegraphCrimsonLight->SetIntensity((1800.f + 1200.f * Progress) * Pulse);
		}

		if (TelegraphCrimsonMID)
		{
			const float EmissiveBoost = FMath::Lerp(1.6f, 3.2f, Progress) * Pulse;
			const FLinearColor EmissiveCol = TelegraphColor * EmissiveBoost;
			TelegraphCrimsonMID->SetVectorParameterValue(TEXT("Color"), EmissiveCol);
			TelegraphCrimsonMID->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveCol);
		}

		// Keep chest orb hidden so SevarogBloodred's dark Gothic armor & spiked hammer stay 100% crisp and unobstructed
		if (TelegraphAuraCoreMesh)
		{
			TelegraphAuraCoreMesh->SetVisibility(false);
		}
	}
	else if (ImpactLightDecayTimer > 0.f)
	{
		if (TelegraphRingMesh)     { TelegraphRingMesh->SetVisibility(false); }
		if (TelegraphAuraCoreMesh) { TelegraphAuraCoreMesh->SetVisibility(false); }
		ImpactLightDecayTimer = FMath::Max(0.f, ImpactLightDecayTimer - DeltaTime);
		if (TelegraphCrimsonLight)
		{
			const float Alpha = FMath::Clamp(ImpactLightDecayTimer / 0.65f, 0.f, 1.f);
			TelegraphCrimsonLight->SetIntensity(ImpactLightPeakLumens * Alpha * Alpha);
		}
	}
	else
	{
		if (TelegraphRingMesh)     { TelegraphRingMesh->SetVisibility(false); }
		if (TelegraphAuraCoreMesh) { TelegraphAuraCoreMesh->SetVisibility(false); }
		if (TelegraphCrimsonLight)
		{
			// Subtle ominous sovereign aura (600 lm when engaged, 0 when dormant) so SevarogBloodred silhouette stays crisp
			const float TargetIdle = (bBossEngaged && !bDead) ? 600.f : 0.f;
			TelegraphCrimsonLight->SetLightColor(FLinearColor(0.95f, 0.10f, 0.08f));
			TelegraphCrimsonLight->SetIntensity(FMath::FInterpTo(TelegraphCrimsonLight->Intensity, TargetIdle, DeltaTime, 4.f));
		}
	}
}

void AAetheriaBossCharacter::UpdateEnemyAI(float DeltaTime)
{
	if (!HasAuthority() || bDead)
	{
		return;
	}

	// 1. Handle Stagger Break vulnerability window
	if (bStaggerBroken)
	{
		StaggerBreakTimer -= DeltaTime;
		StaggerGauge = FMath::Max(0.f, MaxStagger * (StaggerBreakTimer / FMath::Max(0.1f, StaggerBreakTotalDuration)));
		if (StaggerBreakTimer <= 0.f)
		{
			bStaggerBroken = false;
			StaggerGauge = 0.f;
		}
		return;
	}

	// 2. Engagement / Disengagement Hysteresis (3200 cm engage, 5400 cm clean disengage!)
	float NearestHeroDist = 0.f;
	AAetheriaHeroCharacter* TargetHero = FindNearestAliveHero(DISENGAGE_RADIUS, NearestHeroDist);

	if (!bBossEngaged)
	{
		if (TargetHero && NearestHeroDist <= ENGAGE_RADIUS)
		{
			bBossEngaged = true;
			BossAttackCooldown = 0.9f;
			if (UAetheriaMusicSubsystem* MusicSys = UAetheriaMusicSubsystem::Get(this))
			{
				MusicSys->SetBossEncounterActive(true);
			}
			UE_LOG(LogChronicles, Log, TEXT("%s engaged in Highkeep Citadel!"), *BossTitle);
		}
		else
		{
			return;
		}
	}
	else if (!TargetHero || NearestHeroDist > DISENGAGE_RADIUS)
	{
		// Hero left Highkeep Citadel — disengage cleanly so Boss Bar never sticks!
		bBossEngaged = false;
		if (UAetheriaMusicSubsystem* MusicSys = UAetheriaMusicSubsystem::Get(this))
		{
			MusicSys->SetBossEncounterActive(false);
		}
		bIsTelegraphing = false;
		bCataclysmStillnessPhase = false;
		CurrentAttack = EBossAttackKind::None;
		TelegraphTimer = 0.f;
		if (TelegraphRingMesh)     { TelegraphRingMesh->SetVisibility(false); }
		if (TelegraphAuraCoreMesh) { TelegraphAuraCoreMesh->SetVisibility(false); }
		return;
	}

	// 3. Phase 3 ChronoCataclysm "Stillness Before Climax" beat (0.45s eerie pause before detonation)
	if (bCataclysmStillnessPhase)
	{
		CataclysmStillnessTimer -= DeltaTime;
		if (CataclysmStillnessTimer <= 0.f)
		{
			bCataclysmStillnessPhase = false;
			ExecuteCurrentBossAttack();
		}
		return;
	}

	// 4. Active 0.95s Crimson Glow Telegraph countdown
	if (bIsTelegraphing)
	{
		TelegraphTimer -= DeltaTime;

		// Smoothly track hero facing during early half of telegraph
		if (TargetHero && TelegraphTimer > TELEGRAPH_DURATION * 0.45f)
		{
			const FVector ToHero = (TargetHero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			if (!ToHero.IsNearlyZero())
			{
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToHero.Rotation(), DeltaTime, 5.5f));
			}
		}

		if (TelegraphTimer <= 0.f)
		{
			if (CurrentAttack == EBossAttackKind::ChronoCataclysm)
			{
				bIsTelegraphing = false;
				bCataclysmStillnessPhase = true;
				CataclysmStillnessTimer = 0.45f;
			}
			else
			{
				ExecuteCurrentBossAttack();
			}
		}
		return;
	}

	// 5. Pursue hero & schedule next telegraphed attack
	if (BossAttackCooldown > 0.f)
	{
		BossAttackCooldown = FMath::Max(0.f, BossAttackCooldown - DeltaTime);
	}

	if (!bFrozen && TargetHero)
	{
		const FVector ToHero = (TargetHero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (NearestHeroDist > 360.f)
		{
			AddMovementInput(ToHero, 1.0f);
		}

		if (BossAttackCooldown <= 0.f && NearestHeroDist <= 1650.f)
		{
			const EBossAttackKind NextAttack = ChooseNextBossAttack(NearestHeroDist);
			BeginTelegraphedAttack(NextAttack, TargetHero);
		}
	}
}

void AAetheriaBossCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTelegraphVisuals(DeltaTime);
}

// ============================================================================
// 4. AAetheriaNPCActor — Named Story Quest Givers
// ============================================================================

AAetheriaNPCActor::AAetheriaNPCActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("NPCCapsule"));
	CapsuleComp->InitCapsuleSize(40.f, 92.f);
	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComp->SetCollisionResponseToChannel(CHRONICLE_TRACE_PROJECTILE, ECR_Ignore);
	SetRootComponent(CapsuleComp);

	NPCMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NPCMesh"));
	NPCMesh->SetupAttachment(CapsuleComp);
	NPCMesh->SetRelativeLocation(FVector(0.f, 0.f, -92.f));
	NPCMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	NPCMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Floating quest indicator beacon above NPC's head
	QuestBeaconMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("QuestBeaconMesh"));
	QuestBeaconMesh->SetupAttachment(CapsuleComp);
	QuestBeaconMesh->SetRelativeLocation(FVector(0.f, 0.f, 145.f));
	QuestBeaconMesh->SetRelativeRotation(FRotator(45.f, 0.f, 45.f));
	QuestBeaconMesh->SetRelativeScale3D(FVector(0.26f));
	QuestBeaconMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	QuestBeaconMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(AetheriaActorsUtil::PathCube);
	if (CubeFinder.Succeeded())
	{
		QuestBeaconMesh->SetStaticMesh(CubeFinder.Object);
	}

	// Subtle grounded warm lantern/quest accent light (260-420 lm so ground circles don't blow out)
	BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
	BeaconLight->SetupAttachment(QuestBeaconMesh);
	BeaconLight->SetIntensityUnits(ELightUnits::Lumens);
	BeaconLight->SetIntensity(340.f);
	BeaconLight->SetLightColor(FLinearColor(1.0f, 0.80f, 0.25f, 1.0f));
	BeaconLight->SetAttenuationRadius(680.f);
	BeaconLight->SetCastShadows(false);
}

void AAetheriaNPCActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAetheriaNPCActor, NpcId);
	DOREPLIFETIME(AAetheriaNPCActor, DisplayName);
	DOREPLIFETIME(AAetheriaNPCActor, RoleTitle);
	DOREPLIFETIME(AAetheriaNPCActor, QuestId);
}

void AAetheriaNPCActor::BeginPlay()
{
	Super::BeginPlay();

	BeaconMID = AetheriaActorsUtil::CreateColorMID(this, FLinearColor(9.0f, 6.8f, 1.6f, 1.0f), true);
	if (QuestBeaconMesh && BeaconMID)
	{
		QuestBeaconMesh->SetMaterial(0, BeaconMID);
	}

	if (!bMeshBound)
	{
		BindNPCMeshAndAnim();
	}

	FAetheriaStyleHarmonizer::HarmonizeActorMaterials(this, EChronicleStyleCategory::Character);
}

void AAetheriaNPCActor::ConfigureNPC(
	FName InNpcId,
	const FText& InDisplayName,
	const FText& InRoleTitle,
	FName InQuestId,
	int32 InVisualVariantIndex)
{
	NpcId = InNpcId;
	DisplayName = InDisplayName;
	RoleTitle = InRoleTitle;
	QuestId = InQuestId;
	VisualVariantIndex = InVisualVariantIndex;

	BindNPCMeshAndAnim();
}

void AAetheriaNPCActor::BindNPCMeshAndAnim()
{
	if (!NPCMesh)
	{
		return;
	}

	static const TCHAR* NPCMeshPool[6] = {
		TEXT("/Game/Characters/Heroes/SKM_Hero_01.SKM_Hero_01"),
		TEXT("/Game/Characters/Heroes/SKM_Hero_02.SKM_Hero_02"),
		TEXT("/Game/Characters/Heroes/SKM_Hero_03.SKM_Hero_03"),
		TEXT("/Game/Characters/Heroes/SKM_Hero_04.SKM_Hero_04"),
		TEXT("/Game/Characters/Heroes/SKM_Hero_05.SKM_Hero_05"),
		AetheriaActorsUtil::PathQuinnSimple
	};

	const uint32 HashSeed = (VisualVariantIndex > 0)
		? static_cast<uint32>(VisualVariantIndex)
		: (NpcId.IsNone() ? 0u : GetTypeHash(NpcId));
	const int32 PoolIdx = static_cast<int32>(HashSeed % 6u);

	USkeletalMesh* ChosenMesh = LoadObject<USkeletalMesh>(nullptr, NPCMeshPool[PoolIdx]);
	if (!ChosenMesh)
	{
		ChosenMesh = LoadObject<USkeletalMesh>(nullptr, AetheriaActorsUtil::PathQuinnSimple);
	}
	if (!ChosenMesh)
	{
		ChosenMesh = LoadObject<USkeletalMesh>(nullptr, AetheriaActorsUtil::PathMannySimple);
	}

	if (ChosenMesh)
	{
		NPCMesh->SetSkeletalMesh(ChosenMesh);
		AetheriaActorsUtil::TryBindCompatibleAnimBP(NPCMesh, AetheriaActorsUtil::PathABPUnarmed, AetheriaActorsUtil::PathABPCombat);
		bMeshBound = true;
	}
}

void AAetheriaNPCActor::UpdateQuestBeacon(float DeltaTime)
{
	BeaconBobTime += DeltaTime;

	if (QuestBeaconMesh && QuestBeaconMesh->IsVisible())
	{
		const float BobZ = 146.f + FMath::Sin(BeaconBobTime * 2.8f) * 8.f;
		QuestBeaconMesh->SetRelativeLocation(FVector(0.f, 0.f, BobZ));
		QuestBeaconMesh->AddRelativeRotation(FRotator(0.f, DeltaTime * 68.f, 0.f));
	}

	QuestCheckTimer -= DeltaTime;
	if (QuestCheckTimer > 0.f || QuestId.IsNone())
	{
		return;
	}
	QuestCheckTimer = 0.45f;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (const UQuestSubsystem* QuestSys = GI->GetSubsystem<UQuestSubsystem>())
		{
			if (const FQuestDef* Quest = QuestSys->FindQuest(QuestId))
			{
				const bool bVisible = (Quest->State != EQuestState::Completed);
				if (QuestBeaconMesh)
				{
					QuestBeaconMesh->SetVisibility(bVisible);
				}

				FLinearColor BeaconColor(9.0f, 6.8f, 1.4f, 1.0f);
				float LightLm = 340.f;

				switch (Quest->State)
				{
				case EQuestState::Available:
					BeaconColor = FLinearColor(10.0f, 7.2f, 1.4f, 1.0f);
					LightLm = 360.f;
					break;
				case EQuestState::Active:
					BeaconColor = FLinearColor(4.5f, 5.2f, 6.2f, 1.0f);
					LightLm = 260.f;
					break;
				case EQuestState::ReadyToTurnIn:
					BeaconColor = FLinearColor(2.4f, 11.0f, 4.5f, 1.0f);
					LightLm = 420.f;
					break;
				case EQuestState::Completed:
					LightLm = 0.f;
					break;
				}

				if (BeaconMID)
				{
					BeaconMID->SetVectorParameterValue(TEXT("Color"), BeaconColor);
					BeaconMID->SetVectorParameterValue(TEXT("EmissiveColor"), BeaconColor);
				}
				if (BeaconLight)
				{
					BeaconLight->SetLightColor(BeaconColor.GetClamped(0.f, 1.f));
					BeaconLight->SetIntensity(LightLm);
				}
			}
		}
	}
}

void AAetheriaNPCActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateQuestBeacon(DeltaTime);

	// Smoothly turn to face nearest Hero within 900 cm so villagers feel alive
	if (UWorld* World = GetWorld())
	{
		AAetheriaHeroCharacter* NearestHero = nullptr;
		float BestDistSq = FMath::Square(900.f);
		const FVector MyLoc = GetActorLocation();

		for (TActorIterator<AAetheriaHeroCharacter> It(World); It; ++It)
		{
			AAetheriaHeroCharacter* Hero = *It;
			if (!Hero || Hero->bDead)
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(MyLoc, Hero->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				NearestHero = Hero;
			}
		}

		if (NearestHero)
		{
			const FVector ToHero = (NearestHero->GetActorLocation() - MyLoc).GetSafeNormal2D();
			if (!ToHero.IsNearlyZero())
			{
				const FRotator TargetYawRot(0.f, ToHero.Rotation().Yaw, 0.f);
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetYawRot, DeltaTime, 4.5f));
			}
		}
	}
}
