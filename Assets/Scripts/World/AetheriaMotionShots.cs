using System.Collections;
using System.IO;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Animation review capture ("-motion-shots &lt;dir&gt;"): puts the hero on the flat Oakhaven plaza, drives the
    /// Animator the way HeroController does for idle, walk, jog, sprint, melee and cast, and saves side and
    /// three-quarter frames every 0.2 s so the locomotion and attack poses can be judged frame by frame.
    /// </summary>
    [DefaultExecutionOrder(200)]
    public class AetheriaMotionShots : MonoBehaviour
    {
        private string _dir;
        private HeroController _hero;
        private float _speed;          // forced locomotion speed for this phase (m/s), -1 = leave to the hero
        private bool _side = true;

        public static void Begin(string dir)
        {
            var go = new GameObject("AetheriaMotionShots");
            DontDestroyOnLoad(go);
            var s = go.AddComponent<AetheriaMotionShots>();
            s._dir = dir;
            s.StartCoroutine(s.Run());
        }

        private IEnumerator Run()
        {
            Directory.CreateDirectory(_dir);
            for (int i = 0; i < 300 && HeroController.Instance == null; i++) yield return null;
            _hero = HeroController.Instance;
            if (_hero == null) { Application.Quit(2); yield break; }
            yield return new WaitForSeconds(2f);
            AetheriaPrologueDirector.Instance?.Abort();
            var hud = AetheriaHUDManager.Instance;
            if (hud != null) hud.SetOverlay(OverlayMode.None);
            _hero.HasEnteredWorld = true;
            _hero.CinematicLock = true;              // no player input; this script drives the body
            _hero.TeleportSafe(new Vector3(0f, 0f, 40f));
            _hero.transform.rotation = Quaternion.Euler(0f, 90f, 0f);
            yield return new WaitForSeconds(1.5f);

            yield return Phase("a_idle", 0f, 2.0f);
            yield return Phase("b_walk", 1.8f, 2.4f);
            yield return Phase("c_jog", 6.2f, 2.4f);
            yield return Phase("d_sprint", 8.5f, 1.8f);
            _speed = 0f;
            yield return new WaitForSeconds(0.6f);
            _side = false;
            for (int i = 0; i < 4; i++)
            {
                _hero.CinematicLock = false;
                _hero.PerformMeleeAttack();
                _hero.CinematicLock = true;
                yield return Shots("e_attack" + (i + 1), 0.8f);
            }
            _hero.PlayCastAnimation();
            yield return Shots("f_cast", 1.4f);
            _side = true;
            yield return Phase("g_idle_again", 0f, 1.2f);
            Application.Quit(0);
        }

        private IEnumerator Phase(string name, float speed, float seconds)
        {
            _speed = speed;
            yield return new WaitForSeconds(0.5f);   // let the blend settle
            yield return Shots(name, seconds);
        }

        private IEnumerator Shots(string name, float seconds)
        {
            int n = 0;
            for (float t = 0f; t < seconds; t += 0.2f)
            {
                yield return new WaitForEndOfFrame();
                ScreenCapture.CaptureScreenshot(Path.Combine(_dir, $"{name}_{n++:00}.png"));
                yield return new WaitForSeconds(0.2f);
            }
        }

        private void Update()
        {
            if (_hero == null) return;
            var anim = _hero.ActiveAnimator;
            if (_speed > 0f)
            {
                // Move him along the plaza and feed the Animator the same parameters HeroController would.
                _hero.transform.position += _hero.transform.forward * _speed * Time.deltaTime;
                if (Mathf.Abs(_hero.transform.position.x) > 18f) _hero.transform.Rotate(0f, 180f, 0f);
            }
            if (anim != null && anim.isActiveAndEnabled && _speed >= 0f)
            {
                anim.SetFloat("Speed", _speed);
                foreach (var p in anim.parameters)
                    if (p.name == "MoveScale") anim.SetFloat("MoveScale", Mathf.Clamp(_speed / 6.2f, 0.55f, 1.6f));
            }
        }

        private void LateUpdate()
        {
            if (_hero == null || _hero.MainCam == null) return;
            var cam = _hero.MainCam;
            Vector3 hp = _hero.transform.position;
            Vector3 fwd = _hero.transform.forward, right = _hero.transform.right;
            Vector3 want = _side ? hp + right * 4.2f + Vector3.up * 1.2f : hp + fwd * 3.0f + right * 2.2f + Vector3.up * 1.6f;
            cam.transform.position = want;
            cam.transform.rotation = Quaternion.LookRotation(hp + Vector3.up * 1.0f - want, Vector3.up);
            cam.fieldOfView = 45f;
        }
    }
}
