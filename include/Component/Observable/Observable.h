
#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <vector>

template <typename Event> class Observable {
public:
  using Callback = std::function<void(const Event &)>;
  using SubscriptionId = std::size_t;

  explicit Observable(bool threadSafe = false) : threadSafe_(threadSafe) {}

  // Subscribe returns a token you can use to unsubscribe
  SubscriptionId subscribe(Callback cb) {
    guard lock(threadSafe_ ? std::optional<std::unique_lock<std::mutex>>(
                                 std::unique_lock<std::mutex>(mtx_))
                           : std::optional<std::unique_lock<std::mutex>>{});
    SubscriptionId id = nextId_++;
    observers_.push_back({id, std::move(cb)});
    return id;
  }

  // Unsubscribe by token; returns true if removed
  bool unsubscribe(SubscriptionId id) {
    guard lock(threadSafe_ ? std::optional<std::unique_lock<std::mutex>>(
                                 std::unique_lock<std::mutex>(mtx_))
                           : std::optional<std::unique_lock<std::mutex>>{});
    auto it = std::remove_if(observers_.begin(), observers_.end(),
                             [id](const Observer &o) { return o.id == id; });
    bool removed = it != observers_.end();
    observers_.erase(it, observers_.end());
    return removed;
  }

  // Notify all observers with an event (push model)
  void notify(const Event &e) {
    // Copy callbacks to avoid iterator invalidation if someone unsubscribes
    // during notify
    std::vector<Callback> callbacks;
    {
      guard lock(threadSafe_ ? std::optional<std::unique_lock<std::mutex>>(
                                   std::unique_lock<std::mutex>(mtx_))
                             : std::optional<std::unique_lock<std::mutex>>{});
      callbacks.reserve(observers_.size());
      for (const auto &o : observers_) {
        callbacks.push_back(o.cb);
      }
    }
    for (auto &cb : callbacks) {
      cb(e);
    }
  }

  // Convenience: clear all observers
  void clear() {
    guard lock(threadSafe_ ? std::optional<std::unique_lock<std::mutex>>(
                                 std::unique_lock<std::mutex>(mtx_))
                           : std::optional<std::unique_lock<std::mutex>>{});
    observers_.clear();
  }

  // Count observers
  std::size_t size() const {
    guard lock(threadSafe_ ? std::optional<std::unique_lock<std::mutex>>(
                                 std::unique_lock<std::mutex>(mtx_))
                           : std::optional<std::unique_lock<std::mutex>>{});
    return observers_.size();
  }

private:
  struct Observer {
    SubscriptionId id;
    Callback cb;
  };

  // Tiny RAII helper to keep code clean when optional locking is enabled
  struct guard {
    std::optional<std::unique_lock<std::mutex>> &lk;
    guard(std::optional<std::unique_lock<std::mutex>> &&lock) : lk(lock) {}
    ~guard() = default;
  };

  mutable std::mutex mtx_;
  std::vector<Observer> observers_;
  SubscriptionId nextId_ = 1;
  bool threadSafe_;
};
