#include "sysguardd/config.hpp"

#include <stdexcept>
#include <string>

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

  for (int i = 1; i < argc; ++i) {
    std::string arg{argv[i]};
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
    if (arg == "--help" || arg == "-h") {
      throw std::invalid_argument(
          "usage: sysguardd [--mode monitor|enforce] [--policy path]");
    }
    throw std::invalid_argument("unknown argument: " + arg);
  }

  if (cfg.policy_path.empty()) {
    throw std::invalid_argument("policy path must not be empty");
  }

  return cfg;
}

}  // namespace sysguardd
