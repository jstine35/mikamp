/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: mi8.c
 
  Assembly Mixer Plugin : Mono 8 bit Sample data / Interpolation
  
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
#include "..\wrap8.h"
#include "asmapi.h"


VMIXER ASM_M8_MONO_INTERP =
{
    NULL,
    "Assembly Mono-8 (Mono/Interp) v0.1",

    nc8_Check_MonoInterp,
    SF_BITS_8,
    MD_MONO,

    MD_MONO,

    NULL,
    NULL,
    VC_Volcalc8_Mono,
    VC_Volramp8_Mono,
    Mix8MonoInterp_NoClick,
    Mix8MonoInterp_NoClick,
    AsmMonoInterp,
    AsmMonoInterp,
};

VMIXER ASM_M8_STEREO_INTERP =
{   NULL,

    "Assembly Mono-8 (Stereo/Interp) v0.1",

    nc8_Check_StereoInterp,
    SF_BITS_8,
    MD_MONO,

    MD_STEREO,

    NULL,
    NULL,
    VC_Volcalc8_Stereo,
    VC_Volramp8_Stereo,
    Mix8StereoInterp_NoClick,
    Mix8SurroundInterp_NoClick,
    AsmStereoInterp,
    AsmSurroundInterp
};
