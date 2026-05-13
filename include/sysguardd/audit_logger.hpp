#pragma once

#include <iosfwd>
#include <mutex>
#include <string>

#include "sysguardd/event.hpp"
#include "sysguardd/policy_engine.hpp"

namespace sysguardd {

class AuditLogger {
 public:
  explicit AuditLogger(std::ostream& out);

  void log(const ProcessEvent& event, const Decision& decision, bool enforced,
           const std::string& action, const std::string& action_error);

 private:
  std::ostream& out_;
  std::mutex mu_;

  static std::string json_escape(const std::string& value);
};

}  // namespace sysguardd
