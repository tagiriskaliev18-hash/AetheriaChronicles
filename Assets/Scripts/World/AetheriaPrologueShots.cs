using System.Collections;
using System.IO;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// "-prologue-shots &lt;dir&gt;": plays the directed prologue start to finish, the way a player would,
    /// and captures a frame at every beat (cave, sage, talk, vista, fall, glade), then quits.
    /// </summary>
    public static class AetheriaPrologueShots
    {
        public static IEnumerator Run(string dir)
        {
            Directory.CreateDirectory(dir);
            yield return new WaitForSeconds(1f);
            var hero = HeroController.Instance;
            var hud = AetheriaHUDManager.Instance;
            var director = AetheriaPrologueDirector.Instance;
            if (hero == null || hud == null || director == null) { Application.Quit(2); yield break; }

            hud.SetOverlay(OverlayMode.None);
            hero.StartPrologueAwakening();
            yield return new WaitForSeconds(3.0f);
            Shot(dir, "01_cave_wake");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame
            yield return new WaitForSeconds(1.0f);

            // Walk toward the light: turn the hero and look down the tunnel the way the chase camera does.
            hero.transform.rotation = Quaternion.LookRotation(Flat(director.LightTrigger.position - hero.transform.position).normalized);
            hero.TeleportSafe(Vector3.Lerp(hero.transform.position, director.LightTrigger.position, 0.55f));
            yield return new WaitForSeconds(1.2f);
            Shot(dir, "02_cave_toward_light");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame

            hero.TeleportSafe(director.LightTrigger.position + (hero.transform.position - director.LightTrigger.position).normalized * 3f);
            yield return new WaitForSeconds(0.3f);
            director.DebugReachLight();
            yield return new WaitForSeconds(1.1f);
            Shot(dir, "03_sage_appears");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame
            yield return new WaitForSeconds(3.0f);
            Shot(dir, "04_sage_talks");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame

            director.DebugFinishDialogue();
            hero.transform.rotation = Quaternion.LookRotation(Flat(director.LedgeTrigger.position - director.CaveMouth.position).normalized);
            hero.TeleportSafe(director.CaveMouth.position + Flat(director.LedgeTrigger.position - director.CaveMouth.position) * 0.45f);
            yield return new WaitForSeconds(1.5f);
            Shot(dir, "05_vista");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame

            hero.TeleportSafe(director.LedgeTrigger.position);
            yield return new WaitForSeconds(0.7f);
            Shot(dir, "06_ledge_collapse");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame
            yield return new WaitForSeconds(2.6f);
            Shot(dir, "07_waterfall_fall");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame
            yield return new WaitForSeconds(2.3f);
            Shot(dir, "08_splash");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame
            yield return new WaitForSeconds(7.6f);
            Shot(dir, "09_glade_wake");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame
            yield return new WaitForSeconds(3.0f);
            Shot(dir, "10_glade_after");
            yield return new WaitForSeconds(0.25f);   // the capture lands at the end of the frame
            yield return new WaitForSeconds(0.5f);
            Debug.Log($"PROLOGUE_SHOTS_DONE dir={dir}");
            Application.Quit(0);
        }


        /// <summary>"-world-shots &lt;dir&gt;": fixed views of the village, the glade and the cave hill for review.</summary>
        public static IEnumerator RunWorld(string dir)
        {
            Directory.CreateDirectory(dir);
            yield return new WaitForSeconds(1f);
            var hero = HeroController.Instance;
            var hud = AetheriaHUDManager.Instance;
            if (hero == null || hud == null) { Application.Quit(2); yield break; }
            hud.SetOverlay(OverlayMode.None);
            hero.SkipToOpenWorld(false);
            yield return new WaitForSeconds(0.5f);
            hero.CinematicLock = true;
            if (hud.MainCanvas != null) hud.MainCanvas.enabled = false;
            var cam = hero.MainCam != null ? hero.MainCam : Camera.main;
            Vector3 plaza = new Vector3(0f, 0f, 42f);
            var views = new (string name, Vector3 from, Vector3 at)[]
            {
                ("w01_village_aerial", plaza + new Vector3(-60f, 45f, -55f), plaza),
                ("w02_plaza_north", plaza + new Vector3(0f, 2.2f, -18f), plaza + new Vector3(0f, 3f, 30f)),
                ("w03_plaza_west", plaza + new Vector3(16f, 2.2f, 0f), plaza + new Vector3(-30f, 3f, 0f)),
                ("w04_plaza_east", plaza + new Vector3(-16f, 2.2f, 4f), plaza + new Vector3(30f, 3f, 0f)),
                ("w05_road_south", new Vector3(2f, 2.4f, 8f), new Vector3(2f, 2f, -40f)),
                ("w06_market", plaza + new Vector3(4f, 2.2f, -6f), plaza + new Vector3(-14f, 1f, -14f)),
            };
            foreach (var v in views)
            {
                Vector3 from = v.from; from.y += AetheriaWorld.SampleHeight(from);
                Vector3 at = v.at; at.y += AetheriaWorld.SampleHeight(at);
                for (int i = 0; i < 20; i++)
                {
                    cam.transform.position = from;
                    cam.transform.rotation = Quaternion.LookRotation(at - from);
                    yield return null;
                }
                yield return new WaitForSeconds(0.6f);
                cam.transform.position = from;
                cam.transform.rotation = Quaternion.LookRotation(at - from);
                Shot(dir, v.name);
                yield return new WaitForSeconds(0.3f);
            }
            var d = AetheriaPrologueDirector.Instance;
            if (d != null)
            {
                Vector3 mouth = d.CaveMouth.position, ledge = d.LedgeTrigger.position, glade = d.GladeWakePoint.position;
                var extra = new (string name, Vector3 from, Vector3 at)[]
                {
                    ("w07_cave_hill", mouth + (ledge - mouth) * 1.6f + Vector3.up * 8f, mouth + Vector3.up * 2f),
                    ("w08_waterfall", ledge + (ledge - mouth).normalized * 25f + Vector3.up * -6f + Vector3.Cross(Vector3.up, (ledge - mouth).normalized) * 14f, ledge + Vector3.down * 10f),
                    ("w09_glade", glade + new Vector3(6f, 3f, 6f), glade),
                };
                foreach (var v in extra)
                {
                    for (int i = 0; i < 20; i++) { cam.transform.position = v.from; cam.transform.rotation = Quaternion.LookRotation(v.at - v.from); yield return null; }
                    yield return new WaitForSeconds(0.6f);
                    Shot(dir, v.name);
                    yield return new WaitForSeconds(0.3f);
                }
            }
            Debug.Log($"WORLD_SHOTS_DONE dir={dir}");
            Application.Quit(0);
        }

        private static Vector3 Flat(Vector3 v) { v.y = 0f; return v; }

        private static void Shot(string dir, string name)
        {
            var d = AetheriaPrologueDirector.Instance;
            Debug.Log($"[PrologueShots] {name}: stage {(d != null ? d.Current.ToString() : "-")} hero {HeroController.Instance?.transform.position}");
            ScreenCapture.CaptureScreenshot(Path.Combine(dir, name + ".png"));
        }
    }
}
