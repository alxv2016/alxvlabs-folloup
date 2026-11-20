"""Reads and writes the PNGs the simulator and the bench take, with only Python's own zlib — no Pillow to install.

    from pngio import read_png, write_png
    w, h, rgb = read_png("shot.png")        # rgb: bytes, 3 a pixel, rows top to bottom
    write_png("diff.png", w, h, rgb)

Reads 8-bit RGB and RGBA (alpha dropped). From claude-agent-deck-3, unchanged but for this line; tests/bench/
screenshot.py writes with it.
"""

from __future__ import annotations

import struct
import zlib


def read_png(path) -> tuple[int, int, bytes]:
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    i, idat, w, h, kind = 8, bytearray(), 0, 0, 0
    while i < len(data):
        (n,), tag = struct.unpack(">I", data[i:i + 4]), data[i + 4:i + 8]
        body = data[i + 8:i + 8 + n]
        i += 12 + n
        if tag == b"IHDR":
            w, h, depth, kind, _, _, interlace = struct.unpack(">IIBBBBB", body)
            if depth != 8 or kind not in (2, 6) or interlace:
                raise ValueError(f"{path}: only 8-bit RGB or RGBA, not interlaced")
        elif tag == b"IDAT":
            idat += body
    bpp = 3 if kind == 2 else 4
    raw, stride = zlib.decompress(bytes(idat)), w * bpp
    out, prev, at = bytearray(), bytearray(stride), 0
    for _ in range(h):
        f, line = raw[at], bytearray(raw[at + 1:at + 1 + stride])
        at += 1 + stride
        if f == 1:
            for x in range(bpp, stride):
                line[x] = (line[x] + line[x - bpp]) & 255
        elif f == 2:
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 255
        elif f == 3:
            for x in range(stride):
                line[x] = (line[x] + ((line[x - bpp] if x >= bpp else 0) + prev[x]) // 2) & 255
        elif f == 4:
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                b, c = prev[x], prev[x - bpp] if x >= bpp else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        out += line if bpp == 3 else bytes(v for k, v in enumerate(line) if k % 4 != 3)
        prev = line
    return w, h, bytes(out)


def write_png(path, w: int, h: int, rgb: bytes) -> None:
    def chunk(tag: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + tag + body + struct.pack(">I", zlib.crc32(tag + body))

    rows = b"".join(b"\0" + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(rows, 6)) + chunk(b"IEND", b""))


def diff_picture(w: int, h: int, old: bytes, new: bytes, tolerance: int = 0) -> tuple[bytes, int]:
    """The new picture dimmed to a third, every pixel that moved more than `tolerance` in any channel in red; and how
    many moved."""
    out, moved = bytearray(len(new)), 0
    for p in range(0, len(new), 3):
        if max(abs(new[p] - old[p]), abs(new[p + 1] - old[p + 1]), abs(new[p + 2] - old[p + 2])) > tolerance:
            out[p], moved = 255, moved + 1
        else:
            out[p], out[p + 1], out[p + 2] = new[p] // 3, new[p + 1] // 3, new[p + 2] // 3
    return bytes(out), moved
