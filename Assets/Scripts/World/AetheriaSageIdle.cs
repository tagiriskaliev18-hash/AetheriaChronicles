using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Keeps the sculpted (unrigged) sage alive: slow breathing, a faint weight shift on his staff, and a small
    /// nod at the start of every dialogue line so his speech has a physical beat.
    /// </summary>
    public class AetheriaSageIdle : MonoBehaviour
    {
        private static readonly System.Reflection.FieldInfo LineField =
            typeof(AetheriaPrologueDirector).GetField("_line", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);

        private Vector3 _baseScale;
        private Quaternion _baseRot;
        private float _seed;
        private int _lastLine = -1;
        private float _nod;

        private void Start()
        {
            _baseScale = transform.localScale;
            _baseRot = transform.localRotation;
            _seed = Random.value * 10f;
        }

        private void LateUpdate()
        {
            float t = Time.time + _seed;
            var d = AetheriaPrologueDirector.Instance;
            if (d != null && d.Current == AetheriaPrologueDirector.Stage.Talk && LineField != null)
            {
                int line = (int)LineField.GetValue(d);
                if (line != _lastLine) { _lastLine = line; _nod = 1f; }
            }
            _nod = Mathf.MoveTowards(_nod, 0f, Time.deltaTime * 1.6f);
            float nodAngle = Mathf.Sin((1f - _nod) * Mathf.PI) * 4f * (_nod > 0f ? 1f : 0f);

            float breath = Mathf.Sin(t * 1.35f) * 0.006f;
            transform.localScale = new Vector3(_baseScale.x * (1f + breath * 0.6f), _baseScale.y * (1f + breath), _baseScale.z * (1f + breath * 0.6f));
            float sway = Mathf.Sin(t * 0.42f) * 0.9f;
            transform.localRotation = _baseRot * Quaternion.Euler(nodAngle, Mathf.Sin(t * 0.27f) * 1.5f, sway);
        }
    }
}
