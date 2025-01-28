/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

.section .dsp_code,"a"

.align 4
.global __admp3_dsp_begin
__admp3_dsp_begin:
.incbin "admp3.dsp"
__admp3_dsp_end:
.global __admp3_dsp_end

.align 4
.global __adAAC_dsp_begin
__adAAC_dsp_begin:
.incbin "adAAC.dsp"
__adAAC_dsp_end:
.global __adAAC_dsp_end

.align 4
.global __adSBC_dsp_begin
__adSBC_dsp_begin:
.incbin "adSBC.dsp"
__adSBC_dsp_end:
.global __adSBC_dsp_end

.align 4
.global __audiopp_dsp_begin
__audiopp_dsp_begin:
.incbin "audiopp.dsp"
__audiopp_dsp_end:
.global __audiopp_dsp_end

.align 4
.global __hfp_dsp_begin
__hfp_dsp_begin:
.incbin "hfp.dsp"
__hfp_dsp_end:
.global __hfp_dsp_end

.align 4
.global __sv_al_dsp_begin
__sv_al_dsp_begin:
.incbin "sv_al.dsp"
__sv_al_dsp_end:
.global __sv_al_dsp_end
