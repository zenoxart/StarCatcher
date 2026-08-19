#include "projectiles.h"
#include "sprites.h"
#include "game.h"

void projectiles_init(Projectile projectiles[MAX_PROJECTILES]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].active = false;
    }
}

void projectiles_spawn(Projectile projectiles[MAX_PROJECTILES], int x, int y) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            projectiles[i].active = true;
            projectiles[i].x = x;
            projectiles[i].y = y;
            return;
        }
    }
}

void projectiles_update(Projectile projectiles[MAX_PROJECTILES]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            continue;
        }
        projectiles[i].y -= PROJECTILE_SPEED;
        if (projectiles[i].y < -PROJECTILE_SIZE) {
            projectiles[i].active = false;
        }
    }
}

void projectiles_draw(const Projectile projectiles[MAX_PROJECTILES], int oamStartId) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        int id = oamStartId + i;
        const Projectile* p = &projectiles[i];
        oamSet(&oamMain, id,
               p->x, p->y,
               0,              // priority: in front, same as the player
               15,
               SpriteSize_16x16, SpriteColorFormat_Bmp,
               gfxProjectile,
               -1,
               false, !p->active,
               false, false,
               false);
    }
}
