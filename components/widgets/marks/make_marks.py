#!/usr/bin/env python3
"""Regenerates marks.c: the 1-bit marks of the widgets component, from 128 x 128 alpha artwork — a checkbox card's check
and the status block's check, clock and link-off from the agent deck's (STICKY_DESIGN_TOKENS §9 says what the deck is), and
the ten glyphs of the buttons, the checkbox and the radio from BYTE-90's icon PNGs, and the six of the Wi-Fi network row and the
dropdown, cut from the same PNGs to a box of their own (DESIGN → Components → Icons).

    python3 make_marks.py <the agent deck's components/icons/source folder> <BYTE-90's assets/icons folder>

Each is box-averaged to its size, thresholded at half, trimmed to its ink and packed as LVGL's LV_COLOR_FORMAT_A1 (one bit a
pixel, the most significant bit first), so a mark draws solid on the glass whatever its tint. A glyph's size is the art box
`spacing.icon`, 40 px, so its ink is 18 to 33 px and no stroke is under `spacing.stroke.hairline`, 2 px. A mark with a `crop`
is cut from that box of the 128 x 128 art — x0, y0, x1, y1 — and scaled to its width and height without trimming, so the
three signal glyphs, which share one box, register over one another (the unlit bars drawn under the lit ones). Python's
standard library only.
"""
import struct
import sys
import zlib
from pathlib import Path

GLYPH = 40  # spacing.icon
# (C name, art file, size of its box, what it is for, where the art is: "deck" or "byte90"[, crop box, (width, height)])
SIGNAL_CROP = (3, 21, 125, 111)  # the three signal glyphs' shared ink box in the 128 canvas
EYE_CROP = (2, 12, 126, 120)     # the two eyes' shared ink box, so the pair registers and the face swaps with no shift
MARKS = [("check", "check", 38, "the checkbox card's check, about 25 px of ink", "deck"),
         ("status_check", "check", 85, "the status block's check (spacing.status-mark 85 px of art)", "deck"),
         ("status_clock", "clock", 85, "the status block's clock", "deck"),
         ("status_link_off", "link_off", 85, "the status block's link-off", "deck"),
         ("glyph_arrow_back", "arrow_back", GLYPH, "Back: an arrow pointing left", "byte90"),
         ("glyph_close", "close", GLYPH, "Close: a cross", "byte90"),
         ("glyph_play", "play", GLYPH, "Play", "byte90"),
         ("glyph_pause", "pause", GLYPH, "Pause", "byte90"),
         ("glyph_stop", "stop", GLYPH, "Stop", "byte90"),
         ("glyph_speaker_on", "speaker-on", GLYPH, "sound on: the speaker with waves", "byte90"),
         ("glyph_speaker_off", "speaker-off", GLYPH, "sound off: the speaker with a slash", "byte90"),
         ("glyph_check", "check", GLYPH, "Save, and a checkbox's check", "byte90"),
         ("glyph_mic", "mic-on", GLYPH, "Record: the microphone", "byte90"),
         ("glyph_refresh", "refresh", GLYPH, "Retry: the refresh arrow", "byte90"),
         ("glyph_signal_weak", "wifi-1bar", None, "a network's signal, weak: the dot", "byte90", SIGNAL_CROP, (40, 30)),
         ("glyph_signal_fair", "wifi-2bar", None, "a network's signal, fair: the dot and one arc", "byte90", SIGNAL_CROP, (40, 30)),
         ("glyph_signal_strong", "wifi-3bar", None, "a network's signal, strong: the dot and two arcs; also the unlit bars", "byte90",
          SIGNAL_CROP, (40, 30)),
         ("glyph_lock", "lock", None, "a secured network's padlock", "byte90", (19, 4, 109, 119), (19, 24)),
         ("glyph_fact_check", "check", None, "the connected network's check on its facts line", "byte90", (21, 33, 107, 95), (28, 20)),
         ("glyph_chevron", "select-chevron", None, "a dropdown's up-and-down chevron", "byte90", (40, 17, 88, 112), (14, 28)),
         ("glyph_eye_on", "visible-on", None, "a password field's show: the open eye, the action while the text is hidden", "byte90",
          EYE_CROP, (30, 26)),
         ("glyph_eye_off", "visible-off", None, "a password field's hide: the slashed eye, the action while the text shows", "byte90",
          EYE_CROP, (30, 26))]


def read_alpha(path):
    data = Path(path).read_bytes()
    i, idat = 8, bytearray()
    while i < len(data):
        n = struct.unpack(">I", data[i:i + 4])[0]
        tag, body = data[i + 4:i + 8], data[i + 8:i + 8 + n]
        i += 12 + n
        if tag == b"IHDR":
            w, h, depth, kind = struct.unpack(">IIBB", body[:10])
            assert depth == 8 and kind == 6, "8-bit RGBA only"
        elif tag == b"IDAT":
            idat += body
    raw, stride, bpp = zlib.decompress(bytes(idat)), w * 4, 4
    rows, prev, at = [], bytearray(stride), 0
    for _ in range(h):
        f, line = raw[at], bytearray(raw[at + 1:at + 1 + stride])
        at += 1 + stride
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0
            b = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if f == 1:
                line[x] = (line[x] + a) & 255
            elif f == 2:
                line[x] = (line[x] + b) & 255
            elif f == 3:
                line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(bytes(line[3::4]))
        prev = line
    return w, h, rows


def one_bit(path, size):
    w, h, rows = read_alpha(path)
    out = []
    for j in range(size):
        y0, y1 = j * h / size, (j + 1) * h / size
        row = []
        for i in range(size):
            x0, x1 = i * w / size, (i + 1) * w / size
            s = n = 0
            for yy in range(int(y0), max(int(y0) + 1, int(y1 + 0.999))):
                for xx in range(int(x0), max(int(x0) + 1, int(x1 + 0.999))):
                    if yy < h and xx < w:
                        s += rows[yy][xx]
                        n += 1
            row.append(1 if n and s / n / 255 >= 0.5 else 0)
        out.append(row)
    ys = [j for j, r in enumerate(out) if any(r)]
    xs = [i for i in range(size) if any(r[i] for r in out)]
    return [r[xs[0]:xs[-1] + 1] for r in out[ys[0]:ys[-1] + 1]]


def one_bit_cut(path, crop, width, height):
    """The `crop` box of the art scaled to width x height and thresholded at half, with no trim."""
    w, h, rows = read_alpha(path)
    x0c, y0c, x1c, y1c = crop
    cw, ch = x1c - x0c, y1c - y0c
    out = []
    for j in range(height):
        y0, y1 = y0c + j * ch / height, y0c + (j + 1) * ch / height
        row = []
        for i in range(width):
            x0, x1 = x0c + i * cw / width, x0c + (i + 1) * cw / width
            s = n = 0
            for yy in range(int(y0), max(int(y0) + 1, int(y1 + 0.999))):
                for xx in range(int(x0), max(int(x0) + 1, int(x1 + 0.999))):
                    if yy < h and xx < w:
                        s += rows[yy][xx]
                        n += 1
            row.append(1 if n and s / n / 255 >= 0.5 else 0)
        out.append(row)
    return out


def main():
    sources = {"deck": Path(sys.argv[1]), "byte90": Path(sys.argv[2])}
    text = ["// Generated by make_marks.py from the agent deck's and BYTE-90's 128 x 128 art — see that script. Do not edit by hand.",
            "// LV_COLOR_FORMAT_A1: one bit a pixel, most significant first, a mark's ink trimmed to its box.", "",
            '#include "lvgl.h"', ""]
    for name, art, size, what, where, *cut in MARKS:
        if cut:
            g = one_bit_cut(sources[where] / (art + ".png"), cut[0], *cut[1])
        else:
            g = one_bit(sources[where] / (art + ".png"), size)
        h, w = len(g), len(g[0])
        stride = (w + 7) // 8
        data = bytearray()
        for r in g:
            for b in range(stride):
                byte = 0
                for k in range(8):
                    x = b * 8 + k
                    byte |= (r[x] if x < w else 0) << (7 - k)
                data.append(byte)
        text.append(f"// {name}: {what}; {w} x {h}, {len(data)} bytes.")
        text.append(f"static const uint8_t {name}_map[] = {{")
        for i in range(0, len(data), 16):
            text.append("    " + ", ".join(f"0x{v:02X}" for v in data[i:i + 16]) + ",")
        text.append("};")
        text.append(f"extern const lv_image_dsc_t widgets_mark_{name};")
        text.append(f"const lv_image_dsc_t widgets_mark_{name} = {{")
        text.append(f"    .header = {{.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A1, .flags = 0, .w = {w}, .h = {h},")
        text.append(f"               .stride = {stride}, .reserved_2 = 0}},")
        text.append(f"    .data_size = sizeof({name}_map),")
        text.append(f"    .data = {name}_map,")
        text.append("    .reserved = NULL,")
        text.append("    .reserved_2 = NULL,")
        text.append("};")
        text.append("")
        print(f"{name}: {w} x {h}")
    (Path(__file__).parent / "marks.c").write_text("\n".join(text))


if __name__ == "__main__":
    main()
