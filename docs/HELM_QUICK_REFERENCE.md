# Helm Chart Quick Reference

For end-users installing SysGuardd via Helm from the published OCI registry.

## Installation

```bash
# Install from OCI registry (Docker Hub)
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  --namespace kube-system \
  --create-namespace

# Verify installation
kubectl get daemonset -n kube-system sysguardd
kubectl get pods -n kube-system -l app=sysguardd
```

## Configuration

### Monitor Mode (Default)

Logs policy violations without enforcement:

```bash
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  --set sysguardd.mode=monitor
```

### Enforce Mode

Terminates unauthorized processes:

```bash
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  --set sysguardd.mode=enforce
```

### Custom Policy

Supply your own policy file:

```bash
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  --set policy.data="$(cat /path/to/custom.policy)"
```

## Common Operations

### Check Status

```bash
# Get pod logs
kubectl logs -n kube-system -l app=sysguardd -f

# Get pod status
kubectl get pods -n kube-system -l app=sysguardd -o wide

# Describe pod
kubectl describe pod -n kube-system -l app=sysguardd
```

### Switch Modes

```bash
# Change from monitor to enforce
helm upgrade sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  --reuse-values \
  --set sysguardd.mode=enforce
```

### View Audit Logs

```bash
# Stream real-time audit logs (JSON format)
kubectl logs -n kube-system -l app=sysguardd -f | jq .

# Get last 100 log lines
kubectl logs -n kube-system -l app=sysguardd --tail=100 | tail -100 | jq .
```

### Uninstall

```bash
helm uninstall sysguardd -n kube-system
```

## Troubleshooting

### Chart Not Found

**Error**: `Error: failed to fetch "oci://registry-1.docker.io/bansikah/sysguardd-helm": unsupported protocol scheme "oci"`

**Solution**: Upgrade Helm to 3.8+
```bash
helm version  # Must be v3.8.0 or higher
```

### Pod CrashLoopBackOff

```bash
# Check pod events
kubectl describe pod -n kube-system -l app=sysguardd

# View pod logs
kubectl logs -n kube-system -l app=sysguardd --previous
```

### Authentication Issues

If using private registry, create pull secret:

```bash
kubectl create secret docker-registry regcred \
  --docker-server=registry-1.docker.io \
  --docker-username=<username> \
  --docker-password=<password> \
  -n kube-system

helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  --set imagePullSecrets[0].name=regcred
```

## Resources

- **Full Documentation**: [docs/HELM_DISTRIBUTION.md](HELM_DISTRIBUTION.md)
- **Kubernetes Guide**: [docs/KUBERNETES.md](KUBERNETES.md)
- **Event Injection Tests**: [docs/EVENT_INJECTION_TESTING.md](EVENT_INJECTION_TESTING.md)
- **GitHub Repository**: https://github.com/bansikah22/sysguardd
- **Docker Hub**: https://hub.docker.com/r/bansikah/sysguardd

## Chart Versions

| Version | App Version | Released | Notes |
|---------|-------------|----------|-------|
| 1.0.0 | 0.1.0 | 2026-05-13 | Initial OCI release |

For version history, see [GitHub Releases](https://github.com/bansikah22/sysguardd/releases)
