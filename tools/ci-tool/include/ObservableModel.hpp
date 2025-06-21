#pragma once

#include <functional>
#include <vector>
#include <mutex>

namespace tooling {

template<typename CallbackType>
class ObservableModel {
public:
    using Callback = CallbackType;
    
    void add_callback(Callback callback) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        callbacks_.push_back(callback);
    }
    
    void remove_callback(const Callback& callback) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        callbacks_.erase(
            std::remove_if(callbacks_.begin(), callbacks_.end(),
                [&callback](const Callback& cb) {
                    return cb.target<void()>() == callback.target<void()>();
                }),
            callbacks_.end());
    }
    
    void clear_callbacks() {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        callbacks_.clear();
    }

protected:
    void notify_observers() {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        for (const auto& callback : callbacks_) {
            callback();
        }
    }
    
    template<typename... Args>
    void notify_observers(Args&&... args) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        for (const auto& callback : callbacks_) {
            callback(std::forward<Args>(args)...);
        }
    }

private:
    std::vector<Callback> callbacks_;
    mutable std::mutex callbacks_mutex_;
};

}
