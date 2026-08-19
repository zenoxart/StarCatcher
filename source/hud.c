#include <stdio.h>

#include "hud.h"

#define HUD_MAX_HEARTS 5
#define HEADER_HEIGHT  40

// Palette indices for the decorative bitmap background (BG3, 256-color).
// Index 15 is intentionally left untouched: the console's default font
// uses palette bank 0 / color 15 for its glyph color, and BG3 shares the
// same physical sub-engine palette RAM as the BG0 text layer.
#define PAL_GRADIENT_BASE  1
#define PAL_GRADIENT_COUNT 40
#define PAL_HEADER_BG      50
#define PAL_HEADER_BORDER  51
#define PAL_FOOTER_LINE    52

static PrintConsole hudConsole;
static int bgHudId;
static u16* gfxHeart = NULL;

static void buildPalette(void) {
    for (int i = 0; i < PAL_GRADIENT_COUNT; i++) {
        int r = 1  + (5  * i) / PAL_GRADIENT_COUNT;
        int g = 2  + (6  * i) / PAL_GRADIENT_COUNT;
        int b = 9  + (16 * i) / PAL_GRADIENT_COUNT;
        BG_PALETTE_SUB[PAL_GRADIENT_BASE + i] = RGB15(r, g, b);
    }
    BG_PALETTE_SUB[PAL_HEADER_BG]     = RGB15(3, 4, 9);
    BG_PALETTE_SUB[PAL_HEADER_BORDER] = RGB15(11, 21, 31);
    BG_PALETTE_SUB[PAL_FOOTER_LINE]   = RGB15(11, 21, 31);
}

static void drawBackground(void) {
    u8* buf = (u8*)bgGetGfxPtr(bgHudId);

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        int step = (y * PAL_GRADIENT_COUNT) / SCREEN_HEIGHT;
        u8 color = (u8)(PAL_GRADIENT_BASE + step);
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            buf[y * 256 + x] = color;
        }
    }

    for (int y = 0; y < HEADER_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            buf[y * 256 + x] = PAL_HEADER_BG;
        }
    }
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        buf[(HEADER_HEIGHT - 1) * 256 + x] = PAL_HEADER_BORDER;
    }
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        buf[(SCREEN_HEIGHT - 2) * 256 + x] = PAL_FOOTER_LINE;
    }
}

// Two round lobes + a tapering point, approximated with simple distance checks.
static void drawHeartSprite(u16* gfx, u16 color) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int dxL = x - 6, dyL = y - 6;
            int dxR = x - 10, dyR = y - 6;
            bool inLobe = (dxL * dxL + dyL * dyL <= 13) || (dxR * dxR + dyR * dyR <= 13);

            bool inPoint = false;
            if (y >= 6 && y <= 13) {
                int halfWidth = 13 - y;
                if (halfWidth > 7) halfWidth = 7;
                inPoint = (x >= 8 - halfWidth) && (x <= 8 + halfWidth);
            }

            if (inLobe || inPoint) {
                gfx[y * 16 + x] = color;
            }
        }
    }
}

static void box(int x, int y, int w, int h) {
    iprintf("\x1b[%d;%dH+", y, x);
    for (int i = 1; i < w - 1; i++) iprintf("-");
    iprintf("+");

    for (int row = 1; row < h - 1; row++) {
        iprintf("\x1b[%d;%dH|", y + row, x);
        iprintf("\x1b[%d;%dH|", y + row, x + w - 1);
    }

    iprintf("\x1b[%d;%dH+", y + h - 1, x);
    for (int i = 1; i < w - 1; i++) iprintf("-");
    iprintf("+");
}

static void setHearts(int lives) {
    if (lives < 0) lives = 0;
    if (lives > HUD_MAX_HEARTS) lives = HUD_MAX_HEARTS;

    for (int i = 0; i < HUD_MAX_HEARTS; i++) {
        int x = SCREEN_WIDTH - 8 - (i + 1) * 18;
        int y = 10;
        oamSet(&oamSub, i,
               x, y,
               0, 15,
               SpriteSize_16x16, SpriteColorFormat_Bmp,
               gfxHeart,
               -1,
               false, i >= lives, // hide hearts beyond current life count
               false, false,
               false);
    }
}

void hud_init(void) {
    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);

    // IMPORTANT: a text background's mapBase is a hardware register field
    // limited to 0-31 (2KB units => only the first 64KB of sub-BG VRAM is
    // reachable for a tile MAP, no matter which bank backs it). A bitmap
    // background's mapBase instead means a 16KB offset for the pixel data
    // itself. Our decorative bitmap is placed in the *second* half of the
    // 128KB bank (mapBase 4 = 64KB) so the *first* half stays free for the
    // console's map (mapBase 0) and tile graphics (tileBase 1 = 16KB) -
    // otherwise the two layers alias onto the same bytes and the console
    // renders garbage tiles (this is what caused the glitching).
    bgHudId = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 4, 0);
    bgSetPriority(bgHudId, 3);

    consoleInit(&hudConsole, 0, BgType_Text4bpp, BgSize_T_256x256, 0, 1, false, true);
    bgSetPriority(hudConsole.bgId, 0);
    consoleSelect(&hudConsole);
    BG_PALETTE_SUB[15] = RGB15(26, 30, 31); // soft cyan-white text color

    buildPalette();
    drawBackground();

    vramSetBankD(VRAM_D_SUB_SPRITE);
    oamInit(&oamSub, SpriteMapping_Bmp_1D_128, false);
    gfxHeart = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_Bmp);
    for (int i = 0; i < 16 * 16; i++) gfxHeart[i] = 0;
    drawHeartSprite(gfxHeart, ARGB16(1, 28, 5, 10));
}

static void drawTitleStatic(void) {
    consoleClear();
    box(1, 1, 30, 13);
    iprintf("\x1b[3;10HSTAR CATCHER");
    iprintf("\x1b[5;3HSterne einsammeln,");
    iprintf("\x1b[6;3HMeteoriten zersprengen!");
    iprintf("\x1b[9;3HSTEUERKREUZ  Bewegen");
    iprintf("\x1b[10;3HA            Schiessen");
    iprintf("\x1b[11;3HSTART        Spiel starten");
    iprintf("\x1b[12;3HSELECT       Bestenliste");
}

static void drawPlayingStatic(void) {
    consoleClear();
    iprintf("\x1b[0;1HLEVEL");
    iprintf("\x1b[1;1HSCORE");
    iprintf("\x1b[2;1HSTERNE");
}

static void drawPlayingDynamic(int score, int level, int starsCaught, int starsNeeded) {
    iprintf("\x1b[0;8H%-4d", level);
    iprintf("\x1b[1;8H%-6d", score);
    iprintf("\x1b[2;8H%d/%-4d", starsCaught, starsNeeded);
}

static void drawLevelComplete(int level) {
    consoleClear();
    box(3, 6, 26, 8);
    iprintf("\x1b[8;7HLEVEL %d GESCHAFFT!", level);
    iprintf("\x1b[10;6HPortal durchquert.");
    iprintf("\x1b[12;6HSTART: naechstes Level");
}

static void drawGameOver(int score, int level, int rank) {
    consoleClear();
    box(2, 4, 28, 14);
    iprintf("\x1b[6;10HGAME OVER");
    iprintf("\x1b[9;5HPunkte:  %d", score);
    iprintf("\x1b[10;5HLevel:   %d", level);
    if (rank > 0) {
        iprintf("\x1b[12;5HNEUER HIGHSCORE!");
        iprintf("\x1b[13;5HPlatz %d in der Bestenliste", rank);
    } else {
        iprintf("\x1b[12;5HKein Platz in der");
        iprintf("\x1b[13;5HBestenliste diesmal.");
    }
    iprintf("\x1b[16;5HSTART: weiter");
}

static void drawLeaderboardScreen(void) {
    consoleClear();
    box(4, 8, 24, 8);
    iprintf("\x1b[10;9HBESTENLISTE");
    iprintf("\x1b[12;5HSchau auf den");
    iprintf("\x1b[13;5Hoberen Bildschirm!");
    iprintf("\x1b[15;5HSTART: zurueck zum Titel");
}

PrintConsole* hud_getConsole(void) {
    return &hudConsole;
}

void hud_render(GameState state, int score, int lives, int level,
                 int starsCaught, int starsNeeded, int rank) {
    // Other modules (topscreen.c) select their own console when drawing to
    // the top screen; reselect ours here so we always target the bottom
    // screen regardless of what ran earlier in the frame.
    consoleSelect(&hudConsole);

    static int lastState = -1;
    bool changed = ((int)state != lastState);
    lastState = (int)state;

    switch (state) {
        case STATE_TITLE:
            if (changed) drawTitleStatic();
            break;

        case STATE_PLAYING:
            if (changed) drawPlayingStatic();
            drawPlayingDynamic(score, level, starsCaught, starsNeeded);
            break;

        case STATE_SHIP_EXPLODING:
            // Keep showing the final in-run stats while the ship explodes
            // on the top screen.
            if (changed) drawPlayingStatic();
            drawPlayingDynamic(score, level, starsCaught, starsNeeded);
            break;

        case STATE_LEVEL_COMPLETE:
            if (changed) drawLevelComplete(level);
            break;

        case STATE_GAMEOVER:
            if (changed) drawGameOver(score, level, rank);
            break;

        case STATE_LEADERBOARD:
            if (changed) drawLeaderboardScreen();
            break;

        case STATE_ENTER_INITIALS:
            // Bottom screen is fully owned by keyboard.c in this state.
            break;
    }

    setHearts(lives);
    oamUpdate(&oamSub);
}
