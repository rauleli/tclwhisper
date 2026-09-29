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

- Slice 4 — Per-call thread count (completed)
  - `-n_threads <integer>`; positive upstream `int`
  - no handle state; omission preserves upstream default
  - RTX matrix completed; see `CHARACTERIZATION.md`

- Infrastructure phase — Repository portability / publishing preparation
  - portable build documentation
  - clean-source build/install validation
  - reproducible benchmark tooling
  - public repository preparation

## Evidence for future selection

Slice 4 was selected and completed exclusively as per-call `-n_threads`.
RTX measurements are recorded in `CHARACTERIZATION.md`; Jetson Orin NX
characterization remains future measurement work.

Evidence from intended Iik' workloads — including aviation phraseology,
mixed Spanish/English commands, and the intended operator's own voice — should
be collected and recorded or referenced in `CHARACTERIZATION.md` before a
future slice that depends on recognition behavior is selected. The evidence
relevant to a proposed slice should be reviewed before that slice is approved;
this does not make those corpora an unconditional prerequisite for a purely
introspective slice that does not depend on recognition behavior.

## Possible future slices

`whisper::info` remains an unselected candidate, outside completed Slice 4.

### Slice 5 — To be selected

No capability has been selected for Slice 5. D-008 defers possible `s16le`
support until Iik's actual audio acquisition and normalization pipeline is
characterized. The resulting evidence may justify reconsidering conversion
inside `tclwhisper`, but D-008 does not approve that API. Slice 5 remains
available for another capability supported by operational evidence.

### Slice 6 — Additional inference controls

Possible candidates include:

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
