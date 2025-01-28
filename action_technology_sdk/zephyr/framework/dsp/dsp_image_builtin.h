/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSP_BUILTIN_IMAGES_H__
#define __DSP_BUILTIN_IMAGES_H__

#include <stddef.h>

#define DSP_IMAGE_BEGIN(name) __##name##_dsp_begin
#define DSP_IMAGE_END(name) __##name##_dsp_end
#define DSP_IMAGE_SIZE(name) ((size_t)(__##name##_dsp_end - __##name##_dsp_begin))

#define DECLARE_DSP_IMAGE(name) \
	extern const char __##name##_dsp_begin[]; \
	extern const char __##name##_dsp_end[];

DECLARE_DSP_IMAGE(admp3)
DECLARE_DSP_IMAGE(adAAC)
DECLARE_DSP_IMAGE(adSBC)
DECLARE_DSP_IMAGE(audiopp)
DECLARE_DSP_IMAGE(hfp)
DECLARE_DSP_IMAGE(sv_al)

#endif /* __DSP_BUILTIN_IMAGES_H__ */
