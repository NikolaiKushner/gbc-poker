#ifndef UIDATA_H
#define UIDATA_H

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

/* Read-only UI tables kept in bank 1 (see uidata.c) so the home bank does not
   overflow. Bank 1 is the switchable bank left mapped after ui_init, so these
   are always readable without an explicit SWITCH_ROM. */
extern const palette_color_t felt_palettes[];   /* 8 bkg palettes, idx*4 */
extern const palette_color_t chip_pal[4];       /* chip sprite palette */
extern const palette_color_t menu_hi_pal[4];    /* borrowed menu-highlight slot */
extern const uint8_t chip_sprite[16];           /* chip sprite tile */
extern const char *const CAT_NAMES[9];          /* hand-category names */

#endif
