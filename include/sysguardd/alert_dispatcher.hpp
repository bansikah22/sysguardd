#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "sysguardd/config.hpp"

namespace sysguardd {

// Normalized alert event sent to the webhook dispatcher.
struct AlertEvent {
  std::string event_id;
  std::string severity;    // "info" | "warning" | "critical"
  std::string timestamp;   // ISO8601
  std::string node_id;
  std::string policy_version;
  int pid{0};
  int ppid{0};
  std::string exe;
  std::string reason;
  std::string action;
};

// Non-blocking, thread-safe dispatcher that forwards security alert events to
// a configured webhook endpoint (Slack / Teams compatible JSON payload).
// Includes per-key deduplication and per-minute rate limiting to prevent
// alert storms from crashing downstream notification services.
class AlertDispatcher {
 public:
  AlertDispatcher(const AlertConfig& cfg, std::string node_id, std::string policy_version);
  ~AlertDispatcher();

  AlertDispatcher(const AlertDispatcher&) = delete;
  AlertDispatcher& operator=(const AlertDispatcher&) = delete;

  // Queue an alert. Returns immediately; never blocks the caller.
  // Events below min_severity or exceeding queue capacity are silently dropped.
  void dispatch(AlertEvent event);

 private:
  static constexpr size_t kMaxQueueSize = 128;
  static constexpr int kLevelInfo = 0;
  static constexpr int kLevelWarning = 1;
  static constexpr int kLevelCritical = 2;

  static int severity_level(const std::string& s);

  // Called only from worker thread — no locking needed.
  bool should_send(const AlertEvent& evt);
  void send_webhook(const AlertEvent& evt);
  void worker_loop();

  AlertConfig cfg_;
  std::string node_id_;
  std::string policy_version_;
  int min_level_{kLevelWarning};

  std::deque<AlertEvent> queue_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::thread worker_;
  bool stop_{false};

  // Deduplication state (worker thread only)
  std::unordered_map<std::string, std::chrono::system_clock::time_point> dedupe_map_;

  // Token-bucket rate-limit state (worker thread only)
  int tokens_{0};
  std::chrono::system_clock::time_point last_refill_;
};

}  // namespace sysguardd
