# Changelog

This file records changes made to `tclwhisper` in chronological order.
Dates and times use the local time zone stated in each entry.

## 2026-09-27 21:35:02 MDT (-0600) — Slice 2

### Added

- Added the blocking `whisper::transcribe handle pcm` operation.
- Established the initial input contract as mono, 16 kHz, little-endian
  float32 PCM with nominally normalized samples.
- Added an aligned, call-local sample buffer with explicit little-endian
  decoding instead of casting Tcl bytearray storage to `float *`.

### Behavior

- Empty PCM returns an empty Tcl string without calling `whisper_full()`.
- Segment text is concatenated exactly in upstream order, without trimming or
  binding-added separators.
- Recognition starts from upstream greedy defaults. Only presentation-only
  progress and timestamp printing are disabled; recognition parameters remain
  unchanged.

### Characterized

- Slice 2 integration probes cover Tcl bytearray conversion, short and long
  PCM inputs, repeated calls, language-default behavior, lifecycle paths,
  timing, memory, and error handling.

## 2026-09-27 19:48:22 MDT (-0600) — Slice 1 closure

### Fixed

- Replaced public handle names derived from state addresses with identities
  generated from interpreter-associated state.
- Prevented ABA aliasing: a handle identity is never reused during the life of
  its interpreter, even after the corresponding command is destroyed.
- Added an explicit collision check so a candidate name already owned by any
  Tcl command is consumed and skipped rather than replaced.
- Hardened `whisper::init` to retrieve its interpreter state from the owning
  assoc data instead of retaining a duplicate pointer as command client data.

### Characterized

- The address-based implementation reproduced stale-handle aliasing in all 12
  pre-change iterations: the allocator reused the state address immediately.
- A handle renamed to a non-empty name remains valid through its unchanged Tcl
  command token. The original name becomes invalid, while `whisper::free` on
  the new name invokes the existing single cleanup path.
- Handle identity generators are independent per interpreter and their
  associated state is deleted with the interpreter.
- Model paths remain unmodified by the binding. Callers are responsible for
  any desired Tcl filesystem expansion or normalization; relative paths and
  symbolic links remain usable according to upstream and filesystem behavior.

### Validated

- Eight repeated stale-handle cycles produced distinct old and new names; each
  old name remained invalid without affecting its live replacement.
- A foreign Tcl procedure occupying the next candidate name was preserved and
  the new handle safely skipped to the following identity.
- Explicit free, empty-name rename, non-empty rename followed by free, and
  interpreter deletion each invoked handle cleanup exactly once.
- Valgrind reported zero definitely or indirectly lost bytes and zero errors
  across the corrected lifecycle and failure paths.
- The final non-instrumented build passes with `-Wall -Wextra -Werror`.
- Missing interpreter assoc data produces a clean Tcl error instead of using
  stale command client data.

### Scope

- The original Slice 1 history remains intact; this entry records its separate
  identity-hardening closure.
- No new public API or Slice 2 functionality was added.

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
