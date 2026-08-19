#include <stdio.h>
#include <string.h>

#include "intro.h"

#define INTRO_FPS          24
#define INTRO_FRAME_BYTES  (256 * 192) // 8bpp, one byte per pixel

static u8 frameBuf[INTRO_FRAME_BYTES];

void intro_play(void) {
    FILE* f = fopen("nitro:/intro.raw", "rb");
    if (!f) {
        return; // no intro asset available - just skip straight to the title screen
    }

    FILE* palFile = fopen("nitro:/intro.pal", "rb");
    if (palFile) {
        u16 pal[256];
        fread(pal, sizeof(u16), 256, palFile);
        fclose(palFile);
        for (int i = 0; i < 256; i++) {
            BG_PALETTE[i] = pal[i];
        }
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    int totalFrames = (int)(fileSize / INTRO_FRAME_BYTES);
    if (totalFrames < 1) {
        fclose(f);
        return;
    }

    // Bitmap layer shares VRAM bank B with the top screen's text console
    // (topscreen.c, BG0). The console only uses tileBase 0 (~15KB) and
    // mapBase 30 (~2KB near offset 60KB), so placing this bitmap at
    // mapBase 4 (64KB offset, bitmap mapBase unit = 16KB) keeps it clear of
    // both - same technique as the HUD's bitmap+console combo on the
    // sub screen (see hud.c for the fuller VRAM layout explanation).
    int bg2 = bgInit(2, BgType_Bmp8, BgSize_B8_256x256, 4, 0);
    bgSetPriority(bg2, 0);
    u8* vram = (u8*)bgGetGfxPtr(bg2);

    fread(frameBuf, 1, INTRO_FRAME_BYTES, f);
    memcpy(vram, frameBuf, INTRO_FRAME_BYTES);

    int displayedFrame = 0;
    int vblankCount = 0;

    while (1) {
        swiWaitForVBlank();
        vblankCount++;

        scanKeys();
        if (keysDown() & (KEY_START | KEY_A | KEY_B)) {
            break;
        }

        int targetFrame = (vblankCount * INTRO_FPS) / 60;
        if (targetFrame >= totalFrames) {
            break;
        }
        if (targetFrame != displayedFrame) {
            fseek(f, (long)targetFrame * INTRO_FRAME_BYTES, SEEK_SET);
            fread(frameBuf, 1, INTRO_FRAME_BYTES, f);
            memcpy(vram, frameBuf, INTRO_FRAME_BYTES);
            displayedFrame = targetFrame;
        }
    }

    fclose(f);
    bgHide(bg2); // done with the bitmap layer; gameplay sprites take over from here
}
