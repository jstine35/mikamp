/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: vc16.c

 Description:
  Low-level mixer functions for mixing 16 bit sample data.  Includes normal and
  interpolated mixing, without declicking (no microramps, see vc16nc.c
  for those).

*/

#include "mikamp.h"
#include "wrap16.h"

typedef SWORD SCAST;

#include "stdmix.h"


VMIXER M16_MONO_INTERP =
{
    NULL,
    "Mono-16 (Mono/Interp) v0.1",

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
    MixMonoInterp,
    MixMonoInterp,
};

VMIXER M16_STEREO_INTERP =
{
    NULL,
    "Mono-16 (Stereo/Interp) v0.1",

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
    MixStereoInterp,
    MixSurroundInterp,
};

VMIXER M16_MONO =
{
    NULL,
    "Mono-16 (Mono) v0.1",

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
    MixMonoNormal,
    MixMonoNormal,
};

VMIXER M16_STEREO =
{
    NULL,
    "Mono-16 (Stereo) v0.1",

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
    MixStereoNormal,
    MixSurroundNormal,
};
