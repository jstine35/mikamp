/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 mmcopy.c
 
*/

#include "mmio.h"
#include <string.h>

// _____________________________________________________________________________________
// Same as Watcom's strdup function - allocates memory for a string and makes a copy of
// it.  Returns NULL if failed.
//
// TODO : Obsolete.  Should be removed and replaced with C++ string class use.
//
CHAR *_mm_strdup(MM_ALLOC *allochandle, const CHAR *src)
{
    CHAR *buf;

    if(!src) return NULL;
    if((buf = (CHAR *)_mm_malloc(allochandle, (strlen(src)+1) * sizeof(CHAR))) == NULL) return NULL;

    strcpy(buf,src);
    return buf;
}

// _____________________________________________________________________________________
// Same as Watcom's strdup function - allocates memory for a string and makes a copy of
// it-- limiting the destination string length to 'length'.  Returns NULL if failed.
//
// TODO : Obsolete.  Should be removed and replaced with C++ string class use.
//
CHAR *_mm_strndup(MM_ALLOC *allochandle, const CHAR *src, uint length)
{
    CHAR   *buf;
    uint    stl;

    if(!src || !length) return NULL;

    stl = strlen(src);
    if(stl < length) length = stl;
    if((buf = (CHAR *)_mm_malloc(allochandle, length+1)) == NULL) return NULL;

    memcpy(buf, src, sizeof(CHAR) * length);
    buf[length] = 0;

    return buf;
}

// _____________________________________________________________________________________
// copies the source string onto the dest, ensuring the dest never goes above maxlen
// characters!  This is effectively a more useful version of strncpy.  Use it to
// prevent buffer overflows and such.
//
CHAR *_mm_strcpy(CHAR *dest, const CHAR *src, const uint maxlen)
{
    if(dest && src)
    {
        strncpy(dest, src, maxlen-1);
        dest[maxlen-1] = 0;                   // strncpy not guaranteed to do this
    }

    return dest;
}

// _____________________________________________________________________________________
// concatenates the source string onto the dest, ensuring the dest never goes above
// maxlen characters!  This is effectively a more useful version of strncat.  USe it to
// prevent buffer overflows and such.
//
CHAR *_mm_strcat(CHAR *dest, const CHAR *src, const uint maxlen)
{
    strncat(dest, src, maxlen);
    dest[maxlen-1] = 0;                   // strncat not guaranteed to do this

    return dest;
}

// _____________________________________________________________________________________
// Compares the names of two filesnames and gives the alphabetical relationship.
// In the world of files, extensions are effectively ignored during the initial sorting
// process and are then compared by themselves to determine rank if the pre-extension
// portions are identical.
//
// If str1 is greater than str2, a positive number is returned.  If str2 is greater than
// str1 then a negative number is rturned.  When both strings are identical, 0 is
// returned.
//
int _mmstr_filecmp(const CHAR *str1, const CHAR *str2)
{
    CHAR temp01[_MAX_PATH*2];
    CHAR temp02[_MAX_PATH*2];

    CHAR *tptr01 = temp01;
    CHAR *tptr02 = temp02;

    int  extpick = 0;

    CHAR numsup01[64];
    CHAR numsup02[64];
    int  nidx1 = 0, nidx2 = 0;

    if(!str1 && !str2) return 0;
    if(!str1) return -1;
    if(!str2) return 1;

    // Step One: Strip extensions
    
    _mm_strcpy(temp01, str1, _MAX_PATH*2);
    _mm_strcpy(temp02, str2, _MAX_PATH*2);

    _mmstr_newext(temp01, NULL);
    _mmstr_newext(temp02, NULL);

    // Heuristics!  While processing we strip out all numbers.
    // If the text without the numbers is identical, then order
    // the files based on the numbers!

    // Step Two: Compare non-extension versions

    if(temp01[0] || temp02[0])
    {
        CHAR c1, c2;

        if(!temp01[0]) return -1;
        if(!temp02[0]) return 1;

        while( TRUE )
        {
            c1 = *tptr01;
            c2 = *tptr02;

            if(_mmchr_numeric(c1))
            {
                numsup01[nidx1] = c1;
                nidx1++;
                tptr01++;
                continue;
            }
            if(_mmchr_numeric(c2))
            {
                numsup02[nidx2] = c2;
                nidx2++;
                tptr02++;
                continue;
            }
            if(!c1 && !c2) break;
            if(c1 != c2)   return (c1 < c2) ? -1 : 1;

            tptr01++;
            tptr02++;
        }
    }
    
    // Step Three: Equal-length so compare extensions!

    while(TRUE)
    {
        CHAR c1, c2;

        c1 = *tptr01;
        c2 = *tptr02;

        if(!c1 && !c2) break;
        if(c1 != c2)   return (c1 < c2) ? -1 : 1;
        tptr01++;
        tptr02++;
    }

    numsup01[nidx1] = 0;
    numsup02[nidx2] = 0;

    {
        int tmp1 = atoi( numsup01 ), 
            tmp2 = atoi( numsup02 );

        return ( tmp1 == tmp2 ) ? 0 : (( tmp1 < tmp2 ) ? -1 : 1 );
    }
}

int _mm_strcmp(const CHAR *str1, const CHAR *str2)
{
    CHAR c1, c2;
    if(!str1 && !str2) return 0;
    if(!str1) return -1;
    if(!str2) return 1;
    
    while(TRUE)
    {
        c1 = *str1;
        c2 = *str2;

        if(!c1 && !c2) return 0;
        if(c1 != c2)   return (c1 < c2) ? -1 : 1;

        str1++;
        str2++;
    }
    return 0;
}

// _____________________________________________________________________________________
// Makes a duplicate of the specified struct.  It is the user's responsibility to ensure
// that the 'size' parameter is at least the same size as the original struc size (usually
// through the use of the sizeof() operator).  The parameter can be larger than the or-
// iginal however.
// NOTE : This function should never be used on mmalloc-based objects, since the mmalloc
//        object data would not be properly initialized.
//
void *_mm_structdup(MM_ALLOC *allochandle, const void *src, const size_t size)
{
    void *tribaldance;
    
    if(!src) return NULL;

    tribaldance = _mm_malloc(allochandle, size);
    if(tribaldance) memcpy(tribaldance, src, size);

    return tribaldance;
}

// _____________________________________________________________________________________
// Makes a duplicate of an mmalloc object.  This is similar to duplicating a struct except
// it avoids overwriting critical mmalloc data at the head of the structure. This function
// should always be used when make duplicates of objects based on mmalloc.
// size     - size of the object, must be at least sizeof(MMALLOC) bytes or else an
//            assertion failure will occur.
//
void *_mm_objdup(MM_ALLOC *parent, const void *src, const uint blksize)
{
    void  *dest = _mmalloc_create_ex("Sumkinda Duplicate", parent, blksize);
   
    {
        UBYTE        *stupid = (UBYTE *)dest;
        const UBYTE  *does   = (UBYTE *)src;

        memcpy(&stupid[sizeof(MM_ALLOC)], &does[sizeof(MM_ALLOC)], blksize - sizeof(MM_ALLOC));
    }
    return dest;
}

// _____________________________________________________________________________________
// inserts the specified source character into the destination string at position.  It
// is the responsibility of the caller to ensure the string has enough room to contain
// the resultant string!
//
void _mm_insertchr(CHAR *dest, const CHAR src, const uint pos)
{
    if(dest)
    {   const uint dlen = strlen(dest);
        if(pos >= dlen)
        {
            // Append Mode
            // -----------
            // Just attach the character to the end of the string.

            dest[dlen]   = src;
            dest[dlen+1] = 0;
        } else
        {
            // Insert Mode
            // -----------
            // copy contents of string, in reverse, so we don't overwrite our
            // source data!

            uint  i;
            for(i=dlen+1; i>pos; i--)
                dest[i] = dest[i-1];

            dest[pos] = src;
        }
    }
}

// _____________________________________________________________________________________
// Deletes a character from a string, from the given position.  Moves all characters
// following it inward, to ensure no space is left behind.
//
void _mm_deletechr(CHAR *dest, uint pos)
{
    if(dest)
    {   const uint dlen = strlen(dest);
        if(pos < dlen)
            memcpy(&dest[pos], &dest[pos+1], strlen(&dest[pos+1])+1);
    }
}

// _____________________________________________________________________________________
// inserts the specified source string into the destination string at position.  It
// is the responsibility of the caller to ensure the string has enough room to contain
// the resultant string!
//
void _mm_insertstr(CHAR *dest, const CHAR *src, const uint pos)
{
    if(dest)
    {   const uint dlen = strlen(dest);
        if(pos >= dlen)
        {
            // Append Mode
            // -----------
            // Just attach the source to the end of the string.

            strcat(dest, src);
        } else
        {
            // Insert Mode
            // -----------
            // copy contents of string, in reverse, so we don't overwrite our
            // source data!

            uint  i, srclen = strlen(src);
            for(i=dlen; i>pos; i--)
                dest[i] = dest[i-srclen];

            memcpy(&dest[pos], src, sizeof(CHAR) * srclen);
        }
    }
}

// _____________________________________________________________________________________
// Breaks a fullpath into the path and filename parts.
//
void _mm_splitpath(const CHAR *fullpath, CHAR *path, CHAR *fname)
{
    if(fullpath)
    {
        int    i;
        const int  flen = strlen(fullpath);

        if(path) strcpy(path, fullpath);

        for(i=flen-1; i>=0; i--)
            if(fullpath[i] == '\\') break;

        if(fname) strcpy(fname, &fullpath[i+1]);
        if(path) path[i+1] = 0;
    }
}

const static CHAR  invalidchars[] = "'|()*%?/\\\"";

// _____________________________________________________________________________________
// Takes the given string and converts it into a valid filename by stripping out any and
// all invalid characters and replacing them with underscores.
//
CHAR *_mmstr_filename(CHAR *src)
{
    if(src)
    {
        uint    idx = 0;
        while(src[idx])
        {
            const CHAR  *ic  = invalidchars;
            while(*ic)
            {   if(src[idx] == *ic)
                {   src[idx] = '_';
                    break;
                }
            }
            idx++;
        }
    }
    return src;
}

// _____________________________________________________________________________________
// Concatenates the source path onto the dest, ensuring the dest never goes above
// _MAX_PATH!  If the source path has root characters, then it overwrites the dest
// (proper root behavior)
//
CHAR *_mmstr_pathcat(CHAR *dest, const CHAR *src)
{
    if(!dest) return NULL;
    if(!src)  return dest;

    // Check for Colon / Backslash
    // ---------------------------
    // A colon, in DOS terms, designates a drive specification, which means we should
    // ignore the root path.  Same goes for a backslash as the first character in the
    // path!

    if((strcspn(src, ":") < strlen(src)) || (src[0] == '\\'))
        strcpy(dest, src);
    else
    {
        if(dest[0])
        {
            // Remove Trailing Backslashes on dest
            // -----------------------------------
            // We'll add our own in later and they mess up some of the logic.

            uint    t   = 0;
            uint    len = strlen(dest);
            len--;
            while(len && (dest[len] == '\\')) len--;

            dest[len+1] = 0;

            // Check for Periods in src
            // ------------------------
            // Periods encapsulated by '\' represent folder nesting: '.' is current (ignored),
            // '..' is previous, etc.

            while(src[t] && (src[t] == '.')) t++;
            if((src[t] == '\\') || (src[t] == 0))
                src  += t;
            else
                t     = 0;

            if(t)
            {
                t--;
                while(t && len)
                {
                    if(dest[len] == '\\') t--;
                    len--;
                }
                dest[len+1] = 0;
            }
        }

        if(dest[0] != 0)
            _mm_strcat(dest, "\\", _MAX_PATH);
        _mm_strcat(dest, src, _MAX_PATH);
    }
    return dest;
}

// _____________________________________________________________________________________
// Attaches a new extension to the specified string, removing the old one if it existed.
// if ext is NULL, then the extension and the period is removed.  if it is an empty
// string (""), the period is pasted but nothign after it.
//
CHAR *_mmstr_newext( CHAR *path, const CHAR *ext )
{
    uint    i;
    uint    slen = strlen( path );

    if( !path ) return NULL;

    for( i=slen-1; i; i-- )
        if( path[i] == '.' ) break;

    if( i == 0 )
    {
        if( ext )
        {
            strcat( path, "." );
            strcat( path, ext );
        }
    }
    else
    {
        i++;
        path[i] = 0;
        if( ext )
            strcpy( &path[i], ext );
        else
            path[i-1] = 0;
    }

    return path;
}
