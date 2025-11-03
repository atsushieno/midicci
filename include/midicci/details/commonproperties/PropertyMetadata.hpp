#pragma once

#include "midicci/midicci.hpp"

namespace midicci::commonproperties {
    class PropertyMetadata {
    public:
        virtual ~PropertyMetadata() = default;

        virtual const std::string& getPropertyId() const = 0;
        virtual const std::string& getResourceId() const = 0;
        virtual const std::string& getName() const = 0;
        virtual const std::string& getMediaType() const = 0;
        virtual const std::string& getEncoding() const = 0;
        virtual const std::vector<uint8_t>& getData() const = 0;
        virtual std::string getExtra(const std::string& key) const = 0;
    };
}
