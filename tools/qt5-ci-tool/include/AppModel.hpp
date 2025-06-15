#pragma once

#include <memory>

namespace ci_tool {
    class CIToolRepository;
}

namespace qt5_ci_tool {

void initializeAppModel();

void shutdownAppModel();

ci_tool::CIToolRepository& getAppModel();

} // namespace qt5_ci_tool
