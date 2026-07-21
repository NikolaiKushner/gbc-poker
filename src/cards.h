#ifndef CARDS_H
#define CARDS_H

#include <stdint.h>

/* card = suit*16 + rank; suit 0..3 in high nibble, rank 2..14 in low nibble */
#define RANK(c)         ((uint8_t)((c) & 0x0F))
#define SUIT(c)         ((uint8_t)((c) >> 4))
#define MAKE_CARD(s, r) ((uint8_t)(((s) << 4) | (r)))
#define NO_CARD         0

#define SUIT_SPADES   0
#define SUIT_HEARTS   1
#define SUIT_DIAMONDS 2
#define SUIT_CLUBS    3

#define RANK_MIN   2
#define RANK_JACK  11
#define RANK_QUEEN 12
#define RANK_KING  13
#define RANK_ACE   14

#define DECK_SIZE 52

#endif
