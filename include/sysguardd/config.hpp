#pragma once

#include <string>

namespace sysguardd {

enum class Mode {
  monitor,
  enforce,
};

struct AlertConfig {
  bool enabled{false};
  std::string webhook_url;
  std::string min_severity{"warning"};  // "info" | "warning" | "critical"
  int dedupe_window_sec{60};
  int rate_limit_per_minute{60};
};

struct Config {
  Mode mode{Mode::monitor};
  std::string policy_path{"./policies/default.policy"};
  std::string node_id;          // defaults to hostname
  std::string policy_version;   // optional version tag

  AlertConfig alert;
};

Config parse_config(int argc, char** argv);

}  // namespace sysguardd
