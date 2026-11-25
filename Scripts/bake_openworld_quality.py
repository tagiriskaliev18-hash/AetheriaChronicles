"""
Crimson Highland landscape bake (EDITOR-ONLY — run inside UE 5.8 Editor: Tools -> Execute Python Script).

Creates/overwrites /Game/Maps/MI_AetheriaCrimsonHighlandLandscape as a MaterialInstanceConstant
parented to the proven MWAM IslandExample auto-material (same family as the shipped
MI_AetheriaEmeraldMeadowLandscape), with a warmer Crimson Highland grade and stronger
slope-rock coverage so cliffs and scree read like Crimson Desert rather than green lawn.

This script:
  1. Writes the MIC to disk.
  2. Sets it as landscape_material on every LandscapeProxy in the currently loaded world.
  3. Saves the map. The game boots unchanged until you run this.

Nothing here mutates landscape at runtime; do not call from game code.
Re-running is safe (idempotent): the MIC is deleted and recreated each run.
"""

import unreal

TARGET_MAP = "/Game/Maps/AetheriaChroniclesWorld"
PARENT_MAT_PATH = "/Game/MWLandscapeAutoMaterial/Materials/Landscape/MTL_MWAM_Landscape_IslandExample"
CUSTOM_MIC_PATH = "/Game/Maps/MI_AetheriaCrimsonHighlandLandscape"

parent_mat = unreal.load_asset(PARENT_MAT_PATH)
if not parent_mat:
    raise RuntimeError(f"[BakeCrimsonHighland] Missing parent material {PARENT_MAT_PATH}")

unreal.EditorAssetLibrary.make_directory("/Game/Maps")
if unreal.EditorAssetLibrary.does_asset_exist(CUSTOM_MIC_PATH):
    unreal.EditorAssetLibrary.delete_asset(CUSTOM_MIC_PATH)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.MaterialInstanceConstantFactoryNew()
mic = asset_tools.create_asset("MI_AetheriaCrimsonHighlandLandscape", "/Game/Maps",
                               unreal.MaterialInstanceConstant, factory)
unreal.MaterialEditingLibrary.set_material_instance_parent(mic, parent_mat)

# Height/slope blend kept compatible with the shipped emerald-meadow bake, graded warmer.
exact_scalars = {
    "MW_WaterMaxWorldPosition": -8500.0,
    "MW_DirtMaxWorldPosition": -6500.0,
    "MW_GrassMaxWorldPosition": 14500.0,
    "MW_SnowWorldPosition": 9200.0,
    "MW_GrassSlopePower": 3.4,      # slightly tighter grass on slopes
    "MW_StonesSlopePower": 2.6,     # MORE exposed rock on slopes (Crimson Desert scree)
    "MW_GrassRoughness": 0.94,
    "MW_GrassSpecular": 0.15,
    "MW_StonesSpecular": 0.20,
    "MW_RockSpecular": 0.23,
    "MW_DirtSpecular": 0.17,
}
for k, v in exact_scalars.items():
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mic, k, v)

# Warm Crimson Highland grade: sun-baked grass, dusty stone, honeyed dirt.
exact_vectors = {
    "MW_GrassColorCorrection": unreal.LinearColor(0.92, 1.10, 0.58, 1.0),
    "MW_StonesColorCorrection": unreal.LinearColor(0.88, 0.88, 0.80, 1.0),
    "MW_DirtColorCorrection": unreal.LinearColor(1.02, 1.00, 0.78, 1.0),
    "MW_RockColorCorrection": unreal.LinearColor(0.95, 0.92, 0.86, 1.0),
}
for k, v in exact_vectors.items():
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mic, k, v)

unreal.MaterialEditingLibrary.update_material_instance(mic)
unreal.EditorAssetLibrary.save_loaded_asset(mic)

world = unreal.EditorLoadingAndSavingUtils.load_map(TARGET_MAP)
asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
ext_assets = asset_reg.get_assets_by_path("/Game/__ExternalActors__/Maps/AetheriaChroniclesWorld", recursive=True)
for a in ext_assets:
    try:
        a.get_asset()
    except Exception:
        pass

patched = 0
for obj in unreal.ObjectIterator(unreal.LandscapeProxy):
    if obj and obj.get_outer():
        try:
            obj.modify()
            obj.set_editor_property("landscape_material", mic)
            patched += 1
        except Exception:
            pass

unreal.log(f"[BakeCrimsonHighland] Baked warm Crimson Highland landscape MIC onto {patched} landscape proxies!")
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.EditorLoadingAndSavingUtils.save_current_level()
