/* 
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 ------------------------------------------
 trace_riff_wav.c

  A lightweight periodic-flushing RiffWav output to file, ideal for use in debugging mixers.
  The output file headers are flushed periodically to ensure that the resulting .wav file is
  loadable into popular audio inspection tools such as Audacity or SoundForge.

  Minimal parameter validation is implemented.
  
  Based on free documentation found at:
   - https://en.wikipedia.org/wiki/WAV
   - http://soundfile.sapp.org/doc/WaveFormat/
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "trace_riff_wav.h"

#if defined(_MSC_VER) || defined(__MINGW64__)
#   define fseeko  _fseeki64
#   define ftello  _ftelli64
#endif

static const int kFlushThresholdBytes = 4 * 1024;       // 4kb

static int FormatTypeToAudioFormat(RiffWavFormatType code) {
    switch(code) {
        case kWavAudio_PCM_UInt8     : return 1;
        case kWavAudio_PCM_Int16     : return 1;
        case kWavAudio_PCM_Flt32     : return 3;
        case kWavAudio_PCM_Flt64     : return 3;
        case kWavAudio_ADPCM         : return 2;
    }
    assert(0);
    return 0;
}

static int FormatTypeToBytesPerSample(RiffWavFormatType code) {
    switch(code) {
        case kWavAudio_PCM_UInt8     : return 1;
        case kWavAudio_PCM_Int16     : return 2;
        case kWavAudio_PCM_Flt32     : return 4;
        case kWavAudio_PCM_Flt64     : return 8;
        case kWavAudio_ADPCM         : return 0;
    }
    assert(0);
    return 0;
};

extern void TraceRiffWav_Init(TraceRiffWav* rwo, int SampleRate, int nChannels, RiffWavFormatType fmt, FILE* fp) {
    if (!rwo) return;
    assert(!rwo->m_fp);

    memset(rwo, 0, sizeof(*rwo));
    if (!fp) return;

    rwo->m_fp = fp;
    rwo->m_format_id = fmt;

    // The complete RIFF/WAV header set it written out and positions of the various chunk and subchunk lengths
    // is saved for use later during "flush" operations.
 
    uint32_t ChunkFileSize = 0;
    fwrite("RIFF", sizeof(uint32_t), 1, rwo->m_fp);
    rwo->m_seekpos_riff_length = ftello(rwo->m_fp);
    fwrite(&ChunkFileSize, sizeof(uint32_t), 1, rwo->m_fp);
    fwrite("WAVE", sizeof(uint32_t), 1, rwo->m_fp);

    uint16_t CompCode           = FormatTypeToAudioFormat(rwo->m_format_id);
    uint16_t BytesPerSample     = FormatTypeToBytesPerSample(rwo->m_format_id);
    uint32_t AvgBytesPerSecond  = SampleRate * nChannels * BytesPerSample;
    uint16_t BlockAlign         = BytesPerSample * nChannels;
    uint16_t BitsPerSample      = BytesPerSample * 8;

    // RIFF considers headers and the data represented by them as independent subchunks, which is
    // somewhat clever but also confusing. Refer to them here as SubchunkHead and SubchunkData.

    uint32_t SubchunkHeadSize = 16;
    fwrite("fmt ",              1, sizeof(uint32_t), rwo->m_fp);     // subchunk ID "fmt " with a space.
    fwrite(&SubchunkHeadSize,   1, sizeof(uint32_t), rwo->m_fp);
    fwrite(&CompCode,           1, sizeof(uint16_t), rwo->m_fp);
    fwrite(&nChannels,          1, sizeof(uint16_t), rwo->m_fp);
    fwrite(&SampleRate,         1, sizeof(uint32_t), rwo->m_fp);
    fwrite(&AvgBytesPerSecond,  1, sizeof(uint32_t), rwo->m_fp);
    fwrite(&BlockAlign,         1, sizeof(uint16_t), rwo->m_fp);
    fwrite(&BitsPerSample,      1, sizeof(uint16_t), rwo->m_fp);

    uint32_t SubchunkDataSize = 0;
    fwrite("data", 4, 1, rwo->m_fp);
    rwo->m_seekpos_subchunk_length = ftello(rwo->m_fp);
    fwrite(&SubchunkDataSize, 4, 1, rwo->m_fp);
}

static void _impl_writeDataVoid(TraceRiffWav* rwo, void const* data, int bytes) {
    fwrite(data, 1, bytes, rwo->m_fp);
    rwo->m_unflushed_bytes += bytes;

    if (rwo->m_unflushed_bytes >= kFlushThresholdBytes) {
        TraceRiffWav_Flush(rwo);
    }
}

void TraceRiffWav_WriteDataFlt32(TraceRiffWav* rwo, float const* data, int bytes) {
    if (!rwo) return;
    if (!rwo->m_fp) return;
    assert(rwo->m_format_id == kWavAudio_PCM_Flt32);
    _impl_writeDataVoid(rwo, data, bytes);
}

void TraceRiffWav_WriteDataInt16(TraceRiffWav* rwo, int16_t const* data, int bytes) {
    if (!rwo) return;
    if (!rwo->m_fp) return;
    assert(rwo->m_format_id == kWavAudio_PCM_Int16);
    _impl_writeDataVoid(rwo, data, bytes);
}

void TraceRiffWav_WriteDataUInt8(TraceRiffWav* rwo, uint8_t const* data, int bytes) {
    if (!rwo) return;
    if (!rwo->m_fp) return;
    assert(rwo->m_format_id == kWavAudio_PCM_UInt8);
    _impl_writeDataVoid(rwo, data, bytes);
}

void TraceRiffWav_Flush(TraceRiffWav* rwo) {
    if (!rwo) return;
    if (!rwo->m_fp) return;
    if (!rwo->m_unflushed_bytes) return;

    intmax_t CurrentPos = ftello(rwo->m_fp);
    rwo->m_subchunk_length += rwo->m_unflushed_bytes;
    fseeko(rwo->m_fp, rwo->m_seekpos_subchunk_length, SEEK_SET);
    fwrite(&rwo->m_subchunk_length, 4, 1, rwo->m_fp);
    fseeko(rwo->m_fp, rwo->m_seekpos_riff_length, SEEK_SET);

    intmax_t NewLength = CurrentPos - 8;
    fwrite(&NewLength, 4, 1, rwo->m_fp);
    fseeko(rwo->m_fp, CurrentPos, SEEK_SET);

    fflush(rwo->m_fp);
    rwo->m_unflushed_bytes = 0;
}

void TraceRiffWav_Close(TraceRiffWav* rwo) {
    if (!rwo) return;
    if (!rwo->m_fp) return;

    TraceRiffWav_Flush(rwo);
    fclose(rwo->m_fp);
    rwo->m_fp = NULL;
}
