#pragma once

#include <string>

namespace sysguardd {

enum class Mode {
  monitor,
  enforce,
};

struct Config {
  Mode mode{Mode::monitor};
  std::string policy_path{"./policies/default.policy"};
};

Config parse_config(int argc, char** argv);

}  // namespace sysguardd
