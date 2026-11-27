using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Animations;
using UnityEngine;
using Object = UnityEngine.Object;

namespace Aetheria.Editor
{
    public static class AetheriaDemoAnimations
    {
        private const string Sources = "Assets/_AAA_Imported/";
        private const string Controllers = "Assets/Generated/DemoAnimations/";
        // v3: pelvis translation is retargeted (deaths reach the ground, locomotion bobs), the Move state
        // is time-scaled by MoveScale against foot sliding, borrowed Hit/Death clips for heroes that had
        // none, and cross-fades sized per transition instead of one 0.07 s snap everywhere.
        private const string BakeVersion = "aetheria-demo-rig-v5";
        private const string ParagonHit = "Heroes/Kwang:Hitreact_Fwd";
        private const string ParagonDeath = "Heroes/Kwang:Death_Bwd";

        [MenuItem("Aetheria/Prepare Demo Animations")]
        public static void Prepare()
        {
            Directory.CreateDirectory(Controllers);
            AssetDatabase.Refresh();

            Bake("Creatures/MinionMelee", "Enemies/Minions", false, "Idle", "Enemies/Whisper:Anim_Whisper_Run",
                ("Attack1", "BiteAttack_A"), ("Attack2", "BiteAttack_B"),
                ("Attack3", "BiteAttack_Fast"), ("Death", "Death_front"));
            Bake("Creatures/MinionRanged", "Enemies/Minions", false, "Idle", "Enemies/Whisper:Anim_Whisper_Run",
                ("Cast", "Spit_Attack"), ("Attack1", "Spit_Attack_Fast_A"),
                ("Attack2", "Spit_Attack_Fast_B"), ("Attack3", "Spit_Attack_Fast_C"), ("Death", "Death_back"));
            Bake("Creatures/Whisper", "Enemies/Whisper", false, "Anim_Whisper_Idle1", "Anim_Whisper_Run",
                ("Attack1", "Anim_Whisper_Attack1"), ("Attack2", "Anim_Whisper_Attack2"),
                ("Attack3", "Anim_Whisper_Attack3"), ("Death", "Anim_Whisper_Death"));
            Bake("Creatures/Rampage", "Enemies/Rampage", false, "Idle", "Heroes/Khaimera:Jog_Fwd",
                ("Attack1", "Attack_Melee_A"), ("Attack2", "Attack_Melee_B"),
                ("Attack3", "Attack_Melee_A"), ("Death", "Death_A"));
            Bake("Creatures/MalakorBoss", "Boss_Sevarog", false, "Idle", "Run_Fwd",
                ("Attack1", "Swing1_Medium"), ("Attack2", "Swing2_Medium"), ("Attack3", "Swing3_Medium"),
                ("Attack4", "Ultimate_Swing_120fps"), ("Cast", "Cast"), ("Stun", "Stun_Start"), ("Death", "Death_front"));
            Bake("Creatures/IgnisDragon", "Dragon", false, "Idle01Anim", null,
                ("Fly", "FlyForwardAnim"), ("Breath", "AttackFireBallAnim"), ("Scream", "ScreamAnim"), ("Hit", "GetHitAnim"));

            foreach (var name in new[] { "Kwang", "KwangAlbino", "KwangFrost", "KwangManbun", "KwangSunrise" })
                Bake("Heroes/Hero_" + name, "Heroes/Kwang", true, "Idle", "Jog_Fwd",
                    ("Attack1", "PrimaryAttack_A_Slow"), ("Attack2", "PrimaryAttack_B_Slow"),
                    ("Attack3", "PrimaryAttack_C_Slow"), ("Attack4", "PrimaryAttack_D_Slow"),
                    ("Cast", "Cast"), ("Hit", "Hitreact_Fwd"), ("Death", "Death_Bwd"));

            foreach (var name in new[] { "Gideon", "GideonInq", "GideonMeph", "Morigesh", "MorigeshMystic" })
                Bake("Heroes/Hero_" + name, "Heroes/Gideon", true, "Idle", "Jog_Fwd",
                    ("Attack1", "Primary_Attack_A_Medium"), ("Attack2", "Primary_Attack_A_Medium"),
                    ("Attack3", "Primary_Attack_A_Medium"), ("Cast", "Cast"),
                    ("Hit", ParagonHit), ("Death", ParagonDeath));

            foreach (var name in new[] { "Serath", "SerathGold", "SerathFrost", "Kallari", "KallariLotus" })
                Bake("Heroes/Hero_" + name, "Heroes/Serath", true, "Idle", "Jog_Fwd",
                    ("Attack1", "Primary_Attack_A_Medium"), ("Attack2", "Primary_Attack_B_Medium"),
                    ("Attack3", "Primary_Attack_D_Medium"), ("Cast", "Primary_Attack_D_Medium"), ("Death", "Death_Fwd"),
                    ("Hit", ParagonHit));

            foreach (var name in new[] { "Khaimera", "KhaimeraPelt" })
                Bake("Heroes/Hero_" + name, "Heroes/Khaimera", true, "Idle_NonAdditive", "Jog_Fwd",
                    ("Attack1", "Melee_A"), ("Attack2", "Melee_B"), ("Attack3", "Melee_A"),
                    ("Cast", "Melee_B"), ("Death", "Death_A"), ("Hit", ParagonHit));

            AssetDatabase.SaveAssets();
        }

        private static void Bake(string resource, string sources, bool hero, string idle, string move,
            params (string state, string clip)[] actions)
        {
            string prefabPath = $"Assets/Resources/{resource}.prefab";
            string controllerPath = Controllers + Path.GetFileName(resource) + ".controller";
            var target = PrefabUtility.LoadPrefabContents(prefabPath);
            try
            {
                bool dragon = resource == "Creatures/IgnisDragon";
                var animator = target.GetComponent<Animator>();
                var existing = AssetDatabase.LoadAssetAtPath<AnimatorController>(controllerPath);
                if (existing != null && animator != null && animator.runtimeAnimatorController == existing &&
                    AssetImporter.GetAtPath(controllerPath).userData == BakeVersion) return;
                if (animator == null) animator = target.AddComponent<Animator>();
                foreach (var nestedAnimator in target.GetComponentsInChildren<Animator>(true))
                    if (nestedAnimator != animator) nestedAnimator.enabled = false;

                if (existing != null) AssetDatabase.DeleteAsset(controllerPath);
                var controller = AnimatorController.CreateAnimatorControllerAtPath(controllerPath);
                var machine = controller.layers[0].stateMachine;
                var rest = machine.AddState("Idle");
                machine.defaultState = rest;
                rest.motion = BakeMotion(controller, target, sources, idle, true);
                controller.AddParameter("Speed", AnimatorControllerParameterType.Float);

                void Connect(AnimatorState from, AnimatorState to, AnimatorConditionMode condition, float threshold, string name,
                    float duration = 0.08f)
                {
                    var transition = from == null ? machine.AddAnyStateTransition(to) : from.AddTransition(to);
                    transition.hasExitTime = false;
                    transition.hasFixedDuration = true;
                    transition.duration = duration;
                    transition.canTransitionToSelf = false;
                    transition.AddCondition(condition, threshold, name);
                }

                controller.AddParameter("MoveScale", AnimatorControllerParameterType.Float);
                if (move != null)
                {
                    var moving = machine.AddState("Move");
                    moving.motion = BakeMotion(controller, target, sources, move, true);
                    // Gameplay writes MoveScale = actual speed / authored speed so the feet stay planted.
                    moving.speedParameter = "MoveScale";
                    moving.speedParameterActive = true;
                    Connect(rest, moving, AnimatorConditionMode.Greater, 0.12f, "Speed", 0.2f);
                    Connect(moving, rest, AnimatorConditionMode.Less, 0.12f, "Speed", 0.25f);
                }

                controller.AddParameter("CombatSpeed", AnimatorControllerParameterType.Float);
                var parameters = controller.parameters;
                parameters.Single(p => p.name == "CombatSpeed").defaultFloat = 1f;
                parameters.Single(p => p.name == "MoveScale").defaultFloat = 1f;
                controller.parameters = parameters;
                string attack = hero ? "Attack" : "DoAction";
                string index = hero ? "AttackIndex" : "Action";
                controller.AddParameter(attack, AnimatorControllerParameterType.Trigger);
                controller.AddParameter(index, AnimatorControllerParameterType.Int);
                controller.AddParameter("Die", AnimatorControllerParameterType.Trigger);
                string respawn = hero ? "Respawn" : "Revive";
                controller.AddParameter(respawn, AnimatorControllerParameterType.Trigger);
                controller.AddParameter(hero ? "HitReact" : "Hit", AnimatorControllerParameterType.Trigger);
                if (hero)
                {
                    controller.AddParameter("Blocking", AnimatorControllerParameterType.Bool);
                    controller.AddParameter("Cast", AnimatorControllerParameterType.Trigger);
                }
                if (dragon)
                {
                    controller.AddParameter("Flying", AnimatorControllerParameterType.Bool);
                    controller.AddParameter("Breath", AnimatorControllerParameterType.Bool);
                    controller.AddParameter("Scream", AnimatorControllerParameterType.Trigger);
                }

                var motions = new Dictionary<string, AnimationClip>();
                foreach (var (stateName, sourceClip) in actions)
                {
                    if (!motions.TryGetValue(sourceClip, out var motion))
                        motions[sourceClip] = motion = BakeMotion(controller, target, sources, sourceClip, stateName == "Fly");
                    var state = machine.AddState(stateName);
                    state.motion = motion;
                    if (stateName == "Fly")
                    {
                        Connect(rest, state, AnimatorConditionMode.If, 0, "Flying", 0.35f);
                        Connect(state, rest, AnimatorConditionMode.IfNot, 0, "Flying", 0.45f);
                        continue;
                    }
                    var actionIndex = stateName switch
                    {
                        "Attack1" => 1, "Attack2" => 2, "Attack3" => 3, "Attack4" => 4,
                        "Cast" when !hero => 5, "Stun" => 6, _ => 0
                    };
                    if (actionIndex > 0)
                    {
                        Connect(null, state, AnimatorConditionMode.If, 0, attack);
                        machine.anyStateTransitions.Last().AddCondition(AnimatorConditionMode.Equals, actionIndex, index);
                        if (hero)
                        {
                            state.speedParameter = "CombatSpeed";
                            state.speedParameterActive = true;
                        }
                    }
                    else
                    {
                        string trigger = stateName switch
                        {
                            "Death" => "Die",
                            "Hit" => hero ? "HitReact" : "Hit",
                            _ => stateName
                        };
                        float fade = stateName switch { "Death" => 0.15f, "Hit" => 0.06f, _ => 0.12f };
                        Connect(null, state, AnimatorConditionMode.If, 0, trigger, fade);
                    }

                    if (stateName == "Death")
                    {
                        // Nothing but the respawn trigger may pull a corpse back up.
                        Connect(state, rest, AnimatorConditionMode.If, 0, respawn, 0.2f);
                    }
                    else
                    {
                        // Blend back to idle over ~0.2 s that ends exactly on the clip's last frame.
                        float length = Mathf.Max(0.1f, motion.length);
                        const float recover = 0.2f;
                        var back = state.AddTransition(rest);
                        back.hasExitTime = true;
                        back.exitTime = Mathf.Clamp(1f - recover / length, 0.6f, 0.97f);
                        back.hasFixedDuration = true;
                        back.duration = Mathf.Min(recover, length * 0.4f);
                    }
                }

                EditorUtility.SetDirty(controller);
                AssetDatabase.SaveAssetIfDirty(controller);
                var importer = AssetImporter.GetAtPath(controllerPath);
                importer.userData = BakeVersion;
                importer.SaveAndReimport();
                var savedController = AssetDatabase.LoadAssetAtPath<AnimatorController>(controllerPath);
                if (savedController == null || savedController.animationClips.Length < 2)
                    throw new IOException($"Animation controller could not be saved at {controllerPath}.");
                animator.runtimeAnimatorController = savedController;
                animator.applyRootMotion = false;
                PrefabUtility.SaveAsPrefabAsset(target, prefabPath);
                Debug.Log($"[DemoAnimations] {resource}: {actions.Length} action states.");
            }
            catch
            {
                AssetDatabase.DeleteAsset(controllerPath);
                throw;
            }
            finally
            {
                PrefabUtility.UnloadPrefabContents(target);
            }
        }

        private static AnimationClip BakeMotion(AnimatorController controller, GameObject target, string folder,
            string name, bool loop)
        {
            int separator = name.IndexOf(':');
            if (separator >= 0)
            {
                folder = name.Substring(0, separator);
                name = name.Substring(separator + 1);
            }
            string path = $"{Sources}{folder}/Animations/{name}.fbx";
            var sourceAsset = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            var clip = AssetDatabase.LoadAllAssetsAtPath(path).OfType<AnimationClip>()
                .FirstOrDefault(c => !c.name.StartsWith("__preview__", StringComparison.Ordinal));
            if (sourceAsset == null || clip == null || clip.length <= 0f)
                throw new InvalidOperationException($"Missing usable clip at {path}.");

            var source = Object.Instantiate(sourceAsset);
            try
            {
                var sourceBones = UniqueBones(source.transform);
                bool legacyDragonRig = folder == "Dragon" && sourceBones.ContainsKey("Root_Pelvis");
                if (legacyDragonRig)
                    sourceBones = UniqueBones(source.transform, name => name.Replace("_Left", "_L").Replace("_Right", "_R"));
                var targetBones = UniqueBones(target.transform);
                string SourceName(string n) => legacyDragonRig ? DragonSourceBone(n) : n;

                // Matched bones, parents first (GetComponentsInChildren is a depth-first pre-order walk).
                var match = new Dictionary<Transform, Transform>();
                foreach (var (boneName, targetBone) in targetBones)
                {
                    if (targetBone == target.transform) continue;
                    if (sourceBones.TryGetValue(SourceName(boneName), out var sourceBone) && sourceBone != source.transform)
                        match[targetBone] = sourceBone;
                }
                if (match.Count < 12) throw new InvalidOperationException($"{path}: only {match.Count} bones match {target.name}.");

                // The rest pose of the target must survive the bake: this is the prefab that gets saved.
                var targetAll = target.GetComponentsInChildren<Transform>(true);
                var restLocalRot = targetAll.Select(t => t.localRotation).ToArray();
                var restLocalPos = targetAll.Select(t => t.localPosition).ToArray();
                void RestoreTarget()
                {
                    for (int i = 0; i < targetAll.Length; i++)
                    {
                        targetAll[i].localRotation = restLocalRot[i];
                        targetAll[i].localPosition = restLocalPos[i];
                    }
                }

                // Coordinate frames. Both rigs carry the UE "root" bone at the feet, so a pose expressed in
                // the source root frame maps 1:1 onto the target root frame; this also strips root motion.
                sourceBones.TryGetValue(SourceName("root"), out var sourceRoot);
                targetBones.TryGetValue("root", out var targetRoot);
                if (sourceRoot == null || targetRoot == null) { sourceRoot = source.transform; targetRoot = target.transform; }
                Quaternion fallbackAlign = Quaternion.FromToRotation(RightAxis(sourceBones), RightAxis(targetBones));
                bool rootFrames = sourceRoot != source.transform;

                var plans = new List<SolvePlan>();
                foreach (var t in targetAll)
                {
                    if (!match.TryGetValue(t, out var s) || t == targetRoot) continue;
                    var plan = new SolvePlan { Target = t, Source = s, Path = AnimationUtility.CalculateTransformPath(t, target.transform) };
                    var kids = new List<Transform>();
                    foreach (Transform child in t)
                        if (match.ContainsKey(child)) kids.Add(child);
                    var primary = kids.Where(k => !k.name.ToLowerInvariant().Contains("twist"))
                        .OrderByDescending(k => IsSided(k.name) ? 0 : 1)
                        .ThenByDescending(k => k.GetComponentsInChildren<Transform>(true).Count(match.ContainsKey))
                        .FirstOrDefault();
                    if (primary != null)
                    {
                        plan.TargetPrimary = primary;
                        plan.SourcePrimary = match[primary];
                        // Twist reference: the two remaining children farthest apart (thighs of the pelvis,
                        // clavicles of the chest, thumb/pinky of the hand).
                        var others = kids.Where(k => k != primary && !k.name.ToLowerInvariant().Contains("twist")).ToList();
                        float best = 0.0001f;
                        for (int a = 0; a < others.Count; a++)
                        for (int b = a + 1; b < others.Count; b++)
                        {
                            float d = (others[a].position - others[b].position).sqrMagnitude;
                            if (d <= best) continue;
                            best = d;
                            plan.TargetA = others[a]; plan.TargetB = others[b];
                            plan.SourceA = match[others[a]]; plan.SourceB = match[others[b]];
                        }
                    }
                    plans.Add(plan);
                }

                // Pelvis placement, scaled by leg length so the feet land on the ground of a taller or shorter body.
                sourceBones.TryGetValue("pelvis", out var sourceHip);
                targetBones.TryGetValue("pelvis", out var targetHip);
                bool bakeHip = sourceHip != null && targetHip != null && targetHip.parent != null;
                float LegLength(Dictionary<string, Transform> d) =>
                    d.TryGetValue("thigh_l", out var th) && d.TryGetValue("calf_l", out var ca) && d.TryGetValue("foot_l", out var fo)
                        ? Vector3.Distance(th.position, ca.position) + Vector3.Distance(ca.position, fo.position) : 0f;
                float targetLeg = LegLength(targetBones);
                float sourceLeg = LegLength(sourceBones);
                float hipRatio = targetLeg > 0.01f && sourceLeg > 0.01f ? targetLeg / sourceLeg : 1f;

                var bones = plans.Where(p => p.TargetPrimary != null).ToList();
                int frames = Mathf.Max(2, Mathf.CeilToInt(clip.length * 30f) + 1);
                var bindings = new List<EditorCurveBinding>();
                var curves = new List<AnimationCurve>();
                var poses = new Quaternion[bones.Count, frames];
                var hipPositions = new Vector3[frames];
                float largestMotion = 0f;
                try
                {
                    for (int frame = 0; frame < frames; frame++)
                    {
                        float time = clip.length * frame / (frames - 1);
                        clip.SampleAnimation(source, time);
                        RestoreTarget();

                        // Maps a source world vector into target world space: a yaw only. The FBX root bone carries
                        // the Z-up conversion (-90 X) while the glTF root does not, so the full root rotation would
                        // tip the whole body over; the root X axis is horizontal in both and gives the facing.
                        Quaternion map = fallbackAlign;
                        if (rootFrames)
                        {
                            Vector3 sx = Vector3.ProjectOnPlane(sourceRoot.rotation * Vector3.right, Vector3.up);
                            Vector3 tx = Vector3.ProjectOnPlane(targetRoot.rotation * Vector3.right, Vector3.up);
                            if (sx.sqrMagnitude > 1e-4f && tx.sqrMagnitude > 1e-4f) map = Quaternion.FromToRotation(sx.normalized, tx.normalized);
                        }

                        if (bakeHip)
                        {
                            targetHip.position = targetRoot.position + map * (sourceHip.position - sourceRoot.position) * hipRatio;
                            hipPositions[frame] = targetHip.localPosition;
                        }

                        // Aim every bone down its source direction, parents first, then roll it about that
                        // direction so its side children line up too. No rest pose is involved, so a clip whose
                        // first frame is not the bind pose can no longer twist the whole skeleton.
                        foreach (var p in plans)
                        {
                            if (p.TargetPrimary == null) continue;
                            Vector3 want = map * (p.SourcePrimary.position - p.Source.position);
                            Vector3 have = p.TargetPrimary.position - p.Target.position;
                            if (want.sqrMagnitude < 1e-10f || have.sqrMagnitude < 1e-10f) continue;
                            Quaternion q = Quaternion.FromToRotation(have, want);
                            if (p.TargetA != null)
                            {
                                Vector3 axis = want.normalized;
                                Vector3 wantSide = Vector3.ProjectOnPlane(map * (p.SourceB.position - p.SourceA.position), axis);
                                Vector3 haveSide = Vector3.ProjectOnPlane(q * (p.TargetB.position - p.TargetA.position), axis);
                                if (wantSide.sqrMagnitude > 1e-8f && haveSide.sqrMagnitude > 1e-8f)
                                    q = Quaternion.AngleAxis(Vector3.SignedAngle(haveSide, wantSide, axis), axis) * q;
                            }
                            p.Target.rotation = q * p.Target.rotation;
                        }

                        for (int i = 0; i < bones.Count; i++)
                        {
                            Quaternion pose = bones[i].Target.localRotation;
                            if (frame > 0 && Quaternion.Dot(poses[i, frame - 1], pose) < 0f)
                                pose = new Quaternion(-pose.x, -pose.y, -pose.z, -pose.w);
                            poses[i, frame] = pose;
                            if (frame > 0) largestMotion = Mathf.Max(largestMotion, Quaternion.Angle(poses[i, 0], pose));
                        }
                    }
                }
                finally
                {
                    RestoreTarget();
                }
                if (largestMotion < 1f) throw new InvalidOperationException($"{path}: the imported clip did not move the rig.");

                for (int i = 0; i < bones.Count; i++)
                {
                    var framesToKeep = new List<int> { 0 };
                    KeepFrames(i, 0, frames - 1, poses, framesToKeep);
                    framesToKeep.Add(frames - 1);
                    for (int j = 0; j < 4; j++)
                    {
                        bindings.Add(EditorCurveBinding.FloatCurve(bones[i].Path, typeof(Transform), "m_LocalRotation." + "xyzw"[j]));
                        var track = new Keyframe[framesToKeep.Count];
                        for (int k = 0; k < track.Length; k++)
                        {
                            int frame = framesToKeep[k];
                            track[k] = new Keyframe(clip.length * frame / (frames - 1), poses[i, frame][j]);
                        }
                        for (int k = 1; k < track.Length; k++)
                        {
                            float slope = (track[k].value - track[k - 1].value) / (track[k].time - track[k - 1].time);
                            track[k - 1].outTangent = slope;
                            track[k].inTangent = slope;
                        }
                        curves.Add(new AnimationCurve(track));
                    }
                }

                if (bakeHip)
                {
                    string hipPath = AnimationUtility.CalculateTransformPath(targetHip, target.transform);
                    for (int axis = 0; axis < 3; axis++)
                    {
                        var track = new Keyframe[frames];
                        for (int frame = 0; frame < frames; frame++)
                            track[frame] = new Keyframe(clip.length * frame / (frames - 1), hipPositions[frame][axis]);
                        var curve = new AnimationCurve(track);
                        for (int k = 0; k < track.Length; k++) curve.SmoothTangents(k, 0f);
                        bindings.Add(EditorCurveBinding.FloatCurve(hipPath, typeof(Transform), "m_LocalPosition." + "xyz"[axis]));
                        curves.Add(curve);
                    }
                }

                var result = new AnimationClip { name = name, frameRate = 30f };
                AnimationUtility.SetEditorCurves(result, bindings.ToArray(), curves.ToArray());
                result.EnsureQuaternionContinuity();
                var settings = AnimationUtility.GetAnimationClipSettings(result);
                settings.loopTime = loop;
                AnimationUtility.SetAnimationClipSettings(result, settings);
                AssetDatabase.AddObjectToAsset(result, controller);
                return result;
            }
            finally
            {
                Object.DestroyImmediate(source);
            }
        }

        private static void KeepFrames(int bone, int first, int last, Quaternion[,] poses, List<int> frames)
        {
            float largestError = 0.35f;
            int split = -1;
            for (int frame = first + 1; frame < last; frame++)
            {
                float t = (frame - first) / (float)(last - first);
                var estimate = Quaternion.Lerp(poses[bone, first], poses[bone, last], t);
                float error = Quaternion.Angle(poses[bone, frame], estimate);
                if (error <= largestError) continue;
                split = frame;
                largestError = error;
            }
            if (split < 0) return;
            KeepFrames(bone, first, split, poses, frames);
            frames.Add(split);
            KeepFrames(bone, split, last, poses, frames);
        }

        /// <summary>How one target bone is aimed: down its primary child, rolled by a pair of side children.</summary>
        private sealed class SolvePlan
        {
            public Transform Target, Source, TargetPrimary, SourcePrimary, TargetA, TargetB, SourceA, SourceB;
            public string Path;
        }

        private static bool IsSided(string name)
        {
            string n = name.ToLowerInvariant();
            return n.EndsWith("_l") || n.EndsWith("_r") || n.Contains("_l_") || n.Contains("_r_");
        }

        private static string DragonSourceBone(string name) => name switch
        {
            "Spine01" => "Spine",
            "Neck01" => "Neck",
            "Jaw01" => "Jaw",
            "Jaw02" => "JawTip",
            "UpperMouth01" => "UpperMouth",
            "RowerArm_R" => "LowerArm_R",
            "UpperReg_R" => "UpperLeg_R",
            "RowerReg_R" => "LowerLeg_R",
            "WingMiddle04_L" => "WingMiddle03_L",
            "WingMiddle04_R" => "WingMiddle03_R",
            "WingPinky04_L" => "WingPinky03_L",
            "WingPinky04_R" => "WingPinky03_R",
            _ => name
        };

        private static Dictionary<string, Transform> UniqueBones(Transform root, Func<string, string> mapName = null) =>
            root.GetComponentsInChildren<Transform>(true)
                .GroupBy(t => mapName == null ? t.name : mapName(t.name), StringComparer.OrdinalIgnoreCase)
                .Where(group => group.Count() == 1)
                .ToDictionary(group => group.Key, group => group.Single(), StringComparer.OrdinalIgnoreCase);

        private static Vector3 RightAxis(Dictionary<string, Transform> bones)
        {
            if (!bones.TryGetValue("upperarm_l", out var left) || !bones.TryGetValue("upperarm_r", out var right))
                return Vector3.right;
            Vector3 axis = Vector3.ProjectOnPlane(right.position - left.position, Vector3.up);
            return axis.sqrMagnitude > 0.000001f ? axis.normalized : Vector3.right;
        }
    }
}
