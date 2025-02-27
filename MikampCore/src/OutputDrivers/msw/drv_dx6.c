/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 drv_dx6.c
 
 Mikamp driver for output via directsound

 Original code by Matthew Gambrell
 Improved, optimized, and bugfixes by Jake Stine (Air) and Jan L�nnberg
*/

/*
Notes about Directsound:

 Directsound is kinda funny about working in cooperative and exclusive
 modes, since it doesn't really behave how the docs would indicate (at
 least by how I read them).

 Our primary buffer always has to be DSSCL_PRIMARY, otherwise we can-
 not alter the mixer settings.

 In exclusive mode, we set the primary buffer to DSSCL_EXCLUSIVE,
 which keeps the mixer from continuing while the game is in the back-
 ground.  Without it, some drivers will become unstable and crash,
 and the music will play but not be heard (usually undesired).

 In Cooperative mode, the secondary buffer has to be flagged with
 DSCAPS_GLOBALFOCUS, which works perfectly fine for any soundcard
 (or driver) which supports playing multiple waveforms.  This will
 not work on SB16s and other like soundcards, but well, they're
 used to getting shafted.

 NOTE:
    Currently the primary buffer is *forced* to 44100 khz.  This should
    be changed to be smarter: to set itself to the max output of the
    driver's capabilities.

*/

/* This driver had a nasty habit of overwriting the playing sound
   data causing horrible glitches in the sound output. This was
   fixed by changing the driver to synchronize its buffer writes
   with playback instead of just writing more or less to any old
   part of the buffer. Also fixed problem with fixed latency.
     - Jan L�nnberg. */

//#ifdef HAVE_CONFIG_H
//#include "config.h"
//#endif

#define NO_STRICT

#include <windows.h>

#include "mikamp.h"
#include "virtch.h"

#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>

#define MINIMUM_LATENCY     (30)        // was 10
#define BUFFER_SIZE         (2000)
#define LAG_INCREMENT       (64)        // was 16
#define ERROR_TOLERANCE     (2500)

static void DS_WipeBuffers( MDRIVER *md );

static uint dx6_def_mixmode =       // assign "default" mikamp behaviors (everything enabled)
    DMODE_SOFTWARE | DMODE_INTERP | DMODE_NOCLICK | DMODE_RESONANCE | DMODE_SURROUND;
                              
// -------------------------------------------------------------------------------------
// New and improved instance-ized directsound localinfo crap, which is allocated and
// attached to the MD_DEVICE info block during the call to DS_INIT (called from Mikamp_Init).
//
typedef struct DS_LOCALINFO
{
    HINSTANCE           dll;

    LPDIRECTSOUND       ds;
    LPDIRECTSOUNDBUFFER pb;
    LPDIRECTSOUNDBUFFER bb;
    DSCAPS              DSCaps;

    uint         mode;
    uint         mixspeed;
    enum MD_CHANNELS channels;
    enum SF_BITS     bitdepth;

    int     dorefresh;          // enables buffer writes

    int     bufsize;
    int     samplesize;
    int     lag;                // allowed lag threshold
    int     currpos;
    int     lastwrite;
    int     lastwritecursor;
    int     lasterror;          // timer for measuring lag between updates

    SBYTE  *nullbuf;            // null buffer used for mixing "lost bits" of music

} DS_LOCALINFO;

// _____________________________________________________________________________________
//
static void logerror( const CHAR *function, int code )
{
    static CHAR *error;

    switch (code)
    {
        case E_NOINTERFACE:
            error = "Unsupported interface (DirectX version incompatability)";
        break;

        case DSERR_ALLOCATED:
            // This one gets a special action since the user may want to be
            // informed that their sound is tied up.
            error = "Audio device in use.";
            _mmerr_set(MMERR_INITIALIZING_DEVICE, "DirectSound Initialization Failure", "The audio device is already in use and I'm not allowed to use it.  Woops!");
        break;

        case DSERR_BADFORMAT:
            error = "Unsupported audio format";
        break;

        case DSERR_BUFFERLOST:
            error = "Mixing buffer was lost";
        break;

        case DSERR_INVALIDPARAM:
            error = "Invalid parameter";
        break;

        case DSERR_NODRIVER:
            error = "No audio device found";
        break;

        case DSERR_OUTOFMEMORY:
            error = "DirectSound ran out of memory";
        break;

        case DSERR_PRIOLEVELNEEDED:
            error = "Caller doesn't have priority";
        break;

        case DSERR_UNSUPPORTED:
            error = "Function not supported (DirectX driver inferiority)";
        break;
    }
    _mmlog( "Mikamp > drv_ds > %s : %s", function, error );
    return;
}

// _____________________________________________________________________________________
//
static BOOL DS_IsPresent(void)
{
    HINSTANCE dsdll;
    int       ok;

    // Version check DSOUND.DLL
    ok = 0;
    dsdll = LoadLibrary("DSOUND.DLL");
    
    if (dsdll)
    {
        // DirectSound available.  Good.
        
        ok = 1;
        
        /*  Note: NT has latency problems, it seems.  We may want to uncomment
            this code and force NT users to drop back to winmm audio.
        
        OSVERSIONINFO ver;
        ver.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        GetVersionEx(&ver);
        switch (ver.dwPlatformId)
        {   case VER_PLATFORM_WIN32_NT:
                if (ver.dwMajorVersion > 4)
                {   // Win2K
                    ok = 1;
                } else
                {   // WinNT
                    ok = 0;
                }
            break;

            default:
                // Win95 or Win98
                dsound_ok = 1;
            break;
        }
        */

        FreeLibrary(dsdll);
    }
    
    return ok;
}

// _____________________________________________________________________________________
//
static BOOL DS_Init( MDRIVER *md )
{
    HRESULT      (WINAPI *DSoundCreate)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
    HRESULT       hres;
    DS_LOCALINFO *hwdata;

    hwdata      = _mmobj_allocblock( md, DS_LOCALINFO );
    hwdata->dll = LoadLibrary("DSOUND.DLL");
    
    if( !hwdata->dll )
    {
        _mmlog( "Mikamp > drv_ds > Failed loading DSOUND.DLL!" );
        goto InitError;
    }

    // we safely assume that the user detected the dsound first.
    DSoundCreate = (void *)GetProcAddress(hwdata->dll, "DirectSoundCreate");

    //SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS); //HIGH_PRIORITY_CLASS);
    //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    
    hres = DSoundCreate( NULL, &hwdata->ds, NULL );
    if( hres != DS_OK )
    {
        logerror( "DSoundCreate", hres );
        goto InitError;
    }

    if( !md->device.vc )
    {
        md->device.vc = VC_Init( &md->allochandle );
        if( !md->device.vc ) goto InitError;
    }

    md->device.local = hwdata;
    //hwdata->mutex    = _mmforbid_init();
    return TRUE;

InitError:
    if( hwdata->ds ) IDirectSound_Release( hwdata->ds );
    _mmobj_free( md, hwdata );
    return FALSE;
}

// _____________________________________________________________________________________
//
static void DS_Exit( MDRIVER *md )
{
    DS_LOCALINFO  *hwdata = md->device.local;

    if( !hwdata ) return;

    //_mmforbid_deinit( hwdata->mutex );
    if( hwdata->dll )
    {
        if( hwdata->bb ) IDirectSoundBuffer_Stop( hwdata->bb );

        VC_Exit(md->device.vc);

        if( hwdata->bb ) IDirectSoundBuffer_Release( hwdata->bb );
        if( hwdata->pb ) IDirectSoundBuffer_Release( hwdata->pb );
        if( hwdata->ds ) IDirectSound_Release( hwdata->ds );

        FreeLibrary( hwdata->dll );
    }

    _mmobj_free( md, hwdata );
}

static DWORD last;

// _____________________________________________________________________________________
//
static void DS_Update(MDRIVER *md)
{
    DS_LOCALINFO  *hwdata = md->device.local;
    HRESULT        h;
    DWORD          play, write;
    DWORD          ptr1len, ptr2len;
    void          *ptr1, *ptr2;
    int            diff, errordiff, thistime;

    if( !hwdata->dorefresh ) return;

    IDirectSoundBuffer_GetCurrentPosition( hwdata->bb, &play, &write );

    diff = (int)write - hwdata->lastwritecursor;
    if(diff < 0) diff += hwdata->bufsize;

    hwdata->lastwritecursor = (int)write;

    if( diff > hwdata->lag )
    {
        thistime  = (int)timeGetTime();
        errordiff = thistime - hwdata->lasterror;

        //_mmlogd( "Mikamp DSound > Buffer Lag : %d > %d", diff, hwdata->lag);

        if( errordiff < ERROR_TOLERANCE )
        {
            hwdata->lag += hwdata->samplesize * LAG_INCREMENT;
            if( hwdata->lag > hwdata->bufsize )
            {
                // we needed more samples worth of lag than BUFFER_SIZE
                // things are never going to work without a larger buffer
                hwdata->lag = hwdata->bufsize;
            }

        }

        hwdata->lasterror = thistime;

        diff = (int)write - hwdata->lastwrite;
        if( diff < 0 )
            diff += hwdata->bufsize;

        VC_WriteBytes( md, hwdata->nullbuf, diff );

        diff = hwdata->lag;
        hwdata->lastwrite = write;

    }

    if( diff > 0 )
    {
        h = IDirectSoundBuffer_Lock(hwdata->bb,hwdata->lastwrite, diff, &ptr1, &ptr1len, &ptr2, &ptr2len, 0);
        if( h == DSERR_BUFFERLOST )
        {
            IDirectSoundBuffer_Restore(hwdata->bb);
            h = IDirectSoundBuffer_Lock(hwdata->bb, hwdata->lastwrite, diff, &ptr1, &ptr1len, &ptr2, &ptr2len, 0);
        }

        if( h == DS_OK )
        {   
            
            VC_WriteBytes( md, ptr1, ptr1len );
            if(ptr2) VC_WriteBytes( md, ptr2, ptr2len );

            IDirectSoundBuffer_Unlock( hwdata->bb, ptr1, ptr1len, ptr2, ptr2len );
        }

        hwdata->lastwrite += diff;
        if( hwdata->lastwrite >= hwdata->bufsize )
            hwdata->lastwrite -= hwdata->bufsize;
    }
}

// _____________________________________________________________________________________
// A simple clearing of the dsound buffer, thus so it only contains silence.
//
static void DS_WipeBuffers( MDRIVER *md )
{
    DS_LOCALINFO  *hwdata = md->device.local;

    if( hwdata->dorefresh )
    {
        DWORD    ptr1len;
        void    *ptr1;
        HRESULT  h;

        h = IDirectSoundBuffer_Lock( hwdata->bb,0,0,&ptr1,&ptr1len, NULL, NULL, DSBLOCK_ENTIREBUFFER );
        if( h == DSERR_BUFFERLOST )
        {
            IDirectSoundBuffer_Restore( hwdata->bb );
            h = IDirectSoundBuffer_Lock( hwdata->bb,0,0,&ptr1,&ptr1len, NULL, NULL, DSBLOCK_ENTIREBUFFER );
        }

        if( h == DS_OK )
        {
            VC_SilenceBytes(md, ptr1, ptr1len);
            IDirectSoundBuffer_Unlock(hwdata->bb,ptr1,ptr1len,0,0);
        }
    }
}

// _____________________________________________________________________________________
//
static void DS_SetOption( MDRIVER *md, enum MD_OPTIONS option, uint value )
{
    DS_LOCALINFO  *hwdata = md->device.local;
    if( !hwdata->ds ) return;

    switch( option )
    {
        case MD_OPT_MIXSPEED:
        {
            if( hwdata->dorefresh )
            {
                _mmlog( "Mikamp > DirectSound > Cannot change mixing speed on-the-fly!" );
                _mmlog( "                     > Use Mikamp_DeviceConfig_Enter() and Exit() to fix this warning." );
            }
            if( hwdata->mixspeed == value ) return;
            hwdata->mixspeed = value;
            if( hwdata->mixspeed > 44100 ) hwdata->mixspeed = 44100;

            VC_SetOption( md, option, hwdata->mixspeed );
        }
        break;

        case MD_OPT_BITDEPTH:
        {
            if( hwdata->dorefresh )
            {
                _mmlog( "Mikamp > DirectSound > Cannot change mixer bitdepth on-the-fly!" );
                _mmlog( "                     > Use Mikamp_DeviceConfig_Enter() and Exit() to fix this warning." );
            }
            if( hwdata->bitdepth == value ) return;
            VC_SetOption( md, option, hwdata->bitdepth );
        }
        break;

        case MD_OPT_CHANNELS:
        {
            if( hwdata->dorefresh )
            {
                _mmlog( "Mikamp > DirectSound > Cannot change mono/stereo status on-the-fly!" );
                _mmlog( "                     > Use Mikamp_DeviceConfig_Enter() and Exit() to fix this warning." );
            }
            if( value > MD_STEREO ) value = MD_STEREO;
            if( hwdata->channels == value ) return;
            hwdata->channels = value;
            VC_SetOption( md, option, hwdata->channels );
        }
        break;

        case MD_OPT_FLAGS:
        {
            BOOL changed_exclusive;

            if( value == DMODE_DEFAULT ) value = dx6_def_mixmode;

            changed_exclusive = ( hwdata->mode & DMODE_EXCLUSIVE ) != ( value & DMODE_EXCLUSIVE );

            if( hwdata->dorefresh && changed_exclusive )
            {
                _mmlog( "Mikamp > DirectSound > Cannot change exclusive/cooperative status on-the-fly!" );
                _mmlog( "                     > Use Mikamp_DeviceConfig_Enter() and Exit() to fix this warning." );
                if( hwdata->mode & DMODE_EXCLUSIVE )
                    value |= DMODE_EXCLUSIVE;
                else
                    value &= ~DMODE_EXCLUSIVE;
            }

            if( hwdata->mode == value ) return;
            hwdata->mode = value;
            VC_SetOption( md, option, hwdata->mode );
        }
        break;

        case MD_OPT_LATENCY:        // ignored for now
        break;

        case MD_OPT_CPU:
            VC_SetOption( md, option, value );
        break;
    }
}

// _____________________________________________________________________________________
// By Mikamp Driver rules, this function should shut down everything related to playing
// sound, but not actually deallocate samples or memory.  It just "stops" things so that
// the user can muck around with driver options without fear of explosions.
//
static void DS_PlayStop( MDRIVER *md )
{
    DS_LOCALINFO  *hwdata = md->device.local;
    if( !hwdata->ds || !hwdata->dorefresh ) return;

    hwdata->dorefresh = FALSE;
    if( hwdata->bb ) IDirectSoundBuffer_Stop( hwdata->bb );
    if( hwdata->bb ) IDirectSoundBuffer_Release( hwdata->bb );
    if( hwdata->pb ) IDirectSoundBuffer_Release( hwdata->pb );
    VC_PlayStop( md );
}

// _____________________________________________________________________________________
// This is the main sound-playing initialization section.
//
static BOOL DS_PlayStart( MDRIVER *md )
{
    DSBUFFERDESC   bf;
    WAVEFORMATEX   pcmwf;
    HRESULT        hres;
    uint           outrate;

    DS_LOCALINFO  *hwdata = md->device.local;

    // Terminal Flaw Check : This function should never be called when dorefresh is
    // already set to true.  That means we've got a coder logic error somewheres.

    assert( !hwdata->dorefresh );
    if( hwdata->dorefresh ) return TRUE;

    hres = hwdata->ds->lpVtbl->SetCooperativeLevel( hwdata->ds, GetForegroundWindow(),DSSCL_PRIORITY | ((hwdata->mode & DMODE_EXCLUSIVE) ? DSSCL_EXCLUSIVE : 0) );
    if( hres != DS_OK )
    {
        logerror( "SetCooperativeLevel", hres );
        return FALSE;
    }

    // Weird code: Some dsound drivers don't like non-44100/22050/11025 khz settings,
    // so for now I force the primary to match one of those three.

    if( hwdata->mixspeed <= 11025 ) outrate = 11025;
    else if( hwdata->mixspeed <= 22050 ) outrate = 22050;
    else if( hwdata->mixspeed <= 44100 ) outrate = 44100;

    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
    pcmwf.wFormatTag      = WAVE_FORMAT_PCM;
    pcmwf.nChannels       = hwdata->channels;
    pcmwf.nSamplesPerSec  = outrate;
    pcmwf.wBitsPerSample  = hwdata->bitdepth * 8;
    pcmwf.nBlockAlign     = ( pcmwf.wBitsPerSample * pcmwf.nChannels ) / 8;
    pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;

    hwdata->samplesize = pcmwf.nBlockAlign;

    // set bufsize and round to nearest samplesize
    hwdata->bufsize  = ( BUFFER_SIZE * hwdata->mixspeed * hwdata->samplesize ) / 1000;
    hwdata->bufsize += (hwdata->samplesize - (hwdata->bufsize % hwdata->samplesize)) % hwdata->samplesize;

    // allocate scratch buffer for dumping lost samples
    _mmobj_free( md, hwdata->nullbuf );
    hwdata->nullbuf = _mmobj_array( md, hwdata->bufsize, UBYTE );

    // set write cursor lag and round to nearest samplesize
    hwdata->lag  = (MINIMUM_LATENCY * hwdata->mixspeed * hwdata->samplesize) / 1000;
    hwdata->lag += (hwdata->samplesize - (hwdata->samplesize - hwdata->lag % hwdata->samplesize)) % hwdata->samplesize;

    //initial write cursors
    hwdata->currpos         = hwdata->lag;
    hwdata->lastwrite       = 0;
    hwdata->lastwritecursor = 0;

    // error tolerance timer
    hwdata->lasterror       = 0;
    
    memset( &bf, 0, sizeof(DSBUFFERDESC) );
    bf.dwSize          = sizeof(DSBUFFERDESC);
    bf.dwFlags         = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE | ((hwdata->mode & DMODE_EXCLUSIVE) ? 0 : DSBCAPS_GLOBALFOCUS);
    bf.dwBufferBytes   = hwdata->bufsize;
    bf.lpwfxFormat     = &pcmwf;
    
    hres = IDirectSound_CreateSoundBuffer( hwdata->ds, &bf, &hwdata->bb, NULL );
    if(hres != DS_OK)
    {
        logerror("CreateSoundBuffer (Secondary)",hres);
        return FALSE;
    }

    // Finally, start playing the buffer (and wipe it first)

    IDirectSoundBuffer_Play( hwdata->bb, 0, 0, DSBPLAY_LOOPING );
    VC_PlayStart( md );
    hwdata->dorefresh = TRUE;
    DS_WipeBuffers( md );

    return TRUE;
}

// _____________________________________________________________________________________
//
static BOOL DS_SetSoftVoices(MDRIVER *md, uint num)
{
    return VC_SetSoftVoices(md->device.vc, num);
}

// _____________________________________________________________________________________
//
static uint DS_GetOption( MDRIVER *md, enum MD_OPTIONS option )
{
    return VC_GetOption( md, option );
}


MD_DEVICE drv_ds =
{
    "DirectSound",
    "DirectSound Driver (DX6) v0.6",
    0, VC_MAXVOICES,

    NULL,       // Linked list!
    NULL,
    NULL,

    // sample Loading
    VC_SampleAlloc,
    VC_SampleGetPtr,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    VC_GetSampleCaps,
    VC_GetSampleFormat,

    // Detection and initialization
    DS_IsPresent,
    DS_Init,
    DS_Exit,
    DS_Update,
    DS_PlayStart,
    DS_PlayStop,

    NULL,
    DS_SetSoftVoices,

    DS_SetOption,
    DS_GetOption,

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
    VC_VoiceRealVolume,

    DS_WipeBuffers
};
