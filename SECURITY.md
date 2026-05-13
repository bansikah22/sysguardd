# Security Guidelines

This project is a security control and must follow conservative secure defaults.

## Build Hardening
- Use warnings as errors (`-Werror`) for CI builds.
- Use sanitizers in debug builds (`ASAN` and `UBSAN`).
- Prefer static analysis before release.
- Require CI to pass before merge.
- Validate with both GCC and Clang in CI.

## Runtime Hardening
- Default mode should be `monitor` until policy quality is validated.
- Enforce mode should only run with reviewed deny rules.
- Reject non-absolute executable paths during policy evaluation.
- Refuse dangerous mitigation actions (PID 1 and self-kill).

## Operational Safety
- Keep policy files in restricted permissions (`0600` or stricter where possible).
- Run daemon with least privilege necessary.
- Protect audit log transport and storage.
- Rotate and monitor audit logs for tampering.

## Secure Development Practices
- Require code review for all mitigation and policy-engine changes.
- Add tests for every new deny/allow decision branch.
- Never merge code that bypasses audit emission on deny events.
- Prefer established libraries over home-grown parsers for complex formats.
- Keep `.github/workflows/ci.yml` protections enabled on protected branches.
