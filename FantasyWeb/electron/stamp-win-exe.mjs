// ============================================================================
// electron/stamp-win-exe.mjs — Sets icon + version metadata on the unpacked
// .exe AFTER `electron-builder --win dir` and BEFORE the NSIS installer packs it.
// Needed because signAndEditExecutable=false (rcedit path is unavailable on
// machines without symlink privileges); this stamp does the same via the npm
// `rcedit` package, whose rcedit-x64.exe is bundled and needs no extra rights.
// ============================================================================
import path from 'node:path';
import fs from 'node:fs';
import { rcedit } from 'rcedit';

const ROOT = path.resolve(import.meta.dirname, '..');
const outDir = path.join(ROOT, 'release', 'win-unpacked');
const ICON = path.join(ROOT, 'assets', 'icon.ico');

const exeName = fs.readdirSync(outDir).find(f => f.toLowerCase().endsWith('.exe') && !/unins/i.test(f));
if (!exeName) {
  console.error(`[stamp] No .exe found in ${outDir} — run electron-builder --win dir first.`);
  process.exit(1);
}
const exePath = path.join(outDir, exeName);

const pkg = JSON.parse(fs.readFileSync(path.join(ROOT, 'package.json'), 'utf8'));
const version = pkg.version || '1.0.0';

await rcedit(exePath, {
  icon: ICON,
  'version-string': {
    FileDescription: 'Aethelgard: Ashes of the Drake — 3D Open-World Fantasy Action RPG',
    ProductName: 'Aethelgard - Ashes of the Drake',
    LegalCopyright: 'Copyright © 2026 Tagir Iskaliev',
    CompanyName: 'Tagir Iskaliev',
    InternalName: 'Aethelgard - Ashes of the Drake',
    OriginalFilename: exeName,
  },
  'file-version': `${version}.0`,
  'product-version': `${version}.0`,
});

console.log(`[stamp] Stamped ${exePath} (icon + v${version} metadata)`);
