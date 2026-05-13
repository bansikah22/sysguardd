#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Bootstrap SysGuardd locally (deps, build, test, optional install).

Usage:
  scripts/bootstrap.sh [options]

Options:
  --skip-deps            Skip dependency installation
  --build-type <type>    CMake build type (Debug|Release), default: Debug
  --no-sanitizers        Disable sanitizers
  --build-dir <path>     Build directory, default: build
  --install              Install binary and policy to system paths
  --prefix <path>        Install prefix, default: /usr/local
  --systemd              Install systemd unit on Linux
  --systemd-enable       Enable systemd unit at boot (implies --systemd)
  --systemd-start        Start systemd unit now (implies --systemd)
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

main() {
  local skip_deps=0
  local build_type="Debug"
  local sanitizers="ON"
  local build_dir="build"
  local do_install=0
  local prefix="/usr/local"
  local do_systemd=0
  local do_systemd_enable=0
  local do_systemd_start=0

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --skip-deps)
        skip_deps=1
        shift
        ;;
      --build-type)
        [[ $# -lt 2 ]] && { echo "--build-type requires a value" >&2; exit 2; }
        build_type="$2"
        shift 2
        ;;
      --no-sanitizers)
        sanitizers="OFF"
        shift
        ;;
      --build-dir)
        [[ $# -lt 2 ]] && { echo "--build-dir requires a value" >&2; exit 2; }
        build_dir="$2"
        shift 2
        ;;
      --install)
        do_install=1
        shift
        ;;
      --prefix)
        [[ $# -lt 2 ]] && { echo "--prefix requires a value" >&2; exit 2; }
        prefix="$2"
        shift 2
        ;;
      --systemd)
        do_systemd=1
        shift
        ;;
      --systemd-enable)
        do_systemd=1
        do_systemd_enable=1
        shift
        ;;
      --systemd-start)
        do_systemd=1
        do_systemd_start=1
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
  local repo_dir
  repo_dir="$(cd "${script_dir}/.." && pwd)"

  if [[ "${skip_deps}" -eq 0 ]]; then
    "${script_dir}/install-deps.sh"
  fi

  cmake -S "${repo_dir}" -B "${repo_dir}/${build_dir}" \
    -DCMAKE_BUILD_TYPE="${build_type}" \
    -DSYSGUARDD_ENABLE_SANITIZERS="${sanitizers}"

  cmake --build "${repo_dir}/${build_dir}" --parallel
  ctest --test-dir "${repo_dir}/${build_dir}" --output-on-failure

  if [[ "${do_install}" -eq 1 ]]; then
    local s
    s="$(need_sudo)"

    ${s} install -d -m 0755 "${prefix}/bin"
    ${s} install -m 0755 "${repo_dir}/${build_dir}/sysguardd" "${prefix}/bin/sysguardd"
    ${s} install -m 0755 "${repo_dir}/scripts/status.sh" "${prefix}/bin/sysguardd-status"

    ${s} install -d -m 0755 /etc/sysguardd
    ${s} install -m 0600 "${repo_dir}/policies/default.policy" /etc/sysguardd/default.policy

    echo "Installed binary to ${prefix}/bin/sysguardd"
    echo "Installed status helper to ${prefix}/bin/sysguardd-status"
    echo "Installed policy to /etc/sysguardd/default.policy"

    if [[ "${do_systemd}" -eq 1 ]]; then
      local svc_args=(--prefix "${prefix}" --policy /etc/sysguardd/default.policy --mode monitor)
      if [[ "${do_systemd_enable}" -eq 1 ]]; then
        svc_args+=(--enable)
      fi
      if [[ "${do_systemd_start}" -eq 1 ]]; then
        svc_args+=(--start)
      fi
      "${script_dir}/install-service.sh" "${svc_args[@]}"
    fi
  fi

  echo "Bootstrap completed successfully."
}

main "$@"
