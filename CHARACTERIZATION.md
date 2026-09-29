# Characterization record

This document preserves observations made while implementing and validating
Slices 2–4 and subsequent focused experiments. It is not an API specification,
and it does not make future
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
focal check and real-audio inference were pending in that historical corpus;
both are now closed by the Slice 4 RTX evidence below. Neither follows from
the extremely short-PCM test.

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

The following evidence was not present in the Slice 2/3 corpus. Later closure
is marked explicitly; unmarked items remain pending:

- a corpus recorded with the intended maintainer/operator's own voice — five
  short recordings are now characterized below; broader coverage remains open;
- aviation phraseology — the five recordings below provide initial coverage;
  broader representative coverage remains open;
- commands that mix Spanish and English;
- English aviation terminology embedded in Spanish phrases;
- real audio captured through Iik's intended acquisition chain;
- a focused empty-PCM acceptance check with `-language spanish` — closed below;
- a complete real-audio inference with `-language spanish` — closed below;
- characterization on an NVIDIA RTX system — recorded below;
- later characterization on Jetson Orin NX;
- comparison of the Whisper models of practical interest to Iik' — prior RTX
  model coverage is recorded below; target-workload comparison remains open.

New records should follow `BENCHMARKING.md` and retain the raw result location
or reference, exact date, host, CPU/architecture, model identity and checksum,
whisper.cpp and tclwhisper commits, backend, language, input identity and
duration, power/scheduling configuration when relevant, and run-level timing.


## Slice 4 — RTX per-call n_threads (2026-09-29 UTC)

This section complements the historical Slice 2/3 records above. It closes
those records' RTX and real-audio `spanish` evidence gaps; it does not replace
CPU observations or claim that the remaining intended-operator corpus exists.

### Host, artifacts and method

Linode host `172-235-199-210`, Fedora kernel `7.2.7-200.fc44.x86_64`, x86_64;
AMD EPYC 9474F guest with **4 online logical CPUs**, allowed CPUs `0-3`;
16,360,788 kB RAM. NVIDIA RTX 4000 Ada Generation, driver 615.71.09,
CUDA toolkit 13.4.92; runtime logs confirm `using CUDA0 backend` for every
matrix and regression process. Tcl 8.6.14 from `/opt/ActiveTcl-8.6`,
tclwhisper package 0.1, GCC 16.2.1. GPU snapshot after the matrix: P0,
49 W / 130 W, no running GPU processes. CPU contention and power/frequency
were not continuously sampled; no affinity or power settings were changed.

Source baseline/HEAD: `fd2e6a2287fb4ee4429175024672a05453802fbe` plus the
uncommitted Slice 4 diff (no release commit). Initial tree had only the
intentional `get-source-whisper.sh` parallelism change, preserved byte for byte.
whisper.cpp v1.9.4 commit `927cfce34f31707e17f2bff35c349632fb9e2c3a`, using
its existing CUDA installation, without rebuilding upstream.

Inputs were local to the experiment; SHA-256:

- `ggml-medium.bin`: `6c14d5adee5f86394037b4e4e8b59f1673b6cee10e3cf0b11bbdbee79c156208`
- `audio1.f32`: `3e98db61ebe069fcfcdf4c70abaeea447afbb03dad96b817cd6bffc5a57d9a21`
- `audio2.f32`: `4c7afcaf42b11b75f5c716808aee1b89253bbc3171bd33c11fb004af19fd691c`
- `audio3.f32`: `ab75920f0d48673cba7e3e8ef63a5b834ade2fbb82a2d6316a657f8d2247dd26`

Matrix: `ggml-medium.bin`, `audio1.f32`, `-language es`, thread values
1, 2, 4, 6, 8 in that order. One fresh process/handle per value, five sequential
calls on that handle, no warmup discarded. Model initialization is separate;
PCM file read precedes timing; transcribe timing includes binding PCM copying.
All other parameters remain identical. Audio1 duration: 3.2914375 seconds.
`init_seconds` brackets only `whisper::init $model`, after reading PCM and
before reading the selected thread count or calling `whisper::transcribe`.
`whisper::init` uses `whisper_context_params`, not `whisper_full_params`;
`-n_threads` is applied only during transcription. The initialization times
are observations of separate processes, not evidence that thread count caused
a change in initialization speed.

Raw evidence and executable probes are retained outside the repository in the
Slice 4 results directory:
`threads-{1,2,4,6,8}.log`, `regression-{1,2,3}.log`, `parser-cuda.log`,
`state.log`, `system-info.txt`, `checksums.txt`, `benchmark.tcl`, `state.tcl`,
`observe.c`, and `slice4.patch`. The benchmark is a local copy of the existing
benchmark script with the selected thread argument; repository tooling and
`BENCHMARKING.md` were not changed. The old protocol's “not currently exposed”
wording describes its pre-Slice-4 context; no measurement-protocol correction
was needed.

### Native API facts and inference

Direct source inspection, before editing, established:

- `include/whisper.h:490`: `whisper_full_params.n_threads` is `int`.
- `src/whisper.cpp:6038`: default is
  `std::min(4, (int32_t) std::thread::hardware_concurrency())`.
- `whisper_full` takes params by value and forwards them to
  `whisper_full_with_state`. Its mel calculation (6931), language detection
  (6948), encoder (7160), decoder (7286/7598), and decoder worker loops
  (7377/7627) use this call's `params.n_threads`. Graph helpers set backend
  thread counts (197–206); this is not a CUDA GPU-thread-count promise.
- No zero/negative auto sentinel or normalization exists along this path.
  Mel creates `std::vector<std::thread>(n_threads - 1)` (3265), with worker
  strides based on `n_threads` (3166/3211).

Inference from those facts: changing the local field suffices for per-call
control; nonpositive counts must not be treated as a safe upstream default
request. No CLI behavior was copied. Omission leaves the native default intact.

### Functional and lifecycle evidence

Strict build `make CFLAGS='-O2 -Wall -Wextra -Werror'` and corresponding
`make test` passed in the existing CUDA build directory. `make test` is only
the repository version/load smoke test. Separately, `tests/options.tcl MODEL`
passed 50 assertions: no options, each option alone, both orders, 1/2/4/8,
INT_MAX on empty PCM, unknown/missing/duplicate options, invalid language,
zero, negative, fractional, noninteger, empty and out-of-range counts.
Invalid counts also precede malformed PCM errors. On this host's Tcl 8.6.14,
`Tcl_GetWideIntFromObj(interp, obj, &value)` with a string object containing
`-18446744073709551615` returned `TCL_OK` and `value=1`. This is an observed
conversion, not a claim about every Tcl 8.6 version. The parser rejects a
minus sign before conversion, then checks a wide integer against `1..INT_MAX`
before casting to `int`. `tests/options.tcl` covers that exact negative value
with both empty and malformed PCM.

An external LD_PRELOAD observer forwarded real calls to libwhisper and logged
`n_threads` plus the live upstream default. Sequences on one real handle were
`1, omitted => 1,4` and `8,2,omitted => 8,2,4`; both option orders and
language-only also passed real inference. No observer was loaded for timings.
Initial sandbox probes could not see CUDA; final functional probes and all
reported timings ran with CUDA access. The observer initially needed explicit
library linkage/search paths; the retained final `state.log` is successful.

Review found only stack-local thread state, no change to `WhisperHandle`, no
new persistent allocation or retained Tcl reference, and all new errors before
PCM allocation. Existing PCM and language cleanup on success/inference failure
is unchanged. No full CUDA Memcheck session was run.

### RTX results

Seconds; RTF is transcription seconds divided by 3.2914375. Every first run
is retained in min/max/mean/median calculations.

| Threads | Init | Runs 1–5 (seconds) | Min | Max | Mean | Median | Median RTF |
|---:|---:|---|---:|---:|---:|---:|---:|
| 1 | 0.470039 | 0.199654, 0.113335, 0.111066, 0.112741, 0.111462 | 0.111066 | 0.199654 | 0.129652 | 0.112741 | 0.034253 |
| 2 | 0.398883 | 0.192168, 0.111732, 0.108590, 0.107797, 0.108046 | 0.107797 | 0.192168 | 0.125667 | 0.108590 | 0.032992 |
| 4 | 0.395404 | 0.192431, 0.109339, 0.107708, 0.108879, 0.107449 | 0.107449 | 0.192431 | 0.125161 | 0.108879 | 0.033079 |
| 6 | 0.392729 | 0.192627, 0.107393, 0.107835, 0.108347, 0.108006 | 0.107393 | 0.192627 | 0.124842 | 0.108006 | 0.032814 |
| 8 | 0.396190 | 0.189555, 0.108508, 0.108026, 0.107323, 0.107682 | 0.107323 | 0.189555 | 0.124219 | 0.108026 | 0.032820 |

Per-run RTF values and exact text are also retained in each raw log. All 25
results were identical (including the leading space):

```text
 El niño toca la guitarra en la mañana.
```

One thread had the highest median. Medians for 2/4/6/8 are close; five ordered
runs on this short input do not establish an optimal count. Every first call
was substantially slower than subsequent calls. There is no evidence here
that more threads monotonically improve latency, especially with only four
guest CPUs exposed. This CUDA matrix does not establish behavior on hardware
with more useful CPU cores than requested threads, nor that 6 or 8 threads
are better than 2 or 4.

### Default-thread regression after the matrix

One fresh-process call per input, `medium`, explicit language, no `-n_threads`.
All three exact texts match the previous RTX medium logs.

| Input / language | Init (s) | Transcribe (s) | RTF | Exact text |
|---|---:|---:|---:|---|
| audio1 / es | 0.397862 | 0.190764 | 0.057958 | ` El niño toca la guitarra en la mañana.` |
| audio2 / es | 0.395275 | 0.227037 | 0.041191 | ` No quiero probar nada más, solo quiero una enorme taza de café.` |
| audio3 / en | 0.395108 | 0.239494 | 0.033583 | ` I don't want to try anything else. I just want a huge cup of coffee.` |

### Prior RTX evidence retained, not rerun

The parent raw-results directory and the local archive
`linode-rtx4000ada-tclwhisper-results.tar.gz`
retain the pre-Slice-4 CUDA matrices for small, medium, large-v3-q5_0,
large-v3-turbo and large-v3 on audio1/es, audio2/es, audio3/en, plus medium and
large-v3-turbo with auto. Model/input checksums are in the parent `checksums.txt`.
Those tests establish existing CUDA coverage; they were not repeated here.

`medium-audio1-spanish.txt` records five real-audio calls, identical text
` El niño toca la guitarra en la mañana.`, median **0.109715 s**. This closes
the historical real-audio full-name gap without rerunning that benchmark.
The new parser tests also close the empty-PCM `spanish` check.

Prior median auto versus explicit seconds for audio1/2/3 were:

- medium: 0.149188/0.182826/0.195324 versus 0.107787/0.142299/0.154676;
- large-v3-turbo: 0.154461/0.171820/0.175843 versus
  0.088674/0.105976/0.110043.

Auto added about 40–41 ms for medium and 66 ms for turbo in those measurements.
This is a host/model/corpus observation, not a portable overhead rule, and
must not be conflated with the historical CPU 90–92% observation.

### Remaining uncertainty and Jetson follow-up

No optimal portable count is established. Reproduce the same 1/2/4/6/8 matrix
on Jetson with these checksummed inputs and explicit es, retaining all runs,
init and text. Record online CPUs, cpuset/affinity, `nvpmodel`, clocks/frequencies,
temperature, power, backend (CUDA or CPU-only), GPU configuration and competing
load. Counterbalance value order and use
more repetitions to distinguish small differences. Longer representative
inputs and the intended Iik' voice/acquisition corpus remain useful later
measurements; none constitutes approval of another API slice.

## Initial-prompt experiment — operator voice (2026-09-29 UTC)

The reproducible harness, exact conditions, input checksums, per-call results
and limitations are preserved in [`experiments/initial_prompt/`](experiments/initial_prompt/README.md).
This experiment used five short aviation recordings of the intended operator,
`ggml-small.bin`, CPU-only whisper.cpp v1.9.4, `-language spanish`,
`n_threads=4`, and the same contextual prompt for every recording. Each
no-prompt control reproduced its previously supplied baseline exactly.

With the prompt, `test5` changed `BFR` to the spoken `VFR` and `Herring` to
the spoken `heading`. Controls `test2`–`test4` had no relevant degradation;
`test4` preserved the spoken NATO-alphabet waypoint rather than converting it
to an identifier. The opening of `test1` changed from `y` to `Iik',`. At the
time of the experiment its spoken reference had not been preserved; on
2026-09-29 the operator confirmed that `Iik'` was spoken. This is a correction
for that recording. Under these specific conditions, a common contextual
prompt produced concrete domain-term improvements without relevant
control-sample degradation. One run per condition with one model cannot
establish that `initial_prompt` generally improves transcription.

### Slice 5 reproduction with the public binding

On 2026-09-29 UTC, the Slice 5 binding was tested on `gepeto` (x86_64,
AMD Phenom II X4 965, four online cores), with the CPU-only whisper.cpp v1.9.4
installation from source commit `927cfce34f31707e17f2bff35c349632fb9e2c3a`.
The model was the same checksummed `ggml-small.bin` as above, and the five
audio checksums matched the experimental record. The pre-Slice-5 tclwhisper
revision was `da04866b022018587c3e567b8a2f7a0c7b3dcffb`.

Each call used the same raw PCM, greedy defaults, `-language spanish` and
`-n_threads 4`; treatment added only this exact prompt:

```text
Iik', squawk, ILS, NOTAM, waypoint, VFR, heading, KREPE, Chihuahua.
```

The model was initialized once. For `test1`–`test4`, the no-prompt call
preceded the prompted call. For `test5`, the prompted call preceded the
no-prompt call, followed by `-initial_prompt ""`. Timing is wall-clock around
the blocking Tcl transcription call, excluding model initialization and file
read. These are single runs, not a formal latency benchmark.

| Audio | No prompt: exact text; seconds | Prompt: exact text; seconds |
|---|---|---|
| `test1.f32` | ` y confirma el Squawk 7700`; 66.222439 | ` Iik', confirma el squawk 7700.`; 69.462792 |
| `test2.f32` | ` Estamos establecidos en el ILS de la pista 2-1.`; 67.347342 | ` Estamos establecidos en el ILS de la pista 2-1.`; 70.327103 |
| `test3.f32` | ` Revisa el Notam para Chihuahua.`; 66.477150 | ` Revisa el NOTAM para Chihuahua.`; 69.072701 |
| `test4.f32` | ` El siguiente Waypoint es Kilo Romeo Echo Papa Echo`; 66.769540 | ` El siguiente waypoint es Kilo Romeo Echo Papa Echo.`; 69.633381 |
| `test5.f32` | ` Confirma BFR y mantén Herring 270.`; 66.745968 | ` Confirma VFR y mantén heading 270.`; 69.605500 |

All five no-prompt texts reproduced their prior baselines. On `test1`, the
operator-confirmed `Iik'` improved the opening. On `test5`, `BFR → VFR` and
`Herring → heading` corrected the two target errors. `test2` was unchanged;
`test3` changed only NOTAM capitalization; `test4` preserved the valid spoken
NATO sequence, with only capitalization and punctuation changes. Those are
not substantive improvements or relevant degradations.

On the same handle, a prompted `test5` call followed by a no-prompt call
returned the baseline again. A third call with `-initial_prompt ""` returned
that same baseline (`66.718927 s`), confirming empty/omitted equivalence in
this probe and no observed call-to-call prompt persistence.

For a separate compatibility probe with `ggml-tiny.bin` and `audio1.f32`, the
four preexisting call forms (no options, `-language spanish`, `-n_threads 4`,
and both options) returned respectively ` The name of the manor.`,
` El niño toca la guitarra en la mañana.`, ` The name of the manor.`, and
` El niño toca la guitarra en la mañana.`. These texts match the historical
default-English and explicit-Spanish outcomes; the corresponding times were
6.526941, 5.873722, 6.473636 and 5.834952 s. This checks compatibility on
one recording, not all prior inputs.

An additional same-handle `test5` pair used `-language auto` with the same
model, PCM, thread count and prompt. Upstream reported `es` with
`p = 0.934197` both without and with the prompt. The texts were respectively
` Confirma BFR y mantén Herring 270.` (132.561897 s) and
` Confirma VFR y mantén heading 270.` (135.153723 s). This one sample shows
no change in detected language, not a general rule for `auto`.

Nonblocking follow-up includes audio longer than 30 s with a domain term in
a later internal window (no suitable supplied `.f32` exists), broader
interaction checks with `-language auto`, and generic versus mission-specific
prompts. Constructing mission-specific prompts belongs to the Iik' Tcl
orchestrator, not this binding. These are open questions, not portable
performance or recognition guarantees.
