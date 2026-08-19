#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <nds.h>
#include "game.h"

#define LEADERBOARD_SIZE 5

typedef struct {
    int score;
    int level;
    char initials[INITIALS_LEN + 1]; // +1 for the null terminator
} LeaderboardEntry;

extern LeaderboardEntry leaderboard[LEADERBOARD_SIZE];
extern int leaderboardCount;

// Tries to mount the filesystem (libfat) and load a previously saved
// leaderboard from "/StarCatcher.sav". If that fails (no flashcard/SD,
// no file yet, running on an emulator without FAT support, ...) the
// leaderboard just starts empty and stays in-memory only for this session.
void leaderboard_init(void);

// True if `score` would take a spot in the top LEADERBOARD_SIZE entries.
// Use this to decide whether to show the initials-entry keyboard at all.
bool leaderboard_wouldQualify(int score);

// Inserts (score, level, initials) into the leaderboard if it qualifies.
// Returns the 1-based rank it was inserted at, or 0 if it did not make the
// list. Persists to disk when possible. `initials` is copied and safely
// truncated/padded to INITIALS_LEN characters.
int leaderboard_submit(int score, int level, const char* initials);

// True if leaderboard_init() successfully mounted a filesystem, i.e.
// leaderboard_submit() actually writes "/StarCatcher.sav" to persistent
// storage. False means the leaderboard only lives in RAM for this session
// (no flashcard/SD, or the emulator has none configured).
bool leaderboard_isPersistent(void);

#endif // LEADERBOARD_H
