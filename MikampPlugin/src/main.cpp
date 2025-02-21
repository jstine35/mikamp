/*
 Mikamp Plugin for Winamp

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 main.cpp
 
 The Winamp Input Plugin API Implementation is found here, along with basic management of
 loading and playing back module music files.

*/

#define SILENCE_THRESHOLD  10800

#include "main.h"
#include "rf.h"
#include "virtch.h"

// Static Globals!
// ---------------

static char       lastfn[1024];
static char       curfn[1024];
static int        is_tempfile = 0;
static char       t_file[MAX_PATH];
static int        mpeg_threadid=0,killDecodeThread=0;
static HANDLE     mpeg_thread_handle=INVALID_HANDLE_VALUE;

static int (__cdecl *readerSource)(HINSTANCE hIns, reader_source **s);

// Public Globals!
// ---------------

UNIMOD           *mf;
MPLAYER          *mp;
int               paused;
int               decode_pos_ms;

DWORD WINAPI __stdcall decodeThread(volatile void *b);


#include "log.h"

void b(char *s) { MessageBox(NULL,s,s,0); }

void mmerr( MM_ERRINFO *info )
{
    CHAR  whee[2048];

    sprintf( whee,"File: %s\n,Error: %s",curfn, info->desc );
    if( info->num != MMERR_OPENING_FILE )
	    MessageBox( NULL, whee, "Module Error",0 );
}

WReader       *wreader;

void infobox_setmodule(HWND hwnd);

// _____________________________________________________________________________________
//
void __cdecl init(void)
{
    _mmerr_sethandler( &mmerr );

#ifdef MM_LOG_VERBOSE
    log_init("c:\\temp\\in_mod", LOG_SILENT);
#endif
    config_read();

    ML_RegisterLoader(&load_it);
    ML_RegisterLoader(&load_xm);
    ML_RegisterLoader(&load_s3m);
    ML_RegisterLoader(&load_mod);
    ML_RegisterLoader(&load_mtm);
    ML_RegisterLoader(&load_stm);
    //ML_RegisterLoader(&load_dsm);
    //ML_RegisterLoader(&load_med);
    //ML_RegisterLoader(&load_far);   
    ML_RegisterLoader(&load_ult);
    ML_RegisterLoader(&load_669);
    ML_RegisterLoader(&load_m15);

    MD_RegisterDriver(&drv_amp);

}

void __cdecl quit()
{  
#ifdef MM_LOG_VERBOSE
    log_exit();
#endif
}

static MDRIVER  *md;

// _____________________________________________________________________________________
//
UNIMOD *get_unimod(char *file)
{
	// if the requested file is the same one we have loaded,
	// then just reference the in-memory *mf!
	
	if(lstrcmpi(file, lastfn))
		return NULL;
	else
	    return mf;
}

// _____________________________________________________________________________________
//
int __cdecl play(char *fn)
{
    uint  md_mode = 0;

    strcpy(lastfn,fn);
    strcpy(curfn,fn);
	if (strstr(fn,"://"))
	{

#define IPC_GETHTTPGETTER 240

		int (__cdecl *httpRetrieveFile)(HWND hwnd, char *url, char *file, char *dlgtitle);
		char p[MAX_PATH];
		int t = SendMessage(mikamp.hMainWindow,WM_USER,0,IPC_GETHTTPGETTER);
		
		if (!t || t==1)
		{
			t_file[0]=0;
	        MessageBox(mikamp.hMainWindow,"URLs only supported in Winamp 2.10+","Error (in_mod.dll)",0);
	        return -1;	
		}
		
		httpRetrieveFile = (int (__cdecl *)(struct HWND__ *,char *,char *,char *))t;
		t_file[0]=0;
		if (config_savestr)
		{
			char *p;
			OPENFILENAME l;

			p = fn+strlen(fn);
			while (*p != '/' && p >= fn) p--;
			strcpy(t_file,++p);

			l.lStructSize       = sizeof(l);
			l.hwndOwner         = mikamp.hMainWindow;
			l.hInstance         = NULL;
			l.lpstrFilter       = "All files\0*.*\0";
			l.lpstrCustomFilter = NULL;
			l.nMaxCustFilter    = 0;
			l.nFilterIndex      = 0;
			l.lpstrFile         = t_file;
			l.nMaxFile          = MAX_PATH-1;
			l.lpstrFileTitle    = 0;;
			l.nMaxFileTitle     = 0;
			l.lpstrInitialDir   = NULL;
			l.lpstrTitle        = "Save MOD";
			l.lpstrDefExt       = "mod";
			l.Flags             = OFN_HIDEREADONLY|OFN_EXPLORER|OFN_OVERWRITEPROMPT; 	        

			if (!GetSaveFileName(&l)) 
			{
				t_file[0]=0;
			}
		}
		
		if (!t_file[0])
		{
			GetTempPath(sizeof(p),p);
			GetTempFileName(p,"mod",0,t_file);
			is_tempfile=1;		
		}
		
		if (httpRetrieveFile(mikamp.hMainWindow,fn,t_file,"Retrieving MOD-type file"))
		{
			is_tempfile=0;
			t_file[0]=0;
			return -1;
		}
		fn = t_file;
	} else 
	{	t_file[0] = 0;
		is_tempfile = 0;
	}


    // Initialize MDRVER
    // -----------------

    md = Mikamp_InitializeEx();

	if( config_interp & 1 ) md_mode |= DMODE_INTERP;
	if( config_interp & 2 ) md_mode |= DMODE_NOCLICK;
    if( config_nch==3 )     md_mode |= DMODE_SURROUND;
    if( config_resonance )  md_mode |= DMODE_RESONANCE;

    Mikamp_SetDeviceOption( md, MD_OPT_MIXSPEED,  config_srate );
    Mikamp_SetDeviceOption( md, MD_OPT_FLAGS,     md_mode );
    Mikamp_SetDeviceOption( md, MD_OPT_BITDEPTH,  config_8bit ? SF_BITS_8 : SF_BITS_16 ); 
    Mikamp_SetDeviceOption( md, MD_OPT_CHANNELS,  (config_nch == 1) ? MD_MONO : MD_STEREO ); 
    Mikamp_PlayStart( md );
    md->pansep = config_pansep;
    if( config_panrev ) md->pansep = -md->pansep;

    // Register non-interpolation mixers
    // ---------------------------------
    // if the user has disabled interpolation...

    if(!(config_interp & 1))
    {
        VC_RegisterMixer(md->device.vc, &RF_M8_MONO);
        VC_RegisterMixer(md->device.vc, &RF_M16_MONO);
        VC_RegisterMixer(md->device.vc, &RF_M8_STEREO);
        VC_RegisterMixer(md->device.vc, &RF_M16_STEREO);

		VC_RegisterMixer(md->device.vc, &M8_MONO);
		VC_RegisterMixer(md->device.vc, &M16_MONO);
		VC_RegisterMixer(md->device.vc, &M8_STEREO);
		VC_RegisterMixer(md->device.vc, &M16_STEREO);

        //VC_RegisterMixer(md->device.vc, &ASM_M8_MONO);
        //VC_RegisterMixer(md->device.vc, &ASM_M16_MONO);
        //VC_RegisterMixer(md->device.vc, &ASM_M8_STEREO);
        //VC_RegisterMixer(md->device.vc, &ASM_M16_STEREO);
    }

    if(!md) return -1;

    // LOADING THE SONG
    // ----------------
    // Check through the list of active info boxes for a matching filename.  If found,
    // then we use the already-loaded module information instead!

    {
        INFOBOX  *cruise;

        cruise = infobox_list;
        while(cruise)
        {
            if(!lstrcmpi(cruise->module->filename, fn))
            {
                MMSTREAM  *smpfp;
                mf = cruise->module;

                smpfp = _mm_fopen( fn, "rb" );
                Unimod_LoadSamples( mf, md, smpfp );
                _mm_fclose( smpfp );
            }
            cruise = cruise->next;
        }
    }


    if(!mf)
    {
        if((mf=Unimod_Load(md,fn)) == NULL)
        {
            Mikamp_Exit(md);
            return -1;
        }
        mf->filename = _mm_strdup(mf->allochandle, fn);
    }

    if(config_playflag & CPLAYFLG_STRIPSILENCE) Unimod_StripSilence(mf, SILENCE_THRESHOLD);

    md_mode = 0;
    mp = Player_InitSong(mf, NULL, PF_TIMESEEK, config_voices);
    Player_SetLoopStatus(mp, config_playflag & CPLAYFLG_LOOPALL, config_loopcount);
    Player_BuildQuickLookups(mp);
    if(config_playflag & CPLAYFLG_FADEOUT) Player_VolumeFadeEx(mp, MP_VOLUME_CUR, 0, config_fadeout, MP_SEEK_END, config_fadeout);
    mf->songlen = mp->songlen;

	decode_pos_ms = 0;

    mikamp.outMod->SetVolume(-666);
    mikamp.SetInfo(mf->numchn*10000, config_srate/1000, (config_nch > 1) ? 2 : 1, 1);

    Player_Start(mp);

    _mmalloc_report(NULL);

    killDecodeThread=0;
    mpeg_thread_handle = (HANDLE) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) decodeThread,(void *) &killDecodeThread,0,(ulong *)&mpeg_threadid);
    paused = 0;

    return 0; 
}


void __cdecl pause(void)    { paused=1; mikamp.outMod->Pause(1); }
void __cdecl unpause(void)  { paused=0; mikamp.outMod->Pause(0); }
int  __cdecl ispaused(void) { return paused; }

// _____________________________________________________________________________________
//
void __cdecl stop(void)
{
	if (mpeg_thread_handle != INVALID_HANDLE_VALUE)
    {   killDecodeThread=1;
        if (WaitForSingleObject(mpeg_thread_handle,2000) == WAIT_TIMEOUT)
        {   MessageBox(NULL,"error asking thread to die!\n","error killing decode thread",0);
            TerminateThread(mpeg_thread_handle,0);
        }

        CloseHandle(mpeg_thread_handle);
        mpeg_threadid = 0;
        mpeg_thread_handle = INVALID_HANDLE_VALUE;
		if (is_tempfile && t_file[0])
		{	t_file[0]   = 0;
			is_tempfile = 0;
			DeleteFile(t_file);
		}
    }

    Player_Free(mp); mp = NULL;

    {
        // We need to see if mf is in use.  If so, then we can't unload it.
        // Bute we *do* have to unload its samples, because those are not needed.
        
        INFOBOX  *cruise;
        BOOL      nofree = FALSE;

        cruise = infobox_list;
        while(cruise)
        {   if(cruise->module == mf)
                nofree = TRUE;
            cruise = cruise->next;
        }

        if(nofree)
            Unimod_UnloadSamples(mf);
        else
            Unimod_Free(mf);
    }
    
    mf = NULL;

    Mikamp_Exit(md); md = NULL;
    mikamp.SAVSADeInit();
}

volatile int seek_needed=-1;

int __cdecl getlength(void )  { if (mp) return mp->songlen; else return 0; }
int __cdecl getoutputtime(void)
{   return (decode_pos_ms/64) + (mikamp.outMod->GetOutputTime()-mikamp.outMod->GetWrittenTime());
}

void __cdecl setoutputtime(int time_in_ms)
{
	seek_needed = time_in_ms;
}

void __cdecl setvolume(int volume) { mikamp.outMod->SetVolume(volume); }
void __cdecl setpan(int pan)       { mikamp.outMod->SetPan(pan); }


// _____________________________________________________________________________________
//
int __cdecl infobox(char *fn, HWND hwnd)
{
    UNIMOD  *m = NULL;
    INFOBOX *cruise;

    if (!lstrcmpi(fn,lastfn) && t_file[0])
        fn = t_file;

    strcpy(curfn,fn);

    // First we check our array of loaded dialog boxes.  If there are any filename matches,
    // then we just bring that window to the foreground!

    cruise = infobox_list;
    while(cruise)
    {   if(!lstrcmpi(cruise->module->filename, fn))
        {   SetForegroundWindow(cruise->hwnd);  return 0;  }
        cruise = cruise->next;
    }

    // Now we check the filename against the currently playing song:

    if(mf && !lstrcmpi(mf->filename, fn))
        m = mf;
    else
    {   MPLAYER   *ps;
        m  = Unimod_LoadInfo(fn);
        if(!m) return -1;

        if(config_playflag & CPLAYFLG_STRIPSILENCE) Unimod_StripSilence(m, SILENCE_THRESHOLD);
        ps = Player_Create(m, PF_TIMESEEK);
        Player_SetLoopStatus(ps, config_playflag & CPLAYFLG_LOOPALL, config_loopcount);
        Player_BuildQuickLookups(ps);
        m->songlen = ps->songlen;
        Player_Free(ps);
    }

    // Create a local dataspace for this window, which will store it's instance-data.

    if(m)
    {   DLGHDR  *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR)); 

        if(pHdr)
        {   HWND     dialog;
            INFOBOX *newbox;
            CHAR     str[128];

            pHdr->left    = 7;  pHdr->top    = 168;
            pHdr->module  = m;  pHdr->maxv   = 0;

            dialog = infoDlg(hwnd, pHdr);

            // add this puppy to the list:

            newbox         = (INFOBOX *)_mm_calloc(NULL, 1, sizeof(INFOBOX));
            newbox->next   = infobox_list;
            infobox_list   = newbox;

            newbox->hwnd   = dialog;
            newbox->module = m;
 
            wsprintf(str,"%d\n%d\n0 of %d\n\nNot Playing...", m->inittempo, m->initspeed, m->numpos);
            SetWindowText(GetDlgItem(dialog,IDC_INFORIGHT),str);
       }
    }

    return 0;
}


int __cdecl isourfile(char *fn) { return 0; }

// _____________________________________________________________________________________
//
void __cdecl getfileinfo(char *filename, char *title, int *length_in_ms)
{
    UNIMOD *m = mf;

    if(filename && *filename)
    if(!m || strcmp(filename, mf->filename))
    {   if (!lstrcmpi(filename,lastfn) && t_file[0]) filename = t_file;
        strcpy(curfn,filename);
		m = Unimod_LoadInfo(filename);
        if(!m) return;
        if(config_playflag & CPLAYFLG_STRIPSILENCE) Unimod_StripSilence(m, SILENCE_THRESHOLD);
    }

    if (m)
	{   char *p = m->songname;
		
	    if (p)
		{	while (*p == ' ') p++;
			if (!*p) 
			{	if (!*filename) filename=lastfn;
                strcpy(curfn,filename);
			    p = filename+strlen(filename);
		        while (p >= filename && *p != '\\') p--;
			    if (title) strcpy(title,++p);
			} else if (title) strcpy(title,m->songname);
		}

		if (m != mf)
        {   if(length_in_ms)
            {   MPLAYER *ptmp;

                ptmp = Player_Create(m, PF_TIMESEEK);
                Player_SetLoopStatus(ptmp, config_playflag & CPLAYFLG_LOOPALL, config_loopcount);
                Player_PredictSongLength(ptmp);
                *length_in_ms = ptmp->songlen;
                Player_Free(ptmp);
            }
            Unimod_Free(m);
        } else
        {   if(length_in_ms && mp) *length_in_ms = mp->songlen;
        }
	} else 
	{   if (title) strcpy(title,filename);
		if (length_in_ms) *length_in_ms = -1000;
	}

}

// _____________________________________________________________________________________
//
void __cdecl eq_set(int on, char data[10], int preamp)
{ 
}

static CHAR capnstupid[3072];

// _____________________________________________________________________________________
//
In_Module mikamp = 
{
    IN_VER,
    "Mikamp Module Music Decoder 3.2.0"
#ifdef __alpha
    "(AXP)"
#else
    "(x86)"
#endif
    ,
    0,  // hMainWindow
    0,  // hDllInstance
    /*"IT\0Impulse Tracker Modules (*.IT)\0"
    "XM\0eXtended Modules (*.XM)\0"
    "S3M;STM\0Screamtracker Modules (*.S3M;*.STM)\0"
    "MOD\0Protracker Modules (*.MOD)\0"
    "MTM;ULT\0Other Module Types (*.MTM;*.ULT)\0"*/
    //"DSM;FAR;ULT;MTM;669\0Other Module Types (*.MTM;*.DSM;*.FAR;*.ULT;*.669)\0"

    capnstupid,
    1,  // is_seekable
	1,  // uses_output_plug
    config,
    about,
    init,
    quit,
    getfileinfo,
    infobox,
    isourfile,
    play,
    pause,
    unpause,
    ispaused,
    stop,

    getlength,
    getoutputtime,
    setoutputtime,

    setvolume,
    setpan,

	0,0,0,0,0,0,0,0,0, // vis stuff

    0,0, // dsp shit

    eq_set,

    NULL,        // setinfo
	NULL // outmod
};

extern "C" __declspec( dllexport ) In_Module * __cdecl winampGetInModule2();

// _____________________________________________________________________________________
//
__declspec( dllexport ) In_Module * __cdecl winampGetInModule2()
{
    return &mikamp;
}

// _____________________________________________________________________________________
//
DWORD WINAPI __stdcall decodeThread(volatile void *b)
{
    int  has_flushed = 0;
    while (! *((int *)b) ) 
    {
        if (seek_needed >=0 )
        {
            int a=seek_needed;
            seek_needed=-1;
            Player_SetPosTime(mp,a);
            mikamp.outMod->Flush(a);
			decode_pos_ms = a*64;
            if( paused ) mikamp.outMod->Pause(1);
        }
        
		if (!Player_Active(mp))
        {
            if (!has_flushed)
            {   has_flushed=1;
                mikamp.outMod->Write(NULL,0); // write all samples into buffer queue
            }
            if (!mikamp.outMod->IsPlaying()) 
            {   PostMessage(mikamp.hMainWindow,WM_WA_MPEG_EOF,0,0);
                return 0;
            } else mikamp.outMod->CanWrite(); // make sure plug-in can do any extra processing needed
		    Sleep(10);

        } else
        {
            Mikamp_Update(md);
            Sleep(1);
        }
    }

    return 0;
}

