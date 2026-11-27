using System;
using UnityEngine;

namespace Aetheria
{
    /// <summary>The lands of Aetheria. Oakhaven is the hub; every other biome is a named region with its own landmark.</summary>
    public enum BiomeId
    {
        OakhavenValley  = 0,
        MirrorLake      = 1,
        DragonAerie     = 2,
        Greenhollow     = 3,
        SapphireWoods   = 4,
        HighkeepCitadel = 5,
        AshCaldera      = 6,
        CrystalGlacier  = 7,
        GlimmercapGrove = 8,
        NocturneHollow  = 9,
        SunblightWastes = 10,
        MurkfenMarsh    = 11,
        AetherianSea    = 12
    }

    public enum SiteKind
    {
        Village, Lake, Cliff, Aerie, Farm, Forest, Citadel, Caldera, Glacier, Grove, Hamlet, Wastes, Marsh, Shrine, Isle
    }

    [Serializable]
    public sealed class SiteDef
    {
        public string Id;
        public string Label;
        public BiomeId Biome;
        public SiteKind Kind;
        public Vector3 Position;
        public bool StartDiscovered;

        public SiteDef(string id, string label, BiomeId biome, SiteKind kind, float x, float z, bool discovered = false)
        {
            Id = id; Label = label; Biome = biome; Kind = kind; Position = new Vector3(x, 0f, z); StartDiscovered = discovered;
        }
    }

    public sealed class BiomeRegion
    {
        public BiomeId Biome;
        public Vector2 Center;
        public float InnerRadius;
        public float OuterRadius;
        public BiomeRegion(BiomeId biome, float x, float z, float inner, float outer)
        {
            Biome = biome; Center = new Vector2(x, z); InnerRadius = inner; OuterRadius = outer;
        }
    }

    /// <summary>
    /// Single source of truth for the world: continent shape, biome regions, landmark sites, roads,
    /// terrain height and the stylized faceted-terrain palette. Pure math (no scene objects), so the
    /// terrain builder, the map, the quest system and gameplay all agree on the same world.
    /// </summary>
    public static class AetheriaWorld
    {
        public const float HalfExtent = 640f;      // world spans [-HalfExtent, +HalfExtent] on X and Z
        public const float WorldSize = HalfExtent * 2f;
        public const float SeaLevel = 0f;
        public const float CellSize = 4f;          // one terrain facet cell
        public const int Cells = (int)(WorldSize / CellSize);
        public const int BiomeCount = 13;

        /// <summary>+Z is north on the map.</summary>
        private static readonly SiteDef[] ContinentSites =
        {
            new SiteDef("OakhavenVillage",    "Oakhaven Village",           BiomeId.OakhavenValley,  SiteKind.Village,     0f,   20f, true),
            new SiteDef("Shrine_Oakhaven",    "Oakhaven Hearth Shrine",     BiomeId.OakhavenValley,  SiteKind.Shrine,    -24f,   36f, true),
            new SiteDef("MirrorLake",         "Mirror Lake & Stone Bridge", BiomeId.MirrorLake,      SiteKind.Lake,     -130f,  -60f),
            new SiteDef("PrologueCliff",      "Awakening Cave Cliff",       BiomeId.MirrorLake,      SiteKind.Cliff,    -235f,  -60f),
            new SiteDef("Shrine_Lake",        "Lakefall Shrine",            BiomeId.MirrorLake,      SiteKind.Shrine,    -70f,  -14f),
            new SiteDef("DragonAerie",        "Dragon Aerie",               BiomeId.DragonAerie,     SiteKind.Aerie,    -300f,   60f),
            new SiteDef("Shrine_Aerie",       "Aerie Wind Shrine",          BiomeId.DragonAerie,     SiteKind.Shrine,   -262f,   86f),
            new SiteDef("GreenhollowFarm",    "Greenhollow Farmlands",      BiomeId.Greenhollow,     SiteKind.Farm,       60f,  230f),
            new SiteDef("Shrine_Greenhollow", "Greenhollow Harvest Shrine", BiomeId.Greenhollow,     SiteKind.Shrine,    112f,  196f),
            new SiteDef("SapphireWoods",      "Sapphire Whispering Woods",  BiomeId.SapphireWoods,   SiteKind.Forest,    270f,   60f),
            new SiteDef("Shrine_Woods",       "Whispering Grove Shrine",    BiomeId.SapphireWoods,   SiteKind.Shrine,    322f,   96f),
            new SiteDef("HighkeepCitadel",    "Highkeep Citadel",           BiomeId.HighkeepCitadel, SiteKind.Citadel,     0f,  440f),
            new SiteDef("Shrine_Highkeep",    "Highkeep Gate Shrine",       BiomeId.HighkeepCitadel, SiteKind.Shrine,     26f,  352f),
            new SiteDef("EmberCrypt",         "Ash Caldera & Ember Crypt",  BiomeId.AshCaldera,      SiteKind.Caldera,   360f,  300f),
            new SiteDef("Shrine_Caldera",     "Ember Crypt Shrine",         BiomeId.AshCaldera,      SiteKind.Shrine,    292f,  226f),
            new SiteDef("CrystalGlacier",     "Crystal Glacier Peaks",      BiomeId.CrystalGlacier,  SiteKind.Glacier,  -360f,  300f),
            new SiteDef("Shrine_Glacier",     "Glacier Silence Shrine",     BiomeId.CrystalGlacier,  SiteKind.Shrine,   -300f,  244f),
            new SiteDef("GlimmercapGrove",    "Glimmercap Grove",           BiomeId.GlimmercapGrove, SiteKind.Grove,    -230f, -250f),
            new SiteDef("Shrine_Grove",       "Glimmercap Ring Shrine",     BiomeId.GlimmercapGrove, SiteKind.Shrine,   -186f, -284f),
            new SiteDef("NocturneHollow",     "Nocturne Hollow",            BiomeId.NocturneHollow,  SiteKind.Hamlet,    240f, -290f),
            new SiteDef("Shrine_Hollow",      "Nocturne Lantern Shrine",    BiomeId.NocturneHollow,  SiteKind.Shrine,    276f, -246f),
            new SiteDef("SunblightWastes",    "Sunblight Wastes",           BiomeId.SunblightWastes, SiteKind.Wastes,     40f, -440f),
            new SiteDef("Shrine_Wastes",      "Sunblight Obelisk Shrine",   BiomeId.SunblightWastes, SiteKind.Shrine,    84f, -404f),
            new SiteDef("MurkfenMarsh",       "Murkfen Marsh",              BiomeId.MurkfenMarsh,    SiteKind.Marsh,    -380f, -300f),
            new SiteDef("Shrine_Marsh",       "Murkfen Bog Shrine",         BiomeId.MurkfenMarsh,    SiteKind.Shrine,   -344f, -278f),
            new SiteDef("EchoIsle",           "Isle of Echoes",             BiomeId.AetherianSea,    SiteKind.Isle,     -598f, -142f),
            new SiteDef("GullIsle",           "Gullskerry Isle",            BiomeId.AetherianSea,    SiteKind.Isle,      556f, -296f),
            new SiteDef("StarIsle",           "Starspire Isle",             BiomeId.AetherianSea,    SiteKind.Isle,      148f,  612f),
        };

        public static SiteDef[] Sites { get; private set; } = ContinentSites;
        public static bool UsesCourtyardLayout => Sites != ContinentSites;

        /// <summary>The twelve biome regions partition the whole continent; boundaries are blended.</summary>
        public static readonly BiomeRegion[] Regions =
        {
            new BiomeRegion(BiomeId.OakhavenValley,     0f,   10f,  80f, 170f),
            new BiomeRegion(BiomeId.MirrorLake,      -150f,  -70f,  70f, 140f),
            new BiomeRegion(BiomeId.DragonAerie,     -330f,   70f,  70f, 140f),
            new BiomeRegion(BiomeId.Greenhollow,       50f,  235f,  80f, 150f),
            new BiomeRegion(BiomeId.SapphireWoods,   320f,   70f, 100f, 170f),
            new BiomeRegion(BiomeId.HighkeepCitadel,   0f,  440f,  85f, 140f),
            new BiomeRegion(BiomeId.AshCaldera,      380f,  330f,  80f, 140f),
            new BiomeRegion(BiomeId.CrystalGlacier, -390f,  310f,  95f, 160f),
            new BiomeRegion(BiomeId.GlimmercapGrove,-250f, -260f,  75f, 135f),
            new BiomeRegion(BiomeId.NocturneHollow,  260f, -310f,  75f, 135f),
            new BiomeRegion(BiomeId.SunblightWastes,  30f, -440f,  85f, 150f),
            new BiomeRegion(BiomeId.MurkfenMarsh,   -420f, -300f,  75f, 135f),
        };

        private static readonly Vector2[][] Roads =
        {
            new[] { new Vector2(0f, 20f),   new Vector2(38f, 120f), new Vector2(60f, 230f), new Vector2(22f, 300f), new Vector2(0f, 352f), new Vector2(0f, 410f) },
            new[] { new Vector2(-8f, 14f),  new Vector2(-62f, -12f), new Vector2(-70f, -60f), new Vector2(-190f, -60f), new Vector2(-235f, -60f), new Vector2(-268f, 10f), new Vector2(-300f, 60f) },
            new[] { new Vector2(10f, 24f),  new Vector2(140f, 44f), new Vector2(270f, 60f), new Vector2(330f, 180f), new Vector2(360f, 232f) },
            new[] { new Vector2(6f, 10f),   new Vector2(22f, -90f), new Vector2(62f, -220f), new Vector2(70f, -340f), new Vector2(44f, -400f) },
            new[] { new Vector2(22f, -90f), new Vector2(-110f, -176f), new Vector2(-230f, -250f), new Vector2(-310f, -282f), new Vector2(-380f, -300f) },
            new[] { new Vector2(150f, 48f), new Vector2(228f, -140f), new Vector2(240f, -260f) },
        };

        // Mountain ridges: a polyline with a radius and a peak height. They frame the horizon and the biomes.
        private static readonly Vector2[][] Ridges =
        {
            new[] { new Vector2(-560f, 430f), new Vector2(-470f, 520f), new Vector2(-330f, 575f), new Vector2(-200f, 560f) },
            new[] { new Vector2(110f, 545f),  new Vector2(240f, 560f),  new Vector2(380f, 520f) },
            new[] { new Vector2(540f, 120f),  new Vector2(560f, 10f),   new Vector2(530f, -110f) },
            new[] { new Vector2(-505f, -95f), new Vector2(-470f, -215f) },
            new[] { new Vector2(-520f, -500f), new Vector2(-440f, -565f) },
            new[] { new Vector2(470f, -480f), new Vector2(545f, -400f) },
            new[] { new Vector2(-20f, -330f), new Vector2(12f, -362f) },
            new[] { new Vector2(135f, -348f), new Vector2(172f, -318f) },
        };
        private static readonly float[] RidgeRadius = { 95f, 85f, 80f, 90f, 85f, 80f, 58f, 58f };
        private static readonly float[] RidgeHeight = { 95f, 80f, 85f, 70f, 60f, 62f, 46f, 42f };

        // Offshore isles: (x, z, radius, height)
        private static readonly Vector4[] Isles =
        {
            new Vector4(-598f, -142f, 30f, 17f),
            new Vector4( 556f, -296f, 24f, 12f),
            new Vector4( 148f,  612f, 34f, 20f),
        };

        // Flat-topped mesas of the Sunblight Wastes: (x, z, radius, height)
        private static readonly Vector4[] Mesas =
        {
            new Vector4( -28f, -478f, 22f, 15f),
            new Vector4( 96f, -506f, 26f, 19f),
            new Vector4( 118f, -404f, 18f, 12f),
            new Vector4(-76f, -408f, 16f, 10f),
        };

        // ---- Palette (sRGB, authored values; converted to linear when used) ----
        private struct Palette { public Color Ground, Ground2, Rock, High; }

        private static readonly Palette[] Palettes =
        {
            Pal(0x86, 0xB4, 0x4C,  0xA8, 0xC8, 0x5E,  0x8C, 0x85, 0x6E,  0x9A, 0x93, 0x86), // Oakhaven: warm grass
            Pal(0x5F, 0xA8, 0x68,  0x84, 0xC2, 0x72,  0x7E, 0x86, 0x6C,  0x9A, 0x9F, 0x92), // Mirror Lake: lush
            Pal(0xB4, 0x8E, 0x5C,  0xCB, 0xA6, 0x6E,  0x7D, 0x6A, 0x5A,  0xA6, 0x94, 0x82), // Dragon Aerie: ochre rock
            Pal(0x8E, 0xC4, 0x54,  0xB4, 0xD4, 0x62,  0x8F, 0x8C, 0x6A,  0xA8, 0xAA, 0x8E), // Greenhollow: bright meadow
            Pal(0x2F, 0x7F, 0x6B,  0x40, 0x9C, 0x7E,  0x56, 0x68, 0x78,  0x74, 0x88, 0x96), // Sapphire Woods: teal
            Pal(0x7C, 0x8F, 0x66,  0x96, 0xA6, 0x78,  0x7A, 0x7D, 0x82,  0xA0, 0xA2, 0xA6), // Highkeep: grey moor
            Pal(0x3B, 0x33, 0x37,  0x5A, 0x4F, 0x52,  0x2A, 0x25, 0x2A,  0x6E, 0x62, 0x66), // Ash Caldera: basalt
            Pal(0xEE, 0xF4, 0xFB,  0xBE, 0xE0, 0xF5,  0x8F, 0xA3, 0xB8,  0xFF, 0xFF, 0xFF), // Glacier: snow & ice
            Pal(0x6E, 0x9A, 0x5C,  0xA4, 0x8B, 0xC9,  0x5E, 0x72, 0x5A,  0x8E, 0xA2, 0x84), // Glimmercap: moss & violet
            Pal(0x5E, 0x5A, 0x58,  0x74, 0x69, 0x6A,  0x4A, 0x44, 0x48,  0x66, 0x60, 0x64), // Nocturne: dead grey
            Pal(0xD8, 0xA4, 0x5C,  0xE8, 0xBF, 0x78,  0xB5, 0x71, 0x4A,  0xC9, 0x8A, 0x58), // Sunblight: sand & rust
            Pal(0x4F, 0x6B, 0x3A,  0x6B, 0x7F, 0x45,  0x4A, 0x4E, 0x3C,  0x5E, 0x6E, 0x4C), // Murkfen: murky green
            Pal(0x6F, 0x9F, 0xB0,  0x86, 0xB2, 0xBE,  0x5C, 0x7C, 0x8C,  0x7A, 0xA2, 0xAE), // Sea
        };

        private static readonly Color SandColor   = Srgb(0xE6, 0xD6, 0xA4);
        private static readonly Color DirtColor   = Srgb(0xA9, 0x8A, 0x60);
        private static readonly Color SnowColor   = Srgb(0xF7, 0xFA, 0xFF);
        private static readonly Color SeabedColor = Srgb(0x9E, 0xB6, 0x8E);

        private static Palette Pal(int gr, int gg, int gb, int g2r, int g2g, int g2b, int rr, int rg, int rb, int hr, int hg, int hb) => new Palette
        {
            Ground = Srgb(gr, gg, gb), Ground2 = Srgb(g2r, g2g, g2b), Rock = Srgb(rr, rg, rb), High = Srgb(hr, hg, hb)
        };

        private static Color Srgb(int r, int g, int b) => ((Color)new Color32((byte)r, (byte)g, (byte)b, 255)).linear;

        // ------------------------------------------------------------------ queries

        public static void ReadSceneLandmarks()
        {
            Sites = ContinentSites;
            if (GameObject.Find("PrologueWaterfallMist") == null) return;
            var found = new System.Collections.Generic.List<SiteDef>();
            void Add(string id, string objectName)
            {
                var obj = GameObject.Find(objectName);
                var site = FindSite(id);
                if (obj == null || site == null) return;
                var p = obj.transform.position;
                found.Add(new SiteDef(id, site.Label, site.Biome, site.Kind, p.x, p.z, site.StartDiscovered));
            }
            Add("PrologueCliff", "PrologueWaterfallMist");
            Add("OakhavenVillage", "ElderAldric");
            Add("Shrine_Oakhaven", "Shrine_OakhavenForge");
            Add("MirrorLake", "MirrorLakeStoneBridge");
            Add("DragonAerie", "IgnisCrimsonDragon");
            Add("SapphireWoods", "TempestConductor_1");
            Add("EmberCrypt", "EmberCryptObsidianSeal");
            Add("CrystalGlacier", "FrostWarden");
            Add("HighkeepCitadel", "Boss_Malakor");
            Sites = found.ToArray();
        }

        public static SiteDef FindSite(string id)
        {
            foreach (var s in Sites) if (s.Id == id) return s;
            if (UsesCourtyardLayout)
                foreach (var s in ContinentSites) if (s.Id == id) return s;
            return null;
        }

        public static string BiomeDisplayName(BiomeId b) => b switch
        {
            BiomeId.OakhavenValley  => "Golden Oakhaven Valley",
            BiomeId.MirrorLake      => "Mirror Lake",
            BiomeId.DragonAerie     => "Dragon Aerie Cliffs",
            BiomeId.Greenhollow     => "Greenhollow Farmlands",
            BiomeId.SapphireWoods   => "Sapphire Whispering Woods",
            BiomeId.HighkeepCitadel => "Highkeep Citadel",
            BiomeId.AshCaldera      => "Ash Caldera",
            BiomeId.CrystalGlacier  => "Crystal Glacier Peaks",
            BiomeId.GlimmercapGrove => "Glimmercap Grove",
            BiomeId.NocturneHollow  => "Nocturne Hollow",
            BiomeId.SunblightWastes => "Sunblight Wastes",
            BiomeId.MurkfenMarsh    => "Murkfen Marsh",
            _                       => "Aetherian Sea"
        };

        /// <summary>Cartographic color used on the 2D overview and for map legends (sRGB).</summary>
        public static Color BiomeMapColor(BiomeId b) => Palettes[(int)b].Ground.gamma;

        public static Vector2 WorldToUV(float x, float z) => new Vector2(
            Mathf.Clamp01((x + HalfExtent) / WorldSize),
            Mathf.Clamp01((z + HalfExtent) / WorldSize));

        public static Vector2 UVToWorld(float u, float v) => new Vector2(u * WorldSize - HalfExtent, v * WorldSize - HalfExtent);

        /// <summary>Fills biome blend weights (length must be BiomeCount; the sea stays 0). Sum = 1.</summary>
        public static void BiomeWeights(float x, float z, float[] w)
        {
            for (int i = 0; i < w.Length; i++) w[i] = 0f;
            var p = new Vector2(x, z);
            float sum = 0f;
            for (int i = 0; i < Regions.Length; i++)
            {
                var r = Regions[i];
                float d = Vector2.Distance(r.Center, p);
                float t = Mathf.Max(0f, d - r.InnerRadius) + r.OuterRadius * 0.22f;
                float k = 1f / (t * t);
                w[(int)r.Biome] = k;
                sum += k;
            }
            float inv = 1f / Mathf.Max(1e-8f, sum);
            for (int i = 0; i < w.Length; i++) w[i] *= inv;
        }

        /// <summary>Discrete biome at a world position (sea when the point is under water).</summary>
        public static BiomeId BiomeAt(float x, float z)
        {
            if (UsesCourtyardLayout)
            {
                float nearest = float.PositiveInfinity;
                BiomeId biome = BiomeId.OakhavenValley;
                foreach (var site in Sites)
                {
                    float dx = site.Position.x - x, dz = site.Position.z - z;
                    float distance = dx * dx + dz * dz;
                    if (distance >= nearest) continue;
                    nearest = distance;
                    biome = site.Biome;
                }
                return biome;
            }
            var p = new Vector2(x, z);
            foreach (var isle in Isles)
                if (Vector2.Distance(p, new Vector2(isle.x, isle.y)) < isle.z)
                    return BiomeId.OakhavenValley;

            if (x >= -210f && x <= -60f && z >= -130f && z <= 0f)
            {
                if (HeightAt(x, z) <= SeaLevel + 0.15f || EllipseMask(x, z, -130f, -60f, 1.2f, 1f, 28f, 56f) > 0f)
                    return BiomeId.MirrorLake;
            }

            if (HeightAt(x, z) <= SeaLevel + 0.15f)
                return BiomeId.AetherianSea;

            var best = BiomeId.OakhavenValley;
            float bestW = float.MaxValue;
            for (int i = 0; i < Regions.Length; i++)
            {
                var r = Regions[i];
                float d = Vector2.Distance(r.Center, p);
                float t = Mathf.Max(0f, d - r.InnerRadius) + r.OuterRadius * 0.22f;
                if (t < bestW) { bestW = t; best = r.Biome; }
            }
            return best;
        }

        public static BiomeId BiomeAt(Vector3 p) => BiomeAt(p.x, p.z);

        // ------------------------------------------------------------------ height field

        public static float HeightAt(float x, float z)
        {
            float d = Mathf.Sqrt(x * x + z * z);
            float ang = Mathf.Atan2(z, x);

            // Irregular coastline: lobes and bays, periodic in angle so there is no seam.
            float ca = Mathf.Cos(ang), sa = Mathf.Sin(ang);
            float coastR = 522f + 40f * Mathf.Sin(ang * 3f + 0.8f) + 27f * Mathf.Sin(ang * 5f + 2.3f)
                                 + 15f * Mathf.Sin(ang * 7f + 4.1f)
                                 + 18f * (Mathf.PerlinNoise(ca * 1.7f + 11.3f, sa * 1.7f + 4.7f) - 0.5f) * 2f;
            float land = SmoothStep(coastR + 40f, coastR - 36f, d);

            float landH = LandHeight(x, z);
            float seaFloor = -15f + 3.5f * Fbm(x * 0.006f + 91f, z * 0.006f + 37f);
            float h = Mathf.Lerp(seaFloor, landH, land);

            // Offshore isles
            foreach (var isle in Isles)
            {
                float di = Vector2.Distance(new Vector2(x, z), new Vector2(isle.x, isle.y));
                float k = 1f - SmoothStep(isle.z * 0.42f, isle.z, di);
                if (k <= 0f) continue;
                float top = isle.w * (0.62f + 0.38f * Fbm(x * 0.03f + 5f, z * 0.03f + 9f));
                h = Mathf.Max(h, Mathf.Lerp(seaFloor, top, k));
            }
            return h;
        }

        public static float SampleHeight(Vector3 p)
        {
            var terrain = Terrain.activeTerrain;
            if (terrain != null && terrain.terrainData != null)
            {
                var origin = terrain.transform.position;
                var size = terrain.terrainData.size;
                if (p.x >= origin.x && p.x <= origin.x + size.x &&
                    p.z >= origin.z && p.z <= origin.z + size.z)
                    return origin.y + terrain.SampleHeight(p);
            }
            return HeightAt(p.x, p.z);
        }

        public static bool IsOpenSea(float x, float z) => HeightAt(x, z) <= SeaLevel - 2.2f;

        private static float LandHeight(float x, float z)
        {
            float h = 5.5f + 8.5f * Fbm(x * 0.0035f + 31.7f, z * 0.0035f + 17.3f)
                        + 2.4f * Fbm(x * 0.017f + 3.1f, z * 0.017f + 8.9f);

            // Mountain ridges (ridged silhouette)
            var p = new Vector2(x, z);
            for (int r = 0; r < Ridges.Length; r++)
            {
                float dist = PolylineDistance(p, Ridges[r]);
                float k = 1f - SmoothStep(RidgeRadius[r] * 0.40f, RidgeRadius[r], dist);
                if (k <= 0f) continue;
                float ridge = Ridged(x * 0.011f + r * 3.7f, z * 0.011f + r * 1.9f);
                h += k * RidgeHeight[r] * (0.52f + 0.48f * ridge);
            }

            // Oakhaven plaza: a calm flat hub
            h = Toward(h, 8.2f, Mask(x, z, 0f, 20f, 20f, 46f));

            // Mirror Lake basin (below sea level -> water)
            h = Toward(h, -5.5f, EllipseMask(x, z, -130f, -60f, 1.2f, 1f, 28f, 56f));

            // Awakening cave cliff
            h = Toward(h, 27f, Mask(x, z, -235f, -60f, 24f, 46f));

            // Dragon Aerie crag
            h = Toward(h, 31f, Mask(x, z, -300f, 60f, 13f, 38f));

            // Greenhollow: gentle fields
            h = Toward(h, 9.5f + 1.2f * Mathf.Sin(x * 0.05f) * Mathf.Cos(z * 0.05f), Mask(x, z, 60f, 230f, 58f, 104f));

            // Highkeep plateau + grand approach ramp from the south
            h = Toward(h, 48f, Mask(x, z, 0f, 432f, 66f, 122f));
            if (z > 236f && z < 376f && Mathf.Abs(x) < 46f)
            {
                float t = Mathf.Clamp01((z - 236f) / 132f);
                float target = Mathf.Lerp(Mathf.Min(h, 16f), 48f, t * t * (3f - 2f * t));
                float side = 1f - SmoothStep(15f, 34f, Mathf.Abs(x));
                h = Mathf.Lerp(h, target, side);
            }

            // Ash Caldera: sunken floor with a raised rim
            float dCal = Vector2.Distance(p, new Vector2(360f, 300f));
            h = Toward(h, 4.5f, Mask(x, z, 360f, 300f, 26f, 44f));
            h += 24f * Mathf.Exp(-Mathf.Pow((dCal - 58f) / 15f, 2f));

            // Crystal Glacier: high ice shelf with seracs
            h = Toward(h, 58f + 20f * Ridged(x * 0.016f + 5f, z * 0.016f + 9f), Mask(x, z, -360f, 300f, 66f, 132f));

            // Glimmercap Grove & Nocturne Hollow: soft shelves
            h = Toward(h, 9.5f, Mask(x, z, -230f, -250f, 44f, 96f));
            h = Toward(h, 7.5f, Mask(x, z, 240f, -290f, 42f, 98f));

            // Murkfen Marsh: waterlogged lowland with pools dipping below sea level
            float hummock = 2.6f * (Fbm(x * 0.028f + 61f, z * 0.028f + 12f) - 0.5f) * 2f;
            h = Toward(h, 0.9f + hummock, Mask(x, z, -380f, -300f, 58f, 118f));

            // Sunblight Wastes: dunes and flat-topped mesas
            float dunes = 2.2f * Mathf.Abs(Mathf.Sin(x * 0.045f + 2.2f * Fbm(x * 0.01f, z * 0.01f) + z * 0.012f));
            h = Toward(h, 5.5f + dunes, Mask(x, z, 30f, -440f, 74f, 146f));
            foreach (var m in Mesas)
            {
                float dist = Vector2.Distance(p, new Vector2(m.x, m.y));
                float k = 1f - SmoothStep(m.z * 0.72f, m.z, dist);
                h += k * m.w;
            }

            // Sapphire Woods: rolling, slightly lifted
            h += 3.5f * Mask(x, z, 280f, 60f, 70f, 150f) * Fbm(x * 0.009f + 44f, z * 0.009f + 71f);

            return h;
        }

        // ------------------------------------------------------------------ roads

        /// <summary>1 on the road center, fading to 0 at the verge (in meters).</summary>
        public static float RoadFactor(float x, float z)
        {
            float d = DistanceToRoad(x, z);
            return d >= 5.2f ? 0f : 1f - SmoothStep(2.2f, 5.2f, d);
        }

        public static float DistanceToRoad(float x, float z) => PolylineDistance(new Vector2(x, z), Roads);

        /// <summary>True inside a landmark clearing where wild vegetation must not spawn.</summary>
        public static bool InSiteClearing(float x, float z, float extra = 0f)
        {
            for (int i = 0; i < Sites.Length; i++)
            {
                var s = Sites[i];
                float r = (s.Kind == SiteKind.Shrine ? 15f : 42f) + extra;
                if ((new Vector2(x, z) - new Vector2(s.Position.x, s.Position.z)).sqrMagnitude < r * r) return true;
            }
            return false;
        }

        // ------------------------------------------------------------------ faceted terrain color

        private static readonly float[] _weightScratch = new float[BiomeCount];

        /// <summary>Linear-space color for one terrain facet (sampled at its centroid).</summary>
        public static Color TerrainColor(float x, float z, float y, float normalY, float faceRandom)
        {
            BiomeWeights(x, z, _weightScratch);

            float v = Fbm(x * 0.012f + 7.7f, z * 0.012f + 2.3f);
            Color ground = Color.black, rock = Color.black, high = Color.black;
            for (int i = 0; i < BiomeCount - 1; i++)
            {
                float w = _weightScratch[i];
                if (w <= 0.002f) continue;
                var p = Palettes[i];
                ground += Color.Lerp(p.Ground, p.Ground2, v) * w;
                rock += p.Rock * w;
                high += p.High * w;
            }

            // Steep faces read as bare rock
            float slope = SmoothStep(0.80f, 0.52f, normalY);
            Color c = Color.Lerp(ground, rock, slope);

            // High ground: rock, then snow caps
            float alt = SmoothStep(34f, 62f, y);
            c = Color.Lerp(c, high, alt * 0.85f);
            c = Color.Lerp(c, SnowColor, SmoothStep(76f, 94f, y));

            bool glacier = _weightScratch[(int)BiomeId.CrystalGlacier] > 0.55f;
            bool caldera = _weightScratch[(int)BiomeId.AshCaldera] > 0.55f;

            // Beaches around sea level (ice at the glacier, no sand in the caldera)
            if (!glacier && !caldera)
            {
                float beach = SmoothStep(3.2f, 0.9f, y) * SmoothStep(-1.6f, -0.3f, y);
                c = Color.Lerp(c, SandColor, beach);
            }
            if (glacier) c = Color.Lerp(c, Palettes[(int)BiomeId.CrystalGlacier].Ground2, SmoothStep(2.5f, 0.4f, y) * 0.6f);

            // Under water: seabed tint
            c = Color.Lerp(c, SeabedColor, SmoothStep(-0.4f, -4f, y) * 0.8f);

            // Dirt roads
            float road = RoadFactor(x, z);
            if (road > 0f && y > 1.2f) c = Color.Lerp(c, DirtColor, road * 0.88f);

            // Facet variation: the signature low-poly shading
            float k = 1f + (faceRandom - 0.5f) * 0.11f;
            return new Color(c.r * k, c.g * k, c.b * k, 1f);
        }

        /// <summary>Color of one pixel of the flat 2D overview used by the minimap (sRGB).</summary>
        public static Color MapColor(float x, float z)
        {
            if (UsesCourtyardLayout)
            {
                var p = new Vector3(x, 0f, z);
                var palette = Palettes[(int)BiomeAt(x, z)];
                float elevation = SampleHeight(p);
                if (elevation < 0.25f) return Palettes[(int)BiomeId.AetherianSea].Ground.gamma;
                return Color.Lerp(palette.Ground2, palette.Rock, Mathf.Clamp01(elevation / 80f)).gamma;
            }
            float y = HeightAt(x, z);
            if (y <= SeaLevel)
            {
                float deep = SmoothStep(0f, -9f, y);
                var sea = Palettes[(int)BiomeId.AetherianSea];
                return Color.Lerp(sea.Ground2, sea.Rock, deep).gamma;
            }
            const float e = 1.5f;
            float hx = HeightAt(x + e, z) - HeightAt(x - e, z);
            float hz = HeightAt(x, z + e) - HeightAt(x, z - e);
            Vector3 n = new Vector3(-hx, 2f * e, -hz).normalized;
            return TerrainColor(x, z, y, n.y, 0.5f).gamma;
        }

        // ------------------------------------------------------------------ helpers

        private static float Mask(float x, float z, float cx, float cz, float inner, float outer) =>
            1f - SmoothStep(inner, outer, Vector2.Distance(new Vector2(x, z), new Vector2(cx, cz)));

        private static float EllipseMask(float x, float z, float cx, float cz, float sx, float sz, float inner, float outer) =>
            1f - SmoothStep(inner, outer, Vector2.Distance(new Vector2(x / sx, z / sz), new Vector2(cx / sx, cz / sz)));

        private static float Toward(float value, float target, float mask) => Mathf.Lerp(value, target, mask);

        public static float SmoothStep(float edge0, float edge1, float x)
        {
            float t = Mathf.Clamp01((x - edge0) / (edge1 - edge0));
            return t * t * (3f - 2f * t);
        }

        /// <summary>Fractal value noise in 0..1.</summary>
        public static float Fbm(float x, float z)
        {
            float a = Mathf.PerlinNoise(x, z);
            float b = Mathf.PerlinNoise(x * 2.03f + 13.1f, z * 2.03f + 7.7f);
            float c = Mathf.PerlinNoise(x * 4.07f + 3.3f, z * 4.07f + 27.9f);
            return (a * 0.55f + b * 0.3f + c * 0.15f);
        }

        private static float Ridged(float x, float z) => 1f - Mathf.Abs(Mathf.PerlinNoise(x, z) * 2f - 1f);

        private static float PolylineDistance(Vector2 p, Vector2[][] lines)
        {
            float best = float.MaxValue;
            for (int l = 0; l < lines.Length; l++) best = Mathf.Min(best, PolylineDistance(p, lines[l]));
            return best;
        }

        private static float PolylineDistance(Vector2 p, Vector2[] pts)
        {
            float best = float.MaxValue;
            for (int i = 0; i < pts.Length - 1; i++)
            {
                var ab = pts[i + 1] - pts[i];
                float len2 = ab.sqrMagnitude;
                float t = len2 > 1e-6f ? Mathf.Clamp01(Vector2.Dot(p - pts[i], ab) / len2) : 0f;
                best = Mathf.Min(best, Vector2.Distance(p, pts[i] + ab * t));
            }
            return best;
        }

        /// <summary>Deterministic hash in 0..1 for two integers (facet variation, scattering).</summary>
        public static float Hash(int x, int y, int seed = 0)
        {
            unchecked
            {
                uint h = (uint)(x * 374761393) ^ (uint)(y * 668265263) ^ (uint)(seed * 2147483647 + seed * 1013904223);
                h = (h ^ (h >> 13)) * 1274126177u;
                h ^= h >> 16;
                return (h & 0x00FFFFFF) / (float)0x01000000;
            }
        }
    }
}
