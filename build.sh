#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR=${BUILD_DIR:-out}
VCPKG_DIR="$SCRIPT_DIR/vcpkg"
TRIPLET="x64-linux"
TYPE=""
MODE=""   # server / client


usage() {
    echo "Usage: $0 <command>"
    echo "Commands:"
    echo "  --server    Configure and build as a server"
    echo "  --client    Configure and build as a client"
    echo "  --no-vcpkg  Configure and build without vcpkg (via lib-devel)"
    echo "  --clean     Remove build directory"
    exit 0
}

cmd_build_no_vcpkg() {
    mkdir -p ${BUILD_DIR}
    echo "-- Looking in local packages"
    ( cmake -S . -B $BUILD_DIR \
        -DVCPKG_TARGET_TRIPLET=$TRIPLET \
        -DSERVER=$([[ "$MODE" == "server" ]] && echo ON || echo OFF) \
        -DCLIENT=$([[ "$MODE" == "client" ]] && echo ON || echo OFF);
      cmake --build $BUILD_DIR;
    )

    echo "-- Build done!"
    exit 0
}

cmd_build_via_vcpkg() {
    mkdir -p ${BUILD_DIR}
    if [ ! -d "$VCPKG_DIR" ]; then
        echo "-- Error: vcpkg not found. Run ./init_boost.sh"
        exit 1
    fi
    ( cmake -S . -B $BUILD_DIR \
        -DCMAKE_TOOLCHAIN_FILE=$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=$TRIPLET \
        -DSERVER=$([[ "$MODE" == "server" ]] && echo ON || echo OFF) \
        -DCLIENT=$([[ "$MODE" == "client" ]] && echo ON || echo OFF);
      cmake --build $BUILD_DIR; 
    )

    echo "-- Build done!"
    exit 0
}

cmd_clean() {
    if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
    echo "-- Removed $BUILD_DIR"
  fi
  echo "-- Clean done!"
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean) cmd_clean ;;
    -h|--help) usage ;;

    --server) MODE="server" ;;
    --client) MODE="client" ;;
    --no-vcpkg) TYPE="$1" ;;
    *) echo "Unknown arg: $1"; exit 1 ;;
  esac
  shift
done

if [[ -z "$MODE" ]]; then
  echo "Usage: $0 [server|client] [Debug|Release|RelWithDebInfo|MinSizeRel]"
  exit 1
fi

case "${TYPE}" in
    --no-vcpkg) cmd_build_no_vcpkg ;;
    *) cmd_build_via_vcpkg ;;
esac