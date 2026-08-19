#ifndef GAME_H
#define GAME_H

#include <nds.h>

// SCREEN_WIDTH / SCREEN_HEIGHT (256x192) already come from nds.h

#define SPRITE_SIZE      32 // player ship / entities: sourced from graphics/spritesheet.png
#define PROJECTILE_SIZE  16 // laser bolt: still procedural, not part of the sheet
#define INITIALS_LEN     3

typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_SHIP_EXPLODING,
    STATE_LEVEL_COMPLETE,
    STATE_GAMEOVER,
    STATE_ENTER_INITIALS,
    STATE_LEADERBOARD
} GameState;

#endif // GAME_H
