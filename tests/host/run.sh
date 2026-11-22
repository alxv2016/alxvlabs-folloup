#!/usr/bin/env bash
# Builds and runs the host tests — the firmware's pure logic, tested on the computer with its own C++ compiler, no
# board and no ESP-IDF. Each tests/host/*_test.cpp is one program; the run fails if any fails to build or any check
# fails. A test named *_lvgl_test.cpp links LVGL's real sources, taken from managed_components (a firmware build fetches
# them) and compiled once, with LVGL's own default configuration, into build/host/lvgl. The binaries go to build/host/,
# which is never committed. Run from anywhere: bash tests/host/run.sh
set -euo pipefail
cd "$(dirname "$0")/../.."

CXX="${CXX:-c++}"
CC="${CC:-cc}"
OUT=build/host
LVGL=managed_components/lvgl__lvgl
mkdir -p "$OUT"

# The object a LVGL source compiles to: its path under LVGL's src, slashes made underscores.
lvgl_object() {
    echo "$OUT/lvgl/$(echo "${1#"$LVGL"/}" | tr / _).o"
}

# Compiles every LVGL source whose object is missing or older than it, eight at a time. LV_CONF_SKIP is LVGL's own
# defaults: the scroll and input code under test needs nothing more. A source that fails to build leaves no object, and
# that fails the build.
build_lvgl() {
    if [ ! -d "$LVGL/src" ]; then
        echo "host tests: LVGL is not in $LVGL — run a firmware build once, which fetches it"
        return 1
    fi
    mkdir -p "$OUT/lvgl"
    local f o missing=0
    while IFS= read -r f; do
        o="$(lvgl_object "$f")"
        if [ ! "$o" -nt "$f" ]; then
            echo "$f $o"
        fi
    done < <(find "$LVGL/src" -name '*.c') | xargs -P 8 -n 2 sh -c "$CC -O0 -w -DLV_CONF_SKIP=1 -I $LVGL -c \"\$0\" -o \"\$1\"" || true
    while IFS= read -r f; do
        [ -f "$(lvgl_object "$f")" ] || { echo "host tests: LVGL's $f did not build"; missing=1; }
    done < <(find "$LVGL/src" -name '*.c')
    return "$missing"
}

status=0
shopt -s nullglob
tests=(tests/host/*_test.cpp)
if [ ${#tests[@]} -eq 0 ]; then
    echo "host tests: none in tests/host/"
    exit 1
fi
lvgl_built=0
for test in "${tests[@]}"; do
    name="$(basename "$test" .cpp)"
    flags=(-std=c++20 -Wall -Wextra -Werror -I components/lvgl_display/include -I components/widgets/src
           -I components/design_tokens/include -I components/ui/include)
    extra=()
    if [[ "$name" == *_lvgl_test ]]; then
        # LVGL is compiled for the test, and so are the firmware sources it exercises, against it: the files on the test's
        # `// host-sources:` line — a .cpp is compiled with the test, a .c (a font) once into build/host/c.
        if [ "$lvgl_built" -eq 0 ]; then
            build_lvgl || { echo "host tests: $name did not build"; status=1; continue; }
            lvgl_built=1
        fi
        # The widgets' headers, and stand-ins for the ESP-IDF headers the theme includes (tests/host/stubs).
        flags+=(-Wno-deprecated-declarations -DLV_CONF_SKIP=1 -isystem "$LVGL" -isystem "$LVGL/src"
                -I components/widgets/include -I tests/host/stubs)
        for src in $(sed -n 's#^// host-sources:##p' "$test"); do
            if [[ "$src" == *.c ]]; then
                mkdir -p "$OUT/c"
                obj="$OUT/c/$(basename "$src").o"
                if [ ! "$obj" -nt "$src" ]; then
                    if ! "$CC" -O0 -w -DLV_CONF_SKIP=1 -DLV_LVGL_H_INCLUDE_SIMPLE=1 -I "$LVGL" -c "$src" -o "$obj"; then
                        echo "host tests: $src did not build"
                        status=1
                        continue 2
                    fi
                fi
                extra+=("$obj")
            else
                extra+=("$src")
            fi
        done
        extra+=("$OUT"/lvgl/*.o)
    fi
    # A test that fails to build fails the run, and the rest still build and run.
    if "$CXX" "${flags[@]}" "$test" ${extra[@]+"${extra[@]}"} -o "$OUT/$name"; then
        "$OUT/$name" || status=1
    else
        echo "host tests: $name did not build"
        status=1
    fi
done
exit "$status"
