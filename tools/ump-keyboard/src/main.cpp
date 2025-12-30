
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QMetaObject>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include "main_window.h"
#include "keyboard_controller.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    
    midicci::keyboard::MainWindow mainWindow;
    auto& keyboard = *mainWindow.findChild<KeyboardWidget*>();
    auto* logger = mainWindow.getLogger();
    KeyboardController controller(logger);
    
    // Set up callbacks
    keyboard.setKeyPressedCallback([&controller](int note) {
        controller.noteOn(note, 0xF800); // Default velocity (high velocity in MIDI 2.0 16-bit format)
        std::cout << "Note ON: " << note << std::endl;
    });
    
    keyboard.setKeyReleasedCallback([&controller](int note) {
        controller.noteOff(note);
        std::cout << "Note OFF: " << note << std::endl;
    });
    
    keyboard.setDeviceRefreshCallback([&controller, &keyboard]() {
        auto inputDevices = controller.getInputDevices();
        auto outputDevices = controller.getOutputDevices();
        keyboard.updateMidiDevices(inputDevices, outputDevices);
        
        // MIDI-CI status should remain static - only update device list if connections change
    });
    
    // Set up MIDI-CI callback
    keyboard.setMidiCIDiscoveryCallback([&controller, &keyboard]() {
        controller.sendMidiCIDiscovery();
        std::cout << "MIDI-CI Discovery sent" << std::endl;
        
        // Update device list after discovery
        keyboard.updateMidiCIDevices(controller.getMidiCIDeviceDetails());
    });
    
    // Set up MIDI-CI devices changed callback for automatic updates
    controller.setMidiCIDevicesChangedCallback([&controller, &keyboard]() {
        std::cout << "MIDI-CI device list updated" << std::endl;
        
        // Ensure UI updates happen on the main Qt thread
        QMetaObject::invokeMethod(&keyboard, [&controller, &keyboard]() {
            keyboard.updateMidiCIDevices(controller.getMidiCIDeviceDetails());
        }, Qt::QueuedConnection);
    });
    
    // Removed periodic MIDI-CI updates - now using event-driven callbacks
    
    // Set up MIDI-CI device provider for detailed info
    keyboard.setMidiCIDeviceProvider([&controller](uint32_t muid) -> MidiCIDeviceInfo* {
        return controller.getMidiCIDeviceByMuid(muid);
    });
    
    // Set up property data providers - read-only; explicit buttons send requests
    keyboard.setPropertyDataProvider(
        [&controller](uint32_t muid) { return controller.getAllCtrlList(muid); },
        [&controller](uint32_t muid) { return controller.getProgramList(muid); }
    );

    // Set up explicit property requesters
    keyboard.setPropertyRequesters(
        [&controller](uint32_t muid) { controller.requestAllCtrlList(muid); },
        [&controller](uint32_t muid) { controller.requestProgramList(muid); },
        [&controller](uint32_t muid) { controller.requestSaveState(muid); }
    );

    // Set up state management callbacks
    keyboard.setStateSendCallback(
        [&controller](uint32_t muid, const std::string& stateId, const std::vector<uint8_t>& data) {
            controller.sendState(muid, stateId, data);
        }
    );

    controller.setStateSaveCallback([&keyboard, &controller](uint32_t muid, const std::vector<uint8_t>& stateData) {
        QMetaObject::invokeMethod(&keyboard, [&keyboard, &controller, muid, stateData]() {
            QSettings settings("midicci", "keyboard");
            QString lastDir = settings.value("lastStateDirectory", QDir::homePath()).toString();

            MidiCIDeviceInfo* device = controller.getMidiCIDeviceByMuid(muid);
            QString deviceName = device ? QString::fromStdString(device->model) : QString("device");

            // Sanitize device name to remove illegal filename characters
            // On Windows: < > : " / \ | ? *
            // On Unix/macOS: / and potentially :
            // We also remove control characters (0x00-0x1F) for safety
            QString sanitized = deviceName;
            const QString illegalChars = R"(<>:"/\|?*)";
            for (const QChar& ch : illegalChars) {
                sanitized.replace(ch, '-');
            }
            // Remove control characters
            sanitized.remove(QRegularExpression("[\\x00-\\x1F]"));

            QString defaultFileName = QString("State - %1.state").arg(sanitized);
            QString defaultPath = QDir(lastDir).filePath(defaultFileName);

            QString filename = QFileDialog::getSaveFileName(
                &keyboard,
                QObject::tr("Save Device State"),
                defaultPath,
                QObject::tr("State Files (*.state);;All Files (*)")
            );

            if (filename.isEmpty()) {
                return;
            }

            if (!filename.endsWith(".state")) {
                filename += ".state";
            }

            settings.setValue("lastStateDirectory", QFileInfo(filename).absolutePath());

            std::ofstream outfile(filename.toStdString(), std::ios::binary);
            if (outfile) {
                outfile.write(reinterpret_cast<const char*>(stateData.data()), stateData.size());
                outfile.close();

                if (!outfile.good()) {
                    QMessageBox::warning(&keyboard, QObject::tr("Save State"),
                                        QObject::tr("Failed to write data to file:\n%1").arg(filename));
                }
            } else {
                QMessageBox::warning(&keyboard, QObject::tr("Save State"),
                                    QObject::tr("Failed to open file for writing:\n%1").arg(filename));
            }
        }, Qt::QueuedConnection);
    });

    // Set up control map provider for enumerated values
    keyboard.setControlMapProvider(
        [&controller](uint32_t muid, const std::string& ctrlMapId) {
            return controller.getCtrlMapList(muid, ctrlMapId);
        }
    );
    keyboard.setControlMapRequester(
        [&controller](uint32_t muid, const std::string& ctrlMapId) {
            controller.requestCtrlMapList(muid, ctrlMapId);
        }
    );

    // Set up properties changed callback
    controller.setMidiCIPropertiesChangedCallback([&keyboard](uint32_t muid, const std::string& propertyId, const std::string& resId) {
        std::cout << "Property updated for MUID: 0x" << std::hex << muid << std::dec << ", id='" << propertyId << "', resId='" << resId << "'" << std::endl;

        // Ensure UI updates happen on the main Qt thread, pass property id
        QMetaObject::invokeMethod(&keyboard, [&keyboard, muid, propertyId, resId]() {
            keyboard.onPropertiesUpdated(muid,
                                         QString::fromStdString(propertyId),
                                         QString::fromStdString(resId));
        }, Qt::QueuedConnection);
    });
    
    
    // Set up MIDI connection state change callback for auto-discovery
    controller.setMidiConnectionChangedCallback([&controller, &keyboard](bool hasValidPair) {
        // Do not auto-start MIDI-CI session; just update UI
        QMetaObject::invokeMethod(&keyboard, [&controller, &keyboard]() {
            keyboard.updateMidiCIDevices(controller.getMidiCIDeviceDetails());
        }, Qt::QueuedConnection);
    });
    
    // Set up control change callbacks
    keyboard.setControlChangeCallback([&controller](int channel, int cc, uint32_t value) {
        controller.sendControlChange(channel, cc, value);
    });
    
    keyboard.setRPNCallback([&controller](int channel, int msb, int lsb, uint32_t value) {
        controller.sendRPN(channel, msb, lsb, value);
    });
    
    keyboard.setNRPNCallback([&controller](int channel, int msb, int lsb, uint32_t value) {
        controller.sendNRPN(channel, msb, lsb, value);
    });
    
    keyboard.setPerNoteControlCallback([&controller](int channel, int note, int cc, uint32_t value) {
        controller.sendPerNoteControlChange(channel, note, cc, value);
    });
    
    keyboard.setPerNoteAftertouchCallback([&controller](int channel, int note, uint32_t value) {
        controller.sendPerNoteAftertouch(channel, note, value);
    });
    
    keyboard.setProgramChangeCallback([&controller](int channel, uint8_t program, uint8_t bankMSB, uint8_t bankLSB) {
        controller.sendProgramChange(channel, program, bankMSB, bankLSB);
    });
    
    // Connect device selection signals
    QObject::connect(&keyboard, &KeyboardWidget::midiInputDeviceChanged,
                    [&controller](const QString& deviceId) {
                        controller.selectInputDevice(deviceId.toStdString());
                    });
    
    QObject::connect(&keyboard, &KeyboardWidget::midiOutputDeviceChanged,
                    [&controller](const QString& deviceId) {
                        controller.selectOutputDevice(deviceId.toStdString());
                    });
    
    // Initialize device lists
    auto inputDevices = controller.getInputDevices();
    auto outputDevices = controller.getOutputDevices();
    keyboard.updateMidiDevices(inputDevices, outputDevices);
    
    // Update MIDI-CI status
    keyboard.updateMidiCIStatus(
        controller.isMidiCIInitialized(),
        controller.getMidiCIMuid(),
        controller.getMidiCIDeviceName()
    );
    keyboard.updateMidiCIDevices(controller.getMidiCIDeviceDetails());
    
    mainWindow.show();
    
    return app.exec();
}
