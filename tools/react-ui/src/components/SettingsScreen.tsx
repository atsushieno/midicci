import { useState, useEffect } from 'react';
import { useMidiCIBridge } from '../hooks/useMidiCIBridge';

interface SettingsScreenProps {
  isInitialized: boolean;
}

export function SettingsScreen({ isInitialized }: SettingsScreenProps) {
  const [configFile, setConfigFile] = useState('');
  const {
    availableDevices,
    selectedInputDevice,
    selectedOutputDevice,
    refreshDevices,
    selectInputDevice,
    selectOutputDevice
  } = useMidiCIBridge();

  useEffect(() => {
    if (isInitialized) {
      refreshDevices();
    }
  }, [isInitialized, refreshDevices]);

  const loadConfiguration = () => {
    console.log('Loading configuration from:', configFile);
  };

  const saveConfiguration = () => {
    console.log('Saving configuration to:', configFile);
  };

  const handleInputDeviceChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    selectInputDevice(e.target.value);
  };

  const handleOutputDeviceChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    selectOutputDevice(e.target.value);
  };

  const handleRefreshDevices = () => {
    refreshDevices();
  };

  return (
    <div className="space-y-6">
      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">MIDI Device Configuration</h2>
        
        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-2">
              MIDI Input Device
            </label>
            <select
              value={selectedInputDevice}
              onChange={handleInputDeviceChange}
              className="block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
            >
              <option value="">-- Select Input Device --</option>
              {availableDevices.inputs.map((device) => (
                <option key={device.id} value={device.id}>
                  {device.name}
                </option>
              ))}
            </select>
          </div>

          <div>
            <label className="block text-sm font-medium text-gray-700 mb-2">
              MIDI Output Device
            </label>
            <select
              value={selectedOutputDevice}
              onChange={handleOutputDeviceChange}
              className="block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
            >
              <option value="">-- Select Output Device --</option>
              {availableDevices.outputs.map((device) => (
                <option key={device.id} value={device.id}>
                  {device.name}
                </option>
              ))}
            </select>
          </div>

          <div className="flex space-x-4">
            <button
              onClick={handleRefreshDevices}
              disabled={!isInitialized}
              className="bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white px-4 py-2 rounded-md"
            >
              Refresh Devices
            </button>
            <button
              onClick={handleRefreshDevices}
              disabled={!isInitialized}
              className="bg-green-600 hover:bg-green-700 disabled:bg-gray-400 text-white px-4 py-2 rounded-md"
            >
              Apply Settings
            </button>
          </div>
        </div>
      </div>

      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">Configuration Management</h2>
        
        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-2">
              Configuration File Path
            </label>
            <input
              type="text"
              value={configFile}
              onChange={(e) => setConfigFile(e.target.value)}
              placeholder="/path/to/config.json"
              className="block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
            />
          </div>

          <div className="flex space-x-4">
            <button
              onClick={loadConfiguration}
              disabled={!isInitialized || !configFile}
              className="bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white px-4 py-2 rounded-md"
            >
              Load Configuration
            </button>
            <button
              onClick={saveConfiguration}
              disabled={!isInitialized || !configFile}
              className="bg-green-600 hover:bg-green-700 disabled:bg-gray-400 text-white px-4 py-2 rounded-md"
            >
              Save Configuration
            </button>
            <button
              disabled={!isInitialized}
              className="bg-gray-600 hover:bg-gray-700 disabled:bg-gray-400 text-white px-4 py-2 rounded-md"
            >
              Load Default
            </button>
          </div>
        </div>
      </div>

      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">Application Information</h2>
        
        <div className="space-y-3 text-sm">
          <div className="flex justify-between">
            <span className="font-medium">Application:</span>
            <span>MIDICCI React UI</span>
          </div>
          <div className="flex justify-between">
            <span className="font-medium">Version:</span>
            <span>1.0.0</span>
          </div>
          <div className="flex justify-between">
            <span className="font-medium">Bridge Status:</span>
            <span className={isInitialized ? 'text-green-600' : 'text-red-600'}>
              {isInitialized ? 'Connected' : 'Disconnected'}
            </span>
          </div>
          <div className="flex justify-between">
            <span className="font-medium">MIDI-CI Support:</span>
            <span className="text-green-600">Enabled</span>
          </div>
        </div>
      </div>

      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">Advanced Settings</h2>
        
        <div className="space-y-4">
          <div className="flex items-center">
            <input
              type="checkbox"
              id="verbose-logging"
              className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
            />
            <label htmlFor="verbose-logging" className="ml-2 block text-sm text-gray-900">
              Enable verbose logging
            </label>
          </div>
          
          <div className="flex items-center">
            <input
              type="checkbox"
              id="auto-discovery"
              className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
            />
            <label htmlFor="auto-discovery" className="ml-2 block text-sm text-gray-900">
              Auto-discover devices on startup
            </label>
          </div>
          
          <div className="flex items-center">
            <input
              type="checkbox"
              id="profile-notifications"
              className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
            />
            <label htmlFor="profile-notifications" className="ml-2 block text-sm text-gray-900">
              Show profile change notifications
            </label>
          </div>
        </div>
      </div>
    </div>
  );
}
