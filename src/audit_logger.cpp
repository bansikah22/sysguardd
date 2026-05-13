#include "sysguardd/audit_logger.hpp"

#include <chrono>
#include <cstdio>
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

  // Use thread-safe gmtime_r and check for NULL to prevent crashes (CWE-476)
  struct std::tm tm_buf;
  if (gmtime_r(&now_t, &tm_buf) == nullptr) {
    tm_buf = {};  // Fallback to epoch (1970-01-01)
  }

  std::ostringstream ts;
  ts << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");

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
  escaped.reserve(value.size() * 2);  // Reserve extra for unicode escapes

  for (unsigned char c : value) {
    // Escape control characters (0x00-0x1F) as \uXXXX to prevent JSON injection (CWE-116)
    if (c <= 0x1F) {
      char buf[7];
      snprintf(buf, sizeof(buf), "\\u%04x", c);
      escaped += buf;
      continue;
    }

    switch (c) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\x7F':  // DEL character
        escaped += "\\u007f";
        break;
      default:
        escaped += static_cast<char>(c);
    }
  }

  return escaped;
}

}  // namespace sysguardd
