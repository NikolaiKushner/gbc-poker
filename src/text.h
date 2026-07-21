#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>

/* "As" "Td" ... into out[2] (rank char + suit char) */
void card_str(uint8_t card, char *out);

extern const char *const PLAYER_NAMES[4];

#endif
