#include "deck.h"
#include "rand.h"

void deck_init(Deck *d)
{
    uint8_t s, r, i = 0;
    for (s = 0; s < 4; s++)
        for (r = RANK_MIN; r <= RANK_ACE; r++)
            d->cards[i++] = MAKE_CARD(s, r);
    d->pos = 0;
}

void deck_shuffle(Deck *d)
{
    uint8_t i, j, tmp;
    for (i = DECK_SIZE - 1; i > 0; i--) {
        j = rand_below(i + 1);
        tmp = d->cards[i];
        d->cards[i] = d->cards[j];
        d->cards[j] = tmp;
    }
    d->pos = 0;
}

uint8_t deck_deal(Deck *d)
{
    return d->cards[d->pos++];
}
