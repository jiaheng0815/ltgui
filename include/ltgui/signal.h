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
// Using 'inline thread_local' (C++17) avoids a layering violation: no
// separate .cpp definition needed.
inline thread_local const void* tls_emitting_signal = nullptr;

// RAII guard that saves/restores emit state on the TLS slot and emitting_
// flag.  Using shared_ptr<bool> for emitting_ prevents use-after-free
// when a callback destroys the Signal during emission: the guard holds a
// shared reference, keeping the flag memory alive until the destructor runs.
// This also makes emit() exception-safe — even if a callback throws, the
// destructor always runs, so emitting_ and the TLS pointer are never left
// in a corrupted state.
struct ScopedEmitGuard {
    const void*&            tlsSlot;
    const void*             prev;
    std::shared_ptr<bool>   emitting;
    ScopedEmitGuard(const void*& slot, const void* self, std::shared_ptr<bool> em)
        : tlsSlot(slot), prev(slot), emitting(std::move(em)) {
        tlsSlot   = self;
        *emitting = true;
    }
    ~ScopedEmitGuard() {
        *emitting = false;
        tlsSlot   = prev;
    }
};
} // namespace detail

template<typename... Args>
class Signal {
public:
    using Callback = std::function<void(Args...)>;

    Signal() : emitting_(std::make_shared<bool>(false)), generation_(std::make_shared<int>(0)) {}
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
    // We invalidate the old generation_ so ScopedConnections pointing to the
    // moved-from Signal will see an expired weak_ptr and skip disconnect().
    Signal(Signal&& other) noexcept
        : slots_(std::move(other.slots_))
        , pendingSlots_(std::move(other.pendingSlots_))
        , nextId_(other.nextId_)
        , emitting_(std::move(other.emitting_))
        , generation_(other.generation_) {
        // Invalidate old generation so ScopedConnections to the moved-from
        // Signal see an expired weak_ptr (prevents UAF via dangling signal_).
        if (other.generation_) {
            ++(*other.generation_);
            other.generation_.reset();
        }
        // Assign a fresh generation for the moved-to Signal.
        generation_ = std::make_shared<int>(0);
        other.nextId_   = 1;
        other.emitting_ = std::make_shared<bool>(false);
    }

    Signal& operator=(Signal&& other) noexcept {
        if (this != &other) {
            // Invalidate our old ScopedConnections before replacing state
            if (generation_) ++(*generation_);
            slots_        = std::move(other.slots_);
            pendingSlots_ = std::move(other.pendingSlots_);
            nextId_       = other.nextId_;
            emitting_     = std::move(other.emitting_);
            // Transfer generation from source, then assign fresh generation
            generation_ = other.generation_;
            if (other.generation_) {
                ++(*other.generation_);
                other.generation_.reset();
            }
            generation_ = std::make_shared<int>(0);
            other.nextId_   = 1;
            other.emitting_ = std::make_shared<bool>(false);
        }
        return *this;
    }

    int connect(Callback cb) {
        int id = nextId_++;
        if (emitting_ && *emitting_) {
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

        if (eraseFrom(slots_)) {
            if (!emitting_ || !*emitting_) compactSlots();
            return;
        }
        if (eraseFrom(pendingSlots_)) {
            if (!emitting_ || !*emitting_) compactPending();
            return;
        }
    }

    void disconnectAll() {
        for (auto& s : slots_)        s.cb = nullptr;
        for (auto& s : pendingSlots_) s.cb = nullptr;
        if (!emitting_ || !*emitting_) {
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
               "Recursive Signal::emit() detected — a callback triggered emit() "
               "on the same Signal. Use a deferred/post mechanism instead.");
        if (detail::tls_emitting_signal == this) return;

        // RAII guard: saves TLS + emitting_, restores on scope exit.
        // This makes emit() exception-safe — even if a callback throws,
        // the state is never left corrupted.
        detail::ScopedEmitGuard guard(detail::tls_emitting_signal, this, emitting_);

        // Iterate over a SIZE snapshot so newly-added slots (in pendingSlots_)
        // are NOT fired during this emit.  Disconnect during emit nulls callbacks
        // but never erases (compactSlots is deferred until after the loop), so
        // slots_.size() is constant and we don't need a bounds re-check.
        size_t count = slots_.size();
        for (size_t i = 0; i < count; ++i) {
            auto& slot = slots_[i];
            if (slot.cb) {
                slot.cb(args...);
            }
        }
        // guard destructor runs here: emitting_ = false, TLS restored

        // Merge pending connections added during emit
        if (!pendingSlots_.empty()) {
            compactSlots(); // remove nulled before merging
            for (auto& s : pendingSlots_) {
                if (s.cb) {
                    slots_.push_back(std::move(s));
                }
            }
            pendingSlots_.clear();
        }

        // Clean up callbacks nulled during emission.
        // Because same-signal recursion is blocked (TLS guard above),
        // we are ALWAYS at the outermost emit for this Signal instance,
        // so it's always safe to compact here.
        compactSlots();
    }

    bool empty() const { return size() == 0; }
    size_t size() const {
        size_t n = 0;
        for (auto& s : slots_)        { if (s.cb) n++; }
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
    int                      nextId_   = 1;
    std::shared_ptr<bool>    emitting_;

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
            signal_     = other.signal_;
            generation_ = std::move(other.generation_);
            id_         = other.id_;
            other.signal_ = nullptr;
            other.id_     = -1;
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
            id_     = -1;
            generation_.reset();
        }
    }

    bool isConnected() const {
        return signal_ != nullptr && id_ >= 0 && !generation_.expired();
    }

private:
    Signal<Args...>*    signal_ = nullptr;
    std::weak_ptr<int>  generation_;
    int                 id_ = -1;
};

} // namespace ltgui
