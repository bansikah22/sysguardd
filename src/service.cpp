#include "sysguardd/service.hpp"

#include <chrono>
#include <cctype>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace sysguardd {
namespace {

std::string trim(const std::string& s) {
  size_t start = 0;
  size_t end = s.size();
  while (start < end && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
    ++start;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
    --end;
  }
  return s.substr(start, end - start);
}

ProcessEvent parse_event_line(const std::string& line) {
  std::istringstream iss(line);
  ProcessEvent evt;
  if (!(iss >> evt.pid >> evt.ppid >> evt.exe)) {
    throw std::runtime_error("invalid event line format");
  }

  std::string arg;
  while (iss >> arg) {
    evt.args.push_back(arg);
  }
  evt.timestamp = std::chrono::system_clock::now();
  return evt;
}

}  // namespace

Service::Service(const Mode mode, PolicyEngine policy, Mitigator& mitigator, AuditLogger& logger)
  : mode_(mode), policy_(std::move(policy)), mitigator_(mitigator), logger_(logger) {}

void Service::run(std::istream& input) {
  std::string line;
  while (std::getline(input, line)) {
    if (line.size() > 4096) {
      std::cerr << "event dropped: line too long\n";
      continue;
    }

    const auto clean = trim(line);
    if (clean.empty()) {
      continue;
    }

    ProcessEvent event;
    try {
      event = parse_event_line(clean);
    } catch (const std::exception& ex) {
      std::cerr << "event parse error: " << ex.what() << '\n';
      continue;
    }

    const Decision decision = policy_.evaluate(event);
    if (decision.allowed) {
      logger_.log(event, decision, false, "", "");
      continue;
    }

    bool enforced = false;
    std::string action;
    std::string action_error;

    if (mode_ == Mode::enforce) {
      action = "sigkill";
      try {
        mitigator_.kill_process(event.pid);
        enforced = true;
      } catch (const std::exception& ex) {
        action_error = ex.what();
      }
    }

    logger_.log(event, decision, enforced, action, action_error);
  }
}

}  // namespace sysguardd
