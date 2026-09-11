# majestic-af

Autofocus engine for the Majestic IP camera streamer, loaded as a runtime
plugin.

Majestic provides the ISP focus statistic. This plugin owns the AF algorithm
and sends logical motor requests through `libmotors`.

## How it plugs in

Majestic `dlopen`s `/usr/lib/majestic-af.so` and drives it with two commands
(`autofocus`, `zoom`) over a tiny C ABI (`include/majestic/af_plugin_abi.h`). The
plugin resolves the focus value and a few helpers back from the majestic
executable at load time. Nothing links majestic; the two sides only share one
header.

`motorsd` coordinates motor clients. Its selected driver owns the hardware,
movement timing, and device-specific delivery rules.

## Build

Cross-compile against the same OpenIPC toolchain as majestic:

```sh
cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE=<majestic>/tools/cmake/toolchains/<cc>.cmake
cmake --build build
```

The build requires `json-c` and the `motorsd` prototype. By default, CMake
looks for `motors/motorsd` beside this repository. Set `LIBMOTORS_DIR` to use
another path:

```sh
cmake -Bbuild -DLIBMOTORS_DIR=/path/to/motorsd
```

This produces `majestic-af.so`. Copy it to `/usr/lib/majestic-af.so` on the
camera. It is picked up when `isp.autofocus.enabled` is set and the majestic
binary was built with plugin-symbol export enabled; otherwise majestic falls back
to its built-in engine, so a missing or mismatched plugin degrades rather than
breaks.

## Select the AF algorithm

Set the algorithm in `majestic.yaml`. The plugin reads this value once and
keeps the same algorithm until Majestic reloads the plugin.

Use the blind-seek model for a P035 controller:

```yaml
isp:
  autofocus:
    enabled: true
    algorithm: blind_seek
```

Use AF2 only with its calibrated lens and valid zoom magnification:

```yaml
isp:
  autofocus:
    enabled: true
    algorithm: af2
```

The prototype accepts only `blind_seek` and `af2`. If the value is missing or
invalid, the plugin disables AF and writes an error to the log.

The AF component selects the algorithm. The motor driver reports capabilities
and telemetry, but it does not select AF behavior.

## Documentation

- [Blind-seek autofocus](docs/blind-seek-autofocus.md) describes the P035 path.
- [P035 field notes](docs/hieasy-p035-field-notes.md) record the observed controller behavior.

## Status

The plugin works on HiSilicon, where the focus statistic is available. It uses
the public `libmotors` API from the current `motorsd` prototype. The AF plugin
does not contain motor protocols or device configuration.

## Contributing

`master` is protected — please open a pull request. CI builds the plugin and runs
the AF model and motor adapter tests on every PR. Run them locally with
`cmake -Bbuild && cmake --build build && ctest --test-dir build`. See `CLAUDE.md`
for the architecture, the ABI contract, and the one hard rule (thread teardown
before `dlclose`).
