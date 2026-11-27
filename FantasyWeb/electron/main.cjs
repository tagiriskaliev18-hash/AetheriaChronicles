// ============================================================================
// electron/main.cjs — Steam-ready Electron Desktop Shell (Windows & macOS)
// ============================================================================
const { app, BrowserWindow, ipcMain, Menu, screen } = require('electron');
const path = require('node:path');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');

// Minimal launch diagnostics — written next to nothing, safe to keep permanently
const LOG_FILE = path.join(os.tmpdir(), 'aethelgard-launch.log');
function logLine(...args) {
  try {
    fs.appendFileSync(LOG_FILE, `[${new Date().toISOString()}] ${args.map(a => (a && a.stack ? a.stack : String(a))).join(' ')}\n`);
  } catch {
    // logging is best-effort
  }
}
process.on('uncaughtException', (err) => { logLine('uncaughtException:', err); app.exit(1); });
process.on('unhandledRejection', (err) => { logLine('unhandledRejection:', err); app.exit(1); });

// Steam Overlay & WebGL hardware acceleration tuning
app.commandLine.appendSwitch('enable-webgl');
app.commandLine.appendSwitch('ignore-gpu-blocklist');
app.commandLine.appendSwitch('enable-gpu-rasterization');
// Keep frame pacing smooth without throttling when cursor is locked
app.commandLine.appendSwitch('disable-renderer-backgrounding');

// Optional Steamworks SDK hook (install `steamworks.js` when you have a real Steam AppID)
let steamClient = null;
function initSteamworksIfInstalled() {
  try {
    // eslint-disable-next-line global-require
    const steamworks = require('steamworks.js');
    // Replace 480 with your real Steam AppID from Steamworks Partner Dashboard
    const appId = Number(process.env.STEAM_APP_ID || 480);
    steamClient = steamworks.init(appId);
    steamworks.electronEnableSteamOverlay();
    console.log('[Steamworks] Initialized for AppID:', appId);
  } catch {
    // Running cleanly without steamworks.js (local testing / DRM-free build)
  }
}

// ES modules are CORS-blocked over file://, so the bundled game is served by a tiny
// embedded HTTP server on 127.0.0.1 (ephemeral port, loopback only — no network exposure).
const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.mjs': 'application/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.json': 'application/json; charset=utf-8',
  '.wasm': 'application/wasm',
  '.woff2': 'font/woff2',
};

let staticServer = null;
let staticPort = 0;

function ensureStaticServer() {
  if (staticServer) return Promise.resolve(staticPort);
  const desktopEntry = path.join(__dirname, '..', 'dist-desktop', 'index.html');
  const root = fs.existsSync(desktopEntry) ? path.dirname(desktopEntry) : path.join(__dirname, '..');
  staticServer = http.createServer((req, res) => {
    try {
      const p = decodeURIComponent((req.url || '/').split('?')[0]);
      const f = path.join(root, p === '/' ? 'index.html' : p.replace(/^\/+/, ''));
      if (!f.startsWith(root) || !fs.existsSync(f) || !fs.statSync(f).isFile()) {
        res.writeHead(404);
        res.end('Not found');
        return;
      }
      res.writeHead(200, { 'Content-Type': MIME[path.extname(f).toLowerCase()] || 'application/octet-stream', 'Cache-Control': 'no-cache' });
      fs.createReadStream(f).pipe(res);
    } catch {
      res.writeHead(500);
      res.end();
    }
  });
  return new Promise((resolve, reject) => {
    staticServer.once('error', reject);
    staticServer.listen(0, '127.0.0.1', () => {
      staticPort = staticServer.address().port;
      resolve(staticPort);
    });
  });
}

let mainWindow = null;

async function createGameWindow() {
  logLine('createGameWindow: enter');
  const { width, height } = screen.getPrimaryDisplay().workAreaSize;
  logLine('primary display', width, 'x', height);
  const iconPath = path.join(__dirname, '..', 'assets', process.platform === 'win32' ? 'icon.ico' : 'icon.png');

  mainWindow = new BrowserWindow({
    title: 'Aethelgard: Ashes of the Drake',
    width: Math.min(1600, width),
    height: Math.min(900, height),
    minWidth: 960,
    minHeight: 540,
    backgroundColor: '#09090f',
    fullscreen: process.env.START_WINDOWED !== '1',
    autoHideMenuBar: true,
    show: false,
    icon: fs.existsSync(iconPath) ? iconPath : undefined,
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      backgroundThrottling: false,
    },
  });

  // Remove default browser menu so shortcuts (Ctrl+R, Ctrl+W, Alt) don't disrupt gameplay
  Menu.setApplicationMenu(null);

  // Serve the 100%-offline bundle (dist-desktop/index.html) — fallback to repo root index.html.
  // ES modules require http(s), hence the embedded loopback server instead of loadFile.
  try {
    const port = await ensureStaticServer();
    logLine('static server up on port', port);
    mainWindow.loadURL(`http://127.0.0.1:${port}/`);
    logLine('loadURL called');
  } catch (err) {
    logLine('static-server FAILED:', err);
    console.error('[static-server] failed to start embedded loopback server:', err);
    app.exit(1);
    return;
  }

  mainWindow.once('ready-to-show', () => {
    logLine('ready-to-show');
    mainWindow.show();
    mainWindow.focus();
  });

  // F11 or Alt+Enter toggles Fullscreen cleanly
  mainWindow.webContents.on('before-input-event', (event, input) => {
    if (input.type !== 'keyDown') return;
    if (input.key === 'F11' || (input.alt && input.key === 'Enter')) {
      event.preventDefault();
      mainWindow.setFullScreen(!mainWindow.isFullScreen());
    } else if (input.key === 'F12' && !app.isPackaged) {
      mainWindow.webContents.toggleDevTools();
    }
  });

  // Deny external popups inside the game window
  mainWindow.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

// IPC bridge for desktop-only features (fullscreen toggle, clean quit, Steam achievements)
ipcMain.handle('desktop:toggleFullscreen', () => {
  if (!mainWindow) return false;
  const next = !mainWindow.isFullScreen();
  mainWindow.setFullScreen(next);
  return next;
});

ipcMain.handle('desktop:quit', () => {
  app.quit();
});

ipcMain.handle('steam:unlockAchievement', (_evt, achievementId) => {
  if (!steamClient || typeof achievementId !== 'string') return false;
  try {
    return steamClient.achievement.activate(achievementId);
  } catch {
    return false;
  }
});

// Single-instance lock (required for clean Steam launches)
const gotLock = app.requestSingleInstanceLock();
if (!gotLock) {
  logLine('second instance detected — quitting silently');
  app.quit();
} else {
  app.on('second-instance', () => {
    if (mainWindow) {
      if (mainWindow.isMinimized()) mainWindow.restore();
      mainWindow.focus();
    }
  });

  app.whenReady().then(() => {
    logLine('whenReady fired');
    initSteamworksIfInstalled();
    createGameWindow().catch(err => {
      logLine('createGameWindow rejected:', err);
      console.error('[window] failed to create game window:', err);
      app.exit(1);
    });

    app.on('activate', () => {
      if (BrowserWindow.getAllWindows().length === 0) {
        createGameWindow().catch(err => console.error('[window] failed to recreate game window:', err));
      }
    });
  });

  app.on('window-all-closed', () => {
    if (staticServer) {
      staticServer.close();
      staticServer = null;
    }
    app.quit();
  });
}
