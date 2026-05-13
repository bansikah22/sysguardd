#include "sysguardd/audit_logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace sysguardd {

AuditLogger::AuditLogger(std::ostream& out) : out_(out) {}

void AuditLogger::log(const ProcessEvent& event, const Decision& decision, const bool enforced,
                      const std::string& action, const std::string& action_error) {
  std::lock_guard<std::mutex> lock(mu_);

  const auto now = std::chrono::system_clock::now();
  const std::time_t now_t = std::chrono::system_clock::to_time_t(now);

  std::ostringstream ts;
  ts << std::put_time(std::gmtime(&now_t), "%Y-%m-%dT%H:%M:%SZ");

  out_ << "{"
       << "\"timestamp\":\"" << ts.str() << "\","
       << "\"pid\":" << event.pid << ","
       << "\"ppid\":" << event.ppid << ","
       << "\"exe\":\"" << json_escape(event.exe) << "\","
       << "\"decision\":\"" << (decision.allowed ? "allow" : "deny") << "\","
       << "\"reason\":\"" << json_escape(decision.reason) << "\","
       << "\"enforced\":" << (enforced ? "true" : "false") << ","
       << "\"action\":\"" << json_escape(action) << "\","
       << "\"action_error\":\"" << json_escape(action_error) << "\""
       << "}"
       << '\n';
}

std::string AuditLogger::json_escape(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());

  for (const char c : value) {
    switch (c) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += c;
    }
  }

  return escaped;
}

}  // namespace sysguardd
