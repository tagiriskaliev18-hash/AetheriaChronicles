// Read-only inspection of imported animation FBX and character GLB assets.
// Run with:  unity command script run --file Tools/Inspect/InspectAnimations.cs --timeout 300
// Writes a plain-text report to Tools/Reports/animations.txt (never modifies assets).
using System;
using System.IO;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEngine;

namespace AetheriaTools
{
    public static class InspectAnimations
    {
        public static string Run()
        {
            var sb = new StringBuilder();
            string root = "Assets/_AAA_Imported";

            // ---------- 1. Animation FBX ----------
            foreach (string fbx in AssetDatabase.FindAssets("t:Model", new[] { root })
                         .Select(AssetDatabase.GUIDToAssetPath)
                         .Where(p => p.EndsWith(".fbx", StringComparison.OrdinalIgnoreCase) && p.Contains("/Animations/"))
                         .OrderBy(p => p))
            {
                var all = AssetDatabase.LoadAllAssetsAtPath(fbx);
                var clips = all.OfType<AnimationClip>().Where(c => !c.name.StartsWith("__preview__")).ToList();
                var go = all.OfType<GameObject>().FirstOrDefault();
                sb.AppendLine($"### {fbx.Replace(root + "/", "")}");
                if (go == null) { sb.AppendLine("   NO GAMEOBJECT"); continue; }

                var bones = go.GetComponentsInChildren<Transform>(true);
                var smr = go.GetComponentInChildren<SkinnedMeshRenderer>(true);
                var anim = go.GetComponent<Animator>();
                sb.AppendLine($"   root='{go.name}' transforms={bones.Length} skinnedMesh={(smr != null ? smr.name : "none")} " +
                              $"avatar={(anim != null && anim.avatar != null ? anim.avatar.name + "/human=" + anim.avatar.isHuman + "/valid=" + anim.avatar.isValid : "null")}");
                sb.AppendLine("   bones: " + string.Join(",", bones.Select(t => t.name).Take(70)));

                foreach (var c in clips)
                {
                    var bindings = AnimationUtility.GetCurveBindings(c);
                    var paths = bindings.Select(b => b.path).Distinct().ToList();
                    sb.AppendLine($"   CLIP '{c.name}' len={c.length:F2}s rate={c.frameRate} human={c.humanMotion} curves={bindings.Length} paths={paths.Count} loop={c.isLooping}");
                    var rot = bindings.Count(b => b.propertyName.StartsWith("localRot"));
                    var pos = bindings.Count(b => b.propertyName.StartsWith("localPos"));
                    var scl = bindings.Count(b => b.propertyName.StartsWith("localScale"));
                    sb.AppendLine($"      rot={rot} pos={pos} scale={scl} samplePaths: " + string.Join(" | ", paths.Take(6)));
                }
                if (clips.Count == 0) sb.AppendLine("   NO ANIMATION CLIPS");
            }

            // ---------- 2. Character / creature GLB ----------
            string[] glbs =
            {
                "Heroes/Kwang/Meshes/Kwang_GDC.glb", "Heroes/Kwang/Meshes/KwangManbun.glb", "Heroes/Kwang/Meshes/KwangFrostwalker.glb",
                "Heroes/Kwang/Meshes/KwangAlbino.glb", "Heroes/Kwang/Meshes/KwangSunrise.glb",
                "Heroes/Gideon/Meshes/Gideon.glb", "Heroes/Gideon/Meshes/Gideon_Inquisitor.glb", "Heroes/Gideon/Meshes/Gideon_Mephisto.glb",
                "Heroes/Khaimera/Meshes/Khaimera.glb", "Heroes/Khaimera/Meshes/Khaimera_GruxPelt.glb",
                "Heroes/Serath/Meshes/Serath.glb", "Heroes/Serath/Meshes/SerathGold.glb", "Heroes/Serath/Meshes/SerathFrost.glb",
                "Heroes/Kallari/Meshes/Kallari.glb", "Heroes/Kallari/Meshes/Kallari_DeathLotus.glb",
                "Heroes/Morigesh/Meshes/Morigesh.glb", "Heroes/Morigesh/Meshes/Morigesh_NorthernMystic.glb",
                "Boss_Sevarog/Meshes/SevarogBloodred.glb", "Dragon/Meshes/DragonTheUsurperSK.glb",
                "Enemies/Whisper/Meshes/SK_Whisper.glb", "Enemies/Rampage/Meshes/Rampage.glb",
                "Enemies/Minions/Meshes/Minion_Lane_Melee_Dusk.glb", "Enemies/Minions/Meshes/Minion_Lane_Ranged_Core_Dusk.glb",
                "WizardRaw/GLBModels/hero_archmage.glb", "WizardRaw/GLBModels/mob_boss_archon.glb",
                "WizardRaw/GLBModels/mob_bruiser_juggernaut.glb", "WizardRaw/GLBModels/mob_ranged_lich.glb", "WizardRaw/GLBModels/mob_swarm_reaper.glb",
            };
            foreach (string rel in glbs)
            {
                string p = root + "/" + rel;
                var go = AssetDatabase.LoadAssetAtPath<GameObject>(p);
                sb.AppendLine($"### {rel}");
                if (go == null) { sb.AppendLine("   LOAD FAILED"); continue; }
                var all = AssetDatabase.LoadAllAssetsAtPath(p);
                var clips = all.OfType<AnimationClip>().Where(c => !c.name.StartsWith("__preview__")).ToList();
                var anim = go.GetComponentInChildren<Animator>(true);
                var transforms = go.GetComponentsInChildren<Transform>(true);
                var rends = go.GetComponentsInChildren<Renderer>(true);
                sb.AppendLine($"   root='{go.name}' scale={go.transform.localScale} children={transforms.Length} renderers={rends.Length}");
                if (anim != null)
                    sb.AppendLine($"   animator='{anim.name}' avatar={(anim.avatar != null ? anim.avatar.name + "/human=" + anim.avatar.isHuman + "/valid=" + anim.avatar.isValid : "null")} controller={(anim.runtimeAnimatorController != null ? anim.runtimeAnimatorController.name : "null")} enabled={anim.enabled}");
                sb.AppendLine("   CLIPS: " + (clips.Count == 0 ? "none" : string.Join(", ", clips.Select(c => $"{c.name}({c.length:F2}s,human={c.humanMotion})"))));
                sb.AppendLine("   BONES: " + string.Join(",", transforms.Select(t => t.name).Take(90)));
                foreach (var r in rends)
                {
                    string kind = r is SkinnedMeshRenderer s ? $"Skinned verts={s.sharedMesh.vertexCount} bones={s.bones.Length}" : $"Mesh verts={(r.GetComponent<MeshFilter>()?.sharedMesh?.vertexCount ?? 0)}";
                    sb.AppendLine($"   RENDERER '{r.name}' {kind} bounds={r.bounds.size}");
                    var mats = r.sharedMaterials;
                    for (int i = 0; i < mats.Length; i++)
                    {
                        var m = mats[i];
                        if (m == null) { sb.AppendLine($"      [{i}] NULL MATERIAL"); continue; }
                        string tex = "";
                        foreach (string prop in new[] { "_BaseMap", "_MainTex", "baseColorTexture" })
                            if (m.HasProperty(prop) && m.GetTexture(prop) != null) { tex = m.GetTexture(prop).name; break; }
                        sb.AppendLine($"      [{i}] mat='{m.name}' shader='{m.shader?.name}' baseTex='{tex}' color={m.color}");
                    }
                }
            }

            Directory.CreateDirectory("Tools/Reports");
            File.WriteAllText("Tools/Reports/animations.txt", sb.ToString());
            return "OK lines=" + sb.ToString().Split('\n').Length;
        }
    }
}
