#!/usr/bin/env bash

# Build on Windows with the mingw-w64 toolchain (e.g. w64devkit) instead of MSVC.
# Override the defaults with MAKE_PROGRAM, BUILD_TYPE or NUM_JOBS if needed.

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Release}"
NUM_JOBS="${NUM_JOBS:-$(nproc 2>/dev/null || echo 4)}"

# Neither the compiler nor CMake is reliably on PATH on Windows: the mingw
# toolchains simply unpack wherever you put them, and the winget copy lands
# under a long generated directory name. Look where they land rather than
# ask for the exports up front.
if ! command -v g++ >/dev/null 2>&1; then
    for candidate in \
        "$HOME/AppData/Local/Microsoft/WinGet/Packages"/BrechtSanders.WinLibs.*/mingw64/bin \
        /c/w64devkit/bin \
        /c/msys64/mingw64/bin \
        /c/mingw64/bin
    do
        if [ -x "$candidate/g++.exe" ]; then
            PATH="$candidate:$PATH"
            break
        fi
    done
fi

if ! command -v cmake >/dev/null 2>&1; then
    for candidate in "/c/Program Files/CMake/bin" "/c/Program Files (x86)/CMake/bin"
    do
        if [ -x "$candidate/cmake.exe" ]; then
            PATH="$candidate:$PATH"
            break
        fi
    done
fi

# w64devkit and WinLibs both ship mingw32-make rather than make, so looking
# only for the latter finds nothing on a perfectly good toolchain.
MAKE_PROGRAM="${MAKE_PROGRAM:-$(command -v make || command -v mingw32-make || true)}"

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake was not found. Install it, for instance with:"
    echo "    winget install --id Kitware.CMake"
    exit 1
fi

if [ -z "$MAKE_PROGRAM" ]; then
    echo "Neither make nor mingw32-make was found. Install a mingw-w64"
    echo "toolchain, for instance with:"
    echo "    winget install --id BrechtSanders.WinLibs.POSIX.UCRT"
    echo "or point MAKE_PROGRAM at the make you want used."
    exit 1
fi

# CMake is a native Windows program, so it cannot follow the /c/... paths
# Git Bash hands out.
if command -v cygpath >/dev/null 2>&1; then
    MAKE_PROGRAM="$(cygpath -m "$MAKE_PROGRAM")"
fi

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
