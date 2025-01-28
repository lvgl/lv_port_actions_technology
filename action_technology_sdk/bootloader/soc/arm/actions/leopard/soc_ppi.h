/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral reset configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_PPI_H_
#define	_ACTIONS_SOC_PPI_H_

/******************************************************************************/
//constants
/******************************************************************************/
// selection of ppi channel which wants to configure
enum ppi_ch_cfg_sel {
	PPI_CH0 = 0,
	PPI_CH1,
	PPI_CH2,
	PPI_CH3,
	PPI_CH4,
	PPI_CH5,
	PPI_CH6,
	PPI_CH7,
	PPI_CH8,
	PPI_CH9,
	PPI_CH10,
	PPI_CH11
};

// selection of task which links to ppi channel
enum ppi_ch_cfg_task_sel {
	SPIMT0_TASK0 = 0,
	SPIMT0_TASK1,
	SPIMT0_TASK2,
	SPIMT0_TASK3,
	SPIMT0_TASK4,
	SPIMT0_TASK5,
	SPIMT0_TASK6,
	SPIMT0_TASK7,
	SPIMT1_TASK0,
	SPIMT1_TASK1,
	SPIMT1_TASK2,
	SPIMT1_TASK3,
	SPIMT1_TASK4,
	SPIMT1_TASK5,
	SPIMT1_TASK6,
	SPIMT1_TASK7,
	I2CMT0_TASK0,
	I2CMT0_TASK1,
	I2CMT0_TASK2,
	I2CMT0_TASK3,
	I2CMT1_TASK0,
	I2CMT1_TASK1,
	I2CMT1_TASK2,
	I2CMT1_TASK3,
};

// selection of trigger source which acts on ppi channel
enum ppi_trig_src_sel {
	TIMER0 = 0,
	TIMER1,
	TIMER2,
	TIMER3,
	TIMER4,
	IO0_IRQ,
	IO1_IRQ,
	IO2_IRQ,
	IO3_IRQ,
	IO4_IRQ,
	IO5_IRQ,
	IO6_IRQ,
	IO7_IRQ,
	IO8_IRQ,
	IO9_IRQ,
	IO10_IRQ,
	IO11_IRQ,
	SPIMT0_TASK0_CIP,
	SPIMT0_TASK1_CIP,
	SPIMT0_TASK2_CIP,
	SPIMT0_TASK3_CIP,
	SPIMT0_TASK4_CIP,
	SPIMT0_TASK5_CIP,
	SPIMT0_TASK6_CIP,
	SPIMT0_TASK7_CIP,
	SPIMT1_TASK0_CIP,
	SPIMT1_TASK1_CIP,
	SPIMT1_TASK2_CIP,
	SPIMT1_TASK3_CIP,
	SPIMT1_TASK4_CIP,
	SPIMT1_TASK5_CIP,
	SPIMT1_TASK6_CIP,
	SPIMT1_TASK7_CIP,
	I2CMT0_TASK0_CIP,
	I2CMT0_TASK1_CIP,
	I2CMT0_TASK2_CIP,
	I2CMT0_TASK3_CIP,
	I2CMT1_TASK0_CIP,
	I2CMT1_TASK1_CIP,
	I2CMT1_TASK2_CIP,
	I2CMT1_TASK3_CIP,
};


/******************************************************************************/
//functions
/******************************************************************************/
/**
 * @brief Init PPI module.
 *
 * @param N/A
 *
 * @return N/A
 */
void ppi_init(void);

/**
 * @brief Enable ppi trigger source.
 *
 * @param trig_src The trigger source.
 * @param enable   Enable or disable.
 *
 * @return N/A
 */
void ppi_trig_src_en(int trig_src, int enable);

/**
 * @brief Configure the attribute of a ppi channel.
 *
 * @param ppi_channel The channel which wants to configure. See enum ppi_ch_cfg_sel 
 *										for more details.
 * @param task_select The task which links to this ppi channel. See enum ppi_ch_cfg_task_sel 
 *										for more details.
 * @param trig_src_select The trigger source which wants to acts on this ppi channel. 
 *												See enum ppi_trig_src_sel for more details.
 *
 * @return N/A
 */
void ppi_task_trig_config(int ppi_channel, int task_select, int trig_src_select);

/**
 * @brief Check if ppi trigger source has pending.
 *
 * @param ppi_trig_src The trigger source which wants to check. See enum 
											 ppi_trig_src_sel for more details.
 *
 * @return 1: pending		0: no pending
 */
int ppi_trig_src_is_pending(int ppi_trig_src);

/**
 * @brief Clear ppi trigger source pending.
 *
 * @param ppi_trig_src The trigger source pending which wants to clear. 
 *										 See enum ppi_trig_src_sel for more details.
 *
 * @return N/A
 */
void ppi_trig_src_clr_pending(int ppi_trig_src);

#endif /* _ACTIONS_SOC_PPI_H_	*/
