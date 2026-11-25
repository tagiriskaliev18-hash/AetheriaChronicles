using System;
using System.Collections.Generic;
using UnityEngine;

namespace Aetheria
{
    /// <summary>
    /// Quest-gated 10-Spell Invoker component:
    /// Unlock Exort (1 spell) -> Wex (4 spells) -> Quas (all 10 spells), plus Classic E/W/Q -> R -> D/F & quick slots 1..0.
    /// </summary>
    public class SpellSystem : MonoBehaviour
    {
        public bool UnlockedExort = false;
        public bool UnlockedWex = false;
        public bool UnlockedQuas = false;
        public bool GodModeActive = false;
        public bool BlockedByStature = false;

        public CastMode Mode = CastMode.ClassicInvoker;
        public readonly List<OrbType> ActiveOrbs = new List<OrbType>(3);
        public SpellId SlotD = SpellId.None;
        public SpellId SlotF = SpellId.None;

        public float Mana = 120f;
        public float MaxMana = 120f;
        public float ExortPowerScale = 1f;
        public float WexSpeedScale = 1f;
        public float QuasControlScale = 1f;
        public float WexPowerScale => WexSpeedScale;
        public float QuasPowerScale => QuasControlScale;
        public float ManaCostMultiplier = 1f;
        public float CooldownMultiplier = 1f;
        public float ManaRegenPerSec = 9f;

        public float InvokeCooldownRemaining { get; private set; }
        private readonly Dictionary<SpellId, float> _cooldowns = new Dictionary<SpellId, float>();
        private readonly List<SpellId> _cooldownKeysBuffer = new List<SpellId>(10);

        public event Action OnSpellStateChanged;
        public event Action<SpellId> OnSpellCast;

        private static List<SpellDef> _defs;

        public static List<SpellDef> GetAllSpellDefs()
        {
            if (_defs != null) return _defs;
            _defs = new List<SpellDef>
            {
                new SpellDef { Id = SpellId.SunStrike, ComboKey = "EEE", DisplayName = "Solar Cataclysm", Delivery = SpellDelivery.OrbitalStrike,
                    Description = "Calls down a concentrated pillar of Exort solar plasma from the heavens.",
                    NeedsExort = true, ManaCost = 35f, Cooldown = 5f, BaseDamage = 125f, BlastRadius = 5.4f, DestructionImpulse = 28f, PrimaryColor = new Color(1.0f, 0.42f, 0.06f) },
                new SpellDef { Id = SpellId.ChaosMeteor, ComboKey = "EEW", DisplayName = "Chaos Meteor", Delivery = SpellDelivery.RollingMeteor,
                    Description = "Summons a blazing astral boulder that crashes down and rolls forward along the earth.",
                    NeedsExort = true, NeedsWex = true, ManaCost = 45f, Cooldown = 7.5f, BaseDamage = 95f, BlastRadius = 5.2f, DestructionImpulse = 32f, PrimaryColor = new Color(1.0f, 0.22f, 0.04f) },
                new SpellDef { Id = SpellId.StormAlacrity, ComboKey = "EWW", DisplayName = "Tempest Surge", Delivery = SpellDelivery.SelfBuff,
                    Description = "Surrounds the caster in crackling solar-tempest rings, boosting speed & attack tempo.",
                    NeedsExort = true, NeedsWex = true, ManaCost = 25f, Cooldown = 6f, BaseDamage = 35f, BlastRadius = 3.6f, DestructionImpulse = 12f, PrimaryColor = new Color(0.95f, 0.78f, 0.18f) },
                new SpellDef { Id = SpellId.StormEMP, ComboKey = "WWW", DisplayName = "Plasma EMP Nova", Delivery = SpellDelivery.RadialNova,
                    Description = "Charges a volatile violet tempest singularity that detonates & siphons mana.",
                    NeedsWex = true, ManaCost = 40f, Cooldown = 7f, BaseDamage = 105f, BlastRadius = 6.8f, DestructionImpulse = 26f, PrimaryColor = new Color(0.76f, 0.24f, 1.0f) },
                new SpellDef { Id = SpellId.ColdSnap, ComboKey = "QQQ", DisplayName = "Glacial Cold Snap", Delivery = SpellDelivery.Projectile,
                    Description = "Launches a piercing cryo-crystal shard that freezes targets with repeating frost snaps.",
                    NeedsQuas = true, ManaCost = 22f, Cooldown = 3.5f, BaseDamage = 60f, BlastRadius = 2.6f, DestructionImpulse = 14f, PrimaryColor = new Color(0.22f, 0.85f, 1.0f) },
                new SpellDef { Id = SpellId.ChronoBlink, ComboKey = "QQW", DisplayName = "Chrono-Ghost Walk", Delivery = SpellDelivery.SelfBuff,
                    Description = "Phase-steps swiftly through fractured time leaving a detonating cryo-echo behind.",
                    NeedsWex = true, NeedsQuas = true, ManaCost = 28f, Cooldown = 5.5f, BaseDamage = 40f, BlastRadius = 4.2f, DestructionImpulse = 15f, PrimaryColor = new Color(0.36f, 0.68f, 1.0f) },
                new SpellDef { Id = SpellId.IcePrismWall, ComboKey = "QQE", DisplayName = "Cryo-Prism Wall", Delivery = SpellDelivery.BarrierWall,
                    Description = "Erupts a crystalline ice wall perpendicular to aim that blocks & chills enemies.",
                    NeedsExort = true, NeedsQuas = true, ManaCost = 34f, Cooldown = 6.5f, BaseDamage = 50f, BlastRadius = 4.8f, DestructionImpulse = 19f, PrimaryColor = new Color(0.48f, 0.94f, 1.0f) },
                new SpellDef { Id = SpellId.GravityTornado, ComboKey = "QWW", DisplayName = "Tempest Tornado", Delivery = SpellDelivery.Projectile,
                    Description = "Unleashes a roaring cyclone sweeping across terrain, lifting adversaries skyward.",
                    NeedsWex = true, NeedsQuas = true, ManaCost = 38f, Cooldown = 6.5f, BaseDamage = 72f, BlastRadius = 4.6f, DestructionImpulse = 24f, PrimaryColor = new Color(0.58f, 0.56f, 1.0f) },
                new SpellDef { Id = SpellId.EmberConduit, ComboKey = "QEE", DisplayName = "Forge Lava Conduit", Delivery = SpellDelivery.RadialNova,
                    Description = "Manifests a searing magma spire that radiates thermal shockwaves melting armor.",
                    NeedsExort = true, NeedsQuas = true, ManaCost = 36f, Cooldown = 6f, BaseDamage = 68f, BlastRadius = 5.0f, DestructionImpulse = 22f, PrimaryColor = new Color(1.0f, 0.30f, 0.14f) },
                new SpellDef { Id = SpellId.PrimalBlast, ComboKey = "QWE", DisplayName = "Primal Deafening Blast", Delivery = SpellDelivery.ConeWave,
                    Description = "Unites Exort, Wex, and Quas into an expanding tri-elemental shockwave wall.",
                    NeedsExort = true, NeedsWex = true, NeedsQuas = true, ManaCost = 50f, Cooldown = 8f, BaseDamage = 140f, BlastRadius = 6.4f, DestructionImpulse = 36f, PrimaryColor = new Color(0.85f, 0.62f, 1.0f) }
            };
            return _defs;
        }

        public static SpellDef GetSpellDef(SpellId id) => GetAllSpellDefs().Find(s => s.Id == id);

        private void Update()
        {
            float dt = Time.deltaTime;
            if (InvokeCooldownRemaining > 0f) InvokeCooldownRemaining = Mathf.Max(0f, InvokeCooldownRemaining - dt);
            Mana = GodModeActive ? MaxMana : Mathf.Min(MaxMana, Mana + ManaRegenPerSec * dt);

            if (_cooldowns.Count > 0)
            {
                _cooldownKeysBuffer.Clear();
                foreach (var k in _cooldowns.Keys) _cooldownKeysBuffer.Add(k);
                for (int i = 0; i < _cooldownKeysBuffer.Count; i++)
                {
                    SpellId k = _cooldownKeysBuffer[i];
                    float rem = Mathf.Max(0f, _cooldowns[k] - dt);
                    if (rem <= 0f) _cooldowns.Remove(k);
                    else _cooldowns[k] = rem;
                }
            }
        }

        public void UnlockOrb(OrbType orb)
        {
            switch (orb)
            {
                case OrbType.Exort: UnlockedExort = true; break;
                case OrbType.Wex:   UnlockedWex = true; break;
                case OrbType.Quas:  UnlockedQuas = true; break;
            }
            if (ActiveOrbs.Count < 3) PushOrb(orb);
            OnSpellStateChanged?.Invoke();
        }

        public bool IsOrbUnlocked(OrbType orb) => GodModeActive || orb switch
        {
            OrbType.Exort => UnlockedExort,
            OrbType.Wex   => UnlockedWex,
            OrbType.Quas  => UnlockedQuas,
            _             => false
        };

        public bool IsSpellUnlocked(SpellId id)
        {
            if (id == SpellId.None) return false;
            if (GodModeActive) return true;
            var d = GetSpellDef(id);
            if (d == null) return false;
            return (!d.NeedsExort || UnlockedExort) && (!d.NeedsWex || UnlockedWex) && (!d.NeedsQuas || UnlockedQuas);
        }

        public int GetUnlockedSpellCount()
        {
            int c = 0;
            foreach (var d in GetAllSpellDefs()) if (IsSpellUnlocked(d.Id)) c++;
            return c;
        }

        public float GetCooldownRemaining(SpellId id) => GodModeActive ? 0f : (_cooldowns.TryGetValue(id, out var v) ? v : 0f);
        public bool IsOnCooldown(SpellId id) => !GodModeActive && GetCooldownRemaining(id) > 0f;

        public float GetSpellPowerScale(SpellId id) => id switch
        {
            SpellId.SunStrike or SpellId.ChaosMeteor or SpellId.EmberConduit => ExortPowerScale,
            SpellId.StormAlacrity or SpellId.StormEMP                        => WexPowerScale,
            SpellId.ColdSnap or SpellId.ChronoBlink or SpellId.IcePrismWall  => QuasPowerScale,
            SpellId.GravityTornado                                           => (QuasPowerScale + WexPowerScale * 2f) / 3f,
            SpellId.PrimalBlast                                              => (ExortPowerScale + WexPowerScale + QuasPowerScale) / 3f,
            _                                                                => ExortPowerScale
        };

        public void SetGodMode(bool on)
        {
            GodModeActive = on;
            if (on)
            {
                UnlockedExort = UnlockedWex = UnlockedQuas = true;
                Mana = MaxMana;
                _cooldowns.Clear();
                if (ActiveOrbs.Count == 0) { ActiveOrbs.Add(OrbType.Exort); ActiveOrbs.Add(OrbType.Wex); ActiveOrbs.Add(OrbType.Quas); }
                if (SlotD == SpellId.None) SlotD = SpellId.SunStrike;
                if (SlotF == SpellId.None) SlotF = SpellId.ChaosMeteor;
            }
            OnSpellStateChanged?.Invoke();
        }

        public bool PushOrb(OrbType orb)
        {
            if (BlockedByStature && !GodModeActive)
            {
                QuestAndMapService.Instance?.PushNotification("Stature < 1.45m locks primal magic channeling!");
                return false;
            }
            if (!IsOrbUnlocked(orb)) return false;

            if (ActiveOrbs.Count >= 3) ActiveOrbs.RemoveAt(0);
            ActiveOrbs.Add(orb);
            AudioAndQualityService.Instance?.PlaySfx2D("SFX_Banner", 0.35f);
            OnSpellStateChanged?.Invoke();

            if (Mode == CastMode.InstantCombo && ActiveOrbs.Count == 3)
            {
                SpellId resolved = ResolveCurrentCombo();
                if (resolved != SpellId.None && !IsOnCooldown(resolved))
                {
                    if (InvokeCurrentSpell())
                    {
                        var hero = HeroController.Instance;
                        Vector3 origin = transform.position + Vector3.up * 1.4f;
                        Vector3 dir = hero != null && hero.MainCam != null ? hero.MainCam.transform.forward : transform.forward;
                        Vector3 target = origin + dir * 16f;
                        target.y = AetheriaWorld.SampleHeight(target);
                        CastSpecificSpell(resolved, origin, dir, target);
                    }
                }
            }
            return true;
        }

        public SpellId ResolveCurrentCombo()
        {
            if (ActiveOrbs.Count < 3) return SpellId.None;
            int e = 0, w = 0, q = 0;
            foreach (var o in ActiveOrbs)
            {
                if (o == OrbType.Exort) e++;
                else if (o == OrbType.Wex) w++;
                else if (o == OrbType.Quas) q++;
            }
            SpellId id = SpellId.None;
            if (e == 3) id = SpellId.SunStrike;
            else if (e == 2 && w == 1) id = SpellId.ChaosMeteor;
            else if (e == 1 && w == 2) id = SpellId.StormAlacrity;
            else if (w == 3) id = SpellId.StormEMP;
            else if (q == 3) id = SpellId.ColdSnap;
            else if (q == 2 && w == 1) id = SpellId.ChronoBlink;
            else if (q == 2 && e == 1) id = SpellId.IcePrismWall;
            else if (q == 1 && w == 2) id = SpellId.GravityTornado;
            else if (q == 1 && e == 2) id = SpellId.EmberConduit;
            else if (q == 1 && w == 1 && e == 1) id = SpellId.PrimalBlast;
            return IsSpellUnlocked(id) ? id : SpellId.None;
        }

        public bool InvokeCurrentSpell()
        {
            if (BlockedByStature && !GodModeActive)
            {
                QuestAndMapService.Instance?.PushNotification("Stature < 1.45m cannot invoke spells!");
                return false;
            }
            if (!GodModeActive && InvokeCooldownRemaining > 0f) return false;

            SpellId resolved = ResolveCurrentCombo();
            if (resolved == SpellId.None) return false;

            if (SlotD != resolved)
            {
                if (SlotD != SpellId.None) SlotF = SlotD;
                SlotD = resolved;
            }
            InvokeCooldownRemaining = GodModeActive ? 0f : 0.65f * CooldownMultiplier;
            AudioAndQualityService.Instance?.PlaySfx2D("SW_MagicCast_Buff1_1", 0.55f);
            OnSpellStateChanged?.Invoke();
            return true;
        }

        public bool CastSlot(bool isSlotF, Vector3 origin, Vector3 dir, Vector3 target)
        {
            SpellId spell = isSlotF ? SlotF : SlotD;
            return CastSpecificSpell(spell, origin, dir, target);
        }

        public bool CastSpecificSpell(SpellId spell, Vector3 origin, Vector3 dir, Vector3 target)
        {
            if (spell == SpellId.None) return false;
            if (BlockedByStature && !GodModeActive)
            {
                QuestAndMapService.Instance?.PushNotification("Small Stature (<1.45m) forbids spellcasting!");
                return false;
            }
            if (!IsSpellUnlocked(spell))
            {
                QuestAndMapService.Instance?.PushNotification("This primal spell has not been unlocked yet.");
                return false;
            }
            var def = GetSpellDef(spell);
            if (def == null) return false;

            if (IsOnCooldown(spell)) return false;
            float cost = GodModeActive ? 0f : def.ManaCost * ManaCostMultiplier;
            if (Mana < cost)
            {
                QuestAndMapService.Instance?.PushNotification($"Low Mana ({Mana:F0}/{cost:F0})");
                return false;
            }

            Mana -= cost;
            if (!GodModeActive) _cooldowns[spell] = def.Cooldown * CooldownMultiplier;

            float powerScale = GetSpellPowerScale(spell);
            float dmg = def.BaseDamage * powerScale;
            SpellVfxExecutor.SpawnSpell(def, gameObject, origin, dir, target, dmg, powerScale, WexSpeedScale, QuasControlScale);
            OnSpellCast?.Invoke(spell);
            OnSpellStateChanged?.Invoke();
            AudioAndQualityService.Instance?.NotifyCombat();
            return true;
        }

        public string GetOrbsString()
        {
            string s = "";
            for (int i = 0; i < 3; i++)
            {
                if (i < ActiveOrbs.Count)
                    s += ActiveOrbs[i] == OrbType.Exort ? "E" : ActiveOrbs[i] == OrbType.Wex ? "W" : "Q";
                else s += "-";
            }
            return s;
        }
    }
}
