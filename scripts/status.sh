#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Check SysGuardd installation and runtime status.

Usage:
  scripts/status.sh [options]

Options:
  --binary <path>         SysGuardd binary path (default: /usr/local/bin/sysguardd)
  --policy <path>         Policy file path (default: /etc/sysguardd/default.policy)
  --require-running       Exit non-zero if process is not running
  --json                  Output status as JSON
  -h, --help              Show this help
EOF
}

json_escape() {
  local s="$1"
  s="${s//\\/\\\\}"
  s="${s//\"/\\\"}"
  s="${s//$'\n'/\\n}"
  s="${s//$'\r'/\\r}"
  s="${s//$'\t'/\\t}"
  printf '%s' "$s"
}

main() {
  local binary_path="/usr/local/bin/sysguardd"
  local policy_path="/etc/sysguardd/default.policy"
  local require_running=0
  local json=0

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --binary)
        [[ $# -lt 2 ]] && { echo "--binary requires a value" >&2; exit 2; }
        binary_path="$2"
        shift 2
        ;;
      --policy)
        [[ $# -lt 2 ]] && { echo "--policy requires a value" >&2; exit 2; }
        policy_path="$2"
        shift 2
        ;;
      --require-running)
        require_running=1
        shift
        ;;
      --json)
        json=1
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

  local binary_ok=0
  local policy_exists=0
  local policy_readable=0
  local installed=0
  local running=0
  local pids=""
  local systemd_active="unknown"
  local systemd_enabled="unknown"

  if [[ -x "${binary_path}" ]]; then
    binary_ok=1
  fi

  if [[ -e "${policy_path}" ]]; then
    policy_exists=1
  fi

  if [[ -r "${policy_path}" ]]; then
    policy_readable=1
  fi

  if [[ "${binary_ok}" -eq 1 && "${policy_exists}" -eq 1 ]]; then
    installed=1
  fi

  if command -v pgrep >/dev/null 2>&1; then
    pids="$(pgrep -fa sysguardd || true)"
    if [[ -n "${pids}" ]]; then
      running=1
    fi
  fi

  if command -v systemctl >/dev/null 2>&1; then
    if systemctl list-unit-files | grep -q '^sysguardd\.service'; then
      if systemctl is-active --quiet sysguardd.service; then
        systemd_active="active"
      else
        systemd_active="inactive"
      fi

      if systemctl is-enabled --quiet sysguardd.service; then
        systemd_enabled="enabled"
      else
        systemd_enabled="disabled"
      fi
    fi
  fi

  if [[ "${json}" -eq 1 ]]; then
    printf '{'
    printf '"installed":%s,' "$( [[ ${installed} -eq 1 ]] && echo true || echo false )"
    printf '"binary_ok":%s,' "$( [[ ${binary_ok} -eq 1 ]] && echo true || echo false )"
    printf '"policy_exists":%s,' "$( [[ ${policy_exists} -eq 1 ]] && echo true || echo false )"
    printf '"policy_readable":%s,' "$( [[ ${policy_readable} -eq 1 ]] && echo true || echo false )"
    printf '"running":%s,' "$( [[ ${running} -eq 1 ]] && echo true || echo false )"
    printf '"binary_path":"%s",' "$(json_escape "${binary_path}")"
    printf '"policy_path":"%s",' "$(json_escape "${policy_path}")"
    printf '"systemd_active":"%s",' "$(json_escape "${systemd_active}")"
    printf '"systemd_enabled":"%s",' "$(json_escape "${systemd_enabled}")"
    printf '"pids":"%s"' "$(json_escape "${pids}")"
    printf '}\n'
  else
    echo "SysGuardd Status"
    echo "- Installed: $( [[ ${installed} -eq 1 ]] && echo yes || echo no )"
    echo "- Binary (${binary_path}): $( [[ ${binary_ok} -eq 1 ]] && echo present || echo missing )"
    echo "- Policy (${policy_path}): $( [[ ${policy_exists} -eq 1 ]] && echo present || echo missing )"
    echo "- Policy readable by current user: $( [[ ${policy_readable} -eq 1 ]] && echo yes || echo no )"
    echo "- Running process: $( [[ ${running} -eq 1 ]] && echo yes || echo no )"
    if [[ -n "${pids}" ]]; then
      echo "- Process details:"
      echo "${pids}"
    fi
    echo "- systemd active: ${systemd_active}"
    echo "- systemd enabled: ${systemd_enabled}"
  fi

  if [[ "${installed}" -ne 1 ]]; then
    exit 1
  fi

  if [[ "${require_running}" -eq 1 && "${running}" -ne 1 ]]; then
    exit 3
  fi

  exit 0
}

main "$@"
