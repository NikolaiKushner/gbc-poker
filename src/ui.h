#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "game.h"
#include "save.h"

/* ui_prompt_action returns this when the player opens the in-game menu */
#define ACT_MENU 0xFE

/* main-menu results */
#define MM_NEW   0
#define MM_LOAD  1
#define MM_STATS 2

/* in-game menu results */
#define IGM_CONTINUE 0
#define IGM_SAVE     1
#define IGM_QUIT     2

void ui_init(void);
void ui_title(void);                /* boot splash; seeds RNG on START */
uint8_t ui_main_menu(uint8_t can_load);   /* -> MM_* */
uint8_t ui_ingame_menu(void);             /* -> IGM_* */
void ui_toast(const char *msg);     /* brief centered message */
void ui_draw_table(const GameState *g, uint8_t hand_no);
void ui_flash_action(const GameState *g, uint8_t seat, uint8_t action, uint16_t paid);
uint8_t ui_prompt_action(const GameState *g, uint16_t *raise_to);
void ui_show_showdown(const GameState *g);
void ui_show_stats(const SaveData *s);
void ui_game_over(uint8_t place);
void ui_victory(void);

#endif
