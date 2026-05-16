#include "sysguardd/config.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <limits>
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

std::string trim(const std::string& value) {
  size_t start = 0;
  size_t end = value.size();
  while (start < end && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
    ++start;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    --end;
  }
  return value.substr(start, end - start);
}

void load_dotenv_file(const char* path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::string clean = trim(line);
    if (clean.empty() || clean[0] == '#') {
      continue;
    }
    if (clean.rfind("export ", 0) == 0) {
      clean = trim(clean.substr(7));
    }

    const auto eq_pos = clean.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    const std::string key = trim(clean.substr(0, eq_pos));
    std::string value = trim(clean.substr(eq_pos + 1));
    if (key.empty()) {
      continue;
    }

    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
      value = value.substr(1, value.size() - 2);
    }

    if (std::getenv(key.c_str()) == nullptr) {
      setenv(key.c_str(), value.c_str(), 0);
    }
  }
}

int parse_int_arg(const std::string& value, const std::string& flag_name, int min_value,
                  int max_value) {
  size_t pos = 0;
  long long parsed = 0;
  try {
    parsed = std::stoll(value, &pos, 10);
  } catch (const std::exception&) {
    throw std::invalid_argument(flag_name + " requires a valid integer");
  }

  if (pos != value.size()) {
    throw std::invalid_argument(flag_name + " requires a valid integer");
  }
  if (parsed < min_value || parsed > max_value) {
    throw std::invalid_argument(flag_name + " is out of range");
  }
  return static_cast<int>(parsed);
}

}  // namespace

Config parse_config(int argc, char** argv) {
  Config cfg{};

  load_dotenv_file(".env");

  // Default node_id to system hostname
  {
    char hostname[256] = {};
    if (gethostname(hostname, sizeof(hostname) - 1) == 0) {
      cfg.node_id = hostname;
    }
  }

  if (const char* webhook_url = std::getenv("SYSGUARDD_ALERT_WEBHOOK_URL");
      webhook_url != nullptr && *webhook_url != '\0') {
    cfg.alert.webhook_url = webhook_url;
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
      cfg.alert.dedupe_window_sec =
          parse_int_arg(argv[++i], "--alert-dedupe-window", 0, std::numeric_limits<int>::max());
      continue;
    }
    if (arg == "--alert-rate-limit") {
      if (i + 1 >= argc) {
        throw std::invalid_argument("--alert-rate-limit requires a value");
      }
      cfg.alert.rate_limit_per_minute =
          parse_int_arg(argv[++i], "--alert-rate-limit", 1, std::numeric_limits<int>::max());
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

  if (cfg.alert.enabled && cfg.alert.webhook_url.empty()) {
    throw std::invalid_argument(
        "alert webhook URL must be set via --alert-webhook-url or SYSGUARDD_ALERT_WEBHOOK_URL");
  }

  return cfg;
}

}  // namespace sysguardd
