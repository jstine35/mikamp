/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 virtch.h
 
*/

#pragma once

typedef struct _VIRTCH  VIRTCH;
typedef struct _VINFO   VINFO;
typedef struct _VMIXER  VMIXER;

// -------------------------------------------------------------------------------------
// The resonant filter information block.  This contains the current running information
// for a resonant filter.  Normally this structure is attached to each of the channels.
// The use of 64 bit integers hiere is a necessity!  Filters require a high level of ac-
// curacy which just can't be fit in 32 bits.
//
typedef struct VC_RESFILTER
{
    long     resfactor, cofactor;   // these are pregen values based on the resonance and cutoff.
    INT64S   speed, pos;            // running-sum type values, used and modded by thr mixers.

} VC_RESFILTER;

// -------------------------------------------------------------------------------------
//
struct _VMIXER
{   
    VMIXER *next;

    CHAR   *name;            // name and version of this mixer!

    BOOL  (*Check)( uint mixmode, uint format, uint flags );
    enum SF_BITS     bitdepth;
    enum MD_CHANNELS channels_sample;   // sample type (mono/stereo)
    enum MD_CHANNELS channels_output;   // output type (mono/stereo)

    BOOL  (*Init)( struct _VMIXER *mixer );
    void  (*Exit)( struct _VMIXER *mixer );
    void  (*CalculateVolumes)( VIRTCH *vc, VINFO *vnf );
    void  (*RampVolume)( VINFO *vnf, int done );

    void  (__cdecl *NoClick)( void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo );
    void  (__cdecl *NoClickSurround)( void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo );

    void  (__cdecl *Mix)( void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo );
    void  (__cdecl *MixSurround)( void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo );

    MMVOLUME  *vol;
    void      *mixdat;

};

// -------------------------------------------------------------------------------------
//
typedef struct VSAMPLE
{   
    VMIXER  *mixer;

    uint     flags;        // looping flags (used for resetting sustain loops only)
    uint     format;       // dataformat flags (16bit, stereo) -- *unchangeable*
    enum SF_BITS     bitdepth;
    enum MD_CHANNELS channels;
    SBYTE   *data;

} VSAMPLE;

#define VC_SURROUND   1


// -------------------------------------------------------------------------------------
//
struct _VINFO
{   
    // These should be first, since they benefit from 8 byte alignment
    // (or, well, as according to Intel's VTune).

    INT64S    current;        // current index in the sample
    INT64S    increment;      // fixed-point increment value

    VC_RESFILTER resfilter;
    int       cutoff,
              resonance;

    // Local instance sample information

    BOOL      onhold;           // suspended voices, but not yet dead!
    int       samplehandle;     // sample handle by number (indexes vsample[])

    uint      flags;            // sample flags
    uint      size;             // sample size (length) in samples
    int       reppos,           // loop start
              repend;           // loop end
    int       suspos,           // sustain loop start
              susend;           // sustain loop end

    uint      start;            // start index
    BOOL      kick;             // =1 -> sample has to be restarted
    ULONG     frq;              // current frequency

    int       panflg;           // Panning Flags (VC_SURROUND, etc)
    MMVOLUME  volume;

    // micro volume ramping (declicker)
    int       volramp;
    MMVOLUME  vol, oldvol;

    // Virtch VoiceUpdate Callback Features
    // ------------------------------------

    struct VC_CALLBACK
    {
        long    pos;             // specific position (in samples) to trigger at
        void  (*proc)( SBYTE *dest, void *userinfo );
        void   *userinfo;

    } callback;

    UBYTE     loopbuf[32];       // 32 byte loop buffer for clickless looping
};

// -------------------------------------------------------------------------------------
//
struct _VIRTCH
{
    MM_ALLOC   allochandle;
    BOOL       initialized;

    uint       mode;
    enum SF_BITS     bitdepth;
    enum MD_CHANNELS channels;

    uint       mixspeed;
    uint       cpu;
    uint       numchn;                 // software mixing voices
    uint       memory;

    uint       clickcnt;               // number of channels in the vold decliker buffer.
    MMVOLUME   volume;

    uint       samplehandles;

    VINFO     *vinf, *vold;
    VSAMPLE   *sample;

    long       TICK, TICKLEFT, TICKREMAIN, samplesthatfit;
    SLONG     *TICKBUF;
    volatile BOOL preemption;         // see explaination of preemption in the mikamp docs.

    VMIXER    *mixerlist;             // registered mixers available for use

};


#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
****** Virtual channel stuff: ********************************************
**************************************************************************/

#define VC_MAXVOICES 0xfffffful

extern VIRTCH *VC_Init( MM_ALLOC *parent );
extern void    VC_Exit(VIRTCH *vc);
extern void    VC_Preempt(MD_DEVICE *md);
extern BOOL    VC_SetSoftVoices(VIRTCH *vc, uint num);
extern void    VC_SetOption( MDRIVER *md, enum MD_OPTIONS option, uint value );
extern uint    VC_GetOption( MDRIVER *md, enum MD_OPTIONS option );
extern BOOL    VC_PlayStart( MDRIVER *md );
extern void    VC_PlayStop( MDRIVER *md );
extern void    VC_RegisterMixer(VIRTCH *vc, VMIXER *mixer);
extern void    VC_RegisterStereoMixers(VIRTCH *vc);
extern void    VC_RegisterModuleMixers(VIRTCH *vc);

extern void    VC_SetVolume(MD_DEVICE *md, const MMVOLUME *volume);
extern void    VC_GetVolume(MD_DEVICE *md, MMVOLUME *volume);

extern ULONG   VC_SampleSpace(MD_DEVICE *md, int type);
extern ULONG   VC_SampleLength(MD_DEVICE *md, SM_SAMPLE *s);
extern void   *VC_SampleGetPtr(MD_DEVICE *md, uint handle);
extern int     VC_SampleAlloc(MD_DEVICE *md, SM_SAMPLE *samp);
extern void    VC_SampleLoad(MD_DEVICE *md, SAMPLE_LOADER *sload, SL_SAMPLE *samp);
extern void    VC_SampleUnload(MD_DEVICE *md, uint handle);
extern uint    VC_GetSampleCaps( MDRIVER *md, enum MD_CAPS captype );
extern uint    VC_GetSampleFormat( MDRIVER *md, int handle );

extern int     VC_GetActiveVoices(MD_DEVICE *md);

extern void    VC_VoiceSetVolume(MD_DEVICE *md, uint voice, const MMVOLUME *volume);
extern void    VC_VoiceGetVolume(MD_DEVICE *md, uint voice, MMVOLUME *volume);
extern void    VC_VoiceSetFrequency(MD_DEVICE *md, uint voice, ulong frq);
extern ulong   VC_VoiceGetFrequency(MD_DEVICE *md, uint voice);
extern void    VC_VoiceSetPosition(MD_DEVICE *md, uint voice, ulong pos);
extern ulong   VC_VoiceGetPosition(MD_DEVICE *md, uint voice);

extern void    VC_VoiceSetResonance(MD_DEVICE *md, uint voice, int cutoff, int resonance);
extern void    VC_VoiceSetSurround(MD_DEVICE *md, uint voice, int flags);

extern void    VC_VoicePlay(MD_DEVICE *md, uint voice, uint handle, long start, uint length, int reppos, int repend, int suspos, int susend, uint flags);
extern void    VC_VoiceResume(MD_DEVICE *md, uint voice);

extern void    VC_VoiceStop(MD_DEVICE *md, uint voice);
extern BOOL    VC_VoiceStopped(MD_DEVICE *md, uint voice);
extern void    VC_VoiceReleaseSustain(MD_DEVICE *md, uint voice);
extern ULONG   VC_VoiceRealVolume(MD_DEVICE *md, uint voice);

// These are functions the drivers use to update the mixing buffers.

extern void    VC_WriteSamples(MDRIVER *md, SBYTE *buf, long todo);
extern ULONG   VC_WriteBytes(MDRIVER *md, SBYTE *buf, long todo);
extern void    VC_SilenceBytes(MDRIVER *md, SBYTE *buf, long todo);

//extern void    VC_EnableInterpolation(VIRTCH *vc, int handle);
//extern void    VC_DisableInterpolation(VIRTCH *vc, int handle);


// =====================================
//    Mikamp Dynamic Pluggable Mixers
// =====================================

extern VMIXER VC_MIXER_PLACEBO;             // the do-nothing mixer, (c) Creative Silence

// Default 'C' Mixers.
// -------------------

extern VMIXER M8_MONO_INTERP, M8_STEREO_INTERP,
              S8_MONO_INTERP, S8_STEREO_INTERP,
              M8_MONO, M8_STEREO,
              S8_MONO, S8_STEREO;

extern VMIXER M16_MONO_INTERP, M16_STEREO_INTERP,
              S16_MONO_INTERP, S16_STEREO_INTERP,
              M16_MONO, M16_STEREO,
              S16_MONO, S16_STEREO;

// With resonant filters!

extern VMIXER RF_M8_MONO_INTERP, RF_M8_STEREO_INTERP,
              RF_S8_MONO_INTERP, RF_S8_STEREO_INTERP,
              RF_M8_MONO, RF_M8_STEREO,
              RF_S8_MONO, RF_S8_STEREO;

extern VMIXER RF_M16_MONO_INTERP, RF_M16_STEREO_INTERP,
              RF_S16_MONO_INTERP, RF_S16_STEREO_INTERP,
              RF_M16_MONO, RF_M16_STEREO,
              RF_S16_MONO, RF_S16_STEREO;


// Intel Assembly Mixers.

extern VMIXER ASM_M8_MONO_INTERP, ASM_M8_STEREO_INTERP,
              ASM_S8_MONO_INTERP, ASM_S8_STEREO_INTERP,
              ASM_M8_MONO, ASM_M8_STEREO,
              ASM_S8_MONO, ASM_S8_STEREO;

extern VMIXER ASM_M16_MONO_INTERP, ASM_M16_STEREO_INTERP,
              ASM_S16_MONO_INTERP, ASM_S16_STEREO_INTERP,
              ASM_M16_MONO, ASM_M16_STEREO,
              ASM_S16_MONO, ASM_S16_STEREO;

#ifdef __cplusplus
}
#endif

