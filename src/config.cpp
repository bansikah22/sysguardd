#include "sysguardd/config.hpp"

#include <stdexcept>
#include <string>
#include <unistd.h>

namespace sysguardd {
namespace {

Mode parse_mode(const std::string& mode) {
  if (mode == "monitor") {
    return Mode::monitor;
  }
  if (mode == "enforce") {
    return Mode::enforce;
  }
  throw std::invalid_argument("invalid mode: " + mode);
}

}  // namespace

Config parse_config(int argc, char** argv) {
  Config cfg{};

  // Default node_id to system hostname
  {
    char hostname[256] = {};
    if (gethostname(hostname, sizeof(hostname) - 1) == 0) {
      cfg.node_id = hostname;
    }
  }

  for (int i = 1; i < argc; ++i) {
    std::string arg{argv[i]};
    if (arg == "daemon") {
      continue;
    }
    if (arg == "--mode") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--mode requires a value");
      }
      cfg.mode = parse_mode(argv[++i]);
      continue;
    }
    if (arg == "--policy") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--policy requires a value");
      }
      cfg.policy_path = argv[++i];
      continue;
    }
    if (arg == "--node-id") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--node-id requires a value");
      }
      cfg.node_id = argv[++i];
      continue;
    }
    if (arg == "--policy-version") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--policy-version requires a value");
      }
      cfg.policy_version = argv[++i];
      continue;
    }
    if (arg == "--alert-enabled") {
      cfg.alert.enabled = true;
      continue;
    }
    if (arg == "--alert-webhook-url") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--alert-webhook-url requires a value");
      }
      cfg.alert.webhook_url = argv[++i];
      continue;
    }
    if (arg == "--alert-min-severity") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--alert-min-severity requires a value");
      }
      const std::string sev{argv[++i]};
      if (sev != "info" && sev != "warning" && sev != "critical") {
        throw std::invalid_argument("--alert-min-severity must be info, warning, or critical");
      }
      cfg.alert.min_severity = sev;
      continue;
    }
    if (arg == "--alert-dedupe-window") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--alert-dedupe-window requires a value");
      }
      cfg.alert.dedupe_window_sec = std::stoi(argv[++i]);
      continue;
    }
    if (arg == "--alert-rate-limit") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--alert-rate-limit requires a value");
      }
      cfg.alert.rate_limit_per_minute = std::stoi(argv[++i]);
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      throw std::invalid_argument(
          "usage: sysguardd [--mode monitor|enforce] [--policy path]\n"
          "                  [--node-id id] [--policy-version ver]\n"
          "                  [--alert-enabled] [--alert-webhook-url url]\n"
          "                  [--alert-min-severity info|warning|critical]\n"
          "                  [--alert-dedupe-window sec] [--alert-rate-limit N]");
    }
    throw std::invalid_argument("unknown argument: " + arg);
  }

  if (cfg.policy_path.empty()) {
    throw std::invalid_argument("policy path must not be empty");
  }

  return cfg;
}

}  // namespace sysguardd
