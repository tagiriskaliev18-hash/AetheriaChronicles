using System.Collections;
using UnityEngine;

namespace Aetheria
{
    internal static class AetheriaDemoVerification
    {
        public static IEnumerator Run()
        {
            yield return null;
            var hero = HeroController.Instance;
            var hud = AetheriaHUDManager.Instance;
            var quests = QuestAndMapService.Instance;
            if (!Check(hero != null && hud != null && quests != null, "Scene services initialized")) yield break;

            var terrain = Terrain.activeTerrain;
            if (!Check(terrain != null && terrain.GetComponent<TerrainCollider>()?.terrainData == terrain.terrainData,
                "Visible terrain and collision share a heightmap")) yield break;
            var bridge = GameObject.Find("MirrorLakeStoneBridge");
            if (!Check(bridge != null && bridge.GetComponentInChildren<MeshCollider>() != null,
                "Stone bridge has mesh collision")) yield break;

            hero.StartPrologueAwakening();
            var tutor = AetheriaNPC.AllNPCs.Find(npc => npc.NpcId == "Npc_CaveTutor");
            hud.SetOverlay(OverlayMode.None);
            yield return new WaitForSeconds(0.5f);
            Debug.Log($"[DemoVerification] Prologue spawn {hero.transform.position}, tutor {tutor?.transform.position}");
            if (!Check(tutor != null && Vector3.Distance(hero.transform.position, tutor.transform.position) < 4.4f &&
                hero.GetComponent<CharacterController>().isGrounded, "Prologue spawns on solid ground in dialogue range")) yield break;

            quests.AcceptQuest("Q_PrologueAwakening");
            var prologue = quests.FindQuest("Q_PrologueAwakening");
            hero.TeleportSafe(prologue.ObjectiveWorldLocation);
            yield return new WaitForSeconds(4.5f);
            Debug.Log($"[DemoVerification] Cliff exit {hero.transform.position}, stage {hero.PrologueStage}, quest {prologue.State}");
            if (!Check(prologue.State == QuestState.Completed && hero.PrologueStage == 4 &&
                hero.GetComponent<CharacterController>().isGrounded && AetheriaWorld.SampleHeight(hero.transform.position) > 0.25f,
                "Cliff trigger completes the prologue and lands on dry ground")) yield break;

            var checkpoint = hero.transform.position;
            hero.transform.position = checkpoint - Vector3.up * 2f;
            yield return new WaitForSeconds(0.8f);
            if (!Check(Vector3.Distance(checkpoint, hero.transform.position) < 1f &&
                hero.GetComponent<CharacterController>().isGrounded, "Recovery returns to the last dry checkpoint")) yield break;

            var boss = EnemyAndBossAI.ActiveBoss;
            if (!Check(boss != null && boss.Anim.Valid, "Malakor has its animation controller")) yield break;
            var heroAnimator = hero.GetComponentInChildren<Animator>();
            if (!Check(heroAnimator != null && heroAnimator.isActiveAndEnabled &&
                heroAnimator.runtimeAnimatorController != null, "Default hero has its animation controller")) yield break;

            var outlier = GameObject.Find("RisenOutrider_1")?.GetComponent<EnemyAndBossAI>();
            if (!Check(outlier != null && outlier.Anim.Valid, "Outrider has its animation controller")) yield break;
            hud.SetOverlay(OverlayMode.MainMenu);
            var spine = AetheriaPalette.FindBoneAny(outlier.transform, "spine_02");
            if (!Check(spine != null, "Outrider has a skinned upper body")) yield break;
            var poses = new Quaternion[3];
            foreach (int action in new[] { 1, 2, 3 })
            {
                outlier.Anim.PlayAction(action);
                yield return new WaitForSeconds(0.18f);
                if (!Check(outlier.Anim.InActionState(action, out _), $"Outrider entered attack {action}")) yield break;
                poses[action - 1] = spine.localRotation;
            }
            if (!Check(Quaternion.Angle(poses[0], poses[1]) > 1f || Quaternion.Angle(poses[1], poses[2]) > 1f,
                "The attack clips change the skinned pose")) yield break;

            hero.RespawnAtOakhaven();
            hud.SetOverlay(OverlayMode.None);
            yield return new WaitForSeconds(0.5f);
            if (!Check(hero.GetComponent<CharacterController>().isGrounded &&
                Vector3.Distance(hero.transform.position, hero.OakhavenRespawnPoint) < 1f,
                "Respawn places the hero at the authored Oakhaven entry")) yield break;
            Debug.Log("[DemoVerification] PASS");
            Quit(0);
        }

        private static bool Check(bool condition, string description)
        {
            if (condition) Debug.Log($"[DemoVerification] PASS {description}");
            else
            {
                Debug.LogError($"[DemoVerification] FAIL {description}");
                Quit(2);
            }
            return condition;
        }

        private static void Quit(int exitCode)
        {
#if UNITY_EDITOR
            UnityEditor.EditorApplication.Exit(exitCode);
#else
            Application.Quit(exitCode);
#endif
        }
    }
}
