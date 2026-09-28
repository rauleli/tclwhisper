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
