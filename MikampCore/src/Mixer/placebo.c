/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 placebo.c

 This is the placebo mixer for when mikamp cannot seem to find a valid mixer
 to match a sample's format and flags and stuff.  You do not have to register
 this mixer.  Mikamp will use it automatically.

}
*/

#include "vchcrap.h"

static void vc_placebo_volcalc(VIRTCH *vc, VINFO *vnf) {};
static void vc_placebo_volramp(VINFO *vnf, int done) {};
static void __cdecl vc_placebo_mix(void *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo) {};

VMIXER VC_MIXER_PLACEBO =
{
    NULL,
    "Placebo Mixer v1.0",

    NULL,
    SF_BITS_UNKNOWN,
    0,
    0,

    NULL,
    NULL,
    
    vc_placebo_volcalc,
    vc_placebo_volramp,

    vc_placebo_mix,
    vc_placebo_mix,
    vc_placebo_mix,
    vc_placebo_mix,
};


