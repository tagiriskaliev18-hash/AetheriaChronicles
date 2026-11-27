// ============================================================================
// src/combat.js — Skeletons, Forest Trolls, Castle Boss Malakor, Combat & Loot
// ============================================================================
import * as THREE from 'three';
import {
  WORLD,
  PALETTE,
  LANDMARKS,
  getWalkableHeight,
  resolveWorldCollisions,
  isInsideDungeon,
  mulberry32,
} from './config.js';

// Preallocated scratch objects (zero per-frame heap allocations in update())
const _v1 = new THREE.Vector3();
const _v2 = new THREE.Vector3();
const _v3 = new THREE.Vector3();
const _v4 = new THREE.Vector3();
const _v5 = new THREE.Vector3();
const _upAxis = new THREE.Vector3(0, 1, 0);
const _m4 = new THREE.Matrix4();
const _q1 = new THREE.Quaternion();
const _scaleVec = new THREE.Vector3();
const _col1 = new THREE.Color();
const _col2 = new THREE.Color();
const _flashCol = new THREE.Color(0xfff0d4);
const _burnCol = new THREE.Color(0xff5a12);
const _deepCrimsonCol = new THREE.Color(0xc81010);
const _scarletBlazeCol = new THREE.Color(0xff2211);
const _bossIdleEmissive = new THREE.Color(0x1f0508);
const _bossFurnaceIdle = new THREE.Color(0xff3808);
const _bossCrownIdle = new THREE.Color(0x8a180e);
const _bossHammerIdle = new THREE.Color(0xd93a12);
const _zeroCol = new THREE.Color(0x000000);

/**
 * Init-only helper to merge static same-material sub-geometries into a single
 * BufferGeometry, drastically reducing enemy & boss draw calls.
 */
function mergeGeos(parts) {
  const transformed = [];
  let totalVerts = 0;
  const mat = new THREE.Matrix4();
  const pos = new THREE.Vector3();
  const quat = new THREE.Quaternion();
  const euler = new THREE.Euler();
  const scl = new THREE.Vector3();

  for (let i = 0; i < parts.length; i++) {
    const p = parts[i];
    const g = p.geo.index ? p.geo.toNonIndexed() : p.geo.clone();
    if (p.matrix) {
      g.applyMatrix4(p.matrix);
    } else {
      if (p.pos) pos.set(p.pos[0], p.pos[1], p.pos[2]);
      else pos.set(0, 0, 0);
      if (p.rot) euler.set(p.rot[0], p.rot[1], p.rot[2], 'XYZ');
      else euler.set(0, 0, 0, 'XYZ');
      quat.setFromEuler(euler);
      if (p.scale) scl.set(p.scale[0], p.scale[1], p.scale[2]);
      else scl.set(1, 1, 1);
      mat.compose(pos, quat, scl);
      g.applyMatrix4(mat);
    }
    if (!g.attributes.normal) g.computeVertexNormals();
    totalVerts += g.attributes.position.count;
    transformed.push(g);
  }

  const positions = new Float32Array(totalVerts * 3);
  const normals = new Float32Array(totalVerts * 3);
  const uvs = new Float32Array(totalVerts * 2);
  let vOff = 0;
  for (let i = 0; i < transformed.length; i++) {
    const g = transformed[i];
    const cnt = g.attributes.position.count;
    positions.set(g.attributes.position.array, vOff * 3);
    normals.set(g.attributes.normal.array, vOff * 3);
    if (g.attributes.uv) {
      uvs.set(g.attributes.uv.array, vOff * 2);
    }
    vOff += cnt;
    g.dispose();
  }

  const merged = new THREE.BufferGeometry();
  merged.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  merged.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
  merged.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
  merged.computeBoundingSphere();
  return merged;
}

function angleDiff(from, to) {
  let d = (to - from + Math.PI) % (Math.PI * 2);
  if (d < 0) d += Math.PI * 2;
  return d - Math.PI;
}

function turnToward(current, target, maxStep) {
  const diff = angleDiff(current, target);
  if (Math.abs(diff) <= maxStep) return target;
  return current + Math.sign(diff) * maxStep;
}

/**
 * Ensures non-dungeon ground spawns near Mirror Lake sit on solid dry bank.
 */
function findDrySpawnXZ(x, z, out) {
  let cx = x;
  let cz = z;
  if (isInsideDungeon(cx, cz)) {
    out.set(cx, LANDMARKS.dungeonCenter.floorY, cz);
    return out;
  }
  let y = getWalkableHeight(cx, cz, 12);
  if (y < WORLD.WATER_LEVEL + 0.45) {
    const dx = cx - LANDMARKS.lake.x;
    const dz = cz - LANDMARKS.lake.z;
    const len = Math.hypot(dx, dz) || 1;
    const nx = dx / len;
    const nz = dz / len;
    for (let step = 0; step < 36 && y < WORLD.WATER_LEVEL + 0.55; step++) {
      cx += nx * 1.4;
      cz += nz * 1.4;
      y = getWalkableHeight(cx, cz, 12);
    }
  }
  out.set(cx, y, cz);
  return out;
}

function makeGlowSprite(textures, colorHex, scale, opacity = 0.85) {
  const mat = new THREE.SpriteMaterial({
    map: textures && textures.softGlow ? textures.softGlow : null,
    color: colorHex,
    transparent: true,
    opacity,
    blending: THREE.AdditiveBlending,
    depthWrite: false,
  });
  const sp = new THREE.Sprite(mat);
  sp.scale.set(scale, scale, 1);
  return sp;
}

export function createCombatSystem(scene, textures, sound) {
  const rng = mulberry32(84729);
  const combatRoot = new THREE.Group();
  combatRoot.name = 'CombatSystem';
  scene.add(combatRoot);

  // ==========================================================================
  // SHARED GEOMETRIES (stylized 6-8 segment low-poly + merged rigid sub-parts)
  // ==========================================================================
  // Temporary primitives consumed once to build merged shared geometries
  const _tSkelEye = new THREE.SphereGeometry(0.036, 6, 4);
  const _tSkelSpine = new THREE.CylinderGeometry(0.048, 0.055, 0.46, 6);
  const _tSkelSternum = new THREE.BoxGeometry(0.065, 0.34, 0.05);
  const _tSkelRib = new THREE.TorusGeometry(0.175, 0.025, 5, 8, Math.PI * 1.38);
  const _tSkelCloth = new THREE.BoxGeometry(0.24, 0.34, 0.03);
  const _tSkelBoneLong = new THREE.CylinderGeometry(0.036, 0.032, 0.36, 6);
  const _tSkelLegBone = new THREE.CylinderGeometry(0.042, 0.034, 0.42, 6);
  const _tSkelJoint = new THREE.SphereGeometry(0.048, 6, 4);
  const _tSkelFoot = new THREE.BoxGeometry(0.11, 0.06, 0.21);
  const _tSkelBlade = new THREE.BoxGeometry(0.055, 0.78, 0.02);
  const _tSkelBladeTip = new THREE.ConeGeometry(0.038, 0.16, 4);
  const _tSkelGuard = new THREE.BoxGeometry(0.24, 0.045, 0.06);
  const _tSkelBucklerRim = new THREE.TorusGeometry(0.32, 0.028, 5, 8);
  const _tSkelBucklerBoss = new THREE.ConeGeometry(0.075, 0.11, 6);

  const _tTrollTorso = new THREE.DodecahedronGeometry(0.76, 0);
  const _tTrollHump = new THREE.DodecahedronGeometry(0.62, 0);
  const _tTrollMossPad = new THREE.DodecahedronGeometry(0.36, 0);
  const _tTrollEye = new THREE.SphereGeometry(0.052, 6, 4);
  const _tTrollTusk = new THREE.ConeGeometry(0.055, 0.26, 5);
  const _tTrollHorn = new THREE.ConeGeometry(0.085, 0.56, 6);
  const _tTrollUpperArm = new THREE.CylinderGeometry(0.24, 0.2, 0.78, 7);
  const _tTrollForearm = new THREE.CylinderGeometry(0.22, 0.28, 0.84, 7);
  const _tTrollThigh = new THREE.CylinderGeometry(0.28, 0.22, 0.68, 7);
  const _tTrollShin = new THREE.CylinderGeometry(0.22, 0.25, 0.64, 7);
  const _tClubSpike = new THREE.ConeGeometry(0.058, 0.24, 4);

  const _trollMossOffsets = [
    [-0.56, 1.24, -0.14, 1.05],
    [0.54, 1.22, -0.12, 1.1],
    [0.02, 1.38, -0.24, 1.2],
    [-0.28, 1.12, -0.54, 0.9],
    [0.32, 1.05, -0.52, 0.85],
  ];
  const _trollMossParts = _trollMossOffsets.map((mp) => ({
    geo: _tTrollMossPad,
    pos: [mp[0], mp[1], mp[2]],
    scale: [mp[3], mp[3] * 0.65, mp[3] * 0.9],
  }));
  const _clubSpikeParts = [];
  for (let s = 0; s < 8; s++) {
    const sa = (s / 8) * Math.PI * 2;
    const sy = 1.05 + (s % 2) * 0.42;
    _clubSpikeParts.push({
      geo: _tClubSpike,
      pos: [Math.cos(sa) * 0.24, sy, Math.sin(sa) * 0.24],
      rot: [Math.sin(sa) * Math.PI * 0.5, 0, -Math.cos(sa) * Math.PI * 0.5],
    });
  }

  const geo = {
    // Skeleton (merged rigid sub-parts keep silhouettes sharp at ~half the draw calls)
    skelSkull: new THREE.DodecahedronGeometry(0.175, 0),
    skelJaw: new THREE.BoxGeometry(0.18, 0.08, 0.17),
    skelSocket: new THREE.BoxGeometry(0.22, 0.09, 0.06),
    skelEyes: mergeGeos([
      { geo: _tSkelEye, pos: [-0.06, 0.015, 0.155] },
      { geo: _tSkelEye, pos: [0.06, 0.015, 0.155] },
    ]),
    skelPelvis: new THREE.CylinderGeometry(0.17, 0.13, 0.16, 6),
    skelCloth: mergeGeos([
      { geo: _tSkelCloth, pos: [0, -0.14, 0.13] },
      { geo: _tSkelCloth, pos: [0, -0.14, -0.13] },
    ]),
    skelTorsoBone: mergeGeos([
      { geo: _tSkelSpine, pos: [0, 0.24, 0] },
      { geo: _tSkelSternum, pos: [0, 0.3, 0.14] },
      { geo: _tSkelRib, pos: [0, 0.19, 0.01], rot: [Math.PI * 0.5, 0, Math.PI * 0.81], scale: [0.86, 0.86, 1] },
      { geo: _tSkelRib, pos: [0, 0.30, 0.01], rot: [Math.PI * 0.5, 0, Math.PI * 0.81], scale: [0.96, 0.96, 1] },
      { geo: _tSkelRib, pos: [0, 0.41, 0.01], rot: [Math.PI * 0.5, 0, Math.PI * 0.81], scale: [1.06, 1.06, 1] },
    ]),
    skelPauldron: new THREE.SphereGeometry(0.14, 7, 4, 0, Math.PI * 2, 0, Math.PI * 0.6),
    skelRightArmBone: mergeGeos([
      { geo: _tSkelBoneLong, pos: [0, -0.18, 0] },
      { geo: _tSkelJoint, pos: [0, -0.36, 0] },
      { geo: _tSkelBoneLong, pos: [0, -0.49, 0.12], rot: [-0.58, 0, 0] },
    ]),
    skelLeftArmBone: mergeGeos([
      { geo: _tSkelBoneLong, pos: [0, -0.18, 0] },
      { geo: _tSkelJoint, pos: [0, -0.36, 0] },
      { geo: _tSkelBoneLong, pos: [0.03, -0.46, 0.16], rot: [-0.82, 0, 0] },
    ]),
    skelLegBone: mergeGeos([
      { geo: _tSkelLegBone, pos: [0, -0.21, 0] },
      { geo: _tSkelJoint, pos: [0, -0.42, 0] },
      { geo: _tSkelLegBone, pos: [0, -0.63, 0] },
      { geo: _tSkelFoot, pos: [0, -0.82, 0.05] },
    ]),
    skelSwordIron: mergeGeos([
      { geo: _tSkelGuard, pos: [0, 0.09, 0] },
      { geo: _tSkelBlade, pos: [0, 0.49, 0] },
      { geo: _tSkelBladeTip, pos: [0, 0.95, 0], rot: [0, Math.PI * 0.25, 0] },
    ]),
    skelHilt: new THREE.CylinderGeometry(0.024, 0.024, 0.18, 6),
    skelBuckler: new THREE.CylinderGeometry(0.32, 0.32, 0.045, 8),
    skelBucklerIron: mergeGeos([
      { geo: _tSkelBucklerRim },
      { geo: _tSkelBucklerBoss, pos: [0, 0, 0.04], rot: [Math.PI * 0.5, 0, 0] },
    ]),
    skelHelm: new THREE.CylinderGeometry(0.19, 0.2, 0.13, 7, 1, true),

    // Forest Troll (stylized faceted stone-moss silhouette with merged spikes/moss/horns)
    trollTorsoSkin: mergeGeos([
      { geo: _tTrollTorso, pos: [0, 0.62, 0.06], scale: [1.26, 1.14, 0.96] },
      { geo: _tTrollHump, pos: [0, 1.08, -0.22], scale: [1.38, 0.88, 1.05] },
    ]),
    trollBelly: new THREE.SphereGeometry(0.62, 7, 5),
    trollMossPads: mergeGeos(_trollMossParts),
    trollHead: new THREE.DodecahedronGeometry(0.38, 0),
    trollJaw: new THREE.BoxGeometry(0.46, 0.22, 0.42),
    trollBrow: new THREE.BoxGeometry(0.54, 0.12, 0.24),
    trollEyes: mergeGeos([
      { geo: _tTrollEye, pos: [-0.14, 0.04, 0.34] },
      { geo: _tTrollEye, pos: [0.14, 0.04, 0.34] },
    ]),
    trollHornsTusks: mergeGeos([
      { geo: _tTrollTusk, pos: [-0.17, -0.08, 0.36], rot: [0.32, 0, 0.24] },
      { geo: _tTrollTusk, pos: [0.17, -0.08, 0.36], rot: [0.32, 0, -0.24] },
      { geo: _tTrollHorn, pos: [-0.34, 0.28, -0.04], rot: [-0.45, 0, 0.72] },
      { geo: _tTrollHorn, pos: [0.34, 0.28, -0.04], rot: [-0.45, 0, -0.72] },
    ]),
    trollUpperArm: _tTrollUpperArm,
    trollForearm: _tTrollForearm,
    trollRightArmSkin: mergeGeos([
      { geo: _tTrollUpperArm, pos: [0, -0.35, 0], rot: [0, 0, -0.12] },
      { geo: _tTrollForearm, pos: [0.08, -1.02, 0.14], rot: [-0.28, 0, 0] },
    ]),
    trollFist: new THREE.DodecahedronGeometry(0.23, 0),
    trollLegSkin: mergeGeos([
      { geo: _tTrollThigh, pos: [0, -0.3, 0] },
      { geo: _tTrollShin, pos: [0, -0.78, 0] },
    ]),
    trollFoot: new THREE.BoxGeometry(0.36, 0.16, 0.54),
    trollLoin: new THREE.CylinderGeometry(0.66, 0.58, 0.44, 7),
    clubShaft: new THREE.CylinderGeometry(0.085, 0.065, 1.45, 6),
    clubHead: new THREE.CylinderGeometry(0.28, 0.14, 0.92, 6),
    clubSpikes: mergeGeos(_clubSpikeParts),

    // Loot
    coin: new THREE.CylinderGeometry(0.26, 0.26, 0.06, 10),
    coinRim: new THREE.TorusGeometry(0.25, 0.032, 5, 10),
    healthOrb: new THREE.OctahedronGeometry(0.27, 0),

    // Pooled VFX (sleek elongated diamond/octahedron ember shards)
    spark: new THREE.OctahedronGeometry(0.14, 0),
    fireballCore: new THREE.IcosahedronGeometry(0.48, 0),
    fireballShell: new THREE.IcosahedronGeometry(0.62, 0),
    shockwaveTorus: new THREE.TorusGeometry(1.0, 0.11, 6, 32),
    shockwaveDisc: new THREE.RingGeometry(0.74, 1.03, 32),
    cleaveWave: new THREE.RingGeometry(2.4, 5.9, 24, 1, -Math.PI * 0.5 - 1.12, 2.24),
  };

  // Dispose temporary source primitives that are no longer directly referenced
  _tSkelEye.dispose();
  _tSkelSpine.dispose();
  _tSkelSternum.dispose();
  _tSkelRib.dispose();
  _tSkelCloth.dispose();
  _tSkelBoneLong.dispose();
  _tSkelLegBone.dispose();
  _tSkelJoint.dispose();
  _tSkelFoot.dispose();
  _tSkelBlade.dispose();
  _tSkelBladeTip.dispose();
  _tSkelGuard.dispose();
  _tSkelBucklerRim.dispose();
  _tSkelBucklerBoss.dispose();
  _tTrollTorso.dispose();
  _tTrollHump.dispose();
  _tTrollMossPad.dispose();
  _tTrollEye.dispose();
  _tTrollTusk.dispose();
  _tTrollHorn.dispose();
  _tTrollThigh.dispose();
  _tTrollShin.dispose();
  _tClubSpike.dispose();

  // ==========================================================================
  // 1. SPARK / EMBER SHARD POOL (InstancedMesh — velocity-stretched diamonds)
  // ==========================================================================
  const MAX_SPARKS = 180;
  const sparkMat = new THREE.MeshBasicMaterial({
    color: 0xffffff,
    transparent: true,
    opacity: 0.85,
    blending: THREE.AdditiveBlending,
    depthWrite: false,
  });
  const sparkMesh = new THREE.InstancedMesh(geo.spark, sparkMat, MAX_SPARKS);
  sparkMesh.frustumCulled = false;
  sparkMesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
  combatRoot.add(sparkMesh);

  const sparks = new Array(MAX_SPARKS);
  let sparkCursor = 0;
  _scaleVec.set(0, 0, 0);
  _q1.identity();
  _m4.compose(_v1.set(0, -100, 0), _q1, _scaleVec);
  _col1.setHex(0xffb838);
  for (let i = 0; i < MAX_SPARKS; i++) {
    sparks[i] = {
      active: false,
      x: 0,
      y: -100,
      z: 0,
      vx: 0,
      vy: 0,
      vz: 0,
      life: 0,
      maxLife: 0.5,
      size: 1,
      gravity: 14,
      r: 1,
      g: 0.72,
      b: 0.22,
    };
    sparkMesh.setMatrixAt(i, _m4);
    sparkMesh.setColorAt(i, _col1);
  }
  sparkMesh.instanceMatrix.needsUpdate = true;
  if (sparkMesh.instanceColor) sparkMesh.instanceColor.needsUpdate = true;

  function spawnSpark(x, y, z, vx, vy, vz, colorHex, life = 0.45, size = 1.0, gravity = 14) {
    const idx = sparkCursor;
    sparkCursor = (sparkCursor + 1) % MAX_SPARKS;
    const s = sparks[idx];
    s.active = true;
    s.x = x;
    s.y = y;
    s.z = z;
    s.vx = vx;
    s.vy = vy;
    s.vz = vz;
    s.life = life;
    s.maxLife = life;
    s.size = size;
    s.gravity = gravity;
    _col1.setHex(colorHex);
    s.r = _col1.r;
    s.g = _col1.g;
    s.b = _col1.b;
    sparkMesh.setColorAt(idx, _col1);
    if (sparkMesh.instanceColor) sparkMesh.instanceColor.needsUpdate = true;
  }

  function spawnBurst(x, y, z, count, colorHex, speedMin = 2.5, speedMax = 8.5, life = 0.5, size = 1.0, gravity = 14) {
    // Pick a warm/magical companion ember tint so bursts shimmer instead of flat single tone
    let altHex = colorHex;
    if (colorHex === 0xff3318 || colorHex === 0xff4518 || colorHex === 0xff2222 || colorHex === 0xff2c2c) {
      altHex = 0xff9e1c;
    } else if (colorHex === 0xffc84a || colorHex === 0xffcc48 || colorHex === 0xffb824 || colorHex === 0xffd24c || colorHex === 0xffd859) {
      altHex = 0xff8c1a;
    } else if (colorHex === 0x4de6ff || colorHex === 0x62e8ff || colorHex === 0x2ce8ff) {
      altHex = 0xffc842;
    }
    for (let i = 0; i < count; i++) {
      const a = Math.random() * Math.PI * 2;
      const elev = (Math.random() - 0.15) * Math.PI * 0.65;
      const spd = speedMin + Math.random() * (speedMax - speedMin);
      const vx = Math.cos(a) * Math.cos(elev) * spd;
      const vy = Math.abs(Math.sin(elev)) * spd + 1.2;
      const vz = Math.sin(a) * Math.cos(elev) * spd;
      const col = (i & 1) === 0 ? colorHex : altHex;
      spawnSpark(x, y, z, vx, vy, vz, col, life * (0.7 + Math.random() * 0.55), size * (0.65 + Math.random() * 0.65), gravity);
    }
  }

  function updateSparks(dt) {
    let anyChanged = false;
    for (let i = 0; i < MAX_SPARKS; i++) {
      const s = sparks[i];
      if (!s.active) continue;
      anyChanged = true;
      s.life -= dt;
      if (s.life <= 0) {
        s.active = false;
        _scaleVec.set(0, 0, 0);
        _v1.set(0, -100, 0);
        _q1.identity();
        _m4.compose(_v1, _q1, _scaleVec);
        sparkMesh.setMatrixAt(i, _m4);
        continue;
      }
      s.vy -= s.gravity * dt;
      const drag = Math.max(0, 1 - dt * 1.45);
      s.vx *= drag;
      s.vz *= drag;
      s.x += s.vx * dt;
      s.y += s.vy * dt;
      s.z += s.vz * dt;

      const ratio = s.life / s.maxLife;
      const spdSq = s.vx * s.vx + s.vy * s.vy + s.vz * s.vz;
      let spd = 0;
      if (spdSq > 0.0025) {
        spd = Math.sqrt(spdSq);
        _v2.set(s.vx / spd, s.vy / spd, s.vz / spd);
        if (_v2.y < -0.999) {
          _q1.set(1, 0, 0, 0);
        } else {
          _q1.setFromUnitVectors(_upAxis, _v2);
        }
      } else {
        _q1.identity();
      }

      // Sleek elongated diamond ember shard stretched along velocity
      const width = s.size * (0.22 + ratio * 0.34);
      const stretch = s.size * (0.55 + ratio * 0.85) * Math.min(3.8, 1.45 + spd * 0.26);
      _v1.set(s.x, s.y, s.z);
      _scaleVec.set(width, stretch, width);
      _m4.compose(_v1, _q1, _scaleVec);
      sparkMesh.setMatrixAt(i, _m4);

      // Smooth luminance decay so additive ember shards glow richly without white-out
      const lum = 0.2 + ratio * 0.8;
      _col1.setRGB(s.r * lum, s.g * lum, s.b * lum);
      sparkMesh.setColorAt(i, _col1);
    }
    if (anyChanged) {
      sparkMesh.instanceMatrix.needsUpdate = true;
      if (sparkMesh.instanceColor) sparkMesh.instanceColor.needsUpdate = true;
    }
  }

  // ==========================================================================
  // 2. FLOATING DAMAGE / LOOT NUMBERS (pooled free-list + clean array splice)
  // ==========================================================================
  const damageNumbers = [];
  const dmgFreeList = [];

  function spawnDamageNumber(x, y, z, text, color = '#ffffff', maxLife = 0.95) {
    let entry;
    if (dmgFreeList.length > 0) {
      entry = dmgFreeList.pop();
      entry.text = text;
      entry.color = color;
      entry.worldPos.set(
        x + (Math.random() - 0.5) * 0.65,
        y,
        z + (Math.random() - 0.5) * 0.65
      );
      entry.life = maxLife;
      entry.maxLife = maxLife;
    } else {
      entry = {
        text,
        color,
        worldPos: new THREE.Vector3(
          x + (Math.random() - 0.5) * 0.65,
          y,
          z + (Math.random() - 0.5) * 0.65
        ),
        life: maxLife,
        maxLife,
      };
    }
    damageNumbers.push(entry);
    // Cap active list so UI pool never overflows
    if (damageNumbers.length > 24) {
      const oldest = damageNumbers.shift();
      dmgFreeList.push(oldest);
    }
  }

  function updateDamageNumbers(dt) {
    for (let i = damageNumbers.length - 1; i >= 0; i--) {
      const dn = damageNumbers[i];
      dn.life -= dt;
      dn.worldPos.y += dt * 1.45;
      if (dn.life <= 0) {
        damageNumbers.splice(i, 1);
        dmgFreeList.push(dn);
      }
    }
  }

  // ==========================================================================
  // 3. POOLED BOSS PROJECTILES, SHOCKWAVES & CLEAVE SLASH WAVES
  // ==========================================================================
  const FIREBALL_POOL_SIZE = 10;
  const fireballs = [];
  const fbCoreMat = new THREE.MeshBasicMaterial({ color: 0xffd26a });
  const fbShellMat = new THREE.MeshStandardMaterial({
    color: 0xff3b10,
    emissive: 0xff2200,
    emissiveIntensity: 2.4,
    roughness: 0.3,
    transparent: true,
    opacity: 0.88,
  });
  for (let i = 0; i < FIREBALL_POOL_SIZE; i++) {
    const g = new THREE.Group();
    g.visible = false;
    const core = new THREE.Mesh(geo.fireballCore, fbCoreMat);
    const shell = new THREE.Mesh(geo.fireballShell, fbShellMat);
    const halo = makeGlowSprite(textures, 0xff4c18, 2.7, 0.92);
    g.add(core, shell, halo);
    combatRoot.add(g);
    fireballs.push({
      active: false,
      group: g,
      shell,
      pos: g.position,
      vx: 0,
      vy: 0,
      vz: 0,
      life: 0,
      damage: 20,
      emberAccum: 0,
    });
  }

  function spawnFireball(ox, oy, oz, vx, vy, vz, damage = 20) {
    for (let i = 0; i < fireballs.length; i++) {
      const fb = fireballs[i];
      if (!fb.active) {
        fb.active = true;
        fb.group.visible = true;
        fb.pos.set(ox, oy, oz);
        fb.vx = vx;
        fb.vy = vy;
        fb.vz = vz;
        fb.life = 2.8;
        fb.damage = damage;
        fb.emberAccum = 0;
        return fb;
      }
    }
    return null;
  }

  // Shockwave Rings (Boss Attack 2 — Seismic Ground Slam)
  const SHOCKWAVE_POOL_SIZE = 4;
  const shockwaves = [];
  for (let i = 0; i < SHOCKWAVE_POOL_SIZE; i++) {
    const g = new THREE.Group();
    g.visible = false;
    const torusMat = new THREE.MeshBasicMaterial({
      color: 0xff5a18,
      transparent: true,
      opacity: 0.95,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });
    const torus = new THREE.Mesh(geo.shockwaveTorus, torusMat);
    torus.rotation.x = Math.PI * 0.5;
    torus.position.y = 0.22;

    const discMat = new THREE.MeshBasicMaterial({
      color: 0xff1e1e,
      transparent: true,
      opacity: 0.65,
      side: THREE.DoubleSide,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });
    const disc = new THREE.Mesh(geo.shockwaveDisc, discMat);
    disc.rotation.x = -Math.PI * 0.5;
    disc.position.y = 0.08;

    g.add(torus, disc);
    combatRoot.add(g);
    shockwaves.push({
      active: false,
      group: g,
      torusMat,
      discMat,
      origin: new THREE.Vector3(),
      radius: 1.5,
      maxRadius: 16.0,
      speed: 12.8,
      damage: 30,
      hitHero: false,
    });
  }

  function spawnShockwave(x, y, z, damage = 30) {
    for (let i = 0; i < shockwaves.length; i++) {
      const sw = shockwaves[i];
      if (!sw.active) {
        sw.active = true;
        sw.group.visible = true;
        sw.origin.set(x, y, z);
        sw.group.position.set(x, y, z);
        sw.radius = 1.5;
        sw.maxRadius = 16.0;
        sw.speed = 12.8;
        sw.damage = damage;
        sw.hitHero = false;
        sw.group.scale.set(sw.radius, 1, sw.radius);
        sw.torusMat.opacity = 0.95;
        sw.discMat.opacity = 0.68;
        return sw;
      }
    }
    return null;
  }

  // Crimson Slash Wave (Boss Attack 1 — Crimson Cleave)
  const SLASH_POOL_SIZE = 4;
  const slashWaves = [];
  for (let i = 0; i < SLASH_POOL_SIZE; i++) {
    const mat = new THREE.MeshBasicMaterial({
      color: 0xff2222,
      transparent: true,
      opacity: 0.9,
      side: THREE.DoubleSide,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });
    const mesh = new THREE.Mesh(geo.cleaveWave, mat);
    mesh.rotation.x = -Math.PI * 0.5;
    const holder = new THREE.Group();
    holder.visible = false;
    holder.add(mesh);
    combatRoot.add(holder);
    slashWaves.push({
      active: false,
      group: holder,
      mat,
      life: 0,
      maxLife: 0.38,
      dirX: 0,
      dirZ: 1,
    });
  }

  function spawnCleaveWave(x, y, z, yaw) {
    for (let i = 0; i < slashWaves.length; i++) {
      const sw = slashWaves[i];
      if (!sw.active) {
        sw.active = true;
        sw.group.visible = true;
        sw.group.position.set(x, y + 1.55, z);
        sw.group.rotation.set(0, yaw, 0);
        sw.group.scale.set(0.65, 1, 0.65);
        sw.life = sw.maxLife;
        sw.dirX = Math.sin(yaw);
        sw.dirZ = Math.cos(yaw);
        sw.mat.opacity = 0.92;
        return sw;
      }
    }
    return null;
  }

  // ==========================================================================
  // 4. LOOT SYSTEM (Spinning Gold Coins & Crimson Health Orbs, pooled!)
  // ==========================================================================
  const loot = [];
  const lootCoinPool = [];
  const lootOrbPool = [];
  const coinMat = new THREE.MeshStandardMaterial({
    color: 0xffd24c,
    emissive: 0xa86008,
    emissiveIntensity: 0.65,
    roughness: 0.22,
    metalness: 0.92,
  });
  const healthOrbMat = new THREE.MeshStandardMaterial({
    color: 0xff2a48,
    emissive: 0xd9102c,
    emissiveIntensity: 1.4,
    roughness: 0.18,
    metalness: 0.25,
  });

  function createCoinMesh() {
    const g = new THREE.Group();
    const disc = new THREE.Mesh(geo.coin, coinMat);
    disc.rotation.x = Math.PI * 0.5;
    const rim = new THREE.Mesh(geo.coinRim, coinMat);
    const halo = makeGlowSprite(textures, 0xffcc44, 1.05, 0.72);
    g.add(disc, rim, halo);
    g.visible = false;
    combatRoot.add(g);
    return g;
  }

  function createHealthOrbMesh() {
    const g = new THREE.Group();
    const orb = new THREE.Mesh(geo.healthOrb, healthOrbMat);
    const halo = makeGlowSprite(textures, 0xff2a44, 1.25, 0.85);
    g.add(orb, halo);
    g.visible = false;
    combatRoot.add(g);
    return g;
  }

  for (let i = 0; i < 28; i++) lootCoinPool.push(createCoinMesh());
  for (let i = 0; i < 10; i++) lootOrbPool.push(createHealthOrbMesh());

  function spawnLoot(type, x, y, z, value) {
    let group;
    if (type === 'gold') {
      group = lootCoinPool.length > 0 ? lootCoinPool.pop() : createCoinMesh();
    } else {
      group = lootOrbPool.length > 0 ? lootOrbPool.pop() : createHealthOrbMesh();
    }
    group.visible = true;
    group.position.set(x, y + 0.65, z);
    group.scale.set(1, 1, 1);
    const angle = Math.random() * Math.PI * 2;
    const speed = 1.8 + Math.random() * 2.8;
    loot.push({
      type,
      value,
      group,
      position: group.position,
      vx: Math.cos(angle) * speed,
      vy: 5.8 + Math.random() * 2.6,
      vz: Math.sin(angle) * speed,
      age: 0,
      grounded: false,
      spinOffset: Math.random() * Math.PI * 2,
    });
  }

  function recycleLoot(index) {
    const item = loot[index];
    item.group.visible = false;
    if (item.type === 'gold') lootCoinPool.push(item.group);
    else lootOrbPool.push(item.group);
    loot.splice(index, 1);
  }

  function updateLoot(dt, elapsed, hero, dragon) {
    if (loot.length === 0) return;
    const focus = dragon && dragon.mounted ? dragon.position : hero ? hero.position : null;

    for (let i = loot.length - 1; i >= 0; i--) {
      const item = loot[i];
      item.age += dt;
      item.group.rotation.y = elapsed * (item.type === 'gold' ? 3.8 : 2.2) + item.spinOffset;

      let magnetized = false;
      if (focus && item.age > 0.28) {
        const tx = focus.x;
        const ty = focus.y + 1.05;
        const tz = focus.z;
        const dx = tx - item.position.x;
        const dy = ty - item.position.y;
        const dz = tz - item.position.z;
        const distSq = dx * dx + dy * dy + dz * dz;
        const pullRadius = dragon && dragon.mounted ? 5.5 : 3.0;
        if (distSq < pullRadius * pullRadius) {
          magnetized = true;
          const dist = Math.sqrt(distSq) || 0.001;
          const pullSpeed = 13.5 + (pullRadius - dist) * 3.0;
          item.position.x += (dx / dist) * pullSpeed * dt;
          item.position.y += (dy / dist) * pullSpeed * dt;
          item.position.z += (dz / dist) * pullSpeed * dt;

          if (dist < 0.92) {
            if (item.type === 'gold') {
              if (hero) {
                if (typeof hero.addGold === 'function') hero.addGold(item.value);
                else if (typeof hero.gold === 'number') hero.gold += item.value;
              }
              if (sound && sound.playGold) sound.playGold();
              spawnDamageNumber(tx, ty + 0.9, tz, `+${item.value}g`, '#ffd859', 1.05);
              spawnBurst(tx, ty, tz, 6, 0xffd24c, 1.5, 4.5, 0.35, 0.75, 6);
            } else {
              if (hero) {
                if (typeof hero.heal === 'function') hero.heal(item.value);
                else if (typeof hero.hp === 'number') {
                  hero.hp = Math.min(hero.maxHp || 100, hero.hp + item.value);
                }
              }
              if (sound && sound.playGold) sound.playGold();
              spawnDamageNumber(tx, ty + 0.9, tz, `+${item.value} HP`, '#60ff9b', 1.05);
              spawnBurst(tx, ty, tz, 8, 0xff3b5c, 1.5, 4.5, 0.4, 0.8, 4);
            }
            recycleLoot(i);
            continue;
          }
        }
      }

      if (!magnetized) {
        const floorY = getWalkableHeight(item.position.x, item.position.z, item.position.y + 1.0) + 0.42;
        if (!item.grounded) {
          item.vy -= 18.0 * dt;
          item.position.x += item.vx * dt;
          item.position.y += item.vy * dt;
          item.position.z += item.vz * dt;
          item.vx *= Math.max(0, 1 - dt * 2.2);
          item.vz *= Math.max(0, 1 - dt * 2.2);
          if (item.position.y <= floorY) {
            item.position.y = floorY;
            if (Math.abs(item.vy) > 2.2) {
              item.vy = -item.vy * 0.34;
              item.vx *= 0.5;
              item.vz *= 0.5;
            } else {
              item.vy = 0;
              item.grounded = true;
            }
          }
        } else {
          item.position.y = floorY + Math.sin(elapsed * 3.6 + item.spinOffset) * 0.14;
        }
      }

      // Auto-despawn old uncollected loot after 55s
      if (item.age > 55) {
        recycleLoot(i);
      }
    }
  }

  // ==========================================================================
  // 5. PROCEDURAL ENEMY BUILDERS (Skeletons & Forest Trolls)
  // ==========================================================================
  function createSkeleton(id, name, spawnX, spawnZ, isCrypt) {
    const group = new THREE.Group();
    group.name = id;

    // Per-enemy materials for individual hit-flash, burn glow & death fade
    const boneMat = new THREE.MeshStandardMaterial({
      color: isCrypt ? 0xcac2b4 : 0xe0d7c3,
      roughness: 0.72,
      metalness: 0.08,
    });
    const darkBoneMat = new THREE.MeshStandardMaterial({
      color: 0x918775,
      roughness: 0.85,
      metalness: 0.06,
    });
    const rustIronMat = new THREE.MeshStandardMaterial({
      color: isCrypt ? 0x4b4f5a : 0x625a54,
      roughness: 0.52,
      metalness: 0.74,
    });
    const woodMat = new THREE.MeshStandardMaterial({
      color: 0x67442a,
      roughness: 0.86,
      metalness: 0.05,
    });
    const clothMat = new THREE.MeshStandardMaterial({
      color: isCrypt ? 0x381926 : 0x262834,
      roughness: 0.92,
    });
    const socketMat = new THREE.MeshBasicMaterial({ color: 0x0b0e14 });
    const eyeMat = new THREE.MeshBasicMaterial({ color: PALETTE.runeCyan });
    const fadeMats = [boneMat, darkBoneMat, rustIronMat, woodMat, clothMat];
    const flashMats = [boneMat, darkBoneMat, rustIronMat];

    // Hip / body pivot (allows bob, lean, and tumble death fall)
    const bodyPivot = new THREE.Group();
    bodyPivot.position.y = 0.88;
    group.add(bodyPivot);

    // Pelvis + tattered cloth
    const pelvis = new THREE.Mesh(geo.skelPelvis, boneMat);
    const cloth = new THREE.Mesh(geo.skelCloth, clothMat);
    bodyPivot.add(pelvis, cloth);

    // Upper torso pivot (single shadow-casting ribcage+spine torso mesh)
    const torso = new THREE.Group();
    torso.position.y = 0.08;
    bodyPivot.add(torso);

    const torsoBone = new THREE.Mesh(geo.skelTorsoBone, boneMat);
    torsoBone.castShadow = true;
    torso.add(torsoBone);

    // Left rusted pauldron
    const pauldron = new THREE.Mesh(geo.skelPauldron, rustIronMat);
    pauldron.position.set(-0.27, 0.48, 0);
    pauldron.rotation.z = 0.35;
    torso.add(pauldron);

    // Skull with dark recessed sockets & glowing cyan eyes
    const headGroup = new THREE.Group();
    headGroup.position.set(0, 0.64, 0.03);
    torso.add(headGroup);

    const skull = new THREE.Mesh(geo.skelSkull, boneMat);
    skull.scale.set(0.92, 1.08, 0.98);
    headGroup.add(skull);

    const jaw = new THREE.Mesh(geo.skelJaw, darkBoneMat);
    jaw.position.set(0, -0.13, 0.045);
    headGroup.add(jaw);

    const sockets = new THREE.Mesh(geo.skelSocket, socketMat);
    sockets.position.set(0, 0.01, 0.135);
    headGroup.add(sockets);

    const eyes = new THREE.Mesh(geo.skelEyes, eyeMat);
    headGroup.add(eyes);

    if (isCrypt) {
      const helm = new THREE.Mesh(geo.skelHelm, rustIronMat);
      helm.position.set(0, 0.07, 0);
      headGroup.add(helm);
    }

    // Right arm + Rusted Iron Sword
    const rightArm = new THREE.Group();
    rightArm.position.set(0.28, 0.48, 0);
    torso.add(rightArm);

    const rArmBone = new THREE.Mesh(geo.skelRightArmBone, boneMat);
    rightArm.add(rArmBone);

    const swordGroup = new THREE.Group();
    swordGroup.position.set(0, -0.62, 0.25);
    swordGroup.rotation.x = Math.PI * 0.42;
    const hilt = new THREE.Mesh(geo.skelHilt, woodMat);
    const swordIron = new THREE.Mesh(geo.skelSwordIron, rustIronMat);
    swordGroup.add(hilt, swordIron);
    rightArm.add(swordGroup);

    // Left arm + Octagonal Wooden Buckler
    const leftArm = new THREE.Group();
    leftArm.position.set(-0.28, 0.48, 0);
    torso.add(leftArm);

    const lArmBone = new THREE.Mesh(geo.skelLeftArmBone, boneMat);
    leftArm.add(lArmBone);

    const bucklerGroup = new THREE.Group();
    bucklerGroup.position.set(-0.08, -0.52, 0.28);
    bucklerGroup.rotation.y = -0.35;
    const bucklerFace = new THREE.Mesh(geo.skelBuckler, woodMat);
    bucklerFace.rotation.x = Math.PI * 0.5;
    const bucklerIron = new THREE.Mesh(geo.skelBucklerIron, rustIronMat);
    bucklerGroup.add(bucklerFace, bucklerIron);
    leftArm.add(bucklerGroup);

    // Legs (merged femur+knee+tibia+foot per leg)
    const leftLeg = new THREE.Group();
    leftLeg.position.set(-0.12, -0.04, 0);
    leftLeg.add(new THREE.Mesh(geo.skelLegBone, boneMat));

    const rightLeg = new THREE.Group();
    rightLeg.position.set(0.12, -0.04, 0);
    rightLeg.add(new THREE.Mesh(geo.skelLegBone, boneMat));

    bodyPivot.add(leftLeg, rightLeg);

    // Position safely on walkable ground
    findDrySpawnXZ(spawnX, spawnZ, _v1);
    group.position.copy(_v1);
    const startYaw = rng() * Math.PI * 2;
    group.rotation.y = startYaw;
    combatRoot.add(group);

    return {
      id,
      kind: 'skeleton',
      name,
      group,
      position: group.position,
      hp: 55,
      maxHp: 55,
      alive: true,
      aggro: false,
      height: 1.95,
      // Internal AI & combat state
      isCrypt,
      homePos: _v1.clone(),
      patrolTarget: _v1.clone(),
      patrolWait: 1.0 + rng() * 2.5,
      yaw: startYaw,
      radius: 0.58,
      speed: 4.8,
      aggroRange: 17,
      provokedRange: 28,
      provoked: false,
      attackRange: 2.3,
      damage: 11,
      state: 'patrol', // patrol | chase | windup | strike | recover | dead
      stateTimer: 0,
      cooldown: 0,
      hitApplied: false,
      knockVx: 0,
      knockVz: 0,
      hitFlash: 0,
      burnTimer: 0,
      burnTickDmg: 0,
      burnTickTimer: 0,
      deathTimer: 0,
      animPhase: rng() * Math.PI * 2,
      parts: { bodyPivot, torso, headGroup, rightArm, leftArm, leftLeg, rightLeg },
      flashMats,
      fadeMats,
      eyeMat,
    };
  }

  function createTroll(id, name, spawnX, spawnZ) {
    const group = new THREE.Group();
    group.name = id;

    // Per-enemy materials
    const skinMat = new THREE.MeshStandardMaterial({
      color: 0x4c5d4c,
      map: textures && textures.stoneMasonry ? textures.stoneMasonry : null,
      roughness: 0.86,
      metalness: 0.08,
    });
    const mossMat = new THREE.MeshStandardMaterial({
      color: 0x2d623b,
      roughness: 0.94,
      metalness: 0.02,
    });
    const bellyMat = new THREE.MeshStandardMaterial({
      color: 0x67725c,
      roughness: 0.9,
      metalness: 0.04,
    });
    const hornMat = new THREE.MeshStandardMaterial({
      color: 0xcab998,
      roughness: 0.62,
      metalness: 0.06,
    });
    const hideMat = new THREE.MeshStandardMaterial({
      color: 0x3b2a1c,
      roughness: 0.92,
    });
    const clubMat = new THREE.MeshStandardMaterial({
      color: 0x626670,
      map: textures && textures.stoneMasonry ? textures.stoneMasonry : null,
      roughness: 0.8,
      metalness: 0.16,
    });
    const spikeMat = new THREE.MeshStandardMaterial({
      color: 0x2a2e36,
      roughness: 0.44,
      metalness: 0.78,
    });
    const eyeMat = new THREE.MeshBasicMaterial({ color: 0xffaa22 });
    const fadeMats = [skinMat, mossMat, bellyMat, hornMat, hideMat, clubMat, spikeMat];
    const flashMats = [skinMat, mossMat, bellyMat, clubMat];

    // Body pivot at waist (~1.16m, total height ~2.9m)
    const bodyPivot = new THREE.Group();
    bodyPivot.position.y = 1.16;
    group.add(bodyPivot);

    const loin = new THREE.Mesh(geo.trollLoin, hideMat);
    bodyPivot.add(loin);

    const torso = new THREE.Group();
    torso.position.y = 0.25;
    bodyPivot.add(torso);

    // Broad muscular mossy-stone chest + hunched back hump (main torso shadow caster)
    const torsoSkin = new THREE.Mesh(geo.trollTorsoSkin, skinMat);
    torsoSkin.castShadow = true;
    torsoSkin.receiveShadow = true;
    torso.add(torsoSkin);

    const belly = new THREE.Mesh(geo.trollBelly, bellyMat);
    belly.position.set(0, 0.22, 0.22);
    belly.scale.set(1.06, 0.92, 0.85);
    torso.add(belly);

    const mossPads = new THREE.Mesh(geo.trollMossPads, mossMat);
    torso.add(mossPads);

    // Hunched head with tusks, curved horns & glowing amber eyes
    const headGroup = new THREE.Group();
    headGroup.position.set(0, 1.18, 0.56);
    torso.add(headGroup);

    const cranium = new THREE.Mesh(geo.trollHead, skinMat);
    cranium.scale.set(1.05, 0.96, 1.06);
    headGroup.add(cranium);

    const brow = new THREE.Mesh(geo.trollBrow, mossMat);
    brow.position.set(0, 0.14, 0.26);
    headGroup.add(brow);

    const jaw = new THREE.Mesh(geo.trollJaw, bellyMat);
    jaw.position.set(0, -0.18, 0.14);
    headGroup.add(jaw);

    const eyes = new THREE.Mesh(geo.trollEyes, eyeMat);
    const hornsTusks = new THREE.Mesh(geo.trollHornsTusks, hornMat);
    headGroup.add(eyes, hornsTusks);

    // Right arm + Heavy Spiked Stone Warclub
    const rightArm = new THREE.Group();
    rightArm.position.set(0.92, 0.96, 0.08);
    torso.add(rightArm);

    const rArmSkin = new THREE.Mesh(geo.trollRightArmSkin, skinMat);
    const rFist = new THREE.Mesh(geo.trollFist, bellyMat);
    rFist.position.set(0.08, -1.42, 0.26);
    rightArm.add(rArmSkin, rFist);

    // Heavy Spiked Stone Warclub (club head casts shadow)
    const clubGroup = new THREE.Group();
    clubGroup.position.set(0.08, -1.44, 0.32);
    clubGroup.rotation.x = Math.PI * 0.46;
    const clubShaft = new THREE.Mesh(geo.clubShaft, hideMat);
    clubShaft.position.y = 0.42;
    const clubHead = new THREE.Mesh(geo.clubHead, clubMat);
    clubHead.position.y = 1.32;
    clubHead.castShadow = true;
    const clubSpikes = new THREE.Mesh(geo.clubSpikes, spikeMat);
    clubGroup.add(clubShaft, clubHead, clubSpikes);
    rightArm.add(clubGroup);

    // Left arm
    const leftArm = new THREE.Group();
    leftArm.position.set(-0.92, 0.96, 0.08);
    torso.add(leftArm);

    const lUpper = new THREE.Mesh(geo.trollUpperArm, skinMat);
    lUpper.position.y = -0.35;
    lUpper.rotation.z = 0.12;
    const lFore = new THREE.Mesh(geo.trollForearm, mossMat);
    lFore.position.set(-0.08, -1.02, 0.14);
    lFore.rotation.x = -0.22;
    const lFist = new THREE.Mesh(geo.trollFist, bellyMat);
    lFist.position.set(-0.08, -1.42, 0.24);
    leftArm.add(lUpper, lFore, lFist);

    // Thick pillar legs (merged thigh+shin per leg)
    const leftLeg = new THREE.Group();
    leftLeg.position.set(-0.42, -0.05, 0);
    const lLegSkin = new THREE.Mesh(geo.trollLegSkin, skinMat);
    const lFoot = new THREE.Mesh(geo.trollFoot, bellyMat);
    lFoot.position.set(0, -1.06, 0.1);
    leftLeg.add(lLegSkin, lFoot);

    const rightLeg = new THREE.Group();
    rightLeg.position.set(0.42, -0.05, 0);
    const rLegSkin = new THREE.Mesh(geo.trollLegSkin, skinMat);
    const rFoot = new THREE.Mesh(geo.trollFoot, bellyMat);
    rFoot.position.set(0, -1.06, 0.1);
    rightLeg.add(rLegSkin, rFoot);

    bodyPivot.add(leftLeg, rightLeg);

    findDrySpawnXZ(spawnX, spawnZ, _v1);
    group.position.copy(_v1);
    const startYaw = rng() * Math.PI * 2;
    group.rotation.y = startYaw;
    combatRoot.add(group);

    return {
      id,
      kind: 'troll',
      name,
      group,
      position: group.position,
      hp: 135,
      maxHp: 135,
      alive: true,
      aggro: false,
      height: 2.95,
      // Internal state
      isCrypt: false,
      homePos: _v1.clone(),
      patrolTarget: _v1.clone(),
      patrolWait: 1.5 + rng() * 3.0,
      yaw: startYaw,
      radius: 1.05,
      speed: 3.7,
      aggroRange: 20,
      provokedRange: 30,
      provoked: false,
      attackRange: 3.1,
      damage: 22,
      state: 'patrol',
      stateTimer: 0,
      cooldown: 0,
      hitApplied: false,
      knockVx: 0,
      knockVz: 0,
      hitFlash: 0,
      burnTimer: 0,
      burnTickDmg: 0,
      burnTickTimer: 0,
      deathTimer: 0,
      animPhase: rng() * Math.PI * 2,
      parts: { bodyPivot, torso, headGroup, rightArm, leftArm, leftLeg, rightLeg, clubGroup },
      flashMats,
      fadeMats,
      eyeMat,
    };
  }

  // Spawn all 9 Skeletons & 4 Forest Trolls
  const enemies = [];
  const skeletonSpawns = [
    // 5 patrolling near Stone Bridge north bank & valley roads
    { id: 'skel_1', name: 'Bridge Skeleton', x: -12, z: -38, crypt: false },
    { id: 'skel_2', name: 'Road Skeleton', x: 14, z: -42, crypt: false },
    { id: 'skel_3', name: 'Shore Skeleton', x: -22, z: -26, crypt: false },
    { id: 'skel_4', name: 'Valley Skeleton', x: 24, z: -30, crypt: false },
    { id: 'skel_5', name: 'Highroad Skeleton', x: 0, z: -52, crypt: false },
    // 4 Crypt Skeletons inside the Dark Dungeon
    { id: 'skel_crypt_1', name: 'Crypt Skeleton', x: 114, z: -24, crypt: true },
    { id: 'skel_crypt_2', name: 'Crypt Skeleton', x: 128, z: -16, crypt: true },
    { id: 'skel_crypt_3', name: 'Crypt Skeleton', x: 136, z: -32, crypt: true },
    { id: 'skel_crypt_4', name: 'Crypt Warden', x: 148, z: -24, crypt: true },
  ];
  for (let i = 0; i < skeletonSpawns.length; i++) {
    const s = skeletonSpawns[i];
    enemies.push(createSkeleton(s.id, s.name, s.x, s.z, s.crypt));
  }

  const trollSpawns = [
    { id: 'troll_1', name: 'Mossback Troll', x: -58, z: -10 },
    { id: 'troll_2', name: 'Deepwood Troll', x: -72, z: -38 },
    { id: 'troll_3', name: 'Thornwood Troll', x: 62, z: -2 },
    { id: 'troll_4', name: 'Craghide Troll', x: 66, z: -46 },
  ];
  for (let i = 0; i < trollSpawns.length; i++) {
    const t = trollSpawns[i];
    enemies.push(createTroll(t.id, t.name, t.x, t.z));
  }

  // ==========================================================================
  // 6. CASTLE BOSS — MALAKOR, THE GRAVE SOVEREIGN (~4.8m Colossus)
  // ==========================================================================
  const BOSS_HOME_X = 0;
  const BOSS_HOME_Y = LANDMARKS.castle.floorY;
  const BOSS_HOME_Z = -116;

  function createBoss() {
    const group = new THREE.Group();
    group.name = 'Boss_Malakor';

    const obsidianMat = new THREE.MeshStandardMaterial({
      color: 0x1a1d27,
      map: textures && textures.stoneMasonry ? textures.stoneMasonry : null,
      roughness: 0.4,
      metalness: 0.78,
      emissive: _bossIdleEmissive.clone(),
      emissiveIntensity: 0.4,
    });
    const darkIronMat = new THREE.MeshStandardMaterial({
      color: 0x2a2f3d,
      roughness: 0.34,
      metalness: 0.88,
      emissive: _bossIdleEmissive.clone(),
      emissiveIntensity: 0.25,
    });
    const boneArmorMat = new THREE.MeshStandardMaterial({
      color: 0xcbc0ab,
      roughness: 0.65,
      metalness: 0.12,
      emissive: new THREE.Color(0x000000),
      emissiveIntensity: 0.0,
    });
    const furnaceMat = new THREE.MeshStandardMaterial({
      color: 0xff6e28,
      emissive: _bossFurnaceIdle.clone(),
      emissiveIntensity: 1.9,
      roughness: 0.18,
      metalness: 0.1,
    });
    const crownTipMat = new THREE.MeshStandardMaterial({
      color: 0x351418,
      roughness: 0.28,
      metalness: 0.72,
      emissive: _bossCrownIdle.clone(),
      emissiveIntensity: 0.65,
    });
    const hammerHeadMat = new THREE.MeshStandardMaterial({
      color: 0x262936,
      map: textures && textures.runeStone ? textures.runeStone : null,
      roughness: 0.34,
      metalness: 0.84,
      emissive: _bossHammerIdle.clone(),
      emissiveIntensity: 0.72,
    });
    const capeMat = new THREE.MeshStandardMaterial({
      color: 0x2b0b12,
      roughness: 0.9,
      side: THREE.DoubleSide,
    });
    const bossEyeMat = new THREE.MeshStandardMaterial({
      color: 0xff4518,
      emissive: _bossFurnaceIdle.clone(),
      emissiveIntensity: 1.55,
      roughness: 0.2,
      metalness: 0.05,
    });
    const fadeMats = [obsidianMat, darkIronMat, boneArmorMat, furnaceMat, crownTipMat, hammerHeadMat, capeMat, bossEyeMat];

    // Body pivot at hip (~1.88m)
    const bodyPivot = new THREE.Group();
    bodyPivot.position.y = 1.88;
    group.add(bodyPivot);

    // Armored hip tassets & girdle
    const waist = new THREE.Mesh(new THREE.CylinderGeometry(0.84, 0.72, 0.66, 8), obsidianMat);
    waist.castShadow = true;
    bodyPivot.add(waist);

    const _bTassetGeo = new THREE.BoxGeometry(0.46, 0.82, 0.14);
    const tassetsMerged = mergeGeos([
      { geo: _bTassetGeo, pos: [-0.48, -0.38, 0.62], rot: [-0.14, 0, 0] },
      { geo: _bTassetGeo, pos: [0, -0.38, 0.62], rot: [-0.14, 0, 0] },
      { geo: _bTassetGeo, pos: [0.48, -0.38, 0.62], rot: [-0.14, 0, 0] },
    ]);
    _bTassetGeo.dispose();
    bodyPivot.add(new THREE.Mesh(tassetsMerged, darkIronMat));

    // Upper torso
    const torso = new THREE.Group();
    torso.position.y = 0.34;
    bodyPivot.add(torso);

    // Broad V-tapered obsidian cuirass
    const cuirass = new THREE.Mesh(new THREE.CylinderGeometry(1.22, 0.76, 1.56, 8), obsidianMat);
    cuirass.position.y = 0.78;
    cuirass.scale.set(1.12, 1, 0.76);
    cuirass.castShadow = true;
    cuirass.receiveShadow = true;
    torso.add(cuirass);

    // Rib-cage bone plating framing the furnace chest core (merged 6->1)
    const _bRibGeo = new THREE.BoxGeometry(0.46, 0.11, 0.16);
    const _bRibParts = [];
    for (let r = 0; r < 3; r++) {
      const yRib = 0.56 + r * 0.28;
      _bRibParts.push(
        { geo: _bRibGeo, pos: [-0.56, yRib, 0.76], rot: [0, 0.35, -0.12] },
        { geo: _bRibGeo, pos: [0.56, yRib, 0.76], rot: [0, -0.35, 0.12] }
      );
    }
    const ribsMerged = mergeGeos(_bRibParts);
    _bRibGeo.dispose();
    torso.add(new THREE.Mesh(ribsMerged, boneArmorMat));

    // Glowing Furnace Chest Core + additive aura sprite
    const furnaceCore = new THREE.Mesh(new THREE.OctahedronGeometry(0.42, 0), furnaceMat);
    furnaceCore.position.set(0, 0.86, 0.62);
    furnaceCore.scale.set(0.9, 1.22, 0.65);
    torso.add(furnaceCore);

    const chestGlow = makeGlowSprite(textures, 0xff3b10, 1.85, 0.62);
    chestGlow.position.set(0, 0.86, 0.96);
    torso.add(chestGlow);

    // Tattered crimson-black warcloak
    const cape = new THREE.Mesh(new THREE.PlaneGeometry(2.15, 3.05, 2, 3), capeMat);
    cape.position.set(0, -0.05, -0.92);
    cape.rotation.x = 0.18;
    torso.add(cape);

    // Jagged Pauldrons (layered obsidian plates + curving bone spikes, merged per material)
    const _bPauldronBase = new THREE.DodecahedronGeometry(0.68, 0);
    const _bPauldronTrim = new THREE.BoxGeometry(0.86, 0.16, 0.88);
    const _bPauldronSpike = new THREE.ConeGeometry(0.14, 0.92, 6);
    const pauldronShells = new THREE.Mesh(
      mergeGeos([
        { geo: _bPauldronBase, pos: [-1.42, 1.54, 0.02], rot: [0, 0, 0.25], scale: [1.18, 0.78, 1.05] },
        { geo: _bPauldronBase, pos: [1.42, 1.54, 0.02], rot: [0, 0, -0.25], scale: [1.18, 0.78, 1.05] },
      ]),
      obsidianMat
    );
    pauldronShells.castShadow = true;
    const pauldronIron = new THREE.Mesh(
      mergeGeos([
        { geo: _bPauldronTrim, pos: [-1.54, 1.32, 0.02], rot: [0, 0, 0.3] },
        { geo: _bPauldronTrim, pos: [1.54, 1.32, 0.02], rot: [0, 0, -0.3] },
        { geo: _bPauldronSpike, pos: [-1.94, 1.78, -0.16], rot: [0, 0, 0.92], scale: [0.78, 0.78, 0.78] },
        { geo: _bPauldronSpike, pos: [1.94, 1.78, -0.16], rot: [0, 0, -0.92], scale: [0.78, 0.78, 0.78] },
      ]),
      darkIronMat
    );
    const pauldronBoneSpikes = new THREE.Mesh(
      mergeGeos([
        { geo: _bPauldronSpike, pos: [-1.76, 2.1, 0.1], rot: [0, 0, 0.52] },
        { geo: _bPauldronSpike, pos: [1.76, 2.1, 0.1], rot: [0, 0, -0.52] },
      ]),
      boneArmorMat
    );
    _bPauldronBase.dispose();
    _bPauldronTrim.dispose();
    _bPauldronSpike.dispose();
    torso.add(pauldronShells, pauldronIron, pauldronBoneSpikes);

    // Head + Skeletal Greathelm + Crown of Dark Iron Horns with Blazing Crimson Tips (~4.8m!)
    const headGroup = new THREE.Group();
    headGroup.position.set(0, 1.98, 0.14);
    torso.add(headGroup);

    const helm = new THREE.Mesh(new THREE.DodecahedronGeometry(0.46, 0), obsidianMat);
    helm.scale.set(0.95, 1.12, 1.04);
    helm.castShadow = true;
    headGroup.add(helm);

    const skullMask = new THREE.Mesh(new THREE.BoxGeometry(0.48, 0.42, 0.28), boneArmorMat);
    skullMask.position.set(0, -0.06, 0.32);
    headGroup.add(skullMask);

    const jawGrill = new THREE.Mesh(new THREE.BoxGeometry(0.38, 0.22, 0.26), darkIronMat);
    jawGrill.position.set(0, -0.28, 0.3);
    headGroup.add(jawGrill);

    const _bEyeGeo = new THREE.SphereGeometry(0.068, 6, 5);
    const bEyes = new THREE.Mesh(
      mergeGeos([
        { geo: _bEyeGeo, pos: [-0.14, 0.04, 0.46] },
        { geo: _bEyeGeo, pos: [0.14, 0.04, 0.46] },
      ]),
      bossEyeMat
    );
    _bEyeGeo.dispose();
    headGroup.add(bEyes);

    // Crown of Dark Iron Horns: dark forged bases + glowing crimson-scarlet tips & crown spires
    const _bHornShaftGeo = new THREE.CylinderGeometry(0.07, 0.15, 0.72, 6);
    const _bHornTipGeo = new THREE.ConeGeometry(0.076, 0.56, 6);
    const _bSpireGeo = new THREE.ConeGeometry(0.08, 0.64, 5);

    const hornMatL = new THREE.Matrix4().compose(
      new THREE.Vector3(-0.56, 0.54, -0.04),
      new THREE.Quaternion().setFromEuler(new THREE.Euler(-0.18, 0, 0.52)),
      new THREE.Vector3(1, 1, 1)
    );
    const hornMatR = new THREE.Matrix4().compose(
      new THREE.Vector3(0.56, 0.54, -0.04),
      new THREE.Quaternion().setFromEuler(new THREE.Euler(-0.18, 0, -0.52)),
      new THREE.Vector3(1, 1, 1)
    );
    const shiftShaft = new THREE.Matrix4().makeTranslation(0, -0.25, 0);
    const shiftTip = new THREE.Matrix4().makeTranslation(0, 0.36, 0);

    const greatHornBases = new THREE.Mesh(
      mergeGeos([
        { geo: _bHornShaftGeo, matrix: hornMatL.clone().multiply(shiftShaft) },
        { geo: _bHornShaftGeo, matrix: hornMatR.clone().multiply(shiftShaft) },
      ]),
      darkIronMat
    );
    const crownTips = new THREE.Mesh(
      mergeGeos([
        { geo: _bHornTipGeo, matrix: hornMatL.clone().multiply(shiftTip) },
        { geo: _bHornTipGeo, matrix: hornMatR.clone().multiply(shiftTip) },
        { geo: _bSpireGeo, pos: [-0.24, 0.58, 0.14], rot: [0, 0, 0.2] },
        { geo: _bSpireGeo, pos: [0, 0.72, 0.14], rot: [0, 0, 0] },
        { geo: _bSpireGeo, pos: [0.24, 0.58, 0.14], rot: [0, 0, -0.2] },
      ]),
      crownTipMat
    );
    _bHornShaftGeo.dispose();
    _bHornTipGeo.dispose();
    _bSpireGeo.dispose();
    headGroup.add(greatHornBases, crownTips);

    // Right Arm + Colossal Runed Warhammer
    const rightArm = new THREE.Group();
    rightArm.position.set(1.48, 1.32, 0.06);
    torso.add(rightArm);

    const _bArmUpperGeo = new THREE.CylinderGeometry(0.32, 0.26, 1.05, 7);
    const rUpper = new THREE.Mesh(_bArmUpperGeo, obsidianMat);
    rUpper.position.y = -0.46;
    const rFore = new THREE.Mesh(new THREE.CylinderGeometry(0.28, 0.34, 1.05, 7), darkIronMat);
    rFore.position.set(0.06, -1.32, 0.18);
    rFore.rotation.x = -0.34;
    rightArm.add(rUpper, rFore);

    // Colossal Runed Warhammer (~4.1m length!)
    const hammerGroup = new THREE.Group();
    hammerGroup.position.set(0.06, -1.78, 0.38);
    hammerGroup.rotation.x = Math.PI * 0.44;
    const _bHaftGeo = new THREE.CylinderGeometry(0.09, 0.075, 3.65, 7);
    const _bHSpikeGeo = new THREE.ConeGeometry(0.22, 0.68, 6);
    const hammerIron = new THREE.Mesh(
      mergeGeos([
        { geo: _bHaftGeo, pos: [0, 0.95, 0] },
        { geo: _bHSpikeGeo, pos: [0, 3.44, 0] },
      ]),
      darkIronMat
    );
    _bHaftGeo.dispose();
    _bHSpikeGeo.dispose();
    const pommel = new THREE.Mesh(new THREE.OctahedronGeometry(0.22, 0), boneArmorMat);
    pommel.position.y = -0.88;
    const hHead = new THREE.Mesh(new THREE.BoxGeometry(0.84, 0.88, 1.54), hammerHeadMat);
    hHead.position.y = 2.74;
    hHead.castShadow = true;
    const hBand = new THREE.Mesh(new THREE.BoxGeometry(0.92, 0.96, 0.38), obsidianMat);
    hBand.position.y = 2.74;
    const hGlow = makeGlowSprite(textures, 0xff2211, 2.1, 0.42);
    hGlow.position.y = 2.74;
    hammerGroup.add(hammerIron, pommel, hHead, hBand, hGlow);
    rightArm.add(hammerGroup);

    // Left Arm (armored claw gauntlet)
    const leftArm = new THREE.Group();
    leftArm.position.set(-1.48, 1.32, 0.06);
    torso.add(leftArm);

    const _bVambraceGeo = new THREE.BoxGeometry(0.22, 0.96, 0.58);
    const lArmObsidian = new THREE.Mesh(
      mergeGeos([
        { geo: _bArmUpperGeo, pos: [0, -0.46, 0] },
        { geo: _bVambraceGeo, pos: [-0.28, -1.32, 0.2] },
      ]),
      obsidianMat
    );
    _bVambraceGeo.dispose();
    const lFore = new THREE.Mesh(new THREE.CylinderGeometry(0.28, 0.36, 1.08, 7), darkIronMat);
    lFore.position.set(-0.06, -1.32, 0.16);
    lFore.rotation.x = -0.32;
    leftArm.add(lArmObsidian, lFore);

    // Towering Armored Legs (merged thigh+sabaton obsidian per leg)
    const _bLegThighGeo = new THREE.CylinderGeometry(0.36, 0.28, 0.95, 7);
    const _bSabatonGeo = new THREE.BoxGeometry(0.48, 0.26, 0.76);
    const legObsidianGeo = mergeGeos([
      { geo: _bLegThighGeo, pos: [0, -0.45, 0] },
      { geo: _bSabatonGeo, pos: [0, -1.72, 0.14] },
    ]);
    _bLegThighGeo.dispose();
    _bSabatonGeo.dispose();
    const legGreaveGeo = new THREE.CylinderGeometry(0.3, 0.36, 0.96, 7);
    const kneeGeo = new THREE.ConeGeometry(0.2, 0.34, 5);

    const leftLeg = new THREE.Group();
    leftLeg.position.set(-0.56, -0.05, 0);
    const lObs = new THREE.Mesh(legObsidianGeo, obsidianMat);
    lObs.castShadow = true;
    const lKn = new THREE.Mesh(kneeGeo, boneArmorMat);
    lKn.position.set(0, -0.92, 0.3);
    lKn.rotation.x = Math.PI * 0.5;
    const lGr = new THREE.Mesh(legGreaveGeo, darkIronMat);
    lGr.position.y = -1.34;
    leftLeg.add(lObs, lKn, lGr);

    const rightLeg = new THREE.Group();
    rightLeg.position.set(0.56, -0.05, 0);
    const rObs = new THREE.Mesh(legObsidianGeo, obsidianMat);
    rObs.castShadow = true;
    const rKn = new THREE.Mesh(kneeGeo, boneArmorMat);
    rKn.position.set(0, -0.92, 0.3);
    rKn.rotation.x = Math.PI * 0.5;
    const rGr = new THREE.Mesh(legGreaveGeo, darkIronMat);
    rGr.position.y = -1.34;
    rightLeg.add(rObs, rKn, rGr);

    bodyPivot.add(leftLeg, rightLeg);

    // Dynamic Red Telegraph PointLight on Boss (positioned cleanly forward so armor stays sculptural)
    const telegraphLight = new THREE.PointLight(0xd81812, 0, 22, 1.5);
    telegraphLight.position.set(0, 2.8, 1.35);
    group.add(telegraphLight);

    // Crisp Glowing Red Ground Danger Zone (`telegraphRing`)
    const telegraphRing = new THREE.Group();
    telegraphRing.visible = false;
    telegraphRing.position.set(BOSS_HOME_X, BOSS_HOME_Y + 0.09, BOSS_HOME_Z);

    const ringOuterMat = new THREE.MeshBasicMaterial({
      color: 0xff1e1e,
      transparent: true,
      opacity: 0.92,
      side: THREE.DoubleSide,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });
    const ringFillMat = new THREE.MeshBasicMaterial({
      color: 0xff1818,
      transparent: true,
      opacity: 0.28,
      side: THREE.DoubleSide,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });
    // Full circle (used for Slam & Barrage)
    const circleOuter = new THREE.Mesh(new THREE.RingGeometry(0.92, 1.0, 36), ringOuterMat);
    circleOuter.rotation.x = -Math.PI * 0.5;
    const circleFill = new THREE.Mesh(new THREE.CircleGeometry(1.0, 36), ringFillMat);
    circleFill.rotation.x = -Math.PI * 0.5;
    const circleProgress = new THREE.Mesh(new THREE.CircleGeometry(1.0, 36), ringFillMat);
    circleProgress.rotation.x = -Math.PI * 0.5;
    circleProgress.position.y = 0.02;
    const circleGroup = new THREE.Group();
    circleGroup.add(circleOuter, circleFill, circleProgress);

    // Cleave Arc Wedge (points along local +Z, matches range 6.2m)
    const cleaveHalfArc = 1.24; // ~142 deg cone
    const wedgeOuter = new THREE.Mesh(
      new THREE.RingGeometry(5.85, 6.25, 24, 1, -Math.PI * 0.5 - cleaveHalfArc, cleaveHalfArc * 2),
      ringOuterMat
    );
    wedgeOuter.rotation.x = -Math.PI * 0.5;
    const wedgeFill = new THREE.Mesh(
      new THREE.CircleGeometry(6.2, 24, -Math.PI * 0.5 - cleaveHalfArc, cleaveHalfArc * 2),
      ringFillMat
    );
    wedgeFill.rotation.x = -Math.PI * 0.5;
    const wedgeProgress = new THREE.Mesh(
      new THREE.CircleGeometry(6.2, 24, -Math.PI * 0.5 - cleaveHalfArc, cleaveHalfArc * 2),
      ringFillMat
    );
    wedgeProgress.rotation.x = -Math.PI * 0.5;
    wedgeProgress.position.y = 0.02;
    const cleaveGroup = new THREE.Group();
    cleaveGroup.add(wedgeOuter, wedgeFill, wedgeProgress);

    // 3 Barrage Aim Lanes (spread -16°, 0°, +16°)
    const laneGeo = new THREE.PlaneGeometry(0.55, 16.0);
    const barrageGroup = new THREE.Group();
    const laneAngles = [-0.28, 0, 0.28];
    for (let li = 0; li < 3; li++) {
      const laneHolder = new THREE.Group();
      laneHolder.rotation.y = laneAngles[li];
      const laneMesh = new THREE.Mesh(laneGeo, ringFillMat);
      laneMesh.rotation.x = -Math.PI * 0.5;
      laneMesh.position.z = 8.5;
      laneHolder.add(laneMesh);
      barrageGroup.add(laneHolder);
    }

    telegraphRing.add(circleGroup, cleaveGroup, barrageGroup);
    combatRoot.add(telegraphRing);

    group.position.set(BOSS_HOME_X, BOSS_HOME_Y, BOSS_HOME_Z);
    combatRoot.add(group);

    return {
      name: 'Malakor, the Grave Sovereign',
      group,
      position: group.position,
      hp: 460,
      maxHp: 460,
      alive: true,
      engaged: false,
      attackState: 'idle', // idle | chase | telegraph | attack | recover | dead
      telegraphRatio: 0,
      currentAttack: null, // 'cleave' | 'slam' | 'barrage'
      height: 4.8,
      // Internal boss state
      radius: 1.6,
      speed: 4.6,
      yaw: 0,
      stateTimer: 0,
      cooldown: 0.75,
      attackHitApplied: false,
      lastAttack: null,
      attackCycle: 0,
      hitFlash: 0,
      burnTimer: 0,
      burnTickDmg: 0,
      burnTickTimer: 0,
      deathTimer: 0,
      animPhase: 0,
      parts: {
        bodyPivot,
        torso,
        headGroup,
        rightArm,
        leftArm,
        leftLeg,
        rightLeg,
        hammerGroup,
        furnaceCore,
        chestGlow,
        hGlow,
        cape,
      },
      mats: {
        obsidianMat,
        darkIronMat,
        boneArmorMat,
        furnaceMat,
        crownTipMat,
        hammerHeadMat,
        bossEyeMat,
        fadeMats,
        ringOuterMat,
        ringFillMat,
      },
      telegraphLight,
      telegraphRing,
      ringSub: { circleGroup, circleProgress, cleaveGroup, wedgeProgress, barrageGroup },
    };
  }

  const boss = createBoss();

  // ==========================================================================
  // 7. SHARED DAMAGE HELPERS (Hero taking hits & Enemies/Boss taking hits)
  // ==========================================================================
  function isHeroDead(hero) {
    if (!hero) return true;
    return Boolean(hero.dead || hero.alive === false || (typeof hero.hp === 'number' && hero.hp <= 0));
  }

  function isHeroRolling(hero) {
    if (!hero) return false;
    return Boolean(hero.isRolling || hero.rolling || (typeof hero.rollTimer === 'number' && hero.rollTimer > 0));
  }

  function isHeroInvulnerable(hero) {
    if (!hero) return false;
    return isHeroRolling(hero) || (typeof hero.invulnTimer === 'number' && hero.invulnTimer > 0);
  }

  function applyDamageToHero(hero, dragon, amount, sourcePos, allowBlock = true) {
    if (isHeroDead(hero)) return false;

    const mounted = Boolean(dragon && dragon.mounted);
    const targetPos = mounted ? dragon.position : hero.position;

    // Q-roll i-frames cleanly dodge incoming attacks
    if (isHeroRolling(hero)) {
      spawnDamageNumber(targetPos.x, targetPos.y + 2.1, targetPos.z, 'DODGE', '#62f4ff', 0.75);
      return false;
    }
    // Brief post-hit invulnerability prevents overlapping multi-frame damage
    if (typeof hero.invulnTimer === 'number' && hero.invulnTimer > 0) {
      return false;
    }

    const scaledAmt = mounted ? Math.round(amount * 0.65) : amount;
    const hpBefore = typeof hero.hp === 'number' ? hero.hp : 100;

    if (typeof hero.takeDamage === 'function') {
      // Pass null knockback origin while mounted on dragon so seated rider isn't pushed
      const res = hero.takeDamage(scaledAmt, mounted ? null : sourcePos, sound);
      const dealt = res && typeof res.actualDamage === 'number'
        ? res.actualDamage
        : hpBefore - (typeof hero.hp === 'number' ? hero.hp : hpBefore);
      const parried = Boolean(res && res.parried);
      const blocked = Boolean(
        (res && res.blocked) ||
        res === 'blocked' ||
        (allowBlock && (hero.blocking || hero.isBlocking) && dealt < scaledAmt * 0.6)
      );

      if (parried) {
        spawnDamageNumber(targetPos.x, targetPos.y + 2.2, targetPos.z, 'PARRY!', '#ffe56b', 0.95);
        spawnBurst(targetPos.x, targetPos.y + 1.25, targetPos.z, 16, 0xffcc38, 3.0, 8.5, 0.4, 1.0, 8);
      } else if (blocked) {
        const label = dealt > 0.5 ? `BLOCK -${Math.round(dealt)}` : 'BLOCKED';
        spawnDamageNumber(targetPos.x, targetPos.y + 2.1, targetPos.z, label, '#cde6ff', 0.85);
        spawnBurst(targetPos.x, targetPos.y + 1.2, targetPos.z, 10, 0xffb444, 2.5, 6.5, 0.32, 0.8, 10);
      } else if (dealt > 0.5) {
        spawnDamageNumber(targetPos.x, targetPos.y + 2.1, targetPos.z, `-${Math.round(dealt)}`, '#ff4848', 0.95);
        spawnBurst(targetPos.x, targetPos.y + 1.2, targetPos.z, 9, 0xff2c2c, 2.2, 6.0, 0.35, 0.85, 12);
      }
      return dealt > 0;
    }

    // Fallback when hero has plain numeric HP without takeDamage()
    if (allowBlock && (hero.blocking || hero.isBlocking)) {
      if (sound && sound.playBlock) sound.playBlock();
      spawnDamageNumber(targetPos.x, targetPos.y + 2.1, targetPos.z, 'BLOCKED', '#cde6ff', 0.85);
      spawnBurst(targetPos.x, targetPos.y + 1.2, targetPos.z, 10, 0xffb444, 2.5, 6.5, 0.32, 0.8, 10);
      return false;
    }

    if (typeof hero.hp === 'number') {
      hero.hp = Math.max(0, hero.hp - scaledAmt);
      if (hero.hp <= 0) {
        if ('dead' in hero) hero.dead = true;
        if ('alive' in hero) hero.alive = false;
      }
    }
    if (sound && sound.playPlayerHurt) sound.playPlayerHurt();
    spawnDamageNumber(targetPos.x, targetPos.y + 2.1, targetPos.z, `-${Math.round(scaledAmt)}`, '#ff4848', 0.95);
    spawnBurst(targetPos.x, targetPos.y + 1.2, targetPos.z, 9, 0xff2c2c, 2.2, 6.0, 0.35, 0.85, 12);
    return true;
  }

  function killEnemy(enemy, hero, quests) {
    enemy.alive = false;
    enemy.hp = 0;
    enemy.state = 'dead';
    enemy.deathTimer = 0;
    enemy.aggro = false;
    if (hero && typeof hero.slainCount === 'number') hero.slainCount += 1;

    // Celebratory hit sparks
    spawnBurst(
      enemy.position.x,
      enemy.position.y + enemy.height * 0.5,
      enemy.position.z,
      enemy.kind === 'troll' ? 20 : 12,
      enemy.kind === 'troll' ? 0xffbb55 : 0x62e8ff,
      2.5,
      7.5,
      0.55,
      1.05,
      12
    );

    // Drop Gold Coins + 45% chance Crimson Health Orb
    if (enemy.kind === 'skeleton') {
      spawnLoot('gold', enemy.position.x, enemy.position.y, enemy.position.z, 12 + Math.floor(Math.random() * 8));
      if (Math.random() < 0.45) {
        spawnLoot('health', enemy.position.x, enemy.position.y, enemy.position.z, 22);
      }
    } else {
      for (let c = 0; c < 3; c++) {
        spawnLoot('gold', enemy.position.x, enemy.position.y, enemy.position.z, 14 + Math.floor(Math.random() * 7));
      }
      if (Math.random() < 0.45) {
        spawnLoot('health', enemy.position.x, enemy.position.y, enemy.position.z, 22);
      }
    }

    if (quests && quests.onEnemyKilled) {
      quests.onEnemyKilled(enemy.kind);
    }
  }

  function damageEnemy(enemy, amount, sourceX, sourceZ, knockback, isFire, hero, quests, heavy = false) {
    if (!enemy.alive) return;
    enemy.hp -= amount;
    enemy.aggro = true;
    enemy.provoked = true;

    if (isFire) {
      enemy.burnTimer = 0.48;
      enemy.burnTickDmg += amount;
    } else {
      enemy.hitFlash = 0.16;
      const dx = enemy.position.x - sourceX;
      const dz = enemy.position.z - sourceZ;
      const len = Math.hypot(dx, dz) || 1;
      const kbScale = enemy.kind === 'troll' ? 0.48 : 1.0;
      enemy.knockVx = (dx / len) * knockback * kbScale;
      enemy.knockVz = (dz / len) * knockback * kbScale;

      if (sound && sound.playHit) sound.playHit(heavy);
      spawnBurst(
        enemy.position.x,
        enemy.position.y + enemy.height * 0.58,
        enemy.position.z,
        heavy ? 14 : 9,
        heavy ? 0xffb824 : 0xffcc48,
        3.0,
        8.5,
        0.38,
        heavy ? 1.15 : 0.9,
        15
      );
      spawnDamageNumber(
        enemy.position.x,
        enemy.position.y + enemy.height + 0.3,
        enemy.position.z,
        `-${Math.round(amount)}`,
        heavy ? '#ffda55' : '#ffffff',
        0.95
      );
    }

    if (enemy.hp <= 0) {
      if (isFire && enemy.burnTickDmg >= 1) {
        spawnDamageNumber(
          enemy.position.x,
          enemy.position.y + enemy.height + 0.3,
          enemy.position.z,
          `-${Math.round(enemy.burnTickDmg)}`,
          '#ff922e',
          0.95
        );
        enemy.burnTickDmg = 0;
      }
      killEnemy(enemy, hero, quests);
    }
  }

  function killBoss(hero, quests) {
    boss.alive = false;
    boss.hp = 0;
    boss.engaged = false;
    boss.attackState = 'dead';
    boss.telegraphRatio = 0;
    boss.currentAttack = null;
    boss.deathTimer = 0;
    boss.telegraphRing.visible = false;
    boss.telegraphLight.color.setHex(0xff7722);
    boss.telegraphLight.intensity = 32;
    if (hero && typeof hero.slainCount === 'number') hero.slainCount += 1;

    // Clear active boss hazards cleanly on victory
    clearBossHazards();

    // Dramatic ember explosion
    spawnBurst(boss.position.x, boss.position.y + 2.4, boss.position.z, 55, 0xff4518, 4.0, 15.0, 1.15, 1.55, 10);
    spawnBurst(boss.position.x, boss.position.y + 2.8, boss.position.z, 30, 0xffd24c, 3.0, 12.0, 1.0, 1.3, 8);
    if (sound && sound.playHit) sound.playHit(true);

    // Fountain of 10 golden coins + 2 health orbs!
    for (let i = 0; i < 10; i++) {
      spawnLoot('gold', boss.position.x, boss.position.y + 1.2, boss.position.z, 25);
    }
    spawnLoot('health', boss.position.x, boss.position.y + 1.2, boss.position.z, 22);
    spawnLoot('health', boss.position.x, boss.position.y + 1.2, boss.position.z, 22);

    if (quests && quests.onBossKilled) {
      quests.onBossKilled();
    }
  }

  function damageBoss(amount, sourceX, sourceZ, isFire, hero, quests, heavy = false) {
    if (!boss.alive) return;
    boss.hp -= amount;
    if (!boss.engaged) {
      const srcDistArena = Math.hypot(sourceX - LANDMARKS.bossArena.x, sourceZ - LANDMARKS.bossArena.z);
      if (srcDistArena <= 54) {
        boss.engaged = true;
        boss.attackState = 'chase';
        boss.cooldown = 0.65;
      }
    }

    if (isFire) {
      boss.burnTimer = 0.45;
      boss.burnTickDmg += amount;
    } else {
      boss.hitFlash = 0.14;
      if (sound && sound.playHit) sound.playHit(heavy);
      spawnBurst(
        boss.position.x + (sourceX - boss.position.x) * 0.28,
        boss.position.y + 2.3,
        boss.position.z + (sourceZ - boss.position.z) * 0.28,
        heavy ? 16 : 10,
        heavy ? 0xffb824 : 0xffcc48,
        3.2,
        9.0,
        0.4,
        1.15,
        14
      );
      spawnDamageNumber(
        boss.position.x,
        boss.position.y + 4.3,
        boss.position.z,
        `-${Math.round(amount)}`,
        heavy ? '#ffda55' : '#ffffff',
        1.0
      );
    }

    if (boss.hp <= 0) {
      if (isFire && boss.burnTickDmg >= 1) {
        spawnDamageNumber(
          boss.position.x,
          boss.position.y + 4.3,
          boss.position.z,
          `-${Math.round(boss.burnTickDmg)}`,
          '#ff922e',
          1.0
        );
        boss.burnTickDmg = 0;
      }
      killBoss(hero, quests);
    }
  }

  // ==========================================================================
  // 8. HERO SWORD ARC & DRAGON FIRE BREATH HIT DETECTION
  // ==========================================================================
  const swingHitSet = new Set();
  let lastSwingToken = -1;
  let lastHeroFlagWasTrue = false;
  let fireSoundCooldown = 0;

  function processHeroAttack(hero, quests) {
    if (!hero || typeof hero.getAttackArc !== 'function') return;
    if (isHeroDead(hero)) return;

    const arc = hero.getAttackArc();
    const hasHitFlag = 'attackHitApplied' in hero;

    if (!arc || arc.active === false) {
      swingHitSet.clear();
      lastSwingToken = -1;
      lastHeroFlagWasTrue = false;
      return;
    }

    // Detect fresh swing cleanly across both flag-based and multi-frame arc APIs
    if (hasHitFlag && hero.attackHitApplied === false && lastHeroFlagWasTrue) {
      swingHitSet.clear();
      lastHeroFlagWasTrue = false;
    }
    if (arc.swingId !== undefined && arc.swingId !== lastSwingToken) {
      swingHitSet.clear();
      lastSwingToken = arc.swingId;
    }
    if (hasHitFlag && hero.attackHitApplied === true) {
      lastHeroFlagWasTrue = true;
      return;
    }

    const origin = arc.origin || arc.pos || arc.position || hero.position;
    const range = arc.range ?? arc.radius ?? 3.45;
    const damage = arc.damage ?? (24 * (hero.damageBonus || 1));
    const heavy = Boolean(arc.heavy || hero.attackCombo === 2 || (arc.combo >= 2) || damage >= 34);

    // Resolve attack forward vector
    const fwdObj = arc.forward || arc.dir || arc.direction;
    if (fwdObj && typeof fwdObj.x === 'number') {
      _v1.set(fwdObj.x, 0, fwdObj.z).normalize();
    } else {
      const yaw = arc.yaw ?? hero.yaw ?? (hero.group ? hero.group.rotation.y : 0);
      _v1.set(Math.sin(yaw), 0, Math.cos(yaw));
    }

    let minDot = 0.12;
    if (typeof arc.minDot === 'number') minDot = arc.minDot;
    else if (typeof arc.cosHalfAngle === 'number') minDot = arc.cosHalfAngle;
    else if (typeof arc.halfAngle === 'number') minDot = Math.cos(arc.halfAngle);
    else if (typeof arc.angle === 'number') {
      const rad = arc.angle > Math.PI * 2 ? THREE.MathUtils.degToRad(arc.angle) : arc.angle;
      minDot = Math.cos(rad > 1.3 ? rad * 0.5 : rad);
    }

    let hitSomething = false;

    // Test against all living enemies
    for (let i = 0; i < enemies.length; i++) {
      const e = enemies[i];
      if (!e.alive || swingHitSet.has(e)) continue;
      const dx = e.position.x - origin.x;
      const dz = e.position.z - origin.z;
      const dy = Math.abs((e.position.y + e.height * 0.4) - (origin.y + 0.9));
      if (dy > 3.1) continue;
      const dist = Math.hypot(dx, dz);
      const maxReach = range + e.radius;
      if (dist > maxReach) continue;
      const dot = dist > 0.001 ? (dx * _v1.x + dz * _v1.z) / dist : 1;
      if (dot >= minDot || dist < e.radius + 1.15) {
        swingHitSet.add(e);
        hitSomething = true;
        damageEnemy(e, damage, origin.x, origin.z, heavy ? 8.5 : 5.2, false, hero, quests, heavy);
      }
    }

    // Test against Castle Boss
    if (boss.alive && !swingHitSet.has(boss)) {
      const dx = boss.position.x - origin.x;
      const dz = boss.position.z - origin.z;
      const dy = Math.abs((boss.position.y + 1.6) - (origin.y + 0.9));
      if (dy <= 4.0) {
        const dist = Math.hypot(dx, dz);
        const maxReach = range + boss.radius;
        if (dist <= maxReach) {
          const dot = dist > 0.001 ? (dx * _v1.x + dz * _v1.z) / dist : 1;
          if (dot >= minDot || dist < boss.radius + 1.35) {
            swingHitSet.add(boss);
            hitSomething = true;
            damageBoss(damage, origin.x, origin.z, false, hero, quests, heavy);
          }
        }
      }
    }

    if (hitSomething && hasHitFlag) {
      hero.attackHitApplied = true;
      lastHeroFlagWasTrue = true;
    }
  }

  function isPointInFireVolume(tx, ty, tz, targetRadius, vol, origin, dir, hasDir, range) {
    const dx = tx - origin.x;
    const dy = ty - origin.y;
    const dz = tz - origin.z;

    if (!hasDir) {
      const r = (vol.radius ?? 8.5) + targetRadius;
      return dx * dx + dy * dy + dz * dz <= r * r;
    }

    const along = dx * dir.x + dy * dir.y + dz * dir.z;
    if (along < -1.0 || along > range + targetRadius) return false;
    const clampedAlong = Math.max(0, along);
    const distSq = dx * dx + dy * dy + dz * dz;
    const perpSq = Math.max(0, distSq - clampedAlong * clampedAlong);

    let coneRadius;
    if (typeof vol.halfAngle === 'number') {
      coneRadius = 1.3 + clampedAlong * Math.tan(vol.halfAngle);
    } else if (typeof vol.angle === 'number') {
      coneRadius = 1.3 + clampedAlong * Math.tan(vol.angle * 0.5);
    } else if (typeof vol.cosHalfAngle === 'number') {
      const c = Math.max(0.15, vol.cosHalfAngle);
      coneRadius = 1.3 + clampedAlong * Math.sqrt(Math.max(0, 1 - c * c)) / c;
    } else if (typeof vol.radius === 'number') {
      coneRadius = 1.3 + (clampedAlong / Math.max(1, range)) * vol.radius;
    } else {
      coneRadius = 1.6 + clampedAlong * 0.25;
    }
    const maxR = coneRadius + targetRadius;
    return perpSq <= maxR * maxR;
  }

  function processDragonFire(dt, hero, dragon, quests) {
    if (fireSoundCooldown > 0) fireSoundCooldown -= dt;
    if (!dragon || typeof dragon.getFireHitVolume !== 'function') return;

    const vol = dragon.getFireHitVolume();
    if (!vol || vol.active === false) return;

    const origSrc = vol.origin || vol.start || vol.center || vol.pos || vol.position || dragon.position;
    if (!origSrc) return;
    _v1.set(origSrc.x, origSrc.y, origSrc.z);

    const dirSrc = vol.dir || vol.direction || vol.forward;
    let hasDir = false;
    if (dirSrc && typeof dirSrc.x === 'number') {
      _v2.set(dirSrc.x, dirSrc.y || 0, dirSrc.z);
      if (_v2.lengthSq() > 0.0001) {
        _v2.normalize();
        hasDir = true;
      }
    }

    const range = vol.range ?? vol.length ?? vol.maxDist ?? 28;
    const dps = vol.dps ?? vol.damagePerSecond ?? 58;
    const frameDmg = dps * dt;
    let scorchedAny = false;

    for (let i = 0; i < enemies.length; i++) {
      const e = enemies[i];
      if (!e.alive) continue;
      if (isPointInFireVolume(e.position.x, e.position.y + e.height * 0.5, e.position.z, e.radius + 0.5, vol, _v1, _v2, hasDir, range)) {
        scorchedAny = true;
        damageEnemy(e, frameDmg, _v1.x, _v1.z, 0, true, hero, quests, false);
        if (Math.random() < dt * 14) {
          spawnSpark(
            e.position.x + (Math.random() - 0.5) * 0.8,
            e.position.y + 0.5 + Math.random() * e.height * 0.7,
            e.position.z + (Math.random() - 0.5) * 0.8,
            (Math.random() - 0.5) * 2.5,
            2.2 + Math.random() * 2.5,
            (Math.random() - 0.5) * 2.5,
            0xff6a14,
            0.35,
            1.1,
            2
          );
        }
      }
    }

    if (boss.alive) {
      if (isPointInFireVolume(boss.position.x, boss.position.y + 2.2, boss.position.z, boss.radius + 0.8, vol, _v1, _v2, hasDir, range)) {
        scorchedAny = true;
        damageBoss(frameDmg, _v1.x, _v1.z, true, hero, quests, false);
        if (Math.random() < dt * 20) {
          spawnSpark(
            boss.position.x + (Math.random() - 0.5) * 1.8,
            boss.position.y + 1.0 + Math.random() * 3.0,
            boss.position.z + (Math.random() - 0.5) * 1.8,
            (Math.random() - 0.5) * 3.2,
            2.6 + Math.random() * 3.2,
            (Math.random() - 0.5) * 3.2,
            0xff5a12,
            0.42,
            1.35,
            2
          );
        }
      }
    }

    if (scorchedAny && fireSoundCooldown <= 0) {
      fireSoundCooldown = 0.34;
      if (sound && sound.playHit) sound.playHit(false);
    }
  }

  // ==========================================================================
  // 9. ENEMY AI, ANIMATION, FLASH & DEATH FADE
  // ==========================================================================
  function clampEnemyGround(enemy, prevX, prevZ) {
    resolveWorldCollisions(enemy.position, enemy.radius);

    // Keep Crypt Skeletons inside the Ember Crypt chamber
    if (enemy.isCrypt) {
      const d = LANDMARKS.dungeonCenter;
      enemy.position.x = Math.max(d.minX + 2.5, Math.min(d.maxX - 3.0, enemy.position.x));
      enemy.position.z = Math.max(d.minZ + 2.5, Math.min(d.maxZ - 2.5, enemy.position.z));
      enemy.position.y = d.floorY;
      return;
    }

    const groundY = getWalkableHeight(enemy.position.x, enemy.position.z, enemy.position.y + 1.2);
    // Prevent outdoor enemies from walking into Mirror Lake
    if (groundY < WORLD.WATER_LEVEL + 0.15) {
      enemy.position.x = prevX;
      enemy.position.z = prevZ;
      enemy.position.y = Math.max(WORLD.WATER_LEVEL + 0.15, getWalkableHeight(prevX, prevZ, enemy.position.y + 1.2));
    } else {
      enemy.position.y = groundY;
    }
  }

  function updateEnemyFlashAndBurn(enemy, dt) {
    if (enemy.hitFlash > 0) enemy.hitFlash = Math.max(0, enemy.hitFlash - dt);
    if (enemy.burnTimer > 0) {
      enemy.burnTimer = Math.max(0, enemy.burnTimer - dt);
      enemy.burnTickTimer += dt;
      if (enemy.burnTickTimer >= 0.28 && enemy.burnTickDmg >= 1) {
        spawnDamageNumber(
          enemy.position.x,
          enemy.position.y + enemy.height + 0.35,
          enemy.position.z,
          `-${Math.round(enemy.burnTickDmg)}`,
          '#ff922e',
          0.85
        );
        enemy.burnTickDmg = 0;
        enemy.burnTickTimer = 0;
      }
    } else if (enemy.burnTickDmg >= 1) {
      spawnDamageNumber(
        enemy.position.x,
        enemy.position.y + enemy.height + 0.35,
        enemy.position.z,
        `-${Math.round(enemy.burnTickDmg)}`,
        '#ff922e',
        0.85
      );
      enemy.burnTickDmg = 0;
      enemy.burnTickTimer = 0;
    }

    const fT = enemy.hitFlash / 0.16;
    const bT = Math.min(1, enemy.burnTimer / 0.35);
    for (let m = 0; m < enemy.flashMats.length; m++) {
      const mat = enemy.flashMats[m];
      if (fT > 0.01) {
        mat.emissive.copy(_flashCol);
        mat.emissiveIntensity = fT * 1.45;
      } else if (bT > 0.01) {
        mat.emissive.copy(_burnCol);
        mat.emissiveIntensity = bT * 1.15;
      } else {
        mat.emissive.copy(_zeroCol);
        mat.emissiveIntensity = 0;
      }
    }
  }

  function updateEnemies(dt, elapsed, hero, dragon) {
    const heroAlive = !isHeroDead(hero);
    const targetPos = dragon && dragon.mounted ? dragon.position : hero ? hero.position : null;

    for (let i = 0; i < enemies.length; i++) {
      const e = enemies[i];

      // Handle dead tumble fall & fade out after 1.6s
      if (!e.alive) {
        if (!e.group.visible) continue;
        e.deathTimer += dt;
        const fallT = Math.min(1, e.deathTimer / 0.55);
        const easeFall = 1 - (1 - fallT) * (1 - fallT);
        e.parts.bodyPivot.rotation.x = -easeFall * Math.PI * 0.48;
        e.parts.bodyPivot.position.y = (e.kind === 'troll' ? 1.16 : 0.88) * (1 - easeFall * 0.72);
        e.parts.rightArm.rotation.x = easeFall * 0.8;
        e.parts.leftArm.rotation.z = easeFall * 0.6;

        if (e.deathTimer > 0.95) {
          const fade = Math.max(0, 1 - (e.deathTimer - 0.95) / 0.65);
          for (let m = 0; m < e.fadeMats.length; m++) {
            e.fadeMats[m].transparent = true;
            e.fadeMats[m].opacity = fade;
          }
          e.eyeMat.transparent = true;
          e.eyeMat.opacity = fade;
          if (e.deathTimer >= 1.6) {
            e.group.visible = false;
          }
        }
        continue;
      }

      updateEnemyFlashAndBurn(e, dt);

      // Apply decaying knockback velocity
      const prevX = e.position.x;
      const prevZ = e.position.z;
      if (Math.abs(e.knockVx) > 0.05 || Math.abs(e.knockVz) > 0.05) {
        e.position.x += e.knockVx * dt;
        e.position.z += e.knockVz * dt;
        const drag = Math.max(0, 1 - dt * 9.5);
        e.knockVx *= drag;
        e.knockVz *= drag;
      }

      if (e.cooldown > 0) e.cooldown -= dt;

      let distXZ = 999;
      let distY = 0;
      let toTargetX = 0;
      let toTargetZ = 1;
      if (heroAlive && targetPos) {
        toTargetX = targetPos.x - e.position.x;
        toTargetZ = targetPos.z - e.position.z;
        distXZ = Math.hypot(toTargetX, toTargetZ);
        distY = Math.abs(targetPos.y - e.position.y);
      }

      const distHome = Math.hypot(e.position.x - e.homePos.x, e.position.z - e.homePos.z);
      const effectiveAggro = e.provoked ? e.provokedRange : e.aggroRange;
      const canSeePlayer = heroAlive && distXZ <= effectiveAggro && distY < (e.isCrypt ? 8.5 : 13.0);

      // Aggro & Leash checks
      if (!e.aggro && canSeePlayer) {
        e.aggro = true;
        e.state = 'chase';
      } else if (e.aggro && (!heroAlive || distXZ > Math.max(effectiveAggro + 10, 34) || distHome > 44)) {
        e.aggro = false;
        e.provoked = false;
        e.state = 'patrol';
        e.patrolTarget.copy(e.homePos);
      }

      let moveSpeed = 0;
      const p = e.parts;
      const isTroll = e.kind === 'troll';
      const basePivotY = isTroll ? 1.16 : 0.88;

      if (e.state === 'patrol') {
        const pdx = e.patrolTarget.x - e.position.x;
        const pdz = e.patrolTarget.z - e.position.z;
        const pdist = Math.hypot(pdx, pdz);
        if (pdist < 1.1) {
          e.patrolWait -= dt;
          if (e.patrolWait <= 0) {
            const ang = Math.random() * Math.PI * 2;
            const rad = 3.0 + Math.random() * 5.5;
            e.patrolTarget.set(e.homePos.x + Math.cos(ang) * rad, e.homePos.y, e.homePos.z + Math.sin(ang) * rad);
            e.patrolWait = 1.6 + Math.random() * 2.8;
          }
        } else {
          const desiredYaw = Math.atan2(pdx, pdz);
          e.yaw = turnToward(e.yaw, desiredYaw, dt * 4.2);
          moveSpeed = e.speed * 0.42;
          e.position.x += (pdx / pdist) * moveSpeed * dt;
          e.position.z += (pdz / pdist) * moveSpeed * dt;
        }
      } else if (e.state === 'chase') {
        const desiredYaw = Math.atan2(toTargetX, toTargetZ);
        e.yaw = turnToward(e.yaw, desiredYaw, dt * (isTroll ? 5.2 : 7.5));

        if (distXZ <= e.attackRange && distY < 3.0 && e.cooldown <= 0) {
          e.state = 'windup';
          e.stateTimer = isTroll ? 0.72 : 0.36;
          e.hitApplied = false;
        } else if (distXZ > e.attackRange * 0.85) {
          moveSpeed = e.speed;
          const nx = toTargetX / (distXZ || 1);
          const nz = toTargetZ / (distXZ || 1);
          // Subtle separation from other living enemies so packs fan out cleanly
          let sepX = 0;
          let sepZ = 0;
          for (let j = 0; j < enemies.length; j++) {
            if (j === i || !enemies[j].alive) continue;
            const sx = e.position.x - enemies[j].position.x;
            const sz = e.position.z - enemies[j].position.z;
            const minSep = e.radius + enemies[j].radius + 0.55;
            const sSq = sx * sx + sz * sz;
            if (sSq < minSep * minSep && sSq > 0.001) {
              const sd = Math.sqrt(sSq);
              const push = (minSep - sd) / minSep;
              sepX += (sx / sd) * push * 2.2;
              sepZ += (sz / sd) * push * 2.2;
            }
          }
          e.position.x += (nx * moveSpeed + sepX) * dt;
          e.position.z += (nz * moveSpeed + sepZ) * dt;
        }
      } else if (e.state === 'windup') {
        // Track hero gently during early windup
        if (e.stateTimer > (isTroll ? 0.25 : 0.12)) {
          e.yaw = turnToward(e.yaw, Math.atan2(toTargetX, toTargetZ), dt * 4.0);
        }
        e.stateTimer -= dt;
        if (e.stateTimer <= 0) {
          e.state = 'strike';
          e.stateTimer = isTroll ? 0.24 : 0.22;
          e.hitApplied = false;
        }
      } else if (e.state === 'strike') {
        e.stateTimer -= dt;
        const hitMoment = isTroll ? 0.14 : 0.12;
        if (!e.hitApplied && e.stateTimer <= hitMoment) {
          e.hitApplied = true;
          if (isTroll) {
            // Stone warclub ground slam dust & heavy thud!
            const slamX = e.position.x + Math.sin(e.yaw) * 2.1;
            const slamZ = e.position.z + Math.cos(e.yaw) * 2.1;
            spawnBurst(slamX, e.position.y + 0.25, slamZ, 14, 0xd89c46, 2.2, 6.8, 0.48, 1.05, 14);
            if (sound && sound.playHit) sound.playHit(true);
          }
          if (heroAlive && distXZ <= e.attackRange + (isTroll ? 0.85 : 0.65) && distY < 2.9) {
            const dot = distXZ > 0.01 ? (toTargetX * Math.sin(e.yaw) + toTargetZ * Math.cos(e.yaw)) / distXZ : 1;
            if (dot > 0.05 || distXZ < e.radius + 1.0) {
              applyDamageToHero(hero, dragon, e.damage, e.position, true);
            }
          }
        }
        if (e.stateTimer <= 0) {
          e.state = 'recover';
          e.stateTimer = isTroll ? 0.82 : 0.56;
          e.cooldown = isTroll ? 1.15 : 0.72;
        }
      } else if (e.state === 'recover') {
        e.stateTimer -= dt;
        if (e.stateTimer <= 0) {
          e.state = e.aggro ? 'chase' : 'patrol';
        }
      }

      clampEnemyGround(e, prevX, prevZ);
      e.group.rotation.y = e.yaw;

      // Procedural Limb & Attack Animations
      e.animPhase += dt * (moveSpeed > 0.2 ? (isTroll ? 6.2 : 9.4) : 2.2);
      const stride = moveSpeed > 0.2 ? Math.sin(e.animPhase) : 0;
      const bob = moveSpeed > 0.2 ? Math.abs(Math.cos(e.animPhase)) * (isTroll ? 0.11 : 0.07) : Math.sin(elapsed * 2.4 + e.animPhase) * 0.025;

      p.bodyPivot.position.y = basePivotY + bob;
      p.bodyPivot.rotation.x = 0;
      p.leftLeg.rotation.x = stride * (isTroll ? 0.45 : 0.62);
      p.rightLeg.rotation.x = -stride * (isTroll ? 0.45 : 0.62);

      if (e.state === 'windup') {
        const wDur = isTroll ? 0.72 : 0.36;
        const t = 1 - Math.max(0, e.stateTimer / wDur);
        if (isTroll) {
          // Unmistakable towering overhead warclub raise!
          p.torso.rotation.x = -0.32 * t;
          p.torso.rotation.y = -0.25 * t;
          p.rightArm.rotation.x = -2.75 * t;
          p.rightArm.rotation.z = -0.22 * t;
          p.leftArm.rotation.x = -1.65 * t;
        } else {
          p.torso.rotation.y = -0.55 * t;
          p.torso.rotation.x = -0.1 * t;
          p.rightArm.rotation.x = -1.95 * t;
          p.rightArm.rotation.z = -0.35 * t;
          p.leftArm.rotation.x = -0.55;
        }
      } else if (e.state === 'strike') {
        const sDur = isTroll ? 0.24 : 0.22;
        const t = 1 - Math.max(0, e.stateTimer / sDur);
        if (isTroll) {
          // Violent overhead club slam down into ground
          p.torso.rotation.x = THREE.MathUtils.lerp(-0.32, 0.48, t);
          p.torso.rotation.y = THREE.MathUtils.lerp(-0.25, 0.22, t);
          p.rightArm.rotation.x = THREE.MathUtils.lerp(-2.75, 0.65, t);
          p.rightArm.rotation.z = THREE.MathUtils.lerp(-0.22, 0.08, t);
          p.leftArm.rotation.x = THREE.MathUtils.lerp(-1.65, 0.2, t);
        } else {
          p.torso.rotation.y = THREE.MathUtils.lerp(-0.55, 0.65, t);
          p.torso.rotation.x = THREE.MathUtils.lerp(-0.1, 0.2, t);
          p.rightArm.rotation.x = THREE.MathUtils.lerp(-1.95, 0.75, t);
          p.rightArm.rotation.z = THREE.MathUtils.lerp(-0.35, 0.35, t);
        }
      } else if (e.state === 'recover') {
        const rDur = isTroll ? 0.82 : 0.56;
        const t = 1 - Math.max(0, e.stateTimer / rDur);
        p.torso.rotation.x = THREE.MathUtils.lerp(isTroll ? 0.45 : 0.18, 0, t);
        p.torso.rotation.y = THREE.MathUtils.lerp(isTroll ? 0.2 : 0.55, 0, t);
        p.rightArm.rotation.x = THREE.MathUtils.lerp(0.65, -0.25, t);
        p.rightArm.rotation.z = THREE.MathUtils.lerp(0.2, 0, t);
        p.leftArm.rotation.x = THREE.MathUtils.lerp(0.2, -0.3, t);
      } else {
        p.torso.rotation.x = moveSpeed > 0.2 ? (isTroll ? 0.14 : 0.08) : 0;
        p.torso.rotation.y = stride * 0.14;
        p.rightArm.rotation.x = -stride * 0.48 - 0.25;
        p.rightArm.rotation.z = 0;
        p.leftArm.rotation.x = stride * 0.42 - (isTroll ? 0.15 : 0.45);
        p.leftArm.rotation.z = 0;
      }
    }
  }

  // ==========================================================================
  // 10. CASTLE BOSS AI, RED TELEGRAPHS & 3 SIGNATURE ATTACKS
  // ==========================================================================
  const TELEGRAPH_DURATION = 0.95;

  function chooseBossAttack(distXZ, targetHigh) {
    // If target is far or soaring on the Red Dragon, prioritize Abyssal Fire Volley
    if (targetHigh || distXZ > 11.5) {
      boss.lastAttack = 'barrage';
      return 'barrage';
    }
    // Medium range: alternate between Seismic Ground Slam & Abyssal Fire Volley
    if (distXZ > 6.4) {
      const next = boss.lastAttack === 'slam' ? 'barrage' : 'slam';
      boss.lastAttack = next;
      return next;
    }
    // Close range: cycle cleanly across all 3 attacks so player experiences every move
    const order = ['cleave', 'slam', 'barrage'];
    const pick = order[boss.attackCycle % 3];
    boss.attackCycle += 1;
    boss.lastAttack = pick;
    return pick;
  }

  function startBossTelegraph(attackType) {
    boss.attackState = 'telegraph';
    boss.currentAttack = attackType;
    boss.telegraphRatio = 0;
    boss.stateTimer = TELEGRAPH_DURATION;
    boss.attackHitApplied = false;

    if (sound && sound.playBossTelegraph) {
      sound.playBossTelegraph();
    }

    // Configure crisp red ground danger zone mesh
    boss.telegraphRing.visible = true;
    boss.telegraphRing.position.set(boss.position.x, LANDMARKS.castle.floorY + 0.09, boss.position.z);
    boss.telegraphRing.rotation.y = boss.yaw;
    boss.ringSub.circleGroup.visible = attackType === 'slam' || attackType === 'barrage';
    boss.ringSub.cleaveGroup.visible = attackType === 'cleave';
    boss.ringSub.barrageGroup.visible = attackType === 'barrage';
  }

  function updateBossVisualGlow(dt, elapsed) {
    if (boss.hitFlash > 0) boss.hitFlash = Math.max(0, boss.hitFlash - dt);
    if (boss.burnTimer > 0) {
      boss.burnTimer = Math.max(0, boss.burnTimer - dt);
      boss.burnTickTimer += dt;
      if (boss.burnTickTimer >= 0.3 && boss.burnTickDmg >= 1) {
        spawnDamageNumber(boss.position.x, boss.position.y + 4.4, boss.position.z, `-${Math.round(boss.burnTickDmg)}`, '#ff922e', 0.95);
        boss.burnTickDmg = 0;
        boss.burnTickTimer = 0;
      }
    } else if (boss.burnTickDmg >= 1) {
      spawnDamageNumber(boss.position.x, boss.position.y + 4.4, boss.position.z, `-${Math.round(boss.burnTickDmg)}`, '#ff922e', 0.95);
      boss.burnTickDmg = 0;
      boss.burnTickTimer = 0;
    }

    const m = boss.mats;
    const isTelegraph = boss.attackState === 'telegraph';
    const isAttack = boss.attackState === 'attack';
    const pulse = 0.5 + 0.5 * Math.sin(elapsed * 18.0);
    const flashT = boss.hitFlash / 0.14;
    const burnT = Math.min(1, boss.burnTimer / 0.35);
    const idleBreath = 0.85 + 0.15 * Math.sin(elapsed * 2.8);

    if (isTelegraph || isAttack) {
      // Refined crimson telegraph: keep obsidian armor sculptural & fierce (<= 0.95 deep crimson)
      // while furnace core, eyes, horned crown tips & warhammer head blaze hot scarlet (<= 2.4)
      const charge = isTelegraph ? 0.3 + boss.telegraphRatio * 0.58 + pulse * 0.12 : 1.0;

      m.obsidianMat.emissive.copy(_deepCrimsonCol);
      m.obsidianMat.emissiveIntensity = Math.min(0.95, 0.28 + charge * 0.67);
      m.darkIronMat.emissive.copy(_deepCrimsonCol);
      m.darkIronMat.emissiveIntensity = Math.min(0.75, 0.2 + charge * 0.55);
      m.boneArmorMat.emissive.copy(_deepCrimsonCol);
      m.boneArmorMat.emissiveIntensity = Math.min(0.32, 0.08 + charge * 0.24);

      m.furnaceMat.emissive.copy(_scarletBlazeCol);
      m.furnaceMat.emissiveIntensity = 1.45 + charge * 0.95;
      m.crownTipMat.emissive.copy(_scarletBlazeCol);
      m.crownTipMat.emissiveIntensity = 1.25 + charge * 1.15;
      m.hammerHeadMat.emissive.copy(_scarletBlazeCol);
      m.hammerHeadMat.emissiveIntensity = 1.25 + charge * 1.15;
      m.bossEyeMat.color.setHex(0xff3b22);
      m.bossEyeMat.emissive.copy(_scarletBlazeCol);
      m.bossEyeMat.emissiveIntensity = 1.5 + charge * 0.9;

      boss.telegraphLight.color.setHex(0xd81812);
      boss.telegraphLight.intensity = isTelegraph ? 3.5 + boss.telegraphRatio * 8.5 + pulse * 2.0 : 12.0;

      boss.parts.hGlow.material.color.setHex(0xff2211);
      boss.parts.hGlow.material.opacity = Math.min(0.86, 0.44 + charge * 0.42);
      const hScale = 2.1 + charge * 1.05;
      boss.parts.hGlow.scale.set(hScale, hScale, 1);

      boss.parts.chestGlow.material.color.setHex(0xff2211);
      boss.parts.chestGlow.material.opacity = Math.min(0.8, 0.44 + charge * 0.36);
      const cScale = 1.8 + charge * 0.78;
      boss.parts.chestGlow.scale.set(cScale, cScale, 1);
    } else if (flashT > 0.01) {
      m.obsidianMat.emissive.copy(_flashCol);
      m.obsidianMat.emissiveIntensity = flashT * 1.1;
      m.darkIronMat.emissive.copy(_flashCol);
      m.darkIronMat.emissiveIntensity = flashT * 1.0;
      m.boneArmorMat.emissive.copy(_flashCol);
      m.boneArmorMat.emissiveIntensity = flashT * 0.85;
      m.hammerHeadMat.emissive.copy(_bossHammerIdle);
      m.hammerHeadMat.emissiveIntensity = 0.75;
      m.furnaceMat.emissive.copy(_bossFurnaceIdle);
      m.furnaceMat.emissiveIntensity = 1.9 * idleBreath;
      m.crownTipMat.emissive.copy(_bossCrownIdle);
      m.crownTipMat.emissiveIntensity = 0.65 * idleBreath;
      m.bossEyeMat.color.setHex(0xff4c18);
      m.bossEyeMat.emissive.copy(_bossFurnaceIdle);
      m.bossEyeMat.emissiveIntensity = 1.55 * idleBreath;
      boss.telegraphLight.intensity = Math.max(0, boss.telegraphLight.intensity - dt * 45);
    } else if (burnT > 0.01) {
      m.obsidianMat.emissive.copy(_burnCol);
      m.obsidianMat.emissiveIntensity = burnT * 0.85;
      m.darkIronMat.emissive.copy(_burnCol);
      m.darkIronMat.emissiveIntensity = burnT * 0.7;
      m.boneArmorMat.emissive.copy(_burnCol);
      m.boneArmorMat.emissiveIntensity = burnT * 0.45;
      m.hammerHeadMat.emissive.copy(_bossHammerIdle);
      m.hammerHeadMat.emissiveIntensity = 0.85;
      m.furnaceMat.emissive.copy(_bossFurnaceIdle);
      m.furnaceMat.emissiveIntensity = 2.0 * idleBreath;
      m.crownTipMat.emissive.copy(_bossCrownIdle);
      m.crownTipMat.emissiveIntensity = 0.75 * idleBreath;
      m.bossEyeMat.color.setHex(0xff4c18);
      m.bossEyeMat.emissive.copy(_bossFurnaceIdle);
      m.bossEyeMat.emissiveIntensity = 1.65 * idleBreath;
      boss.telegraphLight.intensity = Math.max(0, boss.telegraphLight.intensity - dt * 30);
    } else {
      m.obsidianMat.emissive.copy(_bossIdleEmissive);
      m.obsidianMat.emissiveIntensity = 0.4;
      m.darkIronMat.emissive.copy(_bossIdleEmissive);
      m.darkIronMat.emissiveIntensity = 0.25;
      m.boneArmorMat.emissive.copy(_zeroCol);
      m.boneArmorMat.emissiveIntensity = 0;
      m.hammerHeadMat.emissive.copy(_bossHammerIdle);
      m.hammerHeadMat.emissiveIntensity = 0.72 * idleBreath;
      m.furnaceMat.emissive.copy(_bossFurnaceIdle);
      m.furnaceMat.emissiveIntensity = 1.9 * idleBreath;
      m.crownTipMat.emissive.copy(_bossCrownIdle);
      m.crownTipMat.emissiveIntensity = 0.65 * idleBreath;
      m.bossEyeMat.color.setHex(0xff4c18);
      m.bossEyeMat.emissive.copy(_bossFurnaceIdle);
      m.bossEyeMat.emissiveIntensity = 1.55 * idleBreath;
      boss.telegraphLight.intensity = Math.max(0, boss.telegraphLight.intensity - dt * 36);
      boss.parts.hGlow.material.color.setHex(0xff2211);
      boss.parts.hGlow.material.opacity = 0.4;
      boss.parts.hGlow.scale.set(2.1, 2.1, 1);
      boss.parts.chestGlow.material.color.setHex(0xff3b10);
      boss.parts.chestGlow.material.opacity = 0.62;
      boss.parts.chestGlow.scale.set(1.85 * idleBreath, 1.85 * idleBreath, 1);
    }
  }

  function updateBoss(dt, elapsed, hero, dragon) {
    if (!boss.alive) {
      if (boss.telegraphLight.intensity > 0.05) {
        boss.telegraphLight.intensity = Math.max(0, boss.telegraphLight.intensity - dt * 26);
      }
      if (!boss.group.visible) return;
      boss.deathTimer += dt;
      const fallT = Math.min(1, boss.deathTimer / 0.95);
      const ease = 1 - (1 - fallT) * (1 - fallT);
      boss.parts.bodyPivot.rotation.x = -ease * Math.PI * 0.48;
      boss.parts.bodyPivot.position.y = 1.88 * (1 - ease * 0.68);
      boss.parts.rightArm.rotation.x = ease * 0.7;
      boss.parts.leftArm.rotation.z = ease * 0.65;
      if (boss.deathTimer > 1.8) {
        const fade = Math.max(0, 1 - (boss.deathTimer - 1.8) / 1.1);
        for (let m = 0; m < boss.mats.fadeMats.length; m++) {
          boss.mats.fadeMats[m].transparent = true;
          boss.mats.fadeMats[m].opacity = fade;
        }
        boss.parts.chestGlow.material.opacity = fade * 0.62;
        boss.parts.hGlow.material.opacity = fade * 0.42;
        if (boss.deathTimer >= 2.95) {
          boss.group.visible = false;
        }
      }
      return;
    }

    const heroAlive = !isHeroDead(hero);
    const mounted = Boolean(dragon && dragon.mounted);
    const focusPos = mounted ? dragon.position : hero ? hero.position : null;
    const arenaDistXZ = focusPos
      ? Math.hypot(focusPos.x - LANDMARKS.bossArena.x, focusPos.z - LANDMARKS.bossArena.z)
      : 999;

    // Auto-disengage when player leaves Highkeep Castle (> 54m from bossArena)
    if (boss.engaged && (!focusPos || arenaDistXZ > 54)) {
      boss.engaged = false;
      boss.attackState = 'idle';
      boss.telegraphRatio = 0;
      boss.currentAttack = null;
      boss.stateTimer = 0;
      boss.cooldown = 0.65;
      boss.telegraphRing.visible = false;
    }

    // Engage when hero enters Highkeep Castle courtyard (`z < -81 && |x| < 29`)
    if (!boss.engaged && heroAlive && hero) {
      if (
        (!mounted && hero.position.z < -81 && Math.abs(hero.position.x) < 29) ||
        (mounted && dragon.position.z < -81 && Math.abs(dragon.position.x) < 29 && dragon.position.y < 42)
      ) {
        boss.engaged = true;
        boss.attackState = 'chase';
        boss.cooldown = 0.65;
        if (sound && sound.playBossTelegraph) sound.playBossTelegraph();
      }
    }

    updateBossVisualGlow(dt, elapsed);

    const p = boss.parts;
    let moveSpeed = 0;

    if (!boss.engaged || !heroAlive || !focusPos) {
      boss.attackState = 'idle';
      boss.telegraphRatio = 0;
      boss.currentAttack = null;
      boss.telegraphRing.visible = false;

      // Smoothly walk Malakor back toward courtyard anchor (0, -116) when disengaged
      const homeDx = BOSS_HOME_X - boss.position.x;
      const homeDz = BOSS_HOME_Z - boss.position.z;
      const homeDist = Math.hypot(homeDx, homeDz);

      if (homeDist > 0.35) {
        const returnSpeed = boss.speed * 0.72;
        const stepDist = Math.min(homeDist, returnSpeed * dt);
        boss.position.x = Math.max(-24, Math.min(24, boss.position.x + (homeDx / homeDist) * stepDist));
        boss.position.z = Math.max(-133, Math.min(-85, boss.position.z + (homeDz / homeDist) * stepDist));
        boss.position.y = BOSS_HOME_Y;
        boss.yaw = turnToward(boss.yaw, Math.atan2(homeDx, homeDz), dt * 4.0);
        boss.group.rotation.y = boss.yaw;

        boss.animPhase += dt * 4.6;
        const returnStride = Math.sin(boss.animPhase);
        p.bodyPivot.position.y = 1.88 + Math.abs(Math.cos(boss.animPhase)) * 0.1;
        p.torso.rotation.x = THREE.MathUtils.lerp(p.torso.rotation.x, 0.06, dt * 6);
        p.torso.rotation.y = returnStride * 0.1;
        p.torso.rotation.z = 0;
        p.rightArm.rotation.set(
          THREE.MathUtils.lerp(p.rightArm.rotation.x, -returnStride * 0.3 - 0.22, dt * 6),
          THREE.MathUtils.lerp(p.rightArm.rotation.y, 0, dt * 6),
          THREE.MathUtils.lerp(p.rightArm.rotation.z, 0, dt * 6)
        );
        p.leftArm.rotation.set(
          THREE.MathUtils.lerp(p.leftArm.rotation.x, returnStride * 0.3 - 0.18, dt * 6),
          0,
          THREE.MathUtils.lerp(p.leftArm.rotation.z, 0, dt * 6)
        );
        p.leftLeg.rotation.x = returnStride * 0.38;
        p.rightLeg.rotation.x = -returnStride * 0.38;
        p.cape.rotation.x = 0.24 + Math.sin(elapsed * 2.8) * 0.06;
      } else {
        boss.position.set(BOSS_HOME_X, BOSS_HOME_Y, BOSS_HOME_Z);
        boss.yaw = turnToward(boss.yaw, 0, dt * 3.0);
        boss.group.rotation.y = boss.yaw;

        p.bodyPivot.position.y = 1.88 + Math.sin(elapsed * 1.9) * 0.05;
        p.torso.rotation.x = THREE.MathUtils.lerp(p.torso.rotation.x, 0, dt * 6);
        p.torso.rotation.y = THREE.MathUtils.lerp(p.torso.rotation.y, 0, dt * 6);
        p.torso.rotation.z = 0;
        p.rightArm.rotation.set(
          THREE.MathUtils.lerp(p.rightArm.rotation.x, -0.22, dt * 6),
          THREE.MathUtils.lerp(p.rightArm.rotation.y, 0, dt * 6),
          THREE.MathUtils.lerp(p.rightArm.rotation.z, 0, dt * 6)
        );
        p.leftArm.rotation.set(
          THREE.MathUtils.lerp(p.leftArm.rotation.x, -0.18, dt * 6),
          0,
          THREE.MathUtils.lerp(p.leftArm.rotation.z, 0, dt * 6)
        );
        p.leftLeg.rotation.x = THREE.MathUtils.lerp(p.leftLeg.rotation.x, 0, dt * 6);
        p.rightLeg.rotation.x = THREE.MathUtils.lerp(p.rightLeg.rotation.x, 0, dt * 6);
        p.cape.rotation.x = 0.18 + Math.sin(elapsed * 2.2) * 0.05;
      }
      return;
    }

    const toX = focusPos.x - boss.position.x;
    const toZ = focusPos.z - boss.position.z;
    const distXZ = Math.hypot(toX, toZ);
    const targetHigh = focusPos.y > LANDMARKS.castle.floorY + 4.5;
    const phaseTwo = boss.hp <= boss.maxHp * 0.5;

    if (boss.cooldown > 0) boss.cooldown -= dt;

    if (boss.attackState === 'idle' || boss.attackState === 'chase') {
      boss.attackState = 'chase';
      boss.telegraphRatio = 0;
      boss.telegraphRing.visible = false;

      const desiredYaw = Math.atan2(toX, toZ);
      boss.yaw = turnToward(boss.yaw, desiredYaw, dt * 4.4);

      if (boss.cooldown <= 0 && (distXZ <= 24 || targetHigh)) {
        const chosen = chooseBossAttack(distXZ, targetHigh);
        startBossTelegraph(chosen);
      } else if (distXZ > 4.6) {
        moveSpeed = boss.speed * (phaseTwo ? 1.12 : 1.0);
        boss.position.x += (toX / (distXZ || 1)) * moveSpeed * dt;
        boss.position.z += (toZ / (distXZ || 1)) * moveSpeed * dt;
      }
    } else if (boss.attackState === 'telegraph') {
      boss.stateTimer -= dt;
      boss.telegraphRatio = Math.max(0, Math.min(1, 1 - boss.stateTimer / TELEGRAPH_DURATION));

      // Track target gently during first 58% of telegraph, then commit so player can dodge!
      if (boss.telegraphRatio < 0.58) {
        boss.yaw = turnToward(boss.yaw, Math.atan2(toX, toZ), dt * 4.8);
      }

      // Update glowing red danger zone under boss
      boss.telegraphRing.visible = true;
      boss.telegraphRing.position.set(boss.position.x, LANDMARKS.castle.floorY + 0.09, boss.position.z);
      boss.telegraphRing.rotation.y = boss.yaw;
      const tr = boss.telegraphRatio;
      boss.mats.ringOuterMat.opacity = 0.55 + tr * 0.45;
      boss.mats.ringFillMat.opacity = 0.16 + tr * 0.34;

      if (boss.currentAttack === 'cleave') {
        const s = 0.25 + tr * 0.75;
        boss.ringSub.wedgeProgress.scale.set(s, s, 1);
      } else if (boss.currentAttack === 'slam') {
        const ringR = 2.6 + tr * 5.2;
        boss.ringSub.circleGroup.scale.set(ringR, 1, ringR);
        boss.ringSub.circleProgress.scale.set(tr, tr, 1);
      } else {
        const ringR = 2.8 + tr * 1.4;
        boss.ringSub.circleGroup.scale.set(ringR, 1, ringR);
        boss.ringSub.circleProgress.scale.set(tr, tr, 1);
      }

      // Emit crackling crimson sparks from warhammer / chest core during telegraph!
      if (Math.random() < dt * 26) {
        spawnSpark(
          boss.position.x + (Math.random() - 0.5) * 2.2,
          boss.position.y + 1.8 + Math.random() * 2.2,
          boss.position.z + (Math.random() - 0.5) * 2.2,
          (Math.random() - 0.5) * 3.0,
          1.5 + Math.random() * 3.0,
          (Math.random() - 0.5) * 3.0,
          0xff1e1e,
          0.32,
          1.25,
          4
        );
      }

      if (boss.stateTimer <= 0) {
        boss.attackState = 'attack';
        boss.telegraphRatio = 1;
        boss.telegraphRing.visible = false;
        boss.stateTimer = boss.currentAttack === 'cleave' ? 0.34 : boss.currentAttack === 'slam' ? 0.38 : 0.36;
        boss.attackHitApplied = false;
      }
    } else if (boss.attackState === 'attack') {
      boss.stateTimer -= dt;
      const triggerTime = boss.currentAttack === 'cleave' ? 0.22 : boss.currentAttack === 'slam' ? 0.24 : 0.25;

      if (!boss.attackHitApplied && boss.stateTimer <= triggerTime) {
        boss.attackHitApplied = true;
        const fwdX = Math.sin(boss.yaw);
        const fwdZ = Math.cos(boss.yaw);

        if (boss.currentAttack === 'cleave') {
          // Attack 1: Crimson Cleave — wide horizontal hammer sweep (range 6.2m, damage 28)
          spawnCleaveWave(boss.position.x, boss.position.y, boss.position.z, boss.yaw);
          spawnBurst(
            boss.position.x + fwdX * 3.2,
            boss.position.y + 1.7,
            boss.position.z + fwdZ * 3.2,
            20,
            0xff2222,
            3.5,
            10.5,
            0.48,
            1.35,
            10
          );
          if (sound && sound.playHit) sound.playHit(true);

          const dy = Math.abs(focusPos.y - boss.position.y);
          if (distXZ <= 6.2 && dy < 3.6) {
            const dot = distXZ > 0.01 ? (toX * fwdX + toZ * fwdZ) / distXZ : 1;
            if (dot >= -0.15 || distXZ < 2.6) {
              applyDamageToHero(hero, dragon, 28, boss.position, true);
            }
          }
        } else if (boss.currentAttack === 'slam') {
          // Attack 2: Seismic Ground Slam — expanding fiery Shockwave Ring (1.5 -> 16m, damage 30)
          const slamX = boss.position.x + fwdX * 2.1;
          const slamZ = boss.position.z + fwdZ * 2.1;
          spawnShockwave(slamX, LANDMARKS.castle.floorY + 0.05, slamZ, 30);
          spawnBurst(slamX, LANDMARKS.castle.floorY + 0.35, slamZ, 32, 0xff4518, 4.0, 13.0, 0.65, 1.45, 14);
          if (sound && sound.playHit) sound.playHit(true);

          // Direct point-blank smash right under warhammer (still dodgeable with Q-roll or high jump!)
          const directDist = Math.hypot(focusPos.x - slamX, focusPos.z - slamZ);
          const aboveGround = focusPos.y - LANDMARKS.castle.floorY;
          if (directDist <= 3.4 && aboveGround < 2.2) {
            applyDamageToHero(hero, dragon, 30, boss.position, true);
          }
        } else if (boss.currentAttack === 'barrage') {
          // Attack 3: Abyssal Fire Volley — 3 molten fireballs in a spread (20m/s, damage 20)
          const ox = boss.position.x + fwdX * 1.4;
          const oy = boss.position.y + 2.85;
          const oz = boss.position.z + fwdZ * 1.4;
          const targetY = focusPos.y + (dragon && dragon.mounted ? 0.5 : 1.05);
          const aimX = focusPos.x - ox;
          const aimY = targetY - oy;
          const aimZ = focusPos.z - oz;
          const aimHoriz = Math.hypot(aimX, aimZ) || 1;
          const baseYaw = Math.atan2(aimX, aimZ);
          const pitch = Math.atan2(aimY, aimHoriz);
          const cosP = Math.cos(pitch);
          const sinP = Math.sin(pitch);
          const spreads = [-0.28, 0, 0.28]; // ~16 deg spread
          const speed = 20.0;

          for (let k = 0; k < 3; k++) {
            const ang = baseYaw + spreads[k];
            spawnFireball(
              ox,
              oy,
              oz,
              Math.sin(ang) * cosP * speed,
              sinP * speed,
              Math.cos(ang) * cosP * speed,
              20
            );
          }
          spawnBurst(ox, oy, oz, 18, 0xff5a18, 3.0, 9.0, 0.45, 1.25, 4);
          if (sound && sound.playFireBreath) sound.playFireBreath();
        }
      }

      if (boss.stateTimer <= 0) {
        boss.attackState = 'recover';
        boss.telegraphRatio = 0;
        boss.stateTimer = phaseTwo ? 0.72 : 0.9;
        boss.cooldown = phaseTwo ? 1.05 : 1.38;
      }
    } else if (boss.attackState === 'recover') {
      boss.stateTimer -= dt;
      if (boss.stateTimer <= 0) {
        boss.attackState = 'chase';
        boss.currentAttack = null;
      }
    }

    // Keep Boss solidly within Highkeep Castle Courtyard
    boss.position.x = Math.max(-24, Math.min(24, boss.position.x));
    boss.position.z = Math.max(-133, Math.min(-85, boss.position.z));
    boss.position.y = LANDMARKS.castle.floorY;
    boss.group.rotation.y = boss.yaw;

    // Boss Procedural Animation Choreography
    boss.animPhase += dt * (moveSpeed > 0.2 ? 5.6 : 2.1);
    const stride = moveSpeed > 0.2 ? Math.sin(boss.animPhase) : 0;
    p.leftLeg.rotation.x = stride * 0.44;
    p.rightLeg.rotation.x = -stride * 0.44;
    p.cape.rotation.x = 0.2 + (moveSpeed > 0.2 ? 0.16 : 0) + Math.sin(elapsed * 3.4) * 0.07;

    if (boss.attackState === 'telegraph') {
      const t = boss.telegraphRatio;
      if (boss.currentAttack === 'cleave') {
        p.bodyPivot.position.y = 1.88;
        p.torso.rotation.y = -0.95 * t;
        p.torso.rotation.x = -0.14 * t;
        p.rightArm.rotation.set(-1.15 * t, -0.45 * t, -0.95 * t);
        p.leftArm.rotation.set(-0.55 * t, 0, 0.35 * t);
      } else if (boss.currentAttack === 'slam') {
        // Dramatic leap/lift & towering overhead warhammer raise!
        p.bodyPivot.position.y = 1.88 + Math.sin(t * Math.PI * 0.65) * 0.72;
        p.torso.rotation.x = -0.34 * t;
        p.torso.rotation.y = 0;
        p.rightArm.rotation.set(-2.95 * t, 0, -0.18 * t);
        p.leftArm.rotation.set(-2.25 * t, 0, 0.25 * t);
      } else {
        // Abyssal Fire Volley: spreads arms wide exposing blazing furnace chest core!
        p.bodyPivot.position.y = 1.88 + t * 0.14;
        p.torso.rotation.x = -0.22 * t;
        p.torso.rotation.y = 0;
        p.rightArm.rotation.set(-0.45 * t, 0, -1.05 * t);
        p.leftArm.rotation.set(-0.45 * t, 0, 1.05 * t);
      }
    } else if (boss.attackState === 'attack') {
      const aDur = boss.currentAttack === 'cleave' ? 0.34 : boss.currentAttack === 'slam' ? 0.38 : 0.36;
      const t = 1 - Math.max(0, boss.stateTimer / aDur);
      if (boss.currentAttack === 'cleave') {
        p.bodyPivot.position.y = 1.88;
        p.torso.rotation.y = THREE.MathUtils.lerp(-0.95, 1.05, t);
        p.torso.rotation.x = THREE.MathUtils.lerp(-0.14, 0.18, t);
        p.rightArm.rotation.set(
          THREE.MathUtils.lerp(-1.15, -0.35, t),
          THREE.MathUtils.lerp(-0.45, 0.9, t),
          THREE.MathUtils.lerp(-0.95, 0.65, t)
        );
      } else if (boss.currentAttack === 'slam') {
        p.bodyPivot.position.y = THREE.MathUtils.lerp(2.45, 1.72, Math.min(1, t * 1.6));
        p.torso.rotation.x = THREE.MathUtils.lerp(-0.34, 0.52, t);
        p.rightArm.rotation.set(THREE.MathUtils.lerp(-2.95, 0.72, t), 0, 0);
        p.leftArm.rotation.set(THREE.MathUtils.lerp(-2.25, 0.35, t), 0, 0);
      } else {
        p.bodyPivot.position.y = 1.88;
        p.torso.rotation.x = THREE.MathUtils.lerp(-0.22, 0.24, t);
        p.rightArm.rotation.set(-0.5, 0, THREE.MathUtils.lerp(-1.05, -0.15, t));
        p.leftArm.rotation.set(-0.5, 0, THREE.MathUtils.lerp(1.05, 0.15, t));
      }
    } else if (boss.attackState === 'recover') {
      const rDur = phaseTwo ? 0.72 : 0.9;
      const t = 1 - Math.max(0, boss.stateTimer / rDur);
      p.bodyPivot.position.y = THREE.MathUtils.lerp(1.78, 1.88, t);
      p.torso.rotation.x = THREE.MathUtils.lerp(p.torso.rotation.x, 0, dt * 6);
      p.torso.rotation.y = THREE.MathUtils.lerp(p.torso.rotation.y, 0, dt * 6);
      p.rightArm.rotation.set(
        THREE.MathUtils.lerp(p.rightArm.rotation.x, -0.25, dt * 6),
        THREE.MathUtils.lerp(p.rightArm.rotation.y, 0, dt * 6),
        THREE.MathUtils.lerp(p.rightArm.rotation.z, 0, dt * 6)
      );
      p.leftArm.rotation.set(
        THREE.MathUtils.lerp(p.leftArm.rotation.x, -0.2, dt * 6),
        0,
        THREE.MathUtils.lerp(p.leftArm.rotation.z, 0, dt * 6)
      );
    } else {
      const bob = moveSpeed > 0.2 ? Math.abs(Math.cos(boss.animPhase)) * 0.14 : Math.sin(elapsed * 2.0) * 0.04;
      p.bodyPivot.position.y = 1.88 + bob;
      p.torso.rotation.x = moveSpeed > 0.2 ? 0.1 : 0;
      p.torso.rotation.y = stride * 0.12;
      p.rightArm.rotation.set(-stride * 0.36 - 0.24, 0, 0);
      p.leftArm.rotation.set(stride * 0.36 - 0.2, 0, 0);
    }
  }

  // ==========================================================================
  // 11. UPDATE ACTIVE FIREBALLS, SHOCKWAVES & SLASH WAVES
  // ==========================================================================
  function clearBossHazards() {
    for (let i = 0; i < fireballs.length; i++) {
      fireballs[i].active = false;
      fireballs[i].group.visible = false;
    }
    for (let i = 0; i < shockwaves.length; i++) {
      shockwaves[i].active = false;
      shockwaves[i].group.visible = false;
    }
    for (let i = 0; i < slashWaves.length; i++) {
      slashWaves[i].active = false;
      slashWaves[i].group.visible = false;
    }
  }

  function updateProjectilesAndWaves(dt, elapsed, hero, dragon) {
    const heroAlive = !isHeroDead(hero);
    const targetPos = dragon && dragon.mounted ? dragon.position : hero ? hero.position : null;
    const hitOffsetY = dragon && dragon.mounted ? 0.4 : 1.05;

    // 1. Abyssal Molten Fireballs
    for (let i = 0; i < fireballs.length; i++) {
      const fb = fireballs[i];
      if (!fb.active) continue;
      fb.life -= dt;
      fb.pos.x += fb.vx * dt;
      fb.pos.y += fb.vy * dt;
      fb.pos.z += fb.vz * dt;
      fb.shell.rotation.x += dt * 7.5;
      fb.shell.rotation.y += dt * 5.2;
      const pulse = 0.92 + Math.sin(elapsed * 22 + i) * 0.12;
      fb.group.scale.setScalar(pulse);

      // Trailing embers
      fb.emberAccum += dt;
      if (fb.emberAccum >= 0.035) {
        fb.emberAccum = 0;
        spawnSpark(
          fb.pos.x + (Math.random() - 0.5) * 0.3,
          fb.pos.y + (Math.random() - 0.5) * 0.3,
          fb.pos.z + (Math.random() - 0.5) * 0.3,
          -fb.vx * 0.12 + (Math.random() - 0.5) * 2.0,
          0.8 + Math.random() * 1.8,
          -fb.vz * 0.12 + (Math.random() - 0.5) * 2.0,
          0xff5a14,
          0.32,
          0.95,
          3
        );
      }

      // Check hero/dragon collision
      let exploded = false;
      if (heroAlive && targetPos) {
        const dx = targetPos.x - fb.pos.x;
        const dy = (targetPos.y + hitOffsetY) - fb.pos.y;
        const dz = targetPos.z - fb.pos.z;
        const hitR = dragon && dragon.mounted ? 2.6 : 1.35;
        if (dx * dx + dy * dy + dz * dz <= hitR * hitR) {
          applyDamageToHero(hero, dragon, fb.damage, fb.pos, true);
          exploded = true;
        }
      }

      // Terrain / expiry collision
      if (!exploded) {
        const groundY = getWalkableHeight(fb.pos.x, fb.pos.z, fb.pos.y + 1.0);
        if (fb.pos.y <= groundY + 0.3 || fb.life <= 0) {
          exploded = true;
        }
      }

      if (exploded) {
        spawnBurst(fb.pos.x, fb.pos.y, fb.pos.z, 16, 0xff4518, 2.5, 9.0, 0.45, 1.15, 10);
        fb.active = false;
        fb.group.visible = false;
      }
    }

    // 2. Expanding Seismic Ground Slam Shockwave Rings (radius 1.5 -> 16m)
    for (let i = 0; i < shockwaves.length; i++) {
      const sw = shockwaves[i];
      if (!sw.active) continue;
      sw.radius += sw.speed * dt;
      sw.group.scale.set(sw.radius, 1, sw.radius);

      const prog = (sw.radius - 1.5) / (sw.maxRadius - 1.5);
      const alpha = Math.max(0, 1 - Math.max(0, prog - 0.55) / 0.45);
      sw.torusMat.opacity = alpha * 0.95;
      sw.discMat.opacity = alpha * 0.62;

      // Occasional fiery rim sparks
      if (Math.random() < dt * 28) {
        const a = Math.random() * Math.PI * 2;
        spawnSpark(
          sw.origin.x + Math.cos(a) * sw.radius,
          sw.origin.y + 0.25,
          sw.origin.z + Math.sin(a) * sw.radius,
          Math.cos(a) * 2.0,
          2.2 + Math.random() * 2.4,
          Math.sin(a) * 2.0,
          0xff5a14,
          0.35,
          1.1,
          8
        );
      }

      // Check player crossing shockwave ring (Jump over or Q-roll through with i-frames!)
      if (!sw.hitHero && heroAlive && hero && !(dragon && dragon.mounted && dragon.position.y > sw.origin.y + 2.2)) {
        const pPos = dragon && dragon.mounted ? dragon.position : hero.position;
        const distXZ = Math.hypot(pPos.x - sw.origin.x, pPos.z - sw.origin.z);
        if (Math.abs(distXZ - sw.radius) < 1.28) {
          const heightAbove = pPos.y - sw.origin.y;
          if (heightAbove < 1.25) {
            sw.hitHero = true;
            if (isHeroRolling(hero)) {
              spawnDamageNumber(pPos.x, pPos.y + 2.1, pPos.z, 'DODGED!', '#62f4ff', 0.85);
            } else if (!isHeroInvulnerable(hero)) {
              applyDamageToHero(hero, dragon, sw.damage, sw.origin, true);
            }
          }
        }
      }

      if (sw.radius >= sw.maxRadius) {
        sw.active = false;
        sw.group.visible = false;
      }
    }

    // 3. Crimson Cleave Slash Waves
    for (let i = 0; i < slashWaves.length; i++) {
      const sw = slashWaves[i];
      if (!sw.active) continue;
      sw.life -= dt;
      if (sw.life <= 0) {
        sw.active = false;
        sw.group.visible = false;
        continue;
      }
      const prog = 1 - sw.life / sw.maxLife;
      const sc = 0.65 + prog * 0.85;
      sw.group.scale.set(sc, 1, sc);
      sw.group.position.x += sw.dirX * dt * 8.5;
      sw.group.position.z += sw.dirZ * dt * 8.5;
      sw.mat.opacity = (1 - prog) * 0.92;
    }
  }

  // ==========================================================================
  // 12. WORLD QUEST INTERACTABLES (Sky Rune Rings & Sunstone Relic)
  // ==========================================================================
  function updateWorldInteractables(hero, dragon, quests, world) {
    if (!world) return;

    // Sky Rune Rings (while mounted on the Red Dragon)
    if (dragon && dragon.mounted && dragon.position && Array.isArray(world.skyRings)) {
      const dp = dragon.position;
      for (let i = 0; i < world.skyRings.length; i++) {
        const ring = world.skyRings[i];
        if (!ring || ring.collected) continue;
        const rx = ring.position ? ring.position.x : ring.x;
        const ry = ring.position ? ring.position.y : ring.y;
        const rz = ring.position ? ring.position.z : ring.z;
        const rr = ring.radius ?? 7.5;
        const dist = Math.hypot(dp.x - rx, dp.y - ry, dp.z - rz);
        if (dist < rr + 3.0) {
          ring.collected = true;
          // Preserve mesh visibility when world.js animates its shrink-spin disappear; hide otherwise
          if (ring.mesh && !ring._inner) ring.mesh.visible = false;
          if (ring.group) ring.group.visible = false;
          if (ring.ringMesh) ring.ringMesh.visible = false;
          if (ring.light) ring.light.visible = false;

          // Burst celebratory velocity-stretched rune ember shards around ring!
          for (let k = 0; k < 24; k++) {
            const a = (k / 24) * Math.PI * 2;
            const radialSpd = 6.5 + (k % 3) * 1.8;
            spawnSpark(
              rx + Math.cos(a) * rr * 0.72,
              ry + Math.sin(a) * rr * 0.72,
              rz,
              Math.cos(a) * radialSpd,
              Math.sin(a) * radialSpd,
              (Math.random() - 0.5) * 6.5,
              (k % 3) === 0 ? 0x2ce8ff : (k % 3) === 1 ? 0xffbf38 : 0xff8c1a,
              0.82,
              1.3,
              2
            );
          }
          spawnDamageNumber(rx, ry + 2.0, rz, 'SKY RING!', '#5ef2ff', 1.25);
          if (sound && sound.playQuest) sound.playQuest();
          if (quests && quests.onSkyRingPassed) quests.onSkyRingPassed(i);
        }
      }
    }

    // Sunstone Relic inside Dark Dungeon
    const relic = world.sunstoneRelic;
    if (hero && hero.position && !(dragon && dragon.mounted) && relic && !relic.collected) {
      const rx = relic.position ? relic.position.x : relic.x;
      const ry = relic.position ? relic.position.y : relic.y;
      const rz = relic.position ? relic.position.z : relic.z;
      const dx = hero.position.x - rx;
      const dy = (hero.position.y + 1.0) - ry;
      const dz = hero.position.z - rz;
      const dist = Math.hypot(dx, dy, dz);
      const horizDist = Math.hypot(dx, dz);
      if (dist < 3.2 || (horizDist < 3.2 && Math.abs(dy) < 3.5)) {
        relic.collected = true;
        if (relic.mesh) relic.mesh.visible = false;
        if (relic.group) relic.group.visible = false;
        if (relic.orb) relic.orb.visible = false;
        if (relic.light) relic.light.visible = false;

        spawnBurst(rx, ry, rz, 28, 0xffd859, 2.5, 9.5, 0.9, 1.35, 5);
        spawnDamageNumber(rx, ry + 1.4, rz, 'SUNSTONE RELIC!', '#ffd859', 1.35);
        if (sound && sound.playQuest) sound.playQuest();
        if (quests && quests.onRelicCollected) quests.onRelicCollected();
      }
    }
  }

  // ==========================================================================
  // 13. PUBLIC API: update, resetBoss, getVisibleEnemies
  // ==========================================================================
  const _visibleEnemies = [];

  function getVisibleEnemies(playerPos, maxDist = 34) {
    _visibleEnemies.length = 0;
    if (!playerPos) return _visibleEnemies;
    const maxDistSq = maxDist * maxDist;
    const px = playerPos.x;
    const py = playerPos.y;
    const pz = playerPos.z;

    for (let i = 0; i < enemies.length; i++) {
      const e = enemies[i];
      if (!e.alive) continue;
      const dx = e.position.x - px;
      const dy = e.position.y - py;
      const dz = e.position.z - pz;
      if (dx * dx + dy * dy + dz * dz <= maxDistSq) {
        _visibleEnemies.push(e);
      }
    }
    return _visibleEnemies;
  }

  function resetBoss() {
    if (!boss.alive) return;
    boss.hp = boss.maxHp;
    boss.engaged = false;
    boss.attackState = 'idle';
    boss.telegraphRatio = 0;
    boss.currentAttack = null;
    boss.stateTimer = 0;
    boss.cooldown = 0.75;
    boss.attackHitApplied = false;
    boss.hitFlash = 0;
    boss.burnTimer = 0;
    boss.burnTickDmg = 0;
    boss.burnTickTimer = 0;
    boss.yaw = 0;
    boss.position.set(BOSS_HOME_X, BOSS_HOME_Y, BOSS_HOME_Z);
    boss.group.rotation.set(0, 0, 0);
    boss.telegraphRing.visible = false;
    boss.telegraphLight.intensity = 0;
    clearBossHazards();
  }

  function update(dt, elapsed, hero, dragon, quests, world) {
    const step = Math.min(dt, 0.05);
    if (step <= 0) return;

    processHeroAttack(hero, quests);
    processDragonFire(step, hero, dragon, quests);
    updateEnemies(step, elapsed, hero, dragon);
    updateBoss(step, elapsed, hero, dragon);
    updateProjectilesAndWaves(step, elapsed, hero, dragon);
    updateLoot(step, elapsed, hero, dragon);
    updateWorldInteractables(hero, dragon, quests, world);
    updateSparks(step);
    updateDamageNumbers(step);
  }

  return {
    enemies,
    boss,
    loot,
    damageNumbers,
    update,
    resetBoss,
    getVisibleEnemies,
  };
}
