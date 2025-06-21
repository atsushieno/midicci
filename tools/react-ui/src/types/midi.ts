export interface MidiCIProfileId {
  bytes: number[];
}

export interface MidiCIProfileState {
  group: number;
  address: number;
  profile: MidiCIProfileId;
  enabled: boolean;
  numChannelsRequested: number;
}

export interface PropertyValue {
  id: string;
  mediaType: string;
  body: Uint8Array;
}

export interface PropertyMetadata {
  propertyId: string;
  resource: string;
  canGet: boolean;
  canSet: PropertySetAccess;
  canSubscribe: boolean;
  requireResId: boolean;
  mediaTypes: string[];
  encodings: string[];
  schema?: string;
  canPaginate: boolean;
  columns: PropertyColumn[];
}

export interface PropertyColumn {
  title: string;
  link?: string;
  property?: string;
}

export enum PropertySetAccess {
  NONE = 'none',
  FULL = 'full',
  PARTIAL = 'partial'
}

export enum SubscriptionStateType {
  Subscribing = 'Subscribing',
  Subscribed = 'Subscribed',
  Unsubscribed = 'Unsubscribed'
}

export interface SubscriptionState {
  propertyId: string;
  state: SubscriptionStateType;
}

export interface ClientConnection {
  targetMUID: number;
  productInstanceId: string;
  maxSimultaneousPropertyRequests: number;
}

export interface MidiDevice {
  id: string;
  name: string;
  type: 'input' | 'output';
}

export interface DeviceInfo {
  manufacturer: string;
  manufacturerId: number;
  family: string;
  familyId: number;
  model: string;
  modelId: number;
  version: string;
  versionId: number;
  serialNumber?: string;
}

export interface ClientConnectionModel {
  connection: ClientConnection;
  profiles: MidiCIProfileState[];
  subscriptions: SubscriptionState[];
  properties: PropertyValue[];
  deviceInfo: DeviceInfo;
}

export enum MessageDirection {
  In = 'In',
  Out = 'Out'
}

export interface LogEntry {
  timestamp: Date;
  direction: MessageDirection;
  message: string;
}

export interface MidiCINativeBridge {
  initialize(): Promise<void>;
  shutdown(): Promise<void>;
  
  sendDiscovery(): Promise<void>;
  getConnections(): Promise<ClientConnectionModel[]>;
  
  setProfile(group: number, address: number, profile: MidiCIProfileId, enabled: boolean, numChannels: number): Promise<void>;
  subscribeProperty(propertyId: string, encoding?: string): Promise<void>;
  unsubscribeProperty(propertyId: string): Promise<void>;
  refreshPropertyValue(propertyId: string, encoding?: string, offset?: number, limit?: number): Promise<void>;
  createProperty(metadata: PropertyMetadata): Promise<void>;
  updatePropertyMetadata(propertyId: string, metadata: PropertyMetadata): Promise<void>;
  updatePropertyValue(propertyId: string, resId: string | undefined, data: Uint8Array): Promise<void>;
  removeProperty(propertyId: string): Promise<void>;
  
  getLogs(): Promise<LogEntry[]>;
  clearLogs(): Promise<void>;
  
  getMUID(): Promise<number>;
  
  getAvailableDevices(): Promise<{ inputs: MidiDevice[], outputs: MidiDevice[] }>;
  setInputDevice(deviceId: string): Promise<boolean>;
  setOutputDevice(deviceId: string): Promise<boolean>;
  getCurrentInputDevice(): Promise<string>;
  getCurrentOutputDevice(): Promise<string>;
  
  onConnectionsChanged(callback: (connections: ClientConnectionModel[]) => void): void;
  onProfilesChanged(callback: (profiles: MidiCIProfileState[]) => void): void;
  onPropertiesChanged(callback: (properties: PropertyValue[]) => void): void;
  onLogAdded(callback: (entry: LogEntry) => void): void;
}

export class MockMidiCINativeBridge implements MidiCINativeBridge {
  private connections: ClientConnectionModel[] = [];
  private logs: LogEntry[] = [];
  private muid: number = 0x12345678;
  
  private connectionsCallbacks: ((connections: ClientConnectionModel[]) => void)[] = [];
  private profilesCallbacks: ((profiles: MidiCIProfileState[]) => void)[] = [];
  private propertiesCallbacks: ((properties: PropertyValue[]) => void)[] = [];
  private logCallbacks: ((entry: LogEntry) => void)[] = [];

  async initialize(): Promise<void> {
    this.addLog('MIDI-CI Bridge initialized', MessageDirection.Out);
  }

  async shutdown(): Promise<void> {
    this.addLog('MIDI-CI Bridge shutdown', MessageDirection.Out);
  }

  async sendDiscovery(): Promise<void> {
    this.addLog('Discovery inquiry sent', MessageDirection.Out);
    
    setTimeout(() => {
      const mockConnection: ClientConnectionModel = {
        connection: {
          targetMUID: 0x87654321,
          productInstanceId: 'Mock Device',
          maxSimultaneousPropertyRequests: 4
        },
        profiles: [
          {
            group: 0,
            address: 0x7F,
            profile: { bytes: [0x7E, 0x00, 0x01, 0x02, 0x03] },
            enabled: false,
            numChannelsRequested: 0
          }
        ],
        subscriptions: [],
        properties: [
          {
            id: 'mock.property.1',
            mediaType: 'application/json',
            body: new Uint8Array([123, 34, 118, 97, 108, 117, 101, 34, 58, 49, 50, 51, 125])
          }
        ],
        deviceInfo: {
          manufacturer: 'Mock Manufacturer',
          manufacturerId: 0x7D,
          family: 'Mock Family',
          familyId: 0x1234,
          model: 'Mock Model',
          modelId: 0x5678,
          version: '1.0.0',
          versionId: 0x0100,
          serialNumber: 'MOCK001'
        }
      };
      
      this.connections = [mockConnection];
      this.connectionsCallbacks.forEach(cb => cb(this.connections));
      this.addLog('Discovery response received', MessageDirection.In);
    }, 1000);
  }

  async getConnections(): Promise<ClientConnectionModel[]> {
    return this.connections;
  }

  async setProfile(group: number, address: number, profile: MidiCIProfileId, enabled: boolean, numChannels: number): Promise<void> {
    this.addLog(`Profile ${enabled ? 'enabled' : 'disabled'}: Group ${group}, Address ${address}`, MessageDirection.Out);
    
    this.connections.forEach(conn => {
      const profileState = conn.profiles.find(p => 
        p.group === group && p.address === address && 
        JSON.stringify(p.profile.bytes) === JSON.stringify(profile.bytes)
      );
      if (profileState) {
        profileState.enabled = enabled;
        profileState.numChannelsRequested = numChannels;
      }
    });
    
    const allProfiles = this.connections.flatMap(conn => conn.profiles);
    this.profilesCallbacks.forEach(cb => cb(allProfiles));
  }

  async subscribeProperty(propertyId: string, encoding?: string): Promise<void> {
    this.addLog(`Subscribing to property: ${propertyId}${encoding ? ` (${encoding})` : ''}`, MessageDirection.Out);
    
    this.connections.forEach(conn => {
      const existing = conn.subscriptions.find(s => s.propertyId === propertyId);
      if (existing) {
        existing.state = SubscriptionStateType.Subscribed;
      } else {
        conn.subscriptions.push({
          propertyId,
          state: SubscriptionStateType.Subscribed
        });
      }
    });
  }

  async unsubscribeProperty(propertyId: string): Promise<void> {
    this.addLog(`Unsubscribing from property: ${propertyId}`, MessageDirection.Out);
    
    this.connections.forEach(conn => {
      const existing = conn.subscriptions.find(s => s.propertyId === propertyId);
      if (existing) {
        existing.state = SubscriptionStateType.Unsubscribed;
      }
    });
  }

  async refreshPropertyValue(propertyId: string, _encoding?: string, _offset?: number, _limit?: number): Promise<void> {
    this.addLog(`Refreshing property value: ${propertyId}`, MessageDirection.Out);
    
    setTimeout(() => {
      this.addLog(`Property value refreshed: ${propertyId}`, MessageDirection.In);
      const allProperties = this.connections.flatMap(conn => conn.properties);
      this.propertiesCallbacks.forEach(cb => cb(allProperties));
    }, 500);
  }

  async createProperty(metadata: PropertyMetadata): Promise<void> {
    this.addLog(`Creating property: ${metadata.propertyId}`, MessageDirection.Out);
    
    setTimeout(() => {
      this.addLog(`Property created: ${metadata.propertyId}`, MessageDirection.In);
    }, 500);
  }

  async updatePropertyMetadata(propertyId: string, _metadata: PropertyMetadata): Promise<void> {
    this.addLog(`Updating property metadata: ${propertyId}`, MessageDirection.Out);
    
    setTimeout(() => {
      this.addLog(`Property metadata updated: ${propertyId}`, MessageDirection.In);
    }, 500);
  }

  async updatePropertyValue(propertyId: string, resId: string | undefined, data: Uint8Array): Promise<void> {
    this.addLog(`Updating property value: ${propertyId}${resId ? ` (resId: ${resId})` : ''}`, MessageDirection.Out);
    
    setTimeout(() => {
      this.addLog(`Property value updated: ${propertyId} (${data.length} bytes)`, MessageDirection.In);
    }, 500);
  }

  async removeProperty(propertyId: string): Promise<void> {
    this.addLog(`Removing property: ${propertyId}`, MessageDirection.Out);
    
    setTimeout(() => {
      this.addLog(`Property removed: ${propertyId}`, MessageDirection.In);
    }, 500);
  }

  async getLogs(): Promise<LogEntry[]> {
    return this.logs;
  }

  async clearLogs(): Promise<void> {
    this.logs = [];
    this.addLog('Logs cleared', MessageDirection.Out);
  }

  async getMUID(): Promise<number> {
    return this.muid;
  }

  async getAvailableDevices(): Promise<{ inputs: MidiDevice[], outputs: MidiDevice[] }> {
    return { inputs: [], outputs: [] };
  }

  async setInputDevice(deviceId: string): Promise<boolean> {
    this.addLog(`Setting input device: ${deviceId}`, MessageDirection.Out);
    return false;
  }

  async setOutputDevice(deviceId: string): Promise<boolean> {
    this.addLog(`Setting output device: ${deviceId}`, MessageDirection.Out);
    return false;
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

  private addLog(message: string, direction: MessageDirection): void {
    const entry: LogEntry = {
      timestamp: new Date(),
      direction,
      message
    };
    this.logs.push(entry);
    this.logCallbacks.forEach(cb => cb(entry));
  }
}
