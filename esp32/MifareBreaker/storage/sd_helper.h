#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool sd_init(void);
bool sd_is_available(void);

#ifdef __cplusplus
}
#endif
