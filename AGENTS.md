# AGENTS.md

## What this is

VSNA — a C++23 CLI project (WebSocket-based data exchange over VLAN). Early-stage; many features are stubs.

## Build

```bash
make configure     # configures all targets
make build         # configures and builds all targets
make format        # runs clang-format on all .cpp/.h
```

- Build outputs go to `out/` (`vsna_server`, `vsna_client`), deps via manifest into `out/vcpkg_installed/`.
- `vcpkg` is a git submodule; there is **no init_modules script**. The vcpkg CMake toolchain (referenced in `CMakePresets.json`) auto-bootstraps vcpkg and auto-installs all deps declared in `vcpkg.json` during configure.
- CMake is the entry point — there is **no build.sh/build.bat either**.
- Requires C++23 (`<print>`, `<format>`, `<source_location>`).
- `BUILD_SERVER_EXE` / `BUILD_CLIENT_EXE` CMake options (both default `ON`, built in one configure). `-DBUILD_SERVER_EXE=OFF` builds client only, and vice versa.
- To clean: `rm -rf out` (or `cmake --build --preset default --target clean`).
- No tests, no CI, no linter beyond `clang-format`.

### Build on Termux (Android)

The `vcpkg` toolchain doesn't work on Termux (its prebuilt `vcpkg` binary is glibc
and can't drive the environment's toolchain). Use native packages and the `native`
preset instead; the vcpkg `default` flow stays intact for other platforms.

```bash
pkg install -y boost boost-headers cli11 nlohmann-json
make configure-native     # cmake --preset native
make build-native         # cmake --build --preset native
```

- Output lands in `out-native/`.
- Boost is header-only here (Asio/Beast/`boost::system`), so `libboost_system` /
  `libboost_exception` are not needed — hence CMake requests only `Boost::headers`
  (component libs are not required in `CMakeLists.txt`).
- `nlohmann-json`, `CLI11`, Boost headers are picked up from `/usr/include` and the
  standard CMake package paths (`find_package`) on the native preset.

## Architecture

- **Shared entry point** (`src/main.cpp`): compile-time `#if BUILD_SERVER` / `#if BUILD_CLIENT` selects the role; it is compiled into two executables (`vsna_server`, `vsna_client`).
- `src/` contains only two folders — `Core/` and `UI/` — plus `main.cpp`.
- Three static libs: `utils` (always), plus `server` and `client` (built only when the matching executable is enabled).
- **Core/** — business logic, no UI dependencies:
  - `Core/common/types/pch.h` — precompiled header with Boost.Beast/Asio includes and common `using` declarations.
  - `Core/utils/` — addr, config, helper, logger.
  - `Core/server/` — `Server` class and `ServerSession` (WebSocket handling).
  - `Core/client/` — `Client` class and `ClientSession` (WebSocket handling).
- **UI/** — presentation layer, calls only Core methods:
  - `UI/server/` — `ServerCLI` (CLI arg parsing, server startup).
  - `UI/client/` — `ClientUI` (REPL loop), `CommandManager` (command dispatch), `menu/` (command definitions).
- `src/utils/helper/helper.h` — inline helpers (trim, split, isValidIPv4, join) + constants `max_length` (1024) and `max_threads` (4).
- `src/utils/logger/logger.h` — C++23 `std::print`-based logging.

## Conventions

- Commands must be written in **lowercase** (enforced in `command_manager.cpp`).
- Console messages use prefix patterns: `[~]` info, `[=]` display, `[!]` error.
- Error handling: throw `std::invalid_argument` / `std::runtime_error` with `[!]`-prefixed messages.
- All deps come from `vcpkg` (`vcpkg.json`: Boost, cli11, nlohmann-json). CLI11 is included as `<CLI/CLI.hpp>` (v2.7 multi-header layout — not `<CLI/CLI11.hpp>`) and nlohmann as `<nlohmann/json.hpp>`. CLI11 has out-of-line symbols, so `find_package(CLI11 CONFIG)` + link `CLI11::CLI11` is required (set in `CMakeLists.txt`).
- `.clang-format` is GNU-based, 4-space indent, `ColumnLimit 100`, `SortIncludes: false`.

## Gotchas

- `getpid()` in `Core/server/server.cpp` is POSIX-only.
- No `#pragma once` in some headers (e.g. Core server `session.h`, Core client `session.h`).
