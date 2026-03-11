/*
 * swan_core.h
 * SwanDeltaCore
 *
 * Black-box C-API for the WonderSwan emulation engine.
 * Based on beetle-wswan-libretro (Mednafen/Cygne).
 * 
 * Output is ALWAYS 224×144. No rotation, no transformation.
 * Licensed under GPL-2.0.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* === 1. Lifecycle ========================================= */

/* Initialize the emulation engine.
 * audio_sample_rate: target sample rate (e.g. 48000.0) */
bool swan_init(double audio_sample_rate);

/* Load a ROM from memory. Parses the ROM header to determine
 * SRAM/EEPROM size and WS/WSC mode automatically.
 * rom_data: pointer to the complete ROM file in memory
 * rom_size: byte length of the ROM */
bool swan_load_rom(const uint8_t* rom_data, size_t rom_size);

/* Hard reset the emulated system */
void swan_reset(void);

/* Shut down and free all resources */
void swan_shutdown(void);

/* === 2. Frame Stepping ==================================== */

/* Execute one frame (~1/75.47 second of emulated time).
 * input_mask: physical button bitmask
 *   Bit 0-3:  X1, X2, X3, X4
 *   Bit 4-7:  Y1, Y2, Y3, Y4
 *   Bit 8:    A
 *   Bit 9:    B
 *   Bit 10:   Start
 * Returns: number of stereo audio frames produced this tick */
int swan_run_frame(uint16_t input_mask);

/* === 3. Media Extraction (Zero-Copy) ====================== */

/* Returns pointer to the 224×144 video framebuffer.
 * - Format: 32-bit XRGB8888 (or 16-bit depending on pixel depth config)
 * - Stride: 224 pixels per row
 * - Updated after each swan_run_frame() call */
const void* swan_get_video_buffer(void);

/* Returns pointer to the interleaved stereo 16-bit PCM audio buffer.
 * - Format: int16_t, left/right interleaved
 * - Length: 2 * frame_count samples (frame_count from swan_run_frame return) */
const int16_t* swan_get_audio_buffer(void);

/* === 4. Save States (for Rewind) ========================== */

/* Get the size, in bytes, of a serialized save state */
size_t swan_get_state_size(void);

/* Serialize the current state into buffer (must be >= swan_get_state_size()) */
bool swan_save_state(uint8_t* buffer, size_t buffer_size);

/* Deserialize a state from buffer */
bool swan_load_state(const uint8_t* buffer, size_t buffer_size);

/* === 5. Battery Save (SRAM / EEPROM) ====================== */

/* Get the size of the game's persistent save data (0 if none) */
size_t swan_get_save_size(void);

/* Read save data into buffer */
bool swan_read_save(uint8_t* buffer, size_t buffer_size);

/* Write save data from buffer back into the engine */
bool swan_write_save(const uint8_t* buffer, size_t buffer_size);

/* === 6. Hardware Probe ==================================== */

/* Returns true if the loaded ROM is WonderSwan Color, false for original WS */
bool swan_is_color_mode(void);

/* Get the video buffer pixel depth currently configured */
int swan_get_pixel_depth(void);

/* Get bytes per pixel for the current pixel format */
int swan_get_pixel_bytes(void);

#ifdef __cplusplus
}
#endif
