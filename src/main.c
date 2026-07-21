#include <gb/gb.h>
#include "game.h"
#include "ai.h"
#include "rand.h"
#include "eval.h"
#include "ui.h"
#include "save.h"
#include "sound.h"

/* short burst of card-deal ticks */
static void deal_fx(uint8_t n)
{
    while (n--) { sfx_deal(); delay(70); }
}

#define START_STACK 1000
#define HANDS_PER_BLIND_LEVEL 8

static GameState g;
static SaveData stats;

/* one betting round; returns nothing, acts until closed */
static void run_betting(uint8_t hand_no)
{
    while (!game_betting_done(&g)) {
        uint8_t seat = g.to_act;
        uint8_t action;
        uint16_t raise_to = 0;
        uint16_t owed = game_to_call(&g);

        ui_draw_table(&g, hand_no);
        if (g.players[seat].persona == 0) {
            action = ui_prompt_action(&g, &raise_to);
            if (action == ACT_FOLD && owed > 0)
                ai_note_fold_to_raise(0);
        } else {
            action = ai_decide(&g, &raise_to);
        }
        game_apply_action(&g, action, raise_to);
        ui_flash_action(&g, seat, action, owed);
    }
}

static void run_tournament(void)
{
    uint8_t hand_no = 0;

    game_init(&g, START_STACK);
    ai_reset_memory();

    for (;;) {
        g.bb_level = hand_no >> 3;   /* HANDS_PER_BLIND_LEVEL = 8 */
        if (g.bb_level >= NUM_BLIND_LEVELS) g.bb_level = NUM_BLIND_LEVELS - 1;

        game_start_hand(&g);
        ui_draw_table(&g, hand_no);
        deal_fx(3);                 /* hole cards dealt */

        for (;;) {
            run_betting(hand_no);
            if (game_advance_street(&g)) break;
            ui_draw_table(&g, hand_no);
            deal_fx(1);             /* community card(s) dealt */
        }
        game_showdown(&g);
        ui_draw_table(&g, hand_no);
        ui_show_showdown(&g);

        stats.hands_played++;
        if (g.shown[0] && g.won[0] &&
            EVAL_CATEGORY(g.shown[0]) > stats.best_category)
            stats.best_category = EVAL_CATEGORY(g.shown[0]);
        save_store(&stats);

        hand_no++;

        if (g.players[0].flags & PF_OUT) {
            ui_game_over((uint8_t)(game_alive_count(&g) + 1));
            return;
        }
        if (game_alive_count(&g) < 2) {
            stats.championships++;
            save_store(&stats);
            ui_victory();
            return;
        }
    }
}

void main(void)
{
    ui_init();
    save_load(&stats);
    for (;;) {
        ui_title();
        stats.tourneys_played++;
        save_store(&stats);
        run_tournament();
    }
}
