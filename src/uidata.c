#pragma bank 1
/* Read-only UI tables parked in bank 1 to keep the home bank (0) under 16 KB.
   The game only ever switches ROM in ui_init, which leaves bank 1 mapped, so
   these are always readable at their 0x4000-0x7FFF addresses. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include "uidata.h"

/* CGB background palettes (idx0 = face/bg, idx3 = ink) */
const palette_color_t felt_palettes[] = {
    RGB(3, 18, 7),   RGB(2, 12, 5),   RGB(5, 22, 10),  RGB(31, 31, 31), /* FELT */
    RGB(31, 31, 31), RGB(24, 24, 24), RGB(12, 12, 12), RGB(0, 0, 0),    /* CARD_BK */
    RGB(31, 31, 31), RGB(28, 12, 12), RGB(28, 6, 6),   RGB(27, 0, 0),   /* CARD_RD */
    RGB(3, 6, 16),   RGB(6, 10, 22),  RGB(10, 16, 28), RGB(20, 26, 31), /* BACK */
    RGB(3, 18, 7),   RGB(2, 12, 5),   RGB(5, 22, 10),  RGB(31, 27, 4),  /* HILITE */
    RGB(3, 18, 7),   RGB(30, 22, 16), RGB(4, 3, 2),    RGB(25, 25, 25), /* PROF: skin/dark/grey */
    RGB(3, 18, 7),   RGB(30, 22, 16), RGB(16, 9, 3),   RGB(28, 4, 4),   /* COWB: skin/brown/red */
    RGB(3, 18, 7),   RGB(15, 17, 21), RGB(5, 7, 11),   RGB(31, 31, 31), /* SHRK: grey/dark/white */
};

/* chip sprite (obj 0): a small gold disc that flies bets into the pot */
const uint8_t chip_sprite[16] = {
    0x3C, 0x00, 0x7E, 0x00, 0xFF, 0x00, 0xE7, 0x00,
    0xE7, 0x00, 0xFF, 0x00, 0x7E, 0x00, 0x3C, 0x00,
};
const palette_color_t chip_pal[4] = {
    RGB(0, 0, 0), RGB(31, 27, 4), RGB(24, 18, 0), RGB(0, 0, 0),
};

/* white bg / blue ink, borrowed into one bkg palette slot for the menu cursor */
const palette_color_t menu_hi_pal[4] = {
    RGB(31, 31, 31), RGB(18, 20, 31), RGB(6, 10, 28), RGB(2, 6, 28),
};

const char *const CAT_NAMES[9] = {
    "HIGH CARD", "PAIR", "TWO PAIR", "TRIPS", "STRAIGHT",
    "FLUSH", "FULL HOUSE", "QUADS", "STR FLUSH"
};
