#pragma once

#include "mmio.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================
//  LOG.C Prototypes
// ==================

#define LOG_SILENT   0
#define LOG_VERBOSE  1

extern int  log_init    (const CHAR *logfile, BOOL val);
extern void log_exit    (void);
extern void log_verbose (void);
extern void log_silent  (void);
extern void __cdecl log_print   (const CHAR *fmt, ... );
extern void __cdecl log_printv  (const CHAR *fmt, ... );


#ifdef __cplusplus
};
#endif
