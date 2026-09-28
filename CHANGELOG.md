# Changelog

This file records changes made to `tclwhisper` in chronological order.
Dates and times use the local time zone stated in each entry.

## 2026-09-27 18:16:23 MDT (-0600) — Slice 0

### Added

- Minimal Tcl extension skeleton using TEA and Autoconf.
- Tcl package `tclwhisper` version `0.1`.
- The `whisper` namespace and `whisper::version` command.
- A `whisper::version` implementation that directly returns the result of
  `whisper_version()` without hardcoding the whisper.cpp version.
- Discovery of whisper.cpp headers and libraries through `pkg-config`.
- An RPATH derived from the `libdir` reported by `pkg-config` to resolve the
  existing local prefix on Linux.
- Minimal build, test, and installation targets.

### Characterized

- whisper.cpp source: `../whisper.cpp-v1.9.4`.
- Existing build: `../whisper-build-v1.9.4`.
- Existing installation prefix: `../whisper-install-v1.9.4`.
- Tag: `v1.9.4`.
- Commit: `927cfce34f31707e17f2bff35c349632fb9e2c3a`.
- `WHISPER_VERSION_BASE`: `1.9.4`.
- `WHISPER_BUILD_IS_DEV`: `OFF`.
- Observed shared libraries: `libwhisper`, `libggml`, `libggml-base`,
  `libggml-cpu`, and `libparakeet`.
- The installation and its `whisper.pc` file already existed; neither
  `cmake --install` nor a whisper.cpp rebuild was performed.

### Validated

- `package require tclwhisper` returns `0.1`.
- `whisper::version` returns exactly `1.9.4`.
- `make test` completes successfully.
- The dynamic dependencies of `tclwhisper.so` resolve successfully, including
  transitive GGML dependencies.

### Scope

- Neither whisper.cpp nor its build was modified.
- No functionality belonging to Slice 1 or later was implemented.
