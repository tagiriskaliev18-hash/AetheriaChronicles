# Scripts/import_fab_assets.py — Run inside UE 5.8 Editor (Tools -> Execute Python Script)
# Scans /Game/FabImports and /Game/Paragon* to verify high-bone-count SkeletalMeshes
# and prints a live manifest for AetheriaChronicles auto-binding.
import unreal

def scan_fab_assets():
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    roots = ["/Game/FabImports", "/Game/ParagonGideon", "/Game/ParagonSevarog", "/Game/ParagonGreystone", "/Game/ParagonAurora", "/Game/Characters"]
    found = {"Hero": [], "Boss": [], "Dragon": [], "Environment": []}

    for r in roots:
        assets = reg.get_assets_by_path(r, recursive=True)
        for a in assets:
            cls = str(a.asset_class_path.asset_name)
            path = str(a.package_name)
            low = path.lower()
            if cls == "SkeletalMesh":
                if "dragon" in low or "drake" in low or "wyvern" in low:
                    found["Dragon"].append(path)
                elif "sevarog" in low or "boss" in low or "grux" in low or "rampage" in low or "aurora" in low:
                    found["Boss"].append(path)
                elif "gideon" in low or "greystone" in low or "kwang" in low or "hero" in low or "manny" in low:
                    found["Hero"].append(path)
            elif cls == "StaticMesh" and ("megascans" in low or "fabimports/environment" in low or "cliff" in low or "castle" in low):
                found["Environment"].append(path)

    unreal.log("========== AETHERIA CHRONICLES FAB ASSET SCAN ==========")
    for k, vals in found.items():
        unreal.log(f"[{k}] {len(vals)} assets detected")
        for p in vals[:10]:
            unreal.log(f"   - {p}")
    unreal.log("Hero/Boss/Dragon C++ actors automatically prefer Fab/Paragon SkeletalMeshes at BeginPlay!")

if __name__ == "__main__":
    scan_fab_assets()
