# Contributing to tclwhisper

Contributions are welcome. Keep changes small, independently understandable,
and accompanied by evidence appropriate to their risk.

## Development setup

1. Build and install the characterized whisper.cpp dependency as described in
   [BUILDING.md](BUILDING.md).
2. Configure tclwhisper with `PKG_CONFIG_PATH` pointing at that installation.
3. Build and run the baseline tests:

```sh
./configure
make CFLAGS='-O2 -Wall -Wextra -Werror'
make test
```

Use `tests/integration.tcl` with caller-supplied model and PCM paths for an
end-to-end transcription check.

## Expectations

- Preserve Tcl 8.6 compatibility.
- Preserve observable API and lifecycle behavior unless a change is explicit.
- Keep changes narrowly scoped and reviewable.
- Add or update tests when behavior changes.
- Update README/build/API documentation when the public interface changes.
- Do not commit model weights, personal audio, generated build products, logs,
  credentials, or machine-specific paths.

Bug reports should include reproduction steps, Tcl version, operating system,
compiler, exact whisper.cpp revision, backend, and relevant error output.

Contributions are accepted under the repository's MIT License.
