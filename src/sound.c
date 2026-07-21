#include <gb/gb.h>
#include <gb/hardware.h>
#include "sound.h"

/* 11-bit period values for a few notes (reg = 2048 - 131072/freq).
   Larger value = higher pitch. */
#define N_C5 1797
#define N_E5 1849
#define N_G5 1881
#define N_A5 1902
#define N_C6 1923
#define N_E6 1961
#define N_G6 1975
#define N_G4 1714
#define N_E4 1650
#define N_C4 1547

void sound_init(void)
{
    NR52_REG = 0x80;   /* APU on */
    NR51_REG = 0xFF;   /* every channel to both terminals */
    NR50_REG = 0x77;   /* max master volume L/R */
}

/* Trigger channel 1 (square + envelope) at a period, with a volume envelope. */
static void ch1_note(uint16_t period, uint8_t env)
{
    NR10_REG = 0x00;                                   /* no sweep */
    NR11_REG = 0x80;                                   /* 50% duty */
    NR12_REG = env;                                    /* volume envelope */
    NR13_REG = (uint8_t)(period & 0xFF);
    NR14_REG = (uint8_t)(0x80 | ((period >> 8) & 0x07)); /* trigger */
}

/* Trigger channel 2 (square) - used for short blips. */
static void ch2_blip(uint16_t period, uint8_t env)
{
    NR21_REG = 0xC0;                                   /* 75% duty, short length */
    NR22_REG = env;
    NR23_REG = (uint8_t)(period & 0xFF);
    NR24_REG = (uint8_t)(0x80 | ((period >> 8) & 0x07));
}

void sfx_deal(void)
{
    NR41_REG = 0x30;        /* length */
    NR42_REG = 0xA2;        /* vol 10, decay */
    NR43_REG = 0x37;        /* mid-high noise pitch */
    NR44_REG = 0xC0;        /* trigger + length enable */
}

void sfx_fold(void)
{
    NR41_REG = 0x20;
    NR42_REG = 0xC3;
    NR43_REG = 0x55;        /* lower, softer shuffle */
    NR44_REG = 0xC0;
}

void sfx_chip(void)
{
    ch2_blip(N_G6, 0xA2);   /* bright short clink */
}

void sfx_move(void)
{
    ch2_blip(N_C5, 0x71);   /* quiet tick */
}

void jingle_win(void)
{
    ch1_note(N_C5, 0xF3); delay(90);
    ch1_note(N_E5, 0xF3); delay(90);
    ch1_note(N_G5, 0xF3); delay(90);
    ch1_note(N_C6, 0xF4); delay(220);
}

void jingle_lose(void)
{
    ch1_note(N_G4, 0xF3); delay(140);
    ch1_note(N_E4, 0xF3); delay(140);
    ch1_note(N_C4, 0xF5); delay(300);
}

void jingle_title(void)
{
    ch1_note(N_C5, 0xE2); delay(70);
    ch1_note(N_G5, 0xE2); delay(70);
    ch1_note(N_C6, 0xE4); delay(160);
}
