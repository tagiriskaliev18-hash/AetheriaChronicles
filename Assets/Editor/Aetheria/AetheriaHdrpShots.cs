using UnityEditor;
using UnityEngine;

namespace Aetheria.Editor
{
    /// <summary>
    /// Stages the verification shots one at a time in Play Mode and leaves each on screen, so
    /// `unity command capture game_view --camera "Main Camera"` can grab it at full resolution.
    /// </summary>
    public static class AetheriaShots
    {
        [MenuItem("Aetheria/Shots/01 Title")]              public static void S0() => Stage(0);
        [MenuItem("Aetheria/Shots/02 Hero Customizer")]    public static void S1() => Stage(1);
        [MenuItem("Aetheria/Shots/03 Prologue Cave")]      public static void S2() => Stage(2);
        [MenuItem("Aetheria/Shots/04 Oakhaven Arrival")]   public static void S3() => Stage(3);
        [MenuItem("Aetheria/Shots/05 Combat Sunstrike")]   public static void S4() => Stage(4);
        [MenuItem("Aetheria/Shots/06 Dragon Flight")]      public static void S5() => Stage(5);
        [MenuItem("Aetheria/Shots/07 Boss Highkeep")]      public static void S6() => Stage(6);
        [MenuItem("Aetheria/Shots/08 World Map")]          public static void S7() => Stage(7);

        [MenuItem("Aetheria/Shots/Cycle Time of Day")]
        public static void CycleTimeOfDay()
        {
            if (Application.isPlaying) AetheriaWorldGenerator.Instance?.CycleTimeOfDayPreset();
        }

        private static void Stage(int n)
        {
            if (!Application.isPlaying) { Debug.LogError("[Shots] Enter Play Mode first."); return; }
            AetheriaWorldGenerator.Instance?.StageShot(n);
        }
    }
}
