/*
Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 mlreg.c
 
  A single routine for registering all loaders in Mikamp for the current platform.

*/

#include "mikamp.h"
#include "uniform.h"


void Mikamp_RegisterAllLoaders(void)
{
   //Mikamp_RegisterLoader(load_uni);
   Mikamp_RegisterLoader(load_it);
   Mikamp_RegisterLoader(load_xm);
   Mikamp_RegisterLoader(load_s3m);
   Mikamp_RegisterLoader(load_mod);
   Mikamp_RegisterLoader(load_mtm);
   Mikamp_RegisterLoader(load_stm);
   //Mikamp_RegisterLoader(load_dsm);
   //Mikamp_RegisterLoader(load_med);
   //Mikamp_RegisterLoader(load_far);
   //Mikamp_RegisterLoader(load_ult);
   Mikamp_RegisterLoader(load_669);
}
