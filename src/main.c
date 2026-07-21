#include <gb/gb.h>
#include "game.h"
#include "ai.h"
#include "rand.h"
#include "eval.h"
#include "ui.h"
#include "save.h"
#include "sound.h"

#define START_STACK 1000
#define HANDS_PER_BLIND_LEVEL 8

static GameState g;
static SaveData stats;
static uint8_t quit_to_menu;

/* short burst of card-deal ticks */
static void deal_fx(uint8_t n)
{
    while (n--) { sfx_deal(); delay(70); }
}

/* Snapshot the tournament at a between-hands boundary. Chips still committed
   to the current hand are returned to their owners so the resume replays a
   fresh hand with correct (pre-hand) stacks. */
static void save_current_game(uint8_t hand_no)
{
    SavedGame sv;
    uint8_t i;
    sv.hand_no = hand_no;
    sv.dealer = g.dealer;
    sv.bb_level = g.bb_level;
    for (i = 0; i < MAX_PLAYERS; i++) {
        sv.stacks[i] = g.players[i].stack + g.players[i].total_bet;
        sv.out[i] = (g.players[i].flags & PF_OUT) ? 1 : 0;
    }
    save_game_store(&sv);
}

/* one betting round; returns when closed or when quit_to_menu is set */
static void run_betting(uint8_t hand_no)
{
    while (!game_betting_done(&g)) {
        uint8_t seat = g.to_act;
        uint8_t action;
        uint16_t raise_to = 0;
        uint16_t owed = game_to_call(&g);

        ui_draw_table(&g, hand_no);
        if (g.players[seat].persona == 0) {
            do {
                action = ui_prompt_action(&g, &raise_to);
                if (action == ACT_MENU) {
                    uint8_t m = ui_ingame_menu();
                    if (m == IGM_SAVE) {
                        save_current_game(hand_no);
                        ui_toast("GAME SAVED");
                    } else if (m == IGM_QUIT) {
                        quit_to_menu = 1;
                        return;
                    }
                    ui_draw_table(&g, hand_no);
                }
            } while (action == ACT_MENU);
            if (action == ACT_FOLD && owed > 0)
                ai_note_fold_to_raise(0);
        } else {
            action = ai_decide(&g, &raise_to);
        }
        game_apply_action(&g, action, raise_to);
        ui_flash_action(&g, seat, action, owed);
    }
}

/* Play a tournament. resume != 0 restores the stored SavedGame. */
static void run_tournament(uint8_t resume)
{
    uint8_t hand_no = 0;

    game_init(&g, START_STACK);
    ai_reset_memory();
    quit_to_menu = 0;

    if (resume) {
        SavedGame sv;
        uint8_t i;
        save_game_load(&sv);
        hand_no = sv.hand_no;
        g.dealer = sv.dealer;
        for (i = 0; i < MAX_PLAYERS; i++) {
            g.players[i].stack = sv.stacks[i];
            g.players[i].flags = sv.out[i] ? PF_OUT : 0;
        }
    }

    for (;;) {
        g.bb_level = hand_no >> 3;   /* HANDS_PER_BLIND_LEVEL = 8 */
        if (g.bb_level >= NUM_BLIND_LEVELS) g.bb_level = NUM_BLIND_LEVELS - 1;

        game_start_hand(&g);
        ui_draw_table(&g, hand_no);
        deal_fx(3);                 /* hole cards dealt */

        for (;;) {
            run_betting(hand_no);
            if (quit_to_menu) return;
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
            save_game_clear();
            ui_game_over((uint8_t)(game_alive_count(&g) + 1));
            return;
        }
        if (game_alive_count(&g) < 2) {
            save_game_clear();
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
    ui_title();
    for (;;) {
        uint8_t choice = ui_main_menu(save_game_exists());
        if (choice == MM_NEW) {
            save_game_clear();
            stats.tourneys_played++;
            save_store(&stats);
            run_tournament(0);
        } else if (choice == MM_LOAD) {
            run_tournament(1);
        } else {
            ui_show_stats(&stats);
        }
    }
}
