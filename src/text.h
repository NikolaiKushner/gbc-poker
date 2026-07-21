#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>

/* "As" "Td" ... into out[2] (rank char + suit char) */
void card_str(uint8_t card, char *out);

extern const char *const PLAYER_NAMES[4];

/* persona chatter */
#define LINE_RAISE 0   /* value raise */
#define LINE_BLUFF 1   /* bluff raise */
#define LINE_WIN   2   /* won the pot at showdown */

/* a short line for an AI persona (1..3) and event; "" for the human. */
const char *persona_line(uint8_t persona, uint8_t event);

#endif
