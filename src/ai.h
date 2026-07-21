#ifndef AI_H
#define AI_H

#include <stdint.h>
#include "game.h"

/* Personas (index = Player.persona - 1 for AI seats) */
#define PERSONA_PROFESSOR 1   /* tight-passive */
#define PERSONA_COWBOY    2   /* loose-aggressive */
#define PERSONA_SHARK     3   /* balanced */

typedef struct {
    uint8_t aggression;   /* raise eagerness 0..255 */
    uint8_t looseness;    /* how weak a hand it will play 0..255 */
    uint8_t bluff_freq;   /* chance per late street to bluff 0..255 */
} AiProfile;

extern const AiProfile AI_PROFILES[4];

extern uint8_t ai_last_bluff;   /* 1 if ai_decide's last ACT_RAISE was a bluff */

void ai_reset_memory(void);
void ai_note_fold_to_raise(uint8_t player);  /* opponent modelling: one counter */

/* Decide for g->to_act. Returns ACT_*; for ACT_RAISE, *raise_to
   receives the target total street bet. */
uint8_t ai_decide(const GameState *g, uint16_t *raise_to);

/* Exposed for tests/tuning */
uint8_t ai_preflop_score(uint8_t c1, uint8_t c2);
uint8_t ai_postflop_score(const GameState *g, uint8_t idx);

#endif
