# Reproducible benchmarking

`tools/benchmark.tcl` measures model initialization separately from repeated
blocking transcriptions. It has no hidden warmup: every requested run is
reported and included in the statistics.

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

## Directed thread-count characterization

Slice 2 observed an effective default of `n_threads=4` on the development
machine. In the characterized whisper.cpp v1.9.4 source, that default is based
on:

```text
min(4, hardware_concurrency)
```

Hardware with more than four CPU cores therefore warrants an explicit
measurement before deciding whether a public `-n_threads` option would provide
operational value. For example, the Jetson Orin NX 16 GB has eight
Cortex-A78AE cores arranged as two clusters of four cores, not a `6 + 2`
topology (NVIDIA Jetson Orin NX Series Modules Data Sheet,
DS-10712-001).

Use a controlled candidate matrix such as:

```text
n_threads:
1
2
4
6
8
```

For each value, keep model, audio, language, backend, and repetitions fixed,
and record initialization separately from inference. Also retain CPU online
state, power mode, scheduling/cpuset restrictions, memory conditions, GPU
backend configuration, and competing process load.

This is a measurement, not an advance recommendation. Four threads may remain
optimal, and more threads do not guarantee lower latency. Memory behavior,
GPU/CPU work division, power mode, scheduling, cpusets, and other processes can
change the result. tclwhisper exposes per-call `-n_threads` on
`whisper::transcribe`; omission preserves the upstream default. Use this
protocol to measure its behavior on each hardware/backend combination.

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
