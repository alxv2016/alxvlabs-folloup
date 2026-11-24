#!/usr/bin/env bash
# Compiles Bitcount Single at one bit a pixel for LVGL: the type scale's ladder at weights 600 and 700, and 30 px at 400
# (STICKY_DESIGN_TOKENS §3). The three TTFs are the upright Regular, SemiBold and Bold instances of Bitcount Single's variable
# font from petrvanblokland/TYPETR-Bitcount (OFL.txt), Version 1.001 — wght 400, 600 and 700, ELXP 0, ELSH 0, slnt 0,
# CRSV 0 — cut with fontTools' instancer. Bitcount draws on a 100-unit cell, a tenth of the em, so a size that is a multiple of
# 10 px puts every cell on whole panel pixels. Run from anywhere: bash components/ui/fonts/bitcount/make_fonts.sh
set -euo pipefail
cd "$(dirname "$0")"

TEXT_SIZES=(20 30 40 50 60 70 80)   # ASCII and the marks below; Bitcount has no arrows
FIGURE_SIZES=(100 120 160)          # figures only: a time or a count
TEXT_RANGE="0x20-0x7E,0xB0,0xB7,0xD7,0x2013,0x2014,0x2019,0x2022,0x2026"
FIGURE_RANGE="0x20,0x30-0x3A"

conv() {  # weight-name file size range
    local name="bitcount_$1_$3"
    npx --offline -y lv_font_conv@1.5.3 --no-compress --no-prefilter --bpp 1 --size "$3" --font "$2" -r "$4" \
        --format lvgl --force-fast-kern-format --lv-font-name "$name" -o "$name.c"
}

for weight in 600 700; do
    file=BitcountSingle-SemiBold.ttf
    [[ $weight == 700 ]] && file=BitcountSingle-Bold.ttf
    for size in "${TEXT_SIZES[@]}"; do conv "$weight" "$file" "$size" "$TEXT_RANGE"; done
    for size in "${FIGURE_SIZES[@]}"; do conv "$weight" "$file" "$size" "$FIGURE_RANGE"; done
done
# The one regular size: body, 30 px at weight 400 (DESIGN → Typography → Scale and roles).
conv 400 BitcountSingle-Regular.ttf 30 "$TEXT_RANGE"
