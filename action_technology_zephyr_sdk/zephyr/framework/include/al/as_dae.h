/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Author: jpl<jianpingliao@actions-semi.com>
 *
 * Change log:
 *	2019/12/16: Created by jpl.
 */

#ifndef __AS_DAE_H__
#define __AS_DAE_H__

#include "as_dae_setting.h"

#define MAX_CHANNEL_AS_DAE_INOUT    4

/*!
 * \brief
 *      dae process input/output pcm structure
 */
typedef struct
{
	/*! [INPUT]: input/output pcm array.
	 * pcm[0] is left channel, pcm[1] is right channel, pcm[2] is mic input (single channel).
	 **/
	void *pcm[MAX_CHANNEL_AS_DAE_INOUT];
	/*! [INPUT] input pcm channels */
	int channels;
	/*! [INPUT] input pcm samples (pairs) */
	int samples;
	/*! [INPUT] input pcm sample bits, 16, 24 or 32 */
	int sample_bits;
	/*! [INPUT] input pcm fraction bits */
	int frac_bits;
} as_dae_inout_pcm_t;

/*!
 * \brief
 *      dae ops error code
 */
typedef enum
{
	/*! unexpected error */
	DAE_RET_UNEXPECTED = -3,
	/*! not enough memory */
	DAE_RET_OUTOFMEMORY,
	/*! unsupported format */
	DAE_RET_UNSUPPORTED,
	/*! no error */
	DAE_RET_OK,
	/*! data underflow */
	DAE_RET_DATAUNDERFLOW,
} as_dae_ret_t;

/*!
 * \brief
 *      dae ops command
 */
typedef enum
{
	/* open, without argument */
	DAE_CMD_OPEN = 0,
	/* close */
	DAE_CMD_CLOSE,
	/* get status */
	DAE_CMD_GET_STATUS,
	/* frame process, with argument address of structure as_dae_frame_info_t */
	DAE_CMD_FRAME_PROCESS,
	/* set dae parameter buffer address, with argument address of structure as_dae_para_info_t */
	DAE_CMD_SET_DAE_PARA,
	/* get maximum energy and RMS energy */
	DAE_CMD_GET_ENERGY,
	/* get frequency energy */
	DAE_CMD_GET_FREQ_POINT_ENERGY,
	/** get memory require size(bytes) */
	DAE_CMD_MEM_REQUIRE,
	/** get codec verison */
	DAE_CMD_GET_VERSION,
	/** enable fade, with argument address of structure as_dae_para_info_t */
	DAE_CMD_ENA_FADE,
	/* update soft volume, with argument address of structure as_dae_para_info_t */
	DAE_CMD_UPDATE_SOFT_VOLUME,
} as_dae_ex_ops_cmd_t;

typedef enum
{
	/* fade in processing*/
	DAE_FADE_IN_PROCESSING = 0,
	/* fade in standby(not working or processed)*/
	DAE_FADE_IN_STANDBY,
	/* fade out processing*/
	DAE_FADE_OUT_PROCESSING,
	/* fade out standby(not working or processed)*/
	DAE_FADE_OUT_STANDBY,

	/* mic channal fade in processing*/
	DAE_MIC_FADE_IN_PROCESSING,
	/* mic channal fade in standby(not working or processed)*/
	DAE_MIC_FADE_IN_STANDBY,
	/* mic channal fade out processing*/
	DAE_MIC_FADE_OUT_PROCESSING,
	/* mic channal fade out standby(not working or processed)*/
	DAE_MIC_FADE_OUT_STANDBY,
} as_dae_status_e;

typedef struct
{
	/* [OUTPUT]: SPEAKER fade in status */
	short fade_in_status;
	/* [OUTPUT]: SPEAKER fade out status */
	short fade_out_status;
	/* [OUTPUT]: MIC fade in status */
	short mic_fade_in_status;
	/* [OUTPUT]: MIC fade out status */
	short mic_fade_out_status;
} as_dae_status_t;

typedef struct
{
	/* [OUTPUT]: SPEAKER average energy of current frame */
	short energy;
	/* [OUTPUT]: SPEAKER maximum energy of current frame */
	short energy_max;
	/* [OUTPUT]: MIC average energy of current frame */
	short mic_energy;
	/* [OUTPUT]: MIC maximum energy of current frame */
	short mic_energy_max;
} as_dae_energy_t;


/**
 * @brief as dae operation
 *
 * This routine provides as dae operation
 *
 * @param hnd  handle of as dae
 * @param cmd  operation cmd, type of as_dae_ex_ops_cmd_t
 * 				for DAE_CMD_MEM_REQUIRE, args type is as_mem_info_t* in audio_codec.h
 * 				for DAE_CMD_OPEN, args for reserve
 * 				for DAE_CMD_SET_DAE_PARA, args type is as_dae_para_info_t* in as_dae_setting.h
 * 				for DAE_CMD_FRAME_PROCESS, args type is as_dae_inout_pcm_t*
 * 				for DAE_CMD_GET_STATUS, args type is as_dae_status_t*
 *				for DAE_CMD_GET_ENERGY, args type is as_dae_energy_t*
 *				for DAE_CMD_GET_FREQ_POINT_ENERGY, args type is short array whose size is freq_point_num*sizeof(short),
 *						freq_point_num can see freq_display_para in as_dae_setting.h, which is set by DAE_CMD_SET_DAE_PARA.
 * @param args args of dae parama addr
 *
 * @return type of as_dae_ret_t;
 */
int as_dae_ops(void *hnd, as_dae_ex_ops_cmd_t cmd, unsigned int args);

#endif /* __AS_DAE_H__ */
