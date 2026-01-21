#pragma once

#include <chrono>

namespace umppi {

class MidiPlayerTimer {
public:
    virtual ~MidiPlayerTimer() = default;
    virtual void waitBySeconds(double seconds) = 0;
    virtual void stop() {}
};

class SimpleAdjustingMidiPlayerTimer : public MidiPlayerTimer {
private:
    std::chrono::high_resolution_clock::time_point startedTime_;
    double nominalTotalSeconds_ = 0.0;
    bool initialized_ = false;

public:
    void waitBySeconds(double addedSeconds) override;
    void stop() override {}
};

}
