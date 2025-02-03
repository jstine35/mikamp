/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 sample_loader.c
 
 This is an API for automating the loading and re-loading of samples.
 Uses Mikamp's built in Sample Manager to allocate device memory needed to load the
 specified sample.

 Rewrite Candidate:
   The original design principle to this code is FLAWED and should be scrapped.  I was
   originally over-focused on trying to perform the complete sample/data conversion with
   a single-stage read-modify-write operation -- in order to preserve as much memory and cpu
   load as possible.  The reuslting conversion code is unweildly and (ironically) quite slow
   on modern CPUs. --air
   
   A new approach should instead convert most everything into a universal internal 32 bit
   format, and then downsample back to ideal in-memory representations.  This could be divided
   into a set of sample importers and exporters, with importers all fixed to output 32 bit
   sample data, and exporters all fixed to read 32 bit sample data. --air

*/

#include "mikamp.h"
#include "mmforbid.h"
#include "mminline.h"

#include <string.h>

static SL_DECOMPRESS_API   *sl_loadlist;

// _____________________________________________________________________________________
//
void SampleLoader_Create( MDRIVER *md )
{
    if( !md ) return;
    md->sampload  = _mmobj_new( md, SAMPLE_LOADER );
}

// _____________________________________________________________________________________
// This cleans up allocations used by the sample loader.  Frees roughly 128k of memory
// so it's pretty unimportant on robust modern desktops, but is probably a good thing to
// use on smaller portable machines.
//
// Using this function and then trying to load samples again later is *safe* -- Load code
// checks for NULL pointers and re-allocates memory if needed.
//
void SampleLoader_Cleanup( MDRIVER *md )
{
    if( !md || !md->sampload ) return;
    _mmobj_free( md->sampload, md->sampload->buffer );
    //_mmforbid_deinit( sman->mmcs );
    //sman->mmcs = NULL;
}

// _____________________________________________________________________________________
// Registers a decompressor into the global list, which is checked whenever a sample is
// specified with a decompress code.
//
// Checks if the requested decompressor is already registered, and quits if so (duplicate
// safe).
//
void SL_RegisterDecompressor( SL_DECOMPRESS_API *ldr )
{
    SL_DECOMPRESS_API   *cruise = sl_loadlist;

    while( cruise )
    {
        if( cruise == ldr ) return;
        cruise = cruise->next;
    }

    if( sl_loadlist == NULL )
    {
        sl_loadlist  = ldr;
        ldr->next    = NULL;
    }
    else
    {
        ldr->next    = sl_loadlist;
        sl_loadlist  = ldr;
    }
}

// _____________________________________________________________________________________
// Preps the sample manager for loading the specified sample.  After a call to this
// function, calls to SL_Load will load sampledata for the selected sample.
//
BOOL SampleLoader_Start( SAMPLE_LOADER *sload, SL_SAMPLE *s )
{
    if( !sload || !s || !s->managed->length ) return FALSE;

    // Notice : You should Call SampleLoader_End before calling this function!
    assert( !sload->cursamp );
    if( sload->cursamp ) SampleLoader_End( sload );

    if( !sload->buffer ) sload->buffer = _mm_malloc( &sload->allochandle, 131072 );
    if( !sload->buffer ) return FALSE;
    
    /* Legacy mutex code, no longer needed thanks to sample_loader encapsulation
    if( !sload->mmcs ) sload->mmcs = _mmforbid_init();
    if( !sload->mmcs ) { assert( FALSE ); }  // programmer warning! this is worthy of investigation when it fails.
    _mmforbid_enter( mmcs );*/

    // Initialize Variables for Incremental Loading
    // --------------------------------------------

    sload->rlength = s->managed->length;
    sload->rlength *= s->managed->channels;

    sload->delta_old  = sload->delta_new = 0;
    sload->ditherval  = 32;

    // Open File-handle if Needed
    // --------------------------
    // Some uses of the API provide an opened file, others load samples from multiple
    // files and provide filename + seekinfo instead of a pre-set filehandle.

    if( s->filemode == SL_OPEN_FILENAME )
    {
        if( !s->filename || !strlen( s->filename ) )
        {
            // TODO : Insert more robust error handling here.
            assert( FALSE );
            return FALSE;
        }

        s->mmfp = _mm_fopen( s->filename, "rb" );
    }

    if( s->mmfp && s->seekpos )
        _mm_fseek( s->mmfp, s->seekpos, SEEK_SET );

    // Assign Decompression API
    // ------------------------

    if( s->decompress.type != SL_COMPRESS_NONE )
    {
        SL_DECOMPRESS_API   *cruise = sl_loadlist;
        while( cruise )
        {
            if( cruise->type == s->decompress.type )
            {
                s->decompress.api       = cruise;
                s->decompress.handle    = cruise->init( s->mmfp );
                break;
            }
            cruise = cruise->next;
        }
        if( !cruise )
        {
            // TODO : Handle this error such that mikamp still recovers but does not load
            // any sample data.
            _mmlog( "Mikamp > SampleLoader > Failed to find suitable sample decompression API!" );
            assert( FALSE );
        }
    }

    sload->cursamp = s;

    //_mmforbid_exit( mmcs );
    return TRUE;
}

// _____________________________________________________________________________________
// Preps the sample manager for loading the next sample in the samplelist.  After a call
// to this function calls to SampleLoader_LoadChunk will load sampledata for the next
// unloaded sample in the sample list.
//
// Returns:
//   NULL if all samples are loaded / loading is complete.
//   else returns pointer to sample being loaded

SL_SAMPLE *SampleLoader_StartNext( MDRIVER *md )
{
    SAMPLE_LOADER *sload;
    SL_SAMPLE     *toload;

    if( !md ) return NULL;
    sload = md->sampload;
    if( !sload ) return NULL;

    // Notice : You should Call SampleLoader_End before calling this function!
    assert( !sload->cursamp );
    if( sload->cursamp ) SampleLoader_End( sload );

    if( !sload->lastload )
        toload = sload->samplelist;
    else
        toload = sload->lastload->next;
    
    if( !toload )
    {
        sload->lastload = NULL;
        return NULL;
    }

    SampleLoader_Start( sload, toload );
    return toload;
}

// _____________________________________________________________________________________
// Clean up left-over datajunk from last sample load (usually stuff allocated by custom
// decompression routines)
//
void SampleLoader_End( SAMPLE_LOADER *sload )
{
    SL_SAMPLE     *s;

    if( !sload ) return;
    s = sload->cursamp;
    if( !s || !s->managed->length ) return;

    if( ( s->decompress.type != SL_COMPRESS_NONE ) && s->decompress.handle && s->decompress.api->cleanup )
        s->decompress.api->cleanup( s->decompress.handle );

    if( s->filemode == SL_OPEN_FILENAME )
        _mm_fclose( s->mmfp );

    s->decompress.handle = NULL;
    s->mmfp              = NULL;
    sload->lastload      = s;
    sload->cursamp       = NULL;

    // Done to ensure that sample libs/modules which don't use absolute seek indexes
    // still load sample data properly... because sometimes the scaling routines
    // won't load every last byte of the sample.
    //  Removed for unknown reasons.

    //if(sl_rlength > 0) _mm_fseek(s->mmfp,sl_rlength,SEEK_CUR);
}

// _____________________________________________________________________________________
// Loads a chunk of sample data and ensures it formatted properly.  This function is
// complex to allow chunking so that very large samples can be loaded into device memory
// without the need to allocate megs of ram to do it.  Chunking is also useful for
// streaming audio.
// This function should only be called from Mikamp drivers (and virtch), and should
// never have to be used by an end user or any other part of mikamp really.
//
// length  - number of samples to read.  Any unread data will be skipped.
//    This way, the drivers can dictate to only load a portion of the
//    sample, if such behaviour is needed for any reason.
//
// outfmt  - Output format flags wanted/needed by device.
//
void SampleLoader_LoadChunk( SAMPLE_LOADER *sload, void *buffer, int length, uint outfmt )
{
    const SL_SAMPLE *smp;
    enum SF_BITS     outdepth;
    enum MD_CHANNELS outchan;

    int        t, u;
    int        inlen;        // length of the input and output (in samples)
    SBYTE     *bptr = (SBYTE *)buffer;
    SWORD     *wptr = (SWORD *)buffer;

    // Assert Notice here means you need to call SampleLoader_Start() first!
    assert( !sload || ( sload && sload->cursamp ) );
    if( !sload || !sload->cursamp ) return;

    smp      = sload->cursamp;
    if( smp->managed->handle == -1 ) return;    // abort if no memory has been allocated.

    while( length )
    {
        // Get the # of samples to process
        // -------------------------------
        // Must be less than the following: sl_rlength, 32768, and length!

        inlen  = MIN( sload->rlength, 32768 );
        if( inlen > length ) inlen  = length;

        // ---------------------------------------------
        // Load and decompress sample data

        if( smp->decompress.api )
        {
            switch( smp->indepth )
            {
                case SF_BITS_8:
                    inlen = smp->decompress.api->decompress8( smp->decompress.handle, sload->buffer, inlen, smp->mmfp );
                break;

                case SF_BITS_16:
                    inlen = smp->decompress.api->decompress16( smp->decompress.handle, sload->buffer, inlen, smp->mmfp );
                break;
            }
        }

        if( smp->streamback )
        {
            SBYTE   *ss;
            SWORD   *dd;
            int      tt;

            inlen = smp->streamback( smp, sload->buffer, buffer, inlen );
            if( !inlen ) return;

            // TODO : tEMP hACK aLERT !!
            // Check if the indepth is 8 bit and if so, manually convert it to 16 bit,
            // because that's what the next block of code expects until we fix it.

            ss  = (SBYTE *)sload->buffer;
            dd  = (SWORD *)sload->buffer;
            ss += inlen;
            dd += inlen;

            for( tt=0; tt<inlen; tt++ )
            {
                ss--;
                dd--;
                *dd = (*ss) << 8;
            }
        }

        // TODO : Adapt this auto-check conversion code to have customized/optimized
        // versions for each 8, 16, 24, and 32 bit samples?

        outdepth = smp->managed->bitdepth;
        outchan  = smp->managed->channels;
        length  *= outchan;

        // ---------------------------------------------
        // Delta-to-Normal sample conversion

        if( smp->infmt & SF_DELTA )
        {
            for(t=0; t<inlen; t++)
            {
                sload->buffer[t] += sload->delta_old;
                sload->delta_old  = sload->buffer[t];
            }
        }

        // ---------------------------------------------
        // signed/unsigned sample conversion!

        if( ( smp->infmt^outfmt ) & SF_SIGNED )
        {
            for(t=0; t<inlen; t++)
                sload->buffer[t] ^= 0x8000;
        }

        if( smp->inchan == MD_STEREO )
        {
            if( !( outchan == MD_STEREO ) )
            {
                // convert stereo to mono!  Easy!
                // NOTE: Should I divide the result by two, or not?
                SWORD  *s, *d;
                s = d = sload->buffer;

                for(t=0; t<inlen; t++, s++, d+=2)
                    *s = (*d + *(d+1)) / 2;
            }
        }
        else
        {
            if( outchan & MD_STEREO )
            {
                // Yea, it might seem stupid, but I am sure someone will do
                // it someday - convert a mono sample to stereo!
                SWORD  *s, *d;
                s = d = sload->buffer;
                s += inlen;
                d += inlen;

                for(t=0; t<inlen; t++)
                {
                    s -= 2;
                    d--;
                    *s = *(s+1) = *d;
                }
            }
        }

        if( smp->managed->scalefactor > 1 )
        {
            int   idx = 0;
            SLONG scaleval;

            // Sample Scaling... average values for better results.
            t = 0;
            while( (t < inlen) && length )
            {
                scaleval = 0;
                for(u=smp->managed->scalefactor; u && (t < inlen); u--, t++)
                    scaleval += sload->buffer[t];
                sload->buffer[idx++] = scaleval / (smp->managed->scalefactor-u);
                length--;
            }
        }

        length         -= inlen;
        sload->rlength -= inlen;

        // ---------------------------------------------
        // Normal-to-Delta sample conversion

        if( outfmt & SF_DELTA )
        {
            for( t=0; t<inlen; t++ )
            {   
                int   ewoo       = sload->delta_new;
                sload->delta_new = sload->buffer[t];
                sload->buffer[t] = sload->buffer[t] - ewoo;
            }
        }

        switch( outdepth )
        {
            case SF_BITS_8:
            {
                if( outfmt & SF_SIGNED )
                {
                    for(t=0; t<inlen; t++, bptr++)
                    {
                        int  eep = ( sload->buffer[t]+sload->ditherval );
                        *bptr    =  _mm_boundscheck( eep, -32768l, 32767l ) >> 8;
                        sload->ditherval = sload->buffer[t] - (eep & ~255l);
                    }
                }
                else
                {
                    for( t=0; t<inlen; t++, bptr++ )
                    {
                        int  eep = (sload->buffer[t]+sload->ditherval);
                        *bptr    =  _mm_boundscheck( eep, 0, 65535l ) >> 8;
                        sload->ditherval = sload->buffer[t] - (eep & ~255l);
                    }
                }
            }
            break;

            case SF_BITS_16:
            {
                memcpy( wptr, sload->buffer, inlen*2 );
                wptr += inlen;
            }
            break;
        }
    }
}

// _____________________________________________________________________________________
// Adds a sample to the Managed SampleLoader list.
//
// in_format   - input format, see SF_* sample formmating flags
// in_bitdepth - input (disk) bitdepth
//
SL_SAMPLE *SampleLoader_AddSample( MDRIVER *md, uint in_format, enum SF_BITS in_bitdepth, enum MD_CHANNELS in_channels, uint length )
{
    SL_SAMPLE       *news, *cruise;
    SAMPLE_LOADER   *sload;
    if( !md ) return FALSE;

    if( !md->sampload )
        SampleLoader_Create( md );

    sload = md->sampload;
    if( !sload ) return NULL;

    cruise = sload->samplelist;

    // Allocate and add structure to the END of the list
    // -------------------------------------------------
    // Duplicate Check Notes [not implemented yet]:
    //  If the filename or filepointer and seekpos match then the
    //   sample is a duplicate.  I don't do this check yet because I'm not sure
    //   how useful it would be?

    if(( news = _mmobj_allocblock( sload, SL_SAMPLE ) ) == NULL) return NULL;

    if( cruise )
    {
        while( cruise->next )  cruise = cruise->next;
        cruise->next = news;
    }
    else
        sload->samplelist = news;

    news->infmt           = in_format;
    news->indepth         = in_bitdepth;
    news->inchan          = in_channels;
    news->decompress.type = SL_COMPRESS_NONE;
    
    news->managed         = ManagedSample_Create( md, length );

    return news;
}

// _____________________________________________________________________________________
// Loads the next sample in the driver load lists.
//
// Returns:
//   TRUE   - all samples are loaded / loading is complete.
//   FALSE  - there are more samples to load.
//
BOOL SampleLoader_LoadNextSample( MDRIVER *md )
{
    SL_SAMPLE      *samp;

    if( !md || !md->sampload ) return TRUE;
    if( !md->sample_manager.active || !md->sample_manager.uptodate )
        SampleManager_Commit( md );

    samp = SampleLoader_StartNext( md );
    if( !samp ) return TRUE;
    md->device.SampleLoad( &md->device, md->sampload, samp );
    SampleLoader_End( md->sampload );

    return FALSE;

    //SL_SampleDelta( s, FALSE );
    //SL_Init( s, &md->allochandle );
    //result = md->device.SampleLoad( &md->device, s, type );
    //SL_Exit( s );
    //return result;
}

// _____________________________________________________________________________________
// Loads all samples stored in the sample cache in one shot!
// This function is provided for simple/quick projects and testing.  It is HIGHLY recom-
// mended that you use Mikamp_LoadNextSample instead, so that you can check for messages
// and update status bars, etc. during the loading process.
//
void SampleLoader_LoadSamples( MDRIVER *md )
{
    SL_SAMPLE *samp;

    if( !md || !md->sampload ) return;
    if( !md->sample_manager.active || !md->sample_manager.uptodate )
        SampleManager_Commit( md );

    /*if( md->device.sl_static )
    {
        while( ( samp = SampleLoader_StartNext( md->device.sl_static ) ) != NULL )
        {
            md->device.SampleLoad( &md->device, samp );
            SampleLoader_End( md->device.sl_static );
        }
    }*/

    while( ( samp = SampleLoader_StartNext( md ) ) != NULL )
    {
        md->device.SampleLoad( &md->device, md->sampload, samp );
        SampleLoader_End( md->sampload );
    }
}

void SL_In_SetStream( SL_SAMPLE *s, SL_API_STREAM_CALLBACK, void *streamdata )
{
    if( !s ) return;
    s->streamback      = streamback;
    s->streamdata      = streamdata;
}

void SL_In_SetStreamFP( SL_SAMPLE *s, SL_API_STREAM_CALLBACK, void *streamdata, MMSTREAM *fp, uint seekpos )
{
    if( !s ) return;
    SL_In_SetStream( s, streamback, streamdata );
    s->filemode        = SL_USE_FILEPOINTER;
    if( s->decompress.type == SL_COMPRESS_NONE )
        s->decompress.type = SL_COMPRESS_RAW;
    s->mmfp            = fp;
    s->seekpos         = seekpos;    
}

void SL_In_SetStreamFilename( SL_SAMPLE *s, SL_API_STREAM_CALLBACK, void *streamdata, const CHAR *filename, uint seekpos )
{
    if( !s ) return;
    SL_In_SetStream( s, streamback, streamdata );
    s->filemode        = SL_OPEN_FILENAME;
    if( s->decompress.type == SL_COMPRESS_NONE )
        s->decompress.type = SL_COMPRESS_RAW;
    s->filename        = filename;
    s->seekpos         = seekpos;    
}

void SL_In_SetFileFP( SL_SAMPLE *s, MMSTREAM *fp, uint seekpos )
{
    if( !s ) return;
    s->filemode        = SL_USE_FILEPOINTER;
    if( s->decompress.type == SL_COMPRESS_NONE )
        s->decompress.type = SL_COMPRESS_RAW;
    s->mmfp            = fp;
    s->seekpos         = seekpos;
}

void SL_In_SetFileName( SL_SAMPLE *s, const CHAR *filename, uint seekpos )
{
    if( !s ) return;
    s->filemode         = SL_OPEN_FILENAME;
    if( s->decompress.type == SL_COMPRESS_NONE )
        s->decompress.type  = SL_COMPRESS_RAW;
    s->filename         = filename;
    s->seekpos          = seekpos;
}

void SL_In_Decompress( SL_SAMPLE *s, uint compress_type )
{
    if( !s ) return;
    s->decompress.type = compress_type;
}

////////////////////////////////////////////////////////////////////////////////////////
// Input Format Configuration Functions
//
// Sets the known/expected condition of the sample data being read from disk or being
// streamed in from the decompression callback.  This is combined with outfmt to deter-
// mine the most efficient series of functions needed to convert the sample data to
// outfmt requested.
//
// You could alter the infmt variable directly, but this is a nice human-readable
// layer.

void SL_In_16bit( SL_SAMPLE *s )
{
    if( !s ) return;
    s->indepth = SF_BITS_8;
}

void SL_In_8bit( SL_SAMPLE *s )
{
    if( !s ) return;
    s->indepth = SF_BITS_16;
}

void SL_In_Signed( SL_SAMPLE *s )
{
    if( !s ) return;
    s->infmt |= SF_SIGNED;
}

void SL_In_Unsigned( SL_SAMPLE *s )
{
    if( !s ) return;
    s->infmt &= ~SF_SIGNED;
}

void SL_In_Delta( SL_SAMPLE *s, BOOL yesno )
{
    if( !s ) return;
    if( yesno )
        s->infmt |= SF_DELTA;
    else
        s->infmt &= ~SF_DELTA;
}

