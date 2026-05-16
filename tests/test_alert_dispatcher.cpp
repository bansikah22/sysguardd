#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "sysguardd/alert_dispatcher.hpp"

namespace {

const char* get_env(const char* name) {
  const char* value = std::getenv(name);
  if (value == nullptr || *value == '\0') {
    return nullptr;
  }
  return value;
}

}  // namespace

int main() {
  const char* webhook_url = get_env("SYSGUARDD_TEST_WEBHOOK_URL");
  if (webhook_url == nullptr) {
    std::cout << "skipping alert dispatcher integration test: SYSGUARDD_TEST_WEBHOOK_URL not set\n";
    return 0;
  }

  sysguardd::AlertConfig cfg;
  cfg.enabled = true;
  cfg.webhook_url = webhook_url;
  cfg.min_severity = "warning";
  cfg.dedupe_window_sec = 0;
  cfg.rate_limit_per_minute = 60;

  sysguardd::AlertDispatcher dispatcher(cfg, "test-node", "test-policy");

  sysguardd::AlertEvent event;
  event.event_id = "0000000000000001";
  event.severity = "warning";
  event.timestamp = "2026-05-16T00:00:00Z";
  event.node_id = "test-node";
  event.policy_version = "test-policy";
  event.pid = 1234;
  event.ppid = 1;
  event.exe = "/bin/bash";
  event.reason = "deny_executable_match";
  event.action = "sigkill";

  dispatcher.dispatch(std::move(event));

  std::cout << "alert dispatcher integration test dispatched event\n";
  return 0;
}