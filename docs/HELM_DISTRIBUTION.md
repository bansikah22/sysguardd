# Helm Chart Distribution

This document explains how the SysGuardd Helm chart is published and how to use it.

## Chart Distribution Strategy

SysGuardd uses **OCI Registry** (Docker Hub) to distribute Helm charts. This is the modern approach supported by Helm 3.8+.

### Why OCI Registry?

PASS Unified Distribution: Charts and container images in the same registry
PASS No Additional Infrastructure: Reuse existing Docker Hub account
PASS Versioning: Automatic version management per chart release
PASS Modern Standard: Industry standard for cloud-native tools
PASS Secure: Leverage Docker Hub authentication and access controls  

## Published Chart Location

| Item | Value |
|------|-------|
| **Registry** | Docker Hub |
| **Repository** | bansikah/sysguardd-helm |
| **Chart Name** | sysguardd |
| **Latest Version** | 1.0.0 |
| **Access** | Public (no authentication required) |

## Installation Methods

### Method 1: OCI Registry (Recommended)

Requires **Helm 3.8+**

```bash
# Install latest version
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm

# Install specific version
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm --version 1.0.0

# Using short registry URL (requires helm 3.9+)
helm install sysguardd oci://docker.io/bansikah/sysguardd-helm
```

### Method 2: Local Installation

For development or airgapped environments:

```bash
# Clone repository
git clone https://github.com/bansikah22/sysguardd
cd sysguardd

# Install from local chart
helm install sysguardd ./helm
```

### Method 3: Search Registry

List available versions:

```bash
# Pull chart to inspect available versions
helm pull oci://registry-1.docker.io/bansikah/sysguardd-helm --version 1.0.0 --untar

# View Chart.yaml
cat sysguardd/Chart.yaml
```

## Publishing Workflow

The chart is automatically published when:

1. **Manual Trigger**: Via GitHub Actions `workflow_dispatch`
2. **Helm Files Changed**: On push to main/master with changes to `helm/` directory
3. **Release Published**: On GitHub release creation

### Publishing Pipeline

```
Chart.yaml (version: 1.0.0)
    ↓
[GitHub Actions: helm-oci-push.yml]
    ├─ Extract version from Chart.yaml
    ├─ Run: helm package helm/
    ├─ Login to Docker Hub
    ├─ Run: helm push sysguardd-1.0.0.tgz oci://registry-1.docker.io/bansikah
    └─ Verify chart in registry
    ↓
oci://registry-1.docker.io/bansikah/sysguardd-helm:1.0.0
```

## Version Management

### Chart Versioning

The chart uses **independent versioning** (separate from app version):

- **Chart Version**: `version` field in Chart.yaml (e.g., 1.0.0)
- **App Version**: `appVersion` field in Chart.yaml (e.g., 0.1.0)

This allows:
- Chart improvements without app releases
- Patch Helm values/templates independently
- Semantic versioning for each

### Semantic Versioning

Following [Helm Best Practices](https://helm.sh/docs/chart_best_practices/):

- **MAJOR**: Breaking changes in template structure or values schema
- **MINOR**: New features (new templates, ConfigMap keys, etc.)
- **PATCH**: Bug fixes, documentation, minor improvements

Example releases:

```
1.0.0  → Initial stable release
1.0.1  → Fix imagePullPolicy default
1.1.0  → Add HPA support
2.0.0  → Breaking: Change RBAC structure
```

## Chart Contents

```
sysguardd-helm:1.0.0/
├── Chart.yaml                 # Chart metadata
├── values.yaml                # Default configuration
├── templates/
│   ├── daemonset.yaml         # DaemonSet spec
│   ├── rbac.yaml              # ServiceAccount, ClusterRole, etc.
│   ├── configmap.yaml         # Policy ConfigMap
│   ├── _helpers.tpl           # Template helpers
│   └── NOTES.txt              # Post-install notes
└── README.md                  # Chart documentation
```

## Configuration

Default values in `values.yaml`:

```yaml
image:
  repository: bansikah/sysguardd
  tag: "0.1.0"
  pullPolicy: IfNotPresent

sysguardd:
  mode: "monitor"  # or "enforce"
  policyFile: "/etc/sysguardd/default.policy"

resources:
  requests:
    cpu: 100m
    memory: 128Mi
  limits:
    cpu: 500m
    memory: 512Mi
```

### Override Values

```bash
# Override at install time
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  --set sysguardd.mode=enforce \
  --set image.tag=latest

# Using values file
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  -f custom-values.yaml
```

## Prerequisites

- Kubernetes 1.19+
- Helm 3.8+ (for OCI registry support)
- Docker Hub access (public, no auth needed)

## Troubleshooting

### Chart Not Found

```bash
# Verify chart exists in registry
helm search repo | grep sysguardd

# Manual registry lookup
curl -s https://registry.hub.docker.com/v2/bansikah/sysguardd-helm/manifests/1.0.0
```

### Helm Version Too Old

```bash
# Check Helm version
helm version

# Upgrade Helm (requires v3.8+)
helm version --short  # Output: v3.x.x

# Install newer Helm
curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash
```

### Pull Rate Limits

If hitting Docker Hub rate limits:

```bash
# Authenticate to Docker Hub for higher limits
docker login
helm login registry-1.docker.io -u <username>

# Then install normally
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm
```

## Integration with Artifact Hub

To make the chart discoverable on [Artifact Hub](https://artifacthub.io):

1. Create `artifacthub-repo.yml` in repository root:

```yaml
repositoryID: sysguardd-helm
owners:
  - name: bansikah
    email: team@kodecloud.dev
```

2. Add to Chart.yaml:

```yaml
annotations:
  artifacthub.io/prerelease: "false"
  artifacthub.io/signKey: "4CC8A93750F3F22FD04778F1D962FD5E7F7CA0AD"
```

3. Reference in release notes linking to this chart.

## Future Distribution Options

### GitHub Container Registry (GHCR)

Alternative OCI registry for chart distribution:

```bash
helm push sysguardd-1.0.0.tgz oci://ghcr.io/bansikah22/helm-charts
helm install sysguardd oci://ghcr.io/bansikah22/helm-charts/sysguardd --version 1.0.0
```

### Helm Repository (Traditional HTTP)

For broader compatibility with older Helm versions:

```bash
# Host on GitHub Pages
helm repo add sysguardd https://bansikah22.github.io/sysguardd-helm-repo
helm install sysguardd sysguardd/sysguardd
```

## Support & Updates

- **Documentation**: [docs/KUBERNETES.md](KUBERNETES.md)
- **Issue Tracking**: [GitHub Issues](https://github.com/bansikah22/sysguardd/issues)
- **Releases**: [GitHub Releases](https://github.com/bansikah22/sysguardd/releases)

## Related Documentation

- [Kubernetes Deployment Guide](./KUBERNETES.md)
- [Installation & Testing](./INSTALL-TEST.md)
- [Operations Guide](./OPERATIONS.md)
