/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 drv_wav.c

 Exports a RIFF WAVE file affixed the name "music.wav"
 
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

#define WAVBUFFERSIZE 8192

// -------------------------------------------------------------------------------------
//
typedef struct WAV_LOCALINFO
{
    MMSTREAM  *wavout;
    SBYTE     *WAV_DMABUF;
    ULONG      dumpsize;

} WAV_LOCALINFO;

static uint drvwav_def_mode = DMODE_INTERP | DMODE_NOCLICK;

// _____________________________________________________________________________________
//
static BOOL WAV_IsThere(void)
{
    return 1;
}

// _____________________________________________________________________________________
//
static BOOL WAV_Init( MDRIVER *md )
{
    WAV_LOCALINFO  *hwdata;

    hwdata = _mmobj_allocblock( md, WAV_LOCALINFO );
    if(NULL == (hwdata->wavout = _mm_fopen("music.wav", "wb"))) goto InitError;
    if(NULL == (hwdata->WAV_DMABUF = _mm_malloc( &md->allochandle, WAVBUFFERSIZE ))) goto InitError;

    md->device.vc = VC_Init( &md->allochandle );
    if(!md->device.vc) goto InitError;

    VC_SetOption( md, MD_OPT_FLAGS,    drvwav_def_mode );
    VC_SetOption( md, MD_OPT_MIXSPEED, 48000 );
    VC_SetOption( md, MD_OPT_CHANNELS, MD_STEREO );
    VC_SetOption( md, MD_OPT_BITDEPTH, SF_BITS_16 );

    md->device.local = hwdata;
    
    {
    WAV_LOCALINFO  *hwdata = md->device.local;
    VIRTCH         *vc     = md->device.vc;

    _mm_write_stringz( "RIFF    WAVEfmt ", hwdata->wavout );
    _mm_write_I_ULONG( 18, hwdata->wavout );     // length of this RIFF block crap

    _mm_write_I_UWORD( 1, hwdata->wavout );     // microsoft format type
    _mm_write_I_UWORD( vc->channels, hwdata->wavout );
    _mm_write_I_ULONG( vc->mixspeed, hwdata->wavout );
    _mm_write_I_ULONG( vc->mixspeed * vc->channels * vc->bitdepth, hwdata->wavout);

    _mm_write_I_UWORD( vc->bitdepth * vc->channels, hwdata->wavout);    // block alignment (8/16 bit)

    _mm_write_I_UWORD( vc->bitdepth, hwdata->wavout );
    _mm_write_I_UWORD( 0, hwdata->wavout );      // No extra data here.

    _mm_write_stringz( "data    " , hwdata->wavout );

    hwdata->dumpsize = 0;
    }
    return TRUE;

InitError:
    _mm_fclose( hwdata->wavout );
    _mmobj_free( md, hwdata->WAV_DMABUF );
    return FALSE;
}

// _____________________________________________________________________________________
//
static void WAV_Exit( MDRIVER *md )
{
    WAV_LOCALINFO  *hwdata = md->device.local;

    VC_Exit( md->device.vc );

    // write in the actual sizes now

    if( hwdata->wavout )
    {
        _mm_fseek(hwdata->wavout,4,SEEK_SET);
        _mm_write_I_ULONG(hwdata->dumpsize + 34, hwdata->wavout);
        _mm_fseek(hwdata->wavout,42,SEEK_SET);
        _mm_write_I_ULONG(hwdata->dumpsize, hwdata->wavout);

        _mm_fclose(hwdata->wavout);
    }
    _mmobj_free( md, hwdata->WAV_DMABUF );
}

// _____________________________________________________________________________________
//
static void WAV_Update(MDRIVER *md)
{
    WAV_LOCALINFO  *hwdata = md->device.local;

    VC_WriteBytes(md, hwdata->WAV_DMABUF, WAVBUFFERSIZE);
    _mm_write_SBYTES(hwdata->WAV_DMABUF, WAVBUFFERSIZE, hwdata->wavout);
    hwdata->dumpsize += WAVBUFFERSIZE;
}

// _____________________________________________________________________________________
//
static BOOL WAV_SetSoftVoices(MDRIVER *md, uint voices)
{
    return VC_SetSoftVoices(md->device.vc, voices);
}


MD_DEVICE drv_wav =
{   
    "music.wav file",
    "WAV [music.wav] file output driver v1.0",
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
    WAV_IsThere,
    WAV_Init,
    WAV_Exit,
    WAV_Update,
    VC_PlayStart,
    VC_PlayStop,

    NULL,
    WAV_SetSoftVoices,

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
    VC_VoiceGetPosition
};

