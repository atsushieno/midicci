#pragma once

#include <umppi/details/Utility.hpp>
#include <vector>
#include <functional>
#include <stdexcept>

namespace umppi {

template<typename T>
class DeltaTimeComputer {
public:
    virtual ~DeltaTimeComputer() = default;

    virtual int messageToDeltaTime(const T& message) const = 0;
    virtual bool isTempoMessage(const T& message) const = 0;
    virtual int getTempoValue(const T& message) const = 0;

    std::vector<Timed<T>> filterEvents(
        const std::vector<T>& messages,
        std::function<bool(const T&)> filter) const
    {
        std::vector<Timed<T>> result;
        int v = 0;
        for (const auto& m : messages) {
            v += messageToDeltaTime(m);
            if (filter(m)) {
                result.emplace_back(Dc{v}, m);
            }
        }
        return result;
    }

    int getTotalPlayTimeMilliseconds(
        const std::vector<T>& messages,
        int deltaTimeSpec) const
    {
        int totalTicks = 0;
        for (const auto& m : messages) {
            totalTicks += messageToDeltaTime(m);
        }
        return getPlayTimeMillisecondsAtTick(messages, totalTicks, deltaTimeSpec);
    }

    int getPlayTimeMillisecondsAtTick(
        const std::vector<T>& messages,
        int ticks,
        int deltaTimeSpec) const
    {
        if (deltaTimeSpec < 0) {
            throw std::runtime_error("non-tick based DeltaTime not supported");
        }

        static constexpr int DEFAULT_TEMPO = 500000;
        int tempo = DEFAULT_TEMPO;
        double v = 0.0;
        int t = 0;

        for (const auto& m : messages) {
            int messageDeltaTime = messageToDeltaTime(m);
            int deltaTime = (t + messageDeltaTime < ticks) ? messageDeltaTime : (ticks - t);

            v += static_cast<double>(tempo) / 1000.0 * deltaTime / deltaTimeSpec;

            if (deltaTime != messageDeltaTime) {
                break;
            }

            t += messageDeltaTime;

            if (isTempoMessage(m)) {
                tempo = getTempoValue(m);
            }
        }

        return static_cast<int>(v);
    }
};

} // namespace umppi
