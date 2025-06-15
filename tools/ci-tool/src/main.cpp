#include "CIToolRepository.hpp"
#include "CIDeviceManager.hpp"
#include "MidiDeviceManager.hpp"
#include "CIDeviceModel.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

namespace {
    volatile bool running = true;
    
    void signal_handler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            std::cout << "\nShutting down..." << std::endl;
            running = false;
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "MIDI-CI Tool - Proof of Concept Application" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        ci_tool::CIToolRepository repository;
        
        repository.add_log_callback([](const ci_tool::LogEntry& entry) {
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] ";
            std::cout << (entry.direction == ci_tool::MessageDirection::In ? "IN " : "OUT") << " ";
            std::cout << entry.message << std::endl;
        });
        
        std::cout << "Initializing MIDI-CI device with MUID: 0x" << std::hex << repository.get_muid() << std::dec << std::endl;
        
        auto midi_manager = repository.get_midi_device_manager();
        auto ci_manager = repository.get_ci_device_manager();
        
        midi_manager->initialize();
        ci_manager->initialize();
        
        auto device_model = ci_manager->get_device_model();
        if (device_model) {
            std::cout << "Device model initialized successfully" << std::endl;
            
            auto profiles = device_model->get_local_profile_states();
            std::cout << "Local profiles: " << profiles.size() << std::endl;
            
            for (const auto& profile : profiles) {
                std::cout << "  Profile - Group: " << static_cast<int>(profile->get_group())
                          << ", Address: " << static_cast<int>(profile->get_address())
                          << ", Enabled: " << profile->is_enabled() << std::endl;
            }
        }
        
        repository.log("MIDI-CI Tool started", ci_tool::MessageDirection::Out);
        
        std::cout << "\nMIDI-CI Tool is running. Press Ctrl+C to exit." << std::endl;
        std::cout << "Available commands:" << std::endl;
        std::cout << "  d - Send discovery inquiry" << std::endl;
        std::cout << "  s - Save configuration" << std::endl;
        std::cout << "  l - Load configuration" << std::endl;
        std::cout << "  c - Clear logs" << std::endl;
        std::cout << "  q - Quit" << std::endl;
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (std::cin.rdbuf()->in_avail() > 0) {
                char command;
                std::cin >> command;
                
                switch (command) {
                    case 'd':
                        if (device_model) {
                            device_model->send_discovery();
                            repository.log("Discovery inquiry sent", ci_tool::MessageDirection::Out);
                        }
                        break;
                    case 's':
                        repository.save_default_config();
                        break;
                    case 'l':
                        repository.load_default_config();
                        break;
                    case 'c':
                        repository.clear_logs();
                        std::cout << "Logs cleared" << std::endl;
                        break;
                    case 'q':
                        running = false;
                        break;
                    default:
                        std::cout << "Unknown command: " << command << std::endl;
                        break;
                }
            }
        }
        
        repository.log("MIDI-CI Tool shutting down", ci_tool::MessageDirection::Out);
        
        ci_manager->shutdown();
        midi_manager->shutdown();
        
        std::cout << "MIDI-CI Tool shutdown complete" << std::endl;
        
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}
