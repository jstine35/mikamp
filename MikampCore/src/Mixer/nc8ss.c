/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 nc8.c

 Description:
  Mix 8 bit *STEREO* (oh yea!) data with volume ramping.  These functions 
  use special global variables that differ fom the ones used by the normal 
  mixers, so that they do not interfere with the true volume of the sound.

  (see v8 extern in wrap8.h)

*/

#include "mikamp.h"
#include "wrap8.h"

BOOL nc8ss_Check_Mono(uint mixmode, uint format, uint flags)
{
    return 1;
}


BOOL nc8ss_Check_MonoInterp(uint mixmode, uint format, uint flags)
{
    if(mixmode & DMODE_INTERP) return 1;
    return 0;
}


BOOL nc8ss_Check_Stereo(uint mixmode, uint format, uint flags)
{
    return 1;
}


BOOL nc8ss_Check_StereoInterp(uint mixmode, uint format, uint flags)
{
    if(mixmode & DMODE_INTERP) return 1;
    return 0;
}

// _____________________________________________________________________________________
//
void __cdecl Mix8StereoSS_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   v8.left.vol    += v8.left.inc;
        v8.right.vol   += v8.right.inc;
        *dest++ += v8.left.vol  * srce[(himacro(index)*2)];
        *dest++ += v8.right.vol * srce[(himacro(index)*2)+1];
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix8StereoSSI_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
	for(; todo; todo--)
    {   SLONG  sroot;

        v8.left.vol   += v8.left.inc;
        v8.right.vol  += v8.right.inc;

        sroot = srce[(himacro(index)*2)];
        *dest++  += v8.left.vol  * (SBYTE)(sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        sroot = srce[(himacro(index)*2) + 1];
        *dest++  += v8.right.vol * (SBYTE)(sroot + ((((SLONG)srce[(himacro(index)*2) + 3] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));


        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix8MonoSS_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   v8.left.vol     += v8.left.inc;
        *dest++          += (v8.left.vol * (srce[himacro(index)] + srce[(himacro(index)*2)+1])) / 2;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix8MonoSSI_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot = srce[himacro(index)*2], crap;
        v8.left.vol += v8.left.inc;
        crap  = (sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        sroot = srce[(himacro(index)*2) + 1];        
        *dest++ += v8.left.vol * (crap + (sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS))) / 2;

        index   += increment;
    }
}
