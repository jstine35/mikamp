/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 ------------------------------------------
  Not done code.  Beware.  muhahahaha
*/


#include "mikamp.h"
#include "mdsfx.h"

#include <string.h>


typedef struct WAV
{   
    CHAR  rID[5];
    ULONG rLen;
    CHAR  wID[5];
    CHAR  fID[5];
    ULONG fLen;
    UWORD wFormatTag;
    UWORD nChannels;
    ULONG nSamplesPerSec;
    ULONG nAvgBytesPerSec;
    UWORD nBlockAlign;
    UWORD nFormatSpecific;

} WAV;

// _____________________________________________________________________________________
//
MD_SAMPLE *mdsfx_loadwavheader( MDRIVER *md, MMSTREAM *mmfp, uint *format, enum SF_BITS *bitdepth, enum MD_CHANNELS *channels )
{
    MD_SAMPLE   *si;
    WAV          wh;
    CHAR         dID[5];

    // read wav header

    _mm_read_string(wh.rID,4,mmfp);
    wh.rLen = _mm_read_I_ULONG(mmfp);
    _mm_read_string(wh.wID,4,mmfp);

    if( _mm_feof(mmfp) ||
        memcmp(wh.rID,"RIFF",4) ||
        memcmp(wh.wID,"WAVE",4))
    {
	    _mmlog("Mikamp > mwav > Failure: Not a vaild RIFF waveformat!");
        return NULL;
    }

    while( TRUE )
    {
        _mm_read_string(wh.fID,4,mmfp);
        wh.fLen = _mm_read_I_ULONG(mmfp);
        wh.fID[4] = 0;
        if(memcmp(wh.fID,"fmt ",4) == 0) break;
        _mm_fseek(mmfp,wh.fLen,SEEK_CUR);
    }

    wh.wFormatTag      = _mm_read_I_UWORD(mmfp);
    wh.nChannels       = _mm_read_I_UWORD(mmfp);
    wh.nSamplesPerSec  = _mm_read_I_ULONG(mmfp);
    wh.nAvgBytesPerSec = _mm_read_I_ULONG(mmfp);
    wh.nBlockAlign     = _mm_read_I_UWORD(mmfp);
    wh.nFormatSpecific = _mm_read_I_UWORD(mmfp);

    // check it

    if(_mm_feof(mmfp))
    {
        _mmlog("Mikamp > mwav > Failure: Unexpected end of file loading wavefile.");
        return NULL;
    }

    // skip other crap

    _mm_fseek(mmfp,wh.fLen-16,SEEK_CUR);
    _mm_read_string(dID,4,mmfp);

    if(memcmp(dID,"data",4))
    {
        dID[4] = 0;
        _mmlog("Mikamp > mwav > Failure: Unexpected token %s; 'data' not found!",dID);
        return NULL;
    }

    if(wh.nChannels > 2)
    {
        _mmlog("Mikamp > mwav > Failure: Unsupported type (more than two channels)");
        return NULL;
    }

    if( ( si = _mmobj_allocblock( md, MD_SAMPLE ) ) == NULL ) return NULL;
    
    si->speed  = wh.nSamplesPerSec;
    si->volume = 128;
    si->length = _mm_read_I_ULONG(mmfp);

    *format      = 0;
    *bitdepth    = SF_BITS_8;
    
    //si->data = _mm_malloc(si->length);
    //_mm_read_UBYTES(si->data,si->length, mmfp);

    if( (wh.nBlockAlign / wh.nChannels) == 2 )
    {
        *format      = SF_SIGNED;
        *bitdepth    = SF_BITS_16;
        si->length >>= 1;
    }

    if( wh.nChannels == 2 )
    {
        *channels     = MD_STEREO;
        si->length  >>= 1;
    }
    else
        *channels     = MD_MONO;
    return si;
}

typedef struct MDSFX_STREAM
{
    uint    blocksize;      // block size in samples
    uint    numblocks;      // number of blocks

} MDSFX_STREAM;

MDSFX_STREAM *_stream_makeblock( MDRIVER *md )
{
    MDSFX_STREAM *mss;
    
    mss = _mmobj_allocblock( md, MDSFX_STREAM );
    mss->blocksize = md->stream.blocksize;
    mss->numblocks = md->stream.numblocks;

    return mss;
}

// _____________________________________________________________________________________
// Internal streaming audio manager, good for any form of basic from-disk streaming.
//
// You'll notice that this function is very simple, thanks to mikamp's sampleloader
// doing almost all the work for us!  All we have to do is manage and update the driver
// so that it'll call us again the next time a block of sampledata is ready to be loaded.
//
static int _stream_callback( const SL_SAMPLE *sinfo, void *srcbuf, void *dstbuf, uint inlen )
{

    return inlen;       // Let mikamp do the rest for us!
}

// _____________________________________________________________________________________
// Opens a wav file for streaming.  The specified file pointer *cannot be closed* before
// the stream is stopped, lest great and terrible things occur.
//
MD_SAMPLE *mdsfx_streamwavfp( MDRIVER *md, MMSTREAM *mmfp )
{
    uint         format;
    enum SF_BITS bitdepth;
    enum MD_CHANNELS channels;

    MD_SAMPLE *si = mdsfx_loadwavheader( md, mmfp, &format, &bitdepth, &channels );
    SL_SAMPLE *sl = SampleLoader_AddSample( md, format, bitdepth, channels, si->length );

    ManagedSample_AddUserHandle( sl->managed, &si->handle );
    SL_In_SetStreamFP( sl, _stream_callback, _stream_makeblock( md ), mmfp, _mm_ftell( mmfp ) );

    return si;
}

// _____________________________________________________________________________________
//
MD_SAMPLE *mdsfx_loadwavfp( MDRIVER *md, MMSTREAM *mmfp )
{
    uint         format;
    enum SF_BITS bitdepth;
    enum MD_CHANNELS channels;
    
    MD_SAMPLE *si = mdsfx_loadwavheader( md, mmfp, &format, &bitdepth, &channels );
    SL_SAMPLE *sl = SampleLoader_AddSample( md, format, bitdepth, channels, si->length );

    ManagedSample_AddUserHandle( sl->managed, &si->handle );
    SL_In_SetFileFP( sl, mmfp, _mm_ftell( mmfp ) );

    return si;
}

// _____________________________________________________________________________________
// Adds a RIFF WAVE file to be laoded from disk via filename to the sample cache list.
// Note that samples are *not* usable until you've called SampleManager_LoadSamples or
// equivilent APIs that allocate memory and load samples.
//
// Notes:
//  - this function does not leave any open file handles.  Mikamp's SampleManager is 
//    provided the filename so that it can open the file when SampleManager_LoadSamples
//    is called.
//
MD_SAMPLE *mdsfx_loadwav( MDRIVER *md, const CHAR *fn )
{
    uint         format;
    enum SF_BITS bitdepth;
    enum MD_CHANNELS channels;

    MMSTREAM   *mmfp;
    MD_SAMPLE  *si;
    SL_SAMPLE  *sl;

    if( ( mmfp=_mm_fopen( fn, "rb" ) ) == NULL ) return NULL;

    si = mdsfx_loadwavheader( md, mmfp, &format, &bitdepth, &channels );
    sl = SampleLoader_AddSample( md, format, bitdepth, channels, si->length );

    ManagedSample_AddUserHandle( sl->managed, &si->handle );
    SL_In_SetFileName( sl, fn, _mm_ftell( mmfp ) );
    _mm_fclose( mmfp );

    return si;
}

// _____________________________________________________________________________________
//
MD_SAMPLE *mdsfx_create( MDRIVER *md )
{
    MD_SAMPLE *si;

    si          = _mmobj_allocblock( md, MD_SAMPLE );
    si->md      = md;
    return si;
}

// _____________________________________________________________________________________
//
MD_SAMPLE *mdsfx_duplicate( MD_SAMPLE *src )
{
    MD_SAMPLE  *newug;

    // Allocate and duplicate:

    newug = _mmobj_allocblock( src->md, MD_SAMPLE );
    *newug = *src;
    return newug;
}

// _____________________________________________________________________________________
//
void mdsfx_free( MD_SAMPLE *samp )
{
    ManagedSample_RemoveUserHandle( SampleManager_FindSampleHandle( samp->md, samp->handle ), &samp->handle );
    _mmobj_free( samp->md, samp );
}

// _____________________________________________________________________________________
//
void mdsfx_play(MD_SAMPLE *s, MD_VOICESET *vs, uint voice, int start)
{
    Voice_Play( vs, vs->vdesc[voice].voice, s->handle, start, s->length, s->reppos, s->repend, s->suspos, s->susend, s->flags );
    Voice_SetVolume( vs,voice, 128 );
}

// _____________________________________________________________________________________
// Mikamp's automated sound effects sample player. Picks a voice from the given voiceset,
// based upon the voice either being empty (silent), or being the oldest sound in the 
// voiceset.  Any sound flagged as critical will not be replaced.
//
// Returns the voice that the sound is being played on.  If no voice was available (usually
// by fault of criticals) then -1 is returned.
//
// flags == MDVD_* flags in mdsfx.h
//
int mdsfx_playeffect( MD_SAMPLE *s, MD_VOICESET *vs, uint start, uint flags )
{
    uint     orig;     // for cases where all channels are critical
    int      voice;

    // check for invalid parameters
    if(!vs || !vs->voices || !s) return -1;

    orig = vs->sfxpool;

    // Find a suitable voice for this sample to be played in.
    // Use the user-definable callback procedure to do so!

    voice = (s->findvoice_proc) ? s->findvoice_proc(vs, flags) : Voice_Find(vs, flags);
    if(voice == -1) return -1;

    //volume  = _mm_boundscheck(s->volume, 0, VOLUME_FULL);
    //panning = (s->panning == PAN_SURROUND) ? PAN_SURROUND : _mm_boundscheck(s->panning, PAN_LEFT, PAN_RIGHT);
    
    Voice_Play(vs, voice, s->handle, start, s->length, s->reppos, s->repend, s->suspos, s->susend, s->flags);
    Voice_SetVolume(vs, voice, s->volume);
    Voice_SetPanning(vs, voice, s->panning, 0);
    Voice_SetFrequency(vs, voice, s->speed);

    return voice;
}
