#pragma once

#include <umppi/details/Common.hpp>
#include <umppi/details/Utility.hpp>
#include <vector>
#include <memory>
#include <stdexcept>

namespace umppi {

class Midi1Exception : public std::runtime_error {
public:
    explicit Midi1Exception(const std::string& message) : std::runtime_error(message) {}
};

class Midi1Message {
public:
    virtual ~Midi1Message() = default;

    virtual int getValue() const = 0;

    uint8_t getStatusByte() const {
        return static_cast<uint8_t>(getValue() & 0xFF);
    }

    uint8_t getStatusCode() const {
        uint8_t sb = getStatusByte();
        if (sb == Midi1Status::META ||
            sb == Midi1Status::SYSEX ||
            sb == Midi1Status::SYSEX_END) {
            return sb;
        }
        return static_cast<uint8_t>(getValue() & 0xF0);
    }

    uint8_t getMsb() const {
        return static_cast<uint8_t>((getValue() & 0xFF00) >> 8);
    }

    uint8_t getLsb() const {
        return static_cast<uint8_t>((getValue() & 0xFF0000) >> 16);
    }

    uint8_t getMetaType() const {
        return getMsb();
    }

    uint8_t getChannel() const {
        return static_cast<uint8_t>(getValue() & 0x0F);
    }

    static uint8_t fixedDataSize(uint8_t statusByte) {
        switch (statusByte & 0xF0) {
            case 0xF0:
                switch (statusByte) {
                    case 0xF1:
                    case 0xF3:
                        return 1;
                    case 0xF2:
                        return 2;
                    default:
                        return 0;
                }
            case 0xC0:
            case 0xD0:
                return 1;
            default:
                return 2;
        }
    }
};

class Midi1SimpleMessage : public Midi1Message {
private:
    int value_;

public:
    Midi1SimpleMessage(int value) : value_(value) {}

    Midi1SimpleMessage(int type, int arg1, int arg2)
        : value_(static_cast<int>((static_cast<uint32_t>(type) +
                                   (static_cast<uint32_t>(arg1) << 8) +
                                   (static_cast<uint32_t>(arg2) << 16)))) {}

    int getValue() const override { return value_; }
};

class Midi1CompoundMessage : public Midi1Message {
private:
    int value_;
    std::vector<uint8_t> extraData_;
    size_t extraDataOffset_;
    size_t extraDataLength_;

public:
    Midi1CompoundMessage(int type, int arg1, int arg2,
                        const std::vector<uint8_t>& extraData = {},
                        size_t extraOffset = 0,
                        size_t extraLength = 0)
        : value_(static_cast<int>((static_cast<uint32_t>(type) +
                                   (static_cast<uint32_t>(arg1) << 8) +
                                   (static_cast<uint32_t>(arg2) << 16)))),
          extraData_(extraData),
          extraDataOffset_(extraOffset),
          extraDataLength_(extraLength == 0 ? extraData.size() : extraLength) {}

    int getValue() const override { return value_; }

    const std::vector<uint8_t>& getExtraData() const { return extraData_; }
    size_t getExtraDataOffset() const { return extraDataOffset_; }
    size_t getExtraDataLength() const { return extraDataLength_; }
};

} // namespace umppi
