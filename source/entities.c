#include <stdlib.h>

#include "entities.h"
#include "sprites.h"
#include "game.h"

static void startCollected(Entity* e) {
    e->state = ENTITY_COLLECTED;
    e->animFrame = 0;
    e->animTimer = COLLECTED_DURATION;
}

static void startExploding(Entity* e) {
    e->state = ENTITY_EXPLODING;
    e->animFrame = 0; // 0 = crack, 1 = boom
    e->animTimer = CRACK_DURATION;
}

void entities_init(Entity entities[MAX_ENTITIES]) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        entities[i].active = false;
        entities[i].type = ENTITY_NONE;
        entities[i].state = ENTITY_ALIVE;
        entities[i].idleTick = 0;
    }
}

void entities_spawn(Entity entities[MAX_ENTITIES], int minSpeed, int maxSpeed) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (entities[i].active) {
            continue;
        }

        entities[i].active = true;
        entities[i].state = ENTITY_ALIVE;
        entities[i].idleTick = rand() % 100; // desync animations between entities
        entities[i].type = (rand() % 4 == 0) ? ENTITY_METEOR : ENTITY_STAR; // ~25% meteors
        entities[i].x = rand() % (SCREEN_WIDTH - SPRITE_SIZE);
        entities[i].y = -SPRITE_SIZE;

        int range = maxSpeed - minSpeed;
        entities[i].speed = minSpeed + (range > 0 ? rand() % (range + 1) : 0);
        return;
    }
    // no free slot: silently skip this spawn, the pool is simply full for now
}

void entities_spawnPortal(Entity entities[MAX_ENTITIES]) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (entities[i].active) {
            continue;
        }

        entities[i].active = true;
        entities[i].state = ENTITY_ALIVE;
        entities[i].idleTick = 0;
        entities[i].type = ENTITY_PORTAL;
        entities[i].x = rand() % (SCREEN_WIDTH - SPRITE_SIZE);
        entities[i].y = -SPRITE_SIZE;
        entities[i].speed = 1;
        return;
    }
}

bool entities_hitByProjectile(Entity entities[MAX_ENTITIES], int index) {
    Entity* e = &entities[index];
    if (e->active && e->type == ENTITY_METEOR && e->state == ENTITY_ALIVE) {
        startExploding(e);
        return true;
    }
    return false;
}

void entities_update(Entity entities[MAX_ENTITIES], const Player* player,
                      int* score, int* lives, int* starsCaught, bool* portalReached) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        Entity* e = &entities[i];
        if (!e->active) {
            continue;
        }

        if (e->state == ENTITY_COLLECTED) {
            e->animTimer--;
            if (e->animTimer <= 0) {
                e->active = false;
            }
            continue;
        }

        if (e->state == ENTITY_EXPLODING) {
            e->animTimer--;
            if (e->animTimer <= 0) {
                if (e->animFrame == 0) {
                    e->animFrame = 1; // crack -> boom
                    e->animTimer = BOOM_DURATION;
                } else {
                    e->active = false;
                }
            }
            continue;
        }

        // ENTITY_ALIVE
        e->idleTick++;
        e->y += e->speed;

        bool overlapsX = e->x + SPRITE_SIZE > player->x && e->x < player->x + player->width;
        bool overlapsY = e->y + SPRITE_SIZE > player->y && e->y < player->y + player->height;

        if (overlapsX && overlapsY) {
            switch (e->type) {
                case ENTITY_STAR:
                    *score += 10;
                    (*starsCaught)++;
                    startCollected(e);
                    break;
                case ENTITY_METEOR:
                    (*lives)--;
                    startExploding(e);
                    break;
                case ENTITY_PORTAL:
                    *portalReached = true;
                    e->active = false;
                    break;
                default:
                    break;
            }
            continue;
        }

        if (e->y > SCREEN_HEIGHT) {
            e->active = false;
        }
    }
}

void entities_draw(const Entity entities[MAX_ENTITIES], int oamStartId) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        int id = oamStartId + i;
        const Entity* e = &entities[i];

        u16* gfx;
        if (e->state == ENTITY_COLLECTED) {
            gfx = gfxStar[2];
        } else if (e->state == ENTITY_EXPLODING) {
            gfx = (e->animFrame == 0) ? gfxMeteor[2] : gfxShip[2];
        } else {
            switch (e->type) {
                case ENTITY_METEOR:
                    gfx = gfxMeteor[(e->idleTick / METEOR_ANIM_INTERVAL) % 2];
                    break;
                case ENTITY_PORTAL:
                    gfx = gfxPortal[(e->idleTick / PORTAL_ANIM_INTERVAL) % SHEET_FRAMES];
                    break;
                default: // ENTITY_STAR
                    gfx = gfxStar[(e->idleTick / STAR_ANIM_INTERVAL) % 2];
                    break;
            }
        }

        oamSet(&oamMain, id,
               e->x, e->y,
               1,              // priority (behind the player sprite)
               15,             // alpha (bmp sprites: 15 = fully opaque)
               SpriteSize_32x32, SpriteColorFormat_Bmp,
               gfx,
               -1,
               false, !e->active, // hide inactive slots
               false, false,
               false);
    }
}
