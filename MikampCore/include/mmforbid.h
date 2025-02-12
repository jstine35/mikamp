/*
  Cross-Platform Thread Synchronization Shizat
  By Jake Stine of Divine Entertainment
  ----------------------------------------------
  I use the term cross-platform rather loosely for now since I've never tested this
  code on anything but the Win32 platform.

  Anyway, it uses the platform's default critical section/mutex API.  If the platform
  has none, then it uses basic dumb 'forbid flagging.'  -- set the flag, check the
  flag, unset the flag.

  Note:  The trouble with flagging is that something of the same thread cannot enter
  a forbidden code block, which would cause a lockup.  This should never really happen
  anyway - but it might if I made a booboo somewhere.
*/

#ifndef _MM_FORBID_H_
#define _MM_FORBID_H_

// =====================================================================================
    #ifdef _INC_WINDOWS
// =====================================================================================
// This is stuff we only include if the user has included windows.h.  This is essentially
// platform dependant code.  Well, no essentially.  It plain is.  However, my original
// resoning behind doing this was to remove the #include <windows.h> from this header
// file and, consequently, speed up complation a whole lot!

#define MM_FORBID CRITICAL_SECTION

#else
#ifndef _WIN32

#include <pthread.h>

#define MM_FORBID pthread_mutex_t

#else

struct _MM_FORBID;
#define MM_FORBID struct _MM_FORBID

#endif
#endif

extern MM_FORBID *_mmforbid_init(void);
extern void       _mmforbid_deinit(MM_FORBID *forbid);
extern void       _mmforbid_enter(MM_FORBID *forbid);
extern void       _mmforbid_exit(MM_FORBID *forbid);

#endif
