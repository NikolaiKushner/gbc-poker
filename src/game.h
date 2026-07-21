#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "cards.h"
#include "deck.h"

#define MAX_PLAYERS 4

/* Player.flags */
#define PF_FOLDED 0x01
#define PF_ALLIN  0x02
#define PF_OUT    0x04   /* busted from the table */
#define PF_ACTED  0x08   /* has acted since the last raise on this street */

/* GameState.street */
#define ST_PREFLOP 0
#define ST_FLOP    1
#define ST_TURN    2
#define ST_RIVER   3
#define ST_SHOWDOWN 4

/* Actions */
#define ACT_FOLD  0
#define ACT_CHECK 1
#define ACT_CALL  2
#define ACT_RAISE 3   /* amount = new total street bet (bet or raise-to) */

typedef struct {
    uint8_t  hole[2];
    uint16_t stack;
    uint16_t bet;        /* committed on the current street */
    uint16_t total_bet;  /* committed over the whole hand (for side pots) */
    uint8_t  flags;
    uint8_t  persona;    /* 0 = human, 1..3 = AI profile */
} Player;

typedef struct {
    Player  players[MAX_PLAYERS];
    Deck    deck;
    uint8_t board[5];
    uint8_t board_count;
    uint16_t pot;        /* sum of all total_bet */
    uint16_t cur_bet;    /* street bet to match */
    uint16_t min_raise;  /* minimum raise increment */
    uint8_t dealer;
    uint8_t street;
    uint8_t to_act;      /* index of the player whose turn it is */
    uint8_t bb_level;    /* index into blinds table */
    /* showdown results, filled by game_showdown() */
    uint16_t won[MAX_PLAYERS];
    uint32_t shown[MAX_PLAYERS];  /* eval7 of live hands, 0 if folded */
} GameState;

extern const uint16_t BLIND_LEVELS[][2];  /* {sb, bb} */
#define NUM_BLIND_LEVELS 8

void game_init(GameState *g, uint16_t start_stack);
void game_start_hand(GameState *g);       /* shuffle, deal, post blinds */

/* Betting interface: while !game_betting_done(g), get g->to_act,
   ask that player (UI or AI) and call game_apply_action().
   When done, game_advance_street() deals the next street or returns 1
   at showdown time (street == ST_SHOWDOWN). */
uint8_t game_betting_done(const GameState *g);
uint8_t game_legal_check(const GameState *g);         /* can current actor check? */
uint16_t game_to_call(const GameState *g);            /* chips owed by actor (capped by stack) */
void game_apply_action(GameState *g, uint8_t action, uint16_t amount);
uint8_t game_advance_street(GameState *g);            /* returns 1 when hand should go to showdown */

void game_showdown(GameState *g);         /* build side pots, award chips into won[] */
uint8_t game_players_left(const GameState *g);        /* not folded this hand */
uint8_t game_alive_count(const GameState *g);         /* not busted from table */

#endif
