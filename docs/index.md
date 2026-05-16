# SysGuardd Documentation

SysGuardd is a runtime process enforcement daemon for Linux hosts and Kubernetes nodes.

It helps security and platform teams monitor and block unauthorized executable launches with structured audit logs and optional real-time alerts.

## Choose Your Path

### For End Users

Start here if you want to install, run, and operate SysGuardd.

- [Quick Start](USER_GUIDE.md)
- [Installation and Validation](INSTALL-TEST.md)
- [CLI Reference](CLI.md)
- [Operations Guide](OPERATIONS.md)
- [Kubernetes Deployment](KUBERNETES.md)

### For Contributors and Maintainers

Use these pages for architecture, publishing, and project maintenance tasks.

- [Maintainer Guide](MAINTAINERS.md)
- [Architecture](ARCHITECTURE.md)
- [Components](COMPONENTS.md)

## Fast Install (Curl)

```bash
curl -fsSL https://raw.githubusercontent.com/bansikah22/sysguardd/master/scripts/install.sh -o install.sh && bash install.sh --repo-url https://github.com/bansikah22/sysguardd.git --ref master --smoke-test
```

## Quick Run (From Source)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/sysguardd daemon --mode monitor --policy ./policies/default.policy
```

## What Is Included

- Runtime policy enforcement in monitor or enforce mode
- Structured JSON audit logs with event correlation fields
- Optional webhook alerting with dedupe and rate limiting
- Linux host and Kubernetes deployment support
