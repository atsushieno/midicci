#pragma once

#include <functional>
#include <mutex>
#include <queue>

namespace midicci::app {

class ImGuiEventLoop {
public:
    using Task = std::function<void()>;

    void enqueue(Task task) {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }

    void processQueuedTasks() {
        std::queue<Task> local;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(local, tasks_);
        }
        while (!local.empty()) {
            auto task = std::move(local.front());
            local.pop();
            if (task) {
                task();
            }
        }
    }

private:
    std::queue<Task> tasks_;
    std::mutex mutex_;
};

} // namespace midicci::app
