# Scripts/tools/inventory_assets.py — headless asset audit.
# Run: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file> -unattended -nopause -NullRHI -NoSound
# Writes Saved/AetheriaAudit/inventory.json with every asset grouped by class, plus
# skeleton bindings for skeletal meshes / anim sequences and collision info for static meshes.
import unreal, json, os, time

OUT_DIR = unreal.Paths.project_saved_dir() + "AetheriaAudit"
os.makedirs(OUT_DIR, exist_ok=True)
T0 = time.time()
reg = unreal.AssetRegistryHelpers.get_asset_registry()
reg.scan_paths_synchronous(["/Game"], True)
assets = reg.get_all_assets(include_only_on_disk_assets=True)
unreal.log(f"[Inventory] {len(assets)} assets on disk")

by_class = {}
for a in assets:
    cls = str(a.asset_class_path.asset_name)
    by_class.setdefault(cls, []).append(str(a.package_name))

summary = {k: len(v) for k, v in sorted(by_class.items(), key=lambda kv: -len(kv[1]))}
unreal.log("[Inventory] classes: " + json.dumps(summary))

# Per-top-folder counts
folders = {}
for cls, paths in by_class.items():
    for p in paths:
        top = p.split("/")[2] if p.count("/") >= 2 else "?"
        key = (top, cls)
        folders[key] = folders.get(key, 0) + 1

skel_meshes = []
for p in by_class.get("SkeletalMesh", []):
    try:
        m = unreal.load_asset(p)
        if not m:
            skel_meshes.append({"path": p, "error": "load failed"})
            continue
        skel = m.get_editor_property("skeleton")
        mats = []
        for i in range(m.get_num_materials()):
            mi = m.get_material(i) if hasattr(m, "get_material") else None
            mi = mi if mi else m.static_materials[i].material_interface
            mi = mi or m.static_materials[i].material_interface
            mats.append(str(mi.get_path_name()) if mi else None)
        skel_meshes.append({
            "path": p,
            "name": m.get_name(),
            "skeleton": str(skel.get_path_name()) if skel else None,
            "materials": mats,
            "bounds": [m.get_bounds().box_extent.x, m.get_bounds().box_extent.y, m.get_bounds().box_extent.z],
        })
    except Exception as e:
        skel_meshes.append({"path": p, "error": str(e)[:200]})

anim_lib = unreal.AnimationLibrary
anims = []
for p in by_class.get("AnimSequence", []):
    try:
        s = unreal.load_asset(p)
        if not s:
            anims.append({"path": p, "error": "load failed"})
            continue
        skel = s.get_editor_property("skeleton")
        try:
            add = anim_lib.get_animation_additive_settings(s)
            add_type = str(add.additive_anim_type)
            ref_type = str(add.ref_pose_type)
            ref_seq = str(add.ref_pose_seq.get_path_name()) if add.ref_pose_seq else None
        except Exception:
            add_type = ref_type = ref_seq = "n/a"
        try:
            length = anim_lib.get_animation_length(s)
            frames = anim_lib.get_num_frames(s)
        except Exception:
            length = frames = -1
        try:
            curve_names = [str(c) for c in anim_lib.get_animation_curve_names(s)]
        except Exception:
            curve_names = []
        anims.append({
            "path": p, "name": s.get_name(),
            "skeleton": str(skel.get_path_name()) if skel else None,
            "additive": add_type, "ref_pose_type": ref_type, "ref_pose": ref_seq,
            "length": round(length, 3), "frames": frames, "curves": curve_names[:40],
        })
    except Exception as e:
        anims.append({"path": p, "error": str(e)[:200]})

static_meshes = []
for p in by_class.get("StaticMesh", []) :
    try:
        m = unreal.load_asset(p)
        if not m:
            static_meshes.append({"path": p, "error": "load failed"})
            continue
        ext = m.get_bounds().box_extent
        mats = []
        n = m.get_num_materials()
        for i in range(n):
            try:
                mi = m.static_materials[i].material_interface
                mats.append(str(mi.get_path_name()) if mi else None)
            except Exception:
                mats.append(None)
        bs = m.get_editor_property("body_setup")
        agg = bs.get_editor_property("agg_geom") if bs else None
        coll = {}
        if agg:
            for k in ("box_elems", "sphere_elems", "sphyl_elems", "convex_elems", "tapered_capsule_elems"):
                try:
                    coll[k] = len(agg.get_editor_property(k))
                except Exception:
                    coll[k] = -1
        cs = m.get_editor_property("complex_as_simple_collision_enabled") if False else None
        static_meshes.append({
            "path": p, "name": m.get_name(), "ext": [round(ext.x), round(ext.y), round(ext.z)],
            "materials": mats, "collision": coll,
            "nanite": bool(m.get_editor_property("nanite_settings").enabled) if m.get_editor_property("nanite_settings") else False,
        })
    except Exception as e:
        static_meshes.append({"path": p, "error": str(e)[:200]})

result = {
    "summary": summary,
    "folders": {f"{k[0]}|{k[1]}": v for k, v in sorted(folders.items())},
    "skeletal_meshes": skel_meshes,
    "anim_sequences": anims,
    "static_meshes": static_meshes,
    "other": {k: v for k, v in by_class.items() if k not in ("SkeletalMesh", "AnimSequence", "StaticMesh")},
}
out = os.path.join(OUT_DIR, "inventory.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(result, f, ensure_ascii=False, indent=1)
unreal.log(f"[Inventory] wrote {out} in {time.time()-T0:.1f}s")
