#include <stdio.h>
#include <string.h>
#include <fat.h>

#include "leaderboard.h"

#define SAVE_PATH "/StarCatcher.sav"
#define FAT_INIT_ATTEMPTS 3 // real hardware can need a moment before the SD/card is ready

LeaderboardEntry leaderboard[LEADERBOARD_SIZE];
int leaderboardCount = 0;

static bool fatAvailable = false;

static void save(void) {
    if (!fatAvailable) {
        return;
    }
    FILE* f = fopen(SAVE_PATH, "wb");
    if (!f) {
        return;
    }
    fwrite(&leaderboardCount, sizeof(int), 1, f);
    fwrite(leaderboard, sizeof(LeaderboardEntry), leaderboardCount, f);
    fclose(f);
}

void leaderboard_init(void) {
    leaderboardCount = 0;
    memset(leaderboard, 0, sizeof(leaderboard));

    fatAvailable = fatInitDefault();
    for (int attempt = 1; !fatAvailable && attempt < FAT_INIT_ATTEMPTS; attempt++) {
        fatAvailable = fatInitDefault();
    }
    if (!fatAvailable) {
        return; // genuinely no SD/flashcard filesystem available this session
    }

    FILE* f = fopen(SAVE_PATH, "rb");
    if (!f) {
        // No save on disk yet (first run, or a freshly inserted card) -
        // create an empty one right away instead of only writing lazily
        // on the first leaderboard_submit().
        save();
        return;
    }

    int count = 0;
    if (fread(&count, sizeof(int), 1, f) == 1) {
        if (count < 0) count = 0;
        if (count > LEADERBOARD_SIZE) count = LEADERBOARD_SIZE;
        leaderboardCount = (int)fread(leaderboard, sizeof(LeaderboardEntry), count, f);
    }
    fclose(f);
}

bool leaderboard_wouldQualify(int score) {
    if (leaderboardCount < LEADERBOARD_SIZE) {
        return true;
    }
    return score > leaderboard[LEADERBOARD_SIZE - 1].score;
}

int leaderboard_submit(int score, int level, const char* initials) {
    int insertAt = -1;
    for (int i = 0; i < leaderboardCount; i++) {
        if (score > leaderboard[i].score) {
            insertAt = i;
            break;
        }
    }
    if (insertAt == -1 && leaderboardCount < LEADERBOARD_SIZE) {
        insertAt = leaderboardCount;
    }
    if (insertAt == -1) {
        return 0; // did not beat the lowest entry on a full board
    }

    int last = (leaderboardCount < LEADERBOARD_SIZE) ? leaderboardCount : LEADERBOARD_SIZE - 1;
    for (int i = last; i > insertAt; i--) {
        leaderboard[i] = leaderboard[i - 1];
    }
    leaderboard[insertAt].score = score;
    leaderboard[insertAt].level = level;
    strncpy(leaderboard[insertAt].initials, initials, INITIALS_LEN);
    leaderboard[insertAt].initials[INITIALS_LEN] = '\0';

    if (leaderboardCount < LEADERBOARD_SIZE) {
        leaderboardCount++;
    }

    save();
    return insertAt + 1;
}

bool leaderboard_isPersistent(void) {
    return fatAvailable;
}
