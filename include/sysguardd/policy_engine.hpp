#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "sysguardd/event.hpp"

namespace sysguardd {

struct Decision {
  bool allowed{false};
  std::string reason;
};

class PolicyEngine {
 public:
  static PolicyEngine load_from_file(const std::string& path);

  [[nodiscard]] Decision evaluate(const ProcessEvent& event) const;

 private:
  std::unordered_set<std::string> deny_executables_;
  std::vector<std::string> deny_prefixes_;

  static std::string normalize_abs_path(std::string_view path);
};

}  // namespace sysguardd
