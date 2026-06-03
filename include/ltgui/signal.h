#pragma once
#include <functional>
#include <vector>
#include <cstdint>

namespace ltgui {

// Lightweight single-shot signal. Each connection gets an ID;
// call disconnect(id) to remove it. Copying a Connection to the
// heap or capturing it in a lambda gives safe auto-disconnect.
//
// Usage:
//   Signal<int> onValueChanged;
//   int id = onValueChanged.connect([](int v) { ... });
//   onValueChanged.emit(42);
//   onValueChanged.disconnect(id);

template<typename... Args>
class Signal {
public:
    using Callback = std::function<void(Args...)>;

    Signal() = default;

    // Connect a callback. Returns a connection ID for later disconnect.
    int connect(Callback cb) {
        int id = nextId_++;
        slots_.push_back({id, std::move(cb)});
        return id;
    }

    // Disconnect a previously connected callback by its ID.
    void disconnect(int id) {
        for (auto it = slots_.begin(); it != slots_.end(); ++it) {
            if (it->id == id) {
                slots_.erase(it);
                return;
            }
        }
    }

    // Disconnect all callbacks.
    void disconnectAll() {
        slots_.clear();
    }

    // Fire all connected callbacks with the given arguments.
    void emit(Args... args) {
        // Copy slots so callbacks can safely connect/disconnect during emit
        auto copy = slots_;
        for (auto& slot : copy) {
            if (slot.cb) slot.cb(args...);
        }
    }

    bool empty() const { return slots_.empty(); }
    size_t size() const { return slots_.size(); }

private:
    struct Slot {
        int id;
        Callback cb;
    };
    std::vector<Slot> slots_;
    int nextId_ = 1;
};

// Scoped connection guard: disconnects automatically when destroyed.
// Capture this in your class to auto-disconnect on destruction.
template<typename... Args>
class ScopedConnection {
public:
    ScopedConnection() = default;
    ScopedConnection(Signal<Args...>* signal, int id) : signal_(signal), id_(id) {}
    ~ScopedConnection() { disconnect(); }

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
    ScopedConnection(ScopedConnection&& other) noexcept
        : signal_(other.signal_), id_(other.id_) {
        other.signal_ = nullptr;
        other.id_ = -1;
    }
    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) { disconnect(); signal_ = other.signal_; id_ = other.id_; other.signal_ = nullptr; other.id_ = -1; }
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
