import { useState, useEffect, useCallback } from 'react';
import { 
  MidiCINativeBridge, 
  MockMidiCINativeBridge, 
  ClientConnectionModel, 
  MidiCIProfileState, 
  PropertyValue, 
  LogEntry,
  MidiCIProfileId
} from '../types/midi';

export function useMidiCIBridge() {
  const [bridge] = useState<MidiCINativeBridge>(() => new MockMidiCINativeBridge());
  const [isInitialized, setIsInitialized] = useState(false);
  const [connections, setConnections] = useState<ClientConnectionModel[]>([]);
  const [profiles, setProfiles] = useState<MidiCIProfileState[]>([]);
  const [properties, setProperties] = useState<PropertyValue[]>([]);
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [selectedConnectionMUID, setSelectedConnectionMUID] = useState<number>(0);

  useEffect(() => {
    const initializeBridge = async () => {
      try {
        await bridge.initialize();
        
        bridge.onConnectionsChanged(setConnections);
        bridge.onProfilesChanged(setProfiles);
        bridge.onPropertiesChanged(setProperties);
        bridge.onLogAdded((entry) => {
          setLogs(prev => [...prev, entry]);
        });

        const initialLogs = await bridge.getLogs();
        setLogs(initialLogs);
        
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

  const selectedConnection = connections.find(conn => conn.connection.targetMUID === selectedConnectionMUID);

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
    clearLogs
  };
}
