// ============================================================================
// src/entities.js — Articulated Hero, Rideable Red Dragon (Ignis), & Villagers
// ============================================================================
import * as THREE from 'three';
import {
  WORLD,
  PALETTE,
  LANDMARKS,
  getTerrainHeight,
  getWalkableHeight,
  resolveWorldCollisions,
  isInsideDungeon,
} from './config.js';

// Preallocated scratch objects (zero per-frame heap allocations)
const _v1 = new THREE.Vector3();
const _v2 = new THREE.Vector3();
const _v3 = new THREE.Vector3();
const _v4 = new THREE.Vector3();
const _upAxis = new THREE.Vector3(0, 1, 0);
const _vpSize = new THREE.Vector2(1280, 720);
const _cTemp = new THREE.Color();
const _cWhite = new THREE.Color(0xfffbe6);
const _cGold = new THREE.Color(0xffd244);
const _cOrange = new THREE.Color(0xff520c);
const _cCrimson = new THREE.Color(0xa81206);
const _cEmberDark = new THREE.Color(0x280402);
const _cFlash = new THREE.Color(0x881818);
const _cZero = new THREE.Color(0x000000);

function wrapAngle(a) {
  a = (a + Math.PI) % (Math.PI * 2);
  return a < 0 ? a + Math.PI : a - Math.PI;
}

function lerpAngle(current, target, t) {
  const diff = wrapAngle(target - current);
  return current + diff * Math.min(1, t);
}

function asMajor(mesh) {
  mesh.userData.majorShadow = true;
  return mesh;
}

function applyShadows(root) {
  root.traverse((obj) => {
    if (obj.isMesh && obj.material && !obj.material.transparent) {
      obj.castShadow = !!obj.userData.majorShadow;
      obj.receiveShadow = !(obj.material.emissiveIntensity && obj.material.emissiveIntensity >= 0.5);
    }
  });
}

// ============================================================================
// Procedural Geometry Builders (Hero Shield, Greatsword, Slash Arc, Dragon Wings)
// ============================================================================

function createKiteShieldGeometry(scale = 1, depth = 0.05) {
  const s = new THREE.Shape();
  s.moveTo(0, 0.42 * scale);
  s.quadraticCurveTo(0.26 * scale, 0.40 * scale, 0.30 * scale, 0.24 * scale);
  s.quadraticCurveTo(0.29 * scale, -0.08 * scale, 0, -0.48 * scale);
  s.quadraticCurveTo(-0.29 * scale, -0.08 * scale, -0.30 * scale, 0.24 * scale);
  s.quadraticCurveTo(-0.26 * scale, 0.40 * scale, 0, 0.42 * scale);

  const geo = new THREE.ExtrudeGeometry(s, {
    depth,
    bevelEnabled: true,
    bevelThickness: 0.016 * scale,
    bevelSize: 0.014 * scale,
    bevelSegments: 1,
    curveSegments: 7,
  });
  geo.center();
  return geo;
}

function createGreatswordBladeGeometry() {
  const s = new THREE.Shape();
  s.moveTo(-0.058, 0);
  s.lineTo(0.058, 0);
  s.lineTo(0.044, 0.86);
  s.lineTo(0, 1.06);
  s.lineTo(-0.044, 0.86);
  s.closePath();

  const geo = new THREE.ExtrudeGeometry(s, {
    depth: 0.018,
    bevelEnabled: true,
    bevelThickness: 0.011,
    bevelSize: 0.008,
    bevelSegments: 1,
  });
  geo.translate(0, 0, -0.009);
  return geo;
}

function createSlashArcGeometry() {
  const segs = 20;
  const positions = new Float32Array((segs + 1) * 2 * 3);
  const colors = new Float32Array((segs + 1) * 2 * 3);
  const indices = [];

  const startAngle = -1.32;
  const sweepAngle = 2.64;

  for (let i = 0; i <= segs; i++) {
    const u = i / segs; // 0 = trailing tail, 1 = leading blade tip
    const angle = startAngle + u * sweepAngle;
    const sinA = Math.sin(angle);
    const cosA = Math.cos(angle);

    // Sculpted crescent thickness: broad in front-mid, razor sharp tip & tapering tail
    const crescent = Math.sin(u * Math.PI) * (0.35 + 0.65 * u);
    const rOuter = 2.48;
    const rInner = Math.max(0.85, rOuter - 1.38 * crescent);

    const vIdx = i * 2;
    positions[vIdx * 3] = sinA * rOuter;
    positions[vIdx * 3 + 1] = 0.02 * Math.sin(u * Math.PI);
    positions[vIdx * 3 + 2] = cosA * rOuter;

    positions[(vIdx + 1) * 3] = sinA * rInner;
    positions[(vIdx + 1) * 3 + 1] = -0.02 * Math.sin(u * Math.PI);
    positions[(vIdx + 1) * 3 + 2] = cosA * rInner;

    // Additive color gradient: black tail -> luminous cyan-gold body -> white-gold leading edge
    const fade = Math.pow(u, 1.35) * Math.sin(Math.pow(u, 0.65) * Math.PI);
    const leadBoost = Math.pow(u, 3.0);

    // Outer razor edge (bright gold-white)
    colors[vIdx * 3] = Math.min(1, fade * 1.15 + leadBoost * 0.65);
    colors[vIdx * 3 + 1] = Math.min(1, fade * 0.92 + leadBoost * 0.65);
    colors[vIdx * 3 + 2] = Math.min(1, fade * 0.65 + leadBoost * 0.55);

    // Inner trail (cyan-gold magical sheen, softer)
    colors[(vIdx + 1) * 3] = fade * 0.22;
    colors[(vIdx + 1) * 3 + 1] = fade * 0.58;
    colors[(vIdx + 1) * 3 + 2] = fade * 0.72;

    if (i < segs) {
      const a = vIdx;
      const b = vIdx + 1;
      const c = vIdx + 2;
      const d = vIdx + 3;
      indices.push(a, b, c, b, d, c);
    }
  }

  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  geo.setAttribute('color', new THREE.BufferAttribute(colors, 3));
  geo.setIndex(indices);
  geo.computeVertexNormals();
  return geo;
}

function createCapeSegmentGeometry(topW, botW, height) {
  const geo = new THREE.BufferGeometry();
  const hwT = topW * 0.5;
  const hwB = botW * 0.5;
  const positions = new Float32Array([
    -hwT, 0, 0,
    0, 0, -0.04,
    hwT, 0, 0,
    -hwB, -height, -0.02,
    0, -height, -0.08,
    hwB, -height, -0.02,
  ]);
  const indices = [
    0, 3, 1, 1, 3, 4,
    1, 4, 2, 2, 4, 5,
  ];
  geo.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  geo.setIndex(indices);
  geo.computeVertexNormals();
  return geo;
}

function createTriangleFanWeb(pts) {
  // pts: array of [x, y, z] triplets forming a convex/fan membrane from pts[0]
  const verts = [];
  for (let i = 1; i < pts.length - 1; i++) {
    verts.push(...pts[0], ...pts[i], ...pts[i + 1]);
  }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.Float32BufferAttribute(verts, 3));
  geo.computeVertexNormals();
  return geo;
}

function makeSparBetween(p0, p1, r0, r1, mat) {
  const dir = new THREE.Vector3().subVectors(p1, p0);
  const len = dir.length();
  const mesh = new THREE.Mesh(new THREE.CylinderGeometry(r1, r0, len, 6), mat);
  mesh.position.copy(p0).addScaledVector(dir, 0.5);
  mesh.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), dir.normalize());
  return mesh;
}

// ============================================================================
// 1. CREATE HERO — Stylized Articulated Fantasy Knight / Ranger
// ============================================================================

export function createHero(scene, textures) {
  const group = new THREE.Group();
  const spawnX = LANDMARKS.spawn.x;
  const spawnZ = LANDMARKS.spawn.z;
  const spawnY = getWalkableHeight(spawnX, spawnZ, 10);
  group.position.set(spawnX, spawnY, spawnZ);
  group.rotation.y = Math.PI; // Facing North (-Z) toward bridge & lake

  // Materials (calibrated for crisp readability under warm sunset & moonlit shadows)
  const steelMat = new THREE.MeshStandardMaterial({
    color: 0x6c7f9e,
    metalness: 0.45,
    roughness: 0.42,
  });
  const brightSteelMat = new THREE.MeshStandardMaterial({
    color: 0xc8d6ea,
    metalness: 0.52,
    roughness: 0.30,
  });
  const navyMat = new THREE.MeshStandardMaterial({
    color: 0x284e94,
    metalness: 0.16,
    roughness: 0.58,
  });
  const goldMat = new THREE.MeshStandardMaterial({
    color: 0xf4c452,
    metalness: 0.54,
    roughness: 0.34,
  });
  const leatherMat = new THREE.MeshStandardMaterial({
    color: 0x5c3a24,
    metalness: 0.1,
    roughness: 0.72,
  });
  const capeMat = new THREE.MeshStandardMaterial({
    color: 0xc2283c,
    roughness: 0.62,
    metalness: 0.08,
    side: THREE.DoubleSide,
  });
  const capeTrimMat = new THREE.MeshStandardMaterial({
    color: 0x354fa8,
    roughness: 0.56,
    metalness: 0.18,
    side: THREE.DoubleSide,
  });
  const visorDarkMat = new THREE.MeshStandardMaterial({
    color: 0x101522,
    roughness: 0.48,
    metalness: 0.28,
  });
  const runeCyanMat = new THREE.MeshStandardMaterial({
    color: PALETTE.runeCyan,
    emissive: PALETTE.runeCyan,
    emissiveIntensity: 1.6,
    roughness: 0.2,
    metalness: 0.2,
  });
  const runeGoldMat = new THREE.MeshStandardMaterial({
    color: PALETTE.runeGold,
    emissive: PALETTE.runeGold,
    emissiveIntensity: 1.2,
    roughness: 0.2,
    metalness: 0.4,
  });

  // Track flashable armor materials for hitFlash without per-frame allocation
  const flashMats = [steelMat, navyMat, goldMat, capeMat];

  // Center-of-mass pivot (used for roll somersault, lean, jump squash, death collapse)
  const bodyPivot = new THREE.Group();
  bodyPivot.position.set(0, 0.96, 0);
  group.add(bodyPivot);

  // ---- Torso & Armored Cuirass ----
  const torsoGroup = new THREE.Group();
  torsoGroup.position.set(0, 0.06, 0);
  bodyPivot.add(torsoGroup);

  const cuirass = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.24, 0.18, 0.48, 8), steelMat));
  cuirass.scale.set(1.14, 1.0, 0.76);
  cuirass.position.set(0, 0.08, 0);
  torsoGroup.add(cuirass);

  // Sculpted upper pectoral armor plate
  const chestPlate = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.42, 0.22, 0.28), steelMat));
  chestPlate.position.set(0, 0.16, 0.04);
  torsoGroup.add(chestPlate);

  // Sapphire-gold surcoat tabard front & back
  const tabardFront = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.24, 0.54, 0.04), navyMat));
  tabardFront.position.set(0, -0.06, 0.175);
  torsoGroup.add(tabardFront);

  const tabardTrim = new THREE.Mesh(new THREE.BoxGeometry(0.26, 0.045, 0.048), goldMat);
  tabardTrim.position.set(0, -0.31, 0.176);
  torsoGroup.add(tabardTrim);

  // Gold chest chevron crest
  const crestGeo = new THREE.BoxGeometry(0.14, 0.028, 0.03);
  const crestL = new THREE.Mesh(crestGeo, goldMat);
  crestL.position.set(0.055, 0.17, 0.185);
  crestL.rotation.z = 0.36;
  torsoGroup.add(crestL);
  const crestR = new THREE.Mesh(crestGeo, goldMat);
  crestR.position.set(-0.055, 0.17, 0.185);
  crestR.rotation.z = -0.36;
  torsoGroup.add(crestR);

  // Gorget collar
  const gorget = new THREE.Mesh(new THREE.CylinderGeometry(0.145, 0.185, 0.09, 7), goldMat);
  gorget.position.set(0, 0.34, 0);
  torsoGroup.add(gorget);

  // War belt & hip tassets
  const belt = new THREE.Mesh(new THREE.CylinderGeometry(0.21, 0.21, 0.09, 8), leatherMat);
  belt.scale.set(1.08, 1, 0.82);
  belt.position.set(0, -0.15, 0);
  torsoGroup.add(belt);

  const buckle = new THREE.Mesh(new THREE.BoxGeometry(0.10, 0.085, 0.05), goldMat);
  buckle.position.set(0, -0.15, 0.17);
  torsoGroup.add(buckle);

  const tassetGeo = new THREE.BoxGeometry(0.12, 0.20, 0.22);
  const tassetTrimGeo = new THREE.BoxGeometry(0.128, 0.028, 0.226);
  for (let side = -1; side <= 1; side += 2) {
    const tasset = asMajor(new THREE.Mesh(tassetGeo, steelMat));
    tasset.position.set(side * 0.21, -0.24, 0);
    tasset.rotation.z = side * 0.22;
    torsoGroup.add(tasset);
    const tassetTrim = new THREE.Mesh(tassetTrimGeo, goldMat);
    tassetTrim.position.set(side * 0.22, -0.33, 0);
    tassetTrim.rotation.z = side * 0.22;
    torsoGroup.add(tassetTrim);
  }

  // ---- Head, Sculpted Helmet/Hood, Visor & Plume ----
  const headGroup = new THREE.Group();
  headGroup.position.set(0, 0.39, 0);
  torsoGroup.add(headGroup);

  const neck = new THREE.Mesh(new THREE.CylinderGeometry(0.085, 0.10, 0.10, 6), visorDarkMat);
  neck.position.set(0, 0.04, 0);
  headGroup.add(neck);

  // Knight helmet cranium
  const helm = asMajor(new THREE.Mesh(new THREE.SphereGeometry(0.175, 10, 8), steelMat));
  helm.scale.set(0.96, 1.08, 1.04);
  helm.position.set(0, 0.20, 0);
  headGroup.add(helm);

  // Armored ranger cowl/hood framing helm back & sides
  const hood = asMajor(new THREE.Mesh(
    new THREE.SphereGeometry(0.195, 8, 6, Math.PI * 0.22, Math.PI * 1.56, 0, Math.PI * 0.68),
    navyMat
  ));
  hood.rotation.y = Math.PI * 0.5;
  hood.position.set(0, 0.21, -0.02);
  headGroup.add(hood);

  // Angular face visor plate & dark slit
  const visorPlate = new THREE.Mesh(new THREE.CylinderGeometry(0.155, 0.135, 0.17, 6, 1, false, -0.95, 1.9), steelMat);
  visorPlate.position.set(0, 0.17, 0.04);
  headGroup.add(visorPlate);

  const visorSlit = new THREE.Mesh(new THREE.BoxGeometry(0.20, 0.036, 0.05), visorDarkMat);
  visorSlit.position.set(0, 0.205, 0.165);
  headGroup.add(visorSlit);

  // Subtle heroic eye gleam inside visor
  const eyeGleamGeo = new THREE.BoxGeometry(0.042, 0.015, 0.02);
  const eyeGleamL = new THREE.Mesh(eyeGleamGeo, runeCyanMat);
  eyeGleamL.position.set(0.048, 0.206, 0.184);
  headGroup.add(eyeGleamL);
  const eyeGleamR = new THREE.Mesh(eyeGleamGeo, runeCyanMat);
  eyeGleamR.position.set(-0.048, 0.206, 0.184);
  headGroup.add(eyeGleamR);

  // Forged gold brow & crest ridge
  const helmCrest = new THREE.Mesh(new THREE.BoxGeometry(0.042, 0.09, 0.30), goldMat);
  helmCrest.position.set(0, 0.37, -0.01);
  headGroup.add(helmCrest);

  // Flowing helmet plume (sways with movement)
  const plumePivot = new THREE.Group();
  plumePivot.position.set(0, 0.38, -0.14);
  headGroup.add(plumePivot);

  const plumeA = new THREE.Mesh(new THREE.ConeGeometry(0.052, 0.24, 6), capeMat);
  plumeA.rotation.x = -1.25;
  plumeA.position.set(0, 0.01, -0.10);
  plumePivot.add(plumeA);

  const plumeB = new THREE.Mesh(new THREE.ConeGeometry(0.036, 0.20, 6), capeTrimMat);
  plumeB.rotation.x = -1.65;
  plumeB.position.set(0, -0.06, -0.23);
  plumePivot.add(plumeB);

  // ---- Flowing 2-Segment Crimson-Sapphire Cape ----
  const capeUpperPivot = new THREE.Group();
  capeUpperPivot.position.set(0, 0.30, -0.16);
  torsoGroup.add(capeUpperPivot);

  const capeUpper = asMajor(new THREE.Mesh(createCapeSegmentGeometry(0.54, 0.68, 0.48), capeMat));
  capeUpperPivot.add(capeUpper);

  const capeUpperInlay = new THREE.Mesh(createCapeSegmentGeometry(0.28, 0.36, 0.47), capeTrimMat);
  capeUpperInlay.position.z = -0.008;
  capeUpperPivot.add(capeUpperInlay);

  const capeLowerPivot = new THREE.Group();
  capeLowerPivot.position.set(0, -0.47, -0.02);
  capeUpperPivot.add(capeLowerPivot);

  const capeLower = asMajor(new THREE.Mesh(createCapeSegmentGeometry(0.68, 0.84, 0.50), capeMat));
  capeLowerPivot.add(capeLower);

  const capeHem = new THREE.Mesh(new THREE.BoxGeometry(0.80, 0.042, 0.022), goldMat);
  capeHem.position.set(0, -0.48, -0.045);
  capeLowerPivot.add(capeHem);

  // Shared lean limb/armor geometries
  const pauldronGeo = new THREE.SphereGeometry(0.145, 8, 6, 0, Math.PI * 2, 0, Math.PI * 0.62);
  const pauldronRimGeo = new THREE.TorusGeometry(0.135, 0.018, 5, 8, Math.PI * 1.15);
  const upperArmGeo = new THREE.CylinderGeometry(0.075, 0.068, 0.28, 6);
  const vambraceGeo = new THREE.CylinderGeometry(0.072, 0.058, 0.26, 6);
  const gauntletGeo = new THREE.BoxGeometry(0.09, 0.09, 0.095);

  // ---- Left Arm (+X) & Heraldic Kite Shield ----
  const leftArmPivot = new THREE.Group();
  leftArmPivot.rotation.order = 'YXZ';
  leftArmPivot.position.set(0.32, 0.26, 0);
  torsoGroup.add(leftArmPivot);

  // Sculpted layered pauldron
  const leftPauldron = asMajor(new THREE.Mesh(pauldronGeo, steelMat));
  leftPauldron.scale.set(1.15, 0.85, 1.05);
  leftPauldron.rotation.z = -0.35;
  leftPauldron.position.set(0.03, 0.05, 0);
  leftArmPivot.add(leftPauldron);

  const leftPauldronRim = new THREE.Mesh(pauldronRimGeo, goldMat);
  leftPauldronRim.rotation.set(Math.PI * 0.5, 0.3, -0.4);
  leftPauldronRim.position.set(0.04, 0.01, 0);
  leftArmPivot.add(leftPauldronRim);

  const leftUpperArm = asMajor(new THREE.Mesh(upperArmGeo, navyMat));
  leftUpperArm.position.set(0, -0.14, 0);
  leftArmPivot.add(leftUpperArm);

  const leftForearmPivot = new THREE.Group();
  leftForearmPivot.position.set(0, -0.28, 0);
  leftArmPivot.add(leftForearmPivot);

  const leftVambrace = asMajor(new THREE.Mesh(vambraceGeo, steelMat));
  leftVambrace.position.set(0, -0.12, 0);
  leftForearmPivot.add(leftVambrace);

  const leftGauntlet = new THREE.Mesh(gauntletGeo, goldMat);
  leftGauntlet.position.set(0, -0.26, 0);
  leftForearmPivot.add(leftGauntlet);

  // Heraldic Kite Shield on left arm
  const shieldGroup = new THREE.Group();
  shieldGroup.position.set(0.12, -0.14, 0.08);
  shieldGroup.rotation.set(0.12, Math.PI * 0.46, 0.06);
  leftForearmPivot.add(shieldGroup);

  const shieldBody = asMajor(new THREE.Mesh(createKiteShieldGeometry(1.0, 0.045), navyMat));
  shieldGroup.add(shieldBody);

  const shieldRim = new THREE.Mesh(createKiteShieldGeometry(1.08, 0.03), goldMat);
  shieldRim.position.z = -0.012;
  shieldGroup.add(shieldRim);

  // Mind / Aethelgard glowing rune emblem on shield face (+Z of shieldGroup)
  const shieldRuneGroup = new THREE.Group();
  shieldRuneGroup.position.set(0, 0.04, 0.042);
  shieldGroup.add(shieldRuneGroup);

  const runeDiamond = new THREE.Mesh(new THREE.OctahedronGeometry(0.072, 0), runeCyanMat);
  runeDiamond.scale.set(0.85, 1.25, 0.22);
  shieldRuneGroup.add(runeDiamond);

  // Stylized 'M' / Aethelgard twin-wing bars
  const mBarGeo = new THREE.BoxGeometry(0.026, 0.24, 0.018);
  const mDiagGeo = new THREE.BoxGeometry(0.024, 0.14, 0.018);
  const mBarL = new THREE.Mesh(mBarGeo, runeGoldMat);
  mBarL.position.set(-0.11, -0.02, 0);
  mBarL.rotation.z = 0.08;
  shieldRuneGroup.add(mBarL);
  const mBarR = new THREE.Mesh(mBarGeo, runeGoldMat);
  mBarR.position.set(0.11, -0.02, 0);
  mBarR.rotation.z = -0.08;
  shieldRuneGroup.add(mBarR);
  const mDiagL = new THREE.Mesh(mDiagGeo, runeGoldMat);
  mDiagL.position.set(-0.052, 0.045, 0);
  mDiagL.rotation.z = -0.62;
  shieldRuneGroup.add(mDiagL);
  const mDiagR = new THREE.Mesh(mDiagGeo, runeGoldMat);
  mDiagR.position.set(0.052, 0.045, 0);
  mDiagR.rotation.z = 0.62;
  shieldRuneGroup.add(mDiagR);

  // ---- Right Arm (-X) & Sculpted Rune-Edged Greatsword ----
  const rightArmPivot = new THREE.Group();
  rightArmPivot.rotation.order = 'YXZ';
  rightArmPivot.position.set(-0.32, 0.26, 0);
  torsoGroup.add(rightArmPivot);

  const rightPauldron = asMajor(new THREE.Mesh(pauldronGeo, steelMat));
  rightPauldron.scale.set(1.15, 0.85, 1.05);
  rightPauldron.rotation.z = 0.35;
  rightPauldron.position.set(-0.03, 0.05, 0);
  rightArmPivot.add(rightPauldron);

  const rightPauldronRim = new THREE.Mesh(pauldronRimGeo, goldMat);
  rightPauldronRim.rotation.set(Math.PI * 0.5, -0.3, 0.4);
  rightPauldronRim.position.set(-0.04, 0.01, 0);
  rightArmPivot.add(rightPauldronRim);

  const rightUpperArm = asMajor(new THREE.Mesh(upperArmGeo, navyMat));
  rightUpperArm.position.set(0, -0.14, 0);
  rightArmPivot.add(rightUpperArm);

  const rightForearmPivot = new THREE.Group();
  rightForearmPivot.position.set(0, -0.28, 0);
  rightArmPivot.add(rightForearmPivot);

  const rightVambrace = asMajor(new THREE.Mesh(vambraceGeo, steelMat));
  rightVambrace.position.set(0, -0.12, 0);
  rightForearmPivot.add(rightVambrace);

  const rightGauntlet = new THREE.Mesh(gauntletGeo, goldMat);
  rightGauntlet.position.set(0, -0.26, 0);
  rightForearmPivot.add(rightGauntlet);

  // Sculpted Rune-Edged Greatsword
  const swordGroup = new THREE.Group();
  swordGroup.position.set(0, -0.27, 0.04);
  swordGroup.rotation.set(Math.PI * 0.46, 0, 0); // Blade extends forward along hand
  rightForearmPivot.add(swordGroup);

  const swordGrip = new THREE.Mesh(new THREE.CylinderGeometry(0.022, 0.025, 0.28, 6), leatherMat);
  swordGrip.position.set(0, -0.08, 0);
  swordGroup.add(swordGrip);

  const swordPommel = new THREE.Mesh(new THREE.OctahedronGeometry(0.045, 0), goldMat);
  swordPommel.position.set(0, -0.23, 0);
  swordGroup.add(swordPommel);

  const crossguard = new THREE.Mesh(new THREE.BoxGeometry(0.32, 0.042, 0.056), goldMat);
  crossguard.position.set(0, 0.065, 0);
  swordGroup.add(crossguard);

  const guardWingGeo = new THREE.ConeGeometry(0.028, 0.11, 5);
  const guardWingL = new THREE.Mesh(guardWingGeo, goldMat);
  guardWingL.rotation.z = -1.2;
  guardWingL.position.set(0.17, 0.09, 0);
  swordGroup.add(guardWingL);
  const guardWingR = new THREE.Mesh(guardWingGeo, goldMat);
  guardWingR.rotation.z = 1.2;
  guardWingR.position.set(-0.17, 0.09, 0);
  swordGroup.add(guardWingR);

  const guardGem = new THREE.Mesh(new THREE.OctahedronGeometry(0.032, 0), runeCyanMat);
  guardGem.position.set(0, 0.065, 0.028);
  swordGroup.add(guardGem);

  const swordBlade = asMajor(new THREE.Mesh(createGreatswordBladeGeometry(), brightSteelMat));
  swordBlade.position.set(0, 0.08, 0);
  swordGroup.add(swordBlade);

  // Glowing golden-cyan rune fuller & edge inlay
  const swordEdgeMat = new THREE.MeshStandardMaterial({
    color: PALETTE.runeGold,
    emissive: PALETTE.runeCyan,
    emissiveIntensity: 1.15,
    roughness: 0.18,
    metalness: 0.5,
  });
  const swordFuller = new THREE.Mesh(new THREE.BoxGeometry(0.022, 0.78, 0.042), swordEdgeMat);
  swordFuller.position.set(0, 0.50, 0);
  swordGroup.add(swordFuller);

  const swordEdgeGeo = new THREE.BoxGeometry(0.011, 0.84, 0.016);
  const swordEdgeL = new THREE.Mesh(swordEdgeGeo, runeGoldMat);
  swordEdgeL.position.set(0.052, 0.52, 0);
  swordEdgeL.rotation.z = 0.015;
  swordGroup.add(swordEdgeL);

  const swordEdgeR = new THREE.Mesh(swordEdgeGeo, runeGoldMat);
  swordEdgeR.position.set(-0.052, 0.52, 0);
  swordEdgeR.rotation.z = -0.015;
  swordGroup.add(swordEdgeR);

  // ---- Articulated Legs ----
  const thighGeo = new THREE.CylinderGeometry(0.09, 0.076, 0.38, 7);
  const cuisseGeo = new THREE.BoxGeometry(0.11, 0.24, 0.12);
  const poleynGeo = new THREE.SphereGeometry(0.072, 6, 5);
  const shinGeo = new THREE.CylinderGeometry(0.074, 0.062, 0.34, 7);
  const bootGeo = new THREE.BoxGeometry(0.125, 0.11, 0.23);
  const bootToeGeo = new THREE.BoxGeometry(0.115, 0.065, 0.10);

  function buildLeg(side) {
    const hipPivot = new THREE.Group();
    hipPivot.position.set(side * 0.13, -0.14, 0);
    bodyPivot.add(hipPivot);

    const thigh = asMajor(new THREE.Mesh(thighGeo, navyMat));
    thigh.position.set(0, -0.19, 0);
    hipPivot.add(thigh);

    const cuisse = new THREE.Mesh(cuisseGeo, steelMat);
    cuisse.position.set(side * 0.02, -0.18, 0.03);
    hipPivot.add(cuisse);

    const kneePivot = new THREE.Group();
    kneePivot.position.set(0, -0.38, 0);
    hipPivot.add(kneePivot);

    const poleyn = new THREE.Mesh(poleynGeo, goldMat);
    poleyn.scale.set(1, 0.9, 1.1);
    poleyn.position.set(0, 0, 0.045);
    kneePivot.add(poleyn);

    const shin = asMajor(new THREE.Mesh(shinGeo, steelMat));
    shin.position.set(0, -0.18, 0);
    kneePivot.add(shin);

    const boot = asMajor(new THREE.Mesh(bootGeo, leatherMat));
    boot.position.set(0, -0.385, 0.045);
    kneePivot.add(boot);

    const bootToe = new THREE.Mesh(bootToeGeo, steelMat);
    bootToe.position.set(0, -0.405, 0.13);
    kneePivot.add(bootToe);

    return { hipPivot, kneePivot };
  }

  const leftLeg = buildLeg(1);
  const rightLeg = buildLeg(-1);

  applyShadows(group);

  // ---- Additive Curved Sword-Slash Arc ----
  const slashArcMat = new THREE.MeshBasicMaterial({
    vertexColors: true,
    transparent: true,
    opacity: 0,
    blending: THREE.AdditiveBlending,
    depthWrite: false,
    side: THREE.DoubleSide,
  });
  const slashArc = new THREE.Mesh(createSlashArcGeometry(), slashArcMat);
  slashArc.position.set(0, 1.16, 0.22);
  slashArc.visible = false;
  slashArc.renderOrder = 5;
  group.add(slashArc);

  scene.add(group);

  // Preallocated attack arc payload (zero per-frame allocations)
  const attackArcData = {
    active: false,
    origin: new THREE.Vector3(),
    forward: new THREE.Vector3(),
    range: 3.6,
    halfAngle: 1.15,
    damage: 24,
  };

  let walkPhase = 0;
  let lastElapsed = 0;
  let outOfCombatTimer = 5;

  const hero = {
    group,
    position: group.position,
    velocity: new THREE.Vector3(),
    yaw: Math.PI,
    onGround: true,
    hp: 100,
    maxHp: 100,
    stamina: 100,
    maxStamina: 100,
    gold: 0,
    damageBonus: 1.0,
    dead: false,
    invulnTimer: 0,
    hitFlash: 0,
    attackTimer: 0,
    attackDuration: 0.42,
    attackCombo: 0,
    attackHitApplied: false,
    blocking: false,
    blockStartTime: 0,
    rollTimer: 0,
    rollDuration: 0.45,
    rollDir: new THREE.Vector3(0, 0, -1),
    slainCount: 0,
    slashArc,

    startAttack(sound) {
      if (this.dead || this.rollTimer > 0 || this.attackTimer > 0.08 || this.stamina < 14) {
        return false;
      }
      this.stamina -= 14;
      this.attackCombo = (this.attackCombo + 1) % 3;
      this.attackTimer = this.attackDuration;
      this.attackHitApplied = false;
      this.blocking = false;
      outOfCombatTimer = 0;
      if (sound && sound.playSwordSwing) sound.playSwordSwing(this.attackCombo);
      return true;
    },

    startRoll(input, camYaw, sound) {
      if (this.dead || this.rollTimer > 0 || this.stamina < 20) {
        return false;
      }
      this.stamina -= 20;
      const fwd = input ? input.forward || 0 : 0;
      const right = input ? input.right || 0 : 0;
      const fwdX = Math.sin(camYaw);
      const fwdZ = Math.cos(camYaw);
      const rightX = -Math.cos(camYaw);
      const rightZ = Math.sin(camYaw);
      const dx = fwdX * fwd + rightX * right;
      const dz = fwdZ * fwd + rightZ * right;
      const len = Math.hypot(dx, dz);
      if (len > 0.01) {
        this.rollDir.set(dx / len, 0, dz / len);
        this.yaw = Math.atan2(this.rollDir.x, this.rollDir.z);
      } else {
        this.rollDir.set(Math.sin(this.yaw), 0, Math.cos(this.yaw));
      }
      this.rollTimer = this.rollDuration;
      this.invulnTimer = 0.44;
      this.attackTimer = 0;
      this.blocking = false;
      if (sound && sound.playRoll) sound.playRoll();
      return true;
    },

    takeDamage(amount, attackerPos, sound) {
      if (this.dead || this.invulnTimer > 0) {
        return { actualDamage: 0, blocked: false, parried: false, died: false };
      }

      outOfCombatTimer = 0;

      if (this.blocking && this.stamina > 5) {
        const blockAge = lastElapsed - this.blockStartTime;
        if (blockAge >= 0 && blockAge <= 0.25) {
          this.invulnTimer = 0.24;
          if (sound && sound.playBlock) sound.playBlock();
          return { actualDamage: 0, blocked: true, parried: true, died: false };
        }
        const reduced = Math.max(1, Math.round(amount * 0.2));
        this.stamina = Math.max(0, this.stamina - 14);
        this.hp -= reduced;
        this.invulnTimer = 0.28;
        if (attackerPos) {
          const kx = this.position.x - attackerPos.x;
          const kz = this.position.z - attackerPos.z;
          const kl = Math.hypot(kx, kz) || 1;
          this.velocity.x += (kx / kl) * 4.0;
          this.velocity.z += (kz / kl) * 4.0;
        }
        if (sound && sound.playBlock) sound.playBlock();
        let died = false;
        if (this.hp <= 0) {
          this.hp = 0;
          this.dead = true;
          died = true;
        }
        return { actualDamage: reduced, blocked: true, parried: false, died };
      }

      const actualDamage = Math.max(1, Math.round(amount));
      this.hp -= actualDamage;
      this.hitFlash = 0.25;
      this.invulnTimer = 0.35;
      if (attackerPos) {
        const kx = this.position.x - attackerPos.x;
        const kz = this.position.z - attackerPos.z;
        const kl = Math.hypot(kx, kz) || 1;
        this.velocity.x = (kx / kl) * 8.5;
        this.velocity.z = (kz / kl) * 8.5;
        this.velocity.y = Math.max(this.velocity.y, 4.4);
      }
      if (sound && sound.playPlayerHurt) sound.playPlayerHurt();
      let died = false;
      if (this.hp <= 0) {
        this.hp = 0;
        this.dead = true;
        died = true;
      }
      return { actualDamage, blocked: false, parried: false, died };
    },

    respawn() {
      this.dead = false;
      this.hp = this.maxHp;
      this.stamina = this.maxStamina;
      this.invulnTimer = 1.0;
      this.hitFlash = 0;
      this.attackTimer = 0;
      this.attackHitApplied = false;
      this.rollTimer = 0;
      this.blocking = false;
      this.velocity.set(0, 0, 0);
      const sx = LANDMARKS.spawn.x;
      const sz = LANDMARKS.spawn.z;
      this.position.set(sx, getWalkableHeight(sx, sz, 10), sz);
      this.yaw = Math.PI;
      this.group.rotation.set(0, this.yaw, 0);
      this.onGround = true;
      bodyPivot.position.set(0, 0.96, 0);
      bodyPivot.rotation.set(0, 0, 0);
      torsoGroup.rotation.set(0, 0, 0);
      slashArc.visible = false;
      slashArcMat.opacity = 0;
    },

    getAttackArc() {
      attackArcData.active = !this.dead && this.attackTimer > 0.14 && this.attackTimer < 0.34;
      attackArcData.origin.set(this.position.x, this.position.y + 1.15, this.position.z);
      attackArcData.forward.set(Math.sin(this.yaw), 0, Math.cos(this.yaw));
      attackArcData.range = 3.6;
      attackArcData.halfAngle = 1.15;
      attackArcData.damage = Math.round((24 + this.attackCombo * 6) * this.damageBonus);
      return attackArcData;
    },

    update(dt, elapsed, input, camYaw, mounted, sound) {
      const step = Math.min(dt, 0.05);
      lastElapsed = elapsed;

      if (this.invulnTimer > 0) this.invulnTimer = Math.max(0, this.invulnTimer - step);
      if (this.hitFlash > 0) this.hitFlash = Math.max(0, this.hitFlash - step);
      if (this.attackTimer > 0) this.attackTimer = Math.max(0, this.attackTimer - step);
      if (this.rollTimer > 0) this.rollTimer = Math.max(0, this.rollTimer - step);

      if (this.attackTimer > 0 || this.hitFlash > 0 || this.blocking) {
        outOfCombatTimer = 0;
      } else {
        outOfCombatTimer += step;
      }

      // Hit flash visual feedback on armor materials
      const flashStrength = this.hitFlash > 0 ? (this.hitFlash / 0.25) : 0;
      for (let i = 0; i < flashMats.length; i++) {
        flashMats[i].emissive.copy(flashStrength > 0 ? _cFlash : _cZero);
        flashMats[i].emissiveIntensity = flashStrength * 1.4;
      }

      // Stamina & gentle out-of-combat HP regeneration
      if (!this.dead) {
        if (mounted) {
          this.stamina = Math.min(this.maxStamina, this.stamina + 30 * step);
          if (this.hp < this.maxHp) this.hp = Math.min(this.maxHp, this.hp + 1.0 * step);
        } else {
          const stamRate = this.blocking ? 6.0 : (this.rollTimer > 0 || this.attackTimer > 0 ? 4.0 : 25.0);
          this.stamina = Math.min(this.maxStamina, this.stamina + stamRate * step);
          if (outOfCombatTimer > 4.0 && this.hp < this.maxHp) {
            this.hp = Math.min(this.maxHp, this.hp + 1.0 * step);
          }
        }
      }

      // ---- Mounted Dragon-Rider Pose ----
      if (mounted) {
        this.blocking = false;
        this.onGround = false;
        slashArc.visible = false;
        slashArcMat.opacity = 0;

        bodyPivot.position.set(0, 0.72, 0);
        bodyPivot.rotation.set(0.22, 0, 0);
        torsoGroup.position.set(0, 0.06, 0);
        torsoGroup.rotation.set(0.12, 0, 0);
        headGroup.rotation.set(-0.18, 0, 0);

        leftLeg.hipPivot.rotation.set(-0.68, 0, 0.36);
        leftLeg.kneePivot.rotation.set(1.02, 0, 0);
        rightLeg.hipPivot.rotation.set(-0.68, 0, -0.36);
        rightLeg.kneePivot.rotation.set(1.02, 0, 0);

        // Left arm gripping saddle reins, right arm holding greatsword back
        leftArmPivot.rotation.set(-0.72, 0, 0.18);
        leftForearmPivot.rotation.set(-0.55, 0, 0);
        rightArmPivot.rotation.set(0.35, 0, -0.25);
        rightForearmPivot.rotation.set(-0.35, 0, 0);

        // Cape streaming dramatically in flight wind
        capeUpperPivot.rotation.x = 0.92 + Math.sin(elapsed * 14.0) * 0.14;
        capeLowerPivot.rotation.x = 0.38 + Math.sin(elapsed * 16.5 - 0.9) * 0.20;
        plumePivot.rotation.x = 0.35 + Math.sin(elapsed * 15.0) * 0.12;
        return;
      }

      // ---- Fallen / Dead State ----
      if (this.dead) {
        this.blocking = false;
        slashArc.visible = false;
        slashArcMat.opacity = 0;
        this.velocity.x *= Math.max(0, 1 - step * 8);
        this.velocity.z *= Math.max(0, 1 - step * 8);
        this.velocity.y -= WORLD.GRAVITY * step;
        this.position.addScaledVector(this.velocity, step);
        const groundY = Math.max(-1.1, getWalkableHeight(this.position.x, this.position.z, this.position.y));
        if (this.position.y <= groundY) {
          this.position.y = groundY;
          this.velocity.y = 0;
          this.onGround = true;
        }
        bodyPivot.rotation.x = THREE.MathUtils.lerp(bodyPivot.rotation.x, -Math.PI * 0.48, Math.min(1, step * 7));
        bodyPivot.position.y = THREE.MathUtils.lerp(bodyPivot.position.y, 0.22, Math.min(1, step * 7));
        return;
      }

      // ---- Blocking State ----
      const wantsBlock = !!(input && input.blockHeld) && this.rollTimer <= 0 && this.attackTimer <= 0 && this.stamina > 3;
      if (wantsBlock && !this.blocking) {
        this.blockStartTime = elapsed;
      }
      this.blocking = wantsBlock;

      // ---- Movement & Physics ----
      const inWater = this.position.y < WORLD.WATER_LEVEL - 0.18;
      const wadeFactor = inWater ? 0.62 : 1.0;

      const fwd = input ? input.forward || 0 : 0;
      const right = input ? input.right || 0 : 0;
      let moveLen = 0;
      let mx = 0;
      let mz = 0;

      if (this.rollTimer > 0) {
        const rollProgress = 1 - this.rollTimer / this.rollDuration;
        const rollSpeed = (15.2 - rollProgress * 3.8) * wadeFactor;
        this.velocity.x = this.rollDir.x * rollSpeed;
        this.velocity.z = this.rollDir.z * rollSpeed;
      } else {
        const fwdX = Math.sin(camYaw);
        const fwdZ = Math.cos(camYaw);
        const rightX = -Math.cos(camYaw);
        const rightZ = Math.sin(camYaw);
        mx = fwdX * fwd + rightX * right;
        mz = fwdZ * fwd + rightZ * right;
        moveLen = Math.hypot(mx, mz);

        if (moveLen > 0.01) {
          mx /= moveLen;
          mz /= moveLen;
          const baseSpeed = this.blocking ? 4.2 : (this.attackTimer > 0.1 ? 3.4 : 9.4);
          const targetSpeed = baseSpeed * wadeFactor;
          const accel = this.onGround ? Math.min(1, step * 16) : Math.min(1, step * 5.5);
          this.velocity.x = THREE.MathUtils.lerp(this.velocity.x, mx * targetSpeed, accel);
          this.velocity.z = THREE.MathUtils.lerp(this.velocity.z, mz * targetSpeed, accel);
        } else {
          const decel = this.onGround ? Math.min(1, step * 14) : Math.min(1, step * 2.5);
          this.velocity.x = THREE.MathUtils.lerp(this.velocity.x, 0, decel);
          this.velocity.z = THREE.MathUtils.lerp(this.velocity.z, 0, decel);
        }

        // Smooth facing yaw
        if (this.blocking || this.attackTimer > 0.12) {
          this.yaw = lerpAngle(this.yaw, camYaw, step * 16);
        } else if (moveLen > 0.01) {
          const desiredYaw = Math.atan2(mx, mz);
          this.yaw = lerpAngle(this.yaw, desiredYaw, step * 13);
        }
      }

      // Jump
      if (input && input.jump && this.onGround && this.rollTimer <= 0) {
        this.velocity.y = 11.2;
        this.onGround = false;
      }

      // Gravity & Integration
      this.velocity.y -= WORLD.GRAVITY * step;
      this.position.x += this.velocity.x * step;
      this.position.y += this.velocity.y * step;
      this.position.z += this.velocity.z * step;

      // Collisions & Walkable Terrain Floor
      resolveWorldCollisions(this.position, WORLD.PLAYER_RADIUS);

      const groundY = Math.max(-1.1, getWalkableHeight(this.position.x, this.position.z, this.position.y));
      if (this.position.y <= groundY) {
        this.position.y = groundY;
        if (this.velocity.y < 0) this.velocity.y = 0;
        this.onGround = true;
      } else if (this.position.y > groundY + 0.18) {
        this.onGround = false;
      }

      if (isInsideDungeon(this.position.x, this.position.z)) {
        const ceilLimit = LANDMARKS.dungeonCenter.ceilingY - WORLD.PLAYER_HEIGHT - 0.2;
        if (this.position.y > ceilLimit) {
          this.position.y = ceilLimit;
          if (this.velocity.y > 0) this.velocity.y = 0;
        }
      }

      this.group.rotation.set(0, this.yaw, 0);

      // ---- Procedural Articulated Character Animation ----
      const horizSpeed = Math.hypot(this.velocity.x, this.velocity.z);
      const speedNorm = Math.min(1, horizSpeed / 9.2);

      if (this.rollTimer > 0) {
        // Tuck somersault dodge roll
        const p = 1 - this.rollTimer / this.rollDuration;
        bodyPivot.rotation.set(p * Math.PI * 2, 0, 0);
        bodyPivot.position.set(0, 0.58 + Math.sin(p * Math.PI) * 0.26, 0);
        torsoGroup.rotation.set(0.35, 0, 0);
        headGroup.rotation.set(0.3, 0, 0);
        leftLeg.hipPivot.rotation.set(-1.1, 0, 0);
        leftLeg.kneePivot.rotation.set(1.65, 0, 0);
        rightLeg.hipPivot.rotation.set(-1.1, 0, 0);
        rightLeg.kneePivot.rotation.set(1.65, 0, 0);
        leftArmPivot.rotation.set(-1.1, 0, 0.35);
        leftForearmPivot.rotation.set(-0.9, 0, 0);
        rightArmPivot.rotation.set(-0.85, 0, -0.35);
        rightForearmPivot.rotation.set(-0.8, 0, 0);
        capeUpperPivot.rotation.x = 0.7;
        capeLowerPivot.rotation.x = 0.4;
        slashArc.visible = false;
        slashArcMat.opacity = 0;
        return;
      }

      // Reset roll tilt smoothly
      bodyPivot.rotation.x = THREE.MathUtils.lerp(bodyPivot.rotation.x, 0, Math.min(1, step * 18));
      bodyPivot.rotation.z = 0;

      // Leg stride & locomotion bob
      if (this.onGround && speedNorm > 0.06) {
        walkPhase += step * (4.5 + horizSpeed * 1.15);
        const stride = 0.72 * speedNorm;
        const s = Math.sin(walkPhase);
        const c = Math.cos(walkPhase);
        leftLeg.hipPivot.rotation.set(s * stride, 0, 0);
        rightLeg.hipPivot.rotation.set(-s * stride, 0, 0);
        leftLeg.kneePivot.rotation.set(Math.max(0, -c) * 0.78 * speedNorm, 0, 0);
        rightLeg.kneePivot.rotation.set(Math.max(0, c) * 0.78 * speedNorm, 0, 0);

        const baseH = this.blocking ? 0.90 : 0.96;
        bodyPivot.position.y = baseH + Math.abs(c) * 0.065 * speedNorm;
        torsoGroup.rotation.x = 0.14 * speedNorm + (this.blocking ? 0.08 : 0);
        torsoGroup.rotation.y = s * 0.12 * speedNorm;
      } else if (!this.onGround) {
        // Mid-air athletic jump pose
        bodyPivot.position.y = 0.96;
        torsoGroup.rotation.x = 0.12;
        torsoGroup.rotation.y = 0;
        leftLeg.hipPivot.rotation.set(-0.55, 0, 0.06);
        leftLeg.kneePivot.rotation.set(0.95, 0, 0);
        rightLeg.hipPivot.rotation.set(0.28, 0, -0.06);
        rightLeg.kneePivot.rotation.set(0.45, 0, 0);
      } else {
        // Heroic idle breathing stance
        const breath = Math.sin(elapsed * 2.5);
        const baseH = this.blocking ? 0.90 : 0.96;
        bodyPivot.position.y = THREE.MathUtils.lerp(bodyPivot.position.y, baseH + breath * 0.014, Math.min(1, step * 10));
        torsoGroup.rotation.x = THREE.MathUtils.lerp(torsoGroup.rotation.x, this.blocking ? 0.12 : 0.03, Math.min(1, step * 10));
        torsoGroup.rotation.y = THREE.MathUtils.lerp(torsoGroup.rotation.y, this.blocking ? -0.28 : -0.10, Math.min(1, step * 10));
        leftLeg.hipPivot.rotation.set(-0.08, 0, 0.05);
        leftLeg.kneePivot.rotation.set(this.blocking ? 0.24 : 0.08, 0, 0);
        rightLeg.hipPivot.rotation.set(0.08, 0, -0.05);
        rightLeg.kneePivot.rotation.set(this.blocking ? 0.24 : 0.08, 0, 0);
      }

      headGroup.rotation.set(-torsoGroup.rotation.x * 0.5, -torsoGroup.rotation.y * 0.65, 0);

      // Shield & Left Arm animation
      if (this.blocking) {
        leftArmPivot.rotation.set(-0.88, -0.52, 0.20);
        leftForearmPivot.rotation.set(-0.42, 0, 0);
        shieldGroup.rotation.set(1.28, 0.34, 0.0);
        // Parry window bright rune flare
        const inParry = (elapsed - this.blockStartTime) <= 0.25;
        runeCyanMat.emissiveIntensity = inParry ? 3.4 : 1.9;
      } else {
        const armSwing = this.onGround ? -Math.sin(walkPhase) * 0.52 * speedNorm : -0.4;
        leftArmPivot.rotation.set(armSwing - 0.15, 0, 0.14);
        leftForearmPivot.rotation.set(-0.38, 0, 0);
        shieldGroup.rotation.set(0.12, Math.PI * 0.46, 0.06);
        runeCyanMat.emissiveIntensity = 1.45;
      }

      // Sword Swing, Right Arm & Slash Arc VFX
      if (this.attackTimer > 0) {
        const p = 1 - this.attackTimer / this.attackDuration; // 0 -> 1
        // Smooth S-curve swing velocity: quick windup (0..0.16), explosive cleave (0.16..0.72), settle
        const swingT = THREE.MathUtils.smoothstep(p, 0.16, 0.72);

        if (this.attackCombo === 0) {
          // Right-to-left horizontal cleave
          torsoGroup.rotation.y = THREE.MathUtils.lerp(-0.68, 0.78, swingT);
          rightArmPivot.rotation.set(-1.32, THREE.MathUtils.lerp(-1.12, 1.15, swingT), -0.22);
          rightForearmPivot.rotation.set(-0.16, 0, 0);
          swordGroup.rotation.set(Math.PI * 0.90, 0, -0.18);

          slashArc.rotation.set(0.14, THREE.MathUtils.lerp(-0.75, 0.95, swingT), 0.14);
          slashArc.scale.set(1.0, 1.0, 1.0);
        } else if (this.attackCombo === 1) {
          // Left-to-right upward diagonal backhand
          torsoGroup.rotation.y = THREE.MathUtils.lerp(0.72, -0.72, swingT);
          rightArmPivot.rotation.set(THREE.MathUtils.lerp(-0.82, -1.62, swingT), THREE.MathUtils.lerp(1.05, -1.08, swingT), -0.18);
          rightForearmPivot.rotation.set(-0.20, 0, 0);
          swordGroup.rotation.set(Math.PI * 0.88, 0, 0.28);

          slashArc.rotation.set(-0.20, THREE.MathUtils.lerp(0.75, -0.92, swingT), -0.32);
          slashArc.scale.set(-1.06, 1.0, 1.06);
        } else {
          // Heavy finisher sweeping cleave
          torsoGroup.rotation.y = THREE.MathUtils.lerp(-0.82, 0.92, swingT);
          torsoGroup.rotation.x += Math.sin(swingT * Math.PI) * 0.24;
          rightArmPivot.rotation.set(THREE.MathUtils.lerp(-2.12, -0.92, swingT), THREE.MathUtils.lerp(-0.75, 1.22, swingT), -0.24);
          rightForearmPivot.rotation.set(-0.14, 0, 0);
          swordGroup.rotation.set(Math.PI * 0.86, 0, -0.15);

          slashArc.rotation.set(0.10, THREE.MathUtils.lerp(-0.85, 1.05, swingT), 0.42);
          slashArc.scale.set(1.22, 1.0, 1.22);
        }

        // Flash additive slash arc during active cutting frames
        if (p > 0.18 && p < 0.80) {
          const arcEnv = Math.sin(((p - 0.18) / 0.62) * Math.PI);
          slashArc.visible = true;
          slashArcMat.opacity = arcEnv * (this.attackCombo === 2 ? 0.95 : 0.82);
          swordEdgeMat.emissiveIntensity = 1.2 + arcEnv * 2.4;
        } else {
          slashArc.visible = false;
          slashArcMat.opacity = 0;
          swordEdgeMat.emissiveIntensity = 1.15;
        }
      } else {
        slashArc.visible = false;
        slashArcMat.opacity = 0;
        swordEdgeMat.emissiveIntensity = 1.15;

        const rightSwing = this.onGround ? Math.sin(walkPhase) * 0.48 * speedNorm : 0.25;
        rightArmPivot.rotation.set(this.blocking ? -0.45 : (rightSwing + 0.10), 0, -0.16);
        rightForearmPivot.rotation.set(this.blocking ? -0.65 : -0.32, 0, 0);
        swordGroup.rotation.set(Math.PI * 0.44, 0, 0);
      }

      // Flowing 2-segment cape & helmet plume physics
      const windWave = Math.sin(elapsed * (3.6 + speedNorm * 8.0));
      const windWaveLag = Math.sin(elapsed * (3.6 + speedNorm * 8.0) - 0.85);
      const airLift = this.onGround ? 0 : Math.min(0.45, Math.max(-0.2, -this.velocity.y * 0.035));
      capeUpperPivot.rotation.x = 0.16 + speedNorm * 0.56 + airLift + windWave * (0.05 + speedNorm * 0.09);
      capeLowerPivot.rotation.x = 0.08 + speedNorm * 0.28 + windWaveLag * (0.06 + speedNorm * 0.13);
      plumePivot.rotation.x = speedNorm * 0.30 + windWave * 0.08;
    },
  };

  return hero;
}

// ============================================================================
// 2. CREATE DRAGON — Rideable Procedural Red Dragon (Ignis)
// ============================================================================

export function createDragon(scene, textures) {
  const group = new THREE.Group();
  group.rotation.order = 'YXZ';

  const perchX = LANDMARKS.dragonPerch.x;
  const perchZ = LANDMARKS.dragonPerch.z;
  const perchY = getTerrainHeight(perchX, perchZ);
  group.position.set(perchX, perchY, perchZ);
  group.rotation.y = 0.6;

  // Materials
  const scaleMat = new THREE.MeshStandardMaterial({
    map: textures && textures.dragonScales ? textures.dragonScales : null,
    normalMap: textures && textures.dragonScaleNormal ? textures.dragonScaleNormal : null,
    normalScale: new THREE.Vector2(1.15, 1.15),
    color: 0xd92e24,
    roughness: 0.36,
    metalness: 0.55,
    envMapIntensity: 1.0,
  });
  const darkScaleMat = new THREE.MeshStandardMaterial({
    map: textures && textures.dragonScales ? textures.dragonScales : null,
    normalMap: textures && textures.dragonScaleNormal ? textures.dragonScaleNormal : null,
    normalScale: new THREE.Vector2(1.15, 1.15),
    color: 0x7d1412,
    roughness: 0.42,
    metalness: 0.6,
    envMapIntensity: 1.0,
  });
  const bellyMat = new THREE.MeshStandardMaterial({
    color: 0xc99242,
    roughness: 0.40,
    metalness: 0.48,
  });
  const membraneMat = new THREE.MeshStandardMaterial({
    color: 0x941b16,
    emissive: 0x260502,
    roughness: 0.62,
    metalness: 0.08,
    side: THREE.DoubleSide,
  });
  const obsidianMat = new THREE.MeshStandardMaterial({
    color: PALETTE.obsidian,
    roughness: 0.28,
    metalness: 0.68,
  });
  const eyeMat = new THREE.MeshStandardMaterial({
    color: 0xffd040,
    emissive: 0xff8408,
    emissiveIntensity: 2.3,
    roughness: 0.15,
  });
  const toothMat = new THREE.MeshStandardMaterial({
    color: 0xf2ead6,
    roughness: 0.35,
    metalness: 0.1,
  });
  const saddleLeatherMat = new THREE.MeshStandardMaterial({
    color: 0x3b2214,
    roughness: 0.76,
    metalness: 0.12,
  });
  const saddleGoldMat = new THREE.MeshStandardMaterial({
    color: 0xdba948,
    roughness: 0.26,
    metalness: 0.84,
  });
  const throatGlowMat = new THREE.MeshBasicMaterial({
    color: 0xffaa22,
    transparent: true,
    opacity: 0,
  });

  // Main body container elevated above perched feet
  const bodyGroup = new THREE.Group();
  bodyGroup.position.set(0, 1.98, 0);
  group.add(bodyGroup);

  // ---- Sculpted Muscular Crimson Body ----
  const chest = asMajor(new THREE.Mesh(new THREE.SphereGeometry(1.28, 10, 8), scaleMat));
  chest.scale.set(1.06, 0.92, 1.48);
  chest.position.set(0, 0.18, 0.78);
  bodyGroup.add(chest);

  const midTorso = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(1.12, 0.76, 2.35, 10), scaleMat));
  midTorso.rotation.x = Math.PI * 0.5;
  midTorso.scale.set(1.02, 1.0, 0.86);
  midTorso.position.set(0, 0.02, -0.68);
  bodyGroup.add(midTorso);

  const pelvis = asMajor(new THREE.Mesh(new THREE.SphereGeometry(0.84, 8, 7), darkScaleMat));
  pelvis.scale.set(1.06, 0.86, 1.16);
  pelvis.position.set(0, -0.04, -1.78);
  bodyGroup.add(pelvis);

  // Armored bronze-gold underbelly plates
  for (let i = 0; i < 7; i++) {
    const z = 1.68 - i * 0.52;
    const width = 1.15 - Math.abs(i - 1.5) * 0.11;
    const plate = new THREE.Mesh(new THREE.BoxGeometry(width, 0.22, 0.56), bellyMat);
    plate.position.set(0, -0.78 + i * 0.035, z);
    plate.rotation.x = -0.14;
    bodyGroup.add(plate);
  }

  // Dorsal obsidian spine ridge
  for (let i = 0; i < 8; i++) {
    const z = 1.75 - i * 0.52;
    const h = 0.42 + (i < 3 ? i * 0.06 : (7 - i) * 0.04);
    const spine = new THREE.Mesh(new THREE.ConeGeometry(0.11, h, 5), obsidianMat);
    spine.position.set(0, 1.15 - i * 0.055, z);
    spine.rotation.x = -0.45;
    bodyGroup.add(spine);
  }

  // ---- Dragon-Rider Saddle & riderSeat Group ----
  const saddleGroup = new THREE.Group();
  saddleGroup.position.set(0, 1.08, 0.32);
  bodyGroup.add(saddleGroup);

  const saddleBlanket = new THREE.Mesh(new THREE.BoxGeometry(1.24, 0.12, 1.18), saddleLeatherMat);
  saddleGroup.add(saddleBlanket);

  const saddlePommel = new THREE.Mesh(new THREE.BoxGeometry(0.56, 0.30, 0.20), saddleGoldMat);
  saddlePommel.position.set(0, 0.18, 0.48);
  saddlePommel.rotation.x = 0.25;
  saddleGroup.add(saddlePommel);

  const saddleCantle = new THREE.Mesh(new THREE.BoxGeometry(0.66, 0.34, 0.20), saddleLeatherMat);
  saddleCantle.position.set(0, 0.20, -0.46);
  saddleCantle.rotation.x = -0.25;
  saddleGroup.add(saddleCantle);

  const saddleStrap = new THREE.Mesh(new THREE.TorusGeometry(0.95, 0.06, 5, 10), saddleGoldMat);
  saddleStrap.scale.set(1.12, 0.96, 1.0);
  saddleStrap.position.set(0, -0.88, 0);
  saddleGroup.add(saddleStrap);

  const riderSeat = new THREE.Group();
  riderSeat.position.set(0, 1.18, 0.34);
  bodyGroup.add(riderSeat);

  // ---- Curved Segmented Neck & Ferocious Horned Head ----
  const neckBase = new THREE.Group();
  neckBase.position.set(0, 0.55, 2.02);
  bodyGroup.add(neckBase);

  const neckSeg1 = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.56, 0.74, 1.25, 8), scaleMat));
  neckSeg1.rotation.x = 0.92;
  neckSeg1.position.set(0, 0.35, 0.46);
  neckBase.add(neckSeg1);

  const neckBelly1 = new THREE.Mesh(new THREE.BoxGeometry(0.62, 0.16, 1.05), bellyMat);
  neckBelly1.position.set(0, -0.02, 0.62);
  neckBelly1.rotation.x = -0.58;
  neckBase.add(neckBelly1);

  const neckSpine1 = new THREE.Mesh(new THREE.ConeGeometry(0.10, 0.42, 5), obsidianMat);
  neckSpine1.position.set(0, 0.82, 0.36);
  neckSpine1.rotation.x = -0.65;
  neckBase.add(neckSpine1);

  const neckMid = new THREE.Group();
  neckMid.position.set(0, 0.72, 0.98);
  neckBase.add(neckMid);

  const neckSeg2 = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.46, 0.56, 1.18, 8), scaleMat));
  neckSeg2.rotation.x = 1.24;
  neckSeg2.position.set(0, 0.22, 0.48);
  neckMid.add(neckSeg2);

  const neckSpine2 = new THREE.Mesh(new THREE.ConeGeometry(0.09, 0.38, 5), obsidianMat);
  neckSpine2.position.set(0, 0.64, 0.38);
  neckSpine2.rotation.x = -0.58;
  neckMid.add(neckSpine2);

  // Head Pivot
  const headPivot = new THREE.Group();
  headPivot.position.set(0, 0.42, 1.02);
  neckMid.add(headPivot);

  const skull = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.88, 0.62, 0.96), scaleMat));
  skull.position.set(0, 0.12, 0.26);
  headPivot.add(skull);

  // Tapered upper snout
  const snout = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.60, 0.36, 1.18), scaleMat));
  snout.position.set(0, 0.08, 1.05);
  headPivot.add(snout);

  const snoutTip = new THREE.Mesh(new THREE.ConeGeometry(0.32, 0.55, 4), darkScaleMat);
  snoutTip.rotation.x = Math.PI * 0.5;
  snoutTip.rotation.z = Math.PI * 0.25;
  snoutTip.position.set(0, 0.09, 1.72);
  headPivot.add(snoutTip);

  // Nasal obsidian horn
  const nasalHorn = new THREE.Mesh(new THREE.ConeGeometry(0.075, 0.34, 5), obsidianMat);
  nasalHorn.position.set(0, 0.34, 1.44);
  nasalHorn.rotation.x = -0.35;
  headPivot.add(nasalHorn);

  // Shared head detail geometries
  const browGeo = new THREE.BoxGeometry(0.24, 0.12, 0.52);
  const eyeGeo = new THREE.SphereGeometry(0.11, 6, 6);
  const hornBaseGeo = new THREE.ConeGeometry(0.14, 1.05, 6);
  const hornTipGeo = new THREE.ConeGeometry(0.085, 0.92, 6);
  const cheekHornGeo = new THREE.ConeGeometry(0.08, 0.56, 5);
  const upperFangGeo = new THREE.ConeGeometry(0.045, 0.22, 5);

  // Brow ridges & Glowing Fiery Amber Eyes
  for (let side = -1; side <= 1; side += 2) {
    const brow = new THREE.Mesh(browGeo, darkScaleMat);
    brow.position.set(side * 0.38, 0.36, 0.56);
    brow.rotation.z = -side * 0.24;
    brow.rotation.y = side * 0.14;
    headPivot.add(brow);

    const eye = new THREE.Mesh(eyeGeo, eyeMat);
    eye.scale.set(0.75, 0.65, 1.15);
    eye.position.set(side * 0.41, 0.23, 0.64);
    headPivot.add(eye);

    // Massive swept-back curved obsidian horns (2 segments each)
    const hornBase = asMajor(new THREE.Mesh(hornBaseGeo, obsidianMat));
    hornBase.position.set(side * 0.36, 0.56, -0.18);
    hornBase.rotation.set(-1.12, side * 0.22, -side * 0.28);
    headPivot.add(hornBase);

    const hornTip = new THREE.Mesh(hornTipGeo, obsidianMat);
    hornTip.position.set(side * 0.56, 0.90, -0.96);
    hornTip.rotation.set(-1.38, side * 0.14, -side * 0.18);
    headPivot.add(hornTip);

    // Lower cheek horn
    const cheekHorn = new THREE.Mesh(cheekHornGeo, obsidianMat);
    cheekHorn.position.set(side * 0.48, -0.02, -0.08);
    cheekHorn.rotation.set(-1.42, side * 0.36, 0);
    headPivot.add(cheekHorn);

    // Upper fangs
    const fang = new THREE.Mesh(upperFangGeo, toothMat);
    fang.position.set(side * 0.22, -0.14, 1.38);
    fang.rotation.x = Math.PI;
    headPivot.add(fang);
  }

  // Articulated Lower Jaw (opens when breathing fire!)
  const jawPivot = new THREE.Group();
  jawPivot.position.set(0, -0.14, 0.30);
  headPivot.add(jawPivot);

  const lowerJaw = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.54, 0.20, 1.32), bellyMat));
  lowerJaw.position.set(0, -0.06, 0.66);
  jawPivot.add(lowerJaw);

  const lowFangGeo = new THREE.ConeGeometry(0.038, 0.16, 5);
  for (let side = -1; side <= 1; side += 2) {
    const lowFang = new THREE.Mesh(lowFangGeo, toothMat);
    lowFang.position.set(side * 0.20, 0.08, 1.15);
    jawPivot.add(lowFang);
  }

  // Fiery gullet core inside mouth
  const throatGlow = new THREE.Mesh(new THREE.SphereGeometry(0.28, 6, 6), throatGlowMat);
  throatGlow.position.set(0, -0.04, 0.95);
  headPivot.add(throatGlow);

  // Mouth fire emitter anchor & blazing dynamic PointLight
  const mouthAnchor = new THREE.Object3D();
  mouthAnchor.position.set(0, -0.05, 1.65);
  headPivot.add(mouthAnchor);

  const mouthLight = new THREE.PointLight(0xff7a1a, 0, 40, 1.3);
  mouthAnchor.add(mouthLight);

  // ---- 4 Clawed Legs ----
  const frontUpperLegGeo = new THREE.CylinderGeometry(0.34, 0.25, 1.05, 7);
  const rearUpperLegGeo = new THREE.CylinderGeometry(0.42, 0.28, 1.05, 7);
  const lowerLegGeo = new THREE.CylinderGeometry(0.24, 0.18, 0.88, 7);
  const clawGeo = new THREE.ConeGeometry(0.068, 0.38, 5);

  function buildDragonLeg(side, isFront) {
    const hip = new THREE.Group();
    hip.position.set(side * (isFront ? 1.05 : 0.92), isFront ? -0.32 : -0.22, isFront ? 1.05 : -1.48);
    bodyGroup.add(hip);

    const upper = asMajor(new THREE.Mesh(
      isFront ? frontUpperLegGeo : rearUpperLegGeo,
      darkScaleMat
    ));
    upper.position.set(0, -0.45, 0);
    upper.rotation.x = isFront ? -0.18 : 0.28;
    hip.add(upper);

    const knee = new THREE.Group();
    knee.position.set(0, -0.92, isFront ? -0.12 : 0.18);
    hip.add(knee);

    const lower = asMajor(new THREE.Mesh(lowerLegGeo, scaleMat));
    lower.position.set(0, -0.38, 0.08);
    lower.rotation.x = 0.20;
    knee.add(lower);

    // 3 splayed obsidian claws
    for (let c = -1; c <= 1; c++) {
      const claw = new THREE.Mesh(clawGeo, obsidianMat);
      claw.position.set(c * 0.14, -0.78, 0.34);
      claw.rotation.set(1.25, c * 0.28, 0);
      knee.add(claw);
    }

    return { hip, knee };
  }

  const legs = [
    buildDragonLeg(1, true),
    buildDragonLeg(-1, true),
    buildDragonLeg(1, false),
    buildDragonLeg(-1, false),
  ];

  // ---- 5-Segment Sinuous Tapering Tail with Obsidian Spade Tip ----
  const tailSegs = [];
  let tailParent = bodyGroup;
  const tailRadii = [0.64, 0.48, 0.36, 0.25, 0.16, 0.08];
  const tailLengths = [1.22, 1.18, 1.14, 1.08, 1.02];

  for (let i = 0; i < 5; i++) {
    const segPivot = new THREE.Group();
    if (i === 0) {
      segPivot.position.set(0, -0.05, -2.25);
    } else {
      segPivot.position.set(0, 0, -tailLengths[i - 1]);
    }
    tailParent.add(segPivot);

    const len = tailLengths[i];
    const segMesh = asMajor(new THREE.Mesh(
      new THREE.CylinderGeometry(tailRadii[i + 1], tailRadii[i], len, 7),
      i % 2 === 0 ? scaleMat : darkScaleMat
    ));
    segMesh.rotation.x = -Math.PI * 0.5;
    segMesh.position.set(0, 0, -len * 0.5);
    segPivot.add(segMesh);

    const segSpine = new THREE.Mesh(
      new THREE.ConeGeometry(0.085 - i * 0.01, 0.36 - i * 0.04, 5),
      obsidianMat
    );
    segSpine.position.set(0, tailRadii[i] * 0.95, -len * 0.45);
    segSpine.rotation.x = -0.55;
    segPivot.add(segSpine);

    // Obsidian spade blade on final tail segment
    if (i === 4) {
      const spade = asMajor(new THREE.Mesh(new THREE.OctahedronGeometry(0.46, 0), obsidianMat));
      spade.scale.set(1.05, 0.18, 1.95);
      spade.position.set(0, 0, -len - 0.32);
      segPivot.add(spade);
    }

    tailSegs.push(segPivot);
    tailParent = segPivot;
  }

  // ---- 2 Wide Articulated Bat-Like Wings (~13.4m Total Wingspan!) ----
  const wingShoulderGeo = new THREE.SphereGeometry(0.36, 6, 6);
  const thumbClawGeo = new THREE.ConeGeometry(0.09, 0.52, 5);

  function buildWing(side) {
    const innerPivot = new THREE.Group();
    innerPivot.position.set(side * 1.02, 0.62, 0.58);
    bodyGroup.add(innerPivot);

    // Shoulder joint cap
    const shoulder = new THREE.Mesh(wingShoulderGeo, darkScaleMat);
    innerPivot.add(shoulder);

    const elbowPos = new THREE.Vector3(side * 2.45, 0.10, 0.24);
    innerPivot.add(asMajor(makeSparBetween(new THREE.Vector3(0, 0, 0), elbowPos, 0.18, 0.13, darkScaleMat)));

    // Inner wing membrane connecting torso flank to elbow
    const innerWeb = asMajor(new THREE.Mesh(
      createTriangleFanWeb([
        [0, 0, 0.12],
        [side * 2.45, 0.08, 0.24],
        [side * 2.28, -0.14, -2.18],
        [side * 1.15, -0.10, -1.85],
        [0, -0.22, -2.08],
      ]),
      membraneMat
    ));
    innerPivot.add(innerWeb);

    // Outer wing elbow/wrist pivot
    const outerPivot = new THREE.Group();
    outerPivot.position.copy(elbowPos);
    innerPivot.add(outerPivot);

    // Iconic wing-bend thumb claw
    const thumbClaw = new THREE.Mesh(thumbClawGeo, obsidianMat);
    thumbClaw.position.set(side * 0.08, 0.12, 0.26);
    thumbClaw.rotation.set(0.95, 0, -side * 0.22);
    outerPivot.add(thumbClaw);

    // 4 Radiating bony bat-wing fingers (tip reaches 1.02 + 2.45 + 3.30 = 6.77m each side -> 13.5m span!)
    const f0 = new THREE.Vector3(side * 3.30, 0.14, -0.32);
    const f1 = new THREE.Vector3(side * 2.88, -0.04, -1.78);
    const f2 = new THREE.Vector3(side * 1.82, -0.14, -2.65);
    const f3 = new THREE.Vector3(side * 0.42, -0.18, -2.62);
    const fInner = new THREE.Vector3(-side * 0.18, -0.22, -2.40);

    const origin = new THREE.Vector3(0, 0, 0);
    outerPivot.add(asMajor(makeSparBetween(origin, f0, 0.125, 0.045, darkScaleMat)));
    outerPivot.add(makeSparBetween(origin, f1, 0.095, 0.036, darkScaleMat));
    outerPivot.add(makeSparBetween(origin, f2, 0.085, 0.032, darkScaleMat));
    outerPivot.add(makeSparBetween(origin, f3, 0.075, 0.028, darkScaleMat));

    // Scalloped crimson wing webs between finger spars
    const mid01 = [
      (f0.x + f1.x) * 0.46,
      (f0.y + f1.y) * 0.5 - 0.04,
      (f0.z + f1.z) * 0.46,
    ];
    const mid12 = [
      (f1.x + f2.x) * 0.45,
      (f1.y + f2.y) * 0.5 - 0.05,
      (f1.z + f2.z) * 0.45,
    ];
    const mid23 = [
      (f2.x + f3.x) * 0.44,
      (f2.y + f3.y) * 0.5 - 0.05,
      (f2.z + f3.z) * 0.44,
    ];

    outerPivot.add(asMajor(new THREE.Mesh(
      createTriangleFanWeb([
        [0, 0, 0],
        [f0.x, f0.y, f0.z],
        mid01,
        [f1.x, f1.y, f1.z],
        mid12,
        [f2.x, f2.y, f2.z],
        mid23,
        [f3.x, f3.y, f3.z],
        [fInner.x, fInner.y, fInner.z],
      ]),
      membraneMat
    )));

    return { innerPivot, outerPivot };
  }

  const leftWing = buildWing(1);
  const rightWing = buildWing(-1);

  applyShadows(group);
  scene.add(group);

  // ---- Single-Draw-Call Dragon Fire Breath Particle System (THREE.Points + ShaderMaterial) ----
  const FIRE_POOL_SIZE = 150;
  const fireGlowTex = textures && textures.softGlow ? textures.softGlow : null;

  const firePositions = new Float32Array(FIRE_POOL_SIZE * 3);
  const fireSizes = new Float32Array(FIRE_POOL_SIZE);
  const fireColors = new Float32Array(FIRE_POOL_SIZE * 3);
  const fireAlphas = new Float32Array(FIRE_POOL_SIZE);
  const fireSeeds = new Float32Array(FIRE_POOL_SIZE);

  const firePosAttr = new THREE.BufferAttribute(firePositions, 3);
  const fireSizeAttr = new THREE.BufferAttribute(fireSizes, 1);
  const fireColorAttr = new THREE.BufferAttribute(fireColors, 3);
  const fireAlphaAttr = new THREE.BufferAttribute(fireAlphas, 1);
  const fireSeedAttr = new THREE.BufferAttribute(fireSeeds, 1);
  firePosAttr.setUsage(THREE.DynamicDrawUsage);
  fireSizeAttr.setUsage(THREE.DynamicDrawUsage);
  fireColorAttr.setUsage(THREE.DynamicDrawUsage);
  fireAlphaAttr.setUsage(THREE.DynamicDrawUsage);
  fireSeedAttr.setUsage(THREE.DynamicDrawUsage);

  const fireGeo = new THREE.BufferGeometry();
  fireGeo.setAttribute('position', firePosAttr);
  fireGeo.setAttribute('aSize', fireSizeAttr);
  fireGeo.setAttribute('aColor', fireColorAttr);
  fireGeo.setAttribute('aAlpha', fireAlphaAttr);
  fireGeo.setAttribute('aSeed', fireSeedAttr);

  const fireUniforms = {
    uMap: { value: fireGlowTex },
    uHasMap: { value: fireGlowTex ? 1.0 : 0.0 },
    uPixelHeight: {
      value: typeof window !== 'undefined'
        ? window.innerHeight * Math.min(window.devicePixelRatio || 1, 1.75)
        : 900,
    },
    uTime: { value: 0 },
  };

  const fireMat = new THREE.ShaderMaterial({
    uniforms: fireUniforms,
    vertexShader: /* glsl */ `
      attribute float aSize;
      attribute vec3 aColor;
      attribute float aAlpha;
      attribute float aSeed;

      uniform float uPixelHeight;
      uniform float uTime;

      varying vec3 vColor;
      varying float vAlpha;
      varying float vAngle;
      varying float vPhase;

      void main() {
        vColor = aColor;
        vAlpha = aAlpha;
        float spinDir = aSeed > 0.5 ? 1.0 : -1.0;
        vAngle = aSeed * 6.2831853 + uTime * (1.8 + fract(aSeed * 7.13) * 2.4) * spinDir;
        vPhase = aSeed * 25.0 + uTime * 7.0;

        if (aAlpha <= 0.001) {
          gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
          gl_PointSize = 0.0;
          return;
        }

        vec4 mvPosition = modelViewMatrix * vec4(position, 1.0);
        gl_Position = projectionMatrix * mvPosition;
        float depth = max(0.6, -mvPosition.z);
        float pxSize = aSize * projectionMatrix[1][1] * (uPixelHeight * 0.5) / depth;
        gl_PointSize = clamp(pxSize, 1.0, 280.0);
      }
    `,
    fragmentShader: /* glsl */ `
      uniform sampler2D uMap;
      uniform float uHasMap;

      varying vec3 vColor;
      varying float vAlpha;
      varying float vAngle;
      varying float vPhase;

      void main() {
        if (vAlpha <= 0.001) discard;

        vec2 uv = gl_PointCoord;
        vec2 centered = uv - 0.5;
        float r = length(centered);
        if (r > 0.5) discard;

        float cs = cos(vAngle);
        float sn = sin(vAngle);
        vec2 rot = vec2(centered.x * cs - centered.y * sn, centered.x * sn + centered.y * cs);
        float wisp = sin(rot.x * 10.0 + vPhase) * cos(rot.y * 10.0 - vPhase * 0.8) * 0.042;
        vec2 sampleUv = clamp(uv + vec2(wisp, -wisp * 0.75), 0.0, 1.0);

        vec4 tex;
        if (uHasMap > 0.5) {
          tex = texture2D(uMap, sampleUv);
        } else {
          float falloff = pow(max(0.0, 1.0 - r * 2.0), 1.8);
          tex = vec4(1.0, 0.9, 0.75, falloff);
        }

        float flicker = 0.88 + 0.12 * sin(rot.x * 13.0 - rot.y * 11.0 + vPhase * 1.2);
        float edgeSoft = smoothstep(0.5, 0.34, r);
        float alpha = tex.a * vAlpha * edgeSoft;
        if (alpha <= 0.002) discard;

        vec3 col = tex.rgb * vColor * flicker * (1.25 + 0.55 * tex.a);
        col = col / (col + vec3(0.32));
        col = pow(max(col, vec3(0.0)), vec3(0.454545));
        gl_FragColor = vec4(col, alpha);
      }
    `,
    transparent: true,
    blending: THREE.AdditiveBlending,
    depthWrite: false,
    toneMapped: false,
  });

  const firePoints = new THREE.Points(fireGeo, fireMat);
  firePoints.frustumCulled = false;
  firePoints.renderOrder = 6;
  firePoints.visible = false;
  firePoints.onBeforeRender = (renderer) => {
    renderer.getDrawingBufferSize(_vpSize);
    fireUniforms.uPixelHeight.value = _vpSize.y || 720;
  };
  scene.add(firePoints);

  const firePool = [];
  for (let i = 0; i < FIRE_POOL_SIZE; i++) {
    firePool.push({
      active: false,
      isEmber: false,
      age: 0,
      maxAge: 0.72,
      vx: 0,
      vy: 0,
      vz: 0,
      rx: 1,
      ry: 0,
      rz: 0,
      ux: 0,
      uy: 1,
      uz: 0,
      startScale: 1.2,
      endScale: 5.8,
      swirlPhase: 0,
      swirlFreq: 8.5,
      swirlAmp: 3.0,
    });
  }

  let firePoolCursor = 0;
  let fireEmitAccum = 0;
  let fireActiveCount = 0;
  let fireDirty = false;
  let wasBreathingFire = false;
  let fireSoundCd = 0;
  let flapPhase = 0;
  let prevFlapSin = 0;
  let flapSoundCd = 0;
  let smoothedTurnRate = 0;

  // Preallocated fire volume payload (zero per-frame allocations)
  const fireVolume = {
    active: false,
    origin: new THREE.Vector3(),
    direction: new THREE.Vector3(0, 0, -1),
    range: 28,
    radius: 5.5,
    dps: 68,
  };

  function spawnFireParticle(origin, dir, right, up, baseSpeed, subStep) {
    const idx = firePoolCursor;
    firePoolCursor = (firePoolCursor + 1) % FIRE_POOL_SIZE;
    const p = firePool[idx];

    const isEmber = (idx % 5) === 0;
    p.active = true;
    p.isEmber = isEmber;
    p.age = subStep;
    p.maxAge = isEmber ? (0.72 + Math.random() * 0.28) : (0.66 + Math.random() * 0.22);
    p.startScale = isEmber ? (0.48 + Math.random() * 0.28) : (1.15 + Math.random() * 0.30);
    p.endScale = isEmber ? (1.05 + Math.random() * 0.55) : (5.4 + Math.random() * 1.0);

    // Energetic roaring jet (34..46 m/s ahead of dragon) with wider cone & helical swirl
    const jetSpeed = baseSpeed + 34 + Math.random() * 12;
    const theta = Math.random() * Math.PI * 2;
    const coneRad = isEmber
      ? (2.6 + Math.random() * 4.4)
      : (0.8 + Math.sqrt(Math.random()) * 3.8);
    const spreadR = Math.cos(theta) * coneRad;
    const spreadU = Math.sin(theta) * coneRad;

    p.vx = dir.x * jetSpeed + right.x * spreadR + up.x * spreadU;
    p.vy = dir.y * jetSpeed + right.y * spreadR + up.y * spreadU;
    p.vz = dir.z * jetSpeed + right.z * spreadR + up.z * spreadU;

    p.rx = right.x;
    p.ry = right.y;
    p.rz = right.z;
    p.ux = up.x;
    p.uy = up.y;
    p.uz = up.z;
    p.swirlPhase = theta;
    p.swirlFreq = ((isEmber ? 11.5 : 8.2) + Math.random() * 4.5) * ((idx & 1) === 0 ? 1 : -1);
    p.swirlAmp = isEmber ? (4.0 + Math.random() * 3.0) : (2.2 + Math.random() * 2.6);

    const jR = (Math.random() - 0.5) * 0.34;
    const jU = (Math.random() - 0.5) * 0.28;
    const p3 = idx * 3;
    firePositions[p3] = origin.x + right.x * jR + up.x * jU + p.vx * subStep;
    firePositions[p3 + 1] = origin.y + right.y * jR + up.y * jU + p.vy * subStep;
    firePositions[p3 + 2] = origin.z + right.z * jR + up.z * jU + p.vz * subStep;
    fireSizes[idx] = p.startScale;
    fireColors[p3] = _cWhite.r;
    fireColors[p3 + 1] = _cWhite.g;
    fireColors[p3 + 2] = _cWhite.b;
    fireAlphas[idx] = isEmber ? 0.94 : 0.78;
    fireSeeds[idx] = Math.random();

    fireDirty = true;
    fireActiveCount++;
  }

  function updateFireParticles(step, elapsed) {
    fireUniforms.uTime.value = elapsed;
    fireActiveCount = 0;
    fireDirty = false;
    const drag = Math.max(0, 1 - step * 0.44);

    for (let i = 0; i < FIRE_POOL_SIZE; i++) {
      const p = firePool[i];
      if (!p.active) continue;

      fireDirty = true;
      p.age += step;
      const p3 = i * 3;

      if (p.age >= p.maxAge) {
        p.active = false;
        fireAlphas[i] = 0;
        fireSizes[i] = 0;
        continue;
      }

      fireActiveCount++;
      const t = p.age / p.maxAge;

      // Mild aerodynamic drag + progressive heat buoyancy near far billows
      p.vx *= drag;
      p.vz *= drag;
      p.vy = p.vy * drag + (p.isEmber ? 3.6 : 2.2 * t) * step;

      // Energetic helical turbulent swirl expanding outward along the streaming cone
      const swirlAngle = p.swirlPhase + p.age * p.swirlFreq;
      const swirlMag = p.swirlAmp * (0.35 + 1.35 * t);
      const swC = Math.cos(swirlAngle) * swirlMag;
      const swS = Math.sin(swirlAngle) * swirlMag;

      firePositions[p3] += (p.vx + p.rx * swC + p.ux * swS) * step;
      firePositions[p3 + 1] += (p.vy + p.ry * swC + p.uy * swS) * step;
      firePositions[p3 + 2] += (p.vz + p.rz * swC + p.uz * swS) * step;

      // Growing flame scale: ~1.2m tight core at jaws -> ~5.8m billowing roaring cone 24m ahead
      const scaleEase = p.isEmber ? Math.pow(t, 0.55) : Math.pow(t, 0.74);
      fireSizes[i] = THREE.MathUtils.lerp(p.startScale, p.endScale, scaleEase);

      // White-gold core -> vivid fiery orange -> deep crimson ember -> smooth fade-out
      let alpha;
      if (t < 0.16) {
        const k = t / 0.16;
        _cTemp.copy(_cWhite).lerp(_cGold, k);
        alpha = p.isEmber ? 0.96 : 0.78;
      } else if (t < 0.48) {
        const k = (t - 0.16) / 0.32;
        _cTemp.copy(_cGold).lerp(_cOrange, k);
        alpha = p.isEmber ? (0.94 - k * 0.24) : (0.76 - k * 0.20);
      } else if (t < 0.78) {
        const k = (t - 0.48) / 0.30;
        _cTemp.copy(_cOrange).lerp(_cCrimson, k);
        alpha = p.isEmber ? (0.70 - k * 0.36) : (0.56 - k * 0.32);
      } else {
        const k = (t - 0.78) / 0.22;
        _cTemp.copy(_cCrimson).lerp(_cEmberDark, k);
        alpha = (p.isEmber ? 0.34 : 0.24) * (1 - k);
      }

      fireColors[p3] = _cTemp.r;
      fireColors[p3 + 1] = _cTemp.g;
      fireColors[p3 + 2] = _cTemp.b;
      fireAlphas[i] = alpha;
    }
  }

  function flushFireBuffers() {
    if (fireDirty) {
      firePosAttr.needsUpdate = true;
      fireSizeAttr.needsUpdate = true;
      fireColorAttr.needsUpdate = true;
      fireAlphaAttr.needsUpdate = true;
      fireSeedAttr.needsUpdate = true;
    }
    firePoints.visible = fireActiveCount > 0;
  }

  const dragon = {
    group,
    position: group.position,
    riderSeat,
    mounted: false,
    yaw: 0.6,
    pitch: 0,
    roll: 0,
    speed: 0,
    fireEnergy: 100,
    breathingFire: false,

    mount(hero, sound) {
      this.mounted = true;
      this.position.y += 3.5;
      this.speed = 18;
      this.pitch = 0.06;
      this.roll = 0;

      if (hero && hero.group) {
        this.riderSeat.add(hero.group);
        hero.group.position.set(0, 0, 0);
        hero.group.rotation.set(0, 0, 0);
        hero.velocity.set(0, 0, 0);
        hero.onGround = false;
        hero.blocking = false;
        hero.attackTimer = 0;
        hero.rollTimer = 0;
      }

      if (sound && sound.playDragonFlap) sound.playDragonFlap();
    },

    dismount(hero, sound) {
      this.mounted = false;
      this.breathingFire = false;
      this.speed = 0;
      this.pitch = 0;
      this.roll = 0;
      mouthLight.intensity = 0;
      throatGlowMat.opacity = 0;

      // Safe landing height — if over lake water, nudge toward shore or hover-perch 0.4m above water
      let landX = this.position.x;
      let landZ = this.position.z;
      let groundY = getWalkableHeight(landX, landZ, this.position.y);

      if (groundY < WORLD.WATER_LEVEL + 0.15) {
        const dx = landX - LANDMARKS.lake.x;
        const dz = landZ - LANDMARKS.lake.z;
        const dLen = Math.hypot(dx, dz) || 1;
        const nx = dx / dLen;
        const nz = dz / dLen;
        // Probe up to 12m outward for dry bank
        let foundShore = false;
        for (let stepM = 3; stepM <= 12; stepM += 3) {
          const tx = landX + nx * stepM;
          const tz = landZ + nz * stepM;
          const ty = getWalkableHeight(tx, tz, this.position.y);
          if (ty >= WORLD.WATER_LEVEL + 0.25) {
            landX = tx;
            landZ = tz;
            groundY = ty;
            foundShore = true;
            break;
          }
        }
        if (!foundShore) {
          groundY = Math.max(groundY, WORLD.WATER_LEVEL + 0.4);
        }
      }

      this.position.set(landX, groundY, landZ);
      this.group.rotation.set(0, this.yaw, 0);

      if (hero && hero.group) {
        scene.add(hero.group);
        // Place hero 3.2m to dragon's right side at safe walkable height
        const rightX = -Math.cos(this.yaw);
        const rightZ = Math.sin(this.yaw);
        const hx = landX + rightX * 3.2;
        const hz = landZ + rightZ * 3.2;
        const hy = Math.max(WORLD.WATER_LEVEL - 0.6, getWalkableHeight(hx, hz, groundY + 2));
        hero.position.set(hx, hy, hz);
        resolveWorldCollisions(hero.position, WORLD.PLAYER_RADIUS);
        hero.position.y = Math.max(WORLD.WATER_LEVEL - 0.6, getWalkableHeight(hero.position.x, hero.position.z, hero.position.y + 1));
        hero.yaw = this.yaw;
        hero.group.rotation.set(0, this.yaw, 0);
        hero.velocity.set(0, 0, 0);
        hero.onGround = true;
      }

      if (sound && sound.playDragonFlap) sound.playDragonFlap();
    },

    getFireHitVolume() {
      this.group.updateMatrixWorld(true);
      mouthAnchor.getWorldPosition(fireVolume.origin);
      const firePitch = this.pitch - 0.08;
      const cosP = Math.cos(firePitch);
      fireVolume.direction.set(
        Math.sin(this.yaw) * cosP,
        Math.sin(firePitch),
        Math.cos(this.yaw) * cosP
      ).normalize();
      fireVolume.active = this.breathingFire;
      fireVolume.range = 28;
      fireVolume.radius = 5.5;
      fireVolume.dps = 68;
      return fireVolume;
    },

    update(dt, elapsed, input, mouseDelta, hero, sound) {
      const step = Math.min(dt, 0.05);
      if (fireSoundCd > 0) fireSoundCd = Math.max(0, fireSoundCd - step);
      if (flapSoundCd > 0) flapSoundCd = Math.max(0, flapSoundCd - step);

      if (!this.mounted) {
        // ---- Unmounted Living Perch Pose ----
        this.breathingFire = false;
        wasBreathingFire = false;
        fireEmitAccum = 0;
        this.fireEnergy = Math.min(100, this.fireEnergy + 24 * step);
        this.pitch = THREE.MathUtils.lerp(this.pitch, 0, Math.min(1, step * 5));
        this.roll = THREE.MathUtils.lerp(this.roll, 0, Math.min(1, step * 5));
        this.group.rotation.set(-this.pitch, this.yaw, this.roll);

        const breath = Math.sin(elapsed * 1.9);
        bodyGroup.position.y = 1.98 + breath * 0.055;
        bodyGroup.scale.set(1 + breath * 0.015, 1 + breath * 0.012, 1);

        // Relaxed folded wings perched on promontory
        const wingBreath = breath * 0.04;
        leftWing.innerPivot.rotation.set(0.18, 0.72, 0.44 + wingBreath);
        leftWing.outerPivot.rotation.set(0, 0.62, -1.02);
        rightWing.innerPivot.rotation.set(0.18, -0.72, -(0.44 + wingBreath));
        rightWing.outerPivot.rotation.set(0, -0.62, 1.02);

        // Planted clawed legs
        for (let i = 0; i < 4; i++) {
          legs[i].hip.rotation.x = THREE.MathUtils.lerp(legs[i].hip.rotation.x, 0, Math.min(1, step * 8));
          legs[i].knee.rotation.x = THREE.MathUtils.lerp(legs[i].knee.rotation.x, 0, Math.min(1, step * 8));
        }

        // Gentle tail undulation
        for (let i = 0; i < 5; i++) {
          tailSegs[i].rotation.y = Math.sin(elapsed * 1.45 - i * 0.68) * (0.08 + i * 0.03);
          tailSegs[i].rotation.x = -0.05 + Math.cos(elapsed * 1.2 - i * 0.5) * 0.04;
        }

        // Head subtly looks toward nearby hero
        let targetHeadYaw = Math.sin(elapsed * 0.6) * 0.14;
        let targetHeadPitch = 0.06;
        if (hero && !hero.dead) {
          const dx = hero.position.x - this.position.x;
          const dz = hero.position.z - this.position.z;
          const distSq = dx * dx + dz * dz;
          if (distSq < 26 * 26 && distSq > 1.0) {
            const toHeroYaw = Math.atan2(dx, dz);
            const relYaw = wrapAngle(toHeroYaw - this.yaw);
            targetHeadYaw = THREE.MathUtils.clamp(relYaw, -0.72, 0.72);
            const dy = (hero.position.y + 1.4) - (this.position.y + 3.4);
            targetHeadPitch = THREE.MathUtils.clamp(-dy / Math.sqrt(distSq), -0.32, 0.38);
          }
        }
        neckBase.rotation.set(-0.18 + breath * 0.025, targetHeadYaw * 0.35, 0);
        neckMid.rotation.set(0.08, targetHeadYaw * 0.35, 0);
        headPivot.rotation.set(targetHeadPitch, targetHeadYaw * 0.35, 0);
        jawPivot.rotation.x = THREE.MathUtils.lerp(jawPivot.rotation.x, 0.03 + Math.max(0, breath) * 0.04, Math.min(1, step * 8));
        mouthLight.intensity = THREE.MathUtils.lerp(mouthLight.intensity, 0, Math.min(1, step * 10));
        throatGlowMat.opacity = THREE.MathUtils.lerp(throatGlowMat.opacity, 0, Math.min(1, step * 10));

        updateFireParticles(step, elapsed);
        flushFireBuffers();
        return;
      }

      // ---- Mounted Flight Controls & Physics ----
      const mdx = mouseDelta ? mouseDelta.x : 0;
      const mdy = mouseDelta ? mouseDelta.y : 0;
      const rightInput = input ? input.right || 0 : 0;
      const fwdInput = input ? input.forward || 0 : 0;
      const upInput = input && input.up ? 1 : 0;
      const downInput = input && input.down ? 1 : 0;

      const yawDelta = -mdx * 0.0024 - rightInput * step * 1.4;
      this.yaw += yawDelta;
      this.pitch = THREE.MathUtils.clamp(this.pitch - mdy * 0.002, -0.65, 0.65);

      const targetRoll = THREE.MathUtils.clamp(-yawDelta * 14, -0.68, 0.68);
      this.roll = THREE.MathUtils.lerp(this.roll, targetRoll, Math.min(1, step * 7.5));

      // Throttle: W speeds up to 44 m/s, neutral cruise ~18 m/s, S slows to 7 m/s
      const targetSpeed = fwdInput > 0 ? 44 : (fwdInput < 0 ? 7 : 18);
      this.speed = THREE.MathUtils.lerp(this.speed, targetSpeed, Math.min(1, step * 3.2));

      const vertVel = (upInput - downInput) * 14.0;
      const cosP = Math.cos(this.pitch);
      const sinP = Math.sin(this.pitch);
      const fwdX = Math.sin(this.yaw) * cosP;
      const fwdY = sinP;
      const fwdZ = Math.cos(this.yaw) * cosP;

      this.position.x += fwdX * this.speed * step;
      this.position.y += (fwdY * this.speed + vertVel) * step;
      this.position.z += fwdZ * this.speed * step;

      // Playable world radius (220m), ceiling (115m), & safe terrain clearance
      const dist = Math.hypot(this.position.x, this.position.z);
      if (dist > 220) {
        const s = 220 / dist;
        this.position.x *= s;
        this.position.z *= s;
      }
      if (this.position.y > 115) this.position.y = 115;

      const safeFloor = Math.max(
        WORLD.WATER_LEVEL + 2.2,
        getWalkableHeight(this.position.x, this.position.z, this.position.y) + 2.4
      );
      if (this.position.y < safeFloor) {
        this.position.y = safeFloor;
        if (this.pitch < 0) this.pitch = THREE.MathUtils.lerp(this.pitch, 0, Math.min(1, step * 8));
      }

      this.group.rotation.set(-this.pitch, this.yaw, this.roll);

      // ---- Dynamic Flight Wingbeat & Gliding Pose ----
      const gliding = downInput > 0 || (this.pitch < -0.25 && upInput === 0 && fwdInput <= 0);
      const flapRate = gliding
        ? 0.8
        : (3.6 + (upInput > 0 ? 3.4 : 0) + (fwdInput > 0 ? 1.9 : 0) + Math.max(0, this.pitch) * 2.4);

      flapPhase += step * flapRate;
      const currSin = Math.sin(flapPhase);
      if (!gliding && prevFlapSin >= 0 && currSin < 0 && flapSoundCd <= 0) {
        if (sound && sound.playDragonFlap) sound.playDragonFlap();
        flapSoundCd = 0.42;
      }
      prevFlapSin = currSin;

      if (gliding) {
        // Streamlined outstretched dive/glide wings
        bodyGroup.position.y = THREE.MathUtils.lerp(bodyGroup.position.y, 1.45, Math.min(1, step * 6));
        bodyGroup.scale.set(1, 1, 1);
        leftWing.innerPivot.rotation.x = THREE.MathUtils.lerp(leftWing.innerPivot.rotation.x, -0.06, Math.min(1, step * 7));
        leftWing.innerPivot.rotation.y = THREE.MathUtils.lerp(leftWing.innerPivot.rotation.y, 0.34, Math.min(1, step * 7));
        leftWing.innerPivot.rotation.z = THREE.MathUtils.lerp(leftWing.innerPivot.rotation.z, 0.14 + Math.sin(elapsed * 2.5) * 0.03, Math.min(1, step * 7));
        leftWing.outerPivot.rotation.y = THREE.MathUtils.lerp(leftWing.outerPivot.rotation.y, 0.18, Math.min(1, step * 7));
        leftWing.outerPivot.rotation.z = THREE.MathUtils.lerp(leftWing.outerPivot.rotation.z, -0.14, Math.min(1, step * 7));

        rightWing.innerPivot.rotation.x = leftWing.innerPivot.rotation.x;
        rightWing.innerPivot.rotation.y = -leftWing.innerPivot.rotation.y;
        rightWing.innerPivot.rotation.z = -leftWing.innerPivot.rotation.z;
        rightWing.outerPivot.rotation.y = -leftWing.outerPivot.rotation.y;
        rightWing.outerPivot.rotation.z = -leftWing.outerPivot.rotation.z;
      } else {
        const innerAngle = currSin * 0.66 + 0.08;
        const outerAngle = Math.sin(flapPhase - 0.82) * 0.56 - 0.12;
        const sweepAngle = Math.cos(flapPhase) * 0.14 + 0.10;

        bodyGroup.position.y = 1.45 - currSin * 0.28;
        bodyGroup.scale.set(1, 1, 1);

        leftWing.innerPivot.rotation.set(0, sweepAngle, innerAngle);
        leftWing.outerPivot.rotation.set(0, 0.08, outerAngle);
        rightWing.innerPivot.rotation.set(0, -sweepAngle, -innerAngle);
        rightWing.outerPivot.rotation.set(0, -0.08, -outerAngle);
      }

      // Aerodynamically tucked legs in flight
      for (let i = 0; i < 4; i++) {
        legs[i].hip.rotation.x = THREE.MathUtils.lerp(legs[i].hip.rotation.x, 1.05, Math.min(1, step * 7));
        legs[i].knee.rotation.x = THREE.MathUtils.lerp(legs[i].knee.rotation.x, -0.85, Math.min(1, step * 7));
      }

      // Sinuous 5-segment tail trailing turns & flight motion
      const turnRate = yawDelta / Math.max(step, 0.008);
      smoothedTurnRate = THREE.MathUtils.lerp(smoothedTurnRate, turnRate, Math.min(1, step * 6));
      for (let i = 0; i < 5; i++) {
        const trailYaw = THREE.MathUtils.clamp(-smoothedTurnRate * 0.12, -0.30, 0.30);
        const waveY = Math.sin(elapsed * 4.4 - i * 0.72) * (0.05 + i * 0.025);
        const waveX = Math.cos(elapsed * 3.6 - i * 0.65) * 0.045 - this.pitch * 0.06;
        tailSegs[i].rotation.y = trailYaw + waveY;
        tailSegs[i].rotation.x = waveX;
      }

      // Neck & head lean into banked turns (craning forward cleanly when roaring fire)
      const wantsFire = !!(input && input.attackHeld);
      const canFire = wantsFire && this.fireEnergy > 2;
      neckBase.rotation.set(canFire ? 0.22 : 0.18, -this.roll * 0.22, 0);
      neckMid.rotation.set(canFire ? -0.10 : -0.08, -this.roll * 0.24, 0);
      headPivot.rotation.set(canFire ? -0.02 : -0.06, -this.roll * 0.18, 0);

      // Advance existing dragonfire particles first so fresh mouth spawns anchor tightly to jaws
      updateFireParticles(step, elapsed);

      // ---- Single-Draw-Call Streaming Dragonfire Breath ----
      if (canFire) {
        if (!wasBreathingFire) {
          fireEmitAccum = Math.max(fireEmitAccum, 4.0);
        }
        wasBreathingFire = true;
        this.breathingFire = true;
        this.fireEnergy = Math.max(0, this.fireEnergy - 26 * step);

        jawPivot.rotation.x = THREE.MathUtils.lerp(jawPivot.rotation.x, 0.60, Math.min(1, step * 16));
        throatGlowMat.opacity = THREE.MathUtils.lerp(throatGlowMat.opacity, 0.96, Math.min(1, step * 16));
        mouthLight.intensity = 26 + Math.sin(elapsed * 38) * 6;

        this.group.updateMatrixWorld(true);
        mouthAnchor.getWorldPosition(_v1);

        // Aim roaring fire torrent slightly downward (-0.08 relative to flight pitch) across the valley
        const firePitch = this.pitch - 0.08;
        const cosFP = Math.cos(firePitch);
        _v2.set(
          Math.sin(this.yaw) * cosFP,
          Math.sin(firePitch),
          Math.cos(this.yaw) * cosFP
        ).normalize();
        // Orthonormal cone basis (_v3 = right, _v4 = up) with zero allocations
        _v3.crossVectors(_v2, _upAxis).normalize();
        _v4.crossVectors(_v3, _v2).normalize();

        fireEmitAccum += step * 185;
        const emitCount = Math.floor(fireEmitAccum);
        if (emitCount > 0) {
          fireEmitAccum -= emitCount;
          const capped = Math.min(emitCount, 10);
          for (let k = 0; k < capped; k++) {
            const subStep = (k / capped) * step;
            spawnFireParticle(_v1, _v2, _v3, _v4, this.speed, subStep);
          }
        }

        if (fireSoundCd <= 0) {
          if (sound && sound.playFireBreath) sound.playFireBreath();
          fireSoundCd = 0.28;
        }
      } else {
        wasBreathingFire = false;
        this.breathingFire = false;
        this.fireEnergy = Math.min(100, this.fireEnergy + 24 * step);
        jawPivot.rotation.x = THREE.MathUtils.lerp(jawPivot.rotation.x, 0.04, Math.min(1, step * 10));
        throatGlowMat.opacity = THREE.MathUtils.lerp(throatGlowMat.opacity, 0, Math.min(1, step * 10));
        mouthLight.intensity = THREE.MathUtils.lerp(mouthLight.intensity, 0, Math.min(1, step * 10));
      }

      flushFireBuffers();
    },
  };

  return dragon;
}

// ============================================================================
// 3. CREATE VILLAGERS — 5 Named Stylized Fantasy NPCs
// ============================================================================

export function createVillagers(scene, textures) {
  const specs = [
    { id: 'aldric', name: 'Elder Aldric', title: 'Village Elder', questId: 'q_bridge', x: 6, z: 44, initYaw: Math.atan2(-6, 10) },
    { id: 'torin', name: 'Blacksmith Torin', title: 'Master Smith', questId: 'q_trolls', x: -14, z: 48, initYaw: Math.atan2(14, 2) },
    { id: 'lyra', name: 'Scholar Lyra', title: 'Dragon Lorekeeper', questId: 'q_dragon', x: 35, z: 13, initYaw: Math.atan2(-18, 12) },
    { id: 'kael', name: 'Warden Kael', title: 'Crypt Watcher', questId: 'q_dungeon', x: 84, z: -22, initYaw: -Math.PI * 0.5 },
    { id: 'rowena', name: 'Captain Rowena', title: 'Castle Vanguard', questId: 'q_boss', x: 7, z: -71, initYaw: 0.22 },
  ];

  const glowTex = textures && textures.softGlow ? textures.softGlow : null;

  const skinMat = new THREE.MeshStandardMaterial({ color: 0xe2b794, roughness: 0.72, metalness: 0.04 });
  const woodMat = new THREE.MeshStandardMaterial({ color: 0x563922, roughness: 0.82 });
  const darkBootMat = new THREE.MeshStandardMaterial({ color: 0x2a1d15, roughness: 0.8 });
  const eyeDotMat = new THREE.MeshStandardMaterial({ color: 0x181c26, roughness: 0.4 });
  const goldTrimMat = new THREE.MeshStandardMaterial({ color: 0xeec052, metalness: 0.56, roughness: 0.34 });

  const sharedHeadGeo = new THREE.SphereGeometry(0.165, 8, 6);
  const sharedEyeGeo = new THREE.BoxGeometry(0.032, 0.024, 0.02);

  const villagers = [];

  for (let i = 0; i < specs.length; i++) {
    const sp = specs[i];
    const group = new THREE.Group();
    const groundY = getWalkableHeight(sp.x, sp.z, 25);
    group.position.set(sp.x, groundY, sp.z);
    group.rotation.y = sp.initYaw;

    const upperBody = new THREE.Group();
    group.add(upperBody);

    const headPivot = new THREE.Group();
    headPivot.position.set(0, 1.48, 0);
    upperBody.add(headPivot);

    const rightArmPivot = new THREE.Group();
    const leftArmPivot = new THREE.Group();
    let propFloatObj = null;
    let propRingObj = null;
    let propSwingObj = null;

    // Head base (shared expressive silhouette)
    const head = asMajor(new THREE.Mesh(sharedHeadGeo, skinMat));
    head.scale.set(0.96, 1.06, 0.98);
    head.position.set(0, 0.14, 0);
    headPivot.add(head);

    const eyeL = new THREE.Mesh(sharedEyeGeo, eyeDotMat);
    eyeL.position.set(0.052, 0.15, 0.148);
    headPivot.add(eyeL);
    const eyeR = new THREE.Mesh(sharedEyeGeo, eyeDotMat);
    eyeR.position.set(-0.052, 0.15, 0.148);
    headPivot.add(eyeR);

    if (sp.id === 'aldric') {
      // Elder Aldric: navy-gold robe, white beard, glowing oak staff
      const robeMat = new THREE.MeshStandardMaterial({ color: 0x223b6e, roughness: 0.70 });
      const whiteHairMat = new THREE.MeshStandardMaterial({ color: 0xeaeef5, roughness: 0.65 });
      const orbMat = new THREE.MeshStandardMaterial({
        color: 0xffdf78,
        emissive: 0xffae28,
        emissiveIntensity: 2.0,
        roughness: 0.15,
      });

      const robe = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.22, 0.36, 1.38, 8), robeMat));
      robe.position.set(0, 0.72, 0);
      upperBody.add(robe);

      const stole = new THREE.Mesh(new THREE.BoxGeometry(0.14, 1.24, 0.44), goldTrimMat);
      stole.position.set(0, 0.80, 0.02);
      upperBody.add(stole);

      const collar = new THREE.Mesh(new THREE.CylinderGeometry(0.24, 0.32, 0.20, 8), robeMat);
      collar.position.set(0, 1.36, 0);
      upperBody.add(collar);

      // White beard & hair
      const beard = new THREE.Mesh(new THREE.ConeGeometry(0.125, 0.34, 6), whiteHairMat);
      beard.rotation.x = Math.PI;
      beard.position.set(0, -0.06, 0.10);
      headPivot.add(beard);

      const hairCap = new THREE.Mesh(new THREE.SphereGeometry(0.175, 8, 6, 0, Math.PI * 2, 0, Math.PI * 0.56), whiteHairMat);
      hairCap.position.set(0, 0.17, -0.02);
      headPivot.add(hairCap);

      // Arms
      const sleeveGeo = new THREE.CylinderGeometry(0.075, 0.10, 0.48, 6);
      leftArmPivot.position.set(0.27, 1.34, 0);
      upperBody.add(leftArmPivot);
      const lSleeve = asMajor(new THREE.Mesh(sleeveGeo, robeMat));
      lSleeve.position.set(0, -0.22, 0);
      leftArmPivot.add(lSleeve);

      rightArmPivot.position.set(-0.27, 1.34, 0);
      upperBody.add(rightArmPivot);
      const rSleeve = asMajor(new THREE.Mesh(sleeveGeo, robeMat));
      rSleeve.position.set(0, -0.22, 0.04);
      rSleeve.rotation.x = -0.35;
      rightArmPivot.add(rSleeve);

      // Glowing oak staff
      const staff = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.032, 0.04, 2.05, 6), woodMat));
      staff.position.set(-0.04, -0.32, 0.24);
      rightArmPivot.add(staff);

      const staffCrown = new THREE.Mesh(new THREE.TorusGeometry(0.11, 0.022, 5, 8), goldTrimMat);
      staffCrown.position.set(-0.04, 0.74, 0.24);
      rightArmPivot.add(staffCrown);

      const staffOrb = new THREE.Mesh(new THREE.OctahedronGeometry(0.088, 0), orbMat);
      staffOrb.position.set(-0.04, 0.74, 0.24);
      rightArmPivot.add(staffOrb);
      propFloatObj = staffOrb;

      const halo = new THREE.Sprite(new THREE.SpriteMaterial({
        map: glowTex,
        color: 0xffc652,
        transparent: true,
        opacity: 0.72,
        blending: THREE.AdditiveBlending,
        depthWrite: false,
      }));
      halo.scale.set(0.75, 0.75, 1);
      halo.position.copy(staffOrb.position);
      rightArmPivot.add(halo);

    } else if (sp.id === 'torin') {
      // Blacksmith Torin: broad shoulders, brown leather apron, forge hammer
      const tunicMat = new THREE.MeshStandardMaterial({ color: 0x3b3e46, roughness: 0.78 });
      const apronMat = new THREE.MeshStandardMaterial({ color: 0x6b4024, roughness: 0.82 });
      const ironMat = new THREE.MeshStandardMaterial({ color: 0x737d8f, metalness: 0.50, roughness: 0.40 });
      const darkHairMat = new THREE.MeshStandardMaterial({ color: 0x261a14, roughness: 0.8 });

      // Broad sturdy legs
      const legGeo = new THREE.CylinderGeometry(0.11, 0.095, 0.76, 6);
      for (let side = -1; side <= 1; side += 2) {
        const leg = asMajor(new THREE.Mesh(legGeo, darkBootMat));
        leg.position.set(side * 0.15, 0.38, 0);
        upperBody.add(leg);
      }

      // Broad muscular torso
      const torso = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.62, 0.72, 0.36), tunicMat));
      torso.position.set(0, 1.08, 0);
      upperBody.add(torso);

      const apron = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.50, 0.96, 0.39), apronMat));
      apron.position.set(0, 0.88, 0.02);
      upperBody.add(apron);

      const belt = new THREE.Mesh(new THREE.BoxGeometry(0.60, 0.10, 0.42), darkBootMat);
      belt.position.set(0, 0.78, 0);
      upperBody.add(belt);

      // Thick dark beard & hair
      const beard = new THREE.Mesh(new THREE.BoxGeometry(0.24, 0.22, 0.16), darkHairMat);
      beard.position.set(0, 0.01, 0.08);
      headPivot.add(beard);

      const hair = new THREE.Mesh(new THREE.SphereGeometry(0.175, 8, 6, 0, Math.PI * 2, 0, Math.PI * 0.55), darkHairMat);
      hair.position.set(0, 0.16, -0.02);
      headPivot.add(hair);

      // Muscular arms
      const armGeo = new THREE.CylinderGeometry(0.10, 0.088, 0.56, 6);
      leftArmPivot.position.set(0.39, 1.36, 0);
      upperBody.add(leftArmPivot);
      const lArm = asMajor(new THREE.Mesh(armGeo, skinMat));
      lArm.position.set(0, -0.26, 0);
      leftArmPivot.add(lArm);

      rightArmPivot.position.set(-0.39, 1.36, 0);
      upperBody.add(rightArmPivot);
      const rArm = asMajor(new THREE.Mesh(armGeo, skinMat));
      rArm.position.set(0, -0.26, 0.04);
      rArm.rotation.x = -0.32;
      rightArmPivot.add(rArm);

      // Heavy forge hammer
      const hammerGroup = new THREE.Group();
      hammerGroup.position.set(0, -0.48, 0.18);
      hammerGroup.rotation.x = 0.75;
      rightArmPivot.add(hammerGroup);

      const handle = new THREE.Mesh(new THREE.CylinderGeometry(0.032, 0.035, 0.78, 6), woodMat);
      handle.position.set(0, 0.18, 0);
      hammerGroup.add(handle);

      const hammerHead = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.18, 0.22, 0.36), ironMat));
      hammerHead.position.set(0, 0.56, 0);
      hammerGroup.add(hammerHead);

    } else if (sp.id === 'lyra') {
      // Scholar Lyra: teal-indigo arcane tunic, floating glowing cyan orb above hand
      const tealMat = new THREE.MeshStandardMaterial({ color: 0x1e7286, roughness: 0.62 });
      const indigoMat = new THREE.MeshStandardMaterial({ color: 0x2b3678, roughness: 0.65 });
      const hairMat = new THREE.MeshStandardMaterial({ color: 0x2a1f3a, roughness: 0.62 });
      const orbMat = new THREE.MeshStandardMaterial({
        color: PALETTE.runeCyan,
        emissive: PALETTE.runeCyan,
        emissiveIntensity: 2.2,
        roughness: 0.12,
      });

      const skirt = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.18, 0.32, 0.88, 8), indigoMat));
      skirt.position.set(0, 0.46, 0);
      upperBody.add(skirt);

      const tunic = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.20, 0.16, 0.58, 8), tealMat));
      tunic.position.set(0, 1.14, 0);
      upperBody.add(tunic);

      const mantle = asMajor(new THREE.Mesh(new THREE.ConeGeometry(0.32, 0.34, 8), indigoMat));
      mantle.position.set(0, 1.32, 0);
      upperBody.add(mantle);

      const sash = new THREE.Mesh(new THREE.BoxGeometry(0.09, 0.92, 0.36), goldTrimMat);
      sash.position.set(0, 0.82, 0.02);
      upperBody.add(sash);

      // Hair & gold circlet
      const hairBack = new THREE.Mesh(new THREE.BoxGeometry(0.30, 0.42, 0.16), hairMat);
      hairBack.position.set(0, 0.02, -0.10);
      headPivot.add(hairBack);

      const hairCrown = new THREE.Mesh(new THREE.SphereGeometry(0.175, 8, 6, 0, Math.PI * 2, 0, Math.PI * 0.58), hairMat);
      hairCrown.position.set(0, 0.16, -0.01);
      headPivot.add(hairCrown);

      const circlet = new THREE.Mesh(new THREE.TorusGeometry(0.168, 0.014, 5, 10), goldTrimMat);
      circlet.rotation.x = Math.PI * 0.5;
      circlet.position.set(0, 0.22, 0);
      headPivot.add(circlet);

      // Arms (right hand raised cradling floating arcane orb)
      const armGeo = new THREE.CylinderGeometry(0.06, 0.07, 0.47, 6);
      leftArmPivot.position.set(0.24, 1.34, 0);
      upperBody.add(leftArmPivot);
      const lArm = asMajor(new THREE.Mesh(armGeo, tealMat));
      lArm.position.set(0, -0.22, 0);
      leftArmPivot.add(lArm);

      rightArmPivot.position.set(-0.24, 1.34, 0);
      upperBody.add(rightArmPivot);
      const rArm = asMajor(new THREE.Mesh(armGeo, tealMat));
      rArm.position.set(0, -0.14, 0.18);
      rArm.rotation.x = -1.05;
      rightArmPivot.add(rArm);

      // Floating glowing cyan orb & orbiting rune ring
      const orbGroup = new THREE.Group();
      orbGroup.position.set(-0.02, 0.04, 0.42);
      rightArmPivot.add(orbGroup);

      const orb = new THREE.Mesh(new THREE.SphereGeometry(0.115, 8, 7), orbMat);
      orbGroup.add(orb);

      const runeRing = new THREE.Mesh(new THREE.TorusGeometry(0.18, 0.012, 5, 10), goldTrimMat);
      runeRing.rotation.x = 1.1;
      orbGroup.add(runeRing);

      const orbHalo = new THREE.Sprite(new THREE.SpriteMaterial({
        map: glowTex,
        color: PALETTE.runeCyan,
        transparent: true,
        opacity: 0.78,
        blending: THREE.AdditiveBlending,
        depthWrite: false,
      }));
      orbHalo.scale.set(0.85, 0.85, 1);
      orbGroup.add(orbHalo);

      propFloatObj = orbGroup;
      propRingObj = runeRing;

    } else if (sp.id === 'kael') {
      // Warden Kael: forest ranger cloak, glowing warm lantern
      const cloakMat = new THREE.MeshStandardMaterial({ color: 0x2c5636, roughness: 0.74, side: THREE.DoubleSide });
      const leatherRangerMat = new THREE.MeshStandardMaterial({ color: 0x4b3523, roughness: 0.78 });
      const lanternGlassMat = new THREE.MeshStandardMaterial({
        color: 0xffd078,
        emissive: 0xff9422,
        emissiveIntensity: 2.3,
        roughness: 0.15,
      });

      const legGeo = new THREE.CylinderGeometry(0.088, 0.075, 0.76, 6);
      for (let side = -1; side <= 1; side += 2) {
        const leg = asMajor(new THREE.Mesh(legGeo, darkBootMat));
        leg.position.set(side * 0.13, 0.38, 0);
        upperBody.add(leg);
      }

      const tunic = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.22, 0.24, 0.72, 8), leatherRangerMat));
      tunic.position.set(0, 1.08, 0);
      upperBody.add(tunic);

      // Ranger cloak & draped shoulders
      const cloak = asMajor(new THREE.Mesh(createCapeSegmentGeometry(0.56, 0.78, 1.15), cloakMat));
      cloak.position.set(0, 1.40, -0.15);
      cloak.rotation.x = 0.08;
      upperBody.add(cloak);

      const shoulderMantle = new THREE.Mesh(new THREE.CylinderGeometry(0.22, 0.34, 0.24, 8), cloakMat);
      shoulderMantle.position.set(0, 1.36, 0);
      upperBody.add(shoulderMantle);

      // Deep ranger hood
      const hood = asMajor(new THREE.Mesh(new THREE.ConeGeometry(0.24, 0.38, 8), cloakMat));
      hood.position.set(0, 0.24, -0.05);
      hood.rotation.x = -0.25;
      headPivot.add(hood);

      // Arms
      const armGeo = new THREE.CylinderGeometry(0.07, 0.065, 0.49, 6);
      leftArmPivot.position.set(0.28, 1.34, 0);
      upperBody.add(leftArmPivot);
      const lArm = asMajor(new THREE.Mesh(armGeo, cloakMat));
      lArm.position.set(0, -0.24, 0);
      leftArmPivot.add(lArm);

      rightArmPivot.position.set(-0.28, 1.34, 0);
      upperBody.add(rightArmPivot);
      const rArm = asMajor(new THREE.Mesh(armGeo, cloakMat));
      rArm.position.set(0, -0.15, 0.16);
      rArm.rotation.x = -0.95;
      rightArmPivot.add(rArm);

      // Swinging glowing warm lantern
      const lanternPivot = new THREE.Group();
      lanternPivot.position.set(0, -0.24, 0.38);
      rightArmPivot.add(lanternPivot);

      const lanternCap = new THREE.Mesh(new THREE.ConeGeometry(0.12, 0.10, 6), darkBootMat);
      lanternCap.position.set(0, -0.06, 0);
      lanternPivot.add(lanternCap);

      const lanternCore = new THREE.Mesh(new THREE.CylinderGeometry(0.082, 0.075, 0.20, 6), lanternGlassMat);
      lanternCore.position.set(0, -0.20, 0);
      lanternPivot.add(lanternCore);

      const lanternBase = new THREE.Mesh(new THREE.CylinderGeometry(0.10, 0.10, 0.04, 6), darkBootMat);
      lanternBase.position.set(0, -0.31, 0);
      lanternPivot.add(lanternBase);

      const lanternHalo = new THREE.Sprite(new THREE.SpriteMaterial({
        map: glowTex,
        color: 0xffa638,
        transparent: true,
        opacity: 0.85,
        blending: THREE.AdditiveBlending,
        depthWrite: false,
      }));
      lanternHalo.scale.set(1.15, 1.15, 1);
      lanternHalo.position.set(0, -0.20, 0);
      lanternPivot.add(lanternHalo);

      propSwingObj = lanternPivot;

    } else if (sp.id === 'rowena') {
      // Captain Rowena: silver-crimson steel breastplate, tall halberd
      const silverMat = new THREE.MeshStandardMaterial({ color: 0xb6c2d6, metalness: 0.50, roughness: 0.36 });
      const crimsonMat = new THREE.MeshStandardMaterial({ color: 0xbf2432, roughness: 0.62, side: THREE.DoubleSide });

      const legGeo = new THREE.CylinderGeometry(0.092, 0.078, 0.78, 6);
      for (let side = -1; side <= 1; side += 2) {
        const leg = asMajor(new THREE.Mesh(legGeo, silverMat));
        leg.position.set(side * 0.13, 0.39, 0);
        upperBody.add(leg);
      }

      const cuirass = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.25, 0.19, 0.64, 8), silverMat));
      cuirass.scale.set(1.12, 1.0, 0.80);
      cuirass.position.set(0, 1.10, 0);
      upperBody.add(cuirass);

      const tabard = new THREE.Mesh(new THREE.BoxGeometry(0.24, 0.86, 0.36), crimsonMat);
      tabard.position.set(0, 0.80, 0.02);
      upperBody.add(tabard);

      const cloak = asMajor(new THREE.Mesh(createCapeSegmentGeometry(0.52, 0.72, 1.05), crimsonMat));
      cloak.position.set(0, 1.40, -0.16);
      cloak.rotation.x = 0.08;
      upperBody.add(cloak);

      // Crested silver vanguard helm with crimson plume
      const helm = new THREE.Mesh(new THREE.SphereGeometry(0.185, 8, 6, 0, Math.PI * 2, 0, Math.PI * 0.65), silverMat);
      helm.position.set(0, 0.17, -0.01);
      headPivot.add(helm);

      const plume = new THREE.Mesh(new THREE.ConeGeometry(0.055, 0.32, 6), crimsonMat);
      plume.rotation.x = -1.1;
      plume.position.set(0, 0.36, -0.12);
      headPivot.add(plume);

      // Gold pauldrons & armored arms
      const pauldGeo = new THREE.SphereGeometry(0.13, 6, 5);
      const armGeo = new THREE.CylinderGeometry(0.072, 0.062, 0.50, 6);
      leftArmPivot.position.set(0.31, 1.36, 0);
      upperBody.add(leftArmPivot);
      const lPauld = new THREE.Mesh(pauldGeo, goldTrimMat);
      leftArmPivot.add(lPauld);
      const lArm = asMajor(new THREE.Mesh(armGeo, silverMat));
      lArm.position.set(0, -0.25, 0);
      leftArmPivot.add(lArm);

      rightArmPivot.position.set(-0.31, 1.36, 0);
      upperBody.add(rightArmPivot);
      const rPauld = new THREE.Mesh(pauldGeo, goldTrimMat);
      rightArmPivot.add(rPauld);
      const rArm = asMajor(new THREE.Mesh(armGeo, silverMat));
      rArm.position.set(0, -0.22, 0.08);
      rArm.rotation.x = -0.35;
      rightArmPivot.add(rArm);

      // Tall steel halberd
      const pole = asMajor(new THREE.Mesh(new THREE.CylinderGeometry(0.026, 0.030, 2.50, 6), woodMat));
      pole.position.set(-0.06, -0.10, 0.24);
      rightArmPivot.add(pole);

      const spearTip = new THREE.Mesh(new THREE.ConeGeometry(0.055, 0.36, 4), silverMat);
      spearTip.position.set(-0.06, 1.30, 0.24);
      rightArmPivot.add(spearTip);

      const axeBlade = asMajor(new THREE.Mesh(new THREE.BoxGeometry(0.026, 0.34, 0.26), silverMat));
      axeBlade.position.set(-0.06, 1.02, 0.38);
      rightArmPivot.add(axeBlade);

      const backSpike = new THREE.Mesh(new THREE.ConeGeometry(0.032, 0.22, 4), goldTrimMat);
      backSpike.rotation.x = -Math.PI * 0.5;
      backSpike.position.set(-0.06, 1.04, 0.12);
      rightArmPivot.add(backSpike);
    }

    applyShadows(group);
    scene.add(group);

    const phaseOffset = i * 1.37;
    const baseOrbY = propFloatObj ? propFloatObj.position.y : 0;
    const defaultYaw = sp.initYaw;

    villagers.push({
      id: sp.id,
      name: sp.name,
      title: sp.title,
      questId: sp.questId,
      group,
      position: group.position,

      update(dt, elapsed, playerPos) {
        const step = Math.min(dt, 0.05);
        const t = elapsed + phaseOffset;

        // Smoothly turn to face player when within 9m
        let targetYaw = defaultYaw;
        if (playerPos) {
          const dx = playerPos.x - group.position.x;
          const dz = playerPos.z - group.position.z;
          const dSq = dx * dx + dz * dz;
          if (dSq < 81 && dSq > 0.04) {
            targetYaw = Math.atan2(dx, dz);
          }
        }
        group.rotation.y = lerpAngle(group.rotation.y, targetYaw, step * 5.0);

        // Subtle organic idle life motion
        upperBody.position.y = Math.sin(t * 2.1) * 0.016;
        headPivot.rotation.y = Math.sin(t * 0.85) * 0.12;
        headPivot.rotation.x = Math.sin(t * 1.3) * 0.04;
        leftArmPivot.rotation.x = Math.sin(t * 1.6) * 0.06;
        rightArmPivot.rotation.x = Math.cos(t * 1.4) * 0.05;

        if (propFloatObj) {
          propFloatObj.position.y = baseOrbY + Math.sin(t * 2.8) * 0.045;
          propFloatObj.rotation.y += step * 1.2;
        }
        if (propRingObj) {
          propRingObj.rotation.z += step * 2.1;
        }
        if (propSwingObj) {
          propSwingObj.rotation.z = Math.sin(t * 2.4) * 0.12;
          propSwingObj.rotation.x = Math.cos(t * 1.9) * 0.07;
        }
      },
    });
  }

  return villagers;
}
