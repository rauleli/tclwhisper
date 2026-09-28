# Changelog

This file records changes made to `tclwhisper` in chronological order.
Dates and times use the local time zone stated in each entry.

## 2026-09-27 19:15:51 MDT (-0600) — Slice 1

### Added

- `whisper::init model`, which creates an independent `whisper_context` with
  the unmodified values returned by `whisper_context_default_params()`.
- `whisper::free handle`, which deletes an owned Tcl command-handle.
- Handle-based context ownership with no singleton or mutable global registry.
- A single command deletion callback that calls `whisper_free()` and releases
  binding-owned state for explicit free, command deletion, and interpreter
  destruction.
- Validation by Tcl command token, object command procedure, deletion callback,
  and client data identity before any handle state is accessed.

### Characterized

- The upstream defaults are `use_gpu=true`, `flash_attn=true`, and
  `gpu_device=0`.
- The existing build is CPU-only (`GGML_CUDA=OFF`, `GGML_CPU=ON`,
  `GGML_OPENMP=ON`, and `GGML_NATIVE=ON`). Upstream reports that no GPU is
  found and successfully continues with the CPU backend.
- On this development machine, measured RSS was 8,860 KiB with no contexts,
  115,268 KiB with one, 220,552 KiB with two, and 28,116 KiB after freeing
  both. Measurements used `/proc/self/status` `VmRSS` in one process.
- Five observed initialization times for `ggml-tiny.bin` were 119.480,
  114.399, 107.771, 110.022, and 109.666 ms.
- Explicit process `exit` does not run the command deletion callback; the
  process exits normally and the operating system reclaims its resources.

### Validated

- A valid init creates one handle, and valid free removes it.
- Missing and invalid model files return Tcl errors without publishing a
  handle or retaining binding-owned state.
- A second free reports an invalid handle without attempting a second cleanup.
- Two simultaneous handles own independent contexts.
- `rename $handle {}` and interpreter deletion invoke the same cleanup used by
  `whisper::free`.
- Missing names, built-in commands, Tcl procedures, empty names, and handles
  from another interpreter are rejected without altering those commands.
- Direct handle invocation reports that handles are not directly invocable.
- Valgrind reported zero errors and zero definitely or indirectly lost bytes
  for successful lifecycle paths and failed initialization paths.
- The final non-instrumented build passes with `-Wall -Wextra -Werror`.

### Scope

- Temporary lifecycle tracing was removed before the final build.
- No whisper.cpp source or build files were modified.
- No transcription, audio, model information, logging, or other post-Slice 1
  API was added.

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
