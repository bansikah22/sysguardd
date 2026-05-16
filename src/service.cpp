#include "sysguardd/service.hpp"

#include <chrono>
#include <cctype>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace sysguardd {
namespace {

std::string trim(const std::string& s) {
  size_t start = 0;
  size_t end = s.size();
  while (start < end && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
    ++start;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
    --end;
  }
  return s.substr(start, end - start);
}

ProcessEvent parse_event_line(const std::string& line) {
  std::istringstream iss(line);
  ProcessEvent evt;
  if (!(iss >> evt.pid >> evt.ppid >> evt.exe)) {
    throw std::runtime_error("invalid event line format");
  }

  std::string arg;
  while (iss >> arg) {
    evt.args.push_back(arg);
  }
  evt.timestamp = std::chrono::system_clock::now();
  return evt;
}

std::string iso8601_now() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_t = std::chrono::system_clock::to_time_t(now);
  struct std::tm tm_buf;
  if (gmtime_r(&now_t, &tm_buf) == nullptr) {
    tm_buf = {};
  }
  std::ostringstream ts;
  ts << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
  return ts.str();
}

}  // namespace

// ---- Static helpers ---------------------------------------------------------

std::string Service::compute_severity(const Decision& d, Mode mode, bool enforce_failure) {
  if (enforce_failure) return "critical";
  if (d.allowed) return "info";
  if (mode == Mode::enforce) return "critical";
  return "warning";
}

std::string Service::format_event_id(const uint64_t counter) {
  std::ostringstream ss;
  ss << std::hex << std::setw(16) << std::setfill('0') << counter;
  return ss.str();
}

// ---- Service ----------------------------------------------------------------

Service::Service(const Mode mode, PolicyEngine policy, Mitigator& mitigator, AuditLogger& logger,
                 AlertDispatcher* dispatcher)
    : mode_(mode),
      policy_(std::move(policy)),
      mitigator_(mitigator),
      logger_(logger),
      dispatcher_(dispatcher) {}

void Service::run(std::istream& input) {
  std::string line;
  while (std::getline(input, line)) {
    if (line.size() > 4096) {
      std::cerr << "event dropped: line too long\n";
      continue;
    }

    const auto clean = trim(line);
    if (clean.empty()) {
      continue;
    }

    ProcessEvent event;
    try {
      event = parse_event_line(clean);
    } catch (const std::exception& ex) {
      std::cerr << "event parse error: " << ex.what() << '\n';
      continue;
    }

    const std::string event_id = format_event_id(event_counter_.fetch_add(1));
    const Decision decision = policy_.evaluate(event);

    if (decision.allowed) {
      logger_.log(event, decision, false, "", "", event_id, "info");
      continue;
    }

    bool enforced = false;
    std::string action;
    std::string action_error;

    if (mode_ == Mode::enforce) {
      action = "sigkill";
      try {
        // Note: TOCTOU race condition (CWE-367) exists between evaluation and mitigation.
        // Process could exit and PID could be reused between decision and kill().
        // Mitigator::kill_process() checks process existence to mitigate this.
        mitigator_.kill_process(event.pid);
        enforced = true;
      } catch (const std::exception& ex) {
        action_error = ex.what();
      }
    }

    const bool enforce_failure = !action_error.empty();
    const std::string severity = compute_severity(decision, mode_, enforce_failure);

    logger_.log(event, decision, enforced, action, action_error, event_id, severity);

    // Trigger alert for any deny event (monitor: warning, enforce: critical)
    if (dispatcher_ != nullptr) {
      AlertEvent alert;
      alert.event_id = event_id;
      alert.severity = severity;
      alert.timestamp = iso8601_now();
      alert.pid = event.pid;
      alert.ppid = event.ppid;
      alert.exe = event.exe;
      alert.reason = decision.reason;
      alert.action = action;
      dispatcher_->dispatch(std::move(alert));
    }
  }
}

}  // namespace sysguardd
