# SysGuardd Operations Guide

## Purpose
This guide defines how to run, configure, and operate SysGuardd safely in development and production environments.

## Runtime Modes

### Monitor Mode
- Captures and evaluates process events.
- Does not terminate processes.
- Best for baseline learning and policy tuning.

### Enforce Mode
- Captures and evaluates process events.
- Terminates unauthorized processes using `SIGKILL`.
- Requires mature policy coverage and alerting.

## Suggested Configuration Keys
Use these keys as the initial configuration contract.

- `mode`: `monitor` or `enforce`
- `policy.source`: `file` or `remote`
- `policy.path`: local policy file path
- `policy.refresh_interval_seconds`: remote policy refresh cadence
- `telemetry.format`: `json` or `grpc`
- `telemetry.endpoint`: remote sink for gRPC stream
- `log.level`: `info`, `warn`, `error`, `debug`

## Policy Authoring Guidelines
- Start with monitor mode and capture at least 7 days of process data.
- Convert known-bad executables and suspicious paths into deny rules.
- Keep rules explicit and reviewable.
- Version all policy changes in source control.
- Require peer review for policy updates before enforce rollout.

## Rollout Strategy
1. Deploy in monitor mode to a non-critical environment.
2. Validate event volume, false positives, and expected process baselines.
3. Apply policy updates until false positives are near zero.
4. Roll out enforce mode to a small canary subset.
5. Expand enforce coverage incrementally.

## Real-Time Alerting

SysGuardd includes a built-in non-blocking webhook dispatcher. It is **disabled by default** and opt-in via flags.

### Enabling Alerts

For local runs, put `SYSGUARDD_ALERT_WEBHOOK_URL=...` in a repo-root `.env` file. The daemon loads it automatically, and `.env` is already ignored by git.

```bash
sysguardd daemon --mode enforce \
  --alert-enabled \
  --alert-webhook-url https://hooks.slack.com/services/XXX/YYY/ZZZ \
  --alert-min-severity critical
```

For monitor-mode deployments during policy tuning:

```bash
sysguardd daemon --mode monitor \
  --alert-enabled \
  --alert-webhook-url http://hooks.example.com/sysguardd \
  --alert-min-severity warning \
  --alert-dedupe-window 120 \
  --alert-rate-limit 20
```

### Alert Configuration Reference

| Flag | Default | Description |
|---|---|---|
| `--alert-enabled` | off | Enable the dispatcher. Without this flag no dispatcher is created. |
| `--alert-webhook-url` | — | HTTP URL to POST alerts to. Only `http://` is supported. |
| `--alert-min-severity` | `warning` | Drop alerts below this level. Values: `info`, `warning`, `critical`. |
| `--alert-dedupe-window` | `60` | Seconds. Suppress repeated alerts for the same exe+severity. |
| `--alert-rate-limit` | `60` | Max alerts dispatched per minute. Excess alerts are dropped silently. |

### Severity Levels

| Condition | Severity |
|---|---|
| Process allowed by policy | `info` |
| Process denied in monitor mode | `warning` |
| Process denied in enforce mode | `critical` |
| Mitigation (SIGKILL) failed | `critical` |

### Audit Log Fields Added

Every event log line now includes:

```json
{
  "event_id": "0000000000000000",
  "severity": "critical",
  "node_id": "k8s-node-01",
  "policy_version": "v1.2.3",
  ...
}
```

- `event_id` — monotonic hex counter, unique per daemon run. Use this to correlate audit log entries with webhook alerts.
- `node_id` — defaults to system hostname; override with `--node-id`.
- `policy_version` — optional tag set via `--policy-version`.

### Safety Guarantees

- Alert dispatch **never blocks the event loop**. Events are queued and delivered by a background thread.
- If the webhook is unreachable, the failure is logged to stderr and the daemon continues.
- If the queue reaches 128 entries (e.g. webhook is down), the oldest queued alert is dropped to prevent unbounded memory growth.
- `--alert-dedupe-window` and `--alert-rate-limit` prevent alert storms from overwhelming downstream notification services.

## Legacy Alerting Recommendations
Create alerts at your SIEM/monitoring layer for:
- Unauthorized process killed.
- Mitigation failed.
- Ring buffer drop rate above threshold.
- Telemetry export failures.
- Policy fetch or signature verification failures.

## Incident Response Checklist
1. Capture blocked executable path and command arguments.
2. Identify parent process and user context.
3. Determine whether the event is malicious or false positive.
4. If false positive, patch policy and re-test in monitor mode.
5. If malicious, preserve evidence and rotate compromised credentials.

## Performance and Reliability Checks
Track the following indicators:
- Event ingestion rate
- Policy decision latency
- Mitigation execution latency
- Ring buffer utilization and drop rate
- CPU and memory footprint of daemon

## Maintenance Tasks
- Rotate logs and archive high-fidelity audit trails.
- Review policy exceptions monthly.
- Re-test enforcement after kernel upgrades.
- Validate integration endpoints and certificates.

## Change Management
- Every production policy change should include:
  - Change reason
  - Risk assessment
  - Rollback procedure
  - Approval record

## Minimal Runbook Template
Use this template for on-call handoffs:
- Service owner
- Escalation contacts
- Current mode (monitor/enforce)
- Policy version
- Last deployment timestamp
- Known exceptions
- Rollback command/process
