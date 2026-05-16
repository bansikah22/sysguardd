# SysGuardd Documentation

SysGuardd is a runtime process enforcement daemon for Linux hosts and Kubernetes nodes.

## Start Here

- [Architecture](ARCHITECTURE.md)
- [Components](COMPONENTS.md)
- [CLI](CLI.md)
- [Operations](OPERATIONS.md)
- [Kubernetes](KUBERNETES.md)

## Quick Run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/sysguardd daemon --mode monitor --policy ./policies/default.policy
```

## Alerting Feature

Real-time alerting supports webhook dispatch with dedupe and rate limiting.

See:

- [Operations Guide](OPERATIONS.md)
- [Kubernetes Guide](KUBERNETES.md)
- [Architecture](ARCHITECTURE.md)
