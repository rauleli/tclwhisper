# tclwhisper

`tclwhisper` is a Tcl 8.6 extension for speech-to-text transcription through
[`whisper.cpp`](https://github.com/ggml-org/whisper.cpp).

**Status: early development / pre-1.0.** The public API may evolve before a
stable release.

## Current API

```tcl
whisper::version
whisper::init model
whisper::free handle
whisper::transcribe handle pcm ?-language language|auto?
```

Each successful `whisper::init` creates an independent context and returns a
Tcl command handle that owns it. `whisper::transcribe` is blocking and returns
the concatenated text supplied by upstream.

Language selection applies only to one transcription:

```tcl
whisper::transcribe $handle $pcm                 ;# upstream default
whisper::transcribe $handle $pcm -language en
whisper::transcribe $handle $pcm -language es
whisper::transcribe $handle $pcm -language auto
```

Without `-language`, the characterized whisper.cpp v1.9.4 default is English.
`auto` performs upstream language detection before transcription and has a
significant additional cost on the development machine; that measured cost is
not a portable performance guarantee.

## PCM contract

The current input contract is:

- mono;
- 16000 Hz;
- IEEE-754 float32 little-endian (`f32le`);
- samples nominally normalized to `[-1.0, +1.0]`.

The producer must prepare the audio. The binding currently provides no
resampling, channel mixing, file decoding, `s16le`, streaming, or VAD.

## Quick start

Build and install the dependency and extension as described in
[BUILDING.md](BUILDING.md). Models are not included; consult the upstream
[whisper.cpp model instructions](https://github.com/ggml-org/whisper.cpp/tree/v1.9.4/models).

Prepare raw PCM outside the binding, for example with an existing audio tool,
then read it as binary Tcl data:

```tcl
package require tclwhisper

set model [lindex $argv 0]
set pcmFile [lindex $argv 1]

set channel [open $pcmFile rb]
set pcm [read $channel]
close $channel

set handle [whisper::init $model]
try {
    set text [whisper::transcribe $handle $pcm -language es]
    puts $text
} finally {
    whisper::free $handle
}
```

Run it with paths supplied by the caller:

```sh
tclsh transcribe.tcl /path/to/model.bin /path/to/audio.f32
```

## Build, tests, and benchmarks

- [BUILDING.md](BUILDING.md) documents reproducible CPU and NVIDIA CUDA builds.
- [BENCHMARKING.md](BENCHMARKING.md) defines the comparison protocol.
- `tools/benchmark.tcl` measures model loading and repeated transcription.
- `tools/system-info.sh` records relevant host and toolchain information.
- `tests/integration.tcl` is an optional model/audio integration smoke test.

## Known limitations

- The API is pre-1.0.
- Input is fixed to mono, 16 kHz, `f32le` PCM.
- Transcription is synchronous and blocking.
- WAV and MP3 decoding are not included.
- Selecting a known language avoids the extra work performed by `auto`.
- Model weights and audio samples are not distributed with this repository.

## Contributing and security

See [CONTRIBUTING.md](CONTRIBUTING.md) for development guidance and
[SECURITY.md](SECURITY.md) for reporting guidance.

## License

`tclwhisper` is available under the [MIT License](LICENSE). `whisper.cpp`,
GGML, and model weights are separate upstream works and retain their own
licenses and terms.

## Acknowledgments

- `whisper.cpp` and its contributors;
- GGML and its contributors;
- the Tcl/Tk community;
- OpenAI Whisper as the upstream model architecture and model family.

No affiliation with or endorsement by these projects is implied.

---

## ☕ Support my work

If this project has been helpful to you or saved you some development time, consider buying me a coffee! Your support helps me keep exploring new optimizations and sharing quality code.

[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-ffdd00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://www.buymeacoffee.com/rauleli)
