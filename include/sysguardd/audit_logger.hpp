#pragma once

#include <iosfwd>
#include <mutex>
#include <string>

#include "sysguardd/event.hpp"
#include "sysguardd/policy_engine.hpp"

namespace sysguardd {

class AuditLogger {
 public:
  AuditLogger(std::ostream& out, std::string node_id, std::string policy_version);

  void log(const ProcessEvent& event, const Decision& decision, bool enforced,
           const std::string& action, const std::string& action_error,
           const std::string& event_id, const std::string& severity);

 private:
  std::ostream& out_;
  std::mutex mu_;
  std::string node_id_;
  std::string policy_version_;

  static std::string json_escape(const std::string& value);
};

}  // namespace sysguardd
