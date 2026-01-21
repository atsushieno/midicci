#include <umppi/details/MidiPlayerTimer.hpp>
#include <thread>

namespace umppi {

void SimpleAdjustingMidiPlayerTimer::waitBySeconds(double addedSeconds) {
    if (addedSeconds > 0) {
        double delta = addedSeconds;

        if (initialized_) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - startedTime_);
            double actualTotalSeconds = elapsed.count() / 1'000'000.0;
            delta -= actualTotalSeconds - nominalTotalSeconds_;
        } else {
            startedTime_ = std::chrono::high_resolution_clock::now();
            initialized_ = true;
        }

        if (delta > 0) {
            auto duration = std::chrono::microseconds(static_cast<long long>(delta * 1'000'000));
            std::this_thread::sleep_for(duration);
        }

        nominalTotalSeconds_ += addedSeconds;
    }
}

}
