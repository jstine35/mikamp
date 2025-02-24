
/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: regmix_ss.c

 regmix_*: These are separate modules to aid linkers in dead-code elimination.

 Modern Era Remarks (2025): This separation of registration functions into individual
 files is no longer needed. Linkers do a great job of eliminating unused function
 dependencies now, and sometimes do too good a job to the extent that they tend to
 eliminate code in surprising ways. However, it's easy enough to keep these separations
 and it may have some benefits with MikAmp being mostly C code and possibly being used
 to target certain older hardware.
  
*/

#include "vchcrap.h"

// Registers the default stereo sample mixers. These are optionally separated as stereo sample
// mixers are not used by any module music format. Stereo sample mixers are only used by for
// custom game engine sound effects systems.
void VC_RegisterStereoMixers(VIRTCH *vc) {
    VC_RegisterMixer(vc, &S8_MONO_INTERP);
    VC_RegisterMixer(vc, &S16_MONO_INTERP);
    VC_RegisterMixer(vc, &S8_STEREO_INTERP);
    VC_RegisterMixer(vc, &S16_STEREO_INTERP);
}
