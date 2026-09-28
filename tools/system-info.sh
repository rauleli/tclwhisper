#!/bin/sh

section() {
    printf '\n[%s]\n' "$1"
}

run_if_available() {
    command_name=$1
    shift
    if command -v "$command_name" >/dev/null 2>&1; then
        "$command_name" "$@"
    else
        echo "$command_name: not available"
    fi
}

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_root=$(CDPATH= cd -- "$script_dir/.." && pwd)

section date
date

section kernel
uname -a

section architecture
uname -m

section cpu
if command -v lscpu >/dev/null 2>&1; then
    lscpu | sed -n '/^Architecture:/p;/^CPU(s):/p;/^Model name:/p'
elif [ -r /proc/cpuinfo ]; then
    sed -n 's/^model name[[:space:]]*: /model: /p' /proc/cpuinfo | head -n 1
fi
run_if_available getconf _NPROCESSORS_ONLN

section memory
if [ -r /proc/meminfo ]; then
    sed -n '/^MemTotal:/p;/^MemAvailable:/p' /proc/meminfo
else
    run_if_available free -h
fi

section nvidia-smi
run_if_available nvidia-smi

section nvcc
run_if_available nvcc --version

section cmake
run_if_available cmake --version

section compiler
if command -v cc >/dev/null 2>&1; then
    cc --version | sed -n '1p'
else
    echo "cc: not available"
fi

section tcl
if command -v tclsh >/dev/null 2>&1; then
    echo 'puts "executable=[info nameofexecutable]"; puts "patchlevel=[info patchlevel]"' | tclsh
else
    echo "tclsh: not available"
fi

section tclwhisper
if command -v git >/dev/null 2>&1 && git -C "$source_root" rev-parse --git-dir >/dev/null 2>&1; then
    git -C "$source_root" rev-parse HEAD
else
    echo "git commit: not available"
fi

section whisper.cpp-characterized
echo "version=v1.9.4"
echo "commit=927cfce34f31707e17f2bff35c349632fb9e2c3a"
