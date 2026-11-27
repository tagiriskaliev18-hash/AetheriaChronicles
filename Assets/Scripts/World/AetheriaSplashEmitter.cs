using UnityEngine;

namespace Aetheria
{
    /// <summary>Churning spray and mist where a waterfall meets the pool; idles when the camera is far away.</summary>
    public class AetheriaSplashEmitter : MonoBehaviour
    {
        public float Interval = 0.22f;
        public float ActiveDistance = 90f;
        private float _timer;

        private void Update()
        {
            var cam = Camera.main;
            if (cam == null || (cam.transform.position - transform.position).sqrMagnitude > ActiveDistance * ActiveDistance) return;
            _timer -= Time.deltaTime;
            if (_timer > 0f) return;
            _timer = Interval;
            Vector3 p = transform.position + new Vector3(Random.Range(-1.5f, 1.5f), 0f, Random.Range(-1.5f, 1.5f));
            AetheriaFx.Puff(p, new Color(0.9f, 0.95f, 1f, 0.42f), 4, 1.6f, 1.8f, 1.8f, 1.1f);
            if (Random.value < 0.4f)
                AetheriaFx.Burst(p, new Color(0.85f, 0.93f, 1f), 8, 1.5f, 4f, 0.05f, 0.12f, 0.9f, 1.4f, 0.8f);
        }
    }
}
