#pragma once

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace r3d {

template <typename T> class Observer {
public:
    virtual ~Observer() = default;
    virtual void on_next(const T &value) = 0;
};

template <typename T> class ObservableData {
public:
    void subscribe(Observer<T> *observer) {
        if (!observer)
            throw std::invalid_argument("Null observer.");
        if (std::find(observers_.begin(), observers_.end(), observer) != observers_.end())
            return;
        observers_.push_back(observer);
        if (value_)
            observer->on_next(*value_);
    }

    void unsubscribe(Observer<T> *observer) { std::erase(observers_, observer); }

    void set(T value) {
        value_ = std::move(value);
        for (Observer<T> *observer : observers_) {
            observer->on_next(*value_);
        }
    }

    const T &value() const { return value_.value(); }

private:
    std::vector<Observer<T> *> observers_;
    std::optional<T> value_;
};

} // namespace r3d
