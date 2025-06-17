#pragma once

#include <functional>
#include <mutex>

namespace ci_tool {

template<typename T>
class MutableState {
public:
    using ValueChangedHandler = std::function<void(const T& newValue)>;
    
    explicit MutableState(const T& initialValue = T{}) : value_(initialValue) {}
    
    const T& get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }
    
    void set(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (value_ != value) {
            value_ = value;
            if (handler_) {
                handler_(value_);
            }
        }
    }
    
    void set_value_changed_handler(ValueChangedHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handler_ = handler;
    }
    
    T& operator*() {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }
    
    const T& operator*() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

private:
    T value_;
    ValueChangedHandler handler_;
    mutable std::mutex mutex_;
};

enum class StateChangeAction {
    ADDED,
    REMOVED
};

template<typename T>
class MutableStateList {
public:
    using CollectionChangedHandler = std::function<void(StateChangeAction action, const T& item)>;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    
    MutableStateList() = default;
    
    MutableStateList(const MutableStateList&) = delete;
    MutableStateList& operator=(const MutableStateList&) = delete;
    
    MutableStateList(MutableStateList&& other) noexcept 
        : items_(std::move(other.items_)), handler_(std::move(other.handler_)) {}
    
    MutableStateList& operator=(MutableStateList&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock1(mutex_);
            std::lock_guard<std::mutex> lock2(other.mutex_);
            items_ = std::move(other.items_);
            handler_ = std::move(other.handler_);
        }
        return *this;
    }
    
    void add(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.push_back(item);
        if (handler_) {
            handler_(StateChangeAction::ADDED, item);
        }
    }
    
    void remove(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find(items_.begin(), items_.end(), item);
        if (it != items_.end()) {
            items_.erase(it);
            if (handler_) {
                handler_(StateChangeAction::REMOVED, item);
            }
        }
    }
    
    template<typename Predicate>
    void remove_if(Predicate pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::remove_if(items_.begin(), items_.end(), pred);
        for (auto removed_it = it; removed_it != items_.end(); ++removed_it) {
            if (handler_) {
                handler_(StateChangeAction::REMOVED, *removed_it);
            }
        }
        items_.erase(it, items_.end());
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& item : items_) {
            if (handler_) {
                handler_(StateChangeAction::REMOVED, item);
            }
        }
        items_.clear();
    }
    
    iterator begin() {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.begin();
    }
    
    iterator end() {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.end();
    }
    
    const_iterator begin() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.begin();
    }
    
    const_iterator end() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.end();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.empty();
    }
    
    const T& operator[](size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_[index];
    }
    
    T& operator[](size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_[index];
    }
    
    std::vector<T> to_vector() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_;
    }
    
    void set_collection_changed_handler(CollectionChangedHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handler_ = handler;
    }

private:
    std::vector<T> items_;
    CollectionChangedHandler handler_;
    mutable std::mutex mutex_;
};

}
