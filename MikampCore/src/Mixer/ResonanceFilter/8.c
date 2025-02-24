/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 vc8.c

  Low-level mixer functions for mixing 8 bit sample data.  Includes normal and
  interpolated mixing, without declicking (no microramps, see vcmix8_noclick.c
  for those).

*/

#include "mikamp.h"
#include "../wrap8.h"

#include "resshare.h"

// _____________________________________________________________________________________
//
static SLONG _mm_inline filter(long sroot, VC_RESFILTER *r8)
{

    r8->speed += (((sroot<<10) - r8->pos) * r8->cofactor) >> VC_COFACTOR;
    r8->pos   += r8->speed;

    r8->speed *= r8->resfactor;
    r8->speed >>= VC_RESFACTOR;

    return r8->pos>>10;   //>>VC_COFACTOR;

    //return sroot;

}

// _____________________________________________________________________________________
//
static void VC_ResVolcalc8_Mono(VIRTCH *vc, VINFO *vnf)
{
    vnf->vol.front.left  = ((vnf->volume.front.left+vnf->volume.front.right) * (vc->volume.front.left+vc->volume.front.right) * BIT8_VOLFAC) / 3;

    lvolsel = vnf->vol.front.left;

    // declicker: Set us up to volramp!
    if(vc->mode & DMODE_NOCLICK)
    {
        if(vnf->vol.front.left != vnf->oldvol.front.left)
        {
            if(!vnf->volramp) vnf->volramp = RAMPLEN_VOLUME;
            v8.left.inc = ((vnf->vol.front.left - (v8.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
        } else if(vnf->volramp)
        {
            v8.left.inc  = ((vnf->vol.front.left - (v8.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            if(!v8.left.inc) vnf->volramp = 0;
        }
    }

    VC_CalcResonance(vc, vnf);
    r8 = &vnf->resfilter;
}

// _____________________________________________________________________________________
//
static void VC_ResVolcalc8_Stereo(VIRTCH *vc, VINFO *vnf)
{
    vnf->vol.front.left  = vnf->volume.front.left  * vc->volume.front.left  * BIT8_VOLFAC;
    vnf->vol.front.right = vnf->volume.front.right * vc->volume.front.right * BIT8_VOLFAC;

    lvolsel = vnf->vol.front.left;
    rvolsel = vnf->vol.front.right;

    // declicker: Set us up to volramp!
    if(vc->mode & DMODE_NOCLICK)
    {   
        if((vnf->vol.front.left != vnf->oldvol.front.left) || (vnf->vol.front.right != vnf->oldvol.front.right))
        {
            if(!vnf->volramp) vnf->volramp = RAMPLEN_VOLUME;
            v8.left.inc  = ((vnf->vol.front.left - (v8.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            v8.right.inc = ((vnf->vol.front.right - (v8.right.vol = vnf->oldvol.front.right)) / (int)vnf->volramp);
        } else if(vnf->volramp)
        {
            v8.left.inc  = ((vnf->vol.front.left - (v8.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            v8.right.inc = ((vnf->vol.front.right - (v8.right.vol = vnf->oldvol.front.right)) / (int)vnf->volramp);
            
            if(!v8.left.inc && !v8.right.inc) vnf->volramp = 0;
        }
    }

    VC_CalcResonance(vc, vnf);
    r8 = &vnf->resfilter;
}

// _____________________________________________________________________________________
//
void __cdecl Res8StereoNormal(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    SBYTE  sample;

    for(; todo; todo--)
    {
        sample = filter(srce[himacro(index)], r8);
        index += increment;

        *dest++ += lvolsel * sample;
        *dest++ += rvolsel * sample;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8StereoInterp(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {
        SLONG  sroot  = srce[himacro(index)];
        sroot = filter((SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r8);

        *dest++ += lvolsel * sroot;
        *dest++ += rvolsel * sroot;

        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8SurroundNormal(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    SLONG  sample;

    for (; todo; todo--)
    {
        sample = lvolsel * filter(srce[himacro(index)], r8);
        index += increment;

        *dest++ += sample;
        *dest++ -= sample;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8SurroundInterp(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for (; todo; todo--)
    {
        SLONG  sroot = srce[himacro(index)];
        sroot = lvolsel * filter((SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r8);

        *dest++ += sroot;
        *dest++ -= sroot;
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8MonoNormal(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {
        *dest++ += lvolsel * filter(srce[himacro(index)], r8);
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8MonoInterp(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {
        SLONG  sroot = srce[himacro(index)];
        sroot = lvolsel * filter((SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r8);
        *dest++ += sroot;
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8StereoNormal_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {   
        v8.left.vol    += v8.left.inc;
        v8.right.vol   += v8.right.inc;
        {
        SLONG sample = filter(srce[himacro(index)], r8);
        *dest++ += v8.left.vol   * sample;
        *dest++ += v8.right.vol  * sample;
        }
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8StereoInterp_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {
        SLONG  sroot  = srce[himacro(index)];
        sroot = filter((SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r8);

        v8.left.vol    += v8.left.inc;
        v8.right.vol   += v8.right.inc;

        *dest++  += v8.left.vol  * sroot;
        *dest++  += v8.right.vol * sroot;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8SurroundNormal_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {
        SLONG sample;

        v8.left.vol += v8.left.inc;
        sample       = v8.left.vol * filter(srce[himacro(index)], r8);

        *dest++ += sample;
        *dest++ -= sample;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8SurroundInterp_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {
        SLONG  sroot = srce[himacro(index)];
        v8.left.vol += v8.left.inc;
        sroot        = v8.left.vol  * filter((SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r8);

        *dest++ += sroot;
        *dest++ -= sroot;
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8MonoNormal_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    SLONG  sample;

    for(; todo; todo--)
    {
        v8.left.vol  += v8.left.inc;
        sample        = v8.left.vol  * filter(srce[himacro(index)], r8);

        *dest++ += sample;
        index  += increment;
    }
}

// _____________________________________________________________________________________
//
void __cdecl Res8MonoInterp_NoClick(SBYTE *srce, SLONG *dest, INT64S index, INT64S increment, SLONG todo)
{
    for(; todo; todo--)
    {
        SLONG  sroot = srce[himacro(index)];
        v8.left.vol += v8.left.inc;
        sroot        = v8.left.vol * filter((SBYTE)(sroot + ((((SLONG)srce[himacro(index) + 1] - sroot) * (lomacro(index)>>INTERPBITS)) >> INTERPBITS)), r8);

        *dest++ += sroot;
        index   += increment;
    }
}

// _____________________________________________________________________________________
//
static BOOL Check_Mono(uint mixmode, uint format, uint flags)
{
    if(flags & SL_RESONANCE_FILTER) return 1;
    return 0;
}

// _____________________________________________________________________________________
//
static BOOL Check_MonoInterp(uint mixmode, uint format, uint flags)
{
    if((flags & SL_RESONANCE_FILTER) && (mixmode & DMODE_INTERP)) return 1;
    return 0;
}

// _____________________________________________________________________________________
//
static BOOL Check_Stereo(uint mixmode, uint format, uint flags)
{
    if(flags & SL_RESONANCE_FILTER) return 1;
    return 0;
}

// _____________________________________________________________________________________
//
static BOOL Check_StereoInterp(uint mixmode, uint format, uint flags)
{
    if((flags & SL_RESONANCE_FILTER) && (mixmode & DMODE_INTERP)) return 1;
    return 0;
}



VMIXER RF_M8_MONO_INTERP =
{
    NULL,
    "Resonance Mono-8 (Mono/Interp) v0.1",

    Check_MonoInterp,
    SF_BITS_8,
    MD_MONO,
    MD_MONO,

    NULL,
    NULL,

    VC_ResVolcalc8_Mono,
    VC_Volramp8_Mono,
    Res8MonoInterp_NoClick,
    Res8MonoInterp_NoClick,
    Res8MonoInterp,
    Res8MonoInterp,
};

VMIXER RF_M8_STEREO_INTERP =
{
    NULL,
    "Resonance Mono-8 (Stereo/Interp) v0.1",

    Check_StereoInterp,
    SF_BITS_8,
    MD_MONO,
    MD_STEREO,

    NULL,
    NULL,

    VC_ResVolcalc8_Stereo,
    VC_Volramp8_Stereo,
    Res8StereoInterp_NoClick,
    Res8SurroundInterp_NoClick,
    Res8StereoInterp,
    Res8SurroundInterp,
};

VMIXER RF_M8_MONO =
{
    NULL,
    "Resonance Mono-8 (Mono) v0.1",

    Check_Mono,
    SF_BITS_8,
    MD_MONO,
    MD_MONO,

    NULL,
    NULL,

    VC_ResVolcalc8_Mono,
    VC_Volramp8_Mono,
    Res8MonoNormal_NoClick,
    Res8MonoNormal_NoClick,
    Res8MonoNormal,
    Res8MonoNormal,
};

VMIXER RF_M8_STEREO =
{
    NULL,
    "Resonance Mono-8 (Stereo) v0.1",

    Check_Stereo,
    SF_BITS_8,
    MD_MONO,
    MD_STEREO,

    NULL,
    NULL,

    VC_ResVolcalc8_Stereo,
    VC_Volramp8_Stereo,
    Res8StereoNormal_NoClick,
    Res8SurroundNormal_NoClick,
    Res8StereoNormal,
    Res8SurroundNormal,
};
