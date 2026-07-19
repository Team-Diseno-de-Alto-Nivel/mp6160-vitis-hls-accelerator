#pragma once

/*
 * Expected image format: RAW RGB 1080p, no header, row-major.
 * Change here to test with other resolutions.
 *
 * Shared by the SystemC model (C++) and the ARM64 driver (C) — see
 * memory_map.h for why these are #define rather than constexpr.
 */

#include <stdint.h>

#define IMAGE_WIDTH        1920u
#define IMAGE_HEIGHT       1080u
#define IMAGE_PIXEL_COUNT  ((uint64_t)IMAGE_WIDTH * (uint64_t)IMAGE_HEIGHT)
#define IMAGE_BYTES_RGB    (IMAGE_PIXEL_COUNT * 3) /* input:  3 bytes/pixel RGB */
#define IMAGE_BYTES_GRAY   (IMAGE_PIXEL_COUNT)     /* output: 1 byte/pixel grayscale */
