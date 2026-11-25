# Scripts/tools/inspect_deep.py - deep read-only inspection: map/landscape, AnimBP graphs, sockets, anim additive types.
# Run: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file> -unattended -nopause -NullRHI -NoSound
# Output: Saved/AetheriaAudit/deep.json
import unreal, json, os, traceback

OUT = unreal.Paths.project_saved_dir() + "AetheriaAudit"
os.makedirs(OUT, exist_ok=True)
res = {}


def L(*a):
    unreal.log("[Deep] " + " ".join(str(x) for x in a))


def _ok(o, k):
    try:
        o.get_editor_property(k)
        return True
    except Exception:
        return False


reg = unreal.AssetRegistryHelpers.get_asset_registry()
reg.scan_paths_synchronous(["/Game"], True)
by_class = {}
for a in reg.get_all_assets(include_only_on_disk_assets=True):
    by_class.setdefault(str(a.asset_class_path.asset_name), []).append(str(a.package_name))

# ---------- A. static meshes ----------
sm_out = []
for p in by_class.get("StaticMesh", []):
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
            coll["cts"] = int(bs.get_editor_property("collision_trace_flag"))
        ns = m.get_editor_property("nanite_settings")
        nl = m.get_num_lods()
        sm_out.append({
            "path": p, "ext": [round(b.x), round(b.y), round(b.z)], "materials": mats, "collision": coll,
            "nanite": bool(ns.enabled) if ns else False,
            "tris": (m.get_num_triangles(0) if nl > 0 else 0), "lods": nl,
            "sockets": [str(m.get_socket_by_index(i).socket_name) for i in range(m.num_sockets)],
        })
    except Exception as e:
        sm_out.append({"path": p, "error": str(e)[:160]})
res["static_meshes"] = sm_out
L("static meshes", len(sm_out))

# ---------- B. skeletal meshes ----------
sk_out = []
for p in by_class.get("SkeletalMesh", []):
    try:
        m = unreal.load_asset(p)
        b = m.get_bounds().box_extent
        socks = []
        for i in range(m.num_sockets):
            s = m.get_socket_by_index(i)
            socks.append([str(s.socket_name), str(s.bone_name)])
        skel = m.get_editor_property("skeleton")
        pa = m.get_editor_property("physics_asset")
        sk_out.append({
            "path": p, "ext": [round(b.x), round(b.y), round(b.z)],
            "skeleton": str(skel.get_path_name()) if skel else None,
            "materials": [str(x.get_path_name()) if x else None for x in m.materials],
            "physics_asset": str(pa.get_path_name()) if pa else None,
            "sockets": socks,
        })
    except Exception as e:
        sk_out.append({"path": p, "error": str(e)[:160]})
res["skeletal_meshes"] = sk_out
L("skeletal meshes", len(sk_out))

# ---------- C. anim sequences ----------
AL = unreal.AnimationLibrary
an_out = []
for p in by_class.get("AnimSequence", []):
    try:
        s = unreal.load_asset(p)
        sk = s.get_skeleton()
        d = {"path": p, "skel": str(sk.get_name()) if sk else None,
             "len": round(AL.get_sequence_length(s), 3), "frames": AL.get_num_frames(s)}
        for k, f in (("add", AL.get_additive_animation_type), ("base", AL.get_additive_base_pose_type),
                     ("rm", AL.is_root_motion_enabled)):
            try:
                d[k] = str(f(s)).split(".")[-1]
            except Exception:
                d[k] = "n/a"
        try:
            d["curves"] = [str(c) for c in AL.get_animation_curve_names(s)][:30]
        except Exception:
            d["curves"] = []
        an_out.append(d)
    except Exception as e:
        an_out.append({"path": p, "error": str(e)[:160]})
res["anim_sequences"] = an_out
L("anim sequences", len(an_out))

# ---------- D. montages ----------
mo = []
for p in by_class.get("AnimMontage", []):
    try:
        m = unreal.load_asset(p)
        mo.append({"path": p, "slots": [str(x) for x in AL.get_montage_slot_names(m)],
                   "len": round(AL.get_sequence_length(m), 3),
                   "sections": [str(m.get_section_name(i)) for i in range(m.get_num_sections())]})
    except Exception as e:
        mo.append({"path": p, "error": str(e)[:160]})
res["montages"] = mo


# ---------- E. AnimBP graphs ----------
def title_of(n):
    for fn in (lambda: n.get_node_title(unreal.NodeTitleType.FULL_TITLE),
               lambda: n.get_node_title(2),
               lambda: n.get_editor_property("node_title")):
        try:
            return str(fn())
        except Exception:
            continue
    return "?"


def dump_graph(g, depth, out, seen):
    if g in seen:
        return
    seen.add(g)
    try:
        nodes = g.get_nodes()
    except Exception:
        return
    for n in nodes:
        cls = n.get_class().get_name()
        out.append("  " * depth + cls + ": " + title_of(n))
        for pname in ("editor_state_machine_graph", "editor_graph", "bound_graph",
                      "boundary_editor_node_graph", "transition_graph"):
            try:
                sub = n.get_editor_property(pname)
                if sub is not None and hasattr(sub, "get_nodes"):
                    out.append("  " * (depth + 1) + "<" + pname + ">")
                    dump_graph(sub, depth + 2, out, seen)
            except Exception:
                pass
        if cls == "AnimStateTransitionNode":
            for pname in ("prerequisite_state", "target_state"):
                try:
                    o = n.get_editor_property(pname)
                    out.append("  " * (depth + 1) + pname + "=" + (o.get_name() if o else "None"))
                except Exception:
                    pass


abp_out = {}
for p in by_class.get("AnimBlueprint", []):
    try:
        bp = unreal.load_asset(p)
        lines = []
        try:
            lines.append("VARS: " + ", ".join(str(x) for x in bp.list_member_variable_names()))
        except Exception as e:
            lines.append("VARS: n/a " + str(e)[:80])
        try:
            par = bp.get_blueprint_parent_class()
            lines.append("PARENT: " + (str(par.get_path_name()) if par else "None"))
        except Exception:
            pass
        for g in bp.get_animation_graphs():
            lines.append("== ANIM GRAPH " + g.get_name())
            dump_graph(g, 1, lines, set())
        try:
            for g in bp.get_editor_property("ubergraph_pages"):
                lines.append("== UBERGRAPH " + g.get_name())
                dump_graph(g, 1, lines, set())
        except Exception as e:
            lines.append("UBERGRAPH n/a " + str(e)[:80])
        abp_out[p] = lines
    except Exception as e:
        abp_out[p] = ["ERR " + str(e)[:200]]
res["animbp"] = abp_out
L("animbps", len(abp_out))

# ---------- F. material params of materials used by skeletal meshes ----------
MEL = unreal.MaterialEditingLibrary
mp = {}
for sk in sk_out:
    for matp in sk.get("materials", []):
        if not matp or matp in mp:
            continue
        try:
            m = unreal.load_asset(matp)
            d = {"scalars": [str(x) for x in MEL.get_scalar_parameter_names(m)],
                 "vectors": [str(x) for x in MEL.get_vector_parameter_names(m)],
                 "blend": str(m.get_editor_property("blend_mode")),
                 "two_sided": bool(m.get_editor_property("two_sided"))}
            for key, fn in (("tex", "get_texture_parameter_names"), ("switches", "get_static_switch_parameter_names")):
                try:
                    d[key] = [str(x) for x in getattr(MEL, fn)(m)]
                except Exception:
                    d[key] = []
            mp[matp] = d
        except Exception as e:
            mp[matp] = {"error": str(e)[:160]}
res["materials_of_skel"] = mp

# ---------- G. map / landscape ----------
try:
    world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/AetheriaChroniclesWorld")
    acts = []
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        try:
            loc = a.get_actor_location()
            ext, org = a.get_actor_bounds(False)
            e = {"cls": a.get_class().get_name(), "name": a.get_name(), "label": a.get_actor_label(),
                 "loc": [round(loc.x), round(loc.y), round(loc.z)],
                 "bext": [round(ext.x), round(ext.y), round(ext.z)], "borg": [round(org.x), round(org.y), round(org.z)]}
            if e["cls"] in ("Landscape", "LandscapeStreamingProxy"):
                props = {}
                for pn in ("max_lod", "component_count", "component_size_quads", "subsection_size_quads", "num_subsections",
                           "num_components", "landscape_material", "landscape_guid", "ld_max_draw_distance"):
                    try:
                        props[pn] = str(a.get_editor_property(pn))[:120]
                    except Exception as ex:
                        props[pn] = "ERR " + str(ex)[:60]
                e["props"] = props
                try:
                    comps = a.get_components_by_class(unreal.LandscapeComponent)
                    e["num_components"] = len(comps)
                except Exception as ex:
                    e["comps_err"] = str(ex)[:100]
            acts.append(e)
        except Exception as ex:
            acts.append({"cls": "?", "error": str(ex)[:100]})
    res["map_actors"] = acts
    try:
        wp = world.get_editor_property("world_partition")
        res["world_partition"] = [x for x in dir(wp) if not x.startswith("_")]
        for k in ("runtime_hash", "editor_hash"):
            try:
                v = wp.get_editor_property(k)
                if v is not None:
                    res["wp_" + k] = [x for x in dir(v) if not x.startswith("_")]
                    d = {}
                    for pn in ("grid_name", "cell_size", "loading_range", "block_slow_loading", "log_levels", "server_only", "is_partitioned"):
                        if _ok(v, pn):
                            d[pn] = str(v.get_editor_property(pn))[:200]
                    res["wp_" + k + "_vals"] = d
            except Exception as ex:
                res["wp_" + k] = "ERR " + str(ex)[:200]
    except Exception as ex:
        res["world_partition"] = "ERR " + str(ex)[:200]
    try:
        ws = world.get_world_settings()
        res["world_settings"] = {k: str(ws.get_editor_property(k))[:160] for k in
                                 ("default_reverb", "default_base_ambient_sound", "kill_z", "global_gravity_z")
                                 if _ok(ws, k)}
    except Exception as ex:
        res["world_settings"] = "ERR " + str(ex)[:200]
except Exception:
    res["map_error"] = traceback.format_exc()[-800:]

# ---------- H. niagara + sounds names ----------
res["niagara_systems"] = sorted(by_class.get("NiagaraSystem", []))
cnt = {}
for p in by_class.get("SoundCue", []):
    f = "/".join(p.split("/")[2:-1])
    cnt[f] = cnt.get(f, 0) + 1
res["sound_cues_by_folder"] = cnt
res["niagara_emitters"] = sorted(by_class.get("NiagaraEmitter", []))
res["worlds"] = sorted(by_class.get("World", []))

with open(os.path.join(OUT, "deep.json"), "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1, default=str)
L("wrote deep.json; actors", len(res.get("map_actors", [])))
