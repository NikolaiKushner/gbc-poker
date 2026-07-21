#include <gb/gb.h>
#include "save.h"

#define SAVE_MAGIC 0x504B   /* "PK" */
#define GAME_MAGIC 0x4748   /* "GH" */

#define SRAM_SAVE ((SaveData *)0xA000)
#define SRAM_GAME ((SavedGame *)0xA020)

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

uint8_t save_game_exists(void)
{
    uint8_t ok;
    ENABLE_RAM;
    SWITCH_RAM(0);
    ok = (SRAM_GAME->magic == GAME_MAGIC && SRAM_GAME->valid);
    DISABLE_RAM;
    return ok;
}

void save_game_store(const SavedGame *in)
{
    ENABLE_RAM;
    SWITCH_RAM(0);
    *SRAM_GAME = *in;
    SRAM_GAME->magic = GAME_MAGIC;
    SRAM_GAME->valid = 1;
    DISABLE_RAM;
}

void save_game_load(SavedGame *out)
{
    ENABLE_RAM;
    SWITCH_RAM(0);
    *out = *SRAM_GAME;
    DISABLE_RAM;
}

void save_game_clear(void)
{
    ENABLE_RAM;
    SWITCH_RAM(0);
    SRAM_GAME->valid = 0;
    DISABLE_RAM;
}
