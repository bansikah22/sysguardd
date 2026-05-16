#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <unistd.h>

#include "sysguardd/config.hpp"

namespace fs = std::filesystem;

int main() {
  try {
    const fs::path original_cwd = fs::current_path();
    const fs::path temp_dir = fs::temp_directory_path() / "sysguardd-dotenv-test";
    fs::create_directories(temp_dir);

    const char* original_env = std::getenv("SYSGUARDD_ALERT_WEBHOOK_URL");
    std::string original_env_value;
    if (original_env != nullptr) {
      original_env_value = original_env;
    }
    unsetenv("SYSGUARDD_ALERT_WEBHOOK_URL");

    const fs::path dotenv_file = temp_dir / ".env";
    {
      std::ofstream out(dotenv_file);
      out << "SYSGUARDD_ALERT_WEBHOOK_URL=https://example.invalid/webhook\n";
    }

    if (chdir(temp_dir.c_str()) != 0) {
      throw std::runtime_error("failed to chdir to temp test directory");
    }

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

    if (!original_env_value.empty()) {
      setenv("SYSGUARDD_ALERT_WEBHOOK_URL", original_env_value.c_str(), 1);
    } else {
      unsetenv("SYSGUARDD_ALERT_WEBHOOK_URL");
    }

    fs::current_path(original_cwd);
    fs::remove_all(temp_dir);

    std::cout << "dotenv config test passed\n";
    return 0;
  } catch (...) {
    std::cerr << "dotenv config test failed\n";
    return 1;
  }
}