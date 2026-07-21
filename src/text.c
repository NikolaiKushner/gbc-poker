#include "text.h"
#include "cards.h"

const char *const PLAYER_NAMES[4] = { "You ", "Prof", "Cowb", "Shrk" };

static const char RANK_CHARS[15] = {
    '?', '?', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
};
static const char SUIT_CHARS[4] = { 's', 'h', 'd', 'c' };

void card_str(uint8_t card, char *out)
{
    out[0] = RANK_CHARS[RANK(card)];
    out[1] = SUIT_CHARS[SUIT(card)];
}
