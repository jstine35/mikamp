/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: vc8.c

  Low-level mixer functions for mixing 8 bit sample data.  Includes normal and
  interpolated mixing, without declicking (no microramps, see vcMix_noclick.c
  for those).

*/

#include "mikamp.h"
#include "wrap8.h"

typedef SBYTE SCAST;

#include "stdmix.h"

VMIXER M8_MONO_INTERP =
{
    NULL,
    "Mono-8 (Mono/Interp) v0.1",

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
    MixMonoInterp,
    MixMonoInterp,
};

VMIXER M8_STEREO_INTERP =
{
    NULL,
    "Mono-8 (Stereo/Interp) v0.1",

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
    MixStereoInterp,
    MixSurroundInterp,
};

VMIXER M8_MONO =
{
    NULL,
    "Mono-8 (Mono) v0.1",

    nc8_Check_Mono,
    SF_BITS_8,
    MD_MONO,
    MD_MONO,

    NULL,
    NULL,

    VC_Volcalc8_Mono,
    VC_Volramp8_Mono,
    Mix8MonoNormal_NoClick,
    Mix8MonoNormal_NoClick,
    MixMonoNormal,
    MixMonoNormal,
};

VMIXER M8_STEREO =
{
    NULL,
    "Mono-8 (Stereo) v0.1",

    nc8_Check_Stereo,
    SF_BITS_8,
    MD_MONO,
    MD_STEREO,

    NULL,
    NULL,

    VC_Volcalc8_Stereo,
    VC_Volramp8_Stereo,
    Mix8StereoNormal_NoClick,
    Mix8SurroundNormal_NoClick,
    MixStereoNormal,
    MixSurroundNormal,
};
