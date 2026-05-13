#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Install and manage SysGuardd systemd service.

Usage:
  scripts/install-service.sh [options]

Options:
  --prefix <path>        Binary prefix (default: /usr/local)
  --policy <path>        Policy file path (default: /etc/sysguardd/default.policy)
  --mode <mode>          Runtime mode monitor|enforce (default: monitor)
  --enable               Enable service at boot
  --start                Start service after install
  --restart              Restart service after install
  --status               Show service status at end
  --uninstall            Remove service unit
  -h, --help             Show this help
EOF
}

need_sudo() {
  if [[ "${EUID}" -eq 0 ]]; then
    echo ""
  else
    echo "sudo"
  fi
}

ensure_linux_systemd() {
  if [[ "$(uname -s)" != "Linux" ]]; then
    echo "systemd service installation is only supported on Linux." >&2
    exit 1
  fi
  if ! command -v systemctl >/dev/null 2>&1; then
    echo "systemctl not found. This system does not appear to use systemd." >&2
    exit 1
  fi
}

main() {
  local prefix="/usr/local"
  local policy="/etc/sysguardd/default.policy"
  local mode="monitor"
  local do_enable=0
  local do_start=0
  local do_restart=0
  local show_status=0
  local uninstall=0

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --prefix)
        [[ $# -lt 2 ]] && { echo "--prefix requires a value" >&2; exit 2; }
        prefix="$2"
        shift 2
        ;;
      --policy)
        [[ $# -lt 2 ]] && { echo "--policy requires a value" >&2; exit 2; }
        policy="$2"
        shift 2
        ;;
      --mode)
        [[ $# -lt 2 ]] && { echo "--mode requires a value" >&2; exit 2; }
        mode="$2"
        shift 2
        ;;
      --enable)
        do_enable=1
        shift
        ;;
      --start)
        do_start=1
        shift
        ;;
      --restart)
        do_restart=1
        shift
        ;;
      --status)
        show_status=1
        shift
        ;;
      --uninstall)
        uninstall=1
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

  if [[ "${mode}" != "monitor" && "${mode}" != "enforce" ]]; then
    echo "--mode must be monitor or enforce" >&2
    exit 2
  fi

  ensure_linux_systemd

  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  local repo_dir
  repo_dir="$(cd "${script_dir}/.." && pwd)"

  local unit_src="${repo_dir}/systemd/sysguardd.service"
  local unit_dst="/etc/systemd/system/sysguardd.service"
  local s
  s="$(need_sudo)"

  if [[ "${uninstall}" -eq 1 ]]; then
    ${s} systemctl disable --now sysguardd.service >/dev/null 2>&1 || true
    ${s} rm -f "${unit_dst}"
    ${s} systemctl daemon-reload
    echo "Removed sysguardd.service"
    exit 0
  fi

  local tmp
  tmp="$(mktemp)"
  cp "${unit_src}" "${tmp}"
  sed -i "s|^ExecStart=.*|ExecStart=${prefix}/bin/sysguardd --mode ${mode} --policy ${policy}|" "${tmp}"

  ${s} install -m 0644 "${tmp}" "${unit_dst}"
  rm -f "${tmp}"

  ${s} systemctl daemon-reload

  if [[ "${do_enable}" -eq 1 ]]; then
    ${s} systemctl enable sysguardd.service
  fi

  if [[ "${do_start}" -eq 1 ]]; then
    ${s} systemctl start sysguardd.service
  fi

  if [[ "${do_restart}" -eq 1 ]]; then
    ${s} systemctl restart sysguardd.service
  fi

  if [[ "${show_status}" -eq 1 ]]; then
    ${s} systemctl status --no-pager sysguardd.service || true
  fi

  echo "Installed systemd unit: ${unit_dst}"
  echo "Use: systemctl status sysguardd.service"
}

main "$@"
