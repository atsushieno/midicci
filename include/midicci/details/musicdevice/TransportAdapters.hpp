#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <span>
#include <umppi/details/Ump.hpp>
#include "midicci/details/musicdevice/MidiCISession.hpp"

namespace midicci::musicdevice {

using ByteStreamCallback = std::function<void(const uint8_t*, size_t, size_t, uint64_t)>;
using ByteStreamListenerAdder = std::function<void(ByteStreamCallback)>;
using ByteStreamSender = std::function<void(const uint8_t*, size_t, size_t, uint64_t)>;

inline MidiInputListenerAdder adaptByteInputListener(ByteStreamListenerAdder byte_adder) {
    return [byte_adder](MidiInputCallback callback) {
        byte_adder([callback](const uint8_t* data, size_t start, size_t length, uint64_t timestamp) {
            if (data == nullptr || length == 0) {
                return;
            }

            auto umps = umppi::parseUmpsFromBytes(data, start, length);
            if (umps.empty()) {
                return;
            }

            std::vector<uint32_t> words;
            for (const auto& ump : umps) {
                size_t base = words.size();
                ump.toWords(words, base);
            }

            callback(umppi::UmpWordSpan{words.data(), words.size()}, timestamp);
        });
    };
}

inline std::function<void(umppi::UmpWordSpan, uint64_t)> adaptByteOutputSender(ByteStreamSender byte_sender) {
    return [byte_sender](umppi::UmpWordSpan words, uint64_t timestamp) {
        if (words.empty()) {
            return;
        }

        std::vector<uint8_t> bytes;
        bytes.reserve(words.size() * 4);
        for (uint32_t word : words) {
            bytes.push_back(static_cast<uint8_t>((word >> 24) & 0xFF));
            bytes.push_back(static_cast<uint8_t>((word >> 16) & 0xFF));
            bytes.push_back(static_cast<uint8_t>((word >> 8) & 0xFF));
            bytes.push_back(static_cast<uint8_t>(word & 0xFF));
        }

        byte_sender(bytes.data(), 0, bytes.size(), timestamp);
    };
}

} // namespace midicci::musicdevice
