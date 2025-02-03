
#ifndef _MDSFX_H_
#define _MDSFX_H_

#include "mikamp.h"

typedef struct _MD_SAMPLE MD_SAMPLE;

// -------------------------------------------------------------------------------------
//
// Notes:
//  - Modifying the length without reloading the sampledata properly can result in pops
//    or clicks when the sample ends.  All other values can be modified with few or no
//    adverse effects.
//
struct _MD_SAMPLE
{
    MDRIVER *md;             // driver & device this sample is bound to
    int      handle;         // driver sample allocation handle

    // The procedure used by mdsfx_playeffect, to pick a free voice from the
    // given voiceset.

    int    (*findvoice_proc)(MD_VOICESET *vs, uint flags);
    
    // These represent the default (overridable) values for playing the given sample:
    // Use the Voice_* API to change these attributes at run-time.

    uint     speed;           // default playback speed (almost anything is legal)
    int      volume;          // default volume.  0 to 128 volume range
    int      panning;         // default panning. PAN_LEFT to PAN_RIGHT range

    int      length;          // length of the sample, in samples.
    int      reppos,          // loopstart position (in samples)
             repend;          // loopend position (in samples)
    int      suspos,          // sustain loopstart (in samples)
             susend;          // sustain loopend (in samples)

    uint     flags;           // sample-looping and other flags.

    int      cutoff,          // the cutoff frequency (range 0 to 256)
             resonance;       // the resonance factor (range 0 to 256)

    void    *respak;          // resource package pointer (used by gamework's respak, or your own shizat)
};


#ifdef __cplusplus
extern "C" {
#endif

MMEXPORT MD_SAMPLE *mdsfx_loadwavfp(MDRIVER *md, MMSTREAM *mmfp);
MMEXPORT MD_SAMPLE *mdsfx_loadwav(MDRIVER *md, const CHAR *fn);

MMEXPORT MD_SAMPLE *mdsfx_create(MDRIVER *md);
MMEXPORT MD_SAMPLE *mdsfx_duplicate(MD_SAMPLE *src);
MMEXPORT void       mdsfx_free(MD_SAMPLE *samp);
MMEXPORT void       mdsfx_play(MD_SAMPLE *s, MD_VOICESET *vs, uint voice, int start);
MMEXPORT int        mdsfx_playeffect(MD_SAMPLE *s, MD_VOICESET *vs, uint start, uint flags);

#ifdef __cplusplus
}
#endif

#endif

