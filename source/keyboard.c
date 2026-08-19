#include <stdio.h>

#include "keyboard.h"
#include "hud.h"

// QWERTY layout, one row of the alphabet per line. Each letter occupies a
// 2-column-wide, 2-row-tall touch target (16x16 px) for reasonably
// forgiving stylus/mouse taps, even though only the top row visually
// shows the glyph.
static const char* const kbRows[3] = { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
static const int kbRowStartCol[3]  = { 6, 7, 9 };
static const int kbRowY[3]         = { 9, 11, 13 };

#define KB_ACTION_ROW_Y 15
#define KB_DEL_COL      10
#define KB_OK_COL       19

static bool hit(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

// Builds "Q W E R T Y ..." (space-spaced) so each row prints as a single
// contiguous string with one cursor-position escape, instead of one escape
// sequence per letter - matching the pattern used everywhere else in this
// codebase (title screen, game over, ...) that is known to render cleanly.
static void drawRow(int row, int startCol, const char* letters) {
    char buf[24];
    int n = 0;
    for (int i = 0; letters[i] != '\0' && n < (int)sizeof(buf) - 1; i++) {
        buf[n++] = letters[i];
        if (letters[i + 1] != '\0' && n < (int)sizeof(buf) - 1) {
            buf[n++] = ' ';
        }
    }
    buf[n] = '\0';
    iprintf("\x1b[%d;%dH%s", row, startCol, buf);
}

void keyboard_draw(const char initials[INITIALS_LEN], int filledCount) {
    // consoleSelect() is a single global switch shared with the top
    // screen's console (see topscreen.c); reselect ours to be safe
    // regardless of what ran earlier in the frame.
    consoleSelect(hud_getConsole());
    consoleClear();
    iprintf("\x1b[2;7HINITIALEN EINGEBEN");

    char preview[2 * INITIALS_LEN + 1];
    int n = 0;
    for (int i = 0; i < INITIALS_LEN; i++) {
        preview[n++] = i < filledCount ? initials[i] : '_';
        if (i < INITIALS_LEN - 1) preview[n++] = ' ';
    }
    preview[n] = '\0';
    iprintf("\x1b[4;13H%s", preview);

    for (int r = 0; r < 3; r++) {
        drawRow(kbRowY[r], kbRowStartCol[r], kbRows[r]);
    }

    iprintf("\x1b[%d;%dH[DEL]", KB_ACTION_ROW_Y, KB_DEL_COL);
    iprintf("\x1b[%d;%dH[OK]", KB_ACTION_ROW_Y, KB_OK_COL);

    iprintf("\x1b[19;2HBeruehre einen Buchstaben");
    iprintf("\x1b[20;2H(B: loeschen, START: fertig)");
}

void keyboard_update(char initials[INITIALS_LEN], int* filledCount, bool* confirmed) {
    *confirmed = false;
    int down = keysDown();

    if (down & KEY_B) {
        if (*filledCount > 0) {
            (*filledCount)--;
        }
        return;
    }

    if (down & KEY_START) {
        if (*filledCount == INITIALS_LEN) {
            *confirmed = true;
        }
        return;
    }

    if (!(down & KEY_TOUCH)) {
        return;
    }

    touchPosition touch;
    touchRead(&touch);
    int px = touch.px;
    int py = touch.py;

    for (int r = 0; r < 3; r++) {
        int row = kbRowY[r];
        int col = kbRowStartCol[r];
        for (int i = 0; kbRows[r][i] != '\0'; i++) {
            int x = (col + i * 2) * 8;
            int y = row * 8;
            if (hit(px, py, x, y, 16, 16)) {
                if (*filledCount < INITIALS_LEN) {
                    initials[*filledCount] = kbRows[r][i];
                    (*filledCount)++;
                }
                return;
            }
        }
    }

    if (hit(px, py, KB_DEL_COL * 8, KB_ACTION_ROW_Y * 8, 40, 16)) {
        if (*filledCount > 0) {
            (*filledCount)--;
        }
        return;
    }

    if (hit(px, py, KB_OK_COL * 8, KB_ACTION_ROW_Y * 8, 32, 16)) {
        if (*filledCount == INITIALS_LEN) {
            *confirmed = true;
        }
    }
}
