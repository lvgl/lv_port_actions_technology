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

#ifndef __AS_DAE_H_H__
#define __AS_DAE_H_H__

#include "as_dae_h_setting.h"

typedef struct
{
	/* [INPUT]: sample rate in Hz*/
	int sample_rate;
	short peq_max_num;
	short reserve;
} as_dae_h_init_t;

typedef struct
{
	as_dae_h_init_t as_dae_h_init;
	/*! [INPUT] global memory block address array */
	void *global_buf_addr[3];
	/*! [INPUT] share memory block address array */
	void *share_buf_addr[3];
} as_dae_h_open_t;

#define MAX_CHANNEL_AS_DAE_H_INOUT    2

/*!
 * \brief
 *      dae process input/output pcm structure
 */
typedef struct
{
	/*! [INPUT]: input/output pcm array. only support 1 channel so far. */
	void *pcm[MAX_CHANNEL_AS_DAE_H_INOUT];
	/*! [INPUT] input pcm channels */
	int channels;
	/*! [INPUT] input pcm samples (pairs). must not greater than 128 so far. */
	int samples;
	/*! [INPUT] input pcm sample bits, 16, 24 or 32 */
	int sample_bits;
	/*! [INPUT] input pcm fraction bits */
	int frac_bits;
} as_dae_h_inout_pcm_t;

/*!
 * \brief
 *      dae ops error code
 */
typedef enum
{
	/*! unexpected error */
	DAE_H_RET_UNEXPECTED = -3,
	/*! not enough memory */
	DAE_H_RET_OUTOFMEMORY,
	/*! unsupported format */
	DAE_H_RET_UNSUPPORTED,
	/*! no error */
	DAE_H_RET_OK,
	/*! data underflow */
	DAE_H_RET_DATAUNDERFLOW,
} as_dae_h_ret_t;

/*!
 * \brief
 *      dae ops command
 */
typedef enum
{
	/* open, with argument as_dae_h_open_t */
	DAE_H_CMD_OPEN = 0,
	/* close */
	DAE_H_CMD_CLOSE,
	/* get status */
	DAE_H_CMD_GET_STATUS,
	/* SPEAKER frame process, with argument address of structure as_structure dae_h_inout_pcm_t */
	DAE_H_CMD_FRAME_PROCESS_SPEAKER,
	/* MIC frame process, with argument address of structure as_dae_h_inout_pcm_t*/
	DAE_H_CMD_FRAME_PROCESS_MIC,
	/* set dae parameter buffer address, with argument address of structure as_dae_h_para_info_t */
	DAE_H_CMD_SET_DAE_PARA,
	/** get memory require size(bytes) */
	DAE_H_CMD_MEM_REQUIRE,
	/** get codec verison */
	DAE_H_CMD_GET_VERSION,
} as_dae_h_ex_ops_cmd_t;

/**
 * @brief as dae operation
 *
 * This routine provides as dae operation
 *
 * @param hnd  handle of as dae
 * @param cmd  operation cmd, type of as_dae_h_ex_ops_cmd_t
 * 				for DAE_H_CMD_MEM_REQUIRE, args type is as_mem_info_t* in audio_codec.h
 * 				for DAE_H_CMD_OPEN, args type is as_dae_h_open_t*
 * 				for DAE_H_CMD_SET_DAE_PARA, args type is as_dae_h_para_info_t* in as_dae_h_setting.h
 * 				for DAE_H_CMD_FRAME_PROCESS_SPEAKER, args type is as_dae_h_inout_pcm_t*
 * 				for DAE_H_CMD_FRAME_PROCESS_MIC, args type is as_dae_h_inout_pcm_t*
 * @param args args of dae parama addr
 *
 * @return type of as_dae_h_ret_t;
 */
int as_dae_h_ops(void *hnd, as_dae_h_ex_ops_cmd_t cmd, unsigned int args);

#endif /* __AS_DAE_H_H__ */
