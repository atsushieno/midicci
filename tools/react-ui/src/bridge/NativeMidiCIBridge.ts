import { 
  MidiCINativeBridge, 
  ClientConnectionModel, 
  MidiCIProfileState, 
  PropertyValue, 
  LogEntry, 
  MidiCIProfileId, 
  PropertyMetadata, 
  MessageDirection 
} from '../types/midi';

let nativeBridge: any = null;

try {
  nativeBridge = require('../../native/build/Release/midicci_bridge.node');
} catch (error) {
  console.warn('Failed to load native bridge:', error);
}

export class NativeMidiCIBridge implements MidiCINativeBridge {
  private repository: any;
  private deviceManager: any;
  private midiManager: any;
  
  private connectionsCallbacks: ((connections: ClientConnectionModel[]) => void)[] = [];
  private profilesCallbacks: ((profiles: MidiCIProfileState[]) => void)[] = [];
  private propertiesCallbacks: ((properties: PropertyValue[]) => void)[] = [];
  private logCallbacks: ((entry: LogEntry) => void)[] = [];

  constructor() {
    if (!nativeBridge) {
      throw new Error('Native bridge not available');
    }
    
    this.repository = new nativeBridge.CIToolRepository();
    this.deviceManager = null;
    this.midiManager = null;
  }

  async initialize(): Promise<void> {
    try {
      await this.repository.initialize();
      this.deviceManager = new nativeBridge.CIDeviceManager(this.repository);
      this.midiManager = new nativeBridge.MidiDeviceManager();
      
      if (this.midiManager) {
        await this.midiManager.initialize();
      }
      
      this.setupCallbacks();
    } catch (error) {
      throw new Error(`Failed to initialize native bridge: ${error}`);
    }
  }

  async shutdown(): Promise<void> {
    try {
      if (this.midiManager) {
        await this.midiManager.shutdown();
      }
      await this.repository.shutdown();
    } catch (error) {
      console.error('Failed to shutdown native bridge:', error);
    }
  }

  async sendDiscovery(): Promise<void> {
    try {
      await this.repository.sendDiscovery();
      setTimeout(() => {
        this.updateConnections();
      }, 100);
    } catch (error) {
      throw new Error(`Failed to send discovery: ${error}`);
    }
  }

  async getConnections(): Promise<ClientConnectionModel[]> {
    try {
      if (!this.deviceManager) {
        return [];
      }
      return await this.deviceManager.getConnections();
    } catch (error) {
      console.error('Failed to get connections:', error);
      return [];
    }
  }

  async setProfile(group: number, address: number, profile: MidiCIProfileId, enabled: boolean, numChannels: number): Promise<void> {
    try {
      if (!this.deviceManager) {
        throw new Error('Device manager not available');
      }
      await this.deviceManager.setProfile(group, address, profile, enabled, numChannels);
      setTimeout(() => {
        this.updateProfiles();
      }, 100);
    } catch (error) {
      throw new Error(`Failed to set profile: ${error}`);
    }
  }

  async subscribeProperty(propertyId: string, _encoding?: string): Promise<void> {
    try {
      if (!this.deviceManager) {
        throw new Error('Device manager not available');
      }
      await this.deviceManager.subscribeProperty(propertyId);
    } catch (error) {
      throw new Error(`Failed to subscribe property: ${error}`);
    }
  }

  async unsubscribeProperty(propertyId: string): Promise<void> {
    try {
      if (!this.deviceManager) {
        throw new Error('Device manager not available');
      }
      await this.deviceManager.unsubscribeProperty(propertyId);
    } catch (error) {
      throw new Error(`Failed to unsubscribe property: ${error}`);
    }
  }

  async refreshPropertyValue(propertyId: string, _encoding?: string, _offset?: number, _limit?: number): Promise<void> {
    try {
      if (!this.deviceManager) {
        throw new Error('Device manager not available');
      }
      await this.deviceManager.refreshPropertyValue(propertyId);
      setTimeout(() => {
        this.updateProperties();
      }, 100);
    } catch (error) {
      throw new Error(`Failed to refresh property value: ${error}`);
    }
  }

  async createProperty(metadata: PropertyMetadata): Promise<void> {
    try {
      if (!this.deviceManager) {
        throw new Error('Device manager not available');
      }
      await this.deviceManager.createProperty(metadata);
    } catch (error) {
      throw new Error(`Failed to create property: ${error}`);
    }
  }

  async updatePropertyMetadata(propertyId: string, metadata: PropertyMetadata): Promise<void> {
    try {
      if (!this.deviceManager) {
        throw new Error('Device manager not available');
      }
      await this.deviceManager.updatePropertyMetadata(propertyId, metadata);
    } catch (error) {
      throw new Error(`Failed to update property metadata: ${error}`);
    }
  }

  async updatePropertyValue(propertyId: string, _resId: string | undefined, data: Uint8Array): Promise<void> {
    try {
      if (!this.deviceManager) {
        throw new Error('Device manager not available');
      }
      await this.deviceManager.updatePropertyValue(propertyId, data);
    } catch (error) {
      throw new Error(`Failed to update property value: ${error}`);
    }
  }

  async removeProperty(propertyId: string): Promise<void> {
    try {
      if (!this.deviceManager) {
        throw new Error('Device manager not available');
      }
      await this.deviceManager.removeProperty(propertyId);
    } catch (error) {
      throw new Error(`Failed to remove property: ${error}`);
    }
  }

  async getLogs(): Promise<LogEntry[]> {
    try {
      const logs = await this.repository.getLogs();
      return logs.map((log: any) => ({
        timestamp: new Date(log.timestamp),
        direction: log.direction as MessageDirection,
        message: log.message
      }));
    } catch (error) {
      console.error('Failed to get logs:', error);
      return [];
    }
  }

  async clearLogs(): Promise<void> {
    try {
      await this.repository.clearLogs();
    } catch (error) {
      throw new Error(`Failed to clear logs: ${error}`);
    }
  }

  async getMUID(): Promise<number> {
    try {
      return await this.repository.getMUID();
    } catch (error) {
      console.error('Failed to get MUID:', error);
      return 0;
    }
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
    setInterval(() => {
      this.updateConnections();
      this.updateLogs();
    }, 1000);
  }

  private async updateConnections(): Promise<void> {
    try {
      const connections = await this.getConnections();
      this.connectionsCallbacks.forEach(cb => cb(connections));
      
      const allProfiles = connections.flatMap(conn => conn.profiles);
      this.profilesCallbacks.forEach(cb => cb(allProfiles));
    } catch (error) {
      console.error('Failed to update connections:', error);
    }
  }

  private async updateProfiles(): Promise<void> {
    try {
      const connections = await this.getConnections();
      const allProfiles = connections.flatMap(conn => conn.profiles);
      this.profilesCallbacks.forEach(cb => cb(allProfiles));
    } catch (error) {
      console.error('Failed to update profiles:', error);
    }
  }

  private async updateProperties(): Promise<void> {
    try {
      const connections = await this.getConnections();
      const allProperties = connections.flatMap(conn => conn.properties);
      this.propertiesCallbacks.forEach(cb => cb(allProperties));
    } catch (error) {
      console.error('Failed to update properties:', error);
    }
  }

  private async updateLogs(): Promise<void> {
    try {
      const logs = await this.getLogs();
      const lastLog = logs[logs.length - 1];
      if (lastLog) {
        this.logCallbacks.forEach(cb => cb(lastLog));
      }
    } catch (error) {
      console.error('Failed to update logs:', error);
    }
  }
}
