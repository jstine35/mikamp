
#include "vchcrap.h"

// _____________________________________________________________________________________
// Registers all the stereo sample mixers.  Only call this procedure if you need them.
//
void VC_RegisterStereoMixers(VIRTCH *vc)
{
#ifdef CPU_INTEL
    VC_RegisterMixer(vc, &ASM_S8_MONO_INTERP);
    VC_RegisterMixer(vc, &ASM_S16_MONO_INTERP);
    VC_RegisterMixer(vc, &ASM_S8_STEREO_INTERP);
    VC_RegisterMixer(vc, &ASM_S16_STEREO_INTERP);
#else
    VC_RegisterMixer(vc, &S8_MONO_INTERP);
    VC_RegisterMixer(vc, &S16_MONO_INTERP);
    VC_RegisterMixer(vc, &S8_STEREO_INTERP);
    VC_RegisterMixer(vc, &S16_STEREO_INTERP);
#endif
}
