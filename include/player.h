#ifndef PLAYER_H
#define PLAYER_H

#include <nds.h>

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int speed;
} Player;

typedef enum {
    SHIP_IDLE,
    SHIP_BLINK,
    SHIP_EXPLODE
} ShipFrame;

void player_init(Player* p);
void player_update(Player* p, int keysHeldMask);

// hidden: true suppresses the sprite (used on menu/leaderboard screens where
// the top screen shows text instead of gameplay).
void player_draw(const Player* p, int oamId, ShipFrame frame, bool hidden);

#endif // PLAYER_H
