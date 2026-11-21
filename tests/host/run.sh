#!/usr/bin/env bash
# Builds and runs the host tests — the firmware's pure logic, tested on the computer with its own C++ compiler, no
# board and no ESP-IDF. Each tests/host/*_test.cpp is one program; the run fails if any fails to build or any check
# fails. The binaries go to build/host/, which is never committed. Run from anywhere: bash tests/host/run.sh
set -euo pipefail
cd "$(dirname "$0")/../.."

CXX="${CXX:-c++}"
OUT=build/host
mkdir -p "$OUT"
status=0
shopt -s nullglob
tests=(tests/host/*_test.cpp)
if [ ${#tests[@]} -eq 0 ]; then
    echo "host tests: none in tests/host/"
    exit 1
fi
for test in "${tests[@]}"; do
    name="$(basename "$test" .cpp)"
    # A test that fails to build fails the run, and the rest still build and run.
    if "$CXX" -std=c++20 -Wall -Wextra -Werror -I components/lvgl_display/include "$test" -o "$OUT/$name"; then
        "$OUT/$name" || status=1
    else
        echo "host tests: $name did not build"
        status=1
    fi
done
exit "$status"
