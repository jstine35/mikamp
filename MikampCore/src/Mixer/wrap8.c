/*

 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 -----------------------------------------
 Module: wrap8.c

  Basic common wrapper functions for all Mikamp-standard 8-bit mixers (both
  C and assembly versions).

  - CalculateVolumes
  - RampVolume
  
*/

#include "mikamp.h"
#include "wrap8.h"

VOLINFO8       v8;
VC_RESFILTER  *r8;

static int  alloc;

// =====================================================================================
    void VC_Volcalc8_Mono(VIRTCH *vc, VINFO *vnf)
// =====================================================================================
{
    vnf->vol.front.left  = ((vnf->volume.front.left+vnf->volume.front.right) * (vc->volume.front.left+vc->volume.front.right) * BIT8_VOLFAC) / 3;

    lvolsel = vnf->vol.front.left;

    // declicker: Set us up to volramp!
    if(vc->mode & DMODE_NOCLICK)
    {   if(vnf->vol.front.left != vnf->oldvol.front.left)
        {   if(!vnf->volramp) vnf->volramp = RAMPLEN_VOLUME;
            v8.left.inc = ((vnf->vol.front.left - (v8.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
        } else if(vnf->volramp)
        {   v8.left.inc  = ((vnf->vol.front.left - (v8.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            if(!v8.left.inc) vnf->volramp = 0;
        }
    }
}


// =====================================================================================
    void VC_Volcalc8_Stereo(VIRTCH *vc, VINFO *vnf)
// =====================================================================================
{
    vnf->vol.front.left  = vnf->volume.front.left  * vc->volume.front.left  * BIT8_VOLFAC;
    vnf->vol.front.right = vnf->volume.front.right * vc->volume.front.right * BIT8_VOLFAC;

    lvolsel = vnf->vol.front.left;
    rvolsel = vnf->vol.front.right;

    // declicker: Set us up to volramp!
    if(vc->mode & DMODE_NOCLICK)
    {   
        if((vnf->vol.front.left != vnf->oldvol.front.left) || (vnf->vol.front.right != vnf->oldvol.front.right))
        {   if(!vnf->volramp) vnf->volramp = RAMPLEN_VOLUME;
            v8.left.inc  = ((vnf->vol.front.left - (v8.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            v8.right.inc = ((vnf->vol.front.right - (v8.right.vol = vnf->oldvol.front.right)) / (int)vnf->volramp);
            //_mmlog("left: %5d %5d  right: %5d %5d  volincs: %7d, %7d",vnf->vol.front.left, vnf->oldvol.front.left, vnf->vol.front.right, vnf->oldvol.front.right, v8.left.inc, v8.right.inc);
        } else if(vnf->volramp)
        {   v8.left.inc  = ((vnf->vol.front.left - (v8.left.vol = vnf->oldvol.front.left)) / (int)vnf->volramp);
            v8.right.inc = ((vnf->vol.front.right - (v8.right.vol = vnf->oldvol.front.right)) / (int)vnf->volramp);
            
            if(!v8.left.inc && !v8.right.inc) vnf->volramp = 0;
        }
    }
}


// =====================================================================================
    void VC_Volramp8_Mono(VINFO *vnf, int done)
// =====================================================================================
{
    vnf->oldvol.front.left += (v8.left.inc * done);
}


// =====================================================================================
    void VC_Volramp8_Stereo(VINFO *vnf, int done)
// =====================================================================================
{
    vnf->oldvol.front.left  += (v8.left.inc * done);
    vnf->oldvol.front.right += (v8.right.inc * done);
}
