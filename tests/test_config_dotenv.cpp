#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <unistd.h>

#include "sysguardd/config.hpp"

namespace fs = std::filesystem;

int main() {
  const fs::path original_cwd = fs::current_path();
  const fs::path temp_dir = fs::temp_directory_path() / "sysguardd-dotenv-test";
  fs::create_directories(temp_dir);

  const fs::path dotenv_file = temp_dir / ".env";
  {
    std::ofstream out(dotenv_file);
    out << "SYSGUARDD_ALERT_WEBHOOK_URL=https://example.invalid/webhook\n";
  }

  if (chdir(temp_dir.c_str()) != 0) {
    throw std::runtime_error("failed to chdir to temp test directory");
  }

  try {
    char arg0[] = "sysguardd";
    char arg1[] = "daemon";
    char arg2[] = "--mode";
    char arg3[] = "enforce";
    char arg4[] = "--alert-enabled";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};

    const auto cfg = sysguardd::parse_config(5, argv);
    if (!cfg.alert.enabled) {
      throw std::runtime_error("expected alerting to be enabled");
    }
    if (cfg.alert.webhook_url != "https://example.invalid/webhook") {
      throw std::runtime_error("failed to load webhook URL from .env");
    }

    std::cout << "dotenv config test passed\n";
  } catch (...) {
    fs::current_path(original_cwd);
    throw;
  }

  fs::current_path(original_cwd);
  fs::remove_all(temp_dir);
  return 0;
}