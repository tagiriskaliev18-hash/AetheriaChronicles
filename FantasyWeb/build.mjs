// ============================================================================
// build.mjs — Bundles modular src/*.js + styles into ONE standalone index.html
// Uses isolated per-module scopes (__M) so there are zero identifier collisions.
// Zero external image files; Three.js r186 loaded from CDN via importmap.
// ============================================================================
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { execFileSync } from 'node:child_process';

const ROOT = path.resolve(import.meta.dirname);
const MODULE_ORDER = [
  'src/config.js',
  'src/textures.js',
  'src/audio.js',
  'src/world.js',
  'src/entities.js',
  'src/combat.js',
  'src/quests.js',
  'src/ui.js',
  'src/main.js',
];

const addonImports = new Map(); // specifier -> original import statement

function wrapModule(code, filename) {
  const modKey = './' + path.basename(filename);
  const exportsList = [];

  // 0. Hoist `three/addons/...` imports — bare imports are illegal inside the IIFE wrapper.
  //    `[^;]*?` keeps the match inside ONE statement so it can't swallow earlier imports.
  let working = code.replace(
    /^\s*import\s+([^;]*?)\s+from\s+['"](three\/addons\/[^'"]+)['"];?\s*$/gm,
    (_m, what, spec) => {
      if (!addonImports.has(spec)) addonImports.set(spec, `import ${what.trim()} from '${spec}';`);
      return '';
    },
  );

  // 1. Remove `import * as THREE from 'three';` (provided at top of bundle)
  let body = working.replace(/^\s*import\s+\*\s+as\s+THREE\s+from\s+['"]three['"];?\s*$/gm, '');

  // 2. Rewrite local named imports `import { ... } from './foo.js';` (supports multi-line)
  body = body.replace(
    /^\s*import\s*\{([\s\S]*?)\}\s*from\s*['"](\.\/[^'"]+)['"];?\s*$/gm,
    (_m, names, spec) => {
      const cleaned = names.replace(/\s+as\s+/g, ': ').trim();
      return `const { ${cleaned} } = __M['${spec}'];`;
    },
  );

  // 3. Rewrite exported declarations & collect export names
  body = body.replace(
    /^\s*export\s+(async\s+)?(function|class|const|let|var)\s+([A-Za-z_$][\w$]*)/gm,
    (_m, asyncKw, kind, name) => {
      exportsList.push(name);
      return `${asyncKw || ''}${kind} ${name}`;
    },
  );

  // 4. Support explicit `export { a, b };`
  body = body.replace(/^\s*export\s*\{([\s\S]*?)\};?\s*$/gm, (_m, names) => {
    names.split(',').forEach((part) => {
      const n = part.trim();
      if (n) exportsList.push(n);
    });
    return '';
  });

  const returnStmt = exportsList.length ? `\n  return { ${exportsList.join(', ')} };` : '';
  return `\n// ================ ${filename} ================\n__M['${modKey}'] = (() => {\n${body.trim()}${returnStmt}\n})();\n`;
}

const css = fs.readFileSync(path.join(ROOT, 'src/styles.css'), 'utf8');
let bundledModules = '';
for (const file of MODULE_ORDER) {
  const fullPath = path.join(ROOT, file);
  const src = fs.readFileSync(fullPath, 'utf8');
  bundledModules += wrapModule(src, file);
}
const bundledJs = `import * as THREE from 'three';\n${[...addonImports.values()].join('\n')}\nconst __M = {};\n${bundledModules}`;

const html = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  <title>Aethelgard: Ashes of the Drake — 3D Open-World Fantasy RPG</title>
  <link rel="preconnect" href="https://fonts.googleapis.com" />
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin />
  <link href="https://fonts.googleapis.com/css2?family=Cinzel:wght@600;700;800&family=JetBrains+Mono:wght@500;600&family=Plus+Jakarta+Sans:wght@400;500;600;700&display=swap" rel="stylesheet" />
  <style>
${css}
  </style>
  <script type="importmap">
  {
    "imports": {
      "three": "https://cdn.jsdelivr.net/npm/three@0.186.0/build/three.module.js",
      "three/addons/": "https://cdn.jsdelivr.net/npm/three@0.186.0/examples/jsm/"
    }
  }
  </script>
</head>
<body>
  <canvas id="game-canvas"></canvas>
  <div id="ui-root"></div>
  <script type="module">
${bundledJs}
  </script>
</body>
</html>
`;

const outPath = path.join(ROOT, 'index.html');
fs.writeFileSync(outPath, html, 'utf8');

// Verify syntax of bundled ES module automatically
const tmpCheck = path.join(os.tmpdir(), 'aethelgard_bundle_check.mjs');
fs.writeFileSync(tmpCheck, bundledJs, 'utf8');
try {
  execFileSync(process.execPath, ['--check', tmpCheck], { stdio: 'inherit' });
} finally {
  fs.rmSync(tmpCheck, { force: true });
}
console.log(`Built & verified ${outPath} (${(Buffer.byteLength(html) / 1024).toFixed(1)} KB)`);
