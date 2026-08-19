#ifndef SPRITES_H
#define SPRITES_H

#include <nds.h>

#define SHEET_FRAMES 3

// Star: 0/1 = idle rotation (alternate while falling), 2 = collected sparkle.
extern u16* gfxStar[SHEET_FRAMES];
// Meteor: 0/1 = idle rock variants (alternate while falling), 2 = cracking (hit).
extern u16* gfxMeteor[SHEET_FRAMES];
// Portal: 0/1/2 = spin animation, cycle continuously.
extern u16* gfxPortal[SHEET_FRAMES];
// Ship: 0 = idle, 1 = nav lights blinking, 2 = exploding.
extern u16* gfxShip[SHEET_FRAMES];
// Boss: 0 = idle, 1 = attacking, 2 = destroyed.
extern u16* gfxBoss[SHEET_FRAMES];

// Still procedural (not part of graphics/spritesheet.png): the player's
// laser bolt, the boss's bullets, and its health bar pips.
extern u16* gfxProjectile;
extern u16* gfxBossBullet;
extern u16* gfxHealthPip;

// Allocates OAM graphics memory and loads all spritesheet frames plus the
// procedural projectile bolt. Must be called after oamInit(&oamMain, ...)
// and vramSetBankA(VRAM_A_MAIN_SPRITE).
void sprites_init(void);

#endif // SPRITES_H
