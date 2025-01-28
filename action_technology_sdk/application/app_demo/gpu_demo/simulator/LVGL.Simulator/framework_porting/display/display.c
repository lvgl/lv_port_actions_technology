/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <drivers/display/display_graphics.h>

bool display_format_is_opaque(uint32_t pixel_format)
{
    switch (pixel_format) {
    case PIXEL_FORMAT_ARGB_8888:
    case PIXEL_FORMAT_BGRA_5658:
    case PIXEL_FORMAT_BGRA_6666:
    case PIXEL_FORMAT_RGBA_6666:
    case PIXEL_FORMAT_BGRA_5551:
    case PIXEL_FORMAT_I8:
    case PIXEL_FORMAT_I4:
    case PIXEL_FORMAT_I2:
    case PIXEL_FORMAT_I1:
    case PIXEL_FORMAT_A8:
    case PIXEL_FORMAT_A4:
    case PIXEL_FORMAT_A2:
    case PIXEL_FORMAT_A1:
    case PIXEL_FORMAT_A4_LE:
    case PIXEL_FORMAT_A1_LE:
        return false;
    default:
        return true;
    }
}

uint8_t display_format_get_bits_per_pixel(uint32_t pixel_format)
{
    switch (pixel_format) {
    case PIXEL_FORMAT_ARGB_8888:
    case PIXEL_FORMAT_XRGB_8888:
        return 32;
    case PIXEL_FORMAT_BGR_565:
    case PIXEL_FORMAT_RGB_565:
    case PIXEL_FORMAT_BGRA_5551:
        return 16;
    case PIXEL_FORMAT_RGB_888:
    case PIXEL_FORMAT_BGRA_5658:
    case PIXEL_FORMAT_BGRA_6666:
    case PIXEL_FORMAT_RGBA_6666:
        return 24;
    case PIXEL_FORMAT_A8:
    case PIXEL_FORMAT_I8:
        return 8;
    case PIXEL_FORMAT_A4:
    case PIXEL_FORMAT_A4_LE:
    case PIXEL_FORMAT_I4:
        return 4;
    case PIXEL_FORMAT_A2:
    case PIXEL_FORMAT_I2:
        return 2;
    case PIXEL_FORMAT_A1:
    case PIXEL_FORMAT_A1_LE:
    case PIXEL_FORMAT_I1:
    case PIXEL_FORMAT_MONO01: /* 0=Black 1=White */
    case PIXEL_FORMAT_MONO10: /* 1=Black 0=White */
        return 1;
    default:
        return 0;
    }
}

const char* display_format_get_name(uint32_t pixel_format)
{
    switch (pixel_format) {
    case PIXEL_FORMAT_ARGB_8888:
        return "ARGB_8888";
    case PIXEL_FORMAT_XRGB_8888:
        return "XRGB_8888";
    case PIXEL_FORMAT_BGR_565:
        return "RGB_565";
    case PIXEL_FORMAT_RGB_565:
        return "RGB_565_BE";
    case PIXEL_FORMAT_BGRA_5551:
        return "ARGB_1555";
    case PIXEL_FORMAT_RGB_888:
        return "RGB_888";
    case PIXEL_FORMAT_BGRA_5658:
        return "ARGB_8565";
    case PIXEL_FORMAT_BGRA_6666:
        return "ARGB_6666";
    case PIXEL_FORMAT_RGBA_6666:
        return "ABGR_6666";
    case PIXEL_FORMAT_A8:
        return "A8";
    case PIXEL_FORMAT_A4:
        return "A4";
    case PIXEL_FORMAT_A2:
        return "A2";
    case PIXEL_FORMAT_A1:
        return "A1";
    case PIXEL_FORMAT_A4_LE:
        return "A4_LE";
    case PIXEL_FORMAT_A1_LE:
        return "A1_LE";
    case PIXEL_FORMAT_I8:
        return "I8";
    case PIXEL_FORMAT_I4:
        return "I4";
    case PIXEL_FORMAT_I2:
        return "I2";
    case PIXEL_FORMAT_I1:
        return "I1";
    case PIXEL_FORMAT_MONO01: /* 0=Black 1=White */
        return "MONO_01";
    case PIXEL_FORMAT_MONO10: /* 1=Black 0=White */
        return "MONO_10";
    default:
        return "unknown";
    }
}
