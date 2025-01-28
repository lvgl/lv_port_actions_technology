/*********************************************************************************************************
 *               Copyright(c) 2022, vcare Corporation. All rights reserved.
 **********************************************************************************************************
 * @file     vc9202_user.h
 * @brief    functional Module Layer
 * @details  The functional interface is implemented by calling the driver.contain init_func,irq_handler
 * @author   
 * @date
 * @version  
 *********************************************************************************************************/

#ifndef __VC9202_USER_H__
#define __VC9202_USER_H__

#include "vc9202_common.h"

/* gsensor struct */
typedef struct
{
	unsigned char sampling_rate;	/* 25hz,50hz */
	unsigned char sensing_range;	/* ±4G, ±8G, ±16G*/
	unsigned char effective_bits;	/* 11(12)bit, 15(16)bit */

	int gsensor_x_axis[40];
	int gsensor_y_axis[40];
	int gsensor_z_axis[40];
}GsensorTypeDef;


typedef struct
{
	short int hr_rate;
	unsigned char spo2_val;
	unsigned char hrv_results;
	unsigned char stress_results;
	signed short int temperature_value;
}AlgoCalcResultsTypeDef;


void vc9202_usr_init( InitParamTypeDef *pInitParam );
void vc9202_usr_get_chip_id( unsigned char *pchip_id, unsigned char *pvers_id );
void vc9202_usr_start_sampling(void);
void vc9202_usr_stop_sampling(void);
void vc9202_usr_soft_reset(void);
unsigned char vc9202_usr_channel_switch(unsigned char num_channel, unsigned char enable_flag);
unsigned char vc9202_usr_get_int_reason(void);
unsigned char vc9202_usr_get_wear_status(void);
void vc9202_usr_interrupt_handler( AlgoSportMode hr_alg_mode, unsigned char spo2_alg_mode );

#endif

