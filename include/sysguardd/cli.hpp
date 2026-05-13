#pragma once

#include <string>
#include <vector>

#include "sysguardd/config.hpp"

namespace sysguardd {

/**
 * @brief Docker daemon-like CLI interface for SysGuardd
 *
 * Provides familiar subcommand patterns:
 *   sysguardd daemon [--background] [--mode monitor|enforce] [--policy <file>]
 *   sysguardd status [--json]
 *   sysguardd logs [--follow] [--tail N]
 *   sysguardd policy validate <file>
 *   sysguardd policy load <file>
 *   sysguardd mode [monitor|enforce]
 *   sysguardd config
 *   sysguardd version
 *   sysguardd help [command]
 */

struct CLIContext {
  std::string command;
  std::vector<std::string> args;
  bool help_requested{false};
  bool version_requested{false};
};

/**
 * @brief Parse CLI arguments into command context
 */
CLIContext parse_cli_args(int argc, char** argv);

/**
 * @brief Execute the requested command
 * @return Exit code (0 for success)
 */
int execute_command(const CLIContext& ctx);

/**
 * @brief Show help for a specific command or general help
 */
void show_help(const std::string& command = "");

/**
 * @brief Show version information
 */
void show_version();

}  // namespace sysguardd
