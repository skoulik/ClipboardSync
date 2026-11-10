"""Draw Clipboard Sync's icons: the tray icon's clipboard and sync arrows, as vector art.

Writes, relative to the repository root:

    icon/ClipboardSync.ico          Windows: the bare clipboard, 16-256 px
    icon/ClipboardSync.icns         macOS: the clipboard on a navy tile, Apple's 824-in-1024 grid
    TrayIcon[@2x].png               the tray states at 32 and 64 px: idle, transmitting
    TrayIconTransmitting[@2x].png   (green and blue boosted, as the original tray art was)
    TrayIconDisabled[@2x].png       and disabled (darkened greyscale)

The outputs are committed, so a build needs neither this script nor Pillow:

    python icon/create_icon.py
"""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent

NAVY = (0, 39, 116, 255)
YELLOW_TOP = (255, 214, 40, 255)
YELLOW_BOTTOM = (254, 194, 36, 255)
ORANGE = (252, 148, 19, 255)
ORANGE_DARK = (226, 112, 30, 255)
TEAL = (2, 126, 95, 255)
TILE_TOP = (52, 86, 168, 255)
TILE_BOTTOM = (24, 52, 120, 255)

SS = 4
ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)
ICNS_SIZES = (16, 32, 64, 128, 256, 512, 1024)


def _vgradient(size, box, top, bottom):
    x0, y0, x1, y1 = box
    grad = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(grad)
    for y in range(int(y0), int(y1) + 1):
        t = (y - y0) / max(1, y1 - y0)
        d.line([(x0, y), (x1, y)], fill=tuple(round(a + (b - a) * t) for a, b in zip(top, bottom)))
    return grad


def _arrow(d, c, r, start, end, width, color):
    """A clockwise arc from `start` to `end` degrees with an arrowhead at `end`."""
    head_len = width * 1.25
    end_arc = end - math.degrees(head_len * 0.55 / r)
    cx, cy = c
    d.arc([cx - r - width / 2, cy - r - width / 2, cx + r + width / 2, cy + r + width / 2],
          start, end_arc, fill=color, width=round(width))
    a = math.radians(end_arc)
    base = (cx + r * math.cos(a), cy + r * math.sin(a))
    tangent = (-math.sin(a), math.cos(a))
    normal = (math.cos(a), math.sin(a))
    half = width * 1.15
    tip = (base[0] + tangent[0] * head_len, base[1] + tangent[1] * head_len)
    d.polygon([(base[0] + normal[0] * half, base[1] + normal[1] * half),
               (base[0] - normal[0] * half, base[1] - normal[1] * half), tip], fill=color)


def clipboard(px: int, box=(0.0, 0.0, 1.0, 1.0)) -> Image.Image:
    """The clipboard, drawn into `box` (fractions of a `px`-square canvas)."""
    S = px * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    bx0, by0, bx1, by1 = (v * S for v in box)
    W = bx1 - bx0

    def X(u):
        return bx0 + u * W

    def Y(v):
        return by0 + v * (by1 - by0)

    def stroke(unit, floor_px):
        return max(unit * W, floor_px * SS * (W / S))

    outline = stroke(0.028, 1.0)
    board = (X(0.125), Y(0.15), X(0.875), Y(0.96))
    radius = 0.06 * W

    d = ImageDraw.Draw(img)
    d.rounded_rectangle(board, radius, fill=NAVY)
    inner = (board[0] + outline, board[1] + outline, board[2] - outline, board[3] - outline)
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle(inner, max(0, radius - outline), fill=255)
    img.paste(_vgradient((S, S), inner, YELLOW_TOP, YELLOW_BOTTOM), (0, 0), mask)

    knob_r = 0.085 * W
    knob_c = (X(0.5), Y(0.085))
    clip = (X(0.31), Y(0.085), X(0.69), Y(0.235))
    clip_r = 0.04 * W
    d.ellipse([knob_c[0] - knob_r, knob_c[1] - knob_r, knob_c[0] + knob_r, knob_c[1] + knob_r], fill=NAVY)
    d.rounded_rectangle(clip, clip_r, fill=NAVY)
    ki = knob_r - outline
    d.ellipse([knob_c[0] - ki, knob_c[1] - ki, knob_c[0] + ki, knob_c[1] + ki], fill=ORANGE)
    ci = (clip[0] + outline, clip[1] + outline, clip[2] - outline, clip[3] - outline)
    d.rounded_rectangle(ci, max(0, clip_r - outline), fill=ORANGE)
    band = ci[3] - (ci[3] - ci[1]) * 0.32
    d.rounded_rectangle((ci[0], band, ci[2], ci[3]), max(0, clip_r - outline), fill=ORANGE_DARK)
    d.rectangle((ci[0], band, ci[2], band + (ci[3] - band) / 2), fill=ORANGE_DARK)
    hole = max(0.032 * W, 0.9 * SS * (W / S))
    d.ellipse([knob_c[0] - hole, knob_c[1] - hole * 1.3, knob_c[0] + hole, knob_c[1] + hole * 0.7], fill=NAVY)

    arrow_w = stroke(0.0625, 2.0)
    c = (X(0.5), Y(0.56))
    r = 0.19 * W
    _arrow(d, c, r, 195, 345, arrow_w, TEAL)
    _arrow(d, c, r, 15, 165, arrow_w, TEAL)

    return img.resize((px, px), Image.LANCZOS)


def tile(px: int) -> Image.Image:
    """The clipboard on a navy rounded tile, on Apple's 824-in-1024 grid."""
    S = px * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    m = S * 100 / 1024
    box = (m, m, S - m, S - m)
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle(box, (S - 2 * m) * 0.225, fill=255)
    img.paste(_vgradient((S, S), box, TILE_TOP, TILE_BOTTOM), (0, 0), mask)
    out = img.resize((px, px), Image.LANCZOS)
    inset = 0.205
    out.alpha_composite(clipboard(px, (inset, inset - 0.01, 1 - inset, 1 - inset - 0.01)))
    return out


def transmitting(img: Image.Image) -> Image.Image:
    r, g, b, a = img.split()
    boost = lambda v: min(255, round(v * 1.25))  # noqa: E731
    return Image.merge("RGBA", (r, g.point(boost), b.point(boost), a))


def disabled(img: Image.Image) -> Image.Image:
    grey = img.convert("L").point(lambda v: round(v * 0.72))
    return Image.merge("RGBA", (grey, grey, grey, img.getchannel("A")))


def main() -> None:
    ico = [clipboard(n) for n in ICO_SIZES]
    ico[-1].save(ROOT / "icon" / "ClipboardSync.ico", sizes=[(n, n) for n in ICO_SIZES], append_images=ico[:-1])
    icns = [tile(n) for n in ICNS_SIZES]
    icns[-1].save(ROOT / "icon" / "ClipboardSync.icns", append_images=icns[:-1])
    for px, suffix in ((32, ""), (64, "@2x")):
        idle = clipboard(px)
        idle.save(ROOT / f"TrayIcon{suffix}.png")
        transmitting(idle).save(ROOT / f"TrayIconTransmitting{suffix}.png")
        disabled(idle).save(ROOT / f"TrayIconDisabled{suffix}.png")


if __name__ == "__main__":
    main()
