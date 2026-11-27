// ============================================================================
// src/textures.js — 100% Procedural Canvas2D Textures (zero external image files)
// ============================================================================
import * as THREE from 'three';

function makeCanvas(w, h) {
  const c = document.createElement('canvas');
  c.width = w;
  c.height = h;
  return c;
}

function finalizeTexture(canvas, repeatX = 1, repeatY = 1, isSRGB = true) {
  const tex = new THREE.CanvasTexture(canvas);
  tex.wrapS = THREE.RepeatWrapping;
  tex.wrapT = THREE.RepeatWrapping;
  tex.repeat.set(repeatX, repeatY);
  if (isSRGB) tex.colorSpace = THREE.SRGBColorSpace;
  tex.anisotropy = 4;
  tex.needsUpdate = true;
  return tex;
}

// Derive a tangent-space normal map from a canvas' luminance (Sobel, wrapped edges).
// Kills the "flat clay" read on otherwise good albedo textures.
function makeNormalMap(sourceCanvas, strength = 1.35) {
  const w = sourceCanvas.width;
  const h = sourceCanvas.height;
  const srcCtx = sourceCanvas.getContext('2d');
  const src = srcCtx.getImageData(0, 0, w, h).data;
  const out = makeCanvas(w, h);
  const outCtx = out.getContext('2d');
  const img = outCtx.createImageData(w, h);
  const lum = (x, y) => {
    const xx = (x + w) % w;
    const yy = (y + h) % h;
    const i = (yy * w + xx) * 4;
    return (src[i] * 0.2126 + src[i + 1] * 0.7152 + src[i + 2] * 0.0722) / 255;
  };
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const dx = (lum(x + 1, y) - lum(x - 1, y)) * strength;
      const dy = (lum(x, y + 1) - lum(x, y - 1)) * strength;
      const inv = 1 / Math.sqrt(dx * dx + dy * dy + 1);
      const i = (y * w + x) * 4;
      img.data[i] = (-dx * inv * 0.5 + 0.5) * 255;
      img.data[i + 1] = (dy * inv * 0.5 + 0.5) * 255;
      img.data[i + 2] = (inv * 0.5 + 0.5) * 255;
      img.data[i + 3] = 255;
    }
  }
  outCtx.putImageData(img, 0, 0);
  return out;
}

// Mid-gray roughness variation so microfacet response isn't perfectly uniform
function makeRoughnessNoise(base = 205, variance = 42, seed = 7) {
  const c = makeCanvas(256, 256);
  const ctx = c.getContext('2d');
  ctx.fillStyle = `rgb(${base},${base},${base})`;
  ctx.fillRect(0, 0, 256, 256);
  for (let i = 0; i < 5200; i++) {
    const x = (Math.sin(i * 12.9898 + seed) * 43758.5453 % 1 + 1) % 1 * 256;
    const y = (Math.sin(i * 78.233 + seed * 1.7) * 24634.6345 % 1 + 1) % 1 * 256;
    const v = Math.max(0, Math.min(255, base + (((i * 37 + seed * 11) % (variance * 2)) - variance)));
    ctx.fillStyle = `rgb(${v},${v},${v})`;
    ctx.fillRect(x, y, 2 + (i % 3), 2 + ((i * 7) % 4));
  }
  return c;
}

export function createProceduralTextures() {
  // 1. Grass / Terrain Micro-Detail Texture
  const grassCanvas = makeCanvas(256, 256);
  const gCtx = grassCanvas.getContext('2d');
  gCtx.fillStyle = '#c8d6bc';
  gCtx.fillRect(0, 0, 256, 256);
  for (let i = 0; i < 4200; i++) {
    const x = (Math.sin(i * 12.9898) * 43758.5453 % 1 + 1) % 1 * 256;
    const y = (Math.sin(i * 78.233) * 24634.6345 % 1 + 1) % 1 * 256;
    const shade = 175 + ((i * 29) % 65);
    gCtx.fillStyle = `rgb(${shade - 10}, ${shade + 8}, ${shade - 22})`;
    gCtx.fillRect(x, y, 2, 5);
  }
  const terrainDetail = finalizeTexture(grassCanvas, 48, 48, true);

  // 2. Castle & Bridge Stone Masonry
  const stoneCanvas = makeCanvas(256, 256);
  const sCtx = stoneCanvas.getContext('2d');
  sCtx.fillStyle = '#3b3f47';
  sCtx.fillRect(0, 0, 256, 256);
  const rows = 8;
  const rowH = 256 / rows;
  for (let r = 0; r < rows; r++) {
    const cols = 4;
    const colW = 256 / cols;
    const offset = (r % 2) * (colW * 0.5);
    for (let c = -1; c <= cols; c++) {
      const bx = c * colW + offset;
      const by = r * rowH;
      const v = 96 + (((r * 17 + c * 31) & 31) - 12);
      sCtx.fillStyle = `rgb(${v}, ${v + 3}, ${v + 8})`;
      sCtx.fillRect(bx + 2, by + 2, colW - 4, rowH - 4);
      // Subtle highlight edge
      sCtx.fillStyle = `rgba(255,255,255,0.08)`;
      sCtx.fillRect(bx + 2, by + 2, colW - 4, 3);
      // Subtle grit
      for (let k = 0; k < 14; k++) {
        const gx = bx + 4 + ((k * 37 + r * 11) % (colW - 8));
        const gy = by + 4 + ((k * 19 + c * 13) % (rowH - 8));
        sCtx.fillStyle = k % 2 ? 'rgba(0,0,0,0.14)' : 'rgba(255,255,255,0.08)';
        sCtx.fillRect(gx, gy, 3, 2);
      }
    }
  }
  const stoneMasonry = finalizeTexture(stoneCanvas, 4, 4, true);

  // 3. Village Timber & Plaster Wall
  const wallCanvas = makeCanvas(256, 256);
  const wCtx = wallCanvas.getContext('2d');
  wCtx.fillStyle = '#dfd5c3';
  wCtx.fillRect(0, 0, 256, 256);
  for (let i = 0; i < 600; i++) {
    wCtx.fillStyle = i % 2 ? 'rgba(120,100,80,0.06)' : 'rgba(255,250,240,0.08)';
    wCtx.fillRect((i * 53) % 256, (i * 97) % 256, 6, 4);
  }
  wCtx.fillStyle = '#4e3524';
  wCtx.fillRect(0, 0, 256, 18);
  wCtx.fillRect(0, 238, 256, 18);
  wCtx.fillRect(0, 0, 18, 256);
  wCtx.fillRect(238, 0, 18, 256);
  wCtx.fillRect(120, 0, 16, 256);
  wCtx.strokeStyle = '#4e3524';
  wCtx.lineWidth = 12;
  wCtx.beginPath();
  wCtx.moveTo(14, 14);
  wCtx.lineTo(124, 242);
  wCtx.moveTo(242, 14);
  wCtx.lineTo(132, 242);
  wCtx.stroke();
  const cottageWall = finalizeTexture(wallCanvas, 1, 1, true);

  // 4. Shingle Roof
  const roofCanvas = makeCanvas(256, 256);
  const rCtx = roofCanvas.getContext('2d');
  rCtx.fillStyle = '#4f2c22';
  rCtx.fillRect(0, 0, 256, 256);
  for (let r = 0; r < 16; r++) {
    for (let c = 0; c < 8; c++) {
      const ox = (r % 2) * 16;
      const shade = 75 + ((r * 13 + c * 7) % 26);
      rCtx.fillStyle = `rgb(${shade + 22}, ${shade - 8}, ${shade - 16})`;
      rCtx.fillRect(c * 32 + ox - 16, r * 16, 30, 14);
    }
  }
  const cottageRoof = finalizeTexture(roofCanvas, 2, 2, true);

  // 5. Dragon Scales (crimson metallic)
  const scaleCanvas = makeCanvas(256, 256);
  const dCtx = scaleCanvas.getContext('2d');
  dCtx.fillStyle = '#6f1111';
  dCtx.fillRect(0, 0, 256, 256);
  for (let r = 0; r < 14; r++) {
    for (let c = 0; c < 10; c++) {
      const cx = c * 28 + (r % 2) * 14;
      const cy = r * 20;
      const grad = dCtx.createRadialGradient(cx, cy - 4, 2, cx, cy, 16);
      grad.addColorStop(0, '#e33b2d');
      grad.addColorStop(0.65, '#9d1818');
      grad.addColorStop(1, '#3c0709');
      dCtx.fillStyle = grad;
      dCtx.beginPath();
      dCtx.arc(cx, cy, 14, 0, Math.PI * 2);
      dCtx.fill();
    }
  }
  const dragonScales = finalizeTexture(scaleCanvas, 2, 2, true);

  // 6. Carved Rune Pillar Texture
  const runeCanvas = makeCanvas(256, 512);
  const ruCtx = runeCanvas.getContext('2d');
  ruCtx.fillStyle = '#505560';
  ruCtx.fillRect(0, 0, 256, 512);
  for (let i = 0; i < 900; i++) {
    ruCtx.fillStyle = i % 2 ? 'rgba(0,0,0,0.12)' : 'rgba(255,255,255,0.06)';
    ruCtx.fillRect((i * 43) % 256, (i * 89) % 512, 4, 6);
  }
  ruCtx.strokeStyle = '#ffd56a';
  ruCtx.lineWidth = 5;
  ruCtx.shadowColor = '#ffaa22';
  ruCtx.shadowBlur = 12;
  for (let i = 0; i < 5; i++) {
    const y = 64 + i * 92;
    ruCtx.beginPath();
    ruCtx.moveTo(128, y - 28);
    ruCtx.lineTo(128, y + 28);
    ruCtx.moveTo(100, y - 10);
    ruCtx.lineTo(128, y - 28);
    ruCtx.lineTo(156, y - 4);
    ruCtx.moveTo(106, y + 14);
    ruCtx.lineTo(150, y - 10);
    ruCtx.stroke();
  }
  const runeStone = finalizeTexture(runeCanvas, 1, 1, true);

  // 6b. Bark (vertical fissures) — breaks up trunks & dead trees
  const barkCanvas = makeCanvas(256, 256);
  const bCtx = barkCanvas.getContext('2d');
  bCtx.fillStyle = '#43301f';
  bCtx.fillRect(0, 0, 256, 256);
  for (let x = 0; x < 256; x += 6) {
    const wave = Math.sin(x * 0.31) * 7 + Math.sin(x * 0.11 + 1.7) * 11;
    const shade = 46 + ((x * 41) % 38);
    bCtx.fillStyle = `rgb(${shade + 18},${shade + 2},${shade - 12})`;
    bCtx.fillRect(x + wave * 0.15, 0, 4, 256);
  }
  for (let i = 0; i < 260; i++) {
    const x = (i * 67) % 256;
    const y = (i * 131) % 256;
    const len = 14 + ((i * 29) % 46);
    bCtx.fillStyle = i % 3 ? 'rgba(20,12,6,0.5)' : 'rgba(122,92,58,0.42)';
    bCtx.fillRect(x, y, 3 + (i % 2), len);
  }
  const bark = finalizeTexture(barkCanvas, 1, 2, true);

  // 6c. Data maps derived from the albedo canvases (linear color space)
  const terrainNormal = finalizeTexture(makeNormalMap(grassCanvas, 1.15), 48, 48, false);
  const stoneNormal = finalizeTexture(makeNormalMap(stoneCanvas, 1.5), 4, 4, false);
  const roofNormal = finalizeTexture(makeNormalMap(roofCanvas, 1.4), 2, 2, false);
  const dragonScaleNormal = finalizeTexture(makeNormalMap(scaleCanvas, 1.7), 2, 2, false);
  const barkNormal = finalizeTexture(makeNormalMap(barkCanvas, 1.6), 1, 2, false);
  const terrainRough = finalizeTexture(makeRoughnessNoise(208, 40, 7), 48, 48, false);
  const stoneRough = finalizeTexture(makeRoughnessNoise(212, 34, 13), 4, 4, false);

  // 7. Soft Radial Glow Sprite (fire, embers, sun rays, dust, loot halo)
  const glowCanvas = makeCanvas(128, 128);
  const glCtx = glowCanvas.getContext('2d');
  const glGrad = glCtx.createRadialGradient(64, 64, 2, 64, 64, 62);
  glGrad.addColorStop(0.0, 'rgba(255,255,255,1)');
  glGrad.addColorStop(0.25, 'rgba(255,235,190,0.85)');
  glGrad.addColorStop(0.6, 'rgba(255,140,40,0.32)');
  glGrad.addColorStop(1.0, 'rgba(255,80,10,0)');
  glCtx.fillStyle = glGrad;
  glCtx.fillRect(0, 0, 128, 128);
  const softGlow = new THREE.CanvasTexture(glowCanvas);
  softGlow.colorSpace = THREE.SRGBColorSpace;

  return {
    terrainDetail,
    stoneMasonry,
    cottageWall,
    cottageRoof,
    dragonScales,
    runeStone,
    softGlow,
    bark,
    terrainNormal,
    stoneNormal,
    roofNormal,
    dragonScaleNormal,
    barkNormal,
    terrainRough,
    stoneRough,
  };
}
