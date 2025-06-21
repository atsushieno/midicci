import React from 'react';
import { LogEntry, MessageDirection } from '../types/midi';

interface LogScreenProps {
  logs: LogEntry[];
  clearLogs: () => Promise<void>;
}

export function LogScreen({ logs, clearLogs }: LogScreenProps) {
  const formatTime = (timestamp: Date) => {
    return timestamp.toLocaleTimeString();
  };

  const getDirectionColor = (direction: MessageDirection) => {
    return direction === MessageDirection.In 
      ? 'text-blue-600 bg-blue-50' 
      : 'text-green-600 bg-green-50';
  };

  const getDirectionIcon = (direction: MessageDirection) => {
    return direction === MessageDirection.In ? '←' : '→';
  };

  return (
    <div className="space-y-6">
      <div className="bg-white shadow rounded-lg p-6">
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-lg font-medium text-gray-900">MIDI-CI Log Messages</h2>
          <button
            onClick={clearLogs}
            className="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-md"
          >
            Clear Logs
          </button>
        </div>

        <div className="space-y-2 max-h-96 overflow-y-auto">
          {logs.length === 0 ? (
            <div className="text-gray-500 text-center py-8">
              No log messages yet. Start by sending a discovery message.
            </div>
          ) : (
            logs.map((entry, index) => (
              <div key={index} className="border rounded-lg p-3">
                <div className="flex items-center justify-between mb-1">
                  <div className="flex items-center space-x-2">
                    <span className={`px-2 py-1 rounded text-xs font-medium ${getDirectionColor(entry.direction)}`}>
                      {getDirectionIcon(entry.direction)} {entry.direction}
                    </span>
                    <span className="text-xs text-gray-500">
                      {formatTime(entry.timestamp)}
                    </span>
                  </div>
                </div>
                <div className="text-sm text-gray-800 font-mono">
                  {entry.message}
                </div>
              </div>
            ))
          )}
        </div>
      </div>

      <div className="bg-white shadow rounded-lg p-6">
        <h2 className="text-lg font-medium text-gray-900 mb-4">Log Statistics</h2>
        <div className="grid grid-cols-3 gap-4 text-center">
          <div className="bg-gray-50 rounded-lg p-4">
            <div className="text-2xl font-bold text-gray-900">{logs.length}</div>
            <div className="text-sm text-gray-600">Total Messages</div>
          </div>
          <div className="bg-blue-50 rounded-lg p-4">
            <div className="text-2xl font-bold text-blue-600">
              {logs.filter(l => l.direction === MessageDirection.In).length}
            </div>
            <div className="text-sm text-gray-600">Incoming</div>
          </div>
          <div className="bg-green-50 rounded-lg p-4">
            <div className="text-2xl font-bold text-green-600">
              {logs.filter(l => l.direction === MessageDirection.Out).length}
            </div>
            <div className="text-sm text-gray-600">Outgoing</div>
          </div>
        </div>
      </div>
    </div>
  );
}
