#ifndef DECK_H
#define DECK_H

#include <stdint.h>
#include "cards.h"

typedef struct {
    uint8_t cards[DECK_SIZE];
    uint8_t pos;            /* next card to deal */
} Deck;

void    deck_init(Deck *d);
void    deck_shuffle(Deck *d);          /* Fisher-Yates on rand_below() */
uint8_t deck_deal(Deck *d);

#endif
