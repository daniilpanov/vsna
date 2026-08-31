# AGENTS.md

## What this is

VSNA — a C++23 CLI project (WebSocket-based data exchange over VLAN). Early-stage; many features are stubs.

## Build

```bash
git submodule update --init vcpkg   # first time only in a fresh clone
cmake --preset default               # or: cmake -B out; bootstraps vcpkg + installs Boost + configures both targets
cmake --build --preset default      # builds vsna_server AND vsna_client together
make                                 # runs clang-format on all .cpp/.h (excluding .git, out, libs)
```

- Build outputs go to `out/` (`vsna_server`, `vsna_client`), Boost via manifest into `out/vcpkg_installed/`.
- `vcpkg` is a git submodule; there is **no init_modules script**. The vcpkg CMake toolchain (referenced in `CMakePresets.json`) auto-bootstraps vcpkg and auto-installs Boost declared in `vcpkg.json` during configure.
- CMake is the entry point — there is **no build.sh/build.bat either**.
- Requires C++23 (`<print>`, `<format>`, `<source_location>`).
- `BUILD_SERVER_EXE` / `BUILD_CLIENT_EXE` CMake options (both default `ON`, built in one configure). `-DBUILD_SERVER_EXE=OFF` builds client only, and vice versa.
- To clean: `rm -rf out` (or `cmake --build --preset default --target clean`).
- No tests, no CI, no linter beyond `clang-format`.

## Architecture

- **Shared entry point** (`src/main.cpp`): compile-time `#if BUILD_SERVER` / `#if BUILD_CLIENT` selects the role; it is compiled into two executables (`vsna_server`, `vsna_client`).
- Three static libs: `utils` (always), plus `server` and `client` (built only when the matching executable is enabled).
- `src/common/types/pch.h` — precompiled header with Boost.Beast/Asio includes and common `using` declarations.
- `src/utils/helper/helper.h` — inline helpers (trim, split, isValidIPv4, join) + constants `max_length` (1024) and `max_threads` (4).
- `src/utils/logger/logger.h` — C++23 `std::print`-based logging.

## Conventions

- Commands must be written in **lowercase** (enforced in `command_manager.cpp`).
- Console messages use prefix patterns: `[~]` info, `[=]` display, `[!]` error.
- Error handling: throw `std::invalid_argument` / `std::runtime_error` with `[!]`-prefixed messages.
- Header-only vendored libs are committed in `libs/` (CLI11, nlohmann/json) and included as `<libs/CLI11.hpp>`, `<libs/json.hpp>`.
- `.clang-format` is GNU-based, 4-space indent, `ColumnLimit 100`, `SortIncludes: false`.

## Gotchas

- README has stale paths (e.g. `src/utils/helpers/` vs actual `src/utils/helper/`) and wrong default port claim (code defaults to 5555, README says 8080).
- `getpid()` in `server.cpp` is POSIX-only.
- No `#pragma once` in some headers (e.g. server `session.h`, client `session.h`).
