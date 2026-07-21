#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

typedef struct {
    uint16_t magic;
    uint16_t hands_played;
    uint16_t tourneys_played;
    uint16_t championships;
    uint16_t best_category;   /* highest HAND_* ever made at showdown */
} SaveData;

void save_load(SaveData *out);       /* loads or initializes SRAM */
void save_store(const SaveData *in); /* commits to SRAM */

#endif
