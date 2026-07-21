#include "text.h"
#include "cards.h"
#include "rand.h"

const char *const PLAYER_NAMES[4] = { "You ", "Prof", "Cowb", "Shrk" };

/* [event][option] - keep <=18 chars, only glyphs in the card font
   (uppercase, digits, and  ! $ % / : - . > < = + , * ( ) ) */
static const char *const L_PROF[3][3] = {
    { "CALCULATED.",     "TEXTBOOK PLAY.",   "THE MATH IS CLEAR." },
    { "A LOGICAL RISK.", "PROBABILITIES...", "TRUST THE MODEL." },
    { "AS PREDICTED.",   "ELEMENTARY.",      "Q.E.D." },
};
static const char *const L_COWB[3][3] = {
    { "YEEHAW!",         "SADDLE UP!",       "RIDE OR FOLD!" },
    { "FEELIN LUCKY!",   "ALL HAT NO CARDS", "CATCH ME IF YA CAN" },
    { "GIDDYUP! MINE!",  "YEEHAW!",          "DINNER IS SERVED" },
};
static const char *const L_SHRK[3][3] = {
    { "BITE.",           "I SMELL BLOOD.",   "CIRCLING NOW." },
    { "FEARLESS.",       "FEEL THE PRESSURE","NO MERCY." },
    { "CHOMP.",          "EASY MEAT.",       "SWIM AWAY." },
};

const char *persona_line(uint8_t persona, uint8_t event)
{
    uint8_t pick;
    if (persona < 1 || persona > 3 || event > LINE_WIN) return "";
    pick = rand8() % 3;
    switch (persona) {
    case 1: return L_PROF[event][pick];
    case 2: return L_COWB[event][pick];
    default: return L_SHRK[event][pick];
    }
}

static const char RANK_CHARS[15] = {
    '?', '?', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
};
static const char SUIT_CHARS[4] = { 's', 'h', 'd', 'c' };

void card_str(uint8_t card, char *out)
{
    out[0] = RANK_CHARS[RANK(card)];
    out[1] = SUIT_CHARS[SUIT(card)];
}
