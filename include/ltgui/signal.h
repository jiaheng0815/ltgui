#pragma once
#include <functional>
#include <vector>
#include <algorithm>
#include <cassert>
#include <memory>

namespace ltgui {

namespace detail {
// Thread-local flag to detect re-entrant emit() on the same Signal.
// Stored as a pointer so each Signal instance gets its own flag without
// adding a member to every template instantiation that uses Signal inline.
extern thread_local const void* tls_emitting_signal;
} // namespace detail

template<typename... Args>
class Signal {
public:
    using Callback = std::function<void(Args...)>;

    Signal() : generation_(std::make_shared<int>(0)) {}
    ~Signal() {
        // Invalidate all ScopedConnections by incrementing the generation
        // counter. ScopedConnection stores a weak_ptr to this counter;
        // when it tries to disconnect(), it will see the generation has
        // changed and skip the call on the now-destroyed Signal.
        if (generation_) ++(*generation_);
    }

    // Non-copyable (ScopedConnection tracks this Signal via generation_ pointer)
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    // Movable: generation_ is transferred so ScopedConnections stay valid.
    // The moved-from Signal is left empty — its emit() will be a no-op.
    Signal(Signal&& other) noexcept
        : slots_(std::move(other.slots_))
        , pendingSlots_(std::move(other.pendingSlots_))
        , nextId_(other.nextId_)
        , emitting_(other.emitting_)
        , inCleanup_(other.inCleanup_)
        , emitDepth_(other.emitDepth_)
        , generation_(std::move(other.generation_)) {
        // The moved-from Signal should not be usable — reset its state.
        // We DON'T increment generation_ because the shared_ptr was moved intact.
        other.nextId_ = 1;
        other.emitting_ = false;
        other.inCleanup_ = false;
        other.emitDepth_ = 0;
    }

    Signal& operator=(Signal&& other) noexcept {
        if (this != &other) {
            // Invalidate our old ScopedConnections before replacing state
            if (generation_) ++(*generation_);
            slots_ = std::move(other.slots_);
            pendingSlots_ = std::move(other.pendingSlots_);
            nextId_ = other.nextId_;
            emitting_ = other.emitting_;
            inCleanup_ = other.inCleanup_;
            emitDepth_ = other.emitDepth_;
            generation_ = std::move(other.generation_);
            other.nextId_ = 1;
            other.emitting_ = false;
            other.inCleanup_ = false;
            other.emitDepth_ = 0;
        }
        return *this;
    }

    int connect(Callback cb) {
        int id = nextId_++;
        if (emitting_) {
            // During emission, new connections go into a pending list that
            // is merged after the current emit() completes. This prevents
            // the newly-connected slot from firing mid-emit (surprising
            // the caller who just connected it) and avoids iterator
            // invalidation.
            pendingSlots_.push_back({id, std::move(cb)});
        } else {
            slots_.push_back({id, std::move(cb)});
        }
        return id;
    }

    void disconnect(int id) {
        auto eraseFrom = [id](std::vector<Slot>& vec) -> bool {
            auto it = std::find_if(vec.begin(), vec.end(),
                                   [id](const Slot& s) { return s.id == id; });
            if (it != vec.end()) {
                it->cb = nullptr; // null the callback so emit skips it
                return true;
            }
            return false;
        };

        if (eraseFrom(slots_)) return;
        eraseFrom(pendingSlots_);
        // Don't physically erase during emit — cleanup happens after emit
        if (!emitting_ && !inCleanup_) {
            compactSlots();
            compactPending();
        }
    }

    void disconnectAll() {
        for (auto& s : slots_) s.cb = nullptr;
        for (auto& s : pendingSlots_) s.cb = nullptr;
        if (!emitting_ && !inCleanup_) {
            slots_.clear();
            pendingSlots_.clear();
        }
    }

    void emit(Args... args) {
        // Guard against recursive emit of the SAME Signal instance.
        // Recursive emit would:
        //   1. Re-use the same slots_ vector while iterating it
        //   2. Allow callbacks to see stale emitting_ state
        //   3. Risk double-firing newly-connected slots
        // We detect this and assert in debug; in release, we bail.
        assert(detail::tls_emitting_signal != this &&
               "Recursive Signal::emit() detected — a callback triggered emit() on the same Signal. "
               "Use a deferred/post mechanism instead.");
        if (detail::tls_emitting_signal == this) return;

        auto* prev = detail::tls_emitting_signal;
        detail::tls_emitting_signal = this;

        // Snapshot the current emission depth so nested emits on DIFFERENT
        // Signal instances can still work correctly.
        int snapshotDepth = emitDepth_;
        emitDepth_++;

        emitting_ = true;

        // Iterate over a SIZE snapshot so newly-added slots (in pendingSlots_)
        // are NOT fired during this emit.
        size_t count = slots_.size();
        for (size_t i = 0; i < count; ++i) {
            if (i >= slots_.size()) break; // slot erased by disconnect
            auto& slot = slots_[i];
            if (slot.cb) {
                slot.cb(args...);
            }
        }

        emitting_ = false;

        // Merge pending connections added during emit
        if (!pendingSlots_.empty()) {
            inCleanup_ = true;
            // Compact nulled slots before merging
            compactSlots();
            for (auto& s : pendingSlots_) {
                if (s.cb) {
                    slots_.push_back(std::move(s));
                }
            }
            pendingSlots_.clear();
            inCleanup_ = false;
        }

        // Clean up callbacks nulled during emission (only after all nesting unwinds)
        if (emitDepth_ == snapshotDepth + 1) {
            compactSlots();
            emitDepth_ = 0;
        } else {
            emitDepth_ = snapshotDepth;
        }

        detail::tls_emitting_signal = prev;
    }

    bool empty() const { return slots_.empty() && pendingSlots_.empty(); }
    size_t size() const {
        size_t n = 0;
        for (auto& s : slots_) { if (s.cb) n++; }
        for (auto& s : pendingSlots_) { if (s.cb) n++; }
        return n;
    }

    // Expose generation counter so ScopedConnection can detect Signal destruction.
    // Returns a weak_ptr: when the Signal is destroyed, the weak_ptr expires.
    std::weak_ptr<int> generationWeak() const { return generation_; }

private:
    struct Slot { int id; Callback cb; };
    std::vector<Slot> slots_;
    std::vector<Slot> pendingSlots_;
    int nextId_ = 1;
    bool emitting_ = false;
    bool inCleanup_ = false;
    int emitDepth_ = 0;

    // Shared generation counter. Signal holds the owning shared_ptr.
    // ScopedConnection stores a weak_ptr copy. When Signal is destroyed,
    // we increment the counter so any surviving ScopedConnection can
    // detect that its Signal is gone and skip the dangling disconnect().
    std::shared_ptr<int> generation_;

    void compactSlots() {
        slots_.erase(
            std::remove_if(slots_.begin(), slots_.end(),
                           [](const Slot& s) { return !s.cb; }),
            slots_.end());
    }

    void compactPending() {
        pendingSlots_.erase(
            std::remove_if(pendingSlots_.begin(), pendingSlots_.end(),
                           [](const Slot& s) { return !s.cb; }),
            pendingSlots_.end());
    }
};

template<typename... Args>
class ScopedConnection {
public:
    ScopedConnection() = default;
    ScopedConnection(Signal<Args...>* sig, int id)
        : signal_(sig),
          generation_(sig ? sig->generationWeak() : std::weak_ptr<int>()),
          id_(id) {}
    ~ScopedConnection() { disconnect(); }

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&& other) noexcept
        : signal_(other.signal_),
          generation_(std::move(other.generation_)),
          id_(other.id_) {
        other.signal_ = nullptr;
        other.id_ = -1;
        other.generation_.reset();
    }

    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            disconnect();
            signal_ = other.signal_;
            generation_ = std::move(other.generation_);
            id_ = other.id_;
            other.signal_ = nullptr;
            other.id_ = -1;
            other.generation_.reset();
        }
        return *this;
    }

    void disconnect() {
        if (signal_ && id_ >= 0) {
            // Safety check: verify the Signal is still alive via the
            // generation counter before touching the raw pointer.
            // If the weak_ptr is expired, the Signal has been destroyed
            // and signal_ is dangling — skip the call.
            if (!generation_.expired()) {
                signal_->disconnect(id_);
            }
            signal_ = nullptr;
            id_ = -1;
            generation_.reset();
        }
    }

    bool isConnected() const {
        return signal_ != nullptr && id_ >= 0 && !generation_.expired();
    }

private:
    Signal<Args...>* signal_ = nullptr;
    std::weak_ptr<int> generation_;
    int id_ = -1;
};

} // namespace ltgui
