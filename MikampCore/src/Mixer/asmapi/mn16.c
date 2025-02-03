/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: mi8.c

  Assembly Mixer Plugin : Mono 16 bit Sample data

  Generic all-purpose assembly wrapper for mikamp VMIXER plugin thingie.
  As long as your platform-dependant asm code uses the naming conventions
  below (Asm StereoInterp, SurroundInterp, etc), then you can use this
  module and the others related to it to register your mixer into mikamp.

  See Also:

    mi8.c
    mn8.c
    sn8.c
    si8.c
    sn16.c
    mi16.c
    si16.c
*/

#include "mikamp.h"
#include "..\wrap16.h"
#include "asmapi.h"


VMIXER ASM_M16_MONO =
{
    NULL,
    "Assembly Mono-16 (Mono) v0.1",

    nc16_Check_Mono,
    SF_BITS_16,
    MD_MONO,

    MD_MONO,

    NULL,
    NULL,
    VC_Volcalc16_Mono,
    VC_Volramp16_Mono,
    Mix16MonoNormal_NoClick,
    Mix16MonoNormal_NoClick,
    Asm16MonoNormal,
    Asm16MonoNormal,
};

VMIXER ASM_M16_STEREO =
{
    NULL,
    "Assembly Mono-16 (Stereo) v0.1",

    nc16_Check_Stereo,
    SF_BITS_16,
    MD_MONO,

    MD_STEREO,

    NULL,
    NULL,
    VC_Volcalc16_Stereo,
    VC_Volramp16_Stereo,
    Mix16StereoNormal_NoClick,
    Mix16SurroundNormal_NoClick,
    Asm16StereoNormal,
    Asm16SurroundNormal
};
