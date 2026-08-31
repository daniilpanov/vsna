#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VCPKG_DIR="$ROOT/vcpkg"
TRIPLET="x64-linux"
SKIP_BOOST="false"

usage() {
    echo "Usage: $0 <command>"
    echo "Commands:"
    echo "  --init       Bootstrap vcpkg and install Boost"
    echo "  --skip-boost Skip vcpkg/Boost setup"
    echo "  --clean      Remove vcpkg directory"
    exit 0
}

cmd_init() {
    if [ "$SKIP_BOOST" = "false" ]; then
        cd "$VCPKG_DIR"
        ./bootstrap-vcpkg.sh

        ./vcpkg install boost-filesystem boost-system boost-asio boost-beast boost-thread --triplet "$TRIPLET"
        ./vcpkg integrate install
        echo "-- Installed Boost"
    fi

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
