#include "ai.h"
#include "eval.h"
#include "rand.h"

/* {aggression, looseness, bluff_freq} */
const AiProfile AI_PROFILES[4] = {
    {128, 128, 40},   /* 0: unused (human seat) */
    {60,  70,  15},   /* Professor: tight-passive */
    {200, 190, 90},   /* Cowboy: loose-aggressive */
    {150, 130, 55},   /* Shark: balanced */
};

/* one counter per seat: how often this player folds when raised */
static uint8_t fold_to_raise[MAX_PLAYERS];

void ai_reset_memory(void)
{
    uint8_t i;
    for (i = 0; i < MAX_PLAYERS; i++) fold_to_raise[i] = 0;
}

void ai_note_fold_to_raise(uint8_t player)
{
    if (fold_to_raise[player] < 250) fold_to_raise[player] += 10;
}

/* Chen-like preflop strength, 0..255 (AA ~ 230, 72o ~ 35) */
uint8_t ai_preflop_score(uint8_t c1, uint8_t c2)
{
    uint8_t r1 = RANK(c1), r2 = RANK(c2), t, gap;
    uint8_t score;
    if (r2 > r1) { t = r1; r1 = r2; r2 = t; }

    if (r1 == r2) {
        score = (uint8_t)(146 + r1 * 6);          /* 22=158 .. AA=230 */
        return score;
    }
    score = (uint8_t)(r1 * 7 + r2 * 3);           /* AKo = 98+39 = 137 */
    if (SUIT(c1) == SUIT(c2)) score += 14;
    gap = (uint8_t)(r1 - r2);
    if (gap == 1) score += 10;
    else if (gap == 2) score += 5;
    if (r2 >= 10) score += 12;                    /* two broadway cards */
    return score;
}

/* Detect draws from known cards; returns bonus 0..40 */
static uint8_t draw_bonus(const uint8_t *cards, uint8_t n)
{
    uint8_t suit_cnt[4] = {0, 0, 0, 0};
    uint16_t mask = 0;
    uint8_t i, r, bonus = 0;

    for (i = 0; i < n; i++) {
        suit_cnt[SUIT(cards[i])]++;
        mask |= (uint16_t)1 << RANK(cards[i]);
    }
    for (i = 0; i < 4; i++)
        if (suit_cnt[i] == 4) { bonus += 25; break; }   /* flush draw */
    if (mask & (1u << RANK_ACE)) mask |= (1u << 1);
    /* open-ended-ish: any window of 5 with exactly 4 ranks present */
    for (r = 1; r <= 10; r++) {
        uint16_t w = (uint16_t)((mask >> r) & 0x1F);
        /* popcount of 5 bits */
        uint8_t pc = (uint8_t)((w & 1) + ((w >> 1) & 1) + ((w >> 2) & 1)
                             + ((w >> 3) & 1) + ((w >> 4) & 1));
        if (pc == 4) { bonus += 15; break; }            /* straight draw */
    }
    return bonus;
}

/* Made-hand strength from current known cards, 0..255 */
uint8_t ai_postflop_score(const GameState *g, uint8_t idx)
{
    static const uint8_t cat_base[9] = {
        10,   /* high card */
        70,   /* pair */
        120,  /* two pair */
        150,  /* trips */
        180,  /* straight */
        200,  /* flush */
        220,  /* full house */
        240,  /* quads */
        250,  /* straight flush */
    };
    uint8_t cards[7];
    uint8_t i, n = 2, cat, score;
    uint32_t v;
    const Player *p = &g->players[idx];

    cards[0] = p->hole[0];
    cards[1] = p->hole[1];
    for (i = 0; i < g->board_count; i++) cards[n++] = g->board[i];
    /* pad to 7 by duplicating? no — eval7 needs exactly 7; on flop/turn
       evaluate the 5/6 known cards by filling with distinct dead low cards
       is wrong. Instead: category from partial evaluation trick — just
       run eval7 on river; earlier streets use a lighter path below. */
    if (g->board_count == 5) {
        v = eval7(cards);
        cat = EVAL_CATEGORY(v);
        score = cat_base[cat];
        /* nudge by top deciding rank so TP good kicker > TP weak kicker */
        score += (uint8_t)((v >> 16) & 0x0F);
        return score;
    }

    /* flop/turn: histogram over the known 5-6 cards */
    {
        uint8_t rank_cnt[15];
        uint8_t suit_cnt2[4] = {0, 0, 0, 0};
        uint16_t mask = 0;
        uint8_t pairs = 0, trips = 0, quads = 0, hi_pair = 0, flush = 0;
        uint8_t r, straight;

        for (i = 0; i < 15; i++) rank_cnt[i] = 0;
        for (i = 0; i < n; i++) {
            r = RANK(cards[i]);
            rank_cnt[r]++;
            suit_cnt2[SUIT(cards[i])]++;
            mask |= (uint16_t)1 << r;
        }
        for (r = RANK_ACE; r >= RANK_MIN; r--) {
            if (rank_cnt[r] == 4) quads = r;
            else if (rank_cnt[r] == 3) trips = r;
            else if (rank_cnt[r] == 2) { pairs++; if (!hi_pair) hi_pair = r; }
        }
        for (i = 0; i < 4; i++) if (suit_cnt2[i] >= 5) flush = 1;
        if (mask & (1u << RANK_ACE)) mask |= (1u << 1);
        straight = 0;
        for (r = RANK_ACE; r >= 5; r--) {
            uint16_t need = (uint16_t)(0x1Fu << (r - 4));
            if ((mask & need) == need) { straight = 1; break; }
        }

        if (quads) score = cat_base[HAND_QUADS];
        else if (trips && pairs) score = cat_base[HAND_FULL_HOUSE];
        else if (flush) score = cat_base[HAND_FLUSH];
        else if (straight) score = cat_base[HAND_STRAIGHT];
        else if (trips) score = cat_base[HAND_TRIPS];
        else if (pairs >= 2) score = cat_base[HAND_TWO_PAIR];
        else if (pairs) {
            score = cat_base[HAND_PAIR];
            /* pair made with a hole card is worth more than a board pair */
            if (rank_cnt[RANK(p->hole[0])] >= 2 || rank_cnt[RANK(p->hole[1])] >= 2)
                score += hi_pair;                /* top pair high rank bonus */
        } else {
            score = cat_base[HAND_HIGH];
            if (RANK(p->hole[0]) == RANK_ACE || RANK(p->hole[1]) == RANK_ACE)
                score += 15;
        }
        if (g->board_count == 3) score += draw_bonus(cards, n);
        else score += draw_bonus(cards, n) >> 1;   /* one card to come */
        return score;
    }
}

/* pot-fraction raise sizing without division: pot/2 + call, snapped up to min */
static uint16_t size_raise(const GameState *g, uint8_t aggression)
{
    uint16_t target = g->cur_bet + g->min_raise + (g->pot >> 1);
    if (aggression > 170) target += g->pot >> 2;
    return target;
}

uint8_t ai_decide(const GameState *g, uint16_t *raise_to)
{
    uint8_t idx = g->to_act;
    const Player *p = &g->players[idx];
    const AiProfile *prof = &AI_PROFILES[p->persona];
    uint16_t to_call = game_to_call(g);
    uint8_t score;
    uint8_t call_thresh, raise_thresh;

    if (g->street == ST_PREFLOP)
        score = ai_preflop_score(p->hole[0], p->hole[1]);
    else
        score = ai_postflop_score(g, idx);

    /* thresholds shift with personality: loose lowers the bar to play,
       aggressive lowers the bar to raise */
    call_thresh  = (uint8_t)(120 - (prof->looseness >> 2));    /* 72..119 */
    raise_thresh = (uint8_t)(205 - (prof->aggression >> 2));   /* 142..204 */

    /* facing a big bet (relative to stack) demands more strength */
    if (to_call > (p->stack >> 2)) call_thresh += 25;
    if (to_call > (p->stack >> 1)) call_thresh += 25;

    /* exploit: if the target folds a lot, raise thinner */
    if (fold_to_raise[0] > 60) raise_thresh -= 15;

    if (score >= raise_thresh) {
        *raise_to = size_raise(g, prof->aggression);
        return ACT_RAISE;
    }

    /* bluff: late street, at most one bet to call, few opponents */
    if (g->street >= ST_TURN && game_players_left(g) <= 2 &&
        to_call == 0 && rand8() < prof->bluff_freq) {
        *raise_to = size_raise(g, prof->aggression);
        return ACT_RAISE;
    }

    if (to_call == 0) return ACT_CHECK;
    if (score >= call_thresh) return ACT_CALL;

    /* very cheap calls with any playable hand (pot odds, crudely) */
    if (to_call <= (g->pot >> 3) && score >= (uint8_t)(call_thresh - 20))
        return ACT_CALL;

    return ACT_FOLD;
}
