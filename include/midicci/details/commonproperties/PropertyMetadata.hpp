#pragma once

#include "midicci/midicci.hpp"

namespace midicci::commonproperties {
    class PropertyMetadata {
    public:
        virtual ~PropertyMetadata() = default;

        virtual const std::string& getPropertyId() const = 0;
        virtual std::string getExtra(const std::string& key) const = 0;
    };
}
