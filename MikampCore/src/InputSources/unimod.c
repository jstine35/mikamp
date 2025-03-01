/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 unimod.c
 
  Structure handling and manipulation.  Includes loading, freeing, and track
  manipulation functions for the UNIMOD format.

*/

#include <string.h>
#include <stdarg.h>
#include "mikamp.h"
#include "uniform.h"

// _____________________________________________________________________________________
//
static BOOL loadsamples( MDRIVER *md, UNIMOD *of, MMSTREAM *fp )
{
    UNISAMPLE  *s;
    int         u;

    for( u=of->numsmp, s=of->samples; u; u--, s++ )
    {
        if( s->length )
        {   
            // if the seekpos is 0, then we need to set it automatically, assuming that each
            // sample follows suit from the current file seek position

            SL_SAMPLE *sap = SampleLoader_AddSample( md, s->format, s->bitdepth, s->channels, s->length );
            ManagedSample_AddUserHandle( sap->managed, &s->handle );
            SL_In_Decompress( sap, s->compress );
            SL_In_SetFileFP( sap, fp, s->seekpos );
        }
    }
    return 1;
}


/******************************************

    Next are the user-callable functions

******************************************/

// _____________________________________________________________________________________
// Unloads all samples, and sets the sample handles to -1.
//
void Unimod_UnloadSamples(UNIMOD *mf)
{
    if(mf && mf->md && mf->samples)
    {   uint t;
        _mmlogd("Unimod_UnloadSamples > Unloading Sampledata...");
        for(t=0; t<mf->numsmp; t++)
        {   if(mf->samples[t].handle >= 0)
            {   MD_SampleUnload(mf->md, mf->samples[t].handle);
                mf->samples[t].handle = -1;
            }
        }
        _mmlogd(" UnloadSamples Done!");
    }
}

// _____________________________________________________________________________________
// Before a proper shutdown of a loaded module, we have to unload the samples from the
// driver.  They are not allocated as part of our unimod allocation tree, hence:
//
static void ML_Shutdown(UNIMOD *mf)
{
    if(mf && mf->samples)
    {   uint t;
        for(t=0; t<mf->numsmp; t++)
            MD_SampleUnload(mf->md, mf->samples[t].handle);
    }
}

// _____________________________________________________________________________________
//
void Unimod_Free(UNIMOD *mf)
{
    if(!mf) return;
    _mmlog("Unimod > Unloading module \'%s\'", mf->songname);
    _mmalloc_close(mf->allochandle);
}

extern MLOADER *firstloader;

// _____________________________________________________________________________________
//
CHAR *Unimod_LoadTitle(CHAR *filename)
{
    MLOADER *l;
    CHAR    *retval;
    MMSTREAM *mmp;

    if((mmp = _mm_fopen(filename,"rb"))==NULL) return NULL;

    // Try to find a loader that recognizes the module

    for(l=firstloader; l; l=l->next)
    {   _mm_rewind(mmp);
        if(l->enabled && l->Test(mmp)) break;
    }

    if(l==NULL)
    {   CHAR  stmp[_MAX_PATH + 64];
        sprintf(stmp,"Module file: %s\nThis is an unknown module format or corrupted a file.", filename);
        _mmerr_set(MMERR_UNSUPPORTED_FILE, "Failure loading module music", stmp);
        return NULL;
    }

    retval = l->LoadTitle(mmp);

    _mm_fclose(mmp);
    return(retval);
}

// _____________________________________________________________________________________
// Loads a module given a file pointer.  Useable for situations that involve packed file
// formats - a single file that contains all module data, or UniMod modules which use the
// sample library feature.
//
// - Songs and samples are loaded from the file positions specified.
// - The file positions will remain unchanged after the call to this procedure.
// - mode specifies the module as MM_STATIC or MM_DYNAMIC
//
UNIMOD *Unimod_LoadFP(MDRIVER *md, MMSTREAM *modfp, MMSTREAM *smpfp, int mode)
{
    uint         t;
    MLOADER     *l;
    BOOL         ok;
    UNIMOD      *mf;
    void        *lh;

    {
        MM_ALLOC    *allochandle;
        allochandle = _mmalloc_create("UNIMOD-LOADING", NULL);
        mf = _mm_calloc(allochandle, 1, sizeof(UNIMOD));
        mf->allochandle = allochandle;
    }
    _mmalloc_setshutdown(mf->allochandle, (void (*)(void *))ML_Shutdown, mf);

    // Try to find a loader that recognizes the module

    for(l=firstloader; l; l=l->next)
    {   _mm_rewind(modfp);
        if(l->enabled && l->Test(modfp)) break;
    }

    if(l==NULL)
    {   //_mmerr_set(MMERR_UNSUPPORTED_FILE, "Failure loading module music", "Unknown module format or corrupted file.");
        return NULL;
    }

    mf->initvolume = 128;
    mf->memsize    = PTMEM_LAST;

    // init panning array

    for(t=0; t<64; t++) mf->panning[t] = ((t+1)&2) ? (PAN_RIGHT-l->defpan) : (PAN_LEFT+l->defpan);
    
    // default channel volume set to max...
    for(t=0; t<64; t++) mf->chanvol[t] = 64;

    mf->pansep=128; /* Full stereo pan. */

    // all channels are unmuted.
    memset(mf->muted, 0, sizeof(BOOL) * 64);

    // init module loader and load the header / patterns
    if(lh = l->Init())
    {
        _mm_rewind(modfp);
        ok = l->Load(lh, mf, modfp);
    } else ok = 0;

    // free loader and unitrk allocations
    mf->numtrk = utrk_cleanup(mf->ut);
    l->Cleanup(lh);

    if(!ok)
    {
        Unimod_Free(mf);
        return NULL;
    }

    // Check the number of positions (song order list).  If there are none, then we
    // generate a 'dud' list which comprises of all the patterns of the song, in order.

    if(!mf->numpos)
    {
        uint   k;

        AllocPositions(mf, mf->numpos = mf->numpat);
        for(k=0; k<mf->numpat; k++) mf->positions[k] = k;
    }
    /*else
    {   // Make sure there aren't any illegal positions.
        uint  k;        
        
        for(k=0; k<mf->numpos; k++)
            if(mf->positions[k] > mf->numpat);
    }*/

    if( md && smpfp )
    {
        if( !loadsamples(md, mf, smpfp) )
        {
            Unimod_Free( mf );
            return NULL;
        }
    }

    if( l->nopaneff ) mf->flags |= UF_NO_PANNING;
    if( l->noreseff ) mf->flags |= UF_NO_RESONANCE;

    mf->md = md;

    if( mf->allochandle )
    {   
        // Set Allochandle Blockname
        // -------------------------
        // Oh what wondrous crap I put in place for the sake of easier debugging!

        sprintf( mf->allochandle->name, "UNIMOD-%.16s",mf->songname );
    }

    return mf;
}

// _____________________________________________________________________________________
// Sets the default panning position for all registered loaders.
//
void Unimod_SetDefaultPan(int defpan)
{
    MLOADER   *cruise = firstloader;

    while(cruise)
    {
        cruise->defpan = defpan;
        cruise = cruise->next;
    }
}

// _____________________________________________________________________________________
// Returns FALSE on error, TRUE on success!
//
BOOL Unimod_LoadSamples(UNIMOD *mf, MDRIVER *md, MMSTREAM *smpfp)
{
    if(mf && md && smpfp)
    {
        if( !loadsamples( md, mf, smpfp ) ) return FALSE;
        SampleLoader_LoadSamples( md );
        mf->md = md;
    }
    return TRUE;
}

// _____________________________________________________________________________________
// Open a module via it's filename and loads only the header information.
//
UNIMOD *Unimod_LoadInfo(const CHAR *filename)
{
    MMSTREAM  *fp;
    UNIMOD    *mf;
    
    if((fp = _mm_fopen(filename,"rb"))==NULL) return NULL;
    if(mf = Unimod_LoadFP(NULL, fp, NULL, MD_ALLOC_STATIC))
    {
        //int   i;
        //if(mf->loopcount = config_loopcount) mf->loop = 1; else mf->loop = 0;
        //mf->extspd = 1;
        //mf->patloop = (PATLOOP *)_mm_calloc(mf->numchn, sizeof(PATLOOP));
        //mf->memory  = (MP_EFFMEM **)_mm_calloc(mf->numchn, sizeof(MP_EFFMEM *));
        //for(i=0; i<mf->numchn; i++)
            //if((mf->memory[i]  = (MP_EFFMEM *)_mm_calloc(mf->memsize, sizeof(MP_EFFMEM))) == NULL)
            //{   Mikamp_FreeSong(mf);  return NULL;  }
        
        //MP_QuickClean(mf);
        //PredictSongLength(mf);

        _mm_fseek(fp,0,SEEK_END);
        mf->filesize = _mm_ftell(fp);
        mf->filename = _mm_strdup(mf->allochandle, filename);

    }
    _mm_fclose(fp);
    
    return mf;
}

// _____________________________________________________________________________________
// Open a module via it's filename.  This is fairly automated song loader,
// which does not support fancy things like sample libraries (for shared
// samples and instruments).  See song_loadfp for those.
//
// In addition, this puppy will also load the samples automatically, and it
// sets the filesize.
//
UNIMOD *Unimod_Load( MDRIVER *md, const CHAR *filename )
{
    MMSTREAM *fp;
    UNIMOD  *mf;
    
    _mmlogd( "Mikamp > unimod_load > Loading Module : %s",filename );

    if( ( fp = _mm_fopen( filename,"rb" ) ) == NULL ) return NULL;
    if( mf = Unimod_LoadFP( md, fp, fp, MD_ALLOC_STATIC ) )
    {
        // TODO : Add Try/Catch exception handling here?
        //  or fix LoadSamples to return true/false success value?
        SampleLoader_LoadSamples( md );
        /*{
            Unimod_Free( mf );
            mf = NULL;
        }
        else*/
        {
            _mm_fseek( fp, 0, SEEK_END );
            mf->filesize = _mm_ftell( fp );
        }
    }

    _mm_fclose( fp );
    if( !mf )
    {
        CHAR  stmp[_MAX_PATH + 64];
        _mmlog( "Unimod > Module load failed : %s", filename );
        sprintf( stmp, "Module file: %s\nThis is an unknown module format or corrupted a file.", filename );
        _mmerr_set( MMERR_UNSUPPORTED_FILE, "Failure loading module music", stmp );
    }
    return mf;
}

// _____________________________________________________________________________________
// loads a song from memory instead of a filename.  pointers to both the module
// header and samples should be given.
//
UNIMOD *Unimod_LoadMem(MDRIVER *md, void *module)
{
    MMSTREAM *fp;
    UNIMOD   *mf;
    
    if( ( fp = _mmstream_createmem( module, 0 ) ) == NULL ) return NULL;
    if( mf = Unimod_LoadFP(md, fp, fp, MD_ALLOC_STATIC) )
    {
        // TODO : Add Try/Catch exception handling here?
        //  or fix LoadSamples to return true/false success value?
        SampleLoader_LoadSamples( md );
        /*{
        Unimod_Free( mf );
        mf = NULL;
        }
        else*/
        {
            _mm_fseek( fp, 0, SEEK_END );
            mf->filesize = _mm_ftell( fp );
        }
    }

    _mm_fclose(fp);

    return mf;
}

// _____________________________________________________________________________________
// Returns TRUE for a pattern that has any sort of pattern data in it.  Global track 
// contents are considered during the check.
// TODO: add a flag so that I can optionally include the global track in the check.
//
BOOL _mm_inline MF_PatternInuse(UNIMOD *mf, uint pattern)
{

    if(pattern < mf->numpat)
    {
        uint   i = (pattern*mf->numchn),
               t = i+mf->numchn;

        uint   longlen;

        //longlen = utrk_global_getlength(mf->globtracks[pattern]);

        longlen = 0;
        for(; i<t; i++)
        {   // find the length of the longest track, in rows.
            uint  res = utrk_local_getlength(mf->tracks[mf->patterns[i]]);
            if(res > longlen) longlen = res;
        }
        if(longlen)
        {   if(longlen<mf->pattrows[pattern])
                mf->patpos_silence = longlen;
            return TRUE;
        }
    }

    return FALSE;
}

// _____________________________________________________________________________________
// strips trailing silence from the end of a song, by searching backwards through the
// order list and decrementing the order count as long as they are completely empty.
// Threshold is the time, in milliseconds.  IF ZERO, no stripping will occur!
//
void Unimod_StripSilence(UNIMOD *mf, long threshold)
{

    uint   i;
    uint   numpos = mf->numpos;

    for(i=(mf->numpos-1); i; i--)
        if(!MF_PatternInuse(mf, mf->positions[i]))
            numpos--;
        else break;

    if(numpos < mf->numpos)
    {   mf->sngpos_silence  = numpos;
        mf->strip_threshold = threshold;
    }
}
