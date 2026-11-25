import json, sys, collections
d = json.load(open(sys.argv[1], encoding="utf-8"))
print("=== folder x class (top folders) ===")
fc = collections.defaultdict(lambda: collections.Counter())
for k, v in d["folders"].items():
    top, cls = k.split("|")
    fc[top][cls] += v
for top in sorted(fc, key=lambda t: -sum(fc[t].values())):
    c = fc[top]
    keys = ["StaticMesh","SkeletalMesh","AnimSequence","Material","MaterialInstanceConstant","NiagaraSystem","ParticleSystem","SoundCue","SoundWave","Texture2D","Blueprint","AnimBlueprint","World"]
    print(f"{top:<28} total={sum(c.values()):5d}  " + " ".join(f"{k}={c[k]}" for k in keys if c[k]))
sm = [m for m in d["static_meshes"] if "error" not in m]
print("\n=== static meshes:", len(sm), "errors:", len(d["static_meshes"]) - len(sm))
def coll(m):
    c = m["collision"]; simple = sum(v for k, v in c.items() if k != "convex_elems") + c.get("convex_elems", 0)
    return simple
# per folder: count, no-collision count, nanite count, null material count
g = collections.defaultdict(lambda: [0, 0, 0, 0, 0, 0.0])
for m in sm:
    p = m["path"].replace("/Game/", "").split("/")
    top = p[0] if p[0] not in ("Meshes", "Characters", "Free_Spells") else "/".join(p[:3]) if p[0] != "Meshes" else "Meshes/" + (p[1] if len(p) > 2 else "")
    r = g[top]
    r[0] += 1
    if coll(m) == 0: r[1] += 1
    if m.get("nanite"): r[2] += 1
    if any(x is None for x in m["materials"]) or not m["materials"]: r[3] += 1
    ext = m["ext"]; vol = ext[0]*ext[1]*ext[2]
    if max(ext) > 2500: r[4] += 1
    r[5] = max(r[5], max(ext))
print("\n=== static meshes per folder: count / noCollision / nanite / missingMaterial / big(>25m) / maxHalfExtent(cm) ===")
for top, r in sorted(g.items(), key=lambda kv: -kv[1][0]):
    print(f"{top[:70]:<70} n={r[0]:4d} noColl={r[1]:4d} nanite={r[2]:4d} noMat={r[3]:3d} big={r[4]:3d} maxExt={r[5]:.0f}")
json.dump({k: v for k, v in g.items()}, open("static_summary.json", "w"), indent=1)
