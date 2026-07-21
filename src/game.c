#include "game.h"
#include "eval.h"
#include "rand.h"

const uint16_t BLIND_LEVELS[][2] = {
    {5, 10}, {10, 20}, {15, 30}, {25, 50},
    {50, 100}, {75, 150}, {100, 200}, {150, 300},
};

#define IS_ACTIVE(p) (!((p)->flags & (PF_OUT | PF_FOLDED | PF_ALLIN)))
#define IN_HAND(p)   (!((p)->flags & (PF_OUT | PF_FOLDED)))

static uint8_t next_alive(const GameState *g, uint8_t i)
{
    uint8_t n;
    for (n = 0; n < MAX_PLAYERS; n++) {
        i = (uint8_t)((i + 1) & (MAX_PLAYERS - 1));
        if (!(g->players[i].flags & PF_OUT)) return i;
    }
    return i;
}

static uint8_t next_actor(const GameState *g, uint8_t i)
{
    uint8_t n;
    for (n = 0; n < MAX_PLAYERS; n++) {
        i = (uint8_t)((i + 1) & (MAX_PLAYERS - 1));
        if (IS_ACTIVE(&g->players[i])) return i;
    }
    return i;
}

uint8_t game_players_left(const GameState *g)
{
    uint8_t i, n = 0;
    for (i = 0; i < MAX_PLAYERS; i++)
        if (IN_HAND(&g->players[i])) n++;
    return n;
}

uint8_t game_alive_count(const GameState *g)
{
    uint8_t i, n = 0;
    for (i = 0; i < MAX_PLAYERS; i++)
        if (!(g->players[i].flags & PF_OUT)) n++;
    return n;
}

/* Move chips from stack into the street bet; sets ALLIN when stack empties. */
static void pay(GameState *g, uint8_t idx, uint16_t amount)
{
    Player *p = &g->players[idx];
    if (amount >= p->stack) {
        amount = p->stack;
        p->flags |= PF_ALLIN;
    }
    p->stack -= amount;
    p->bet += amount;
    p->total_bet += amount;
    g->pot += amount;
}

void game_init(GameState *g, uint16_t start_stack)
{
    uint8_t i;
    for (i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &g->players[i];
        p->stack = start_stack;
        p->flags = 0;
        p->persona = i;
        p->bet = 0;
        p->total_bet = 0;
    }
    g->bb_level = 0;
    g->dealer = MAX_PLAYERS - 1;   /* first hand advances it to 0 */
}

void game_start_hand(GameState *g)
{
    uint8_t i, sb_pos, bb_pos;
    uint16_t sb = BLIND_LEVELS[g->bb_level][0];
    uint16_t bb = BLIND_LEVELS[g->bb_level][1];

    for (i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &g->players[i];
        p->bet = 0;
        p->total_bet = 0;
        p->flags &= PF_OUT;
        g->won[i] = 0;
        g->shown[i] = 0;
    }
    g->pot = 0;
    g->board_count = 0;
    g->street = ST_PREFLOP;

    g->dealer = next_alive(g, g->dealer);

    deck_init(&g->deck);
    deck_shuffle(&g->deck);
    for (i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &g->players[i];
        if (!(p->flags & PF_OUT)) {
            p->hole[0] = deck_deal(&g->deck);
            p->hole[1] = deck_deal(&g->deck);
        }
    }

    if (game_alive_count(g) == 2) {
        /* heads-up: dealer posts SB and acts first preflop */
        sb_pos = g->dealer;
        bb_pos = next_alive(g, sb_pos);
    } else {
        sb_pos = next_alive(g, g->dealer);
        bb_pos = next_alive(g, sb_pos);
    }
    pay(g, sb_pos, sb);
    pay(g, bb_pos, bb);
    g->cur_bet = bb;
    g->min_raise = bb;
    g->to_act = next_actor(g, bb_pos);
}

uint8_t game_betting_done(const GameState *g)
{
    uint8_t i, active = 0, pending = 0;
    const Player *lone = 0;

    if (game_players_left(g) <= 1) return 1;

    for (i = 0; i < MAX_PLAYERS; i++) {
        const Player *p = &g->players[i];
        if (!IS_ACTIVE(p)) continue;
        active++;
        lone = p;
        if (!(p->flags & PF_ACTED) || p->bet != g->cur_bet) pending++;
    }
    if (active == 0) return 1;
    if (active == 1) return lone->bet >= g->cur_bet;  /* raising vs all-ins is moot */
    return pending == 0;
}

uint8_t game_legal_check(const GameState *g)
{
    return g->players[g->to_act].bet >= g->cur_bet;
}

uint16_t game_to_call(const GameState *g)
{
    const Player *p = &g->players[g->to_act];
    uint16_t owed = g->cur_bet - p->bet;
    return owed > p->stack ? p->stack : owed;
}

void game_apply_action(GameState *g, uint8_t action, uint16_t amount)
{
    Player *p = &g->players[g->to_act];
    uint8_t i;

    switch (action) {
    case ACT_FOLD:
        p->flags |= PF_FOLDED;
        break;
    case ACT_CHECK:
        p->flags |= PF_ACTED;
        break;
    case ACT_CALL:
        pay(g, g->to_act, g->cur_bet - p->bet);
        p->flags |= PF_ACTED;
        break;
    case ACT_RAISE:
        /* amount = target total street bet; clamp to legal minimum,
           an all-in for less is always allowed */
        if (amount < g->cur_bet + g->min_raise)
            amount = g->cur_bet + g->min_raise;
        if (amount - p->bet >= p->stack)
            amount = p->bet + p->stack;         /* all-in */
        pay(g, g->to_act, amount - p->bet);
        p->flags |= PF_ACTED;
        if (p->bet > g->cur_bet) {
            if (p->bet - g->cur_bet >= g->min_raise)
                g->min_raise = p->bet - g->cur_bet;
            g->cur_bet = p->bet;
            /* reopen action for everyone else */
            for (i = 0; i < MAX_PLAYERS; i++)
                if (i != g->to_act)
                    g->players[i].flags &= (uint8_t)~PF_ACTED;
        }
        break;
    }

    if (!game_betting_done(g))
        g->to_act = next_actor(g, g->to_act);
}

uint8_t game_advance_street(GameState *g)
{
    uint8_t i;

    if (game_players_left(g) <= 1) {
        g->street = ST_SHOWDOWN;
        return 1;
    }
    if (g->street == ST_RIVER) {
        g->street = ST_SHOWDOWN;
        return 1;
    }

    for (i = 0; i < MAX_PLAYERS; i++) {
        g->players[i].bet = 0;
        g->players[i].flags &= (uint8_t)~PF_ACTED;
    }
    g->cur_bet = 0;
    g->min_raise = BLIND_LEVELS[g->bb_level][1];

    g->street++;
    if (g->street == ST_FLOP) {
        g->board[0] = deck_deal(&g->deck);
        g->board[1] = deck_deal(&g->deck);
        g->board[2] = deck_deal(&g->deck);
        g->board_count = 3;
    } else {
        g->board[g->board_count++] = deck_deal(&g->deck);
    }
    g->to_act = next_actor(g, g->dealer);
    return 0;
}

void game_showdown(GameState *g)
{
    uint16_t contrib[MAX_PLAYERS];
    uint32_t strength[MAX_PLAYERS];
    uint8_t cards[7];
    uint8_t i, j;

    for (i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &g->players[i];
        contrib[i] = p->total_bet;
        strength[i] = 0;
        g->won[i] = 0;
        if (IN_HAND(p) && game_players_left(g) > 1) {
            cards[0] = p->hole[0];
            cards[1] = p->hole[1];
            for (j = 0; j < 5; j++) cards[2 + j] = g->board[j];
            strength[i] = eval7(cards);
            g->shown[i] = strength[i];
        }
    }

    if (game_players_left(g) == 1) {
        for (i = 0; i < MAX_PLAYERS; i++)
            if (IN_HAND(&g->players[i])) {
                g->won[i] = g->pot;
                break;
            }
    } else {
        /* peel off side pots layer by layer */
        for (;;) {
            uint16_t level = 0xFFFF, layer = 0;
            uint32_t best = 0;
            uint8_t winners = 0;
            uint16_t share, rem;

            for (i = 0; i < MAX_PLAYERS; i++)
                if (contrib[i] && contrib[i] < level) level = contrib[i];
            if (level == 0xFFFF) break;

            for (i = 0; i < MAX_PLAYERS; i++) {
                if (!contrib[i]) continue;
                layer += level;
                if (strength[i] > best && !(g->players[i].flags & (PF_FOLDED | PF_OUT)))
                    best = strength[i];
            }
            for (i = 0; i < MAX_PLAYERS; i++)
                if (contrib[i] && strength[i] == best &&
                    !(g->players[i].flags & (PF_FOLDED | PF_OUT)))
                    winners++;

            if (winners == 0) {
                /* only folded money left in this layer: return it pro rata
                   (happens for an uncalled bet's excess) */
                for (i = 0; i < MAX_PLAYERS; i++)
                    if (contrib[i]) g->won[i] += level;
            } else {
                share = layer / winners;
                rem = layer - share * winners;
                /* odd chips go to the first winners after the dealer */
                j = g->dealer;
                for (i = 0; i < MAX_PLAYERS; i++) {
                    j = (uint8_t)((j + 1) & (MAX_PLAYERS - 1));
                    if (contrib[j] && strength[j] == best &&
                        !(g->players[j].flags & (PF_FOLDED | PF_OUT))) {
                        g->won[j] += share;
                        if (rem) { g->won[j]++; rem--; }
                    }
                }
            }
            for (i = 0; i < MAX_PLAYERS; i++)
                if (contrib[i]) contrib[i] -= level;
        }
    }

    for (i = 0; i < MAX_PLAYERS; i++) {
        g->players[i].stack += g->won[i];
        if (g->players[i].stack == 0)
            g->players[i].flags |= PF_OUT;
    }
}
