#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Install SysGuardd build dependencies.

Usage:
  scripts/install-deps.sh [--minimal]

Options:
  --minimal   Install only build essentials (skip clang-tidy/cppcheck when possible)
  -h, --help  Show this help
EOF
}

need_sudo() {
  if [[ "${EUID}" -eq 0 ]]; then
    echo ""
  else
    echo "sudo"
  fi
}

install_ubuntu_debian() {
  local minimal="$1"
  local s
  s="$(need_sudo)"

  ${s} apt-get update
  if [[ "${minimal}" == "1" ]]; then
    ${s} apt-get install -y cmake build-essential ninja-build
  else
    ${s} apt-get install -y cmake build-essential ninja-build clang clang-tidy cppcheck
  fi
}

install_fedora_rhel() {
  local minimal="$1"
  local s
  s="$(need_sudo)"

  ${s} dnf makecache -y
  if [[ "${minimal}" == "1" ]]; then
    ${s} dnf install -y cmake gcc-c++ make ninja-build
  else
    ${s} dnf install -y cmake gcc-c++ make ninja-build clang-tools-extra cppcheck
  fi
}

install_arch() {
  local minimal="$1"
  local s
  s="$(need_sudo)"

  ${s} pacman -Sy --noconfirm
  if [[ "${minimal}" == "1" ]]; then
    ${s} pacman -S --needed --noconfirm cmake gcc make ninja
  else
    ${s} pacman -S --needed --noconfirm cmake gcc make ninja clang cppcheck
  fi
}

install_opensuse() {
  local minimal="$1"
  local s
  s="$(need_sudo)"

  ${s} zypper refresh
  if [[ "${minimal}" == "1" ]]; then
    ${s} zypper install -y cmake gcc-c++ make ninja
  else
    ${s} zypper install -y cmake gcc-c++ make ninja clang-tools cppcheck
  fi
}

install_macos() {
  local minimal="$1"
  if ! command -v brew >/dev/null 2>&1; then
    cat <<'EOF'
Homebrew not found.
Install from: https://brew.sh/
Then rerun this script.
EOF
    exit 1
  fi

  brew update
  if [[ "${minimal}" == "1" ]]; then
    brew install cmake ninja
  else
    brew install cmake ninja llvm cppcheck
  fi
}

main() {
  local minimal=0

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --minimal)
        minimal=1
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

  case "$(uname -s)" in
    Darwin)
      install_macos "${minimal}"
      ;;
    Linux)
      if [[ -r /etc/os-release ]]; then
        # shellcheck disable=SC1091
        source /etc/os-release
      fi

      local id_like_lc="${ID_LIKE:-}"
      id_like_lc="${id_like_lc,,}"
      local id_lc="${ID:-}"
      id_lc="${id_lc,,}"

      if [[ "${id_lc}" == "ubuntu" || "${id_lc}" == "debian" || "${id_like_lc}" == *"debian"* ]]; then
        install_ubuntu_debian "${minimal}"
      elif [[ "${id_lc}" == "fedora" || "${id_lc}" == "rhel" || "${id_lc}" == "rocky" || "${id_lc}" == "almalinux" || "${id_like_lc}" == *"rhel"* || "${id_like_lc}" == *"fedora"* ]]; then
        install_fedora_rhel "${minimal}"
      elif [[ "${id_lc}" == "arch" || "${id_like_lc}" == *"arch"* ]]; then
        install_arch "${minimal}"
      elif [[ "${id_lc}" == "opensuse"* || "${id_lc}" == "sles" || "${id_like_lc}" == *"suse"* ]]; then
        install_opensuse "${minimal}"
      else
        cat <<EOF
Unsupported Linux distribution: ID=${ID:-unknown}, ID_LIKE=${ID_LIKE:-unknown}
Install manually using docs/INSTALL-TEST.md.
EOF
        exit 1
      fi
      ;;
    *)
      echo "Unsupported OS: $(uname -s)" >&2
      exit 1
      ;;
  esac

  echo "Dependency installation completed successfully."
}

main "$@"
