/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TOOL_APP_INNER_H__
#define __TOOL_APP_INNER_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <zephyr.h>
#include <stub_command.h>
#include <stub.h>
#include <usp_protocol.h>
#include <app_defines.h>
#include <app_manager.h>
#include <app_switch.h>
#include <os_common_api.h>
#include <config.h>
#include "config_al.h"
#include <aset.h>
#include <drivers/hrtimer.h>
#include <sys/ring_buffer.h>
#include <thread_timer.h>

#include "tool_app.h"

typedef struct {
	usp_handle_t usp_handle;
    const struct device *dev;
    // struct k_timer timer;
    struct hrtimer timer;
    os_timer ttimer;

	// os_sem init_sem;

	char *stack;
	uint16_t quit : 1;
	uint16_t quited : 1;
	uint16_t type : 8;
	uint16_t init_work : 1;
	uint16_t init_printk_hw : 1;

	uint8_t dev_type;
	uint8_t connect;

    struct ring_buf rx_ringbuf;
    uint8_t *rx_fbuf;

    uint8_t *temp_buff;
	struct k_work work;
	uint8_t type_buf[8];
} act_tool_data_t;

typedef enum {
	sNotReady = 0, /* not ready or after user stop */
	sReady,        /* parameter filled, or configuration file selected */
	sRunning,      /* debugging and running normally */
	sUserStop,     /* user stop state */
	sUserUpdate,   /* user update the parameter */
	sUserStart,    /* user testing state */
	sUserSetParam  /* download the parameter to the board */
} PC_curStatus_e;

/*!
 * \brief 配置工具协议命令
 */
enum CONFIG_PROTOCOL_COMMANDS
{
    CONFIG_PROTOCOL_CMD_ADJUST_CFG = 's',  //!< 调节配置参数 0x73
    CONFIG_PROTOCOL_CMD_ASQT_START = 'q',  //!< 开始 ASQT 通话调试 0x71
    CONFIG_PROTOCOL_CMD_ASQT_STOP  = 'p',  //!< 停止 ASQT 通话调试 0x70
    CONFIG_PROTOCOL_CMD_ASQT_FRAME = 'f',  //!< ASQT 数据帧 0x66
    CONFIG_PROTOCOL_CMD_ANC_START = 0x7a,  //!< 开始dump anc数据
    CONFIG_PROTOCOL_CMD_ANC_STOP  = 0x7b,  //!< 结束dump anc数据
};


extern act_tool_data_t g_tool_data;



// static inline struct stub_device *tool_stub_dev_get(void)
// {
//	 return g_tool_data.dev_stub;
// }

static inline int tool_is_quitting(void)
{
	return g_tool_data.quit;
}

void tool_aset_loop(void *p1, void *p2, void *p3);
void tool_att_loop(void *p1, void *p2, void *p3);
void tool_spp_test_main(void *p1, void *p2, void *p3);
void tool_asqt_loop(void);
void tool_ectt_loop(void);


/*******************************************************************************
 * ASQT specific structure
 ******************************************************************************/
typedef struct {
	uint16_t eq_point_nums;                    /* maximum PEQ point num */
	uint16_t eq_speaker_nums;                  /* speaker PEQ point num */
	/* PEQ version. 0: speaker and mic share one view; 1: speaker and mic use different views */
	uint32_t eq_version;
	uint8_t fw_version[8];                     /* 285A100, with line ending "0" */
	uint8_t sample_rate;                       /* 0: 8kHz, 1: 16kHz */
	uint8_t reserve[15];                       /* reserved */
} asqt_interface_config_t;

typedef struct {
	int16_t sMaxVolume;
	int16_t sVolume;
	int16_t sAnalogGain;
	int16_t sDigitalGain;
	int16_t sRev[4];
} asqt_stControlParam;

typedef struct {
	int16_t sMaxVolume;
	int16_t sAnalogGain;
	int16_t sDigitalGain;
	int16_t sRev[28];
	int16_t sDspDataLen;
} asqt_stControl_BIN;

/* maximum data streams to upload, only 6 at present */
#define ASQT_MAX_OPT_COUNT  10

typedef struct {
	uint8_t bSelArray[ASQT_MAX_OPT_COUNT];
} asqt_ST_ModuleSel;

/*******************************************************************************
 * ASET specific structure
 ******************************************************************************/
typedef enum {
	UNAUX_MODE = 0,
	AUX_MODE   = 1,
} aux_mode_e;

typedef struct {
	uint8_t aux_mode; /* aux_mode_e */
	uint8_t reserved[7];
} aset_application_properties_t;

typedef struct {
	uint8_t state;          /* 0: not start 1:start */
	uint8_t upload_data;    /* 0: not upload 1:upload */
	uint8_t volume_changed; /* 0:not changed 1:changed */
	uint8_t download_data;  /* 0:not download 1:download */

	uint8_t upload_case_info;    /* 0:not upload 1:upload */
	uint8_t main_switch_changed; /* 0:not changed 1:changed */
	uint8_t update_audiopp;      /* 0:not update 1:update */
	uint8_t dae_changed;         /* 0:not update 1:update */

	uint8_t reserved[24];
} aset_status_t;

typedef struct {
	int8_t bEQ_v_1_0;
	int8_t bVBass_v_1_0;
	int8_t bTE_v_1_0;
	int8_t bSurround_v_1_0;

	int8_t bLimiter_v_1_0;
	int8_t bMDRC_v_1_0;
	int8_t bSRC_v_1_0;
	int8_t bSEE_v_1_0;

	int8_t bSEW_v_1_0;
	int8_t bSD_v_1_0;
	int8_t bEQ_v_1_1;
	int8_t bMS_v_1_0;

	int8_t bVBass_v_1_1;
	int8_t bTE_v_1_1;

	int8_t bEQ_v_1_2;
	int8_t bMDRC_v_1_1;
	int8_t bComPressor_v_1_0;
	int8_t bMDRC_v_2_0;
	int8_t bDEQ_v_1_0;
	int8_t bNR_v_1_0;
	int8_t bUD_v_1_0;
	int8_t bEQ_v_1_3;
	int8_t bSW_v_1_0;
	int8_t bSurround_v_2_0;
	int8_t bVolumeTable_v_1_0;
	int8_t szRev[111];
	int8_t szVerInfo[8]; /* project name and version, both in upper case */
} aset_interface_config_t;

typedef struct {
	int8_t peq_point_num;      /* maximum PEQ point num */
	int8_t download_data_over; /* data read finished flag */
	int8_t aux_mode;           /* 1: aux, 0: not aux */
	int8_t b_Max_Volume;       /* maximum volume level */
	int8_t reserved[28];       /* reserved */
	aset_interface_config_t stInterface;
} aset_case_info_t;


static inline struct device* get_tool_dev_device(void)
{
	return (struct device*)g_tool_data.dev;
}

int tool_uart_read(uint8_t *data, uint32_t size, uint32_t timeout_ms);
int tool_uart_write(const uint8_t *data, uint32_t size, uint32_t timeout_ms);
int tool_uart_ioctl(uint32_t mode, void *para1, void *para2);
void tool_uart_init(uint32_t buffer_size);
void tool_uart_exit(void);
#endif /* __TOOL_APP_INNER_H__ */
