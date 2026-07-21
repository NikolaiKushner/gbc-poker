#include <gb/gb.h>
#include "save.h"

#define SAVE_MAGIC 0x504B   /* "PK" */
#define SRAM_SAVE ((SaveData *)0xA000)

void save_load(SaveData *out)
{
    ENABLE_RAM;
    SWITCH_RAM(0);
    if (SRAM_SAVE->magic != SAVE_MAGIC) {
        SRAM_SAVE->magic = SAVE_MAGIC;
        SRAM_SAVE->hands_played = 0;
        SRAM_SAVE->tourneys_played = 0;
        SRAM_SAVE->championships = 0;
        SRAM_SAVE->best_category = 0;
    }
    *out = *SRAM_SAVE;
    DISABLE_RAM;
}

void save_store(const SaveData *in)
{
    ENABLE_RAM;
    SWITCH_RAM(0);
    *SRAM_SAVE = *in;
    DISABLE_RAM;
}
