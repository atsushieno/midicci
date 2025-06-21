#pragma once

#include <memory>

namespace tooling {
    class CIToolRepository;
}

namespace qt5_ci_tool {

void initializeAppModel();

void shutdownAppModel();

tooling::CIToolRepository& getAppModel();

} // namespace qt5_ci_tool
