import { useState, useEffect, useCallback } from 'react';
import { 
  MidiCINativeBridge, 
  MockMidiCINativeBridge, 
  ClientConnectionModel, 
  MidiCIProfileState, 
  PropertyValue, 
  LogEntry,
  MidiCIProfileId,
  PropertyMetadata,
  MidiDevice
} from '../types/midi';
import { NativeMidiCIBridge } from '../bridge/NativeMidiCIBridge';
import { ElectronMidiCIBridge } from '../bridge/ElectronMidiCIBridge';

export function useMidiCIBridge() {
  const [bridge] = useState<MidiCINativeBridge>(() => {
    if (typeof window !== 'undefined' && window.electronAPI) {
      try {
        return new ElectronMidiCIBridge();
      } catch (error) {
        console.warn('Failed to load Electron bridge, falling back to mock:', error);
        return new MockMidiCINativeBridge();
      }
    }
    
    try {
      return new NativeMidiCIBridge();
    } catch (error) {
      console.warn('Failed to load native bridge, falling back to mock:', error);
      return new MockMidiCINativeBridge();
    }
  });
  const [isInitialized, setIsInitialized] = useState(false);
  const [connections, setConnections] = useState<ClientConnectionModel[]>([]);
  const [profiles, setProfiles] = useState<MidiCIProfileState[]>([]);
  const [properties, setProperties] = useState<PropertyValue[]>([]);
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [selectedConnectionMUID, setSelectedConnectionMUID] = useState<number>(0);
  const [availableDevices, setAvailableDevices] = useState<{ inputs: MidiDevice[], outputs: MidiDevice[] }>({ inputs: [], outputs: [] });
  const [selectedInputDevice, setSelectedInputDevice] = useState<string>('');
  const [selectedOutputDevice, setSelectedOutputDevice] = useState<string>('');

  useEffect(() => {
    const initializeBridge = async () => {
      try {
        await bridge.initialize();
        
        bridge.onConnectionsChanged(setConnections);
        bridge.onProfilesChanged(setProfiles);
        bridge.onPropertiesChanged(setProperties);
        bridge.onLogAdded((entry: LogEntry) => {
          setLogs((prev: LogEntry[]) => [...prev, entry]);
        });

        const initialLogs = await bridge.getLogs();
        setLogs(initialLogs);
        
        try {
          const devices = await bridge.getAvailableDevices();
          setAvailableDevices(devices);
        } catch (error) {
          console.error('Failed to load initial devices:', error);
        }
        
        setIsInitialized(true);
      } catch (error) {
        console.error('Failed to initialize MIDI-CI bridge:', error);
      }
    };

    initializeBridge();

    return () => {
      bridge.shutdown().catch(console.error);
    };
  }, [bridge]);

  const sendDiscovery = useCallback(async () => {
    if (!isInitialized) return;
    try {
      await bridge.sendDiscovery();
    } catch (error) {
      console.error('Failed to send discovery:', error);
    }
  }, [bridge, isInitialized]);

  const setProfile = useCallback(async (
    group: number, 
    address: number, 
    profile: MidiCIProfileId, 
    enabled: boolean, 
    numChannels: number
  ) => {
    if (!isInitialized) return;
    try {
      await bridge.setProfile(group, address, profile, enabled, numChannels);
    } catch (error) {
      console.error('Failed to set profile:', error);
    }
  }, [bridge, isInitialized]);

  const subscribeProperty = useCallback(async (propertyId: string, encoding?: string) => {
    if (!isInitialized) return;
    try {
      await bridge.subscribeProperty(propertyId, encoding);
    } catch (error) {
      console.error('Failed to subscribe to property:', error);
    }
  }, [bridge, isInitialized]);

  const unsubscribeProperty = useCallback(async (propertyId: string) => {
    if (!isInitialized) return;
    try {
      await bridge.unsubscribeProperty(propertyId);
    } catch (error) {
      console.error('Failed to unsubscribe from property:', error);
    }
  }, [bridge, isInitialized]);

  const refreshPropertyValue = useCallback(async (
    propertyId: string, 
    encoding?: string, 
    offset?: number, 
    limit?: number
  ) => {
    if (!isInitialized) return;
    try {
      await bridge.refreshPropertyValue(propertyId, encoding, offset, limit);
    } catch (error) {
      console.error('Failed to refresh property value:', error);
    }
  }, [bridge, isInitialized]);

  const clearLogs = useCallback(async () => {
    if (!isInitialized) return;
    try {
      await bridge.clearLogs();
      setLogs([]);
    } catch (error) {
      console.error('Failed to clear logs:', error);
    }
  }, [bridge, isInitialized]);

  const selectedConnection = connections.find((conn: ClientConnectionModel) => conn.connection.targetMUID === selectedConnectionMUID);

  async function createProperty(metadata: PropertyMetadata): Promise<void> {
    if (!bridge || !isInitialized) return;
    
    try {
      await bridge.createProperty(metadata);
    } catch (error) {
      console.error('Failed to create property:', error);
    }
  }

  async function updatePropertyMetadata(propertyId: string, metadata: PropertyMetadata): Promise<void> {
    if (!bridge || !isInitialized) return;
    
    try {
      await bridge.updatePropertyMetadata(propertyId, metadata);
    } catch (error) {
      console.error('Failed to update property metadata:', error);
    }
  }

  async function updatePropertyValue(propertyId: string, resId: string | undefined, data: Uint8Array): Promise<void> {
    if (!bridge || !isInitialized) return;
    
    try {
      await bridge.updatePropertyValue(propertyId, resId, data);
    } catch (error) {
      console.error('Failed to update property value:', error);
    }
  }

  async function removeProperty(propertyId: string): Promise<void> {
    if (!bridge || !isInitialized) return;
    
    try {
      await bridge.removeProperty(propertyId);
    } catch (error) {
      console.error('Failed to remove property:', error);
    }
  }

  const refreshDevices = useCallback(async () => {
    if (!isInitialized) return;
    try {
      const devices = await bridge.getAvailableDevices();
      setAvailableDevices(devices);
    } catch (error) {
      console.error('Failed to refresh devices:', error);
    }
  }, [bridge, isInitialized]);

  const selectInputDevice = useCallback(async (deviceId: string) => {
    if (!isInitialized) return;
    try {
      const success = await bridge.setInputDevice(deviceId);
      if (success) {
        setSelectedInputDevice(deviceId);
      }
    } catch (error) {
      console.error('Failed to select input device:', error);
    }
  }, [bridge, isInitialized]);

  const selectOutputDevice = useCallback(async (deviceId: string) => {
    if (!isInitialized) return;
    try {
      const success = await bridge.setOutputDevice(deviceId);
      if (success) {
        setSelectedOutputDevice(deviceId);
      }
    } catch (error) {
      console.error('Failed to select output device:', error);
    }
  }, [bridge, isInitialized]);

  return {
    isInitialized,
    connections,
    profiles,
    properties,
    logs,
    selectedConnectionMUID,
    selectedConnection,
    setSelectedConnectionMUID,
    sendDiscovery,
    setProfile,
    subscribeProperty,
    unsubscribeProperty,
    refreshPropertyValue,
    clearLogs,
    createProperty,
    updatePropertyMetadata,
    updatePropertyValue,
    removeProperty,
    availableDevices,
    selectedInputDevice,
    selectedOutputDevice,
    refreshDevices,
    selectInputDevice,
    selectOutputDevice
  };
}
