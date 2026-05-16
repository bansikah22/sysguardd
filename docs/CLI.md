# SysGuardd Command-Line Interface

SysGuardd provides a **Docker daemon-like CLI interface** with familiar patterns for managing the enforcement daemon, policies, and operational modes.

## Command Overview

```
sysguardd <command> [options]
```

### Available Commands

- `daemon` - Start the SysGuardd enforcement daemon
- `status` - Show daemon status
- `logs` - View or stream audit logs  
- `policy` - Manage enforcement policies
- `mode` - Switch enforcement mode (monitor/enforce)
- `config` - View current configuration
- `version` - Show version information
- `help` - Show help for commands

## Command Reference

### daemon - Start the Daemon

Start the SysGuardd enforcement daemon with specified mode and policy.

```bash
sysguardd daemon [options]
```

**Options:**
- `--background, -b` - Run in background (daemon mode)
- `--mode [monitor|enforce]` - Set enforcement mode (default: monitor)
- `--policy <file>` - Path to policy file (default: ./policies/default.policy)
- `--node-id <id>` - Override node identifier emitted in audit logs (default: system hostname)
- `--policy-version <ver>` - Tag emitted in audit logs for policy traceability
- `--alert-enabled` - Enable the real-time webhook alerting dispatcher
- `--alert-webhook-url <url>` - HTTP webhook URL to POST alerts to (Slack/Teams compatible)
- `--alert-min-severity [info|warning|critical]` - Minimum severity to dispatch (default: warning)
- `--alert-dedupe-window <sec>` - Suppress duplicate alerts for the same exe+severity within this window (default: 60)
- `--alert-rate-limit <N>` - Maximum alerts dispatched per minute (default: 60)
- `--help, -h` - Show help message

**Examples:**

```bash
# Run daemon in foreground with default settings (monitor mode)
sysguardd daemon

# Run in background with enforce mode
sysguardd daemon --background --mode enforce

# Run with custom policy file
sysguardd daemon --policy /etc/sysguardd/strict.policy

# Run in enforce mode with custom policy in background
sysguardd daemon -b --mode enforce --policy /var/sysguardd/prod.policy

# Run with node labelling and policy version for traceable audit logs
sysguardd daemon --node-id k8s-node-01 --policy-version v1.2.3

# Enable Slack webhook alerts for critical deny events only
sysguardd daemon --mode enforce \
  --alert-enabled \
  --alert-webhook-url http://hooks.slack.com/services/XXX/YYY/ZZZ \
  --alert-min-severity critical

# Enable alerts with noise-reduction tuning
sysguardd daemon --mode monitor \
  --alert-enabled \
  --alert-webhook-url http://hooks.example.com/sysguardd \
  --alert-min-severity warning \
  --alert-dedupe-window 120 \
  --alert-rate-limit 20
```

---

### status - Check Daemon Status

Display the current operational status of the daemon.

```bash
sysguardd status [options]
```

**Options:**
- `--json, -j` - Output in JSON format (suitable for scripts)
- `--help, -h` - Show help message

**Examples:**

```bash
# Human-readable status
$ sysguardd status
SysGuardd Status
  Version: 0.1.0-alpha
  Status: running
  Mode: monitor
  Policy: /etc/sysguardd/default.policy
  PID: 1809592

# JSON output for programmatic use
$ sysguardd status --json
{
  "version": "0.1.0-alpha",
  "status": "running",
  "mode": "monitor",
  "policy": "/etc/sysguardd/default.policy",
  "pid": 1809592,
  "uptime_seconds": 0
}
```

---

### config - View Configuration

Display the current daemon configuration.

```bash
sysguardd config [options]
```

**Options:**
- `--json, -j` - Output in JSON format
- `--help, -h` - Show help message

**Examples:**

```bash
# Human-readable configuration
$ sysguardd config
SysGuardd Configuration
  Mode: monitor
  Policy: /etc/sysguardd/default.policy
  Log Level: info
  Audit Output: stdout

# JSON output for integration with other tools
$ sysguardd config --json
{
  "mode": "monitor",
  "policy_path": "/etc/sysguardd/default.policy",
  "log_level": "info",
  "audit_output": "stdout"
}
```

---

### version - Show Version Information

Display version and build information.

```bash
sysguardd version
```

**Examples:**

```bash
$ sysguardd version
SysGuardd version 0.1.0-alpha
Build date: May 13 2026
```

---

### mode - Switch Enforcement Mode

View or switch between monitor and enforce modes.

```bash
sysguardd mode [monitor|enforce]
```

**Modes:**
- `monitor` - Log violations without blocking (default for safety)
- `enforce` - Actively terminate unauthorized processes

**Examples:**

```bash
# Show current mode
$ sysguardd mode
Current mode: monitor

# Switch to enforce mode
$ sysguardd mode enforce
Switching mode to: enforce
(mode switching not yet implemented, restart daemon to apply)

# Switch back to monitor mode
$ sysguardd mode monitor
Switching mode to: monitor
(mode switching not yet implemented, restart daemon to apply)
```

> **Note:** Phase 1 requires daemon restart to apply mode changes. Hot-reload is planned for Phase 1B.

---

### policy - Manage Policies

Manage enforcement policies.

```bash
sysguardd policy <subcommand> [args]
```

**Subcommands:**

#### `policy validate <file>`

Validate policy file syntax before applying.

```bash
$ sysguardd policy validate /tmp/new-policy.txt
Policy validation successful
  File: /tmp/new-policy.txt
  Lines: 42
  Syntax: OK
```

#### `policy load <file>`

Load and apply a new policy.

```bash
$ sysguardd policy load /etc/sysguardd/strict.policy
Loading policy: /etc/sysguardd/strict.policy
(hot-reload not yet implemented, restart daemon to apply)
```

#### `policy current`

Show the currently active policy file.

```bash
$ sysguardd policy current
Current policy: /etc/sysguardd/default.policy
(policy introspection not yet implemented)
```

**Examples:**

```bash
# Validate a new policy before deployment
sysguardd policy validate /tmp/staging-policy.txt

# View all policy subcommands
sysguardd policy
sysguardd help policy

# Check current policy
sysguardd policy current
```

---

### logs - View Audit Logs

View or stream audit logs.

```bash
sysguardd logs [options]
```

**Options:**
- `--follow, -f` - Stream logs in real-time (like `tail -f`)
- `--tail <N>, -n <N>` - Show last N log entries (default: 10)
- `--help, -h` - Show help message

**Examples:**

```bash
# View last 10 audit log entries
$ sysguardd logs
Last 10 audit log entries:
(log archival not yet implemented)

# View last 50 entries
$ sysguardd logs --tail 50

# Stream logs in real-time
$ sysguardd logs --follow
Following audit logs (stream mode not yet implemented)...

# Tail logs with many entries
$ sysguardd logs -f -n 100
```

> **Note:** Log archival and streaming are planned for Phase 1B. Currently supports JSON output to stdout during daemon execution.

---

### help - Get Help

Show help information for commands.

```bash
sysguardd help [command]
```

**Examples:**

```bash
# Show all available commands
$ sysguardd help

# Get help for a specific command
$ sysguardd help daemon
$ sysguardd help policy
$ sysguardd help mode

# Use global --help flag
$ sysguardd daemon --help
$ sysguardd policy --help
```

---

## Global Options

These options work with any command:

- `--help, -h` - Show help for the command
- `--version, -v` - Show version information

**Examples:**

```bash
sysguardd --help
sysguardd -h
sysguardd daemon --help
sysguardd status --version
sysguardd --version
```

---

## Common Usage Patterns

### Development Setup

```bash
# Build from source
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Test policy before deployment
./build/sysguardd policy validate ./policies/default.policy

# Run daemon in foreground
./build/sysguardd daemon --mode monitor
```

### Production Deployment

```bash
# Check version before deployment
sysguardd version

# Validate custom policy
sysguardd policy validate /etc/sysguardd/production.policy

# Start daemon with custom policy in enforce mode
sysguardd daemon --background --mode enforce --policy /etc/sysguardd/production.policy

# Monitor status
sysguardd status --json | jq .
```

### Monitoring and Troubleshooting

```bash
# Check daemon status
sysguardd status

# Get configuration as JSON for integration
sysguardd config --json

# View audit logs
sysguardd logs --tail 50

# Stream logs in real-time
sysguardd logs --follow

# Show operational mode
sysguardd mode
```

### Policy Management

```bash
# Safe workflow for policy updates
sysguardd policy validate /tmp/staging-policy.txt  # Validate first
sysguardd policy load /tmp/staging-policy.txt      # Load (restart needed)
sysguardd mode enforce                              # Switch mode if ready
sysguardd status                                    # Verify
```

---

## Integration Examples

### With systemd

The daemon is managed as a systemd service:

```bash
# Start daemon
systemctl start sysguardd

# Check status
systemctl status sysguardd

# View logs
journalctl -u sysguardd -f

# Get daemon status via CLI
sysguardd status
```

### With Container Orchestration

In Kubernetes, the daemon runs as a DaemonSet:

```bash
# Deploy daemon
kubectl apply -f k8s/sysguardd-daemonset.yaml

# Check pod status
kubectl get daemonset sysguardd -o json | jq '.status'

# Check daemon status from pod
kubectl exec -it <pod> -- sysguardd status

# Stream audit logs
kubectl logs -f <pod>
```

---

## Phase Timeline

### Phase 1 (Current)
- DONE: Docker-like CLI interface
- DONE: Status/config inspection with JSON output
- DONE: Policy validation
- IN PROGRESS: Policy hot-reload
- IN PROGRESS: Log archival and streaming

### Phase 1B
- Log streaming to stdout/journal
- gRPC telemetry endpoint
- Daemon fork/background mode
- Configuration file support

### Phase 2+
- Advanced policy syntax
- Multi-tenant isolation
- High-performance kernel hooks (eBPF)
- Distributed tracing

---

## Troubleshooting

### Command not found

```bash
# Ensure sysguardd is in PATH
which sysguardd

# Or use full path
/usr/local/bin/sysguardd status
```

### Permission denied

Some commands may require elevated privileges:

```bash
sudo sysguardd daemon --mode enforce
sudo sysguardd mode enforce
sudo sysguardd policy load /etc/sysguardd/admin-policy.txt
```

### Status shows "not running"

The daemon may not be started:

```bash
sysguardd daemon                           # Start in foreground
# OR
systemctl start sysguardd                  # Start via systemd
```

---

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - System design and components
- [OPERATIONS.md](OPERATIONS.md) - Operational procedures
- [INSTALL-TEST.md](INSTALL-TEST.md) - Installation instructions
- [ROADMAP.md](ROADMAP.md) - Feature roadmap and phases
