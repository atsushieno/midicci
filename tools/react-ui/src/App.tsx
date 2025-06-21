import { useState } from 'react'
import { InitiatorScreen } from './components/InitiatorScreen'
import { ResponderScreen } from './components/ResponderScreen'
import { LogScreen } from './components/LogScreen'
import { SettingsScreen } from './components/SettingsScreen'
import { useMidiCIBridge } from './hooks/useMidiCIBridge'

const tabs = [
  { id: 'initiator', label: 'Initiator' },
  { id: 'responder', label: 'Responder' },
  { id: 'logs', label: 'Logs' },
  { id: 'settings', label: 'Settings' },
]

function App() {
  const [selectedTab, setSelectedTab] = useState('initiator')
  const bridgeState = useMidiCIBridge()

  const renderTabContent = () => {
    switch (selectedTab) {
      case 'initiator':
        return <InitiatorScreen {...bridgeState} />
      case 'responder':
        return <ResponderScreen {...bridgeState} />
      case 'logs':
        return <LogScreen {...bridgeState} />
      case 'settings':
        return <SettingsScreen {...bridgeState} />
      default:
        return <InitiatorScreen {...bridgeState} />
    }
  }

  return (
    <div className="min-h-screen bg-gray-100">
      <div className="bg-white shadow-sm border-b">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex space-x-8">
            {tabs.map((tab) => (
              <button
                key={tab.id}
                onClick={() => setSelectedTab(tab.id)}
                className={`py-4 px-1 border-b-2 font-medium text-sm ${
                  selectedTab === tab.id
                    ? 'border-blue-500 text-blue-600'
                    : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
                }`}
              >
                {tab.label}
              </button>
            ))}
          </div>
        </div>
      </div>
      
      <main className="max-w-7xl mx-auto py-6 px-4 sm:px-6 lg:px-8">
        {renderTabContent()}
      </main>
    </div>
  )
}

export default App
