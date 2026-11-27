// Generates assets/icon.png (512x512) and multi-size Windows assets/icon.ico from assets/icon.svg
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const { chromium } = require('playwright');

const ROOT = path.resolve(import.meta.dirname, '..');
const svgPath = path.join(ROOT, 'assets', 'icon.svg');
const pngPath = path.join(ROOT, 'assets', 'icon.png');
const icoPath = path.join(ROOT, 'assets', 'icon.ico');

const svgContent = fs.readFileSync(svgPath, 'utf8');

function buildIcoFromPngs(entries) {
  // ICO header: 6 bytes + 16 bytes per image entry + concatenated PNG payloads
  const header = Buffer.alloc(6);
  header.writeUInt16LE(0, 0); // reserved
  header.writeUInt16LE(1, 2); // type = 1 (ICO)
  header.writeUInt16LE(entries.length, 4);

  const dirBuffers = [];
  let offset = 6 + entries.length * 16;

  for (const { size, buf } of entries) {
    const dir = Buffer.alloc(16);
    dir.writeUInt8(size >= 256 ? 0 : size, 0); // width (0 = 256)
    dir.writeUInt8(size >= 256 ? 0 : size, 1); // height
    dir.writeUInt8(0, 2); // palette
    dir.writeUInt8(0, 3); // reserved
    dir.writeUInt16LE(1, 4); // planes
    dir.writeUInt16LE(32, 6); // bpp
    dir.writeUInt32LE(buf.length, 8);
    dir.writeUInt32LE(offset, 12);
    offset += buf.length;
    dirBuffers.push(dir);
  }

  return Buffer.concat([header, ...dirBuffers, ...entries.map((e) => e.buf)]);
}

const browser = await chromium.launch({ headless: true });
try {
  const sizes = [512, 256, 128, 64, 48, 32, 16];
  const pngBySize = new Map();

  for (const size of sizes) {
    const page = await browser.newPage({ viewport: { width: size, height: size }, deviceScaleFactor: 1 });
    await page.setContent(`<!doctype html><html><body style="margin:0;background:transparent;width:${size}px;height:${size}px;overflow:hidden">${svgContent.replace('width="512" height="512"', `width="${size}" height="${size}"`)}</body></html>`);
    const buf = await page.screenshot({ type: 'png', omitBackground: true });
    pngBySize.set(size, buf);
    await page.close();
  }

  fs.writeFileSync(pngPath, pngBySize.get(512));
  const icoBuf = buildIcoFromPngs([256, 128, 64, 48, 32, 16].map((s) => ({ size: s, buf: pngBySize.get(s) })));
  fs.writeFileSync(icoPath, icoBuf);
  console.log(`Generated ${pngPath} (${(pngBySize.get(512).length / 1024).toFixed(1)} KB) and ${icoPath} (${(icoBuf.length / 1024).toFixed(1)} KB)`);
} finally {
  await browser.close();
}
