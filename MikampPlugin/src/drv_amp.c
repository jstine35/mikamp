/*
 Mikamp Plugin for Winamp

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 drv_amp.c
 
 Output driver for Winamp plugins.  Generates data using Mikamp Sound System's mixer, and
 feeds the result to the mikamp for Winamp plugin interface.
 
 Original driver code provided by Justin Frankel in 1998.
 MDriver API updates, code conformation, and optimizations by Jake Stine.
 
*/

#include <windows.h>

#include "mikamp.h"
#include "virtch.h"
#include "main.h"

#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 28

typedef struct AMP_LOCALINFO
{
    SBYTE   AMP_DMABUF[(BUFSIZE*1024*2) + 64];  // added 64 for mmx mixer (it's not exact :)
    int     block_len, bytes_per_sec;

} AMP_LOCALINFO;


// _____________________________________________________________________________________
//
static BOOL AMP_IsThere( void )
{
    return 1;
}

// _____________________________________________________________________________________
//
static BOOL AMP_Init( MDRIVER *md )
{
    AMP_LOCALINFO  *hwdata;
    
    hwdata        = _mmobj_allocblock( md, AMP_LOCALINFO );
    md->device.vc = VC_Init( &md->allochandle );

    if(!md->device.vc)
    {
        mikamp.outMod->Close();
        _mmobj_free( md, hwdata );
        return FALSE;
    }

    md->device.local = hwdata;

    // drv_amp only needs the standard module mixer set. It does not need stereo samples
    VC_RegisterModuleMixers(md->device.vc);

    return TRUE;
}

// _____________________________________________________________________________________
//
static void AMP_Exit( MDRIVER *md )
{
    VC_Exit( md->device.vc );
    mikamp.outMod->Close();
}

// _____________________________________________________________________________________
//
static void AMP_Update(MDRIVER *md)
{
    AMP_LOCALINFO  *hwdata = md->device.local;
    VIRTCH         *vc     = md->device.vc;
	int             l;

    if ((l=mikamp.outMod->CanWrite()) > hwdata->block_len*16) l = hwdata->block_len*16;
    if (mikamp.dsp_isactive()) l>>=1;

    if (l > hwdata->block_len)
    {
    	int o = 0;

        l -= l % hwdata->block_len;
        VC_WriteBytes(md, hwdata->AMP_DMABUF, l);

        while (o < l)
        {
            int a = min(hwdata->block_len, l - o);
            
            if( mikamp.dsp_isactive() )
            {
            	int t;
                int k = vc->bitdepth * vc->channels;

                t = mikamp.dsp_dosamples((short *)(hwdata->AMP_DMABUF+o), a / k, vc->bitdepth*8, vc->channels, vc->mixspeed) * k;
                mikamp.outMod->Write(hwdata->AMP_DMABUF+o,t);
			} else
                mikamp.outMod->Write(hwdata->AMP_DMABUF+o,a);

			mikamp.SAAddPCMData(hwdata->AMP_DMABUF+o,vc->channels,vc->bitdepth*8,decode_pos_ms/64);
			mikamp.VSAAddPCMData(hwdata->AMP_DMABUF+o,vc->channels,vc->bitdepth*8,decode_pos_ms/64);

			decode_pos_ms += (a*1000*64) / hwdata->bytes_per_sec;
			o+=a;
		}
    } else Sleep(1);
}

// _____________________________________________________________________________________
//
static BOOL AMP_PlayStart( MDRIVER *md )
{
    AMP_LOCALINFO  *hwdata = md->device.local;
    VIRTCH         *vc     = md->device.vc;

    int     z;
	int     a = 576 * vc->bitdepth * vc->channels;

    hwdata->block_len     = a;
	hwdata->bytes_per_sec = vc->mixspeed * vc->bitdepth * vc->channels;

    z = mikamp.outMod->Open( vc->mixspeed, vc->channels, vc->bitdepth * 8, -1, -1 );
    if( z < 0 ) return FALSE;

	mikamp.SAVSAInit( z, vc->mixspeed );
	mikamp.VSASetInfo( vc->mixspeed, vc->channels );

    mikamp.outMod->SetVolume(-666);
    VC_PlayStart( md );

    return TRUE;
}

static void AMP_PlayStop( MDRIVER *md )
{
    mikamp.outMod->Close();
    VC_PlayStop( md );
}

// _____________________________________________________________________________________
//
static BOOL AMP_SetSoftVoices(MDRIVER *md, uint voices)
{
    return VC_SetSoftVoices(md->device.vc, voices);
}


// =====================================================================================
//
MD_DEVICE drv_amp =
{
    "win32au",
    "Winamp output driver v0.8",
    0, VC_MAXVOICES, 

    NULL,
    NULL,
    NULL,
    
    // Sample Loading
    VC_SampleAlloc,
    VC_SampleGetPtr,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    VC_GetSampleCaps,
    VC_GetSampleFormat,

    // Detection and Initialization
    AMP_IsThere,
    AMP_Init,
    AMP_Exit,
    AMP_Update,
    AMP_PlayStart,
    AMP_PlayStop,

    NULL,
    AMP_SetSoftVoices,

    VC_SetOption,
    VC_GetOption,

    VC_SetVolume,
    VC_GetVolume,
    
    // Voice control and Voice information
    VC_GetActiveVoices,

    VC_VoiceSetVolume,
    VC_VoiceGetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceGetFrequency,
    VC_VoiceSetPosition,
    VC_VoiceGetPosition,
    VC_VoiceSetSurround,
    VC_VoiceSetResonance,

    VC_VoicePlay,
    VC_VoiceResume,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceReleaseSustain,
    VC_VoiceRealVolume,

};
