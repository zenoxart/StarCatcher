#include <stdio.h>
#include <filesystem.h>
#include <maxmod9.h>

#include "music.h"

#define MUSIC_SAMPLE_RATE    16000
#define MUSIC_BUFFER_SAMPLES 4096 // ~256ms of slack against card-read/timing jitter

static FILE* musicFile = NULL;

// Called periodically by maxmod's own background thread (we use automatic
// streaming, see mmStreamOpen below) to refill the audio buffer. Reads
// straight from the NitroFS file each time and seeks back to the start on
// EOF for seamless looping.
//
// NOTE: this reads via stdio from a background thread while the main thread
// occasionally does its own file I/O (leaderboard save/load). NitroFS and
// libfat are logically separate filesystems (ROM-embedded vs. SD/flashcard),
// so this should be safe in practice, but it hasn't been verified on real
// hardware.
static mm_word musicStreamCallback(mm_word length, mm_addr dest, mm_stream_formats format) {
    if (!musicFile) {
        return 0;
    }

    size_t bytesWanted = (size_t)length * sizeof(short);
    size_t bytesRead = fread(dest, 1, bytesWanted, musicFile);

    if (bytesRead < bytesWanted) {
        // Reached end of file mid-buffer: loop back to the start and fill the rest.
        fseek(musicFile, 0, SEEK_SET);
        size_t remaining = bytesWanted - bytesRead;
        size_t more = fread((unsigned char*)dest + bytesRead, 1, remaining, musicFile);
        bytesRead += more;
    }

    return (mm_word)(bytesRead / sizeof(short));
}

bool music_init(void) {
    if (!nitroFSInit(NULL)) {
        return false;
    }

    musicFile = fopen("nitro:/music.raw", "rb");
    if (!musicFile) {
        return false;
    }

    // No soundbank needed - we're streaming raw PCM, not playing
    // modules/samples, matching devkitPro's own streaming example.
    //
    // IMPORTANT: unlike that example, we explicitly set fifo_channel rather
    // than leaving it uninitialized. This devkitPro toolchain's ARM7 core
    // uses calico, whose PXI channels 0-22 are reserved for specific system
    // services (5 = touch, 6 = sound, 7 = mic, ...; see
    // calico/nds/pxi.h). A garbage stack value here could alias one of
    // those reserved channels and corrupt/stall its traffic - which lined
    // up exactly with melonDS locking up specifically when the touch
    // keyboard was used. Channels 23-30 are explicitly documented as free
    // for user code, so we use the first of those instead.
    mm_ds_system sys;
    sys.mod_count = 0;
    sys.samp_count = 0;
    sys.mem_bank = 0;
    sys.fifo_channel = 23; // PxiChannel_User0
    mmInit(&sys);

    mm_stream stream;
    stream.sampling_rate = MUSIC_SAMPLE_RATE;
    stream.buffer_length = MUSIC_BUFFER_SAMPLES;
    stream.callback = musicStreamCallback;
    stream.format = MM_STREAM_16BIT_MONO;
    stream.thread_stack_size = 0; // default
    stream.manual = false;        // maxmod runs its own background thread
    mmStreamOpen(&stream);

    return true;
}
