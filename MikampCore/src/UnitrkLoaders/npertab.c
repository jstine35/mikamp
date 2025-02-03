/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
------------------------------------------
 npertab.c
  MOD format period table.  Used by both the MOD and M15 (15-inst mod) Loaders.
  
  This is provided in its own file so that the table won't be linked into the final
  executable, in the event the user is not registering mod/m15 format support.
*/

#include "mmtypes.h"

UWORD npertab[60] =
{   // -> Tuning 0
    1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,906,
    856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,
    214,202,190,180,170,160,151,143,135,127,120,113,
    107,101,95,90,85,80,75,71,67,63,60,56
};

