#include "midi-ci/core/MidiCIDevice.hpp"
#include <iostream>
#include <cassert>

using namespace midi_ci::core;

void test_device_initialization() {
    MidiCIDevice device;
    
    assert(!device.is_initialized());
    
    device.initialize();
    assert(device.is_initialized());
    
    device.shutdown();
    assert(!device.is_initialized());
    
    std::cout << "✓ Device initialization test passed\n";
}

void test_device_id() {
    MidiCIDevice device;
    
    assert(device.get_device_id() == 0x7F);
    
    device.set_device_id(0x42);
    assert(device.get_device_id() == 0x42);
    
    std::cout << "✓ Device ID test passed\n";
}

int run_all_tests() {
    try {
        test_device_initialization();
        test_device_id();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
