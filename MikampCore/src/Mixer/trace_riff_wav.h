#pragma once


#include <stdint.h>

typedef enum {
    kWavAudio_PCM_UInt8,
    kWavAudio_PCM_Int16,
    kWavAudio_PCM_Flt32,
    kWavAudio_PCM_Flt64,
    kWavAudio_ADPCM,      // ADPCM is a custom integer-based format (non-float) [unsupported]
} RiffWavFormatType;

typedef struct {
    RiffWavFormatType m_format_id;
    intmax_t m_seekpos_riff_length;       // typically the length of the entire file
    intmax_t m_seekpos_subchunk_length;   // length of the WAV portion minus the RIFF and subchunk headers.
    intmax_t m_subchunk_length;           // running sum subchunk lenth, flushed to file periodically.

    FILE* m_fp;
    int m_unflushed_bytes;
} TraceRiffWav;

#ifdef __cplusplus
extern "C" {
#endif

extern void TraceRiffWav_Init(TraceRiffWav* rwo, int SampleRate, int nChannels, RiffWavFormatType fmt, FILE* fp);
extern void TraceRiffWav_Close(TraceRiffWav* rwo);
extern void TraceRiffWav_Flush(TraceRiffWav* rwo);
extern void TraceRiffWav_WriteDataUInt8(TraceRiffWav* rwo, uint8_t const* data, int bytes);
extern void TraceRiffWav_WriteDataInt16(TraceRiffWav* rwo, int16_t const* data, int bytes);
extern void TraceRiffWav_WriteDataFlt32(TraceRiffWav* rwo, float   const* data, int bytes);

#ifdef __cplusplus
}
#endif
