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

#ifndef __AS_DAE_SETTING_H__
#define __AS_DAE_SETTING_H__

#define AS_DAE_EQ_NUM_BANDS  (20)
#define AS_DAE_PARA_BIN_SIZE (sizeof(((as_dae_para_info_t*)0)->aset_bin))

typedef struct
{
	// [INPUT]: sample rate in Hz
	int sample_rate;
	// [INPUT]: address of library internal frame buf
	//   dae_buf_addr[0-1]: global buffer, size equal to block_size * 4, aligned to 4 bytes.
	//   dae_buf_addr[2]: share buffer, size equal to block_size * 2, only accessed in
	//               DAE_CMD_FRAME_PROCESS for 1 channel pcm data
	int dae_buf_addr[3];
	// [INPUT]: dae process main switch
	short dae_enable;
	// [INPUT]: dae process channels
	short channels;
	// [INPUT]: dae process block size in sample (pairs)
	short block_size;
	// [INPUT]: dae process bypass switch.
	//   If set 1, all effect will not work except pre_soft_volume, soft_volume, fade in/out, and output_channel_config
	short bypass;
	// [INPUT]: pre soft volume in 0.1 dB. set 0 to bypass it.
	short pre_soft_volume;
	// [INPUT]: soft volume in 0.1 dB. set 0 to bypass it.
	short soft_volume;
	// [INPUT]: fade in time in ms.
	short fade_in_time_ms;
	// [INPUT]: fade out time in ms.
	short fade_out_time_ms;
	// [INPUT]: fade flag. 1: fade in; 2: fade out
	short fade_flag;
	// [INPUT]: dae process output config
	// 0: do nothing: 1: swap left-right channel; 2: mix left-right channel (L=R=L/2 + R/2)
	// 3: only left channel (R=L); 4: only right channel (L=R)
	short output_channel_config;
	// [INPUT]: dae process sample bits. only 16 supported so far
	short bit_width;
	short reserve_0;
} music_common_para_t;

typedef struct
{
	short freq_display_enable;
	short freq_point_num;
	short freq_point[10];
} freq_display_para_t;

/*for DAE AL*/
typedef struct
{
	music_common_para_t music_common_para;
	freq_display_para_t freq_display_para;

	char aset_bin[1024];
} as_dae_para_info_t;

#endif /* __AS_DAE_SETTING_H__ */
