/* Native tests: build with gcc, run on the host. Covers eval7 (table cases,
   kickers, brute-force cross-check), side pots, and full-engine invariants. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cards.h"
#include "rand.h"
#include "deck.h"
#include "eval.h"
#include "game.h"
#include "ai.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        printf("FAIL %s:%d: ", __FILE__, __LINE__); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } \
} while (0)

/* ---- card shorthand: "As" "Td" "9c" ---- */
static uint8_t C(const char *s)
{
    uint8_t r, su;
    switch (s[0]) {
    case 'T': r = 10; break;
    case 'J': r = 11; break;
    case 'Q': r = 12; break;
    case 'K': r = 13; break;
    case 'A': r = 14; break;
    default: r = (uint8_t)(s[0] - '0');
    }
    switch (s[1]) {
    case 's': su = SUIT_SPADES; break;
    case 'h': su = SUIT_HEARTS; break;
    case 'd': su = SUIT_DIAMONDS; break;
    default: su = SUIT_CLUBS;
    }
    return MAKE_CARD(su, r);
}

static uint32_t ev(const char *a, const char *b, const char *c, const char *d,
                   const char *e, const char *f, const char *g)
{
    uint8_t cards[7];
    cards[0] = C(a); cards[1] = C(b); cards[2] = C(c); cards[3] = C(d);
    cards[4] = C(e); cards[5] = C(f); cards[6] = C(g);
    return eval7(cards);
}

/* ---------- reference 5-card evaluator (slow, obviously correct) ---------- */
static uint32_t eval5_ref(const uint8_t *cards)
{
    uint8_t r[5], i, j, t;
    uint8_t cnt[15] = {0};
    uint8_t flush = 1, straight = 0, hi = 0;
    uint8_t quads = 0, trips = 0, phi = 0, plo = 0;
    uint32_t v;

    for (i = 0; i < 5; i++) r[i] = RANK(cards[i]);
    for (i = 0; i < 5; i++)
        for (j = (uint8_t)(i + 1); j < 5; j++)
            if (r[j] > r[i]) { t = r[i]; r[i] = r[j]; r[j] = t; }
    for (i = 0; i < 5; i++) cnt[r[i]]++;
    for (i = 1; i < 5; i++) if (SUIT(cards[i]) != SUIT(cards[0])) flush = 0;

    if (r[0] != r[1] && r[1] != r[2] && r[2] != r[3] && r[3] != r[4]) {
        if (r[0] - r[4] == 4) { straight = 1; hi = r[0]; }
        else if (r[0] == 14 && r[1] == 5 && r[4] == 2 && r[1] - r[4] == 3)
            { straight = 1; hi = 5; }
    }

    for (i = 14; i >= 2; i--) {
        if (cnt[i] == 4) quads = i;
        else if (cnt[i] == 3) trips = i;
        else if (cnt[i] == 2) { if (!phi) phi = i; else plo = i; }
    }

    if (straight && flush) return ((uint32_t)HAND_STR_FLUSH << 20) | ((uint32_t)hi << 16);
    if (quads) {
        for (i = 14; i >= 2; i--) if (cnt[i] && i != quads) break;
        return ((uint32_t)HAND_QUADS << 20) | ((uint32_t)quads << 16) | ((uint32_t)i << 12);
    }
    if (trips && phi)
        return ((uint32_t)HAND_FULL_HOUSE << 20) | ((uint32_t)trips << 16) | ((uint32_t)phi << 12);
    if (flush) {
        v = (uint32_t)HAND_FLUSH << 20;
        for (i = 0; i < 5; i++) v |= (uint32_t)r[i] << (16 - 4 * i);
        return v;
    }
    if (straight) return ((uint32_t)HAND_STRAIGHT << 20) | ((uint32_t)hi << 16);
    if (trips) {
        v = ((uint32_t)HAND_TRIPS << 20) | ((uint32_t)trips << 16);
        j = 12;
        for (i = 14; i >= 2; i--)
            if (cnt[i] && i != trips) { v |= (uint32_t)i << j; j -= 4; }
        return v;
    }
    if (phi && plo) {
        for (i = 14; i >= 2; i--) if (cnt[i] == 1) break;
        return ((uint32_t)HAND_TWO_PAIR << 20) | ((uint32_t)phi << 16)
             | ((uint32_t)plo << 12) | ((uint32_t)i << 8);
    }
    if (phi) {
        v = ((uint32_t)HAND_PAIR << 20) | ((uint32_t)phi << 16);
        j = 12;
        for (i = 14; i >= 2; i--)
            if (cnt[i] == 1) { v |= (uint32_t)i << j; j -= 4; }
        return v;
    }
    v = (uint32_t)HAND_HIGH << 20;
    for (i = 0; i < 5; i++) v |= (uint32_t)r[i] << (16 - 4 * i);
    return v;
}

static uint32_t eval7_ref(const uint8_t *cards)
{
    static const uint8_t combos[21][5] = {
        {0,1,2,3,4},{0,1,2,3,5},{0,1,2,3,6},{0,1,2,4,5},{0,1,2,4,6},
        {0,1,2,5,6},{0,1,3,4,5},{0,1,3,4,6},{0,1,3,5,6},{0,1,4,5,6},
        {0,2,3,4,5},{0,2,3,4,6},{0,2,3,5,6},{0,2,4,5,6},{0,3,4,5,6},
        {1,2,3,4,5},{1,2,3,4,6},{1,2,3,5,6},{1,2,4,5,6},{1,3,4,5,6},
        {2,3,4,5,6},
    };
    uint32_t best = 0, v;
    uint8_t hand[5];
    int i, j;
    for (i = 0; i < 21; i++) {
        for (j = 0; j < 5; j++) hand[j] = cards[combos[i][j]];
        v = eval5_ref(hand);
        if (v > best) best = v;
    }
    return best;
}

/* ------------------------------ eval7 tests ------------------------------ */
static void test_categories(void)
{
    CHECK(EVAL_CATEGORY(ev("As","Ks","Qs","Js","Ts","2d","3c")) == HAND_STR_FLUSH, "royal flush");
    CHECK(ev("As","2s","3s","4s","5s","Kd","Qc") >> 16 == (HAND_STR_FLUSH << 4 | 5), "steel wheel is 5-high");
    CHECK(EVAL_CATEGORY(ev("9h","9d","9s","9c","Ks","2d","3c")) == HAND_QUADS, "quads");
    CHECK(EVAL_CATEGORY(ev("9h","9d","9s","Kc","Ks","2d","3c")) == HAND_FULL_HOUSE, "full house");
    CHECK(EVAL_CATEGORY(ev("Ah","Kh","9h","5h","2h","3c","8d")) == HAND_FLUSH, "flush");
    CHECK(EVAL_CATEGORY(ev("9h","8d","7s","6c","5s","Ad","Ac")) == HAND_STRAIGHT, "straight beats pair");
    CHECK(EVAL_CATEGORY(ev("Ah","2d","3s","4c","5s","9d","Jc")) == HAND_STRAIGHT, "wheel");
    CHECK(ev("Ah","2d","3s","4c","5s","9d","Jc") >> 16 == (HAND_STRAIGHT << 4 | 5), "wheel is 5-high");
    CHECK(EVAL_CATEGORY(ev("9h","9d","9s","Kc","Qs","2d","3c")) == HAND_TRIPS, "trips");
    CHECK(EVAL_CATEGORY(ev("9h","9d","Ks","Kc","Qs","2d","3c")) == HAND_TWO_PAIR, "two pair");
    CHECK(EVAL_CATEGORY(ev("9h","9d","Ks","Qc","Js","2d","3c")) == HAND_PAIR, "pair");
    CHECK(EVAL_CATEGORY(ev("9h","8d","Ks","Qc","Js","2d","3c")) == HAND_HIGH, "high card");
}

static void test_kickers(void)
{
    /* same pair, kicker decides */
    CHECK(ev("Ah","Ad","Ks","7c","5s","3d","2c") >
          ev("As","Ac","Qs","7d","5h","3c","2d"), "AA K-kicker > AA Q-kicker");
    /* 3rd kicker of a pair matters */
    CHECK(ev("Ah","Ad","Ks","Qc","Js","3d","2c") >
          ev("As","Ac","Ks","Qc","Ts","3c","2d"), "pair 3rd kicker J > T");
    /* two pair: bigger low pair wins */
    CHECK(ev("Ah","Ad","Ks","Kc","2s","3d","7c") >
          ev("As","Ac","Qs","Qd","Jh","3c","7d"), "AAKK > AAQQ");
    /* three pairs: best two play, third pair rank is the kicker */
    CHECK((ev("Ah","Ad","Ks","Kc","Qs","Qd","2c") >> 8 & 0xF) == RANK_QUEEN,
          "three pairs: Q kicks");
    /* identical hands chop exactly */
    CHECK(ev("Ah","Kd","Qs","Jc","9s","3d","2c") ==
          ev("Ad","Kh","Qc","Js","9d","3c","2h"), "same ranks = equal value");
    /* board plays: both players' kickers too low to matter */
    CHECK(ev("2h","3d","As","Ac","Ad","Ah","Ks") ==
          ev("2d","3h","As","Ac","Ad","Ah","Ks"), "board quads + K chop");
    /* flush: all five flush cards compared */
    CHECK(ev("Ah","Qh","9h","7h","3h","2c","2d") >
          ev("Ah","Qh","9h","7h","2h","3c","3d"), "flush 5th card 3 > 2");
    /* double trips = full house of the higher trips */
    {
        uint32_t v = ev("Ah","Ad","As","Kc","Ks","Kd","2c");
        CHECK(EVAL_CATEGORY(v) == HAND_FULL_HOUSE && ((v >> 16) & 0xF) == RANK_ACE
              && ((v >> 12) & 0xF) == RANK_KING, "AAA KKK -> aces full of kings");
    }
    /* quads with trips on board: trips rank is the kicker */
    CHECK((ev("Ah","Ad","As","Ac","Kd","Ks","Kc") >> 12 & 0xF) == RANK_KING,
          "quad aces, K kicker");
}

static void test_crosscheck(void)
{
    Deck d;
    uint8_t hand[7];
    uint32_t fast, slow;
    long i, n = 1000000;
    int bad = 0;

    rand_init(12345);
    deck_init(&d);
    for (i = 0; i < n; i++) {
        uint8_t j;
        deck_shuffle(&d);
        for (j = 0; j < 7; j++) hand[j] = deck_deal(&d);
        fast = eval7(hand);
        slow = eval7_ref(hand);
        if (fast != slow) {
            if (bad < 5) {
                printf("MISMATCH: cards");
                for (j = 0; j < 7; j++) printf(" %02x", hand[j]);
                printf("  fast=%06x slow=%06x\n", fast, slow);
            }
            bad++;
        }
    }
    CHECK(bad == 0, "cross-check: %d mismatches of %ld", bad, n);
    printf("cross-check: %ld random 7-card hands, %d mismatches\n", n, bad);
}

/* ------------------------------ side pots ------------------------------- */
static void set_board(GameState *g, const char *a, const char *b, const char *c,
                      const char *d, const char *e)
{
    g->board[0] = C(a); g->board[1] = C(b); g->board[2] = C(c);
    g->board[3] = C(d); g->board[4] = C(e);
    g->board_count = 5;
}

static void setup_showdown(GameState *g)
{
    memset(g, 0, sizeof(*g));
    g->dealer = 0;
}

static void test_sidepots_three_allins(void)
{
    GameState g;
    setup_showdown(&g);
    set_board(&g, "5h", "6h", "7h", "8h", "2c");
    /* P0: trips deuces; P1: 9-high straight flush (short stack);
       P2: king high (big stack); P3 folded 20 */
    g.players[0].hole[0] = C("2d"); g.players[0].hole[1] = C("2s");
    g.players[1].hole[0] = C("9h"); g.players[1].hole[1] = C("4d");
    g.players[2].hole[0] = C("Ks"); g.players[2].hole[1] = C("Qd");
    g.players[3].flags = PF_FOLDED;
    g.players[0].total_bet = 100; g.players[0].flags |= PF_ALLIN;
    g.players[1].total_bet = 50;  g.players[1].flags |= PF_ALLIN;
    g.players[2].total_bet = 200;
    g.players[3].total_bet = 20;
    g.pot = 370;

    game_showdown(&g);
    CHECK(g.won[1] == 170, "short stack wins main pot 170, got %u", g.won[1]);
    CHECK(g.won[0] == 100, "middle stack wins side pot 100, got %u", g.won[0]);
    CHECK(g.won[2] == 100, "big stack gets uncalled 100 back, got %u", g.won[2]);
    CHECK(g.won[3] == 0, "folded player gets nothing");
    CHECK(g.won[0] + g.won[1] + g.won[2] + g.won[3] == 370, "chips conserved");
}

static void test_sidepots_split_odd_chip(void)
{
    GameState g;
    setup_showdown(&g);
    set_board(&g, "Ah", "Kd", "Qs", "Jc", "2c");
    /* P0 and P1 both hold a ten -> identical broadway straights, chop */
    g.players[0].hole[0] = C("Ts"); g.players[0].hole[1] = C("3d");
    g.players[1].hole[0] = C("Th"); g.players[1].hole[1] = C("4d");
    g.players[2].hole[0] = C("9s"); g.players[2].hole[1] = C("9d");
    g.players[3].flags = PF_OUT;
    g.players[0].total_bet = 25; g.players[1].total_bet = 25;
    g.players[2].total_bet = 25;
    g.pot = 75;

    game_showdown(&g);
    CHECK(g.won[0] + g.won[1] == 75, "chop gets whole pot, got %u+%u",
          g.won[0], g.won[1]);
    CHECK(g.won[0] >= 37 && g.won[1] >= 37, "split is near-even (%u/%u)",
          g.won[0], g.won[1]);
    CHECK(g.won[2] == 0, "loser gets nothing");
}

static void test_sidepots_two_allins(void)
{
    GameState g;
    setup_showdown(&g);
    set_board(&g, "Ah", "Kd", "8s", "3c", "2c");
    /* P1 short all-in with the best hand (AK), P0 second (A8), P2 worst */
    g.players[0].hole[0] = C("As"); g.players[0].hole[1] = C("8d");
    g.players[1].hole[0] = C("Ac"); g.players[1].hole[1] = C("Kc");
    g.players[2].hole[0] = C("Qs"); g.players[2].hole[1] = C("Qd");
    g.players[3].flags = PF_OUT;
    g.players[0].total_bet = 300;
    g.players[1].total_bet = 120; g.players[1].flags |= PF_ALLIN;
    g.players[2].total_bet = 300;
    g.pot = 720;

    game_showdown(&g);
    CHECK(g.won[1] == 360, "main pot 3x120=360 to AK, got %u", g.won[1]);
    CHECK(g.won[0] == 360, "side pot 2x180=360 to A8, got %u", g.won[0]);
    CHECK(g.won[2] == 0, "QQ gets nothing, got %u", g.won[2]);
}

/* --------------------------- engine invariants --------------------------- */
static uint32_t total_chips(const GameState *g)
{
    uint32_t sum = 0;
    uint8_t i;
    for (i = 0; i < MAX_PLAYERS; i++) sum += g->players[i].stack;
    return sum;
}

static void test_hand_flow_scripted(void)
{
    GameState g;
    game_init(&g, 1000);
    rand_init(42);
    game_start_hand(&g);

    /* dealer=0, sb=1(5), bb=2(10), first to act = 3 */
    CHECK(g.dealer == 0, "dealer at 0, got %u", g.dealer);
    CHECK(g.players[1].bet == 5, "SB posted 5, got %u", g.players[1].bet);
    CHECK(g.players[2].bet == 10, "BB posted 10, got %u", g.players[2].bet);
    CHECK(g.to_act == 3, "UTG acts first, got %u", g.to_act);
    CHECK(g.pot == 15, "pot 15, got %u", g.pot);

    game_apply_action(&g, ACT_CALL, 0);            /* P3 calls 10 */
    game_apply_action(&g, ACT_RAISE, 30);          /* P0 raises to 30 */
    CHECK(g.cur_bet == 30, "cur_bet 30, got %u", g.cur_bet);
    CHECK(!game_betting_done(&g), "action reopened after raise");
    game_apply_action(&g, ACT_FOLD, 0);            /* P1 folds */
    game_apply_action(&g, ACT_CALL, 0);            /* P2 calls */
    game_apply_action(&g, ACT_CALL, 0);            /* P3 calls */
    CHECK(game_betting_done(&g), "preflop closed");
    CHECK(g.pot == 95, "pot 5 + 30*3 = 95, got %u", g.pot);

    CHECK(game_advance_street(&g) == 0, "flop dealt");
    CHECK(g.board_count == 3, "3 board cards");
    CHECK(g.to_act == 2, "first active after dealer acts, got %u", g.to_act);
    CHECK(game_legal_check(&g), "check legal on fresh street");
    game_apply_action(&g, ACT_CHECK, 0);
    game_apply_action(&g, ACT_CHECK, 0);
    game_apply_action(&g, ACT_CHECK, 0);
    CHECK(game_betting_done(&g), "flop checked through");

    game_advance_street(&g);                        /* turn */
    game_apply_action(&g, ACT_CHECK, 0);
    game_apply_action(&g, ACT_CHECK, 0);
    game_apply_action(&g, ACT_CHECK, 0);
    game_advance_street(&g);                        /* river */
    game_apply_action(&g, ACT_CHECK, 0);
    game_apply_action(&g, ACT_CHECK, 0);
    game_apply_action(&g, ACT_CHECK, 0);
    CHECK(game_advance_street(&g) == 1, "river closes to showdown");

    game_showdown(&g);
    CHECK(total_chips(&g) == 4000, "chips conserved, got %u", total_chips(&g));
}

static void test_fold_around(void)
{
    GameState g;
    game_init(&g, 1000);
    rand_init(7);
    game_start_hand(&g);
    game_apply_action(&g, ACT_FOLD, 0);
    game_apply_action(&g, ACT_FOLD, 0);
    game_apply_action(&g, ACT_FOLD, 0);
    CHECK(game_betting_done(&g), "everyone folded to BB");
    CHECK(game_advance_street(&g) == 1, "hand ends, no more streets");
    game_showdown(&g);
    CHECK(g.players[2].stack == 1005, "BB wins blinds, got %u", g.players[2].stack);
    CHECK(total_chips(&g) == 4000, "chips conserved");
}

/* random-action fuzz: chips are conserved, every hand terminates, and
   stacks never go negative (they're unsigned: catch via total) */
static void test_engine_fuzz(void)
{
    GameState g;
    long hand;
    int guard;
    rand_init(777);
    game_init(&g, 1000);

    for (hand = 0; hand < 20000; hand++) {
        if (game_alive_count(&g) < 2) {
            game_init(&g, 1000);   /* fresh table when someone wins it all */
        }
        game_start_hand(&g);
        for (;;) {
            guard = 0;
            while (!game_betting_done(&g)) {
                uint8_t roll = rand8();
                if (roll < 60) game_apply_action(&g, ACT_FOLD, 0);
                else if (roll < 170) {
                    if (game_legal_check(&g)) game_apply_action(&g, ACT_CHECK, 0);
                    else game_apply_action(&g, ACT_CALL, 0);
                } else {
                    uint16_t amt = (uint16_t)(g.cur_bet + g.min_raise + (rand8() & 0x3F));
                    game_apply_action(&g, ACT_RAISE, amt);
                }
                if (++guard > 200) break;
            }
            CHECK(guard <= 200, "betting round terminates (hand %ld)", hand);
            if (guard > 200) return;
            if (game_advance_street(&g)) break;
        }
        game_showdown(&g);
        CHECK(total_chips(&g) == 4000, "chips conserved (hand %ld): %u",
              hand, total_chips(&g));
        if (total_chips(&g) != 4000) return;
    }
    printf("engine fuzz: 20000 hands, chips conserved\n");
}

/* AI smoke: runs hands with all-AI decisions, must terminate and conserve */
static void test_ai_smoke(void)
{
    GameState g;
    long hand;
    rand_init(31337);
    ai_reset_memory();
    game_init(&g, 1000);

    for (hand = 0; hand < 2000; hand++) {
        if (game_alive_count(&g) < 2) game_init(&g, 1000);
        game_start_hand(&g);
        for (;;) {
            int guard = 0;
            while (!game_betting_done(&g)) {
                uint16_t raise_to = 0;
                uint8_t act = ai_decide(&g, &raise_to);
                game_apply_action(&g, act, raise_to);
                if (++guard > 200) break;
            }
            CHECK(guard <= 200, "AI betting terminates (hand %ld)", hand);
            if (guard > 200) return;
            if (game_advance_street(&g)) break;
        }
        game_showdown(&g);
        CHECK(total_chips(&g) == 4000, "AI chips conserved (hand %ld): %u",
              hand, total_chips(&g));
        if (total_chips(&g) != 4000) return;
    }
    printf("ai smoke: 2000 all-AI hands OK\n");
}

static void test_preflop_ordering(void)
{
    CHECK(ai_preflop_score(C("Ah"), C("Ad")) > ai_preflop_score(C("Kh"), C("Kd")),
          "AA > KK");
    CHECK(ai_preflop_score(C("8h"), C("8d")) > ai_preflop_score(C("Ah"), C("Kd")),
          "88 > AKo (pairs premium)");
    CHECK(ai_preflop_score(C("Ah"), C("Kd")) > ai_preflop_score(C("2h"), C("2d")),
          "AKo > 22 (Chen ordering)");
    CHECK(ai_preflop_score(C("Ah"), C("Kh")) > ai_preflop_score(C("Ah"), C("Kd")),
          "AKs > AKo");
    CHECK(ai_preflop_score(C("Ah"), C("Kd")) > ai_preflop_score(C("7h"), C("2d")),
          "AKo > 72o");
}

int main(void)
{
    test_categories();
    test_kickers();
    test_crosscheck();
    test_sidepots_three_allins();
    test_sidepots_split_odd_chip();
    test_sidepots_two_allins();
    test_hand_flow_scripted();
    test_fold_around();
    test_engine_fuzz();
    test_ai_smoke();
    test_preflop_ordering();

    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
