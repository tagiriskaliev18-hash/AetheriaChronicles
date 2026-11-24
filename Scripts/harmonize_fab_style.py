# ============================================================================
# Scripts/harmonize_fab_style.py — Unified "Crimson Highland" Texture & Material
# Style Harmonizer for UE 5.8.4 (run via Tools -> Execute Python Script).
#
# Prevents "asset-flip patchwork" when mixing Fab packs, Paragon heroes,
# Quixel Megascans, and staged Aetheria meshes by enforcing ONE coherent
# Crimson Desert PBR palette:
#   - Earthy muted albedo (no neon/chalky outlier textures)
#   - Grounded physical roughness ranges (stone 0.80-0.92, wood 0.82-0.92,
#     worn armor steel 0.38-0.52, foliage 0.72-0.86)
#   - High saturation reserved strictly for elemental magic, fire & torches.
# ============================================================================
import unreal

TARGET_ROOTS = [
    "/Game/FabImports",
    "/Game/FourEvilDragonsHP",
    "/Game/Whisper",
    "/Game/ParagonRampage",
    "/Game/ParagonGideon",
    "/Game/ParagonSevarog",
    "/Game/ParagonGreystone",
    "/Game/ParagonKwang",
    "/Game/ParagonSerath",
    "/Game/ParagonKhaimera",
    "/Game/ParagonMinions",
    "/Game/ParagonAurora",
    "/Game/Meshes",
    "/Game/Characters",
]

# Crimson Highland PBR target presets
STYLE_RULES = {
    "stone":   {"rough_min": 0.78, "rough_max": 0.92, "desat": 0.25, "tint": unreal.LinearColor(0.84, 0.85, 0.88, 1.0)},
    "wood":    {"rough_min": 0.80, "rough_max": 0.92, "desat": 0.22, "tint": unreal.LinearColor(0.82, 0.76, 0.70, 1.0)},
    "foliage": {"rough_min": 0.70, "rough_max": 0.86, "desat": 0.30, "tint": unreal.LinearColor(0.76, 0.84, 0.78, 1.0)},
    "metal":   {"rough_min": 0.36, "rough_max": 0.56, "desat": 0.18, "tint": unreal.LinearColor(0.86, 0.88, 0.92, 1.0)},
    "default": {"rough_min": 0.68, "rough_max": 0.88, "desat": 0.20, "tint": unreal.LinearColor(0.86, 0.86, 0.87, 1.0)},
}

def classify_asset(name_lower: str) -> str:
    if any(k in name_lower for k in ("stone", "rock", "cliff", "wall", "castle", "brick", "cobble", "pillar", "statue")):
        return "stone"
    if any(k in name_lower for k in ("wood", "bark", "plank", "timber", "barrel", "crate", "tree_trunk")):
        return "wood"
    if any(k in name_lower for k in ("leaf", "leaves", "grass", "foliage", "moss", "pine", "bush")):
        return "foliage"
    if any(k in name_lower for k in ("iron", "steel", "armor", "sword", "mace", "gate", "metal", "brazier")):
        return "metal"
    return "default"

def is_magic_or_emissive(name_lower: str) -> bool:
    return any(k in name_lower for k in ("emissive", "flame", "fire", "lava", "spell", "fx", "orb", "rune", "glow", "crystal"))

def harmonize_all():
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    mel = unreal.MaterialEditingLibrary
    adjusted_mi = 0
    adjusted_tex = 0

    for root in TARGET_ROOTS:
        if not unreal.EditorAssetLibrary.does_directory_exist(root):
            continue
        assets = reg.get_assets_by_path(root, recursive=True)
        for ad in assets:
            cls = str(ad.asset_class_path.asset_name)
            pkg = str(ad.package_name)
            low = pkg.lower()

            if is_magic_or_emissive(low):
                continue

            if cls == "MaterialInstanceConstant":
                mi = unreal.load_asset(pkg)
                if not mi:
                    continue
                cat = classify_asset(low)
                rule = STYLE_RULES[cat]
                changed = False

                # Clamp overly glossy plastic roughness parameters
                for param in ("Roughness", "RoughnessMin", "RoughnessScale", "RoughnessMultiplier"):
                    try:
                        val = mel.get_material_instance_scalar_parameter_value(mi, param)
                        clamped = max(rule["rough_min"], min(rule["rough_max"], val))
                        if abs(clamped - val) > 0.02:
                            mel.set_material_instance_scalar_parameter_value(mi, param, clamped)
                            changed = True
                    except Exception:
                        pass

                # Gently harmonize loud outlier albedo tints toward Crimson Highland palette
                for param in ("BaseColorTint", "AlbedoTint", "Tint", "Color"):
                    try:
                        mel.get_material_instance_vector_parameter_value(mi, param)
                        mel.set_material_instance_vector_parameter_value(mi, param, rule["tint"])
                        changed = True
                    except Exception:
                        pass

                if changed:
                    mel.update_material_instance(mi)
                    unreal.EditorAssetLibrary.save_loaded_asset(mi, only_if_is_dirty=True)
                    adjusted_mi += 1

            elif cls == "Texture2D":
                # Keep diffuse/albedo textures within natural PBR saturation & brightness bounds
                if any(k in low for k in ("normal", "_nor", "_nrm", "rough", "metal", "ao", "mask", "orm")):
                    continue
                tex = unreal.load_asset(pkg)
                if not tex:
                    continue
                # Enforce consistent streaming & subtle Crimson Highland saturation harmony on outlier albedo maps
                cat = classify_asset(low)
                target_sat = 1.0 - STYLE_RULES[cat]["desat"]
                if abs(tex.get_editor_property("adjust_saturation") - target_sat) > 0.05:
                    tex.set_editor_property("adjust_saturation", target_sat)
                    tex.set_editor_property("adjust_brightness", 0.92)
                    tex.set_editor_property("never_stream", False)
                    unreal.EditorAssetLibrary.save_loaded_asset(tex, only_if_is_dirty=True)
                    adjusted_tex += 1

    unreal.log(f"[StyleHarmonizer] Crimson Highland pass complete: harmonized {adjusted_mi} MaterialInstances & {adjusted_tex} Albedo Textures (all streaming enabled).")

if __name__ == "__main__":
    harmonize_all()
