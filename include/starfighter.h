#ifndef STARFIGHTER_H
#define STARFIGHTER_H

#include <nds.h>

enum {
    STARFIGHTER_STATUS_NOT_INITED = -1,
    STARFIGHTER_STATUS_OK = 0,
    STARFIGHTER_STATUS_NO_PALETTE,    // nitro:/starfighter.pal failed to open
    STARFIGHTER_STATUS_SHORT_PALETTE, // opened but read fewer than 128 entries
    STARFIGHTER_STATUS_NO_RAW,        // nitro:/starfighter.raw failed to open
    STARFIGHTER_STATUS_SHORT_RAW,     // opened but first frame read short
};

typedef enum {
    STARFIGHTER_HIDDEN,     // scoreboard/keyboard - not shown at all
    STARFIGHTER_BACKGROUND, // small corner decal during actual gameplay
    STARFIGHTER_SHOWCASE,   // large, centered - the title screen's main visual
} StarfighterMode;

// Loads the pre-rendered turntable animation (a 3D model rotated on its
// own axis, rendered offline in Blender and streamed as a 2D bitmap
// sequence - see README.md for why: the NDS only has one 3D-capable
// engine and it's already committed to the top screen's 2D gameplay
// sprites) and sets up its own dedicated bitmap layer (BG2) on the top
// screen's Main engine, reusing the exact same VRAM bank/offset the boot
// intro (intro.c) already proved out - that layer sits unused once the
// intro finishes and hides it. Call once, after intro_play().
void starfighter_init(void);

// Advances the animation by one game frame. Call once per frame while the
// viewport should be visible (only during actual gameplay).
void starfighter_update(void);

// Switches between hidden / small background-decal / large title-screen
// showcase, redrawing the current animation frame at the right size and
// position for whichever mode is now active. Cheap to call every frame
// regardless of state - only touches VRAM when the mode actually changes.
void starfighter_setMode(StarfighterMode mode);

// Diagnostic: which STARFIGHTER_STATUS_* the last init landed on.
int starfighter_status(void);

#endif // STARFIGHTER_H
