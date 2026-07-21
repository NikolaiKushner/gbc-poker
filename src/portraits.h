#ifndef PORTRAITS_H
#define PORTRAITS_H

#include <stdint.h>
#include <gb/gb.h>
#include "carddata.h"

/* loaded just above the card tiles */
#define PORTRAIT_TILE_BASE ((uint8_t)(CARD_TILE_BASE + CARD_TILE_COUNT))
#define PORTRAIT_TILE_COUNT 12
/* persona 1..3 -> first of its 4 tiles */
#define PORTRAIT_TILES_OF(persona) ((uint8_t)(PORTRAIT_TILE_BASE + ((persona) - 1) * 4))

BANKREF_EXTERN(PORTRAIT_TILES)
extern const uint8_t PORTRAIT_TILES[192];

#endif
