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
        private const string BakeVersion = "aetheria-demo-rig-v2";

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
                    ("Attack3", "Primary_Attack_A_Medium"), ("Cast", "Cast"));

            foreach (var name in new[] { "Serath", "SerathGold", "SerathFrost", "Kallari", "KallariLotus" })
                Bake("Heroes/Hero_" + name, "Heroes/Serath", true, "Idle", "Jog_Fwd",
                    ("Attack1", "Primary_Attack_A_Medium"), ("Attack2", "Primary_Attack_B_Medium"),
                    ("Attack3", "Primary_Attack_D_Medium"), ("Cast", "Primary_Attack_D_Medium"), ("Death", "Death_Fwd"));

            foreach (var name in new[] { "Khaimera", "KhaimeraPelt" })
                Bake("Heroes/Hero_" + name, "Heroes/Khaimera", true, "Idle_NonAdditive", "Jog_Fwd",
                    ("Attack1", "Melee_A"), ("Attack2", "Melee_B"), ("Attack3", "Melee_A"),
                    ("Cast", "Melee_B"), ("Death", "Death_A"));

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

                void Connect(AnimatorState from, AnimatorState to, AnimatorConditionMode condition, float threshold, string name)
                {
                    var transition = from == null ? machine.AddAnyStateTransition(to) : from.AddTransition(to);
                    transition.hasExitTime = false;
                    transition.hasFixedDuration = true;
                    transition.duration = 0.07f;
                    transition.canTransitionToSelf = false;
                    transition.AddCondition(condition, threshold, name);
                }

                if (move != null)
                {
                    var moving = machine.AddState("Move");
                    moving.motion = BakeMotion(controller, target, sources, move, true);
                    Connect(rest, moving, AnimatorConditionMode.Greater, 0.12f, "Speed");
                    Connect(moving, rest, AnimatorConditionMode.Less, 0.12f, "Speed");
                }

                controller.AddParameter("CombatSpeed", AnimatorControllerParameterType.Float);
                var parameters = controller.parameters;
                parameters.Single(p => p.name == "CombatSpeed").defaultFloat = 1f;
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
                        Connect(rest, state, AnimatorConditionMode.If, 0, "Flying");
                        Connect(state, rest, AnimatorConditionMode.IfNot, 0, "Flying");
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
                        Connect(null, state, AnimatorConditionMode.If, 0, trigger);
                    }

                    if (stateName == "Death") Connect(state, rest, AnimatorConditionMode.If, 0, respawn);
                    else
                    {
                        var back = state.AddTransition(rest);
                        back.hasExitTime = true;
                        back.exitTime = 0.96f;
                        back.hasFixedDuration = true;
                        back.duration = 0.08f;
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
                Quaternion alignment = Quaternion.FromToRotation(RightAxis(sourceBones), RightAxis(targetBones));
                var bones = new List<(Transform source, Quaternion rest, Quaternion basis, Quaternion targetRest, string path)>();
                foreach (var (boneName, targetBone) in targetBones)
                {
                    if (targetBone == target.transform || boneName == "root" ||
                        !sourceBones.TryGetValue(legacyDragonRig ? DragonSourceBone(boneName) : boneName, out var sourceBone) ||
                        sourceBone == source.transform)
                        continue;
                    Quaternion basis = Quaternion.Inverse(targetBone.rotation) * alignment * sourceBone.rotation;
                    bones.Add((sourceBone, sourceBone.localRotation, basis, targetBone.localRotation,
                        AnimationUtility.CalculateTransformPath(targetBone, target.transform)));
                }

                if (bones.Count < 12) throw new InvalidOperationException($"{path}: only {bones.Count} bones match {target.name}.");
                int frames = Mathf.Max(2, Mathf.CeilToInt(clip.length * 30f) + 1);
                var bindings = new List<EditorCurveBinding>();
                var curves = new List<AnimationCurve>();
                var poses = new Quaternion[bones.Count, frames];
                float largestMotion = 0f;
                for (int frame = 0; frame < frames; frame++)
                {
                    float time = clip.length * frame / (frames - 1);
                    clip.SampleAnimation(source, time);
                    for (int i = 0; i < bones.Count; i++)
                    {
                        var bone = bones[i];
                        Quaternion delta = Quaternion.Inverse(bone.rest) * bone.source.localRotation;
                        Quaternion pose = bone.targetRest * bone.basis * delta * Quaternion.Inverse(bone.basis);
                        largestMotion = Mathf.Max(largestMotion, Quaternion.Angle(Quaternion.identity, delta));
                        if (frame > 0 && Quaternion.Dot(poses[i, frame - 1], pose) < 0f)
                            pose = new Quaternion(-pose.x, -pose.y, -pose.z, -pose.w);
                        poses[i, frame] = pose;
                    }
                }
                if (largestMotion < 1f) throw new InvalidOperationException($"{path}: the imported clip did not move the rig.");

                for (int i = 0; i < bones.Count; i++)
                {
                    var framesToKeep = new List<int> { 0 };
                    KeepFrames(i, 0, frames - 1, poses, framesToKeep);
                    framesToKeep.Add(frames - 1);
                    for (int j = 0; j < 4; j++)
                    {
                        bindings.Add(EditorCurveBinding.FloatCurve(bones[i].path, typeof(Transform), "m_LocalRotation." + "xyzw"[j]));
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
