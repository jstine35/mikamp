/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: vc16ss.c - 16bit STEREO sample mixers!

 Description:
  Stereo, stereo.  It comes from all around.  Stereo, stereo, the multi-
  pronged two-headed attack of sound!  Stereo, Stereo, it owns you-hu!
  Stereo, Stereo, It rings soooooo... oh sooooooo... truuuuuuee!

*/

#include "mikamp.h"
#include "wrap16.h"

typedef SWORD SCAST;

#include "ssmix.h"


VMIXER S16_MONO_INTERP =
{
    NULL,
    "Stereo-16 (Mono/Interp) v0.1",

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
    MixMonoSSI,
    MixMonoSSI,
};

    
VMIXER S16_STEREO_INTERP =
{
    NULL,
    "Stereo-16 (Stereo/Interp) v0.1",

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
    MixStereoSSI,
    MixStereoSSI,
};

VMIXER S16_MONO =
{
    NULL,
    "Stereo-16 (Mono) v0.1",

    nc16ss_Check_Mono,
    SF_BITS_16,
    MD_STEREO,
    MD_MONO,

    NULL,
    NULL,

    VC_Volcalc16_Mono,
    VC_Volramp16_Mono,
    Mix16MonoSS_NoClick,
    Mix16MonoSS_NoClick,
    MixMonoSS,
    MixMonoSS,
};

VMIXER S16_STEREO =
{
    NULL,
    "Stereo-16 (Stereo) v0.1",

    nc16ss_Check_Stereo,
    SF_BITS_16,
    MD_STEREO,
    MD_STEREO,

    NULL,
    NULL,

    VC_Volcalc16_Stereo,
    VC_Volramp16_Stereo,
    Mix16StereoSS_NoClick,
    Mix16StereoSS_NoClick,
    MixStereoSS,
    MixStereoSS,
};

