#ifndef RAND_H
#define RAND_H

#include <stdint.h>

void     rand_init(uint16_t seed);
void     rand_mix(uint8_t entropy);   /* stir in a byte (e.g. DIV_REG on keypress) */
uint16_t rand16(void);
uint8_t  rand8(void);
uint8_t  rand_below(uint8_t n);       /* uniform in [0, n), n >= 1, no division */

#endif
