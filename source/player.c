#include "player.h"
#include "sprites.h"
#include "game.h"

void player_init(Player* p) {
    p->width  = SPRITE_SIZE;
    p->height = SPRITE_SIZE;
    p->x = (SCREEN_WIDTH - p->width) / 2;
    p->y = SCREEN_HEIGHT - p->height - 4;
    p->speed = 3;
}

void player_update(Player* p, int keysHeldMask) {
    if (keysHeldMask & KEY_LEFT)  p->x -= p->speed;
    if (keysHeldMask & KEY_RIGHT) p->x += p->speed;

    if (p->x < 0) p->x = 0;
    if (p->x > SCREEN_WIDTH - p->width) p->x = SCREEN_WIDTH - p->width;
}

void player_draw(const Player* p, int oamId, ShipFrame frame, bool hidden) {
    oamSet(&oamMain, oamId,
           p->x, p->y,
           0,              // priority
           15,             // alpha (bmp sprites: 15 = fully opaque)
           SpriteSize_32x32, SpriteColorFormat_Bmp,
           gfxShip[frame],
           -1,             // no affine transform
           false, hidden,  // double size, hide
           false, false,   // vflip, hflip
           false);         // mosaic
}
