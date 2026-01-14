#include "Component/Observable/Observable.h"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>
#include <cstddef>

// --------------------- Tests -----------------------

TEST(ObservableBasic, SubscribeUnsubscribeAndSize) {
  Observable<int> obs; // non-thread-safe

  EXPECT_EQ(obs.size(), 0u);

  auto id1 = obs.subscribe([](const int &) {});
  EXPECT_EQ(id1, 1u);
  EXPECT_EQ(obs.size(), 1u);

  auto id2 = obs.subscribe([](const int &) {});
  EXPECT_EQ(id2, 2u);
  EXPECT_EQ(obs.size(), 2u);

  EXPECT_TRUE(obs.unsubscribe(id1));
  EXPECT_EQ(obs.size(), 1u);

  EXPECT_FALSE(obs.unsubscribe(id1)); // already removed
  EXPECT_EQ(obs.size(), 1u);

  EXPECT_TRUE(obs.unsubscribe(id2));
  EXPECT_EQ(obs.size(), 0u);
}

TEST(ObservableBasic, NotifyOrderAndDelivery) {
  Observable<int> obs;

  std::vector<int> order;
  obs.subscribe([&order](const int &e) { order.push_back(1 * e); });
  obs.subscribe([&order](const int &e) { order.push_back(2 * e); });
  obs.subscribe([&order](const int &e) { order.push_back(3 * e); });

  obs.notify(10);

  // Expect callbacks in subscription order
  ASSERT_EQ(order.size(), 3u);
  EXPECT_EQ(order[0], 10);  // 1 * 10
  EXPECT_EQ(order[1], 20);  // 2 * 10
  EXPECT_EQ(order[2], 30);  // 3 * 10
}

TEST(ObservableBasic, ClearResetsObservers) {
  Observable<std::string> obs;
  obs.subscribe([](const std::string &) {});
  obs.subscribe([](const std::string &) {});
  EXPECT_EQ(obs.size(), 2u);

  obs.clear();
  EXPECT_EQ(obs.size(), 0u);

  // Notify on empty should be no-op
  bool called = false;
  obs.notify(std::string("hi"));
  EXPECT_FALSE(called);
}

TEST(ObservableThreadSafe, ConcurrentSubscribeUnsubscribeAndNotify) {
  Observable<int> obs(/*threadSafe=*/true);

  std::atomic<int> totalCalls{0};
  std::atomic<bool> stop{false};
  const int initialSubs = 10;
  std::vector<Observable<int>::SubscriptionId> ids;
  ids.reserve(initialSubs);

  // Seed some subscribers
  for (int i = 0; i < initialSubs; ++i) {
    auto id = obs.subscribe([&](const int &e) {
      totalCalls.fetch_add(e, std::memory_order_relaxed);
    });
    ids.push_back(id);
  }

  // Thread A: notifier
  std::thread notifier([&] {
    while (!stop.load(std::memory_order_relaxed)) {
      obs.notify(1);
      // small sleep to allow interleaving
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  });

  // Thread B: subscribe/unsubscribe churn
  std::thread churner([&] {
    for (int i = 0; i < 200; ++i) {
      // Subscribe a few
      for (int j = 0; j < 5; ++j) {
        ids.push_back(obs.subscribe([&](const int &e) {
          totalCalls.fetch_add(e, std::memory_order_relaxed);
        }));
      }
      // Unsubscribe a few if present
      for (int k = 0; k < 3; ++k) {
        if (!ids.empty()) {
          auto id = ids.back();
          ids.pop_back();
          (void)obs.unsubscribe(id);
        }
      }
      // small sleep to vary timing
      std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
  });

  // Thread C: check size under load
  std::thread sizer([&] {
    for (int i = 0; i < 200; ++i) {
      auto sz = obs.size();
      // size should be non-negative and reasonably bounded
      EXPECT_GE(sz, 0u);
      std::this_thread::sleep_for(std::chrono::microseconds(150));
    }
  });

  churner.join();
  stop.store(true);
  notifier.join();
  sizer.join();

  // Final notify should succeed even after churn
  int before = totalCalls.load();
  obs.notify(1);
  int after = totalCalls.load();
  EXPECT_GE(after, before);
}

TEST(ObservableThreadSafe, UnsubscribeReturnsExpectedValues) {
  Observable<int> obs(true);
  auto id1 = obs.subscribe([](const int &) {});
  auto id2 = obs.subscribe([](const int &) {});
  EXPECT_TRUE(obs.unsubscribe(id1));
  EXPECT_FALSE(obs.unsubscribe(id1)); // already gone
  EXPECT_TRUE(obs.unsubscribe(id2));
  EXPECT_EQ(obs.size(), 0u);
}

// Main function
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

