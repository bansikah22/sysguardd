# SysGuardd Component Reference

This document describes each component in the SysGuardd codebase, its responsibilities, and how they work together.

## Overview

SysGuardd is organized into several key components that work together to enforce runtime process policies:

```
┌─────────────────────────────────────────────────────────────┐
│  User Interface (CLI)                      [cli.hpp/cpp]   │
├─────────────────────────────────────────────────────────────┤
│  Configuration Management                  [config.hpp/cpp] │
├─────────────────────────────────────────────────────────────┤
│  Service Loop                              [service.hpp/cpp]│
│                                                              │
│  ┌─────────────────────┐  ┌──────────────┐  ┌──────────┐  │
│  │ Policy Engine       │  │ Mitigator    │  │ Logger   │  │
│  │ [policy_engine.*]   │  │ [mitigator.*]│  │ [audit_  │  │
│  │                     │  │              │  │ logger.*]│  │
│  │ - Deny-list lookup  │  │ - SIGKILL    │  │ - JSON   │  │
│  │ - O(1) decisions    │  │ - Safety     │  │ - Thread │  │
│  │ - Policy loading    │  │   guards     │  │ - safe   │  │
│  └─────────────────────┘  └──────────────┘  └──────────┘  │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ Event Model                   [event.hpp]            │  │
│  │ - ProcessEvent: Executable path, args, timestamps    │  │
│  │ - Decision: Allow/deny with reasons                  │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Component Details

### 1. CLI Interface (`include/sysguardd/cli.hpp`, `src/cli.cpp`)

**Purpose:** Provides a Docker daemon-like command-line interface for daemon management, policy operations, and status inspection.

**Responsibilities:**
- Parse command-line arguments into structured commands
- Route commands to appropriate handlers
- Provide user-friendly help and version information
- Format output (human-readable and JSON)

**Key Functions:**
- `parse_cli_args()` - Parse `int argc, char** argv` into `CLIContext`
- `execute_command()` - Route and execute command handlers
- `show_help()` - Display context-sensitive help
- `show_version()` - Display version info

**Available Commands:**
- `daemon` - Start enforcement daemon
- `status` - Show daemon status (with `--json`)
- `config` - View configuration (with `--json`)
- `version` - Show version
- `mode` - Switch between monitor/enforce modes
- `policy` - Policy management (validate, load, current)
- `logs` - View or stream audit logs
- `help` - Show help for any command

**Example Usage:**
```cpp
CLIContext ctx = parse_cli_args(argc, argv);
return execute_command(ctx);
```

---

### 2. Configuration Management (`include/sysguardd/config.hpp`, `src/config.cpp`)

**Purpose:** Handle daemon configuration parsing from command-line arguments and configuration sources.

**Responsibilities:**
- Parse CLI arguments (--mode, --policy)
- Validate configuration parameters
- Store configuration state

**Key Structures:**
```cpp
enum class Mode {
  monitor,  // Log violations without blocking
  enforce   // Actively block violations
};

struct Config {
  Mode mode{Mode::monitor};
  std::string policy_path{"./policies/default.policy"};
};
```

**Key Functions:**
- `parse_config()` - Parse CLI args into Config struct
- `parse_mode()` - Convert string to Mode enum

**Design Notes:**
- Configuration is immutable after parsing
- Defaults favor safety (monitor mode, local policy)
- Future: Support config files and environment variables

---

### 3. Event Model (`include/sysguardd/event.hpp`)

**Purpose:** Define the data structures for process events and enforcement decisions.

**Key Structures:**
```cpp
struct ProcessEvent {
  int pid;
  int ppid;
  std::string exe;
  std::vector<std::string> args;
  std::chrono::system_clock::time_point timestamp;
};

struct Decision {
  bool allowed;
  std::string reason;
};
```

**Responsibilities:**
- Define event format for kernel-to-userspace communication
- Provide decision structure for policy outcomes
- Model the minimal metadata needed for policy evaluation

**Design Notes:**
- Phase 1 uses text format (stdin): `PID PPID EXE [ARG ...]`
- Phase 1B will add eBPF kernel probes
- Structure supports future extension (parent context, capabilities, etc.)

---

### 4. Policy Engine (`include/sysguardd/policy_engine.hpp`, `src/policy_engine.cpp`)

**Purpose:** Load and evaluate process execution policies deterministically.

**Responsibilities:**
- Load policy rules from files
- Evaluate process events against deny-list rules
- Return allow/deny decisions with reasons
- Provide O(1)-style policy lookups

**Key Methods:**
```cpp
class PolicyEngine {
public:
  static PolicyEngine load_from_file(const std::string& path);
  Decision evaluate(const ProcessEvent& event) const;
};
```

**Policy Model (Phase 1):**
- Deny-list of executable paths (one per line)
- Default: allow all (monitor mode)
- Future: allow-list, behavioral rules, parent constraints

**Example Policy File:**
```
/usr/bin/nc
/usr/bin/ncat
/usr/bin/socat
/bin/bash -c
```

**Design Notes:**
- File-based policy for Phase 1
- O(1) deny-list lookup using `std::unordered_set`
- Rejects non-absolute paths for safety
- Thread-safe for concurrent policy reads

---

### 5. Mitigation Engine (`include/sysguardd/mitigator.hpp`, `src/mitigator.cpp`)

**Purpose:** Enforce deny decisions by terminating unauthorized processes.

**Responsibilities:**
- Send SIGKILL to blocked processes
- Guard against dangerous PIDs (1, self)
- Handle mitigation failures gracefully
- Provide error reporting

**Key Methods:**
```cpp
class Mitigator {
public:
  void kill_process(int pid);
};
```

**Safety Guards:**
- Never kill PID 1 (init/systemd)
- Never kill own process (avoid daemon suicide)
- Throw exceptions on failure for audit trail

**Design Notes:**
- SIGKILL ensures no catching/masking by target
- Single action in Phase 1 (extensible for future actions)
- Synchronous operation (could become async in Phase 2)

---

### 6. Audit Logger (`include/sysguardd/audit_logger.hpp`, `src/audit_logger.cpp`)

**Purpose:** Emit structured audit logs for observability and incident response.

**Responsibilities:**
- Format events as JSON for SIEM integration
- Thread-safe logging with mutex protection
- Provide both human-readable and machine-parseable output
- Escape special characters for JSON safety

**Key Methods:**
```cpp
class AuditLogger {
public:
  explicit AuditLogger(std::ostream& stream);
  void log(
    const ProcessEvent& event,
    const Decision& decision,
    bool enforced,
    const std::string& action = "",
    const std::string& action_error = ""
  );
};
```

**Output Format (JSON):**
```json
{
  "timestamp": "2026-05-13T10:00:00.123Z",
  "pid": 1234,
  "ppid": 1,
  "exe": "/usr/bin/bash",
  "args": ["-c", "whoami"],
  "decision": "deny",
  "reason": "executable in deny-list",
  "enforced": true,
  "action": "SIGKILL",
  "action_error": null
}
```

**Design Notes:**
- One event per line (suitable for log aggregation)
- Timestamps in ISO8601 format
- Thread-safe via mutex (supports future async logging)
- Escapable output for special characters

---

### 7. Service Loop (`include/sysguardd/service.hpp`, `src/service.cpp`)

**Purpose:** Main event processing loop that orchestrates policy evaluation and mitigation.

**Responsibilities:**
- Read process events from input (stdin in Phase 1)
- Parse event format (PID PPID EXE [ARG ...])
- Apply policy engine to events
- Trigger mitigation for denials
- Log all decisions to audit stream
- Handle errors gracefully

**Key Methods:**
```cpp
class Service {
public:
  Service(Mode mode, PolicyEngine engine, Mitigator& mitigator, AuditLogger& logger);
  void run(std::istream& input);
};
```

**Event Processing Flow:**
1. Read line from input (max 4096 bytes for safety)
2. Parse into ProcessEvent (PID PPID EXE [ARGS...])
3. Evaluate with PolicyEngine
4. Log decision to AuditLogger
5. If denied and enforce mode: call Mitigator::kill_process()
6. Continue to next event

**Error Handling:**
- Lines exceeding 4096 bytes are dropped with logging
- Parse failures are logged as errors
- Mitigation failures are caught and logged
- Daemon continues running on any single event error

**Design Notes:**
- Dependency injection for testability (references to engine, mitigator, logger)
- Event loop reads from stdin for Phase 1 baseline
- Phase 1B will replace stdin with eBPF kernel probe
- Support for both monitor and enforce modes

---

### 8. Main Entry Point (`src/main.cpp`)

**Purpose:** Application entry point that ties all components together.

**Responsibilities:**
- Parse command-line arguments
- Route to CLI command handlers or legacy daemon mode
- Construct service with dependency injection
- Handle exceptions and provide error messages
- Manage process lifecycle

**Flow:**
```
main()
  ├─> parse_cli_args()
  ├─> Check if legacy daemon mode or new CLI
  ├─> If CLI: execute_command()
  └─> If daemon: 
        ├─> parse_config()
        ├─> load policy_engine
        ├─> create mitigator
        ├─> create audit_logger
        ├─> run service.run(stdin)
```

**Error Handling:**
- Catches all exceptions
- Prints error message to stderr
- Exits with code 1 on failure, 0 on success

---

## Data Flow Through Components

### Example: Process Blocked by Policy

```
┌──────────────────┐
│ Input Event:     │
│ 1234 1 /bin/nc   │
└────────┬──────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│ Service::run()                          │
│ - Parse event line                      │
│ - Create ProcessEvent object            │
└──────────────┬──────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────┐
│ PolicyEngine::evaluate(ProcessEvent)     │
│ - Check if /bin/nc in deny-list          │
│ - Return: Decision{false, "in deny-list"}│
└──────────────┬───────────────────────────┘
               │ (denied)
               ▼
┌──────────────────────────────────────────┐
│ Service::run() (continue)                │
│ - Check enforcement mode                 │
│ - If enforce: call Mitigator::kill_process(1234)
└──────────────┬───────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────┐
│ AuditLogger::log(event, decision,...)    │
│ - Format as JSON                         │
│ - Write to stdout                        │
└──────────────────────────────────────────┘
```

---

## Dependencies and Ownership

**Ownership Model:**
- `Service` owns `PolicyEngine` (move semantics)
- `Service` holds references to `Mitigator` and `AuditLogger` (non-owning)
- `main()` manages all lifetimes

**Why References for Mitigator and Logger?**
- Non-copyable types (mutex in AuditLogger)
- Allows dependency injection for testing
- Clear separation: Service doesn't manage their lifecycle

---

## Testing Strategy

### Unit Tests
- **policy_engine_test**: Load policies, evaluate events
- **Future**: service_test, mitigator_test, logger_test

### Integration Testing
- Event input via stdin
- Policy enforcement verification
- Audit log format validation

### Example Test (policy validation):
```cpp
TEST(PolicyEngine, DenyListEvaluation) {
  auto engine = PolicyEngine::load_from_file("./policies/default.policy");
  ProcessEvent event{1234, 1, "/usr/bin/nc", {}, {}};
  Decision decision = engine.evaluate(event);
  EXPECT_FALSE(decision.allowed);
}
```

---

## Future Enhancements

### Phase 1B
- Daemon fork/background mode
- Policy hot-reload without restart
- Log archival and streaming

### Phase 2
- eBPF kernel probes for real-time execution
- gRPC telemetry endpoint
- Allow-list mode
- Behavioral anomaly detection

### Phase 3
- Trust Gradient Mode
- Multi-tenant isolation
- Policy versioning and rollback

---

## See Also

- [CLI.md](CLI.md) - Command-line interface reference
- [ARCHITECTURE.md](ARCHITECTURE.md) - High-level system design
- [OPERATIONS.md](OPERATIONS.md) - Operational procedures
- [ROADMAP.md](ROADMAP.md) - Feature roadmap
