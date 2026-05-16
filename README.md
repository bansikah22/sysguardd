# SysGuardd

<div align="center">
  <img src="docs/images/sysguardd.png" alt="SysGuardd Logo" width="300" />
</div>

SysGuardd is a runtime process enforcement daemon focused on stopping unauthorized execution on Linux hosts and Kubernetes nodes.

It combines deterministic policy evaluation, active mitigation, and structured audit telemetry.

## Documentation
- Docs site: https://bansikah22.github.io/sysguardd/
- Source docs: [docs/](docs)

Key pages:
- [Architecture](docs/ARCHITECTURE.md)
- [Components](docs/COMPONENTS.md)
- [CLI](docs/CLI.md)
- [Operations](docs/OPERATIONS.md)
- [Kubernetes](docs/KUBERNETES.md)

## C/C++ Quick Start
Build and test:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Installer:

```bash
./scripts/install.sh --smoke-test
```

Systemd install:

```bash
./scripts/install.sh --systemd
```

## Kubernetes (Helm)

Install from local chart:

```bash
helm install sysguardd ./helm --namespace kube-system --create-namespace
```

Install from OCI chart:

```bash
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm --version 1.0.0
```

More Kubernetes setup and alerting examples: [docs/KUBERNETES.md](docs/KUBERNETES.md)

## CLI Quick Commands

```bash
sysguardd version
sysguardd status --json
sysguardd policy validate ./policies/default.policy
sysguardd help
```

## Runtime Check

Run in monitor mode with baseline policy:

```bash
./build/sysguardd daemon --mode monitor --policy ./policies/default.policy
```

Event format for stdin-based testing:

```text
PID PPID EXE [ARG ...]
1200 1 /usr/bin/bash -c whoami
```

Check runtime status:

```bash
sysguardd-status
```

More install/test details: [docs/INSTALL-TEST.md](docs/INSTALL-TEST.md)

## License
This project is licensed under the terms in [LICENSE](LICENSE).
