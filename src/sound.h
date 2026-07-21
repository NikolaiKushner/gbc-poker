#ifndef SOUND_H
#define SOUND_H

/* Tiny APU sound effects and jingles via direct register writes.
   ROM-only (depends on gbdk); not part of the native test build. */

void sound_init(void);

void sfx_deal(void);    /* card dealt      - noise tick (ch4) */
void sfx_chip(void);    /* chips to pot    - square blip (ch2) */
void sfx_move(void);    /* menu cursor     - short click (ch2) */
void sfx_fold(void);    /* cards mucked    - noise sweep (ch4) */

void jingle_win(void);   /* hand / tournament won  - rising arpeggio (ch1) */
void jingle_lose(void);  /* busted out             - falling notes (ch1) */
void jingle_title(void); /* title flourish */

#endif
