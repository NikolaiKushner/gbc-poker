#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "game.h"

void ui_init(void);
void ui_title(void);   /* title screen; seeds RNG from DIV_REG on keypress */
void ui_draw_table(const GameState *g, uint8_t hand_no);
void ui_flash_action(const GameState *g, uint8_t seat, uint8_t action, uint16_t paid);
uint8_t ui_prompt_action(const GameState *g, uint16_t *raise_to);
void ui_show_showdown(const GameState *g);
void ui_game_over(uint8_t place);
void ui_victory(void);

#endif
