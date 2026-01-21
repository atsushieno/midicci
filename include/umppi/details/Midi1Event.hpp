#pragma once

#include <umppi/details/Midi1Message.hpp>
#include <memory>
#include <vector>

namespace umppi {

struct Midi1Event {
    int deltaTime;
    std::shared_ptr<Midi1Message> message;

    Midi1Event(int dt, std::shared_ptr<Midi1Message> msg)
        : deltaTime(dt), message(std::move(msg)) {}

    static std::vector<uint8_t> encode7BitLength(int length) {
        std::vector<uint8_t> result;
        int v = length;
        while (v >= 0x80) {
            result.push_back(static_cast<uint8_t>(v % 0x80 + 0x80));
            v /= 0x80;
        }
        result.push_back(static_cast<uint8_t>(v));
        return result;
    }
};

} // namespace umppi
