#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace sysguardd {

struct ProcessEvent {
  int pid{0};
  int ppid{0};
  std::string exe;
  std::vector<std::string> args;
  std::chrono::system_clock::time_point timestamp{};
};

}  // namespace sysguardd
