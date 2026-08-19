#ifndef HUD_H
#define HUD_H

#include <nds.h>
#include "game.h"

// Sets up the bottom screen: a hand-drawn gradient/panel bitmap background
// (BG3) with the libnds text console (BG0) layered on top of it, plus a
// row of heart sprites (on the independent sub-screen OAM engine) used to
// show the player's remaining lives as icons instead of a plain number.
void hud_init(void);

// Draws/updates the bottom screen for the given game state. Cheap to call
// every frame: static labels/borders are only redrawn when the state
// actually changes, only the live numbers are refreshed every call.
// `rank` is only meaningful for STATE_GAMEOVER (0 = did not make the board).
void hud_render(GameState state, int score, int lives, int level,
                 int starsCaught, int starsNeeded, int rank);

// Lets other bottom-screen UI (the initials keyboard) reselect the same
// console hud_init() set up, since consoleSelect() is a single global switch
// shared with the top screen's console (see topscreen.c).
PrintConsole* hud_getConsole(void);

#endif // HUD_H
