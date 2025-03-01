/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 drv_nos.c

 Mikamp driver for no output on any soundcard, monitor, keyboard, or whatever :)

*/

#include "mikamp.h"
#include <string.h>


static BOOL NS_IsThere(void)
{
    return 1;
}


static int NS_SampleAlloc( MD_DEVICE *device, SM_SAMPLE *s )
{
    return 0;
}

static void NS_SampleLoad( MD_DEVICE *device, SAMPLE_LOADER *sload, SL_SAMPLE *samp )
{
}

static void NS_SampleUnload(MD_DEVICE *device, uint h)
{
}


static ULONG NS_SampleSpace(MD_DEVICE *device, int type)
{
    return 0;
}


static ULONG NS_SampleLength(MD_DEVICE *device, SM_SAMPLE *s)
{
    return 0;
}


uint NS_GetSampleCaps( MDRIVER *md, enum MD_CAPS captype )
{
    return 0;
}


uint NS_GetSampleFormat( MDRIVER *md, int handle )
{
    return 0;
}


static BOOL NS_Init( MDRIVER *md )
{
    return TRUE;
}


static void NS_Exit(MDRIVER *md)
{
}


static void NS_Update(MDRIVER *md)
{
}


BOOL NS_PlayStart( MDRIVER *md )
{
   return TRUE;
}


void NS_PlayStop( MDRIVER *md )
{
}


static BOOL NS_SetHardVoices(MDRIVER *md, uint num)
{
    return 0;
}

static BOOL NS_SetSoftVoices(MDRIVER *md, uint num)
{
    return 0;
}


static uint NS_GetOption( MDRIVER *md, enum MD_OPTIONS option )
{
    return 0;
}


static void NS_SetOption( MDRIVER *md, enum MD_OPTIONS option, uint value )
{
}


static void NS_VoiceSetVolume(MD_DEVICE *device, uint voice, const MMVOLUME *volume)
{
}

static void NS_VoiceGetVolume(MD_DEVICE *device, uint voice, MMVOLUME *volume)
{
    memset(volume,0,sizeof(MMVOLUME));
}


static void NS_VoiceSetFrequency(MD_DEVICE *device, uint voice,ULONG frq)
{
}

static ULONG NS_VoiceGetFrequency(MD_DEVICE *device, uint voice)
{
    return 0;
}


static void NS_VoicePlay(MD_DEVICE *device, uint voice, uint handle, long start, uint length, int reppos, int repend, int suspos, int susend, uint flags)
{
}


static void NS_VoiceStop(MD_DEVICE *device, uint voice)
{
}


static BOOL NS_VoiceStopped(MD_DEVICE *device, uint voice)
{
   return 0;
}


static void NS_VoiceReleaseSustain(MD_DEVICE *device, uint voice)
{
}


static void NS_VoiceSetPosition(MD_DEVICE *device, uint voice, ulong pos)
{
}

static ulong NS_VoiceGetPosition(MD_DEVICE *device, uint voice)
{
   return 0;
}

static void NS_VoiceSetSurround(MD_DEVICE *device, uint voice, int flags)
{

}

static int NS_GetActiveVoices(MD_DEVICE *device)
{
   return 0;
}


static ULONG NS_VoiceRealVolume(MD_DEVICE *device, uint voice)
{
   return 0;
}


static void NS_SetVolume(MD_DEVICE *device, const MMVOLUME *volume)
{

}

static BOOL NS_GetVolume(MD_DEVICE *device, MMVOLUME *volume)
{
    return 0;
}

static BOOL NS_SetMode(MDRIVER *md, uint mixspeed, uint mode, uint channels)
{
    return 0;
}

static void NS_GetMode(MDRIVER *md, uint *mixspeed, uint *mode, uint *channels)
{
    *mixspeed = 0; *mode = 0; *channels = 0;
}

static void NS_VoiceResume(MD_DEVICE *device, uint voice)
{

}

static void *NS_SampleGetPtr(MD_DEVICE *device, uint length)
{
    return NULL;
}

static void NS_VoiceSetResonance(MD_DEVICE *device, uint voice, int cutoff, int resonance)
{

}


MD_DEVICE drv_nos =
{   
    "No Sound",
    "Nosound Driver v2.0 - (c) Creative Silence",
    0,0,

    NULL,
    NULL,
    NULL,

    // Sample loading
    NS_SampleAlloc,
    NS_SampleGetPtr,
    NS_SampleLoad,
    NS_SampleUnload,
    NS_SampleSpace,
    NS_SampleLength,
    NS_GetSampleCaps,
    NS_GetSampleFormat,

    // Detection and initialization
    NS_IsThere,
    NS_Init,
    NS_Exit,
    NS_Update,
    NS_PlayStart,
    NS_PlayStop,

    NS_SetHardVoices,
    NS_SetSoftVoices,

    NS_SetOption,
    NS_GetOption,
    NS_SetVolume,
    NS_GetVolume,

    // Voice control and voice information
    NS_GetActiveVoices,
    NS_VoiceSetVolume,
    NS_VoiceGetVolume,
    NS_VoiceSetFrequency,
    NS_VoiceGetFrequency,
    NS_VoiceSetPosition,
    NS_VoiceGetPosition,

    NS_VoiceSetSurround,
    NS_VoiceSetResonance,

    NS_VoicePlay,
    NS_VoiceResume,
    NS_VoiceStop,
    NS_VoiceStopped,
    NS_VoiceReleaseSustain,
    NS_VoiceRealVolume
};

