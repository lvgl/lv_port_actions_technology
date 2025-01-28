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

#ifndef __AS_FADE_P_H__
#define __AS_FADE_P_H__

typedef struct {
	unsigned int sample_rate;
	unsigned int fade_in_time_ms;
	unsigned int fade_out_time_ms;
} as_fade_p_init_t;

//typedef struct {
//	/* [OUTPUT] global memory block length array, 3 blocks at maximum */
//	int global_buf_len[3];
//	/* [OUTPUT] share memory block length array, 3 blocks at maximum */
//	int share_buf_len[3];
//	/* [INPUT] extended parameters */
//	void* param; //here is (as_mix_p_init_t *)&as_mix_p_open_t.as_mix_p_init
//} as_mem_info_t;

typedef struct {
	as_fade_p_init_t as_fade_p_init;
	as_mem_info_t as_mem_info;
	/*! [INPUT] global memory block address array */
	void *global_buf_addr[3];
	/*! [INPUT] share memory block address array */
	void *share_buf_addr[3];
} as_fade_p_open_t;

#define MAX_CHANNEL_AS_FADE_P_INOUT    2

/*!
 * \brief
 *      fade frame process structure
 */
typedef struct {
	/*! [INPUT] input/output pcm array.
	 * For interweaved pcm, set channels as 2, pcm_in_out[0] as pcm buffer address, and pcm_in_out[1] as NULL
	 */
	void *pcm_in_out[MAX_CHANNEL_AS_FADE_P_INOUT];
	/*! [INPUT] input pcm channels */
	int channels;
	/*! [INPUT] input pcm samples (pairs) */
	int samples;
	/*! [INPUT] input pcm sample bits, 16 or 32 */
	int sample_bits;
	/*! [INPUT] input pcm fraction bits */
	int frac_bits;
} as_fade_p_pcm_t;

/*!
 * \brief
 *      fade ops error code
 */
typedef enum {
	/*! unexpected error */
	AS_FADE_P_RET_UNEXPECTED = -3,
	/*! not enough memory */
	AS_FADE_P_RET_OUTOFMEMORY,
	/*! unsupported format */
	AS_FADE_P_RET_UNSUPPORTED,
	/*! no error */
	AS_FADE_P_RET_OK,
	/*! data underflow */
	AS_FADE_P_RET_DATAUNDERFLOW,
} as_fade_p_ret_t;

/*!
 * \brief
 *      fade ops command
 */
typedef enum {
	/* open, with argument structure as_fade_p_open_t */
	AS_FADE_P_CMD_OPEN = 0,
	/* close */
	AS_FADE_P_CMD_CLOSE,
	/* start fade in, whose argument is fade in time in ms, if set 0 or negative, then use initial value set in AS_FADE_P_CMD_OPEN */
	AS_FADE_P_CMD_SET_FADE_IN,
	/* start fade out, whose argument is fade out time in ms, if set 0 or negative, then use initial value set in AS_FADE_P_CMD_OPEN
	 * 1) if fade in started before, then fade out is delayed until fade in completely
	 * 2) after fade out, pcm output of AS_FADE_P_CMD_FRAME_PROCESS is always 0, that is, slient.
	 */
	AS_FADE_P_CMD_SET_FADE_OUT,
	/* get fade out status. 0: fade out not started yet; 1: processing fade out; 2: fade out completely */
	AS_FADE_P_CMD_GET_FADE_OUT_STATUS,
	/* frame processing with argument, with argument as_fade_p_inout_pcm_t */
	AS_FADE_P_CMD_FRAME_PROCESS,
	/** get memory require size(bytes) */
	AS_FADE_P_CMD_MEM_REQUIRE,
	/** get codec verison */
	AS_FADE_P_CMD_GET_VERSION,
} as_fade_p_ex_ops_cmd_t;

/**
 * @brief as dae fade operation
 *
 * This routine provides as dae fade operation
 *
 * @param hnd handle of as dae fade
 * @param cmd operation cmd, type of as_dae_ex_ops_cmd_t
 * 				for AS_FADE_P_CMD_MEM_REQUIRE, args type is as_mem_info_t* in audio_codec.h
 * 				for AS_FADE_P_CMD_OPEN, args type is as_fade_p_open_t*
 * 				for AS_FADE_P_CMD_FRAME_PROCESS, args type is as_fade_p_pcm_t*
 * @param args args of dae fade parama addr
 *
 * @return type of as_fade_p_ret_t;
 */
int as_fade_p_ops(void *hnd, as_fade_p_ex_ops_cmd_t cmd, unsigned int args);

#endif /* __AS_FADE_P_H__ */
