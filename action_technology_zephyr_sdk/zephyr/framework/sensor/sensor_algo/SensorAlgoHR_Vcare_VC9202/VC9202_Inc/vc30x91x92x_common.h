/*********************************************************************************************************
 *               Copyright(c) 2022, vcare Corporation. All rights reserved.
 **********************************************************************************************************
 * @file     vc30x91x92x_common.h
 * @brief    configuration file for vc30x,vc91x,vc92x.
 * @details  
 * @author   
 * @date
 * @version 
 *********************************************************************************************************/
#ifndef __VC30x91x92x_COMMON_H__
#define __VC30x91x92x_COMMON_H__

#include "algo.h"
#include "spo2Algo.h"
#include "vcSportMotionIntAlgo.h"

#ifndef SUB
#define SUB(a, b) ((a) - (b))
#endif
#ifndef ABS
#define ABS(a, b) (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#endif

#define FUNC_DISABLE (0)
#define FUNC_ENABLE  (!FUNC_DISABLE)
#define WEAR_IS_DROP (0)
#define WEAR_IS_HOLD (1)

typedef enum
{
	WEAR_STA_NONE = 0,
	WEAR_STA_HOLD = 1,
	WEAR_STA_DROP = 2,
}WearStatusTypeDef;

typedef enum
{
	WORK_MODE_NONE    = 0,
	WORK_MODE_HR      = 1,
	WORK_MODE_SPO2    = 2,
	WORK_MODE_HRV     = 3,	/* 100hz������ʹ��BIO������Ҫ��֤�ܹ��õ���ʱ��Ӧ��������Ȼ���ܻᵼ�¶��жϣ����������� */
	WORK_MODE_STRESS  = 4,
	WORK_MODE_WEAR    = 5, //检测佩戴状态，活体检测
	WORK_MODE_FACTORY = 6,
	WORK_MODE_TEMPERATURE = 7,
	WORK_MODE_FPC_TEST = 8,
}WorkModeTypeDef;

typedef struct
{
	unsigned short sampling_gap_time;
	WorkModeTypeDef work_mode;
}InitParamTypeDef;

typedef struct
{
	unsigned char led_cur[3];
	unsigned char pd_res[3];
	unsigned char env_val[3];
	unsigned char pre_val[2];
	unsigned char ps_val;
	unsigned char aenv_val[3];
	unsigned char apdbias_val[4];
    unsigned int  pdbias;
	unsigned char apre_val[2];
	unsigned char aps_val;
	unsigned int ppg_data[128];
	unsigned short int bio_val_i;
	unsigned short int bio_val_o;
	signed short int temp_val_i;
	signed short int temp_val_o;
}SamplingDataTypeDef;

extern unsigned char usr_iic_write_bytes(unsigned char reg_addr, unsigned char *pBuffer, unsigned char buf_size);
extern unsigned char usr_iic_read_bytes(unsigned char reg_addr, unsigned char *pBuffer, unsigned char buf_size);
extern unsigned int usr_get_clock_ticks(void);

#endif 
