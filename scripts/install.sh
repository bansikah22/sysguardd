#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Universal SysGuardd installer for developers and non-developers.

This script works in two modes:
1) Local mode (run inside cloned repository)
2) Remote mode (script fetched via curl/wget) by cloning repo automatically

Usage:
  scripts/install.sh [options]

Options:
  --repo-url <url>      Git repository URL (required when not in repo)
  --ref <name>          Git ref to checkout (branch/tag/commit)
  --prefix <path>       Install prefix for binary (default: /usr/local)
  --skip-deps           Skip dependency installation
  --smoke-test          Run smoke test after install
  --systemd             Install and enable/start systemd service on Linux
  --no-systemd          Do not install systemd service, even on Linux
  --keep-workdir        Keep temporary clone directory in remote mode
  -h, --help            Show this help

Examples:
  # Local repository mode
  ./scripts/install.sh --smoke-test

  # Then check status
  sysguardd-status

  # Remote mode (for curl/wget usage)
  bash install.sh --repo-url https://example.com/org/sysguardd.git --ref main --smoke-test
EOF
}

run_local_install() {
  local repo_dir="$1"
  local prefix="$2"
  local skip_deps="$3"
  local smoke_test="$4"
  local systemd_mode="$5"

  local args=(--prefix "${prefix}")
  if [[ "${skip_deps}" == "1" ]]; then
    args+=(--skip-deps)
  fi
  if [[ "${smoke_test}" == "1" ]]; then
    args+=(--smoke-test)
  fi
  if [[ "${systemd_mode}" == "on" ]]; then
    args+=(--systemd)
  elif [[ "${systemd_mode}" == "off" ]]; then
    args+=(--no-systemd)
  fi

  "${repo_dir}/scripts/nondev-setup.sh" "${args[@]}"
}

main() {
  local repo_url=""
  local ref=""
  local prefix="/usr/local"
  local skip_deps=0
  local smoke_test=0
  local keep_workdir=0
  local systemd_mode="auto"

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --repo-url)
        [[ $# -lt 2 ]] && { echo "--repo-url requires a value" >&2; exit 2; }
        repo_url="$2"
        shift 2
        ;;
      --ref)
        [[ $# -lt 2 ]] && { echo "--ref requires a value" >&2; exit 2; }
        ref="$2"
        shift 2
        ;;
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
      --keep-workdir)
        keep_workdir=1
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
  local local_repo_dir
  local_repo_dir="$(cd "${script_dir}/.." && pwd)"

  if [[ -x "${local_repo_dir}/scripts/nondev-setup.sh" && -f "${local_repo_dir}/CMakeLists.txt" ]]; then
    run_local_install "${local_repo_dir}" "${prefix}" "${skip_deps}" "${smoke_test}" "${systemd_mode}"
    exit 0
  fi

  if [[ -z "${repo_url}" ]]; then
    cat <<'EOF' >&2
Not running from a SysGuardd repository checkout.
Provide --repo-url for remote install mode.
EOF
    exit 1
  fi

  if ! command -v git >/dev/null 2>&1; then
    echo "git is required for remote install mode." >&2
    exit 1
  fi

  # Validate repo_url is a safe git URL to prevent command injection (CWE-78)
  if [[ ! "${repo_url}" =~ ^(https?|git|ssh)://[a-zA-Z0-9._/-]+\.git$ ]] && \
     [[ ! "${repo_url}" =~ ^git@[a-zA-Z0-9._-]+:[a-zA-Z0-9._/-]+\.git$ ]]; then
    echo "Invalid repository URL format. Must be https://, git://, ssh:// or git@host:path format." >&2
    exit 1
  fi

  local workdir
  workdir="$(mktemp -d -t sysguardd-install-XXXXXX)"

  if [[ "${keep_workdir}" -eq 0 ]]; then
    trap 'rm -rf "${workdir}"' EXIT
  fi

  echo "Cloning SysGuardd repository to ${workdir}"
  git clone --depth 1 "${repo_url}" "${workdir}/repo"

  if [[ -n "${ref}" ]]; then
    # Validate ref contains only safe characters to prevent command injection (CWE-78)
    if [[ ! "${ref}" =~ ^[a-zA-Z0-9/_\\.~-]+$ ]]; then
      echo "Invalid git ref format. Must contain only alphanumeric, /, _, ., ~, or - characters." >&2
      rm -rf "${workdir}"
      exit 1
    fi
    git -C "${workdir}/repo" fetch --depth 1 origin "${ref}" || true
    git -C "${workdir}/repo" checkout "${ref}"
  fi

  run_local_install "${workdir}/repo" "${prefix}" "${skip_deps}" "${smoke_test}" "${systemd_mode}"

  if [[ "${keep_workdir}" -eq 1 ]]; then
    echo "Temporary repository kept at: ${workdir}/repo"
  fi
}

main "$@"
