/*********************************************************************************************************
 *               Copyright(c) 2022, vcare Corporation. All rights reserved.
 *********************************************************************************************************
 * @file     vc9202_driver.h
 * @brief    vc9202 driver layer interface implementation.
 * @details  
 * @author   
 * @date
 * @version
 *********************************************************************************************************/

#ifndef __VC9202_DRIVER_H__
#define __VC9202_DRIVER_H__

#include "vc9202_common.h"

#define VC9202_STATUS_CONFLICT (1<<4)
#define VC9202_STATUS_INSAMPLE (1<<3)
#define VC9202_STATUS_OVERLOAD2 (1<<2)
#define VC9202_STATUS_OVERLOAD1 (1<<1)
#define VC9202_STATUS_OVERLOAD0 (1<<0)

#define VC9202_EVENT_TEMP_MARK (1<<6)
#define VC9202_EVENT_BIO_MARK  (1<<5)
#define VC9202_EVENT_PS_MARK   (1<<4)  
#define VC9202_EVENT_OV_MARK   (1<<3)  
#define VC9202_EVENT_FIFO_MARK (1<<2)  
#define VC9202_EVENT_ENV_MARK  (1<<1)  
#define VC9202_EVENT_PPG_MARK  (1<<0)

#define VC9202_MAX_RES_LEVEL  17  //21 9202offset只有0-2，3和2一样、所以表中只有17个等效
#define VC9202_MAX_CUR_LEVEL  0x6F

#define VC9202_ENV_MAX_TH 8*16

/* adjust marco */
#define VC9202_PPG_TH_CELL 10			 // Causes of PPG interruption PPG_TH = 300
#define VC9202_ADJUST_INCREASE 22 // 1.4 << 4 = 22.4//1.4f
#define VC9202_ADJUST_DECREASE 11 // 0.7 << 4 = 11.2//0.7f
#define VC9202_ADJUST_STEP_MAX 32
#define VC9202_ADJUST_STEP_MIN 1
#define VC9202_SLOTCHNU 3

/* bio算法运算结果宏定义 */
#define VC9202_BIO_STA_NONE 0
#define VC9202_BIO_STA_DROP 1
#define VC9202_BIO_STA_HOLD 2

/****************************************************************************
 * driver ret enum
 ***************************************************************************/
typedef enum
{
    RET_OK           = 0,
    RET_STATUS_ERR   = (1<<0),
    RET_OVERLOAD_ERR = (1<<1),
    RET_ENV_ERR      = (1<<2),
    RET_PPG_ERR      = (1<<3),
    RET_OSC_ERR      = (1<<4),
    RET_FIFO_ERR     = (1<<5),
}VC9202DrvRetTypeDef;

/****************************************************************************
 * clock calibration_struct
 ***************************************************************************/
typedef struct
{
    unsigned int last_clk_count;
    unsigned int osc_gap_cnt;
    unsigned char count_index;
    unsigned int osc_count[6];
    unsigned int clk_count[6];
	unsigned int last_clk_check; /* 2023.06.05,check running gaptime */
}VC9202OscClkTypeDef;

/****************************************************************************
 * WearStatus_struct
 ***************************************************************************/
typedef struct
{
		unsigned char wear_ps;
		unsigned char wear_env;
		short    int  wear_bio;
		unsigned char wear_hold_cnt;
		unsigned char wear_drop_cnt;
		WearStatusTypeDef wear_status;
		WearStatusTypeDef last_status;
}VC9202WearInfoTypeDef;

typedef enum
{
    WEAR_NO_CHANGE = 0,
    DROP_TO_HOLD = 1,
    HOLD_TO_DROP = 2,
}VC9202WearChangeTypeDef;

/****************************************************************************
 * ADJUST_struct
 ***************************************************************************/
typedef struct
{
	unsigned char res;
	unsigned char offset;
	unsigned char low;
}VC9202ResTableTypeDef; 

typedef struct
{
	unsigned char res_val;
	unsigned char low_res;
	unsigned char offset;
  unsigned char offset3;
}VC9202ResLevelTypeDef;

typedef enum
{
	AGC_DIR_NULL = 0,
	AGC_DIR_UP   = 1,
	AGC_DIR_DOWN = 2,
}VC9202AgcDirTypeDef;

typedef struct
{
    VC9202AgcDirTypeDef dir;
    VC9202AgcDirTypeDef last_dir;
    unsigned char current_step;
}VC9202AgcInfoTypeDef;

typedef struct
{
    unsigned char max_current;
    unsigned char max_res_level;
    unsigned char record_env;
    unsigned char slot_env;
    unsigned char current;
    unsigned char res_level;
    VC9202AgcInfoTypeDef info;
}VC9202AdjustSlotTypeDef;

/****************************************************************************
 *  Dev_struct
 ***************************************************************************/
typedef struct
{
		unsigned short fifo_time;
    unsigned char work_frequency;
		unsigned char enable_bio;
		WorkModeTypeDef work_mode;
    
    unsigned char work_status;
    unsigned int int_event;

    WearStatusTypeDef wear_status;
    unsigned char osc_clk_status;   
    unsigned char enfifo_div;
    unsigned char enslot[2];
    unsigned char slot_number;
    unsigned char read_fifo_address;
    
}VC9202DrvInfoTypeDef;

/****************************************************************************
 *  test_struct
 ***************************************************************************/
typedef struct
{
    unsigned char wait_cnt;
    unsigned char step1_ret; // step1_ret/step2_ret/step3_ret:0-wait test,1-pass,2-failed
    unsigned char icid;
    unsigned char reason;
    unsigned char leak_pre_value;
    unsigned char max_current;
    signed short temperature;
    unsigned int frequency;

    unsigned char step2_ret;
    unsigned char ps_value;
    unsigned short biovalue_max;
    unsigned short biovalue_now[2];
    unsigned short biovalue_gap[2];
} VC9202TestInfoTypeDef;


#endif
