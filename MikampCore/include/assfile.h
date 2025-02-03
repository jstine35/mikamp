#ifndef _MM_ASSFILE_H
#define _MM_ASSFILE_H

#include "mmio.h"
#include <io.h>

#define ASSFIND_FILES         (1<<0)
#define ASSFIND_FOLDERS       (1<<1)


// -------------------------------------------------------------------------------------
// Descriptor for a list of files found by wildcard search.  The path of the file is not
// included in the filename unless the file was found using one of the nested wildcard
// searches.
//
typedef struct ASSFILE
{
    CHAR        **file;              // list of non-wildcard logfile names
    int           count,             // how many?
                  alloc;             // alloc size of logfile array (uses realloc system)
} ASSFILE;

// -------------------------------------------------------------------------------------
//
typedef struct ASSFILE_API
{
    BOOL    (*filetest)   (const CHAR *fullpath, const struct _finddata_t *fingo);
    void   *(*newfolder)  (void *usrhandle, const CHAR *folder);
    BOOL    (*newfile)    (void *usrhandle, const CHAR *folder, const struct _finddata_t *fingo);
    void    (*closefolder)(void *usrhandle, const CHAR *folder, BOOL errhappen);

} ASSFILE_API;


/*
// -------------------------------------------------------------------------------------
//
typedef struct ASSINFO
{
    CHAR         *name;              // filename!  name only.  Path is stored in assfile_ex
    ulong         time;              // time of file modification (touched)

} ASSINFO;

// -------------------------------------------------------------------------------------
//
typedef struct ASSFILE_EX
{
    MM_ALLOC      allochandle;       // nestable linked list!

    CHAR         *path;              // pathname for this assfile block (doesn't contain parental portions)
    
    ASSINFO      *info;              // list of non-wildcard logfile names
    int           count,             // how many?
                  alloc;             // alloc size of info array (uses realloc system)
} ASSFILE_EX;
*/

extern BOOL         wildcard_asswipe(MM_ALLOC *allochandle, ASSFILE *loglist, const CHAR *path, const CHAR *mask, uint flags);
extern BOOL         _mm_wildcard_nested(void *handle, const ASSFILE_API *api, const CHAR *path);
extern void         assfile_alloc(MM_ALLOC *allochandle, ASSFILE *loglist);

#endif  // _MM_ASSFILE_H
