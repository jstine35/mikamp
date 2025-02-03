/*
 
 Mikamp Portable System Management Facilities (the MMIO)

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 --------------------------------------------------
 wildcard.c

 Generic (portable) findfirst / findnext based wildcard functions.  Then again, maybe
 I don't really give a rat's droppings if this is portable or not!

*/

#include "assfile.h"

#include <string.h>
#include <io.h>
#include <errno.h>

// _____________________________________________________________________________________
// Allocates new assfiles in the given assfile struct.  If the count is greater or equal
// to alloc, then new assfiles are made room for.  Assfiles are fun little pets.  You
// should stop down at the local ass shop and pick a couple up for yourself.
//
void __inline assfile_alloc(MM_ALLOC *allochandle, ASSFILE *loglist)
{
    if(loglist->count >= loglist->alloc)
    {
        loglist->file   = (CHAR **)_mm_recalloc(allochandle, loglist->file, loglist->alloc+256, sizeof(CHAR **));
        loglist->alloc += 256;
    }
}

// _____________________________________________________________________________________
// Search for files based on standard DOS wildcard patterns.
// Files are added to the assfile list, in sorted order.
// Returns TRUE if the wildcard produced no matches!
//
// flags   - see ASSFIND_FILES and ASSFIND_FOLDERS flags
//
BOOL wildcard_asswipe(MM_ALLOC *allochandle, ASSFILE *loglist, const CHAR *path, const CHAR *mask, uint flags)
{
    int                handie;
    struct _finddata_t fingo;
    CHAR               stmp[_MAX_PATH], pfrac[_MAX_PATH];

    _mm_splitpath(mask, pfrac, NULL);
    strcpy(stmp, path); _mmstr_pathcat(stmp, mask);
    handie = _findfirst(stmp, &fingo);

    if((handie == -1) && (errno == ENOENT))
    {
        _mmlogv("Warning! > File(s) not found : %s", stmp);
        return TRUE;
    }

    do
    {
        int    i = 0;
        uint   fgolen;

        if(!strcmp(fingo.name, ".") || !strcmp(fingo.name, ".."))     continue;
        if((fingo.attrib & _A_SUBDIR)  && !(flags & ASSFIND_FOLDERS)) continue;
        if(!(fingo.attrib & _A_SUBDIR) && !(flags & ASSFIND_FILES))   continue;

        assfile_alloc(allochandle, loglist);
        strcpy(stmp, pfrac);
        _mmstr_pathcat(stmp, fingo.name);
        fgolen = strlen(stmp);

        while((i < loglist->count) && (_mmstr_filecmp(stmp, loglist->file[i]) > 0))
            i++;

        if(i != loglist->count)
        {
            int  t = loglist->count-1;
            for(; t>=i; t--)
                loglist->file[t+1] = loglist->file[t];
        }

        loglist->file[i] = _mm_strdup(allochandle, stmp);
        loglist->count++;

    } while(_findnext(handie, &fingo) != -1);

    _findclose(handie);

    return FALSE;
}


/*
typedef struct ASS_CALLBACK
{
    void    (*forfile)
    void    (
} ASS_CALLBACK;
*/

// _____________________________________________________________________________________
// Search for files based on standard DOS wildcard patterns.  Files are added to the
// assfile_ex list, in sorted order.
// Returns a handle to an ASSFILE_EX tree
//
BOOL _mm_wildcard_nested(void *handle, const ASSFILE_API *api, const CHAR *path)
{
    uint               handie;
    struct _finddata_t fingo;
    CHAR               stmp[_MAX_PATH];
    void              *assfile = NULL;
    BOOL               errhappen = FALSE;

    strcpy(stmp, path); _mmstr_pathcat(stmp, "*");

    handie = _findfirst(stmp, &fingo);

    if((handie == -1) && (errno == ENOENT))
    {
        //assert(FALSE);
        //_mmlogv("Warning! > File(s) not found : %s", stmp);
        return FALSE;
    }

    if(api->newfolder)
        assfile = api->newfolder(handle, path);
    else
        assfile = handle;

    do
    {
        if(!strcmp(fingo.name, ".") || !strcmp(fingo.name, "..")) continue;

        if(fingo.attrib & _A_SUBDIR)
        {
            // Nest into this subfolder...
            // ---------------------------
            // The magic of recursion makes this endeavor shorter than this comment!

            if(api->newfolder)
            {
                CHAR        fullpath[_MAX_PATH];
                strcpy(fullpath, path);
                _mmstr_pathcat(fullpath, fingo.name);
                if(!_mm_wildcard_nested(assfile, api, fullpath))
                {
                    errhappen = TRUE;
                    break;
                }
            }
        } else
        {
            if(api->filetest)
            {
                CHAR    fullpath[_MAX_PATH];
                strcpy(fullpath, path);
                _mmstr_pathcat(fullpath, fingo.name);
                if(!api->filetest(fullpath, &fingo)) continue;
            }

            if(!api->newfile(assfile, path, &fingo))
            {
                errhappen = TRUE;
                break;
            }
        }
    } while(_findnext(handie, &fingo) != -1);

    if(api->closefolder) api->closefolder(assfile, path, errhappen);
    _findclose(handie);

    return TRUE;
}
