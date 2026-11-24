import unreal

TARGET_MAP = "/Game/Maps/AetheriaChroniclesWorld"
PARENT_MAT_PATH = "/Game/MWLandscapeAutoMaterial/Materials/Landscape/MTL_MWAM_Landscape_IslandExample"
CUSTOM_MIC_PATH = "/Game/Maps/MI_AetheriaEmeraldMeadowLandscape"

parent_mat = unreal.load_asset(PARENT_MAT_PATH)
if not parent_mat:
    raise RuntimeError(f"[BakeGreenMap] Missing parent material {PARENT_MAT_PATH}")

unreal.EditorAssetLibrary.make_directory("/Game/Maps")
if unreal.EditorAssetLibrary.does_asset_exist(CUSTOM_MIC_PATH):
    unreal.EditorAssetLibrary.delete_asset(CUSTOM_MIC_PATH)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.MaterialInstanceConstantFactoryNew()
mic = asset_tools.create_asset("MI_AetheriaEmeraldMeadowLandscape", "/Game/Maps", unreal.MaterialInstanceConstant, factory)
unreal.MaterialEditingLibrary.set_material_instance_parent(mic, parent_mat)

# Exact MWLandscapeAutoMaterial parameters verified from asset dump:
# Previously MW_GrassMaxWorldPosition was -820.0 (causing Z=0 valley to be 100% brown stones!)
exact_scalars = {
    "MW_WaterMaxWorldPosition": -8500.0,
    "MW_DirtMaxWorldPosition": -6500.0,
    "MW_GrassMaxWorldPosition": 14500.0,
    "MW_SnowWorldPosition": 9200.0,
    "MW_GrassSlopePower": 3.2,
    "MW_StonesSlopePower": 1.8,
    "MW_GrassRoughness": 0.92,
    "MW_GrassSpecular": 0.16,
    "MW_StonesSpecular": 0.22,
    "MW_RockSpecular": 0.24,
    "MW_DirtSpecular": 0.18,
}
for k, v in exact_scalars.items():
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mic, k, v)

exact_vectors = {
    "MW_GrassColorCorrection": unreal.LinearColor(0.84, 1.18, 0.72, 1.0),
    "MW_StonesColorCorrection": unreal.LinearColor(0.80, 0.94, 0.74, 1.0),
    "MW_DirtColorCorrection": unreal.LinearColor(0.95, 1.05, 0.85, 1.0),
    "MW_RockColorCorrection": unreal.LinearColor(0.92, 0.90, 0.88, 1.0),
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

unreal.log(f"[BakeGreenMap] Cleanly baked MW_GrassMaxWorldPosition=14500 onto {patched} landscape proxies!")
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.EditorLoadingAndSavingUtils.save_current_level()
