/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MUSIC_POSTPROCESSOR_DSP_H__
#define __MUSIC_POSTPROCESSOR_DSP_H__

typedef enum {
	L_R_NORMAL_MODE = 0,
	L_R_SWITCH_MODE = 1,
	L_R_MIX_MODE = 2,
	L_ONLY_MODE = 3,
	R_ONLY_MODE = 4,
	L_INV_MODE = 0x0100,
	R_INV_MODE = 0x0200,
} pp_output_mode_e;

typedef struct music_dae_para {
	/******0***dae config reg *******************/
	short pp_enable;                //后处理使能位,0,disable, 1,enable
	short pp_reset;             //后处理复位, 0:不做任何事情，1,复位，dsp复位完会清零，预留
	short pp_change_flag;           //后处理参数变更，预留
	short pp_status;                //后处理状态，预留
	short pp_pa_volume;            //bit 15 for volume change, bit 0-14 PA volume level
	short pp_subwoofer_volume;     //subwoofer da volume
	int   pp_da_volume;

	/******1***dae inpput pcm config reg *******************/
	int   pp_input_sample_rate;            //采样率，输入声音的采样率
	short pp_input_channels;               //通道数，目前都是按双通道处理，就是2
	short pp_input_bit_depth;              //位宽，目前是16bit，预留
	short pp_input_channel_layout;         //通道组织方式，比方说左右交织（目前），还是左一块，右一块，预留
	short pp_input_block_size;             //帧长，目前为可变长度，预留
	short pp_input_config_reserve[2];      //预留做功能升级使用

	/*****2****dae output pcm config reg *******************/
	int   pp_output_sample_rate;            //采样率，预留
	short pp_output_channels;               //通道数，目前都是按双通道处理，预留
	short pp_output_bit_depth;              //位宽，目前是16bit，预留
	short pp_output_channel_layout;         //通道组织方式，比方说左右交织（目前），还是左一块，右一块，预留
	short pp_output_block_size;             //帧长，目前为可变长度
	short pp_output_mode_config;            //输出配置，预留
	short pp_output_config_reserve[1];      //预留做功能升级使用;

	/****3*****dae post process before(such as ennergy caculation,TimeDomainEnergy) config reg *******************/
	short tde_enable;      //预留
	short tde_version;     //预留
	short tde_mean;         //mean,
	short tde_max;          //pcm abs max value
	short tde_reserve[4];   //预留

	/**************freq spetrum display*********************/
	short fsd__enable;
	short fsd__version;
	short duration_ms;                  //统计时长，单位ms
	short num_band;                     //频段数,算法内部最多支持10段
	short f_c[12];                      //带通中心频率
	short energys[12];                  //计算出来的频段能量，供方案端读!!!

	/**************Multi-freq band energy*********************/
	short mfbe_enable;
	short mfbe_version;
	short num_freq_point;               //频点数,算法内部最多支持12点
	short mfbe_Reserve0;
	short freq_point[16];               //default:0 各频点数组，单位Hz
	short freq_point_mag[8];            //每个频点只占用一个byte，计算出来的频点能量, [0, 157], 取负值代表能量的db值, 值越大，能量越大, 0代表最大值，-157代表最小值

	/****4****dae post process end(such as fade in/out and mute) config reg *******************/
	short pp_end_version;
	short fade_in_flag;                 //淡入标志位，置1表示下一帧开始淡入
	short fade_in_time_ms;              //淡入长度，[50 100 200 300 400 500]ms
	short fade_out_flag;                //淡出标志位，置1表示下一帧开始淡出
	short fade_out_time_ms;             //淡出长度，[50 100 200 300 400 500]ms
	short mute_flag;                    //静音标志位，置1表示下一帧开始静音
	short mute_time_ms;                 //静音长度，长度没有限制，单位ms
	short pp_end_reserve[1];   //预留

	/****5* resever such as debug info ****/
	//Pre-cut模块后，有一个交叉混音的模块，即Lout = Lin * a + Rin * b, Rout=Lin * c + Rin * d, 其中a,b,c,d为-1~1之间的系数，为简单起见，可以a=d，b=c
	short pp_ext_mode; //0 for disable, 1 for cross mix, others TBD
	short cross_mix_a; //[-1,1),0.15 signed format, 32767 for 0.999.., -32768 for -1.0
	short cross_mix_b; //[-1,1),0.15 signed format, 32767 for 0.999.., -32768 for -1.0
	short cross_mix_c; //[-1,1),0.15 signed format, 32767 for 0.999.., -32768 for -1.0
	short cross_mix_d; //[-1,1),0.15 signed format, 32767 for 0.999.., -32768 for -1.0
	short PostProcess_ext_reserve[11];

	/**********dae module infomation from tools or bin file *******************/
	short dae_enable;                   //1:enable, 0:disable for bypass
	short dae_reset;                    //1:need reset, 0: no need reset or finish reset
	short dae_change_flag;              //1: has been changed, 0: not changed
	short dae_para_info_len;            //len of dae para info, unit:byte
	void  *dae_para_info_addr_mcu;      //address for dae para info at mcu side
	void  *dae_para_info_addr_dsp;      //address for dae para info at mcu side
	short dae_IP_vendor;                //indicate dae ip vendor, 0 for acts, 1,....
	short dae_config_info_reserve[7];   //reserve

	//dae_para_info_t dae_para_info;
	char dae_para_info_array[1024];    //0x60400
} music_dae_para_t;

#endif /* __MUSIC_POSTPROCESSOR_DSP_H__ */
