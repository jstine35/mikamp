/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 drv_raw.c

 Mikamp driver for output to a file called MUSIC.RAW

*/

#include "mikamp.h"
#include "virtch.h"

#ifdef __GNUC__
#include <sys/types.h>
#else
#include <io.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

#define RAWBUFFERSIZE 8192

// -------------------------------------------------------------------------------------
//
typedef struct RAW_LOCALINFO
{
    int     rawout;
    SBYTE   RAW_DMABUF[RAWBUFFERSIZE];
} RAW_LOCALINFO;

// _____________________________________________________________________________________
//
static BOOL RAW_IsThere(void)
{
    return 1;
}

// _____________________________________________________________________________________
//
static BOOL RAW_Init( MDRIVER *md )
{
    RAW_LOCALINFO  *hwdata;

    hwdata = _mmobj_allocblock( md, RAW_LOCALINFO );

    if(-1 == (hwdata->rawout = open("music.raw", 
#ifndef __GNUC__
                O_BINARY | 
#endif
                O_RDWR | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE)))
    {
        _mmobj_free( md, hwdata );
        return FALSE;
    }

    md->device.vc = VC_Init( &md->allochandle );
    if( !md->device.vc )
    {
        _mmobj_free( md, hwdata );
        return FALSE;
    }

    md->device.local = hwdata;

    return TRUE;
}

// _____________________________________________________________________________________
//
static void RAW_Exit(MDRIVER *md)
{
    RAW_LOCALINFO  *hwdata = md->device.local;

    VC_Exit(md->device.vc);
    close(hwdata->rawout);
}

// _____________________________________________________________________________________
//
static void RAW_Update(MDRIVER *md)
{
    RAW_LOCALINFO  *hwdata = md->device.local;

    VC_WriteBytes(md, hwdata->RAW_DMABUF, RAWBUFFERSIZE);
    write(hwdata->rawout, hwdata->RAW_DMABUF, RAWBUFFERSIZE);
}

// _____________________________________________________________________________________
//
static BOOL RAW_SetSoftVoices(MDRIVER *md, uint voices)
{
    return VC_SetSoftVoices(md->device.vc, voices);
}


MD_DEVICE drv_raw =
{   
    "music.raw file",
    "RAW [music.raw] file output driver v1.0",
    0,VC_MAXVOICES,

    NULL,
    NULL,
    NULL,

    // Sample loading
    VC_SampleAlloc,
    VC_SampleGetPtr,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    VC_GetSampleCaps,
    VC_GetSampleFormat,

    // Detection and initialization
    RAW_IsThere,
    RAW_Init,
    RAW_Exit,
    RAW_Update,
    VC_PlayStart,
    VC_PlayStop,
    VC_Preempt,

    NULL,
    RAW_SetSoftVoices,

    VC_SetOption,
    VC_GetOption,

    VC_SetVolume,
    VC_GetVolume,

    // Voice control and voice information
    VC_GetActiveVoices,

    VC_VoiceSetVolume,
    VC_VoiceGetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceGetFrequency,
    VC_VoiceSetPosition,
    VC_VoiceGetPosition,
    VC_VoiceSetSurround,
    VC_VoiceSetResonance,

    VC_VoicePlay,
    VC_VoiceResume,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceReleaseSustain,

    VC_VoiceRealVolume
};

