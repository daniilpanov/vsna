#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

OUT_DIR=${OUT_DIR:-out}
VCPKG_DIR="$SCRIPT_DIR/vcpkg"
TRIPLET="x64-linux"
USE_VCPKG="true"
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

cmd_build() {
    BUILD_DIR="$OUT_DIR/$MODE"
    mkdir -p $BUILD_DIR
    if [ "$USE_VCPKG" = "true" ]; then
      if [ ! -d "$VCPKG_DIR" ]; then
        echo "-- vcpkg not found, initializing submodule..."
        git submodule update --init vcpkg
      fi
    else
      echo "-- Looking in local packages"
    fi
    
    ( cmake -S . -B $BUILD_DIR \
        -DVCPKG_TARGET_TRIPLET=$TRIPLET \
        -DAPP_NAME="vsna_$MODE" \
        -DSERVER=$([[ "$MODE" == "server" ]] && echo ON || echo OFF) \
        -DCLIENT=$([[ "$MODE" == "client" ]] && echo ON || echo OFF) \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="-g -O0 -fno-omit-frame-pointer";
      cmake --build $BUILD_DIR;
    )

    echo "-- Build done!"
}

cmd_clean() {
    if [ -d "$OUT_DIR" ]; then
    rm -rf "$OUT_DIR"
    echo "-- Removed $OUT_DIR"
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
    --no-vcpkg) USE_VCPKG="false" ;;
    *) echo "Unknown arg: $1"; exit 1 ;;
  esac
  shift
done

if [[ -z "$MODE" ]]; then
  MODE="server"
  cmd_build
  MODE="client"
  cmd_build
  exit 0
else
  cmd_build
  exit 0
fi