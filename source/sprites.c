#include "sprites.h"
#include "game.h"
#include "spritesheet.h"

#define SHEET_COLS   3
#define SHEET_STRIDE 96 // spritesheet.png total width in pixels

#define HEALTHPIP_SIZE 8

u16* gfxStar[SHEET_FRAMES]   = { NULL, NULL, NULL };
u16* gfxMeteor[SHEET_FRAMES] = { NULL, NULL, NULL };
u16* gfxPortal[SHEET_FRAMES] = { NULL, NULL, NULL };
u16* gfxShip[SHEET_FRAMES]   = { NULL, NULL, NULL };
u16* gfxBoss[SHEET_FRAMES]   = { NULL, NULL, NULL };
u16* gfxProjectile = NULL;
u16* gfxBossBullet = NULL;
u16* gfxHealthPip  = NULL;

// Copies one 32x32 cell (row/col in the 3-column sheet grid) out of the
// flat spritesheetBitmap array. The sheet's pixel format already matches
// what OAM bitmap sprites expect (alpha bit set for opaque, clear for the
// white transparent key), so this is a straight copy - no conversion.
static void copyFrame(u16* dest, int col, int row) {
    for (int y = 0; y < SPRITE_SIZE; y++) {
        int srcRow = row * SPRITE_SIZE + y;
        int srcColBase = col * SPRITE_SIZE;
        for (int x = 0; x < SPRITE_SIZE; x++) {
            dest[y * SPRITE_SIZE + x] = spritesheetBitmap[srcRow * SHEET_STRIDE + srcColBase + x];
        }
    }
}

static void drawBolt(u16* gfx, u16 coreColor, u16 glowColor) {
    for (int i = 0; i < PROJECTILE_SIZE * PROJECTILE_SIZE; i++) {
        gfx[i] = 0;
    }
    int center = PROJECTILE_SIZE / 2;
    for (int y = 4; y < PROJECTILE_SIZE - 2; y++) {
        gfx[y * PROJECTILE_SIZE + center - 2] = glowColor;
        gfx[y * PROJECTILE_SIZE + center - 1] = coreColor;
        gfx[y * PROJECTILE_SIZE + center]     = coreColor;
        gfx[y * PROJECTILE_SIZE + center + 1] = glowColor;
    }
}

// Small round bullet (as opposed to the player's straight bolt), so the
// boss's shots read as clearly distinct from the player's.
static void drawOrb(u16* gfx, u16 color) {
    for (int i = 0; i < PROJECTILE_SIZE * PROJECTILE_SIZE; i++) {
        gfx[i] = 0;
    }
    int center = PROJECTILE_SIZE / 2;
    int radius = 4;
    for (int y = 0; y < PROJECTILE_SIZE; y++) {
        for (int x = 0; x < PROJECTILE_SIZE; x++) {
            int dx = x - center;
            int dy = y - center;
            if (dx * dx + dy * dy <= radius * radius) {
                gfx[y * PROJECTILE_SIZE + x] = color;
            }
        }
    }
}

static void drawHealthPip(u16* gfx, u16 color) {
    for (int i = 0; i < HEALTHPIP_SIZE * HEALTHPIP_SIZE; i++) {
        gfx[i] = color;
    }
}

void sprites_init(void) {
    for (int i = 0; i < SHEET_FRAMES; i++) {
        gfxStar[i]   = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);
        gfxMeteor[i] = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);
        gfxPortal[i] = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);
        gfxShip[i]   = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);
        gfxBoss[i]   = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);

        copyFrame(gfxStar[i],   i, 0);
        copyFrame(gfxMeteor[i], i, 1);
        copyFrame(gfxPortal[i], i, 2);
        copyFrame(gfxShip[i],   i, 3);
        copyFrame(gfxBoss[i],   i, 4);
    }

    gfxProjectile = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_Bmp);
    drawBolt(gfxProjectile, ARGB16(1, 31, 31, 20), ARGB16(1, 20, 26, 31));

    gfxBossBullet = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_Bmp);
    drawOrb(gfxBossBullet, ARGB16(1, 31, 10, 20));

    gfxHealthPip = oamAllocateGfx(&oamMain, SpriteSize_8x8, SpriteColorFormat_Bmp);
    drawHealthPip(gfxHealthPip, ARGB16(1, 20, 31, 20));
}
