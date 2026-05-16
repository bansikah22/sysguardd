# Setting Up Helm Chart Publishing

This guide explains how to configure automated Helm chart publishing to Docker Hub OCI registry.

## Prerequisites

- Docker Hub account (free)
- Repository access on GitHub
- Admin or maintainer permissions

## Setup Steps

### Step 1: Create Docker Hub Access Token

1. Go to [Docker Hub](https://hub.docker.com)
2. Sign in with your account
3. Click profile icon → **Account Settings**
4. Select **Security** → **New Access Token**
5. Enter token name: `github-helm-publish`
6. Set permissions: **Read & Write** (for pushing images/charts)
7. Click **Generate** and copy the token

**Example token**: `dckr_pat_XXXXXXXXXXXXXXXXXXXXX`

### Step 2: Add GitHub Secrets

1. Go to your GitHub repository settings
2. Navigate to **Secrets and variables** → **Actions**
3. Click **New repository secret**

Create two secrets:

| Secret Name | Value |
|------------|-------|
| `DOCKER_USERNAME` | Your Docker Hub username (e.g., `bansikah`) |
| `DOCKER_PASSWORD` | Access token from Step 1 |

### Step 3: Verify Workflow Configuration

Check [Workflow File](https://github.com/bansikah22/sysguardd/blob/master/.github/workflows/helm-oci-push.yml):

```yaml
- name: Log in to Docker Hub
  uses: docker/login-action@v3
  with:
    registry: ${{ env.REGISTRY }}
    username: ${{ secrets.DOCKER_USERNAME }}
    password: ${{ secrets.DOCKER_PASSWORD }}
```

### Step 4: Test the Workflow

#### Option A: Manual Trigger (Recommended for first test)

1. Go to **Actions** tab in GitHub
2. Select **Helm Chart OCI Push** workflow
3. Click **Run workflow** → **Run workflow**
4. Wait for completion (usually 1-2 minutes)

#### Option B: Push Trigger

Make a change to `helm/Chart.yaml`:

```bash
git push origin feat/kubernetes-testing
```

This will automatically trigger the workflow.

### Step 5: Verify Chart Published

After workflow completes:

```bash
# List chart tags in registry
helm pull oci://registry-1.docker.io/bansikah/sysguardd-helm --version 1.0.0 --untar

# Verify chart contents
cat sysguardd/Chart.yaml
```

Or check Docker Hub:

1. Go to [Docker Hub Repositories](https://hub.docker.com)
2. Look for `sysguardd-helm` repository
3. Verify in **Tags** section

## Publish Modes

The workflow automatically publishes when:

### 1. **Manual Trigger** (Fastest for testing)

```bash
# GitHub CLI
gh workflow run helm-oci-push.yml

# Or via GitHub web UI
```

### 2. **On Changes** (Recommended for production)

Automatically publishes when `helm/` directory changes:

```bash
git commit -m "Update Helm chart policy"
git push origin main
# Workflow triggers automatically
```

### 3. **On Release** (For coordinated releases)

Publishes when GitHub Release is created:

```bash
# Create release via GitHub web UI or CLI
gh release create v0.1.0 --title "Release 0.1.0"
# Workflow triggers automatically
```

**Note**: Workflow comments on the release with install instructions.

## Workflow Details

### Inputs (for manual trigger)

```
chart_version: "1.0.0"  # Optional, defaults to Chart.yaml version
```

### Outputs

The workflow:

1. DONE: Extracts version from `Chart.yaml`
2. DONE: Packages chart with `helm package`
3. DONE: Authenticates to Docker Hub
4. DONE: Pushes to OCI registry (`helm push`)
5. DONE: Verifies chart in registry
6. DONE: Comments on release (if applicable)

### Registry Output

```
oci://registry-1.docker.io/bansikah/sysguardd-helm:1.0.0
                                                   ↑
                                    Chart version from Chart.yaml
```

## Publishing Your First Chart

### Quick Start

```bash
# 1. Ensure secrets are configured
# 2. Go to Actions → Helm Chart OCI Push → Run workflow

# 3. Once complete, chart is available at:
# oci://registry-1.docker.io/bansikah/sysguardd-helm:1.0.0

# 4. Users can install with:
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm
```

### Example: Release New Chart Version

```bash
# 1. Update Chart.yaml version
# vim helm/Chart.yaml
# version: 1.1.0

# 2. Commit and push
git add helm/Chart.yaml
git commit -m "Release: Update chart to 1.1.0"
git push origin main

# Workflow triggers automatically and publishes to:
# oci://registry-1.docker.io/bansikah/sysguardd-helm:1.1.0

# 3. Create GitHub release (optional but recommended)
gh release create v1.1.0 --title "Chart 1.1.0"
```

## Troubleshooting

### Workflow Fails: "Invalid Username or Password"

```
Error: docker login failed
```

**Solution**:
- Verify secrets are spelled correctly: `DOCKER_USERNAME`, `DOCKER_PASSWORD`
- Check Docker Hub credentials are still valid
- Generate new access token if expired

### Workflow Fails: "Permission Denied"

```
Error: failed to push
```

**Solution**:
- Verify Docker Hub account owns `bansikah` namespace
- Check access token has **Read & Write** permissions
- Verify image name in `Image_NAME` env var

### Chart Not Found After Publishing

```
Error: failed to pull chart
```

**Solution**:
```bash
# Verify chart version matches what was pushed
helm pull oci://registry-1.docker.io/bansikah/sysguardd-helm --version 1.0.0 --untar

# Check available versions
helm search repo sysguardd

# Explicitly specify registry URL
helm install sysguardd oci://registry-1.docker.io/bansikah/sysguardd-helm
```

### Workflow Hangs or Times Out

**Solution**:
- Check Docker Hub service status: https://status.docker.com
- Verify network connectivity
- Retry manual workflow trigger

## Maintenance

### Updating Registry Credentials

If access token expires:

1. Generate new token in Docker Hub
2. Update GitHub secret: `DOCKER_PASSWORD`
3. Workflow will use new credentials on next run

### Monitoring Workflow Status

```bash
# GitHub CLI
gh workflow view helm-oci-push.yml

# Or check GitHub Actions tab in web UI
```

### Rollback (if needed)

Push previous chart version:

```bash
# Checkout previous commit
git checkout HEAD~1 helm/Chart.yaml

# Manually trigger workflow or push
gh workflow run helm-oci-push.yml
```

## Next Steps

1. DONE: Configure GitHub Secrets (DOCKER_USERNAME, DOCKER_PASSWORD)
2. DONE: Test workflow with manual trigger
3. DONE: Verify chart published to OCI registry
4. DONE: Update repository documentation
5. DONE: Announce chart availability to users

## References

- [Docker Hub Access Tokens](https://docs.docker.com/docker-hub/access-tokens/)
- [GitHub Secrets](https://docs.github.com/actions/security-guides/encrypted-secrets)
- [Helm OCI Registry Guide](https://helm.sh/docs/topics/registries/)
- [Workflow File](https://github.com/bansikah22/sysguardd/blob/master/.github/workflows/helm-oci-push.yml)
