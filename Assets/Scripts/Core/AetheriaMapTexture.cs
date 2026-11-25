using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// One painted chart of the whole continent, computed once and shared by the minimap and the
    /// world map. A single 512x512 pass costs a few seconds; every frame after that only copies
    /// pixels, so both maps stay cheap.
    /// </summary>
    public static class AetheriaMapTexture
    {
        public const int Size = 512;

        private static Texture2D _base;
        private static Color[] _pixels;

        /// <summary>Painted terrain colors of the whole continent (sRGB), built on first use.</summary>
        public static Texture2D Base
        {
            get
            {
                if (_base != null) return _base;
                _base = new Texture2D(Size, Size, TextureFormat.RGBA32, false)
                {
                    filterMode = FilterMode.Bilinear,
                    wrapMode = TextureWrapMode.Clamp,
                    name = "AetheriaChartBase"
                };
                _pixels = new Color[Size * Size];
                float half = AetheriaWorld.HalfExtent, span = AetheriaWorld.WorldSize;
                for (int y = 0; y < Size; y++)
                {
                    float wz = -half + (y + 0.5f) / Size * span;
                    for (int x = 0; x < Size; x++)
                    {
                        float wx = -half + (x + 0.5f) / Size * span;
                        _pixels[y * Size + x] = AetheriaWorld.MapColor(wx, wz);
                    }
                }
                _base.SetPixels(_pixels);
                _base.Apply(false);
                return _base;
            }
        }

        public static Color[] BasePixels
        {
            get { if (Base == null) return null; return _pixels; }
        }

        /// <summary>Painted color at a world point (sRGB), sampled from the shared chart.</summary>
        public static Color Sample(float x, float z)
        {
            if (Base == null) return Color.black;
            Vector2 uv = AetheriaWorld.WorldToUV(x, z);
            int px = Mathf.Clamp((int)(uv.x * Size), 0, Size - 1);
            int py = Mathf.Clamp((int)(uv.y * Size), 0, Size - 1);
            return _pixels[py * Size + px];
        }

        /// <summary>The uncharted look: the same land, dimmed under fog.</summary>
        public static Color Fogged(Color c) => Color.Lerp(c * 0.20f, new Color(0.05f, 0.055f, 0.075f), 0.84f);
    }
}
