import { 
  MidiCINativeBridge, 
  ClientConnectionModel, 
  MidiCIProfileState, 
  PropertyValue, 
  LogEntry, 
  MidiCIProfileId, 
  PropertyMetadata,
  MidiDevice
} from '../types/midi';

declare global {
  interface Window {
    electronAPI?: {
      getAppVersion: () => Promise<string>;
      midi: {
        getAvailableDevices: () => Promise<{ inputs: any[], outputs: any[] }>;
        setInputDevice: (deviceId: string) => Promise<boolean>;
        setOutputDevice: (deviceId: string) => Promise<boolean>;
      };
    };
  }
}

export class ElectronMidiCIBridge implements MidiCINativeBridge {
  private connectionsCallbacks: ((connections: ClientConnectionModel[]) => void)[] = [];
  private profilesCallbacks: ((profiles: MidiCIProfileState[]) => void)[] = [];
  private propertiesCallbacks: ((properties: PropertyValue[]) => void)[] = [];
  private logCallbacks: ((entry: LogEntry) => void)[] = [];

  constructor() {
    if (typeof window === 'undefined' || !window.electronAPI) {
      throw new Error('Electron API not available');
    }
  }

  async initialize(): Promise<void> {
    this.setupCallbacks();
  }

  async shutdown(): Promise<void> {
  }

  async sendDiscovery(): Promise<void> {
    throw new Error('Discovery not implemented in Electron bridge');
  }

  async getConnections(): Promise<ClientConnectionModel[]> {
    return [];
  }

  async setProfile(_group: number, _address: number, _profile: MidiCIProfileId, _enabled: boolean, _numChannels: number): Promise<void> {
    throw new Error('Profile management not implemented in Electron bridge');
  }

  async subscribeProperty(_propertyId: string, _encoding?: string): Promise<void> {
    throw new Error('Property subscription not implemented in Electron bridge');
  }

  async unsubscribeProperty(_propertyId: string): Promise<void> {
    throw new Error('Property unsubscription not implemented in Electron bridge');
  }

  async refreshPropertyValue(_propertyId: string, _encoding?: string, _offset?: number, _limit?: number): Promise<void> {
    throw new Error('Property refresh not implemented in Electron bridge');
  }

  async createProperty(_metadata: PropertyMetadata): Promise<void> {
    throw new Error('Property creation not implemented in Electron bridge');
  }

  async updatePropertyMetadata(_propertyId: string, _metadata: PropertyMetadata): Promise<void> {
    throw new Error('Property metadata update not implemented in Electron bridge');
  }

  async updatePropertyValue(_propertyId: string, _resId: string | undefined, _data: Uint8Array): Promise<void> {
    throw new Error('Property value update not implemented in Electron bridge');
  }

  async removeProperty(_propertyId: string): Promise<void> {
    throw new Error('Property removal not implemented in Electron bridge');
  }

  async getLogs(): Promise<LogEntry[]> {
    return [];
  }

  async clearLogs(): Promise<void> {
  }

  async getMUID(): Promise<number> {
    return 0;
  }

  async getAvailableDevices(): Promise<{ inputs: MidiDevice[], outputs: MidiDevice[] }> {
    try {
      if (!window.electronAPI) {
        throw new Error('Electron API not available');
      }
      return await window.electronAPI.midi.getAvailableDevices();
    } catch (error) {
      console.error('Failed to get available devices via IPC:', error);
      return { inputs: [], outputs: [] };
    }
  }

  async setInputDevice(deviceId: string): Promise<boolean> {
    try {
      if (!window.electronAPI) {
        throw new Error('Electron API not available');
      }
      return await window.electronAPI.midi.setInputDevice(deviceId);
    } catch (error) {
      console.error('Failed to set input device via IPC:', error);
      return false;
    }
  }

  async setOutputDevice(deviceId: string): Promise<boolean> {
    try {
      if (!window.electronAPI) {
        throw new Error('Electron API not available');
      }
      return await window.electronAPI.midi.setOutputDevice(deviceId);
    } catch (error) {
      console.error('Failed to set output device via IPC:', error);
      return false;
    }
  }

  async getCurrentInputDevice(): Promise<string> {
    return '';
  }

  async getCurrentOutputDevice(): Promise<string> {
    return '';
  }

  onConnectionsChanged(callback: (connections: ClientConnectionModel[]) => void): void {
    this.connectionsCallbacks.push(callback);
  }

  onProfilesChanged(callback: (profiles: MidiCIProfileState[]) => void): void {
    this.profilesCallbacks.push(callback);
  }

  onPropertiesChanged(callback: (properties: PropertyValue[]) => void): void {
    this.propertiesCallbacks.push(callback);
  }

  onLogAdded(callback: (entry: LogEntry) => void): void {
    this.logCallbacks.push(callback);
  }

  private setupCallbacks(): void {
  }
}
