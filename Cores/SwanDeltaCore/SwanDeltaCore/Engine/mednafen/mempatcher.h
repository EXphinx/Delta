/* Stub mempatcher header - cheat/memory patching functionality is
 * not used in SwanDeltaCore v1. All functions are no-ops. */
#ifndef __MDFN_MEMPATCHER_H
#define __MDFN_MEMPATCHER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void MDFNMP_Init(uint32_t ps, uint32_t numpages) { (void)ps; (void)numpages; }
static inline void MDFNMP_Kill(void) {}
static inline void MDFNMP_InstallReadPatches(void) {}
static inline void MDFNMP_ApplyPeriodicCheats(void) {}

static inline void MDFN_LoadGameCheats(void *override_ptr) { (void)override_ptr; }
static inline void MDFN_FlushGameCheats(int nosave) { (void)nosave; }

#ifdef __cplusplus
}
#endif

#endif
