#ifndef CARDDATA_H
#define CARDDATA_H

#include <stdint.h>
#include <gb/gb.h>

#define CARD_TILE_BASE 0x80
#define CARD_TILE_COUNT 71

#define T_WHITE       ((uint8_t)(CARD_TILE_BASE + 0))
#define T_BACK        ((uint8_t)(CARD_TILE_BASE + 1))
#define T_SUIT_SPADE  ((uint8_t)(CARD_TILE_BASE + 2))
#define T_RANK_2      ((uint8_t)(CARD_TILE_BASE + 6))
#define FONT_BASE     ((uint8_t)(CARD_TILE_BASE + 19))

/* rank r in 2..14 -> big card glyph; suit s in 0..3 -> pip tile */
#define T_RANK(r) ((uint8_t)(T_RANK_2 + ((r) - 2)))
#define T_SUIT(s) ((uint8_t)(T_SUIT_SPADE + (s)))
#define T_GLYPH(slot) ((uint8_t)(FONT_BASE + (slot)))

BANKREF_EXTERN(CARD_TILES)
extern const uint8_t CARD_TILES[1136];
extern const uint8_t FONT_MAP[96];

/* tile for an ASCII char (falls back to space) */
#define T_CHAR(c) ((uint8_t)(FONT_BASE + FONT_MAP[((c) & 0x7F) - 32]))

#endif
