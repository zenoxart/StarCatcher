#ifndef BOSS_H
#define BOSS_H

#include <nds.h>
#include "player.h"

#define BOSS_MAX_HP                8
#define MAX_BOSS_BULLETS           8
#define BOSS_ATTACK_INTERVAL      70 // frames between shots
#define BOSS_ATTACK_FLASH_DURATION 16 // how long the "attack" frame shows around each shot
#define BOSS_DESTROY_DURATION      60 // frames the "destroyed" frame is held before the level advances
#define BOSS_BULLET_TRAVEL_FRAMES 50 // frames for a bullet to cross from the boss to where the player was aimed at

typedef struct {
    int x, y;
    int width, height;
    int hp;
    bool active;
    bool destroyed;   // true once HP hits 0; the destroy animation is playing
    int attackTimer;  // counts down to the next shot
    int flashTimer;   // >0 while showing the "attack" frame
    int destroyTimer; // counts down while showing the "destroyed" frame
} Boss;

typedef struct {
    int x, y;
    int vx, vy; // per-frame velocity, pre-divided at spawn time (straight-line aim, no runtime sqrt needed)
    bool active;
} BossBullet;

// Spawns the boss at a fixed spot near the top of the screen with full HP.
void boss_spawn(Boss* boss);

// Advances the attack/flash/destroy timers and fires an aimed bullet
// whenever the attack timer elapses. Sets *bossDefeated to true on the
// exact frame the destroy animation finishes (caller should then advance
// to the next level).
void boss_update(Boss* boss, BossBullet bullets[MAX_BOSS_BULLETS], const Player* player, bool* bossDefeated);

// Applies one hit of damage if the boss is currently active and alive.
// Returns true if the hit actually landed (so the caller can award points
// and consume the projectile).
bool boss_hitByProjectile(Boss* boss);

void boss_draw(const Boss* boss, int oamId);

// Draws a row of HP pip sprites above the boss (oamStartId..oamStartId+BOSS_MAX_HP-1).
void boss_drawHealthBar(const Boss* boss, int oamStartId);

void bossBullets_init(BossBullet bullets[MAX_BOSS_BULLETS]);
void bossBullets_update(BossBullet bullets[MAX_BOSS_BULLETS]);
void bossBullets_draw(const BossBullet bullets[MAX_BOSS_BULLETS], int oamStartId);

#endif // BOSS_H
