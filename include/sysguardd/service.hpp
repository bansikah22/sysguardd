#pragma once

#include <atomic>
#include <istream>

#include "sysguardd/alert_dispatcher.hpp"
#include "sysguardd/audit_logger.hpp"
#include "sysguardd/config.hpp"
#include "sysguardd/mitigator.hpp"
#include "sysguardd/policy_engine.hpp"

namespace sysguardd {

class Service {
 public:
  // dispatcher may be nullptr when alerting is disabled.
  Service(Mode mode, PolicyEngine policy, Mitigator& mitigator, AuditLogger& logger,
          AlertDispatcher* dispatcher = nullptr);

  // Event line format:
  // PID PPID EXE [ARG ...]
  // Example:
  // 1200 1 /usr/bin/bash -c whoami
  void run(std::istream& input);

 private:
  static std::string compute_severity(const Decision& d, Mode mode, bool enforce_failure);
  static std::string format_event_id(uint64_t counter);

  Mode mode_;
  PolicyEngine policy_;
  Mitigator& mitigator_;
  AuditLogger& logger_;
  AlertDispatcher* dispatcher_;            // non-owning, nullable
  std::atomic<uint64_t> event_counter_{0};
};

}  // namespace sysguardd
