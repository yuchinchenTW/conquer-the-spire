#!/usr/bin/env bash

# Build on Windows with the mingw-w64 toolchain (e.g. w64devkit) instead of MSVC.
# Override the defaults with MAKE_PROGRAM, BUILD_TYPE or NUM_JOBS if needed.

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAKE_PROGRAM="${MAKE_PROGRAM:-$(command -v make)}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
NUM_JOBS="${NUM_JOBS:-$(nproc 2>/dev/null || echo 4)}"

cd "$ROOT_DIR"

# A cache left behind by another generator (e.g. NMake Makefiles) blocks the
# configure step, so drop it and start over.
if [ -f build/CMakeCache.txt ] && ! grep -q "CMAKE_GENERATOR:INTERNAL=Unix Makefiles" build/CMakeCache.txt; then
    rm -rf build/CMakeCache.txt build/CMakeFiles
fi

# Unix Makefiles rather than MinGW Makefiles: the latter refuses to run while
# sh.exe is on the PATH, which it always is under Git Bash.
# CMAKE_POLICY_VERSION_MINIMUM: Libraries/doctest asks for CMake 3.0, which
# CMake 4.x rejects without it.
cmake -S . -B build -G "Unix Makefiles" \
    -DCMAKE_MAKE_PROGRAM="${MAKE_PROGRAM}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if ! cmake --build build -j"${NUM_JOBS}"; then
    # The one failure worth explaining: Windows will not let the library be
    # written over while a python holding it is still running.
    if [ -f build/bin/libconquer-the-spire.dll ]; then
        echo
        echo "If that was a permission denied on libconquer-the-spire.dll,"              "something still has it open."
        echo "Stop the training (and any watch window), then run this again."
        echo "To build and test without touching the library:"              "make -C build UnitTests"
    fi

    exit 1
fi

build/bin/UnitTests
