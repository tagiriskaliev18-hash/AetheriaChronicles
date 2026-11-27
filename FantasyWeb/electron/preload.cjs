// ============================================================================
// electron/preload.cjs — Safe IPC bridge for Desktop & Steamworks features
// ============================================================================
const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('desktopAPI', {
  isDesktop: true,
  platform: process.platform,
  toggleFullscreen: () => ipcRenderer.invoke('desktop:toggleFullscreen'),
  quit: () => ipcRenderer.invoke('desktop:quit'),
  unlockAchievement: (id) => ipcRenderer.invoke('steam:unlockAchievement', id),
});
