#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VCPKG_DIR="$ROOT/vcpkg"
VCPKG_REPO="https://github.com/microsoft/vcpkg.git"
TRIPLET="x64-linux"
SKIP_BOOST="false"

usage() {
    echo "Usage: $0 <command>"
    echo "Commands:"
    echo "  --init   Clone vcpkg and init boost"
    echo "  --clean  Remove vcpkg directory"
    exit 0
}

cmd_init() {
    if [ "$SKIP_BOOST" = "false" ]; then
        if [ ! -d "$VCPKG_DIR" ]; then
            git clone "$VCPKG_REPO" "$VCPKG_DIR"
        fi

        cd "$VCPKG_DIR"
        ./bootstrap-vcpkg.sh

        ./vcpkg install boost-filesystem boost-system boost-asio boost-beast boost-thread --triplet "$TRIPLET"
        ./vcpkg integrate install
        echo "-- Installed Boost" 
    fi
    
    wget -P "$ROOT/libs" https://github.com/CLIUtils/CLI11/releases/download/v2.7.2/CLI11.hpp
    echo "-- Installed CLI11"

    wget -P "$ROOT/libs" https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp
    echo "-- Installed nlohmann/json"

    wget -P "$ROOT/libs" https://raw.githubusercontent.com/jonoton/cpp-tui/v1.12.0/cpptui.hpp
    echo "-- Installed cpptui"

    echo "-- Done!"
    exit 0
}

cmd_clean() {
  if [ -d "$VCPKG_DIR" ]; then
    rm -rf "$VCPKG_DIR"
    echo "-- Removed $VCPKG_DIR"
  fi
  echo "-- Clean done!"
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --init) cmd_init ;;
    --clean) cmd_clean ;;
    -h|--help) usage ;;

    --skip-boost) SKIP_BOOST="true" ;;
    *) echo "Unknown arg: $1"; exit 1 ;;
  esac
  shift
done

cmd_init