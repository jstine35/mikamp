/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 mi8.c

  Assembly Mixer Plugin : Stereo 16 bit Sample data / Interpolation

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

VMIXER ASM_S16_MONO_INTERP =
{
    NULL,
    "Assembly Stereo-16 (Mono/Interp) v0.1",

    nc16ss_Check_MonoInterp,
    SF_BITS_16,
    MD_STEREO,

    MD_MONO,

    NULL,
    NULL,
    VC_Volcalc16_Mono,
    VC_Volramp16_Mono,
    Mix16MonoSSI_NoClick,
    Mix16MonoSSI_NoClick,
    Asm16MonoSSI,
    Asm16MonoSSI,
};

    
VMIXER ASM_S16_STEREO_INTERP =
{
    NULL,
    "Assembly Stereo-16 (Stereo/Interp) v0.1",

    nc16ss_Check_StereoInterp,
    SF_BITS_16,
    MD_STEREO,

    MD_STEREO,

    NULL,
    NULL,
    VC_Volcalc16_Stereo,
    VC_Volramp16_Stereo,
    Mix16StereoSSI_NoClick,
    Mix16StereoSSI_NoClick,
    Asm16StereoSSI,
    Asm16StereoSSI,
};
