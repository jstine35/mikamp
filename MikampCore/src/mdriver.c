/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 mdriver.c

  Primary Mikamp core initialization code!
  [see Mikamp_Init / Mikamp_InitEx]

  Also houses routines used to access the available soundcard/soundsystem
  drivers and mixers.

 Portability:
  All systems - all compilers

*/

#include "mikamp.h"
#include <string.h>

static MD_DEVICE *firstdriver = NULL;

#define MD_TIMING_UNIT    100ul                   // 100ms per timer unit
#define MD_IDLE_THRESHOLD (500ul*MD_TIMING_UNIT)  // 500ms

// _____________________________________________________________________________________
// Mikamp's Multiplexing Core Lives Here.
// This function is called form the device driver at semi-regular intervals so that music
// playing logic can update the state of the player.
//
// timepass  - actual amount of time that's passed since the last call.  Usually this
//   should match the previous value returned by MD_Player.  Exceptions include preempt
//   signals and non-perfect timing mechanisms.
//    (time is in milliseconds * MD_TIMING_UNIT).
//
// Returns: Number of milliseconds before next call to MD_Player should be called.
//          (equivalent to timepass)
//
// Note: In the event of a preempt signal, the driver should call MD_Player immediately
//       rather than waiting until the returned timepass has expired.
//
ulong MD_Player(MDRIVER *drv, ulong timepass)
{
    int          c     = 0xfffffful;
    BOOL         use_c = 0;
    MD_VOICESET *cruise = drv->voiceset;

    while(cruise)
    {
        if( (cruise->flags & MDVS_PLAYER) && cruise->player )
        {
            // the only reason countdown will ever be 0 is if
            // the player has just been activated!
            if( cruise->countdown )
                cruise->countdown -= timepass;

            if( cruise->countdown <= 0 )
                cruise->countdown += cruise->player( cruise, cruise->playerinfo );

            if( c > cruise->countdown )
            {
                c = cruise->countdown;
                use_c = 1;
            }
        }
        cruise = cruise->next;
    }

    // if C is zero, then no player stuff is active, so set us to be called
    // a while from now.  This brings the player to almost no overhead.  Note
    // that the threshold time is no longer important anyway, since anytime a
    // player's timings are changed, or a new player activated, the mixer is
    // preempted to take care of it in a no-latency fashion.

    // give back the # of 100,000th seconds until the next call

    return use_c ? c : MD_IDLE_THRESHOLD;
}

// _____________________________________________________________________________________
//
int Mikamp_GetNumDrivers(void)
{
    int        t;
    MD_DEVICE *l;

    for(t=0,l=firstdriver; l; l=l->next, t++);

    return t;
}

// _____________________________________________________________________________________
//
MD_DEVICE *Mikamp_DriverInfo(int driver)
{
    int        t;
    MD_DEVICE *l;

    for(t=driver-1,l=firstdriver; t && l; l=l->next, t--);

    return l;
}

// _____________________________________________________________________________________
//
void MD_RegisterDriver(MD_DEVICE *drv)
{
    MD_DEVICE *cruise = firstdriver;

    if(cruise)
    {   while(cruise->next)  cruise = cruise->next;
        cruise->next = drv;
    } else
        firstdriver = drv; 
}

// Note: 'type' indicates whether the returned value should be for hardware
//       or software use.

// _____________________________________________________________________________________
//
ulong MD_SampleSpace( MDRIVER *md, int type )
{
    return md ? md->device.FreeSampleSpace(&md->device, type) : 0;
}

// _____________________________________________________________________________________
//
ULONG MD_SampleLength( MDRIVER *md, SM_SAMPLE *s )
{
    return md ? md->device.RealSampleLength( &md->device, s ) : 0;
}


/*UWORD MD_SetDMA(int secs)

// Converts the given number of 1/10th seconds into the number of bytes of
// audio that a sample # 1/10th seconds long would require at the current mdrv.*
// settings.

{
    ULONG result;

    result = (mdrv.mixfreq * ((mdrv.mode & DMODE_STEREO) ? 2 : 1) *
             ((mdrv.mode & DMODE_16BITS) ? 2 : 1) * secs) * 10;

    if(result > 32000) result = 32000;
    return(mdrv.dmabufsize = (result & ~3));  // round it off to an 8 byte boundry
}*/

// _____________________________________________________________________________________
//
//
// Streaming method :
// We can load the sample up from the previous "loadspot" of the sample up to the
// current play position. If the current play position is less than the current
// loadspot, then we load to the end of the sample and then from the beginning of
// the sample to the current playspot.
//


// _____________________________________________________________________________________
//
void MD_SampleUnload( MDRIVER *md, int handle )
{
    if(md && handle >= 0) md->device.SampleUnLoad(&md->device, handle);
}


// ================================
//  Functions prefixed with Mikamp
// ================================

// _____________________________________________________________________________________
// Initializes the driver and creates driver handles, but does not start the audio
// device.  
//
MDRIVER *Mikamp_InitializeEx( void )
{
    UWORD t;
    
    MD_DEVICE *device  = NULL, *cruise;  // always autodetect.. for now!
    MDRIVER   *md;

    // Clear the error handler.
    _mmerr_set( MMERR_NONE, NULL, NULL );
    md = _mmobj_new( NULL, MDRIVER );

    if(!device)
    {
        for(t=1,cruise=firstdriver; cruise; cruise=cruise->next, t++)
        {
            if(cruise->IsPresent()) break;
        }

        if(!cruise)
        {
            _mmerr_setsub(MMERR_DETECTING_DEVICE, "Audio driver autodetect failed.", "The Mikamp Sound System failed to find a suitable or supported audio device.  The application will proceed normally without sound.");
            cruise = &drv_nos;
        }
    } else
    {   // if n>0 use that driver
        cruise = device;
        if(!cruise->IsPresent())
        {
            // The user has forced this driver, so we generate a critical error upon failure.
            _mmerr_setsub(MMERR_DETECTING_DEVICE, "Audio driver failure.", "A specific audio driver was selected, but no compatable hardware was found.");
            cruise = &drv_nos;
        }
    }

    // make a duplicate of our driver header
    md->device = *cruise;

    if( !md->device.Init( md ) )
    {
        // switch to the nosound driver so that the program can continue to run
        // without sound, if the user so desires...

        md->device.Exit(md);
        md->device = drv_nos;
        md->device.Init( md );
    }

    md->name       = md->device.Name;
    md->hardvoices = md->softvoices = 0;
    md->pan        = 0;
    md->pansep     = 128;
    md->volume     = 128;

    md->voiceset   = NULL;    // no voicesets initialized.
    md->vms = md->vmh = NULL;

    return md;
}

// _____________________________________________________________________________________
// 
MDRIVER *Mikamp_Initialize( void )
{
    MDRIVER *device = Mikamp_InitializeEx();
    Mikamp_PlayStart( device );
    return device;
}

// _____________________________________________________________________________________
//
void Mikamp_Update(MDRIVER *md)
{
    if(md && md->isplaying) md->device.Update(md);
}

// _____________________________________________________________________________________
//
void Mikamp_Exit(MDRIVER *md)
{
    if( md )
    {
        md->device.Exit(md);
        _mmobj_close( md );
    }
}

// _____________________________________________________________________________________
//
void Mikamp_SetEvent(MDRIVER *md, int event, void *data, BOOL (*eventproc)(MDRIVER *md, void *data))
{
    if(!md) return;
    assert(event < MDEVT_MAX);          // programmer error?  invalid event specified
    md->event[event].data = data;
    md->event[event].proc = eventproc;
}

// _____________________________________________________________________________________
//
void Mikamp_ThrowEvent( MDRIVER *md, int event )
{
    assert(event < MDEVT_MAX);          // programmer error?  invalid event specified
    if( md && md->event[event].proc )
        md->event[event].proc( md, md->event[event].data );
}

// _____________________________________________________________________________________
//
// Returns:
//   TRUE   if the setting was applied instantly, or matches existing configuration.
//   FALSE  if driver needs to be reset for setting to take effect.
//
void Mikamp_SetDeviceOption( MDRIVER *md, enum MD_OPTIONS option, uint value )
{
    if( !md ) return;
    md->device.SetOption( md, option, value );
}

uint Mikamp_GetDeviceOption( MDRIVER *md, enum MD_OPTIONS option )
{
    if( !md ) return 0;
    return md->device.GetOption( md, option );
}

void Mikamp_SetMixingSpeed( MDRIVER *md, uint mixspeed )
{
    if( !md ) return;
    md->device.SetOption( md, MD_OPT_MIXSPEED, mixspeed );
}

void Mikamp_Enable( MDRIVER *md, uint modeflags )
{
    if( !md ) return;
    md->device.SetOption( md, MD_OPT_FLAGS_ENABLE, modeflags );
}

void Mikamp_Disable( MDRIVER *md, uint modeflags )
{
    if( !md ) return;
    md->device.SetOption( md, MD_OPT_FLAGS_DISABLE, modeflags );
}

// _____________________________________________________________________________________
//
void Mikamp_DeviceConfig_Enter( MDRIVER *md )
{
    if( !md ) return;

    md->isplaying = FALSE;
    md->device.PlayStop( md );
}

CHAR *md_label_channels[] =  { "Mono", "Stereo", "", "QuadSound" };

// _____________________________________________________________________________________
//
BOOL Mikamp_DeviceConfig_Exit( MDRIVER *md )
{
    if( md && md->device.PlayStart( md ) )
    {
        md->isplaying  = 1;

        md->mixspeed  = md->device.GetOption( md, MD_OPT_MIXSPEED );
        md->bitdepth  = md->device.GetOption( md, MD_OPT_BITDEPTH );
        md->channels  = md->device.GetOption( md, MD_OPT_CHANNELS );
        md->modeflags = md->device.GetOption( md, MD_OPT_FLAGS_ENABLE );

        _mmlog( "Mikamp > Initialized %s driver.", md->device.Name );
        _mmlog( "       > %dkhz / %d bits / %s ",
            md->mixspeed,
            md->bitdepth * 8,
            md_label_channels[md->channels] );

        Mikamp_ThrowEvent( md, MDEVT_MODE_CHANGED );
        return TRUE;
    }

    return FALSE;
}

BOOL Mikamp_PlayStart( MDRIVER *md )
{
    return Mikamp_DeviceConfig_Exit( md );
}

void Mikamp_PlayStop( MDRIVER *md )
{
    Mikamp_DeviceConfig_Enter( md );
}

/*void Mikamp_SetVolume(MDRIVER *md, int volume)
// Device-based volume control.  Not recommended this be used (as it may not work!)
{
    //md->device.SetVolume(_mm_boundscheck(volume, 0, 128));
}

uint Mikamp_GetVolume(MDRIVER *md)
{
    return(md->device.GetVolume());
}
*/

// _____________________________________________________________________________________
//
int Mikamp_GetActiveVoices(MDRIVER *md)
{
    return(md ? md->device.GetActiveVoices(&md->device) : 0);
}

// _____________________________________________________________________________________
// This one could be a macro....
//
BOOL Mikamp_Active(MDRIVER *md)
{
    return md ? md->isplaying : 0;
}

// _____________________________________________________________________________________
//
void Mikamp_WipeBuffers(MDRIVER *md)
{
    if(md) md->device.WipeBuffers(md);
}

// Mikamp Sample Precaching! (Raw Sample Data)
// --------------------------------------------
// Precache a sample from a file pointer or from a memory pointer!
// Assigns the handle for the laoded sample to the pointer provided.
// NOTE:
//  - This procedure merely registers the sample for loading.  Precaching only
//    actually occurs with a call to Mikamp_PrecacheSamples(), and hence the
//    handle will only be assigned following that call!
//  - Precached samples are always static (for now).
//      

/*
// _____________________________________________________________________________________
//
void Mikamp_SamplePrecacheFP(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, FILE *fp, long seekpos)
{
    MMSTREAM *mmf;

    mmf = _mmstream_createfp(fp, 0);
    SL_RegisterSample(md, handle, infmt, length, decompress, mmf, seekpos);
}

// _____________________________________________________________________________________
//
void Mikamp_SamplePrecacheMem(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, void *data)
{
    MMSTREAM *mmf;

    mmf = _mmstream_createmem(data, 0);
    SL_RegisterSample(md, handle, infmt, length, decompress, mmf, 0);
}
*/