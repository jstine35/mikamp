/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: mi16.c

  Assembly Mixer Plugin : Mono 16 bit Sample data / Interpolation

  Generic all-purpose assembly wrapper for mikamp VMIXER plugin thingie.
  As long as your platform-dependant asm code uses the naming conventions
  below (Asm StereoInterp, SurroundInterp, etc), then you can use this
  module and the others related to it to register your mixer into mikamp.

  See Also:

    mn8.c
    sn8.c
    si8.c
    mn16.c
    sn16.c
    mi16.c
    si16.c
*/

#include "mikamp.h"
#include "..\wrap16.h"
#include "asmapi.h"


VMIXER ASM_M16_MONO_INTERP =
{
    NULL,
    "Assembly Mono-16 (Mono/Interp) v0.1",

    nc16_Check_MonoInterp,
    SF_BITS_16,
    MD_MONO,
    MD_MONO,

    NULL,
    NULL,
    VC_Volcalc16_Mono,
    VC_Volramp16_Mono,
    Mix16MonoInterp_NoClick,
    Mix16MonoInterp_NoClick,
    Asm16MonoInterp,
    Asm16MonoInterp,
};

VMIXER ASM_M16_STEREO_INTERP =
{
    NULL,
    "Assembly Mono-16 (Stereo/Interp) v0.1",

    nc16_Check_StereoInterp,
    SF_BITS_16,
    MD_MONO,
    MD_STEREO,

    NULL,
    NULL,
    VC_Volcalc16_Stereo,
    VC_Volramp16_Stereo,
    Mix16StereoInterp_NoClick,
    Mix16SurroundInterp_NoClick,
    Asm16StereoInterp,
    Asm16SurroundInterp,
}; 
