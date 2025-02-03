/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 sample_manager.c

  The sample manager is intended for use from the module/streamed audio loaders and the
  sample/sndfx loader.  Sample management is necessary for optimized use of audio hard-
  ware.  It allows our app to calcuate efficient adjustment of sample qualities scaled
  to match hardware limitations.  Management takes place on two levels: The lowest
  critical level is sample-space allocation.  This consists of nothing more than a
  handle, a audio format (outfmt), and allocated memory on the audio device. The upper
  level is the loading of sample data into the allocated memory space.

  When an instance of Mikamp initializes it binds to itself an instance of a sample
  manager.  All samples loaded into that driver are allocated through that sample manager
  instance.

  The sloader can use a combination of built-in common conversions and a user-defined
  conversion/decompression function to decode samples from disk into a memory buffer.
  sloader is intended for use by mixers and drivers.

 TODO:
  Create an compiler-define alternative that is a very simple one-shot allocator of
  memory, for low-overhead systems.

*/

#include "mikamp.h"
#include "mmforbid.h"
#include "mminline.h"

#include <string.h>
#include <assert.h>

// _____________________________________________________________________________________
// Run through list of managed samples and set all bound sample/driver handles to -1!
// Called by Mikamp_Exit.
//
void SampleManager_Close( MDRIVER *md )
{
    if( !md ) return;
}

// _____________________________________________________________________________________
// Registers a sample for allocation when SampleManager_Commit() is called.
//
// handle  - specifies
// infmt   - use SF_* flags
//
SM_SAMPLE *ManagedSample_Create( MDRIVER *md, uint length )
{
    SM_SAMPLE *news, *cruise;

    if( !md ) return NULL;
    cruise = md->sample_manager.samplelist;

    // Allocate and add structure to the END of the list
    // -------------------------------------------------
    // Duplicate Check [not implemented yet]:
    //  If the filename or filepointer and seekpos match then the
    //   sample is a duplicate.  I don't do this check yet because I'm not sure
    //   how useful it would be?

    if(( news = _mmobj_allocblock( md, SM_SAMPLE ) ) == NULL) return NULL;

    if( cruise )
    {
        while( cruise->next )  cruise = cruise->next;
        cruise->next = news;
    }
    else
        md->sample_manager.samplelist = news;

    //news->format      = format;
    //news->bitdepth    = bitdepth;
    news->handle      = -1;
    news->length      = length;

    md->sample_manager.uptodate = FALSE;

    return news;
}

// _____________________________________________________________________________________
//
SM_SAMPLE *SampleManager_FindSampleHandle( const MDRIVER *md, int handle )
{
    SM_SAMPLE *cruise;
    if( !md || handle == -1 || !md->sample_manager.active ) return NULL;
    
    cruise = md->sample_manager.samplelist;
    while( cruise && (cruise->handle != handle) )
        cruise = cruise->next;
    return cruise;
}

// _____________________________________________________________________________________
// Assigns a user-defined handle, which is set to the device/sample handle when the
// sample memory is allocated by SampleManager_Commit.  Passing NULL clears the handle.
// If the managed sample is active and allocated, the usr_handle is assigned the handle
// immediately (before function call return).
//
void ManagedSample_AddUserHandle( SM_SAMPLE *sample, int *usr_handle )
{
    if( !sample ) return;
    sample->usr_handle  = usr_handle;
    if( usr_handle ) *usr_handle = sample->handle;
}

// _____________________________________________________________________________________
// Removes the specified handle from the list of sample handles bound to this sample.
// If all handles have been removed, the sample is flagged for removal (actual removal
// doesn't take effect until next Commit and thus can be rolled back if another user
// handle is bound to the managed sample)
//
void ManagedSample_RemoveUserHandle( SM_SAMPLE *sample, int *usr_handle )
{
    if( !sample ) return;
}

// _____________________________________________________________________________________
// Forces the format of a sample to a specific setting.  The driver will still take liberty
// to reformat the sample if the device requires the sampledata in a different format.
//
void ManagedSample_ForceChannels( SM_SAMPLE *sample, enum MD_CHANNELS channels )
{
    if( !sample ) return;
    sample->channels = channels;
    sample->flags   |= SMF_FORCED_CHANNELS_ENABLE;
}

// _____________________________________________________________________________________
// Forces the bitdepth of a sample to a specific setting.  The driver will still take liberty
// to reformat the sample if the device requires the sampledata in a different bitdepth.
//
void ManagedSample_ForceBitdepth( SM_SAMPLE *sample, enum SF_BITS bitdepth )
{
    if( !sample ) return;
    sample->bitdepth = bitdepth;
    sample->flags   |= SMF_FORCED_BITDEPTH_ENABLE;
}

// _____________________________________________________________________________________
// Gives the SampleManager a hint as to what bitdepth this sample would be best used as.
// Obvious use is to load 16 bit samples at 8 bit (drive capabilities permitting).
// This is also useful for telling the SampleManager to load 8 bit samples as 8 bit,
// even if the device and the default bitdepth are set to 16 or 32. Without use of this
// function, such samples will be converted to 16/32 bit data when loaded.
//
void SM_Sample_Hint_Bitdepth( SM_SAMPLE *sample, enum SF_BITS bitdepth )
{
    if( !sample ) return;
    sample->hint_bitdepth = bitdepth;
    sample->flags        |= SMF_BITDEPTH_HINT;
}

// _____________________________________________________________________________________
// Gives the SampleManager a hint as to how many channels this sample would be best used
// as. This is useful for telling the SampleManager to load mono samples as stereo.
//
void SM_Sample_Hint_Channels( SM_SAMPLE *sample, enum MD_CHANNELS channels )
{
    if( !sample ) return;
    sample->hint_channels = channels;
    sample->flags        |= SMF_CHANNELS_HINT;
}

// _____________________________________________________________________________________
// Attempts to allocate memory for all requested samples.  Sample attributes will be
// adjusted as needed to allow all samples to fit within the confines of the audio device.
// (this includes downgrading to mono, 8 bit, and/or halving the sample quality).
//
// Returns FALSE on failure -- not enough memory or other serious driver failure.
//
BOOL SampleManager_Commit( MDRIVER *md )
{
    struct MD_SAMPLE_MANAGER  *sman;
    SM_SAMPLE       *cruise;
    //enum MD_CHANNELS dev_chan;
    enum SF_BITS     dev_bits;

    if( !md ) return FALSE;
    sman   = &md->sample_manager;
    cruise = sman->samplelist;

    // Propagate Settings
    // ------------------
    // Do this everytime before efficiency calculations to ensure we're
    // working with a "clean slate."  Note that we use the bitdepth "hint"
    // value over the default driver value, if one's specified.
    //
    // Important Note : We handle channels differently from bitdepth. We default
    //   to mono unless the hint or force *explicitly* indicates otherwise.

    //dev_chan = md->device.GetSampleCaps( md, MD_CAPS_CHANNELS );
    dev_bits = md->device.GetSampleCaps( md, MD_CAPS_BITDEPTH );

    while( cruise )
    {
        if( !(cruise->flags & SMF_FORCED_CHANNELS_ENABLE) )
            cruise->channels = (cruise->flags & SMF_CHANNELS_HINT) ?
                cruise->hint_channels : MD_MONO ;
        if( !(cruise->flags & SMF_FORCED_BITDEPTH_ENABLE) )
            cruise->bitdepth = (cruise->flags & SMF_BITDEPTH_HINT) ?
                cruise->hint_bitdepth : dev_bits;
        cruise = cruise->next;
    }

    // Check Memory Allocation Constraints
    // -----------------------------------
    // TODO : Re-implement all that smart sample-resizing logic in the event it's ever
    //   needed again (albeit as doubtful as that might seem right now).

    // Allocate Device Memory
    // ----------------------

    cruise = sman->samplelist;
    while( cruise )
    {
        cruise->handle      = md->device.SampleAlloc( &md->device, cruise );
        if( cruise->usr_handle )
            *cruise->usr_handle = cruise->handle;

        if( cruise->handle == -1 )
        {
            // TODO : Add more robust info, including bitdepth and flags (in written-out
            //   human-readable format preferrably)
            assert( FALSE );
            _mmlog( "Mikamp SampleManager > Allocation Failure: length = %d", cruise->length );
        }

        // Update all user handles:

        if( cruise->usr_handle ) *cruise->usr_handle = cruise->handle;

        cruise = cruise->next;
    }
    
    sman->active    = TRUE;
    sman->uptodate  = TRUE;

    return TRUE;
}