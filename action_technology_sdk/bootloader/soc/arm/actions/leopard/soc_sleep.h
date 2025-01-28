/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sleep for Actions SoC
 */

#ifndef	_ACTIONS_SOC_SLEEP_H_
#define	_ACTIONS_SOC_SLEEP_H_

enum S_WK_SRC_TYPE {
	SLEEP_WK_SRC_BT = 0x00,
	SLEEP_WK_SRC_GPIO,
	SLEEP_WK_SRC_PMU,
	SLEEP_WK_SRC_T0,
	SLEEP_WK_SRC_T1,
	SLEEP_WK_SRC_T2,
	SLEEP_WK_SRC_T3,
	SLEEP_WK_SRC_TWS,
	SLEEP_WK_SRC_SPI0MT,
	SLEEP_WK_SRC_SPI1MT,
	SLEEP_WK_SRC_IIC0MT,
	SLEEP_WK_SRC_IIC1MT,
};

enum WK_RUN_TYPE {
	WK_RUN_IN_SRAM = 0x00,   /*only sram can use*/
	WK_RUN_IN_NOR,  		 /*only sram & nor can use */
	WK_RUN_IN_SYTEM,         /*only sram nor psram can use */
	WK_RUN_MAX,
};

enum WK_CB_RC{
	WK_CB_SLEEP_AGAIN = 0x00,
	WK_CB_RUN_SYSTEM,
};


typedef enum WK_CB_RC (*sleep_wk_callback_t)(enum S_WK_SRC_TYPE wk_src);
typedef enum WK_RUN_TYPE (*sleep_wk_prepare_t)(enum S_WK_SRC_TYPE wk_src);

struct sleep_wk_fun_data {
	sleep_wk_callback_t wk_cb;
	sleep_wk_prepare_t	wk_prep;
	struct sleep_wk_fun_data *next;
};

enum S_WK_SRC_TYPE sys_s3_wksrc_get(void);
void sys_s3_wksrc_set(enum S_WK_SRC_TYPE src);
int sleep_register_wk_callback(enum S_WK_SRC_TYPE wk_src, struct sleep_wk_fun_data *fn_data);
int sleep_sensor_code_set(void *code_addr, uint32_t code_len);

#endif /* _ACTIONS_SOC_SLEEP_H_	*/
