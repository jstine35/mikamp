/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: nc16.c

 Description:
  Mix 16 bit data with volume ramping.  These functions use special global
  variables that differ fom the oes used by the normal mixers, so that they
  do not interfere with the true volume of the sound.

  (see v16 extern in wrap16.h)

*/

#include "mikamp.h"
#include "wrap16.h"


BOOL nc16_Check_Mono(uint mixmode, uint format, uint flags)
{
    return 1;
}


BOOL nc16_Check_MonoInterp(uint mixmode, uint format, uint flags)
{
    if(mixmode & DMODE_INTERP) return 1;
    return 0;
}


BOOL nc16_Check_Stereo(uint mixmode, uint format, uint flags)
{
    return 1;
}


BOOL nc16_Check_StereoInterp(uint mixmode, uint format, uint flags)
{
    if(mixmode & DMODE_INTERP) return 1;
    return 0;
}

// _____________________________________________________________________________________
//
void __cdecl Mix16StereoNormal_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   v16.left.vol    += v16.left.inc;
        v16.right.vol   += v16.right.inc;
        *dest++ += (int)(v16.left.vol  * srce[himacro(index)]) / BIT16_VOLFAC;
        *dest++ += (int)(v16.right.vol * srce[himacro(index)]) / BIT16_VOLFAC;
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix16StereoInterp_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot  = srce[himacro(index)];

        v16.left.vol    += v16.left.inc;
        v16.right.vol   += v16.right.inc;

        sroot = (SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        *dest++  += (int)(v16.left.vol  * sroot) / BIT16_VOLFAC;
        *dest++  += (int)(v16.right.vol * sroot) / BIT16_VOLFAC;
        index    += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix16SurroundNormal_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG sample;

        v16.left.vol += v16.left.inc;
        sample        = (int)(v16.left.vol * srce[himacro(index)]) / BIT16_VOLFAC;

        *dest++ += sample;
        *dest++ -= sample;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix16SurroundInterp_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot = srce[himacro(index)];
        v16.left.vol += v16.left.inc;
        sroot         = (int)(v16.left.vol * (SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS))) / BIT16_VOLFAC;

        *dest++ += sroot;
        *dest++ -= sroot;
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix16MonoNormal_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    SLONG  sample;

    for(; todo; todo--)
    {   v16.left.vol  += v16.left.inc;
        sample         = (int)(v16.left.vol * srce[himacro(index)])  / BIT16_VOLFAC;

        *dest++ += sample;
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix16MonoInterp_NoClick(SWORD *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot = srce[himacro(index)];
        v16.left.vol += v16.left.inc;
        sroot         = (int)(v16.left.vol * (SWORD)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)))  / BIT16_VOLFAC;

        *dest++ += sroot;
        index   += increment;
    }
}
