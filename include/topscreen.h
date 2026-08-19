#ifndef TOPSCREEN_H
#define TOPSCREEN_H

#include <nds.h>
#include "game.h"

// Sets up a text console on the top screen (BG0 on the main engine, its own
// VRAM bank so it never competes with the gameplay sprites' VRAM). Left
// blank during normal gameplay; only written to for the initials-entry and
// leaderboard screens.
void topscreen_init(void);

void topscreen_clear(void);

void topscreen_showEnterInitials(int score, int level,
                                   const char initials[INITIALS_LEN], int filledCount);

// highlightRank: 1-based rank to mark with a "->" (e.g. the entry just
// submitted), or 0 to not highlight anything.
void topscreen_showLeaderboard(int highlightRank);

#endif // TOPSCREEN_H
