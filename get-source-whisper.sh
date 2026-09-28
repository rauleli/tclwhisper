git clone --branch v1.9.4 --depth 1 \
    https://github.com/ggml-org/whisper.cpp.git \
    ../whisper.cpp-v1.9.4

cmake \
    -S ../whisper.cpp-v1.9.4 \
    -B ../whisper-build-v1.9.4 \
    -DCMAKE_INSTALL_PREFIX=../whisper-install-v1.9.4 \
    -DWHISPER_BUILD_IS_DEV=OFF \
    -DBUILD_SHARED_LIBS=ON \
    -DWHISPER_BUILD_TESTS=OFF \
    -DWHISPER_BUILD_EXAMPLES=OFF \
    -DWHISPER_BUILD_SERVER=OFF

cmake --build ../whisper-build-v1.9.4
cmake --install ../whisper-build-v1.9.4
