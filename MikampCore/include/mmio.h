#ifndef _MMIO_H_
#define _MMIO_H_

// ---------------------------
// MMALLOC_NO_FUNCPTR
//  Removes function pointers from the mmalloc block-- disabling user-remappable memory
//  allocation behavior,but resulting in reduced memory footprint and reduced execution
//  times.

#define MMALLOC_NO_FUNCPTR

// ---------------------------
// MM_NAME_LENGTH
//  Length in characters of the allochandle object names.  Object names can be completely
//  disabled by defining MM_NO_NAMES.

#define MM_NAME_LENGTH      24

// ---------------------------
// MM_NO_NAMES
//  Disables all name storage in mm_alloc API.  All objects will be nameless, saving a
//  air amount of memory, improving cache hits, and rendering verbose memory dumps
//  effectively useless!

//#define MM_NO_NAMES


// ---------------------------
// MM_DLL_EXPORT
//  For using Mikamp and MMIO as a DLL library!  Oh swank, thanks to someone cool whom
//  his name I forget.

#ifdef WIN32
#ifndef MMEXPORT
#ifdef MM_DLL_EXPORT
#define MMEXPORT  extern __declspec( dllexport )
#elif MIKAMP_STATIC
#define MMEXPORT  extern
#else
#define MMEXPORT  extern __declspec( dllimport )
#endif
#endif
#else
#define MMEXPORT extern
#endif

#define align(a) __declspec (align (a))

#include <stdio.h>
#include <stdlib.h>
#include "mmtypes.h"

// CPU mode options.  These will force the use of the selected mode, whether that
// cpu is available or not, so make sure you check first!  Speed enhancments are
// present in sample loading and software mixing.

#define CPU_NONE              0
#define CPU_MMX         (1ul<<0)
#define CPU_3DNOW       (1ul<<1)
#define CPU_SSE         (1ul<<2)
#define CPU_SSE2        (1ul<<3)
#define CPU_AUTODETECT    (0xff)

MMEXPORT uint _mm_cpudetect(void);

MMEXPORT void _mm_memcpy_quad(void *dst, const void *src, const int count);
MMEXPORT void _mm_memcpy_long(void *dst, const void *src, const int count);
MMEXPORT void _mm_memcpy_word(void *dst, const void *src, const int count);


// Miscellaneous Macros 
// --------------------
// I should probably put these somewhere else.

// boundschecker macro.  I do a lot of bounds checking ;)

#define _mm_boundscheck(v,a,b)  ((v > b) ? b : ((v < a) ? a : v))

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif


// 64 bit integer macros.  These allow me to get pointers to the high
// and low portions of a 64 bit integer.  Used mostly by the Mikamp
// software mixer - but could be useful for other things.

#ifdef MM_BIG_ENDIAN

#define _mm_HI_SLONG(x) ((SLONG *)&x)
#define _mm_LO_SLONG(x) ((SLONG *)&x + 1)
#define _mm_HI_ULONG(x) ((ULONG *)&x)
#define _mm_LO_ULONG(x) ((ULONG *)&x + 1)

#else

#define _mm_HI_SLONG(x) ((SLONG *)&x + 1)
#define _mm_LO_SLONG(x) ((SLONG *)&x)
#define _mm_HI_ULONG(x) ((ULONG *)&x + 1)
#define _mm_LO_ULONG(x) ((ULONG *)&x)

#endif


#ifdef __cplusplus
extern "C" {
#endif


// =======================================
//  Input / Output and Streaming - MMIO.C
// =======================================

// -------------------------------------------------------------------------------------
// Mikamp customized module file pointer structure.
// Contains various data that may be required for a module.
//
typedef struct MMSTREAM
{   
    FILE   *fp;      // File pointer
    UBYTE  *dp;

    long   iobase;       // base seek position within the file
    long   seekpos;      // used in data(mem) streaming mode only.

} MMSTREAM;

MMEXPORT MMSTREAM *_mmstream_createfp(FILE *fp, int iobase);
MMEXPORT MMSTREAM *_mmstream_createmem(void *data, int iobase);
MMEXPORT void      _mmstream_delete(MMSTREAM *mmf);

MMEXPORT void _mm_RegisterErrorHandler(void (*proc)(int, CHAR *));
MMEXPORT BOOL _mm_fexist(CHAR *fname);

MMEXPORT void StringWrite(const CHAR *s, MMSTREAM *fp);
MMEXPORT CHAR *StringRead(MMSTREAM *fp);


//  Mikamp/DivEnt style file input / output -
//    Solves several portability issues.
//    Notably little vs. big endian machine complications.

#define _mm_rewind(x) _mm_fseek(x,0,SEEK_SET)

MMEXPORT int      _mm_fseek(MMSTREAM *stream, long offset, int whence);
MMEXPORT long     _mm_ftell(MMSTREAM *stream);
MMEXPORT BOOL     _mm_feof(MMSTREAM *stream);
MMEXPORT MMSTREAM *_mm_fopen(const CHAR *fname, const CHAR *attrib);
MMEXPORT void     _mm_fclose(MMSTREAM *mmfile);
MMEXPORT MMSTREAM *_mm_tmpfile(void);
MMEXPORT BOOL     _mm_mkdir(const CHAR *path);

MMEXPORT void     _mm_fgets(MMSTREAM *fp, CHAR *buffer, uint buflen);
MMEXPORT void     _mm_fputs(MMSTREAM *fp, const CHAR *data);
MMEXPORT void     _mm_write_string(const CHAR *data, const uint number, MMSTREAM *fp);
MMEXPORT void     _mm_write_stringz(const CHAR *data, MMSTREAM *fp);
MMEXPORT int      _mm_read_string(CHAR *buffer, const uint number, MMSTREAM *fp);
MMEXPORT long     _mm_flength(MMSTREAM *stream);

MMEXPORT long     _flength(FILE *stream);
MMEXPORT BOOL     _copyfile(FILE *fpi, FILE *fpo, uint len);


MMEXPORT SBYTE _mm_read_SBYTE (MMSTREAM *fp);
MMEXPORT UBYTE _mm_read_UBYTE (MMSTREAM *fp);

MMEXPORT SWORD _mm_read_M_SWORD (MMSTREAM *fp);
MMEXPORT SWORD _mm_read_I_SWORD (MMSTREAM *fp);

MMEXPORT UWORD _mm_read_M_UWORD (MMSTREAM *fp);
MMEXPORT UWORD _mm_read_I_UWORD (MMSTREAM *fp);

MMEXPORT SLONG _mm_read_M_SLONG (MMSTREAM *fp);
MMEXPORT SLONG _mm_read_I_SLONG (MMSTREAM *fp);

MMEXPORT ULONG _mm_read_M_ULONG (MMSTREAM *fp);
MMEXPORT ULONG _mm_read_I_ULONG (MMSTREAM *fp);


MMEXPORT int _mm_read_SBYTES    (SBYTE *buffer, uint number, MMSTREAM *fp);
MMEXPORT int _mm_read_UBYTES    (UBYTE *buffer, uint number, MMSTREAM *fp);

MMEXPORT int _mm_read_M_SWORDS  (SWORD *buffer, uint number, MMSTREAM *fp);
MMEXPORT int _mm_read_I_SWORDS  (SWORD *buffer, uint number, MMSTREAM *fp);

MMEXPORT int _mm_read_M_UWORDS  (UWORD *buffer, uint number, MMSTREAM *fp);
MMEXPORT int _mm_read_I_UWORDS  (UWORD *buffer, uint number, MMSTREAM *fp);

MMEXPORT int _mm_read_M_SLONGS  (SLONG *buffer, uint number, MMSTREAM *fp);
MMEXPORT int _mm_read_I_SLONGS  (SLONG *buffer, uint number, MMSTREAM *fp);

MMEXPORT int _mm_read_M_ULONGS  (ULONG *buffer, uint number, MMSTREAM *fp);
MMEXPORT int _mm_read_I_ULONGS  (ULONG *buffer, uint number, MMSTREAM *fp);


MMEXPORT void _mm_write_SBYTE     (SBYTE data, MMSTREAM *fp);
MMEXPORT void _mm_write_UBYTE     (UBYTE data, MMSTREAM *fp);

MMEXPORT void _mm_write_M_SWORD   (SWORD data, MMSTREAM *fp);
MMEXPORT void _mm_write_I_SWORD   (SWORD data, MMSTREAM *fp);

MMEXPORT void _mm_write_M_UWORD   (UWORD data, MMSTREAM *fp);
MMEXPORT void _mm_write_I_UWORD   (UWORD data, MMSTREAM *fp);

MMEXPORT void _mm_write_M_SLONG   (SLONG data, MMSTREAM *fp);
MMEXPORT void _mm_write_I_SLONG   (SLONG data, MMSTREAM *fp);

MMEXPORT void _mm_write_M_ULONG   (ULONG data, MMSTREAM *fp);
MMEXPORT void _mm_write_I_ULONG   (ULONG data, MMSTREAM *fp);

MMEXPORT void _mm_write_SBYTES    (const SBYTE *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_UBYTES    (const UBYTE *data, int number, MMSTREAM *fp);

MMEXPORT void _mm_write_M_SWORDS  (const SWORD *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_I_SWORDS  (const SWORD *data, int number, MMSTREAM *fp);

MMEXPORT void _mm_write_M_UWORDS  (const UWORD *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_I_UWORDS  (const UWORD *data, int number, MMSTREAM *fp);

MMEXPORT void _mm_write_M_SLONGS  (const SLONG *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_I_SLONGS  (const SLONG *data, int number, MMSTREAM *fp);

MMEXPORT void _mm_write_M_ULONGS  (const ULONG *data, int number, MMSTREAM *fp);
MMEXPORT void _mm_write_I_ULONGS  (const ULONG *data, int number, MMSTREAM *fp);


#include "mmalloc.h"
#include "mmerror.h"


#ifdef __cplusplus
};
#endif

#endif
