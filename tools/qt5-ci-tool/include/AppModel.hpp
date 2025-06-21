#pragma once

#include <memory>

#include "midicci/tooling/CIToolRepository.hpp"


namespace midicci::tooling::qt5 {

void initializeAppModel();

void shutdownAppModel();

midicci::tooling::CIToolRepository& getAppModel();

} // namespace qt5_ci_tool
