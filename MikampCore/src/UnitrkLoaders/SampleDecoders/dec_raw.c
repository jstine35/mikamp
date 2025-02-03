/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 dec_raw.c

 Standard raw sample data loader.  These are basically just fread placebos for
 samples which are not compressed in any way, shape, or form.

*/

#include "mikamp.h"

// _____________________________________________________________________________________
//
static void *Raw_Init( MMSTREAM *mmfp )
{
    //CHAR    stmp[32];
    //_mm_read_UBYTES(stmp, 30, mmfp);

    //RAW_HANDLE *raw = 
    return (void *)TRUE;
}

// _____________________________________________________________________________________
//
static void Raw_Cleanup( void *raw )
{
}

// _____________________________________________________________________________________
//
static BOOL Raw_Decompress16Bit( void *raw, SWORD *dest, int cbcount, MMSTREAM *mmfp )
{
    _mm_read_I_SWORDS( dest, cbcount, mmfp );
    return cbcount;
}

// _____________________________________________________________________________________
// convert 8 bit data to 16 bit!
// We do the conversion in reverse so that the data we're converting isn't overwritten
// by the result.
//
static BOOL Raw_Decompress8Bit( void *raw, SWORD *dest, int cbcount, MMSTREAM *mmfp )
{
    SBYTE   *s;
    SWORD   *d;
    int      t;

    _mm_read_SBYTES( (SBYTE *)dest, cbcount, mmfp );

    s  = (SBYTE *)dest;
    d  = dest;
    s += cbcount;
    d += cbcount;

    for(t=0; t<cbcount; t++)
    {
        s--;
        d--;
        *d = (*s) << 8;
    }
    return cbcount;
}


SL_DECOMPRESS_API dec_raw =
{
    NULL,
    SL_COMPRESS_RAW,
    Raw_Init,
    Raw_Cleanup,
    Raw_Decompress16Bit,
    Raw_Decompress8Bit,
};
