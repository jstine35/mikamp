/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: regmix_mods.c

 regmix_*.c: These are separate modules to aid linkers in dead-code elimination.

 Modern Era Remarks (2025): This separation of registration functions into individual
 files is no longer needed. Linkers do a great job of eliminating unused function
 dependencies now, and sometimes do too good a job to the extent that they tend to
 eliminate code in surprising ways. However, it's easy enough to keep these separations
 and it may have some benefits with MikAmp being mostly C code and possibly being used
 to target certain older hardware.
 
*/

#include "vchcrap.h"

// Registers the default mixers used by module music playback. Generally required for
// standard operation of MikAmp. Includes Resonant Filter mixers required by Impulse
// Tracker. Could be omitted in special circumstances where an app is providing all its
// own custom mixers, or if the app desires to omit the Resonant Filter mixers.
void VC_RegisterModuleMixers(VIRTCH* vc) {
    if (!vc) return;

    // Resonant Filter Mixers. Only required for Impulsetracker playback.
    VC_RegisterMixer(vc, &RF_M8_MONO_INTERP);
    VC_RegisterMixer(vc, &RF_M16_MONO_INTERP);
    VC_RegisterMixer(vc, &RF_M8_STEREO_INTERP);
    VC_RegisterMixer(vc, &RF_M16_STEREO_INTERP);

    VC_RegisterMixer(vc, &M8_MONO_INTERP);
    VC_RegisterMixer(vc, &M16_MONO_INTERP);
    VC_RegisterMixer(vc, &M8_STEREO_INTERP);
    VC_RegisterMixer(vc, &M16_STEREO_INTERP);
}
