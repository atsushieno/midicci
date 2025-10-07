#include "TestCITransport.hpp"
#include <iostream>
#include <midicci/details/ump/Ump.hpp>
#include <midicci/details/ump/UmpFactory.hpp>
#include <midicci/details/ump/UmpRetriever.hpp>

namespace midicci::test {

TestCITransport::TestCITransport() {
    // Strategy: Create virtual ports that other devices can connect to,
    // then open those ports from the opposite device to create the loopback

    // Device1 creates a virtual output port
    device1_output_port_name_ = "TestCITransport_Device1_Out";
    libremidi::output_configuration out1_conf;
    device1_output_ = std::make_unique<libremidi::midi_out>(out1_conf, libremidi::midi2::out_default_configuration());
    auto out1_err = device1_output_->open_virtual_port(device1_output_port_name_);
    if (out1_err != stdx::error{}) {
        throw std::runtime_error("Failed to create virtual output port for Device1");
    }

    // Device2 creates a virtual output port
    device2_output_port_name_ = "TestCITransport_Device2_Out";
    libremidi::output_configuration out2_conf;
    device2_output_ = std::make_unique<libremidi::midi_out>(out2_conf, libremidi::midi2::out_default_configuration());
    auto out2_err = device2_output_->open_virtual_port(device2_output_port_name_);
    if (out2_err != stdx::error{}) {
        throw std::runtime_error("Failed to create virtual output port for Device2");
    }

    // Wait for ports to register in the system
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Now Device2 connects its input to Device1's output port
    // Note: When we create a virtual OUTPUT port, it appears as an INPUT port to other devices
    libremidi::observer obs(
        {.track_hardware = true, .track_virtual = true},
        libremidi::midi2::observer_default_configuration());

    auto input_ports = obs.get_input_ports();

    std::cout << "Available input ports for connection:" << std::endl;
    for (const auto& port : input_ports) {
        std::cout << "  - " << port.port_name << std::endl;
    }

    // Device2's input connects to Device1's virtual output (which appears as an input port)
    bool device2_connected = false;
    for (const auto& port : input_ports) {
        if (port.port_name == device1_output_port_name_) {
            std::cout << "Connecting Device2 input to Device1 output: " << port.port_name << std::endl;

            libremidi::ump_input_configuration in2_conf{
                .on_message = [this](libremidi::ump&& packet) {
                    if (!running_) return;

                    std::vector<midicci::ump::Ump> umps;
                    umps.emplace_back(packet.data[0], packet.data[1], packet.data[2], packet.data[3]);

                    if (umps[0].get_message_type() == midicci::ump::MessageType::SYSEX7) {
                        auto sysex_fragment = midicci::ump::UmpRetriever::get_sysex7_data(umps);
                        auto status = static_cast<midicci::ump::BinaryChunkStatus>(umps[0].get_status_code());

                        if (status == midicci::ump::BinaryChunkStatus::START) {
                            device2_sysex_buffer_.clear();
                        }

                        device2_sysex_buffer_.insert(device2_sysex_buffer_.end(),
                                                    sysex_fragment.begin(), sysex_fragment.end());

                        if (status == midicci::ump::BinaryChunkStatus::END ||
                            status == midicci::ump::BinaryChunkStatus::COMPLETE_PACKET) {

                            if (device2_sysex_buffer_.size() > 2 &&
                                device2_sysex_buffer_[0] == 0x7E &&
                                device2_sysex_buffer_[2] == 0x0D) {

                                device2_->processInput(umps[0].get_group(), device2_sysex_buffer_);

                                std::lock_guard<std::mutex> lock(mutex_);
                                message_received_ = true;
                                cv_.notify_all();
                            }
                            device2_sysex_buffer_.clear();
                        }
                    }
                },
                .ignore_sysex = false
            };

            device2_input_ = std::make_unique<libremidi::midi_in>(in2_conf, libremidi::midi2::in_default_configuration());
            auto in2_err = device2_input_->open_port(port);
            if (in2_err != stdx::error{}) {
                std::cerr << "Failed to connect Device2 input to Device1 output" << std::endl;
            } else {
                device2_connected = true;
                std::cout << "Successfully connected Device2 input to Device1 output" << std::endl;
            }
            break;
        }
    }

    // Device1's input connects to Device2's virtual output (which appears as an input port)
    bool device1_connected = false;
    for (const auto& port : input_ports) {
        if (port.port_name == device2_output_port_name_) {
            std::cout << "Connecting Device1 input to Device2 output: " << port.port_name << std::endl;

            libremidi::ump_input_configuration in1_conf{
                .on_message = [this](libremidi::ump&& packet) {
                    if (!running_) return;

                    std::vector<midicci::ump::Ump> umps;
                    umps.emplace_back(packet.data[0], packet.data[1], packet.data[2], packet.data[3]);

                    if (umps[0].get_message_type() == midicci::ump::MessageType::SYSEX7) {
                        auto sysex_fragment = midicci::ump::UmpRetriever::get_sysex7_data(umps);
                        auto status = static_cast<midicci::ump::BinaryChunkStatus>(umps[0].get_status_code());

                        if (status == midicci::ump::BinaryChunkStatus::START) {
                            device1_sysex_buffer_.clear();
                        }

                        device1_sysex_buffer_.insert(device1_sysex_buffer_.end(),
                                                    sysex_fragment.begin(), sysex_fragment.end());

                        if (status == midicci::ump::BinaryChunkStatus::END ||
                            status == midicci::ump::BinaryChunkStatus::COMPLETE_PACKET) {

                            if (device1_sysex_buffer_.size() > 2 &&
                                device1_sysex_buffer_[0] == 0x7E &&
                                device1_sysex_buffer_[2] == 0x0D) {

                                device1_->processInput(umps[0].get_group(), device1_sysex_buffer_);

                                std::lock_guard<std::mutex> lock(mutex_);
                                message_received_ = true;
                                cv_.notify_all();
                            }
                            device1_sysex_buffer_.clear();
                        }
                    }
                },
                .ignore_sysex = false
            };

            device1_input_ = std::make_unique<libremidi::midi_in>(in1_conf, libremidi::midi2::in_default_configuration());
            auto in1_err = device1_input_->open_port(port);
            if (in1_err != stdx::error{}) {
                std::cerr << "Failed to connect Device1 input to Device2 output" << std::endl;
            } else {
                device1_connected = true;
                std::cout << "Successfully connected Device1 input to Device2 output" << std::endl;
            }
            break;
        }
    }

    if (!device1_connected || !device2_connected) {
        std::cerr << "Warning: Not all port connections established" << std::endl;
        throw std::runtime_error("Failed to establish MIDI port connections");
    }

    // Now setup the MIDI-CI devices
    config1_.device_info = {0x123456, 0x1234, 0x100, 0x00000001,
                           "TestDevice1", "TestFamily1", "TestModel1", "1.0", "DEV1-001"};
    device1_ = std::make_unique<MidiCIDevice>(19474 & 0x7F7F7F7F, config1_);

    config2_.device_info = {0x654321, 0x4321, 0x200, 0x00000002,
                           "TestDevice2", "TestFamily2", "TestModel2", "2.0", "DEV2-002"};
    device2_ = std::make_unique<MidiCIDevice>(37564 & 0x7F7F7F7F, config2_);

    // Set up SysEx senders
    device1_->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        std::vector<uint8_t> sysex_with_markers;
        sysex_with_markers.push_back(0xF0);
        sysex_with_markers.insert(sysex_with_markers.end(), data.begin(), data.end());
        sysex_with_markers.push_back(0xF7);

        std::vector<uint8_t> sysex_payload(sysex_with_markers.begin() + 1, sysex_with_markers.end() - 1);
        auto umps = midicci::ump::UmpFactory::sysex7(group, sysex_payload);

        std::vector<uint32_t> ump_data;
        for (const auto& ump : umps) {
            ump_data.push_back(ump.int1);
            ump_data.push_back(ump.int2);
            ump_data.push_back(ump.int3);
            ump_data.push_back(ump.int4);
        }

        device1_send_ump(ump_data);
        return true;
    });

    device2_->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        std::vector<uint8_t> sysex_with_markers;
        sysex_with_markers.push_back(0xF0);
        sysex_with_markers.insert(sysex_with_markers.end(), data.begin(), data.end());
        sysex_with_markers.push_back(0xF7);

        std::vector<uint8_t> sysex_payload(sysex_with_markers.begin() + 1, sysex_with_markers.end() - 1);
        auto umps = midicci::ump::UmpFactory::sysex7(group, sysex_payload);

        std::vector<uint32_t> ump_data;
        for (const auto& ump : umps) {
            ump_data.push_back(ump.int1);
            ump_data.push_back(ump.int2);
            ump_data.push_back(ump.int3);
            ump_data.push_back(ump.int4);
        }

        device2_send_ump(ump_data);
        return true;
    });

    // Give everything time to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "TestCITransport initialization complete" << std::endl;
}

TestCITransport::~TestCITransport() {
    running_ = false;

    // Close all ports
    if (device1_input_ && device1_input_->is_port_open()) {
        device1_input_->close_port();
    }
    if (device1_output_ && device1_output_->is_port_open()) {
        device1_output_->close_port();
    }
    if (device2_input_ && device2_input_->is_port_open()) {
        device2_input_->close_port();
    }
    if (device2_output_ && device2_output_->is_port_open()) {
        device2_output_->close_port();
    }

    device1_.reset();
    device2_.reset();
}

void TestCITransport::device1_send_ump(const std::vector<uint32_t>& ump_data) {
    if (device1_output_ && device1_output_->is_port_open()) {
        try {
            device1_output_->send_ump(ump_data.data(), ump_data.size());
        } catch (const std::exception& e) {
            std::cerr << "Device1 send error: " << e.what() << std::endl;
        }
    }
}

void TestCITransport::device2_send_ump(const std::vector<uint32_t>& ump_data) {
    if (device2_output_ && device2_output_->is_port_open()) {
        try {
            device2_output_->send_ump(ump_data.data(), ump_data.size());
        } catch (const std::exception& e) {
            std::cerr << "Device2 send error: " << e.what() << std::endl;
        }
    }
}

void TestCITransport::processMessages(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    message_received_ = false;
    cv_.wait_for(lock, timeout);
}

bool TestCITransport::waitForCondition(std::function<bool()> condition,
                                      std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    const auto poll_interval = std::chrono::milliseconds(10);

    while (std::chrono::steady_clock::now() - start < timeout) {
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(poll_interval);
    }

    return condition(); // Final check
}

void TestCITransport::pumpMessages() {
    // Give some time for messages to flow through the virtual ports
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

} // namespace midicci::test
