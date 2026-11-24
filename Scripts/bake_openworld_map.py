import unreal

TARGET_MAP = "/Game/Maps/AetheriaChroniclesWorld"
SRC_MAP = "/Engine/Maps/Templates/OpenWorld"
MAT_CANDIDATES = [
    "/Game/MWLandscapeAutoMaterial/Materials/Landscape/MTL_MWAM_Landscape_IslandExample",
    "/Game/MedievalDungeon/Materials/M_Dirt_Mounds_Inst",
    "/Game/MedievalDungeon/Materials/M_Stone_Altar_Inst",
]

mat = None
for path in MAT_CANDIDATES:
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mat = unreal.load_asset(path)
        if mat:
            unreal.log(f"[BakeMap] Loaded landscape material: {path}")
            break

if not mat:
    raise RuntimeError("[BakeMap] No valid landscape material found!")

# Load template map and save a project-local copy first so OFPA external actors live under /Game/
world = unreal.EditorLoadingAndSavingUtils.load_map(SRC_MAP)
if not world:
    raise RuntimeError(f"[BakeMap] Failed to load {SRC_MAP}")

unreal.EditorAssetLibrary.make_directory("/Game/Maps")
saved = unreal.EditorLoadingAndSavingUtils.save_map(world, TARGET_MAP)
unreal.log(f"[BakeMap] Saved initial copy to {TARGET_MAP}: {saved}")

# Reload our new project map so all external actor packages belong to /Game/Maps/AetheriaChroniclesWorld
world = unreal.EditorLoadingAndSavingUtils.load_map(TARGET_MAP)
if not world:
    raise RuntimeError(f"[BakeMap] Failed to reload {TARGET_MAP}")

# Load all World Partition cells if available so every LandscapeStreamingProxy gets updated
try:
    wp_sub = unreal.get_editor_subsystem(unreal.WorldPartitionEditorSubsystem)
    if wp_sub:
        unreal.log("[BakeMap] WorldPartitionEditorSubsystem available")
except Exception as e:
    unreal.log_warning(f"[BakeMap] WorldPartitionEditorSubsystem note: {e}")

# Load any unloaded external actor packages under __ExternalActors__/Maps/AetheriaChroniclesWorld
ea_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = ea_sub.get_all_level_actors() if ea_sub else []
unreal.log(f"[BakeMap] Loaded level actors count: {len(all_actors)}")

# Also try forcing World Partition loader bounds via EditorWorldExtension / ActorDesc loading
try:
    asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
    ext_assets = asset_reg.get_assets_by_path("/Game/__ExternalActors__/Maps/AetheriaChroniclesWorld", recursive=True)
    unreal.log(f"[BakeMap] External actor packages found: {len(ext_assets)}")
    for a in ext_assets:
        try:
            a.get_asset()
        except Exception:
            pass
    all_actors = ea_sub.get_all_level_actors() if ea_sub else all_actors
    unreal.log(f"[BakeMap] Level actors after external load: {len(all_actors)}")
except Exception as e:
    unreal.log_warning(f"[BakeMap] External actor scan note: {e}")

patched_proxies = 0
for actor in all_actors:
    cls_name = actor.get_class().get_name()
    if "Landscape" in cls_name:
        try:
            actor.modify()
            actor.set_editor_property("landscape_material", mat)
            # Clear any per-component override_material so it inherits landscape_material cleanly
            for comp in actor.get_components_by_class(unreal.PrimitiveComponent):
                if "LandscapeComponent" in comp.get_class().get_name():
                    try:
                        comp.modify()
                        comp.set_editor_property("override_material", None)
                    except Exception:
                        pass
            patched_proxies += 1
            unreal.log(f"[BakeMap] Patched {cls_name}: {actor.get_name()}")
        except Exception as e:
            unreal.log_warning(f"[BakeMap] Failed patching {actor.get_name()}: {e}")

# Also iterate ALL loaded UObjects of ALandscapeProxy in memory in case external packages loaded outside editor actor list
try:
    for obj in unreal.ObjectIterator(unreal.LandscapeProxy):
        if obj and obj.get_outer():
            try:
                obj.modify()
                obj.set_editor_property("landscape_material", mat)
                patched_proxies += 1
            except Exception:
                pass
except Exception:
    pass

unreal.log(f"[BakeMap] Total landscape proxies patched: {patched_proxies}")

# Save map + all dirty external actor packages!
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
saved2 = unreal.EditorLoadingAndSavingUtils.save_current_level()
unreal.log(f"[BakeMap] Final save_current_level={saved2}")
