/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 mdreg.c

 A single routine for registering all drivers in Mikamp for the current
 platform.

*/

#include "mikamp.h"

void Mikamp_RegisterAllDrivers(void)
{
#ifdef MIKAMP_SDL
    Mikamp_RegisterDriver(drv_sdl);
#elif defined(SUN)
    Mikamp_RegisterDriver(drv_sun);
#elif defined(SOLARIS)
    Mikamp_RegisterDriver(drv_sun);
#elif defined(__alpha)
    Mikamp_RegisterDriver(drv_AF);
#elif defined(OSS)
    Mikamp_RegisterDriver(drv_oss);
    #ifdef ULTRA
       Mikamp_RegisterDriver(drv_ultra);
    #endif
#elif defined(__hpux)
    Mikamp_RegisterDriver(drv_hp);
#elif defined(AIX)
    Mikamp_RegisterDriver(drv_aix);
#elif defined(SGI)
    Mikamp_RegisterDriver(drv_sgi);
#elif defined(__OS2__)
    Mikamp_RegisterDriver(drv_os2);
#elif defined(__NT__)
    Mikamp_RegisterDriver(drv_ds);
    //Mikamp_RegisterDriver(drv_win);
#elif defined(__WIN32__)
    Mikamp_RegisterDriver(drv_ds);
    //Mikamp_RegisterDriver(drv_win);
#elif defined(WIN32)
    Mikamp_RegisterDriver(drv_ds);
    //Mikamp_RegisterDriver(drv_win);
#else
//    Mikamp_RegisterDriver(drv_awe);
    Mikamp_RegisterDriver(drv_gus);    // supports both hardware and software mixing - needed for games.
    //Mikamp_RegisterDriver(drv_gus2);     // use for hardware mixing only (smaller / faster)
    Mikamp_RegisterDriver(drv_pas);
//    Mikamp_RegisterDriver(drv_wss);
    Mikamp_RegisterDriver(drv_ss);
    Mikamp_RegisterDriver(drv_sb16);
    Mikamp_RegisterDriver(drv_sbpro);
    Mikamp_RegisterDriver(drv_sb);
#endif

}


