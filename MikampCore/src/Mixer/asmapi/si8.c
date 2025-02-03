/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: mi8.c

  Assembly Mixer Plugin : Stereo 8 bit Sample data / Interpolation
  
  Generic all-purpose assembly wrapper for mikamp VMIXER plugin thingie.
  As long as your platform-dependant asm code uses the naming conventions
  below (Asm StereoInterp, SurroundInterp, etc), then you can use this
  module and the others related to it to register your mixer into mikamp.

  See Also:

    mi8.c
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

VMIXER ASM_S8_MONO_INTERP =
{
    NULL,
    "Assembly Stereo-8 (Mono/Interp) v0.1",

    nc8ss_Check_MonoInterp,
    SF_BITS_8,
    MD_STEREO,

    MD_MONO,

    NULL,
    NULL,
    VC_Volcalc8_Mono,
    VC_Volramp8_Mono,
    Mix8MonoSSI_NoClick,
    Mix8MonoSSI_NoClick,
    AsmMonoSSI,
    AsmMonoSSI,
};


VMIXER ASM_S8_STEREO_INTERP =
{
    NULL,
    "Assembly Stereo-8 (Stereo/Interp) v0.1",

    nc8ss_Check_StereoInterp,
    SF_BITS_8,
    MD_STEREO,

    MD_STEREO,

    NULL,
    NULL,
    VC_Volcalc8_Stereo,
    VC_Volramp8_Stereo,
    Mix8StereoSSI_NoClick,
    Mix8StereoSSI_NoClick,
    AsmStereoSSI,
    AsmStereoSSI,
};
