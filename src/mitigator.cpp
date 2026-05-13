#include "sysguardd/mitigator.hpp"

#include <csignal>
#include <stdexcept>
#include <unistd.h>

namespace sysguardd {

void Mitigator::kill_process(const int pid) const {
  if (pid <= 1) {
    throw std::runtime_error("refusing to kill reserved pid");
  }
  if (pid == static_cast<int>(::getpid())) {
    throw std::runtime_error("refusing to self-kill");
  }
  if (::kill(pid, SIGKILL) != 0) {
    throw std::runtime_error("kill(2) failed");
  }
}

}  // namespace sysguardd
