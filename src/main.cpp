#include <iostream>
#include <stdexcept>

#include "sysguardd/audit_logger.hpp"
#include "sysguardd/cli.hpp"
#include "sysguardd/config.hpp"
#include "sysguardd/mitigator.hpp"
#include "sysguardd/policy_engine.hpp"
#include "sysguardd/service.hpp"

int main(int argc, char** argv) {
  try {
    const auto ctx = sysguardd::parse_cli_args(argc, argv);

    // Route to command handlers or legacy daemon mode
    if (ctx.command == "daemon" || (argc == 1)) {
      // Legacy daemon mode: read events from stdin
      const auto cfg = sysguardd::parse_config(argc, argv);
      auto engine = sysguardd::PolicyEngine::load_from_file(cfg.policy_path);
      sysguardd::Mitigator mitigator;
      sysguardd::AuditLogger logger(std::cout);

      sysguardd::Service svc(cfg.mode, std::move(engine), mitigator, logger);
      svc.run(std::cin);
      return 0;
    }

    // New CLI command execution
    return sysguardd::execute_command(ctx);
  } catch (const std::exception& ex) {
    std::cerr << "fatal: " << ex.what() << '\n';
    return 1;
  }
}
