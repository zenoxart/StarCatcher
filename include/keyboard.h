#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <nds.h>
#include "game.h"

// Draws the on-screen QWERTY keyboard plus the current initials preview to
// the bottom screen. Reuses whichever console hud_init() already selected,
// so call this only while STATE_ENTER_INITIALS is active.
void keyboard_draw(const char initials[INITIALS_LEN], int filledCount);

// Reads touch and button input for the current frame (scanKeys() must
// already have been called by the caller this frame) and updates the
// initials buffer accordingly:
//   - tapping a letter appends it (while filledCount < INITIALS_LEN)
//   - tapping [DEL] or pressing B removes the last letter
//   - tapping [OK] or pressing START sets *confirmed = true, but only once
//     exactly INITIALS_LEN letters have been entered
void keyboard_update(char initials[INITIALS_LEN], int* filledCount, bool* confirmed);

#endif // KEYBOARD_H
