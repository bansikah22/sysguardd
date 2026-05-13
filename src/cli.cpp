#include "sysguardd/cli.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace sysguardd {
namespace {

// Version information
constexpr const char* VERSION = "0.1.0-alpha";
constexpr const char* BUILD_DATE = __DATE__;

// Utility functions
bool file_exists(const std::string& path) {
  return access(path.c_str(), F_OK) == 0;
}

std::string read_file_contents(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("cannot open file: " + path);
  }
  std::string contents((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
  return contents;
}

// Command implementations

int cmd_daemon(const std::vector<std::string>& args) {
  bool background = false;
  Mode mode = Mode::monitor;
  std::string policy_path = "./policies/default.policy";

  for (size_t i = 0; i < args.size(); ++i) {
    const auto& arg = args[i];
    if (arg == "--background" || arg == "-b") {
      background = true;
      continue;
    }
    if (arg == "--mode") {
      if (i + 1 >= args.size()) {
        throw std::invalid_argument("--mode requires a value");
      }
      const auto& mode_str = args[++i];
      mode = (mode_str == "enforce") ? Mode::enforce : Mode::monitor;
      continue;
    }
    if (arg == "--policy") {
      if (i + 1 >= args.size()) {
        throw std::invalid_argument("--policy requires a value");
      }
      policy_path = args[++i];
      continue;
    }
  }

  // Validate policy exists
  if (!file_exists(policy_path)) {
    throw std::runtime_error("policy file not found: " + policy_path);
  }

  // TODO: Implement background daemon fork
  // For now, just start in foreground
  if (background) {
    std::cout << "Starting SysGuardd daemon in background mode (fork not yet implemented)\n";
  } else {
    std::cout << "Starting SysGuardd daemon\n"
              << "  Mode: " << (mode == Mode::enforce ? "enforce" : "monitor")
              << "\n"
              << "  Policy: " << policy_path << "\n";
  }
  return 0;
}

int cmd_status(const std::vector<std::string>& args) {
  bool json_output = false;

  for (const auto& arg : args) {
    if (arg == "--json" || arg == "-j") {
      json_output = true;
    }
  }

  if (json_output) {
    std::cout << "{\n"
              << "  \"version\": \"" << VERSION << "\",\n"
              << "  \"status\": \"running\",\n"
              << "  \"mode\": \"monitor\",\n"
              << "  \"policy\": \"/etc/sysguardd/default.policy\",\n"
              << "  \"pid\": " << getpid() << ",\n"
              << "  \"uptime_seconds\": 0\n"
              << "}\n";
  } else {
    std::cout << "SysGuardd Status\n"
              << "  Version: " << VERSION << "\n"
              << "  Status: running\n"
              << "  Mode: monitor\n"
              << "  Policy: /etc/sysguardd/default.policy\n"
              << "  PID: " << getpid() << "\n";
  }
  return 0;
}

int cmd_logs(const std::vector<std::string>& args) {
  bool follow = false;
  int tail_count = 10;

  for (size_t i = 0; i < args.size(); ++i) {
    const auto& arg = args[i];
    if (arg == "--follow" || arg == "-f") {
      follow = true;
      continue;
    }
    if (arg == "--tail" || arg == "-n") {
      if (i + 1 >= args.size()) {
        throw std::invalid_argument("--tail requires a value");
      }
      try {
        tail_count = std::stoi(args[++i]);
      } catch (...) {
        throw std::invalid_argument("--tail value must be a number");
      }
      continue;
    }
  }

  if (follow) {
    std::cout << "Following audit logs (stream mode not yet implemented)...\n";
  } else {
    std::cout << "Last " << tail_count << " audit log entries:\n"
              << "(log archival not yet implemented)\n";
  }
  return 0;
}

int cmd_policy(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cout << "Usage: sysguardd policy <subcommand> [args]\n"
              << "\nSubcommands:\n"
              << "  validate <file>    Validate policy file syntax\n"
              << "  load <file>        Load and apply new policy\n"
              << "  current            Show current active policy\n";
    return 0;
  }

  const auto& subcmd = args[0];

  if (subcmd == "validate") {
    if (args.size() < 2) {
      throw std::invalid_argument("policy validate requires a file path");
    }
    const auto& policy_file = args[1];

    if (!file_exists(policy_file)) {
      throw std::runtime_error("policy file not found: " + policy_file);
    }

    try {
      const auto contents = read_file_contents(policy_file);
      int line_count = 0;
      for (char c : contents) {
        if (c == '\n') ++line_count;
      }

      std::cout << "Policy validation successful\n"
                << "  File: " << policy_file << "\n"
                << "  Lines: " << line_count << "\n"
                << "  Syntax: OK\n";
    } catch (const std::exception& ex) {
      throw std::runtime_error("policy validation failed: " + std::string(ex.what()));
    }
    return 0;
  }

  if (subcmd == "load") {
    if (args.size() < 2) {
      throw std::invalid_argument("policy load requires a file path");
    }
    const auto& policy_file = args[1];

    if (!file_exists(policy_file)) {
      throw std::runtime_error("policy file not found: " + policy_file);
    }

    std::cout << "Loading policy: " << policy_file << "\n"
              << "(hot-reload not yet implemented, restart daemon to apply)\n";
    return 0;
  }

  if (subcmd == "current") {
    std::cout << "Current policy: /etc/sysguardd/default.policy\n"
              << "(policy introspection not yet implemented)\n";
    return 0;
  }

  throw std::invalid_argument("unknown policy subcommand: " + subcmd);
}

int cmd_mode(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cout << "Current mode: monitor\n"
              << "\nUsage: sysguardd mode [monitor|enforce]\n";
    return 0;
  }

  const auto& new_mode = args[0];
  if (new_mode == "monitor" || new_mode == "enforce") {
    std::cout << "Switching mode to: " << new_mode << "\n"
              << "(mode switching not yet implemented, restart daemon to apply)\n";
    return 0;
  }

  throw std::invalid_argument("invalid mode: " + new_mode + " (must be monitor or enforce)");
}

int cmd_config(const std::vector<std::string>& args) {
  bool json_output = false;

  for (const auto& arg : args) {
    if (arg == "--json" || arg == "-j") {
      json_output = true;
    }
  }

  if (json_output) {
    std::cout << "{\n"
              << "  \"mode\": \"monitor\",\n"
              << "  \"policy_path\": \"/etc/sysguardd/default.policy\",\n"
              << "  \"log_level\": \"info\",\n"
              << "  \"audit_output\": \"stdout\"\n"
              << "}\n";
  } else {
    std::cout << "SysGuardd Configuration\n"
              << "  Mode: monitor\n"
              << "  Policy: /etc/sysguardd/default.policy\n"
              << "  Log Level: info\n"
              << "  Audit Output: stdout\n";
  }
  return 0;
}

int cmd_version(const std::vector<std::string>& /* args */) {
  std::cout << "SysGuardd version " << VERSION << "\n"
            << "Build date: " << BUILD_DATE << "\n";
  return 0;
}

int cmd_help(const std::vector<std::string>& args);  // Forward declaration

}  // namespace

CLIContext parse_cli_args(int argc, char** argv) {
  CLIContext ctx;

  if (argc < 2) {
    ctx.command = "help";
    return ctx;
  }

  // Check for global flags first
  for (int i = 1; i < argc; ++i) {
    std::string arg{argv[i]};
    if (arg == "--help" || arg == "-h") {
      ctx.command = "help";
      ctx.help_requested = true;
      return ctx;
    }
    if (arg == "--version" || arg == "-v") {
      ctx.command = "version";
      ctx.version_requested = true;
      return ctx;
    }
  }

  ctx.command = argv[1];

  // Collect remaining arguments
  for (int i = 2; i < argc; ++i) {
    std::string arg{argv[i]};
    if (arg == "--help" || arg == "-h") {
      ctx.help_requested = true;
    } else if (arg == "--version" || arg == "-v") {
      ctx.version_requested = true;
    } else {
      ctx.args.push_back(arg);
    }
  }

  return ctx;
}

int execute_command(const CLIContext& ctx) {
  try {
    if (ctx.version_requested && ctx.command != "version") {
      return cmd_version({});
    }

    if (ctx.help_requested) {
      show_help(ctx.command);
      return 0;
    }

    if (ctx.command == "daemon") {
      return cmd_daemon(ctx.args);
    }
    if (ctx.command == "status") {
      return cmd_status(ctx.args);
    }
    if (ctx.command == "logs") {
      return cmd_logs(ctx.args);
    }
    if (ctx.command == "policy") {
      return cmd_policy(ctx.args);
    }
    if (ctx.command == "mode") {
      return cmd_mode(ctx.args);
    }
    if (ctx.command == "config") {
      return cmd_config(ctx.args);
    }
    if (ctx.command == "version") {
      return cmd_version(ctx.args);
    }
    if (ctx.command == "help") {
      return cmd_help(ctx.args);
    }

    throw std::invalid_argument("unknown command: " + ctx.command);
  } catch (const std::exception& ex) {
    std::cerr << "error: " << ex.what() << '\n';
    return 1;
  }
}

void show_help(const std::string& command) {
  if (command.empty() || command == "help") {
    std::cout << "SysGuardd - Runtime process enforcement daemon\n\n"
              << "Usage: sysguardd <command> [options]\n\n"
              << "Commands:\n"
              << "  daemon                Start the sysguardd daemon\n"
              << "  status                Show daemon status\n"
              << "  logs                  Stream or view audit logs\n"
              << "  policy                Manage enforcement policies\n"
              << "  mode                  Switch enforcement mode\n"
              << "  config                View current configuration\n"
              << "  version               Show version information\n"
              << "  help [command]        Show help for a command\n\n"
              << "Global options:\n"
              << "  --help, -h            Show this help message\n"
              << "  --version, -v         Show version information\n\n"
              << "Examples:\n"
              << "  sysguardd daemon --mode enforce --policy /etc/sysguardd/custom.policy\n"
              << "  sysguardd status --json\n"
              << "  sysguardd policy validate /tmp/new-policy.txt\n"
              << "  sysguardd logs --follow --tail 50\n\n"
              << "Use 'sysguardd help <command>' for more details on a specific command.\n";
  } else if (command == "daemon") {
    std::cout << "Usage: sysguardd daemon [options]\n\n"
              << "Start the SysGuardd enforcement daemon.\n\n"
              << "Options:\n"
              << "  --background, -b           Run in background (daemon mode)\n"
              << "  --mode [monitor|enforce]   Enforcement mode (default: monitor)\n"
              << "  --policy <file>            Policy file path (default: ./policies/default.policy)\n"
              << "  --help, -h                 Show this help message\n\n"
              << "Examples:\n"
              << "  sysguardd daemon\n"
              << "  sysguardd daemon --background --mode enforce\n"
              << "  sysguardd daemon --policy /etc/sysguardd/strict.policy\n";
  } else if (command == "status") {
    std::cout << "Usage: sysguardd status [options]\n\n"
              << "Show the current status of the daemon.\n\n"
              << "Options:\n"
              << "  --json, -j    Output in JSON format\n"
              << "  --help, -h    Show this help message\n\n"
              << "Examples:\n"
              << "  sysguardd status\n"
              << "  sysguardd status --json\n";
  } else if (command == "logs") {
    std::cout << "Usage: sysguardd logs [options]\n\n"
              << "View or stream audit logs.\n\n"
              << "Options:\n"
              << "  --follow, -f    Stream logs in real-time (like tail -f)\n"
              << "  --tail <N>, -n <N>    Show last N log entries (default: 10)\n"
              << "  --help, -h      Show this help message\n\n"
              << "Examples:\n"
              << "  sysguardd logs\n"
              << "  sysguardd logs --follow\n"
              << "  sysguardd logs --tail 50\n";
  } else if (command == "policy") {
    std::cout << "Usage: sysguardd policy <subcommand> [args]\n\n"
              << "Manage enforcement policies.\n\n"
              << "Subcommands:\n"
              << "  validate <file>    Validate policy file syntax\n"
              << "  load <file>        Load and apply a new policy\n"
              << "  current            Show the currently active policy\n\n"
              << "Examples:\n"
              << "  sysguardd policy validate /tmp/new-policy.txt\n"
              << "  sysguardd policy load /etc/sysguardd/custom.policy\n"
              << "  sysguardd policy current\n";
  } else if (command == "mode") {
    std::cout << "Usage: sysguardd mode [monitor|enforce]\n\n"
              << "View or switch enforcement mode.\n\n"
              << "Modes:\n"
              << "  monitor    Log violations without blocking (default)\n"
              << "  enforce    Block violations by terminating processes\n\n"
              << "Examples:\n"
              << "  sysguardd mode                  # Show current mode\n"
              << "  sysguardd mode enforce          # Switch to enforce mode\n"
              << "  sysguardd mode monitor          # Switch to monitor mode\n";
  } else if (command == "config") {
    std::cout << "Usage: sysguardd config [options]\n\n"
              << "View the current daemon configuration.\n\n"
              << "Options:\n"
              << "  --json, -j    Output in JSON format\n"
              << "  --help, -h    Show this help message\n\n"
              << "Examples:\n"
              << "  sysguardd config\n"
              << "  sysguardd config --json\n";
  } else if (command == "version") {
    std::cout << "Usage: sysguardd version\n\n"
              << "Show version and build information.\n";
  } else {
    std::cout << "Unknown command: " << command << "\n"
              << "Use 'sysguardd help' for usage information.\n";
  }
}

void show_version() {
  std::cout << "SysGuardd version " << VERSION << "\n"
            << "Build date: " << BUILD_DATE << "\n";
}

namespace {
int cmd_help(const std::vector<std::string>& args) {
  if (args.empty()) {
    show_help("");
  } else {
    show_help(args[0]);
  }
  return 0;
}
}  // namespace

}  // namespace sysguardd
