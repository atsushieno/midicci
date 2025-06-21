import React, { useState } from 'react';
import { MidiCIProfileId, MidiCIProfileState } from '../types/midi';

interface ResponderScreenProps {
  isInitialized: boolean;
  profiles: MidiCIProfileState[];
  setProfile: (group: number, address: number, profile: MidiCIProfileId, enabled: boolean, numChannels: number) => Promise<void>;
}

export function ResponderScreen({
  isInitialized,
  profiles,
  setProfile
}: ResponderScreenProps) {
  const [selectedProfile, setSelectedProfile] = useState<MidiCIProfileId | null>(null);
  const [newProfileId, setNewProfileId] = useState('7E:00:01:02:03');

  const formatProfileId = (profile: MidiCIProfileId) => {
    return profile.bytes.map(b => b.toString(16).padStart(2, '0')).join(':');
  };

  const parseProfileId = (idString: string): MidiCIProfileId | null => {
    try {
      const bytes = idString.split(':').map(s => parseInt(s, 16));
      if (bytes.length === 5 && bytes.every(b => !isNaN(b) && b >= 0 && b <= 255)) {
        return { bytes };
      }
    } catch {
    }
    return null;
  };

  const getAddressName = (address: number) => {
    switch (address) {
      case 0x7F: return 'Function Block';
      case 0x7E: return 'Group';
      default: return address.toString();
    }
  };

  const uniqueProfiles = profiles.reduce((acc, profile) => {
    const key = formatProfileId(profile.profile);
    if (!acc.find(p => formatProfileId(p.profile) === key)) {
      acc.push(profile);
    }
    return acc;
  }, [] as MidiCIProfileState[]);

  const addNewProfile = () => {
    const profileId = parseProfileId(newProfileId);
    if (profileId) {
      setProfile(0, 0x7F, profileId, false, 0);
      setNewProfileId('7E:00:01:02:03');
    }
  };

  const addTestProfiles = () => {
    const testProfiles = [
      { bytes: [0x7E, 0x00, 0x01, 0x00, 0x00] },
      { bytes: [0x7E, 0x00, 0x02, 0x00, 0x00] },
      { bytes: [0x7E, 0x00, 0x03, 0x00, 0x00] }
    ];
    
    testProfiles.forEach((profile, index) => {
      setProfile(0, 0x7F, profile, false, 0);
    });
  };

  return (
    <div className="space-y-6">
      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">Local Profile Configuration</h2>
        
        <div className="space-y-4">
          <div className="flex items-center space-x-4">
            <input
              type="text"
              value={newProfileId}
              onChange={(e) => setNewProfileId(e.target.value)}
              placeholder="7E:00:01:02:03"
              className="flex-1 px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
            />
            <button
              onClick={addNewProfile}
              disabled={!isInitialized || !parseProfileId(newProfileId)}
              className="bg-green-600 hover:bg-green-700 disabled:bg-gray-400 text-white px-4 py-2 rounded-md"
            >
              Add Profile
            </button>
            <button
              onClick={addTestProfiles}
              disabled={!isInitialized}
              className="bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white px-4 py-2 rounded-md"
            >
              Add Test Items
            </button>
          </div>

          <div className="space-y-3">
            {uniqueProfiles.map((profile, index) => (
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
              </div>
            ))}
          </div>
        </div>
      </div>

      {selectedProfile && (
        <div className="bg-white shadow rounded-lg p-6">
          <h2 className="text-lg font-medium text-gray-900 mb-4">Profile Details</h2>
          
          <div className="space-y-4">
            <div className="grid grid-cols-4 gap-4 text-sm font-medium text-gray-700 border-b pb-2">
              <div>Enabled</div>
              <div>Channel/Group</div>
              <div>Num Channels</div>
              <div>Actions</div>
            </div>

            {profiles
              .filter(p => JSON.stringify(p.profile.bytes) === JSON.stringify(selectedProfile.bytes))
              .map((profile, index) => (
                <div key={index} className="grid grid-cols-4 gap-4 items-center py-2 border-b">
                  <div>
                    <input
                      type="checkbox"
                      checked={profile.enabled}
                      onChange={(e) => setProfile(profile.group, profile.address, profile.profile, e.target.checked, profile.numChannelsRequested)}
                      className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
                    />
                  </div>
                  <div className="text-sm text-gray-600">
                    [{profile.group} / {getAddressName(profile.address)}]
                  </div>
                  <div>
                    <input
                      type="number"
                      value={profile.numChannelsRequested}
                      onChange={(e) => setProfile(profile.group, profile.address, profile.profile, profile.enabled, parseInt(e.target.value) || 0)}
                      className="w-16 px-2 py-1 border border-gray-300 rounded text-sm"
                      min="0"
                      max="16"
                    />
                  </div>
                  <div>
                    <button
                      onClick={() => {
                        console.log('Remove profile target:', profile);
                      }}
                      className="text-red-600 hover:text-red-800 text-sm"
                    >
                      Delete
                    </button>
                  </div>
                </div>
              ))}

            <div className="pt-4">
              <button
                onClick={() => {
                  setProfile(0, 0x7F, selectedProfile, false, 0);
                }}
                className="bg-green-600 hover:bg-green-700 text-white px-4 py-2 rounded-md text-sm"
              >
                Add Target
              </button>
            </div>
          </div>
        </div>
      )}

      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">Local Property Configuration</h2>
        <div className="text-gray-600">
          <p>Property configuration will be implemented in a future update.</p>
          <p className="mt-2">This section will allow you to:</p>
          <ul className="list-disc list-inside mt-2 space-y-1">
            <li>Create and manage local properties</li>
            <li>Set property metadata and values</li>
            <li>Handle property subscriptions from clients</li>
            <li>Configure property access permissions</li>
          </ul>
        </div>
      </div>
    </div>
  );
}
