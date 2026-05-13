#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "sysguardd/event.hpp"
#include "sysguardd/policy_engine.hpp"

namespace fs = std::filesystem;

int main() {
  const fs::path policy_path = fs::temp_directory_path() / "sysguardd_policy_test.policy";
  {
    std::ofstream out(policy_path);
    out << "deny_executable=/usr/bin/nc\n";
    out << "deny_prefix=/tmp/\n";
  }

  const auto engine = sysguardd::PolicyEngine::load_from_file(policy_path.string());

  const auto denied_exact = engine.evaluate(sysguardd::ProcessEvent{.pid = 10, .ppid = 1, .exe = "/usr/bin/nc"});
  if (denied_exact.allowed) {
    throw std::runtime_error("expected exact deny match");
  }

  const auto denied_prefix = engine.evaluate(sysguardd::ProcessEvent{.pid = 11, .ppid = 1, .exe = "/tmp/payload"});
  if (denied_prefix.allowed) {
    throw std::runtime_error("expected prefix deny match");
  }

  const auto allowed = engine.evaluate(sysguardd::ProcessEvent{.pid = 12, .ppid = 1, .exe = "/usr/bin/bash"});
  if (!allowed.allowed) {
    throw std::runtime_error("expected allow");
  }

  std::cout << "policy tests passed\n";
  fs::remove(policy_path);
  return 0;
}
