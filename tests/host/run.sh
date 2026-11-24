#!/usr/bin/env bash
# Builds and runs the host tests — the firmware's pure logic, tested on the computer with its own C++ compiler, no
# board and no ESP-IDF. Each tests/host/*_test.cpp is one program; the run fails if any fails to build or any check
# fails. A test named *_lvgl_test.cpp links LVGL's real sources, taken from managed_components (a firmware build fetches
# them) and compiled once, with LVGL's own default configuration, into build/host/lvgl. The binaries go to build/host/,
# which is never committed. tests/host/token_parity.py runs too, with its self-test: DESIGN.md's front matter and the
# token header hold the same tokens (STICKY_DESIGN_TOKENS §7). Run from anywhere: bash tests/host/run.sh [part of a
# test's name]
#
# How it stays fast: every test file and every firmware source on a `// host-sources:` line is compiled once into
# build/host/obj/<flag-set hash>/, its path with slashes made `__`, and rebuilt only when the source, a header it
# includes (the compiler's own -MD list) or the flags change; the compiles, then the tests' links and runs, go as many
# at a time as the computer has cores. Each test's output is kept in build/host/logs/ and printed in name order once all
# have run. Only the screen audit writes files (build/host/audit/, .cache/harness/proxies/), so no two tests share an
# output. Runs under macOS's bash 3.2.
set -euo pipefail
cd "$(dirname "$0")/../.."

CXX="${CXX:-c++}"
CC="${CC:-cc}"
OUT=build/host
LVGL=managed_components/lvgl__lvgl
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 8)"
mkdir -p "$OUT"

# The object a LVGL source compiles to, into $o: its path under LVGL's src, slashes made underscores. It sets a variable
# rather than printing, so the check of LVGL's ~480 sources forks nothing.
lvgl_object() {
    o="${1#"$LVGL"/}"
    o="$OUT/lvgl/${o//\//_}.o"
}

# Compiles every LVGL source whose object is missing or older than it, a core's worth at a time. LV_CONF_SKIP is LVGL's
# own defaults: the scroll and input code under test needs nothing more. A source that fails to build leaves no object,
# and that fails the build.
build_lvgl() {
    if [ ! -d "$LVGL/src" ]; then
        echo "host tests: LVGL is not in $LVGL — run a firmware build once, which fetches it"
        return 1
    fi
    mkdir -p "$OUT/lvgl"
    local f o missing=0
    while IFS= read -r f; do
        lvgl_object "$f"
        if [ ! "$o" -nt "$f" ]; then
            echo "$f $o"
        fi
    done < <(find "$LVGL/src" -name '*.c') |
        xargs -P "$JOBS" -n 2 sh -c "$CC -O0 -w -DLV_CONF_SKIP=1 -I $LVGL -c \"\$0\" -o \"\$1\"" || true
    while IFS= read -r f; do
        lvgl_object "$f"
        [ -f "$o" ] || { echo "host tests: LVGL's $f did not build"; missing=1; }
    done < <(find "$LVGL/src" -name '*.c')
    return "$missing"
}

# The three flag sets, as words (no path in them holds a space): a plain test; an LVGL test and the firmware's .cpp it
# names; a .c font. HOST_<set> is the compiler and its flags.
plain=(-std=c++20 -Wall -Wextra -Werror -I components/lvgl_display/include -I components/widgets/src
       -I components/design_tokens/include -I components/ui/include -I components/router/include
       -I components/devices/include)
# The widgets' headers, and stand-ins for the ESP-IDF headers the theme includes (tests/host/stubs).
lvgl=("${plain[@]}" -Wno-deprecated-declarations -DLV_CONF_SKIP=1 -isystem "$LVGL" -isystem "$LVGL/src"
      -I components/widgets/include -I components/app_hooks/include -I components/devices/include -I tests/host/stubs)
font=(-O0 -w -DLV_CONF_SKIP=1 -DLV_LVGL_H_INCLUDE_SIMPLE=1 -I "$LVGL")
export HOST_plain="$CXX ${plain[*]}" HOST_lvgl="$CXX ${lvgl[*]}" HOST_font="$CC ${font[*]}"

# The folder a flag set's objects go to: a hash of the compiler's version and the flags, so a change to either builds
# afresh into a new folder.
obj_dir() {
    local v="HOST_$1"
    echo "$OUT/obj/$1-$( { "${!v%% *}" --version 2>/dev/null; echo "${!v}"; } | cksum | cut -d' ' -f1)"
}

# One compile, run by xargs: set, source, object. The object is fresh when it and its .d exist and nothing the .d lists
# (the source and every header, LVGL's too: -MD keeps system headers) is missing or newer. A compile writes beside the
# object and moves into place only on success; a failure removes the old object and keeps the compiler's words in .log.
compile_one() {
    set -f
    local v="HOST_$1" src="$2" o="$3"
    if [ -f "$o" ] && [ -f "$o.d" ]; then
        local deps
        deps="$(sed -e '1s/^[^:]*://' -e 's/\\$//' "$o.d")"
        # shellcheck disable=SC2086
        if [ -z "$(find $deps -prune -newer "$o" 2>&1 | head -n 1)" ]; then
            return 0
        fi
    fi
    rm -f "$o.log"
    # shellcheck disable=SC2086
    if ${!v} -MD -MF "$o.d.tmp" -c "$src" -o "$o.tmp" > "$o.log" 2>&1; then
        mv "$o.d.tmp" "$o.d"
        mv "$o.tmp" "$o"
        rm -f "$o.log"
    else
        rm -f "$o" "$o.d" "$o.tmp" "$o.d.tmp"
        return 1
    fi
}

# One test, run by xargs: name, then its objects. Links, runs, and keeps what it printed in logs/<name>.log and its
# result (0 passed, 1 failed) in logs/<name>.status; the seconds it took go to times/<name>, so the next run starts the
# slowest first. A missing object is a source that did not build.
link_and_run() {
    local name="$1" log="$OUT/logs/$1.log" o
    shift
    SECONDS=0
    trap 'echo "$SECONDS" > "$OUT/times/$name"' RETURN
    {
        for o in "$@"; do
            [ -f "$o" ] || { echo "host tests: $name did not build"; echo 1 > "$OUT/logs/$name.status"; return 0; }
        done
        if ! $CXX "$@" -o "$OUT/$name"; then
            echo "host tests: $name did not build"
            echo 1 > "$OUT/logs/$name.status"
            return 0
        fi
        if "$OUT/$name"; then echo 0 > "$OUT/logs/$name.status"; else echo 1 > "$OUT/logs/$name.status"; fi
    } > "$log" 2>&1
}
export -f compile_one link_and_run
export OUT CXX

status=0
shopt -s nullglob
# An argument runs only the tests whose name holds it: bash tests/host/run.sh screen_audit
tests=(tests/host/*${1:-}*_test.cpp)
# The token parity check is a test by the name token_parity: no argument, or one its name holds, runs it and its cases.
parity=0
if [[ "token_parity" == *"${1:-}"* ]]; then
    parity=1
    python3 tests/host/token_parity.py || status=1
    python3 tests/host/token_parity.py --test || status=1
fi
if [ ${#tests[@]} -eq 0 ]; then
    if [ "$parity" -eq 1 ]; then
        exit "$status"
    fi
    echo "host tests: none in tests/host/"
    exit 1
fi

# LVGL first, once, if any test needs it.
lvgl_ok=1
for test in "${tests[@]}"; do
    if [[ "$test" == *_lvgl_test.cpp ]]; then
        build_lvgl || lvgl_ok=0
        break
    fi
done

dir_plain="$(obj_dir plain)" dir_lvgl="$(obj_dir lvgl)" dir_font="$(obj_dir font)"
mkdir -p "$dir_plain" "$dir_lvgl" "$dir_font" "$OUT/logs"
rm -f "$OUT"/logs/*
jobs_file="$OUT/logs/compile.jobs"
: > "$jobs_file"
# Each test's link line (name, then its objects) in name order: the test's own object, the objects of the files on its
# `// host-sources:` line in that line's order (a .cpp against LVGL, a .c as a font), then LVGL's. A test needing LVGL
# when LVGL did not build gets no line and fails.
links=()
lvgl_objects="$(echo "$OUT"/lvgl/*.o)"
for test in "${tests[@]}"; do
    name="${test##*/}" name="${name%.cpp}"
    if [[ "$name" == *_lvgl_test ]]; then
        if [ "$lvgl_ok" -eq 0 ]; then
            links+=("-")
            continue
        fi
        o="$dir_lvgl/${test//\//__}.o"
        echo "lvgl $test $o" >> "$jobs_file"
        line="$name $o"
        for src in $(sed -n 's#^// host-sources:##p' "$test"); do
            if [[ "$src" == *.c ]]; then
                o="$dir_font/${src//\//__}.o"
                echo "font $src $o" >> "$jobs_file"
            else
                o="$dir_lvgl/${src//\//__}.o"
                echo "lvgl $src $o" >> "$jobs_file"
            fi
            line="$line $o"
        done
        links+=("$line $lvgl_objects")
    else
        o="$dir_plain/${test//\//__}.o"
        echo "plain $test $o" >> "$jobs_file"
        links+=("$name $o")
    fi
done

# Every object once, all cores at a time; a source shared by several tests is one job.
sort -u "$jobs_file" | xargs -P "$JOBS" -n 3 "$BASH" -c 'compile_one "$@"' _ || true
while read -r _ src o; do
    if [ -f "$o.log" ]; then
        cat "$o.log"
        echo "host tests: $src did not build"
        status=1
    fi
done < <(sort -u "$jobs_file")

# Every test linked and run, all cores at a time — the slowest of the last run first, so the screen audit is not left
# to start last — then their output in name order.
mkdir -p "$OUT/times"
i=0
for line in "${links[@]}"; do
    [ "$line" = "-" ] || echo "$(cat "$OUT/times/${line%% *}" 2>/dev/null || echo 0) $line"
done | sort -s -rn -k 1,1 | cut -d ' ' -f 2- | xargs -P "$JOBS" -L 1 "$BASH" -c 'link_and_run "$@"' _ || true
for test in "${tests[@]}"; do
    name="${test##*/}" name="${name%.cpp}"
    if [ "${links[$i]}" = "-" ]; then
        echo "host tests: $name did not build"
        status=1
    else
        cat "$OUT/logs/$name.log"
        [ "$(cat "$OUT/logs/$name.status" 2>/dev/null || echo 1)" = 0 ] || status=1
    fi
    i=$((i + 1))
done
exit "$status"
