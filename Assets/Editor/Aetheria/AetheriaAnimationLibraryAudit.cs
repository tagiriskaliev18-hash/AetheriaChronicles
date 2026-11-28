using System.Linq;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Animations;
using UnityEngine.Playables;

namespace Aetheria.Editor
{
    /// <summary>Reports imported clips and rig compatibility without changing existing hero controllers.</summary>
    public static class AetheriaAnimationLibraryAudit
    {
        private const string Folder = "Assets/_AAA_Imported/KayKitAnimations";

        [MenuItem("Aetheria/Audit Kwang Materials")]
        public static void ProbeHeroMaterials()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Resources/Heroes/Hero_Kwang.prefab");
            var look = prefab != null ? prefab.GetComponent<AetheriaCharacterLook>() : null;
            if (look == null)
            {
                Debug.LogWarning("[MaterialAudit] Hero material slot metadata is missing.");
                foreach (var renderer in prefab.GetComponentsInChildren<SkinnedMeshRenderer>(true))
                {
                    var mesh = renderer.sharedMesh;
                    if (mesh == null) continue;
                    var weights = mesh.boneWeights;
                    for (int i = 0; i < mesh.subMeshCount; i++)
                    {
                        var sums = new Dictionary<string, float>();
                        foreach (int vertex in mesh.GetIndices(i).Distinct())
                        {
                            if (vertex >= weights.Length) continue;
                            var w = weights[vertex];
                            void Add(int index, float weight)
                            {
                                if (weight <= 0f || index >= renderer.bones.Length || renderer.bones[index] == null) return;
                                var bone = renderer.bones[index].name;
                                sums[bone] = sums.TryGetValue(bone, out var total) ? total + weight : weight;
                            }
                            Add(w.boneIndex0, w.weight0); Add(w.boneIndex1, w.weight1);
                            Add(w.boneIndex2, w.weight2); Add(w.boneIndex3, w.weight3);
                        }
                        var top = sums.OrderByDescending(p => p.Value).Take(4).Select(p => p.Key + ":" + p.Value.ToString("F0"));
                        string mat = i < renderer.sharedMaterials.Length && renderer.sharedMaterials[i] != null ? renderer.sharedMaterials[i].name : "none";
                        Debug.Log($"[MaterialAudit] {renderer.name} slot={i}, material={mat}, bones={string.Join(",", top)}");
                    }
                }
                return;
            }
            foreach (var entry in look.Renderers)
            {
                if (entry.Renderer == null) continue;
                var mats = entry.Renderer.sharedMaterials;
                for (int i = 0; i < mats.Length; i++)
                {
                    var m = mats[i];
                    var slot = i < entry.Classes.Length ? entry.Classes[i] : SlotClass.Unknown;
                    Debug.Log($"[MaterialAudit] renderer={entry.Renderer.name} slot={i} class={slot} material={(m != null ? m.name : "null")} metallic={(m != null && m.HasProperty("_Metallic") ? m.GetFloat("_Metallic") : -1f)} smooth={(m != null && m.HasProperty("_Smoothness") ? m.GetFloat("_Smoothness") : -1f)}");
                }
            }
        }

        [MenuItem("Aetheria/Probe Kwang Humanoid Avatar")]
        public static void ProbeHeroAvatar()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Resources/Heroes/Hero_Kwang.prefab");
            if (prefab == null) { Debug.LogError("[AnimationAudit] Kwang prefab is missing."); return; }
            var hero = Object.Instantiate(prefab);
            try
            {
                var mapping = new Dictionary<string, string>
                {
                    { "Hips", "pelvis" }, { "Spine", "spine_01" }, { "Chest", "spine_02" },
                    { "UpperChest", "spine_03" }, { "Neck", "neck_01" }, { "Head", "head" },
                    { "LeftShoulder", "clavicle_l" }, { "LeftUpperArm", "upperarm_l" },
                    { "LeftLowerArm", "lowerarm_l" }, { "LeftHand", "hand_l" },
                    { "RightShoulder", "clavicle_r" }, { "RightUpperArm", "upperarm_r" },
                    { "RightLowerArm", "lowerarm_r" }, { "RightHand", "hand_r" },
                    { "LeftUpperLeg", "thigh_l" }, { "LeftLowerLeg", "calf_l" },
                    { "LeftFoot", "foot_l" }, { "LeftToes", "ball_l" },
                    { "RightUpperLeg", "thigh_r" }, { "RightLowerLeg", "calf_r" },
                    { "RightFoot", "foot_r" }, { "RightToes", "ball_r" }
                };
                var transforms = hero.GetComponentsInChildren<Transform>(true);
                var found = new HashSet<string>(transforms.Select(t => t.name));
                foreach (var pair in mapping)
                    if (!found.Contains(pair.Value)) Debug.LogWarning($"[AnimationAudit] Missing hero bone {pair.Value}");
                var description = new HumanDescription
                {
                    human = mapping.Where(p => found.Contains(p.Value)).Select(p => new HumanBone
                    {
                        humanName = p.Key, boneName = p.Value, limit = new HumanLimit { useDefaultValues = true }
                    }).ToArray(),
                    skeleton = transforms.Select(t => new SkeletonBone
                    {
                        name = t.name, position = t.localPosition, rotation = t.localRotation, scale = t.localScale
                    }).ToArray(),
                    armStretch = 0.05f, legStretch = 0.05f, upperArmTwist = 0.5f, lowerArmTwist = 0.5f,
                    upperLegTwist = 0.5f, lowerLegTwist = 0.5f, feetSpacing = 0f
                };
                var avatar = AvatarBuilder.BuildHumanAvatar(hero, description);
                Debug.Log($"[AnimationAudit] Experimental Kwang avatar: valid={avatar != null && avatar.isValid}, humanoid={avatar != null && avatar.isHuman}");
                if (avatar != null && avatar.isValid && avatar.isHuman) PreviewSamples(hero, avatar);
                if (avatar != null) Object.DestroyImmediate(avatar);
            }
            finally { Object.DestroyImmediate(hero); }
        }

        private static void PreviewSamples(GameObject hero, Avatar avatar)
        {
            var animator = hero.GetComponent<Animator>();
            if (animator == null) { Debug.LogError("[AnimationAudit] Hero animator missing."); return; }
            animator.runtimeAnimatorController = null;
            animator.avatar = avatar;
            animator.applyRootMotion = false;
            var renderers = hero.GetComponentsInChildren<SkinnedMeshRenderer>(true);
            foreach (var renderer in renderers) renderer.updateWhenOffscreen = true;
            var kwang = renderers.FirstOrDefault(r => (r.transform == hero.transform || r.name == "Hero_Kwang") && r.sharedMaterials.Length >= 18);
            if (kwang != null)
            {
                var materials = kwang.sharedMaterials;
                foreach (int i in new[] { 6, 7, 8, 11, 13, 15, 17 })
                {
                    if (materials[i] == null) continue;
                    materials[i] = new Material(materials[i]) { name = materials[i].name + " (Preview)" };
                    var slot = i == 6 ? SlotClass.Eye : i == 13 ? SlotClass.Mouth : i == 17 ? SlotClass.Weapon : SlotClass.Skin;
                    AetheriaSurfaceDetail.Apply(materials[i], slot);
                    if (slot == SlotClass.Skin) AetheriaRender.SetBaseColor(materials[i], new Color(0.60f, 0.39f, 0.28f));
                }
                kwang.sharedMaterials = materials;
                Debug.Log($"[MaterialAudit] Preview hero root={hero.name}, renderer={kwang.name}, slots={materials.Length}");
            }
            var cameraObject = new GameObject("AnimationAuditCamera");
            var lightObject = new GameObject("AnimationAuditLight");
            var cam = cameraObject.AddComponent<Camera>();
            var light = lightObject.AddComponent<Light>();
            light.type = LightType.Directional;
            light.intensity = 3f;
            lightObject.transform.rotation = Quaternion.Euler(35f, -35f, 0f);
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.backgroundColor = new Color(0.18f, 0.18f, 0.18f);
            cam.fieldOfView = 35f;
            var rt = new RenderTexture(768, 768, 24);
            var tex = new Texture2D(768, 768, TextureFormat.RGB24, false);
            var outputDir = Path.Combine(Path.GetTempPath(), "AetheriaAnimationAudit");
            Directory.CreateDirectory(outputDir);
            string[] wanted = { "Idle_A", "Walking_A", "Running_A", "Jump_Start", "Jump_Idle", "Jump_Land" };
            try
            {
                foreach (string name in wanted)
                {
                    var clip = AssetDatabase.FindAssets("t:Model", new[] { Folder })
                        .SelectMany(g => AssetDatabase.LoadAllAssetsAtPath(AssetDatabase.GUIDToAssetPath(g)).OfType<AnimationClip>())
                        .FirstOrDefault(c => c.name == name);
                    if (clip == null) { Debug.LogError($"[AnimationAudit] Missing clip {name}"); continue; }
                    var graph = PlayableGraph.Create("AnimationAudit");
                    graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);
                    var playable = AnimationClipPlayable.Create(graph, clip);
                    var output = AnimationPlayableOutput.Create(graph, "Hero", animator);
                    output.SetSourcePlayable(playable);
                    graph.Play();
                    playable.SetTime(clip.length * 0.5f);
                    graph.Evaluate(0f);
                    var leftLeg = animator.GetBoneTransform(HumanBodyBones.LeftUpperLeg);
                    var leftArm = animator.GetBoneTransform(HumanBodyBones.LeftUpperArm);
                    Debug.Log($"[AnimationAudit] {name}: humanMotion={clip.humanMotion}, leg={leftLeg?.localRotation.eulerAngles}, arm={leftArm?.localRotation.eulerAngles}");
                    var bounds = renderers[0].bounds;
                    foreach (var renderer in renderers.Skip(1)) bounds.Encapsulate(renderer.bounds);
                    var height = Mathf.Max(1f, bounds.size.y);
                    cam.transform.position = bounds.center + new Vector3(height * 1.7f, height * 0.22f, -height * 2.4f);
                    cam.transform.LookAt(bounds.center);
                    cam.targetTexture = rt;
                    cam.Render(); // warm the render pipeline before capturing the first sample
                    cam.Render();
                    var previous = RenderTexture.active;
                    RenderTexture.active = rt;
                    tex.ReadPixels(new Rect(0, 0, rt.width, rt.height), 0, 0);
                    tex.Apply();
                    RenderTexture.active = previous;
                    File.WriteAllBytes(Path.Combine(outputDir, name + ".png"), tex.EncodeToPNG());
                    Debug.Log($"[AnimationAudit] Sampled {name}: bounds={bounds.size}, output={outputDir}");
                    graph.Destroy();
                }
            }
            finally
            {
                cam.targetTexture = null;
                Object.DestroyImmediate(tex); Object.DestroyImmediate(rt);
                Object.DestroyImmediate(cameraObject); Object.DestroyImmediate(lightObject);
            }
        }

        [MenuItem("Aetheria/Configure KayKit Source Humanoid Rigs")]
        public static void ConfigureSources()
        {
            foreach (string guid in AssetDatabase.FindAssets("t:Model", new[] { Folder }))
            {
                string path = AssetDatabase.GUIDToAssetPath(guid);
                if (!(AssetImporter.GetAtPath(path) is ModelImporter importer)) continue;
                importer.animationType = ModelImporterAnimationType.Human;
                importer.avatarSetup = ModelImporterAvatarSetup.CreateFromThisModel;
                importer.SaveAndReimport();
                var avatar = AssetDatabase.LoadAllAssetsAtPath(path).OfType<Avatar>().FirstOrDefault();
                if (avatar == null || !avatar.isValid || !avatar.isHuman)
                {
                    Debug.LogWarning($"[AnimationAudit] {path}: humanoid avatar could not be created; restoring Generic import.");
                    importer.animationType = ModelImporterAnimationType.Generic;
                    importer.SaveAndReimport();
                }
            }
            Run();
        }

        [MenuItem("Aetheria/Audit KayKit Animation Library")]
        public static void Run()
        {
            int total = 0;
            foreach (string guid in AssetDatabase.FindAssets("t:Model", new[] { Folder }))
            {
                string path = AssetDatabase.GUIDToAssetPath(guid);
                var importer = AssetImporter.GetAtPath(path) as ModelImporter;
                var clips = AssetDatabase.LoadAllAssetsAtPath(path).OfType<AnimationClip>()
                    .Where(c => !c.name.StartsWith("__preview__")).ToArray();
                var avatar = AssetDatabase.LoadAllAssetsAtPath(path).OfType<Avatar>().FirstOrDefault();
                total += clips.Length;
                Debug.Log($"[AnimationAudit] {path}: clips={clips.Length}, rig={importer?.animationType}, avatarValid={avatar != null && avatar.isValid}, humanoid={avatar != null && avatar.isHuman}");
                foreach (var clip in clips) Debug.Log($"[AnimationAudit] clip {clip.name} ({clip.length:F2}s)");
            }
            var hero = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Resources/Heroes/Hero_Kwang.prefab");
            var animator = hero != null ? hero.GetComponentInChildren<Animator>(true) : null;
            var heroAvatar = animator != null ? animator.avatar : null;
            Debug.Log($"[AnimationAudit] Hero_Kwang avatarValid={heroAvatar != null && heroAvatar.isValid}, humanoid={heroAvatar != null && heroAvatar.isHuman}; totalSourceClips={total}");
        }
    }
}
