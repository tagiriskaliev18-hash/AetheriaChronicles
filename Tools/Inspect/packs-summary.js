#!/usr/bin/env node
// Summarises extracted .unitypackage folders (untrusted data, read-only): what each pack imports,
// which shaders its materials use, which scripts/shaders it carries, and its prefab inventory.
// usage: node packs-summary.js <PacksInspectDir> <outFile>
'use strict';
const fs = require('fs');
const path = require('path');

const root = process.argv[2];
const outFile = process.argv[3];
const KNOWN = {
  '933532a4fcc9baf4fa0491de14d08ed7': 'URP Lit (also Built-in Standard)',
  '8d2bb70cbf9db8d4da26e15b26e7a48': 'URP Simple Lit',
  '650dd9526735d5b461792c5cdadd6295': 'URP Unlit',
  '6e4ae4064600d784cac1e41a9e6f2e59': 'HDRP Lit',
  '480f866df4e9d5040bee072212d12a16': 'HDRP Unlit',
  '0000000000000000f000000000000000': 'Built-in Diffuse',
  '10755': 'Built-in (fileID only)',
};
const out = [];
const log = (s = '') => out.push(s);

function walkAssets(dir) {
  const items = [];
  for (const name of fs.readdirSync(dir)) {
    const p = path.join(dir, name);
    const st = fs.statSync(p);
    if (!st.isDirectory()) continue;
    const pn = path.join(p, 'pathname');
    if (!fs.existsSync(pn)) continue;
    items.push({ dir: p, path: fs.readFileSync(pn, 'utf8').trim() });
  }
  return items;
}

const packs = fs.readdirSync(root).filter((n) => fs.statSync(path.join(root, n)).isDirectory()).sort();
const table = [];
for (const pack of packs) {
  const items = walkAssets(path.join(root, pack));
  const byExt = {};
  const byTop = {};
  const prefabs = [];
  const scripts = [];
  const shaders = [];
  const shaderGuidToName = {};
  const shaderUse = {};
  const materialsWithUnknown = [];
  let rootPath = '';
  for (const it of items) {
    const ext = path.extname(it.path).toLowerCase() || '(folder)';
    const top = it.path.split('/').slice(0, 2).join('/');
    byExt[ext] = (byExt[ext] || 0) + 1;
    byTop[top] = (byTop[top] || 0) + 1;
    if (!rootPath && ext !== '(folder)') rootPath = it.path.split('/').slice(0, 2).join('/');
    if (ext === '.prefab') prefabs.push(it.path);
    if (ext === '.cs') scripts.push(it.path);
    if (ext === '.shader' || ext === '.shadergraph' || ext === '.shadersubgraph' || ext === '.hlsl' || ext === '.cginc') shaders.push(it.path);
    // shader guid from meta
    if (ext === '.shader' || ext === '.shadergraph') {
      const meta = path.join(it.dir, 'asset.meta');
      if (fs.existsSync(meta)) {
        const m = fs.readFileSync(meta, 'utf8').match(/^guid:\s*([0-9a-f]{32})/m);
        if (m) shaderGuidToName[m[1]] = path.basename(it.path);
      }
    }
    if (ext === '.mat' && fs.existsSync(path.join(it.dir, 'asset'))) {
      const text = fs.readFileSync(path.join(it.dir, 'asset'), 'utf8');
      const m = text.match(/m_Shader:\s*\{fileID:\s*(-?\d+),\s*guid:\s*([0-9a-f]{32})/);
      if (m) {
        const key = m[2];
        shaderUse[key] = (shaderUse[key] || 0) + 1;
        if (!KNOWN[key] && !shaderGuidToName[key]) materialsWithUnknown.push(path.basename(it.path));
      }
      // shader graph target
    }
  }
  const shaderSummary = Object.entries(shaderUse)
    .sort((a, b) => b[1] - a[1])
    .map(([g, n]) => `${KNOWN[g] || shaderGuidToName[g] || 'UNKNOWN ' + g} x${n}`);
  const graphTargets = [];
  for (const s of shaders) {
    if (!s.endsWith('.shadergraph')) continue;
    const dir = items.find((i) => i.path === s);
    if (!dir) continue;
    const f = path.join(dir.dir, 'asset');
    if (!fs.existsSync(f)) continue;
    const t = fs.readFileSync(f, 'utf8');
    const rp = [...t.matchAll(/"m_RenderPipeline"\s*:\s*"([^"]+)"/g)].map((x) => x[1]);
    graphTargets.push(`${path.basename(s)} [${[...new Set(rp)].join(',') || 'n/a'}]`);
  }
  log(`\n################ ${pack}`);
  log(`root: ${rootPath}   assets: ${items.length}`);
  log(`by ext: ${Object.entries(byExt).sort((a, b) => b[1] - a[1]).map(([k, v]) => `${k}:${v}`).join(' ')}`);
  log(`folders: ${Object.entries(byTop).sort((a, b) => b[1] - a[1]).slice(0, 8).map(([k, v]) => `${k}:${v}`).join(' | ')}`);
  log(`material shaders: ${shaderSummary.join(' ; ') || '(no materials)'}`);
  if (materialsWithUnknown.length) log(`materials with shaders NOT in package & not known: ${[...new Set(materialsWithUnknown)].slice(0, 6).join(', ')} (${materialsWithUnknown.length} mats)`);
  if (shaders.length) log(`shaders (${shaders.length}): ${shaders.slice(0, 14).map((s) => s.split('/').pop()).join(', ')}${shaders.length > 14 ? ' …' : ''}`);
  if (graphTargets.length) log(`shader graph targets: ${graphTargets.slice(0, 10).join(' ; ')}`);
  if (scripts.length) log(`SCRIPTS (${scripts.length}): ${scripts.map((s) => s.replace(rootPath + '/', '')).join(' | ')}`);
  // prefab inventory grouped by folder
  const groups = {};
  for (const p of prefabs) {
    const folder = path.dirname(p).replace(rootPath + '/', '');
    (groups[folder] = groups[folder] || []).push(path.basename(p, '.prefab'));
  }
  log(`prefabs: ${prefabs.length}`);
  for (const [folder, names] of Object.entries(groups).slice(0, 26)) {
    log(`   ${folder} (${names.length}): ${names.slice(0, 16).join(', ')}${names.length > 16 ? ' …' : ''}`);
  }
  table.push({ pack, root: rootPath, prefabs: prefabs.length, scripts: scripts.length, shaders: shaders.length, mats: Object.values(shaderUse).reduce((a, b) => a + b, 0), shaderSummary: shaderSummary.slice(0, 3).join(' ; ') });
}
log('\n################ SUMMARY TABLE');
for (const t of table) log(`${t.pack.slice(0, 46).padEnd(46)} prefabs=${String(t.prefabs).padStart(4)} mats=${String(t.mats).padStart(3)} scripts=${t.scripts} shaders=${t.shaders} | ${t.shaderSummary}`);
fs.writeFileSync(outFile, out.join('\n'));
console.log(`wrote ${outFile} (${out.length} lines)`);
