// ============================================================================
// src/world.js — Open-World Fantasy Realm ("Aethelgard: Ashes of the Drake")
// ============================================================================
import * as THREE from 'three';
import {
  WORLD,
  PALETTE,
  LANDMARKS,
  COLLIDERS,
  addCircleCollider,
  addBoxCollider,
  fbm,
  noise2D,
  isInsideDungeon,
  getBridgeDeckHeight,
  getTerrainHeight,
  mulberry32,
} from './config.js';

// Preallocated scratch objects — zero per-frame heap allocations in update()
const _v1 = new THREE.Vector3();
const _v2 = new THREE.Vector3();
const _v3 = new THREE.Vector3();
const _up = new THREE.Vector3(0, 1, 0);
const _sunDir = new THREE.Vector3(0, 1, 0);
const _moonDir = new THREE.Vector3(0, 0.47, -0.88);
const _lightDir = new THREE.Vector3(0, 1, 0);
const _mat4 = new THREE.Matrix4();
const _quat = new THREE.Quaternion();
const _euler = new THREE.Euler();
const _scale = new THREE.Vector3();

const _col1 = new THREE.Color();
const _col2 = new THREE.Color();
const _colZenith = new THREE.Color();
const _colMidSky = new THREE.Color();
const _colHorizon = new THREE.Color();
const _colSun = new THREE.Color();
const _colFog = new THREE.Color();
const _colHemiSky = new THREE.Color();
const _colHemiGround = new THREE.Color();

// Rich multi-stop sky & lighting palettes (Day / Golden Sunset / Starry Night)
const _cZenithDay = new THREE.Color(PALETTE.skyZenithDay);
const _cMidDay = new THREE.Color(0x6ba6de);
const _cHorizonDay = new THREE.Color(0xb2dbf4);

const _cZenithSunset = new THREE.Color(0x2e4f8a); // Royal twilight azure-indigo
const _cMidSunset = new THREE.Color(0xe08952);    // Warm peach-amber
const _cHorizonSunset = new THREE.Color(0xf7bc74); // Radiant golden apricot

const _cZenithNight = new THREE.Color(0x070d24);
const _cMidNight = new THREE.Color(0x102046);
const _cHorizonNight = new THREE.Color(0x1c325e);

const _cSunDay = new THREE.Color(0xfff4dd);
const _cSunSunset = new THREE.Color(0xffd194);    // Warm golden hour sun
const _cMoonLight = new THREE.Color(0xa4cbff);    // Cool silver-blue moonlight
const _cSunsetGlow = new THREE.Color(0xffbe6b);   // Soft golden-apricot horizon glow

const _cHemiSkyDay = new THREE.Color(0x9ecdf7);
const _cHemiSkySunset = new THREE.Color(0x9bc4f5);
const _cHemiSkyNight = new THREE.Color(0x486ca8);

const _cHemiGndDay = new THREE.Color(0x4d6942);
const _cHemiGndSunset = new THREE.Color(0x5b7447);
const _cHemiGndNight = new THREE.Color(0x1e2e44);

const _cDungFog = new THREE.Color(0x0d080c);
const _cDungHemiSky = new THREE.Color(0x2d1316);
const _cDungHemiGnd = new THREE.Color(0x180a09);
const _cRayDay = new THREE.Color(0xfff0cc);
const _cRaySunset = new THREE.Color(0xffc978);

function smoothstep(edge0, edge1, x) {
  const t = Math.max(0, Math.min(1, (x - edge0) / (edge1 - edge0)));
  return t * t * (3 - 2 * t);
}

function pointToSegDist(px, pz, ax, az, bx, bz) {
  const vx = bx - ax;
  const vz = bz - az;
  const wx = px - ax;
  const wz = pz - az;
  const lenSq = vx * vx + vz * vz;
  const t = lenSq > 0.0001 ? Math.max(0, Math.min(1, (wx * vx + wz * vz) / lenSq)) : 0;
  return Math.hypot(px - (ax + vx * t), pz - (az + vz * t));
}

/**
 * Computes 0..1 weight for roads, village square, bridge approaches, and dungeon forecourt.
 */
function getRoadWeight(x, z) {
  let w = 0;

  // Village central cobblestone square
  const dVill = Math.hypot(x - LANDMARKS.village.x, z - LANDMARKS.village.z);
  w = Math.max(w, 1.0 - smoothstep(10.5, 15.2, dVill));

  // Main south-north avenue through village up to south bridge bank
  if (z >= 17 && z <= 64) {
    const endFade = smoothstep(16.5, 19.5, z) * (1.0 - smoothstep(58, 64, z));
    w = Math.max(w, (1.0 - smoothstep(3.6, 6.2, Math.abs(x))) * endFade);
  }

  // Grand road from north bridge to castle gatehouse
  if (z <= -33 && z >= -83) {
    const endFade = smoothstep(-84, -80, z) * (1.0 - smoothstep(-35, -32.5, z));
    w = Math.max(w, (1.0 - smoothstep(4.2, 6.8, Math.abs(x))) * endFade);
  }

  // Village to Dragon Perch footpath
  const dDragRoad = pointToSegDist(x, z, -8, 40, LANDMARKS.dragonPerch.x, LANDMARKS.dragonPerch.z);
  w = Math.max(w, 1.0 - smoothstep(2.2, 4.2, dDragRoad));

  // Village to Rune Circle road
  const dRuinsRoad = pointToSegDist(x, z, 8, 39, LANDMARKS.ruins.x, LANDMARKS.ruins.z);
  w = Math.max(w, 1.0 - smoothstep(2.5, 4.6, dRuinsRoad));

  // Rune Circle to Dark Dungeon entrance road
  const dDungRoad = pointToSegDist(x, z, LANDMARKS.ruins.x, LANDMARKS.ruins.z, 92, -24);
  w = Math.max(w, 1.0 - smoothstep(2.8, 5.2, dDungRoad));

  // Dungeon entrance forecourt
  const dFore = Math.hypot(x - 89, z + 24);
  w = Math.max(w, 1.0 - smoothstep(5.5, 10.5, dFore));

  return w;
}

function getTerrainSlopeY(x, z) {
  const e = 1.2;
  const hx = getTerrainHeight(x + e, z) - getTerrainHeight(x - e, z);
  const hz = getTerrainHeight(x, z + e) - getTerrainHeight(x, z - e);
  const invLen = 1.0 / Math.hypot(hx, 2 * e, hz);
  return (2 * e) * invLen;
}

/**
 * Scales box/plane UVs cleanly without cloning textures.
 */
function scaleGeometryUVs(geo, su, sv) {
  const uv = geo.attributes.uv;
  if (!uv) return geo;
  for (let i = 0; i < uv.count; i++) {
    uv.setXY(i, uv.getX(i) * su, uv.getY(i) * sv);
  }
  uv.needsUpdate = true;
  return geo;
}

// Init-time matrix helpers (called only inside createWorld, never in update())
const _localMat = new THREE.Matrix4();

function makeTransformMatrix(x, y, z, rx = 0, ry = 0, rz = 0, sx = 1, sy = 1, sz = 1) {
  const m = new THREE.Matrix4();
  _v1.set(x, y, z);
  _euler.set(rx, ry, rz);
  _quat.setFromEuler(_euler);
  _scale.set(sx, sy, sz);
  m.compose(_v1, _quat, _scale);
  return m;
}

function pushChildMatrix(list, parentM, lx, ly, lz, rx = 0, ry = 0, rz = 0, sx = 1, sy = 1, sz = 1) {
  _v1.set(lx, ly, lz);
  _euler.set(rx, ry, rz);
  _quat.setFromEuler(_euler);
  _scale.set(sx, sy, sz);
  _localMat.compose(_v1, _quat, _scale);
  const out = new THREE.Matrix4();
  out.multiplyMatrices(parentM, _localMat);
  list.push(out);
}

function buildInstancedMesh(geo, mat, matrices, castShadow = false, receiveShadow = false) {
  if (!matrices || matrices.length === 0) return null;
  const inst = new THREE.InstancedMesh(geo, mat, matrices.length);
  inst.castShadow = castShadow;
  inst.receiveShadow = receiveShadow;
  for (let i = 0; i < matrices.length; i++) {
    inst.setMatrixAt(i, matrices[i]);
  }
  inst.instanceMatrix.needsUpdate = true;
  inst.computeBoundingSphere();
  return inst;
}

/**
 * Creates a GPU-billboarded InstancedMesh shader material for soft additive glow halos.
 */
function createBillboardGlowMaterial(map, colorHex, opacity = 0.72, additive = true) {
  return new THREE.ShaderMaterial({
    uniforms: {
      uMap: { value: map },
      uColor: { value: new THREE.Color(colorHex) },
      uOpacity: { value: opacity },
    },
    transparent: true,
    depthWrite: false,
    blending: additive ? THREE.AdditiveBlending : THREE.NormalBlending,
    vertexShader: /* glsl */ `
      varying vec2 vUv;
      void main() {
        vUv = uv;
        vec4 worldCenter = modelMatrix * instanceMatrix * vec4(0.0, 0.0, 0.0, 1.0);
        float sx = length(vec3(instanceMatrix[0][0], instanceMatrix[0][1], instanceMatrix[0][2]));
        float sy = length(vec3(instanceMatrix[1][0], instanceMatrix[1][1], instanceMatrix[1][2]));
        vec4 mvCenter = viewMatrix * worldCenter;
        mvCenter.xy += position.xy * vec2(sx, sy);
        gl_Position = projectionMatrix * mvCenter;
      }
    `,
    fragmentShader: /* glsl */ `
      uniform sampler2D uMap;
      uniform vec3 uColor;
      uniform float uOpacity;
      varying vec2 vUv;
      void main() {
        vec4 tex = texture2D(uMap, vUv);
        gl_FragColor = vec4(uColor * tex.rgb, tex.a * uOpacity);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
      }
    `,
  });
}

// ============================================================================
// MAIN WORLD BUILDER
// ============================================================================
export function createWorld(scene, textures, isMobile = false) {
  // Clear static colliders in case world is recreated
  COLLIDERS.length = 0;

  const worldGroup = new THREE.Group();
  worldGroup.name = 'AethelgardWorld';
  scene.add(worldGroup);

  // Track animated elements for update()
  const flameSpecs = [];
  const flickeringLights = [];
  const smokeWisps = [];
  const runeMaterials = [];
  const grassUniforms = { uTime: { value: 0 } };
  const canopyUniforms = { uTime: { value: 0 } };
  const bannerUniforms = { uTime: { value: 0 } };

  // Global instanced batch collectors for repeated small structures across the world
  const brazierBaseMats = [];
  const brazierStemMats = [];
  const brazierBowlMats = [];
  const brazierRimMats = [];
  const brazierCoalMats = [];
  const purpleCrystalMats = [];
  const amberCrystalMats = [];

  // Shared unit primitives
  const unitBoxGeo = new THREE.BoxGeometry(1, 1, 1);
  const billboardQuadGeo = new THREE.PlaneGeometry(1, 1);

  // Shared materials
  const stoneMat = new THREE.MeshStandardMaterial({
    map: textures.stoneMasonry,
    normalMap: textures.stoneNormal,
    normalScale: new THREE.Vector2(0.85, 0.85),
    roughnessMap: textures.stoneRough,
    color: 0xbec4ce,
    roughness: 0.86,
    metalness: 0.06,
  });

  const stoneDarkMat = new THREE.MeshStandardMaterial({
    map: textures.stoneMasonry,
    normalMap: textures.stoneNormal,
    normalScale: new THREE.Vector2(0.9, 0.9),
    roughnessMap: textures.stoneRough,
    color: 0x6c727e,
    roughness: 0.9,
    metalness: 0.08,
  });

  const obsidianMat = new THREE.MeshStandardMaterial({
    map: textures.stoneMasonry,
    normalMap: textures.stoneNormal,
    normalScale: new THREE.Vector2(0.7, 0.7),
    color: 0x26252f,
    roughness: 0.72,
    metalness: 0.16,
  });

  const darkWoodMat = new THREE.MeshStandardMaterial({
    color: 0x4b3222,
    roughness: 0.88,
    metalness: 0.04,
  });

  const warmWoodMat = new THREE.MeshStandardMaterial({
    color: 0x765033,
    roughness: 0.85,
    metalness: 0.04,
  });

  const ironMat = new THREE.MeshStandardMaterial({
    color: 0x2a2d35,
    roughness: 0.4,
    metalness: 0.85,
    envMapIntensity: 1.1,
  });

  const goldTrimMat = new THREE.MeshStandardMaterial({
    color: PALETTE.gold,
    roughness: 0.26,
    metalness: 0.9,
    envMapIntensity: 1.15,
  });

  const coalMat = new THREE.MeshStandardMaterial({
    color: 0x3b1208,
    emissive: 0xff4510,
    emissiveIntensity: 2.4,
    roughness: 0.9,
  });

  const cottageWindowMat = new THREE.MeshStandardMaterial({
    color: 0xffd591,
    emissive: 0xffb347,
    emissiveIntensity: 1.5,
    roughness: 0.3,
  });

  const lanternGlassMat = new THREE.MeshStandardMaterial({
    color: 0xffe2a8,
    emissive: 0xffb347,
    emissiveIntensity: 1.9,
    roughness: 0.22,
  });

  const runePillarMat = new THREE.MeshStandardMaterial({
    map: textures.runeStone,
    color: 0xc2c6cf,
    emissive: PALETTE.runeGold,
    emissiveMap: textures.runeStone,
    emissiveIntensity: 0.65,
    roughness: 0.82,
    metalness: 0.1,
  });
  runeMaterials.push(runePillarMat);

  const dungeonRuneMat = new THREE.MeshStandardMaterial({
    map: textures.runeStone,
    color: 0x565460,
    emissive: 0xff8c2b,
    emissiveMap: textures.runeStone,
    emissiveIntensity: 0.95,
    roughness: 0.76,
    metalness: 0.16,
  });
  runeMaterials.push(dungeonRuneMat);

  const purpleCrystalMat = new THREE.MeshStandardMaterial({
    color: 0x9b4dff,
    emissive: 0x6e20dd,
    emissiveIntensity: 1.55,
    roughness: 0.16,
    metalness: 0.28,
  });
  const amberCrystalMat = new THREE.MeshStandardMaterial({
    color: 0xffb838,
    emissive: 0xff6e14,
    emissiveIntensity: 1.65,
    roughness: 0.18,
    metalness: 0.25,
  });
  runeMaterials.push(purpleCrystalMat, amberCrystalMat);

  // Register a brazier into the global instanced batches + animated flame list
  function registerBrazier(wx, wy, wz, scale = 1.0, phase = 0) {
    const pM = makeTransformMatrix(wx, wy, wz, 0, 0, 0, scale, scale, scale);
    pushChildMatrix(brazierBaseMats, pM, 0, 0.125, 0);
    pushChildMatrix(brazierStemMats, pM, 0, 0.52, 0);
    pushChildMatrix(brazierBowlMats, pM, 0, 0.98, 0);
    pushChildMatrix(brazierRimMats, pM, 0, 1.18, 0, Math.PI * 0.5, 0, 0);
    pushChildMatrix(brazierCoalMats, pM, 0, 1.12, 0, 0, 0, 0, 1, 0.38, 1);
    flameSpecs.push({ x: wx, y: wy + 1.12 * scale, z: wz, scale: scale * 0.95, phase });
  }

  // Register a crystal shard cluster into global instanced crystal batches
  function registerCrystalCluster(wx, wy, wz, isPurple = true, scale = 1.0, seed = 1) {
    const crng = mulberry32(seed);
    const targetList = isPurple ? purpleCrystalMats : amberCrystalMats;
    const pM = makeTransformMatrix(wx, wy, wz);
    const count = 4 + Math.floor(crng() * 3);
    for (let k = 0; k < count; k++) {
      const h = (0.55 + crng() * 0.85) * scale;
      const w = (0.55 + crng() * 0.55) * scale;
      const a = (k / count) * Math.PI * 2 + crng() * 0.5;
      pushChildMatrix(
        targetList,
        pM,
        Math.cos(a) * 0.26 * scale,
        -0.1,
        Math.sin(a) * 0.26 * scale,
        (crng() - 0.5) * 0.55,
        crng() * Math.PI,
        (crng() - 0.5) * 0.55,
        w,
        h,
        w
      );
    }
  }

  // ==========================================================================
  // 0. LIGHTING & SCENE FOG
  // ==========================================================================
  const hemiLight = new THREE.HemisphereLight(0x9bc4f5, 0x5b7447, 1.15);
  scene.add(hemiLight);

  const sunLight = new THREE.DirectionalLight(0xffd194, 2.6);
  sunLight.castShadow = true;
  const shadowRes = isMobile ? 1024 : 2048;
  sunLight.shadow.mapSize.set(shadowRes, shadowRes);
  sunLight.shadow.camera.near = 10;
  sunLight.shadow.camera.far = 260;
  const shadowHalf = isMobile ? 68 : 88;
  sunLight.shadow.camera.left = -shadowHalf;
  sunLight.shadow.camera.right = shadowHalf;
  sunLight.shadow.camera.top = shadowHalf;
  sunLight.shadow.camera.bottom = -shadowHalf;
  sunLight.shadow.bias = -0.0005;
  sunLight.shadow.normalBias = 0.045;
  scene.add(sunLight);
  scene.add(sunLight.target);

  scene.fog = new THREE.FogExp2(0xf7bc74, 0.0038);

  // ==========================================================================
  // 1. SCULPTED TERRAIN WITH VERTEX COLORS (120x120 — smooth hills, half tris!)
  // ==========================================================================
  {
    const segs = 120;
    const terrainGeo = new THREE.PlaneGeometry(WORLD.SIZE, WORLD.SIZE, segs, segs);
    terrainGeo.rotateX(-Math.PI * 0.5);

    const pos = terrainGeo.attributes.position;
    for (let i = 0; i < pos.count; i++) {
      const x = pos.getX(i);
      const z = pos.getZ(i);
      pos.setY(i, getTerrainHeight(x, z));
    }
    terrainGeo.computeVertexNormals();

    const norm = terrainGeo.attributes.normal;
    const colors = new Float32Array(pos.count * 3);

    const cLush = new THREE.Color(PALETTE.grassLush);
    const cSunlit = new THREE.Color(PALETTE.grassSunlit);
    const cValley = new THREE.Color(PALETTE.grassValley);
    const cMossBlue = new THREE.Color(0x28565d);
    const cRock = new THREE.Color(0x5b5856);
    const cHighPeak = new THREE.Color(0x6e737f);
    const cSand = new THREE.Color(0xa69470);
    const cLakeBed = new THREE.Color(0x1a3438);
    const cDirtRoad = new THREE.Color(0x8e7452);
    const cCobble = new THREE.Color(0x9a9180);
    const cBasalt = new THREE.Color(0x221f26);

    const l = LANDMARKS.lake;

    for (let i = 0; i < pos.count; i++) {
      const x = pos.getX(i);
      const y = pos.getY(i);
      const z = pos.getZ(i);
      const ny = norm.getY(i);

      // Rich organic grass blend
      const n1 = fbm(x * 0.022, z * 0.022, 3) * 0.5 + 0.5;
      const n2 = noise2D(x * 0.065 + 4.1, z * 0.065 - 7.3);
      _col1.copy(cValley).lerp(cLush, smoothstep(0.18, 0.72, n1));
      _col1.lerp(cSunlit, smoothstep(0.45, 0.92, n1 * 0.65 + n2 * 0.35) * smoothstep(2.0, 14.0, y));

      // Subtle enchanted mossy-teal undertone in blue-leaf forest hills
      const forestTint = smoothstep(32, 85, Math.hypot(x, z - 10)) * (1.0 - smoothstep(130, 175, Math.hypot(x, z)));
      if (forestTint > 0.01 && z > -105) {
        _col1.lerp(cMossBlue, forestTint * 0.24 * n2);
      }

      // Sandy / pebbled shoreline & silt lakebed near Mirror Lake
      const lakeNorm = Math.hypot((x - l.x) / l.rx, (z - l.z) / l.rz);
      if (lakeNorm < 1.38) {
        const shoreT = 1.0 - smoothstep(0.2, 1.55, y - WORLD.WATER_LEVEL);
        const bedT = 1.0 - smoothstep(-2.4, 0.1, y - WORLD.WATER_LEVEL);
        _col2.copy(cSand).lerp(cLakeBed, bedT);
        _col1.lerp(_col2, shoreT * (1.0 - smoothstep(1.05, 1.38, lakeNorm)));
      }

      // Steep cliff rock grey-brown based on normal.y slope
      const slopeRock = smoothstep(0.86, 0.58, ny);
      if (slopeRock > 0.001) {
        _col2.copy(cRock).lerp(cHighPeak, smoothstep(26, 52, y));
        _col1.lerp(_col2, slopeRock);
      }

      // Warm dirt & cobblestone roads
      const roadW = getRoadWeight(x, z);
      if (roadW > 0.001) {
        const isCobble =
          Math.hypot(x - LANDMARKS.village.x, z - LANDMARKS.village.z) < 16 ||
          (Math.abs(x) < 6.5 && z < -32 && z > -84) ||
          (Math.abs(x) < 6.5 && z > 17 && z < 44);
        _col2.copy(isCobble ? cCobble : cDirtRoad);
        _col2.offsetHSL(0, 0, (n2 - 0.5) * 0.06);
        _col1.lerp(_col2, roadW * 0.88);
      }

      // Dark volcanic basalt near Ember Crypt dungeon entrance & chamber
      if (x > 74 && Math.abs(z + 24) < 30) {
        const bX = smoothstep(74, 92, x);
        const bZ = 1.0 - smoothstep(12, 28, Math.abs(z + 24));
        _col1.lerp(cBasalt, bX * bZ * 0.94);
      }

      colors[i * 3] = _col1.r;
      colors[i * 3 + 1] = _col1.g;
      colors[i * 3 + 2] = _col1.b;
    }

    terrainGeo.setAttribute('color', new THREE.BufferAttribute(colors, 3));

    const terrainMat = new THREE.MeshStandardMaterial({
      map: textures.terrainDetail,
      normalMap: textures.terrainNormal,
      normalScale: new THREE.Vector2(0.65, 0.65),
      roughnessMap: textures.terrainRough,
      vertexColors: true,
      roughness: 0.92,
      metalness: 0.02,
    });

    const terrainMesh = new THREE.Mesh(terrainGeo, terrainMat);
    terrainMesh.receiveShadow = true;
    worldGroup.add(terrainMesh);
  }

  // ==========================================================================
  // 2. MIRROR LAKE (CUSTOM GLSL WATER SHADER)
  // ==========================================================================
  const waterUniforms = {
    uTime: { value: 0 },
    uSunDir: { value: new THREE.Vector3(-0.91, 0.13, -0.38) },
    uMoonDir: { value: new THREE.Vector3(-0.09, 0.47, -0.88) },
    uSunColor: { value: new THREE.Color(0xffd194) },
    uSkyZenith: { value: new THREE.Color(0x2e4f8a) },
    uSkyHorizon: { value: new THREE.Color(0xf7bc74) },
    uMoonStrength: { value: 0.0 },
    uSunStrength: { value: 1.0 },
    uFogColor: { value: new THREE.Color(0xf7bc74) },
    uFogDensity: { value: 0.0038 },
  };

  {
    const waterGeo = new THREE.PlaneGeometry(106, 68, 48, 32);
    waterGeo.rotateX(-Math.PI * 0.5);
    const wPos = waterGeo.attributes.position;
    const depths = new Float32Array(wPos.count);

    for (let i = 0; i < wPos.count; i++) {
      const wx = wPos.getX(i) + LANDMARKS.lake.x;
      const wz = wPos.getZ(i) + LANDMARKS.lake.z;
      const th = getTerrainHeight(wx, wz);
      depths[i] = Math.max(0.0, WORLD.WATER_LEVEL - th);
    }
    waterGeo.setAttribute('aDepth', new THREE.BufferAttribute(depths, 1));

    const waterMat = new THREE.ShaderMaterial({
      uniforms: waterUniforms,
      transparent: true,
      depthWrite: false,
      vertexShader: /* glsl */ `
        attribute float aDepth;
        uniform float uTime;
        varying vec3 vWorldPos;
        varying float vDepth;
        void main() {
          vec3 p = position;
          vec4 wp = modelMatrix * vec4(p, 1.0);
          float swell = sin(wp.x * 0.24 + uTime * 1.15) * cos(wp.z * 0.28 - uTime * 0.9) * 0.045;
          p.y += swell * smoothstep(0.0, 0.8, aDepth);
          wp = modelMatrix * vec4(p, 1.0);
          vWorldPos = wp.xyz;
          vDepth = aDepth;
          gl_Position = projectionMatrix * viewMatrix * wp;
        }
      `,
      fragmentShader: /* glsl */ `
        uniform float uTime;
        uniform vec3 uSunDir;
        uniform vec3 uMoonDir;
        uniform vec3 uSunColor;
        uniform vec3 uSkyZenith;
        uniform vec3 uSkyHorizon;
        uniform float uMoonStrength;
        uniform float uSunStrength;
        uniform vec3 uFogColor;
        uniform float uFogDensity;
        varying vec3 vWorldPos;
        varying float vDepth;

        float waveH(vec2 p) {
          float w = 0.0;
          w += sin(p.x * 0.65 + p.y * 0.42 + uTime * 1.65) * 0.45;
          w += sin(p.x * -0.48 + p.y * 0.78 - uTime * 1.25) * 0.35;
          w += sin(p.x * 1.42 + p.y * -1.15 + uTime * 2.35) * 0.14;
          w += sin(p.x * -2.1 + p.y * -1.7 + uTime * 2.9) * 0.07;
          return w;
        }

        void main() {
          if (vDepth <= 0.005) discard;
          vec2 p = vWorldPos.xz;
          float eps = 0.16;
          float h0 = waveH(p);
          float hx = waveH(p + vec2(eps, 0.0));
          float hz = waveH(p + vec2(0.0, eps));
          vec3 N = normalize(vec3((h0 - hx) * 0.24, 1.0, (h0 - hz) * 0.24));
          vec3 V = normalize(cameraPosition - vWorldPos);

          // Depth-based emerald/teal water body tint (linear space for AgX)
          vec3 shallowCol = vec3(0.024, 0.19, 0.18);
          vec3 deepCol = vec3(0.007, 0.036, 0.068);
          float depthT = smoothstep(0.0, 3.2, vDepth);
          vec3 waterBase = mix(shallowCol, deepCol, depthT);

          // Fresnel sky reflection
          float ndv = max(dot(N, V), 0.0);
          float fresnel = 0.08 + 0.88 * pow(1.0 - ndv, 3.6);
          vec3 R = reflect(-V, N);
          vec3 reflSky = mix(uSkyHorizon, uSkyZenith, smoothstep(0.0, 0.65, max(R.y, 0.0)));
          vec3 col = mix(waterBase, reflSky, clamp(fresnel * 0.85, 0.0, 0.92));

          // Sun specular sparkle + broad golden reflection path
          vec3 Hsun = normalize(uSunDir + V);
          float specSharp = pow(max(dot(N, Hsun), 0.0), 140.0) * 2.4;
          float specBroad = pow(max(dot(N, Hsun), 0.0), 18.0) * 0.34;
          col += uSunColor * (specSharp + specBroad) * uSunStrength;

          // Silver moonlight sparkle & shimmer path at night
          vec3 Hmoon = normalize(uMoonDir + V);
          float specMoon = pow(max(dot(N, Hmoon), 0.0), 92.0) * 1.45 + pow(max(dot(N, Hmoon), 0.0), 22.0) * 0.22;
          col += vec3(0.48, 0.72, 1.0) * specMoon * uMoonStrength;

          // Subtle animated shoreline foam ripples
          float foamZone = 1.0 - smoothstep(0.04, 0.72, vDepth);
          float foamWave = sin(vDepth * 14.0 - uTime * 2.6 + h0 * 2.4) * 0.5 + 0.5;
          float foam = foamZone * smoothstep(0.42, 0.88, foamWave) * 0.65 + (1.0 - smoothstep(0.0, 0.16, vDepth)) * 0.45;
          vec3 foamCol = mix(vec3(0.72, 0.86, 0.90), uSkyHorizon * 1.15, 0.35);
          col = mix(col, foamCol, clamp(foam, 0.0, 0.85));

          // Smooth shoreline transparency & exponential valley fog
          float alpha = smoothstep(0.0, 0.22, vDepth) * mix(0.84, 0.96, depthT);
          float fogDist = length(cameraPosition - vWorldPos);
          float fogFactor = 1.0 - exp(-fogDist * fogDist * uFogDensity * uFogDensity);
          col = mix(col, uFogColor, clamp(fogFactor, 0.0, 1.0));

          gl_FragColor = vec4(col, alpha);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }
      `,
    });

    const waterMesh = new THREE.Mesh(waterGeo, waterMat);
    waterMesh.position.set(LANDMARKS.lake.x, WORLD.WATER_LEVEL, LANDMARKS.lake.z);
    waterMesh.renderOrder = 2;
    worldGroup.add(waterMesh);
  }

  // ==========================================================================
  // 3. ARCHED STONE BRIDGE (LANDMARKS.bridge: x=0, z=19..-35)
  // ==========================================================================
  {
    const bridgeGroup = new THREE.Group();
    const b = LANDMARKS.bridge;
    const totalLen = b.zStart - b.zEnd; // 54
    const segCount = 27;
    const segLen = totalLen / segCount;
    const fullW = b.halfWidth * 2;

    // Segmented paved stone deck following getBridgeDeckHeight(z)
    const deckGeo = scaleGeometryUVs(new THREE.BoxGeometry(fullW, 0.68, segLen + 0.18), 1.4, 0.45);
    const curbGeo = scaleGeometryUVs(new THREE.BoxGeometry(0.44, 0.28, segLen + 0.16), 0.2, 0.4);
    const parapetGeo = scaleGeometryUVs(new THREE.BoxGeometry(0.52, 1.05, segLen + 0.12), 0.25, 0.45);
    const copingGeo = new THREE.BoxGeometry(0.68, 0.18, segLen + 0.15);

    const deckInst = new THREE.InstancedMesh(deckGeo, stoneMat, segCount);
    const curbInst = new THREE.InstancedMesh(curbGeo, stoneDarkMat, segCount * 2);
    const paraInst = new THREE.InstancedMesh(parapetGeo, stoneMat, segCount * 2);
    const copeInst = new THREE.InstancedMesh(copingGeo, stoneDarkMat, segCount * 2);
    deckInst.receiveShadow = true;
    deckInst.castShadow = true;
    curbInst.castShadow = false;
    curbInst.receiveShadow = true;
    paraInst.castShadow = true;
    paraInst.receiveShadow = true;
    copeInst.castShadow = false;
    copeInst.receiveShadow = true;

    let sideIdx = 0;
    for (let i = 0; i < segCount; i++) {
      const z0 = b.zStart - i * segLen;
      const z1 = z0 - segLen;
      const zMid = (z0 + z1) * 0.5;
      const y0 = getBridgeDeckHeight(z0);
      const y1 = getBridgeDeckHeight(z1);
      const yMid = getBridgeDeckHeight(zMid);
      const pitch = Math.atan2(y1 - y0, segLen);

      _euler.set(pitch, 0, 0);
      _quat.setFromEuler(_euler);
      _scale.set(1, 1, 1);

      // Deck slab top sits precisely at getBridgeDeckHeight(zMid)
      _v1.set(b.x, yMid - 0.34, zMid);
      _mat4.compose(_v1, _quat, _scale);
      deckInst.setMatrixAt(i, _mat4);

      for (let side = -1; side <= 1; side += 2) {
        const sx = b.x + side * (b.halfWidth - 0.26);
        _v1.set(sx, yMid + 0.08, zMid);
        _mat4.compose(_v1, _quat, _scale);
        curbInst.setMatrixAt(sideIdx, _mat4);

        _v1.set(sx, yMid + 0.52, zMid);
        _mat4.compose(_v1, _quat, _scale);
        paraInst.setMatrixAt(sideIdx, _mat4);

        _v1.set(sx, yMid + 1.09, zMid);
        _mat4.compose(_v1, _quat, _scale);
        copeInst.setMatrixAt(sideIdx, _mat4);
        sideIdx++;
      }
    }
    deckInst.instanceMatrix.needsUpdate = true;
    curbInst.instanceMatrix.needsUpdate = true;
    paraInst.instanceMatrix.needsUpdate = true;
    copeInst.instanceMatrix.needsUpdate = true;
    deckInst.computeBoundingSphere();
    curbInst.computeBoundingSphere();
    paraInst.computeBoundingSphere();
    copeInst.computeBoundingSphere();
    bridgeGroup.add(deckInst, curbInst, paraInst, copeInst);

    // Crenellated merlons along bridge parapets (1 InstancedMesh, no shadow cast on small trim)
    const merlonGeo = new THREE.BoxGeometry(0.56, 0.42, 0.85);
    const merlonMats = [];
    for (let z = b.zStart - 2.0; z >= b.zEnd + 2.0; z -= 2.7) {
      const ym = getBridgeDeckHeight(z);
      for (let side = -1; side <= 1; side += 2) {
        merlonMats.push(makeTransformMatrix(b.x + side * (b.halfWidth - 0.26), ym + 1.38, z));
      }
    }
    bridgeGroup.add(buildInstancedMesh(merlonGeo, stoneMat, merlonMats, false, true));

    // Stone piers plunging into Mirror Lake + cutwaters + caps (instanced!)
    const pierZs = [19.0, 5.5, -8.0, -21.5, -35.0];
    const pierShaftGeo = scaleGeometryUVs(new THREE.BoxGeometry(fullW + 0.8, 1.0, 2.2), 1.6, 1.2);
    const cutwaterGeo = new THREE.CylinderGeometry(1.15, 1.35, 1.0, 6);
    const pierCapGeo = new THREE.BoxGeometry(fullW + 1.3, 0.38, 2.65);

    const pierShaftMats = [];
    const cutwaterMats = [];
    const pierCapMats = [];

    for (let p = 0; p < pierZs.length; p++) {
      const pz = pierZs[p];
      const topY = getBridgeDeckHeight(pz) - 0.35;
      const botY = -5.8;
      const h = topY - botY;

      pierShaftMats.push(makeTransformMatrix(b.x, botY + h * 0.5, pz, 0, 0, 0, 1, h, 1));
      for (let side = -1; side <= 1; side += 2) {
        cutwaterMats.push(
          makeTransformMatrix(b.x + side * (b.halfWidth + 0.45), botY + h * 0.36, pz, 0, 0, 0, 0.95, h * 0.72, 1.05)
        );
      }
      pierCapMats.push(makeTransformMatrix(b.x, topY - 0.12, pz));
    }
    bridgeGroup.add(
      buildInstancedMesh(pierShaftGeo, stoneDarkMat, pierShaftMats, true, true),
      buildInstancedMesh(cutwaterGeo, stoneMat, cutwaterMats, false, true),
      buildInstancedMesh(pierCapGeo, stoneMat, pierCapMats, false, true)
    );

    // Semicircular stone arch ribs & solid spandrel masonry between consecutive piers
    const voussoirGeo = new THREE.BoxGeometry(fullW - 0.16, 0.62, 1.32);
    const spandrelGeo = new THREE.BoxGeometry(fullW - 0.36, 1.0, 1.24);
    const ribArcSteps = 12;
    const totalVoussoirs = (pierZs.length - 1) * ribArcSteps;
    const archInst = new THREE.InstancedMesh(voussoirGeo, stoneDarkMat, totalVoussoirs);
    const spandrelInst = new THREE.InstancedMesh(spandrelGeo, stoneMat, totalVoussoirs);
    archInst.castShadow = true;
    archInst.receiveShadow = true;
    spandrelInst.castShadow = true;
    spandrelInst.receiveShadow = true;

    let aIdx = 0;
    for (let s = 0; s < pierZs.length - 1; s++) {
      const zA = pierZs[s];
      const zB = pierZs[s + 1];
      const zMid = (zA + zB) * 0.5;
      const halfSpan = Math.abs(zA - zB) * 0.5 - 0.9;
      const deckMid = getBridgeDeckHeight(zMid) - 0.58;
      const springY = Math.min(getBridgeDeckHeight(zA), getBridgeDeckHeight(zB)) - 2.9;
      const archRise = Math.max(1.4, deckMid - springY);

      for (let k = 0; k < ribArcSteps; k++) {
        const ang = ((k + 0.5) / ribArcSteps) * Math.PI;
        const az = zMid + Math.cos(ang) * halfSpan;
        const ay = springY + Math.sin(ang) * archRise;
        const tangentAng = Math.atan2(Math.cos(ang) * archRise, Math.sin(ang) * halfSpan);

        _euler.set(tangentAng, 0, 0);
        _quat.setFromEuler(_euler);
        _v1.set(b.x, ay, az);
        _scale.set(1, 1, 1);
        _mat4.compose(_v1, _quat, _scale);
        archInst.setMatrixAt(aIdx, _mat4);

        const deckAtZ = getBridgeDeckHeight(az) - 0.34;
        const fillH = Math.max(0.15, deckAtZ - ay);
        _quat.identity();
        _v1.set(b.x, ay + fillH * 0.5, az);
        _scale.set(1, fillH, 1);
        _mat4.compose(_v1, _quat, _scale);
        spandrelInst.setMatrixAt(aIdx, _mat4);
        aIdx++;
      }
    }
    archInst.instanceMatrix.needsUpdate = true;
    spandrelInst.instanceMatrix.needsUpdate = true;
    archInst.computeBoundingSphere();
    spandrelInst.computeBoundingSphere();
    bridgeGroup.add(archInst, spandrelInst);

    // 4 Monumental Pillar Pedestals + 2 Center Pillars + 4 Glowing Iron Braziers (instanced!)
    const brazierZs = [17.0, -33.0];
    const pillarGeo = scaleGeometryUVs(new THREE.BoxGeometry(1.18, 2.05, 1.18), 0.5, 0.8);
    const pillarCapGeo = new THREE.BoxGeometry(1.42, 0.24, 1.42);
    const bridgePillarMats = [];
    const bridgeCapMats = [];
    let bPhase = 0;

    for (let i = 0; i < brazierZs.length; i++) {
      const bz = brazierZs[i];
      const by = getBridgeDeckHeight(bz);
      for (let side = -1; side <= 1; side += 2) {
        const bx = b.x + side * (b.halfWidth - 0.18);
        bridgePillarMats.push(makeTransformMatrix(bx, by + 1.0, bz));
        bridgeCapMats.push(makeTransformMatrix(bx, by + 2.1, bz));
        registerBrazier(bx, by + 2.22, bz, 0.72, bPhase++);
      }
    }

    const midY = getBridgeDeckHeight(-8.0);
    for (let side = -1; side <= 1; side += 2) {
      bridgePillarMats.push(makeTransformMatrix(b.x + side * (b.halfWidth - 0.18), midY + 0.85, -8.0, 0, 0, 0, 0.85, 0.85, 0.85));
    }

    bridgeGroup.add(
      buildInstancedMesh(pillarGeo, stoneMat, bridgePillarMats, true, true),
      buildInstancedMesh(pillarCapGeo, stoneDarkMat, bridgeCapMats, false, true)
    );

    const bridgeLight = new THREE.PointLight(0xff9238, 26, 28, 1.5);
    bridgeLight.position.set(0, midY + 3.2, -8.0);
    bridgeGroup.add(bridgeLight);
    flickeringLights.push({ light: bridgeLight, base: 26, amp: 4.5, phase: 1.1, nightBoost: true });

    worldGroup.add(bridgeGroup);
  }

  // ==========================================================================
  // 4. OAKHAVEN VILLAGE (LANDMARKS.village at (0, 42))
  // ==========================================================================
  const lanternHaloMat = createBillboardGlowMaterial(textures.softGlow, 0xffb858, 0.65, true);

  {
    const villageGroup = new THREE.Group();

    const cottageWallMat = new THREE.MeshStandardMaterial({
      map: textures.cottageWall,
      roughness: 0.88,
      metalness: 0.02,
    });
    const cottageRoofMat = new THREE.MeshStandardMaterial({
      map: textures.cottageRoof,
      normalMap: textures.roofNormal,
      normalScale: new THREE.Vector2(0.9, 0.9),
      roughness: 0.82,
      metalness: 0.04,
    });
    const cottageRoofAltMat = new THREE.MeshStandardMaterial({
      map: textures.cottageRoof,
      normalMap: textures.roofNormal,
      normalScale: new THREE.Vector2(0.9, 0.9),
      color: 0xd0a894,
      roughness: 0.82,
      metalness: 0.04,
    });

    // Instanced collectors for cottages, fences, and village accents (slashes ~260 draw calls to ~16!)
    const villPlinthMats = [];
    const villBodyMats = [];
    const villChimneyMats = [];
    const villRoofMainMats = [];
    const villRoofAltMats = [];
    const villDarkWoodMats = [];
    const villWarmWoodMats = [];
    const villStoneDarkTrimMats = [];
    const villWinGlassMats = [];
    const villPorchLanternMats = [];

    function buildCottage(cfg) {
      const { x, z, w, d, h, doorSide, hasSmoke, roofAlt } = cfg;
      const groundY = getTerrainHeight(x, z);
      const baseM = makeTransformMatrix(x, groundY, z);

      // Stone foundation plinth sunken into terrain
      pushChildMatrix(villPlinthMats, baseM, 0, -0.35, 0, 0, 0, 0, w + 0.35, 1.6, d + 0.35);

      // Timber & plaster main body
      pushChildMatrix(villBodyMats, baseM, 0, 0.45 + h * 0.5, 0, 0, 0, 0, w, h, d);

      // Dark oak corner posts
      for (let sx = -1; sx <= 1; sx += 2) {
        for (let sz = -1; sz <= 1; sz += 2) {
          pushChildMatrix(
            villDarkWoodMats,
            baseM,
            sx * (w * 0.5 - 0.04),
            0.45 + h * 0.5,
            sz * (d * 0.5 - 0.04),
            0,
            0,
            0,
            0.28,
            h + 0.1,
            0.28
          );
        }
      }

      // Pitched Gable Roof (ridge runs along the longer horizontal axis)
      const ridgeAlongX = w >= d;
      const span = ridgeAlongX ? d : w;
      const length = ridgeAlongX ? w : d;
      const roofH = span * 0.46;
      const eaveY = 0.45 + h;

      // Triangular gable wall end-caps
      const gableShape = new THREE.Shape();
      gableShape.moveTo(-span * 0.5, 0);
      gableShape.lineTo(span * 0.5, 0);
      gableShape.lineTo(0, roofH);
      gableShape.closePath();

      const gableGeo = new THREE.ExtrudeGeometry(gableShape, { depth: length, bevelEnabled: false });
      gableGeo.translate(0, 0, -length * 0.5);
      const gableMesh = new THREE.Mesh(gableGeo, cottageWallMat);
      gableMesh.position.set(x, groundY + eaveY, z);
      if (ridgeAlongX) gableMesh.rotation.y = Math.PI * 0.5;
      gableMesh.castShadow = true;
      gableMesh.receiveShadow = true;
      villageGroup.add(gableMesh);

      // Overhanging pitched shingle roof planes (left & right slopes)
      const slopeLen = Math.hypot(span * 0.5 + 0.65, roofH + 0.3);
      const slopeAng = Math.atan2(roofH, span * 0.5);
      const roofLen = length + 1.15;
      const targetRoofList = roofAlt ? villRoofAltMats : villRoofMainMats;
      const slopeParentM = makeTransformMatrix(
        x,
        groundY + eaveY + roofH + 0.06,
        z,
        0,
        ridgeAlongX ? Math.PI * 0.5 : 0,
        0
      );
      for (let s = -1; s <= 1; s += 2) {
        pushChildMatrix(
          targetRoofList,
          slopeParentM,
          s * Math.cos(slopeAng) * slopeLen * 0.48,
          -Math.sin(slopeAng) * slopeLen * 0.48,
          0,
          0,
          0,
          -s * slopeAng,
          slopeLen,
          0.22,
          roofLen
        );
      }

      // Timber ridge cap beam
      pushChildMatrix(
        villDarkWoodMats,
        baseM,
        0,
        eaveY + roofH + 0.14,
        0,
        0,
        0,
        0,
        ridgeAlongX ? roofLen + 0.1 : 0.3,
        0.28,
        ridgeAlongX ? 0.3 : roofLen + 0.1
      );

      // Stone chimney & cap
      const chimX = ridgeAlongX ? w * 0.28 : -w * 0.3;
      const chimZ = ridgeAlongX ? -d * 0.26 : d * 0.24;
      const chimH = h + roofH + 1.45;
      pushChildMatrix(villChimneyMats, baseM, chimX, 0.45 + chimH * 0.5, chimZ, 0, 0, 0, 1.05, chimH, 1.05);
      pushChildMatrix(villStoneDarkTrimMats, baseM, chimX, 0.45 + chimH + 0.1, chimZ, 0, 0, 0, 1.3, 0.24, 1.3);

      // Register rising chimney smoke wisps for selected cottages (batched into 1 InstancedMesh!)
      if (hasSmoke) {
        for (let k = 0; k < 5; k++) {
          smokeWisps.push({
            baseX: x + chimX,
            baseY: groundY + 0.45 + chimH + 0.35,
            baseZ: z + chimZ,
            phase: k * 0.2 + x * 0.05,
          });
        }
      }

      // Recessed wooden door + doorstep + porch lantern
      let doorX = 0;
      let doorZ = 0;
      let doorRot = 0;
      if (doorSide === '+X') {
        doorX = w * 0.5 + 0.04;
        doorRot = Math.PI * 0.5;
      } else if (doorSide === '-X') {
        doorX = -w * 0.5 - 0.04;
        doorRot = -Math.PI * 0.5;
      } else if (doorSide === '+Z') {
        doorZ = d * 0.5 + 0.04;
      } else {
        doorZ = -d * 0.5 - 0.04;
        doorRot = Math.PI;
      }
      const doorM = makeTransformMatrix(x + doorX, groundY + 0.45, z + doorZ, 0, doorRot, 0);
      pushChildMatrix(villDarkWoodMats, doorM, 0, 1.16, 0, 0, 0, 0, 1.48, 2.32, 0.22);
      pushChildMatrix(villWarmWoodMats, doorM, 0, 1.08, 0, 0, 0, 0, 1.16, 2.1, 0.26);
      pushChildMatrix(villStoneDarkTrimMats, doorM, 0, -0.05, 0.42, 0, 0, 0, 1.8, 0.34, 0.9);
      pushChildMatrix(villPorchLanternMats, doorM, 0.92, 2.05, 0.28, 0, 0, 0, 0.24, 0.34, 0.24);

      // Glowing amber multi-pane windows with wooden sills
      function addWindow(wx, wy, wz, rotY) {
        const winM = makeTransformMatrix(x + wx, groundY + wy, z + wz, 0, rotY, 0);
        pushChildMatrix(villDarkWoodMats, winM, 0, 0, 0, 0, 0, 0, 1.12, 1.18, 0.14);
        pushChildMatrix(villWinGlassMats, winM, 0, 0, 0, 0, 0, 0, 0.86, 0.92, 0.18);
        pushChildMatrix(villDarkWoodMats, winM, 0, 0, 0, 0, 0, 0, 0.92, 0.08, 0.22);
        pushChildMatrix(villDarkWoodMats, winM, 0, 0, 0, 0, 0, 0, 0.08, 0.96, 0.22);
        pushChildMatrix(villDarkWoodMats, winM, 0, -0.6, 0.08, 0, 0, 0, 1.28, 0.14, 0.3);
      }

      const winY = 0.45 + h * 0.56;
      if (doorSide === '+X' || doorSide === '-X') {
        const sx = doorSide === '+X' ? w * 0.5 + 0.02 : -w * 0.5 - 0.02;
        const ry = doorSide === '+X' ? Math.PI * 0.5 : -Math.PI * 0.5;
        addWindow(sx, winY, -d * 0.28, ry);
        addWindow(sx, winY, d * 0.28, ry);
        addWindow(0, winY, d * 0.5 + 0.02, 0);
        addWindow(0, winY, -d * 0.5 - 0.02, Math.PI);
      } else {
        const sz = doorSide === '+Z' ? d * 0.5 + 0.02 : -d * 0.5 - 0.02;
        const ry = doorSide === '+Z' ? 0 : Math.PI;
        addWindow(-w * 0.28, winY, sz, ry);
        addWindow(w * 0.28, winY, sz, ry);
        addWindow(w * 0.5 + 0.02, winY, 0, Math.PI * 0.5);
        addWindow(-w * 0.5 - 0.02, winY, 0, -Math.PI * 0.5);
      }

      // Register accurate AABB collider
      addBoxCollider(x - w * 0.5 - 0.25, x + w * 0.5 + 0.25, z - d * 0.5 - 0.25, z + d * 0.5 + 0.25, groundY - 1, groundY + h + roofH + 2);
    }

    // 6 arranged cottages around Oakhaven Square (roads, spawn, & forge completely clear!)
    const cottages = [
      { x: -21.0, z: 40.5, w: 7.4, d: 5.8, h: 3.4, doorSide: '+X', hasSmoke: true, roofAlt: false },
      { x: -12.5, z: 26.5, w: 6.8, d: 5.6, h: 3.2, doorSide: '+Z', hasSmoke: false, roofAlt: true },
      { x: 12.5, z: 25.5, w: 7.2, d: 5.8, h: 3.3, doorSide: '+Z', hasSmoke: true, roofAlt: false },
      { x: 18.5, z: 43.0, w: 6.8, d: 8.6, h: 4.1, doorSide: '-X', hasSmoke: true, roofAlt: true },
      { x: 14.5, z: 54.5, w: 7.2, d: 5.8, h: 3.3, doorSide: '-X', hasSmoke: false, roofAlt: false },
      { x: -13.5, z: 58.0, w: 8.2, d: 6.0, h: 3.8, doorSide: '+X', hasSmoke: false, roofAlt: true },
    ];
    for (let i = 0; i < cottages.length; i++) buildCottage(cottages[i]);

    // Village Stone Well relocated off center boulevard to (-8, 39) for unobstructed North vista!
    {
      const wx = -8;
      const wz = 39;
      const wy = getTerrainHeight(wx, wz);
      const wellG = new THREE.Group();
      wellG.position.set(wx, wy, wz);

      const baseRing = new THREE.Mesh(new THREE.CylinderGeometry(1.55, 1.75, 1.05, 12), stoneMat);
      baseRing.position.y = 0.45;
      baseRing.castShadow = true;
      baseRing.receiveShadow = true;
      wellG.add(baseRing);

      const wellRim = new THREE.Mesh(new THREE.TorusGeometry(1.48, 0.18, 8, 16), stoneDarkMat);
      wellRim.rotation.x = Math.PI * 0.5;
      wellRim.position.y = 0.98;
      wellG.add(wellRim);

      const wellWater = new THREE.Mesh(
        new THREE.CircleGeometry(1.3, 12),
        new THREE.MeshStandardMaterial({ color: 0x164a5c, roughness: 0.15, metalness: 0.4 })
      );
      wellWater.rotation.x = -Math.PI * 0.5;
      wellWater.position.y = 0.65;
      wellG.add(wellWater);

      const wellM = makeTransformMatrix(wx, wy, wz);
      for (let s = -1; s <= 1; s += 2) {
        pushChildMatrix(villDarkWoodMats, wellM, s * 1.38, 1.95, 0, 0, 0, 0, 0.22, 2.35, 0.22);
      }

      const windlass = new THREE.Mesh(new THREE.CylinderGeometry(0.1, 0.1, 2.9, 8), warmWoodMat);
      windlass.rotation.z = Math.PI * 0.5;
      windlass.position.y = 2.05;
      wellG.add(windlass);

      const bucket = new THREE.Mesh(new THREE.CylinderGeometry(0.24, 0.18, 0.36, 8), warmWoodMat);
      bucket.position.set(0.2, 1.35, 0);
      wellG.add(bucket);

      const wRoofGeo = new THREE.ConeGeometry(2.15, 1.35, 4);
      wRoofGeo.rotateY(Math.PI * 0.25);
      const wRoof = new THREE.Mesh(wRoofGeo, cottageRoofMat);
      wRoof.position.y = 3.55;
      wRoof.castShadow = true;
      wellG.add(wRoof);

      villageGroup.add(wellG);
      addCircleCollider(wx, wz, 1.8, wy - 1, wy + 5);
    }

    // Blacksmith Forge Awning, Glowing Coal Bed & Iron Anvil near (-14, 48)
    {
      const fx = -14;
      const fz = 48;
      const fy = getTerrainHeight(fx, fz);
      const forgeG = new THREE.Group();
      forgeG.position.set(fx, fy, fz);
      const forgeM = makeTransformMatrix(fx, fy, fz);

      const apron = new THREE.Mesh(new THREE.BoxGeometry(5.6, 0.28, 4.6), stoneDarkMat);
      apron.position.y = 0.08;
      apron.receiveShadow = true;
      forgeG.add(apron);

      // 4 Timber awning posts batched into village darkWoodInst
      for (let sx = -1; sx <= 1; sx += 2) {
        for (let sz = -1; sz <= 1; sz += 2) {
          const postH = sx < 0 ? 3.5 : 2.85;
          pushChildMatrix(villDarkWoodMats, forgeM, sx * 2.45, postH * 0.5, sz * 1.95, 0, 0, 0, 0.24, postH, 0.24);
        }
      }

      const awning = new THREE.Mesh(new THREE.BoxGeometry(6.1, 0.22, 4.9), cottageRoofMat);
      awning.position.set(0, 3.25, 0);
      awning.rotation.z = -0.14;
      awning.castShadow = true;
      forgeG.add(awning);

      const hearthBase = new THREE.Mesh(new THREE.BoxGeometry(1.75, 1.15, 2.3), stoneMat);
      hearthBase.position.set(-1.55, 0.62, 0);
      hearthBase.castShadow = true;
      forgeG.add(hearthBase);

      const hood = new THREE.Mesh(new THREE.CylinderGeometry(0.55, 1.1, 2.6, 4), stoneDarkMat);
      hood.rotation.y = Math.PI * 0.25;
      hood.position.set(-1.65, 2.95, 0);
      hood.castShadow = true;
      forgeG.add(hood);

      const coalBed = new THREE.Mesh(new THREE.BoxGeometry(1.15, 0.28, 1.55), coalMat);
      coalBed.position.set(-1.35, 1.24, 0);
      forgeG.add(coalBed);

      flameSpecs.push({ x: fx - 1.35, y: fy + 1.3, z: fz, scale: 0.78, phase: 2.7 });

      // Iron Anvil on sturdy oak stump
      const stump = new THREE.Mesh(new THREE.CylinderGeometry(0.36, 0.42, 0.75, 8), darkWoodMat);
      stump.position.set(0.85, 0.45, 0.2);
      forgeG.add(stump);

      const anvilBase = new THREE.Mesh(new THREE.BoxGeometry(0.58, 0.16, 0.42), ironMat);
      anvilBase.position.set(0.85, 0.88, 0.2);
      forgeG.add(anvilBase);
      const anvilWaist = new THREE.Mesh(new THREE.BoxGeometry(0.34, 0.24, 0.26), ironMat);
      anvilWaist.position.set(0.85, 1.06, 0.2);
      forgeG.add(anvilWaist);
      const anvilTop = new THREE.Mesh(new THREE.BoxGeometry(0.86, 0.2, 0.36), ironMat);
      anvilTop.position.set(0.85, 1.24, 0.2);
      forgeG.add(anvilTop);
      const anvilHorn = new THREE.Mesh(new THREE.ConeGeometry(0.16, 0.46, 6), ironMat);
      anvilHorn.rotation.z = -Math.PI * 0.5;
      anvilHorn.position.set(1.45, 1.24, 0.2);
      forgeG.add(anvilHorn);

      const barrel = new THREE.Mesh(new THREE.CylinderGeometry(0.36, 0.34, 0.82, 10), warmWoodMat);
      barrel.position.set(1.35, 0.48, -1.25);
      forgeG.add(barrel);

      const forgeLight = new THREE.PointLight(0xff7728, 32, 24, 1.5);
      forgeLight.position.set(-0.6, 2.3, 0);
      forgeG.add(forgeLight);
      flickeringLights.push({ light: forgeLight, base: 32, amp: 7.5, phase: 0.4, nightBoost: false });

      villageGroup.add(forgeG);
      addBoxCollider(fx - 2.6, fx - 0.5, fz - 1.35, fz + 1.35, fy - 1, fy + 4);
      addCircleCollider(fx + 0.85, fz + 0.2, 0.65, fy - 1, fy + 2.5);
    }

    // Market Stall at (9.8, 39.2)
    {
      const mx = 9.8;
      const mz = 39.2;
      const my = getTerrainHeight(mx, mz);
      const stallG = new THREE.Group();
      stallG.position.set(mx, my, mz);
      stallG.rotation.y = -Math.PI * 0.5;
      const stallM = makeTransformMatrix(mx, my, mz, 0, -Math.PI * 0.5, 0);

      const counter = new THREE.Mesh(new THREE.BoxGeometry(3.8, 1.0, 1.35), warmWoodMat);
      counter.position.y = 0.5;
      counter.castShadow = true;
      stallG.add(counter);

      for (let sx = -1; sx <= 1; sx += 2) {
        pushChildMatrix(villDarkWoodMats, stallM, sx * 1.85, 1.35, 0.62, 0, 0, 0, 0.16, 2.7, 0.16);
        pushChildMatrix(villDarkWoodMats, stallM, sx * 1.85, 1.18, -0.62, 0, 0, 0, 0.16, 2.35, 0.16);
      }

      const redCanvasMat = new THREE.MeshStandardMaterial({ color: PALETTE.crimson, roughness: 0.86 });
      const creamCanvasMat = new THREE.MeshStandardMaterial({ color: 0xe9dec8, roughness: 0.86 });
      const redStripeMats = [];
      const creamStripeMats = [];
      for (let s = 0; s < 5; s++) {
        pushChildMatrix(s % 2 === 0 ? redStripeMats : creamStripeMats, stallM, -1.68 + s * 0.84, 2.6, 0.08, 0.18, 0, 0, 0.84, 0.1, 2.05);
      }
      villageGroup.add(
        buildInstancedMesh(unitBoxGeo, redCanvasMat, redStripeMats, true, false),
        buildInstancedMesh(unitBoxGeo, creamCanvasMat, creamStripeMats, true, false)
      );

      pushChildMatrix(villWarmWoodMats, stallM, -1.1, 1.28, 0, 0, 0, 0, 0.68, 0.58, 0.68);
      const produceMat = new THREE.MeshStandardMaterial({ color: 0xd99636, roughness: 0.7 });
      const basket = new THREE.Mesh(new THREE.SphereGeometry(0.34, 8, 6), produceMat);
      basket.scale.set(1.2, 0.65, 1.1);
      basket.position.set(0.35, 1.16, 0.05);
      stallG.add(basket);

      villageGroup.add(stallG);
      addBoxCollider(mx - 0.9, mx + 0.9, mz - 2.0, mz + 2.0, my - 1, my + 3.2);
    }

    // Wooden Fence Accents bordering village gardens (batched into village instanced wood!)
    {
      const fenceSegments = [
        [-24.5, 46.5, -18.5, 46.5],
        [-22.0, 54.0, -18.5, 57.5],
        [18.5, 50.0, 22.5, 50.0],
        [17.5, 31.5, 22.0, 31.5],
        [-17.5, 31.0, -21.5, 34.0],
      ];
      for (let i = 0; i < fenceSegments.length; i++) {
        const [x1, z1, x2, z2] = fenceSegments[i];
        const len = Math.hypot(x2 - x1, z2 - z1);
        const cx = (x1 + x2) * 0.5;
        const cz = (z1 + z2) * 0.5;
        const cy = getTerrainHeight(cx, cz);
        const ang = Math.atan2(x2 - x1, z2 - z1);
        const fM = makeTransformMatrix(cx, cy, cz, 0, ang, 0);

        const posts = Math.max(2, Math.round(len / 1.8) + 1);
        for (let p = 0; p < posts; p++) {
          const pz = -len * 0.5 + (p / (posts - 1)) * len;
          pushChildMatrix(villDarkWoodMats, fM, 0, 0.58, pz, 0, 0, 0, 0.18, 1.25, 0.18);
        }
        pushChildMatrix(villWarmWoodMats, fM, 0, 0.96, 0, 0, 0, 0, 0.1, 0.12, len);
        pushChildMatrix(villWarmWoodMats, fM, 0, 0.52, 0, 0, 0, 0, 0.1, 0.12, len);
      }
    }

    // Flush village batched InstancedMeshes (castShadow true on main cottage volumes, false on small trim/windows/fences!)
    villageGroup.add(
      buildInstancedMesh(unitBoxGeo, stoneDarkMat, villPlinthMats, true, true),
      buildInstancedMesh(unitBoxGeo, cottageWallMat, villBodyMats, true, true),
      buildInstancedMesh(unitBoxGeo, stoneMat, villChimneyMats, true, true),
      buildInstancedMesh(unitBoxGeo, cottageRoofMat, villRoofMainMats, true, true),
      buildInstancedMesh(unitBoxGeo, cottageRoofAltMat, villRoofAltMats, true, true),
      buildInstancedMesh(unitBoxGeo, darkWoodMat, villDarkWoodMats, false, false),
      buildInstancedMesh(unitBoxGeo, warmWoodMat, villWarmWoodMats, false, false),
      buildInstancedMesh(unitBoxGeo, stoneDarkMat, villStoneDarkTrimMats, false, true),
      buildInstancedMesh(unitBoxGeo, cottageWindowMat, villWinGlassMats, false, false),
      buildInstancedMesh(unitBoxGeo, lanternGlassMat, villPorchLanternMats, false, false)
    );

    // 6 Warm Glowing Village Street Lanterns (fully instanced, castShadow = false!)
    // Lantern #3 moved from (-8.8, 41.0) to (-8.2, 44.5) to give clear space around relocated well at (-8, 39)
    const lanternCoords = [
      [-6.5, 51.5],
      [6.5, 51.5],
      [-8.2, 44.5],
      [8.2, 44.5],
      [-6.8, 30.5],
      [6.8, 30.5],
    ];
    const lanPostMats = [];
    const lanArmMats = [];
    const lanCapMats = [];
    const lanGlassMats = [];
    const lanHaloMats = [];

    for (let i = 0; i < lanternCoords.length; i++) {
      const [lx, lz] = lanternCoords[i];
      const ly = getTerrainHeight(lx, lz);
      const rotY = Math.atan2(-lx, 42 - lz) - Math.PI * 0.5;
      const lM = makeTransformMatrix(lx, ly, lz, 0, rotY, 0);

      pushChildMatrix(lanPostMats, lM, 0, 1.8, 0);
      pushChildMatrix(lanArmMats, lM, 0.32, 3.48, 0, 0, 0, 0, 0.75, 0.09, 0.09);
      pushChildMatrix(lanCapMats, lM, 0.62, 3.42, 0);
      pushChildMatrix(lanGlassMats, lM, 0.62, 3.12, 0);
      pushChildMatrix(lanHaloMats, lM, 0.62, 3.12, 0, 0, 0, 0, 2.4, 2.4, 1);

      addCircleCollider(lx, lz, 0.35, ly - 1, ly + 4);
    }

    const lanPostGeo = new THREE.CylinderGeometry(0.09, 0.13, 3.6, 7);
    const lanCapGeo = new THREE.ConeGeometry(0.32, 0.24, 6);
    const lanGlassGeo = new THREE.CylinderGeometry(0.19, 0.14, 0.42, 6);

    const lanHaloInst = buildInstancedMesh(billboardQuadGeo, lanternHaloMat, lanHaloMats, false, false);
    if (lanHaloInst) lanHaloInst.renderOrder = 3;

    villageGroup.add(
      buildInstancedMesh(lanPostGeo, ironMat, lanPostMats, false, false),
      buildInstancedMesh(unitBoxGeo, ironMat, lanArmMats, false, false),
      buildInstancedMesh(lanCapGeo, ironMat, lanCapMats, false, false),
      buildInstancedMesh(lanGlassGeo, lanternGlassMat, lanGlassMats, false, false),
      lanHaloInst
    );

    // Warm Village Square PointLight (strengthens at dusk/night)
    const squareLight = new THREE.PointLight(0xffb454, 34, 38, 1.45);
    squareLight.position.set(0, getTerrainHeight(0, 42) + 5.8, 42);
    villageGroup.add(squareLight);
    flickeringLights.push({ light: squareLight, base: 34, amp: 2.5, phase: 2.2, nightBoost: true });

    worldGroup.add(villageGroup);
  }

  // Batched GPU-billboarded chimney smoke wisps (1 draw call for all 15 wisps!)
  let smokeInst = null;
  let smokeAlphaAttr = null;
  if (smokeWisps.length > 0) {
    const smokeGeo = billboardQuadGeo.clone();
    smokeAlphaAttr = new THREE.InstancedBufferAttribute(new Float32Array(smokeWisps.length), 1);
    smokeGeo.setAttribute('aAlpha', smokeAlphaAttr);
    const smokeMat = new THREE.ShaderMaterial({
      uniforms: {
        uMap: { value: textures.softGlow },
        uColor: { value: new THREE.Color(0xd8dce4) },
      },
      transparent: true,
      depthWrite: false,
      vertexShader: /* glsl */ `
        attribute float aAlpha;
        varying vec2 vUv;
        varying float vAlpha;
        void main() {
          vUv = uv;
          vAlpha = aAlpha;
          vec4 worldCenter = modelMatrix * instanceMatrix * vec4(0.0, 0.0, 0.0, 1.0);
          float sx = length(vec3(instanceMatrix[0][0], instanceMatrix[0][1], instanceMatrix[0][2]));
          float sy = length(vec3(instanceMatrix[1][0], instanceMatrix[1][1], instanceMatrix[1][2]));
          vec4 mvCenter = viewMatrix * worldCenter;
          mvCenter.xy += position.xy * vec2(sx, sy);
          gl_Position = projectionMatrix * mvCenter;
        }
      `,
      fragmentShader: /* glsl */ `
        uniform sampler2D uMap;
        uniform vec3 uColor;
        varying vec2 vUv;
        varying float vAlpha;
        void main() {
          vec4 tex = texture2D(uMap, vUv);
          gl_FragColor = vec4(uColor * tex.rgb, tex.a * vAlpha);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }
      `,
    });
    smokeInst = new THREE.InstancedMesh(smokeGeo, smokeMat, smokeWisps.length);
    smokeInst.castShadow = false;
    smokeInst.receiveShadow = false;
    smokeInst.frustumCulled = false;
    smokeInst.renderOrder = 3;
    worldGroup.add(smokeInst);
  }

  // Batched will-o'-wisps drifting over Mirror Lake's shore & the Rune Circle (dusk/night ambience, 1 draw call)
  const willWisps = [];
  {
    const wrng = mulberry32(771177);
    const wispCount = isMobile ? 36 : 72;
    for (let i = 0; i < wispCount; i++) {
      let wx, wz;
      if (wrng() < 0.6) {
        // Shoreline ring around Mirror Lake (outside the water ellipse, above the grass)
        const ang = wrng() * Math.PI * 2.0;
        const ringScale = 0.88 + wrng() * 0.38;
        wx = LANDMARKS.lake.x + Math.cos(ang) * LANDMARKS.lake.rx * ringScale;
        wz = LANDMARKS.lake.z + Math.sin(ang) * LANDMARKS.lake.rz * ringScale;
      } else {
        // Scattered around the Rune Circle clearing
        const ang = wrng() * Math.PI * 2.0;
        const rad = 2.5 + wrng() * 9.5;
        wx = LANDMARKS.ruins.x + Math.cos(ang) * rad;
        wz = LANDMARKS.ruins.z + Math.sin(ang) * rad;
      }
      const wy = getTerrainHeight(wx, wz) + 0.7 + wrng() * 1.9;
      willWisps.push({
        x: wx,
        y: wy,
        z: wz,
        phase: wrng() * Math.PI * 2.0,
        span: 1.1 + wrng() * 1.6,
        speed: 0.75 + wrng() * 0.8,
      });
    }
  }

  let wispInst = null;
  let wispAlphaAttr = null;
  if (willWisps.length > 0) {
    const wispGeo = billboardQuadGeo.clone();
    wispAlphaAttr = new THREE.InstancedBufferAttribute(new Float32Array(willWisps.length), 1);
    wispGeo.setAttribute('aAlpha', wispAlphaAttr);
    const wispMat = new THREE.ShaderMaterial({
      uniforms: {
        uMap: { value: textures.softGlow },
        uColor: { value: new THREE.Color(0xd4ff70) },
      },
      transparent: true,
      depthWrite: false,
      blending: THREE.AdditiveBlending,
      vertexShader: /* glsl */ `
        attribute float aAlpha;
        varying vec2 vUv;
        varying float vAlpha;
        void main() {
          vUv = uv;
          vAlpha = aAlpha;
          vec4 worldCenter = modelMatrix * instanceMatrix * vec4(0.0, 0.0, 0.0, 1.0);
          float sx = length(vec3(instanceMatrix[0][0], instanceMatrix[0][1], instanceMatrix[0][2]));
          float sy = length(vec3(instanceMatrix[1][0], instanceMatrix[1][1], instanceMatrix[1][2]));
          vec4 mvCenter = viewMatrix * worldCenter;
          mvCenter.xy += position.xy * vec2(sx, sy);
          gl_Position = projectionMatrix * mvCenter;
        }
      `,
      fragmentShader: /* glsl */ `
        uniform sampler2D uMap;
        uniform vec3 uColor;
        varying vec2 vUv;
        varying float vAlpha;
        void main() {
          vec4 tex = texture2D(uMap, vUv);
          gl_FragColor = vec4(uColor * tex.rgb, tex.a * vAlpha);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }
      `,
    });
    wispInst = new THREE.InstancedMesh(wispGeo, wispMat, willWisps.length);
    wispInst.castShadow = false;
    wispInst.receiveShadow = false;
    wispInst.frustumCulled = false;
    wispInst.renderOrder = 3;
    worldGroup.add(wispInst);
  }

  // ==========================================================================
  // 5. BLUE-LEAF ENCHANTED FOREST, BOULDERS, ANCIENT RUNE CIRCLE & CRYSTALS
  // ==========================================================================
  {
    const rng = mulberry32(42197);

    // Sculpted organic trunk geometry (root flare + gentle bend, lean 3 height segments)
    const trunkGeo = new THREE.CylinderGeometry(0.16, 0.44, 4.6, 7, 3);
    trunkGeo.translate(0, 2.3, 0);
    const tPos = trunkGeo.attributes.position;
    for (let i = 0; i < tPos.count; i++) {
      const y = tPos.getY(i);
      const t = y / 4.6;
      const bendX = Math.sin(t * Math.PI * 1.1) * 0.24;
      const bendZ = Math.cos(t * Math.PI * 0.8) * 0.14;
      const flare = y < 0.7 ? 1.0 + (0.7 - y) * 0.65 : 1.0;
      tPos.setXYZ(i, tPos.getX(i) * flare + bendX, y, tPos.getZ(i) * flare + bendZ);
    }
    trunkGeo.computeVertexNormals();

    // Sculpted painterly canopy cluster geometry (detail=1 -> 80 tris instead of 320, saves 350k+ tris!)
    const puffGeo = new THREE.IcosahedronGeometry(1.0, 1);
    const pPos = puffGeo.attributes.position;
    for (let i = 0; i < pPos.count; i++) {
      const px = pPos.getX(i);
      const py = pPos.getY(i);
      const pz = pPos.getZ(i);
      const n = 0.86 + 0.28 * Math.sin(px * 3.7 + py * 2.9) * Math.cos(pz * 3.3 - px * 2.1);
      pPos.setXYZ(i, px * n, py * (n * 0.82), pz * n);
    }
    puffGeo.computeVertexNormals();

    function makeCanopyMat(hex) {
      const m = new THREE.MeshStandardMaterial({
        color: hex,
        roughness: 0.78,
        metalness: 0.04,
      });
      m.customProgramCacheKey = () => 'aethelgard_canopy_wind';
      m.onBeforeCompile = (shader) => {
        shader.uniforms.uTime = canopyUniforms.uTime;
        shader.vertexShader = shader.vertexShader
          .replace('#include <common>', '#include <common>\nuniform float uTime;')
          .replace(
            '#include <begin_vertex>',
            `#include <begin_vertex>
            #ifdef USE_INSTANCING
              vec3 iw = vec3(instanceMatrix[3][0], instanceMatrix[3][1], instanceMatrix[3][2]);
              float sway = sin(uTime * 1.45 + iw.x * 0.11 + iw.z * 0.09) * 0.14;
              transformed.x += sway * (position.y + 1.0) * 0.55;
              transformed.z += cos(uTime * 1.15 + iw.z * 0.13) * 0.09 * (position.y + 1.0) * 0.55;
            #endif`
          );
      };
      return m;
    }

    const trunkMat = new THREE.MeshStandardMaterial({
      map: textures.bark,
      normalMap: textures.barkNormal,
      normalScale: new THREE.Vector2(1.0, 1.0),
      color: 0x9a8266,
      roughness: 0.9,
      metalness: 0.02,
    });
    const blueDarkMat = makeCanopyMat(PALETTE.blueLeafDark);
    const blueMidMat = makeCanopyMat(PALETTE.blueLeafMid);
    const blueBrightMat = makeCanopyMat(PALETTE.blueLeafBright);
    const cypressMat = makeCanopyMat(0x286346);

    const blueNear = [];
    const blueFar = [];
    const cypressNear = [];
    const cypressFar = [];
    const allTreeXY = [];

    function canPlaceTree(x, z, minSpacing = 5.4) {
      const dist = Math.hypot(x, z);
      if (dist > 178) return false;
      const y = getTerrainHeight(x, z);
      if (y < 1.35 || y > 32) return false;
      if (getTerrainSlopeY(x, z) < 0.79) return false;

      const l = LANDMARKS.lake;
      if (Math.hypot((x - l.x) / l.rx, (z - l.z) / l.rz) < 1.16) return false;
      if (getRoadWeight(x, z) > 0.14) return false;
      if (Math.hypot(x - LANDMARKS.village.x, z - LANDMARKS.village.z) < 27.5) return false;
      if (Math.hypot(x - LANDMARKS.dragonPerch.x, z - LANDMARKS.dragonPerch.z) < 13.5) return false;
      if (Math.hypot(x - LANDMARKS.ruins.x, z - LANDMARKS.ruins.z) < 13.5) return false;
      if (Math.hypot(x - LANDMARKS.castle.x, z - LANDMARKS.castle.z) < 42.5) return false;
      if (Math.abs(x) < 12 && z < 24 && z > -42) return false;
      if (x > 76 && z > -56 && z < 8) return false;

      const minSq = minSpacing * minSpacing;
      for (let i = 0; i < allTreeXY.length; i++) {
        const dx = x - allTreeXY[i].x;
        const dz = z - allTreeXY[i].z;
        if (dx * dx + dz * dz < minSq) return false;
      }
      return true;
    }

    // Place ~210 Blue-Leaf Enchanted Trees (split <= 110m shadow-casting vs > 110m non-shadow-casting)
    let attempts = 0;
    while (blueNear.length + blueFar.length < 210 && attempts < 5500) {
      attempts++;
      const angle = rng() * Math.PI * 2;
      const r = 24 + Math.sqrt(rng()) * 150;
      const x = Math.cos(angle) * r;
      const z = Math.sin(angle) * r;
      if (!canPlaceTree(x, z, 5.4)) continue;

      const y = getTerrainHeight(x, z);
      const s = 0.82 + rng() * 0.68;
      const rot = rng() * Math.PI * 2;
      const entry = { x, y, z, s, rot };
      const distOrigin = Math.hypot(x, z);
      if (distOrigin <= 110) blueNear.push(entry);
      else blueFar.push(entry);
      allTreeXY.push({ x, z });
      if (distOrigin < 158) {
        addCircleCollider(x, z, 0.7, y - 1, y + 6 * s);
      }
    }

    // Place ~32 Emerald / Cypress accent trees
    attempts = 0;
    while (cypressNear.length + cypressFar.length < 32 && attempts < 1800) {
      attempts++;
      const angle = rng() * Math.PI * 2;
      const r = 28 + rng() * 98;
      const x = Math.cos(angle) * r;
      const z = Math.sin(angle) * r + 10;
      if (!canPlaceTree(x, z, 4.8)) continue;

      const y = getTerrainHeight(x, z);
      const s = 0.85 + rng() * 0.55;
      const entry = { x, y, z, s, rot: rng() * Math.PI * 2 };
      const distOrigin = Math.hypot(x, z);
      if (distOrigin <= 110) cypressNear.push(entry);
      else cypressFar.push(entry);
      allTreeXY.push({ x, z });
      if (distOrigin < 158) {
        addCircleCollider(x, z, 0.7, y - 1, y + 7 * s);
      }
    }

    const cypressConeGeo = new THREE.ConeGeometry(1.45, 3.6, 7);

    function buildTreeBatch(bList, cList, castShadow) {
      const total = bList.length + cList.length;
      if (total === 0) return;

      const tInst = new THREE.InstancedMesh(trunkGeo, trunkMat, total);
      tInst.castShadow = castShadow;
      tInst.receiveShadow = true;

      if (bList.length > 0) {
        const dInst = new THREE.InstancedMesh(puffGeo, blueDarkMat, bList.length * 2);
        const mInst = new THREE.InstancedMesh(puffGeo, blueMidMat, bList.length * 3);
        const brInst = new THREE.InstancedMesh(puffGeo, blueBrightMat, bList.length * 2);
        dInst.castShadow = castShadow;
        mInst.castShadow = castShadow;
        brInst.castShadow = castShadow;
        dInst.receiveShadow = true;
        mInst.receiveShadow = true;

        let dIdx = 0;
        let mIdx = 0;
        let bIdx = 0;

        for (let i = 0; i < bList.length; i++) {
          const tr = bList[i];
          _euler.set((rng() - 0.5) * 0.08, tr.rot, (rng() - 0.5) * 0.08);
          _quat.setFromEuler(_euler);
          _v1.set(tr.x, tr.y - 0.35, tr.z);
          _scale.set(tr.s, tr.s * (0.95 + rng() * 0.25), tr.s);
          _mat4.compose(_v1, _quat, _scale);
          tInst.setMatrixAt(i, _mat4);
          _col1.setHex(0x453227).offsetHSL(0, 0, (rng() - 0.5) * 0.07);
          tInst.setColorAt(i, _col1);

          const crownY = tr.y + 4.1 * tr.s;

          for (let k = 0; k < 2; k++) {
            const a = tr.rot + k * Math.PI + (rng() - 0.5) * 0.7;
            _v1.set(tr.x + Math.cos(a) * 1.15 * tr.s, crownY - 0.25 * tr.s, tr.z + Math.sin(a) * 1.15 * tr.s);
            _euler.set(rng() * 0.3, rng() * Math.PI, rng() * 0.3);
            _quat.setFromEuler(_euler);
            const rad = (1.65 + rng() * 0.4) * tr.s;
            _scale.set(rad, rad * 0.84, rad);
            _mat4.compose(_v1, _quat, _scale);
            dInst.setMatrixAt(dIdx, _mat4);
            _col1.setHex(0xffffff).offsetHSL((rng() - 0.5) * 0.04, 0, (rng() - 0.5) * 0.08);
            dInst.setColorAt(dIdx++, _col1);
          }

          for (let k = 0; k < 3; k++) {
            const a = tr.rot + (k * Math.PI * 2) / 3 + 0.4;
            _v1.set(tr.x + Math.cos(a) * 1.3 * tr.s, crownY + (0.65 + k * 0.18) * tr.s, tr.z + Math.sin(a) * 1.3 * tr.s);
            _euler.set(rng() * 0.4, rng() * Math.PI, rng() * 0.4);
            _quat.setFromEuler(_euler);
            const rad = (1.55 + rng() * 0.45) * tr.s;
            _scale.set(rad, rad * 0.88, rad);
            _mat4.compose(_v1, _quat, _scale);
            mInst.setMatrixAt(mIdx, _mat4);
            _col1.setHex(0xffffff).offsetHSL((rng() - 0.5) * 0.05, 0, (rng() - 0.5) * 0.08);
            mInst.setColorAt(mIdx++, _col1);
          }

          for (let k = 0; k < 2; k++) {
            const a = tr.rot + k * 2.6 + 1.1;
            const off = k === 0 ? 0.35 : 0.95;
            _v1.set(tr.x + Math.cos(a) * off * tr.s, crownY + (1.75 - k * 0.45) * tr.s, tr.z + Math.sin(a) * off * tr.s);
            _euler.set(rng() * 0.3, rng() * Math.PI, rng() * 0.3);
            _quat.setFromEuler(_euler);
            const rad = (1.28 + rng() * 0.32) * tr.s;
            _scale.set(rad, rad * 0.82, rad);
            _mat4.compose(_v1, _quat, _scale);
            brInst.setMatrixAt(bIdx, _mat4);
            _col1.setHex(0xffffff).offsetHSL((rng() - 0.5) * 0.04, 0, (rng() - 0.5) * 0.07);
            brInst.setColorAt(bIdx++, _col1);
          }
        }

        dInst.instanceMatrix.needsUpdate = true;
        if (dInst.instanceColor) dInst.instanceColor.needsUpdate = true;
        dInst.computeBoundingSphere();
        mInst.instanceMatrix.needsUpdate = true;
        if (mInst.instanceColor) mInst.instanceColor.needsUpdate = true;
        mInst.computeBoundingSphere();
        brInst.instanceMatrix.needsUpdate = true;
        if (brInst.instanceColor) brInst.instanceColor.needsUpdate = true;
        brInst.computeBoundingSphere();
        worldGroup.add(dInst, mInst, brInst);
      }

      if (cList.length > 0) {
        const cInst = new THREE.InstancedMesh(cypressConeGeo, cypressMat, cList.length * 3);
        cInst.castShadow = castShadow;
        cInst.receiveShadow = true;

        let cIdx = 0;
        for (let i = 0; i < cList.length; i++) {
          const tr = cList[i];
          _euler.set(0, tr.rot, 0);
          _quat.setFromEuler(_euler);
          _v1.set(tr.x, tr.y - 0.3, tr.z);
          _scale.set(tr.s * 0.75, tr.s * 0.8, tr.s * 0.75);
          _mat4.compose(_v1, _quat, _scale);
          tInst.setMatrixAt(bList.length + i, _mat4);
          tInst.setColorAt(bList.length + i, _col1.setHex(0x3c2b20));

          for (let tier = 0; tier < 3; tier++) {
            const tierScale = (1.0 - tier * 0.22) * tr.s;
            _v1.set(tr.x, tr.y + (2.8 + tier * 2.1) * tr.s, tr.z);
            _euler.set(0, tr.rot + tier * 0.6, 0);
            _quat.setFromEuler(_euler);
            _scale.set(tierScale, tr.s * (1.05 - tier * 0.08), tierScale);
            _mat4.compose(_v1, _quat, _scale);
            cInst.setMatrixAt(cIdx, _mat4);
            _col1.setHex(0xffffff).offsetHSL((rng() - 0.5) * 0.03, 0, tier * 0.06 - 0.04);
            cInst.setColorAt(cIdx++, _col1);
          }
        }
        cInst.instanceMatrix.needsUpdate = true;
        if (cInst.instanceColor) cInst.instanceColor.needsUpdate = true;
        cInst.computeBoundingSphere();
        worldGroup.add(cInst);
      }

      tInst.instanceMatrix.needsUpdate = true;
      if (tInst.instanceColor) tInst.instanceColor.needsUpdate = true;
      tInst.computeBoundingSphere();
      worldGroup.add(tInst);
    }

    buildTreeBatch(blueNear, cypressNear, true);
    buildTreeBatch(blueFar, cypressFar, false);

    // Instanced Mossy Boulders & Shoreline Rocks (~150, lean 80-tri rock geometry)
    const rockGeo = new THREE.IcosahedronGeometry(1.0, 1);
    const rPos = rockGeo.attributes.position;
    const rColors = new Float32Array(rPos.count * 3);
    const cRockBase = new THREE.Color(0x525761);
    const cRockMoss = new THREE.Color(0x4c6848);
    for (let i = 0; i < rPos.count; i++) {
      const rx = rPos.getX(i);
      const ry = rPos.getY(i);
      const rz = rPos.getZ(i);
      const bump = 0.82 + 0.34 * Math.sin(rx * 4.1 + rz * 3.2) * Math.cos(ry * 3.7);
      const ny = ry < 0 ? ry * 0.62 : ry * 0.85;
      rPos.setXYZ(i, rx * bump, ny * bump, rz * bump);
      _col1.copy(cRockBase).lerp(cRockMoss, smoothstep(0.18, 0.72, ry));
      rColors[i * 3] = _col1.r;
      rColors[i * 3 + 1] = _col1.g;
      rColors[i * 3 + 2] = _col1.b;
    }
    rockGeo.setAttribute('color', new THREE.BufferAttribute(rColors, 3));
    rockGeo.computeVertexNormals();

    const rockMat = new THREE.MeshStandardMaterial({
      vertexColors: true,
      roughness: 0.9,
      metalness: 0.05,
    });

    const rockNear = [];
    const rockFar = [];
    let rTry = 0;
    while (rockNear.length + rockFar.length < 150 && rTry < 2000) {
      rTry++;
      const a = rng() * Math.PI * 2;
      const rad = 18 + Math.sqrt(rng()) * 155;
      const rx = Math.cos(a) * rad;
      const rz = Math.sin(a) * rad;
      if (getRoadWeight(rx, rz) > 0.14) continue;
      if (Math.hypot(rx - LANDMARKS.village.x, rz - LANDMARKS.village.z) < 24) continue;
      if (Math.hypot(rx - LANDMARKS.castle.x, rz - LANDMARKS.castle.z) < 36) continue;
      if (Math.hypot(rx - LANDMARKS.dragonPerch.x, rz - LANDMARKS.dragonPerch.z) < 9) continue;
      if (rx > 84 && rz > -48 && rz < 2) continue;
      const ry = getTerrainHeight(rx, rz);
      if (ry < -0.6) continue;
      const sx = 0.65 + rng() * 1.9;
      const sy = 0.45 + rng() * 1.15;
      const sz = 0.65 + rng() * 1.8;
      const rd = { rx, ry, rz, sx, sy, sz, rotY: rng() * Math.PI * 2 };
      if (Math.hypot(rx, rz) <= 110) rockNear.push(rd);
      else rockFar.push(rd);
    }

    function buildRockBatch(list, castShadow) {
      if (!list.length) return;
      const inst = new THREE.InstancedMesh(rockGeo, rockMat, list.length);
      inst.castShadow = castShadow;
      inst.receiveShadow = true;
      for (let i = 0; i < list.length; i++) {
        const rd = list[i];
        _v1.set(rd.rx, rd.ry - 0.15, rd.rz);
        _euler.set((rng() - 0.5) * 0.25, rd.rotY, (rng() - 0.5) * 0.25);
        _quat.setFromEuler(_euler);
        _scale.set(rd.sx, rd.sy, rd.sz);
        _mat4.compose(_v1, _quat, _scale);
        inst.setMatrixAt(i, _mat4);
        _col1.setHex(0xffffff).offsetHSL(0, 0, (rng() - 0.5) * 0.14);
        inst.setColorAt(i, _col1);
      }
      inst.instanceMatrix.needsUpdate = true;
      if (inst.instanceColor) inst.instanceColor.needsUpdate = true;
      inst.computeBoundingSphere();
      worldGroup.add(inst);
    }
    buildRockBatch(rockNear, true);
    buildRockBatch(rockFar, false);

    // Ancient Rune Circle at LANDMARKS.ruins (38, 12)
    {
      const rx = LANDMARKS.ruins.x;
      const rz = LANDMARKS.ruins.z;
      const ry = getTerrainHeight(rx, rz);
      const ruinsG = new THREE.Group();
      ruinsG.position.set(rx, ry, rz);

      const ringSlab = new THREE.Mesh(new THREE.RingGeometry(6.8, 9.6, 24), stoneDarkMat);
      ringSlab.rotation.x = -Math.PI * 0.5;
      ringSlab.position.y = 0.08;
      ringSlab.receiveShadow = true;
      ruinsG.add(ringSlab);

      const centerSeal = new THREE.Mesh(
        new THREE.RingGeometry(2.1, 2.55, 24),
        new THREE.MeshStandardMaterial({
          color: PALETTE.runeGold,
          emissive: PALETTE.runeGold,
          emissiveIntensity: 1.2,
          roughness: 0.4,
          side: THREE.DoubleSide,
        })
      );
      centerSeal.rotation.x = -Math.PI * 0.5;
      centerSeal.position.y = 0.1;
      ruinsG.add(centerSeal);

      // Monumental Carved Stone Archway on NE arc
      const archG = new THREE.Group();
      archG.position.set(4.5, 0, -5.8);
      archG.rotation.y = -Math.PI * 0.22;

      const archPierGeo = new THREE.BoxGeometry(1.7, 7.6, 1.5);
      for (let s = -1; s <= 1; s += 2) {
        const pier = new THREE.Mesh(archPierGeo, runePillarMat);
        pier.position.set(s * 2.8, 3.6, 0);
        pier.castShadow = true;
        pier.receiveShadow = true;
        archG.add(pier);

        const cap = new THREE.Mesh(new THREE.BoxGeometry(2.15, 0.55, 1.9), stoneDarkMat);
        cap.position.set(s * 2.8, 7.4, 0);
        archG.add(cap);
      }
      const lintel = new THREE.Mesh(new THREE.BoxGeometry(8.6, 1.45, 1.85), runePillarMat);
      lintel.position.set(0, 8.2, 0);
      lintel.castShadow = true;
      archG.add(lintel);

      const crownStone = new THREE.Mesh(new THREE.BoxGeometry(4.2, 1.1, 1.4), stoneDarkMat);
      crownStone.position.set(0, 9.25, 0);
      archG.add(crownStone);

      const archGlow = new THREE.Sprite(
        new THREE.SpriteMaterial({
          map: textures.softGlow,
          color: 0xffd278,
          transparent: true,
          opacity: 0.42,
          blending: THREE.AdditiveBlending,
          depthWrite: false,
        })
      );
      archGlow.position.set(0, 4.4, 0);
      archGlow.scale.set(6.5, 8.5, 1);
      archG.add(archGlow);
      ruinsG.add(archG);

      // 5 Tall Weathered Stone Monoliths around the circle (1 InstancedMesh!)
      const monoAngles = [0.45, 1.55, 2.55, 3.65, 4.75];
      const monoGeo = new THREE.CylinderGeometry(0.62, 0.95, 1.0, 6);
      const monoMats = [];
      for (let i = 0; i < monoAngles.length; i++) {
        const a = monoAngles[i];
        const mx = Math.cos(a) * 8.4;
        const mz = Math.sin(a) * 8.4;
        const mh = 5.4 + (i % 3) * 1.15;

        monoMats.push(
          makeTransformMatrix(
            rx + mx,
            ry + mh * 0.46,
            rz + mz,
            (i % 2 === 0 ? 1 : -1) * 0.05,
            a,
            (i % 3 === 0 ? -1 : 1) * 0.04,
            1,
            mh,
            1
          )
        );
        addCircleCollider(rx + mx, rz + mz, 1.05, ry - 1, ry + mh + 1);

        // Glowing purple crystal clusters beside monoliths (batched globally)
        registerCrystalCluster(rx + mx * 0.84, ry, rz + mz * 0.84, true, 0.85 + (i % 2) * 0.3, 800 + i * 31);
      }
      worldGroup.add(buildInstancedMesh(monoGeo, runePillarMat, monoMats, true, true));

      // Extra iconic purple crystal spire beside the stone archway
      registerCrystalCluster(rx + 8.2, ry, rz - 3.2, true, 1.35, 991);

      worldGroup.add(ruinsG);
    }

    // Dragon Perch subtle ancient stone rim ring at (-32, 22)
    {
      const dx = LANDMARKS.dragonPerch.x;
      const dz = LANDMARKS.dragonPerch.z;
      const dy = getTerrainHeight(dx, dz);
      const perchRing = new THREE.Mesh(new THREE.RingGeometry(4.8, 6.4, 18), stoneDarkMat);
      perchRing.rotation.x = -Math.PI * 0.5;
      perchRing.position.set(dx, dy + 0.06, dz);
      perchRing.receiveShadow = true;
      worldGroup.add(perchRing);
    }
  }

  // ==========================================================================
  // 6. WIND-SWAYING GRASS (LEAN 9-TRI 3-BLADE TUFT, REFINED 0.45–0.75m HEIGHT)
  // ==========================================================================
  {
    const grassCount = isMobile ? 2600 : 5500;
    const grng = mulberry32(90817);

    // Lean 3-blade 2-segment tapered tuft BufferGeometry (3 tris/blade = 9 tris/tuft!)
    const positions = [];
    const normals = [];
    const colors = [];
    const indices = [];

    const cBase = new THREE.Color(0x1f4126);
    const cMid = new THREE.Color(0x437c3d);
    const cTip = new THREE.Color(0x9ac052);

    let vOffset = 0;
    const bladeCount = 3;

    for (let b = 0; b < bladeCount; b++) {
      const ang = (b / bladeCount) * Math.PI * 2 + 0.35;
      const ca = Math.cos(ang);
      const sa = Math.sin(ang);
      const px = -sa;
      const pz = ca;
      const bladeH = 0.46 + b * 0.055; // 0.46..0.57m base -> scaled to ~0.45–0.74m
      const baseW = 0.056;
      const lean = 0.16 + b * 0.03;

      // Base pair (t = 0)
      const bCx = ca * 0.05;
      const bCz = sa * 0.05;
      positions.push(bCx - px * baseW, 0, bCz - pz * baseW);
      positions.push(bCx + px * baseW, 0, bCz + pz * baseW);
      normals.push(ca * 0.3, 0.95, sa * 0.3, ca * 0.3, 0.95, sa * 0.3);
      colors.push(cBase.r, cBase.g, cBase.b, cBase.r, cBase.g, cBase.b);

      // Mid pair (t = 0.55)
      const mW = baseW * 0.52;
      const mOut = 0.05 + 0.3 * lean;
      const mCx = ca * mOut;
      const mCz = sa * mOut;
      positions.push(mCx - px * mW, bladeH * 0.55, mCz - pz * mW);
      positions.push(mCx + px * mW, bladeH * 0.55, mCz + pz * mW);
      normals.push(ca * 0.3, 0.95, sa * 0.3, ca * 0.3, 0.95, sa * 0.3);
      colors.push(cMid.r, cMid.g, cMid.b, cMid.r, cMid.g, cMid.b);

      // Tapered tip vertex (t = 1.0)
      const tOut = 0.05 + lean;
      positions.push(ca * tOut, bladeH, sa * tOut);
      normals.push(ca * 0.3, 0.95, sa * 0.3);
      colors.push(cTip.r, cTip.g, cTip.b);

      // 3 triangles per blade: lower quad (2 tris) + upper tapered tip (1 tri)
      indices.push(
        vOffset, vOffset + 1, vOffset + 2,
        vOffset + 1, vOffset + 3, vOffset + 2,
        vOffset + 2, vOffset + 3, vOffset + 4
      );
      vOffset += 5;
    }

    const grassGeo = new THREE.BufferGeometry();
    grassGeo.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
    grassGeo.setAttribute('normal', new THREE.Float32BufferAttribute(normals, 3));
    grassGeo.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
    grassGeo.setIndex(indices);

    const grassMat = new THREE.MeshLambertMaterial({
      vertexColors: true,
      side: THREE.DoubleSide,
    });
    grassMat.customProgramCacheKey = () => 'aethelgard_grass_wind';
    grassMat.onBeforeCompile = (shader) => {
      shader.uniforms.uTime = grassUniforms.uTime;
      shader.vertexShader = shader.vertexShader
        .replace('#include <common>', '#include <common>\nuniform float uTime;')
        .replace(
          '#include <begin_vertex>',
          `#include <begin_vertex>
          #ifdef USE_INSTANCING
            vec3 instW = vec3(instanceMatrix[3][0], instanceMatrix[3][1], instanceMatrix[3][2]);
            float hFrac = clamp(position.y / 0.58, 0.0, 1.0);
            float bend = hFrac * hFrac;
            float waveA = sin(instW.x * 0.072 + instW.z * 0.044 + uTime * 2.05);
            float waveB = sin(instW.x * 0.028 - instW.z * 0.056 + uTime * 1.18);
            float gust = waveA * 0.65 + waveB * 0.35;
            transformed.x += (gust * 0.19 + 0.05) * bend;
            transformed.z += (waveB * 0.11 + 0.03) * bend;
            transformed.y -= abs(gust) * 0.045 * bend;
          #endif`
        );
    };

    const grassInst = new THREE.InstancedMesh(grassGeo, grassMat, grassCount);
    grassInst.receiveShadow = true;
    grassInst.castShadow = false;
    grassInst.frustumCulled = false;

    const l = LANDMARKS.lake;
    let placed = 0;
    let tries = 0;
    while (placed < grassCount && tries < grassCount * 8) {
      tries++;
      const a = grng() * Math.PI * 2;
      const rad = Math.sqrt(grng()) * 162;
      const gx = Math.cos(a) * rad;
      const gz = Math.sin(a) * rad;

      // Lake basin & widened sandy shore bank exclusion
      if (Math.hypot((gx - l.x) / l.rx, (gz - l.z) / l.rz) < 1.22) continue;
      // Widened roads, village square & spawn camera clearance
      if (getRoadWeight(gx, gz) > 0.05) continue;
      if (Math.hypot(gx - LANDMARKS.village.x, gz - LANDMARKS.village.z) < 25.0) continue;
      if (Math.hypot(gx - LANDMARKS.spawn.x, gz - LANDMARKS.spawn.z) < 12.0) continue;
      // Castle stone courtyard
      if (Math.abs(gx) < 31.5 && gz < -78 && gz > -139) continue;
      // Dungeon mountain & chamber
      if (gx > 78 && gz > -52 && gz < 4) continue;
      // Bridge & approach banks
      if (Math.abs(gx) < 9.5 && gz < 26 && gz > -42) continue;
      // Dragon perch stone rim & Rune Circle stone ring
      if (Math.hypot(gx - LANDMARKS.dragonPerch.x, gz - LANDMARKS.dragonPerch.z) < 9.0) continue;
      if (Math.hypot(gx - LANDMARKS.ruins.x, gz - LANDMARKS.ruins.z) < 10.2) continue;

      const gy = getTerrainHeight(gx, gz);
      if (gy < 1.85 || gy > 27) continue;
      if (getTerrainSlopeY(gx, gz) < 0.81) continue;

      const sxz = 0.85 + grng() * 0.36;
      const sy = 0.94 + grng() * 0.34; // Final blade heights: ~0.45–0.74m
      _v1.set(gx, gy - 0.04, gz);
      _euler.set(0, grng() * Math.PI * 2, 0);
      _quat.setFromEuler(_euler);
      _scale.set(sxz, sy, sxz);
      _mat4.compose(_v1, _quat, _scale);
      grassInst.setMatrixAt(placed, _mat4);

      _col1.setHex(0xffffff).offsetHSL((grng() - 0.5) * 0.06, (grng() - 0.5) * 0.08, (grng() - 0.5) * 0.14);
      grassInst.setColorAt(placed, _col1);
      placed++;
    }
    grassInst.count = placed;
    grassInst.instanceMatrix.needsUpdate = true;
    if (grassInst.instanceColor) grassInst.instanceColor.needsUpdate = true;
    worldGroup.add(grassInst);
  }

  // ==========================================================================
  // 7. HILLTOP CASTLE (LANDMARKS.castle at (0, -108), floorY = 18.0)
  // ==========================================================================
  {
    const castleG = new THREE.Group();
    const cy = LANDMARKS.castle.floorY; // 18.0
    const slateRoofMat = new THREE.MeshStandardMaterial({
      color: 0x262c3d,
      roughness: 0.72,
      metalness: 0.16,
    });

    // Animated Crimson-and-Gold Banner (instanced across all 8 castle banners!)
    const bannerGeo = new THREE.PlaneGeometry(1.6, 6.2, 4, 10);
    bannerGeo.translate(0, -3.1, 0);
    const bPos = bannerGeo.attributes.position;
    const bColors = new Float32Array(bPos.count * 3);
    const cCrim = new THREE.Color(PALETTE.crimson);
    const cGold = new THREE.Color(PALETTE.gold);
    for (let i = 0; i < bPos.count; i++) {
      const bx = bPos.getX(i);
      const by = bPos.getY(i);
      if (by < -5.2) {
        const notch = 1.0 - Math.abs(bx) / 0.8;
        bPos.setY(i, by + notch * 0.95);
      }
      const isTrim = Math.abs(bx) > 0.62 || (by < -4.9 && by > -5.35) || (Math.abs(bx) < 0.16 && by < -1.1 && by > -4.3);
      _col1.copy(isTrim ? cGold : cCrim);
      bColors[i * 3] = _col1.r;
      bColors[i * 3 + 1] = _col1.g;
      bColors[i * 3 + 2] = _col1.b;
    }
    bannerGeo.setAttribute('color', new THREE.BufferAttribute(bColors, 3));

    const bannerMat = new THREE.MeshStandardMaterial({
      vertexColors: true,
      roughness: 0.78,
      metalness: 0.1,
      side: THREE.DoubleSide,
    });
    bannerMat.customProgramCacheKey = () => 'aethelgard_banner_wind';
    bannerMat.onBeforeCompile = (shader) => {
      shader.uniforms.uTime = bannerUniforms.uTime;
      shader.vertexShader = shader.vertexShader
        .replace('#include <common>', '#include <common>\nuniform float uTime;')
        .replace(
          '#include <begin_vertex>',
          `#include <begin_vertex>
          float hang = clamp(-position.y / 6.2, 0.0, 1.0);
          #ifdef USE_INSTANCING
            float wPhase = instanceMatrix[3][0] * 0.25 + instanceMatrix[3][2] * 0.18;
          #else
            float wPhase = modelMatrix[3][0] * 0.25 + modelMatrix[3][2] * 0.18;
          #endif
          transformed.z += sin(uTime * 3.8 + position.y * 1.15 + wPhase) * 0.34 * hang * hang;
          transformed.x += cos(uTime * 2.5 + position.y * 0.85 + wPhase) * 0.12 * hang;`
        );
    };

    const bannerPoleMats = [];
    const bannerClothMats = [];
    function registerBanner(x, y, z, rotY) {
      const bm = makeTransformMatrix(x, y, z, 0, rotY, 0);
      pushChildMatrix(bannerPoleMats, bm, 0, 0, 0, 0, 0, Math.PI * 0.5);
      bannerClothMats.push(bm);
    }

    // Courtyard paved stone deck & hilltop foundation plinth (56x54)
    const deckPlinth = new THREE.Mesh(scaleGeometryUVs(new THREE.BoxGeometry(56, 5.4, 54), 5, 5), stoneMat);
    deckPlinth.position.set(0, cy - 2.69, -108);
    deckPlinth.receiveShadow = true;
    castleG.add(deckPlinth);

    // Boss Arena decorative stone mosaic ring at (0, -112)
    const arenaRingOuter = new THREE.Mesh(new THREE.RingGeometry(15.2, 16.5, 32), stoneDarkMat);
    arenaRingOuter.rotation.x = -Math.PI * 0.5;
    arenaRingOuter.position.set(0, cy + 0.04, -112);
    arenaRingOuter.receiveShadow = true;
    castleG.add(arenaRingOuter);

    const arenaRingInner = new THREE.Mesh(new THREE.RingGeometry(7.0, 7.6, 28), stoneDarkMat);
    arenaRingInner.rotation.x = -Math.PI * 0.5;
    arenaRingInner.position.set(0, cy + 0.04, -112);
    castleG.add(arenaRingInner);

    // Curtain walls (foundation down to y=13.5, battlements top at y=27.5)
    const wallH = 14.0;
    const wallY = 13.5 + wallH * 0.5;
    const wallThick = 2.6;
    const corniceMats = [];

    function addCurtainWall(x, z, lenX, lenZ) {
      const mesh = new THREE.Mesh(
        scaleGeometryUVs(new THREE.BoxGeometry(lenX, wallH, lenZ), Math.max(1, Math.max(lenX, lenZ) / 6), 2.2),
        stoneMat
      );
      mesh.position.set(x, wallY, z);
      mesh.castShadow = true;
      mesh.receiveShadow = true;
      castleG.add(mesh);

      corniceMats.push(makeTransformMatrix(x, cy + 9.4, z, 0, 0, 0, lenX + 0.5, 0.55, lenZ + 0.5));
    }

    addCurtainWall(0, -134, 54, wallThick);
    addCurtainWall(-27, -108, wallThick, 52);
    addCurtainWall(27, -108, wallThick, 52);
    addCurtainWall(-16.25, -82, 21.5, wallThick);
    addCurtainWall(16.25, -82, 21.5, wallThick);

    castleG.add(buildInstancedMesh(unitBoxGeo, stoneDarkMat, corniceMats, false, true));

    // Instanced Battlement Merlons along all curtain walls (castShadow = false on small crenellations)
    const merlonMats = [];
    for (let x = -23; x <= 23; x += 3.2) merlonMats.push(makeTransformMatrix(x, cy + 10.35, -134.8));
    for (let z = -130; z <= -86; z += 3.2) {
      merlonMats.push(makeTransformMatrix(-27.8, cy + 10.35, z));
      merlonMats.push(makeTransformMatrix(27.8, cy + 10.35, z));
    }
    for (let x = -23; x <= -10; x += 3.2) merlonMats.push(makeTransformMatrix(x, cy + 10.35, -81.2));
    for (let x = 10; x <= 23; x += 3.2) merlonMats.push(makeTransformMatrix(x, cy + 10.35, -81.2));

    const cMerlonGeo = new THREE.BoxGeometry(1.5, 1.35, 1.1);
    castleG.add(buildInstancedMesh(cMerlonGeo, stoneMat, merlonMats, false, true));

    // Grand Southern Arched Gatehouse (x in [-5.5, 5.5] wide open!)
    {
      const gTowerGeo = scaleGeometryUVs(new THREE.BoxGeometry(4.8, 20.5, 5.4), 1.2, 3.0);
      const gCrownGeo = new THREE.BoxGeometry(5.5, 1.1, 6.1);
      const gRoofGeo = new THREE.ConeGeometry(4.0, 6.8, 4);

      const gTowerMats = [];
      const gCrownMats = [];
      const gRoofMats = [];
      for (let s = -1; s <= 1; s += 2) {
        const gx = s * 7.8;
        gTowerMats.push(makeTransformMatrix(gx, 13.5 + 10.25, -82));
        gCrownMats.push(makeTransformMatrix(gx, 34.2, -82));
        gRoofMats.push(makeTransformMatrix(gx, 38.0, -82, 0, Math.PI * 0.25, 0));
        registerBanner(gx, 29.8, -79.1, 0);
      }
      castleG.add(
        buildInstancedMesh(gTowerGeo, stoneMat, gTowerMats, true, true),
        buildInstancedMesh(gCrownGeo, stoneDarkMat, gCrownMats, false, true),
        buildInstancedMesh(gRoofGeo, slateRoofMat, gRoofMats, true, false)
      );

      // Upper gatehouse bridge span above the x in [-5.5, 5.5] entrance
      const gateSpan = new THREE.Mesh(scaleGeometryUVs(new THREE.BoxGeometry(11.6, 5.2, 4.4), 2.0, 1.0), stoneMat);
      gateSpan.position.set(0, cy + 11.4, -82);
      gateSpan.castShadow = true;
      gateSpan.receiveShadow = true;
      castleG.add(gateSpan);

      // Arched stone voussoirs framing the open gateway (1 InstancedMesh)
      const vMats = [];
      for (let k = 0; k < 9; k++) {
        const a = ((k + 0.5) / 9) * Math.PI;
        vMats.push(makeTransformMatrix(Math.cos(a) * 5.3, cy + 5.8 + Math.sin(a) * 3.2, -82, 0, 0, a - Math.PI * 0.5));
      }
      castleG.add(buildInstancedMesh(new THREE.BoxGeometry(1.45, 0.8, 4.65), stoneDarkMat, vMats, false, true));

      // Raised iron portcullis teeth tucked high beneath arch (1 InstancedMesh)
      const portMats = [];
      for (let px = -4.2; px <= 4.2; px += 1.2) {
        portMats.push(makeTransformMatrix(px, cy + 8.3, -82));
      }
      castleG.add(buildInstancedMesh(new THREE.CylinderGeometry(0.08, 0.05, 1.4, 6), ironMat, portMats, false, false));
    }

    // 4 Tall Corner Towers with Conical Slate Roofs & Finials (4 InstancedMeshes!)
    const towerCoords = [
      [-27, -82, 0],
      [27, -82, 0],
      [-27, -134, Math.PI],
      [27, -134, Math.PI],
    ];
    const tShaftMats = [];
    const tCollarMats = [];
    const tSpireMats = [];
    const tFinialMats = [];
    for (let i = 0; i < towerCoords.length; i++) {
      const [tx, tz, bRot] = towerCoords[i];
      tShaftMats.push(makeTransformMatrix(tx, 23.0, tz));
      tCollarMats.push(makeTransformMatrix(tx, 34.4, tz));
      tSpireMats.push(makeTransformMatrix(tx, 40.4, tz));
      tFinialMats.push(makeTransformMatrix(tx, 46.8, tz));
      registerBanner(tx, 31.0, tz + (tz > -100 ? 4.65 : -4.65), bRot);
      addCircleCollider(tx, tz, 4.6, 12, 44);
    }
    castleG.add(
      buildInstancedMesh(scaleGeometryUVs(new THREE.CylinderGeometry(4.2, 4.6, 22.0, 10), 2.5, 3.2), stoneMat, tShaftMats, true, true),
      buildInstancedMesh(new THREE.CylinderGeometry(4.85, 4.35, 1.6, 10), stoneDarkMat, tCollarMats, false, true),
      buildInstancedMesh(new THREE.ConeGeometry(5.15, 10.5, 10), slateRoofMat, tSpireMats, true, false),
      buildInstancedMesh(new THREE.CylinderGeometry(0.06, 0.16, 2.8, 6), goldTrimMat, tFinialMats, false, false)
    );

    // Inside Courtyard: 6 Ancient Carved Rune Pillars (3 InstancedMeshes!)
    const pillarZ = [-94, -108, -122];
    const pBaseMats = [];
    const pShaftMats = [];
    const pCapMats = [];
    for (let i = 0; i < pillarZ.length; i++) {
      const pz = pillarZ[i];
      for (let s = -1; s <= 1; s += 2) {
        const px = s * 19.5;
        pBaseMats.push(makeTransformMatrix(px, cy + 0.4, pz));
        pShaftMats.push(makeTransformMatrix(px, cy + 4.9, pz));
        pCapMats.push(makeTransformMatrix(px, cy + 9.2, pz));
        addCircleCollider(px, pz, 1.25, cy - 1, cy + 10);
      }
    }
    castleG.add(
      buildInstancedMesh(new THREE.BoxGeometry(2.3, 0.8, 2.3), stoneDarkMat, pBaseMats, false, true),
      buildInstancedMesh(new THREE.CylinderGeometry(0.82, 0.96, 8.2, 8), runePillarMat, pShaftMats, true, true),
      buildInstancedMesh(new THREE.BoxGeometry(2.2, 0.65, 2.2), stoneDarkMat, pCapMats, false, true)
    );

    // Inside Courtyard: 6 Flaming Braziers (batched globally)
    const castleBraziers = [
      [-13.5, -90],
      [13.5, -90],
      [-16.5, -110],
      [16.5, -110],
      [-10.5, -126],
      [10.5, -126],
    ];
    for (let i = 0; i < castleBraziers.length; i++) {
      const [bx, bz] = castleBraziers[i];
      registerBrazier(bx, cy, bz, 1.05, i * 1.15);
    }

    // 2 Warm Courtyard Fire PointLights for dramatic boss-arena night lighting
    for (let s = -1; s <= 1; s += 2) {
      const cLight = new THREE.PointLight(0xff8430, 48, 42, 1.45);
      cLight.position.set(s * 14, cy + 4.8, -110);
      castleG.add(cLight);
      flickeringLights.push({ light: cLight, base: 48, amp: 8, phase: s * 1.5, nightBoost: true });
    }

    // Northern Raised Stone Throne Dais & Ruined Crown Arch
    {
      const daisG = new THREE.Group();
      daisG.position.set(0, cy, -129.5);

      for (let step = 0; step < 3; step++) {
        const w = 15.0 - step * 2.2;
        const d = 7.4 - step * 1.2;
        const st = new THREE.Mesh(new THREE.BoxGeometry(w, 0.55, d), step === 2 ? stoneMat : stoneDarkMat);
        st.position.set(0, 0.275 + step * 0.52, -step * 0.55);
        st.receiveShadow = true;
        st.castShadow = true;
        daisG.add(st);
      }

      const throneSeat = new THREE.Mesh(new THREE.BoxGeometry(2.8, 1.1, 2.2), obsidianMat);
      throneSeat.position.set(0, 2.1, -1.6);
      throneSeat.castShadow = true;
      daisG.add(throneSeat);

      const throneBack = new THREE.Mesh(new THREE.BoxGeometry(2.6, 4.4, 0.55), obsidianMat);
      throneBack.position.set(0, 4.1, -2.5);
      throneBack.castShadow = true;
      daisG.add(throneBack);

      const throneCrest = new THREE.Mesh(new THREE.OctahedronGeometry(0.65, 0), goldTrimMat);
      throneCrest.position.set(0, 6.7, -2.5);
      daisG.add(throneCrest);

      for (let s = -1; s <= 1; s += 2) {
        const ribBase = new THREE.Mesh(new THREE.BoxGeometry(1.6, 8.5, 1.6), runePillarMat);
        ribBase.position.set(s * 6.4, 4.25, -2.4);
        ribBase.castShadow = true;
        daisG.add(ribBase);

        const ribUpper = new THREE.Mesh(new THREE.BoxGeometry(1.4, 5.2, 1.4), stoneMat);
        ribUpper.position.set(s * 4.5, 10.2, -2.4);
        ribUpper.rotation.z = s * 0.62;
        ribUpper.castShadow = true;
        daisG.add(ribUpper);
      }

      registerBanner(-7.2, cy + 8.5, -131.6, 0);
      registerBanner(7.2, cy + 8.5, -131.6, 0);

      castleG.add(daisG);
    }

    // Flush all 8 instanced castle banners (2 draw calls total!)
    castleG.add(
      buildInstancedMesh(new THREE.CylinderGeometry(0.06, 0.06, 2.0, 6), goldTrimMat, bannerPoleMats, false, false),
      buildInstancedMesh(bannerGeo, bannerMat, bannerClothMats, false, false)
    );

    // Castle Wall Colliders (south wall split around x in [-5.4, 5.4] gateway!)
    addBoxCollider(-28.5, -5.4, -83.6, -80.4, 13, 35);
    addBoxCollider(5.4, 28.5, -83.6, -80.4, 13, 35);
    addBoxCollider(-10.3, -5.4, -84.8, -79.2, 13, 38);
    addBoxCollider(5.4, 10.3, -84.8, -79.2, 13, 38);
    addBoxCollider(-28.5, 28.5, -135.6, -132.4, 13, 35);
    addBoxCollider(-28.5, -25.4, -135.6, -80.4, 13, 35);
    addBoxCollider(25.4, 28.5, -135.6, -80.4, 13, 35);

    worldGroup.add(castleG);
  }

  // ==========================================================================
  // 8. DARK LAVA DUNGEON ("EMBER CRYPT") & SUNSTONE RELIC
  // ==========================================================================
  const lavaUniforms = {
    uTime: { value: 0 },
    uFogColor: { value: new THREE.Color(0x0d080c) },
    uFogDensity: { value: 0.026 },
  };

  let sunstoneRelic = null;

  {
    const d = LANDMARKS.dungeonCenter;
    const dungG = new THREE.Group();
    const midX = (d.minX + d.maxX) * 0.5; // 131
    const midZ = (d.minZ + d.maxZ) * 0.5; // -24
    const lenX = d.maxX - d.minX; // 70
    const lenZ = d.maxZ - d.minZ; // 36
    const chamberH = d.ceilingY - d.floorY; // 14

    const cavernWallMat = new THREE.MeshStandardMaterial({
      map: textures.stoneMasonry,
      normalMap: textures.stoneNormal,
      normalScale: new THREE.Vector2(1.1, 1.1),
      roughnessMap: textures.stoneRough,
      color: 0x24202b,
      roughness: 0.88,
      metalness: 0.14,
    });

    const mountainRockMat = new THREE.MeshStandardMaterial({
      color: 0x2d2932,
      roughness: 0.94,
      metalness: 0.06,
    });

    // Sealed light-tight interior cavern walls & ceiling (castShadow = false — interior dungeon!)
    const wallY = d.floorY + chamberH * 0.5;

    const northWall = new THREE.Mesh(scaleGeometryUVs(new THREE.BoxGeometry(lenX + 6, chamberH + 4, 3.6), 5, 2), cavernWallMat);
    northWall.position.set(midX, wallY, d.minZ - 1.8);
    dungG.add(northWall);

    const southWall = new THREE.Mesh(scaleGeometryUVs(new THREE.BoxGeometry(lenX + 6, chamberH + 4, 3.6), 5, 2), cavernWallMat);
    southWall.position.set(midX, wallY, d.maxZ + 1.8);
    dungG.add(southWall);

    const eastWall = new THREE.Mesh(scaleGeometryUVs(new THREE.BoxGeometry(4.0, chamberH + 4, lenZ + 8), 3, 2), cavernWallMat);
    eastWall.position.set(d.maxX + 2.0, wallY, midZ);
    dungG.add(eastWall);

    const westNorth = new THREE.Mesh(new THREE.BoxGeometry(3.6, chamberH + 4, 15.5), cavernWallMat);
    westNorth.position.set(d.minX - 1.2, wallY, -36.75);
    dungG.add(westNorth);

    const westSouth = new THREE.Mesh(new THREE.BoxGeometry(3.6, chamberH + 4, 15.5), cavernWallMat);
    westSouth.position.set(d.minX - 1.2, wallY, -11.25);
    dungG.add(westSouth);

    const westLintel = new THREE.Mesh(new THREE.BoxGeometry(4.2, 7.6, 12.0), cavernWallMat);
    westLintel.position.set(d.minX - 1.2, 17.8, midZ);
    dungG.add(westLintel);

    const roofLid = new THREE.Mesh(new THREE.BoxGeometry(lenX + 10, 4.2, lenZ + 10), cavernWallMat);
    roofLid.position.set(midX + 1, d.ceilingY + 2.0, midZ);
    dungG.add(roofLid);

    // Massive Craggy Volcanic Mountainside Exterior Shell (2 InstancedMeshes instead of 15!)
    const domeGeo = new THREE.SphereGeometry(1.0, 10, 7, 0, Math.PI * 2, 0, Math.PI * 0.5);
    const dmPos = domeGeo.attributes.position;
    for (let i = 0; i < dmPos.count; i++) {
      const x = dmPos.getX(i);
      const y = dmPos.getY(i);
      const z = dmPos.getZ(i);
      const crag = 0.84 + 0.28 * Math.sin(x * 4.2 + z * 3.1) * Math.cos(y * 3.5);
      dmPos.setXYZ(i, x * crag, Math.max(0.0, y * crag), z * crag);
    }
    domeGeo.computeVertexNormals();

    const overheadPeaks = [
      [110, 19.8, -24, 21, 14, 24],
      [134, 19.8, -24, 26, 18.5, 26],
      [157, 19.8, -24, 21, 15.5, 24],
      [122, 19.8, -32, 16, 13, 15],
      [146, 19.8, -16, 16, 12.5, 15],
    ];
    const domeMats = [];
    for (let i = 0; i < overheadPeaks.length; i++) {
      const [bx, by, bz, sx, sy, sz] = overheadPeaks[i];
      domeMats.push(makeTransformMatrix(bx, by, bz, 0, i * 0.85, 0, sx, sy, sz));
    }
    dungG.add(buildInstancedMesh(domeGeo, mountainRockMat, domeMats, true, true));

    const peakGeo = new THREE.IcosahedronGeometry(1.0, 1);
    const pkPos = peakGeo.attributes.position;
    for (let i = 0; i < pkPos.count; i++) {
      const x = pkPos.getX(i);
      const y = pkPos.getY(i);
      const z = pkPos.getZ(i);
      const crag = 0.86 + 0.24 * Math.sin(x * 3.8 + z * 2.9) * Math.cos(y * 3.1);
      pkPos.setXYZ(i, x * crag, y * crag, z * crag);
    }
    peakGeo.computeVertexNormals();

    const mtnBlobs = [
      [108, 12.0, -50.5, 16, 14, 6.2],
      [132, 14.0, -51.5, 22, 17, 6.8],
      [156, 13.0, -50.5, 18, 15, 6.2],
      [108, 12.0, 2.5, 16, 14, 6.2],
      [132, 14.0, 3.5, 22, 17, 6.8],
      [156, 13.0, 2.5, 18, 15, 6.2],
      [175.5, 14.0, -24, 6.8, 17, 25],
      [91.8, 12.0, -39.5, 4.2, 13.5, 7.4],
      [91.8, 12.0, -8.5, 4.2, 13.5, 7.4],
      [96.0, 23.0, -24, 6.0, 5.2, 13.5],
    ];
    const blobMats = [];
    for (let i = 0; i < mtnBlobs.length; i++) {
      const [bx, by, bz, sx, sy, sz] = mtnBlobs[i];
      blobMats.push(makeTransformMatrix(bx, by, bz, 0, i * 0.9, 0, sx, sy, sz));
    }
    dungG.add(buildInstancedMesh(peakGeo, mountainRockMat, blobMats, false, true));

    // Dramatic Glowing-Rune Obsidian Entrance Archway facing west at x=94.4, z=-24
    {
      const entG = new THREE.Group();
      entG.position.set(94.4, d.floorY, midZ);

      for (let s = -1; s <= 1; s += 2) {
        const pier = new THREE.Mesh(new THREE.BoxGeometry(2.6, 10.6, 2.5), dungeonRuneMat);
        pier.position.set(0, 5.1, s * 6.0);
        pier.castShadow = true;
        entG.add(pier);

        const horn = new THREE.Mesh(new THREE.ConeGeometry(1.4, 3.6, 5), obsidianMat);
        horn.position.set(0, 11.8, s * 6.0);
        entG.add(horn);

        registerBrazier(94.4 - 2.4, d.floorY, midZ + s * 6.4, 1.1, 4.0 + s);
      }

      const archHeader = new THREE.Mesh(new THREE.BoxGeometry(3.0, 2.4, 15.2), dungeonRuneMat);
      archHeader.position.set(0, 10.2, 0);
      entG.add(archHeader);

      const keystone = new THREE.Mesh(new THREE.OctahedronGeometry(1.35, 0), dungeonRuneMat);
      keystone.position.set(-1.1, 10.4, 0);
      entG.add(keystone);

      dungG.add(entG);
    }

    // Vaulted Obsidian Ceiling Ribs & Wall Pilasters (2 InstancedMeshes instead of 18, castShadow = false!)
    const ribXs = [104, 116, 128, 140, 152, 162];
    const ribGeo = new THREE.TorusGeometry(17.8, 0.95, 6, 16, Math.PI);
    const pilasterGeo = new THREE.BoxGeometry(1.6, 6.8, 1.5);
    const ribMats = [];
    const pilasterMats = [];
    for (let i = 0; i < ribXs.length; i++) {
      ribMats.push(makeTransformMatrix(ribXs[i], 11.4, midZ, 0, Math.PI * 0.5, 0, 1.0, 7.85 / 17.8, 1.0));
      for (let s = -1; s <= 1; s += 2) {
        pilasterMats.push(makeTransformMatrix(ribXs[i], d.floorY + 3.4, midZ + s * 17.4));
      }
    }
    dungG.add(
      buildInstancedMesh(ribGeo, obsidianMat, ribMats, false, false),
      buildInstancedMesh(pilasterGeo, obsidianMat, pilasterMats, false, false)
    );

    // 8 Tall Carved Stone Interior Columns (2 InstancedMeshes instead of 24, castShadow = false!)
    const colXs = [108, 122, 136, 150];
    const colBaseCapMats = [];
    const colShaftMats = [];
    for (let i = 0; i < colXs.length; i++) {
      for (let s = -1; s <= 1; s += 2) {
        const cx = colXs[i];
        const cz = midZ + s * 10.5;
        colBaseCapMats.push(makeTransformMatrix(cx, d.floorY + 0.45, cz, 0, 0, 0, 2.4, 0.9, 2.4));
        colBaseCapMats.push(makeTransformMatrix(cx, d.ceilingY - 0.5, cz, 0, 0, 0, 2.5, 0.85, 2.5));
        colShaftMats.push(makeTransformMatrix(cx, d.floorY + chamberH * 0.5, cz));
        addCircleCollider(cx, cz, 1.15, d.floorY - 1, d.ceilingY + 1);
      }
    }
    dungG.add(
      buildInstancedMesh(unitBoxGeo, obsidianMat, colBaseCapMats, false, false),
      buildInstancedMesh(new THREE.CylinderGeometry(0.82, 0.98, chamberH - 1.2, 8), dungeonRuneMat, colShaftMats, false, false)
    );

    // Custom Animated Molten Lava-Crack Floor Shader (y = 5.53)
    {
      const lavaFloorGeo = new THREE.PlaneGeometry(lenX + 1.5, lenZ + 1.0, 1, 1);
      lavaFloorGeo.rotateX(-Math.PI * 0.5);

      const lavaMat = new THREE.ShaderMaterial({
        uniforms: lavaUniforms,
        vertexShader: /* glsl */ `
          varying vec3 vWorldPos;
          void main() {
            vec4 wp = modelMatrix * vec4(position, 1.0);
            vWorldPos = wp.xyz;
            gl_Position = projectionMatrix * viewMatrix * wp;
          }
        `,
        fragmentShader: /* glsl */ `
          uniform float uTime;
          uniform vec3 uFogColor;
          uniform float uFogDensity;
          varying vec3 vWorldPos;

          vec2 hash22(vec2 p) {
            p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
            return fract(sin(p) * 43758.5453);
          }

          float voronoiEdge(vec2 x) {
            vec2 n = floor(x);
            vec2 f = fract(x);
            float d1 = 8.0;
            float d2 = 8.0;
            for (int j = -1; j <= 1; j++) {
              for (int i = -1; i <= 1; i++) {
                vec2 g = vec2(float(i), float(j));
                vec2 o = hash22(n + g);
                vec2 r = g + o - f;
                float d = dot(r, r);
                if (d < d1) {
                  d2 = d1;
                  d1 = d;
                } else if (d < d2) {
                  d2 = d;
                }
              }
            }
            return sqrt(d2) - sqrt(d1);
          }

          void main() {
            vec2 wp = vWorldPos.xz;
            vec2 warp = vec2(
              sin(wp.y * 0.35 + 1.2) * 0.32 + cos(wp.x * 0.22) * 0.25,
              cos(wp.x * 0.31 - 0.7) * 0.32 + sin(wp.y * 0.27) * 0.25
            );
            float c1 = voronoiEdge((wp + warp) * 0.21);
            float c2 = voronoiEdge((wp * 1.15 - warp * 0.7) * 0.56);

            float centerPath = 1.0 - smoothstep(2.6, 5.8, abs(wp.y + 24.0));
            float crackW1 = mix(0.145, 0.068, centerPath);
            float crackW2 = mix(0.085, 0.038, centerPath);

            float primaryLava = 1.0 - smoothstep(0.0, crackW1, c1);
            float secondaryLava = (1.0 - smoothstep(0.0, crackW2, c2)) * 0.72;
            float magmaMask = clamp(max(primaryLava, secondaryLava), 0.0, 1.0);

            float flow = sin(wp.x * 0.55 - uTime * 2.1 + sin(wp.y * 0.48 + uTime * 1.3)) * 0.5 + 0.5;
            float pulse = 0.78 + 0.34 * sin(uTime * 2.6 + c1 * 6.0 + wp.x * 0.14);

            float grain = fract(sin(dot(floor(wp * 4.5), vec2(12.9898, 78.233))) * 43758.5453);
            vec3 basalt = mix(vec3(0.012, 0.010, 0.016), vec3(0.032, 0.026, 0.038), grain * 0.6 + centerPath * 0.35);

            float rimGlow = (1.0 - smoothstep(crackW1, crackW1 + 0.16, c1)) * 0.45;
            basalt += vec3(0.24, 0.025, 0.004) * rimGlow * pulse;

            vec3 lavaDark = vec3(0.85, 0.04, 0.005);
            vec3 lavaMid = vec3(1.55, 0.22, 0.015);
            vec3 lavaHot = vec3(2.8, 1.35, 0.22);
            float hotCore = smoothstep(0.32, 0.92, magmaMask * (0.75 + 0.38 * flow));
            vec3 lavaCol = mix(lavaDark, lavaMid, magmaMask);
            lavaCol = mix(lavaCol, lavaHot, hotCore) * (1.25 * pulse);

            vec3 col = mix(basalt, lavaCol, smoothstep(0.08, 0.45, magmaMask));

            float dAltar = length(wp - vec2(156.0, -24.0));
            col += vec3(0.35, 0.18, 0.025) * exp(-dAltar * 0.18);

            float fogDist = length(cameraPosition - vWorldPos);
            float fogF = 1.0 - exp(-fogDist * fogDist * uFogDensity * uFogDensity);
            col = mix(col, uFogColor, clamp(fogF, 0.0, 1.0));

            gl_FragColor = vec4(col, 1.0);
            #include <tonemapping_fragment>
            #include <colorspace_fragment>
          }
        `,
      });

      const lavaFloor = new THREE.Mesh(lavaFloorGeo, lavaMat);
      lavaFloor.position.set(midX, d.floorY + 0.03, midZ);
      dungG.add(lavaFloor);
    }

    // 8 Flickering Wall Torches (2 InstancedMeshes for brackets & sticks + global instanced flames!)
    const torchXs = [106, 122, 138, 154];
    const torchBracketMats = [];
    const torchStickMats = [];
    let tIdx = 0;
    for (let i = 0; i < torchXs.length; i++) {
      for (let s = -1; s <= 1; s += 2) {
        const tx = torchXs[i];
        const ty = d.floorY + 3.8;
        const tz = s < 0 ? d.minZ + 0.65 : d.maxZ - 0.65;
        torchBracketMats.push(makeTransformMatrix(tx, ty, tz + (s < 0 ? -0.15 : 0.15), 0, 0, 0, 0.22, 0.6, 0.55));
        torchStickMats.push(makeTransformMatrix(tx, ty + 0.22, tz + (s < 0 ? 0.25 : -0.25), s < 0 ? 0.28 : -0.28, 0, 0));
        flameSpecs.push({ x: tx, y: ty + 0.72, z: tz + (s < 0 ? 0.38 : -0.38), scale: 0.65, phase: tIdx++ * 0.9 });
      }
    }
    dungG.add(
      buildInstancedMesh(unitBoxGeo, ironMat, torchBracketMats, false, false),
      buildInstancedMesh(new THREE.CylinderGeometry(0.08, 0.05, 1.05, 6), darkWoodMat, torchStickMats, false, false)
    );

    // 4 Dungeon PointLights (3 fiery torch corridor lights + 1 golden altar light)
    const dungLightCoords = [
      [106, d.floorY + 5.2, midZ, 0xff7626, 56, 36],
      [126, d.floorY + 5.2, midZ, 0xff7022, 58, 36],
      [144, d.floorY + 5.2, midZ, 0xff7828, 54, 34],
      [LANDMARKS.dungeonRelic.x, d.floorY + 4.2, LANDMARKS.dungeonRelic.z, 0xffc64a, 62, 30],
    ];
    for (let i = 0; i < dungLightCoords.length; i++) {
      const [lx, ly, lz, hex, base, dist] = dungLightCoords[i];
      const pl = new THREE.PointLight(hex, base, dist, 1.45);
      pl.position.set(lx, ly, lz);
      dungG.add(pl);
      flickeringLights.push({ light: pl, base, amp: i === 3 ? 6 : 11, phase: i * 1.7, nightBoost: false });
    }

    // Glowing Purple & Amber Crystal Clusters along dungeon walls (batched globally)
    const dungCrystals = [
      [101, d.minZ + 1.8, true, 1.2],
      [115, d.minZ + 1.5, false, 1.35],
      [130, d.minZ + 1.8, true, 1.15],
      [146, d.minZ + 1.6, false, 1.4],
      [161, d.minZ + 2.4, true, 1.5],
      [101, d.maxZ - 1.8, false, 1.2],
      [116, d.maxZ - 1.6, true, 1.3],
      [131, d.maxZ - 1.8, false, 1.25],
      [147, d.maxZ - 1.6, true, 1.35],
      [161, d.maxZ - 2.4, false, 1.5],
      [163, midZ - 6.5, true, 1.6],
      [163, midZ + 6.5, false, 1.6],
    ];
    for (let i = 0; i < dungCrystals.length; i++) {
      const [cx, cz, isPurp, sc] = dungCrystals[i];
      registerCrystalCluster(cx, d.floorY, cz, isPurp, sc, 2100 + i * 47);
    }

    // Sunstone Relic Altar & Floating Spinning Sunstone Relic at (156, 5.5, -24)
    {
      const rx = LANDMARKS.dungeonRelic.x;
      const rz = LANDMARKS.dungeonRelic.z;

      for (let st = 0; st < 3; st++) {
        const rad = 4.6 - st * 0.9;
        const dais = new THREE.Mesh(new THREE.CylinderGeometry(rad, rad + 0.25, 0.3, 12), st === 2 ? dungeonRuneMat : obsidianMat);
        dais.position.set(rx, d.floorY + 0.15 + st * 0.26, rz);
        dungG.add(dais);
      }

      const pedestal = new THREE.Mesh(new THREE.CylinderGeometry(0.72, 1.05, 0.85, 8), dungeonRuneMat);
      pedestal.position.set(rx, d.floorY + 0.95, rz);
      dungG.add(pedestal);

      const relicY = d.floorY + 1.4;
      const relicGroup = new THREE.Group();
      relicGroup.position.set(rx, relicY, rz);

      const coreMat = new THREE.MeshStandardMaterial({
        color: 0xffe88c,
        emissive: 0xffa618,
        emissiveIntensity: 2.3,
        roughness: 0.12,
        metalness: 0.65,
      });
      const coreMesh = new THREE.Mesh(new THREE.OctahedronGeometry(0.68, 0), coreMat);
      relicGroup.add(coreMesh);

      const haloMat = new THREE.MeshStandardMaterial({
        color: 0xffd666,
        emissive: 0xff9b1a,
        emissiveIntensity: 1.9,
        roughness: 0.2,
        metalness: 0.8,
      });
      const ringA = new THREE.Mesh(new THREE.TorusGeometry(1.12, 0.045, 8, 32), haloMat);
      relicGroup.add(ringA);

      const ringB = new THREE.Mesh(new THREE.TorusGeometry(1.45, 0.036, 8, 36), haloMat);
      ringB.rotation.x = Math.PI * 0.5;
      relicGroup.add(ringB);

      const relicHalo = new THREE.Sprite(
        new THREE.SpriteMaterial({
          map: textures.softGlow,
          color: 0xffcc44,
          transparent: true,
          opacity: 0.85,
          blending: THREE.AdditiveBlending,
          depthWrite: false,
        })
      );
      relicHalo.scale.set(4.2, 4.2, 1);
      relicGroup.add(relicHalo);

      dungG.add(relicGroup);

      sunstoneRelic = {
        group: relicGroup,
        x: rx,
        y: relicY,
        z: rz,
        collected: false,
        _core: coreMesh,
        _ringA: ringA,
        _ringB: ringB,
      };
    }

    // Dungeon Wall Colliders (west wall leaves z in [-29, -19] wide open!)
    addBoxCollider(94, 169, -45.5, -42.0, 2, 24);
    addBoxCollider(94, 169, -6.0, -2.5, 2, 24);
    addBoxCollider(166.0, 170.0, -45.5, -2.5, 2, 24);
    addBoxCollider(93.5, 96.5, -45.5, -29.0, 2, 24);
    addBoxCollider(93.5, 96.5, -19.0, -2.5, 2, 24);

    worldGroup.add(dungG);
  }

  // ==========================================================================
  // 8b. FLUSH GLOBAL INSTANCED BRAZIERS, FLAMES & CRYSTAL SHARDS
  // ==========================================================================
  {
    const shardGeo = new THREE.CylinderGeometry(0.02, 0.34, 2.2, 6);
    shardGeo.translate(0, 1.1, 0);
    const purpInst = buildInstancedMesh(shardGeo, purpleCrystalMat, purpleCrystalMats, false, false);
    const ambInst = buildInstancedMesh(shardGeo, amberCrystalMat, amberCrystalMats, false, false);
    if (purpInst) worldGroup.add(purpInst);
    if (ambInst) worldGroup.add(ambInst);

    const bBaseInst = buildInstancedMesh(new THREE.CylinderGeometry(0.34, 0.46, 0.25, 8), ironMat, brazierBaseMats, false, true);
    const bStemInst = buildInstancedMesh(new THREE.CylinderGeometry(0.18, 0.26, 0.65, 8), ironMat, brazierStemMats, false, true);
    const bBowlInst = buildInstancedMesh(new THREE.CylinderGeometry(0.68, 0.34, 0.44, 8), ironMat, brazierBowlMats, false, true);
    const bRimInst = buildInstancedMesh(new THREE.TorusGeometry(0.66, 0.06, 6, 12), ironMat, brazierRimMats, false, false);
    const bCoalInst = buildInstancedMesh(new THREE.SphereGeometry(0.52, 8, 6), coalMat, brazierCoalMats, false, false);
    if (bBaseInst) worldGroup.add(bBaseInst, bStemInst, bBowlInst, bRimInst, bCoalInst);
  }

  // All 21 flames across Bridge, Forge, Castle & Dungeon batched into 3 InstancedMeshes!
  const flameOuterGeo = new THREE.ConeGeometry(0.36, 1.05, 7);
  const flameInnerGeo = new THREE.ConeGeometry(0.2, 0.68, 6);
  const flameOuterMat = new THREE.MeshBasicMaterial({
    color: 0xff6218,
    transparent: true,
    opacity: 0.88,
    blending: THREE.AdditiveBlending,
    depthWrite: false,
  });
  const flameInnerMat = new THREE.MeshBasicMaterial({
    color: 0xffe066,
    transparent: true,
    opacity: 0.95,
    blending: THREE.AdditiveBlending,
    depthWrite: false,
  });
  const flameGlowMat = createBillboardGlowMaterial(textures.softGlow, 0xff9438, 0.72, true);

  const flameOuterInst = new THREE.InstancedMesh(flameOuterGeo, flameOuterMat, Math.max(1, flameSpecs.length));
  const flameInnerInst = new THREE.InstancedMesh(flameInnerGeo, flameInnerMat, Math.max(1, flameSpecs.length));
  const flameGlowInst = new THREE.InstancedMesh(billboardQuadGeo, flameGlowMat, Math.max(1, flameSpecs.length));
  flameOuterInst.castShadow = false;
  flameInnerInst.castShadow = false;
  flameGlowInst.castShadow = false;
  flameOuterInst.frustumCulled = false;
  flameInnerInst.frustumCulled = false;
  flameGlowInst.frustumCulled = false;
  flameGlowInst.renderOrder = 3;
  worldGroup.add(flameOuterInst, flameInnerInst, flameGlowInst);

  // ==========================================================================
  // 9. SKY RUNE RINGS (SLEEK TRANSLUCENT GOLDEN-CYAN MYSTICAL RINGS)
  // ==========================================================================
  const skyRings = [];
  {
    const ringOuterMat = new THREE.MeshStandardMaterial({
      color: 0xffc864,
      emissive: 0xe69226,
      emissiveIntensity: 0.76,
      roughness: 0.26,
      metalness: 0.42,
      transparent: true,
      opacity: 0.68,
      depthWrite: false,
    });
    const ringInnerMat = new THREE.MeshBasicMaterial({
      color: PALETTE.runeCyan,
      transparent: true,
      opacity: 0.48,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });
    const accentStoneMat = new THREE.MeshStandardMaterial({
      map: textures.runeStone,
      color: 0xd8be82,
      emissive: PALETTE.runeGold,
      emissiveMap: textures.runeStone,
      emissiveIntensity: 0.55,
      roughness: 0.58,
      metalness: 0.35,
    });
    const beaconMat = new THREE.SpriteMaterial({
      map: textures.softGlow,
      color: 0xffdf8c,
      transparent: true,
      opacity: 0.22,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
    });

    const accentGeo = new THREE.OctahedronGeometry(0.36, 0);
    const ringOrientations = [0.28, -0.52, 0.68];

    for (let i = 0; i < LANDMARKS.skyRings.length; i++) {
      const cfg = LANDMARKS.skyRings[i];
      // Sleek thin torus (tube = 0.22)
      const mainRing = new THREE.Mesh(new THREE.TorusGeometry(cfg.radius, 0.22, 10, 40), ringOuterMat);
      mainRing.position.set(cfg.x, cfg.y, cfg.z);
      mainRing.rotation.y = ringOrientations[i] || 0;
      mainRing.castShadow = false;
      mainRing.receiveShadow = false;
      mainRing.renderOrder = 3;

      // Subtle inner mystical cyan glow ring
      const innerHoop = new THREE.Mesh(new THREE.TorusGeometry(cfg.radius - 0.5, 0.085, 8, 36), ringInnerMat);
      innerHoop.castShadow = false;
      innerHoop.receiveShadow = false;
      mainRing.add(innerHoop);

      // 8 Small carved floating stone accents around perimeter (1 InstancedMesh per ring!)
      const accentInst = new THREE.InstancedMesh(accentGeo, accentStoneMat, 8);
      accentInst.castShadow = false;
      accentInst.receiveShadow = false;
      for (let k = 0; k < 8; k++) {
        const a = (k / 8) * Math.PI * 2;
        _v1.set(Math.cos(a) * (cfg.radius + 0.48), Math.sin(a) * (cfg.radius + 0.48), 0);
        _euler.set(0, 0, a - Math.PI * 0.5);
        _quat.setFromEuler(_euler);
        _scale.set(0.68, 1.32, 0.52);
        _mat4.compose(_v1, _quat, _scale);
        accentInst.setMatrixAt(k, _mat4);
      }
      accentInst.instanceMatrix.needsUpdate = true;
      accentInst.computeBoundingSphere();
      mainRing.add(accentInst);

      // Whisper-soft central shimmer
      const beacon = new THREE.Sprite(beaconMat);
      beacon.scale.set(cfg.radius * 1.55, cfg.radius * 1.55, 1);
      mainRing.add(beacon);

      worldGroup.add(mainRing);

      skyRings.push({
        mesh: mainRing,
        x: cfg.x,
        y: cfg.y,
        z: cfg.z,
        radius: cfg.radius,
        collected: false,
        _baseY: cfg.y,
        _inner: innerHoop,
      });
    }
  }

  // ==========================================================================
  // 10. SKY DOME, STARFIELD, FULL MOON, SUN RAYS & VALLEY MIST
  // ==========================================================================
  const skyUniforms = {
    uZenithColor: { value: new THREE.Color(0x2e4f8a) },
    uMidSkyColor: { value: new THREE.Color(0xe08952) },
    uHorizonColor: { value: new THREE.Color(0xf7bc74) },
    uSunsetGlowColor: { value: new THREE.Color(0xffbe6b) },
    uSunsetStrength: { value: 0.98 },
    uSunDir: { value: new THREE.Vector3(-0.91, 0.13, -0.38) },
    uMoonDir: { value: new THREE.Vector3(-0.09, 0.47, -0.88) },
    uNightFactor: { value: 0.0 },
    uTime: { value: 0 },
  };

  const skyGroup = new THREE.Group();
  scene.add(skyGroup);

  // 10a. Procedural 3-Stop GLSL Sky Dome (BackSide, radius 420)
  const skyDomeMesh = new THREE.Mesh(
    new THREE.SphereGeometry(420, 28, 18),
    new THREE.ShaderMaterial({
      uniforms: skyUniforms,
      side: THREE.BackSide,
      depthWrite: false,
      fog: false,
      vertexShader: /* glsl */ `
        varying vec3 vDir;
        void main() {
          vDir = normalize(position);
          gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
        }
      `,
      fragmentShader: /* glsl */ `
        uniform vec3 uZenithColor;
        uniform vec3 uMidSkyColor;
        uniform vec3 uHorizonColor;
        uniform vec3 uSunsetGlowColor;
        uniform float uSunsetStrength;
        uniform vec3 uSunDir;
        uniform vec3 uMoonDir;
        uniform float uNightFactor;
        uniform float uTime;
        varying vec3 vDir;

        float hash2(vec2 p) {
          return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
        }
        float vnoise(vec2 p) {
          vec2 i = floor(p);
          vec2 f = fract(p);
          vec2 u = f * f * (3.0 - 2.0 * f);
          return mix(
            mix(hash2(i), hash2(i + vec2(1.0, 0.0)), u.x),
            mix(hash2(i + vec2(0.0, 1.0)), hash2(i + vec2(1.0, 1.0)), u.x),
            u.y
          );
        }
        float cloudFbm(vec2 p) {
          float v = 0.0;
          float a = 0.5;
          for (int i = 0; i < 4; i++) {
            v += a * vnoise(p);
            p = p * 2.04 + vec2(1.7, 9.2);
            a *= 0.48;
          }
          return v;
        }

        void main() {
          vec3 dir = normalize(vDir);
          float h = clamp(dir.y, 0.0, 1.0);

          // Rich 3-stop sky dome vertical gradient: radiant horizon -> warm mid-sky -> twilight/azure zenith
          vec3 lowerSky = mix(uHorizonColor, uMidSkyColor, smoothstep(0.0, 0.28, h));
          vec3 col = mix(lowerSky, uZenithColor, smoothstep(0.18, 0.78, pow(h, 0.72)));

          // Luminous golden-apricot sunset/sunrise horizon glow (linear space, never crimson/maroon!)
          float sunDot = max(dot(dir, uSunDir), 0.0);
          float horizonBand = exp(-abs(dir.y) * 5.6);
          col += uSunsetGlowColor * (0.12 + 0.42 * pow(sunDot, 3.0)) * horizonBand * uSunsetStrength;

          // Stylized painterly high-altitude clouds
          if (dir.y > 0.02) {
            vec2 cuv = dir.xz / (dir.y + 0.24) * 1.45 + vec2(uTime * 0.008, -uTime * 0.004);
            float cn = cloudFbm(cuv);
            float cloudMask = smoothstep(0.54, 0.82, cn) * smoothstep(0.02, 0.22, dir.y) * (1.0 - smoothstep(0.75, 0.98, dir.y));
            vec3 cloudDay = mix(vec3(0.92, 0.94, 0.98), vec3(1.0, 0.64, 0.35), uSunsetStrength);
            vec3 cloudNight = vec3(0.032, 0.06, 0.14);
            vec3 cloudCol = mix(cloudDay, cloudNight, uNightFactor);
            cloudCol += uSunsetGlowColor * pow(sunDot, 4.0) * 0.32 * uSunsetStrength;
            col = mix(col, cloudCol, cloudMask * 0.44);
          }

          // Sun disc + warm Mie atmospheric corona
          float sunAbove = smoothstep(-0.08, 0.06, uSunDir.y);
          float sunHalo = pow(sunDot, 22.0) * 0.48 + pow(sunDot, 160.0) * 1.25;
          float sunDisc = smoothstep(0.9985, 0.9994, sunDot) * 2.8;
          vec3 sunTint = mix(vec3(1.0, 0.94, 0.78), vec3(1.0, 0.66, 0.30), uSunsetStrength);
          col += sunTint * (sunHalo + sunDisc) * sunAbove;

          // Luminous silver-blue moon atmospheric aureole in starry night sky
          float moonDot = max(dot(dir, uMoonDir), 0.0);
          float moonAbove = smoothstep(0.0, 0.14, uMoonDir.y);
          col += vec3(0.24, 0.48, 0.96) * (pow(moonDot, 18.0) * 0.28 + pow(moonDot, 90.0) * 0.36) * moonAbove * uNightFactor;

          gl_FragColor = vec4(col, 1.0);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }
      `,
    })
  );
  skyGroup.add(skyDomeMesh);

  // 10b. Crisp Twinkling Starfield (~1150 unmistakable stars on sky sphere)
  const starUniforms = {
    uTime: { value: 0 },
    uOpacity: { value: 0 },
  };
  let starPoints = null;
  {
    const srng = mulberry32(77312);
    const starCount = 1150;
    const sPos = new Float32Array(starCount * 3);
    const sSize = new Float32Array(starCount);
    const sPhase = new Float32Array(starCount);
    const sCol = new Float32Array(starCount * 3);
    const starTints = [0xffffff, 0xd4ecff, 0xfff0cc, 0xe0eeff];

    for (let i = 0; i < starCount; i++) {
      const u = srng();
      const v = srng();
      const theta = u * Math.PI * 2;
      const phi = Math.acos(Math.max(0.01, v * 0.99 + 0.01)); // upper hemisphere
      const r = 392;
      sPos[i * 3] = r * Math.sin(phi) * Math.cos(theta);
      sPos[i * 3 + 1] = r * Math.cos(phi);
      sPos[i * 3 + 2] = r * Math.sin(phi) * Math.sin(theta);
      sSize[i] = (2.8 + Math.pow(srng(), 2.0) * 5.4) * (isMobile ? 1.15 : 1.45);
      sPhase[i] = srng() * Math.PI * 2;
      _col1.setHex(starTints[Math.floor(srng() * starTints.length)]);
      sCol[i * 3] = _col1.r;
      sCol[i * 3 + 1] = _col1.g;
      sCol[i * 3 + 2] = _col1.b;
    }

    const starGeo = new THREE.BufferGeometry();
    starGeo.setAttribute('position', new THREE.BufferAttribute(sPos, 3));
    starGeo.setAttribute('aSize', new THREE.BufferAttribute(sSize, 1));
    starGeo.setAttribute('aPhase', new THREE.BufferAttribute(sPhase, 1));
    starGeo.setAttribute('aColor', new THREE.BufferAttribute(sCol, 3));

    const starMat = new THREE.ShaderMaterial({
      uniforms: starUniforms,
      transparent: true,
      depthWrite: false,
      fog: false,
      blending: THREE.AdditiveBlending,
      vertexShader: /* glsl */ `
        attribute float aSize;
        attribute float aPhase;
        attribute vec3 aColor;
        uniform float uTime;
        uniform float uOpacity;
        varying vec3 vColor;
        varying float vAlpha;
        void main() {
          vec3 dir = normalize(position);
          float twinkle = 0.72 + 0.28 * sin(uTime * 3.0 + aPhase);
          float elevFade = smoothstep(0.01, 0.10, dir.y);
          vColor = aColor;
          vAlpha = uOpacity * twinkle * elevFade;
          vec4 mv = modelViewMatrix * vec4(position, 1.0);
          gl_PointSize = aSize * (0.88 + 0.28 * twinkle);
          gl_Position = projectionMatrix * mv;
        }
      `,
      fragmentShader: /* glsl */ `
        varying vec3 vColor;
        varying float vAlpha;
        void main() {
          float d = length(gl_PointCoord - 0.5);
          if (d > 0.5) discard;
          float core = smoothstep(0.5, 0.08, d);
          float hotspot = smoothstep(0.24, 0.0, d) * 1.1;
          gl_FragColor = vec4(vColor * (1.45 + hotspot), clamp(core * 1.15, 0.0, 1.0) * vAlpha);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }
      `,
    });

    starPoints = new THREE.Points(starGeo, starMat);
    starPoints.renderOrder = 1;
    skyGroup.add(starPoints);
  }

  // 10c. Luminous Full Moon + Soft Silver-Blue Halo (framed above Highkeep Castle in northern sky!)
  const moonUniforms = { uOpacity: { value: 1.0 } };
  const moonGroup = new THREE.Group();
  {
    const moonMesh = new THREE.Mesh(
      new THREE.SphereGeometry(17.5, 22, 22),
      new THREE.ShaderMaterial({
        uniforms: moonUniforms,
        transparent: true,
        depthWrite: false,
        fog: false,
        vertexShader: /* glsl */ `
          varying vec3 vLocal;
          varying vec3 vNorm;
          void main() {
            vLocal = normalize(position);
            vNorm = normalize(normalMatrix * normal);
            gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
          }
        `,
        fragmentShader: /* glsl */ `
          uniform float uOpacity;
          varying vec3 vLocal;
          varying vec3 vNorm;
          void main() {
            float m1 = sin(vLocal.x * 5.2 + vLocal.y * 3.8) * cos(vLocal.z * 4.6 - vLocal.x * 2.4);
            float m2 = sin(vLocal.y * 9.5 - vLocal.z * 7.2) * 0.5;
            float maria = smoothstep(-0.2, 0.65, m1 * 0.65 + m2 * 0.35);
            vec3 ivory = vec3(1.15, 1.18, 1.25);
            vec3 crater = vec3(0.45, 0.54, 0.72);
            vec3 col = mix(crater, ivory, maria);
            float limb = 0.86 + 0.28 * clamp(vNorm.z, 0.0, 1.0);
            gl_FragColor = vec4(col * limb, uOpacity);
            #include <tonemapping_fragment>
            #include <colorspace_fragment>
          }
        `,
      })
    );
    moonGroup.add(moonMesh);

    const moonHalo = new THREE.Sprite(
      new THREE.SpriteMaterial({
        map: textures.softGlow,
        color: 0xaad4ff,
        transparent: true,
        opacity: 0.62,
        blending: THREE.AdditiveBlending,
        depthWrite: false,
        fog: false,
      })
    );
    moonHalo.scale.set(86, 86, 1);
    moonGroup.add(moonHalo);
    moonGroup.userData._halo = moonHalo;
    skyGroup.add(moonGroup);
  }

  // 10d. Sun Rays (14 God-Ray Shafts batched into 1 InstancedMesh!)
  const rayUniforms = {
    uTime: { value: 0 },
    uOpacity: { value: 0.22 },
    uColor: { value: new THREE.Color(0xffc978) },
  };
  const rayPositions = [
    [-52, 22, 34],
    [-64, 24, 6],
    [-48, 22, -28],
    [-34, 24, -56],
    [48, 22, 32],
    [56, 23, -4],
    [38, 22, 12],
    [28, 22, -48],
    [-24, 20, 44],
    [24, 20, 46],
    [0, 32, -84],
    [-12, 33, -108],
    [12, 33, -108],
    [0, 34, -126],
  ];
  let sunRayInst = null;
  {
    const rayGeo = new THREE.CylinderGeometry(1.7, 5.6, 44, 9, 1, true);
    const rayMat = new THREE.ShaderMaterial({
      uniforms: rayUniforms,
      transparent: true,
      depthWrite: false,
      blending: THREE.AdditiveBlending,
      side: THREE.DoubleSide,
      vertexShader: /* glsl */ `
        varying vec2 vUv;
        varying vec3 vNormalW;
        varying vec3 vViewDirW;
        void main() {
          vUv = uv;
          #ifdef USE_INSTANCING
            vec4 wp = modelMatrix * instanceMatrix * vec4(position, 1.0);
            vNormalW = normalize(mat3(modelMatrix) * mat3(instanceMatrix) * normal);
          #else
            vec4 wp = modelMatrix * vec4(position, 1.0);
            vNormalW = normalize(mat3(modelMatrix) * normal);
          #endif
          vViewDirW = normalize(cameraPosition - wp.xyz);
          gl_Position = projectionMatrix * viewMatrix * wp;
        }
      `,
      fragmentShader: /* glsl */ `
        uniform float uTime;
        uniform float uOpacity;
        uniform vec3 uColor;
        varying vec2 vUv;
        varying vec3 vNormalW;
        varying vec3 vViewDirW;
        void main() {
          float edge = pow(abs(dot(normalize(vNormalW), normalize(vViewDirW))), 1.75);
          float vert = smoothstep(0.0, 0.3, vUv.y) * smoothstep(1.0, 0.22, vUv.y);
          float shimmer = 0.72 + 0.28 * sin(vUv.x * 18.84 + uTime * 0.45);
          gl_FragColor = vec4(uColor, edge * vert * shimmer * uOpacity);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }
      `,
    });

    sunRayInst = new THREE.InstancedMesh(rayGeo, rayMat, rayPositions.length);
    sunRayInst.castShadow = false;
    sunRayInst.receiveShadow = false;
    sunRayInst.frustumCulled = false;
    sunRayInst.renderOrder = 4;
    worldGroup.add(sunRayInst);
  }

  // 10e. 4 Low-Lying Subtle Valley Mist Shader Sheets hovering over Lake & Hollows
  const mistUniforms = {
    uTime: { value: 0 },
    uColor: { value: new THREE.Color(0xf7bc74) },
    uOpacity: { value: 0.2 },
  };
  const mistGroup = new THREE.Group();
  {
    const mistGeo = new THREE.PlaneGeometry(86, 62);
    mistGeo.rotateX(-Math.PI * 0.5);
    const mistMat = new THREE.ShaderMaterial({
      uniforms: mistUniforms,
      transparent: true,
      depthWrite: false,
      side: THREE.DoubleSide,
      vertexShader: /* glsl */ `
        varying vec2 vUv;
        void main() {
          vUv = uv;
          gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
        }
      `,
      fragmentShader: /* glsl */ `
        uniform float uTime;
        uniform vec3 uColor;
        uniform float uOpacity;
        varying vec2 vUv;

        float h2(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
        float vn(vec2 p) {
          vec2 i = floor(p);
          vec2 f = fract(p);
          vec2 u = f * f * (3.0 - 2.0 * f);
          return mix(mix(h2(i), h2(i + vec2(1.0, 0.0)), u.x),
                     mix(h2(i + vec2(0.0, 1.0)), h2(i + vec2(1.0, 1.0)), u.x), u.y);
        }

        void main() {
          vec2 c = vUv * 2.0 - 1.0;
          float radial = 1.0 - smoothstep(0.18, 0.96, length(c));
          float w1 = vn(vUv * 4.5 + vec2(uTime * 0.04, -uTime * 0.025));
          float w2 = vn(vUv * 8.2 - vec2(uTime * 0.03, uTime * 0.045));
          float mist = smoothstep(0.28, 0.85, w1 * 0.6 + w2 * 0.4);
          gl_FragColor = vec4(uColor, radial * mist * uOpacity);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }
      `,
    });

    const mistCoords = [
      [0, 1.35, -8, 1.15, 1.05],
      [-58, 4.6, 6, 0.9, 0.9],
      [54, 4.8, -6, 0.9, 0.85],
      [0, 2.2, -36, 0.85, 0.75],
    ];
    for (let i = 0; i < mistCoords.length; i++) {
      const [mx, my, mz, sx, sz] = mistCoords[i];
      const sh = new THREE.Mesh(mistGeo, mistMat);
      sh.position.set(mx, my, mz);
      sh.scale.set(sx, 1, sz);
      sh.renderOrder = 3;
      mistGroup.add(sh);
    }
    worldGroup.add(mistGroup);
  }

  // Smooth state for subterranean dungeon lighting transition
  let dungeonFactor = 0.0;

  // ==========================================================================
  // PER-FRAME UPDATE (ZERO HEAP ALLOCATIONS!)
  // ==========================================================================
  function update(dt, elapsed, playerPos, timeOfDay = 0.73) {
    const safeDt = Math.min(dt || 0.016, 0.1);
    const px = playerPos ? playerPos.x : 0;
    const py = playerPos ? playerPos.y : 5;
    const pz = playerPos ? playerPos.z : 0;

    // Update GPU shader timers
    grassUniforms.uTime.value = elapsed;
    canopyUniforms.uTime.value = elapsed;
    bannerUniforms.uTime.value = elapsed;
    waterUniforms.uTime.value = elapsed;
    lavaUniforms.uTime.value = elapsed;
    skyUniforms.uTime.value = elapsed;
    starUniforms.uTime.value = elapsed;
    rayUniforms.uTime.value = elapsed;
    mistUniforms.uTime.value = elapsed;

    // 1. Sun & Moon orbital directions (0.25=dawn, 0.50=noon, 0.73=golden sunset, 0.02=starry full-moon night)
    const tod = ((timeOfDay % 1) + 1) % 1;
    const sunAngle = (tod - 0.25) * Math.PI * 2.0;
    _sunDir.set(Math.cos(sunAngle) * 0.92, Math.sin(sunAngle), -0.38).normalize();

    // Full Moon glides across the northern sky (z < 0), peaking ~28° above Highkeep Castle at tod ~0.02!
    const moonPhase = sunAngle + Math.PI;
    _moonDir.set(Math.cos(moonPhase) * 0.72, Math.sin(moonPhase) * 0.45, -0.84).normalize();

    const sunElev = _sunDir.y;
    const duskToSunset = smoothstep(-0.18, 0.08, sunElev);
    const dayFactor = smoothstep(0.14, 0.56, sunElev);
    const nightFactor = smoothstep(0.05, -0.16, sunElev);
    const sunsetStrength = Math.max(0, 1.0 - Math.abs(sunElev - 0.12) / 0.3) * (1.0 - nightFactor * 0.85);

    // 2. Interpolate 3-stop Sky Dome (Zenith / Mid-Sky / Horizon)
    _colZenith.copy(_cZenithNight).lerp(_cZenithSunset, duskToSunset).lerp(_cZenithDay, dayFactor);
    _colMidSky.copy(_cMidNight).lerp(_cMidSunset, duskToSunset).lerp(_cMidDay, dayFactor);
    _colHorizon.copy(_cHorizonNight).lerp(_cHorizonSunset, duskToSunset).lerp(_cHorizonDay, dayFactor);

    skyUniforms.uZenithColor.value.copy(_colZenith);
    skyUniforms.uMidSkyColor.value.copy(_colMidSky);
    skyUniforms.uHorizonColor.value.copy(_colHorizon);
    skyUniforms.uSunsetGlowColor.value.copy(_cSunsetGlow);
    skyUniforms.uSunsetStrength.value = sunsetStrength;
    skyUniforms.uSunDir.value.copy(_sunDir);
    skyUniforms.uMoonDir.value.copy(_moonDir);
    skyUniforms.uNightFactor.value = nightFactor;

    // Keep sky dome, stars & full moon centered horizontally on player so horizon stays endless
    skyGroup.position.set(px, 0, pz);

    // 3. Smooth subterranean dungeon atmosphere transition
    const inDung = isInsideDungeon(px, pz) ? 1.0 : 0.0;
    const blendRate = 1.0 - Math.exp(-safeDt * 4.5);
    dungeonFactor += (inDung - dungeonFactor) * blendRate;

    // Starfield fade-in during dusk & night
    starUniforms.uOpacity.value = smoothstep(0.08, -0.1, sunElev) * (1.0 - dungeonFactor);
    starPoints.visible = starUniforms.uOpacity.value > 0.01;

    // Full Moon position & visibility above Highkeep Castle (z < 0, elevation ~28° at tod = 0.02)
    const moonVis = smoothstep(0.01, 0.14, _moonDir.y) * (1.0 - dungeonFactor);
    moonGroup.position.set(_moonDir.x * 340, _moonDir.y * 340, _moonDir.z * 340);
    moonUniforms.uOpacity.value = moonVis;
    moonGroup.userData._halo.material.opacity = moonVis * (0.34 + 0.4 * nightFactor);
    moonGroup.visible = moonVis > 0.01;

    // 4. Directional Sun / Moonlight transition (rich golden hour sunset ~2.6, crisp silver moonlight ~1.1)
    const isNightLight = sunElev < -0.02;
    if (isNightLight) {
      _lightDir.copy(_moonDir);
      _colSun.copy(_cMoonLight);
    } else {
      _lightDir.copy(_sunDir);
      _colSun.copy(_cSunSunset).lerp(_cSunDay, dayFactor);
    }
    if (_lightDir.y < 0.22) {
      _lightDir.y = 0.22;
      _lightDir.normalize();
    }

    const horizonDip = 1.0 - Math.exp(-Math.pow((sunElev + 0.02) * 14.0, 2.0)) * 0.65;
    const outdoorSunInt = isNightLight
      ? (0.72 + 0.38 * smoothstep(-0.02, -0.22, sunElev)) * horizonDip
      : (2.62 + 0.23 * dayFactor) * horizonDip;

    sunLight.color.copy(_colSun);
    sunLight.intensity = THREE.MathUtils.lerp(outdoorSunInt, 0.02, dungeonFactor);

    // Snap shadow camera center to 2-unit grid around playerPos to prevent shadow swimming
    const snapX = Math.round(px * 0.5) * 2.0;
    const snapZ = Math.round(pz * 0.5) * 2.0;
    sunLight.target.position.set(snapX, py * 0.3, snapZ);
    sunLight.position.set(
      snapX + _lightDir.x * 115,
      py * 0.3 + _lightDir.y * 115,
      snapZ + _lightDir.z * 115
    );

    // Hemisphere outdoor (~1.15 lush golden sunset, ~0.52 silver-blue night, ~1.18 day) vs subterranean ember-obsidian
    _colHemiSky.copy(_cHemiSkyNight).lerp(_cHemiSkySunset, duskToSunset).lerp(_cHemiSkyDay, dayFactor);
    _colHemiGround.copy(_cHemiGndNight).lerp(_cHemiGndSunset, duskToSunset).lerp(_cHemiGndDay, dayFactor);
    const outdoorHemiInt = THREE.MathUtils.lerp(
      THREE.MathUtils.lerp(0.52, 1.15, duskToSunset),
      1.18,
      dayFactor
    );

    hemiLight.color.copy(_colHemiSky).lerp(_cDungHemiSky, dungeonFactor);
    hemiLight.groundColor.copy(_colHemiGround).lerp(_cDungHemiGnd, dungeonFactor);
    hemiLight.intensity = THREE.MathUtils.lerp(outdoorHemiInt, 0.28, dungeonFactor);

    // 5. Dynamic Exponential Valley Fog vs Dense Subterranean Cavern Smoke
    const outdoorFogDensity = 0.0029 + sunsetStrength * 0.0009 + nightFactor * 0.0008;
    _colFog.copy(_colHorizon).lerp(_cDungFog, dungeonFactor);
    scene.fog.color.copy(_colFog);
    scene.fog.density = THREE.MathUtils.lerp(outdoorFogDensity, 0.026, dungeonFactor);

    // Sync Mirror Lake & Lava shaders to dynamic sky/fog
    waterUniforms.uSunDir.value.copy(_sunDir);
    waterUniforms.uMoonDir.value.copy(_moonDir);
    waterUniforms.uSunColor.value.copy(_colSun);
    waterUniforms.uSkyZenith.value.copy(_colZenith);
    waterUniforms.uSkyHorizon.value.copy(_colHorizon);
    waterUniforms.uSunStrength.value = smoothstep(-0.04, 0.12, sunElev);
    waterUniforms.uMoonStrength.value = smoothstep(0.02, -0.14, sunElev);
    waterUniforms.uFogColor.value.copy(scene.fog.color);
    waterUniforms.uFogDensity.value = scene.fog.density;

    lavaUniforms.uFogColor.value.copy(scene.fog.color);
    lavaUniforms.uFogDensity.value = scene.fog.density;

    // 6. Sun Rays (1 InstancedMesh oriented along sun angle, fading out at night & inside dungeon)
    const rayAlpha = smoothstep(0.0, 0.14, sunElev) * (0.14 + 0.16 * sunsetStrength) * (1.0 - dungeonFactor);
    rayUniforms.uOpacity.value = rayAlpha;
    rayUniforms.uColor.value.copy(_cRayDay).lerp(_cRaySunset, sunsetStrength);
    const raysVisible = rayAlpha > 0.005;
    sunRayInst.visible = raysVisible;
    if (raysVisible) {
      _v1.copy(_sunDir);
      if (_v1.y < 0.22) {
        _v1.y = 0.22;
        _v1.normalize();
      }
      _quat.setFromUnitVectors(_up, _v1);
      _scale.set(1, 1, 1);
      for (let i = 0; i < rayPositions.length; i++) {
        const rp = rayPositions[i];
        _v2.set(rp[0], rp[1], rp[2]);
        _mat4.compose(_v2, _quat, _scale);
        sunRayInst.setMatrixAt(i, _mat4);
      }
      sunRayInst.instanceMatrix.needsUpdate = true;
    }

    // Valley Mist sheets synced to horizon tint & faded in dungeon
    mistUniforms.uColor.value.copy(_colHorizon);
    mistUniforms.uOpacity.value = (0.15 + 0.1 * sunsetStrength + 0.05 * nightFactor) * (1.0 - dungeonFactor);
    mistGroup.visible = dungeonFactor < 0.92;
    skyDomeMesh.visible = dungeonFactor < 0.95;

    // 7. Cottage Amber Windows & Village Lanterns glow warmly at dusk & night
    const duskGlow = 0.55 + 1.85 * Math.max(nightFactor, sunsetStrength * 0.78);
    cottageWindowMat.emissiveIntensity = duskGlow;
    lanternGlassMat.emissiveIntensity = duskGlow * 1.18;
    lanternHaloMat.uniforms.uOpacity.value = (0.36 + 0.42 * Math.max(nightFactor, sunsetStrength * 0.75)) * (1.0 - dungeonFactor);

    // Pulse ancient runes & glowing crystal clusters gently
    const runePulse = 0.78 + 0.26 * Math.sin(elapsed * 2.1);
    for (let i = 0; i < runeMaterials.length; i++) {
      runeMaterials[i].emissiveIntensity = (i < 2 ? 0.85 : 1.45) * runePulse;
    }

    // 8. Animate Batched Brazier, Forge & Torch Flames (3 InstancedMeshes, zero allocations!)
    for (let i = 0; i < flameSpecs.length; i++) {
      const f = flameSpecs[i];
      const s = f.scale;
      const t = elapsed * 9.5 + f.phase;
      const flick = 0.86 + 0.22 * Math.sin(t) + 0.12 * Math.cos(t * 1.73);

      // Outer cone
      _v1.set(f.x, f.y + 0.45 * s, f.z);
      _euler.set(0, elapsed * 1.6 + f.phase, 0);
      _quat.setFromEuler(_euler);
      _scale.set(s * (0.92 + 0.14 * Math.cos(t * 1.3)), s * flick, s * (0.92 + 0.14 * Math.sin(t * 1.1)));
      _mat4.compose(_v1, _quat, _scale);
      flameOuterInst.setMatrixAt(i, _mat4);

      // Inner cone
      _v1.set(f.x, f.y + 0.34 * s, f.z);
      _quat.identity();
      _scale.set(s * (0.9 + 0.12 * Math.sin(t * 1.6)), s * (0.85 + 0.25 * Math.cos(t * 1.2)), s * (0.9 + 0.12 * Math.cos(t * 1.4)));
      _mat4.compose(_v1, _quat, _scale);
      flameInnerInst.setMatrixAt(i, _mat4);

      // Billboarded glow halo
      const gs = s * (2.05 + 0.35 * flick);
      _v1.set(f.x, f.y + 0.45 * s, f.z);
      _scale.set(gs, gs, 1);
      _mat4.compose(_v1, _quat, _scale);
      flameGlowInst.setMatrixAt(i, _mat4);
    }
    flameOuterInst.instanceMatrix.needsUpdate = true;
    flameInnerInst.instanceMatrix.needsUpdate = true;
    flameGlowInst.instanceMatrix.needsUpdate = true;

    const nightMult = 0.45 + 0.9 * Math.max(nightFactor, sunsetStrength * 0.7);
    for (let i = 0; i < flickeringLights.length; i++) {
      const fl = flickeringLights[i];
      const wave = Math.sin(elapsed * 7.8 + fl.phase) * 0.65 + Math.cos(elapsed * 13.4 + fl.phase * 2.1) * 0.35;
      fl.light.intensity = (fl.base + wave * fl.amp) * (fl.nightBoost ? nightMult : 1.0);
    }

    // 9. Animate Batched Oakhaven Chimney Smoke Wisps (1 InstancedMesh, zero allocations!)
    if (smokeInst) {
      const smokeVis = dungeonFactor < 0.95;
      smokeInst.visible = smokeVis;
      if (smokeVis) {
        _quat.identity();
        for (let i = 0; i < smokeWisps.length; i++) {
          const sw = smokeWisps[i];
          const life = (elapsed * 0.16 + sw.phase) % 1.0;
          const driftX = Math.sin(elapsed * 0.7 + sw.phase * 6.0) * 0.65 + life * 2.1;
          const driftZ = Math.cos(elapsed * 0.55 + sw.phase * 5.0) * 0.45 - life * 0.9;
          const sc = 0.85 + life * 2.9;
          _v1.set(sw.baseX + driftX, sw.baseY + life * 7.2, sw.baseZ + driftZ);
          _scale.set(sc, sc, 1);
          _mat4.compose(_v1, _quat, _scale);
          smokeInst.setMatrixAt(i, _mat4);
          smokeAlphaAttr.setX(i, Math.sin(life * Math.PI) * 0.26 * (1.0 - dungeonFactor));
        }
        smokeInst.instanceMatrix.needsUpdate = true;
        smokeAlphaAttr.needsUpdate = true;
      }
    }

    // 9b. Will-o'-wisps drift and blink over the lake & Rune Circle (dusk/night only, 1 InstancedMesh, zero allocations!)
    if (wispInst) {
      const wispNight = Math.max(nightFactor, sunsetStrength * 0.72) * (1.0 - dungeonFactor);
      const wispVis = wispNight > 0.02;
      wispInst.visible = wispVis;
      if (wispVis) {
        _quat.identity();
        for (let i = 0; i < willWisps.length; i++) {
          const w = willWisps[i];
          const t = elapsed * w.speed + w.phase;
          const driftX = Math.sin(t * 0.7) * w.span + Math.sin(t * 1.7) * 0.35;
          const driftZ = Math.cos(t * 0.55) * w.span * 0.8 + Math.cos(t * 1.3) * 0.3;
          const bobY = Math.sin(t * 1.15) * 0.55;
          // Slow blink: bright pulses separated by long dim valleys
          const blink = Math.pow(Math.max(0, Math.sin(elapsed * 1.35 + w.phase * 2.7)), 3.0);
          const scale = 0.2 + 0.18 * blink;
          _v1.set(w.x + driftX, w.y + bobY, w.z + driftZ);
          _scale.set(scale, scale, 1);
          _mat4.compose(_v1, _quat, _scale);
          wispInst.setMatrixAt(i, _mat4);
          wispAlphaAttr.setX(i, (0.16 + 0.84 * blink) * wispNight * 0.85);
        }
        wispInst.instanceMatrix.needsUpdate = true;
        wispAlphaAttr.needsUpdate = true;
      }
    }

    // 10. Animate Sky Rune Rings (Dragon Flight Quest)
    for (let i = 0; i < skyRings.length; i++) {
      const r = skyRings[i];
      if (r.collected) {
        if (r.mesh.visible) {
          const nextS = Math.max(0.001, r.mesh.scale.x - safeDt * 2.6);
          r.mesh.scale.set(nextS, nextS, nextS);
          r.mesh.rotation.z += safeDt * 5.0;
          if (nextS <= 0.01) r.mesh.visible = false;
        }
      } else {
        if (!r.mesh.visible) {
          r.mesh.visible = true;
          r.mesh.scale.set(1, 1, 1);
        }
        r.mesh.position.y = r._baseY + Math.sin(elapsed * 1.45 + i * 2.1) * 0.75;
        r.mesh.rotation.z = elapsed * 0.42 + i * 1.2;
        r._inner.rotation.z = -elapsed * 1.1;
      }
    }

    // 11. Animate Floating Sunstone Relic in Ember Crypt
    if (sunstoneRelic) {
      if (sunstoneRelic.collected) {
        sunstoneRelic.group.visible = false;
      } else {
        sunstoneRelic.group.visible = true;
        sunstoneRelic.group.position.y = sunstoneRelic.y + Math.sin(elapsed * 2.4) * 0.26;
        sunstoneRelic._core.rotation.y = elapsed * 1.5;
        sunstoneRelic._core.rotation.x = Math.sin(elapsed * 1.1) * 0.25;
        sunstoneRelic._ringA.rotation.x = elapsed * 1.3;
        sunstoneRelic._ringA.rotation.y = elapsed * 0.9;
        sunstoneRelic._ringB.rotation.y = -elapsed * 1.45;
        sunstoneRelic._ringB.rotation.z = elapsed * 0.75;
      }
    }
  }

  // Run one initial lighting pass at default golden hour so frame 0 looks gorgeous immediately
  update(0.016, 0, _v3.set(LANDMARKS.spawn.x, 5, LANDMARKS.spawn.z), 0.73);

  return {
    sunLight,
    hemiLight,
    skyRings,
    sunstoneRelic,
    update,
  };
}
