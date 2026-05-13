# Kubernetes Testing Guide

This guide shows how to deploy and test SysGuardd on Kubernetes clusters using different methods.

## Prerequisites

- Kubernetes cluster (1.20+)
- `kubectl` configured to access your cluster
- Helm 3.8+ (for OCI registry support)
- Docker image available (local or public registry)

## Quick Start

### Install from Published Helm Chart (OCI Registry)

The easiest way for end-users to install SysGuardd:

```bash
# Install latest version from Docker Hub OCI registry
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm \
  --namespace kube-system \
  --create-namespace

# Verify deployment
kubectl get daemonset -n kube-system sysguardd
kubectl get pods -n kube-system -l app=sysguardd
```

For details on using the published chart, see [docs/HELM_DISTRIBUTION.md](HELM_DISTRIBUTION.md).

## Deployment Methods

### Method 1: Helm (Recommended)

Helm provides the most flexible and production-ready deployment model.

#### Install via Helm - Local Chart

**From local chart (for development):**
```bash
helm install sysguardd ./helm \
  --namespace kube-system \
  --create-namespace
```

**With monitor mode (default):**
```bash
helm install sysguardd ./helm \
  --namespace kube-system \
  --set sysguardd.mode=monitor
```

**Switch to enforce mode after validation:**
```bash
helm upgrade sysguardd ./helm \
  --namespace kube-system \
  --set sysguardd.mode=enforce
```

#### Common Helm Values

| Value | Default | Description |
|-------|---------|-------------|
| `image.repository` | `bansikah/sysguardd` | Container image name |
| `image.tag` | `v0.1.0` | Image tag |
| `sysguardd.mode` | `monitor` | Enforcement mode: `monitor` or `enforce` |
| `sysguardd.logLevel` | `info` | Log verbosity |
| `resources.requests.cpu` | `100m` | CPU request |
| `resources.limits.cpu` | `500m` | CPU limit |
| `resources.limits.memory` | `512Mi` | Memory limit |

**Custom values file:**
```bash
cat > custom-values.yaml <<EOF
sysguardd:
  mode: enforce
  logLevel: debug
resources:
  requests:
    memory: 256Mi
  limits:
    memory: 1Gi
EOF

helm install sysguardd ./helm -f custom-values.yaml
```

#### Verify Helm Deployment

```bash
# Check DaemonSet status
kubectl get daemonset -n kube-system sysguardd

# Watch pod rollout
kubectl rollout status daemonset/sysguardd -n kube-system --timeout=5m

# View pod logs
kubectl logs -n kube-system -l app.kubernetes.io/name=sysguardd --tail=50 -f

# Check service
kubectl get svc -n kube-system -l app.kubernetes.io/name=sysguardd
```

#### Uninstall

```bash
helm uninstall sysguardd -n kube-system
```

---

### Method 2: Kustomize

Kustomize allows environment-specific overlays without templating.

#### Deploy with Kustomize

**Base deployment:**
```bash
kubectl apply -k k8s/
```

**Create environment overlay (monitor mode):**
```bash
mkdir -p k8s/overlays/monitor
cat > k8s/overlays/monitor/kustomization.yaml <<EOF
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namespace: kube-system

bases:
- ../../

patches:
- target:
    kind: DaemonSet
    name: sysguardd
  patch: |-
    - op: replace
      path: /spec/template/spec/containers/0/args/2
      value: monitor
EOF

kubectl apply -k k8s/overlays/monitor
```

**Create enforce overlay:**
```bash
mkdir -p k8s/overlays/enforce
cat > k8s/overlays/enforce/kustomization.yaml <<EOF
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namespace: kube-system

bases:
- ../../

patches:
- target:
    kind: DaemonSet
    name: sysguardd
  patch: |-
    - op: replace
      path: /spec/template/spec/containers/0/args/2
      value: enforce
EOF

kubectl apply -k k8s/overlays/enforce
```

#### Verify Kustomize Deployment

```bash
kubectl get all -n kube-system -l app=sysguardd
kubectl describe daemonset sysguardd -n kube-system
```

---

### Method 3: Raw kubectl Manifests

Deploy individual manifest files.

```bash
# 1. Create namespace
kubectl create namespace kube-system --dry-run=client -o yaml | kubectl apply -f -

# 2. Deploy RBAC
kubectl apply -f k8s/rbac.yaml

# 3. Deploy ConfigMap (policy)
kubectl apply -f k8s/configmap.yaml

# 4. Deploy DaemonSet
kubectl apply -f k8s/daemonset.yaml
```

---

## Testing Workflows

### Local Testing: kind (Kubernetes in Docker)

**Create a test cluster:**
```bash
kind create cluster --name sysguardd-test
```

**Build and load image:**
```bash
# Build locally
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# (Assuming a Dockerfile exists - see Dockerfile section below)
docker build -t sysguardd:latest .

# Load into kind
kind load docker-image sysguardd:latest --name sysguardd-test
```

**Deploy:**
```bash
kubectl cluster-info --context kind-sysguardd-test
helm install sysguardd ./helm \
  --context kind-sysguardd-test \
  --namespace kube-system
```

**Inspect logs:**
```bash
kubectl logs -n kube-system -l app.kubernetes.io/name=sysguardd \
  --context kind-sysguardd-test --tail=50 -f
```

**Cleanup:**
```bash
kind delete cluster --name sysguardd-test
```

---

### Multi-Node Testing: minikube

**Start multi-node cluster:**
```bash
minikube start --nodes 3 -p sysguardd-test
```

**Verify nodes:**
```bash
kubectl get nodes
```

**Deploy SysGuardd:**
```bash
helm install sysguardd ./helm --namespace kube-system
```

**Check DaemonSet spread:**
```bash
kubectl get pods -n kube-system -o wide | grep sysguardd
```

Expected: One pod per node.

---

### Policy Testing: Monitor → Enforce Workflow

#### 1. Deploy in Monitor Mode (Default)

```bash
helm install sysguardd ./helm --namespace kube-system --set sysguardd.mode=monitor
```

#### 2. Generate Traffic and Observe Logs

```bash
# Create a test pod that violates policy
kubectl run test-shell --image=alpine:latest --rm -it -- /bin/sh

# In another terminal, watch logs for policy violations
kubectl logs -n kube-system -l app.kubernetes.io/name=sysguardd \
  --tail=0 -f | grep "deny"
```

#### 3. Review Deny Events

```bash
# Get all events
kubectl get events -n kube-system --sort-by='.lastTimestamp'

# Watch for enforcement events
kubectl get events -n kube-system -w | grep sysguardd
```

#### 4. Update Policy if Needed

```bash
# Edit ConfigMap
kubectl edit cm sysguardd-policy -n kube-system

# Pods will automatically reload policy
```

#### 5. Switch to Enforce Mode

After validating the policy is correct:

```bash
helm upgrade sysguardd ./helm \
  --namespace kube-system \
  --set sysguardd.mode=enforce
```

#### 6. Verify Enforcement

Test pods that violate policy should now be terminated:

```bash
# Try to execute a denied command - expect SIGKILL
kubectl run test-violation --image=alpine:latest -- /bin/nc localhost 1234
kubectl get pods -l run=test-violation  # Pod should show CrashLoopBackOff or Terminated
```

---

## Troubleshooting

### Pod Not Starting

```bash
# Check pod status
kubectl describe pod <pod-name> -n kube-system

# View init logs
kubectl logs <pod-name> -c sysguardd -n kube-system --previous
```

### Permission Denied / RBAC Issues

```bash
# Verify ServiceAccount permissions
kubectl auth can-i read pods --as=system:serviceaccount:kube-system:sysguardd
kubectl auth can-i read nodes --as=system:serviceaccount:kube-system:sysguardd
```

### Policy Not Loading

```bash
# Verify ConfigMap exists and is readable
kubectl get cm sysguardd-policy -n kube-system
kubectl describe cm sysguardd-policy -n kube-system

# Check volume mount in pod
kubectl exec -it <pod-name> -n kube-system -- ls -la /etc/sysguardd/
```

### High CPU / Memory Usage

Check resource requests/limits in `values.yaml`:

```bash
# View current limits
kubectl describe ds sysguardd -n kube-system | grep -A 5 "Limits\|Requests"

# Adjust via Helm
helm upgrade sysguardd ./helm \
  --namespace kube-system \
  --set resources.limits.cpu=1000m \
  --set resources.limits.memory=1Gi
```

---

## Docker Image Building

Create a `Dockerfile` in the project root:

```dockerfile
# Build stage
FROM ubuntu:22.04 as builder

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    clang \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libpthread-stubs0 \
    && rm -rf /var/lib/apt/lists/* && \
    adduser --system --no-create-home sysguardd

COPY --from=builder /src/build/sysguardd /usr/local/bin/
COPY policies/default.policy /etc/sysguardd/default.policy

USER sysguardd
ENTRYPOINT ["/usr/local/bin/sysguardd"]
CMD ["daemon", "--mode", "monitor", "--policy", "/etc/sysguardd/default.policy"]
```

Build and push:

```bash
docker build -t sysguardd:latest .
docker tag sysguardd:latest sysguardd:v1.0.0
# docker push <registry>/sysguardd:v1.0.0
```

---

## Next Steps (Phase 2)

- [ ] Multi-tenant policy isolation (namespace-scoped policies)
- [ ] gRPC telemetry export to observability stacks
- [ ] eBPF-based kernel probes for real process execution visibility
- [ ] Prometheus metrics export
- [ ] Integration tests with real container runtime violations
- [ ] Helm chart repository (ChartMuseum or GitHub Releases)
- [ ] ArgoCD/Flux GitOps examples
