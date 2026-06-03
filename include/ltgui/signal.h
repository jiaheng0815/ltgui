#pragma once
#include <functional>
#include <vector>
#include <algorithm>

namespace ltgui {

template<typename... Args>
class Signal {
public:
    using Callback = std::function<void(Args...)>;

    int connect(Callback cb) {
        int id = nextId_++;
        slots_.push_back({id, std::move(cb)});
        return id;
    }

    void disconnect(int id) {
        auto it = std::find_if(slots_.begin(), slots_.end(),
                               [id](const Slot& s) { return s.id == id; });
        if (it == slots_.end()) return;
        if (emitting_) {
            it->cb = nullptr; // defer erase until emit completes
        } else {
            slots_.erase(it);
        }
    }

    void disconnectAll() {
        if (emitting_) {
            for (auto& s : slots_) s.cb = nullptr;
        } else {
            slots_.clear();
        }
    }

    void emit(Args... args) {
        emitting_ = true;
        for (auto& slot : slots_) {
            if (slot.cb) slot.cb(args...);
        }
        emitting_ = false;
        // Clean up callbacks nulled during emission
        slots_.erase(
            std::remove_if(slots_.begin(), slots_.end(),
                           [](const Slot& s) { return !s.cb; }),
            slots_.end());
    }

    bool empty() const { return slots_.empty(); }
    size_t size() const { return slots_.size(); }

private:
    struct Slot { int id; Callback cb; };
    std::vector<Slot> slots_;
    int nextId_ = 1;
    bool emitting_ = false;
};

template<typename... Args>
class ScopedConnection {
public:
    ScopedConnection() = default;
    ScopedConnection(Signal<Args...>* sig, int id) : signal_(sig), id_(id) {}
    ~ScopedConnection() { disconnect(); }

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&& other) noexcept
        : signal_(other.signal_), id_(other.id_) {
        other.signal_ = nullptr;
        other.id_ = -1;
    }

    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            disconnect();
            signal_ = other.signal_;
            id_ = other.id_;
            other.signal_ = nullptr;
            other.id_ = -1;
        }
        return *this;
    }

    void disconnect() {
        if (signal_ && id_ >= 0) {
            signal_->disconnect(id_);
            signal_ = nullptr;
            id_ = -1;
        }
    }

    bool isConnected() const { return signal_ != nullptr && id_ >= 0; }

private:
    Signal<Args...>* signal_ = nullptr;
    int id_ = -1;
};

} // namespace ltgui
