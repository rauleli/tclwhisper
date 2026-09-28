# tclwhisper Roadmap

This roadmap records possible future development slices.

It is tentative, not a commitment to implement every item or to implement
them in the listed order. Each slice is selected only after reviewing evidence
from the current implementation, its consumers, and target hardware. The order
may change as that evidence develops.

A capability mentioned here has not necessarily been approved for
`tclwhisper`. Its proper architectural boundary must be established before it
becomes part of the binding.

The binding follows the principle:

> The binding exposes a native capability; Tcl decides how to connect it.

## Completed baseline

- Slice 0 — Bootstrap
  - `package require tclwhisper`
  - `whisper::version`

- Slice 1 — Context lifecycle
  - independent handle-based contexts
  - explicit and implicit cleanup
  - interpreter-local handle management

- Slice 2 — Minimal f32le transcription
  - blocking transcription
  - mono, 16 kHz, IEEE-754 float32 little-endian PCM
  - text result

- Slice 3 — Per-call language selection
  - `-language <language|auto>`
  - no language state stored in the handle

- Repository portability / publishing preparation
  - portable build documentation
  - clean-source build/install validation
  - reproducible benchmark tooling
  - public repository preparation

## Before selecting Slice 4

The next functional slice is intentionally not frozen yet.

Before selecting it, the project should use the portable repository and
benchmark tooling to collect evidence on external hardware where practical,
including:

- an NVIDIA RTX system;
- later, the Jetson Orin NX target.

Measurements should help determine whether controls such as thread count or
other inference parameters are actually useful to the intended deployment.

## Possible future slices

### Slice 4 — Introspection or minimal inference control

Possible directions include:

- `whisper::info`;
- per-call `-n_threads`.

The exact scope is not yet decided.

### Slice 5 — Additional PCM representation

Possible support for `s16le`.

This requires an explicit architectural decision because whisper.cpp consumes
floating-point PCM directly. Supporting `s16le` would add conversion behavior
inside the binding rather than merely expose an upstream capability.

No resampling, channel mixing, file decoding, or audio acquisition is implied.

### Slice 6 — Additional inference controls

Possible candidates include:

- `-n_threads`;
- `-initial_prompt`;
- `-translate`.

Only controls with demonstrated consumers or operational value should become
public API.

### Slice 7 — Structured results

Possible richer results include:

- segments;
- timestamps;
- detected language;
- selected token-level or confidence information where justified.

This slice would require a deliberate Tcl representation design.

The current plain-text result must not be replaced casually.

### Slice 8 — Advanced / realtime capabilities

Possible areas include:

- blank filtering;
- VAD;
- callbacks;
- streaming;
- cancellation.

These may need to be split into separate slices or kept outside
`tclwhisper`, depending on evidence and architecture.

## Likely outside tclwhisper

The following capabilities should not be assumed to belong in this binding:

- microphone or ALSA acquisition;
- RS-232 / RS-485 handling;
- Bluetooth communication;
- WAV, MP3 or general media decoding;
- resampling;
- channel mixing;
- PCM normalization;
- playback;
- system orchestration.

Those belong to other Iik' components or dedicated bindings unless future
evidence justifies otherwise.

## Stopping condition

There is no requirement to reach Slice 8.

Development may stop earlier if the needs of Iik' are fully served by a
smaller API.

A small, stable binding is preferable to exposing upstream options without a
demonstrated consumer.
