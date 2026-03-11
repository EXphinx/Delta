#ifndef RETRO_INLINE_H__
#define RETRO_INLINE_H__

#ifndef INLINE
#if defined(_MSC_VER)
#define INLINE __forceinline
#else
#define INLINE inline __attribute__((always_inline))
#endif
#endif

#endif
