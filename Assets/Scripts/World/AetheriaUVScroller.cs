using UnityEngine;

namespace Aetheria
{
    /// <summary>Scrolls a material's base map, for falling water and drifting mist sheets.</summary>
    public class AetheriaUVScroller : MonoBehaviour
    {
        public Vector2 Speed = new Vector2(0f, -1.2f);
        private Material _mat;
        private Vector2 _offset;

        private void Start()
        {
            var r = GetComponent<Renderer>();
            if (r != null) _mat = r.material;
        }

        private void Update()
        {
            if (_mat == null) return;
            _offset += Speed * Time.deltaTime;
            _offset.x = Mathf.Repeat(_offset.x, 1f);
            _offset.y = Mathf.Repeat(_offset.y, 1f);
            if (_mat.HasProperty("_UnlitColorMap")) _mat.SetTextureOffset("_UnlitColorMap", _offset);
            if (_mat.HasProperty("_BaseColorMap")) _mat.SetTextureOffset("_BaseColorMap", _offset);
        }
    }
}
