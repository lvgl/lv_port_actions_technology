/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "sys/printk.h"

#pragma once

#define GPU_LOG_I(...) printk(__VA_ARGS__)
#define GPU_LOG_D(...) printk(__VA_ARGS__)
#define GPU_LOG_E(...) printk(__VA_ARGS__)
#define GPU_LOG_W(...) printk(__VA_ARGS__)

#ifndef GPU_VG_LITE_STRIDE_ALIGN_PX
#define GPU_VG_LITE_STRIDE_ALIGN_PX 16
#endif

#define GPU_VG_LITE_TILED VG_LITE_LINEAR
#define GPU_VG_LITE_IMAGE_MODE VG_LITE_NORMAL_IMAGE_MODE
#define GPU_VG_LITE_TRANSPARENCY_MODE VG_LITE_IMAGE_OPAQUE

#define PKG_PERSIMMON_TEXTURE_PIXEL_ALLOC_ALIGN 64
