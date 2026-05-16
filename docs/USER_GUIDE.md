# SysGuardd Quick Start

This guide is for end users who want to install and run SysGuardd quickly.

## 1) Install

### Option A: One-command installer (recommended)

```bash
curl -fsSL https://raw.githubusercontent.com/bansikah22/sysguardd/master/scripts/install.sh -o install.sh && bash install.sh --repo-url https://github.com/bansikah22/sysguardd.git --ref master --smoke-test
```

### Option B: Run from source

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## 2) Run in Monitor Mode First

```bash
./build/sysguardd daemon --mode monitor --policy ./policies/default.policy
```

Monitor mode is the safe default and should be used to tune policy before enforce rollouts.

## 3) Switch to Enforce Mode

```bash
./build/sysguardd daemon --mode enforce --policy ./policies/default.policy
```

## 4) Enable Real-time Alerts

```bash
./build/sysguardd daemon --mode enforce --alert-enabled --alert-min-severity critical
```

Set webhook URL by environment variable:

```bash
export SYSGUARDD_ALERT_WEBHOOK_URL="https://hooks.slack.com/services/XXX/YYY/ZZZ"
```

Or store it in a local `.env` file in the repository root:

```dotenv
SYSGUARDD_ALERT_WEBHOOK_URL=https://hooks.slack.com/services/XXX/YYY/ZZZ
```

## 5) Verify Runtime State

```bash
sysguardd status --json
sysguardd-status --json
```

## Next Steps

- CLI details: [CLI Reference](CLI.md)
- Production operation checklist: [Operations Guide](OPERATIONS.md)
- Cluster deployment: [Kubernetes Deployment](KUBERNETES.md)
- Full install and validation matrix: [Installation and Validation](INSTALL-TEST.md)
