
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern void _mmenv_parse_cli_option(char const* kvp);
extern void _mmenv_init(void);
extern char const* _mmenv_get(char const* key);

#ifdef __cplusplus
}
#endif
