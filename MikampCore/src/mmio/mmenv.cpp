
/*
 Mikamp Sound System

 This is free and unencumbered software released into the public domain.
 The Unlicense : See LICENSE.txt file included with this source distribution
 ------------------------------------------
 mmenv.cpp

 Replacement for POSIX getenv() which provides a helper layer for aliasing Environment var
 assignments with CLI assignments. Useful for any software that tends to be invoked as
 either a DLL library or from CLI tools.  Command line (CLI) options and Environment
 Variables are simutaneously managed via mmenv. It uses a standard-ish behavior of treating
 the following style of CLI and env var as aliases:

   CLI: --foo-bar=1
   ENV: MIKAMP_FOO_BAR=1

 Module Initialization: 
   This aliasing is setup once per process by strategically calling _mmenv_init as early as
   possible at a point where multiple threads are understood to not be attmepting to access
   things.

 Thread Safety Policy: Read-only.
   For all functions in this module: Reading is thread-safe. Writing is not thread-safe
   and should be done only during early process init, prior to the invocation of additional
   threads.
 
*/

#include <map>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <cassert>

#include "mmenv.h"
#include "mmio.h"

#define trace_env(fmt, ...) _mmlog(fmt, ## __VA_ARGS__)

std::map<std::string, std::string> s_map_environ;

static std::string& transform_upper(std::string& text) {
    std::transform(text.begin(), text.end(), text.begin(), [](char c) {
        return toupper((uint8_t)c);
    });
    return text;
}

extern "C"
void _mmenv_init(void) {

    static bool s_inited;
    if (s_inited) return;
    s_inited = 1;

    // keep outside loop - a persistent memory allocation for key string.
    std::string key;

    trace_env("[ENV] Scanning for MIKAMP variables...");
    int num_found = 0;
    for(auto const* const* env = environ; env[0]; ++env) {
        if (strncmp(env[0], "MIKAMP_", 7) != 0) continue;
        auto const* takepos = env[0] + 7;

        key.clear();
        for(; takepos[0] && takepos[0] != '='; ++takepos) {
            auto ch = (uint8_t)takepos[0];
            if (ch == '_') ch = '-';
            key += tolower(ch);
        }

        char const* rvalue = "";
        if (takepos[0]) {
            assert(takepos[0] == '=');
            rvalue = takepos+1;
        }

        trace_env("  %s", env[0]);
        s_map_environ.insert({key,rvalue});
        ++num_found;
    }

    trace_env("[ENV] Inherited %d MIKAMP variables from environment.", num_found);
}

// Add a CLI option to the internal getenv registry. KVP input is expected to follow standard
// CLI parsing rules (whitespace and quotes are taken literally).
// Thread safety: none. mutex locks requred around this and _mmenv_get. The recommended use
// use pattern is to perform all CLI parsing early in the process startup, prior to starting
// any threads.
extern "C"
void _mmenv_parse_cli_option(char const* kvp) {
    // as a rule cli options are "well formed" without whitespace:
    //   key=value
    // the 'value' is taken as a literal, spaces, quotes, and all.

    if (auto* cpos = strchr(kvp, '='); cpos != nullptr) {
        auto keyLength = cpos - kvp;
        std::string key(kvp, keyLength);
        std::string value(cpos + 1);
        s_map_environ.insert({key,value});
    }
}

extern "C"
char const* _mmenv_get(char const* key) {
    if (key[0] == '-' && key[1] == '-') {
        if (auto it = s_map_environ.find(key+2); it != s_map_environ.end()) {
            return it->second.c_str();
        }
    }

    return getenv(key);
}

