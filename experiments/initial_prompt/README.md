# initial_prompt characterization experiment

This is an experimental harness and evidence record, not binding code or part
of the installed library or public API. At the time of this experiment, no
Slice 5 capability had been selected; D-009 subsequently selected per-call
`initial_prompt` based in part on this evidence.

## Question and controlled inputs

Does the same contextual vocabulary prompt help the five supplied Iik' aviation
recordings without degrading phrases already recognized correctly?

Model: `ggml-small.bin`; language: `spanish` (upstream ID 3); `n_threads=4`.
The exact prompt for every prompted call is:

```text
Iik', squawk, ILS, NOTAM, waypoint, VFR, heading, KREPE, Chihuahua.
```

Each file is decoded once from raw mono 16 kHz IEEE-754 f32le into aligned
floats. Both conditions use that same sample vector. For each file, the
harness runs a no-prompt control followed by one prompted call, with a fresh
context for each condition. The no-prompt control uses `initial_prompt=nullptr`.
The prompted condition differs only in the `initial_prompt` pointer.

Both conditions start from `whisper_full_default_params(WHISPER_SAMPLING_GREEDY)`
and retain recognition defaults, including `no_context=true`,
`carry_initial_prompt=false`, `detect_language=false`, `translate=false`, and
`greedy.best_of=5`. The language and thread count are set as specified above.
The four `print_*` flags are false, as in the current binding. Context creation
uses the unchanged upstream defaults, as does `whisper::init`.

Timing brackets only `whisper_full()`. It excludes model initialization, file
reading, f32le decoding and final segment concatenation. Initialization is
recorded separately. Text concatenates upstream segments without trimming or
added separators. There is one run per condition and file, no discarded warmup,
and no performance or repeatability claim.

The supplied historical baselines are references from the maintainer. They
are kept distinct from the no-prompt calls reproduced by this harness.

## Source inspection

Upstream: whisper.cpp v1.9.4, commit
`927cfce34f31707e17f2bff35c349632fb9e2c3a`; source working tree was clean.

- `include/whisper.h:527`: `initial_prompt` is a `const char *` field in
  `whisper_full_params`; the caller assigns a pointer to its prompt text.
- `src/whisper.cpp:6044,6066–6069`: defaults are `no_context=true`,
  `initial_prompt=nullptr`, `carry_initial_prompt=false`, `prompt_tokens=nullptr`
  and `prompt_n_tokens=0`.
- `src/whisper.cpp:7040–7083`: `no_context` clears prior static and dynamic
  token history before preparing the supplied prompt. It therefore does not
  disable a newly supplied `initial_prompt`. Explicit `prompt_tokens`, when
  supplied, take precedence over tokenizing `initial_prompt`.
- With carry disabled, the tokenized prompt is prepended to dynamic history.
  With carry enabled, upstream puts it in static history to prepend to each
  decode window. The dynamic history can evolve within one call even with
  `no_context=true`; that flag clears history at call entry.
- `src/whisper.cpp:7230–7249`: prompt history is bounded by the context budget
  and used only when the current decoding temperature is below 0.5. A prompt
  does not force exact output vocabulary.
- `whisper_full()` receives the parameter struct by value and forwards it to
  `whisper_full_with_state()`. Copying the struct does not copy the string.
  The inspected path reads the string while tokenizing and retains token
  values for decoding, not ownership of the caller's text. This harness keeps
  the prompt in static storage, valid throughout the complete blocking call.

`whisper-cli` was absent from PATH and the local build/install tree; the build
has `WHISPER_BUILD_EXAMPLES=OFF`. Its source supports `--prompt`, but uses the
example audio decoding path and overrides additional inference defaults. A
direct harness avoids converting the supplied headerless PCM or inheriting
CLI-specific defaults. Upstream was not rebuilt.

## Provenance

- Date: 2026-09-29 UTC (local session began 2026-09-28 MDT, UTC−06:00).
- Host: `gepeto`, x86_64, AMD Phenom II X4 965; four online logical CPUs.
- Kernel: `7.1.7-100.fc43.x86_64`; harness compiler: GCC 15.3.1.
- Backend: existing CPU installation; `GGML_CPU=ON`, `GGML_CUDA=OFF`,
  `GGML_OPENMP=ON`, `GGML_NATIVE=ON`. Runtime logs report no GPU found.
- tclwhisper baseline: `88c0c155b364066016b29e49cb59ab568f7fe9c7`.
- Model: `ggml-small.bin`.
- Audio directory: `<audio-directory>`.
- Library: `<whisper-install>/lib64/libwhisper.so.1.9.4`.
- Initial execution directory: `<temporary-output-directory>`.

SHA-256 identities inspected before execution:

| Input | Bytes | SHA-256 |
|---|---:|---|
| `ggml-small.bin` | 487601967 | `1be3a9b2063867b937e64e2ec7483364a79917e157fa98c5d94b5c1fffea987b` |
| `test1.f32` | 305948 | `49c15f095b0147f05e139c578cd4b601db532f3d0d1466430b999e598e687cb0` |
| `test2.f32` | 341056 | `3861bf7708d9a34e4f4a5931c269cb62b0bc2cfe765d993b7ec8c3de0fce972e` |
| `test3.f32` | 284212 | `c1c60955c2bf5d9231d45bac8172d52d1afdb90fb25affb1efab04c6701906c1` |
| `test4.f32` | 394556 | `22c81be282138c71567778129c030cb8a30331fd0cd34998f11648ad73279591` |
| `test5.f32` | 356100 | `cbf6a3f2cc1bf5dd01ba55a0317bbbd984bd6932814f91657834b2a5d1bffd8b` |

Library SHA-256:
`8aa168e1ffd7ca70c327eabe70d89989ba058c2b2329eb72ad2d77978dcd804c`.

## Reproduction

The harness is standalone and not integrated into the binding build/tests.
It writes `results.jsonl` and `upstream.log` in its output directory, so use a
fresh directory for each run. A portable equivalent of the build is:

```sh
WHISPER_INSTALL=/path/to/whisper-install-v1.9.4

g++ -std=c++17 -O2 -Wall -Wextra -Werror \
  -I"$WHISPER_INSTALL/include" \
  initial_prompt.cpp \
  -L"$WHISPER_INSTALL/lib64" \
  -Wl,-rpath-link,"$WHISPER_INSTALL/lib64" \
  -Wl,--disable-new-dtags,-rpath,"$WHISPER_INSTALL/lib64" \
  -lwhisper -o initial_prompt

./initial_prompt \
  /path/to/ggml-small.bin \
  /path/to/audios \
  /fresh/output/directory
```

## Observed results

All ten calls completed with return code zero and one segment per call.
All five no-prompt controls reproduced the maintainer's known baselines exactly.
Every displayed result preserves its upstream-provided leading space. The
model for every row is `ggml-small.bin`.

| Audio | Known baseline, also reproduced here | With initial_prompt | Prompt inference (s) |
|---|---|---|---:|
| `test1.f32` | ` y confirma el Squawk 7700` | ` Iik', confirma el squawk 7700.` | 69.849632 |
| `test2.f32` | ` Estamos establecidos en el ILS de la pista 2-1.` | ` Estamos establecidos en el ILS de la pista 2-1.` | 70.200984 |
| `test3.f32` | ` Revisa el Notam para Chihuahua.` | ` Revisa el NOTAM para Chihuahua.` | 68.979087 |
| `test4.f32` | ` El siguiente Waypoint es Kilo Romeo Echo Papa Echo` | ` El siguiente waypoint es Kilo Romeo Echo Papa Echo.` | 69.844570 |
| `test5.f32` | ` Confirma BFR y mantén Herring 270.` | ` Confirma VFR y mantén heading 270.` | 69.594618 |

The following durations are derived from the unchanged byte counts at
16 kHz / four bytes per sample: test1 4.7804375 s, test2 5.329 s,
test3 4.4408125 s, test4 6.1649375 s, and test5 5.5640625 s.
Full unrounded timings, initialization times, configuration and exact text
are preserved in [results.jsonl](results.jsonl). The local [upstream.log](upstream.log)
retains per-condition initialization/backend messages and is ignored by the
repository's existing `*.log` rule. The execution returned zero.
The experimental [source](initial_prompt.cpp) compiled with
`-O2 -Wall -Wextra -Werror` and is not part of the binding build.

## Textual evaluation and limits

- **test1:** `Squawk 7700` survives, with lowercase `squawk` and a final
  period. The opening `y` becomes `Iik',`. The exact spoken opening has not
  been supplied, so this change cannot yet be classified as a correction or
  prompt-induced error. The maintainer was asked for that reference; it is
  not inferred from the model output.
- **test2:** The complete transcription is unchanged, including ILS and
  `pista 2-1`. No degradation is observed in this control.
- **test3:** Only `Notam` becomes `NOTAM`; Chihuahua and the rest of the
  phrase remain correct. Capitalization alone is not a significant improvement.
- **test4:** The operator pronounced the waypoint using the NATO phonetic
  alphabet. The valid spoken sequence `Kilo Romeo Echo Papa Echo`
  is preserved. Only waypoint capitalization and final punctuation change.
  It is not converted to KREPE, and no such normalization is required of STT.
- **test5:** Both target errors are corrected: `BFR → VFR` and
  `Herring → heading`. `Confirma`, `y mantén`, and `270` are preserved.
  Relative to the supplied spoken reference, this is a substantive domain
  vocabulary improvement.

This provides concrete evidence of operational utility on test5, with no
meaningful degradation observed in controls test2–4. The opening of test1
remains an explicit uncertainty, so a claim of improvement without any
degradation across the entire corpus would be premature. It could be a
correction or contextual bias; the available reference does not distinguish
them. No upstream runtime error was observed.

This is one controlled pair per recording, one model, and one shared prompt
on this CPU. It does not establish repeatability, general accuracy, behavior
on unrelated vocabulary, or portable timing. No automatic score was used as
the sole quality criterion. These findings are characterization evidence,
not an implementation of Slice 5.
