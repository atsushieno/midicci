
#include <QtWidgets/QApplication>
#include <QMetaObject>
#include "keyboard_widget.h"
#include "keyboard_controller.h"
#include <iostream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    
    KeyboardWidget keyboard;
    KeyboardController controller;
    
    // Set up callbacks
    keyboard.setKeyPressedCallback([&controller](int note) {
        controller.noteOn(note, 80); // Default velocity
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
    
    // Set up property data providers - simplified API automatically handles requests
    keyboard.setPropertyDataProvider(
        [&controller](uint32_t muid) { return controller.getAllCtrlList(muid); },
        [&controller](uint32_t muid) { return controller.getProgramList(muid); }
    );
    
    // Set up properties changed callback
    controller.setMidiCIPropertiesChangedCallback([&keyboard](uint32_t muid) {
        std::cout << "Properties updated for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        
        // Ensure UI updates happen on the main Qt thread
        QMetaObject::invokeMethod(&keyboard, [&keyboard, muid]() {
            keyboard.onPropertiesUpdated(muid);
        }, Qt::QueuedConnection);
    });
    
    
    // Set up MIDI connection state change callback for auto-discovery
    controller.setMidiConnectionChangedCallback([&controller, &keyboard](bool hasValidPair) {
        if (hasValidPair && controller.isMidiCIInitialized()) {
            std::cout << "Valid MIDI pair established - sending MIDI-CI Discovery" << std::endl;
            controller.sendMidiCIDiscovery();
            
            // Ensure UI updates happen on the main Qt thread
            QMetaObject::invokeMethod(&keyboard, [&controller, &keyboard]() {
                keyboard.updateMidiCIDevices(controller.getMidiCIDeviceDetails());
            }, Qt::QueuedConnection);
        } else if (!hasValidPair) {
            std::cout << "MIDI pair disconnected - clearing MIDI-CI device list" << std::endl;
            
            // Ensure UI updates happen on the main Qt thread
            QMetaObject::invokeMethod(&keyboard, [&controller, &keyboard]() {
                keyboard.updateMidiCIDevices(controller.getMidiCIDeviceDetails());
            }, Qt::QueuedConnection);
        }
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
    
    keyboard.show();
    
    return app.exec();
}

