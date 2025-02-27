#include <vector>
#include <string>
#include <sstream>
#include <format>
#include <algorithm>

#include "vchcrap.h"
#include "vch-trace-wav.h"
#include "mmenv.h"

#if WANT_TRACE_WAVFILE
static std::string s_dbg_wav_basepath;

// Use global instance but keep a pointer to it to simplify away some & syntax.
static TraceRiffWav  _s_trace_wav_finalmix_inst = {};
TraceRiffWav* g_trace_wav_finalmix = &_s_trace_wav_finalmix_inst;

static FILE* s_fp_finalmix;
#endif

static int s_vc_trace_id = 0;

std::string _vc_trace_wav_basepath() {
    if (!s_dbg_wav_basepath.empty()) {
        return s_dbg_wav_basepath;
    }

    if (auto* path = _mmenv_get("--trace-wav-outdir")) {
        s_dbg_wav_basepath = path;
    }

    if (s_dbg_wav_basepath.empty()) {
        s_dbg_wav_basepath = ".";
    }

    return s_dbg_wav_basepath;
}

static std::string& transform_lower(std::string& text) {
    std::transform(text.begin(), text.end(), text.begin(), [](char c) {
        return tolower((uint8_t)c);
    });
    return text;
}

extern "C"
void _vc_trace_init(VIRTCH* vc) {
    char const* wavTraceList = _mmenv_get("--trace-wav");
    if (!wavTraceList) return;
    auto ss = std::stringstream { wavTraceList };

    bool want_finalmix = {};

    std::string item;
    while (std::getline(ss, item, ',')) {
        transform_lower(item);
        if (item == "finalmix") { want_finalmix = 1; continue; }
    }

    if (s_vc_trace_id) {
        // to support multiple instances of VIRTCH internal tracing, there needs to be some sort of
        // short name identifier bound to the VIRTCH instance so that it can be directed to its own
        // subdir or filter-selected by name.
        _mmlog("[trace-wav] Multiple instances of VIRTCH cannot be traced.");
        return;
    }

    if (auto* path = _mmenv_get("--trace-wav-outdir")) {
        s_dbg_wav_basepath = path;
    }

    if (s_dbg_wav_basepath.empty()) {
        s_dbg_wav_basepath = ".";
    }

    if (want_finalmix) {
        auto fullpath = std::format("{}/{}", s_dbg_wav_basepath, "finalmix.wav");

        if (!g_trace_wav_finalmix->m_fp) {
            FILE* fp = fopen(fullpath.c_str(), "wb");
            if (!fp) {
                _mmlog("[trace-wav] failed to open output file: %s", fullpath.c_str());
                _mmlog("[trace-wav] make sure that the output directory exists.");
            }

            _mmlog("[trace-wav] writing finalmix to file: %s", fullpath.c_str());
            TraceRiffWav_Init(g_trace_wav_finalmix, vc->mixspeed, vc->channels, kWavAudio_PCM_Int16, fp);
        }
    }

    // guard against multiple trace instances
    s_vc_trace_id++;
}
