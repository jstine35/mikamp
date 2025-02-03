/*
 Mikamp Plugin for Winamp

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 info.c
 
 Code implementations for handling messages passed in by the controls present on the
 InfoBox.

*/

#include "main.h"
#include <commctrl.h>


static BOOL CALLBACK infoProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
static BOOL CALLBACK tabProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
static void OnSelChanged(HWND hwndDlg);

INFOBOX   *infobox_list = NULL;

// =====================================================================================
    void infobox_delete(HWND hwnd)
// =====================================================================================
// This function is called with the handle of the infobox to destroy.  It unloads the 
// module if appropriate (care must be take not to unload a module which is in use!).
{
    INFOBOX *cruise, *old;

    old = cruise = infobox_list;

    while(cruise)
    {
        if(cruise->hwnd == hwnd)
        {
            if(cruise == infobox_list)
                infobox_list = cruise->next;
            else
                old->next = cruise->next;

            // Destroy the info box window, then unload the module, *if*
            // the module is not actively playing!

            info_killseeker(hwnd, mf);
            DestroyWindow(hwnd);

            if(cruise->module != mf)
                Unimod_Free(cruise->module);

            _mm_free(NULL, cruise);

            return;
        }
        old    = cruise;
        cruise = cruise->next;
    }
}


// =====================================================================================
    MPLAYER *get_player(UNIMOD *othermf)
// =====================================================================================
// Checks the current module against the one given.  if they match the MP is returned,
// else it returns NULL.
{
    if(mf == othermf) return mp;
    return NULL;
}


// =====================================================================================
    static void infoTabInit(HWND hwndDlg, UNIMOD *m, DLGHDR *pHdr) 
// =====================================================================================
{ 
    DWORD   dwDlgBase = GetDialogBaseUnits(); 
    int     cxMargin = LOWORD(dwDlgBase) / 4,
            cyMargin = HIWORD(dwDlgBase) / 8;
    TC_ITEM tie;
    int     tabCounter;
    
    // Add a tab for each of the three child dialog boxes. 
	// and lock the resources for the child frams that appear within.
    
	tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 

    tabCounter = 0;

    if(m->numsmp)
	{	tie.pszText = "Samples";
		TabCtrl_InsertItem(pHdr->hwndTab, tabCounter, &tie); 
        pHdr->apRes[tabCounter] = CreateDialogParam(mikamp.hDllInstance, MAKEINTRESOURCE(IDD_SAMPLES), hwndDlg, tabProc, IDD_SAMPLES);
        SetWindowPos(pHdr->apRes[tabCounter], HWND_TOP, pHdr->left, pHdr->top, 0, 0, SWP_NOSIZE);
		ShowWindow(pHdr->apRes[tabCounter++], SW_HIDE);
	}

    if(m->numins)
	{	tie.pszText = "Instruments"; 
		TabCtrl_InsertItem(pHdr->hwndTab, tabCounter, &tie);
        pHdr->apRes[tabCounter] = CreateDialogParam(mikamp.hDllInstance, MAKEINTRESOURCE(IDD_INSTRUMENTS), hwndDlg, tabProc, IDD_INSTRUMENTS);
        SetWindowPos(pHdr->apRes[tabCounter], HWND_TOP, pHdr->left, pHdr->top, 0, 0, SWP_NOSIZE);
		ShowWindow(pHdr->apRes[tabCounter++], SW_HIDE);
	}

    if(m->comment && m->comment[0])
	{   tie.pszText = "Comment"; 
        TabCtrl_InsertItem(pHdr->hwndTab, tabCounter, &tie);
        pHdr->apRes[tabCounter] = CreateDialogParam(mikamp.hDllInstance, MAKEINTRESOURCE(IDD_COMMENT), hwndDlg, tabProc, CEMENT_BOX);
        SetWindowPos(pHdr->apRes[tabCounter], HWND_TOP, pHdr->left, pHdr->top, 0, 0, SWP_NOSIZE);
		ShowWindow(pHdr->apRes[tabCounter++], SW_HIDE);
	}
 
    // Simulate selection of the first item.
    OnSelChanged(hwndDlg);
} 


// =====================================================================================
    static BOOL CALLBACK tabProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
// =====================================================================================
// This is the callback procedure used by each of the three forms under the
// tab control on the Module info dialog box (sample, instrument, comment
// info forms).
{	
    switch (uMsg)
    {   case WM_INITDIALOG:
		{
			HWND   hwndLB;
		    DLGHDR *pHdr = (DLGHDR *)GetWindowLong(GetParent(hwndDlg), GWL_USERDATA);
			UNIMOD *m = pHdr->module;
			char   sbuf[10280];

            switch(lParam)
			{   case IDD_SAMPLES:
		        {
                    uint  x;

                    hwndLB = GetDlgItem(hwndDlg, IDC_SAMPLIST);
                    for (x=0; x<m->numsmp; x++)
                    {   sprintf(sbuf, "%02d: %s",x+1, m->samples[x].samplename ? m->samples[x].samplename : "");
                        SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM) sbuf);
                    }
                    SendMessage(hwndLB, LB_SETCURSEL, 0, 0);
                    tabProc(hwndDlg, WM_COMMAND, (WPARAM)((LBN_SELCHANGE << 16) + IDC_SAMPLIST), (LPARAM)hwndLB);
                }
			    return TRUE;

				case IDD_INSTRUMENTS:
		        {
                    uint  x;
                    
                    hwndLB = GetDlgItem(hwndDlg, IDC_INSTLIST);
					for (x=0; x<m->numins; x++)
                    {   sprintf(sbuf, "%02d: %s",x+1, m->instruments[x].insname ? m->instruments[x].insname : "");
                        SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM) sbuf);
                    }
                    SendMessage(hwndLB, LB_SETCURSEL, 0, 0);
                    tabProc(hwndDlg, WM_COMMAND, (WPARAM)((LBN_SELCHANGE << 16) + IDC_INSTLIST), (LPARAM)hwndLB);
                }
			    return TRUE;

				case CEMENT_BOX:
		            if(m->comment && m->comment[0])
                    {   
                        uint  x,i;

                        hwndLB = GetDlgItem(hwndDlg, CEMENT_BOX);

					    // convert all CRs to CR/LF pairs.  That's the way the edit box likes them!

			            for(x=0, i=0; x<strlen(m->comment); x++)
					    {	sbuf[i++] = m->comment[x];
    						if(m->comment[x] == 0x0d) sbuf[i++] = 0x0a;
					    }
					    sbuf[i] = 0;
    					SetWindowText(GetDlgItem(hwndDlg, CEMENT_BOX), sbuf);
                    }
			    return TRUE;
			}
		}
		break;

	    case WM_COMMAND:
            if(HIWORD(wParam) == LBN_SELCHANGE)
			{	// Processes the events for the sample and instrument list boxes, namely updating
                // the samp/inst info upon a WM_COMMAND issuing a listbox selection change.

                int   moo = SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
			    char  sbuf[1024], st1[64], st2[20], st3[20];
			    DLGHDR *pHdr = (DLGHDR *)GetWindowLong(GetParent(hwndDlg), GWL_USERDATA);
				UNIMOD *m = pHdr->module;

				switch (LOWORD(wParam))
				{   case IDC_INSTLIST:
					{	INSTRUMENT *inst = &m->instruments[moo];
                        uint        x;
                        int         cnt;

                        // --------------------
                        // Part 1: General instrument header info
                        //   default volume, auto-vibrato, fadeout (in that order).

                        sprintf(sbuf, "%d%%\n%s\n%s", (inst->globvol * 400) / 256, inst->vibdepth ? "Yes" : "No", inst->volfade ? "Yes" : "No");
                        SetWindowText(GetDlgItem(hwndDlg, IDC_INSTHEAD), sbuf);
                        //(inst->nnatype == NNA_CONTINUE) ? "Continue" : (inst->nnatype == NNA_OFF) ? "Off" : (inst->nnatype == NNA_FADE) ? "Fade" : "Cut");

                        // --------------------
                        // Part 2: The instrument envelope info (vol/pan/pitch)

                        // Wow this is ugly, but it works:  Make a set of strings that have the
                        // '(loop / sustain)' string.  Tricky, cuz the '/' is only added if it
                        // is needed of course.

                        if(inst->volflg & (EF_LOOP | EF_SUSTAIN))
						{	sprintf(st1,"(%s%s%s)",(inst->volflg & EF_LOOP) ? "Loop" : "", ((inst->volflg & EF_LOOP) && (inst->volflg & EF_SUSTAIN)) ? " / " : "",
												   (inst->volflg & EF_SUSTAIN) ? "Sustain" : "");
						} else st1[0] = 0;

						if(inst->panflg & (EF_LOOP | EF_SUSTAIN))
						{	sprintf(st2,"(%s%s%s)",(inst->panflg & EF_LOOP) ? "Loop" : "", ((inst->panflg & EF_LOOP) && (inst->panflg & EF_SUSTAIN)) ? " / " : "",
												   (inst->panflg & EF_SUSTAIN) ? "Sustain" : "");
						} else st2[0] = 0;

						if(inst->pitflg & (EF_LOOP | EF_SUSTAIN))
						{	sprintf(st3,"(%s%s%s)",(inst->pitflg & EF_LOOP) ? "Loop" : "", ((inst->pitflg & EF_LOOP) && (inst->pitflg & EF_SUSTAIN)) ? " / " : "",
												   (inst->pitflg & EF_SUSTAIN) ? "Sustain" : "");
						} else st3[0] = 0;
						
						sprintf(sbuf, "%s %s\n%s %s\n%s %s",
							(inst->volflg & EF_ON) ? "On" : "Off", st1[0] ? st1 : "", 
							(inst->panflg & EF_ON) ? "On" : "Off", st2[0] ? st2 : "", 
							(inst->pitflg & EF_ON) ? "On" : "Off", st3[0] ? st3 : "");

                        SetWindowText(GetDlgItem(hwndDlg, IDC_INSTENV), sbuf);

						// --------------------
                        // Part 3: List of samples used by this instrument!
                        // the trick here is that that we have to figure out what samples are used from the
                        // sample index table in inst->samplenumber.

					    memset(pHdr->suse,0,m->numsmp*sizeof(BOOL));
                        for(x=0; x<120; x++)
                            if(inst->samplenumber[x] != 255)
                                pHdr->suse[inst->samplenumber[x]] = 1;

                        sbuf[0] = 0;  cnt = 0;
                        for (x=0; x<m->numsmp; x++)
					    {   if(pHdr->suse[x])
                                cnt += sprintf(&sbuf[cnt], "%02d: %s\r\n",x+1, m->samples[x].samplename);
                            
	    				}
                        sbuf[cnt - 2] = 0;  // cut off the final CR/LF set
                        SetWindowText(GetDlgItem(hwndDlg, TB_SAMPLELIST), sbuf);

					}
					break;

					case IDC_SAMPLIST:
					{	UNISAMPLE *samp = &m->samples[moo];
                        EXTSAMPLE *es = NULL;

                        if(m->extsamples) es = &m->extsamples[moo];

						// Display sampe header info...
                        // Length, Quality, Looping, Auto-vibrato (in that order).

                        sprintf(sbuf, "%u bytes\n%u bits\n%s\n%s",
                            samp->length, samp->bitdepth * 8,
                            (samp->flags & SL_LOOP) ? ((samp->flags & SL_BIDI) ? "Ping-Pong" : "Forward") : "None",
                            (es && es->vibdepth) ? "Yes" : "No");

                        SetWindowText(GetDlgItem(hwndDlg, IDC_SAMPINFO), sbuf);
					}
					break;
				}
			}
		break;

        /*case WM_CHAR:
        case WM_KEYDOWN:
            // Select the comment tab if the user hits shift-F9
            if(GetKeyState(VK_SHIFT)) if(GetKeyState(VK_F9))
            {   TabCtrl_SetCurSel(GetDlgItem(hwndDlg,IDC_TAB), TabCtrl_GetItemCount(GetDlgItem(hwndDlg,IDC_TAB)));
                OnSelChanged(hwndDlg);
            }
        break;*/
    }
	return 0;
}


// =====================================================================================
    static void OnSelChanged(HWND hwndDlg) 
// =====================================================================================
{ 
    DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA); 
    int iSel = TabCtrl_GetCurSel(pHdr->hwndTab); 

	if(pHdr->hwndDisplay)  ShowWindow(pHdr->hwndDisplay,SW_HIDE);
	ShowWindow(pHdr->apRes[iSel],SW_SHOW);
	pHdr->hwndDisplay = pHdr->apRes[iSel];

	// Note to self: Despite their inhernet use in interfaces, coding tab controls
	// apparently REALLY sucks, and it should never ever be done again by myself
	// or anyone else whom I respect as a sane individual and I would like to have
	// remain that way.  As for me, it is too late.  Bruhahahaha!K!J!lkjgkljASBfkJBdglkn.

} 

extern MPLAYER *get_player(UNIMOD *othermf);
extern void     infobox_delete(HWND hwnd);

// =====================================================================================
    static void CALLBACK UpdateInfoRight(HWND hwnd, UINT uMsg, UINT ident, DWORD systime)
// =====================================================================================
{
    char        str[256];
    DLGHDR     *pHdr = (DLGHDR *)GetWindowLong(hwnd, GWL_USERDATA);
	MPLAYER    *mp;
    int         acv;           // current active voices.
    
    // Player info update .. BPM, sngspeed, position, row, voices.
    // Only update if our mf struct is the same as the one currently loaded into the player.

    if((mp = get_player(pHdr->module)) == NULL)
    {
        UNIMOD *m = pHdr->module;

        if(pHdr->seeker)
        {   wsprintf(str,"%d\n%d\n0 of %d\n\nNot Playing...", m->inittempo, m->initspeed, m->numpos);
            SetWindowText(GetDlgItem(hwnd,IDC_INFORIGHT),str);

            pHdr->seeker->statelist   = NULL;
            pHdr->seeker->statecount  = 0;

            Player_Free(pHdr->seeker);
            pHdr->seeker = NULL;
        }
    } else
    {
        long  curtime = mikamp.GetOutputTime() * 64;

        if(!pHdr->seeker)
        {   // Create our new player instance specifically for seeking.
            // Manually Copy over important stuff from the real mp
            
            pHdr->seeker = Player_Create(pHdr->module, PF_TIMESEEK);
            Player_SetLoopStatus(pHdr->seeker, config_playflag & CPLAYFLG_LOOPALL, config_loopcount);
            pHdr->seeker->statelist   = mp->statelist;
            pHdr->seeker->statecount  = mp->statecount;
            pHdr->seeker->songlen     = mp->songlen;
        }

        // Seek to our new song time, using a bastardized version of Player_SetPosTime code:

        if(pHdr->seeker->statelist)
        {   int   t = 0;
            while((t < pHdr->seeker->statecount) && pHdr->seeker->statelist[t].curtime && (curtime > pHdr->seeker->statelist[t].curtime)) t++;
            if(t) Player_Restore(pHdr->seeker, t-1); else Player_Cleaner(pHdr->seeker);
        } else Player_Cleaner(pHdr->seeker);

        while(!pHdr->seeker->ended && (pHdr->seeker->state.curtime < curtime)) Player_PreProcessRow(pHdr->seeker, NULL);

        // Display all the goodie info we have collected:
        // ---------------------------------------------

        acv = Mikamp_GetActiveVoices(mp->vs->md);
        if(acv > pHdr->maxv) pHdr->maxv = acv;

        wsprintf(str,"%d\n%d\n%d of %d\n%d of %d\n%d (%d)",pHdr->seeker->state.bpm, pHdr->seeker->state.sngspd, pHdr->seeker->state.sngpos, pHdr->seeker->mf->numpos, pHdr->seeker->state.patpos, pHdr->seeker->state.numrow, acv, pHdr->maxv);
        SetWindowText(GetDlgItem(hwnd,IDC_INFORIGHT),str);
    }
    
}

// =====================================================================================
    HWND infoDlg(HWND hwnd, DLGHDR *pHdr)
// =====================================================================================
{
    HWND   dialog;
    char   str[256];
    UNIMOD *m;

    if(!pHdr) return NULL;
    
    dialog = CreateDialog(mikamp.hDllInstance,MAKEINTRESOURCE(IDD_ID3EDIT),hwnd,infoProc);
    pHdr->hwndTab = GetDlgItem(dialog,IDC_TAB);

    SetWindowLong(dialog, GWL_USERDATA, (LONG) pHdr); 
    SetWindowText(GetDlgItem(dialog,IDC_ID3_FN),pHdr->module->filename);

    m = pHdr->module;

	// Set IDC_INFOLEFT - contains static module information:
    //   File Size, Length (in mins), channels, samples, instruments.

    wsprintf(str,"%d bytes\n%d:%02d minutes\n%d\n%d\n%d",
	m->filesize, m->songlen/60000,(m->songlen%60000)/1000, m->numchn, m->numsmp, m->numins);
    SetWindowText(GetDlgItem(dialog,IDC_INFOLEFT),str);

    SetWindowText(GetDlgItem(dialog,IDC_TITLE),m->songname);
    SetWindowText(GetDlgItem(dialog,IDC_TYPE), m->modtype);

    // pHdr->suse is a samples-used block, allocated if this module uses
    // instruments, and used to display the sampels that each inst uses

    if(m->numins) pHdr->suse = (BOOL *)_mm_calloc(NULL, m->numsmp, sizeof(BOOL));

    infoTabInit(dialog, m, pHdr);

    // IDC_INFORIGHT will be updated via a timer message 20 times a second.

    SetTimer(dialog, 1, 50, UpdateInfoRight);
    
    ShowWindow(dialog, SW_SHOW);

    return dialog;
}

// =====================================================================================
    void info_killseeker(HWND hwnd, UNIMOD *curmf)
// =====================================================================================
{
    DLGHDR   *pHdr = (DLGHDR *)GetWindowLong(hwnd, GWL_USERDATA);
    
    if(pHdr->seeker)
    {   if(curmf == pHdr->module)
        {   pHdr->seeker->statelist   = NULL;
            pHdr->seeker->statecount  = 0;
        }
        Player_Free(pHdr->seeker);
        pHdr->seeker = NULL;
    }
}


// =====================================================================================
    static BOOL CALLBACK infoProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
// =====================================================================================
{
    UNIMOD   *m = NULL;
    DLGHDR   *pHdr;

    switch (uMsg)
    {   
        case WM_INITDIALOG:
        {
        }
        return TRUE;

        /*case WM_CHAR:
        case WM_KEYDOWN:
            // Select the comment tab if the user hits shift-F9
            if(GetKeyState(VK_SHIFT)) if(GetKeyState(VK_F9))
            {   TabCtrl_SetCurSel(GetDlgItem(hwndDlg,IDC_TAB), TabCtrl_GetItemCount(GetDlgItem(hwndDlg,IDC_TAB)));
                OnSelChanged(hwndDlg);
            }
        break;*/

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {   case IDOK:
                    infobox_delete(hwndDlg);
                return 0;
            }
        break;

        case WM_CLOSE:
            infobox_delete(hwndDlg);
        break;

        case WM_DESTROY:
            pHdr = (DLGHDR *)GetWindowLong(hwndDlg, GWL_USERDATA);
            KillTimer(hwndDlg, 1);  // Kill the player info box updates
            if(pHdr)
            {
                _mm_free(NULL, pHdr->suse);
                LocalFree(pHdr);
            }
        break;

		case WM_NOTIFY:
		{   NMHDR *notice = (NMHDR *) lParam;
			switch(notice->code)
            {   case TCN_SELCHANGE:
                    OnSelChanged(hwndDlg);
			    break;
			}
		}
	    return TRUE;
	}
	return 0;
}
