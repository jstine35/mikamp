/*
 Mikamp Sound System
 
 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution 
 ------------------------------------------
 mikamp.h

  General stuffs needed to use or compile mikamp.  Of course.
  It is easier to list what things are NOT contained within this header
  file:

  a) module player (or any other player) stuffs.  See mplayer.h.
  b) unimod format/structure/object stuffs (unimod.h)
  c) mikamp streaming input/output and memory management (mmio.h,
     automatically included below).
  d) Mikamp typedefs, needed for compiling mikamp only (mmtypes.h)

*/

#pragma once

#include "mmio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GLOBVOL_FULL  128
#define VOLUME_FULL   128

#define PAN_LEFT         (0-128)
#define PAN_RIGHT        128
#define PAN_FRONT        (0-128)
#define PAN_REAR         128
#define PAN_CENTER       0

#define PAN_SURROUND     512         // panning value for Dolby Surround


#define Mikamp_RegisterDriver(x) MD_RegisterDriver(&x)
#define Mikamp_RegisterLoader(x) ML_RegisterLoader(&x)
#define Mikamp_RegisterErrorHandler(x) _mmerr_sethandler(x)

/* typedefs moved up here to satisfy GCC. */

typedef struct _MD_DEVICE   MD_DEVICE;
typedef struct _MDRIVER     MDRIVER;
typedef struct _MD_VOICESET MD_VOICESET;

typedef struct _SL_SAMPLE   SL_SAMPLE;

enum MD_CHANNELS
{
    MD_MONO = 1,
    MD_STEREO = 2,
    MD_QUADSOUND = 4
};

enum MD_ALLOC
{
    MD_ALLOC_STATIC,
    MD_ALLOC_DYNAMIC
};

enum SL_FILEMODE
{
    SL_USE_FILEPOINTER,
    SL_OPEN_FILENAME
};

enum SF_BITS
{
    SF_BITS_UNKNOWN = 0,
    SF_BITS_8 = 1,
    SF_BITS_16,
    SF_BITS_24
};

enum SL_COMPRESS
{   
    SL_COMPRESS_NONE  = 0,
    SL_COMPRESS_RAW   = 1,
    SL_COMPRESS_IT214,
    SL_COMPRESS_IT215,
    SL_COMPRESS_MPEG3,
    SL_COMPRESS_ADPCM,
    SL_COMPRESS_VORBIS,
    SL_COMPRESS_COUNT,
};

// Managed Sample (internal) Mode Flags:
// -------------------------------------

#define SMF_BITDEPTH_HINT            ( 1ul<<0 )
#define SMF_CHANNELS_HINT            ( 1ul<<1 )
#define SMF_FORCED_CHANNELS_ENABLE   ( 1ul<<4 )
#define SMF_FORCED_BITDEPTH_ENABLE   ( 1ul<<5 )

// Sample format [in-memory] flags:
// --------------------------------
// These match up to the bit format passed to the drivers.
// Notes:
//   SL Prefixes - Looping flags.
//   SF Prefixes - Driver flags.  These describe the sample format to the driver
//     for both loading and playback.

#define SL_LOOP             (1ul<<0)
#define SL_BIDI             (1ul<<1)
#define SL_REVERSE          (1ul<<2)
#define SL_SUSTAIN_LOOP     (1ul<<3)   // Sustain looping, as used by ImpulseTracker
#define SL_SUSTAIN_BIDI     (1ul<<4)   //   (I hope someone finds a use for this someday
#define SL_SUSTAIN_REVERSE  (1ul<<5)   //    considering how much work it was!)
#define SL_DECLICK          (1ul<<6)   // enable aggressive declicking - generally unneeded.
#define SL_RESONANCE_FILTER (1ul<<8)   // Impulsetracker-style resonance filters

#define SF_SIGNED           (1ul<<1)
#define SF_DELTA            (1ul<<2)
#define SF_BIG_ENDIAN       (1ul<<3)   // applies to 16, 24, and 32 bit samples.

// Used to specify special values for start in Voice_Play().
// Note that 'SF_START_BEGIN' starts the sample at the END if SL_REVERSE is set!

#define SF_START_BEGIN       -2
#define SF_START_CURRENT     -1

// -------------------------------------------------------------------------------------
// decompress16/8    - returns the number of samples actually loaded.
//
typedef struct SL_DECOMPRESS_API
{
    struct SL_DECOMPRESS_API     *next;

    enum SL_COMPRESS  type;    // compression type!

    void   *(*init)(MMSTREAM *mmfp);
    void    (*cleanup)(void *handle);
    int     (*decompress16)(void *handle, SWORD *dest, int cbcount1, MMSTREAM *mmfp);
    int     (*decompress8)(void *handle, SWORD *dest, int cbcount1, MMSTREAM *mmfp);

} SL_DECOMPRESS_API;

// -------------------------------------------------------------------------------------
// Managed Sample Structure, used by the Mikamp's Sample Manager to build a linked list
// of all samples allocated to the device.
//
// Notes:
//  - user handles are assigned only after the samples have been allocated into the
//    device with a call to "Mikamp_SampleManager_Commit()".
//  - format flags are not important to sample size, thus they are not stored here.
//    The necessary sample format is fetched from the device prior to commit.
//
typedef struct SM_SAMPLE
{
    uint           flags;       // any combination of SMF_* flag defines
    int            handle;      // driver's handle for this sample
    int           *usr_handle;  // user-specified handle pointer (NULL allowed)
    uint           length;      // length, in samples, of memory to be allocated in the device
    int            scalefactor; // scales the sample smaller (outfmt modifier)

    enum SF_BITS   bitdepth;
    enum SF_BITS   hint_bitdepth;
    enum MD_CHANNELS channels;
    enum MD_CHANNELS hint_channels;

    struct SM_SAMPLE *next;     // linked list hoopla!

} SM_SAMPLE;

// ---------------------------------------------------------------------------------
// Encapsulation of the Sample Manager.  Samples can be registered to this freely.
// Loading of samples can either be done in one shot or incrementally.
//
typedef struct MD_SAMPLE_MANAGER
{
    SM_SAMPLE     *samplelist;
    BOOL           active;      // the sample manager has been commited (memory allocated, valid sample handles)
    BOOL           uptodate;    // sample manager is up to date (no sample additions since last commit)

} MD_SAMPLE_MANAGER;

#define SL_API_STREAM_CALLBACK  long (*streamback)( const SL_SAMPLE *sinfo, void *srcbuf, void *dstbuf, uint inlen )

// -------------------------------------------------------------------------------------
// This structure contains all pertinent information to loading and properly
// converting a sample.
//
// Notes : 
//  - scalefactor was originally used for GUS cards to scale down sampledata when the
//    ultrasound was low on memory.
//  - scalefactor is now for internal use by ImpulseTracker (hack!), when samples are
//    set at a very high octave and need to be reduced to improve replay accuracy and
//    timing.
//
struct _SL_SAMPLE
{
    SM_SAMPLE       *managed;   // managed sample handle
    enum SL_FILEMODE filemode;

    uint         blocksize;     // buffer size... (in samples)
    uint         numblocks;     // number of blocks (> 1 means block mode!)
    uint         infmt;         // format of the sample on disk (never changes)
    enum SF_BITS      indepth;
    enum MD_CHANNELS  inchan;

    const CHAR  *filename;      // filename, used if filemode is SLOAD_USE_FILEPOINTER
    MMSTREAM    *mmfp;          // file and seekpos of module
    long         seekpos;       // seek position within the file (-1 = current position)

    // streamback
    //    callback used for streaming audio.  When samples are cached this is called
    //    to fill the buffer, then it is bound to the driver's dynamic sample update
    //    callback so that it is triggered properly as the sample needs filled.
    //  Return 0 if the data is formatted to match sl_sample->outfmt.
    //  Returns actual # of loaded samples (in samples!) if dats matches sl_sample->infmt
    //    (tells SampleLoader to perform automated conversion)
    //  matches sl_sample->infmt.

    SL_API_STREAM_CALLBACK;
    void      *streamdata;

    struct
    {
        enum SL_COMPRESS    type;  // compression type!
        SL_DECOMPRESS_API  *api;   // decompression api!
        void               *handle;// decompression process handle / data!
    } decompress;

    struct _SL_SAMPLE *next;      // linked list hoopla.

};

// -------------------------------------------------------------------------------------
// Encapsulation of the Sample Manager.  Samples can be registered to this freely.
// Loading of samples can either be done in one shot or incrementally.
//
typedef struct SAMPLE_LOADER
{
    MM_ALLOC        allochandle;
    SL_SAMPLE      *samplelist;
    SL_SAMPLE      *cursamp;     // current sample being loaded (set by SampleLoader_Start)
    SL_SAMPLE      *lastload;    // previous loded sample, used by SampleLoader_StartNext()

    // Following variables used when loading individual samples

    int            rlength;
    int            delta_old, delta_new;
    int            ditherval;
    SWORD         *buffer;

} SAMPLE_LOADER;

MMEXPORT BOOL SampleLoader_LoadNextSample( MDRIVER *md );
MMEXPORT void SampleLoader_LoadSamples( MDRIVER *md );

MMEXPORT SL_SAMPLE *SampleLoader_AddSample( MDRIVER *md, uint in_format, enum SF_BITS in_bitdepth, enum MD_CHANNELS in_channels, uint length );
MMEXPORT void SampleLoader_LoadChunk( SAMPLE_LOADER*sload, void *buffer, int length, uint outfmt );
MMEXPORT BOOL SampleLoader_Start( SAMPLE_LOADER *sload, SL_SAMPLE *s );
MMEXPORT SL_SAMPLE *SampleLoader_StartNext( MDRIVER *md );
MMEXPORT void SampleLoader_End( SAMPLE_LOADER *sload );
MMEXPORT void SampleLoader_Cleanup( MDRIVER *md );
MMEXPORT void SampleLoader_Create( MDRIVER *md );

MMEXPORT void SL_RegisterDecompressor( SL_DECOMPRESS_API *ldr );
MMEXPORT void SL_In_SetStream( SL_SAMPLE *s, SL_API_STREAM_CALLBACK, void *streamdata );
MMEXPORT void SL_In_SetStreamFP( SL_SAMPLE *s, SL_API_STREAM_CALLBACK, void *streamdata, MMSTREAM *fp, uint seekpos );
MMEXPORT void SL_In_SetStreamFilename( SL_SAMPLE *s, SL_API_STREAM_CALLBACK, void *streamdata, const CHAR *filename, uint seekpos );
MMEXPORT void SL_In_SetHandle( SL_SAMPLE *s, int handle );
MMEXPORT void SL_In_SetFileFP( SL_SAMPLE *s, MMSTREAM *fp, uint seekpos );
MMEXPORT void SL_In_SetFileName( SL_SAMPLE *s, const CHAR *filename, uint seekpos );
MMEXPORT void SL_In_Decompress( SL_SAMPLE *s, uint compress_type );
MMEXPORT void SL_In_Delta( SL_SAMPLE *s, BOOL yesno );
MMEXPORT void SL_In_Unsigned( SL_SAMPLE *s );
MMEXPORT void SL_In_Signed( SL_SAMPLE *s );
MMEXPORT void SL_In_8bit( SL_SAMPLE *s );
MMEXPORT void SL_In_16bit( SL_SAMPLE *s );

extern SL_DECOMPRESS_API  dec_raw;
extern SL_DECOMPRESS_API  dec_it214;
extern SL_DECOMPRESS_API  dec_vorbis;
extern SL_DECOMPRESS_API  dec_adpcm;


/**************************************************************************
****** Driver stuff: ******************************************************
**************************************************************************/

enum MD_CAPS            // for fetching driver capabilities
{
    MD_CAPS_CHANNELS,
    MD_CAPS_BITDEPTH,
    MD_CAPS_FORMAT,             // format flags available (sample caps only)
    MD_CAPS_ALL                 // TODO : fetch all capabilities into an array?
};

enum MD_OPTIONS
{
    MD_OPT_MIXSPEED,            // mixing output (44100 or 48000 is usual default)
    MD_OPT_BITDEPTH,            // bitdepth output (16 bit default)
    MD_OPT_CHANNELS,            // set mono, stereo, quadsound, etc.
    MD_OPT_FLAGS,
    MD_OPT_LATENCY,             // force latency (largely unsupported)
    MD_OPT_CPU,                 // force specific CPU capabilities (default is autodetect)

    // These are automatically translated by the MDRIVER and so you need not worry about
    // special driver code to handle them:

    MD_OPT_FLAGS_ENABLE,        // sets DMODE_* flags
    MD_OPT_FLAGS_DISABLE,       // unsets DMODE_* flags
};

#define  MD_DISABLE_SURROUND  0
#define  MD_ENABLE_SURROUND   1

#define  MD_AUTO_LATENCY      0


// possible mixing mode bits:
// --------------------------
// These take effect only after Mikamp_Init or Mikamp_Reset.

#define DMODE_DEFAULT        (1ul<<16)   // use default/current settings.  Ignore all other flags

#define DMODE_NOCLICK        (1ul<<1)    // enable declicker - uses volume micro ramps to remove clicks.
#define DMODE_INTERP         (1ul<<3)    // enable interpolation
#define DMODE_EXCLUSIVE      (1ul<<4)    // enable exclusive (non-cooperative) mode.
#define DMODE_SOFTWARE       (1ul<<6)    // force software mixing only (dynamic sample support for all voicesets)
#define DMODE_SURROUND       (1ul<<7)    // Enable support for dolby surround (inverse waveforms left/right)
#define DMODE_RESONANCE      (1ul<<8)    // Enable support for resonance filters

//#define DMODE_FORCE_8BIT     (1ul<<5)    // force use of 8 bit samples only.
//#define DMODE_16BITS         (1ul<<0)    // enable 16 bit output

// -------------------------------------------------------------------------------------
// Used by mdriver to communicate quadsound volumes between itself and the drivers in a
// fast, efficient, and clean manner.  You can use it too, if are elite and don't mind 
// using structs for things.  Or you can just use 'flvol, frvol, rlvol, rrvol' for 
// everything like a dork!
//
typedef struct _MMVOLUME
{
    struct
    {
        int   left, right;
    } front;
    
    struct
    {
        int   left, right;
    } rear;
} MMVOLUME;


// -------------------------------------------------------------------------------------
// driver structure:
// Each driver must have a structure of this type.
//
struct _MD_DEVICE
{   
    CHAR    *Name;
    CHAR    *Version;
    uint     HardVoiceLimit,       // Limit of hardware mixer voices for this driver
             SoftVoiceLimit;       // Limit of software mixer voices for this driver

    // Below is the 'private' stuff, to be used by the MDRIVER and the
    // driver modules themselves [commenting is my version of C++ private]

    struct _MD_DEVICE*  next;
    struct _VIRTCH*     vc;        // software mixer handle (optional) -- some hardware soundcard drivers may skip this
    void*               local;     // local data storage unit.  unit or loose it.  hahaha.. hr.. ugh.

    // sample loading and information

    int     (*SampleAlloc)     (struct _MD_DEVICE *md, SM_SAMPLE *s);
    void   *(*SampleGetPtr)    (struct _MD_DEVICE *md, uint handle);
    void    (*SampleLoad)      (struct _MD_DEVICE *md, SAMPLE_LOADER *sload, SL_SAMPLE *samp);
    void    (*SampleUnLoad)    (struct _MD_DEVICE *md, uint handle);
    ULONG   (*FreeSampleSpace) (struct _MD_DEVICE *md, int type);
    ULONG   (*RealSampleLength)(struct _MD_DEVICE *md, SM_SAMPLE *s);
    uint    (*GetSampleCaps)   (struct _MDRIVER *md, enum MD_CAPS captype);
    uint    (*GetSampleFormat) (struct _MDRIVER *md, int handle);

    // detection / initialization / general util

    BOOL    (*IsPresent)       (void);
    BOOL    (*Init)            ( struct _MDRIVER *md );  // init driver using curent mode settings.
    void    (*Exit)            ( struct _MDRIVER *md );
    void    (*Update)          ( struct _MDRIVER *md );      // update driver (for polling-based updates).
    BOOL    (*PlayStart)       ( struct _MDRIVER *md );
    void    (*PlayStop)        ( struct _MDRIVER *md );

    BOOL    (*SetHardVoices)   (struct _MDRIVER *md, uint num);
    BOOL    (*SetSoftVoices)   (struct _MDRIVER *md, uint num);

    // Notes:
    //  SetOption can be used at any time to re-configure the driver settings.
    //  Some drivers will reconfigure on the fly, others may just ignore it.
    //  Some settings require samples to be reloaded in order to take effect.

    void    (*SetOption)       ( struct _MDRIVER *md, enum MD_OPTIONS option, uint value );
    uint    (*GetOption)       ( struct _MDRIVER *md, enum MD_OPTIONS option );

    // Driver volume system (quadsound support!)
    //  (There is no direct panning function because all panning is done 
    //  through manipulation of the four volumes).

    void    (*SetVolume)       (struct _MD_DEVICE *md, const MMVOLUME *volume);
    void    (*GetVolume)       (struct _MD_DEVICE *md, MMVOLUME *volume);

    // Voice control and Voice information
    
    int     (*GetActiveVoices)    (struct _MD_DEVICE *md);

    void    (*VoiceSetVolume)     (struct _MD_DEVICE *md, uint voice, const MMVOLUME *volume);
    void    (*VoiceGetVolume)     (struct _MD_DEVICE *md, uint voice, MMVOLUME *volume);
    void    (*VoiceSetFrequency)  (struct _MD_DEVICE *md, uint voice, ulong frq);
    ulong   (*VoiceGetFrequency)  (struct _MD_DEVICE *md, uint voice);
    void    (*VoiceSetPosition)   (struct _MD_DEVICE *md, uint voice, ulong pos);
    ulong   (*VoiceGetPosition)   (struct _MD_DEVICE *md, uint voice);
    void    (*VoiceSetSurround)   (struct _MD_DEVICE *md, uint voice, int flags);
    void    (*VoiceSetResonance)  (struct _MD_DEVICE *md, uint voice, int cutoff, int resonance);

    void    (*VoicePlay)          (struct _MD_DEVICE *md, uint voice, uint handle, long start, uint length, int reppos, int repend, int suspos, int susend, uint flags);
    void    (*VoiceResume)        (struct _MD_DEVICE *md, uint voice);
    void    (*VoiceStop)          (struct _MD_DEVICE *md, uint voice);
    BOOL    (*VoiceStopped)       (struct _MD_DEVICE *md, uint voice);
    void    (*VoiceReleaseSustain)(struct _MD_DEVICE *md, uint voice);
    ulong   (*VoiceRealVolume)    (struct _MD_DEVICE *md, uint voice);

    void    (*WipeBuffers)        (struct _MDRIVER *md);

};


// ==========================================================================
// VoiceSets!  The Magic of Modular Design!
//
// Each voiceset has set characteristics, including a couple flags (most
// specifically, the flag for hardware or software mixing, if available), and
// volume/voice descriptors.
//
// The voicesets must all be registered with the driver prior to initial-
// ization, and are then referenced via an integer handle.

#define MDVS_DYNAMIC            (1ul<<0)      // software mixing enable
#define MDVS_STATIC             (1ul<<1)      // preferred hardware mixing
#define MDVS_PLAYER             (1ul<<2)      // enable the player (if (*player) is not NULL)
#define MDVS_FROZEN             (1ul<<3)      // Freeze all voice activity on the voiceset.

#define MDVS_STREAM             (1ul<<6)      // streaming voice set
#define MDVS_INHERITED_VOICES   (1ul<<7)      // inherits the parent's voice pool

#define MDVD_CRITICAL           (1ul<<0)      // Voice is marked as critical
#define MDVD_PAUSED             (1ul<<1)      // voice is suspended (paused) rather than off.

// -------------------------------------------------------------------------------------
//
typedef struct _VS_STREAM
{   
    void    (*callback)(struct _MD_VOICESET *voiceset, uint voice, void *dest, uint len);
    void    *streaminfo;        // information set by and used by the streaming device.

    uint    blocksize;          // buffer size...
    uint    numblocks;
    uint    handle;             // sample handle allocated for streaming
    ulong   oldpos;             // used for hard-headed streaming mode.

} _VS_STREAM;

// -------------------------------------------------------------------------------------
//
typedef struct _MD_VOICE_DESCRIPTOR
{   
    int         voice;             // reference/index to the 'real' voice
    int         volume;            // mdriver's pre-vs->volume modified volumes!
    int         pan_lr, pan_fb;    // panning values.  left/right and front/back.

    uint        flags;             // describes each voice

    // Streaming audio options:
    // Allows the user to put their own user-defineable streaming data into the
    // sample buffer, rather than mikamp filling it itself from precached
    // SAMPLEs.

    _VS_STREAM *stream;

} MD_VDESC;

// -------------------------------------------------------------------------------------
//
struct _MD_VOICESET
{
    // premix modification options:
    // Procedures are available for modifying the sample data before the
    // mixer adds it into the mixing buffer.  This is intended for streaming
    // audio and the option to add additional effects, such as flange or reverb.

    void     (*premix8) (struct _MD_VOICESET *voiceset, uint voice, UBYTE *dest, uint len);
    void     (*premix16)(struct _MD_VOICESET *voiceset, uint voice, SWORD *dest, uint len);

    // Streaming audio options:
    // Allows the user to put their own user-defineable streaming data into the
    // sample buffer, rather than mikamp filling it itself from precached
    // SAMPLEs.  This information is used for any voiceset that has the MDVS_STREAM
    // flag set.


    // Music options:
    // Mikamp includes support for playing music in an accurate and efficient
    // manner.  The mixer will ensure that all players are updated in appro-
    // priate fashion.

    uint     (*player)(struct _MD_VOICESET *voiceset, void *playerinfo);
    void      *playerinfo;      // info block allocated for and used by the player.

    // MDRIVER Private stuff:
    //  Changing this stuff will either cause very bad, or simply unpredictable
    //  behaviour.  Please use the provided VoiceSet_* class of API functions
    //  to modify this jazz.

    UBYTE      flags;            // voiceset flags
                
    uint       voices;           // number of voices allocated to this voiceset (and all it's children)
    MD_VDESC  *vdesc;            // voice descriptors + voice lookup table.

    int        countdown;        // milliseconds until the next call to this player.
    uint       sfxpool;          // sound effects voice-search

    int        volume;           // voiceset 'natural' volume
	int        absvol;           // internal volume (after parents suppress it)
    int        pan_lr, pan_fb;

    struct _MD_VOICESET
              *next, *nextchild,
              *owner,
              *children;

    struct _MDRIVER *md;          // the driver this voiceset is attached to.
};


// Event Enumerations.
// Aren't too many of them yet. :)

enum
{   
    MDEVT_MODE_CHANGED = 1,
    MDEVT_MAX
};

// -------------------------------------------------------------------------------------
// The info in this structure, after it has been returned from Mikamp_Init, will be
// properly updated whenever the status of the driver changes (for whatever reason
// that may be).  Note that none of these should ever be changed manually.  Use
// the appropriate Mikamp_* functions to alter the driver state.
//
struct _MDRIVER
{
    MM_ALLOC   allochandle;

    CHAR     *name;
    int       volume;        // Global sound volume (0-128)
    int       pan;           // panning position (PAN_LEFT to PAN_RIGHT)
    int       pansep;        // 0 == mono, 128 == 100% (full left/right)

    // Mixspeed, bitdepth, channels, and modeflags ar all updated (cached)
    // when Mikamp_PlayStart or Mikamp_DeviceConfig_End are called.

    uint      mixspeed;      // audio samplerate -- 22050?  44100?
    enum SF_BITS bitdepth;   // 8 bit?  16 bit?
    uint      channels;      // mono? stereo? more?
    uint      modeflags;     // see DMODE_* for possibile flags

    volatile
    int       hardvoices,    // total hardware voices allocated (-1 means hardware is not available!)
              softvoices;    // total software voices allocated (-1 means software is not available!)

    // Strictly For Internal / Professional Use
    // ----------------------------------------
    // That includes everything from here to the end of the structure.

    MD_DEVICE  device;      // device info / function pointers.
    SAMPLE_LOADER     *sampload;
    MD_SAMPLE_MANAGER  sample_manager;

    // Voiceset Information

    MD_VOICESET  *voiceset;

    // Mdriver Voice Management:
    //  These indicate which voices of the two voice types have been assigned to
    //  voice sets, and which voice sets they are allocated to.  A value of -1 is
    //  unassigned.  Any other value is the index of the voiceset in use.

    MD_VOICESET **vmh,      // voice management, hardware (NULL if no hardware supported)
                **vms;      // voice management, software (NULL if no software supported)

    BOOL     isplaying;

    struct
    {
        void   *data;
        BOOL  (*proc)(struct _MDRIVER *md, void *data);
    } event[MDEVT_MAX];

    struct
    {
        uint       blocksize;       // buffer size... (in samples)
        uint       numblocks;       // number of blocks (> 1 means block mode!)
    } stream;

};


// main driver prototypes:
// =======================

MMEXPORT void       Mikamp_RegisterAllDrivers(void);
MMEXPORT int        Mikamp_GetNumDrivers(void);
MMEXPORT MD_DEVICE *Mikamp_DriverInfo(int driver);

MMEXPORT void       Mikamp_SamplePrecacheFP(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, FILE *fp, long seekpos);
MMEXPORT void       Mikamp_SamplePrecacheMem(MDRIVER *md, int *handle, uint infmt, uint length, int decompress, void *data);

MMEXPORT MDRIVER   *Mikamp_Initialize( void );
MMEXPORT MDRIVER   *Mikamp_InitializeEx( void );
MMEXPORT void       Mikamp_SetDeviceOption( MDRIVER *md, enum MD_OPTIONS option, uint value );
MMEXPORT uint       Mikamp_GetDeviceOption( MDRIVER *md, enum MD_OPTIONS option );
MMEXPORT BOOL       Mikamp_PlayStart( MDRIVER *md );
MMEXPORT void       Mikamp_PlayStop( MDRIVER *md );
MMEXPORT BOOL       Mikamp_DeviceConfig_Exit( MDRIVER *md );
MMEXPORT void       Mikamp_DeviceConfig_Enter( MDRIVER *md );

MMEXPORT void       Mikamp_Exit(MDRIVER *md);
MMEXPORT BOOL       Mikamp_Reset(MDRIVER *md);
MMEXPORT BOOL       Mikamp_Active(MDRIVER *md);
MMEXPORT void       Mikamp_RegisterPlayer(MDRIVER *md, void (*plr)(void));
MMEXPORT void       Mikamp_Update(MDRIVER *md);
MMEXPORT int        Mikamp_GetActiveVoices(MDRIVER *md);
MMEXPORT void       Mikamp_WipeBuffers(MDRIVER *md);

MMEXPORT void       Mikamp_SetEvent(MDRIVER *md, int event, void *data, BOOL (*eventproc)(MDRIVER *md, void *data));
MMEXPORT void       Mikamp_ThrowEvent(MDRIVER *md, int event);
MMEXPORT BOOL       Mikamp_SetMode(MDRIVER *md, uint mixspeed, uint mode, uint channels);
MMEXPORT void       Mikamp_GetMode(MDRIVER *md, uint *mixspeed, uint *mode, uint *channels);

MMEXPORT void       SampleManager_Close( MDRIVER *md );
MMEXPORT SM_SAMPLE *ManagedSample_Create( MDRIVER *md, uint length );
MMEXPORT SM_SAMPLE *SampleManager_FindSampleHandle( const MDRIVER *md, int handle );
MMEXPORT void       ManagedSample_AddUserHandle( SM_SAMPLE *sample, int *usr_handle );
MMEXPORT void       ManagedSample_RemoveUserHandle( SM_SAMPLE *sample, int *usr_handle );
MMEXPORT void       ManagedSample_ForceChannels( SM_SAMPLE *sample, enum MD_CHANNELS channels );
MMEXPORT void       ManagedSample_ForceBitdepth( SM_SAMPLE *sample, enum SF_BITS bitdepth );
MMEXPORT void       SM_Sample_Hint_Bitdepth( SM_SAMPLE *sample, enum SF_BITS bitdepth );
MMEXPORT void       SM_Sample_Hint_Channels( SM_SAMPLE *sample, enum MD_CHANNELS channels );
MMEXPORT BOOL       SampleManager_Commit( MDRIVER *md );


// Voiceset crapola -- from -- VOICESET.C --
// =========================================

MMEXPORT MD_VOICESET *Voiceset_Create(MDRIVER *drv, MD_VOICESET *owner, uint voices, uint flags);
MMEXPORT void         Voiceset_SetPlayer(MD_VOICESET *vs, uint (*player)(struct _MD_VOICESET *voiceset, void *playerinfo), void *playerinfo);
MMEXPORT MD_VOICESET *Voiceset_CreatePlayer(MDRIVER *md, MD_VOICESET *owner, uint voices, uint (*player)(struct _MD_VOICESET *voiceset, void *playerinfo), void *playerinfo, BOOL hardware);
MMEXPORT void         Voiceset_Free(MD_VOICESET *vs);
MMEXPORT int          Voiceset_SetNumVoices(MD_VOICESET *vs, uint voices);
MMEXPORT int          Voiceset_GetVolume(MD_VOICESET *vs);
MMEXPORT void         Voiceset_SetVolume(MD_VOICESET *vs, int volume);

MMEXPORT void     Voiceset_PlayStart(MD_VOICESET *vs);
MMEXPORT void     Voiceset_PlayStop(MD_VOICESET *vs);
MMEXPORT void     Voiceset_Reset(MD_VOICESET *vs);
MMEXPORT void     Voiceset_EnableOutput(MD_VOICESET *vs);
MMEXPORT void     Voiceset_DisableOutput(MD_VOICESET *vs);

// MD_Player - Voiceset Player Update Procedure
extern   ulong    MD_Player(MDRIVER *drv, ulong timepass);

MMEXPORT void     Voice_SetVolume(MD_VOICESET *vs, uint voice, int ivol);
MMEXPORT int      Voice_GetVolume(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_SetPosition(MD_VOICESET *vs, uint voice, long pos);
MMEXPORT long     Voice_GetPosition(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_SetFrequency(MD_VOICESET *vs, uint voice, long frq);
MMEXPORT long     Voice_GetFrequency(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_SetPanning(MD_VOICESET *vs, uint voice, int pan_lr, int pan_fb);
MMEXPORT void     Voice_GetPanning(MD_VOICESET *vs, uint voice, int *pan_lr, int *pan_fb);
MMEXPORT void     Voice_SetResonance(MD_VOICESET *vs, uint voice, int cutoff, int resonance);

MMEXPORT void     Voice_Play(MD_VOICESET *vs, uint voice, int handle, long start, int length, int reppos, int repend, int suspos, int susend, uint flags);
MMEXPORT void     Voice_Pause(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_Resume(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_Stop(MD_VOICESET *vs, uint voice);
MMEXPORT void     Voice_ReleaseSustain(MD_VOICESET *vs, uint voice);
MMEXPORT BOOL     Voice_Stopped(MD_VOICESET *vs, uint voice);
MMEXPORT BOOL     Voice_Paused(MD_VOICESET *vs, uint voice);

MMEXPORT ulong    Voice_RealVolume(MD_VOICESET *vs, uint voice);

MMEXPORT int      Voice_Find(MD_VOICESET *vs, uint flags);


// Lower level 'stuff'
// -------------------

// use macro Mikamp_RegisterDriver instead
MMEXPORT void     MD_RegisterDriver(MD_DEVICE *drv);

// loads a sample based on the given SL_SAMPLE structure into the driver
MMEXPORT int      MD_SampleLoad(MDRIVER *md, SL_SAMPLE *s, int type);

// allocates a chunk of memory without loading anything into it.
MMEXPORT int      MD_SampleAlloc(MDRIVER *md, int type);
MMEXPORT void     MD_SampleUnload(MDRIVER *md, int handle);

// returns the actual amount of memory a sample will use.  Necessary because
// some drivers may only support 8 or 16 bit data, or may not support stereo.
MMEXPORT ulong    MD_SampleLength( MDRIVER *md, SM_SAMPLE *s );
MMEXPORT ulong    MD_SampleSpace( MDRIVER *md, int type );  // free sample memory in driver.


// ================================================================
// Declare external drivers
// (these all come with various mikamp packages):
// ================================================================

// Multi-platform jazz...
MMEXPORT MD_DEVICE drv_nos;      // nosound driver, REQUIRED!
MMEXPORT MD_DEVICE drv_raw;      // raw file output driver [music.raw]
MMEXPORT MD_DEVICE drv_wav;      // RIFF WAVE file output driver [music.wav]

// Windows95 drivers:
MMEXPORT MD_DEVICE drv_win;      // windows media (waveout) driver
MMEXPORT MD_DEVICE drv_ds;       // Directsound driver
MMEXPORT MD_DEVICE drv_dsaccel;  // accelerated directsound driver (sucks!)

// MS_DOS Drivers:
MMEXPORT MD_DEVICE drv_awe;      // experimental SB-AWE driver
MMEXPORT MD_DEVICE drv_gus;      // gravis ultrasound driver [hardware / software mixing]
MMEXPORT MD_DEVICE drv_gus2;     // gravis ultrasound driver [hardware mixing only]
MMEXPORT MD_DEVICE drv_sb;       // soundblaster 1.5 / 2.0 DSP driver
MMEXPORT MD_DEVICE drv_sbpro;    // soundblaster Pro DSP driver
MMEXPORT MD_DEVICE drv_sb16;     // soundblaster 16 DSP driver
MMEXPORT MD_DEVICE drv_ss;       // ensoniq soundscape driver
MMEXPORT MD_DEVICE drv_pas;      // PAS16 driver
MMEXPORT MD_DEVICE drv_wss;      // Windows Sound System driver

// Various UNIX/Linux drivers:
MMEXPORT MD_DEVICE drv_vox;      // linux voxware driver
MMEXPORT MD_DEVICE drv_AF;       // Dec Alpha AudioFile driver
MMEXPORT MD_DEVICE drv_sun;      // Sun driver
MMEXPORT MD_DEVICE drv_os2;      // Os2 driver
MMEXPORT MD_DEVICE drv_hp;       // HP-UX /dev/audio driver
MMEXPORT MD_DEVICE drv_aix;      // AIX audio-device driver
MMEXPORT MD_DEVICE drv_sgi;      // SGI audio-device driver
MMEXPORT MD_DEVICE drv_tim;      // timing driver
MMEXPORT MD_DEVICE drv_ultra;    // ultra driver for linux

// Very portable SDL driver:
MMEXPORT MD_DEVICE drv_sdl;	// Simple DirectMedia Layer output driver

#ifdef __cplusplus
}
#endif

#include "virtch.h"    // needed because mdriver has a virtch handle now.

#ifdef __cplusplus
extern "C" {
#endif
MMEXPORT void VC_EnableInterpolation(VIRTCH *vc, int handle);
MMEXPORT void VC_DisableInterpolation(VIRTCH *vc, int handle);
#ifdef __cplusplus
}
#endif

