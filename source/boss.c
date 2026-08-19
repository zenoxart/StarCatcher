#include <stdlib.h>

#include "boss.h"
#include "sprites.h"
#include "game.h"

void boss_spawn(Boss* boss) {
    boss->width = SPRITE_SIZE;
    boss->height = SPRITE_SIZE;
    boss->x = (SCREEN_WIDTH - boss->width) / 2;
    boss->y = 16; // the boss holds this position; it never falls like normal entities
    boss->hp = BOSS_MAX_HP;
    boss->active = true;
    boss->destroyed = false;
    boss->attackTimer = BOSS_ATTACK_INTERVAL;
    boss->flashTimer = 0;
    boss->destroyTimer = 0;
}

static void spawnAimedBullet(BossBullet bullets[MAX_BOSS_BULLETS], int fromX, int fromY, int toX, int toY) {
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (bullets[i].active) {
            continue;
        }
        bullets[i].active = true;
        bullets[i].x = fromX;
        bullets[i].y = fromY;
        int dx = toX - fromX;
        int dy = toY - fromY;
        bullets[i].vx = dx / BOSS_BULLET_TRAVEL_FRAMES;
        bullets[i].vy = dy / BOSS_BULLET_TRAVEL_FRAMES;
        if (bullets[i].vy == 0) {
            bullets[i].vy = 1; // always drift toward the player, never stall vertically
        }
        return;
    }
}

void boss_update(Boss* boss, BossBullet bullets[MAX_BOSS_BULLETS], const Player* player, bool* bossDefeated) {
    *bossDefeated = false;
    if (!boss->active) {
        return;
    }

    if (boss->destroyed) {
        boss->destroyTimer--;
        if (boss->destroyTimer <= 0) {
            boss->active = false;
            *bossDefeated = true;
        }
        return;
    }

    if (boss->flashTimer > 0) {
        boss->flashTimer--;
    }

    boss->attackTimer--;
    if (boss->attackTimer <= 0) {
        boss->attackTimer = BOSS_ATTACK_INTERVAL;
        boss->flashTimer = BOSS_ATTACK_FLASH_DURATION;

        int fromX = boss->x + boss->width / 2 - PROJECTILE_SIZE / 2;
        int fromY = boss->y + boss->height / 2;
        int toX = player->x + player->width / 2;
        int toY = player->y + player->height / 2;
        spawnAimedBullet(bullets, fromX, fromY, toX, toY);
    }
}

bool boss_hitByProjectile(Boss* boss) {
    if (!boss->active || boss->destroyed) {
        return false;
    }
    boss->hp--;
    if (boss->hp <= 0) {
        boss->destroyed = true;
        boss->destroyTimer = BOSS_DESTROY_DURATION;
    }
    return true;
}

void boss_draw(const Boss* boss, int oamId) {
    u16* gfx;
    if (boss->destroyed) {
        gfx = gfxBoss[2];
    } else if (boss->flashTimer > 0) {
        gfx = gfxBoss[1];
    } else {
        gfx = gfxBoss[0];
    }

    oamSet(&oamMain, oamId,
           boss->x, boss->y,
           0, 15,
           SpriteSize_32x32, SpriteColorFormat_Bmp,
           gfx,
           -1,
           false, !boss->active,
           false, false,
           false);
}

void boss_drawHealthBar(const Boss* boss, int oamStartId) {
    int pipSize = 8;
    int totalWidth = BOSS_MAX_HP * pipSize;
    int barX = boss->x + (boss->width - totalWidth) / 2;
    int barY = boss->y - 12;

    for (int i = 0; i < BOSS_MAX_HP; i++) {
        int id = oamStartId + i;
        bool hidden = !boss->active || boss->destroyed || i >= boss->hp;
        oamSet(&oamMain, id,
               barX + i * pipSize, barY,
               0, 15,
               SpriteSize_8x8, SpriteColorFormat_Bmp,
               gfxHealthPip,
               -1,
               false, hidden,
               false, false,
               false);
    }
}

void bossBullets_init(BossBullet bullets[MAX_BOSS_BULLETS]) {
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        bullets[i].active = false;
    }
}

void bossBullets_update(BossBullet bullets[MAX_BOSS_BULLETS]) {
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (!bullets[i].active) {
            continue;
        }
        bullets[i].x += bullets[i].vx;
        bullets[i].y += bullets[i].vy;
        if (bullets[i].y > SCREEN_HEIGHT || bullets[i].y < -PROJECTILE_SIZE ||
            bullets[i].x < -PROJECTILE_SIZE || bullets[i].x > SCREEN_WIDTH) {
            bullets[i].active = false;
        }
    }
}

void bossBullets_draw(const BossBullet bullets[MAX_BOSS_BULLETS], int oamStartId) {
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        int id = oamStartId + i;
        const BossBullet* b = &bullets[i];
        oamSet(&oamMain, id,
               b->x, b->y,
               0, 15,
               SpriteSize_16x16, SpriteColorFormat_Bmp,
               gfxBossBullet,
               -1,
               false, !b->active,
               false, false,
               false);
    }
}
