#!/usr/bin/env python3
"""Generate docs/cover.png: box-art style cover for emulator libraries.

Homebrew ROMs aren't in any artwork database, so front-ends (Delta/Provenance on
iOS, EmulationStation on the R36S, RetroArch) show a generic placeholder. This
builds a crisp cover from the game's own title screen that you can set as the
game's custom artwork.
"""
from PIL import Image, ImageDraw

# colors pulled from the in-game CGB palette (0..31 scaled to 0..255)
FELT_LIGHT = (28, 150, 66)
FELT_DARK = (10, 74, 34)
GOLD = (255, 222, 33)
INK = (6, 30, 14)

W = H = 512
SHOT = "docs/title.png"          # 160x144 native title screen
OUT = "docs/cover.png"


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def main():
    img = Image.new("RGB", (W, H), FELT_DARK)
    d = ImageDraw.Draw(img)

    # vertical felt gradient
    for y in range(H):
        d.line([(0, y), (W, y)], fill=lerp(FELT_LIGHT, FELT_DARK, y / (H - 1)))

    # crisp integer upscale of the title screen (160x144 -> 480x432)
    shot = Image.open(SHOT).convert("RGB").resize((480, 432), Image.NEAREST)
    sx, sy = (W - shot.width) // 2, (H - shot.height) // 2

    # drop shadow + gold frame behind the screenshot
    d.rectangle([sx - 2, sy - 2, sx + shot.width + 8, sy + shot.height + 8],
                fill=(0, 0, 0))
    d.rectangle([sx - 6, sy - 6, sx + shot.width + 5, sy + shot.height + 5],
                fill=GOLD)
    d.rectangle([sx - 3, sy - 3, sx + shot.width + 2, sy + shot.height + 2],
                fill=INK)
    img.paste(shot, (sx, sy))

    # suit pips in the corners
    def pip(cx, cy, color, kind):
        r = 9
        if kind in ("h", "d"):
            d.polygon([(cx, cy + r), (cx - r, cy - r // 3), (cx + r, cy - r // 3)],
                      fill=color)
            if kind == "h":
                d.ellipse([cx - r, cy - r, cx, cy], fill=color)
                d.ellipse([cx, cy - r, cx + r, cy], fill=color)
        else:  # spade/club: simple filled diamond-ish blob
            d.polygon([(cx, cy - r), (cx + r, cy), (cx, cy + r), (cx - r, cy)],
                      fill=color)

    m = 26
    pip(m, m, INK, "s")
    pip(W - m, m, (200, 20, 20), "h")
    pip(m, H - m, (200, 20, 20), "d")
    pip(W - m, H - m, INK, "c")

    img.save(OUT)
    print("wrote", OUT, img.size)


if __name__ == "__main__":
    main()
