# Scripts/tools/inspect_static.py - static mesh audit only (fast). Output: Saved/AetheriaAudit/static.json
import unreal, json, os

OUT = unreal.Paths.project_saved_dir() + "AetheriaAudit/static.json"
os.makedirs(os.path.dirname(OUT), exist_ok=True)
reg = unreal.AssetRegistryHelpers.get_asset_registry()
reg.scan_paths_synchronous(["/Game"], True)
out = []
for a in reg.get_all_assets(include_only_on_disk_assets=True):
    if str(a.asset_class_path.asset_name) != "StaticMesh":
        continue
    p = str(a.package_name)
    try:
        m = unreal.load_asset(p)
        b = m.get_bounds().box_extent
        mats = []
        for sml in m.static_materials:
            mi = sml.material_interface
            mats.append(str(mi.get_path_name()) if mi else None)
        coll = {}
        bs = m.get_editor_property("body_setup")
        if bs:
            agg = bs.get_editor_property("agg_geom")
            if agg:
                for k in ("box_elems", "sphere_elems", "sphyl_elems", "convex_elems", "tapered_capsule_elems"):
                    try:
                        coll[k.split("_")[0]] = len(agg.get_editor_property(k))
                    except Exception:
                        pass
            try:
                coll["cts"] = int(bs.get_editor_property("collision_trace_flag"))
            except Exception:
                try:
                    coll["cts"] = int(bs.get_editor_property("collision_trace_flag").value)
                except Exception:
                    coll["cts"] = str(bs.get_editor_property("collision_trace_flag"))
        ns = m.get_editor_property("nanite_settings")
        nl = m.get_num_lods()
        out.append({
            "path": p, "ext": [round(b.x), round(b.y), round(b.z)], "materials": mats, "collision": coll,
            "nanite": bool(ns.enabled) if ns else False,
            "tris": (m.get_num_triangles(0) if nl > 0 else 0), "lods": nl,
        })
        try:
            socks = []
            for i in range(64):
                s = m.get_socket_by_index(i)
                if not s:
                    break
                socks.append(str(s.socket_name))
            out[-1]["sockets"] = socks
        except Exception:
            pass
    except Exception as e:
        out.append({"path": p, "error": str(e)[:200]})
json.dump(out, open(OUT, "w", encoding="utf-8"), indent=0)
unreal.log("[Static] wrote %d meshes to %s" % (len(out), OUT))
