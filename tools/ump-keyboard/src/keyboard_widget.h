#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>
#include <QtCore/QSignalMapper>
#include <QtCore/QTimer>
#include <functional>
#include "midi_ci_manager.h"
#include "virtualized_control_list.h"
#include <midicci/details/commonproperties/StandardProperties.hpp>

class PianoKey;

class KeyboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeyboardWidget(QWidget* parent = nullptr);
    
    void setKeyPressedCallback(std::function<void(int)> callback);
    void setKeyReleasedCallback(std::function<void(int)> callback);
    void setDeviceRefreshCallback(std::function<void()> callback);
    
    // Control change callbacks
    void setControlChangeCallback(std::function<void(int,int,uint32_t)> callback); // channel, controller, value
    void setRPNCallback(std::function<void(int,int,int,uint32_t)> callback); // channel, msb, lsb, value
    void setNRPNCallback(std::function<void(int,int,int,uint32_t)> callback); // channel, msb, lsb, value
    void setPerNoteControlCallback(std::function<void(int,int,int,uint32_t)> callback); // channel, note, controller, value
    void setPerNoteAftertouchCallback(std::function<void(int,int,uint32_t)> callback); // channel, note, value
    void setProgramChangeCallback(std::function<void(int,uint8_t,uint8_t,uint8_t)> callback); // channel, program, bankMSB, bankLSB
    
    void updateMidiDevices(const std::vector<std::pair<std::string, std::string>>& inputDevices,
                          const std::vector<std::pair<std::string, std::string>>& outputDevices);
    
    // MIDI-CI UI methods
    void updateMidiCIStatus(bool initialized, uint32_t muid, const std::string& deviceName);
    void updateMidiCIDevices(const std::vector<MidiCIDeviceInfo>& discoveredDevices);
    void setMidiCIDiscoveryCallback(std::function<void()> callback);
    void setMidiCIDeviceProvider(std::function<MidiCIDeviceInfo*(uint32_t)> provider);
    void setPropertyRequesters(std::function<void(uint32_t)> requestCtrl,
                               std::function<void(uint32_t)> requestProg);
    
    // Property management - updated for simplified API
    void setPropertyDataProvider(std::function<std::optional<std::vector<midicci::commonproperties::MidiCIControl>>(uint32_t)> ctrlProvider,
                                std::function<std::optional<std::vector<midicci::commonproperties::MidiCIProgram>>(uint32_t)> progProvider);
    void setControlMapProvider(std::function<std::optional<std::vector<midicci::commonproperties::MidiCIControlMap>>(uint32_t, const std::string&)> provider);
    void updateProperties(uint32_t muid);
    void updatePropertiesOnMainThread(uint32_t muid);

signals:
    void midiInputDeviceChanged(const QString& deviceId);
    void midiOutputDeviceChanged(const QString& deviceId);

private slots:
    void onKeyPressed(int note);
    void onKeyReleased(int note);
    void onInputDeviceChanged(int index);
    void onOutputDeviceChanged(int index);
    void refreshDevices();
    void sendMidiCIDiscovery();
    void onMidiCIDeviceSelected(int index);
    void refreshProperties();
    void onRequestControlList();
    void onRequestProgramList();
    void onProgramSelected(int row);

public slots:
    void onPropertiesUpdated(uint32_t muid, const QString& propertyId);

private:
    void setupUI();
    void setupKeyboard();
    void setupDeviceSelectors();
    void setupMidiCIControls();
    void setupPropertiesPanel();
    QWidget* createKeyboardWidget();
    
    std::function<void(int)> keyPressedCallback;
    std::function<void(int)> keyReleasedCallback;
    std::function<void()> deviceRefreshCallback;
    std::function<void()> midiCIDiscoveryCallback;
    
    // Control change callbacks
    std::function<void(int,int,int)> controlChangeCallback;
    std::function<void(int,int,int,int)> rpnCallback;
    std::function<void(int,int,int,int)> nrpnCallback;
    std::function<void(int,int,int,int)> perNoteControlCallback;
    std::function<void(int,int,int)> perNoteAftertouchCallback;
    std::function<void(int,uint8_t,uint8_t,uint8_t)> programChangeCallback;
    std::function<MidiCIDeviceInfo*(uint32_t)> midiCIDeviceProvider;
    std::function<std::optional<std::vector<midicci::commonproperties::MidiCIControl>>(uint32_t)> ctrlListProvider;
    std::function<std::optional<std::vector<midicci::commonproperties::MidiCIProgram>>(uint32_t)> programListProvider;
    std::function<std::optional<std::vector<midicci::commonproperties::MidiCIControlMap>>(uint32_t, const std::string&)> controlMapProvider;
    
    QVBoxLayout* mainLayout;
    QWidget* keyboardWidget;
    QGroupBox* deviceGroup;
    QVBoxLayout* deviceLayout;
    QComboBox* inputDeviceCombo;
    QComboBox* outputDeviceCombo;
    QPushButton* refreshButton;
    QHBoxLayout* controlsLayout;
    QLabel* velocityLabel;
    QProgressBar* velocityBar;
    
    // MIDI-CI UI elements
    QGroupBox* midiCIGroup;
    QLabel* midiCIStatusLabel;
    QLabel* midiCIMuidLabel;
    QLabel* midiCIDeviceNameLabel;
    QPushButton* midiCIDiscoveryButton;
    QComboBox* midiCIDeviceCombo;
    QLabel* midiCISelectedDeviceInfo;
    
    // Properties UI elements
    QSplitter* mainSplitter;
    QGroupBox* propertiesGroup;
    QPushButton* refreshPropertiesButton;
    QPushButton* getControlListButton;
    QPushButton* getProgramListButton;
    VirtualizedControlList* controlListWidget;
    QListWidget* programListWidget;
    
    uint32_t selectedDeviceMuid;
    bool propertiesRequested;
    
    // Store current program list for reference when selected
    std::vector<midicci::commonproperties::MidiCIProgram> currentPrograms;
    
    QList<PianoKey*> whiteKeys;
    QList<PianoKey*> blackKeys;
    
    QSignalMapper* pressMapper;
    QSignalMapper* releaseMapper;

    // Explicit request callbacks
    std::function<void(uint32_t)> requestAllCtrlListCallback;
    std::function<void(uint32_t)> requestProgramListCallback;
};
