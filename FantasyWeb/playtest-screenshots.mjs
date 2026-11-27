import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const { chromium } = require('playwright');

const OUT = new URL('./playtest-', import.meta.url).pathname.replace(/^\/([A-Za-z]:)/, '$1');
const URL_GAME = 'file:///C:/Users/user/Desktop/Fantasy/index.html';

const browser = await chromium.launch({
  headless: true,
  args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const page = await browser.newPage({ viewport: { width: 1600, height: 900 } });
page.on('console', m => { if (m.type() === 'error') console.log('[console.error]', m.text().slice(0, 300)); });
page.on('pageerror', e => console.log('[pageerror]', String(e).slice(0, 300)));

await page.goto(URL_GAME, { waitUntil: 'load', timeout: 30000 });
await page.waitForTimeout(3500);
await page.screenshot({ path: OUT + '01-title.png' });

// Start the game via the debug hook
await page.evaluate(() => window.__GAME_DEBUG__?.startGame?.());
await page.waitForTimeout(4000);
await page.screenshot({ path: OUT + '02-spawn.png' });

// Golden-hour view toward the lake & bridge (default tod 0.73)
await page.evaluate(() => { window.__GAME_DEBUG__.setTimeOfDay(0.73); window.__GAME_DEBUG__.teleport(0, 26, Math.PI); });
await page.waitForTimeout(1500);
await page.screenshot({ path: OUT + '03-village-lake-dusk.png' });

// Deep night at the Mirror Lake shore — wisps should be visible
await page.evaluate(() => { window.__GAME_DEBUG__.setTimeOfDay(0.02); window.__GAME_DEBUG__.teleport(0, 10, Math.PI); });
await page.waitForTimeout(1500);
await page.screenshot({ path: OUT + '04-lake-night-wisps.png' });

// Night at the Rune Circle — wisps should be visible
await page.evaluate(() => { window.__GAME_DEBUG__.teleport(38, 20, 0.4); });
await page.waitForTimeout(1500);
await page.screenshot({ path: OUT + '05-runecircle-night.png' });

// Dusk over the lake from the south bank
await page.evaluate(() => { window.__GAME_DEBUG__.setTimeOfDay(0.78); window.__GAME_DEBUG__.teleport(0, 14, Math.PI); });
await page.waitForTimeout(1500);
await page.screenshot({ path: OUT + '06-lake-dusk.png' });

await browser.close();
console.log('done');
