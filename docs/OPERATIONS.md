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

## Alerting Recommendations
Create alerts for:
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
