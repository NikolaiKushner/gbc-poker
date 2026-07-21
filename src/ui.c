#include <gb/gb.h>
#include <gb/cgb.h>
#include "ui.h"
#include "text.h"
#include "rand.h"
#include "eval.h"
#include "carddata.h"
#include "portraits.h"
#include "sound.h"

/* CGB background palettes (idx0 = face/bg, idx3 = ink) */
#define PAL_FELT    0   /* green felt, white text */
#define PAL_CARD_BK 1   /* white card, black ink  */
#define PAL_CARD_RD 2   /* white card, red ink    */
#define PAL_BACK    3   /* navy card back         */
#define PAL_HILITE  4   /* gold ink on felt       */
#define PAL_PROF    5   /* portrait: Professor    */
#define PAL_COWB    6   /* portrait: Cowboy       */
#define PAL_SHRK    7   /* portrait: Shark        */
#define PAL_PORTRAIT(persona) ((uint8_t)(PAL_PROF + (persona) - 1))

static const palette_color_t felt_palettes[] = {
    RGB(3, 18, 7),   RGB(2, 12, 5),   RGB(5, 22, 10),  RGB(31, 31, 31), /* FELT */
    RGB(31, 31, 31), RGB(24, 24, 24), RGB(12, 12, 12), RGB(0, 0, 0),    /* CARD_BK */
    RGB(31, 31, 31), RGB(28, 12, 12), RGB(28, 6, 6),   RGB(27, 0, 0),   /* CARD_RD */
    RGB(3, 6, 16),   RGB(6, 10, 22),  RGB(10, 16, 28), RGB(20, 26, 31), /* BACK */
    RGB(3, 18, 7),   RGB(2, 12, 5),   RGB(5, 22, 10),  RGB(31, 27, 4),  /* HILITE */
    RGB(3, 18, 7),   RGB(30, 22, 16), RGB(4, 3, 2),    RGB(25, 25, 25), /* PROF: skin/dark/grey */
    RGB(3, 18, 7),   RGB(30, 22, 16), RGB(16, 9, 3),   RGB(28, 4, 4),   /* COWB: skin/brown/red */
    RGB(3, 18, 7),   RGB(15, 17, 21), RGB(5, 7, 11),   RGB(31, 31, 31), /* SHRK: grey/dark/white */
};
#define NUM_PALETTES 8

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
    clear_rect(0, 0, SCR_W, SCR_H);
    SHOW_BKG;
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

/* vertical menu: returns chosen index, or 0xFF if cancelled (when allowed).
   disabled_mask bit i set => item i is shown dim and skipped by the cursor. */
static uint8_t ui_menu(uint8_t x, uint8_t y, const char *const *items,
                       uint8_t n, uint8_t allow_cancel, uint8_t disabled_mask)
{
    uint8_t sel = 0, i, k;
    while (sel < n && (disabled_mask & (1 << sel))) sel++;
    for (;;) {
        for (i = 0; i < n; i++) {
            uint8_t row = (uint8_t)(y + i * 2);
            uint8_t on = (i == sel);
            put_str(x, row, on ? ">" : " ", PAL_HILITE);
            put_str((uint8_t)(x + 2), row, items[i], on ? PAL_HILITE : PAL_FELT);
        }
        k = wait_key();
        if (k & (J_UP | J_DOWN)) sfx_move();
        if (k & J_UP)   do { sel = (uint8_t)(sel ? sel - 1 : n - 1); } while (disabled_mask & (1 << sel));
        if (k & J_DOWN) do { sel = (uint8_t)((sel + 1) % n); }         while (disabled_mask & (1 << sel));
        if (k & J_A) { sfx_chip(); return sel; }
        if (allow_cancel && (k & (J_B | J_SELECT))) return 0xFF;
    }
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

static void draw_seat_line(uint8_t x, uint8_t y, char tag, const char *name)
{
    char t[2];
    t[0] = tag; t[1] = 0;
    put_str(x, y, t, tag == '>' ? PAL_HILITE : PAL_FELT);
    put_str((uint8_t)(x + 1), y, name, PAL_FELT);
}

static void draw_opponent(const GameState *g, uint8_t seat)
{
    uint8_t x = OPP_X[seat - 1];
    const Player *p = &g->players[seat];
    char tag = seat_tag(g, seat);
    char t[2];
    uint8_t nx;

    clear_rect(x, 1, 6, 5);
    if (p->flags & PF_OUT) {
        put_str(x, 2, PLAYER_NAMES[seat], PAL_FELT);
        put_str(x, 3, "OUT", PAL_FELT);
        return;
    }

    draw_portrait(x, 1, p->persona);

    /* face-down hand (2 back tiles) unless folded */
    if (!(p->flags & PF_FOLDED)) {
        uint8_t row[2] = { T_BACK, T_BACK };
        set_bkg_tiles(x, 3, 2, 1, row);
        fill_attr(x, 3, 2, 1, PAL_BACK);
    }

    t[0] = tag; t[1] = 0;
    nx = put_str(x, 4, t, tag == '>' ? PAL_HILITE : PAL_FELT);
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
    x = put_str(5, 6, "POT $", PAL_HILITE);
    x = put_num(x, 6, g->pot, PAL_HILITE);
    if (g->cur_bet) {
        x = put_str((uint8_t)(x + 1), 6, "=", PAL_FELT);
        put_num(x, 6, g->cur_bet, PAL_FELT);
    }

    /* hero (centered) */
    clear_rect(0, HERO_Y - 1, SCR_W, 3);
    draw_seat_line(6, (uint8_t)(HERO_Y - 1), seat_tag(g, 0), "YOU $");
    put_num(12, (uint8_t)(HERO_Y - 1), g->players[0].stack, PAL_FELT);
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
    (void)g;
    clear_row(14);
    x = put_str(0, 14, PLAYER_NAMES[seat], PAL_HILITE);
    x = put_str(x, 14, ": ", PAL_FELT);
    switch (action) {
    case ACT_FOLD:  put_str(x, 14, "FOLD", PAL_FELT); sfx_fold(); break;
    case ACT_CHECK: put_str(x, 14, "CHECK", PAL_FELT); break;
    case ACT_CALL:
        x = put_str(x, 14, "CALL ", PAL_FELT);
        put_num(x, 14, paid, PAL_FELT);
        if (paid) sfx_chip();
        break;
    case ACT_RAISE:
        x = put_str(x, 14, "RAISE ", PAL_FELT);
        put_num(x, 14, g->cur_bet, PAL_FELT);
        sfx_chip();
        break;
    }
    for (i = 0; i < 22; i++) vsync();
}

uint8_t ui_prompt_action(const GameState *g, uint16_t *raise_to)
{
    const Player *p = &g->players[0];
    uint16_t to_call = game_to_call(g);
    uint8_t can_check = (p->bet >= g->cur_bet);
    uint8_t sel = 0;
    uint8_t k, x;

    for (;;) {
        clear_row(15);
        clear_row(16);
        x = put_str(0, 15, "FOLD", sel == 2 ? PAL_HILITE : PAL_FELT);
        x++;
        if (can_check) {
            x = put_str(x, 15, "CHECK", sel == 0 ? PAL_HILITE : PAL_FELT);
        } else {
            x = put_str(x, 15, "CALL", sel == 0 ? PAL_HILITE : PAL_FELT);
            x = put_num(x, 15, to_call, sel == 0 ? PAL_HILITE : PAL_FELT);
        }
        x++;
        put_str(x, 15, "RAISE", sel == 1 ? PAL_HILITE : PAL_FELT);
        put_str(5, 16, "SELECT=MENU", PAL_FELT);

        k = wait_key();
        if (k & J_SELECT) { clear_row(15); clear_row(16); return ACT_MENU; }
        if (k & (J_LEFT | J_RIGHT | J_B)) sfx_move();
        if (k & J_LEFT)  sel = sel == 0 ? 2 : (uint8_t)(sel - 1);
        if (k & J_RIGHT) sel = sel == 2 ? 0 : (uint8_t)(sel + 1);
        if (k & J_B) sel = 2;
        if (k & J_A) {
            if (sel == 2) { clear_row(15); clear_row(16); return ACT_FOLD; }
            if (sel == 0) { clear_row(15); clear_row(16); return can_check ? ACT_CHECK : ACT_CALL; }
            {
                uint16_t max_to = p->bet + p->stack;
                uint16_t amt = g->cur_bet + g->min_raise;
                if (amt > max_to) amt = max_to;
                for (;;) {
                    clear_row(16);
                    if (amt >= max_to) {
                        uint8_t xx = put_str(1, 16, "ALL-IN ", PAL_HILITE);
                        put_num(xx, 16, max_to, PAL_HILITE);
                    } else {
                        uint8_t xx = put_str(1, 16, "RAISE ", PAL_HILITE);
                        xx = put_num(xx, 16, amt, PAL_HILITE);
                        put_str((uint8_t)(xx + 1), 16, "U/D", PAL_FELT);
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
                    if (k & J_A) { *raise_to = amt; clear_row(15); clear_row(16); return ACT_RAISE; }
                    if (k & J_B) { clear_row(16); break; }
                }
            }
        }
    }
}

static const char *const CAT_NAMES[9] = {
    "HIGH CARD", "PAIR", "TWO PAIR", "TRIPS", "STRAIGHT",
    "FLUSH", "FULL HOUSE", "QUADS", "STR FLUSH"
};

void ui_show_showdown(const GameState *g)
{
    uint8_t i, x;

    for (i = 1; i < MAX_PLAYERS; i++) {
        const Player *p = &g->players[i];
        uint8_t sx = OPP_X[i - 1];
        if (g->shown[i]) {
            draw_card(sx, 3, p->hole[0]);
            draw_card((uint8_t)(sx + CARD_STEP), 3, p->hole[1]);
        }
        clear_rect(sx, 5, 5, 1);
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
