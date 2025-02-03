/*
 Mikamp Plugin for Winamp

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 main.h
 
*/

#ifndef __MIKAMP_MAIN_H__
#define __MIKAMP_MAIN_H__

#include <windows.h>
#include <stdio.h>
#include "mikamp.h"
#include "mplayer.h"
#include "resource.h"
#include "in2.h"

// This is to help VC generate a smaller .DLL!

#define INFO_CPAGES 3 

#define CPLAYFLG_LOOPALL      (1ul<<0)  // disables selective looping - loop everything!
#define CPLAYFLG_PLAYALL      (1ul<<1)  // plays hidden patterns (tack onto end of the song)
#define CPLAYFLG_FADEOUT      (1ul<<2)  // Fadeout the song before the end cometh?
#define CPLAYFLG_STRIPSILENCE (1ul<<3)  // Strip silence at the end of the song?

typedef struct tag_dlghdr
{   
    HWND     hwndTab;       // tab control 
    HWND     hwndDisplay;   // current child dialog box 
	int      left,top;
    HWND     apRes[INFO_CPAGES]; 

	UNIMOD  *module;
    MPLAYER *seeker;
    int      maxv;

    BOOL    *suse;

} DLGHDR;


typedef struct INFOBOX
{   HWND    hwnd;
    UNIMOD  *module;
    struct INFOBOX *next;
} INFOBOX;



#define WM_WA_MPEG_EOF WM_USER+2

#ifdef __cplusplus
extern "C" {
#endif

extern UBYTE      config_nopan, config_savestr;
extern MD_DEVICE  drv_amp;
extern In_Module  mikamp;
extern UNIMOD    *mf;
extern MPLAYER   *mp;
extern int        decode_pos_ms;


// Defined in INFO.C
// -----------------
extern INFOBOX   *infobox_list;
extern HWND       infoDlg(HWND hwnd, DLGHDR *pHdr);


// Defined in INFO.C
// -----------------
// defined in config.c

extern UBYTE     config_interp,   config_surround;
extern UBYTE     config_8bit,     config_nch, config_panrev;
extern UBYTE     config_priority, config_cpu;
extern uint      config_srate,    config_voices, config_loopcount, config_playflag;
extern int       config_pansep;

extern UBYTE     config_resonance;
extern int       config_fadeout;

extern int       file_length, paused;


// config.c shizat
// ---------------
extern void  set_priority(void);

extern void __cdecl config(HWND hwndParent);
extern void __cdecl about(HWND hwndParent);

extern void config_read();
extern void config_write();

extern void info_killseeker(HWND hwnd, UNIMOD *curmf);

#ifdef __cplusplus
};
#endif

#endif
