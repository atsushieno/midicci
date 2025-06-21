import { app, BrowserWindow, ipcMain } from 'electron';
import { join } from 'path';

const isDev = process.env.IS_DEV === 'true';

const libPath = '/home/ubuntu/repos/midicci/src';
const currentPath = process.env.LD_LIBRARY_PATH || '';
if (!currentPath.includes(libPath)) {
  process.env.LD_LIBRARY_PATH = libPath + ':' + currentPath;
}

let nativeBridge: any = null;

try {
  nativeBridge = require('../../native/build/Release/midicci_bridge.node');
  console.log('✅ Native bridge loaded successfully in main process');
} catch (error) {
  console.warn('Failed to load native bridge in main process:', error);
}

let repository: any = null;
let midiManager: any = null;

async function initializeNativeBridge() {
  if (!nativeBridge) return false;
  
  try {
    repository = new nativeBridge.CIToolRepository();
    await repository.initialize();
    midiManager = await repository.getMidiDeviceManager();
    if (midiManager) {
      await midiManager.initialize();
    }
    return true;
  } catch (error) {
    console.error('Failed to initialize native bridge:', error);
    return false;
  }
}

function createWindow(): void {
  const mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      preload: join(__dirname, '../preload/preload.js'),
      nodeIntegration: false,
      contextIsolation: true,
    },
  });

  if (isDev) {
    mainWindow.loadURL('http://localhost:5173');
    mainWindow.webContents.openDevTools();
  } else {
    mainWindow.loadFile(join(__dirname, '../dist/index.html'));
  }
}

app.whenReady().then(async () => {
  await initializeNativeBridge();
  createWindow();

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

ipcMain.handle('get-app-version', () => {
  return app.getVersion();
});

ipcMain.handle('midi-get-available-devices', async () => {
  if (!midiManager) {
    return { inputs: [], outputs: [] };
  }
  
  try {
    const devices = await midiManager.getDevices();
    return {
      inputs: devices.inputs || [],
      outputs: devices.outputs || []
    };
  } catch (error) {
    console.error('Failed to get available devices:', error);
    return { inputs: [], outputs: [] };
  }
});

ipcMain.handle('midi-set-input-device', async (event, deviceId: string) => {
  if (!midiManager) return false;
  
  try {
    return await midiManager.openDevice(deviceId, 'input');
  } catch (error) {
    console.error('Failed to set input device:', error);
    return false;
  }
});

ipcMain.handle('midi-set-output-device', async (event, deviceId: string) => {
  if (!midiManager) return false;
  
  try {
    return await midiManager.openDevice(deviceId, 'output');
  } catch (error) {
    console.error('Failed to set output device:', error);
    return false;
  }
});
