#!/usr/bin/env python3
"""Generate src/carddata.c/.h: 2bpp background tiles for the poker table.

Every tile uses only color index 0 (background/face) and 3 (ink), so the CGB
background palette selected at draw time decides the real colors:
  PAL_FELT    -> idx0 green, idx3 white   (table + text)
  PAL_CARD_BK -> idx0 white, idx3 black   (spades/clubs)
  PAL_CARD_RD -> idx0 white, idx3 red     (hearts/diamonds)
  PAL_BACK    -> idx0 navy,  idx3 skyblue (face-down backs)

Tile layout in VRAM (base 0x80):
  0            WHITE   (solid idx0: felt when PAL_FELT, card body when a card pal)
  1            BACK    (face-down pattern)
  2..5         suit pips: spade, heart, diamond, club
  6..          font glyphs (see FONT_CHARS); ranks reuse these glyphs
"""

# 3x5 pixel font.
FONT = {
    '0': ["111", "101", "101", "101", "111"],
    '1': ["010", "110", "010", "010", "111"],
    '2': ["111", "001", "111", "100", "111"],
    '3': ["111", "001", "111", "001", "111"],
    '4': ["101", "101", "111", "001", "001"],
    '5': ["111", "100", "111", "001", "111"],
    '6': ["111", "100", "111", "101", "111"],
    '7': ["111", "001", "010", "010", "010"],
    '8': ["111", "101", "111", "101", "111"],
    '9': ["111", "101", "111", "001", "111"],
    'A': ["111", "101", "111", "101", "101"],
    'B': ["110", "101", "110", "101", "110"],
    'C': ["111", "100", "100", "100", "111"],
    'D': ["110", "101", "101", "101", "110"],
    'E': ["111", "100", "110", "100", "111"],
    'F': ["111", "100", "110", "100", "100"],
    'G': ["111", "100", "101", "101", "111"],
    'H': ["101", "101", "111", "101", "101"],
    'I': ["111", "010", "010", "010", "111"],
    'J': ["001", "001", "001", "101", "111"],
    'K': ["101", "110", "100", "110", "101"],
    'L': ["100", "100", "100", "100", "111"],
    'M': ["101", "111", "111", "101", "101"],
    'N': ["101", "111", "111", "111", "101"],
    'O': ["111", "101", "101", "101", "111"],
    'P': ["111", "101", "111", "100", "100"],
    'Q': ["111", "101", "101", "111", "011"],
    'R': ["110", "101", "110", "101", "101"],
    'S': ["111", "100", "111", "001", "111"],
    'T': ["111", "010", "010", "010", "010"],
    'U': ["101", "101", "101", "101", "111"],
    'V': ["101", "101", "101", "101", "010"],
    'W': ["101", "101", "111", "111", "101"],
    'X': ["101", "101", "010", "101", "101"],
    'Y': ["101", "101", "010", "010", "010"],
    'Z': ["111", "001", "010", "100", "111"],
    ' ': ["000", "000", "000", "000", "000"],
    '!': ["010", "010", "010", "000", "010"],
    '$': ["010", "111", "110", "011", "010"],
    '%': ["101", "001", "010", "100", "101"],
    '/': ["001", "001", "010", "100", "100"],
    ':': ["000", "010", "000", "010", "000"],
    '-': ["000", "000", "111", "000", "000"],
    '.': ["000", "000", "000", "000", "010"],
    '>': ["100", "010", "001", "010", "100"],
    '<': ["001", "010", "100", "010", "001"],
    '=': ["000", "111", "000", "111", "000"],
    '+': ["000", "010", "111", "010", "000"],
    ',': ["000", "000", "000", "010", "100"],
    '*': ["000", "101", "010", "101", "000"],
    '(': ["001", "010", "010", "010", "001"],
    ')': ["100", "010", "010", "010", "100"],
}
# order of glyphs in VRAM; index in this list = font slot
FONT_CHARS = ("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              " !$%/:-.><=+,*()")

# Bold 5x7 rank glyphs, drawn large in the corner of a card for readability.
BIG_RANK = {
    '2': ["01110", "10001", "00001", "00110", "01000", "10000", "11111"],
    '3': ["11110", "00001", "00001", "01110", "00001", "00001", "11110"],
    '4': ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    '5': ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],
    '6': ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],
    '7': ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    '8': ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    '9': ["01110", "10001", "10001", "01111", "00001", "00010", "01100"],
    'T': ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    'J': ["00111", "00010", "00010", "00010", "10010", "10010", "01100"],
    'Q': ["01110", "10001", "10001", "10001", "10101", "10010", "01101"],
    'K': ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    'A': ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
}
BIG_RANK_ORDER = ['2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A']

# Bold 5x7 digits (+ $) for key amounts (pot, hero stack) so they stand out
# against the small-font opponent chips. 2..9 reuse the big rank shapes.
BIG_DIGIT = {
    '0': ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    '1': ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    '2': BIG_RANK['2'], '3': BIG_RANK['3'], '4': BIG_RANK['4'],
    '5': BIG_RANK['5'], '6': BIG_RANK['6'], '7': BIG_RANK['7'],
    '8': BIG_RANK['8'], '9': BIG_RANK['9'],
}
BIG_DIGIT_ORDER = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
BIG_DOLLAR = ["00100", "01111", "10100", "01110", "00101", "11110", "00100"]

SUIT = {
    'spade': ["00011000", "00111100", "01111110", "11111111",
              "11111111", "01011010", "00011000", "00111100"],
    'heart': ["01100110", "11111111", "11111111", "11111111",
              "01111110", "00111100", "00011000", "00000000"],
    'diamond': ["00011000", "00111100", "01111110", "11111111",
                "01111110", "00111100", "00011000", "00000000"],
    'club': ["00011000", "00111100", "00011000", "01111110",
             "11111111", "01111110", "00011000", "00111100"],
}
SUIT_ORDER = ['spade', 'heart', 'diamond', 'club']  # matches SUIT_* in cards.h


def blank():
    return [[0] * 8 for _ in range(8)]


def tile_white():
    return blank()


def tile_back():
    t = blank()
    for y in range(8):
        for x in range(8):
            if (x + y) % 2 == 0 and (x % 4 != 3) and (y % 4 != 3):
                t[y][x] = 3
    for i in range(8):
        t[0][i] = t[7][i] = t[i][0] = t[i][7] = 3
    return t


def tile_suit(name):
    t = blank()
    rows = SUIT[name]
    for y in range(8):
        for x in range(8):
            if rows[y][x] == '1':
                t[y][x] = 3
    return t


def tile_glyph(ch):
    t = blank()
    g = FONT[ch]
    ox, oy = 2, 1
    for y in range(5):
        for x in range(3):
            if g[y][x] == '1':
                t[oy + y][ox + x] = 3
    return t


def tile_big_5x7(rows):
    t = blank()
    ox, oy = 2, 0        # 5x7 glyph centered in the 8x8 tile
    for y in range(7):
        for x in range(5):
            if rows[y][x] == '1':
                t[oy + y][ox + x] = 3
    return t


def tile_big_rank(ch):
    return tile_big_5x7(BIG_RANK[ch])


def encode(tile):
    out = []
    for y in range(8):
        p0 = p1 = 0
        for x in range(8):
            c = tile[y][x]
            bit = 7 - x
            if c & 1:
                p0 |= (1 << bit)
            if c & 2:
                p1 |= (1 << bit)
        out.append(p0)
        out.append(p1)
    return out


def main():
    tiles = []
    tiles.append(("WHITE", tile_white()))
    tiles.append(("BACK", tile_back()))
    for s in SUIT_ORDER:
        tiles.append(("SUIT_" + s.upper(), tile_suit(s)))
    rank_base = len(tiles)
    for r in BIG_RANK_ORDER:
        tiles.append(("RANK_" + r, tile_big_rank(r)))
    dig_base = len(tiles)
    for d in BIG_DIGIT_ORDER:
        tiles.append(("DIG_" + d, tile_big_5x7(BIG_DIGIT[d])))
    dollar_index = len(tiles)
    tiles.append(("BIG_DOLLAR", tile_big_5x7(BIG_DOLLAR)))
    font_base = len(tiles)
    for ch in FONT_CHARS:
        tiles.append((None, tile_glyph(ch)))

    # map ASCII 32..127 -> font slot (0xFF = space)
    space_slot = FONT_CHARS.index(' ')
    ascii_map = []
    for code in range(32, 128):
        ch = chr(code)
        up = ch.upper()
        if up in FONT_CHARS:
            ascii_map.append(FONT_CHARS.index(up))
        else:
            ascii_map.append(space_slot)

    # CARD_TILES: bulk tile data, only read once at init -> banked (bank 1)
    with open("src/carddata.c", "w") as f:
        f.write("/* Generated by assets/gen_cards.py - do not edit by hand. */\n")
        f.write("#pragma bank 1\n")
        f.write("#include <gb/gb.h>\n")
        f.write('#include "carddata.h"\n\n')
        f.write("const uint8_t CARD_TILES[%d] = {\n" % (len(tiles) * 16))
        for name, t in tiles:
            b = encode(t)
            tag = name if name else "glyph"
            f.write("    " + ", ".join("0x%02X" % v for v in b) +
                    ",  /* %s */\n" % tag)
        f.write("};\n")
        f.write("BANKREF(CARD_TILES)\n")

    # FONT_MAP: read at runtime by every text draw -> keep in home bank (bank 0)
    with open("src/fontmap.c", "w") as f:
        f.write("/* Generated by assets/gen_cards.py - do not edit by hand. */\n")
        f.write('#include "carddata.h"\n\n')
        f.write("/* ASCII 32..127 -> font glyph slot */\n")
        f.write("const uint8_t FONT_MAP[96] = {\n")
        for i in range(0, 96, 16):
            f.write("    " + ", ".join("%d" % v for v in ascii_map[i:i + 16]) + ",\n")
        f.write("};\n")

    with open("src/carddata.h", "w") as f:
        f.write("#ifndef CARDDATA_H\n#define CARDDATA_H\n\n")
        f.write("#include <stdint.h>\n")
        f.write("#include <gb/gb.h>\n\n")
        f.write("#define CARD_TILE_BASE 0x80\n")
        f.write("#define CARD_TILE_COUNT %d\n\n" % len(tiles))
        f.write("#define T_WHITE       ((uint8_t)(CARD_TILE_BASE + 0))\n")
        f.write("#define T_BACK        ((uint8_t)(CARD_TILE_BASE + 1))\n")
        f.write("#define T_SUIT_SPADE  ((uint8_t)(CARD_TILE_BASE + 2))\n")
        f.write("#define T_RANK_2      ((uint8_t)(CARD_TILE_BASE + %d))\n" % rank_base)
        f.write("#define T_DIG_0       ((uint8_t)(CARD_TILE_BASE + %d))\n" % dig_base)
        f.write("#define T_BIG_DOLLAR  ((uint8_t)(CARD_TILE_BASE + %d))\n" % dollar_index)
        f.write("#define FONT_BASE     ((uint8_t)(CARD_TILE_BASE + %d))\n\n" % font_base)
        f.write("/* rank r in 2..14 -> big card glyph; suit s in 0..3 -> pip tile */\n")
        f.write("#define T_RANK(r) ((uint8_t)(T_RANK_2 + ((r) - 2)))\n")
        f.write("/* bold 5x7 digit d in 0..9, for emphasised amounts */\n")
        f.write("#define T_BIG_DIGIT(d) ((uint8_t)(T_DIG_0 + (d)))\n")
        f.write("#define T_SUIT(s) ((uint8_t)(T_SUIT_SPADE + (s)))\n")
        f.write("#define T_GLYPH(slot) ((uint8_t)(FONT_BASE + (slot)))\n\n")
        f.write("BANKREF_EXTERN(CARD_TILES)\n")
        f.write("extern const uint8_t CARD_TILES[%d];\n" % (len(tiles) * 16))
        f.write("extern const uint8_t FONT_MAP[96];\n\n")
        f.write("/* tile for an ASCII char (falls back to space) */\n")
        f.write("#define T_CHAR(c) ((uint8_t)(FONT_BASE + FONT_MAP[((c) & 0x7F) - 32]))\n\n")
        f.write("#endif\n")

    print("wrote carddata.{c,h} (bank 1) + fontmap.c: %d tiles, font_base=%d" %
          (len(tiles), font_base))


if __name__ == "__main__":
    main()
