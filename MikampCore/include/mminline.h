
#pragma once

#include "mmio.h"
#include <string.h>
#include <stdlib.h>

static long _mm_inline _mm_timeoverflow(ulong newtime, ulong oldtime)
{
    if(newtime < oldtime) return (0xfffffffful - oldtime) + newtime;
    return newtime - oldtime;       // return timediff, with overflow checks!
}

#ifdef _MSC_VER
#	define mm_byteswap16(in)	_byteswap_ushort(in)
#	define mm_byteswap32(in)	_byteswap_ulong(in)
#	define mm_byteswap64(in)	_byteswap_uint64(in)
#else
#	define mm_byteswap16(in)	__builtin_bswap16(in)
#	define mm_byteswap32(in)	__builtin_bswap32(in)
#	define mm_byteswap64(in)	__builtin_bswap64(in)
#endif

#ifdef MM_BIG_ENDIAN
#	define mm_swap_I16(in)		mm_byteswap16(in)
#	define mm_swap_I32(in)		mm_byteswap32(in)
#	define mm_swap_I64(in)		mm_byteswap64(in)
#	define mm_swap_M16(in)		(in)
#	define mm_swap_M32(in)		(in)
#	define mm_swap_M64(in)		(in)
#else
#	define mm_swap_I16(in)		(in)
#	define mm_swap_I32(in)		(in)
#	define mm_swap_I64(in)		(in)
#	define mm_swap_M16(in)		mm_byteswap16(in)
#	define mm_swap_M32(in)		mm_byteswap32(in)
#	define mm_swap_M64(in)		mm_byteswap64(in)
#endif

// =====================================================================================
//                            THE OUTPUT (WRITE) SECTION
// =====================================================================================


#define filewrite_SBYTES(x,y,z)  fwrite((void *)x,1,y,(z)->fp)
#define filewrite_UBYTES(x,y,z)  fwrite((void *)x,1,y,(z)->fp)

#define datawrite_SBYTES(x,y,z)  (memcpy(&(z)->dp[(z)->seekpos],(void *)x,y), (z)->seekpos += y)
#define datawrite_UBYTES(x,y,z)  (memcpy(&(z)->dp[(z)->seekpos],(void *)x,y), (z)->seekpos += y)


// =====================================================================================
//                              THE INPUT (READ) SECTION
// =====================================================================================

#define fileread_SBYTE(x)         (SBYTE)fgetc((x)->fp)
#define fileread_UBYTE(x)         (UBYTE)fgetc((x)->fp)

#define dataread_SBYTE(y)         (y)->dp[(y)->seekpos++]
#define dataread_UBYTE(y)         (y)->dp[(y)->seekpos++]


#define fileread_SBYTES(x,y,z)  fread((void *)x,1,y,(z)->fp)
#define fileread_UBYTES(x,y,z)  fread((void *)x,1,y,(z)->fp)

#define dataread_SBYTES(x,y,z)  memcpy((void *)x,&(z)->dp[(z)->seekpos],y)
#define dataread_UBYTES(x,y,z)  memcpy((void *)x,&(z)->dp[(z)->seekpos],y)
