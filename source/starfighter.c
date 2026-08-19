#include <stdio.h>
#include <string.h>

#include "starfighter.h"

#define SF_FRAME_SIZE   96
#define SF_FRAME_BYTES  (SF_FRAME_SIZE * SF_FRAME_SIZE)
#define SF_FRAME_COUNT  48
#define SF_FRAME_HOLD   3 // vblanks per animation frame (~20fps at 60Hz)

// Background mode (during gameplay): small, tucked into the top-right
// corner, gameplay sprites (priority 0-1) render in front of it wherever
// they cross it - see the priority set on bg2 below.
#define SF_BG_X 152
#define SF_BG_Y 8

// Showcase mode (title screen): the same source frame nearest-neighbor
// upscaled 2x (192x192) and centered, so it reads as the main visual
// instead of a small background decal. 192 fits the screen's full height
// exactly (192x192); horizontal margin centers it ((256-192)/2 = 32).
#define SF_SHOWCASE_SCALE 2
#define SF_SHOWCASE_SIZE  (SF_FRAME_SIZE * SF_SHOWCASE_SCALE)
#define SF_SHOWCASE_X     ((256 - SF_SHOWCASE_SIZE) / 2)
#define SF_SHOWCASE_Y     ((192 - SF_SHOWCASE_SIZE) / 2)

// Palette range reserved inside BG_PALETTE (top screen's Main-engine
// palette) for the ship's 128-color quantized palette. Nothing else on
// this screen uses BG_PALETTE except index 0 (backdrop) and the intro's
// one-shot palette load (stale/irrelevant once intro_play() returns) -
// gameplay sprites are direct-color bitmaps (SpriteColorFormat_Bmp), not
// palette-indexed, and topscreen.c's console uses its own bank 0 (0-15).
#define SF_PAL_BASE 100

static FILE* sfFile = NULL;
static u8 frameBuf[SF_FRAME_BYTES];
static int currentFrame = 0;
static int holdCounter = 0;
static bool loaded = false;
static int status = STARFIGHTER_STATUS_NOT_INITED;
static int bg2 = -1;
static StarfighterMode currentMode = STARFIGHTER_HIDDEN;

static void blitBackground(u8* bgBuf) {
    for (int y = 0; y < SF_FRAME_SIZE; y++) {
        u8* dstRow = bgBuf + (SF_BG_Y + y) * 256 + SF_BG_X;
        u8* srcRow = frameBuf + y * SF_FRAME_SIZE;
        for (int x = 0; x < SF_FRAME_SIZE; x++) {
            dstRow[x] = SF_PAL_BASE + srcRow[x];
        }
    }
}

static void blitShowcase(u8* bgBuf) {
    for (int y = 0; y < SF_SHOWCASE_SIZE; y++) {
        u8* dstRow = bgBuf + (SF_SHOWCASE_Y + y) * 256 + SF_SHOWCASE_X;
        u8* srcRow = frameBuf + (y / SF_SHOWCASE_SCALE) * SF_FRAME_SIZE;
        for (int x = 0; x < SF_SHOWCASE_SIZE; x++) {
            dstRow[x] = SF_PAL_BASE + srcRow[x / SF_SHOWCASE_SCALE];
        }
    }
}

static void drawCurrentMode(void) {
    u8* bgBuf = (u8*)bgGetGfxPtr(bg2);
    if (currentMode == STARFIGHTER_BACKGROUND) {
        blitBackground(bgBuf);
    } else if (currentMode == STARFIGHTER_SHOWCASE) {
        blitShowcase(bgBuf);
    }
}

void starfighter_init(void) {
    // Same bank/offset intro.c already proved out (mapBase 4 = 64KB into
    // VRAM bank B, clear of topscreen.c's console at mapBase 30/tileBase 0).
    // intro.c's own bg2 handle is a local that already called bgHide() and
    // fell out of scope, so re-running bgInit() here just claims a fresh
    // handle for the same physical layer once the intro is done with it.
    bg2 = bgInit(2, BgType_Bmp8, BgSize_B8_256x256, 4, 0);
    bgSetPriority(bg2, 2); // behind every gameplay OAM sprite (priority 0-1)
    bgHide(bg2);

    FILE* palFile = fopen("nitro:/starfighter.pal", "rb");
    if (!palFile) {
        status = STARFIGHTER_STATUS_NO_PALETTE;
        return;
    }
    u16 pal[128];
    size_t palRead = fread(pal, sizeof(u16), 128, palFile);
    fclose(palFile);
    if (palRead != 128) {
        status = STARFIGHTER_STATUS_SHORT_PALETTE;
        return;
    }
    for (int i = 0; i < 128; i++) {
        BG_PALETTE[SF_PAL_BASE + i] = pal[i];
    }

    sfFile = fopen("nitro:/starfighter.raw", "rb");
    if (!sfFile) {
        status = STARFIGHTER_STATUS_NO_RAW;
        return;
    }

    size_t frameRead = fread(frameBuf, 1, SF_FRAME_BYTES, sfFile);
    if (frameRead != SF_FRAME_BYTES) {
        status = STARFIGHTER_STATUS_SHORT_RAW;
        return;
    }

    loaded = true;
    status = STARFIGHTER_STATUS_OK;
}

int starfighter_status(void) {
    return status;
}

void starfighter_update(void) {
    if (!loaded) {
        return;
    }

    holdCounter++;
    if (holdCounter < SF_FRAME_HOLD) {
        return;
    }
    holdCounter = 0;

    currentFrame = (currentFrame + 1) % SF_FRAME_COUNT;
    fseek(sfFile, (long)currentFrame * SF_FRAME_BYTES, SEEK_SET);
    fread(frameBuf, 1, SF_FRAME_BYTES, sfFile);
}

void starfighter_setMode(StarfighterMode mode) {
    if (!loaded) {
        return;
    }

    bool wasVisible = (currentMode != STARFIGHTER_HIDDEN);
    bool wantVisible = (mode != STARFIGHTER_HIDDEN);

    if (mode != currentMode) {
        // Background- and showcase-mode footprints only partially overlap,
        // so switching between them (or hiding) could leave stale pixels
        // from the other mode's footprint behind. Wipe the whole layer to
        // the same dark backdrop color used everywhere else (index 0) first.
        memset(bgGetGfxPtr(bg2), 0, 256 * 192);
    }

    if (wantVisible && !wasVisible) {
        bgShow(bg2);
    } else if (!wantVisible && wasVisible) {
        bgHide(bg2);
    }
    currentMode = mode;

    if (wantVisible) {
        drawCurrentMode();
    }
}
