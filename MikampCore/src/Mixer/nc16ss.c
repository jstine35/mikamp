/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: nc16.c

 Description:
  Mix 16 bit *STEREO* (oh yea!) data with volume ramping.  These functions 
  use special global variables that differ fom the ones used by the normal 
  mixers, so that they do not interfere with the true volume of the sound.

  (see v16 extern in wrap16.h)

*/

#include "mikamp.h"
#include "wrap16.h"


BOOL nc16ss_Check_Mono(uint mixmode, uint format, uint flags)
{
    return 1;
}


BOOL nc16ss_Check_MonoInterp(uint mixmode, uint format, uint flags)
{
    if(mixmode & DMODE_INTERP) return 1;
    return 0;
}


BOOL nc16ss_Check_Stereo(uint mixmode, uint format, uint flags)
{
    return 1;
}


BOOL nc16ss_Check_StereoInterp(uint mixmode, uint format, uint flags)
{
    if(mixmode & DMODE_INTERP) return 1;
    return 0;
}


// _____________________________________________________________________________________
//
void __cdecl Mix16StereoSS_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   v16.left.vol    += v16.left.inc;
        v16.right.vol   += v16.right.inc;
        *dest++ += (v16.left.vol  / BIT16_VOLFAC) * srce[(himacro(index)*2)];
        *dest++ += (v16.right.vol / BIT16_VOLFAC) * srce[(himacro(index)*2)+1];
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix16StereoSSI_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot;

        v16.left.vol   += v16.left.inc;
        v16.right.vol  += v16.right.inc;

        sroot = srce[(himacro(index)*2)];
        *dest++  += (v16.left.vol  / BIT16_VOLFAC) * (SWORD)(sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        sroot = srce[(himacro(index)*2) + 1];
        *dest++  += (v16.right.vol / BIT16_VOLFAC) * (SWORD)(sroot + ((((SLONG)srce[(himacro(index)*2) + 3] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));


        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix16MonoSS_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   v16.left.vol     += v16.left.inc;
        *dest++          += ((v16.left.vol  / BIT16_VOLFAC) * (srce[himacro(index)] + srce[(himacro(index)*2)+1])) / 2;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix16MonoSSI_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot = srce[himacro(index)*2], crap;
        v16.left.vol += v16.left.inc;
        crap  = (sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        sroot = srce[(himacro(index)*2) + 1];        
        *dest++ += (v16.left.vol  / BIT16_VOLFAC) * (crap + (sroot + ((((SLONG)srce[(himacro(index)*2) + 2] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS))) / 2;

        index   += increment;
    }
}
