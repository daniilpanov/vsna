# AGENTS.md

## What this is

VSNA — a C++23 CLI project (WebSocket-based data exchange over VLAN). Early-stage; many features are stubs.

## Build

```bash
./init_modules.sh          # first time only: clones vcpkg, installs Boost, downloads header libs to libs/
./build.sh --server        # or --client; no flag = both
make                       # runs clang-format on all .cpp/.h (excluding .git, out, libs)
```

- Build outputs go to `out/server/` or `out/client/`.
- Requires C++23 (`<print>`, `<format>`, `<source_location>`).
- `SERVER` and `CLIENT` CMake options are mutually exclusive; the build script handles this.
- No tests, no CI, no linter beyond `clang-format`.

## Architecture

- **Single entry point** (`src/main.cpp`): compile-time `#if BUILD_SERVER` / `#if BUILD_CLIENT` selects the role.
- Three static libs: `utils` (always), `server` or `client` (mutually exclusive).
- `src/common/types/pch.h` — precompiled header with Boost.Beast/Asio includes and common `using` declarations.
- `src/utils/helper/helper.h` — inline helpers (trim, split, isValidIPv4, join) + constants `max_length` (1024) and `max_threads` (4).
- `src/utils/logger/logger.h` — C++23 `std::print`-based logging.

## Conventions

- Commands must be written in **lowercase** (enforced in `command_manager.cpp`).
- Console messages use prefix patterns: `[~]` info, `[=]` display, `[!]` error.
- Error handling: throw `std::invalid_argument` / `std::runtime_error` with `[!]`-prefixed messages.
- Header-only vendored libs included as `<libs/CLI11.hpp>`, `<libs/json.hpp>`, `<libs/cpptui.hpp>`.
- `.clang-format` is GNU-based, 4-space indent, `ColumnLimit 100`, `SortIncludes: false`.

## Gotchas

- README has stale paths (e.g. `src/utils/helpers/` vs actual `src/utils/helper/`) and wrong default port claim (code defaults to 5555, README says 8080).
- `src/client/ui/tui.{h,cpp}` references a missing `invoker.h` — dead code, not in the CMake build.
- `getpid()` in `server.cpp` is POSIX-only.
- No `#pragma once` in some headers (e.g. server `session.h`, client `session.h`).
