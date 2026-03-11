/*
 * swan_core.c
 * SwanDeltaCore
 *
 * C-API implementation wrapping the beetle-wswan/Mednafen engine.
 * Licensed under GPL-2.0.
 */

#include "swan_core.h"

#include <stdlib.h>
#include <string.h>

/* Mednafen core headers */
#include "mednafen/mednafen-types.h"
#include "mednafen/git.h"
#include "mednafen/wswan/wswan.h"
#include "mednafen/wswan/gfx.h"
#include "mednafen/wswan/interrupt.h"
#include "mednafen/wswan/wswan-memory.h"
#include "mednafen/wswan/start.inc"
#include "mednafen/wswan/sound.h"
#include "mednafen/wswan/v30mz.h"
#include "mednafen/wswan/rtc.h"
#include "mednafen/wswan/eeprom.h"
#include "mednafen/mempatcher.h"
#include "mednafen/settings.h"

/* ================================================================
 * Internal state
 * ================================================================ */

/* Pixel format config - always use 32-bit XRGB8888 for iOS BGRA */
static int pix_bytes = 4;
static int pix_depth = 24; /* 24 = XRGB8888 in mednafen's convention */

/* Video surface */
static MDFN_Surface *surf = NULL;

/* External globals required by wswan engine */
int    wsc = 1;
uint32 rom_size;
uint16 WSButtonStatus;

/* SRAM/EEPROM bookkeeping */
static uint32 SRAMSize = 0;

/* Audio buffer (dynamically sized by WSwan_SoundFlush) */
static int16_t *audio_samples_buf = NULL;
static int32_t  audio_samples_buf_size = 0;
static int32_t  last_audio_frame_count = 0;

/* Sample rate */
static double target_sample_rate = 48000.0;

/* Input button state pointer (engine reads from here) */
static uint8 *chee = NULL;
static uint16_t input_buf = 0;

/* Color mode detected from ROM header */
static bool is_color = false;

/* Mono palette defaults (classic black-to-white) */
static uint32 mono_pal_start = 0x000000;
static uint32 mono_pal_end   = 0xFFFFFF;

/* Forward declaration (from mednafen engine) */
int StateAction(StateMem *sm, int load, int data_only);

/* ================================================================
 * Internal helpers (adapted from libretro.c)
 * ================================================================ */

static uint32 next_pow2(uint32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static void engine_reset(void) {
    int u0;
    v30mz_reset();
    WSwan_MemoryReset();
    WSwan_GfxReset();
    WSwan_SoundReset();
    WSwan_InterruptReset();
    WSwan_RTCReset();
    WSwan_EEPROMReset();

    for (u0 = 0; u0 < 0xc9; u0++) {
        if (u0 != 0xC4 && u0 != 0xC5 && u0 != 0xBA && u0 != 0xBB)
            WSwan_writeport(u0, startio[u0]);
    }

    v30mz_set_reg(NEC_SS, 0);
    v30mz_set_reg(NEC_SP, 0x2000);
}

/* ================================================================
 * 1. Lifecycle
 * ================================================================ */

bool swan_init(double audio_sample_rate) {
    target_sample_rate = audio_sample_rate;
    
    /* Reset all state */
    audio_samples_buf = NULL;
    audio_samples_buf_size = 0;
    last_audio_frame_count = 0;
    surf = NULL;
    SRAMSize = 0;
    is_color = false;
    input_buf = 0;
    
    return true;
}

bool swan_load_rom(const uint8_t* rom_data, size_t rom_size_bytes) {
    uint32 pow_size;
    uint32 real_rom_size;
    uint8 header[10];

    if (rom_size_bytes < 65536)
        return false;

    real_rom_size = (uint32)((rom_size_bytes + 0xFFFF) & ~0xFFFF);
    pow_size = next_pow2(real_rom_size);
    rom_size = pow_size + (pow_size == 0);

    wsCartROM = (uint8 *)calloc(1, rom_size);
    if (!wsCartROM) return false;

    if (real_rom_size < rom_size)
        memset(wsCartROM, 0xFF, rom_size - real_rom_size);

    memcpy(wsCartROM + (rom_size - real_rom_size), rom_data, rom_size_bytes);

    /* Parse ROM header (last 10 bytes) */
    memcpy(header, wsCartROM + rom_size - 10, 10);

    /* Detect WS vs WSC */
    is_color = (header[0] & 0x01) ? true : false; /* Bit 0 of system byte */
    wsc = is_color ? 1 : 0;

    /* Determine save memory type and size */
    SRAMSize = 0;
    eeprom_size = 0;

    switch (header[5]) {
        case 0x01: SRAMSize =   8 * 1024; break;
        case 0x02: SRAMSize =  32 * 1024; break;
        case 0x03: SRAMSize = 128 * 1024; break;
        case 0x04: SRAMSize = 256 * 1024; break;
        case 0x05: SRAMSize = 512 * 1024; break;
        case 0x10: eeprom_size = 128; break;
        case 0x20: eeprom_size = 2 * 1024; break;
        case 0x50: eeprom_size = 1024; break;
    }

    /* Detective Conan hack (from libretro.c) */
    if ((header[8] | (header[9] << 8)) == 0x8de1 && (header[0] == 0x01) && (header[2] == 0x27)) {
        wsCartROM[0xfffe8] = 0xea;
        wsCartROM[0xfffe9] = 0x00;
        wsCartROM[0xfffea] = 0x00;
        wsCartROM[0xfffeb] = 0x00;
        wsCartROM[0xfffec] = 0x20;
    }

    /* Initialize subsystems */
    MDFNMP_Init(16384, (1 << 20) / 1024);
    v30mz_init(WSwan_readmem20, WSwan_writemem20, WSwan_readport, WSwan_writeport);
    WSwan_MemoryInit(MDFN_GetSettingB("wswan.language"), wsc, SRAMSize, false);
    WSwan_GfxInit();
    WSwan_SoundInit();
    wsMakeTiles();
    engine_reset();

    /* Set up input pointer */
    chee = (uint8 *)&input_buf;

    /* Allocate video surface */
    surf = (MDFN_Surface*)calloc(1, sizeof(*surf));
    if (!surf) return false;
    
    surf->width  = 224;
    surf->height = 144;
    surf->pitch  = 224;
    surf->depth  = pix_depth;
    surf->pixels = (uint16_t*)calloc(1, 224 * 144 * sizeof(uint32_t));
    if (!surf->pixels) { free(surf); surf = NULL; return false; }

    /* Allocate initial audio buffer */
    audio_samples_buf_size = ((int32_t)(target_sample_rate / 75.47) + 1) << 1;
    audio_samples_buf = (int16_t*)malloc(audio_samples_buf_size * sizeof(int16_t));
    if (!audio_samples_buf) return false;

    /* Configure pixel format and audio */
    WSwan_SetPixelFormat(pix_depth, mono_pal_start, mono_pal_end);
    WSwan_SetSoundRate((uint32)target_sample_rate);

    return true;
}

void swan_reset(void) {
    engine_reset();
}

void swan_shutdown(void) {
    WSwan_MemoryKill();
    WSwan_SoundKill();

    if (wsCartROM) { free(wsCartROM); wsCartROM = NULL; }
    if (surf) {
        if (surf->pixels) free(surf->pixels);
        free(surf);
        surf = NULL;
    }
    if (audio_samples_buf) { free(audio_samples_buf); audio_samples_buf = NULL; }
    audio_samples_buf_size = 0;
}

/* ================================================================
 * 2. Frame Stepping
 * ================================================================ */

int swan_run_frame(uint16_t input_mask) {
    EmulateSpecStruct spec;

    /* Map input bitmask to engine format */
    input_buf = input_mask;
    WSButtonStatus = chee[0] | (chee[1] << 8);

    MDFNMP_ApplyPeriodicCheats();

    /* Set up emulation spec */
    spec.surface       = surf;
    spec.DisplayRect.w = 224;
    spec.DisplayRect.h = 144;
    spec.skip          = 0;
    spec.SoundBufSize  = 0;

    /* Run CPU until one full frame completes (scanline loop) */
    while (!wsExecuteLine(spec.surface, spec.skip));

    /* Flush audio samples */
    spec.SoundBufSize = WSwan_SoundFlush(&audio_samples_buf, &audio_samples_buf_size);
    last_audio_frame_count = spec.SoundBufSize;

    v30mz_timestamp = 0;

    return spec.SoundBufSize;
}

/* ================================================================
 * 3. Media Extraction
 * ================================================================ */

const void* swan_get_video_buffer(void) {
    if (surf && surf->pixels)
        return surf->pixels;
    return NULL;
}

const int16_t* swan_get_audio_buffer(void) {
    return audio_samples_buf;
}

/* ================================================================
 * 4. Save States
 * ================================================================ */

/* Forward declaration from state.c */
int MDFNSS_SaveSM(void *st, int, int, const void*, const void*, const void*);
int MDFNSS_LoadSM(void *st, int, int);

size_t swan_get_state_size(void) {
    StateMem st;
    memset(&st, 0, sizeof(st));

    if (!MDFNSS_SaveSM(&st, 0, 0, NULL, NULL, NULL))
        return 0;

    size_t size = st.len;
    free(st.data);
    return size;
}

bool swan_save_state(uint8_t* buffer, size_t buffer_size) {
    StateMem st;
    memset(&st, 0, sizeof(st));
    
    uint8_t *tmp = (uint8_t*)malloc(buffer_size);
    if (!tmp) return false;
    
    st.data = tmp;
    st.malloced = (uint32_t)buffer_size;

    bool ret = MDFNSS_SaveSM(&st, 0, 0, NULL, NULL, NULL);
    if (ret) {
        size_t copy_size = st.len < buffer_size ? st.len : buffer_size;
        memcpy(buffer, st.data, copy_size);
    }
    free(st.data);
    return ret;
}

bool swan_load_state(const uint8_t* buffer, size_t buffer_size) {
    StateMem st;
    memset(&st, 0, sizeof(st));
    
    st.data = (uint8_t*)buffer;
    st.len = (uint32_t)buffer_size;

    return MDFNSS_LoadSM(&st, 0, 0);
}

/* ================================================================
 * 5. Battery Save (SRAM / EEPROM)
 * ================================================================ */

size_t swan_get_save_size(void) {
    if (eeprom_size)
        return eeprom_size;
    if (SRAMSize)
        return SRAMSize;
    return 0;
}

bool swan_read_save(uint8_t* buffer, size_t buffer_size) {
    size_t save_size = swan_get_save_size();
    if (save_size == 0 || buffer_size < save_size)
        return false;

    if (eeprom_size) {
        memcpy(buffer, wsEEPROM, eeprom_size);
    } else if (SRAMSize && wsSRAM) {
        memcpy(buffer, wsSRAM, SRAMSize);
    } else {
        return false;
    }
    return true;
}

bool swan_write_save(const uint8_t* buffer, size_t buffer_size) {
    size_t save_size = swan_get_save_size();
    if (save_size == 0 || buffer_size < save_size)
        return false;

    if (eeprom_size) {
        memcpy(wsEEPROM, buffer, eeprom_size);
    } else if (SRAMSize && wsSRAM) {
        memcpy(wsSRAM, buffer, SRAMSize);
    } else {
        return false;
    }
    return true;
}

/* ================================================================
 * 6. Hardware Probe
 * ================================================================ */

bool swan_is_color_mode(void) {
    return is_color;
}

int swan_get_pixel_depth(void) {
    return pix_depth;
}

int swan_get_pixel_bytes(void) {
    return pix_bytes;
}
