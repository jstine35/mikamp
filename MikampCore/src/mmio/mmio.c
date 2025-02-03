/*
 Mikamp Sound System
 
 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution 
 ------------------------------------------
 mmio.c

  Miscellaneous portable I/O routines.. used to solve some portability
  issues (like big/little endian machines and word alignment in structures).

  Notes:
   Expanded to allow for streaming from a memory block as well as from
   a file.  Also, set up in a manner which allows easy use of packfiles
   without having to overload any functions (faster and easier!).

 -----------------------------------
 The way this module works - By Jake Stine (air)

 - _mm_read_I_UWORD and _mm_read_M_UWORD have distinct differences:
   the first is for reading data written by a little endian (intel) machine,
   and the second is for reading big endian (Mac, RISC, Alpha) machine data.

 - _mm_write functions work the same as the _mm_read functions.

 - _mm_read_string is for reading binary strings.  It is basically the same
   as an fread of bytes.
*/                                                                                     

#include "mmio.h"
#include "mminline.h"
#include <string.h>

#ifndef COPY_BUFSIZE
#define COPY_BUFSIZE  1024
#endif

static UBYTE  _mm_cpybuf[COPY_BUFSIZE];


// _____________________________________________________________________________________
//
    static void __inline mfp_init(MMSTREAM *mfp)
{
}

// _____________________________________________________________________________________
// Creates an MMSTREAM structure from the given file pointer and seekposition
//
MMSTREAM *_mmstream_createfp(FILE *fp, int iobase)
{
    MMSTREAM     *mmf;

    mmf = (MMSTREAM *)_mm_malloc(NULL, sizeof(MMSTREAM));

    mmf->fp      = fp;
    mmf->iobase  = iobase;
    mmf->dp      = NULL;
    mmf->seekpos = 0;

    mfp_init(mmf);

    return mmf;
}

// _____________________________________________________________________________________
// Creates an MMSTREAM structure from the given memory pointer and seekposition
//
    MMSTREAM *_mmstream_createmem(void *data, int iobase)
{
    MMSTREAM *mmf;

    mmf = (MMSTREAM *)_mm_malloc(NULL, sizeof(MMSTREAM));

    mmf->dp        = (UBYTE *)data;
    mmf->iobase    = iobase;
    mmf->fp        = NULL;
    mmf->seekpos   = 0;

    mfp_init(mmf);

    return mmf;
}

// _____________________________________________________________________________________
//
void _mmstream_delete(MMSTREAM *mmf)
{
    _mm_free(NULL, mmf);
}

// _____________________________________________________________________________________
// Specialized file output procedure.  Writes a UWORD length and then a
// string of the specified length (no NULL terminator) afterward.
//
void StringWrite(const CHAR *s, MMSTREAM *fp)
{
    int slen;

    if(s==NULL)
    {   _mm_write_I_UWORD(0,fp);
    } else
    {   _mm_write_I_UWORD(slen = strlen(s),fp);
        _mm_write_UBYTES((UBYTE *)s,slen,fp);
    }
}

// _____________________________________________________________________________________
// Reads strings written out by StringWrite above:  a UWORD length followed by length 
// characters.  A NULL is added to the string after loading.
//
    CHAR *StringRead(MMSTREAM *fp)
{
    CHAR  *s;
    UWORD len;

    len = _mm_read_I_UWORD(fp);
    if(len==0)
    {   s = _mm_calloc(NULL, 16, sizeof(CHAR));
    } else
    {   if((s = (CHAR *)_mm_malloc(NULL, len+1)) == NULL) return NULL;
        _mm_read_UBYTES((UBYTE *)s,len,fp);
        s[len] = 0;
    }

    return s;
}

// _____________________________________________________________________________________
//
MMSTREAM *_mm_tmpfile(void)
{
    MMSTREAM *mfp;

    mfp           = _mmobj_allocblock(NULL, MMSTREAM);
    mfp->fp       = tmpfile();
    mfp->iobase   = 0;
    mfp->dp       = NULL;

    mfp_init(mfp);

    return mfp;
}

// _____________________________________________________________________________________
//
MMSTREAM *_mm_fopen(const CHAR *fname, const CHAR *attrib)
{
    MMSTREAM *mfp = NULL;

    if(fname && attrib)
    {
        FILE     *fp;
        if((fp=fopen(fname, attrib)) == NULL)
        {
            //CHAR   sbuf[_MAX_PATH*2];
            //sprintf(sbuf, "Failed opening file: %s\nSystem message: %s\n"
            //              "(Please ensure the file exists and that you have read/write permissions and access to the file, and then try again)",
            //              fname, _sys_errlist[errno]);

            //_mmerr_set(MMERR_OPENING_FILE, "Error opening file!", sbuf);
            _mmlogd2("Error opening file: %s > %s",fname, _sys_errlist[errno]);
            return NULL;
        }

        mfp           = _mmobj_allocblock( NULL, MMSTREAM );
        mfp->fp       = fp;
        mfp->iobase   = 0;
        mfp->dp       = NULL;

        mfp_init(mfp);
    }

    return mfp;
}

// _____________________________________________________________________________________
//
void _mm_fclose(MMSTREAM *mmfile)
{
    if(mmfile)
    {   if(mmfile->fp) fclose(mmfile->fp);
        _mm_free(NULL, mmfile);
    }
}

#define _my_fseek  fseek
#define _my_ftell  ftell
#define _my_feof   feof
#define _my_fread  fread

// _____________________________________________________________________________________
//
int _mm_fseek(MMSTREAM *stream, long offset, int whence)
{
    if(!stream) return 0;

    if(stream->fp)
    {   // file mode...
        
        return _my_fseek(stream->fp,(whence==SEEK_SET) ? offset+stream->iobase : offset, whence);
    } else
    {   long   tpos = -1;
        switch(whence)
        {   case SEEK_SET: tpos = offset;                   break;
            case SEEK_CUR: tpos = stream->seekpos + offset; break;
            case SEEK_END: /*tpos = stream->length + offset;*/  break; // not supported!
        }
        if((tpos < 0) /*|| (stream->length && (tpos > stream->length))*/) return 1; // seek failed
        stream->seekpos = tpos;
    }

    return 0;
}

// _____________________________________________________________________________________
//
long _mm_ftell(MMSTREAM *stream)
{
   if(!stream) return 0;
   return (stream->fp) ? (_my_ftell(stream->fp) - stream->iobase) : stream->seekpos;
}

// _____________________________________________________________________________________
//
BOOL __inline _mm_feof(MMSTREAM *stream)
{
    if(!stream) return 1;
    if(stream->fp) return _my_feof(stream->fp);

    return 0;
}

// _____________________________________________________________________________________
//
BOOL _mm_fexist(CHAR *fname)
{
   FILE *fp;
   
   if((fp=fopen(fname,"r")) == NULL) return 0;
   fclose(fp);

   return 1;
}

// _____________________________________________________________________________________
//
long _mm_flength(MMSTREAM *stream)
{
   long   tmp, tmp2;

   tmp = _mm_ftell(stream);

   _mm_fseek(stream,0,SEEK_END);
   tmp2 = _mm_ftell(stream);

   _mm_fseek(stream,tmp,SEEK_SET);

   return tmp2;
}

#include <direct.h>

// _____________________________________________________________________________________
// Makes the specified path, in its entirety.  That is, it breaks out each part of the
// path and ensures that part exists before moving on to the next part, forming the
// entire path from start to finish.
//
// Returns TRUE on success, or FALSE if the entire path already existed.
//      (path may exist as a file)
//
BOOL _mm_mkdir(const CHAR *path)
{
    CHAR    work[_MAX_PATH * 4];
    BOOL    finished = FALSE;
    int     result, i=0;

    if(!path) return TRUE;

    assert(strlen(path) < (_MAX_PATH * 4));

    while(path[i])
    {
        if((i > 0) && path[i] == '\\')      // first conditional skips leading backslash (if present)
        {
            strncpy(work, path, i);
            work[i] = 0;
            result = _mkdir(work);
            do { i++; } while(path[i] && (path[i] == '\\'));
            if(!path[i]) finished = TRUE;
        }
        i++;
    }
    if(!finished) result = _mkdir(path);

    return (result == 0);
}

// _____________________________________________________________________________________
//
long _flength(FILE *stream)
{
   long tmp,tmp2;

   tmp = ftell(stream);
   fseek(stream,0,SEEK_END);
   tmp2 = ftell(stream);
   fseek(stream,tmp,SEEK_SET);
   return tmp2-tmp;
}

// _____________________________________________________________________________________
// Copies a given number of bytes from the source file to the destination file.  Copy 
// begins whereever the current read pointers are for the given files.  Returns 0 on error.
//
BOOL _copyfile(FILE *fpi, FILE *fpo, uint len)
{
    ULONG todo;

    while(len)
    {   todo = (len > COPY_BUFSIZE) ? COPY_BUFSIZE : len;
        if(!fread(_mm_cpybuf, todo, 1, fpi))
        {   _mmerr_set(MMERR_END_OF_FILE, "Unexpected End of File", "An unexpected end of file error occured while copying a file.");
            return 0;
        }
        if(!fwrite(_mm_cpybuf, todo, 1, fpo))
        {   _mmerr_set(MMERR_DISK_FULL, "Disk Full Error. Ouch.", "Your disk got full while making a copy of a file.");
            return 0;
        }
        len -= todo;
    }

    return -1;
}

// _____________________________________________________________________________________
// Write an ASCIIZ-terminated string.  Only the number of bytes needed to represent the
// string are written to disk.
//
void _mm_write_stringz(const CHAR *data, MMSTREAM *fp)
{
    if(data) _mm_write_UBYTES((UBYTE *)data, strlen(data), fp);
}

// _____________________________________________________________________________________
// Writes a fixed number of bytes out of an array.  This function is essentially like
// _mm_write_I_UBYTES except behaves properly for UNICODE strings and the like if the
// compiled code happens to be in unicode mode.
// count   - size of the string buffer.  String buffers must be at least this long,
//           although the string itself can be shorter.
//
void _mm_write_string(const CHAR *data, const uint count, MMSTREAM *fp)
{
    if(data) _mm_write_UBYTES((UBYTE *)data, count, fp);
}

// _____________________________________________________________________________________
// A non-sucky verison of fgets!  Keeps reading until the end-of-file or newline is
// encountered, regardless of if the buffer is filled or not before hand.  Also, any
// actual newline characters are ignored!
//
void _mm_fgets(MMSTREAM *fp, CHAR *buffer, uint buflen)
{
    if(buffer && (buflen > 1))
    {
        CHAR c    = _mm_read_UBYTE(fp);
        uint clen = 0;

        while(!_mm_feof(fp) && (c != 13) && (c != 10))
        {
            if(clen < buflen-1)
            {   buffer[clen] = c;
                clen++;
            }

            c  = _mm_read_UBYTE(fp);
        }

        buffer[clen] = 0;
    }
}

// _____________________________________________________________________________________
//
void _mm_fputs(MMSTREAM *fp, const CHAR *data)
{
   if(data) _mm_write_UBYTES((UBYTE *)data, strlen(data), fp);

#ifndef __UNIX__
   _mm_write_UBYTE(13,fp);
#endif
   _mm_write_UBYTE(10,fp);
}


// =====================================================================================
//                            THE OUTPUT (WRITE) SECTION
// =====================================================================================

#ifdef MM_BIG_ENDIAN
#	define filewrite_M_SWORDS(x,y,z) fwrite((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#	define filewrite_M_UWORDS(x,y,z) fwrite((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#	define filewrite_M_SLONGS(x,y,z) fwrite((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#	define filewrite_M_ULONGS(x,y,z) fwrite((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#	define datawrite_M_SWORDS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(SWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#	define datawrite_M_UWORDS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(UWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#	define datawrite_M_SLONGS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(SLONG)), (z)->seekpos += (y)*sizeof(SLONG))
#	define datawrite_M_ULONGS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(ULONG)), (z)->seekpos += (y)*sizeof(ULONG))
#else
#	define filewrite_I_SWORDS(x,y,z) fwrite((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#	define filewrite_I_UWORDS(x,y,z) fwrite((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#	define filewrite_I_SLONGS(x,y,z) fwrite((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#	define filewrite_I_ULONGS(x,y,z) fwrite((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#	define datawrite_I_SWORDS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(SWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#	define datawrite_I_UWORDS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(UWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#	define datawrite_I_SLONGS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(SLONG)), (z)->seekpos += (y)*sizeof(SLONG))
#	define datawrite_I_ULONGS(x,y,z) (memcpy(&(z)->dp[(z)->seekpos],(void *)x,(y)*sizeof(ULONG)), (z)->seekpos += (y)*sizeof(ULONG))
#endif

void _mm_write_SBYTE(SBYTE data, MMSTREAM *fp)
{
	if(fp->fp)
		fputc(data, fp->fp);
	else
		fp->dp[fp->seekpos++] = data;
}

void _mm_write_UBYTE(UBYTE data, MMSTREAM *fp)
{
	if(fp->fp)
		fputc(data, fp->fp);
	else
		fp->dp[fp->seekpos++] = data;
}

void streamwrite16( UWORD data, MMSTREAM *fp )
{
	if(fp->fp)
	{
		fwrite(&data, 2, 1, fp->fp);
	}
	else
	{
		*(UWORD*)fp->dp[fp->seekpos] = data;
		fp->seekpos += 2;
	}
}

void streamwrite32( ULONG data, MMSTREAM *fp )
{
	if(fp->fp)
	{
		fwrite(&data, 4, 1, fp->fp);
	}
	else
	{
		*(ULONG*)fp->dp[fp->seekpos] = data;
		fp->seekpos += 4;
	}
}


void _mm_write_M_UWORD(UWORD data, MMSTREAM *fp)
{
	streamwrite16(mm_swap_M16(data), fp);
}

void _mm_write_I_UWORD(UWORD data, MMSTREAM *fp)
{
	streamwrite16(mm_swap_I16(data), fp);
}

void _mm_write_M_ULONG(ULONG data, MMSTREAM *fp)
{
	streamwrite32(mm_swap_M32(data), fp);
}

void _mm_write_I_ULONG(ULONG data, MMSTREAM *fp)
{
	streamwrite32(mm_swap_I32(data), fp);
}

void _mm_write_M_SWORD(SWORD data, MMSTREAM *fp)
{
	streamwrite16(mm_swap_M16(data), fp);
}

void _mm_write_I_SWORD(SWORD data, MMSTREAM *fp)
{
	streamwrite16(mm_swap_I16(data), fp);
}

void _mm_write_M_SLONG(SLONG data, MMSTREAM *fp)
{
	streamwrite32(mm_swap_M32(data), fp);
}

void _mm_write_I_SLONG(SLONG data, MMSTREAM *fp)
{
	streamwrite32(mm_swap_I32(data), fp);
}

#define DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN(type_name, type, bits)   \
void                                                             \
_mm_write_##type_name##S (const type *data, int number, MMSTREAM *fp)  \
{                                                                \
    while(number>0)                                          \
    {   streamwrite##bits(*data, fp);                    \
        number--;  data++;                                   \
    }                                                        \
}


#define DEFINE_MULTIPLE_WRITE_FUNCTION_NORM(type_name, type)     \
void                                                             \
_mm_write_##type_name##S (const type *data, int number, MMSTREAM *fp)  \
{                                                                \
    if(fp->fp)                                                   \
    {   filewrite_##type_name##S(data,number,fp);                \
    } else                                                       \
    {   datawrite_##type_name##S(data,number,fp);                \
    }                                                            \
}

DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (SBYTE, SBYTE)
DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (UBYTE, UBYTE)


#ifdef MM_BIG_ENDIAN
	DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (I_SWORD, SWORD, 16)
	DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (I_UWORD, UWORD, 16)
	DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (I_SLONG, SLONG, 32)
	DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (I_ULONG, ULONG, 32)
	DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (M_SWORD, SWORD)
	DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (M_UWORD, UWORD)
	DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (M_SLONG, SLONG)
	DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (M_ULONG, ULONG)
#else
	DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (M_SWORD, SWORD, 16)
	DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (M_UWORD, UWORD, 16)
	DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (M_SLONG, SLONG, 32)
	DEFINE_MULTIPLE_WRITE_FUNCTION_ENDIAN (M_ULONG, ULONG, 32)
	DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (I_SWORD, SWORD)
	DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (I_UWORD, UWORD)
	DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (I_SLONG, SLONG)
	DEFINE_MULTIPLE_WRITE_FUNCTION_NORM   (I_ULONG, ULONG)
#endif


// =====================================================================================
//                              THE INPUT (READ) SECTION
// =====================================================================================

#ifdef MM_BIG_ENDIAN
#	define fileread_M_SWORDS(x,y,z)  fread((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#	define fileread_M_UWORDS(x,y,z)  fread((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#	define fileread_M_SLONGS(x,y,z)  fread((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#	define fileread_M_ULONGS(x,y,z)  fread((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#	define dataread_M_SWORDS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(SWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#	define dataread_M_UWORDS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(UWORD)), (z)->seekpos += (y)*sizeof(UWORD))
#	define dataread_M_SLONGS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(SLONG)), (z)->seekpos += (y)*sizeof(SLONG))
#	define dataread_M_ULONGS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(ULONG)), (z)->seekpos += (y)*sizeof(ULONG))
#else
#	define fileread_I_SWORDS(x,y,z)  fread((void *)x,1,(y)*sizeof(SWORD),(z)->fp)
#	define fileread_I_UWORDS(x,y,z)  fread((void *)x,1,(y)*sizeof(UWORD),(z)->fp)
#	define fileread_I_SLONGS(x,y,z)  fread((void *)x,1,(y)*sizeof(SLONG),(z)->fp)
#	define fileread_I_ULONGS(x,y,z)  fread((void *)x,1,(y)*sizeof(ULONG),(z)->fp)
#	define dataread_I_SWORDS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(SWORD)), (z)->seekpos += (y)*sizeof(SWORD))
#	define dataread_I_UWORDS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(UWORD)), (z)->seekpos += (y)*sizeof(UWORD))
#	define dataread_I_SLONGS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(SLONG)), (z)->seekpos += (y)*sizeof(SLONG))
#	define dataread_I_ULONGS(x,y,z)  (memcpy((void *)x,&(z)->dp[(z)->seekpos],(y)*sizeof(ULONG)), (z)->seekpos += (y)*sizeof(ULONG))
#endif

// ============
//   UNSIGNED
// ============

UBYTE _mm_read_UBYTE(MMSTREAM *fp)
{
    return((fp->fp) ? fileread_UBYTE(fp) : dataread_UBYTE(fp));
}

SBYTE _mm_read_SBYTE(MMSTREAM *fp)
{
    return((fp->fp) ? fileread_SBYTE(fp) : dataread_SBYTE(fp));
}

UWORD streamread16( MMSTREAM* fp )
{
	ULONG result;
	if (fp->fp)
		fread(&result, 2, 1, fp->fp);
	else
	{
		result = *(UWORD*)fp->dp[fp->seekpos];
		fp->seekpos += 2;
	}
	return result;
}

ULONG streamread32( MMSTREAM* fp )
{
	ULONG result;
	if (fp->fp)
		fread(&result, 4, 1, fp->fp);
	else
	{
		result = *(ULONG*)fp->dp[fp->seekpos];
		fp->seekpos += 4;
	}
	return result;
}

UWORD _mm_read_I_UWORD(MMSTREAM *fp)
{
    return mm_swap_I16( streamread16(fp) );
}

UWORD _mm_read_M_UWORD(MMSTREAM *fp)
{
	return mm_swap_M16( streamread16(fp) );
}

ULONG _mm_read_I_ULONG(MMSTREAM *fp)
{
	return mm_swap_I32( streamread32(fp) );
}

ULONG _mm_read_M_ULONG(MMSTREAM *fp)
{
	return mm_swap_M32( streamread32(fp) );
}

SWORD _mm_read_I_SWORD(MMSTREAM *fp)
{
	return mm_swap_I16( streamread16(fp) );
}

SWORD _mm_read_M_SWORD(MMSTREAM *fp)
{
	return mm_swap_M16( streamread16(fp) );
}

SLONG _mm_read_I_SLONG(MMSTREAM *fp)
{
	return mm_swap_I32( streamread32(fp) );
}

SLONG _mm_read_M_SLONG(MMSTREAM *fp)
{
	return mm_swap_M32( streamread32(fp) );
}

int _mm_read_string(CHAR *buffer, const uint number, MMSTREAM *fp)
{
    if(fp->fp)
    {   _my_fread(buffer, sizeof(CHAR), number, fp->fp);
        return !_mm_feof(fp);
    } else
    {   memcpy(buffer,&fp->dp[fp->seekpos], sizeof(CHAR) * number);
        fp->seekpos += number;
        return 0;
    }
}


#define DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN(type_name, type, bits)    \
int                                                              \
_mm_read_##type_name##S (type *buffer, uint number, MMSTREAM *fp) \
{                                                                \
    while(number>0)                                          \
    {   *buffer = streamread##bits(fp);                  \
        buffer++;  number--;                                 \
    }                                                        \
    return !_mm_feof(fp);                                        \
}


#define DEFINE_MULTIPLE_READ_FUNCTION_NORM(type_name, type)      \
int                                                              \
_mm_read_##type_name##S (type *buffer, uint number, MMSTREAM *fp) \
{                                                                \
    if(fp->fp)                                                   \
    {   fileread_##type_name##S(buffer,number,fp);               \
    } else                                                       \
    {   dataread_##type_name##S(buffer,number,fp);               \
        fp->seekpos += number;                                   \
    }                                                            \
    return !_mm_feof(fp);                                        \
}

DEFINE_MULTIPLE_READ_FUNCTION_NORM   (SBYTE, SBYTE)
DEFINE_MULTIPLE_READ_FUNCTION_NORM   (UBYTE, UBYTE)

#ifdef MM_BIG_ENDIAN
	DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (I_SWORD, SWORD, 16)
	DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (I_UWORD, UWORD, 16)
	DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (I_SLONG, SLONG, 32)
	DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (I_ULONG, ULONG, 32)
	DEFINE_MULTIPLE_READ_FUNCTION_NORM   (M_SWORD, SWORD)
	DEFINE_MULTIPLE_READ_FUNCTION_NORM   (M_UWORD, UWORD)
	DEFINE_MULTIPLE_READ_FUNCTION_NORM   (M_SLONG, SLONG)
	DEFINE_MULTIPLE_READ_FUNCTION_NORM   (M_ULONG, ULONG)
#else
	DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (M_SWORD, SWORD, 16)
	DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (M_UWORD, UWORD, 16)
	DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (M_SLONG, SLONG, 32)
	DEFINE_MULTIPLE_READ_FUNCTION_ENDIAN (M_ULONG, ULONG, 32)
	DEFINE_MULTIPLE_READ_FUNCTION_NORM   (I_SWORD, SWORD)
	DEFINE_MULTIPLE_READ_FUNCTION_NORM   (I_UWORD, UWORD)
	DEFINE_MULTIPLE_READ_FUNCTION_NORM   (I_SLONG, SLONG)
	DEFINE_MULTIPLE_READ_FUNCTION_NORM   (I_ULONG, ULONG)
#endif

