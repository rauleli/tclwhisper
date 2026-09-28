# Building tclwhisper on Linux

The characterized dependency is:

```text
whisper.cpp v1.9.4
commit 927cfce34f31707e17f2bff35c349632fb9e2c3a
```

Other versions may work, but this repository's current behavior and benchmark
protocol are based on that exact revision.

## Prerequisites

Install a C/C++ toolchain, CMake, Git, `pkg-config`, Tcl 8.6 runtime and
development files, Autoconf-compatible shell tools, and the dependencies
required by the selected whisper.cpp backend. Package names vary by Linux
distribution.

The distributed `configure` script is included, so users building a release or
clone do not need to regenerate it.

## Build and install whisper.cpp for CPU

Choose arbitrary locations; they do not need to be inside the tclwhisper tree:

```sh
export WHISPER_SOURCE="$HOME/src/whisper.cpp-v1.9.4"
export WHISPER_BUILD="$HOME/build/whisper.cpp-v1.9.4-cpu"
export WHISPER_PREFIX="$HOME/opt/whisper-v1.9.4-cpu"

git clone --branch v1.9.4 --depth 1 \
  https://github.com/ggml-org/whisper.cpp.git "$WHISPER_SOURCE"
test "$(git -C "$WHISPER_SOURCE" rev-parse HEAD)" = \
  927cfce34f31707e17f2bff35c349632fb9e2c3a

cmake -S "$WHISPER_SOURCE" -B "$WHISPER_BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$WHISPER_PREFIX" \
  -DWHISPER_BUILD_IS_DEV=OFF \
  -DBUILD_SHARED_LIBS=ON \
  -DWHISPER_BUILD_TESTS=OFF \
  -DWHISPER_BUILD_EXAMPLES=OFF \
  -DWHISPER_BUILD_SERVER=OFF \
  -DGGML_CCACHE=OFF \
  -DGGML_CUDA=OFF

cmake --build "$WHISPER_BUILD" --parallel
cmake --install "$WHISPER_BUILD"
```

The install step supplies headers, shared libraries, CMake metadata, and
`whisper.pc`. tclwhisper consumes that installed interface through
`pkg-config`.

## NVIDIA CUDA

whisper.cpp v1.9.4 defines the CUDA backend option as:

```text
GGML_CUDA=ON
```

Verify the NVIDIA driver and toolkit first when their tools are available:

```sh
nvidia-smi
nvcc --version
```

Use the CPU procedure above with a distinct build and prefix and change only:

```sh
-DGGML_CUDA=ON
```

During configuration, check for messages including:

```text
CUDA Toolkit found
Using CMAKE_CUDA_ARCHITECTURES=...
```

The resulting install should contain a `libggml-cuda` shared library in
addition to `libwhisper` and the common GGML libraries. `ldd` can verify the
runtime dependency graph. During model initialization, whisper.cpp writes its
backend/device discovery to stderr; use that output and `nvidia-smi` to verify
that a GPU backend is available and active.

`tclwhisper` does not implement a CUDA backend. GPU acceleration depends
entirely on how the linked whisper.cpp/GGML libraries were built.

Do not copy a `CMAKE_CUDA_ARCHITECTURES` value from unrelated hardware. In
v1.9.4, when supported by the toolkit and CMake and `GGML_NATIVE` is enabled,
the CUDA build defaults to native architecture detection. An explicit value is
appropriate only when deliberately cross-compiling or producing a binary for
known target architectures.

## Build tclwhisper

From the tclwhisper source directory:

```sh
export WHISPER_PREFIX="$HOME/opt/whisper-v1.9.4-cpu"
export TCLWHISPER_PREFIX="$HOME/opt/tclwhisper"
export PKG_CONFIG_PATH="$WHISPER_PREFIX/lib/pkgconfig:$WHISPER_PREFIX/lib64/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

mkdir -p build
cd build
../configure --prefix="$TCLWHISPER_PREFIX"
make CFLAGS='-O2 -Wall -Wextra -Werror'
make test
make install
```

If Tcl is installed in a nonstandard location, pass the directory containing
`tclConfig.sh`:

```sh
../configure --prefix="$TCLWHISPER_PREFIX" --with-tcl=/path/to/tcl/lib
```

The build discovers whisper headers and link flags from `whisper.pc`. On Linux,
it embeds the discovered whisper library directory as a runtime search path so
the chosen private prefix remains usable without modifying the system loader
configuration.

To load the installed package, place the parent Tcl library directory on
`auto_path`, for example:

```sh
TCLLIBPATH="$TCLWHISPER_PREFIX/lib" tclsh <<'EOF'
package require tclwhisper
puts [whisper::version]
EOF
```

## Jetson Orin NX / Linux ARM64

Linux ARM64 and Jetson Orin NX are targets for validation; they have not yet
been characterized by this repository.

No tclwhisper source uses x86-only instructions. Build whisper.cpp natively on
the target when practical and let CMake detect CPU/CUDA architecture. Do not
hardcode an RTX architecture from another machine. If cross-compiling, choose
`CMAKE_CUDA_ARCHITECTURES` for the actual target based on the v1.9.4 CMake and
NVIDIA toolchain documentation, rather than treating a Linode build as
portable to Jetson.

## Models

Models are not included. Follow the v1.9.4 upstream model instructions:

https://github.com/ggml-org/whisper.cpp/tree/v1.9.4/models
