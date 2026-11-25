# Scripts/tools/deep_report.py - human-readable digest of Saved/AetheriaAudit/deep.json
import json, sys, collections, re

d = json.load(open(sys.argv[1], encoding="utf-8"))
sec = sys.argv[2] if len(sys.argv) > 2 else "all"


def short(p):
    return p.replace("/Game/", "")


if sec in ("all", "map"):
    print("=== MAP ACTORS ===")
    for a in d.get("map_actors", []):
        if "error" in a:
            print("ERR", a)
            continue
        print(a["cls"], a.get("label"), "loc", a["loc"], "bext", a["bext"], "borg", a["borg"])
        for k in ("props", "num_components"):
            if k in a:
                print("     ", k, a[k])
    for k in ("map_error", "world_partition", "wp_runtime_hash", "wp_runtime_hash_vals", "wp_editor_hash_vals", "world_settings"):
        if k in d:
            print(k, "=", str(d[k])[:900])
    print("worlds:", d.get("worlds"))

if sec in ("all", "static"):
    print("\n=== STATIC MESHES per folder: n / noCollision / nanite / missingMat / big>25m / maxHalfExt ===")
    g = collections.defaultdict(lambda: [0, 0, 0, 0, 0, 0])
    err = 0
    for m in d["static_meshes"]:
        if "error" in m:
            err += 1
            continue
        p = m["path"].replace("/Game/", "").split("/")
        if p[0] == "Meshes":
            top = "Meshes/" + "/".join(p[1:3])
        elif p[0] in ("Characters", "Free_Spells", "Vefects"):
            top = "/".join(p[:4])
        else:
            top = p[0]
        r = g[top]
        r[0] += 1
        c = m["collision"]
        simple = sum(v for k, v in c.items() if k not in ("cts",))
        if simple == 0 and c.get("cts", 0) != 3:
            r[1] += 1
        if m["nanite"]:
            r[2] += 1
        if (not m["materials"]) or any(x is None for x in m["materials"]):
            r[3] += 1
        if max(m["ext"]) > 2500:
            r[4] += 1
        r[5] = max(r[5], max(m["ext"]))
    print("errors:", err)
    for top, r in sorted(g.items(), key=lambda kv: -kv[1][0])[:90]:
        print(f"{top[:72]:<72} n={r[0]:4d} noColl={r[1]:4d} nanite={r[2]:4d} noMat={r[3]:3d} big={r[4]:3d} maxExt={r[5]:.0f}")

if sec in ("all", "skel"):
    print("\n=== SKELETAL MESHES ===")
    for m in d["skeletal_meshes"]:
        if "error" in m:
            print("ERR", m)
            continue
        print(f'{short(m["path"]):<88} h={m["ext"][2]*2/100:.2f}m w={m["ext"][0]*2/100:.2f}m skel={(m["skeleton"] or "None").split(".")[-1]} pa={"yes" if m["physics_asset"] else "NO"} mats={[ (x or "NULL").split(".")[-1] for x in m["materials"]]} socks={[s[0] for s in m["sockets"]]}')

if sec in ("all", "anim"):
    print("\n=== ANIMS BY SKELETON ===")
    g = collections.defaultdict(list)
    for a in d["anim_sequences"]:
        if "error" in a:
            print("ERR", a)
            continue
        g[a["skel"]].append(a)
    for sk, lst in sorted(g.items(), key=lambda kv: -len(kv[1])):
        add = collections.Counter(a["add"] for a in lst)
        base = collections.Counter(a["base"] for a in lst)
        rm = sum(1 for a in lst if "true" in a["rm"].lower())
        print(f"{sk}: {len(lst)} anims additive={dict(add)} basepose={dict(base)} rootMotion={rm}")
    print("\n=== MONTAGES ===")
    for m in d["montages"]:
        if "error" in m:
            print("ERR", m)
            continue
        print(f'{short(m["path"]):<95} slots={m["slots"]} len={m["len"]} sections={m["sections"]}')

if sec in ("all", "abp"):
    print("\n=== ANIM BLUEPRINTS ===")
    for p, lines in d["animbp"].items():
        print("####", short(p), f"({len(lines)} lines)")
        for ln in lines[:int(sys.argv[3]) if len(sys.argv) > 3 else 70]:
            print("   ", ln[:230])

if sec in ("all", "mat"):
    print("\n=== MATERIALS OF SKELETAL MESHES ===")
    for p, m in d["materials_of_skel"].items():
        if "error" in m:
            print(short(p), "ERR", m["error"])
            continue
        print(short(p), "| V:", ",".join(m["vectors"])[:150], "| S:", ",".join(m["scalars"])[:170], "| 2S" if m["two_sided"] else "")

if sec in ("all", "fx"):
    print("\n=== NIAGARA SYSTEMS ===")
    for p in d["niagara_systems"]:
        print("  ", short(p))
    print("\n=== SOUND CUES BY FOLDER ===")
    for k, v in sorted(d["sound_cues_by_folder"].items(), key=lambda kv: -kv[1])[:60]:
        print(f"  {v:4d}  {k}")
