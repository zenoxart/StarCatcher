#ifndef MUSIC_H
#define MUSIC_H

#include <nds.h>

// Starts NitroFS and opens a looping maxmod audio stream that reads
// "nitro:/music.raw" (16kHz, mono, 16-bit signed PCM) a chunk at a time.
// The whole song is ~5.7MB - far too big to fit in the 4MB main RAM budget
// alongside everything else if simply embedded in the ARM9 binary, so it's
// streamed on demand from the cart/flashcard instead via NitroFS.
//
// Call once near the start of main(), after basic video/OAM setup. Returns
// false if NitroFS or the stream failed to start (e.g. running on hardware
// without accessible NitroFS/FAT support) - the game still runs fine
// without music in that case, it just stays silent.
bool music_init(void);

#endif // MUSIC_H
