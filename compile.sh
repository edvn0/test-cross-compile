#!/usr/bin/env bash

set -euo pipefail

readonly image="wsl-cross-new:latest"
readonly project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly toolchain_dir="${HOME}/d/cmake"
readonly toolchain_file="/toolchains/windows-mingw-x64.cmake"
readonly build_dir="build/windows-mingw-release"
readonly build_type="${CMAKE_BUILD_TYPE:-Debug}"
readonly cpm_cache_dir="${HOME}/.cache/CPM"
readonly container_cpm_cache="/cpm-cache"

usage() {
  cat <<EOF
Usage:
  ./build.sh --configure
  ./build.sh --build
  ./build.sh --rebuild
  ./build.sh --clean
  ./build.sh --shell

Options:
  --configure  Configure the Windows MinGW build with CMake
  --build      Build the existing configuration
  --rebuild    Remove, configure, and build
  --clean      Remove the Windows MinGW build directory
  --shell      Open an interactive shell inside the build container
EOF
}

run_container() {
  docker run --rm \
    --mount "type=bind,source=${project_dir},target=/workspace" \
    --mount "type=bind,source=${toolchain_dir},target=/toolchains,readonly" \
    --mount "type=bind,source=${cpm_cache_dir},target=${container_cpm_cache}" \
    --env "CPM_SOURCE_CACHE=${container_cpm_cache}" \
    --workdir /workspace \
    "${image}" \
    "$@"
}

configure() {
  mkdir -p "${cpm_cache_dir}"
  run_container \
    cmake \
    -S . \
    -B "${build_dir}" \
    -G Ninja \
    --toolchain "${toolchain_file}" \
    -DCMAKE_BUILD_TYPE="${build_type}"
}

build() {
  run_container \
    cmake \
    --build "${build_dir}" \
    --parallel
}

clean() {
  rm -rf "${project_dir:?}/${build_dir}"
}

shell() {
  docker run --rm -it \
    --mount "type=bind,source=${project_dir},target=/workspace" \
    --mount "type=bind,source=${toolchain_dir},target=/toolchains,readonly" \
    --workdir /workspace \
    "${image}" \
    bash
}

main() {
  if [[ $# -ne 1 ]]; then
    usage
    exit 1
  fi

  case "$1" in
  --configure)
    configure
    ;;
  --build)
    build
    ;;
  --rebuild)
    clean
    configure
    build
    ;;
  --clean)
    clean
    ;;
  --shell)
    shell
    ;;
  --help | -h)
    usage
    ;;
  *)
    echo "Unknown option: $1" >&2
    usage >&2
    exit 1
    ;;
  esac
}

main "$@"
