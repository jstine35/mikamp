#ifndef _PSFORBID_H_
#define _PSFORBID_H_

#include "mplayer.h"

// -------------------------------------------------------------------------------------
//
static void _mm_inline MP_WipePosPlayed(MPLAYER *ps)
{
    if(ps->state.pos_played)
    {   uint   i;
        for(i=0; i<ps->mf->numpos; i++)
            memset(ps->state.pos_played[i],0,sizeof(ULONG) * ((ps->mf->pattrows[ps->mf->positions[i]]/32)+1));
    }
}

static BOOL _mm_inline MP_PosPlayed(MPLAYER *ps)
{
    if(!ps->state.pos_played) return 0;
    return (ps->state.pos_played[ps->state.sngpos][(ps->state.patpos/32)] & (1<<(ps->state.patpos&31)));
}


static void _mm_inline MP_SetPosPlayed(MPLAYER *ps)
{
    if(ps->state.pos_played) ps->state.pos_played[ps->state.sngpos][(ps->state.patpos/32)] |= (1<<(ps->state.patpos&31));
}

static void _mm_inline MP_UnsetPosPlayed(MPLAYER *ps)
{
    if(ps->state.pos_played) ps->state.pos_played[ps->state.sngpos][(ps->state.patpos/32)] &= ~(1<<(ps->state.patpos&31));
}

// -------------------------------------------------------------------------------------
//
static void _mm_inline MP_LoopSong(MPLAYER *ps, const UNIMOD *pf)
{
    if((pf->reppos >= (int)pf->numpos) || (pf->reppos == 0))
    {
        ps->state.sngpos        = 0;
        ps->state.patpos        = 0;
        ps->state.volume        = pf->initvolume;
        ps->state.sngspd        = pf->initspeed;
        ps->state.bpm           = pf->inittempo;
        ps->state.strip_timeout = pf->strip_threshold;

        // reset channel volumes to module default
        memcpy(ps->state.chanvol, pf->chanvol, 64*sizeof(pf->chanvol[0]));

        //Player_Cleaner(ps);
    } else ps->state.sngpos = pf->reppos;
}


// TODO: Remove ps_forbid entirely. Rationale: Legacy multithreaded api, a throwback to the earliest
// editions of MikMod which internally tried to run UniMod decoding and mixing on a thread. This
// simplified locking model was not robust to the needs of asyncronous sound systems. If redone, it
// should use instead a message queue that posts changes to the player state rather than modifying the
// player state variables directly.

#define ps_forbid_init()    ((void)0)
#define ps_forbid_deinit()  ((void)0)
#define ps_forbid()         ((void)0)
#define ps_unforbid()       ((void)0)


#endif
