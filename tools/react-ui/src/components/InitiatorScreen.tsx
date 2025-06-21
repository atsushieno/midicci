import React, { useState } from 'react';
import { MidiCIProfileId, ClientConnectionModel } from '../types/midi';

interface InitiatorScreenProps {
  isInitialized: boolean;
  connections: ClientConnectionModel[];
  selectedConnectionMUID: number;
  selectedConnection?: ClientConnectionModel;
  setSelectedConnectionMUID: (muid: number) => void;
  sendDiscovery: () => Promise<void>;
  setProfile: (group: number, address: number, profile: MidiCIProfileId, enabled: boolean, numChannels: number) => Promise<void>;
  subscribeProperty: (propertyId: string, encoding?: string) => Promise<void>;
  unsubscribeProperty: (propertyId: string) => Promise<void>;
  refreshPropertyValue: (propertyId: string, encoding?: string, offset?: number, limit?: number) => Promise<void>;
}

export function InitiatorScreen({
  isInitialized,
  connections,
  selectedConnectionMUID,
  selectedConnection,
  setSelectedConnectionMUID,
  sendDiscovery,
  setProfile,
  subscribeProperty,
  unsubscribeProperty,
  refreshPropertyValue
}: InitiatorScreenProps) {
  const [selectedProfile, setSelectedProfile] = useState<MidiCIProfileId | null>(null);
  const [selectedProperty, setSelectedProperty] = useState<string>('');

  const formatMUID = (muid: number) => `0x${muid.toString(16).toUpperCase().padStart(8, '0')}`;

  const formatProfileId = (profile: MidiCIProfileId) => {
    return profile.bytes.map(b => b.toString(16).padStart(2, '0')).join(':');
  };

  const getAddressName = (address: number) => {
    switch (address) {
      case 0x7F: return 'Function Block';
      case 0x7E: return 'Group';
      default: return address.toString();
    }
  };

  return (
    <div className="space-y-6">
      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">Device Discovery</h2>
        <button
          onClick={sendDiscovery}
          disabled={!isInitialized}
          className="bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white px-4 py-2 rounded-md"
        >
          Send Discovery
        </button>
      </div>

      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">Connection Selection</h2>
        <select
          value={selectedConnectionMUID}
          onChange={(e) => setSelectedConnectionMUID(parseInt(e.target.value))}
          className="block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
        >
          <option value={0}>-- Select CI Device --</option>
          {connections.map((conn) => (
            <option key={conn.connection.targetMUID} value={conn.connection.targetMUID}>
              {formatMUID(conn.connection.targetMUID)} - {conn.connection.productInstanceId}
            </option>
          ))}
        </select>
      </div>

      {selectedConnection && (
        <>
          <div className="bg-white shadow rounded-lg p-6">
            <h2 className="text-lg font-medium text-gray-900 mb-4">Device Information</h2>
            <div className="grid grid-cols-2 gap-4 text-sm">
              <div>
                <span className="font-medium">Product Instance ID:</span> {selectedConnection.connection.productInstanceId}
              </div>
              <div>
                <span className="font-medium">MUID:</span> {formatMUID(selectedConnection.connection.targetMUID)}
              </div>
              <div>
                <span className="font-medium">Max Connections:</span> {selectedConnection.connection.maxSimultaneousPropertyRequests}
              </div>
              <div>
                <span className="font-medium">Manufacturer:</span> {selectedConnection.deviceInfo.manufacturer || `0x${selectedConnection.deviceInfo.manufacturerId.toString(16)}`}
              </div>
              <div>
                <span className="font-medium">Family:</span> {selectedConnection.deviceInfo.family || `0x${selectedConnection.deviceInfo.familyId.toString(16)}`}
              </div>
              <div>
                <span className="font-medium">Model:</span> {selectedConnection.deviceInfo.model || `0x${selectedConnection.deviceInfo.modelId.toString(16)}`}
              </div>
              <div>
                <span className="font-medium">Version:</span> {selectedConnection.deviceInfo.version || `0x${selectedConnection.deviceInfo.versionId.toString(16)}`}
              </div>
              <div>
                <span className="font-medium">Serial Number:</span> {selectedConnection.deviceInfo.serialNumber || 'N/A'}
              </div>
            </div>
          </div>

          <div className="bg-white shadow rounded-lg p-6">
            <h2 className="text-lg font-medium text-gray-900 mb-4">Profiles</h2>
            <div className="space-y-4">
              {selectedConnection.profiles.map((profile, index) => (
                <div key={index} className="border rounded-lg p-4">
                  <div className="flex items-center justify-between mb-2">
                    <span className="font-medium">{formatProfileId(profile.profile)}</span>
                    <button
                      onClick={() => setSelectedProfile(profile.profile)}
                      className={`px-3 py-1 rounded text-sm ${
                        selectedProfile && JSON.stringify(selectedProfile.bytes) === JSON.stringify(profile.profile.bytes)
                          ? 'bg-blue-100 text-blue-800'
                          : 'bg-gray-100 text-gray-700 hover:bg-gray-200'
                      }`}
                    >
                      Select
                    </button>
                  </div>
                  <div className="text-sm text-gray-600 space-y-1">
                    <div>Group: {profile.group}, Address: {getAddressName(profile.address)}</div>
                    <div className="flex items-center space-x-4">
                      <label className="flex items-center">
                        <input
                          type="checkbox"
                          checked={profile.enabled}
                          onChange={(e) => setProfile(profile.group, profile.address, profile.profile, e.target.checked, profile.numChannelsRequested)}
                          className="mr-2"
                        />
                        Enabled
                      </label>
                      <div className="flex items-center space-x-2">
                        <span>Channels:</span>
                        <input
                          type="number"
                          value={profile.numChannelsRequested}
                          onChange={(e) => setProfile(profile.group, profile.address, profile.profile, profile.enabled, parseInt(e.target.value) || 0)}
                          className="w-16 px-2 py-1 border border-gray-300 rounded text-sm"
                          min="0"
                          max="16"
                        />
                      </div>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>

          <div className="bg-white shadow rounded-lg p-6">
            <h2 className="text-lg font-medium text-gray-900 mb-4">Properties</h2>
            <div className="space-y-4">
              {selectedConnection.properties.map((property, index) => (
                <div key={index} className="border rounded-lg p-4">
                  <div className="flex items-center justify-between mb-2">
                    <span className="font-medium">{property.id}</span>
                    <button
                      onClick={() => setSelectedProperty(property.id)}
                      className={`px-3 py-1 rounded text-sm ${
                        selectedProperty === property.id
                          ? 'bg-blue-100 text-blue-800'
                          : 'bg-gray-100 text-gray-700 hover:bg-gray-200'
                      }`}
                    >
                      Select
                    </button>
                  </div>
                  <div className="text-sm text-gray-600 space-y-2">
                    <div>Media Type: {property.mediaType}</div>
                    <div className="flex space-x-2">
                      <button
                        onClick={() => refreshPropertyValue(property.id)}
                        className="bg-green-600 hover:bg-green-700 text-white px-3 py-1 rounded text-sm"
                      >
                        Refresh Value
                      </button>
                      <button
                        onClick={() => subscribeProperty(property.id)}
                        className="bg-blue-600 hover:bg-blue-700 text-white px-3 py-1 rounded text-sm"
                      >
                        Subscribe
                      </button>
                      <button
                        onClick={() => unsubscribeProperty(property.id)}
                        className="bg-red-600 hover:bg-red-700 text-white px-3 py-1 rounded text-sm"
                      >
                        Unsubscribe
                      </button>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        </>
      )}
    </div>
  );
}
