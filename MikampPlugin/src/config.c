/*
 Mikamp Plugin for Winamp

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 config.c

*/

#include "main.h"
#include <shlobj.h>
#include <commctrl.h>

#define C_PAGES                4
#define C_NUMLOADERS           8

#define CFG_UNCHANGED       (-1)

#define EFFECT_8XX      (1ul<<0)
#define EFFECT_ZXX      (1ul<<1)

#define CPUTBL_COUNT          23
#define MIXTBL_COUNT           7
#define VOICETBL_COUNT         8

 
// =====================================================================================
    typedef struct tag_cdlghdr
// =====================================================================================
{
    CHAR *filename;
    HWND  hwndTab;       // tab control 
    HWND  hwndDisplay;   // current child dialog box 
	int   left,top;
    HWND  apRes[C_PAGES];

} CFG_DLGHDR; 


// Winamp Stuff
UBYTE    config_savestr=0, config_altvolset;  // save to disk/alt volume contorl
UBYTE    config_priority=(1<<4) | (1<<2) | 0; // mixer+player thread priority

// Output Settings
UBYTE    config_8bit=0, config_nch=2;
UBYTE    config_cpu=0; // config_cpu_autodetect = 1; // MMX / 3DNow / whatever else might come out.

// Player Settings...
// ------------------

uint     config_loopcount = 1;    // Auto-looping count, if the song has a natural loop.
uint     config_playflag  = 0;    // See CPLAYFLG_* defines above for flags
int      config_pansep    = 128;  // master panning separation (0 == mono, 128 == stereo, 512 = way-separate)
UBYTE    config_resonance = 1;
int      config_fadeout   = 1000; // fadeout when the song is ending last loop (in ms)

// Mixer Settings...
// -----------------

UBYTE    config_surround  = 1,    // Surround sound support?
         config_panrev    = 0,    // Reverse panning (left<->right)
         config_interp    = 3;    // interpolation (bit 0) / noclick (bit 1)
uint     config_srate     = 44100;
uint     config_voices    = 96;   // maximum voices player can use.


// Local Crud...
// -------------

static char   INI_FILE[MAX_PATH];
static char   app_name[] = "Mikamp Module Music Decoder";

static int    l_quality,   l_srate,    // l_quality indexes mixertbl. l_srate is the actual number.
              l_voices;
static BOOL   l_useres,    l_nch,
              l_iflags;

static char *get_inifile() { return INI_FILE; }


// mhztable
// --------
// This is a mhz 'scale' for each level of voice use.  The value is a ratio of voice-
// to-cpu conversion, where the first value is effectively about 3-to-1 (3 mhz per
// voice Effectivey this makes assumptions that lower voices target slower CPUs and
// hence more mhz are needed.  read: this is a heck of a hack, but it gives pretty
// accurate results!

// cputable
// --------
// list of 'legit' frequency displays for CPUs.  Makes it look neater!


static uint mhztable[VOICETBL_COUNT] = {  188, 156, 124, 110, 100, 95, 88, 84 };

static uint cputable[] = {  16,   20,  33,  40,  50,  66,  75, 100, 133, 166, 200, 233, 266, 
                            300, 333, 350, 400, 450, 500, 550, 600, 733, 833 };

static uint voicetable[VOICETBL_COUNT] =  {  24, 32, 48, 64, 96, 128, 256, 512 };
static uint mixertbl[MIXTBL_COUNT] = 
{ 
    48000,
    44100,
    33075,
    22050,
    16000,
    11025,
    8000,
};


// =====================================================================================
    typedef struct _MLCONF
// =====================================================================================
// Define Extended Loader Details/Options
{
    CHAR     *cfgname;         // configuration name prefix.
    CHAR     *desc;            // long-style multi-line description!
    CHAR     *ext;             // extensions used by this format.

    MLOADER  *loader;          // mikamp loader to register

    BOOL     enabled;

    uint     defpan,           // default panning separation \  range 0 - 128
             pan;              // current panning separation /  (mono to stereo)

    uint     optavail,         // available advanced effects options
             optset;           // current settings for those options.

} MLCONF;


static MLCONF c_mod[C_NUMLOADERS] = 
{
    "mod",
    "Protracker and clones (all versions) [mod]:\nIncludes Protracker, Fasttracker 1, Taketracker, Startrekker. Limited to 30 samples.",
    "mod",
    &load_mod,
    TRUE,

    0,0,
    EFFECT_8XX, 0,

    "m15",
    "Old-skool Amiga Modules [mod]:\n"
    "Soundtracker, Ultimate Soundtracker. Very old and extremely limited in many ways (4 channels, 15 samples)",
    "mod",
    &load_m15,
    TRUE,

    0,0,
    EFFECT_8XX, 0,

    "stm",
    "Scream Tracker 2.xx [stm]:\nSimilar to ST3 modules, but limited to 4 channels; Various format quirks.",
    "stm",
    &load_stm,
    TRUE,

    0,0,
    EFFECT_8XX | EFFECT_ZXX,  0,

    "st3",
    "Scream Tracker 3.xx [s3m]:\nUp to 16 channels; Features a wavetable/FM combo (albeit rarely used).",
    "s3m",
    &load_s3m,
    TRUE,

    32,32,
    EFFECT_8XX | EFFECT_ZXX,  0,


    "it",
    "Impulse Tracker (all versions) [it]:\nSupports 64 channels, new note actions, and resonance filters.",
    "it",
    &load_it,
    TRUE,

    0,0,
    EFFECT_ZXX,  0,

    "ft2",
    "Fasttracker 2.xx [xm]:\nSupports 32 channels, 128 instruments, and volume/panning envelopes.",
    "xm",
    &load_xm,
    TRUE,

    0,0,
    0, 0,

    "mtm",
    "Multitracker (all versions) [mtm]:\nA 'superfied' Protracker-based format, features std effects /w 32 channels.",
    "mtm",
    &load_mtm,
    TRUE,

    0,0,
    EFFECT_8XX,  0,

    "ult",
    "Ultra Tracker (all versions) [ult]:\nDesigned specifically for the Gravis Ultrasound, features 32 channels and two effects per row.",
    "ult",
    &load_ult,
    TRUE,

    0,0,
    EFFECT_8XX,  0,
};

static MLCONF l_mod[C_NUMLOADERS];                  // local copy, for cancelability


// =====================================================================================
    static int _r_i(char *name, int def)
// =====================================================================================
{
    name += 7;
    return GetPrivateProfileInt(app_name,name,def,INI_FILE);
}
#define RI(x) (( x ) = _r_i(#x,( x )))


// =====================================================================================
    static void _w_i(char *name, int d)
// =====================================================================================
{
    char str[120];
    wsprintf(str,"%d",d);
    name += 7;
    WritePrivateProfileString(app_name,name,str,INI_FILE);
}
#define WI(x) _w_i(#x,( x ))


// =====================================================================================
    static void config_buildbindings(CHAR *datext)
// =====================================================================================
// Creates the binding string.
{
    uint   i, pos = 0;
    BOOL   moduse = 0;

    //if(datext) _mm_free(datext,"Bindings String");
    
    datext[0] = 0;
    
    for(i=0; i<C_NUMLOADERS; i++)
    {
        if(c_mod[i].enabled)
        {   if(i!=1 || (i==1 && !moduse))
            {   sprintf(&datext[pos], "%s", c_mod[i].ext);
                pos += strlen(&datext[pos])+1;
                sprintf(&datext[pos], "%s (*.%s)", c_mod[i].loader->Version,c_mod[i].ext);
                pos += strlen(&datext[pos])+1;
            }
            if(i==0) moduse = 1;
        }
    }

    datext[pos] = 0;

    //datext = _mm_malloc(sizeof(CHAR) * pos);
    //memcpy(datext,workspace, sizeof(CHAR) * pos);
}


// =====================================================================================
    static void config_init(void)
// =====================================================================================
{
    char *p;
    GetModuleFileName(NULL,INI_FILE,sizeof(INI_FILE));
    p=INI_FILE+strlen(INI_FILE);
    while (p >= INI_FILE && *p != '.') p--;
    strcpy(++p,"ini");
}


// =====================================================================================
    void config_read(void)
// =====================================================================================
{
    uint   t;

    config_init();

	RI(config_savestr);
    RI(config_priority);

    RI(config_8bit);
    RI(config_nch);
    RI(config_srate);
	RI(config_interp);
	RI(config_surround);
	RI(config_voices);

	RI(config_loopcount);
	RI(config_playflag);
	RI(config_resonance);
	RI(config_fadeout);

	RI(config_pansep);
	RI(config_panrev);

  	//RI(config_cpu);
  	//RI(config_cpu_autodetect);

    /*if(config_cpu_autodetect == CPU_AUTODETECT)*/ config_cpu = _mm_cpudetect();


    // Load settings for each of the individual loaders
    // ------------------------------------------------

    for(t=0; t<C_NUMLOADERS; t++)
    {
        CHAR   stmp[72];

        sprintf(stmp,"%s%s",c_mod[t].cfgname,"enabled");
        c_mod[t].enabled = GetPrivateProfileInt(app_name,stmp,c_mod[t].enabled,INI_FILE);

        sprintf(stmp,"%s%s",c_mod[t].cfgname,"panning");
        c_mod[t].pan = GetPrivateProfileInt(app_name,stmp,c_mod[t].pan,INI_FILE);
        c_mod[t].pan = _mm_boundscheck(c_mod[t].pan, 0, 128);

        sprintf(stmp,"%s%s",c_mod[t].cfgname,"effects");
        c_mod[t].optset = GetPrivateProfileInt(app_name,stmp,c_mod[t].optset,INI_FILE);

        // configure the loaders
        c_mod[t].loader->enabled  = c_mod[t].enabled;
        c_mod[t].loader->nopaneff = (c_mod[t].optset | EFFECT_8XX) ? FALSE : TRUE;
        c_mod[t].loader->noreseff = (c_mod[t].optset | EFFECT_ZXX) ? FALSE : TRUE;
    }

    config_buildbindings(mikamp.FileExtensions);

    // Bounds checking!
    // ----------------
    // This is important to ensure stability of the product in case some
    // doof goes and starts hacking the ini values carelessly - or if some sort
    // of version conflict or corruption causes skewed readings.

    config_pansep    = _mm_boundscheck(config_pansep, 0, 512);
    config_voices    = _mm_boundscheck(config_voices, 2, 1024);

    config_fadeout   = _mm_boundscheck(config_fadeout, 0, 1000l*1000l);
    config_loopcount = _mm_boundscheck(config_loopcount, 0, 64);
}


// =====================================================================================
    void config_write(void)
// =====================================================================================
{
	uint   t;
    
    WI(config_savestr);
    WI(config_priority);

    WI(config_8bit);
    WI(config_nch);
    WI(config_srate);
	WI(config_interp);
	WI(config_surround);
	WI(config_voices);

	WI(config_loopcount);
	WI(config_playflag);
	WI(config_resonance);
	WI(config_fadeout);

	WI(config_pansep);
	WI(config_panrev);
   	
    //WI(config_cpu);
    //WI(config_cpu_autodetect);


    // Save settings for each of the individual loaders
    // ------------------------------------------------

    for(t=0; t<C_NUMLOADERS; t++)
    {
        CHAR   stmp[72], sint[12];

        sprintf(stmp,"%s%s",c_mod[t].cfgname,"enabled");
        sprintf(sint,"%d",c_mod[t].enabled);
        WritePrivateProfileString(app_name,stmp,sint,INI_FILE);

        sprintf(stmp,"%s%s",c_mod[t].cfgname,"panning");
        sprintf(sint,"%d",c_mod[t].pan);
        WritePrivateProfileString(app_name,stmp,sint,INI_FILE);

        sprintf(stmp,"%s%s",c_mod[t].cfgname,"effects");
        sprintf(sint,"%d",c_mod[t].optset);
        WritePrivateProfileString(app_name,stmp,sint,INI_FILE);

        // configure the loaders
        c_mod[t].loader->enabled = c_mod[t].enabled;
        c_mod[t].loader->nopaneff = (c_mod[t].optset | EFFECT_8XX) ? FALSE : TRUE;
        c_mod[t].loader->noreseff = (c_mod[t].optset | EFFECT_ZXX) ? FALSE : TRUE;
    }

    config_buildbindings(mikamp.FileExtensions);
}


static UINT_PTR CALLBACK prefsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);


// =====================================================================================
    void __cdecl config(HWND hwndParent)
// =====================================================================================
{
	DialogBox(mikamp.hDllInstance,MAKEINTRESOURCE(IDD_PREFS),hwndParent,prefsProc);
    config_write();
}



static UINT_PTR CALLBACK tabProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static UINT_PTR CALLBACK mixerProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static UINT_PTR CALLBACK loaderProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void OnSelChanged(HWND hwndDlg);

// =====================================================================================
    static void prefsTabInit(HWND hwndDlg, CFG_DLGHDR *pHdr) 
// =====================================================================================
{ 
    DWORD   dwDlgBase = GetDialogBaseUnits(); 
    int     cxMargin = LOWORD(dwDlgBase) / 4,
            cyMargin = HIWORD(dwDlgBase) / 8;
    TC_ITEM tie;
    int     tabCounter;
    
	tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 

    tabCounter = 0;

	tie.pszText = "Mixer"; 
	TabCtrl_InsertItem(pHdr->hwndTab, tabCounter, &tie);
    pHdr->apRes[tabCounter] = CreateDialogParam(mikamp.hDllInstance, MAKEINTRESOURCE(IDD_PREFTAB_MIXER), hwndDlg, mixerProc, IDD_PREFTAB_MIXER);
    SetWindowPos(pHdr->apRes[tabCounter], HWND_TOP, pHdr->left, pHdr->top, 0, 0, SWP_NOSIZE);
    ShowWindow(pHdr->apRes[tabCounter++], SW_HIDE);

  	tie.pszText = "Player";
	TabCtrl_InsertItem(pHdr->hwndTab, tabCounter, &tie); 
    pHdr->apRes[tabCounter] = CreateDialogParam(mikamp.hDllInstance, MAKEINTRESOURCE(IDD_PREFTAB_DECODER), hwndDlg, tabProc, IDD_PREFTAB_DECODER);
    SetWindowPos(pHdr->apRes[tabCounter], HWND_TOP, pHdr->left, pHdr->top, 0, 0, SWP_NOSIZE);
    ShowWindow(pHdr->apRes[tabCounter++], SW_HIDE);

  	tie.pszText = "Loader";
	TabCtrl_InsertItem(pHdr->hwndTab, tabCounter, &tie); 
    pHdr->apRes[tabCounter] = CreateDialogParam(mikamp.hDllInstance, MAKEINTRESOURCE(IDD_PREFTAB_LOADER), hwndDlg, loaderProc, IDD_PREFTAB_LOADER);
    SetWindowPos(pHdr->apRes[tabCounter], HWND_TOP, pHdr->left, pHdr->top, 0, 0, SWP_NOSIZE);
    ShowWindow(pHdr->apRes[tabCounter++], SW_HIDE);

  	tie.pszText = "General";
	TabCtrl_InsertItem(pHdr->hwndTab, tabCounter, &tie); 
    pHdr->apRes[tabCounter] = CreateDialogParam(mikamp.hDllInstance, MAKEINTRESOURCE(IDD_PREFTAB_GENERAL), hwndDlg, tabProc, IDD_PREFTAB_GENERAL);
    SetWindowPos(pHdr->apRes[tabCounter], HWND_TOP, pHdr->left, pHdr->top, 0, 0, SWP_NOSIZE);
    ShowWindow(pHdr->apRes[tabCounter++], SW_HIDE);

    // Simulate selection of the first item.
    OnSelChanged(hwndDlg);
} 


// =====================================================================================
    static void OnSelChanged(HWND hwndDlg)
// =====================================================================================
{ 
    CFG_DLGHDR *pHdr = (CFG_DLGHDR *) GetWindowLongPtrA(hwndDlg, GWLP_USERDATA);
    int iSel = TabCtrl_GetCurSel(pHdr->hwndTab);

	if(pHdr->hwndDisplay)  ShowWindow(pHdr->hwndDisplay,SW_HIDE);
	ShowWindow(pHdr->apRes[iSel],SW_SHOW);
	pHdr->hwndDisplay = pHdr->apRes[iSel];

} 


// =====================================================================================
    static void Prefs_CPUSetup(HWND hwndDlg)
// =====================================================================================
{
    //if(config_cpu_autodetect)
    {   config_cpu = _mm_cpudetect();
    }

    /*CheckDlgButton(hwndDlg,IDC_CPU_MMX,config_cpu >= CPU_MMX ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg,IDC_CPU_3DNOW,config_cpu == CPU_3DNOW ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg,IDC_CPU_SIMD,config_cpu >= CPU_SIMD ? BST_CHECKED : BST_UNCHECKED);

    EnableWindow(GetDlgItem(hwndDlg,IDC_CPU_MMX),   config_cpu_autodetect ? 0 : (config_cpu >= CPU_MMX));
    EnableWindow(GetDlgItem(hwndDlg,IDC_CPU_3DNOW), config_cpu_autodetect ? 0 : (config_cpu == CPU_3DNOW));
    EnableWindow(GetDlgItem(hwndDlg,IDC_CPU_SIMD),  config_cpu_autodetect ? 0 : (config_cpu >= CPU_SIMD));
    */
}


// =====================================================================================
    static void FadeOutSetup(HWND hwndDlg, BOOL enabled)
// =====================================================================================
{
    EnableWindow(GetDlgItem(hwndDlg, IDC_FADEOUT), enabled);
    EnableWindow(GetDlgItem(hwndDlg, IDC_FADESEC), enabled);
}


// =====================================================================================
    static void Stereo_Dependencies(HWND hwndDlg)
// =====================================================================================
// Enable or Disable the options which are dependant on stereo being enabled
{   
    BOOL val = (l_nch==1) ? 0 : 1;
    EnableWindow(GetDlgItem(hwndDlg,OUTMODE_REVERSE), val);
    EnableWindow(GetDlgItem(hwndDlg,OUTMODE_SURROUND),val);
}


// =====================================================================================
    static void SetVoiceList(HWND hwndMisc)
// =====================================================================================
// erg is the current quality mode (indexes mixertbl).
{
    uint     i,k;
    BOOL     picked = FALSE;
    uint     mul;
    uint     cfgv;

    cfgv = (l_voices==CFG_UNCHANGED) ? config_voices : voicetable[l_voices];

    // Find out what cpu chart we're using, and then the multiplier for it.

    mul = 256;                      // 8 bit fixed (256 == 1)

    if(l_useres)      mul += 86;    // resonance does about 35%
    if(l_iflags & 1)  mul += 60;    // non-interp does -22%
    if(l_iflags & 2)  mul += 16;    // no declicker does -7%

    mul += 48*l_nch;                   // 18% for each channel

    SendMessage(hwndMisc,CB_RESETCONTENT,0,0);

    for(i=0; i<VOICETBL_COUNT; i++)
    {   CHAR  buf[24];
        int   t;
        uint  mhz;

        // Find the appropriate megaherz, by doing a pretty wacky little logic snippet
        // which matches up the multipled mhz with the closet legit CPU bracket (as
        // listed in cputable).

        mhz = ((l_srate + ((l_srate<18000) ? 2500 : 0)) * mhztable[i] * voicetable[i]) / 256;
        mhz *= mul;
        mhz /= 256*21500;

        t=0;
        while((t<CPUTBL_COUNT-1) && (mhz>cputable[t])) t++;
        if(t && (mhz-cputable[t-1]) < (cputable[t]-mhz)) t--;   // round down if need be

        sprintf(buf,"%s%d (%d mhz)",(voicetable[i]<100) ? "  " : "", voicetable[i], cputable[t]);

        SendMessage(hwndMisc,CB_ADDSTRING,0,(LPARAM)buf);
        if(!picked && (voicetable[i] >= cfgv))
        {  k = i;  picked = TRUE;  }
    }

    // If picked is false, then set to 96 (default)

    if(!picked) k = 4;

    SendMessage(hwndMisc,CB_SETCURSEL,k,0);
}


// =====================================================================================
    static void cmod_and_the_moo(HWND dagnergit, int *moo, int count, uint flag)
// =====================================================================================
{
    int    l;
    BOOL   oneway, otherway, thisway, thatway;

    oneway = otherway = thisway = thatway = FALSE;

    // Set oneway/otherway true for selected/unselected options.
    // Set thisway/thatway true for avail/nonawail options.

    for(l=0; l<count; l++)
    {   if(l_mod[moo[l]].optavail & flag)
        {   thisway = TRUE;
            if(l_mod[moo[l]].optset & flag) oneway = TRUE; else otherway = TRUE;
        } else thatway = TRUE;
    }

    EnableWindow(dagnergit, thisway);
    SendMessage(dagnergit, BM_SETCHECK, (!thisway || (oneway != otherway)) ? (oneway ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE, 0);
}


// =====================================================================================
    static UINT_PTR CALLBACK mixerProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
// =====================================================================================
{	

    switch (uMsg)
    {
        // =============================================================================
        case WM_INITDIALOG:
        // =============================================================================
        // Windows dialog box startup message.  This is messaged for each tab form created.
        // Initialize all of the controls on each of those forms!
        {   
            HWND        hwndMisc;
		    CFG_DLGHDR *pHdr = (CFG_DLGHDR *)GetWindowLongPtrA(GetParent(hwndDlg), GWLP_USERDATA);

            CheckDlgButton(hwndDlg,OUTMODE_16BIT,   config_8bit         ? BST_UNCHECKED : BST_CHECKED);
            CheckDlgButton(hwndDlg,OUTMODE_STEREO, (config_nch==2)      ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg,OQ_INTERP,      (config_interp & 1)  ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg,OQ_NOCLICK,     (config_interp & 2)  ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg,OUTMODE_SURROUND,config_surround     ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg,OUTMODE_REVERSE, config_panrev       ? BST_CHECKED : BST_UNCHECKED);

            l_voices  = CFG_UNCHANGED;

            l_srate   = config_srate;
            l_iflags  = config_interp;
            l_nch     = config_nch;
            l_useres  = config_resonance;

            hwndMisc  = GetDlgItem(hwndDlg,OQ_QUALITY);

            {
                uint     erg, i;
                BOOL     picked = FALSE;

                for(i=0; i<MIXTBL_COUNT; i++)
                {   CHAR  buf[24];
                    sprintf(buf,"%d%s",mixertbl[i],(mixertbl[i]==44100) ? " (default)" : "");
                    SendMessage(hwndMisc,CB_ADDSTRING,0,(LPARAM)buf);
                    if(!picked && (mixertbl[i] < config_srate))
                    {  erg = i ? (i-1) : 0;  picked = TRUE;  }
                }
                // If picked is false, then set to 44100 (default)
                if(!picked) erg = 1;
                SendMessage(hwndMisc,CB_SETCURSEL,erg,0);
                l_quality = erg;
            }

            SetVoiceList(GetDlgItem(hwndDlg,IDC_VOICES));
            Stereo_Dependencies(hwndDlg);

			return TRUE;
        }
        break;

        // =============================================================================
        case WM_COMMAND:
        // =============================================================================
        // Process commands and notification messages recieved from our child controls.

            switch(LOWORD(wParam))
            {   case IDOK:
                {   
                    config_8bit     = IsDlgButtonChecked(hwndDlg,OUTMODE_16BIT) ? 0 : 1;
    				
                    if(l_voices  != CFG_UNCHANGED) config_voices = voicetable[l_voices];

                    config_srate     = l_srate;
                    config_interp    = l_iflags;
                    config_nch       = l_nch;
                    config_resonance = l_useres;

                    config_surround = IsDlgButtonChecked(hwndDlg,OUTMODE_SURROUND) ? 1 : 0;
                    config_panrev   = IsDlgButtonChecked(hwndDlg,OUTMODE_REVERSE)  ? 1 : 0;

	    			config_voices   = _mm_boundscheck(config_voices, 2,1024);
                }
                break;

                // Output Quality / Mixing Performance
                // -----------------------------------
                // From here on down we handle the messages from those controls which affect
                // the performance (and therefore the cpu requirements) of the module decoder.
                // Each one assigns values into a temp variable (l_*) so that if the user
                // cancels the changes are not saved.

                case OUTMODE_STEREO:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {   l_nch = SendMessage((HWND)lParam,BM_GETCHECK,0,0) ? 2 : 1;
                        Stereo_Dependencies(hwndDlg);
                        SetVoiceList(GetDlgItem(hwndDlg,IDC_VOICES));
                    }
                break;

                case OQ_INTERP:
                case OQ_NOCLICK:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {   l_iflags    = IsDlgButtonChecked(hwndDlg,OQ_INTERP)    ? 1 : 0;
                        l_iflags   |= IsDlgButtonChecked(hwndDlg,OQ_NOCLICK)   ? 2 : 0;
                        SetVoiceList(GetDlgItem(hwndDlg,IDC_VOICES));
                    }
                break;

                case IDC_VOICES:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {   int taxi = SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                        l_voices = taxi;
                    }
                break;

                case OQ_QUALITY:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {   int taxi = SendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                        l_quality = taxi;
                        l_srate   = mixertbl[l_quality];
                        SetVoiceList(GetDlgItem(hwndDlg,IDC_VOICES));
                    }
                break;
            }
		break;
    }
    return 0;
}


// =====================================================================================
    static UINT_PTR CALLBACK loaderProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
// =====================================================================================
// This is the callback procedure used by each of the three forms under the tab control 
// on the Preferences dialog box.  It handles all the messages for all of the controls
// within those dialog boxes.
{	
    switch (uMsg)
    {
        // --------------------------------------------------------------
        case WM_INITDIALOG:
        // --------------------------------------------------------------
        // Windows dialog box startup message.  This is messaged for each tab 
        // form created. Initialize all of the controls on each of those forms!
        {   
            HWND        hwndMisc;
		    CFG_DLGHDR *pHdr = (CFG_DLGHDR *)GetWindowLongPtrA(GetParent(hwndDlg), GWLP_USERDATA);

            uint   i;

            // Set the range on the panning slider (0 to 16)

            hwndMisc = GetDlgItem(hwndDlg,IDLDR_PANPOS);
            SendMessage(hwndMisc, TBM_SETRANGEMAX,  0, 16);
            SendMessage(hwndMisc, TBM_SETRANGEMIN,  0,  0);
            SendMessage(hwndMisc, TBM_SETPOS,       1, 16);

            // Build our list of loaders in the loader box
            // -------------------------------------------
            // TODO: eventually I would like to display little checkmarks (or llamas or
            // something) in this list to indicate (at a glance) which are enabled and
            // which are not.
            
            hwndMisc = GetDlgItem(hwndDlg,IDLDR_LIST);

            for(i=0; i<C_NUMLOADERS; i++)
            {   CHAR  stupid[256];
                l_mod[i] = c_mod[i];
                sprintf(stupid, "%s [%s]",c_mod[i].loader->Version,c_mod[i].ext);
                SendMessage(hwndMisc, LB_ADDSTRING, 0, (LPARAM) stupid);
            }

            SendMessage(hwndMisc, LB_SETCURSEL, 0, 0);
            loaderProc(hwndDlg, WM_COMMAND, (WPARAM)((LBN_SELCHANGE << 16) + IDLDR_LIST), (LPARAM)hwndMisc);
        }
        return TRUE;

        // --------------------------------------------------------------
        case WM_COMMAND:
        // --------------------------------------------------------------
        // Process commands and notification messages recieved from our child controls.

            switch(LOWORD(wParam))
            {   
                // ______________________________________________________
                case IDOK:
                {   uint  i;
                    for(i=0; i<C_NUMLOADERS; i++)
                    {   c_mod[i] = l_mod[i];
                        c_mod[i].loader->defpan = c_mod[i].pan;
                    }
                }
                break;

                // ______________________________________________________
                case IDLDR_LIST:
                    
                    // The Loader Box Update Balloofie
                    // -------------------------------
                    // Updates the various controls on the 'loader tab' dialog box.  Involves
                    // enabling/disabling advanced-effects boxes, checking the Enabled box, and
                    // setting the panning position.  Also: extra care is taken to allow proper
                    // and intuitive support for multiple selections!

                    if(HIWORD(wParam) == LBN_SELCHANGE)
                    {   int    moo[C_NUMLOADERS], count,l;
                        BOOL   oneway, otherway;
                        HWND   beiownd;

                        // Fetch the array of selected items!

                        count = SendMessage((HWND)lParam, LB_GETSELITEMS, C_NUMLOADERS, (LPARAM)moo);
                        if(!count || (count == LB_ERR))
                        {
                            // Something's not right, so just disable all the controls.

                            SetWindowText(GetDlgItem(hwndDlg, IDLDR_DESCRIPTION), "Select any loader(s) at right to edit their properties...");
                            EnableWindow(beiownd = GetDlgItem(hwndDlg, IDLDR_PANPOS), FALSE);
                            EnableWindow(beiownd = GetDlgItem(hwndDlg, IDLDR_ENABLED), FALSE);
                            SendMessage(hwndDlg, BM_GETCHECK, BST_UNCHECKED,0);
                            EnableWindow(beiownd = GetDlgItem(hwndDlg, IDLDR_EFFOPT1), FALSE);
                            SendMessage(hwndDlg, BM_GETCHECK, BST_UNCHECKED,0);
                            EnableWindow(beiownd = GetDlgItem(hwndDlg, IDLDR_EFFOPT2), FALSE);
                            SendMessage(hwndDlg, BM_GETCHECK, BST_UNCHECKED,0);

                            break;
                        }

                        SetWindowText(GetDlgItem(hwndDlg, IDLDR_DESCRIPTION), (count==1) ? l_mod[moo[0]].desc : "Multiple items selected...");


                        // Enabled Box : First of Many
                        // ---------------------------

                        oneway = otherway = FALSE;
                        for(l=0; l<count; l++)
                            if(l_mod[moo[l]].enabled) oneway = TRUE; else otherway = TRUE;

                        EnableWindow(beiownd = GetDlgItem(hwndDlg, IDLDR_ENABLED), TRUE);
                        SendMessage(beiownd, BM_SETCHECK, (oneway != otherway) ? (oneway ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE, 0);


                        // The PanningPos : Second in Command
                        // ----------------------------------
                        // Only set it if we have a single format selected, otherwise... erm..
                        // do something (to be determined!)

                        beiownd = GetDlgItem(hwndDlg, IDLDR_PANPOS);
                        EnableWindow(beiownd, TRUE);
                        if(count==1)
                            SendMessage(beiownd, TBM_SETPOS, TRUE,  16-((l_mod[moo[0]].pan+1)>>3));
                        //} else
                            //EnableWindow(beiownd, FALSE);


                        // 8xx Panning Disable: Third of Four
                        // Zxx Resonance: All the Duckies are in a Row!
                        // --------------------------------------------

                        cmod_and_the_moo(GetDlgItem(hwndDlg, IDLDR_EFFOPT1), moo, count, EFFECT_8XX);
                        cmod_and_the_moo(GetDlgItem(hwndDlg, IDLDR_EFFOPT2), moo, count, EFFECT_ZXX);
                    }
                break;

                // ______________________________________________________
                case IDLDR_ENABLED:
                case IDLDR_EFFOPT1:
                case IDLDR_EFFOPT2:

                    if(HIWORD(wParam) == BN_CLICKED)
                    {   int  moo[C_NUMLOADERS],count;
                        int  res = SendMessage((HWND)lParam,BM_GETCHECK,0,0);
                        
                        count = SendMessage(GetDlgItem(hwndDlg, IDLDR_LIST), LB_GETSELITEMS, C_NUMLOADERS, (LPARAM)moo);

                        switch(res)
                        {
                            case BST_CHECKED:
                                SendMessage((HWND)lParam,BM_SETCHECK,BST_UNCHECKED,0);
                                res = 0;
                            break;

                            case BST_INDETERMINATE:
                            case BST_UNCHECKED:
                                SendMessage((HWND)lParam,BM_SETCHECK,BST_CHECKED,0);
                                res = 1;
                            break;
                        }

                        if(LOWORD(wParam) == IDLDR_ENABLED)
                        {   int   l;
                            for(l=0; l<count; l++) l_mod[moo[l]].enabled = res;
                        } else
                        {
                            uint  flag = (LOWORD(wParam) == IDLDR_EFFOPT1) ? EFFECT_8XX : EFFECT_ZXX;
                            int   l;

                            for(l=0; l<count; l++)
                            {   if(l_mod[moo[l]].optavail & flag)
                                {   if(res)    
                                        l_mod[moo[l]].optset |= flag;
                                    else
                                        l_mod[moo[l]].optset &= ~flag;
                                }
                            }
                        }
                    }
                break;
            }
        break;

        // --------------------------------------------------------------
        case WM_HSCROLL:
        // --------------------------------------------------------------
        // Whitness Stupidness!
        // Microsoft decides it would be this "brilliant move' to make trackbars send 
        // WM_HSCROLL and WM_VSCROLL messages only!  Like, who the hell uses a trackbar
        // as a scroller anyway?  Whatever happened to the 'standard' system of command/
        // notify messages?  Grrr.

        // Oh look, the LOWORD is the command this time, as opposed to notifies, where the
        // HIWORD is the command.  Jesus fucking Christ I'm in loonyland around here.

        if(LOWORD(wParam) == TB_THUMBPOSITION)
        {
            int  moo[C_NUMLOADERS],count,l;
            count = SendMessage(GetDlgItem(hwndDlg, IDLDR_LIST), LB_GETSELITEMS, C_NUMLOADERS, (LPARAM)moo);

            for(l=0; l<count; l++)
            {   l_mod[moo[l]].pan = (16 - HIWORD(wParam)) << 3;
                l_mod[moo[l]].pan = _mm_boundscheck(l_mod[moo[l]].pan,0,128);
            }
        }

    }

    return 0;
}


// =====================================================================================
    static void FadeoutSetText(HWND hwndDlg)
// =====================================================================================
{
    CHAR  work[32];
    sprintf(work, "%u.%02u",config_fadeout/1000,config_fadeout % 1000);
    SetDlgItemText(hwndDlg, IDC_FADEOUT, work);
}


// =====================================================================================
    static UINT_PTR CALLBACK tabProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
// =====================================================================================
{	
    switch (uMsg)
    {
        // --------------------------------------------------------------
        case WM_INITDIALOG:
        // --------------------------------------------------------------
        // Windows dialog box startup message.  This is messaged for each tab form created.
        // Initialize all of the controls on each of those forms!
        {   
            HWND        hwndMisc;
		    CFG_DLGHDR *pHdr = (CFG_DLGHDR *)GetWindowLongPtrA(GetParent(hwndDlg), GWLP_USERDATA);

            switch(lParam)
            {
                case IDD_PREFTAB_DECODER:
                    SendMessage(GetDlgItem(hwndDlg,IDC_LOOPBOX), EM_SETLIMITTEXT, 3,0);
                    SetDlgItemInt(hwndDlg, IDC_LOOPBOX, config_loopcount, FALSE);

                    SendMessage(GetDlgItem(hwndDlg,IDC_FADEOUT), EM_SETLIMITTEXT, 10,0);
                    FadeoutSetText(hwndDlg);

                    hwndMisc = GetDlgItem(hwndDlg,IDC_PANSEP);
                    SendMessage(hwndMisc,TBM_SETRANGEMAX,0,32);
                    SendMessage(hwndMisc,TBM_SETRANGEMIN,0,0);
                    
                    {
                    int  erg  = config_pansep;

                    if(erg <= 128) erg *= 2;
                    else erg = 256 + ((erg - 128) * 256) / 384;
                    
                    SendMessage(hwndMisc,TBM_SETPOS,1, (erg>>4)&31);
                    }
                    CheckDlgButton(hwndDlg,IDC_LOOPALL,     (config_playflag & CPLAYFLG_LOOPALL)      ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hwndDlg,IDC_PLAYALL,     (config_playflag & CPLAYFLG_PLAYALL)      ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hwndDlg,IDC_FADECHECK,   (config_playflag & CPLAYFLG_FADEOUT)      ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hwndDlg,IDC_STRIPSILENCE,(config_playflag & CPLAYFLG_STRIPSILENCE) ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hwndDlg,IDC_RESONANCE,config_resonance ? BST_CHECKED : BST_UNCHECKED);

                    FadeOutSetup(hwndDlg, config_playflag & CPLAYFLG_FADEOUT);
                return TRUE;

                case IDD_PREFTAB_GENERAL:
                    hwndMisc = GetDlgItem(hwndDlg,IDC_PREFS_PRIORITY_DECODE);
                    SendMessage(hwndMisc,TBM_SETRANGEMAX,0,2);
                    SendMessage(hwndMisc,TBM_SETRANGEMIN,0,0);
                    SendMessage(hwndMisc,TBM_SETPOS,1, (config_priority>>4)&3);

                    CheckDlgButton(hwndDlg,IDC_SAVESTR,config_savestr ? BST_CHECKED : BST_UNCHECKED);

                    //CheckRadioButton(hwndDlg,IDC_CPU_AUTODETECT, IDC_CPU_MANUAL, config_cpu_autodetect ? IDC_CPU_AUTODETECT : IDC_CPU_MANUAL);
                    Prefs_CPUSetup(hwndDlg);
                return TRUE;
            }
        }
        break;

        // --------------------------------------------------------------
        case WM_COMMAND:
        // --------------------------------------------------------------
        // Process commands and notification messages recieved from our child controls.

            switch(LOWORD(wParam))
            {   case IDOK:
                {   BOOL   bresult;

                    switch(lParam)
                    {   
                        CHAR    stmp[32];
                        double  ftmp;

                        case IDD_PREFTAB_DECODER:
                            config_loopcount = GetDlgItemInt(hwndDlg, IDC_LOOPBOX, &bresult, FALSE);
	    				    config_loopcount = _mm_boundscheck(config_loopcount, 0, 16);

                            GetDlgItemText(hwndDlg, IDC_FADEOUT, stmp, 12);
                            ftmp = atof(stmp);
                            config_fadeout    = (int)(ftmp * 1000l);
	    				    config_fadeout    = _mm_boundscheck(config_fadeout, 0, 1000l*1000l);   // bound to 1000 seconds.

                            {
                            int  erg  = SendMessage(GetDlgItem(hwndDlg,IDC_PANSEP),TBM_GETPOS,0,0)<<4;

                            if(erg <= 256) erg /= 2;
                            else erg = 128 + ((erg - 256) * 384) / 256;
                            
                            config_pansep     = erg;
                            }

	            			config_playflag   = IsDlgButtonChecked(hwndDlg,IDC_LOOPALL)      ? CPLAYFLG_LOOPALL : 0;
	            			config_playflag  |= IsDlgButtonChecked(hwndDlg,IDC_PLAYALL)      ? CPLAYFLG_PLAYALL : 0;
	            			config_playflag  |= IsDlgButtonChecked(hwndDlg,IDC_FADECHECK)    ? CPLAYFLG_FADEOUT : 0;
	            			config_playflag  |= IsDlgButtonChecked(hwndDlg,IDC_STRIPSILENCE) ? CPLAYFLG_STRIPSILENCE : 0;
	            			config_resonance  = IsDlgButtonChecked(hwndDlg,IDC_RESONANCE) ? 1 : 0;
                        break;

            			case IDD_PREFTAB_GENERAL:
                            config_priority = SendMessage(GetDlgItem(hwndDlg,IDC_PREFS_PRIORITY_DECODE),TBM_GETPOS,0,0)<<4;

                            //config_cpu_autodetect = IsDlgButtonChecked(hwndDlg,IDC_CPU_AUTODETECT) ? 1 : 0;
                            //config_cpu            = IsDlgButtonChecked(hwndDlg,IDC_CPU_MMX) ? CPU_MMX : 0;
                            //if(IsDlgButtonChecked(hwndDlg,IDC_CPU_3DNOW)) config_cpu = CPU_3DNOW;
                            //if(IsDlgButtonChecked(hwndDlg,IDC_CPU_SIMD))  config_cpu = CPU_SIMD;

	            			config_savestr  = IsDlgButtonChecked(hwndDlg,IDC_SAVESTR)   ? 1 : 0;
                            
                            set_priority();
                        break;
                    }
                }
                break;

                /*case IDC_CPU_AUTODETECT:
                    config_cpu_autodetect = 1;
                    Prefs_CPUSetup(hwndDlg);
                break;

                case IDC_CPU_MANUAL:
                    config_cpu_autodetect = 0;
                    Prefs_CPUSetup(hwndDlg);
                break;*/

                case IDC_FADECHECK:         // hide/unhide fadeout controls
                    if(HIWORD(wParam) == BN_CLICKED)
                    {   int  res = SendMessage((HWND)lParam,BM_GETCHECK,0,0);
                        FadeOutSetup(hwndDlg, res);
                    }
                break;
            }
		break;

        // --------------------------------------------------------------
        case WM_NOTIFY:
        // --------------------------------------------------------------

            switch(LOWORD(wParam))
            {
                case IDC_FADESPIN:
                {   NMUPDOWN *mud = (NMUPDOWN *) lParam;
                    
                    if(mud->hdr.code == UDN_DELTAPOS)
                    {
                        if(mud->iDelta > 0)
                            config_fadeout -= 250;
                        else
                            config_fadeout += 250;
                        FadeoutSetText(hwndDlg);
                    }
                }
                return TRUE;
            }
        break;

    }
    return 0;
}


// =====================================================================================
    static UINT_PTR CALLBACK prefsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
// =====================================================================================
// This is the procedure which initializes the various forms that make up the tabs in
// our preferences box!  This also contains the message handler for the OK and Cancel
// buttons.  After that, all messaging is handled by the tab forms themselves in tabProc();
{
    switch (uMsg)
    {   
        // --------------------------------------------------------------
        case WM_INITDIALOG:
        // --------------------------------------------------------------
        {
            CFG_DLGHDR *pHdr = (CFG_DLGHDR *) LocalAlloc(LPTR, sizeof(CFG_DLGHDR));
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pHdr); 
          	pHdr->hwndTab = GetDlgItem(hwndDlg,MM_PREFTAB);
            pHdr->left   = 8;  pHdr->top      = 30;
 
            prefsTabInit(hwndDlg, pHdr);
        }
        return FALSE;
	
        // --------------------------------------------------------------
	    case WM_COMMAND:
        // --------------------------------------------------------------
			switch (LOWORD(wParam))
			{	case IDOK:
                {   CFG_DLGHDR *pHdr;

                    // Send an IDOK command to both tabcontrol children to let them know
                    // that the world is about to end!
                    
                    pHdr = (CFG_DLGHDR *)GetWindowLongPtrA(hwndDlg, GWLP_USERDATA);
                    SendMessage(pHdr->apRes[0], WM_COMMAND, (WPARAM)IDOK, (LPARAM)IDD_PREFTAB_MIXER);
                    SendMessage(pHdr->apRes[1], WM_COMMAND, (WPARAM)IDOK, (LPARAM)IDD_PREFTAB_DECODER);
                    SendMessage(pHdr->apRes[2], WM_COMMAND, (WPARAM)IDOK, (LPARAM)IDD_PREFTAB_LOADER);
                    SendMessage(pHdr->apRes[3], WM_COMMAND, (WPARAM)IDOK, (LPARAM)IDD_PREFTAB_GENERAL);

                    EndDialog(hwndDlg,0);
                return 0;
                }
				
			    case IDCANCEL:
					EndDialog(hwndDlg,0);
				return FALSE;

                case OQ_QUALITY:
                    uMsg = 8;
                break;
			}
		break;

        // --------------------------------------------------------------
		case WM_NOTIFY:
        // --------------------------------------------------------------
		{
            NMHDR *notice = (NMHDR *) lParam;
            NMHDR *ack;

            uint   k;
            ack = (NMHDR *)lParam;

            if(ack->hwndFrom == GetDlgItem(hwndDlg, OQ_QUALITY))
            {
                switch(ack->code)
                {
                    case CBEN_GETDISPINFO:
                        k = 1;
                    break;
                }
            }

			switch(notice->code)
            {   case TCN_SELCHANGE:
                    OnSelChanged(hwndDlg);
			    return TRUE;
			}
		}
	    return FALSE;

    
    }
    return FALSE;
}


static UINT_PTR CALLBACK AboutProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

// =====================================================================================
    void __cdecl about(HWND hwndParent)
// =====================================================================================
{
    DialogBox(mikamp.hDllInstance,MAKEINTRESOURCE(IDD_DIALOG1),hwndParent,AboutProc);
}


// =====================================================================================
    static UINT_PTR CALLBACK AboutProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
// =====================================================================================
{
    if (uMsg == WM_COMMAND && LOWORD(wParam) == IDOK) EndDialog(hwndDlg,0);
    return 0;
}
