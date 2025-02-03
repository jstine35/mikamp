/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: vc8ss.c  - Stereo Sample Mixer!

  Low-level mixer functions for mixing 8 bit STEREO sample data.  Includes
  normal and interpolated mixing, without declicking (no microramps, see
  vc8ssnc.c for those).

  Note: Stereo Sample Mixing does not support Dolby Surround.  Dolby Surround
  is lame anyway, so get over it!

*/

#include "mikamp.h"
#include "wrap8.h"


typedef SBYTE SCAST;

#include "ssmix.h"


VMIXER S8_MONO_INTERP =
{
    NULL,
    "Stereo-8 (Mono/Interp) v0.1",

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
    MixMonoSSI,
    MixMonoSSI,
};

VMIXER S8_STEREO_INTERP =
{
    NULL,
    "Stereo-8 (Stereo/Interp) v0.1",

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
    MixStereoSSI,
    MixStereoSSI,
};

VMIXER S8_MONO =
{
    NULL,

    "Stereo-8 (Mono) v0.1",
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
    MixMonoSS,
    MixMonoSS,
};

VMIXER S8_STEREO =
{
    NULL,
    "Stereo-8 (Stereo) v0.1",

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
    MixStereoSS,
    MixStereoSS,
};
