#include <gb/gb.h>
#include <gb/cgb.h>
#include "ui.h"
#include "text.h"
#include "rand.h"
#include "eval.h"
#include "carddata.h"
#include "portraits.h"
#include "sound.h"
#include "uidata.h"

/* CGB background palettes (idx0 = face/bg, idx3 = ink) */
#define PAL_FELT    0   /* green felt, white text */
#define PAL_CARD_BK 1   /* white card, black ink  */
#define PAL_CARD_RD 2   /* white card, red ink    */
#define PAL_BACK    3   /* navy card back         */
#define PAL_HILITE  4   /* gold ink on felt       */
#define PAL_SEL     PAL_BACK  /* inverted selection: navy box, light ink */
#define PAL_PROF    5   /* portrait: Professor    */
#define PAL_COWB    6   /* portrait: Cowboy       */
#define PAL_SHRK    7   /* portrait: Shark        */
#define PAL_PORTRAIT(persona) ((uint8_t)(PAL_PROF + (persona) - 1))

#define NUM_PALETTES 8

/* Palettes, sprite tile, and category names live in bank 1 (uidata.c). */

/* White-list menus: normal rows use PAL_CARD_BK (white bg / black ink); the
   selected row borrows one palette slot for white bg / blue ink (menu_hi_pal).
   All 8 CGB background palettes are otherwise in use, so the slot is set on
   menu entry and restored on exit (from felt_palettes). */
#define PAL_MENU     PAL_CARD_BK
#define PAL_MENU_HI  PAL_SHRK

#define SCR_W 20
#define SCR_H 18

#define CARD_STEP 3      /* 2-tile card + 1-tile felt gap */
#define BOARD_X 3
#define BOARD_Y 7
#define HERO_CARDS_X 7   /* centered: (20 - (2+1+2)) / 2 */
#define HERO_Y  11
static const uint8_t OPP_X[3] = { 0, 7, 14 };

static uint8_t prev_keys;
static uint8_t line_buf[SCR_W];   /* scratch for one row of tiles/attrs */

/* ------------------------------ input ------------------------------ */
static uint8_t keys_pressed(void)
{
    uint8_t k = joypad();
    uint8_t fresh = k & (uint8_t)~prev_keys;
    prev_keys = k;
    return fresh;
}

static uint8_t wait_key(void)
{
    uint8_t k;
    for (;;) {
        vsync();
        rand_mix(DIV_REG);
        k = keys_pressed();
        if (k) return k;
    }
}

/* --------------------------- primitives ---------------------------- */
static void fill_attr(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal)
{
    uint8_t i, r;
    for (i = 0; i < w; i++) line_buf[i] = pal;
    for (r = 0; r < h; r++) set_bkg_attributes(x, (uint8_t)(y + r), w, 1, line_buf);
}

static void fill_tiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t tile)
{
    uint8_t i, r;
    for (i = 0; i < w; i++) line_buf[i] = tile;
    for (r = 0; r < h; r++) set_bkg_tiles(x, (uint8_t)(y + r), w, 1, line_buf);
}

/* clear a rectangle to felt */
static void clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    fill_tiles(x, y, w, h, T_WHITE);
    fill_attr(x, y, w, h, PAL_FELT);
}

static void clear_row(uint8_t y) { clear_rect(0, y, SCR_W, 1); }

/* print a string; clears the rest of the field to felt if pad>len */
static uint8_t put_str(uint8_t x, uint8_t y, const char *s, uint8_t pal)
{
    uint8_t n = 0;
    while (s[n] && (x + n) < SCR_W) {
        line_buf[n] = T_CHAR(s[n]);
        n++;
    }
    if (n) {
        set_bkg_tiles(x, y, n, 1, line_buf);
        fill_attr(x, y, n, 1, pal);
    }
    return (uint8_t)(x + n);
}

/* unsigned to decimal into a static buffer, returns pointer to first digit */
static char num_buf[6];
static const char *u16str(uint16_t v)
{
    uint8_t i = 5;
    num_buf[5] = 0;
    if (!v) { num_buf[4] = '0'; return &num_buf[4]; }
    while (v) { num_buf[--i] = (char)('0' + (v % 10)); v /= 10; }
    return &num_buf[i];
}

static uint8_t put_num(uint8_t x, uint8_t y, uint16_t v, uint8_t pal)
{
    return put_str(x, y, u16str(v), pal);
}

/* bold '$' + amount in the large 5x7 digits, for emphasised key totals */
static uint8_t put_money_big(uint8_t x, uint8_t y, uint16_t v, uint8_t pal)
{
    const char *s = u16str(v);
    uint8_t n = 0, i = 0;
    line_buf[n++] = T_BIG_DOLLAR;
    while (s[i] && (x + n) < SCR_W) {
        line_buf[n++] = T_BIG_DIGIT((uint8_t)(s[i] - '0'));
        i++;
    }
    set_bkg_tiles(x, y, n, 1, line_buf);
    fill_attr(x, y, n, 1, pal);
    return (uint8_t)(x + n);
}

/* --------------------------- animation ----------------------------- */
/* slide the chip sprite from tile (fx,fy) to tile (tx,ty), then park it */
static void anim_chip(uint8_t fx, uint8_t fy, uint8_t tx, uint8_t ty)
{
    int16_t x0 = (int16_t)fx * 8 + 8, y0 = (int16_t)fy * 8 + 16;
    int16_t x1 = (int16_t)tx * 8 + 8, y1 = (int16_t)ty * 8 + 16;
    uint8_t i;
    for (i = 1; i <= 8; i++) {
        int16_t cx = x0 + (x1 - x0) * (int16_t)i / 8;
        int16_t cy = y0 + (y1 - y0) * (int16_t)i / 8;
        move_sprite(0, (uint8_t)cx, (uint8_t)cy);
        vsync();
        vsync();
    }
    move_sprite(0, 0, 0);
}

/* ---------------------------- cards -------------------------------- */
static void draw_card(uint8_t x, uint8_t y, uint8_t card)
{
    uint8_t s = SUIT(card), r = RANK(card);
    uint8_t pal = (s == SUIT_HEARTS || s == SUIT_DIAMONDS) ? PAL_CARD_RD : PAL_CARD_BK;
    uint8_t top[2], bot[2];
    top[0] = T_RANK(r); top[1] = T_SUIT(s);   /* big rank + suit pip */
    bot[0] = T_WHITE;   bot[1] = T_SUIT(s);   /* mirrored pip, card-like */
    set_bkg_tiles(x, y, 2, 1, top);
    set_bkg_tiles(x, (uint8_t)(y + 1), 2, 1, bot);
    fill_attr(x, y, 2, 2, pal);
}

static void draw_back(uint8_t x, uint8_t y)
{
    uint8_t row[2] = { T_BACK, T_BACK };
    set_bkg_tiles(x, y, 2, 1, row);
    set_bkg_tiles(x, (uint8_t)(y + 1), 2, 1, row);
    fill_attr(x, y, 2, 2, PAL_BACK);
}

/* 2x2 persona avatar */
static void draw_portrait(uint8_t x, uint8_t y, uint8_t persona)
{
    uint8_t base = PORTRAIT_TILES_OF(persona);
    uint8_t top[2], bot[2];
    top[0] = base;                bot[0] = (uint8_t)(base + 2);
    top[1] = (uint8_t)(base + 1); bot[1] = (uint8_t)(base + 3);
    set_bkg_tiles(x, y, 2, 1, top);
    set_bkg_tiles(x, (uint8_t)(y + 1), 2, 1, bot);
    fill_attr(x, y, 2, 2, PAL_PORTRAIT(persona));
}

/* ------------------------------ init ------------------------------- */
void ui_init(void)
{
    prev_keys = 0;
    sound_init();
    DISPLAY_OFF;
    SWITCH_ROM(BANK(CARD_TILES));         /* tile data lives in bank 1 */
    set_bkg_data(CARD_TILE_BASE, CARD_TILE_COUNT, CARD_TILES);
    SWITCH_ROM(BANK(PORTRAIT_TILES));
    set_bkg_data(PORTRAIT_TILE_BASE, PORTRAIT_TILE_COUNT, PORTRAIT_TILES);
    if (_cpu == CGB_TYPE) set_bkg_palette(0, NUM_PALETTES, felt_palettes);
    set_sprite_data(0, 1, chip_sprite);
    set_sprite_tile(0, 0);
    if (_cpu == CGB_TYPE) set_sprite_palette(0, 1, chip_pal);
    move_sprite(0, 0, 0);          /* parked off-screen until an animation runs */
    clear_rect(0, 0, SCR_W, SCR_H);
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

void ui_title(void)
{
    uint16_t frames = 0;
    clear_rect(0, 0, SCR_W, SCR_H);
    put_str(5, 3, "GBC POKER", PAL_HILITE);
    put_str(3, 5, "TEXAS HOLD-EM", PAL_FELT);
    draw_card(4, 7,  MAKE_CARD(SUIT_SPADES, RANK_ACE));
    draw_card(9, 7,  MAKE_CARD(SUIT_HEARTS, RANK_KING));
    draw_card(14, 7, MAKE_CARD(SUIT_DIAMONDS, RANK_QUEEN));
    put_str(1, 11, "PROF  COWBOY  SHARK", PAL_FELT);
    put_str(4, 15, "PRESS START", PAL_FELT);
    for (;;) {
        vsync();
        frames++;
        if (keys_pressed()) {
            rand_init((uint16_t)(((uint16_t)DIV_REG << 8) ^ frames ^ DIV_REG));
            if (prev_keys & J_START) { jingle_title(); break; }
        }
    }
    clear_rect(0, 0, SCR_W, SCR_H);
}

#define MENU_W 14   /* width of a menu row's white bar (cursor + space + label) */

/* one menu row: a solid white bar, black text, with a '>' cursor; the selected
   row is drawn white bg / blue text via the borrowed PAL_MENU_HI slot. */
static void draw_menu_row(uint8_t x, uint8_t row, const char *label, uint8_t on)
{
    uint8_t n = 0, i = 0;
    line_buf[n++] = T_CHAR(on ? '>' : ' ');
    line_buf[n++] = T_CHAR(' ');
    while (label[i] && n < MENU_W) line_buf[n++] = T_CHAR(label[i++]);
    while (n < MENU_W) line_buf[n++] = T_CHAR(' ');
    set_bkg_tiles(x, row, MENU_W, 1, line_buf);
    fill_attr(x, row, MENU_W, 1, on ? PAL_MENU_HI : PAL_MENU);
}

/* vertical menu: returns chosen index, or 0xFF if cancelled (when allowed).
   disabled_mask bit i set => item i is skipped by the cursor. */
static uint8_t ui_menu(uint8_t x, uint8_t y, const char *const *items,
                       uint8_t n, uint8_t allow_cancel, uint8_t disabled_mask)
{
    uint8_t sel = 0, i, k, result;
    while (sel < n && (disabled_mask & (1 << sel))) sel++;
    if (_cpu == CGB_TYPE) set_bkg_palette(PAL_MENU_HI, 1, menu_hi_pal);
    for (;;) {
        for (i = 0; i < n; i++)
            draw_menu_row(x, (uint8_t)(y + i * 2), items[i], i == sel);
        k = wait_key();
        if (k & (J_UP | J_DOWN)) sfx_move();
        if (k & J_UP)   do { sel = (uint8_t)(sel ? sel - 1 : n - 1); } while (disabled_mask & (1 << sel));
        if (k & J_DOWN) do { sel = (uint8_t)((sel + 1) % n); }         while (disabled_mask & (1 << sel));
        if (k & J_A) { sfx_chip(); result = sel; break; }
        if (allow_cancel && (k & (J_B | J_SELECT))) { result = 0xFF; break; }
    }
    /* give the borrowed palette slot back to the Shark portrait */
    if (_cpu == CGB_TYPE) set_bkg_palette(PAL_MENU_HI, 1, &felt_palettes[PAL_MENU_HI * 4]);
    return result;
}

void ui_toast(const char *msg)
{
    uint8_t i;
    clear_row(9);
    put_str(4, 9, msg, PAL_HILITE);
    for (i = 0; i < 45; i++) vsync();
    clear_row(9);
}

uint8_t ui_main_menu(uint8_t can_load)
{
    static const char *const items[3] = { "NEW GAME", "LOAD GAME", "STATS" };
    uint8_t dis = can_load ? 0 : (1 << MM_LOAD);
    uint8_t r;
    clear_rect(0, 0, SCR_W, SCR_H);
    put_str(5, 2, "GBC POKER", PAL_HILITE);
    draw_card(2, 4,  MAKE_CARD(SUIT_SPADES, RANK_ACE));
    draw_card(16, 4, MAKE_CARD(SUIT_HEARTS, RANK_ACE));
    r = ui_menu(6, 9, items, 3, 0, dis);
    clear_rect(0, 0, SCR_W, SCR_H);
    return r;
}

uint8_t ui_ingame_menu(void)
{
    static const char *const items[3] = { "CONTINUE", "SAVE GAME", "QUIT TO MENU" };
    uint8_t r;
    clear_rect(0, 12, SCR_W, 6);
    put_str(1, 12, "-- MENU --", PAL_HILITE);
    r = ui_menu(3, 13, items, 3, 1, 0);
    clear_rect(0, 12, SCR_W, 6);
    return (r == 0xFF) ? IGM_CONTINUE : r;
}

/* ---------------------------- table -------------------------------- */
static char seat_tag(const GameState *g, uint8_t i)
{
    const Player *p = &g->players[i];
    if (p->flags & PF_OUT) return 'X';
    if (p->flags & PF_FOLDED) return '-';
    if (p->flags & PF_ALLIN) return '!';
    if (g->to_act == i) return '>';
    if (g->dealer == i) return 'D';
    return ' ';
}

static void draw_opponent(const GameState *g, uint8_t seat)
{
    uint8_t x = OPP_X[seat - 1];
    const Player *p = &g->players[seat];
    uint8_t active = (g->to_act == seat) &&
                     !(p->flags & (PF_OUT | PF_FOLDED | PF_ALLIN));
    char tag;
    char t[2];
    uint8_t nx;

    clear_rect(x, 1, 6, 5);
    if (p->flags & PF_OUT) {
        put_str(x, 2, PLAYER_NAMES[seat], PAL_FELT);
        put_str(x, 3, "OUT", PAL_FELT);
        return;
    }

    /* portrait, with its face-down hand tucked beside it (unless folded) */
    draw_portrait(x, 1, p->persona);
    if (!(p->flags & PF_FOLDED)) {
        uint8_t row[2] = { T_BACK, T_BACK };
        set_bkg_tiles((uint8_t)(x + 3), 1, 2, 1, row);
        fill_attr((uint8_t)(x + 3), 1, 2, 1, PAL_BACK);
    }

    /* name directly under the portrait; inverted box when it is this seat's
       turn so the active player is unmistakable */
    put_str(x, 3, PLAYER_NAMES[seat], active ? PAL_SEL : PAL_FELT);

    /* stack directly under the name (dealer/fold/all-in marker kept as prefix) */
    tag = seat_tag(g, seat);
    if (tag == '>') tag = ' ';        /* active is shown by the name highlight */
    t[0] = tag; t[1] = 0;
    nx = put_str(x, 4, t, PAL_FELT);
    nx = put_str(nx, 4, "$", PAL_FELT);
    put_num(nx, 4, p->stack, PAL_FELT);

    if (p->bet && !(p->flags & PF_FOLDED)) {
        nx = put_str(x, 5, "B", PAL_HILITE);
        put_num(nx, 5, p->bet, PAL_HILITE);
    }
}

static void draw_board(const GameState *g)
{
    uint8_t i;
    clear_rect(BOARD_X, BOARD_Y, 5 * CARD_STEP, 2);
    for (i = 0; i < g->board_count; i++)
        draw_card((uint8_t)(BOARD_X + i * CARD_STEP), BOARD_Y, g->board[i]);
}

/* Reveal the board cards added since from_idx with a face-down -> face flip,
   left to right, so a new street "lands" on the table instead of popping in.
   Call right after ui_draw_table so the freshly blitted faces are covered
   before the next vblank shows them. */
void ui_deal_animate(const GameState *g, uint8_t from_idx)
{
    uint8_t i, f;
    for (i = from_idx; i < g->board_count; i++)
        draw_back((uint8_t)(BOARD_X + i * CARD_STEP), BOARD_Y);
    for (i = from_idx; i < g->board_count; i++) {
        sfx_deal();
        for (f = 0; f < 7; f++) vsync();
        draw_card((uint8_t)(BOARD_X + i * CARD_STEP), BOARD_Y, g->board[i]);
        sfx_chip();
        for (f = 0; f < 5; f++) vsync();
    }
}

void ui_draw_table(const GameState *g, uint8_t hand_no)
{
    uint8_t i, x;

    clear_row(0);
    x = put_str(0, 0, "H", PAL_FELT);
    x = put_num(x, 0, (uint16_t)(hand_no + 1), PAL_FELT);
    x = put_str((uint8_t)(x + 1), 0, "L", PAL_FELT);
    x = put_num(x, 0, (uint16_t)(g->bb_level + 1), PAL_FELT);
    x = put_num((uint8_t)(x + 1), 0, BLIND_LEVELS[g->bb_level][0], PAL_FELT);
    x = put_str(x, 0, "/", PAL_FELT);
    put_num(x, 0, BLIND_LEVELS[g->bb_level][1], PAL_FELT);

    for (i = 1; i < MAX_PLAYERS; i++) draw_opponent(g, i);
    draw_board(g);

    clear_row(6);
    put_str(1, 6, "POT", PAL_HILITE);
    x = put_money_big(5, 6, g->pot, PAL_HILITE);
    /* the bet-to-match sits right, with clear air between it and the pot */
    if (g->cur_bet) {
        uint8_t bx = 13;
        if (x + 2 > bx) bx = (uint8_t)(x + 2);
        bx = put_str(bx, 6, "BET", PAL_FELT);
        put_num((uint8_t)(bx + 1), 6, g->cur_bet, PAL_FELT);
    }

    /* hero (centered) */
    clear_rect(0, HERO_Y - 1, SCR_W, 3);
    {
        char tg = seat_tag(g, 0);
        uint8_t active = (g->to_act == 0) &&
                         !(g->players[0].flags & (PF_OUT | PF_FOLDED | PF_ALLIN));
        char t2[2];
        t2[0] = (tg == '>') ? ' ' : tg; t2[1] = 0;
        put_str(4, (uint8_t)(HERO_Y - 1), t2, PAL_FELT);
        put_str(5, (uint8_t)(HERO_Y - 1), "YOU", active ? PAL_SEL : PAL_FELT);
    }
    put_money_big(10, (uint8_t)(HERO_Y - 1), g->players[0].stack, PAL_FELT);
    if (!(g->players[0].flags & PF_OUT)) {
        draw_card(HERO_CARDS_X, HERO_Y, g->players[0].hole[0]);
        draw_card((uint8_t)(HERO_CARDS_X + CARD_STEP), HERO_Y, g->players[0].hole[1]);
    }
    if (g->players[0].bet) {
        uint8_t nx = put_str((uint8_t)(HERO_CARDS_X + 2 * CARD_STEP), HERO_Y, "B", PAL_HILITE);
        put_num(nx, HERO_Y, g->players[0].bet, PAL_HILITE);
    }
}

void ui_flash_action(const GameState *g, uint8_t seat, uint8_t action, uint16_t paid)
{
    uint8_t i, x;
    uint8_t chips = 0;         /* fly a chip to the pot for this action? */
    clear_row(14);
    x = put_str(0, 14, PLAYER_NAMES[seat], PAL_HILITE);
    x = put_str(x, 14, ": ", PAL_FELT);
    switch (action) {
    case ACT_FOLD:  put_str(x, 14, "FOLD", PAL_FELT); sfx_fold(); break;
    case ACT_CHECK: put_str(x, 14, "CHECK", PAL_FELT); break;
    case ACT_CALL:
        x = put_str(x, 14, "CALL ", PAL_FELT);
        put_num(x, 14, paid, PAL_FELT);
        if (paid) { sfx_chip(); chips = 1; }
        break;
    case ACT_RAISE:
        x = put_str(x, 14, "RAISE ", PAL_FELT);
        put_num(x, 14, g->cur_bet, PAL_FELT);
        sfx_chip(); chips = 1;
        break;
    }
    if (chips) {
        /* slide a chip from the actor's seat into the pot (row 6) */
        uint8_t ox, oy;
        if (seat == 0) { ox = (uint8_t)(HERO_CARDS_X + 2 * CARD_STEP); oy = HERO_Y; }
        else { ox = (uint8_t)(OPP_X[seat - 1] + 1); oy = 5; }
        anim_chip(ox, oy, 9, 6);
    } else {
        for (i = 0; i < 22; i++) vsync();
    }
}

uint8_t ui_prompt_action(const GameState *g, uint16_t *raise_to)
{
    const Player *p = &g->players[0];
    uint16_t to_call = game_to_call(g);
    uint8_t can_check = (p->bet >= g->cur_bet);
    uint8_t sel = 0;
    uint8_t k;

    for (;;) {
        /* selected item drawn inverted (dark box, light ink); items spaced
           out — FOLD left, CALL/CHECK centre, RAISE right — for quick reading */
        uint8_t pf = sel == 2 ? PAL_SEL : PAL_FELT;
        uint8_t pc = sel == 0 ? PAL_SEL : PAL_FELT;
        uint8_t pr = sel == 1 ? PAL_SEL : PAL_FELT;
        clear_row(16);
        clear_row(17);
        put_str(0, 16, "FOLD", pf);
        if (can_check) {
            put_str(6, 16, "CHECK", pc);
        } else {
            uint8_t cx = put_str(6, 16, "CALL", pc);
            put_num(cx, 16, to_call, pc);
        }
        put_str(15, 16, "RAISE", pr);

        k = wait_key();
        if (k & J_SELECT) { clear_row(16); clear_row(17); return ACT_MENU; }
        if (k & (J_LEFT | J_RIGHT | J_B)) sfx_move();
        if (k & J_LEFT)  sel = sel == 0 ? 2 : (uint8_t)(sel - 1);
        if (k & J_RIGHT) sel = sel == 2 ? 0 : (uint8_t)(sel + 1);
        if (k & J_B) sel = 2;
        if (k & J_A) {
            if (sel == 2) { clear_row(16); clear_row(17); return ACT_FOLD; }
            if (sel == 0) { clear_row(16); clear_row(17); return can_check ? ACT_CHECK : ACT_CALL; }
            {
                uint16_t max_to = p->bet + p->stack;
                uint16_t amt = g->cur_bet + g->min_raise;
                if (amt > max_to) amt = max_to;
                for (;;) {
                    clear_row(17);
                    if (amt >= max_to) {
                        uint8_t xx = put_str(1, 17, "ALL-IN ", PAL_HILITE);
                        put_num(xx, 17, max_to, PAL_HILITE);
                    } else {
                        uint8_t xx = put_str(1, 17, "RAISE ", PAL_HILITE);
                        xx = put_num(xx, 17, amt, PAL_HILITE);
                        put_str((uint8_t)(xx + 1), 17, "U/D", PAL_FELT);
                    }
                    k = wait_key();
                    if (k & J_UP) {
                        amt += g->min_raise;
                        if (amt > max_to) amt = max_to;
                    }
                    if (k & J_DOWN) {
                        if (amt >= g->cur_bet + g->min_raise + g->min_raise)
                            amt -= g->min_raise;
                        else amt = g->cur_bet + g->min_raise;
                        if (amt > max_to) amt = max_to;
                    }
                    if (k & J_A) { *raise_to = amt; clear_row(16); clear_row(17); return ACT_RAISE; }
                    if (k & J_B) { clear_row(17); break; }
                }
            }
        }
    }
}

void ui_show_showdown(const GameState *g)
{
    uint8_t i, x;

    for (i = 1; i < MAX_PLAYERS; i++) {
        const Player *p = &g->players[i];
        uint8_t sx = OPP_X[i - 1];
        if (g->shown[i]) {   /* reveal beside the portrait, where the backs sat */
            draw_card((uint8_t)(sx + 2), 1, p->hole[0]);
            draw_card((uint8_t)(sx + 4), 1, p->hole[1]);
        }
        clear_rect(sx, 5, 6, 1);
        if (g->won[i]) {
            uint8_t nx = put_str(sx, 5, "+", PAL_HILITE);
            put_num(nx, 5, g->won[i], PAL_HILITE);
        }
    }
    if (g->won[0]) {
        uint8_t nx = put_str((uint8_t)(HERO_CARDS_X + 2 * CARD_STEP), HERO_Y, "+", PAL_HILITE);
        put_num(nx, HERO_Y, g->won[0], PAL_HILITE);
    }

    clear_row(14);
    for (i = 0; i < MAX_PLAYERS; i++)
        if (g->won[i] && g->shown[i]) {
            x = put_str(0, 14, PLAYER_NAMES[i], PAL_HILITE);
            x = put_str(x, 14, " WINS ", PAL_FELT);
            put_str(x, 14, CAT_NAMES[EVAL_CATEGORY(g->shown[i])], PAL_FELT);
            break;
        }
    clear_row(15);
    clear_row(16);
    put_str(2, 16, "A = NEXT HAND", PAL_FELT);
    if (g->won[0]) jingle_win();
    while (!(wait_key() & (J_A | J_START))) ;
    clear_row(14);
    clear_row(16);
}

void ui_show_stats(const SaveData *s)
{
    clear_rect(0, 0, SCR_W, SCR_H);
    put_str(7, 1, "STATS", PAL_HILITE);
    put_str(2, 4, "HANDS", PAL_FELT);
    put_num(13, 4, s->hands_played, PAL_FELT);
    put_str(2, 6, "TABLES", PAL_FELT);
    put_num(13, 6, s->tourneys_played, PAL_FELT);
    put_str(2, 8, "WINS", PAL_HILITE);
    put_num(13, 8, s->championships, PAL_HILITE);
    put_str(2, 10, "BEST", PAL_FELT);
    if (s->hands_played)
        put_str(9, 10, CAT_NAMES[s->best_category], PAL_HILITE);
    else
        put_str(9, 10, "-", PAL_FELT);
    put_str(4, 15, "B = BACK", PAL_FELT);
    while (!(wait_key() & (J_B | J_START | J_A))) ;
    clear_rect(0, 0, SCR_W, SCR_H);
}

void ui_game_over(uint8_t place)
{
    clear_rect(0, 0, SCR_W, SCR_H);
    put_str(4, 7, "BUSTED OUT", PAL_HILITE);
    put_str(4, 9, "PLACE ", PAL_FELT);
    put_num(10, 9, place, PAL_FELT);
    put_str(3, 14, "START = AGAIN", PAL_FELT);
    jingle_lose();
    while (!(wait_key() & J_START)) ;
}

void ui_victory(void)
{
    clear_rect(0, 0, SCR_W, SCR_H);
    put_str(3, 6, "* CHAMPION *", PAL_HILITE);
    put_str(1, 9, "YOU WIN THE TABLE", PAL_FELT);
    put_str(3, 14, "START = AGAIN", PAL_FELT);
    jingle_win(); jingle_win();
    while (!(wait_key() & J_START)) ;
}
