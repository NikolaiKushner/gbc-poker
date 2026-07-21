#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

#ifndef MAX_PLAYERS
#define MAX_PLAYERS 4
#endif

typedef struct {
    uint16_t magic;
    uint16_t hands_played;
    uint16_t tourneys_played;
    uint16_t championships;
    uint16_t best_category;   /* highest HAND_* ever made at showdown */
} SaveData;

/* Resumable tournament snapshot (taken at a between-hands boundary). */
typedef struct {
    uint16_t magic;
    uint8_t  valid;
    uint8_t  hand_no;
    uint8_t  dealer;
    uint8_t  bb_level;
    uint16_t stacks[MAX_PLAYERS];
    uint8_t  out[MAX_PLAYERS];   /* busted flags */
} SavedGame;

void save_load(SaveData *out);         /* loads or initializes stats in SRAM */
void save_store(const SaveData *in);   /* commits stats to SRAM */

uint8_t save_game_exists(void);        /* 1 if a resumable game is stored */
void    save_game_store(const SavedGame *in);
void    save_game_load(SavedGame *out);
void    save_game_clear(void);         /* invalidate the game slot */

#endif
