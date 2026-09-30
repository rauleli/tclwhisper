# Reproducible benchmarking

`tools/benchmark.tcl` measures model initialization separately from repeated
blocking transcriptions. It has no hidden warmup: every requested run is
reported and included in the statistics.

The script is a baseline helper, not the complete Orin NX campaign harness:
it does not vary `-n_threads` or `-initial_prompt`, contrabalance treatments,
or separate `FIRST_CALL` from steady-state comparisons. The campaigns below
specify evidence to collect; this pass neither changes the script nor runs
measurements.

```sh
TCLLIBPATH=/path/to/tclwhisper/package \
  tclsh tools/benchmark.tcl MODEL PCM LANGUAGE RUNS
```

`LANGUAGE` is `default`, `en`, `es`, `auto`, or another value accepted by the
binding. `RUNS` must be a positive integer. Example paths are supplied by the
caller; neither models nor benchmark audio are stored in this repository.

Capture system information alongside results:

```sh
tools/system-info.sh > system-info.txt
tclsh tools/benchmark.tcl /path/to/model.bin /path/to/audio.f32 es 5 \
  > benchmark.txt
```

Do not add those outputs to the repository.

## Functional provenance and scope

- Slice 4, commit `c533aa3fee4a77c9fc8b88021f5300e199fc7779`, exposed
  per-call `-n_threads`. Omitting it preserves the whisper.cpp upstream
  default. The RTX 4000 Ada matrix in `CHARACTERIZATION.md` is evidence for
  that platform and backend, not an Orin NX optimum.
- Slice 5, commit `f7296e3c057297704640e3d691b2877ac14d69b6`, exposed
  per-call `-initial_prompt`. Omission and `-initial_prompt ""` are
  contractually equivalent. Its existing characterization is separate from
  the main thread-count matrix.

The exact Orin NX values, effective backend behavior, and operational
significance remain to be measured. This protocol does not select a
production model, thread count, or power mode.

## Comparison protocol

For a Linode RTX versus Jetson Orin NX comparison, keep all workload inputs
constant:

- the same tclwhisper commit;
- whisper.cpp v1.9.4 at commit
  `927cfce34f31707e17f2bff35c349632fb9e2c3a`;
- the same model file and checksum;
- the same mono 16 kHz `f32le` PCM file and checksum;
- the same explicit language or the same `auto` setting;
- the same number of runs;
- no unreported warmup;
- model initialization reported separately from transcription;
- CPU or CUDA backend recorded explicitly;
- complete `tools/system-info.sh` output retained with the measurements.

Do not compare `auto` with `en` or `es` as though they were the same workload:
automatic language detection adds work. For an initial latency comparison
oriented toward Iik' and a known language, use the same explicit language on
both machines so backend inference is not mixed with detection cost.

The benchmark records observations; it does not predict which system will be
faster.

## Evidence retention

Keep each result with its exact date, host, CPU and architecture, model name
and checksum, whisper.cpp and tclwhisper commits, backend, language setting,
input identity and exact duration, thread count, run-level timings, and any
relevant power, scheduling, or concurrency configuration. Characterized
results that inform future work should be recorded or referenced in
`CHARACTERIZATION.md`; a new machine or corpus complements earlier evidence
rather than replacing it.

## Campaign A — `n_threads` on Orin NX CUDA

Question: How does per-call `-n_threads` affect `whisper::transcribe` latency
on CUDA in Jetson Orin NX 16 GB under controlled conditions?

Slice 2 observed an effective default of four threads on the development
machine. In the characterized whisper.cpp v1.9.4 source, the default is
`min(4, hardware_concurrency)`. Omitting `-n_threads` still uses that upstream
default. The Jetson Orin NX 16 GB has eight Cortex-A78AE cores in two
clusters of four (NVIDIA Jetson Orin NX Series Modules Data Sheet,
DS-10712-001), but its online CPUs, effective topology and restrictions must
be recorded during the actual run. The initial explicit treatment matrix is:

```text
n_threads = 1, 2, 4, 6, 8
```

Start with approximately ten comparative observations per treatment. This is
a starting point, not a fixed sample-size rule. If observed dispersion leaves
particular comparisons ambiguous, add targeted repetitions after reviewing
the complete initial matrix; do not automatically expand every treatment.
Keep model, checksummed PCM, language, backend, prompt condition and other
inference parameters constant; omit `-initial_prompt` throughout A so prompt
effects remain in B. Use CUDA as the effective backend, not merely as a
build-time option. Run A first under one stable, documented power profile;
`n_threads × nvpmodel` is not an initial factorial design. Any tested value
may prove preferable; more threads do not guarantee lower latency.

### Persistent handle and first observations

The main condition represents the intended resident-context architecture:

```text
whisper::init → one resident handle → transcribe → transcribe → ...
```

Record model initialization separately from inference. `-n_threads` applies
to `whisper::transcribe`, so variation in initialization time is evidence to
retain, not a causal effect to attribute to that option.

`FIRST_CALL` means the first `whisper::transcribe` on a newly initialized
`WhisperHandle`. Record, preserve and report it separately, with its exact
options and text; do not hide it or include it in the steady-state comparative
sample. The condition used for `FIRST_CALL` must be declared before the
campaign and kept identical across comparable runs or campaigns.
`FIRST_SAMPLE(treatment)` instead means the first
observation of a particular explicit `-n_threads` value within the matrix.
Label it and retain it in run order; it is not automatically a “warmup” and
changing thread count does not create a new context. Any exclusion needs an
explicit, evidence-based reason recorded with the data.

The repeated controlled input enables reproducible comparison, but operational
Iik' will process different utterances. Repetition therefore does not exactly
recreate all history of an operational context. Add a separate, targeted
fresh-handle diagnostic to test whether context history, buffers, caches or
other handle-associated state cause a material difference. If so, open a
specific lifecycle characterization; do not preemptively add lifecycle as a
full dimension of A.

### Contrabalance and thermal observation

Fix a reproducible, contrabalance treatment order before execution, preserve
that schedule and associate its position with every observation. Do not run
`1 → 2 → 4 → 6 → 8` in every round. For example, five successive rotations
can start at 1, 2, 4, 6 and 8; two complete cycles give ten positions per
treatment while distributing order effects. Do not manually revise the order
in response to partial results. This helps distribute time, temperature,
DVFS, cache and residual-load effects across treatments.

Do not impose an arbitrary fixed cooldown such as `sleep 1.5` or `sleep 2.0`.
First characterize actual thermal behavior with telemetry granular enough to
correlate temperature, effective clocks and latency per run, without assuming
a universal thermal threshold or imposing unassessed monitoring overhead. If
material thermal drift appears, design a separate characterization or an
evidence-based thermal baseline control.

### Required Orin telemetry and workload identity

Retain the following with the matrix and each run's time/order association:

- Platform: exact Jetson module, RAM, JetPack, L4T, kernel and CUDA versions.
- CPU: online CPUs, topology, cpuset, affinity, governor and frequencies.
- GPU/memory: effective backend, GPU clocks and utilization, EMC clocks or
  relevant state, used/available RAM, swap and relevant memory pressure.
- Power/thermal: `nvpmodel`, whether `jetson_clocks` is active, temperatures,
  power, effective clocks, throttling evidence and fan state where relevant.
- Environment: competing processes, CPU/GPU load and special scheduling
  conditions.

Choose the sampling frequency of `tegrastats` or an equivalent tool so that
telemetry can be correlated with runs without introducing unevaluated monitor
load; do not set an arbitrary universal frequency in advance.

Each observation must retain the tclwhisper and whisper.cpp commits, exact
model and checksum, exact audio and checksum, exact duration and PCM format,
language, effective backend, `n_threads`, exact `initial_prompt` or its
absence, execution order/position, run number, exact recognized text,
inference time and associated telemetry. Record init time separately. A
`FIRST_CALL` row must retain the same provenance even though it is excluded
from the steady-state comparison.

The RTX 4000 Ada matrix is an antecedent for its platform and backend; it
does not establish an optimal `n_threads` value for Orin NX. The Orin results
must determine whether any observed difference is reproducible and material.

## Campaign B — independent `initial_prompt` comparison

Question: On controlled Orin NX workloads, does one fixed nonempty contextual
prompt change recognition, and what inference cost accompanies it? Compare
no prompt against the same exact nonempty prompt with model, PCM, language,
backend and other parameters held constant. Preserve exact output text and
per-call timings. Omission and `-initial_prompt ""` are contractually
equivalent; the empty form may be a functional sanity check, not a third
performance treatment.

Keep B separate from A rather than immediately constructing a
`5 thread counts × 2 prompt conditions × N runs` matrix. Use the
upstream/default thread behavior as the reference. If A identifies another
thread value materially relevant to operation, B may also include it; a
single lowest observed latency does not by itself establish an optimum.
Preserve run order, initialization and `FIRST_CALL` separately where
applicable. Do not infer that prompt benefits generalize from one corpus or
that mission-specific prompt construction belongs in tclwhisper.

## Later independent campaigns

| Campaign | Question / boundary | Minimum dependencies |
|---|---|---|
| A — CUDA threads | Per-call thread effect on Orin NX; initial campaign above | tclwhisper, whisper.cpp, model, audio, Jetson Orin NX |
| B — initial prompt | Recognition effect and inference cost, apart from A | The same fundamental dependencies as A |
| C0 — integration latency harness | Tcl event loop, IPC, pipes/UDS, serialization, buffering, logging and process transitions; real components or stubs as the stated question requires. Not “Iik' E2E” and not a substitute for C | Sufficient orchestration infrastructure and real IPC; components or stubs matched to the question |
| C — real Iik' E2E | From the pilot's end of speech to the first audible sample of Iik's response | The real applicable pipeline: `tclaudio`, `tclffmpeg` if applicable, tclwhisper, Tcl orchestrator, tclllama, tools/tclembedding/telemetry and further LLM iterations when applicable, TTS, audio output |
| D — power modes | Separate later characterization, not an initial `n_threads × nvpmodel` matrix | Orin hardware and controlled, recorded power modes |
| E — CPU-only | Separate later baseline, fallback or architectural comparison; backend is not another factor of A | Orin hardware and an equivalent CPU-only build |

C0 measures integration infrastructure and orchestration, potentially with
stubs, not the real operational latency defined by C. None of the named
components is asserted to exist merely because it appears in this dependency
map. If A shows weak CUDA sensitivity to thread count, E may become more
interesting, but that is not an advance decision about its outcome or order.

## Initial analysis and operational relevance

Preserve every individual observation, including temporal order and linked
telemetry. Initially report median, mean, minimum, maximum and IQR for the
relevant comparable runs. Do not initially require P95, Mann–Whitney,
Kruskal–Wallis or p-values; inferential tests can be selected later if the
question, data structure and sample size justify them.

Distinguish an observable, reproducible difference from an operationally
useful one. Do not impose advance absolute thresholds such as 15 ms, 100 ms
or RTF 0.01. Until real E2E evidence exists, treat a tclwhisper improvement
as operationally important only when its magnitude is material relative to
STT latency and/or Iik's conversational E2E budget.

Actual campaign results belong in `CHARACTERIZATION.md` when they exist. If
the volume of Orin NX evidence later justifies a dedicated document, create
it then and reference it from `CHARACTERIZATION.md`; do not create it as part
of this protocol update.

## Directed duration-scaling characterization

Slice 2 found similar inference times for several short inputs of different
durations, but that evidence does not establish a fixed cost per 30-second
window or a stepped 30/60/90-second relationship. Treat scaling with audio
duration as an experimental question.

Prepare controlled, comparable inputs of approximately:

```text
20 s
35 s
50 s
65 s
```

For every input, record:

- exact audio duration and checksum;
- the same model and model checksum;
- equivalent content and speech distribution as far as practical;
- the same language setting;
- the same CPU or GPU backend;
- the same thread count;
- several repetitions with every run retained;
- model initialization separately from inference;
- inference wall-clock time and RTF.

The purpose is to determine empirically whether cost changes are associated
with internal processing boundaries or whether another factor dominates. Do
not assume the result before measuring it.
