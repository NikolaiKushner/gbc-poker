#ifndef EVAL_H
#define EVAL_H

#include <stdint.h>

/* Hand categories, stronger = larger */
#define HAND_HIGH       0
#define HAND_PAIR       1
#define HAND_TWO_PAIR   2
#define HAND_TRIPS      3
#define HAND_STRAIGHT   4
#define HAND_FLUSH      5
#define HAND_FULL_HOUSE 6
#define HAND_QUADS      7
#define HAND_STR_FLUSH  8

#define EVAL_CATEGORY(v) ((uint8_t)((v) >> 20))

/* 7 cards -> comparable strength of the best 5-card hand.
   Packed as (category << 20) | k1<<16 | k2<<12 | k3<<8 | k4<<4 | k5,
   where k1..k5 are the deciding ranks, high to low (unused = 0).
   Larger value = stronger hand; equal value = exact chop. */
uint32_t eval7(const uint8_t *cards);

#endif
