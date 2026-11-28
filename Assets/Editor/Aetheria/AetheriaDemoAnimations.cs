using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
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
        // v8: heroes run entirely on the Quaternius libraries: rotation-delta retarget (keeps limb roll),
        // Idle/Walk/Jog/Sprint blend tree on Speed, sword combo, jump, dodge roll, block, get-up.
        private const string BakeVersion = "aetheria-demo-rig-v8";
        // CC0 Quaternius Universal Animation Library (UE mannequin bone names, T-pose rest).
        private const string Al = "Anim/QuaterniusUAL:AL_Standard#";
        private const string Ual2 = "Anim/QuaterniusUAL:UAL2_Standard#";

        // ---- retarget switches
        /// <summary>Rotation-delta retarget whenever the source file carries a reference take (see ReferenceTake).</summary>
        private static readonly bool UseDeltaRetarget = true;
        private const string ReferenceTake = "A_TPose";
        /// <summary>Spread the wrist twist onto the target's lowerarm_twist_* bones (the library rig has none).</summary>
        private static readonly bool DistributeForearmTwist = true;

        // ---- hero locomotion: authored ground speed of each clip in m/s for the library mannequin (measured
        // offline from the toe travel during stance). The bake measures every hero again and only falls back
        // to these (scaled by leg length) when the measurement is implausible.
        private const float WalkAuthoredSpeed = 0.98f;
        private const float JogAuthoredSpeed = 5.9f;
        private const float SprintAuthoredSpeed = 8.9f;

        // ---- hero action tuning
        private const float Attack3StateSpeed = 1.5f;   // Sword_Regular_C strikes at 0.63 s; 1.5x lands it inside the 0.46 s combo window
        private const float HeavyAttackExit = 0.5f;     // normalised time where Attack3/Attack4 hand back to locomotion
        private const float DodgeStateSpeed = 1.8f;     // Roll is 1.47 s; the dash lasts 0.36 s
        private const float DodgeExit = 0.68f;
        private const float BlockPoseTime = 0.333f;     // the still guard frame inside Sword_Block (raise 0-0.13 s, lower from 0.65 s)
        private static readonly bool BlockOnUpperBodyLayer = true;

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

            // Heroes: every clip comes from the Quaternius libraries (see BuildHeroController), so one line
            // per prefab is all a hero needs. A new hero = Assets/Resources/Heroes/Hero_<Name>.prefab + its name
            // here (or its own line: Bake("Heroes/Hero_Viking", null, true, null, null);). Bone names are
            // normalised (Sketchfab "_NNN" suffixes, Mixamo names), see RigNames.
            foreach (var name in new[]
                     {
                         "Kwang", "KwangAlbino", "KwangFrost", "KwangManbun", "KwangSunrise",
                         "Gideon", "GideonInq", "GideonMeph", "Morigesh", "MorigeshMystic",
                         "Serath", "SerathGold", "SerathFrost", "Kallari", "KallariLotus",
                         "Khaimera", "KhaimeraPelt"
                     })
                Bake("Heroes/Hero_" + name, null, true, null, null);

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

                void Save(int states)
                {
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
                    Debug.Log($"[DemoAnimations] {resource}: {states} action states.");
                }

                if (hero)
                {
                    // Heroes get their own state machine; everything below this branch is the creature path.
                    Save(BuildHeroController(controller, machine, target, resource));
                    return;
                }

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

                if (hero)
                {
                    // Separate cinematic motion while the current carries the hero down the waterfall.
                    controller.AddParameter("WaterfallSwim", AnimatorControllerParameterType.Bool);
                    var swim = machine.AddState("WaterfallSwim");
                    swim.motion = BakeMotion(controller, target, sources, "Anim/QuaterniusUAL:AL_Standard#Swim_Fwd_Loop", true);
                    Connect(null, swim, AnimatorConditionMode.If, 0f, "WaterfallSwim", 0.28f);
                    Connect(swim, rest, AnimatorConditionMode.IfNot, 0f, "WaterfallSwim", 0.22f);
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

                Save(actions.Length);
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

        /// <summary>
        /// Hero state machine, all clips from the Quaternius libraries:
        /// Locomotion (1D blend tree on Speed: idle, walk, jog, sprint), Attack1-4 (+ recoveries), Cast, Hit,
        /// Death, GetUp, JumpStart/JumpLoop/JumpLand (bool Grounded), Dodge (trigger Dodge), Block (bool
        /// Blocking, upper-body layer) and WaterfallSwim. Returns the number of base-layer states.
        /// </summary>
        private static int BuildHeroController(AnimatorController controller, AnimatorStateMachine machine,
            GameObject target, string resource)
        {
            controller.AddParameter("Speed", AnimatorControllerParameterType.Float);
            controller.AddParameter("MoveScale", AnimatorControllerParameterType.Float);
            controller.AddParameter("CombatSpeed", AnimatorControllerParameterType.Float);
            controller.AddParameter("Attack", AnimatorControllerParameterType.Trigger);
            controller.AddParameter("AttackIndex", AnimatorControllerParameterType.Int);
            controller.AddParameter("Die", AnimatorControllerParameterType.Trigger);
            controller.AddParameter("Respawn", AnimatorControllerParameterType.Trigger);
            controller.AddParameter("HitReact", AnimatorControllerParameterType.Trigger);
            controller.AddParameter("Blocking", AnimatorControllerParameterType.Bool);
            controller.AddParameter("Cast", AnimatorControllerParameterType.Trigger);
            controller.AddParameter("WaterfallSwim", AnimatorControllerParameterType.Bool);
            controller.AddParameter("Grounded", AnimatorControllerParameterType.Bool);
            controller.AddParameter("Dodge", AnimatorControllerParameterType.Trigger);
            var parameters = controller.parameters;
            foreach (var parameter in parameters)
            {
                if (parameter.name == "CombatSpeed" || parameter.name == "MoveScale") parameter.defaultFloat = 1f;
                if (parameter.name == "Grounded") parameter.defaultBool = true;
            }
            controller.parameters = parameters;

            AnimationClip Clip(string id, bool loop = false) => BakeMotion(controller, target, null, id, loop);

            AnimatorState State(string stateName, Motion motion, float speed = 1f)
            {
                var state = machine.AddState(stateName);
                state.motion = motion;
                state.speed = speed;
                return state;
            }

            // Condition-driven transition; a null source means "from any state" of the base layer.
            AnimatorStateTransition When(AnimatorState source, AnimatorState destination, float duration,
                AnimatorConditionMode mode, float threshold, string parameter)
            {
                var transition = source == null ? machine.AddAnyStateTransition(destination) : source.AddTransition(destination);
                transition.hasExitTime = false;
                transition.hasFixedDuration = true;
                transition.duration = duration;
                transition.canTransitionToSelf = false;
                transition.AddCondition(mode, threshold, parameter);
                return transition;
            }

            // Leaves the state by itself once its clip reaches exitTime (normalised).
            AnimatorStateTransition After(AnimatorState source, AnimatorState destination, float exitTime, float duration)
            {
                var transition = source.AddTransition(destination);
                transition.hasExitTime = true;
                transition.exitTime = exitTime;
                transition.hasFixedDuration = true;
                transition.duration = duration;
                return transition;
            }

            // ---- locomotion: thresholds are the ground speed each clip was authored for on THIS body, in
            // units of the animator root's scale per second (gameplay divides its speed by that scale).
            float Authored(float fallback, out float measured)
            {
                measured = _lastStanceSpeed;
                return measured > fallback * 0.5f && measured < fallback * 2f ? measured : fallback;
            }
            var idle = Clip(Al + "Idle_Loop", true);
            var walk = Clip(Al + "Walk_Loop", true);
            float walkSpeed = Authored(WalkAuthoredSpeed, out float walkMeasured);
            var jog = Clip(Al + "Jog_Fwd_Loop", true);
            float jogSpeed = Mathf.Max(Authored(JogAuthoredSpeed, out float jogMeasured), walkSpeed + 0.5f);
            var sprint = Clip(Al + "Sprint_Loop", true);
            float sprintSpeed = Mathf.Max(Authored(SprintAuthoredSpeed, out float sprintMeasured), jogSpeed + 0.5f);
            Debug.Log($"[DemoAnimations] {resource}: locomotion thresholds walk {walkSpeed:F2} (measured {walkMeasured:F2}), " +
                      $"jog {jogSpeed:F2} (measured {jogMeasured:F2}), sprint {sprintSpeed:F2} (measured {sprintMeasured:F2}).");

            var locomotion = controller.CreateBlendTreeInController("Locomotion", out BlendTree tree, 0);
            tree.blendType = BlendTreeType.Simple1D;
            tree.blendParameter = "Speed";
            tree.useAutomaticThresholds = false;
            tree.AddChild(idle, 0f);
            tree.AddChild(walk, walkSpeed);
            tree.AddChild(jog, jogSpeed);
            tree.AddChild(sprint, sprintSpeed);
            machine.defaultState = locomotion;
            // MoveScale stays 1 inside the blend range; gameplay only raises it above the sprint threshold.
            locomotion.speedParameter = "MoveScale";
            locomotion.speedParameterActive = true;
            float moving = walkSpeed * 1.5f;   // "the player is steering": recoveries may be cut short

            // ---- cinematic swim (waterfall prologue)
            var swim = State("WaterfallSwim", Clip(Al + "Swim_Fwd_Loop", true));
            When(null, swim, 0.28f, AnimatorConditionMode.If, 0f, "WaterfallSwim");
            When(swim, locomotion, 0.22f, AnimatorConditionMode.IfNot, 0f, "WaterfallSwim");

            // ---- sword combo. A and B end in their follow-through pose: the next step picks it up, otherwise
            // the matching recovery clip stands the hero back up. C and the finisher return by themselves.
            AnimatorState Attack(int index, AnimationClip clip, float speed, float fade)
            {
                var state = State("Attack" + index, clip, speed);
                state.speedParameter = "CombatSpeed";
                state.speedParameterActive = true;
                When(null, state, fade, AnimatorConditionMode.If, 0f, "Attack")
                    .AddCondition(AnimatorConditionMode.Equals, index, "AttackIndex");
                return state;
            }
            void Recover(AnimatorState attackState, string stateName, AnimationClip clip)
            {
                var recovery = State(stateName, clip);
                After(attackState, recovery, 0.9f, 0.04f);
                After(recovery, locomotion, Mathf.Clamp(1f - 0.2f / Mathf.Max(0.1f, clip.length), 0.6f, 0.97f), 0.2f);
                When(recovery, locomotion, 0.2f, AnimatorConditionMode.Greater, moving, "Speed");
            }
            Recover(Attack(1, Clip(Ual2 + "Sword_Regular_A"), 1f, 0.08f), "Attack1Recover", Clip(Ual2 + "Sword_Regular_A_Rec"));
            Recover(Attack(2, Clip(Ual2 + "Sword_Regular_B"), 1f, 0.06f), "Attack2Recover", Clip(Ual2 + "Sword_Regular_B_Rec"));
            After(Attack(3, Clip(Ual2 + "Sword_Regular_C"), Attack3StateSpeed, 0.08f), locomotion, HeavyAttackExit, 0.3f);
            After(Attack(4, Clip(Al + "Sword_Attack"), 1f, 0.1f), locomotion, HeavyAttackExit, 0.3f);

            // ---- cast, hit, death, get-up
            var cast = State("Cast", Clip(Al + "Spell_Simple_Shoot"));
            When(null, cast, 0.12f, AnimatorConditionMode.If, 0f, "Cast");
            After(cast, locomotion, 0.7f, 0.15f);

            var hit = State("Hit", Clip(Al + "Hit_Chest"));
            When(null, hit, 0.06f, AnimatorConditionMode.If, 0f, "HitReact");
            After(hit, locomotion, 0.6f, 0.13f);

            // Nothing but the respawn trigger may pull a corpse back up.
            var death = State("Death", Clip(Al + "Death01"));
            When(null, death, 0.15f, AnimatorConditionMode.If, 0f, "Die");
            if (HasTake(Ual2 + "LayToIdle"))
            {
                var getUp = State("GetUp", Clip(Ual2 + "LayToIdle"));
                When(death, getUp, 0.25f, AnimatorConditionMode.If, 0f, "Respawn");
                After(getUp, locomotion, 0.85f, 0.2f);
                When(getUp, locomotion, 0.25f, AnimatorConditionMode.Greater, moving, "Speed");
            }
            else
            {
                Debug.LogWarning($"[DemoAnimations] {resource}: no LayToIdle take, respawn snaps to locomotion.");
                When(death, locomotion, 0.2f, AnimatorConditionMode.If, 0f, "Respawn");
            }

            // ---- jump: Grounded goes false -> take-off, held air pose, landing when it turns true again.
            var jumpStart = State("JumpStart", Clip(Al + "Jump_Start"));
            var jumpLoop = State("JumpLoop", Clip(Al + "Jump_Loop", true));
            var jumpLand = State("JumpLand", Clip(Al + "Jump_Land"));
            // The first two frames of Jump_Start are the crouch; the hero is already off the ground.
            When(locomotion, jumpStart, 0.1f, AnimatorConditionMode.IfNot, 0f, "Grounded").offset = 0.05f;
            After(jumpStart, jumpLoop, 0.85f, 0.2f);
            When(jumpStart, jumpLand, 0.1f, AnimatorConditionMode.If, 0f, "Grounded");
            When(jumpLoop, jumpLand, 0.1f, AnimatorConditionMode.If, 0f, "Grounded");
            After(jumpLand, locomotion, 0.55f, 0.25f);
            When(jumpLand, locomotion, 0.25f, AnimatorConditionMode.Greater, moving, "Speed");

            // ---- dodge roll (a second dash restarts it)
            var dodge = State("Dodge", Clip(Al + "Roll"), DodgeStateSpeed);
            When(null, dodge, 0.06f, AnimatorConditionMode.If, 0f, "Dodge").canTransitionToSelf = true;
            After(dodge, locomotion, DodgeExit, 0.15f);

            // ---- block: the held guard frame of Sword_Block. On its own upper-body layer the legs keep
            // walking underneath; without a spine bone to mask from it becomes a plain base-layer state.
            var guard = BakeMotion(controller, target, null, Ual2 + "Sword_Block", true, BlockPoseTime, -1f, true);
            guard.name = "Sword_Block_Hold";
            var all = target.GetComponentsInChildren<Transform>(true);
            var names = RigNames(target.transform);
            Transform spine = all.FirstOrDefault(t => string.Equals(names == null ? t.name : names(t.name), "spine_01", StringComparison.OrdinalIgnoreCase));
            if (BlockOnUpperBodyLayer && spine != null)
            {
                var mask = new AvatarMask { name = "UpperBody" };
                mask.transformCount = all.Length;
                for (int i = 0; i < all.Length; i++)
                {
                    mask.SetTransformPath(i, AnimationUtility.CalculateTransformPath(all[i], target.transform));
                    mask.SetTransformActive(i, all[i].IsChildOf(spine));
                }
                AssetDatabase.AddObjectToAsset(mask, controller);
                controller.AddLayer("UpperBody");
                var layers = controller.layers;
                layers[1].avatarMask = mask;
                layers[1].defaultWeight = 1f;
                layers[1].blendingMode = AnimatorLayerBlendingMode.Override;
                controller.layers = layers;
                var upper = layers[1].stateMachine;
                var empty = upper.AddState("Empty");
                upper.defaultState = empty;
                var block = upper.AddState("Block");
                block.motion = guard;
                When(empty, block, 0.12f, AnimatorConditionMode.If, 0f, "Blocking");
                When(block, empty, 0.18f, AnimatorConditionMode.IfNot, 0f, "Blocking");
            }
            else
            {
                var block = State("Block", guard);
                When(locomotion, block, 0.12f, AnimatorConditionMode.If, 0f, "Blocking");
                When(block, locomotion, 0.18f, AnimatorConditionMode.IfNot, 0f, "Blocking");
            }

            return machine.states.Length;
        }

        /// <summary>Ground speed of the last baked clip (stance-foot travel, in units of the target root's scale
        /// per second), 0 when it could not be measured. Read right after BakeMotion.</summary>
        private static float _lastStanceSpeed;

        /// <param name="windowStart">Start of the baked window in the source clip, seconds.</param>
        /// <param name="windowEnd">End of the window; negative = clip end.</param>
        /// <param name="pose">Bake the single frame at <paramref name="windowStart"/> as a held pose.</param>
        private static AnimationClip BakeMotion(AnimatorController controller, GameObject target, string folder,
            string name, bool loop, float windowStart = 0f, float windowEnd = -1f, bool pose = false)
        {
            _lastStanceSpeed = 0f;
            int separator = name.IndexOf(':');
            if (separator >= 0)
            {
                folder = name.Substring(0, separator);
                name = name.Substring(separator + 1);
            }
            // "File#Clip" picks one take from a multi-clip library FBX (the Quaternius libraries).
            string take = null;
            int hash = name.IndexOf('#');
            if (hash >= 0) { take = name.Substring(hash + 1); name = name.Substring(0, hash); }
            string path = $"{Sources}{folder}/Animations/{name}.fbx";
            var sourceAsset = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            var clips = AssetDatabase.LoadAllAssetsAtPath(path).OfType<AnimationClip>()
                .Where(c => !c.name.StartsWith("__preview__", StringComparison.Ordinal)).ToList();
            var clip = clips.FirstOrDefault(c => take == null || IsTake(c, take));
            if (sourceAsset == null || clip == null || clip.length <= 0f)
                throw new InvalidOperationException($"Missing usable clip at {path}{(take != null ? "#" + take : "")}.");
            // Baked window inside the source clip.
            windowStart = Mathf.Clamp(windowStart, 0f, clip.length);
            windowEnd = windowEnd < 0f ? clip.length : Mathf.Clamp(windowEnd, windowStart, clip.length);
            if (pose) windowEnd = windowStart;
            float duration = pose ? 0.5f : windowEnd - windowStart;
            if (!pose && duration <= 0.001f) throw new InvalidOperationException($"{path}: empty bake window.");

            var source = Object.Instantiate(sourceAsset);
            try
            {
                var sourceNames = RigNames(source.transform);
                var targetNames = RigNames(target.transform);
                var sourceBones = UniqueBones(source.transform, sourceNames);
                bool legacyDragonRig = folder == "Dragon" && sourceBones.ContainsKey("Root_Pelvis");
                if (legacyDragonRig)
                    sourceBones = UniqueBones(source.transform, name => name.Replace("_Left", "_L").Replace("_Right", "_R"));
                var targetBones = UniqueBones(target.transform, targetNames);
                string SourceName(string n) => legacyDragonRig ? DragonSourceBone(n) : n;
                // Bone name as the solver sees it: the raw name, or the normalised one on Sketchfab/Mixamo rigs.
                string NameOf(Transform t) => targetNames == null ? t.name : targetNames(t.name);

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
                // The delta retarget strips root motion with the source root alone and stands the body on the
                // target's own ground point, so it also works for targets without a "root" bone (Mixamo).
                Transform sourceRootBone = sourceRoot != null ? sourceRoot : source.transform;
                Transform targetGround = targetRoot != null ? targetRoot : target.transform;
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
                    var primary = kids.Where(k => !NameOf(k).ToLowerInvariant().Contains("twist"))
                        .OrderByDescending(k => IsSided(NameOf(k)) ? 0 : 1)
                        .ThenByDescending(k => k.GetComponentsInChildren<Transform>(true).Count(match.ContainsKey))
                        .FirstOrDefault();
                    if (primary != null)
                    {
                        plan.TargetPrimary = primary;
                        plan.SourcePrimary = match[primary];
                        // Twist reference: the two remaining children farthest apart (thighs of the pelvis,
                        // clavicles of the chest, thumb/pinky of the hand).
                        var others = kids.Where(k => k != primary && !NameOf(k).ToLowerInvariant().Contains("twist")).ToList();
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

                // Aim every bone down its source direction, parents first, then roll it about that
                // direction so its side children line up too. No rest pose is involved, so a clip whose
                // first frame is not the bind pose can no longer twist the whole skeleton.
                void AimSolve(Quaternion map)
                {
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
                }

                // ---- Rotation-delta retarget (keeps limb roll). Needs a reference take in the source file (the
                // library's T-pose). The target's rest pose is aimed ONCE onto that reference pose, which pairs
                // "source reference" with an anatomically equal "target reference" whatever the two rest poses
                // are (A-pose vs T-pose). Every frame then replays the source bone's world-space rotation away
                // from its reference on top of the target reference:
                //     target(t) = M(t) * source(t) * source(ref)^-1 * M(ref)^-1 * target(ref)
                // M is a yaw that lines the source's left-right axis up with the target's and takes the source
                // root's own turning out again. Sources without a reference take (creatures, dragon, the Paragon
                // clip files) keep the aim solver.
                var referenceClip = UseDeltaRetarget ? clips.FirstOrDefault(c => IsTake(c, ReferenceTake)) : null;
                bool delta = referenceClip != null && !legacyDragonRig &&
                             sourceBones.ContainsKey("upperarm_l") && sourceBones.ContainsKey("upperarm_r") &&
                             targetBones.ContainsKey("upperarm_l") && targetBones.ContainsKey("upperarm_r");
                Vector3 SourceFacing() => Vector3.ProjectOnPlane(sourceRootBone.rotation * Vector3.right, Vector3.up);
                Quaternion mapRef = Quaternion.identity;
                Vector3 facingRef = Vector3.zero;
                var twists = new List<TwistPlan>();
                if (delta)
                {
                    var sourceAll = source.GetComponentsInChildren<Transform>(true);
                    var sourceRestRot = sourceAll.Select(t => t.localRotation).ToArray();
                    var sourceRestPos = sourceAll.Select(t => t.localPosition).ToArray();
                    var sourceRestScale = sourceAll.Select(t => t.localScale).ToArray();
                    referenceClip.SampleAnimation(source, 0f);
                    RestoreTarget();
                    mapRef = Quaternion.AngleAxis(
                        Vector3.SignedAngle(RightAxis(sourceBones), RightAxis(targetBones), Vector3.up), Vector3.up);
                    facingRef = SourceFacing();
                    AimSolve(mapRef);
                    foreach (var p in plans)
                    {
                        p.SourceRef = p.Source.rotation;
                        p.TargetRef = p.Target.rotation;
                    }
                    for (int i = 0; i < sourceAll.Length; i++)
                    {
                        sourceAll[i].localRotation = sourceRestRot[i];
                        sourceAll[i].localPosition = sourceRestPos[i];
                        sourceAll[i].localScale = sourceRestScale[i];
                    }
                    RestoreTarget();

                    // Forearm twist bones the source rig does not have: they take a share of the hand's twist
                    // about the forearm (measured against the target's own bind pose) so the wrist skin does
                    // not candy-wrap. The share grows with the bone's position along the forearm.
                    if (DistributeForearmTwist)
                        foreach (string side in new[] { "_l", "_r" })
                        {
                            if (!targetBones.TryGetValue("lowerarm" + side, out var lower) ||
                                !targetBones.TryGetValue("hand" + side, out var hand) || hand.parent != lower ||
                                !match.ContainsKey(hand) || hand.localPosition.sqrMagnitude < 1e-8f) continue;
                            foreach (Transform child in lower)
                            {
                                if (match.ContainsKey(child) ||
                                    !NameOf(child).ToLowerInvariant().StartsWith("lowerarm_twist", StringComparison.Ordinal)) continue;
                                float share = Mathf.Clamp01(Vector3.Dot(child.localPosition, hand.localPosition) / hand.localPosition.sqrMagnitude);
                                if (share < 0.05f) share = 0.5f;
                                twists.Add(new TwistPlan
                                {
                                    Bone = child, Hand = hand, Share = share,
                                    BoneRest = child.localRotation, HandRest = hand.localRotation,
                                    Path = AnimationUtility.CalculateTransformPath(child, target.transform)
                                });
                            }
                        }
                }

                var bones = delta ? plans.ToList() : plans.Where(p => p.TargetPrimary != null).ToList();
                foreach (var twist in twists) bones.Add(new SolvePlan { Target = twist.Bone, Path = twist.Path });
                int frames = pose ? 2 : Mathf.Max(2, Mathf.CeilToInt(duration * 30f) + 1);
                var bindings = new List<EditorCurveBinding>();
                var curves = new List<AnimationCurve>();
                var poses = new Quaternion[bones.Count, frames];
                var hipPositions = new Vector3[frames];
                // Ground contact points for the authored-speed measurement (toes, else ankles).
                Transform contactL = targetBones.TryGetValue("ball_l", out var ballL) ? ballL : targetBones.TryGetValue("foot_l", out var footL) ? footL : null;
                Transform contactR = targetBones.TryGetValue("ball_r", out var ballR) ? ballR : targetBones.TryGetValue("foot_r", out var footR) ? footR : null;
                var contactsL = new Vector3[frames];
                var contactsR = new Vector3[frames];
                float largestMotion = 0f;
                try
                {
                    for (int frame = 0; frame < frames; frame++)
                    {
                        float time = windowStart + (windowEnd - windowStart) * frame / (frames - 1);
                        clip.SampleAnimation(source, time);
                        RestoreTarget();

                        // Maps a source world vector into target world space: a yaw only. The FBX root bone carries
                        // the Z-up conversion (-90 X) while the glTF root does not, so the full root rotation would
                        // tip the whole body over; the root X axis is horizontal in both and gives the facing.
                        Quaternion map = fallbackAlign;
                        if (delta)
                        {
                            Vector3 facing = SourceFacing();
                            Quaternion rootYaw = facingRef.sqrMagnitude > 1e-4f && facing.sqrMagnitude > 1e-4f
                                ? Quaternion.AngleAxis(Vector3.SignedAngle(facingRef, facing, Vector3.up), Vector3.up)
                                : Quaternion.identity;
                            map = mapRef * Quaternion.Inverse(rootYaw);
                        }
                        else if (rootFrames)
                        {
                            Vector3 sx = Vector3.ProjectOnPlane(sourceRoot.rotation * Vector3.right, Vector3.up);
                            Vector3 tx = Vector3.ProjectOnPlane(targetRoot.rotation * Vector3.right, Vector3.up);
                            if (sx.sqrMagnitude > 1e-4f && tx.sqrMagnitude > 1e-4f) map = Quaternion.FromToRotation(sx.normalized, tx.normalized);
                        }

                        if (bakeHip)
                        {
                            targetHip.position = delta
                                ? targetGround.position + map * (sourceHip.position - sourceRootBone.position) * hipRatio
                                : targetRoot.position + map * (sourceHip.position - sourceRoot.position) * hipRatio;
                            hipPositions[frame] = targetHip.localPosition;
                        }

                        if (delta)
                        {
                            Quaternion unmap = Quaternion.Inverse(mapRef);
                            foreach (var p in plans)
                                p.Target.rotation = map * (p.Source.rotation * Quaternion.Inverse(p.SourceRef)) * unmap * p.TargetRef;
                            foreach (var twist in twists)
                            {
                                // Twist part (about the forearm) of the hand's rotation away from its bind pose.
                                Quaternion r = twist.Hand.localRotation * Quaternion.Inverse(twist.HandRest);
                                Vector3 axis = twist.Hand.localPosition.normalized;
                                Vector3 v = Vector3.Dot(new Vector3(r.x, r.y, r.z), axis) * axis;
                                float magnitude = Mathf.Sqrt(v.sqrMagnitude + r.w * r.w);
                                Quaternion roll = Quaternion.identity;
                                if (magnitude > 1e-4f)
                                {
                                    float sign = r.w < 0f ? -1f : 1f;
                                    roll = new Quaternion(sign * v.x / magnitude, sign * v.y / magnitude, sign * v.z / magnitude, sign * r.w / magnitude);
                                }
                                twist.Bone.localRotation = Quaternion.Slerp(Quaternion.identity, roll, twist.Share) * twist.BoneRest;
                            }
                        }
                        else
                        {
                            AimSolve(map);
                        }

                        if (contactL != null) contactsL[frame] = contactL.position - targetGround.position;
                        if (contactR != null) contactsR[frame] = contactR.position - targetGround.position;

                        for (int i = 0; i < bones.Count; i++)
                        {
                            Quaternion pose1 = bones[i].Target.localRotation;
                            if (frame > 0 && Quaternion.Dot(poses[i, frame - 1], pose1) < 0f)
                                pose1 = new Quaternion(-pose1.x, -pose1.y, -pose1.z, -pose1.w);
                            poses[i, frame] = pose1;
                            if (frame > 0) largestMotion = Mathf.Max(largestMotion, Quaternion.Angle(poses[i, 0], pose1));
                        }
                    }
                }
                finally
                {
                    RestoreTarget();
                }
                if (!pose && largestMotion < 1f) throw new InvalidOperationException($"{path}: the imported clip did not move the rig.");
                if (!pose && contactL != null && contactR != null)
                {
                    float rootScale = Mathf.Max(1e-4f, Mathf.Abs(target.transform.lossyScale.y));
                    _lastStanceSpeed = StanceSpeed(contactsL, contactsR, duration / (frames - 1)) / rootScale;
                }

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
                            track[k] = new Keyframe(duration * frame / (frames - 1), poses[i, frame][j]);
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
                            track[frame] = new Keyframe(duration * frame / (frames - 1), hipPositions[frame][axis]);
                        var curve = new AnimationCurve(track);
                        for (int k = 0; k < track.Length; k++) curve.SmoothTangents(k, 0f);
                        bindings.Add(EditorCurveBinding.FloatCurve(hipPath, typeof(Transform), "m_LocalPosition." + "xyz"[axis]));
                        curves.Add(curve);
                    }
                }

                var result = new AnimationClip { name = take ?? name, frameRate = 30f };
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
            /// <summary>World rotations in the shared reference pose (delta retarget only).</summary>
            public Quaternion SourceRef, TargetRef;
        }

        /// <summary>A target forearm twist bone that follows a share of its hand's twist.</summary>
        private sealed class TwistPlan
        {
            public Transform Bone, Hand;
            public Quaternion BoneRest, HandRest;
            public float Share;
            public string Path;
        }

        private static bool IsTake(AnimationClip clip, string take) =>
            clip.name == take || clip.name.EndsWith("|" + take, StringComparison.Ordinal);

        /// <summary>True when "Folder:File#Take" names an existing take.</summary>
        private static bool HasTake(string id)
        {
            int separator = id.IndexOf(':');
            int hash = id.IndexOf('#');
            if (separator < 0 || hash < separator) return false;
            string path = $"{Sources}{id.Substring(0, separator)}/Animations/{id.Substring(separator + 1, hash - separator - 1)}.fbx";
            string take = id.Substring(hash + 1);
            return AssetDatabase.LoadAllAssetsAtPath(path).OfType<AnimationClip>().Any(c => IsTake(c, take));
        }

        /// <summary>
        /// Median horizontal speed of the feet while they are planted (lowest tenth of their height range):
        /// the ground speed a looping locomotion clip was authored for. 0 when there is too little contact.
        /// </summary>
        private static float StanceSpeed(Vector3[] left, Vector3[] right, float frameTime)
        {
            if (frameTime <= 0f) return 0f;
            var samples = new List<float>();
            foreach (var points in new[] { left, right })
            {
                float low = points.Min(v => v.y);
                float high = points.Max(v => v.y);
                float limit = low + Mathf.Max(0.1f * (high - low), 0.004f);
                for (int i = 0; i + 1 < points.Length; i++)
                {
                    if (points[i].y > limit || points[i + 1].y > limit) continue;
                    Vector3 step = points[i + 1] - points[i];
                    step.y = 0f;
                    samples.Add(step.magnitude / frameTime);
                }
            }
            if (samples.Count < 3) return 0f;
            samples.Sort();
            return samples[samples.Count / 2];
        }

        // ------------------------------------------------------------------ bone name normalisation

        private static readonly Regex NumberedMannequinBone = new Regex(@"^(spine|neck)_\d\d$", RegexOptions.IgnoreCase);

        /// <summary>
        /// Name mapper for rigs whose bones are not plain UE-mannequin names, or null when the names are used
        /// as they are (every Paragon hero, creature and library file: their matching is unchanged).
        /// Sketchfab GLB exports (recognised by their "Sketchfab_model"/"_rootJoint" nodes) append "_NNN" to
        /// every joint; Mixamo rigs ("mixamorig:" prefix) are renamed to the mannequin names.
        /// </summary>
        private static Func<string, string> RigNames(Transform root)
        {
            bool sketchfab = false, mixamo = false;
            foreach (var t in root.GetComponentsInChildren<Transform>(true))
            {
                if (t.name == "Sketchfab_model" || t.name == "_rootJoint") sketchfab = true;
                if (t.name.StartsWith("mixamorig", StringComparison.OrdinalIgnoreCase)) mixamo = true;
            }
            if (!sketchfab && !mixamo) return null;
            return raw => NormaliseBoneName(raw, sketchfab, mixamo);
        }

        /// <summary>"spine_01_014" -> "spine_01", "pelvis_01" -> "pelvis", "mixamorig:LeftForeArm_010" -> "lowerarm_l".</summary>
        private static string NormaliseBoneName(string raw, bool stripExportSuffix, bool mixamo)
        {
            string s = raw.Trim();
            if (stripExportSuffix && !NumberedMannequinBone.IsMatch(s))
            {
                // Only the LAST "_<digits>" group is the exporter's index.
                int underscore = s.LastIndexOf('_');
                bool digits = underscore > 0 && underscore < s.Length - 1;
                for (int i = underscore + 1; digits && i < s.Length; i++) digits = char.IsDigit(s[i]);
                if (digits) s = s.Substring(0, underscore);
            }
            if (!mixamo || !s.StartsWith("mixamorig", StringComparison.OrdinalIgnoreCase)) return s;

            string low = s.Substring("mixamorig".Length).TrimStart(':', '_').ToLowerInvariant();
            switch (low)
            {
                case "hips": return "pelvis";
                case "spine": return "spine_01";
                case "spine1": return "spine_02";
                case "spine2": return "spine_03";
                case "neck": return "neck_01";
                case "head": return "head";
            }
            string side;
            if (low.StartsWith("left", StringComparison.Ordinal)) { side = "_l"; low = low.Substring(4); }
            else if (low.StartsWith("right", StringComparison.Ordinal)) { side = "_r"; low = low.Substring(5); }
            else return low;
            switch (low)
            {
                case "shoulder": return "clavicle" + side;
                case "arm": return "upperarm" + side;
                case "forearm": return "lowerarm" + side;
                case "hand": return "hand" + side;
                case "upleg": return "thigh" + side;
                case "leg": return "calf" + side;
                case "foot": return "foot" + side;
                case "toebase": return "ball" + side;
            }
            string finger = low.StartsWith("hand", StringComparison.Ordinal) ? low.Substring(4) : low;
            foreach (string digit in new[] { "thumb", "index", "middle", "ring", "pinky" })
                if (finger.Length == digit.Length + 1 && finger.StartsWith(digit, StringComparison.Ordinal) && char.IsDigit(finger[finger.Length - 1]))
                    return $"{digit}_0{finger[finger.Length - 1]}{side}";
            return (side == "_l" ? "left" : "right") + low;   // unmapped (toe ends...): stays unique, matches nothing
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
