using System;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// The opening cinematic of a new game: letterboxed camera flights over the citadel, Mirror Lake and
    /// Oakhaven with narration, ending on the cliff where the hero wakes. Any of Space / Enter / Esc skips it.
    /// Shots are framed from the scene's own landmarks (<see cref="AetheriaWorld.FindSite"/>), so they follow
    /// the world if it is rebuilt.
    /// </summary>
    public class AetheriaIntroCinematic : MonoBehaviour
    {
        public static bool Active { get; private set; }

        private struct Shot
        {
            public Vector3 From, To, LookFrom, LookTo;
            public float Duration;
            public string Line;
        }

        private Shot[] _shots;
        private int _index;
        private float _time;
        private float _fade = 1f;          // 1 = black
        private bool _ending;
        private Action _onDone;
        private Camera _cam;
        private Canvas _hudCanvas;
        private GUIStyle _caption, _hint;
        private Texture2D _black;

        public static void Play(HeroController hero, Action onDone)
        {
            if (Active) return;
            var go = new GameObject("AetheriaIntroCinematic");
            var c = go.AddComponent<AetheriaIntroCinematic>();
            c.Begin(hero, onDone);
        }

        private static Vector3 Ground(Vector3 p, float up)
        {
            p.y = Mathf.Max(AetheriaWorld.SeaLevel, AetheriaWorld.SampleHeight(p)) + up;
            return p;
        }

        private static Vector3 Site(string id, Vector3 fallback)
        {
            var s = AetheriaWorld.FindSite(id);
            return s != null ? s.Position : fallback;
        }

        private void Begin(HeroController hero, Action onDone)
        {
            Active = true;
            _onDone = onDone;
            _cam = hero != null && hero.MainCam != null ? hero.MainCam : Camera.main;
            hero?.SetCreatorStudioCamera(false);
            _hudCanvas = AetheriaHUDManager.Instance != null ? AetheriaHUDManager.Instance.MainCanvas : null;
            if (_hudCanvas != null) _hudCanvas.enabled = false;

            Vector3 citadel = Site("HighkeepCitadel", new Vector3(0f, 0f, -104f));
            Vector3 lake = Site("MirrorLake", new Vector3(14f, 0f, -8f));
            Vector3 village = Site("OakhavenVillage", new Vector3(0f, 0f, 42f));
            Vector3 cliff = Site("PrologueCliff", new Vector3(-27f, 0f, -8f));

            // Where the hero will wake: next to Guardian Weylan, as StartPrologueAwakening places him.
            Vector3 wake = cliff;
            var tutor = AetheriaNPC.AllNPCs.Find(npc => npc.NpcId == "Npc_CaveTutor");
            if (tutor != null) wake = tutor.transform.position + new Vector3(-2.4f, 0f, 0f);
            if (hero != null)
            {
                hero.transform.rotation = Quaternion.Euler(0f, 90f, 0f);
                hero.TeleportSafe(new Vector3(wake.x, AetheriaWorld.SeaLevel, wake.z));
                wake = hero.transform.position;
            }

            Vector3 toCitadel = (citadel - village); toCitadel.y = 0f; toCitadel.Normalize();
            Vector3 side = Vector3.Cross(Vector3.up, toCitadel);
            _shots = new[]
            {
                new Shot
                {
                    From = Ground(citadel - toCitadel * 95f + side * 40f, 42f),
                    To = Ground(citadel - toCitadel * 60f - side * 25f, 30f),
                    LookFrom = Ground(citadel, 12f), LookTo = Ground(citadel, 8f),
                    Duration = 6f,
                    Line = "Тысячу лет Печать Дракона хранила Эфирию.\nВ ночь, когда она раскололась, пала Высокая Твердыня."
                },
                new Shot
                {
                    From = Ground(lake + new Vector3(-40f, 0f, -30f), 18f),
                    To = Ground(lake + new Vector3(30f, 0f, -24f), 10f),
                    LookFrom = Ground(lake, 1f), LookTo = Ground(lake, 2f),
                    Duration = 5.5f,
                    Line = "Малакор, Повелитель Расколотой Печати, поднял мёртвых\nи послал их к Зеркальному озеру."
                },
                new Shot
                {
                    From = Ground(village + new Vector3(30f, 0f, 34f), 24f),
                    To = Ground(village + new Vector3(-22f, 0f, 26f), 12f),
                    LookFrom = Ground(village, 3f), LookTo = Ground(village, 2f),
                    Duration = 5.5f,
                    Line = "Оукхейвен — последний очаг вольных земель.\nЕго старейшины ждут того, кто пробудится."
                },
                new Shot
                {
                    From = Ground(wake + new Vector3(-14f, 0f, -10f), 14f),
                    To = wake + new Vector3(-3.6f, 2.2f, -2.4f),
                    LookFrom = wake + Vector3.up * 1.2f, LookTo = wake + Vector3.up * 1.4f,
                    Duration = 6f,
                    Line = "Пробудись, Странник."
                },
            };
            _index = 0; _time = 0f; _fade = 1f;
        }

        private void Update()
        {
            if (_shots == null) return;
            if (Input.GetKeyDown(KeyCode.Space) || Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.Escape))
            {
                Finish();
                return;
            }
            _time += Time.deltaTime;
            var shot = _shots[_index];
            // Fade in at the start of every shot and out at its end; the last shot ends on black only when done.
            float fadeIn = 1f - Mathf.Clamp01(_time / 0.8f);
            float fadeOut = Mathf.Clamp01((_time - (shot.Duration - 0.7f)) / 0.7f);
            _fade = Mathf.Max(fadeIn, _index == _shots.Length - 1 ? 0f : fadeOut);
            if (_time >= shot.Duration)
            {
                _time = 0f;
                _index++;
                if (_index >= _shots.Length) Finish();
            }
        }

        private void LateUpdate()
        {
            if (_shots == null || _cam == null || _index >= _shots.Length) return;
            var shot = _shots[_index];
            float u = Mathf.SmoothStep(0f, 1f, Mathf.Clamp01(_time / shot.Duration));
            _cam.transform.position = Vector3.Lerp(shot.From, shot.To, u);
            _cam.transform.rotation = Quaternion.LookRotation(Vector3.Lerp(shot.LookFrom, shot.LookTo, u) - _cam.transform.position, Vector3.up);
        }

        private void OnGUI()
        {
            if (_shots == null) return;
            if (_black == null)
            {
                _black = new Texture2D(1, 1);
                _black.SetPixel(0, 0, Color.black);
                _black.Apply();
            }
            if (_caption == null)
            {
                _caption = new GUIStyle(GUI.skin.label)
                {
                    alignment = TextAnchor.MiddleCenter, wordWrap = true, fontStyle = FontStyle.Italic,
                    fontSize = Mathf.RoundToInt(Screen.height * 0.03f), richText = false
                };
                _caption.normal.textColor = new Color(0.95f, 0.9f, 0.8f);
                _hint = new GUIStyle(GUI.skin.label) { alignment = TextAnchor.MiddleRight, fontSize = Mathf.RoundToInt(Screen.height * 0.016f) };
                _hint.normal.textColor = new Color(1f, 1f, 1f, 0.55f);
            }
            float bar = Screen.height * 0.11f;
            GUI.DrawTexture(new Rect(0, 0, Screen.width, bar), _black);
            GUI.DrawTexture(new Rect(0, Screen.height - bar, Screen.width, bar), _black);
            if (_index < _shots.Length)
            {
                var shot = _shots[_index];
                float textAlpha = Mathf.Clamp01((_time - 0.6f) / 0.6f) * Mathf.Clamp01((shot.Duration - _time) / 0.5f);
                var col = _caption.normal.textColor; col.a = textAlpha; _caption.normal.textColor = col;
                GUI.Label(new Rect(Screen.width * 0.1f, Screen.height - bar, Screen.width * 0.8f, bar), shot.Line, _caption);
            }
            GUI.Label(new Rect(0, 0, Screen.width - 24f, bar), "[Пробел] пропустить", _hint);
            if (_fade > 0.001f)
            {
                var prev = GUI.color;
                GUI.color = new Color(0f, 0f, 0f, _fade);
                GUI.DrawTexture(new Rect(0, 0, Screen.width, Screen.height), _black);
                GUI.color = prev;
            }
        }

        private void Finish()
        {
            if (_ending) return;
            _ending = true;
            _shots = null;
            if (_hudCanvas != null) _hudCanvas.enabled = true;
            Active = false;
            var done = _onDone;
            _onDone = null;
            done?.Invoke();
            Destroy(gameObject);
        }

        private void OnDestroy()
        {
            if (_black != null) Destroy(_black);
            if (!_ending)
            {
                Active = false;
                if (_hudCanvas != null) _hudCanvas.enabled = true;
            }
        }
    }
}
