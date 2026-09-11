# CLAUDE.md

Guidance for Claude Code (claude.ai/code) working in this repository.

Orientation only: what this is, how it builds, where things live, and the rules
that are specific to it. It is not a place to restate general coding standards.

## What this is

`majestic-af` is an autofocus plugin for the Majestic IP camera streamer.
Majestic loads it as `/usr/lib/majestic-af.so` and sends two
commands: `autofocus` and `zoom`. The AF search and worker threads live here.

The motor protocols live in separate driver programs under `OpenIPC/motors`.
Majestic provides the vendor ISP **focus value** through a HAL function.

## The ABI (the one contract that must not drift)

`include/majestic/af_plugin_abi.h` is **vendored byte-identical** from majestic.
It is the entire boundary:

- **This plugin defines** `af_plugin_call(cmd, val)` and `af_plugin_exit()`.
  majestic `dlsym`s them and calls them from its `/autofocus` and `/zoom` handlers.
- **majestic defines** the HAL seams this plugin calls — `sdk_get_focus_value`
  (the focus statistic), `sdk_set_zoom_mag` (push magnification back for the OSD /
  `/zoom` GET), `config_get_string/int/boolean`, `log_log`. They are left
  **undefined** in the `.so` and resolve at `dlopen` against the majestic
  executable, which exports them via its `cmake/dynamic-list.txt` when built
  `WITH_PLUGINS_SUPPORT=ON`.

Only C functions with scalar/pointer arguments cross this boundary — no structs
(`AfIO`/`AfParams` stay inside the plugin) — so the ABI is immune to struct-layout
drift between the firmware toolchain and this one. If you change the ABI, change
the copy in majestic in the same breath.

## Layout

- `src/plugin.c` — the thin adapter from the two-token command ABI to the engine.
  Starts the magnification reader in a constructor at load.
- `src/engine.c` — the AF worker, focus metric, cancellation, result state,
  algorithm selection, dead-reckoning, and the `AfIO` adapter.
- `src/af_motor.c` and `src/af_motor.h` — the adapter from AF operations to
  `libmotors`. This layer contains no hardware protocol.
- `src/af_algorithm.c` — parsing for the configured algorithm name.
- `src/af_blind_seek.c` — a blind seek that needs no saved motor position.
- `src/af2.c` — a calibrated search that uses zoom magnification and a
  time-based focus position.
- `include/majestic/` — the vendored self-contained headers (`af_plugin_abi.h`,
  `af2.h`, `af.h`, `log.h`).

## Build

Cross-compile against the same OpenIPC toolchain majestic uses:

```
cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE=<majestic>/tools/cmake/toolchains/<cc>.cmake
cmake --build build
```

Produces `majestic-af.so`. Deploy it to `/usr/lib/majestic-af.so`. majestic loads
it iff `isp.autofocus.enabled` is true **and** the majestic binary was built
`WITH_PLUGINS_SUPPORT=ON` (that flag is what exports the HAL seams). If the seams
are missing, `RTLD_NOW` makes the `dlopen` fail and majestic keeps its built-in
engine — so a mismatched pair degrades, it does not crash.

Set `isp.autofocus.algorithm` to `blind_seek` or `af2` in `majestic.yaml`. The
plugin reads this value once. A missing or invalid value makes AF unavailable.

Use `blind_seek` for the P035. Use `af2` only with valid zoom magnification and the
calibration that its time-based position model needs. Do not select the
algorithm from a motor driver.

## Tests

`tests/af2_model.c` is the AF search regression guard. It drives AF2 and blind
seek against synthetic lens models on a virtual clock. It uses the vendored
`greatest` framework (`tests/greatest.h`). It
builds host-native (CMake adds the test target only when NOT cross-compiling, since
a cross build has no host runner) and runs under `ctest`:

```
cmake -Bbuild && cmake --build build && ctest --test-dir build --output-on-failure
```

`tests/af_motor_test.c` checks that the AF adapter sends the correct logical
operations, roles, axes, and lease requests through `libmotors`.

The same native build produces the `.so`. CI (`.github/workflows/ci.yml`) runs
all host tests on every push and pull request.

## The rule that must not be broken (teardown)

The worker and reader threads are **joinable**, and `af_plugin_exit()` →
`af_engine_stop()` sets cancel, joins **both**, and returns **before** majestic
`dlclose`s this `.so`. A detached thread that outlives the unmap runs freed code
and faults on the next SIGHUP reload. Keep threads joinable; never detach them.

## Motor service

The AF adapter connects to `/run/motorsd.sock`. It requests an AF lease for
focus or zoom and subscribes to service events. A manual client can revoke this
lease. The AF worker polls the event and cancels its current job.

The selected motor driver contains protocol frames and hardware configuration.
Do not add transport or controller logic to this repository.

## Workflow

`master` is protected: **all changes land through pull requests.** Branch off
`master`, push the branch, open a PR. Do not push to `master` directly.

## Relationship to majestic

This repository owns AF policy and reads the focus value through the Majestic
HAL. It sends logical motor requests through `libmotors`.

The headers under `include/majestic/` must match the Majestic copies. Calibration
constants in `af2.c` are measurements for the 85H50AI lens.
