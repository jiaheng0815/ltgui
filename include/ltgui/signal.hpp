#pragma once
#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace ltgui {

template <typename... Args> class Signal {
public:
  using Callback = std::function<void(Args...)>;

  Signal() : state_(std::make_shared<State>()),
             generation_(std::make_shared<int>(0)) {}

  // Slot lists and emission state live in one shared block. emit() keeps a
  // local reference to the block for the whole fan-out, so even if a
  // callback destroys this Signal mid-emission, the loop data (slots,
  // pending list, flags) stays alive for the remaining callbacks and the
  // post-emit cleanup — no access to freed memory (B2-01).
  ~Signal() {
    // Invalidate all ScopedConnections by incrementing the generation
    // counter. ScopedConnection stores a weak_ptr to this counter;
    // when it tries to disconnect(), it will see the generation has
    // changed and skip the call on the now-destroyed Signal.
    if (generation_)
      ++(*generation_);
  }

  // Non-copyable (ScopedConnection tracks this Signal via generation_ pointer)
  Signal(const Signal &) = delete;
  Signal &operator=(const Signal &) = delete;

  // Movable: the slot list is transferred, generation_ is replaced so
  // ScopedConnections stay valid (moved-to) / expire (moved-from).
  // The moved-from Signal is left empty — its emit() will be a no-op.
  Signal(Signal &&other) noexcept {
    state_ = std::make_shared<State>();
    if (other.state_) {
      state_->slots = std::move(other.state_->slots);
      state_->pendingSlots = std::move(other.state_->pendingSlots);
      state_->nextId = other.state_->nextId;
      other.state_->nextId = 1;
    }
    // Invalidate old generation so ScopedConnections to the moved-from
    // Signal see an expired weak_ptr (prevents UAF via dangling signal_).
    if (other.generation_) {
      ++(*other.generation_);
      other.generation_.reset();
    }
    // Fresh generation for the moved-to Signal.
    generation_ = std::make_shared<int>(0);
  }

  Signal &operator=(Signal &&other) noexcept {
    if (this != &other) {
      // Invalidate our old ScopedConnections before replacing state
      if (generation_)
        ++(*generation_);
      state_ = std::make_shared<State>();
      if (other.state_) {
        state_->slots = std::move(other.state_->slots);
        state_->pendingSlots = std::move(other.state_->pendingSlots);
        state_->nextId = other.state_->nextId;
        other.state_->nextId = 1;
      }
      // Transfer generation from source, then assign fresh generation
      if (other.generation_) {
        ++(*other.generation_);
        other.generation_.reset();
      }
      generation_ = std::make_shared<int>(0);
    }
    return *this;
  }

  int connect(Callback cb) {
    int id = state_->nextId++;
    if (state_->emitting && *state_->emitting) {
      // During emission, new connections go into a pending list that
      // is merged after the current emit() completes. This prevents
      // the newly-connected slot from firing mid-emit (surprising
      // the caller who just connected it) and avoids iterator
      // invalidation.
      state_->pendingSlots.push_back({id, std::move(cb)});
    } else {
      state_->slots.push_back({id, std::move(cb)});
    }
    return id;
  }

  void disconnect(int id) {
    auto eraseFrom = [id](std::vector<Slot> &vec) -> bool {
      auto it = std::find_if(vec.begin(), vec.end(),
                             [id](const Slot &s) { return s.id == id; });
      if (it != vec.end()) {
        it->cb = nullptr; // null the callback so emit skips it
        return true;
      }
      return false;
    };

    if (eraseFrom(state_->slots)) {
      if (!state_->emitting || !*state_->emitting)
        compactSlots(state_->slots);
      return;
    }
    if (eraseFrom(state_->pendingSlots)) {
      if (!state_->emitting || !*state_->emitting)
        compactPending(state_->pendingSlots);
      return;
    }
  }

  void disconnectAll() {
    for (auto &s : state_->slots)
      s.cb = nullptr;
    for (auto &s : state_->pendingSlots)
      s.cb = nullptr;
    if (!state_->emitting || !*state_->emitting) {
      state_->slots.clear();
      state_->pendingSlots.clear();
    }
  }

  void emit(Args... args) {
    // Take the shared reference up-front: even if a callback destroys
    // this Signal (or it gets moved-from) during the emission, the slot
    // list stays alive until the fan-out and cleanup are done (B2-01).
    auto st = state_;
    assert(st->recursionDepth == 0 &&
           "Recursive Signal::emit() detected — a callback triggered emit() "
           "on the same Signal (possibly via a chain A->B->A). Use a "
           "deferred/post mechanism instead.");
    // Same-instance re-entrance guard (per-Signal depth counter, B2-02):
    // the outer emit is already iterating its own stable slot snapshot, so a
    // nested emit of this Signal bails out — it must not re-fire the slots,
    // merge pending connections, or compact the list mid-iteration.
    if (st->recursionDepth > 0)
      return;
    EmitGuard guard(*st, true);

    // Iterate over a SIZE snapshot so newly-added slots (in pendingSlots_)
    // are NOT fired during this emit.  Disconnect during emit nulls callbacks
    // but never erases (compactSlots is deferred until after the loop), so
    // slots_.size() is constant and we don't need a bounds re-check.
    size_t count = st->slots.size();
    for (size_t i = 0; i < count; ++i) {
      auto &slot = st->slots[i];
      if (slot.cb) {
        slot.cb(args...);
      }
    }
    // guard destructor runs here: emitting_ = false, depth restored

    // Merge pending connections added during emit
    if (!st->pendingSlots.empty()) {
      compactSlots(st->slots); // remove nulled before merging
      for (auto &s : st->pendingSlots) {
        if (s.cb) {
          st->slots.push_back(std::move(s));
        }
      }
      st->pendingSlots.clear();
    }

    // Clean up callbacks nulled during emission.
    // Because same-signal recursion is blocked (depth guard above),
    // we are ALWAYS at the outermost emit for this Signal instance,
    // so it's always safe to compact here.
    compactSlots(st->slots);
  }

  // Emit a snapshot of the currently-connected slots. Unlike emit(), the
  // Signal's own connection list is not touched: the slots are copied out
  // first, so the fan-out runs safely even if a slot destroys the Signal
  // (or its owner), and the connections remain installed for the next fire
  // (B2-04). Pending slots (from an in-progress emit) are not included, and
  // emitting_ is not set — so a connect() made by a slot lands directly in
  // the slot list and fires on the next emitCopy/emit.
  void emitCopy(Args... args) {
    auto st = state_;
    assert(st->recursionDepth == 0 &&
           "Recursive Signal::emitCopy() detected — a callback triggered "
           "emitCopy() on the same Signal. Use a deferred/post mechanism "
           "instead.");
    if (st->recursionDepth > 0)
      return;
    EmitGuard guard(*st, false);

    std::vector<Slot> copy(st->slots);
    for (auto &slot : copy) {
      if (slot.cb) {
        slot.cb(args...);
      }
    }
    // guard destructor runs here: depth restored
  }

  bool empty() const { return size() == 0; }
  size_t size() const {
    size_t n = 0;
    for (auto &s : state_->slots) {
      if (s.cb)
        n++;
    }
    for (auto &s : state_->pendingSlots) {
      if (s.cb)
        n++;
    }
    return n;
  }

  // Expose generation counter so ScopedConnection can detect Signal
  // destruction. Returns a weak_ptr: when the Signal is destroyed, the weak_ptr
  // expires.
  std::weak_ptr<int> generationWeak() const { return generation_; }

private:
  struct Slot {
    int id;
    Callback cb;
  };

  // Shared state block (see constructor comment): slot lists plus the
  // emission flags. Keep emitting_ as a shared_ptr<bool> so the flag memory
  // outlives the Signal while an emit is in flight.
  struct State {
    std::vector<Slot> slots;
    std::vector<Slot> pendingSlots;
    int nextId = 1;
    std::shared_ptr<bool> emitting = std::make_shared<bool>(false);
    int recursionDepth = 0;
  };
  std::shared_ptr<State> state_;

  // Shared generation counter. Signal holds the owning shared_ptr.
  // ScopedConnection stores a weak_ptr copy. When Signal is destroyed,
  // we increment the counter so any surviving ScopedConnection can
  // detect that its Signal is gone and skip the dangling disconnect().
  std::shared_ptr<int> generation_;

  // RAII guard for an in-flight emit/emitCopy: increments the per-Signal
  // recursion depth, marks emitting_ (only in emit(), where the pending list
  // is merged afterwards), and restores both on scope exit — exception-safe
  // even if a callback throws.
  struct EmitGuard {
    State &st;
    bool markEmitting;
    EmitGuard(State &s, bool mark) : st(s), markEmitting(mark) {
      st.recursionDepth++;
      if (markEmitting)
        *st.emitting = true;
    }
    ~EmitGuard() {
      if (markEmitting)
        *st.emitting = false;
      st.recursionDepth--;
    }
  };

  static void compactSlots(std::vector<Slot> &vec) {
    vec.erase(std::remove_if(vec.begin(), vec.end(),
                             [](const Slot &s) { return !s.cb; }),
              vec.end());
  }

  static void compactPending(std::vector<Slot> &vec) { compactSlots(vec); }
};

template <typename... Args> class ScopedConnection {
public:
  ScopedConnection() = default;
  ScopedConnection(Signal<Args...> *sig, int id)
      : signal_(sig),
        generation_(sig ? sig->generationWeak() : std::weak_ptr<int>()),
        id_(id) {}
  // Convenience: connect immediately; disconnects automatically on destruction.
  ScopedConnection(Signal<Args...> *sig, typename Signal<Args...>::Callback cb)
      : ScopedConnection(sig, sig ? sig->connect(std::move(cb)) : -1) {}
  ~ScopedConnection() { disconnect(); }

  ScopedConnection(const ScopedConnection &) = delete;
  ScopedConnection &operator=(const ScopedConnection &) = delete;

  ScopedConnection(ScopedConnection &&other) noexcept
      : signal_(other.signal_), generation_(std::move(other.generation_)),
        id_(other.id_) {
    other.signal_ = nullptr;
    other.id_ = -1;
    other.generation_.reset();
  }

  ScopedConnection &operator=(ScopedConnection &&other) noexcept {
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
  Signal<Args...> *signal_ = nullptr;
  std::weak_ptr<int> generation_;
  int id_ = -1;
};

} // namespace ltgui
