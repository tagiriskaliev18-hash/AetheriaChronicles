import json, sys, collections, re
d = json.load(open(sys.argv[1], encoding="utf-8"))
print("=== SKELETAL MESHES ===")
for m in sorted(d["skeletal_meshes"], key=lambda x: x["path"]):
    if "error" in m: print("ERR", m["path"], m["error"]); continue
    ext = m["bounds"]
    print(f'{m["path"].replace("/Game/",""):<95} h={ext[2]*2/100:.2f}m skel={(m["skeleton"] or "None").split(".")[-1]} mats={[ (x or "NULL").split("/")[-1].split(".")[0] for x in m["materials"]]}')
print("\n=== ANIMS BY SKELETON ===")
g = collections.defaultdict(list)
for a in d["anim_sequences"]:
    g[(a.get("skeleton") or "ERR").split(".")[-1]].append(a)
for sk, lst in sorted(g.items(), key=lambda kv: -len(kv[1])):
    add = collections.Counter(a.get("additive") for a in lst)
    errs = sum(1 for a in lst if "error" in a)
    folders = collections.Counter("/".join(a["path"].split("/")[2:-1]).replace("/Game/","") for a in lst)
    print(f'{sk}: {len(lst)} anims, additive={dict(add)}, errors={errs}')
    for f, c in folders.most_common(6): print(f"      {c:4d}  {f}")
print("\n=== ANIM BLUEPRINTS / BLENDS / MONTAGES ===")
for k in ("AnimBlueprint","BlendSpace","BlendSpace1D","AimOffsetBlendSpace","AimOffsetBlendSpace1D","AnimMontage","LevelSequence","Skeleton","IKRigDefinition","IKRetargeter","ControlRigBlueprint","PoseAsset","Blueprint"):
    print(f"-- {k} ({len(d['other'].get(k,[]))})")
    for p in d["other"].get(k, [])[:60]: print("   ", p.replace("/Game/",""))
