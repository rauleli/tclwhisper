# Characterization record

This document preserves observations made while implementing and validating
Slices 2 and 3. It is not an API specification, and it does not make future
design decisions. Approved decisions belong in `DECISIONS.md`; possible future
directions belong in `ROADMAP.md`; procedures for collecting new evidence
belong in `BENCHMARKING.md`.

The results below are measurements under particular conditions. They do not
turn the behavior of one machine, model, backend, or corpus into a portable
guarantee. A measurement should remain associated with its date, host, model,
whisper.cpp version, tclwhisper revision, backend, language configuration, and
audio whenever those facts were preserved. Missing historical details are
identified instead of being reconstructed from the current machine.

Future characterizations, including NVIDIA RTX and Jetson Orin NX results,
should add comparable records rather than overwrite these observations.

## Evidence and provenance

The primary evidence for these records is:

- the archived Codex session that performed the original Slice 2 and Slice 3
  probes and retained their final reports and command output;
- commits `15f4be16aa8eaa64adba3e0775d9ae600fab4ad3` (Slice 2) and
  `e6b7276f358ca76a70764905c6becc01efbfe17c` (Slice 3), including their commit
  messages and diffs;
- the contemporaneous entries in `CHANGELOG.md` and the approved contracts in
  `DECISIONS.md`;
- the preserved upstream source identity and excerpts inspected during those
  sessions.

The measurements in this document are historical evidence recovered from
those sources. They were not reproduced during this documentation pass.
Temporary probe scripts and raw Valgrind logs were removed at the end of the
original slices, as their reports recorded.

### Shared environment

| Field | Preserved value |
|---|---|
| Local measurement date | 2026-09-27 MDT; the Slice 3 session concluded shortly after midnight on 2026-09-28 |
| Development host | Not preserved by an identifiable hostname in the retained evidence |
| Architecture / CPU model | Not preserved; the observed `hardware_concurrency` was 4, but that does not establish a CPU model or architecture |
| Model | local `ggml-tiny.bin`, 77,691,713 bytes; the Slice 3 probe confirmed it was multilingual |
| Model checksum | Not preserved |
| whisper.cpp | v1.9.4, commit `927cfce34f31707e17f2bff35c349632fb9e2c3a` |
| Backend | CPU; the characterized build reported no GPU and had `GGML_CUDA=OFF` |
| tclwhisper revisions | Slice 2 based on `5b1201a82a61fb061fe906be3da624435855fa0f`; Slice 2 result `15f4be1`; Slice 3 result `e6b7276` |
| PCM inputs | mono, 16 kHz, IEEE-754 float32 little-endian |

The model's origin beyond its local name and size, its checksum, the host name,
the CPU model, and the precise operating-system and scheduler state were not
preserved with these measurements. Current-machine values must not be used to
fill those historical gaps.

## Slice 2 — minimal f32le transcription

### Binding behavior established

Slice 2 established a blocking `whisper::transcribe` operation and the
following PCM contract:

- mono;
- 16 kHz;
- IEEE-754 float32 little-endian;
- samples nominally normalized to `[-1,+1]`.

Empty PCM returned `""` before buffer allocation and without invoking
`whisper_full()`. This also prevented a zero-length call from accidentally
reusing upstream mel state from a prior inference.

For a successful inference, the binding appended upstream segment text in
segment order exactly as returned. It added no separator and performed no
trim. The observed non-empty results began with an upstream-provided space,
which the binding preserved.

The parameters began with
`whisper_full_default_params(WHISPER_SAMPLING_GREEDY)`. The effective values
recorded in the probe were:

```text
strategy=greedy
greedy.best_of=5
hardware_concurrency=4
n_threads=4
language=en
detect_language=false
translate=false
no_context=true
print_progress=true
print_realtime=false
print_timestamps=true
print_special=false
```

The binding changed only the presentation flags `print_progress` and
`print_timestamps` from true to false. Recognition parameters remained at
their upstream defaults. The upstream v1.9.4 source used
`min(4, hardware_concurrency)` for the default `n_threads`; the effective value
on this machine was 4.

### Corpus and measured results

The three local files were already prepared as 16 kHz mono f32le PCM. The
spoken references describe the input, not an expected byte-for-byte output.

| Audio | Duration | Spoken reference | Slice 2 result with default English | Time | RTF |
|---|---:|---|---|---:|---:|
| `audio1.f32` (210,652 bytes) | 3.2914375 s | `el niño toca la guitarra en la mañana.` | ` The name of the manor.` | 6.498495 s | 1.9744 |
| `audio2.f32` (352,760 bytes) | 5.511875 s | `No quiero probar nada más. Solo quiero una enorme tasa de café.` | ` I don't want to try anything else, but I just want a huge coffee cup.` | 6.165360 s | 1.1186 |
| `audio3.f32` (456,412 bytes) | 7.1314375 s | `I dont want to try anything else. I just want a huge cup of coffee.` | ` I don't want to try anything else. I just want a huge cup of coffee.` | 6.105836 s | 0.8562 |

`tasa` in the `audio2` spoken reference is intentional. The default-English
run did not recognize either `tasa` or `taza`; it produced the English
interpretation shown above. `audio3` was recognized correctly for the tested
English phrase, including the output contraction `don't`.

The upstream JFK sample was converted outside the binding with ffmpeg 7.1.5
to 704,000 bytes of f32le PCM, exactly 11 seconds. All five runs returned:

```text
 And so my fellow Americans ask not what your country can do for you, ask what you can do for your country.
```

| Run | Time | RTF |
|---:|---:|---:|
| 1 | 6.271742 s | 0.570158 |
| 2 | 6.271971 s | 0.570179 |
| 3 | 6.240986 s | 0.567362 |
| 4 | 6.219292 s | 0.565390 |
| 5 | 6.256518 s | 0.568774 |

For runs 2–5, the recorded range was 6.219292–6.271971 s, average
6.247192 s, and median 6.248752 s. All five texts were identical.

### Short, silent, and longer input

- About 50 ms of silence returned `""`; upstream warned that its effective
  mel length was 40 ms and below 100 ms. This is an observed upstream path,
  not a public tclwhisper policy for short audio.
- 500 ms of silence returned ` [BLANK_AUDIO]` in 5.652939 s.
- 10 seconds of silence returned ` [BLANK_AUDIO]` in 5.661736 s.
- JFK repeated to 33 seconds was accepted and returned in 14.251368 s
  (RTF 0.43186).

Several short inputs of materially different durations had similar inference
times: the 3.291 s, 5.512 s, 7.131 s, and 11 s speech inputs above took roughly
6.1–6.5 s, while the 0.5 s and 10 s silence cases both took about 5.66 s. This
is only the observed timing pattern. It does not demonstrate a fixed cost per
30-second window, and it does not establish a 30/60/90-second step function.
The duration-scaling question remains an experiment in `BENCHMARKING.md`.

### Memory and lifecycle evidence

For the 33-second repeated-JFK probe, the retained process measurements were:

| Point | VmRSS | VmHWM |
|---|---:|---:|
| Loaded context, before transcription | 117,252 KiB | 117,252 KiB |
| Immediately after transcription | 180,580 KiB | 182,360 KiB |
| After `whisper::free` | 25,384 KiB | 182,360 KiB |

The call-local PCM buffer was 2,112,000 bytes. `VmHWM` is the process peak and
therefore did not decrease after `free`. RSS retention or allocator behavior
is not, by itself, evidence of a leak.

The final short-PCM Valgrind lifecycle matrix covered `free`, `rename {}`, and
child-interpreter deletion. It reported zero definitely lost bytes, zero
indirectly lost bytes, and no invalid reads, writes, or frees. It did report
1,841,152 possibly lost bytes in 107 blocks and 565,655 still-reachable bytes;
the retained stack review attributed those records to ActiveTcl and found no
tclwhisper frames. A separate forced error after PCM-buffer allocation also
reported zero definite or indirect loss and no binding frames. Full inference
of a 2.5-second sample under Memcheck did not complete after more than eight
minutes and was aborted, so the completed lifecycle matrix exercised a 50 ms
buffer and upstream's short-input path. The evidence supports no definite or
indirect leak attributable to the binding and no invalid operation in the
completed probes; it is not a claim that RSS must return to its initial value.

## Slice 3 — per-call language selection

Slice 3 tested real inference with each of:

```text
-language es
-language en
-language auto
```

Omitting the option retained the Slice 2 default English behavior. Explicit
language and `auto` applied only to the current call. Automatic selection used
upstream's `params.language = "auto"` path with `detect_language=false`, so
upstream detected a language and then continued transcription.

### Real-inference matrix

Every non-empty result below began with one upstream-provided space and had no
trailing space.

| Audio | Mode | Time | RTF | Exact result |
|---|---|---:|---:|---|
| `audio1.f32` | default | 6.583079 s | 2.0001 | ` The name of the manor.` |
| `audio1.f32` | `en` | 6.468846 s | 1.9654 | ` The name of the manor.` |
| `audio1.f32` | `es` | 5.865969 s | 1.7822 | ` El niño toca la guitarra en la mañana.` |
| `audio1.f32` | `auto` | 11.285227 s | 3.4287 | ` El niño toca la guitarra en la mañana.` |
| `audio2.f32` | default | 6.117281 s | 1.1098 | ` I don't want to try anything else, but I just want a huge coffee cup.` |
| `audio2.f32` | `en` | 6.113712 s | 1.1092 | ` I don't want to try anything else, but I just want a huge coffee cup.` |
| `audio2.f32` | `es` | 6.010199 s | 1.0904 | ` No quiero probar nada más, sólo quiero una enorme tasa de café.` |
| `audio2.f32` | `auto` | 11.509718 s | 2.0882 | ` No quiero probar nada más, sólo quiero una enorme tasa de café.` |
| `audio3.f32` | default | 6.106261 s | 0.8562 | ` I don't want to try anything else. I just want a huge cup of coffee.` |
| `audio3.f32` | `en` | 6.117725 s | 0.8579 | ` I don't want to try anything else. I just want a huge cup of coffee.` |
| `audio3.f32` | `es` | 8.367095 s | 1.1733 | ` Yo no quiero hacer anything else, pero yo quiero una cosa de una cosa de una cosa de una cosa.` |
| `audio3.f32` | `auto` | 11.641432 s | 1.6324 | ` I don't want to try anything else. I just want a huge cup of coffee.` |

The temporary characterization instrumentation recorded these automatic
detections:

| Audio | Detected language | Probability |
|---|---|---:|
| `audio1.f32` | `es` (ID 3) | 0.812187 |
| `audio2.f32` | `es` (ID 3) | 0.918668 |
| `audio3.f32` | `en` (ID 0) | 0.843416 |

Against the correct explicit language, `auto` added 5.419258 s (92.38%) for
audio1, 5.499519 s (91.50%) for audio2, and 5.523707 s (90.29%) for audio3.
Thus, on this machine and corpus, automatic detection added approximately
90–92% to the measured latency. This is not portable to other models,
languages, backends, or hardware. It does show that known explicit language is
an important benchmark variable.

Forcing the wrong language also mattered: applying `es` to the tested English
audio produced the mixed, repetitive output preserved in the table. That is
an observed severe degradation, not a general claim about every wrong-language
combination.

### Extremely short input with language selection

For 20 samples (about 1.25 ms at 16 kHz):

| Mode | Result | Time |
|---|---|---:|
| `en` | `""` | 0.005295 s |
| `es` | `""` | 0.004314 s |
| `auto` | `whisper_full` failure, code `-3` | 0.003364 s |

For about 50 ms:

| Mode | Result | Time |
|---|---|---:|
| `en` | `""` | 0.004177 s |
| `es` | `""` | 0.003527 s |
| `auto` | `""`; detected `en` before the short-input return | 5.423767 s |

These are characterization results, not a promised public response for every
short input. They show that the upstream autodetection path can precede its
short-audio guard.

### UTF-8 and repeated runs

Valid UTF-8 was observed for characters that occurred naturally in the
Spanish outputs:

```text
ñ → c3 b1
á → c3 a1
é → c3 a9
ó → c3 b3
```

Neither `í` nor `ú` appeared naturally, so this corpus does not provide
coverage for them or for unobserved punctuation and scripts.

Three repeated `-language es` runs on `audio1.f32` took 5.858516, 5.827615,
and 5.859659 s: a range of 5.827615–5.859659 s and a 0.032044 s spread. Their
transcription text was identical. Three repeated `auto` runs took 11.273412,
11.243617, and 11.248473 s: a range of 11.243617–11.273412 s and a 0.029795 s
spread. Their transcription text was identical, and each recorded the same
`es` probability of 0.812187. Only that output repeatability under the tested
conditions was demonstrated; the inference times varied.

### Full language name `spanish`

Upstream accepted `"spanish"` and resolved it to the same ID 3 / `es` language
as the short code. A focused binding test also accepted `-language spanish`,
but the retained script shows that its PCM contained 20 samples (80 bytes),
not zero bytes. Therefore the requested historical claim of a specifically
empty-PCM parser test cannot be verified. The retained evidence also does not
show a complete real-audio inference using the full name. Both the empty-PCM
focal check and real-audio inference remain pending; neither should be inferred
from the extremely short-PCM test.

## What these observations do not establish

- They do not define an API beyond the approved decisions in `DECISIONS.md`.
- They do not predict latency, accuracy, memory use, or optimal thread count
  on another host.
- They do not establish that short audio or silence will always yield the same
  tokens with another model or upstream version.
- They do not establish a fixed 30-second processing cost or a stepped
  30/60/90-second latency curve.
- They do not show that `auto` always has a 90–92% cost.
- They do not provide broad UTF-8, language, accent, or phraseology coverage.

## Evidence still needed

The following evidence was not present in the Slice 2/3 corpus and must not be
described as already available:

- a corpus recorded with the intended maintainer/operator's own voice;
- aviation phraseology;
- commands that mix Spanish and English;
- English aviation terminology embedded in Spanish phrases;
- real audio captured through Iik's intended acquisition chain;
- a focused empty-PCM acceptance check with `-language spanish`;
- a complete real-audio inference with `-language spanish`;
- characterization on an NVIDIA RTX system;
- later characterization on Jetson Orin NX;
- comparison of the Whisper models of practical interest to Iik'.

New records should follow `BENCHMARKING.md` and retain the raw result location
or reference, exact date, host, CPU/architecture, model identity and checksum,
whisper.cpp and tclwhisper commits, backend, language, input identity and
duration, power/scheduling configuration when relevant, and run-level timing.
