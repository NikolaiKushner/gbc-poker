#include "rand.h"

/* 16-bit xorshift; period 65535, state never 0 */
static uint16_t state = 0xACE1;

void rand_init(uint16_t seed)
{
    state = seed ? seed : 0xACE1;
}

void rand_mix(uint8_t entropy)
{
    state ^= (uint16_t)entropy << 3;
    if (!state) state = 0xACE1;
    (void)rand16();
}

uint16_t rand16(void)
{
    uint16_t x = state;
    x ^= x << 7;
    x ^= x >> 9;
    x ^= x << 8;
    state = x;
    return x;
}

uint8_t rand8(void)
{
    return (uint8_t)(rand16() >> 8);
}

uint8_t rand_below(uint8_t n)
{
    uint8_t mask = (uint8_t)(n - 1);
    uint8_t r;
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    do {
        r = rand8() & mask;
    } while (r >= n);
    return r;
}
