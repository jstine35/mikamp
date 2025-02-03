/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 msw/mmforbid.c
 
 The Mikamp 'Portable' thread locking / mutexing.

 This is a Win32 version only.  See posix/mmforbid.c for the linux/pthreads edition!

*/

#if  defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#include <windows.h>
#include "mmforbid.h"

#include "mmio.h"

// _____________________________________________________________________________________
//
MM_FORBID *_mmforbid_init( void )
{
    MM_FORBID  *forbid;

    forbid = (MM_FORBID *)_mm_calloc(NULL, 1, sizeof(MM_FORBID));
    InitializeCriticalSection(forbid);

    return forbid;
}

// _____________________________________________________________________________________
//
void _mmforbid_deinit( MM_FORBID *forbid )
{
    if( forbid )
    {
        DeleteCriticalSection( forbid );
        _mm_free( NULL, forbid );
    }
}

// _____________________________________________________________________________________
//
void _mmforbid_enter( MM_FORBID *forbid )
{
    if( forbid ) EnterCriticalSection( forbid );
}

// _____________________________________________________________________________________
// Returns TRUE if the call succeeded and the mutex was not in use.
// Returns FALSE if the mutex is in use.
//
// Notes : This function is only available in WindowsNT and above.
//
#if ( _WIN32_WINNT >= 0x0400 )
BOOL _mmforbid_try( MM_FORBID *forbid )
{
    if( !forbid ) return TRUE;
    return TryEnterCriticalSection( forbid );
}
#endif

// _____________________________________________________________________________________
//
void _mmforbid_exit( MM_FORBID *forbid )
{
    if( forbid ) LeaveCriticalSection( forbid );
}


#endif
