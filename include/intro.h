#ifndef INTRO_H
#define INTRO_H

#include <nds.h>

// Plays the boot intro (nitro:/intro.raw + intro.pal, a 256x192 8bpp
// bitmap "video" streamed frame-by-frame from NitroFS) on the top screen.
// Blocks until the clip finishes or the player skips it with
// START/A/B. Falls through immediately (no-op) if the intro assets aren't
// available, e.g. NitroFS failed to init.
//
// Must be called after videoSetMode(MODE_5_2D) and topscreen_init(), since
// it adds a bitmap background (BG2) on top of the same VRAM bank the top
// screen's text console (BG0) already uses.
void intro_play(void);

#endif // INTRO_H
