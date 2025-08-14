#include "keyboard_widget.h"
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QSlider>
#include <QtCore/QThread>
#include <QtCore/QMetaObject>
#include <iostream>

class PianoKey : public QPushButton {
    Q_OBJECT
public:
    PianoKey(int note, bool isBlack = false, QWidget* parent = nullptr) 
        : QPushButton(parent), noteValue(note), isBlackKey(isBlack) {
        setupKey();
    }
    
    int getNote() const { return noteValue; }
    bool isBlack() const { return isBlackKey; }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        QPushButton::mousePressEvent(event);
        emit keyPressed(noteValue);
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        QPushButton::mouseReleaseEvent(event);
        emit keyReleased(noteValue);
    }

signals:
    void keyPressed(int note);
    void keyReleased(int note);

private:
    void setupKey() {
        if (isBlackKey) {
            setFixedSize(30, 80);
            setStyleSheet(
                "QPushButton {"
                "  background-color: #1a1a1a;"
                "  border: 1px solid #333;"
                "  border-radius: 4px;"
                "  color: white;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #404040;"
                "}"
            );
        } else {
            setFixedSize(50, 120);
            setStyleSheet(
                "QPushButton {"
                "  background-color: white;"
                "  border: 1px solid #333;"
                "  border-radius: 4px;"
                "  color: black;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #f0f0f0;"
                "}"
            );
        }
    }
    
    int noteValue;
    bool isBlackKey;
};

KeyboardWidget::KeyboardWidget(QWidget* parent) 
    : QWidget(parent), pressMapper(new QSignalMapper(this)), releaseMapper(new QSignalMapper(this)), 
      selectedDeviceMuid(0) {
    setupUI();
    
    connect(pressMapper, &QSignalMapper::mappedInt, this, &KeyboardWidget::onKeyPressed);
    connect(releaseMapper, &QSignalMapper::mappedInt, this, &KeyboardWidget::onKeyReleased);
    
    // Timer-based updates removed - using event-driven updates instead
}

void KeyboardWidget::setupUI() {
    // Make window resizable instead of fixed size
    setMinimumSize(900, 600);
    resize(1200, 800);
    setWindowTitle("MIDICCI UMP Keyboard");
    
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Title
    titleLabel = new QLabel("MIDI 2.0 Virtual Keyboard");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);
    
    // Create main splitter for resizable layout
    mainSplitter = new QSplitter(Qt::Vertical, this);
    
    // Top section with devices and controls
    QWidget* topSection = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout(topSection);
    topLayout->setContentsMargins(0, 0, 0, 0);
    
    // Device selectors
    setupDeviceSelectors();
    topLayout->addWidget(deviceGroup);
    
    // MIDI-CI controls
    setupMidiCIControls();
    topLayout->addWidget(midiCIGroup);
    
    // Keyboard
    setupKeyboard();
    topLayout->addWidget(keyboardWidget);
    
    mainSplitter->addWidget(topSection);
    
    // Properties panel
    setupPropertiesPanel();
    mainSplitter->addWidget(propertiesGroup);
    
    // Set initial splitter sizes (top section larger)
    mainSplitter->setSizes({400, 200});
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(mainSplitter);
    
    // Controls
    controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(10);
    
    velocityLabel = new QLabel("Velocity:");
    velocityLabel->setAlignment(Qt::AlignVCenter);
    controlsLayout->addWidget(velocityLabel);
    
    velocityBar = new QProgressBar();
    velocityBar->setFixedSize(200, 20);
    velocityBar->setValue(80);
    velocityBar->setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid gray;"
        "  background-color: lightgray;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #4CAF50;"
        "}"
    );
    controlsLayout->addWidget(velocityBar);
    
    controlsLayout->addStretch();
    mainLayout->addLayout(controlsLayout);
}

void KeyboardWidget::setupDeviceSelectors() {
    deviceGroup = new QGroupBox("MIDI 2.0 Devices");
    deviceLayout = new QHBoxLayout(deviceGroup);
    
    // Input device selector
    QLabel* inputLabel = new QLabel("Input:");
    inputDeviceCombo = new QComboBox();
    inputDeviceCombo->setMinimumWidth(200);
    connect(inputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KeyboardWidget::onInputDeviceChanged);
    
    // Output device selector
    QLabel* outputLabel = new QLabel("Output:");
    outputDeviceCombo = new QComboBox();
    outputDeviceCombo->setMinimumWidth(200);
    connect(outputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KeyboardWidget::onOutputDeviceChanged);
    
    // Refresh button
    refreshButton = new QPushButton("Refresh");
    refreshButton->setMaximumWidth(80);
    connect(refreshButton, &QPushButton::clicked, this, &KeyboardWidget::refreshDevices);
    
    deviceLayout->addWidget(inputLabel);
    deviceLayout->addWidget(inputDeviceCombo);
    deviceLayout->addSpacing(20);
    deviceLayout->addWidget(outputLabel);
    deviceLayout->addWidget(outputDeviceCombo);
    deviceLayout->addWidget(refreshButton);
    deviceLayout->addStretch();
    
    mainLayout->addWidget(deviceGroup);
}

void KeyboardWidget::setupKeyboard() {
    keyboardWidget = createKeyboardWidget();
    mainLayout->addWidget(keyboardWidget);
}

QWidget* KeyboardWidget::createKeyboardWidget() {
    QWidget* container = new QWidget();
    container->setFixedSize(850, 140);
    
    // Create all keys
    struct KeyInfo {
        int note;
        bool isBlack;
        QString label;
        int whiteKeyIndex; // For positioning black keys
    };
    
    // Define two octaves of keys (C4 to B5)
    std::vector<KeyInfo> keyInfos = {
        {60, false, "C", 0},   // C4
        {61, true, "", 0},     // C#4 (between C and D)
        {62, false, "D", 1},   // D4
        {63, true, "", 1},     // D#4 (between D and E)
        {64, false, "E", 2},   // E4
        {65, false, "F", 3},   // F4
        {66, true, "", 3},     // F#4 (between F and G)
        {67, false, "G", 4},   // G4
        {68, true, "", 4},     // G#4 (between G and A)
        {69, false, "A", 5},   // A4
        {70, true, "", 5},     // A#4 (between A and B)
        {71, false, "B", 6},   // B4
        {72, false, "C", 7},   // C5
        {73, true, "", 7},     // C#5
        {74, false, "D", 8},   // D5
        {75, true, "", 8},     // D#5
        {76, false, "E", 9},   // E5
        {77, false, "F", 10},  // F5
        {78, true, "", 10},    // F#5
        {79, false, "G", 11},  // G5
        {80, true, "", 11},    // G#5
        {81, false, "A", 12},  // A5
        {82, true, "", 12},    // A#5
        {83, false, "B", 13},  // B5
    };
    
    // Create white keys first
    for (const auto& keyInfo : keyInfos) {
        if (!keyInfo.isBlack) {
            PianoKey* key = new PianoKey(keyInfo.note, false, container);
            key->setText(keyInfo.label);
            key->move(keyInfo.whiteKeyIndex * 52, 20);
            
            connect(key, &PianoKey::keyPressed, pressMapper, QOverload<>::of(&QSignalMapper::map));
            connect(key, &PianoKey::keyReleased, releaseMapper, QOverload<>::of(&QSignalMapper::map));
            pressMapper->setMapping(key, keyInfo.note);
            releaseMapper->setMapping(key, keyInfo.note);
            
            whiteKeys.append(key);
        }
    }
    
    // Create black keys on top
    for (const auto& keyInfo : keyInfos) {
        if (keyInfo.isBlack) {
            PianoKey* key = new PianoKey(keyInfo.note, true, container);
            
            // Position black keys between white keys
            int xPos = keyInfo.whiteKeyIndex * 52 + 37; // 52 is white key width + spacing, 37 centers the black key
            key->move(xPos, 20);
            key->raise(); // Bring black keys to front
            
            connect(key, &PianoKey::keyPressed, pressMapper, QOverload<>::of(&QSignalMapper::map));
            connect(key, &PianoKey::keyReleased, releaseMapper, QOverload<>::of(&QSignalMapper::map));
            pressMapper->setMapping(key, keyInfo.note);
            releaseMapper->setMapping(key, keyInfo.note);
            
            blackKeys.append(key);
        }
    }
    
    return container;
}

void KeyboardWidget::setKeyPressedCallback(std::function<void(int)> callback) {
    keyPressedCallback = callback;
}

void KeyboardWidget::setKeyReleasedCallback(std::function<void(int)> callback) {
    keyReleasedCallback = callback;
}

void KeyboardWidget::setDeviceRefreshCallback(std::function<void()> callback) {
    deviceRefreshCallback = callback;
}

void KeyboardWidget::setControlChangeCallback(std::function<void(int,int,uint32_t)> callback) {
    controlChangeCallback = callback;
}

void KeyboardWidget::setRPNCallback(std::function<void(int,int,int,uint32_t)> callback) {
    rpnCallback = callback;
}

void KeyboardWidget::setNRPNCallback(std::function<void(int,int,int,uint32_t)> callback) {
    nrpnCallback = callback;
}

void KeyboardWidget::setPerNoteControlCallback(std::function<void(int,int,int,uint32_t)> callback) {
    perNoteControlCallback = callback;
}

void KeyboardWidget::setPerNoteAftertouchCallback(std::function<void(int,int,uint32_t)> callback) {
    perNoteAftertouchCallback = callback;
}

void KeyboardWidget::updateMidiDevices(
    const std::vector<std::pair<std::string, std::string>>& inputDevices,
    const std::vector<std::pair<std::string, std::string>>& outputDevices) {
    
    // Update input devices
    inputDeviceCombo->clear();
    inputDeviceCombo->addItem("No Input Device", "");
    for (const auto& device : inputDevices) {
        inputDeviceCombo->addItem(QString::fromStdString(device.second), 
                                QString::fromStdString(device.first));
    }
    
    // Update output devices
    outputDeviceCombo->clear();
    outputDeviceCombo->addItem("No Output Device", "");
    for (const auto& device : outputDevices) {
        outputDeviceCombo->addItem(QString::fromStdString(device.second), 
                                 QString::fromStdString(device.first));
    }
}

void KeyboardWidget::onKeyPressed(int note) {
    if (keyPressedCallback) {
        keyPressedCallback(note);
    }
}

void KeyboardWidget::onKeyReleased(int note) {
    if (keyReleasedCallback) {
        keyReleasedCallback(note);
    }
}

void KeyboardWidget::onInputDeviceChanged(int index) {
    if (index >= 0) {
        QString deviceId = inputDeviceCombo->itemData(index).toString();
        emit midiInputDeviceChanged(deviceId);
    }
}

void KeyboardWidget::onOutputDeviceChanged(int index) {
    if (index >= 0) {
        QString deviceId = outputDeviceCombo->itemData(index).toString();
        emit midiOutputDeviceChanged(deviceId);
    }
}

void KeyboardWidget::refreshDevices() {
    if (deviceRefreshCallback) {
        deviceRefreshCallback();
    }
}

void KeyboardWidget::setupMidiCIControls() {
    midiCIGroup = new QGroupBox("MIDI-CI Status");
    midiCIGroup->setMaximumHeight(200); // Slightly taller for combobox
    QVBoxLayout* midiCILayout = new QVBoxLayout(midiCIGroup);
    midiCILayout->setSpacing(5); // Reduce spacing
    
    // Top row: Status and MUID in one line
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Status:"));
    midiCIStatusLabel = new QLabel("Not Initialized");
    midiCIStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    topLayout->addWidget(midiCIStatusLabel);
    topLayout->addSpacing(20);
    topLayout->addWidget(new QLabel("MUID:"));
    midiCIMuidLabel = new QLabel("N/A");
    topLayout->addWidget(midiCIMuidLabel);
    topLayout->addStretch();
    midiCILayout->addLayout(topLayout);
    
    // Device name row
    QHBoxLayout* deviceNameLayout = new QHBoxLayout();
    deviceNameLayout->addWidget(new QLabel("Device:"));
    midiCIDeviceNameLabel = new QLabel("N/A");
    deviceNameLayout->addWidget(midiCIDeviceNameLabel);
    deviceNameLayout->addStretch();
    midiCILayout->addLayout(deviceNameLayout);
    
    // Discovery button
    QHBoxLayout* discoveryLayout = new QHBoxLayout();
    midiCIDiscoveryButton = new QPushButton("Send Discovery");
    midiCIDiscoveryButton->setEnabled(false);
    midiCIDiscoveryButton->setMaximumWidth(120);
    connect(midiCIDiscoveryButton, &QPushButton::clicked, this, &KeyboardWidget::sendMidiCIDiscovery);
    discoveryLayout->addWidget(midiCIDiscoveryButton);
    discoveryLayout->addStretch();
    midiCILayout->addLayout(discoveryLayout);
    
    // Device selection combobox
    QHBoxLayout* deviceSelectionLayout = new QHBoxLayout();
    deviceSelectionLayout->addWidget(new QLabel("Select Device:"));
    midiCIDeviceCombo = new QComboBox();
    midiCIDeviceCombo->setEnabled(false);
    midiCIDeviceCombo->addItem("No devices discovered");
    connect(midiCIDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &KeyboardWidget::onMidiCIDeviceSelected);
    deviceSelectionLayout->addWidget(midiCIDeviceCombo, 1);
    midiCILayout->addLayout(deviceSelectionLayout);
    
    // Selected device detailed info
    midiCISelectedDeviceInfo = new QLabel("Select a MIDI-CI device to view details");
    midiCISelectedDeviceInfo->setWordWrap(true);
    midiCISelectedDeviceInfo->setMaximumHeight(50);
    midiCISelectedDeviceInfo->setStyleSheet("font-size: 11px; background-color: #f5f5f5; padding: 4px; border: 1px solid #ccc;");
    midiCILayout->addWidget(midiCISelectedDeviceInfo);
    
    mainLayout->addWidget(midiCIGroup);
}

void KeyboardWidget::setupPropertiesPanel() {
    propertiesGroup = new QGroupBox("MIDI-CI Properties");
    QVBoxLayout* propertiesLayout = new QVBoxLayout(propertiesGroup);
    propertiesLayout->setSpacing(10);
    
    // Header with refresh button
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* headerLabel = new QLabel("Standard Properties");
    headerLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    
    refreshPropertiesButton = new QPushButton("Refresh Properties");
    refreshPropertiesButton->setEnabled(false);
    refreshPropertiesButton->setMaximumWidth(150);
    refreshPropertiesButton->setToolTip("Click to request properties again (forces new requests)");
    connect(refreshPropertiesButton, &QPushButton::clicked, this, &KeyboardWidget::refreshProperties);
    headerLayout->addWidget(refreshPropertiesButton);
    propertiesLayout->addLayout(headerLayout);
    
    // Create horizontal layout for the two property lists
    QHBoxLayout* listsLayout = new QHBoxLayout();
    
    // Control List section
    QVBoxLayout* controlLayout = new QVBoxLayout();
    QLabel* controlLabel = new QLabel("All Controls");
    controlLabel->setStyleSheet("font-weight: bold;");
    controlLayout->addWidget(controlLabel);
    
    controlListWidget = new VirtualizedControlList();
    controlListWidget->setMinimumHeight(150);
    controlListWidget->setEnabled(false);
    controlLayout->addWidget(controlListWidget);
    
    // Connect control value changes to MIDI sending
    controlListWidget->setValueChangeCallback([this](int controlIndex, const midicci::commonproperties::MidiCIControl& control, uint32_t value) {
        QString ctrlType = QString::fromStdString(control.ctrlType);
        int channel = control.channel.value_or(0);
        
        if (ctrlType == "cc" && controlChangeCallback) {
            uint8_t ccNum = control.ctrlIndex.empty() ? 0 : control.ctrlIndex[0];
            controlChangeCallback(channel, ccNum, value);
        } else if (ctrlType == "rpn" && rpnCallback && control.ctrlIndex.size() >= 2) {
            rpnCallback(channel, control.ctrlIndex[0], control.ctrlIndex[1], value);
        } else if (ctrlType == "nrpn" && nrpnCallback && control.ctrlIndex.size() >= 2) {
            nrpnCallback(channel, control.ctrlIndex[0], control.ctrlIndex[1], value);
        } else if ((ctrlType == "pnrc" || ctrlType == "pnac") && perNoteControlCallback) {
            // For per-note controls, we'll use middle C (60) as default note
            // In a real implementation, you might want a separate note selector
            int note = 60;
            uint8_t ctrlNum = control.ctrlIndex.empty() ? 0 : control.ctrlIndex[0];
            if (ctrlType == "pnrc") {
                perNoteControlCallback(channel, note, ctrlNum, value);
            } else if (ctrlType == "pnac" && perNoteAftertouchCallback) {
                perNoteAftertouchCallback(channel, note, value);
            }
        }
    });
    
    listsLayout->addLayout(controlLayout);
    
    // Program List section
    QVBoxLayout* programLayout = new QVBoxLayout();
    QLabel* programLabel = new QLabel("Programs");
    programLabel->setStyleSheet("font-weight: bold;");
    programLayout->addWidget(programLabel);
    
    programListWidget = new QListWidget();
    programListWidget->setMinimumHeight(150);
    programListWidget->addItem("No device selected");
    programListWidget->setEnabled(false);
    programLayout->addWidget(programListWidget);
    listsLayout->addLayout(programLayout);
    
    propertiesLayout->addLayout(listsLayout);
}

void KeyboardWidget::updateMidiCIStatus(bool initialized, uint32_t muid, const std::string& deviceName) {
    if (initialized) {
        midiCIStatusLabel->setText("Initialized");
        midiCIStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        midiCIMuidLabel->setText(QString("0x%1 (%2)").arg(CIFactory::midiCI32to28(muid), 0, 16).arg(muid));
        midiCIDeviceNameLabel->setText(QString::fromStdString(deviceName));
        midiCIDiscoveryButton->setEnabled(true);
    } else {
        midiCIStatusLabel->setText("Not Initialized");
        midiCIStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        midiCIMuidLabel->setText("N/A");
        midiCIDeviceNameLabel->setText("N/A");
        midiCIDiscoveryButton->setEnabled(false);
    }
}

void KeyboardWidget::updateMidiCIDevices(const std::vector<MidiCIDeviceInfo>& discoveredDevices) {
    // Check if the currently selected device is still available
    bool selectedDeviceStillExists = false;
    uint32_t previousSelectedMuid = selectedDeviceMuid;
    
    // Clear and repopulate the combobox
    midiCIDeviceCombo->clear();
    
    // Filter to only show endpoint-ready devices
    std::vector<MidiCIDeviceInfo> readyDevices;
    for (const auto& device : discoveredDevices) {
        if (device.endpoint_ready) {
            readyDevices.push_back(device);
            if (device.muid == selectedDeviceMuid) {
                selectedDeviceStillExists = true;
            }
        }
    }
    
    if (readyDevices.empty()) {
        midiCIDeviceCombo->addItem("No devices ready");
        midiCIDeviceCombo->setEnabled(false);
        midiCISelectedDeviceInfo->setText("MIDI-CI devices discovered but not ready. Waiting for endpoint information...");
        
        // Reset selected device since no devices are available
        if (selectedDeviceMuid != 0) {
            std::cout << "[UI] Clearing selected device - no ready devices available" << std::endl;
            selectedDeviceMuid = 0;
            refreshPropertiesButton->setEnabled(false);
            controlListWidget->setControls({});
            controlListWidget->setEnabled(false);
            programListWidget->clear();
            programListWidget->addItem("No device selected");
        }
    } else {
        midiCIDeviceCombo->setEnabled(true);
        int selectedIndex = -1;
        
        for (size_t i = 0; i < readyDevices.size(); ++i) {
            const auto& device = readyDevices[i];
            QString displayName = QString::fromStdString(device.getDisplayName());
            midiCIDeviceCombo->addItem(displayName, device.muid);
            
            // Check if this is the previously selected device
            if (device.muid == previousSelectedMuid) {
                selectedIndex = static_cast<int>(i);
            }
        }
        
        // Handle device selection logic
        if (!selectedDeviceStillExists && selectedDeviceMuid != 0) {
            // Previously selected device is no longer available
            std::cout << "[UI] Previously selected device (MUID: 0x" << std::hex << selectedDeviceMuid << std::dec 
                      << ") is no longer available, clearing selection" << std::endl;
            selectedDeviceMuid = 0;
            refreshPropertiesButton->setEnabled(false);
            controlListWidget->setControls({});
            controlListWidget->setEnabled(false);
            programListWidget->clear();
            programListWidget->addItem("No device selected");
        }
        
        if (selectedIndex >= 0) {
            // Previously selected device is still available, reselect it
            std::cout << "[UI] Reselecting previously selected device" << std::endl;
            midiCIDeviceCombo->setCurrentIndex(selectedIndex);
        } else if (midiCIDeviceCombo->count() == 1 && selectedDeviceMuid == 0) {
            // Auto-select first device if no device was previously selected and only one is available
            std::cout << "[UI] Auto-selecting first endpoint-ready device" << std::endl;
            onMidiCIDeviceSelected(0);
        }
    }
}

void KeyboardWidget::setMidiCIDiscoveryCallback(std::function<void()> callback) {
    midiCIDiscoveryCallback = callback;
}


void KeyboardWidget::setMidiCIDeviceProvider(std::function<MidiCIDeviceInfo*(uint32_t)> provider) {
    midiCIDeviceProvider = provider;
}

void KeyboardWidget::sendMidiCIDiscovery() {
    if (midiCIDiscoveryCallback) {
        midiCIDiscoveryCallback();
    }
}


void KeyboardWidget::onMidiCIDeviceSelected(int index) {
    if (index < 0 || !midiCIDeviceProvider) {
        midiCISelectedDeviceInfo->setText("No device selected");
        return;
    }
    
    QVariant muidVariant = midiCIDeviceCombo->itemData(index);
    if (!muidVariant.isValid()) {
        midiCISelectedDeviceInfo->setText("Invalid device selection");
        return;
    }
    
    uint32_t muid = muidVariant.toUInt();
    MidiCIDeviceInfo* device = midiCIDeviceProvider(muid);
    
    if (device) {
        QString info = QString("MUID: 0x%1 (%2)\nManufacturer: %3\nModel: %4\nVersion: %5")
                       .arg(muid, 0, 16)
                       .arg(muid)
                       .arg(QString::fromStdString(device->manufacturer))
                       .arg(QString::fromStdString(device->model))
                       .arg(QString::fromStdString(device->version));
        midiCISelectedDeviceInfo->setText(info);
    } else {
        midiCISelectedDeviceInfo->setText("Device information not available");
    }
    
    // Update selected device MUID for property requests
    uint32_t previousDeviceMuid = selectedDeviceMuid;
    selectedDeviceMuid = muid;
    
    // Enable property refresh button
    refreshPropertiesButton->setEnabled(true);
    
    // Only automatically refresh properties if this is a different device
    if (muid != previousDeviceMuid && muid != 0) {
        std::cout << "[UI] Auto-refreshing properties for newly selected device MUID: 0x" << std::hex << muid << std::dec << std::endl;
        refreshProperties();
    } else if (muid == previousDeviceMuid) {
        std::cout << "[UI] Same device selected, not auto-refreshing properties" << std::endl;
        // Just update the display with existing data
        updateProperties(muid);
    }
}

// Property management methods - updated for simplified API
void KeyboardWidget::setPropertyDataProvider(std::function<std::optional<std::vector<midicci::commonproperties::MidiCIControl>>(uint32_t)> ctrlProvider,
                                            std::function<std::optional<std::vector<midicci::commonproperties::MidiCIProgram>>(uint32_t)> progProvider) {
    ctrlListProvider = ctrlProvider;
    programListProvider = progProvider;
}

void KeyboardWidget::refreshProperties() {
    if (selectedDeviceMuid == 0) {
        return;
    }
    
    std::cout << "[UI] Force refreshing properties for MUID: 0x" << std::hex << selectedDeviceMuid << std::dec << std::endl;
    
    // Clear current lists and show loading
    controlListWidget->setControls({});
    controlListWidget->setEnabled(false);
    programListWidget->clear();
    programListWidget->addItem("Loading programs...");
    
    // Request properties by calling updateProperties - this will trigger new requests if needed
    updateProperties(selectedDeviceMuid);
}


void KeyboardWidget::updateProperties(uint32_t muid) {
    if (muid != selectedDeviceMuid) {
        return; // Not for the currently selected device
    }
    
    // Ensure UI updates happen on the main thread
    if (QThread::currentThread() != this->thread()) {
        // Don't call ctrlListProvider from background thread - just queue the UI update
        QMetaObject::invokeMethod(this, [this, muid]() {
            updatePropertiesOnMainThread(muid);
        }, Qt::QueuedConnection);
        return;
    }
    
    updatePropertiesOnMainThread(muid);
}

void KeyboardWidget::updatePropertiesOnMainThread(uint32_t muid) {
    // Update control list using virtualized widget
    if (ctrlListProvider) {
        auto controls_opt = ctrlListProvider(muid);
        
        if (!controls_opt.has_value()) {
            controlListWidget->setControls({});
            controlListWidget->setEnabled(false);
        } else {
            auto controls = controls_opt.value();
            controlListWidget->setControls(controls);
            controlListWidget->setEnabled(!controls.empty());
        }
    }
    
    // Update program list
    if (programListProvider) {
        auto programs_opt = programListProvider(muid);
        programListWidget->clear();
        
        if (!programs_opt.has_value()) {
            programListWidget->addItem("Loading programs...");
            programListWidget->setEnabled(false);
        } else {
            auto programs = programs_opt.value();
            if (programs.empty()) {
                programListWidget->addItem("No programs available");
                programListWidget->setEnabled(false);
            } else {
                programListWidget->setEnabled(true);
                for (const auto& prog : programs) {
                    QString displayText;
                    
                    // Format: title [bank:PC = X:Y:Z]
                    QString title = QString::fromStdString(prog.title);
                    
                    if (prog.bankPC.size() >= 3) {
                        displayText = QString("%1 [bank:PC = %2:%3:%4]")
                                     .arg(title)
                                     .arg(prog.bankPC[0])
                                     .arg(prog.bankPC[1])
                                     .arg(prog.bankPC[2]);
                    } else {
                        displayText = title;
                    }
                    
                    programListWidget->addItem(displayText);
                }
            }
        }
    }
}

void KeyboardWidget::onPropertiesUpdated(uint32_t muid) {
    updateProperties(muid);
}

#include "keyboard_widget.moc"