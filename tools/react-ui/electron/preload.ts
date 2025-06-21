import { contextBridge, ipcRenderer } from 'electron';

contextBridge.exposeInMainWorld('electronAPI', {
  getAppVersion: () => ipcRenderer.invoke('get-app-version'),
});

declare global {
  interface Window {
    electronAPI: {
      getAppVersion: () => Promise<string>;
    };
  }
}
