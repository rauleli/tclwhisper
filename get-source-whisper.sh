#!/bin/sh
set -eu

usage() {
    echo "usage: $0 SOURCE_DIR BUILD_DIR INSTALL_PREFIX [cpu|cuda]" >&2
    exit 2
}

[ "$#" -ge 3 ] && [ "$#" -le 4 ] || usage

source_dir=$1
build_dir=$2
install_prefix=$3
backend=${4:-cpu}

case $backend in
    cpu)  cuda=OFF ;;
    cuda) cuda=ON ;;
    *) usage ;;
esac

tag=v1.9.4
commit=927cfce34f31707e17f2bff35c349632fb9e2c3a

if [ ! -d "$source_dir/.git" ]; then
    git clone --branch "$tag" --depth 1 \
        https://github.com/ggml-org/whisper.cpp.git "$source_dir"
fi

actual_commit=$(git -C "$source_dir" rev-parse HEAD)
if [ "$actual_commit" != "$commit" ]; then
    echo "error: $source_dir is at $actual_commit; expected $commit" >&2
    exit 1
fi

cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$install_prefix" \
    -DWHISPER_BUILD_IS_DEV=OFF \
    -DBUILD_SHARED_LIBS=ON \
    -DWHISPER_BUILD_TESTS=OFF \
    -DWHISPER_BUILD_EXAMPLES=OFF \
    -DWHISPER_BUILD_SERVER=OFF \
    -DGGML_CCACHE=OFF \
    -DGGML_CUDA="$cuda"

cmake --build "$build_dir" --parallel
cmake --install "$build_dir"
