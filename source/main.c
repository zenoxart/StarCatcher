/*---------------------------------------------------------------------------------

	Star Catcher
	A tiny Nintendo DS homebrew game built with devkitARM + libnds.

	Top screen:    gameplay (player ship, falling stars/meteors, projectiles,
	               a level-exit portal, and every 5th level a boss fight
	               instead - all OAM bitmap sprites) during play, and the
	               leaderboard / high-score preview text otherwise.
	Bottom screen: a modern-ish HUD (gradient/panel bitmap background with the
	               text console layered on top, plus heart-sprite lives icons)
	               during play, and an on-screen touch keyboard for entering
	               initials after a qualifying game over.

---------------------------------------------------------------------------------*/
#include <nds.h>
#include <filesystem.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "sprites.h"
#include "player.h"
#include "entities.h"
#include "projectiles.h"
#include "boss.h"
#include "hud.h"
#include "topscreen.h"
#include "keyboard.h"
#include "leaderboard.h"
#include "music.h"
#include "intro.h"
#include "starfighter.h"

#define STARTING_LIVES       3
#define BASE_STARS_NEEDED    8
#define SHOOT_COOLDOWN      10 // frames between shots

#define SHIP_BLINK_INTERVAL 90 // nav lights blink once every N frames
#define SHIP_BLINK_DURATION 12 // ...and stay lit for this many of them
#define SHIP_EXPLODE_DURATION 40 // how long the ship-explosion frame is shown before Game Over

#define BOSS_LEVEL_INTERVAL 5   // every 5th level is a boss fight instead of the normal level
#define BOSS_HIT_SCORE      20  // points per projectile that lands on the boss
#define BOSS_DEFEAT_BONUS  100  // extra points for defeating the boss

static int starsNeededForLevel(int level) {
    return BASE_STARS_NEEDED + (level - 1) * 2;
}

static bool isBossLevel(int level) {
    return level % BOSS_LEVEL_INTERVAL == 0;
}

static void resolveProjectileHits(Projectile projectiles[MAX_PROJECTILES],
                                    Entity entities[MAX_ENTITIES], int* score) {
    for (int p = 0; p < MAX_PROJECTILES; p++) {
        if (!projectiles[p].active) {
            continue;
        }
        for (int e = 0; e < MAX_ENTITIES; e++) {
            if (!entities[e].active || entities[e].type != ENTITY_METEOR ||
                entities[e].state != ENTITY_ALIVE) {
                continue;
            }

            bool overlapsX = projectiles[p].x + PROJECTILE_SIZE > entities[e].x &&
                              projectiles[p].x < entities[e].x + SPRITE_SIZE;
            bool overlapsY = projectiles[p].y + PROJECTILE_SIZE > entities[e].y &&
                              projectiles[p].y < entities[e].y + SPRITE_SIZE;

            if (overlapsX && overlapsY && entities_hitByProjectile(entities, e)) {
                projectiles[p].active = false;
                *score += 5;
                break;
            }
        }
    }
}

static void resolveProjectileHitsOnBoss(Projectile projectiles[MAX_PROJECTILES], Boss* boss, int* score) {
    if (!boss->active || boss->destroyed) {
        return;
    }
    for (int p = 0; p < MAX_PROJECTILES; p++) {
        if (!projectiles[p].active) {
            continue;
        }
        bool overlapsX = projectiles[p].x + PROJECTILE_SIZE > boss->x && projectiles[p].x < boss->x + boss->width;
        bool overlapsY = projectiles[p].y + PROJECTILE_SIZE > boss->y && projectiles[p].y < boss->y + boss->height;

        if (overlapsX && overlapsY && boss_hitByProjectile(boss)) {
            projectiles[p].active = false;
            *score += BOSS_HIT_SCORE;
        }
    }
}

static void resolveBossBulletHits(BossBullet bullets[MAX_BOSS_BULLETS], const Player* player, int* lives) {
    for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
        if (!bullets[i].active) {
            continue;
        }
        bool overlapsX = bullets[i].x + PROJECTILE_SIZE > player->x && bullets[i].x < player->x + player->width;
        bool overlapsY = bullets[i].y + PROJECTILE_SIZE > player->y && bullets[i].y < player->y + player->height;

        if (overlapsX && overlapsY) {
            bullets[i].active = false;
            (*lives)--;
        }
    }
}

int main(void) {
    // Mounted once, up front: music_init() and starfighter_init() both read
    // files from NitroFS, and each used to rely on whichever one happened
    // to run first to also be the one mounting it (nitroFSInit() lived
    // inside music_init()). Mounting here up front removes that ordering
    // trap entirely; nitroFSInit() is safe to call again later (music_init()
    // still does), it just no-ops once already mounted.
    nitroFSInit(NULL);

    // MODE_5_2D (not MODE_0_2D) so the top screen can show the intro's
    // bitmap layer (BG2) in addition to the plain text layer (BG0, see
    // topscreen.c) that the leaderboard/highscore screens use later.
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_SPRITE);
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    BG_PALETTE[0] = RGB15(2, 4, 12); // dark night-blue backdrop, top screen

    hud_init();
    topscreen_init();
    leaderboard_init();
    music_init(); // background music; silently no-ops if it can't start

    intro_play(); // boot intro clip; no-ops instantly if its assets are missing
    BG_PALETTE[0] = RGB15(2, 4, 12); // intro_play() overwrites the whole palette; restore the backdrop

    starfighter_init(); // must run after intro_play(): reclaims its BG2 bitmap layer once it's done with it

    srand(time(NULL));

    sprites_init();

    Player player;
    Entity entities[MAX_ENTITIES];
    Projectile projectiles[MAX_PROJECTILES];
    Boss boss = { 0 };
    BossBullet bossBullets[MAX_BOSS_BULLETS];
    player_init(&player);
    entities_init(entities);
    projectiles_init(projectiles);
    bossBullets_init(bossBullets);

    GameState state = STATE_TITLE;

    int score = 0;
    int lives = STARTING_LIVES;
    int level = 1;
    int starsCaught = 0;
    int starsNeeded = starsNeededForLevel(level);
    int rank = 0;
    bool bossLevel = false;

    int spawnTimer = 0;
    int shootCooldown = 0;
    bool portalSpawned = false;

    int shipBlinkTick = 0;
    int shipExplodeTimer = 0;

    char initials[INITIALS_LEN] = { 'A', 'A', 'A' };
    int initialsFilled = 0;

    while (pmMainLoop()) {
        scanKeys();
        int keys = keysHeld();
        int down = keysDown();

        switch (state) {
            case STATE_TITLE:
                if (down & KEY_START) {
                    score = 0;
                    lives = STARTING_LIVES;
                    level = 1;
                    starsCaught = 0;
                    starsNeeded = starsNeededForLevel(level);
                    portalSpawned = false;
                    bossLevel = isBossLevel(level);
                    spawnTimer = 0;
                    shootCooldown = 0;
                    shipBlinkTick = 0;
                    player_init(&player);
                    entities_init(entities);
                    projectiles_init(projectiles);
                    if (bossLevel) {
                        boss_spawn(&boss);
                        bossBullets_init(bossBullets);
                    } else {
                        boss.active = false;
                    }
                    state = STATE_PLAYING;
                } else if (down & KEY_SELECT) {
                    rank = 0;
                    state = STATE_LEADERBOARD;
                }
                break;

            case STATE_PLAYING: {
                player_update(&player, keys);
                shipBlinkTick++;

                if (shootCooldown > 0) {
                    shootCooldown--;
                }
                if ((keys & KEY_A) && shootCooldown == 0) {
                    int muzzleX = player.x + player.width / 2 - PROJECTILE_SIZE / 2;
                    int muzzleY = player.y - PROJECTILE_SIZE / 2;
                    projectiles_spawn(projectiles, muzzleX, muzzleY);
                    shootCooldown = SHOOT_COOLDOWN;
                }
                projectiles_update(projectiles);

                bool portalReached = false;
                bool bossDefeated = false;

                if (bossLevel) {
                    boss_update(&boss, bossBullets, &player, &bossDefeated);
                    bossBullets_update(bossBullets);
                    resolveProjectileHitsOnBoss(projectiles, &boss, &score);
                    resolveBossBulletHits(bossBullets, &player, &lives);
                    if (bossDefeated) {
                        score += BOSS_DEFEAT_BONUS;
                    }
                } else {
                    spawnTimer--;
                    if (spawnTimer <= 0) {
                        int minSpeed = 1 + score / 150;
                        int maxSpeed = 3 + score / 150;
                        if (maxSpeed > 8) maxSpeed = 8;
                        if (minSpeed > maxSpeed) minSpeed = maxSpeed;
                        entities_spawn(entities, minSpeed, maxSpeed);

                        int interval = 45 - score / 20;
                        if (interval < 12) interval = 12;
                        spawnTimer = interval;
                    }

                    if (!portalSpawned && starsCaught >= starsNeeded) {
                        entities_spawnPortal(entities);
                        portalSpawned = true;
                    }

                    resolveProjectileHits(projectiles, entities, &score);
                    entities_update(entities, &player, &score, &lives, &starsCaught, &portalReached);
                }

                if (portalReached || bossDefeated) {
                    level++;
                    starsCaught = 0;
                    starsNeeded = starsNeededForLevel(level);
                    portalSpawned = false;
                    bossLevel = isBossLevel(level);
                    entities_init(entities);
                    projectiles_init(projectiles);
                    if (bossLevel) {
                        boss_spawn(&boss);
                        bossBullets_init(bossBullets);
                    } else {
                        boss.active = false;
                    }
                    state = STATE_LEVEL_COMPLETE;
                } else if (lives <= 0) {
                    entities_init(entities);
                    projectiles_init(projectiles);
                    boss.active = false;
                    bossBullets_init(bossBullets);
                    shipExplodeTimer = SHIP_EXPLODE_DURATION;
                    state = STATE_SHIP_EXPLODING;
                }
                break;
            }

            case STATE_SHIP_EXPLODING:
                shipExplodeTimer--;
                if (shipExplodeTimer <= 0) {
                    state = STATE_GAMEOVER;
                }
                break;

            case STATE_LEVEL_COMPLETE:
                if (down & KEY_START) {
                    player_init(&player);
                    spawnTimer = 0;
                    shootCooldown = 0;
                    state = STATE_PLAYING;
                }
                break;

            case STATE_GAMEOVER:
                if (down & KEY_START) {
                    if (leaderboard_wouldQualify(score)) {
                        initials[0] = initials[1] = initials[2] = 'A';
                        initialsFilled = 0;
                        state = STATE_ENTER_INITIALS;
                    } else {
                        rank = 0;
                        state = STATE_LEADERBOARD;
                    }
                }
                break;

            case STATE_ENTER_INITIALS: {
                bool confirmed = false;
                keyboard_update(initials, &initialsFilled, &confirmed);
                topscreen_showEnterInitials(score, level, initials, initialsFilled);
                keyboard_draw(initials, initialsFilled);
                if (confirmed) {
                    rank = leaderboard_submit(score, level, initials);
                    state = STATE_LEADERBOARD;
                }
                break;
            }

            case STATE_LEADERBOARD:
                topscreen_showLeaderboard(rank);
                if (down & KEY_START) {
                    topscreen_clear(); // otherwise the list would linger behind gameplay sprites
                    state = STATE_TITLE;
                }
                break;
        }

        hud_render(state, score, lives, level, starsCaught, starsNeeded, rank);

        bool showGameplaySprites = (state == STATE_PLAYING || state == STATE_LEVEL_COMPLETE ||
                                     state == STATE_SHIP_EXPLODING);

        // Rotating ship model viewport (top screen, BG2 - see starfighter.c):
        // a small background decal during actual gameplay, the large main
        // visual on the title screen, hidden everywhere else (scoreboard,
        // keyboard, level-complete/game-over transitions).
        StarfighterMode sfMode = STARFIGHTER_HIDDEN;
        if (state == STATE_PLAYING || state == STATE_SHIP_EXPLODING) {
            sfMode = STARFIGHTER_BACKGROUND;
        } else if (state == STATE_TITLE) {
            sfMode = STARFIGHTER_SHOWCASE;
        }
        starfighter_update();
        starfighter_setMode(sfMode);

        ShipFrame shipFrame = SHIP_IDLE;
        if (state == STATE_SHIP_EXPLODING) {
            shipFrame = SHIP_EXPLODE;
        } else if ((shipBlinkTick % SHIP_BLINK_INTERVAL) < SHIP_BLINK_DURATION) {
            shipFrame = SHIP_BLINK;
        }

        player_draw(&player, 0, shipFrame, !showGameplaySprites);
        entities_draw(entities, 1);
        projectiles_draw(projectiles, 1 + MAX_ENTITIES);
        boss_draw(&boss, 17);
        boss_drawHealthBar(&boss, 18);
        bossBullets_draw(bossBullets, 18 + BOSS_MAX_HP);

        swiWaitForVBlank();
        oamUpdate(&oamMain);
    }

    return 0;
}
