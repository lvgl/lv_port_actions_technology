/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AL_HFP_SPEECH_LIB_DEV_H__
#define __AL_HFP_SPEECH_LIB_DEV_H__

/*******************************************************************************
 * PLC structures
 ******************************************************************************/
#define PLC_GLOBAL_BUF_SIZE(samplerate_khz) \
	((samplerate_khz == 16) ? (2030 * 2) : (1071 * 2))
#define PLC_SHARE_BUF_SIZE(samplerate_khz) \
	((samplerate_khz == 16) ? (1950 * 2) : (978 * 2))

#define PLC_FRAME_SIZE(samplerate_khz) \
	((samplerate_khz == 16) ? 120 : 60)

typedef struct
{
	int* global_buffer; // 8k:1071*2bytes   16k:2030*2bytes
	int* share_buffer; // 8k:978*2bytes   16k:1950*2bytes
	int num; // samples per frame, 8k:60, 16k:120
} plc_prms_t;

typedef struct
{
	short* inoutput;
	int cnt; // number of samples
} plc_inout_t;

/*******************************************************************************
 * AEC structures
 ******************************************************************************/
typedef struct
{
	/*int aec_enable; extended param */
	char aec_enable; //aec module enable
	char aec_no_sat; //0: history mode; 1: convergence every frame
	short init_echo; // initial echo cancellation time. 0: default 40*16ms; otherwise init_echo*16ms
	int tail_length; // echo tail length in ms, mutiple of 16
	//int bulk_delay;           //bulk delay
	/*int nlp_enable; extended param */
	short nlp_enable; //nonlinear processing submodule enable. 1: enable, make sure aec_enable is 1; 0: disable
	char nlp_echolevel; //default 0
	char nlp_speed; //default 0
	int nlp_aggressiveness; //nonlinear processing level, range 0~15, the greater, the more nonlinear.
	int dt_threshold; //speech retention level in dual speech near end, range 0~15, the greater, the more retention of near end in dual speech, while the less in single speech */
	int aes_enable; // residual echo suppression submodule enabe. 1: enable, make sure aec_enable is 1; 0: disable
	int echo_suppress; //single speech echo suppression ratio of echo suppression submodule, range 0~40 dB
	int echo_suppress_avtive; //dual speech echo suppression ratio of echo suppression submodule,range 0~40 dB
} aec_prms_t;

typedef struct
{
	int ans_enable; //denoise enable
	int noise_suppress; //denoise suppression ratio, range 0~25 dB
} ns_prms_t;

typedef struct
{
	int cng_enable; //comfort noise enable
	int noise_floor; //comfort noise level, in dB Fs, less than -45 dB generally
} cng_prms_t;


#define SV_GLOBAL_BUF1_SIZE(samplerate_khz) \
	((samplerate_khz == 16) ? (10536 * 2) : (6184 * 2))
#define SV_GLOBAL_BUF2_SIZE(samplerate_khz) \
	((samplerate_khz == 16) ? (1280 * 2) : (0))
#define SV_GLOBAL_BUF_SIZE(samplerate_khz) \
	(SV_GLOBAL_BUF1_SIZE(samplerate_khz) + SV_GLOBAL_BUF2_SIZE(samplerate_khz))

#define SV_SHARE_BUF_SIZE(samplerate_khz) \
	((samplerate_khz == 16) ? (3082 * 2) : (1546 * 2))
#define SV_MDF_BUF_SIZE(samplerate_khz, tail_length) \
	((samplerate_khz == 16) ? (tail_length * (32 * 6)) : (tail_length * (16 * 6)))
#define SV_TAIL_LENGTH(samplerate_khz, mdf_bufsize) \
	((samplerate_khz == 16) ? (mdf_bufsize / (32 * 6)) : (mdf_bufsize / (16 * 6)))

#define SV_FRAME_SIZE(samplerate_khz) \
	((samplerate_khz == 16) ? 256 : 128)

typedef struct
{
	int frame_size; //frame time in ms, only support 16ms
	int sampling_rate; //sample rate in Hz, only support 8000 and 16000
	// aec prms
	aec_prms_t aec_prms;
	//ns prms
	ns_prms_t ns_prms;
	//cng prms
	cng_prms_t cng_prms;

	int analysis_mode; //test mode, 0: simulation (offline) mode; 1: phonecall mode
	int half_mode; //half duplex mode, 0: full duplex, 1: half duplex

	int* global_buffer; //8k:6184*2 bytes  16k: 10536*2 bytes
	int* global_buffer2; //8k:null  16k:1280*2 bytes
	int* share_buffer; //8k:1546*2 bytes  16k: 3082*2 bytes
	int* mdf_buffer; //8k:tail_length(ms)*16*6bytes  16k:tail_length(ms)*32*6bytes
} sv_prms_t;

typedef struct
{
	short* Txinbuf_mic;
	short* RxinBuf_spk;
	int cnt; //samples count, 16k:256, 8k:128
} aec_inout_t;

/*******************************************************************************
 * AGC structures
 ******************************************************************************/
typedef struct
{
	/*targe amplitude: 4000-24000, suggested 8000*/
	int agc_level;
	/*sample rate in Hz, onley support 8000 or 16000 */
	int sample_rate;
	/* number of channels, only suuport 1*/
	int channels;

	/*reserved for future*/
	int agc_mode;
	/* minimum gain. 0: no limit; others: lower limit of gain */
	int minimum_gain;
	/* maximum gain. 0: no limit; others: upper limit of gain */
	int maximum_gain;

	int zero_crose;

	/* hold time in ms */
	int hold_time;
	/* decay time in ms */
	int decay_time;
	/* attack time in ms */
	int attack_time;

	/* noise gate enable */
	int noise_gate_enable;
	/* noise gate threshold, range 200-1000, suggested 400 */
	int noise_gate_threshold;
	/* noise gate type. 0: fixed, 1: self adaption */
	int noise_gate_type;
} AGCSetting_t;

typedef struct
{
	short* inoutput;
	int cnt; //samples count, 16k:256, 8k:128
} agc_inout_t;

/*******************************************************************************
 * BIN para structures
 ******************************************************************************/
/* totally 60 bytes */
typedef struct as_hfp_speech_para_bin {
	aec_prms_t aec_prms; /* 32 bytes */
	ns_prms_t  ns_prms;  /* 8 bytes */
	cng_prms_t cng_prms; /* 8 bytes */

	short analysis_mode; /* test mode, 0: simulating phonecall, 1: phonecall */
	short half_mode;     /* 0: full-duplex, 1: half-duplex */

	short agc_pre_level;  /* agc level, 0-15 (10000--25000) */
	short agc_post_level; /* agc level, 0-15 (10000--25000) */
	short agc_pre_en;     /* 0: fixed gain, 1: enable agc */
	short agc_post_en;    /* 0: fixed gain, 1: enable agc */
} as_hfp_speech_para_bin_t;

#define AS_HFP_SPEECH_PARA_BIN_SIZE (sizeof(as_hfp_speech_para_bin_t))

/*******************************************************************************
 * library interface
 ******************************************************************************/
/*!
 * \brief
 *      ops error code
 */
typedef enum
{
	/*! unexpected error */
	HS_RET_UNEXPECTED = -3,
	/*! not enough memory */
	HS_RET_OUTOFMEMORY,
	/*! unsupported format */
	HS_RET_UNSUPPORTED,
	/*! no error */
	HS_RET_OK,
	/*! data underflow */
	HS_RET_DATAUNDERFLOW,
} hs_ret_t;

/*!
 * \brief
 *      plc command
 */
typedef enum
{
	HS_CMD_PLC_OPEN = 0,
	HS_CMD_PLC_CLOSE,
	HS_CMD_PLC_PROCESS_NORMAL,
	HS_CMD_PLC_PROCESS_LOSS,
} plc_ex_ops_cmd_t;

/**
 * @brief hfp plc operation
 *
 * This routine provides hfp plc operation
 *
 * @param hnd  handle hfp plc
 * @param cmd  operation cmd, type of plc_ex_ops_cmd_t
 * @param args args of hfp plc parama addr
 *
 * @return type of hs_ret_t;
 */
int hfp_plc_ops(void *handle, plc_ex_ops_cmd_t cmd, unsigned int args);

/*!
 * \brief
 *      speech command
 */
typedef enum
{
	HS_CMD_AGC_PRE_OPEN = 0,
	HS_CMD_AGC_POST_OPEN,
	HS_CMD_AEC_OPEN,
	HS_CMD_CLOSE,
	HS_CMD_AGC_PROCESS,
	HS_CMD_AEC_PROCESS,
	HS_CMD_AEC_GETSTASTUS,
	HS_CMD_CNG_PROCESS,
	HS_CMD_CONNECT,
} hfp_ex_ops_cmd_t;

/**
 * @brief hfp speech operation
 *
 * This routine provides hfp speech operation
 *
 * @param hnd  handle of hfp speech
 * @param cmd  operation cmd, type of hfp_ex_ops_cmd_t
 * @param args args of hfp speech parama addr
 *
 * @return type of hs_ret_t;
 */
int hfp_speech_ops(void *handle, hfp_ex_ops_cmd_t cmd, unsigned int args);

#endif /* __AL_HFP_SPEECH_LIB_DEV_H__ */
