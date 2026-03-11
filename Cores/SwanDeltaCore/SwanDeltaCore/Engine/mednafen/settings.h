/* Stub settings header - replaces Mednafen's configuration system
 * with hardcoded defaults for SwanDeltaCore. */
#ifndef __MDFN_SETTINGS_H
#define __MDFN_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Language setting: 0 = Japanese, 1 = English */
static inline bool MDFN_GetSettingB(const char *name) {
    /* wswan.language -> English (1) */
    return true;
}

static inline int64_t MDFN_GetSettingI(const char *name) {
    return 0;
}

static inline double MDFN_GetSettingF(const char *name) {
    return 0.0;
}

#ifdef __cplusplus
}
#endif

#endif
