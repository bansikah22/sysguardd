#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Non-developer setup helper for SysGuardd.

This script installs dependencies, builds SysGuardd in Release mode,
installs the binary and default policy, and optionally runs a smoke test.

Usage:
  scripts/nondev-setup.sh [options]

Options:
  --prefix <path>      Install prefix for binary (default: /usr/local)
  --skip-deps          Skip dependency installation
  --smoke-test         Run a monitor-mode smoke test after installation
  --systemd            Install and enable/start systemd service on Linux
  --no-systemd         Do not install systemd service, even on Linux
  -h, --help           Show this help
EOF
}

main() {
  local prefix="/usr/local"
  local skip_deps=0
  local smoke_test=0
  local systemd_mode="auto"

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --prefix)
        [[ $# -lt 2 ]] && { echo "--prefix requires a value" >&2; exit 2; }
        prefix="$2"
        shift 2
        ;;
      --skip-deps)
        skip_deps=1
        shift
        ;;
      --smoke-test)
        smoke_test=1
        shift
        ;;
      --systemd)
        systemd_mode="on"
        shift
        ;;
      --no-systemd)
        systemd_mode="off"
        shift
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        echo "Unknown argument: $1" >&2
        usage
        exit 2
        ;;
    esac
  done

  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

  local args=(--build-type Release --no-sanitizers --install --prefix "${prefix}")
  if [[ "${skip_deps}" -eq 1 ]]; then
    args+=(--skip-deps)
  fi

  if [[ "${systemd_mode}" == "on" ]]; then
    args+=(--systemd --systemd-enable --systemd-start)
  elif [[ "${systemd_mode}" == "auto" ]]; then
    if [[ "$(uname -s)" == "Linux" ]] && command -v systemctl >/dev/null 2>&1; then
      args+=(--systemd --systemd-enable --systemd-start)
    fi
  fi

  "${script_dir}/bootstrap.sh" "${args[@]}"

  echo ""
  echo "SysGuardd is installed."
  echo "Binary: ${prefix}/bin/sysguardd"
  echo "Status command: ${prefix}/bin/sysguardd-status"
  echo "Policy: /etc/sysguardd/default.policy"
  echo ""
  echo "Example usage (monitor mode):"
  echo "  ${prefix}/bin/sysguardd --mode monitor --policy /etc/sysguardd/default.policy"
  echo "Check install/runtime status:"
  echo "  ${prefix}/bin/sysguardd-status"

  if [[ "${smoke_test}" -eq 1 ]]; then
    echo ""
    echo "Running smoke test..."
    printf "1200 1 /usr/bin/bash -c whoami\n" | "${prefix}/bin/sysguardd" --mode monitor --policy /etc/sysguardd/default.policy
    echo "Smoke test completed."
  fi
}

main "$@"
