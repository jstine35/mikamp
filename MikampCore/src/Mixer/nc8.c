/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: nc8.c

 Description:
  Mix 8 bit data with volume ramping.  These functions use special global
  variables that differ fom the oes used by the normal mixers, so that they
  do not interfere with the true volume of the sound.

  (see v8 extern in wrap8.h)

*/

#include "mikamp.h"
#include "wrap8.h"


BOOL nc8_Check_Mono(uint mixmode, uint format, uint flags)
{
    return 1;
}


BOOL nc8_Check_MonoInterp(uint mixmode, uint format, uint flags)
{
    if(mixmode & DMODE_INTERP) return 1;
    return 0;
}


BOOL nc8_Check_Stereo(uint mixmode, uint format, uint flags)
{
    return 1;
}


BOOL nc8_Check_StereoInterp(uint mixmode, uint format, uint flags)
{
    if(mixmode & DMODE_INTERP) return 1;
    return 0;
}


// _____________________________________________________________________________________
//
void __cdecl Mix8StereoNormal_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   v8.left.vol    += v8.left.inc;
        v8.right.vol   += v8.right.inc;
        *dest++ += (int)(v8.left.vol  * srce[himacro(index)]);
        *dest++ += (int)(v8.right.vol * srce[himacro(index)]);
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix8StereoInterp_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot  = srce[himacro(index)];

        sroot = (SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS));

        v8.left.vol    += v8.left.inc;
        v8.right.vol   += v8.right.inc;

        *dest++  += (int)(v8.left.vol  * sroot);
        *dest++  += (int)(v8.right.vol * sroot);
        index    += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix8SurroundNormal_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG sample;

        v8.left.vol += v8.left.inc;
        sample        = (int)(v8.left.vol * srce[himacro(index)]);

        *dest++ += sample;
        *dest++ -= sample;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix8SurroundInterp_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot = srce[himacro(index)];
        v8.left.vol += v8.left.inc;
        sroot         = (int)(v8.left.vol * (SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)));

        *dest++ += sroot;
        *dest++ -= sroot;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix8MonoNormal_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    SLONG  sample;

    for(; todo; todo--)
    {   v8.left.vol  += v8.left.inc;
        sample        = (int)(v8.left.vol * srce[himacro(index)]);

        *dest++ += sample;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Mix8MonoInterp_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   SLONG  sroot = srce[himacro(index)];
        v8.left.vol += v8.left.inc;
        sroot         = (int)(v8.left.vol * (SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)));

        *dest++ += sroot;
        index   += increment;
    }
}
