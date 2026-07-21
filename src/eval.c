#include "eval.h"
#include "cards.h"

/* Highest rank of a 5-in-a-row in the rank bitmask, 0 if none.
   Bit r set = rank r present (2..14). Wheel: ace also counts as bit 1. */
static uint8_t straight_high(uint16_t mask)
{
    uint8_t r;
    uint16_t need;
    if (mask & (1u << RANK_ACE)) mask |= (1u << 1);
    for (r = RANK_ACE; r >= 5; r--) {
        need = (uint16_t)(0x1Fu << (r - 4));
        if ((mask & need) == need) return r;
    }
    return 0;
}

#define PACK1(cat, k1) \
    (((uint32_t)(cat) << 20) | ((uint32_t)(k1) << 16))
#define PACK2(cat, k1, k2) \
    (PACK1(cat, k1) | ((uint32_t)(k2) << 12))

uint32_t eval7(const uint8_t *cards)
{
    uint8_t rank_cnt[15];
    uint8_t suit_cnt[4];
    uint16_t rank_mask = 0;
    uint8_t i, r, s;

    for (i = 0; i < 15; i++) rank_cnt[i] = 0;
    for (i = 0; i < 4; i++)  suit_cnt[i] = 0;

    for (i = 0; i < 7; i++) {
        r = RANK(cards[i]);
        s = SUIT(cards[i]);
        rank_cnt[r]++;
        suit_cnt[s]++;
        rank_mask |= (uint16_t)1 << r;
    }

    /* flush / straight flush (at most one suit can hold 5+ of 7 cards) */
    for (s = 0; s < 4; s++) {
        if (suit_cnt[s] >= 5) {
            uint16_t fmask = 0;
            uint32_t v;
            uint8_t cnt = 0, shift;
            for (i = 0; i < 7; i++)
                if (SUIT(cards[i]) == s)
                    fmask |= (uint16_t)1 << RANK(cards[i]);
            r = straight_high(fmask);
            if (r) return PACK1(HAND_STR_FLUSH, r);
            v = (uint32_t)HAND_FLUSH << 20;
            shift = 16;
            for (r = RANK_ACE; cnt < 5; r--)
                if (fmask & ((uint16_t)1 << r)) {
                    v |= (uint32_t)r << shift;
                    shift -= 4;
                    cnt++;
                }
            return v;
        }
    }

    {
        uint8_t quads = 0, trips = 0, pair_hi = 0, pair_lo = 0;
        for (r = RANK_ACE; r >= RANK_MIN; r--) {
            if (rank_cnt[r] == 4) quads = r;
            else if (rank_cnt[r] == 3) {
                if (!trips) trips = r;
                else if (!pair_hi) pair_hi = r;   /* second trips plays as a pair */
            } else if (rank_cnt[r] == 2) {
                if (!pair_hi) pair_hi = r;
                else if (!pair_lo) pair_lo = r;
            }
        }

        if (quads) {
            for (r = RANK_ACE; r >= RANK_MIN; r--)
                if (r != quads && rank_cnt[r]) break;
            return PACK2(HAND_QUADS, quads, r);
        }
        if (trips && pair_hi)
            return PACK2(HAND_FULL_HOUSE, trips, pair_hi);

        r = straight_high(rank_mask);
        if (r) return PACK1(HAND_STRAIGHT, r);

        if (trips) {
            uint32_t v = PACK1(HAND_TRIPS, trips);
            uint8_t cnt = 0, shift = 12;
            for (r = RANK_ACE; cnt < 2; r--)
                if (r != trips && rank_cnt[r]) {
                    v |= (uint32_t)r << shift;
                    shift -= 4;
                    cnt++;
                }
            return v;
        }
        if (pair_hi && pair_lo) {
            /* with three pairs the third pair's rank still competes as kicker */
            for (r = RANK_ACE; r >= RANK_MIN; r--)
                if (r != pair_hi && r != pair_lo && rank_cnt[r]) break;
            return PACK2(HAND_TWO_PAIR, pair_hi, pair_lo) | ((uint32_t)r << 8);
        }
        if (pair_hi) {
            uint32_t v = PACK1(HAND_PAIR, pair_hi);
            uint8_t cnt = 0, shift = 12;
            for (r = RANK_ACE; cnt < 3; r--)
                if (r != pair_hi && rank_cnt[r]) {
                    v |= (uint32_t)r << shift;
                    shift -= 4;
                    cnt++;
                }
            return v;
        }
        {
            uint32_t v = (uint32_t)HAND_HIGH << 20;
            uint8_t cnt = 0, shift = 16;
            for (r = RANK_ACE; cnt < 5; r--)
                if (rank_cnt[r]) {
                    v |= (uint32_t)r << shift;
                    shift -= 4;
                    cnt++;
                }
            return v;
        }
    }
}
