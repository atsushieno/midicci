import { contextBridge, ipcRenderer } from 'electron';

contextBridge.exposeInMainWorld('electronAPI', {
  getAppVersion: () => ipcRenderer.invoke('get-app-version'),
  
  midi: {
    getAvailableDevices: () => ipcRenderer.invoke('midi-get-available-devices'),
    setInputDevice: (deviceId: string) => ipcRenderer.invoke('midi-set-input-device', deviceId),
    setOutputDevice: (deviceId: string) => ipcRenderer.invoke('midi-set-output-device', deviceId),
  }
});

declare global {
  interface Window {
    electronAPI: {
      getAppVersion: () => Promise<string>;
      midi: {
        getAvailableDevices: () => Promise<{ inputs: any[], outputs: any[] }>;
        setInputDevice: (deviceId: string) => Promise<boolean>;
        setOutputDevice: (deviceId: string) => Promise<boolean>;
      };
    };
  }
}
