/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 sn8.c

  Assembly Mixer Plugin : Stereo 8 bit Sample data
  
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

VMIXER ASM_S8_MONO =
{
    NULL,
    "Assembly Stereo-8 (Mono) v0.1",

    nc8ss_Check_Mono,
    SF_BITS_8,
    MD_STEREO,

    MD_MONO,

    NULL,
    NULL,
    VC_Volcalc8_Mono,
    VC_Volramp8_Mono,
    Mix8MonoSS_NoClick,
    Mix8MonoSS_NoClick,
    AsmMonoSS,
    AsmMonoSS,
};

    
VMIXER ASM_S8_STEREO =
{
    NULL,
    "Assembly Stereo-8 (Stereo) v0.1",

    nc8ss_Check_Stereo,
    SF_BITS_8,
    MD_STEREO,

    MD_STEREO,

    NULL,
    NULL,
    VC_Volcalc8_Stereo,
    VC_Volramp8_Stereo,
    Mix8StereoSS_NoClick,
    Mix8StereoSS_NoClick,
    AsmStereoSS,
    AsmStereoSS,
};
