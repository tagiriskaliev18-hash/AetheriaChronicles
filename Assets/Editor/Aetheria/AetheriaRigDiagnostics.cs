using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEditor.Animations;
using UnityEngine;

namespace Aetheria.Editor
{
    /// <summary>
    /// Measures how faithfully a baked controller clip reproduces its source clip: for every bone that has a
    /// child, compares the world-space direction bone->child on the target against the source (after the yaw
    /// alignment). Also compares the two rest poses. Writes Logs/rig_diagnostics.txt.
    /// </summary>
    public static class AetheriaRigDiagnostics
    {
        private static readonly (string prefab, string sourceFbx, string state)[] Cases =
        {
            ("Heroes/Hero_Kwang", "Heroes/Kwang/Animations/Idle.fbx", "Idle"),
            ("Heroes/Hero_Kwang", "Heroes/Kwang/Animations/Jog_Fwd.fbx", "Move"),
            ("Heroes/Hero_Kwang", "Heroes/Kwang/Animations/PrimaryAttack_A_Slow.fbx", "Attack1"),
            ("Heroes/Hero_Gideon", "Heroes/Gideon/Animations/Idle.fbx", "Idle"),
            ("Creatures/MalakorBoss", "Boss_Sevarog/Animations/Idle.fbx", "Idle"),
            ("Creatures/MinionMelee", "Enemies/Minions/Animations/Idle.fbx", "Idle"),
        };

        private static readonly string[] Watch =
            { "pelvis", "spine_01", "spine_03", "neck_01", "head", "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l",
              "upperarm_r", "lowerarm_r", "thigh_l", "calf_l", "foot_l", "thigh_r", "calf_r", "foot_r" };

        [MenuItem("Aetheria/Diagnostics/Rig Retarget Report")]
        public static void Run()
        {
            var sb = new StringBuilder();
            foreach (var c in Cases) Measure(c.prefab, c.sourceFbx, c.state, sb);
            Directory.CreateDirectory("Logs");
            File.WriteAllText("Logs/rig_diagnostics.txt", sb.ToString());
            Debug.Log("[RigDiag] written Logs/rig_diagnostics.txt");
        }

        private static Dictionary<string, Transform> Bones(Transform root) =>
            root.GetComponentsInChildren<Transform>(true).GroupBy(t => t.name.ToLowerInvariant())
                .Where(g => g.Count() == 1).ToDictionary(g => g.Key, g => g.Single());

        private static Transform FirstChildBone(Transform t, Dictionary<string, Transform> other)
        {
            foreach (Transform ch in t)
                if (other.ContainsKey(ch.name.ToLowerInvariant())) return ch;
            return t.childCount > 0 ? t.GetChild(0) : null;
        }

        private static Vector3 Dir(Transform b, Transform child) => (child.position - b.position).normalized;

        private static void Measure(string prefabRes, string fbx, string state, StringBuilder sb)
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>($"Assets/Resources/{prefabRes}.prefab");
            var srcAsset = AssetDatabase.LoadAssetAtPath<GameObject>($"Assets/_AAA_Imported/{fbx}");
            var srcClip = AssetDatabase.LoadAllAssetsAtPath($"Assets/_AAA_Imported/{fbx}").OfType<AnimationClip>()
                .FirstOrDefault(c => !c.name.StartsWith("__preview__"));
            if (prefab == null || srcAsset == null || srcClip == null) { sb.AppendLine($"## {prefabRes} {state}: missing assets"); return; }
            var tgt = Object.Instantiate(prefab);
            var src = Object.Instantiate(srcAsset);
            try
            {
                var anim = tgt.GetComponent<Animator>();
                var ctrl = anim != null ? anim.runtimeAnimatorController as AnimatorController : null;
                var st = ctrl?.layers[0].stateMachine.states.Select(s => s.state).FirstOrDefault(s => s.name == state);
                var bakedClip = st?.motion as AnimationClip;
                var tb = Bones(tgt.transform);
                var sbn = Bones(src.transform);

                Vector3 Right(Dictionary<string, Transform> d) =>
                    d.TryGetValue("upperarm_l", out var l) && d.TryGetValue("upperarm_r", out var r)
                        ? Vector3.ProjectOnPlane(r.position - l.position, Vector3.up).normalized : Vector3.right;
                Quaternion align = Quaternion.FromToRotation(Right(sbn), Right(tb));

                sb.AppendLine($"## {prefabRes} state={state} source={fbx} baked={(bakedClip != null ? bakedClip.name : "NONE")} len={srcClip.length:0.00}s");
                sb.AppendLine($"   target root scale={tgt.transform.lossyScale.x:0.###} src root scale={src.transform.lossyScale.x:0.###}");

                void Compare(string label)
                {
                    if (tb.TryGetValue("root", out var tr) && sbn.TryGetValue("root", out var sr)) align = Quaternion.FromToRotation(Vector3.ProjectOnPlane(sr.rotation * Vector3.right, Vector3.up).normalized, Vector3.ProjectOnPlane(tr.rotation * Vector3.right, Vector3.up).normalized);
                    var line = new StringBuilder($"   {label,-10}");
                    float worst = 0f, sum = 0f; int n = 0;
                    foreach (var name in Watch)
                    {
                        if (!tb.TryGetValue(name, out var t) || !sbn.TryGetValue(name, out var s)) continue;
                        var tc = FirstChildBone(t, sbn); var scn = tc != null && sbn.TryGetValue(tc.name.ToLowerInvariant(), out var sc) ? sc : null;
                        if (tc == null || scn == null) continue;
                        float ang = Vector3.Angle(Dir(t, tc), align * Dir(s, scn));
                        worst = Mathf.Max(worst, ang); sum += ang; n++;
                        line.Append($" {name}:{ang:0}");
                    }
                    sb.AppendLine($"{line}  | mean={(n > 0 ? sum / n : 0):0.0} worst={worst:0}");
                }

                Compare("REST");
                if (bakedClip != null)
                {
                    foreach (float f in new[] { 0f, 0.33f, 0.66f })
                    {
                        srcClip.SampleAnimation(src, srcClip.length * f);
                        bakedClip.SampleAnimation(tgt, bakedClip.length * f);
                        Compare($"t={f:0.00}");
                    }
                    // Feet vs ground: lowest foot height relative to root, source vs target (normalised by hip height).
                    if (tb.TryGetValue("foot_l", out var tfl) && tb.TryGetValue("pelvis", out var tp) &&
                        sbn.TryGetValue("foot_l", out var sfl) && sbn.TryGetValue("pelvis", out var sp))
                        sb.AppendLine($"   hip-foot height ratio  target={(tp.position.y - tfl.position.y):0.00} source={(sp.position.y - sfl.position.y):0.00}");
                    if (tb.TryGetValue("root", out var trr) && sbn.TryGetValue("root", out var srr))
                        sb.AppendLine($"   root rot target={trr.rotation.eulerAngles} source={srr.rotation.eulerAngles} | targetObj={tgt.transform.rotation.eulerAngles}");
                    if (tb.TryGetValue("head", out var th2) && tb.TryGetValue("foot_l", out var tf2) && sbn.TryGetValue("head", out var sh2) && sbn.TryGetValue("foot_l", out var sf2))
                        sb.AppendLine($"   foot->head world target={(th2.position - tf2.position)} source={(sh2.position - sf2.position)}");
                }
            }
            finally
            {
                Object.DestroyImmediate(tgt);
                Object.DestroyImmediate(src);
            }
        }
    }
}
