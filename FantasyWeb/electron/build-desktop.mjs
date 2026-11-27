// ============================================================================
// electron/build-desktop.mjs — Creates a 100% OFFLINE Steam-ready dist-desktop/
// Copies/caches Three.js r186 locally so the packaged Electron app NEVER needs
// internet access or a CDN at runtime (mandatory for Steam & Steam Deck!).
// ============================================================================
import fs from 'node:fs';
import path from 'node:path';

const ROOT = path.resolve(import.meta.dirname, '..');
const DIST_DIR = path.join(ROOT, 'dist-desktop');
const VENDOR_DIR = path.join(DIST_DIR, 'vendor');
const CACHE_DIR = path.join(ROOT, 'electron', 'vendor');

const THREE_VERSION = '0.186.0';
// Anything the bundle pulls from the CDN via the `three/addons/` importmap prefix must
// be vendored too, otherwise the "100% offline" promise breaks. Keep in sync with src/ imports!
const ADDON_FILES = [
  {
    name: 'addons/environments/RoomEnvironment.js',
    url: `https://cdn.jsdelivr.net/npm/three@${THREE_VERSION}/examples/jsm/environments/RoomEnvironment.js`,
  },
];
const THREE_FILES = [
  {
    name: 'three.module.js',
    url: `https://cdn.jsdelivr.net/npm/three@${THREE_VERSION}/build/three.module.js`,
  },
  {
    name: 'three.core.js',
    url: `https://cdn.jsdelivr.net/npm/three@${THREE_VERSION}/build/three.core.js`,
  },
  ...ADDON_FILES,
];

fs.mkdirSync(VENDOR_DIR, { recursive: true });
fs.mkdirSync(CACHE_DIR, { recursive: true });

async function ensureLocalThree() {
  for (const item of THREE_FILES) {
    const cached = path.join(CACHE_DIR, item.name);
    const nmPath = item.name.startsWith('addons/')
      ? path.join(ROOT, 'node_modules', 'three', 'examples', 'jsm', item.name.slice('addons/'.length))
      : path.join(ROOT, 'node_modules', 'three', 'build', item.name);
    const dest = path.join(VENDOR_DIR, item.name);

    fs.mkdirSync(path.dirname(cached), { recursive: true });
    fs.mkdirSync(path.dirname(dest), { recursive: true });

    if (fs.existsSync(nmPath)) {
      fs.copyFileSync(nmPath, dest);
    } else if (fs.existsSync(cached)) {
      fs.copyFileSync(cached, dest);
    } else {
      console.log(`Downloading ${item.name} (${THREE_VERSION}) for offline Steam bundle...`);
      const res = await fetch(item.url);
      if (!res.ok) throw new Error(`Failed to fetch ${item.url}: HTTP ${res.status}`);
      const buf = Buffer.from(await res.arrayBuffer());
      fs.writeFileSync(cached, buf);
      fs.writeFileSync(dest, buf);
    }
  }
}

await ensureLocalThree();

const rootHtmlPath = path.join(ROOT, 'index.html');
if (!fs.existsSync(rootHtmlPath)) {
  throw new Error('Missing index.html — run `node build.mjs` first.');
}

let html = fs.readFileSync(rootHtmlPath, 'utf8');

// 1. Replace CDN importmap with 100% local relative vendor paths (build + addons)
html = html.replace(
  /"three":\s*"https:\/\/cdn\.jsdelivr\.net\/npm\/three@[^"]+\/build\/three\.module\.js"/,
  '"three": "./vendor/three.module.js"',
);
html = html.replace(
  /"three\/addons\/":\s*"https:\/\/cdn\.jsdelivr\.net\/npm\/three@[^"]+\/examples\/jsm\/"/,
  '"three/addons/": "./vendor/addons/"',
);

// 2. Remove external Google Fonts network requests so Steam offline launch has zero stalled requests
html = html.replace(/<link[^>]*fonts\.googleapis\.com[^>]*>\s*/g, '');
html = html.replace(/<link[^>]*fonts\.gstatic\.com[^>]*>\s*/g, '');

// 3. Inject subtle Desktop Steam shortcut hint (F11 Fullscreen) + graceful Exit hook via Alt+F4 / Cmd+Q
const desktopBridgeSnippet = `
  <script>
    // Desktop Steam build offline marker
    window.__DESKTOP_OFFLINE_BUILD__ = true;
  </script>
`;
html = html.replace('</head>', `${desktopBridgeSnippet}</head>`);

const outPath = path.join(DIST_DIR, 'index.html');
fs.writeFileSync(outPath, html, 'utf8');

const totalKb =
  (fs.statSync(outPath).size +
    fs.statSync(path.join(VENDOR_DIR, 'three.module.js')).size +
    fs.statSync(path.join(VENDOR_DIR, 'three.core.js')).size) /
  1024;

console.log(`Built 100% offline Steam desktop bundle -> ${DIST_DIR} (${totalKb.toFixed(1)} KB total)`);
