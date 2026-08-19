#include <stdio.h>

#include "topscreen.h"
#include "leaderboard.h"

static PrintConsole topConsole;

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

void topscreen_init(void) {
    // Bank B is used exclusively for this console (bank A already holds the
    // gameplay sprite graphics, and BG vs OBJ memory are separate hardware
    // regions anyway, so there is no conflict either way).
    //
    // IMPORTANT: plain VRAM_B_MAIN_BG maps bank B to background "slot 1"
    // (VRAM address 0x06020000, +128KB) rather than slot 0 (0x06000000).
    // consoleInit()'s mapBase/tileBase are always relative to slot 0, so
    // without the explicit _0x06000000 variant the console would draw into
    // unmapped memory and never appear on screen.
    vramSetBankB(VRAM_B_MAIN_BG_0x06000000);

    consoleInit(&topConsole, 0, BgType_Text4bpp, BgSize_T_256x256, 30, 0, true, true);
    bgSetPriority(topConsole.bgId, 3); // stay behind the gameplay sprites

    consoleSelect(&topConsole);
    BG_PALETTE[15] = RGB15(28, 30, 31); // bright text color
    consoleClear();
}

void topscreen_clear(void) {
    consoleSelect(&topConsole);
    consoleClear();
}

void topscreen_showEnterInitials(int score, int level,
                                   const char initials[INITIALS_LEN], int filledCount) {
    consoleSelect(&topConsole);
    consoleClear();

    box(3, 4, 26, 11);
    iprintf("\x1b[6;7HNEUER HIGHSCORE!");
    iprintf("\x1b[8;5HPunkte: %-6d Lv %d", score, level);

    iprintf("\x1b[11;11H");
    for (int i = 0; i < INITIALS_LEN; i++) {
        iprintf("%c ", i < filledCount ? initials[i] : '_');
    }

    iprintf("\x1b[13;5HTippe unten deine");
    iprintf("\x1b[14;5HInitialen ein.");
}

void topscreen_showLeaderboard(int highlightRank) {
    consoleSelect(&topConsole);
    consoleClear();

    box(3, 2, 26, 18);
    iprintf("\x1b[4;8HBESTENLISTE");

    if (leaderboardCount == 0) {
        iprintf("\x1b[9;6HNoch keine Eintraege.");
    } else {
        for (int i = 0; i < leaderboardCount; i++) {
            const char* marker = (i + 1 == highlightRank) ? "->" : "  ";
            iprintf("\x1b[%d;4H%s%d. %-3s %-5d Lv%d",
                    7 + i, marker, i + 1,
                    leaderboard[i].initials, leaderboard[i].score, leaderboard[i].level);
        }
    }

    if (leaderboard_isPersistent()) {
        iprintf("\x1b[16;5HGespeichert auf SD/Karte");
    } else {
        iprintf("\x1b[16;5HNur diese Sitzung (kein");
        iprintf("\x1b[17;5HSpeicher gefunden)");
    }
}
