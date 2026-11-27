// ============================================================================
// src/config.js — World layout, deterministic terrain, collision & landmarks
// ============================================================================
import * as THREE from 'three';

export const WORLD = {
  SIZE: 480,
  HALF: 240,
  WATER_LEVEL: 0.0,
  GRAVITY: 28.0,
  PLAYER_RADIUS: 0.7,
  PLAYER_HEIGHT: 1.9,
};

export const PALETTE = {
  gold: 0xf5a64a,
  skyZenithDay: 0x2f63a8,
  skyHorizonDay: 0x9ec8e8,
  skyZenithSunset: 0x222c5c,
  skyHorizonSunset: 0xe98a52,
  skyZenithNight: 0x050816,
  skyHorizonNight: 0x101c38,
  grassLush: 0x3b6e3a,
  grassSunlit: 0x6b8f43,
  grassValley: 0x284e32,
  blueLeafDark: 0x1f5382,
  blueLeafMid: 0x2e75ad,
  blueLeafBright: 0x4ca6db,
  stoneLight: 0x7b808a,
  stoneDark: 0x484d57,
  obsidian: 0x1a1c24,
  crimson: 0xc82828,
  lavaGlow: 0xff4c18,
  runeGold: 0xf2c15a,
  runeCyan: 0x4de6ff,
};

export const LANDMARKS = {
  spawn: { x: 0, z: 54, label: 'Village Gate' },
  village: { x: 0, z: 42, radius: 28, label: 'Oakhaven Village', icon: 'village' },
  dragonPerch: { x: -32, z: 22, label: 'Dragon Aerie', icon: 'dragon' },
  lake: { x: 0, z: -8, rx: 44, rz: 27, label: 'Mirror Lake', icon: 'lake' },
  bridge: { x: 0, zStart: 19, zEnd: -35, halfWidth: 4.1, label: 'Stone Bridge', icon: 'bridge' },
  ruins: { x: 38, z: 12, label: 'Rune Circle', icon: 'ruins' },
  castleGate: { x: 0, z: -76, label: 'Highkeep Gate', icon: 'castle' },
  castle: { x: 0, z: -108, radius: 34, floorY: 18.0, label: 'Highkeep Castle', icon: 'castle' },
  bossArena: { x: 0, z: -112, radius: 22, floorY: 18.0, label: 'Throne Courtyard', icon: 'boss' },
  dungeonEntrance: { x: 94, z: -24, label: 'Ember Crypt', icon: 'dungeon' },
  dungeonCenter: { x: 132, z: -24, minX: 96, maxX: 166, minZ: -42, maxZ: -6, floorY: 5.5, ceilingY: 19.5 },
  dungeonRelic: { x: 156, z: -24, label: 'Sunstone Altar', icon: 'relic' },
  skyRings: [
    { x: -52, y: 26, z: -12, radius: 7.2 },
    { x: 46, y: 31, z: -48, radius: 7.2 },
    { x: -34, y: 36, z: -84, radius: 7.2 },
  ],
};

// Static solid obstacles populated by world.js for smooth circle/AABB collisions
export const COLLIDERS = [];

export function addCircleCollider(x, z, r, minY = -100, maxY = 100) {
  COLLIDERS.push({ type: 'circle', x, z, r, minY, maxY });
}

export function addBoxCollider(minX, maxX, minZ, maxZ, minY = -100, maxY = 100) {
  COLLIDERS.push({ type: 'box', minX, maxX, minZ, maxZ, minY, maxY });
}

// Deterministic hash & smooth 2D value noise
function hash2(ix, iz) {
  let n = Math.sin(ix * 127.1 + iz * 311.7) * 43758.5453123;
  return n - Math.floor(n);
}

export function noise2D(x, z) {
  const ix = Math.floor(x);
  const iz = Math.floor(z);
  const fx = x - ix;
  const fz = z - iz;
  const ux = fx * fx * (3.0 - 2.0 * fx);
  const uz = fz * fz * (3.0 - 2.0 * fz);
  const a = hash2(ix, iz);
  const b = hash2(ix + 1, iz);
  const c = hash2(ix, iz + 1);
  const d = hash2(ix + 1, iz + 1);
  return a + (b - a) * ux + (c - a) * uz + (a - b - c + d) * ux * uz;
}

export function fbm(x, z, octaves = 4) {
  let value = 0;
  let amp = 0.5;
  let freq = 1.0;
  for (let i = 0; i < octaves; i++) {
    value += amp * (noise2D(x * freq, z * freq) * 2.0 - 1.0);
    freq *= 2.03;
    amp *= 0.48;
  }
  return value;
}

function smoothstep(edge0, edge1, x) {
  const t = Math.max(0, Math.min(1, (x - edge0) / (edge1 - edge0)));
  return t * t * (3 - 2 * t);
}

export function isInsideDungeon(x, z) {
  const d = LANDMARKS.dungeonCenter;
  return x >= d.minX - 2 && x <= d.maxX + 2 && z >= d.minZ - 2 && z <= d.maxZ + 2;
}

export function isOnBridge(x, z) {
  const b = LANDMARKS.bridge;
  return Math.abs(x - b.x) <= b.halfWidth && z <= b.zStart + 1.5 && z >= b.zEnd - 1.5;
}

export function getBridgeDeckHeight(z) {
  const b = LANDMARKS.bridge;
  const t = Math.max(0, Math.min(1, (b.zStart - z) / (b.zStart - b.zEnd)));
  // Smooth stone arch: 2.6 at ends, peaking at 5.6 in the center
  const arch = Math.sin(t * Math.PI);
  return 2.75 + arch * 2.95;
}

export function isInLake(x, z) {
  const l = LANDMARKS.lake;
  const dx = (x - l.x) / l.rx;
  const dz = (z - l.z) / l.rz;
  return dx * dx + dz * dz < 0.92;
}

/**
 * Deterministic ground terrain height (excluding bridge deck).
 */
export function getTerrainHeight(x, z) {
  // Base rolling green hills
  let h = 4.2 + fbm(x * 0.011, z * 0.011, 4) * 11.5 + fbm(x * 0.032 + 17.3, z * 0.032 - 9.1, 2) * 2.1;

  // Outer mountain rim beyond radius 165
  const distOrigin = Math.hypot(x, z);
  const rimT = smoothstep(160, 235, distOrigin);
  h += rimT * (38.0 + Math.abs(fbm(x * 0.025, z * 0.025, 3)) * 24.0);

  // Village clearing plateau around (0, 42)
  const dVill = Math.hypot(x - LANDMARKS.village.x, z - LANDMARKS.village.z);
  const villT = 1.0 - smoothstep(22, 42, dVill);
  h = THREE.MathUtils.lerp(h, 4.4, villT);

  // Dragon perch promontory around (-32, 22)
  const dDrag = Math.hypot(x - LANDMARKS.dragonPerch.x, z - LANDMARKS.dragonPerch.z);
  const dragT = 1.0 - smoothstep(8, 22, dDrag);
  h = THREE.MathUtils.lerp(h, 7.2, dragT);

  // Rune circle clearing around (38, 12)
  const dRuins = Math.hypot(x - LANDMARKS.ruins.x, z - LANDMARKS.ruins.z);
  const ruinsT = 1.0 - smoothstep(10, 22, dRuins);
  h = THREE.MathUtils.lerp(h, 5.2, ruinsT);

  // Lake basin depression around (0, -8)
  const l = LANDMARKS.lake;
  const lakeNorm = Math.hypot((x - l.x) / l.rx, (z - l.z) / l.rz);
  if (lakeNorm < 1.38) {
    const lakeT = 1.0 - smoothstep(0.65, 1.35, lakeNorm);
    h = THREE.MathUtils.lerp(h, -4.6, lakeT);
  }

  // Bridge approach banks at north (0, -35) and south (0, 19)
  const dSouth = Math.hypot(x * 0.85, z - 21);
  const southT = 1.0 - smoothstep(4, 13, dSouth);
  h = THREE.MathUtils.lerp(h, 2.75, southT);

  const dNorth = Math.hypot(x * 0.85, z + 37);
  const northT = 1.0 - smoothstep(4, 13, dNorth);
  h = THREE.MathUtils.lerp(h, 2.85, northT);

  // Castle Hill plateau around (0, -108)
  const dCastle = Math.hypot(x - LANDMARKS.castle.x, z - LANDMARKS.castle.z);
  const castleT = 1.0 - smoothstep(33, 62, dCastle);
  h = THREE.MathUtils.lerp(h, LANDMARKS.castle.floorY, castleT);

  // Ascending road from north bridge end (0, -35) to castle gate (0, -76)
  if (z < -33 && z > -79 && Math.abs(x) < 16) {
    const roadAlong = Math.max(0, Math.min(1, (-35 - z) / (76 - 35)));
    const roadTarget = THREE.MathUtils.lerp(2.85, LANDMARKS.castle.floorY, smoothstep(0, 1, roadAlong));
    const roadAcross = 1.0 - smoothstep(4.5, 14.5, Math.abs(x));
    h = THREE.MathUtils.lerp(h, roadTarget, roadAcross);
  }

  // Eastern road toward Dark Dungeon entrance (from (42, 6) -> (92, -24))
  const dDungEnt = Math.hypot(x - 84, z - (-24));
  const entT = 1.0 - smoothstep(10, 24, dDungEnt);
  h = THREE.MathUtils.lerp(h, 5.5, entT);

  // Dungeon chamber carved flat floor + mountain dome overhead (terrain height stays at floor inside chamber so player walks cleanly; world.js builds the cavern shell & rocky mountain dome above it!)
  const d = LANDMARKS.dungeonCenter;
  if (x > d.minX - 14 && x < d.maxX + 18 && z > d.minZ - 14 && z < d.maxZ + 14) {
    const insideX = 1.0 - smoothstep(0, 12, Math.max(d.minX - x, x - d.maxX, 0));
    const insideZ = 1.0 - smoothstep(0, 12, Math.max(d.minZ - z, z - d.maxZ, 0));
    h = THREE.MathUtils.lerp(h, d.floorY, insideX * insideZ);
  }

  return h;
}

/**
 * Walkable height including the arched stone bridge deck.
 */
export function getWalkableHeight(x, z, currentY = 10) {
  const terrain = getTerrainHeight(x, z);
  if (isOnBridge(x, z)) {
    const deck = getBridgeDeckHeight(z);
    // If hero/entity isn't swimming deep under the bridge, stand on the bridge deck
    if (currentY >= deck - 2.6 || terrain < deck - 0.5) {
      return Math.max(terrain, deck);
    }
  }
  return terrain;
}

/**
 * Resolves horizontal collisions against static buildings, castle walls, bridge parapets, and dungeon walls.
 */
export function resolveWorldCollisions(pos, radius = WORLD.PLAYER_RADIUS) {
  // Keep within playable valley bounds
  const maxR = WORLD.HALF - 18;
  const dist = Math.hypot(pos.x, pos.z);
  if (dist > maxR) {
    const s = maxR / dist;
    pos.x *= s;
    pos.z *= s;
  }

  // Bridge side parapets when on the bridge
  const b = LANDMARKS.bridge;
  if (pos.z < b.zStart - 0.5 && pos.z > b.zEnd + 0.5 && Math.abs(pos.x - b.x) < b.halfWidth + 1.2) {
    const deckY = getBridgeDeckHeight(pos.z);
    if (pos.y >= deckY - 1.2 && pos.y <= deckY + 2.4) {
      const limit = b.halfWidth - 0.55 - radius;
      if (pos.x > b.x + limit) pos.x = b.x + limit;
      if (pos.x < b.x - limit) pos.x = b.x - limit;
    }
  }

  for (let i = 0; i < COLLIDERS.length; i++) {
    const c = COLLIDERS[i];
    if (pos.y + WORLD.PLAYER_HEIGHT < c.minY || pos.y > c.maxY) continue;
    if (c.type === 'circle') {
      const dx = pos.x - c.x;
      const dz = pos.z - c.z;
      const minD = c.r + radius;
      const dSq = dx * dx + dz * dz;
      if (dSq < minD * minD && dSq > 0.00001) {
        const d = Math.sqrt(dSq);
        const push = (minD - d) / d;
        pos.x += dx * push;
        pos.z += dz * push;
      }
    } else if (c.type === 'box') {
      const closestX = Math.max(c.minX, Math.min(pos.x, c.maxX));
      const closestZ = Math.max(c.minZ, Math.min(pos.z, c.maxZ));
      const dx = pos.x - closestX;
      const dz = pos.z - closestZ;
      const dSq = dx * dx + dz * dz;
      if (dSq < radius * radius) {
        if (dSq > 0.00001) {
          const d = Math.sqrt(dSq);
          const push = radius - d;
          pos.x += (dx / d) * push;
          pos.z += (dz / d) * push;
        } else {
          // Center inside AABB: push out along nearest edge
          const dl = Math.abs(pos.x - c.minX);
          const dr = Math.abs(c.maxX - pos.x);
          const dt = Math.abs(pos.z - c.minZ);
          const db = Math.abs(c.maxZ - pos.z);
          const m = Math.min(dl, dr, dt, db);
          if (m === dl) pos.x = c.minX - radius;
          else if (m === dr) pos.x = c.maxX + radius;
          else if (m === dt) pos.z = c.minZ - radius;
          else pos.z = c.maxZ + radius;
        }
      }
    }
  }
}

export function mulberry32(seed) {
  let a = seed >>> 0;
  return function () {
    a |= 0;
    a = (a + 0x6d2b79f5) | 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}
