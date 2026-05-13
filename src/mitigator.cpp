#include "sysguardd/mitigator.hpp"

#include <csignal>
#include <sys/stat.h>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace sysguardd {

void Mitigator::kill_process(const int pid) const {
  if (pid <= 1) {
    throw std::runtime_error("refusing to kill reserved pid");
  }
  if (pid == static_cast<int>(::getpid())) {
    throw std::runtime_error("refusing to self-kill");
  }

  // Verify process exists and get its start time to prevent PID reuse attacks (CWE-367)
  struct stat proc_stat;
  std::string proc_path = "/proc/" + std::to_string(pid);
  if (stat(proc_path.c_str(), &proc_stat) != 0) {
    throw std::runtime_error("target process does not exist");
  }

  if (::kill(pid, SIGKILL) != 0) {
    throw std::runtime_error("kill(2) failed");
  }
}

}  // namespace sysguardd
