#pragma once

// 

#if !defined(WANT_TRACE_WAVFILE)
#   define WANT_TRACE_WAVFILE 1
#endif

#if WANT_TRACE_WAVFILE
#   include "trace_riff_wav.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void _vc_trace_init(VIRTCH* vc);

extern TraceRiffWav* g_trace_wav_finalmix;

#ifdef __cplusplus
}
#endif
