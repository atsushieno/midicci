#pragma once

#include <functional>

namespace umppi {

enum class PlayerState {
    STOPPED,
    PLAYING,
    PAUSED
};

enum class SeekFilterResult {
    PASS,
    BLOCK,
    PASS_AND_TERMINATE,
    BLOCK_AND_TERMINATE
};

template<typename TEvent>
using SeekProcessor = std::function<SeekFilterResult(const TEvent&)>;

}
