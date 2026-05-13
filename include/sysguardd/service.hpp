#pragma once

#include <istream>

#include "sysguardd/audit_logger.hpp"
#include "sysguardd/config.hpp"
#include "sysguardd/mitigator.hpp"
#include "sysguardd/policy_engine.hpp"

namespace sysguardd {

class Service {
 public:
  Service(Mode mode, PolicyEngine policy, Mitigator& mitigator, AuditLogger& logger);

  // Event line format:
  // PID PPID EXE [ARG ...]
  // Example:
  // 1200 1 /usr/bin/bash -c whoami
  void run(std::istream& input);

 private:
  Mode mode_;
  PolicyEngine policy_;
  Mitigator& mitigator_;
  AuditLogger& logger_;
};

}  // namespace sysguardd
