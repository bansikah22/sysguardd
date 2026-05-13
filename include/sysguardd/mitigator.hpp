#pragma once

namespace sysguardd {

class Mitigator {
 public:
  void kill_process(int pid) const;
};

}  // namespace sysguardd
