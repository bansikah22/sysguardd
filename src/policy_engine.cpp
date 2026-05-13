#include "sysguardd/policy_engine.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace sysguardd {
namespace {

bool starts_with(const std::string& value, const std::string& prefix) {
  return value.rfind(prefix, 0) == 0;
}

std::string trim(std::string_view value) {
  size_t start = 0;
  size_t end = value.size();
  while (start < end && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
    ++start;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    --end;
  }
  return std::string(value.substr(start, end - start));
}

}  // namespace

PolicyEngine PolicyEngine::load_from_file(const std::string& path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    throw std::runtime_error("unable to open policy file: " + path);
  }

  PolicyEngine engine;
  std::string line;
  size_t line_number = 0;
  while (std::getline(in, line)) {
    ++line_number;
    const auto clean = trim(line);
    if (clean.empty() || clean[0] == '#') {
      continue;
    }

    const auto eq_pos = clean.find('=');
    if (eq_pos == std::string::npos) {
      throw std::runtime_error("invalid policy line " + std::to_string(line_number));
    }

    const auto key = trim(clean.substr(0, eq_pos));
    const auto value = normalize_abs_path(trim(clean.substr(eq_pos + 1)));
    if (value.empty()) {
      throw std::runtime_error("policy path must be absolute at line " + std::to_string(line_number));
    }

    if (key == "deny_executable") {
      engine.deny_executables_.insert(value);
    } else if (key == "deny_prefix") {
      engine.deny_prefixes_.push_back(value);
    } else {
      throw std::runtime_error("unknown policy key at line " + std::to_string(line_number));
    }
  }

  return engine;
}

Decision PolicyEngine::evaluate(const ProcessEvent& event) const {
  const auto normalized = normalize_abs_path(event.exe);
  if (normalized.empty()) {
    return Decision{false, "missing_or_relative_executable_path"};
  }

  if (deny_executables_.find(normalized) != deny_executables_.end()) {
    return Decision{false, "deny_executable_match"};
  }

  for (const auto& prefix : deny_prefixes_) {
    if (starts_with(normalized, prefix)) {
      return Decision{false, "deny_prefix_match"};
    }
  }

  return Decision{true, "allowed"};
}

std::string PolicyEngine::normalize_abs_path(std::string_view path) {
  const std::string raw = trim(path);
  if (raw.empty()) {
    return {};
  }

  fs::path p{raw};
  if (!p.is_absolute()) {
    return {};
  }

  const auto normalized = p.lexically_normal();
  for (const auto& part : normalized) {
    if (part == "..") {
      return {};
    }
  }

  // Canonicalize path to resolve symlinks and prevent bypass attacks (CWE-59)
  std::error_code ec;
  auto canonical = fs::canonical(normalized, ec);
  if (ec) {
    return {};  // Path doesn't exist or can't be resolved
  }

  // Re-check for ".." in the canonical path as an extra safeguard
  const auto final_normalized = canonical.lexically_normal();
  for (const auto& part : final_normalized) {
    if (part == "..") {
      return {};
    }
  }

  return final_normalized.string();
}

}  // namespace sysguardd
