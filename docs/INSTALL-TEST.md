# Installation and Testing Guide

This page is primarily for end users installing SysGuardd on Linux hosts.

If you are a contributor or maintainer, use [Maintainer Guide](MAINTAINERS.md) for internal release and publishing workflows.

## Supported Installation Paths

Recommended for most users:
- Use Section 1 (scripted installation).
- Then run the runtime smoke test in the testing section.

### 1) Scripted Installation (Cross-Platform)
Use project scripts for macOS and major Linux families.

Universal installer script (developer + non-developer):

```bash
./scripts/install.sh --smoke-test
```

Linux + systemd install (install, enable, start service):

```bash
./scripts/install.sh --systemd
```

Curl or wget style usage (remote mode):

```bash
curl -fsSL https://raw.githubusercontent.com/bansikah22/sysguardd/master/scripts/install.sh -o install.sh && bash install.sh --repo-url https://github.com/bansikah22/sysguardd.git --ref master --smoke-test
```

```bash
wget -qO install.sh https://raw.githubusercontent.com/bansikah22/sysguardd/master/scripts/install.sh && bash install.sh --repo-url https://github.com/bansikah22/sysguardd.git --ref master --smoke-test
```

Non-developer one-command setup (install + build + test + system install):

```bash
./scripts/nondev-setup.sh
```

Optional smoke test after setup:

```bash
./scripts/nondev-setup.sh --smoke-test
```

Install dependencies only:

```bash
./scripts/install-deps.sh
```

Run end-to-end bootstrap (deps + build + test):

```bash
./scripts/bootstrap.sh
```

Optional install of binary and default policy:

```bash
./scripts/bootstrap.sh --install
```

Supported by script detection:
- Ubuntu or Debian
- Fedora, RHEL, Rocky, AlmaLinux
- Arch Linux
- openSUSE or SLES
- macOS (with Homebrew)

Notes for non-developers:
- The script installs the binary to `/usr/local/bin/sysguardd` by default.
- The script installs the status command to `/usr/local/bin/sysguardd-status`.
- The script installs the default policy to `/etc/sysguardd/default.policy`.
- You can change binary install location with `--prefix`.

### 1.1) Check Agent Status After Installation
Use the installed status helper:

```bash
sysguardd-status
```

Require the process to be running:

```bash
sysguardd-status --require-running
```

Machine-readable status output:

```bash
sysguardd-status --json
```

If running directly from repository scripts:

```bash
./scripts/status.sh
```

### 1.2) Manage Linux systemd Service
Install or update systemd unit manually:

```bash
./scripts/install-service.sh --enable --start --status
```

Restart service after config or policy changes:

```bash
./scripts/install-service.sh --restart --status
```

Uninstall systemd unit:

```bash
./scripts/install-service.sh --uninstall
```

Direct systemctl commands:

```bash
sudo systemctl status sysguardd.service
sudo systemctl restart sysguardd.service
sudo systemctl stop sysguardd.service
```

### 2) Ubuntu Package Installation (Recommended)
Install required toolchain packages:

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential ninja-build clang clang-tidy cppcheck
```

Use this path when developing directly on Ubuntu.

### 3) Source-Only Build with Existing Toolchain
If your machine already has CMake and a C/C++ compiler:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

No package manager steps are required in this path.

### 4) Contributor Validation in CI (Maintainers)
The repository includes a hardened CI workflow at [ci.yml](https://github.com/bansikah22/sysguardd/blob/master/.github/workflows/ci.yml).

CI provisions dependencies on every run and executes:
- multi-compiler build matrix (GCC and Clang)
- sanitizer-enabled debug run
- static analysis with cppcheck and clang-tidy

This path is intended for maintainers and contributors who need reproducible validation without local setup drift.

## Testing Methods

For end users, start with the runtime smoke test.
Sections for sanitizer and static analysis are primarily contributor-focused.

### 1) Unit and Integration Baseline Tests
Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

### 2) Sanitizer-Enabled Validation
Use Debug mode with sanitizers enabled:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DSYSGUARDD_ENABLE_SANITIZERS=ON
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
```

### 3) Release Build Validation
Run strict release checks:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DSYSGUARDD_ENABLE_SANITIZERS=OFF
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

### 4) Static Analysis
Run cppcheck:

```bash
cppcheck --enable=warning,performance,portability --std=c++20 --error-exitcode=1 -I include src tests
```

Run clang-tidy (after CMake configure with compile commands):

```bash
cmake -S . -B build-analyze -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy src/*.cpp tests/*.cpp -p build-analyze -checks=-*,clang-analyzer-*,bugprone-*
```

### 5) Runtime Smoke Test
Monitor mode smoke test with sample input:

```bash
printf "1200 1 /usr/bin/bash -c whoami\n" | ./build/sysguardd --mode monitor --policy ./policies/default.policy
```

## Security-Focused Test Expectations
- Every deny decision emits an audit record.
- Enforce mode attempts mitigation only for denied events.
- Dangerous kill targets are rejected (PID 1 and self PID).
- Relative or malformed executable paths are denied by policy engine.
