#!/usr/bin/env node
// Read-only GLB inspector: prints materials, textures/images, skins and animations
// from the JSON chunk. Usage: node glb-info.js <file.glb|dir> [more paths...]
'use strict';
const fs = require('fs');
const path = require('path');

function parseGlb(buf) {
  if (buf.readUInt32LE(0) !== 0x46546c67) throw new Error('not a GLB');
  let off = 12, json = null, binLen = 0;
  while (off < buf.length) {
    const len = buf.readUInt32LE(off);
    const type = buf.readUInt32LE(off + 4);
    const data = buf.subarray(off + 8, off + 8 + len);
    if (type === 0x4e4f534a) json = JSON.parse(data.toString('utf8'));
    else if (type === 0x004e4942) binLen = len;
    off += 8 + len;
  }
  return { json, binLen };
}

function info(file) {
  const buf = fs.readFileSync(file);
  const { json, binLen } = parseGlb(buf);
  const out = [];
  out.push(`### ${file}  (glb ${(buf.length / 1048576).toFixed(1)} MB, bin ${(binLen / 1048576).toFixed(1)} MB)`);
  out.push(`  asset: ${json.asset && json.asset.generator}  ext: ${(json.extensionsUsed || []).join(',') || '-'}`);
  const meshes = (json.meshes || []).reduce((n, m) => n + m.primitives.length, 0);
  let verts = 0, tris = 0;
  for (const m of json.meshes || []) for (const p of m.primitives) {
    if (p.attributes.POSITION != null) verts += json.accessors[p.attributes.POSITION].count;
    if (p.indices != null) tris += json.accessors[p.indices].count / 3;
  }
  out.push(`  meshes: ${meshes} prims, ${verts} verts, ${Math.round(tris)} tris; nodes: ${(json.nodes || []).length}; skins: ${(json.skins || []).length} joints=${(json.skins || []).map(s => s.joints.length).join('/')}`);
  const nodeNames = (json.nodes || []).map(n => n.name);
  const legs = ['thigh_l', 'calf_l', 'foot_l', 'thigh_r', 'calf_r', 'foot_r'].filter(n => nodeNames.includes(n));
  out.push(`  legBones: ${legs.join(',') || 'NONE'}; rootNodes: ${json.scenes[json.scene].nodes.map(i => json.nodes[i].name).join(',')}`);
  for (const m of json.materials || []) {
    const p = m.pbrMetallicRoughness || {};
    const tex = (k) => (p[k] ? `#${p[k].index}` : '-');
    const em = m.emissiveTexture ? ` emissiveTex=#${m.emissiveTexture.index}` : '';
    const alpha = m.alphaMode ? ` alpha=${m.alphaMode}` : '';
    const ds = m.doubleSided ? ' doubleSided' : '';
    out.push(`  MAT '${m.name}' baseColor=${tex('baseColorTexture')} baseFactor=${JSON.stringify(p.baseColorFactor || [1, 1, 1, 1])} metal=${tex('metallicRoughnessTexture')} mrFactor=${p.metallicFactor ?? 1}/${p.roughnessFactor ?? 1} normal=${tex('normalTexture')}${em}${alpha}${ds}`);
  }
  for (const [i, im] of (json.images || []).entries()) {
    if (im.bufferView != null) {
      const bv = json.bufferViews[im.bufferView];
      out.push(`  IMG[${i}] embedded ${im.mimeType} ${(bv.byteLength / 1024).toFixed(0)} KB name=${im.name || ''}`);
    } else out.push(`  IMG[${i}] EXTERNAL uri=${im.uri} name=${im.name || ''}`);
  }
  for (const [i, a] of (json.animations || []).entries()) {
    let maxT = 0;
    for (const s of a.samplers) maxT = Math.max(maxT, json.accessors[s.input].max ? json.accessors[s.input].max[0] : 0);
    out.push(`  ANIM[${i}] '${a.name}' channels=${a.channels.length} dur=${maxT.toFixed(2)}s`);
  }
  if (!(json.images || []).length) out.push('  IMAGES: none');
  return out.join('\n');
}

const args = process.argv.slice(2);
const files = [];
for (const a of args) {
  if (fs.statSync(a).isDirectory()) {
    const walk = (d) => { for (const f of fs.readdirSync(d)) { const p = path.join(d, f); if (fs.statSync(p).isDirectory()) walk(p); else if (/\.glb$/i.test(f)) files.push(p); } };
    walk(a);
  } else files.push(a);
}
for (const f of files) { try { console.log(info(f)); } catch (e) { console.log(`### ${f}\n  ERROR ${e.message}`); } }
