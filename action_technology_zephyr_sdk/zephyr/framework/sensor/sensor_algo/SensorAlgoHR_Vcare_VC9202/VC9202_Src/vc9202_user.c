/*********************************************************************************************************
 *               Copyright(c) 2022, vcare Corporation. All rights reserved.
 **********************************************************************************************************
 * @file     vc9202_user.c
 * @brief    functional Module Layer
 * @details  The functional interface is implemented by calling the driver.contain init_func,irq_handler
 * @author   
 * @date
 * @version  
 *********************************************************************************************************/
#include "algo.h"
#include "spo2Algo.h"
#include "vcSportMotionIntAlgo.h"
#include "vc9202_common.h"
#include "vc9202_user.h"
#include "vc9202_driver.h"
#include <sensor_hal.h>

static  WorkModeTypeDef work_mode = WORK_MODE_NONE;
 AlgoCalcResultsTypeDef vc9202_alg_results = {0};

/* 蓝牙发送原始数据接口 */
//extern void vc9202_debug_send_heart_rate_data( SamplingDataTypeDef* presult ,GsensorTypeDef* paxis, unsigned char heart_rate, unsigned char algo_mode,  unsigned char ppgsize );
//extern void vc9202_debug_send_spo2_data( SamplingDataTypeDef* presult ,GsensorTypeDef* paxis, unsigned char spo2_val,  unsigned char ppgsize );

/*********************************************************************************************************
 * PERIPHERAL FUNCTIONS
 ********************************************************************************************************/
#define CHIP_I2C_ADDRESS 0x33
/**
  * @brief  iic多字节写
  * @param
  *      @arg reg_addr:寄存器地址
  *      @arg pBuffer:缓冲区指针
  *      @arg num_bytes:写的字节数
  * @retval  无
  */
unsigned char usr_iic_write_bytes(unsigned char reg_addr, unsigned char *pBuffer, unsigned char buf_size)
{
	sensor_hal_write(ID_HR, reg_addr, pBuffer, buf_size);
	return 0;
}


/**
  * @brief  iic多字节读
  * @param
  *      @arg reg_addr:寄存器地址
  *      @arg pBuffer:缓冲区指针
  *      @arg num_bytes:读的字节数
  * @retval  无
  */
unsigned char usr_iic_read_bytes(unsigned char reg_addr, unsigned char *pBuffer, unsigned char buf_size)
{
	sensor_hal_read(ID_HR, reg_addr, pBuffer, buf_size);   
	return 0;
}


/**
  * @brief  校验用时钟tick获取
  * @param  无
  * @retval 时钟tick
  */
unsigned int usr_get_clock_ticks(void)
{
	return sensor_hal_get_30K_counter(ID_HR);
}

/*********************************************************************************************************
 * API FUNCTIONS
 ********************************************************************************************************/
/**
  * @brief  获取chip_id
  * @param  无
  * @retval chip_id值
  */
void vc9202_usr_get_chip_id( unsigned char *pchip_id, unsigned char *pvers_id )
{
	usr_iic_read_bytes( 0x00, pchip_id, 1 );
	usr_iic_read_bytes( 0x40, pvers_id, 1 );
}

/**
  * @brief  初始化
  * @param  无
  * @retval 无
  */
void vc9202_usr_init( InitParamTypeDef *pInitParam )
{
	memset( &vc9202_alg_results, 0 ,sizeof(vc9202_alg_results) );
	spo2AlgoInit();
	Algo_Init();
	vcSportMotionAlgoInit();
	work_mode = (WorkModeTypeDef)pInitParam->work_mode;
	vc9202_drv_soft_reset();
	vc9202_drv_reg_init( pInitParam );
}

/**
  * @brief  开始采样
  * @param  无
  * @retval 无
  */
void vc9202_usr_start_sampling(void)
{
	vc9202_drv_start_sampling();
}


/**
  * @brief  停止采样
  * @param  无
  * @retval 无
  */
void vc9202_usr_stop_sampling(void)
{
	vc9202_drv_stop_sampling();
}


/**
  * @brief  软复位
  * @param  无
  * @retval 无
  */
void vc9202_usr_soft_reset(void)
{
	vc9202_drv_soft_reset();
}

/**
  * @brief  独立使/失能某一物理通道
  * @param
         @num_channel 通道编号(1,2,3)
         @enable_flag 使能或失能标志位
  * @retval 0-成功，其他-失败
  */
unsigned char vc9202_usr_channel_switch(unsigned char num_channel, unsigned char enable_flag)
{
	return vc9202_drv_channel_switch(num_channel, enable_flag);
}

/**
  * @brief  获取中断原因
  * @param @无
         @ int_reason 中断原因
  * @retval 中断原因
  */
unsigned char vc9202_usr_get_int_reason(void)
{
	/* bit2:fifo,bit4:ps */
	return vc9202_drv_get_int_reason();
}
/**
  * @brief  获取佩戴检测结果
  * @param @无
         @ 无
  * @retval 佩戴检测结果
  */
unsigned char vc9202_usr_get_wear_status(void)
{
	unsigned char sta = 0;
	WearStatusTypeDef retSta = vc9202_drv_get_wear_status();
	if( WEAR_STA_DROP == retSta )
	{
		sta = 0;
	}
	else
	{
		sta = 1;
	}
	return sta;
}

/**********************************************************************/

#define ENABLE_GSENSOR 0
/* gsensor 相关的示例代码 */
#if ENABLE_GSENSOR
typedef struct
{
	int16_t AXIS_X;
	int16_t AXIS_Y;
	int16_t AXIS_Z;
} AxesRaw_t;
static void gsensor_data_match(AxesRaw_t *org_data, GsensorTypeDef *pGsensorParam, unsigned char ppg_size)
{
	const unsigned char arry10[] = {0, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12};
	const unsigned char arry20[] = {0, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
	/* ppglen = 10 抽样 */
	if( 10<=ppg_size )
	{
		for (uint8_t i = 0; i < ppg_size; i++)
		{
			pGsensorParam->gsensor_x_axis[i] = org_data[arry10[i]].AXIS_X;
			pGsensorParam->gsensor_y_axis[i] = org_data[arry10[i]].AXIS_Y;
			pGsensorParam->gsensor_z_axis[i] = org_data[arry10[i]].AXIS_Z;
		}
	}
	else /* ppglen = 20 、25抽样 */
	{
		for (uint8_t i = 0; i < ppg_size; i++)
		{
			pGsensorParam->gsensor_x_axis[i] = org_data[arry20[i]].AXIS_X;
			pGsensorParam->gsensor_y_axis[i] = org_data[arry20[i]].AXIS_Y;
			pGsensorParam->gsensor_z_axis[i] = org_data[arry20[i]].AXIS_Z;
		}
	}
}
#endif

static void get_gsensor_data( GsensorTypeDef *pGsensorParam, unsigned char *pgsensor_len, unsigned char ppg_size )
{
	/*
	Gsensor精度要求 ReadGsensorFIFO(+-4G11bit or +-8G12bit 256/g)
	一个fifo周期为800ms,20组ppg数据，需要获取800ms内至少获取ppgCount组gsensor数据(40ms*20)，用于匹配20组ppg信号，此函数需要根据用户实际的gsensor进行匹配处理。
	*/
#if ENABLE_GSENSOR
	unsigned char i = 0;
	unsigned char gsensor_len = 0;
	AxesRaw_t gsensor_buff[40] = {0};
	pGsensorParam->sampling_rate = 25;
	pGsensorParam->sensing_range = 4;
	pGsensorParam->effective_bits = 12;
	// stk8321_get_fifo_data(&gsensor_len, gsensor_buff);
	if (ppg_size == 0)
	{
		*pgsensor_len = gsensor_len;
		/* 不抽样 */
		for (i = 0; i < *pgsensor_len; i++)
		{
			pGsensorParam->gsensor_x_axis[i] = gsensor_buff[i].AXIS_X;
			pGsensorParam->gsensor_y_axis[i] = gsensor_buff[i].AXIS_Y;
			pGsensorParam->gsensor_z_axis[i] = gsensor_buff[i].AXIS_Z;
		}
	}
	/* 此demo中gsensor的采样频率高于ppg采样，所以正常情况下，gsensor的数量一定是大于ppg的数据。特殊情况需要考虑ppg数量大于gsensor数量。需要对gsensor数据进行补长 */
	if( (gsensor_len>=ppg_size)&&(ppg_size!=0) )
	{
		*pgsensor_len = ppg_size;
		// gsensor_data_match( gsensor_buff,pGsensorParam, ppg_size );
	}
	else if( (gsensor_len<ppg_size)&&(gsensor_len!=0) ) /* gsensor不足补全 */
	{
		*pgsensor_len = gsensor_len;
		for( i=0; i<gsensor_len; i++ )
		{
			pGsensorParam->gsensor_x_axis[i] = gsensor_buff[i].AXIS_X;
			pGsensorParam->gsensor_y_axis[i] = gsensor_buff[i].AXIS_Y;
			pGsensorParam->gsensor_z_axis[i] = gsensor_buff[i].AXIS_Z;
		}
		for( i=gsensor_len; i<ppg_size; i++ )
		{
			pGsensorParam->gsensor_x_axis[i] = gsensor_buff[gsensor_len-1].AXIS_X;
			pGsensorParam->gsensor_y_axis[i] = gsensor_buff[gsensor_len-1].AXIS_Y;
			pGsensorParam->gsensor_z_axis[i] = gsensor_buff[gsensor_len-1].AXIS_Z;
		}
	}
#endif
}

static unsigned int vc9202_usr_gsensor_actuating_quantity(int x_axis, int y_axis, int z_axis)
{
	unsigned int ret_atc = 0;
	unsigned int abs_sum_current = 0;
	static unsigned int abs_sum_last = 0;
	if(x_axis < 0)
	{
		x_axis = -x_axis;
	}
	if(y_axis < 0)
	{
		y_axis = -y_axis;
	}
	if(z_axis < 0)
	{
		z_axis = -z_axis;
	}
	abs_sum_current = x_axis + y_axis + z_axis;
	if(abs_sum_last&&abs_sum_current)
	{
		if( ABS(abs_sum_last,abs_sum_current)>=40 )
		{
			ret_atc = 1;
		}
	}
	abs_sum_last = abs_sum_current;
	/* 存在运动量时处理，释放掉心率算法摘下的状态 */
	if( ret_atc )
	{
		vc9202_drv_set_algo_wear_status( WEAR_STA_HOLD );
	}
	vc9202_drv_set_gsensor_actuating_quantity( ret_atc );
	return ret_atc;
}

static short int alg_calculate_hr_handler( short int last_hr_val, SamplingDataTypeDef *pdata, unsigned char ppg_size, GsensorTypeDef *pgsensor,  AlgoSportMode hr_alg_mode )
{
	unsigned char i=0;
	short int heart_rate_value = last_hr_val;
	AlgoInputData_t alg_i_data={0};
	AlgoOutputData_t alg_o_data={0};
	/* 如果没有fifo则直接推出，此处return需要返回上一次的值 */
	if( ppg_size <= 0 )
	{
		return heart_rate_value;
	}
	if(ppg_size>=40)
	{
		ppg_size = 40;
	}
	alg_i_data.envSample = pdata->env_val[0];
	for( i=0; i<ppg_size; i++ )
	{
		alg_i_data.axes.x    = pgsensor->gsensor_x_axis[i];
		alg_i_data.axes.y    = pgsensor->gsensor_y_axis[i];
		alg_i_data.axes.z    = pgsensor->gsensor_z_axis[i];
		alg_i_data.ppgSample = pdata->ppg_data[i];
		Algo_Input( &alg_i_data, 1000/25, hr_alg_mode, 1, 1 );
	}
	Algo_Output( &alg_o_data );
	if( -1 == alg_o_data.hrData )
	{
		vc9202_drv_set_algo_wear_status( WEAR_STA_DROP );
	}
	else if( alg_o_data.hrData > 0 )
	{
		heart_rate_value = alg_o_data.hrData; 	
	}
	return heart_rate_value;
}


static unsigned char alg_calculate_spo2_handler( unsigned char last_spo2_val , SamplingDataTypeDef *pdata, unsigned char ppg_size, GsensorTypeDef *pgsensor, unsigned char spo2_alg_mode )
{
	unsigned char i=0;
	unsigned char spo2_value = last_spo2_val;
	float ret_spo2 = 0.0f;
	float ir_PPG = 0.0f;
	float red_PPG = 0.0f;
	float sport_flag = 0.0f;
	/* 如果没有fifo则直接推出，此处return需要返回上一次的值 */
	if( ppg_size<=0 )
	{
		return spo2_value;
	}
	for( i=0; i<ppg_size; i++ )
	{
		ir_PPG = (float)pdata->ppg_data[i*2];
		red_PPG  = (float)pdata->ppg_data[i*2+1];
		//ret_spo2 = vcSpo2Calculate( red_PPG, ir_PPG );
		ret_spo2 = spo2Algo( red_PPG, ir_PPG ,1);
		sport_flag = vcSportMotionCalculate( pgsensor->gsensor_x_axis[i], pgsensor->gsensor_y_axis[i], pgsensor->gsensor_z_axis[i] );
		if ( (-1!=ret_spo2)&&(!sport_flag))
		{
			spo2_value = (unsigned char)ret_spo2;
		}
	}
	return spo2_value;
}

static void factory_test_function( SamplingDataTypeDef *pdata )
{
	// /* 最大电流 */
	// unsigned char ledMaxCur = pdata->led_cur[0]; 
	// /* 获取测试数据 */
	// unsigned int pre0 = pdata->apre_val[0];
	// unsigned int ps   = pdata->aps_val;
	// /* 温度 */
	// signed short int temp0 = pdata->temp_val_i;
	// signed short int temp1 = pdata->temp_val_o;
	// /* compare abs(bio0,bio1) */
	// unsigned short int bio0 = pdata->bio_val_o;
	// unsigned short int bio1 = pdata->bio_val_o;
}
static void fpc_test_function(SamplingDataTypeDef *pdata)
{
	extern VC9202TestInfoTypeDef vc9202_test_info;
	/* 等待5次中断工作稳定 */
	if (vc9202_test_info.wait_cnt <= 5)
	{
		vc9202_test_info.wait_cnt++;
		return;
	}
	/* 步骤一：常规数据读取判断是否在范围内，此处得数据请根据实际设计对参数进行调整 */
	if (0 == vc9202_test_info.step1_ret)
	{
		if (vc9202_test_info.icid == 0x21 && vc9202_test_info.reason & 0x3f && 
			vc9202_test_info.frequency >= 16000 && vc9202_test_info.frequency <= 24000 )
		{
			vc9202_test_info.step1_ret = 1;
		}
		else
		{
			vc9202_test_info.step1_ret = 2;
		}
	}
	/* 步骤二：bio/ps数据读取判断是否在范围内 */
	else if (0 == vc9202_test_info.step2_ret)
	{
		if (vc9202_test_info.biovalue_max >= 60 && vc9202_test_info.ps_value >= 200)
		{
			vc9202_test_info.step2_ret = 1;
		}
	}
	/*注意：上述方式会一直等待，直到检测通过，也可以使用人为触发记录，进行比较两次触发时的记录值差距是否满足。
		记录两次不同状态（模拟佩戴和摘下）下时的pdata->bio_val_o，比较差值
		记录模拟佩戴下的pdata->ps_val，判断是否满足PS范围
	*/
	/* 其中step1_ret和step2_ret，分别表示测试是否通过，由于step2_ret需要检测变化量，所以需要一直等待操作，如果测试通过后，驱动会控制红绿灯交替闪烁 */
	//DEBUG_LOG("[FAT]icid=%#x,reason=%#x,frequency=%d,temperature=%d", vc9202_test_info.icid, vc9202_test_info.reason, vc9202_test_info.frequency, vc9202_test_info.temperature);
	//DEBUG_LOG("[FAT]biovalue_max=%d,ps=%d,gap[%d,%d]", vc9202_test_info.biovalue_max, vc9202_test_info.ps_value, vc9202_test_info.biovalue_gap[0], vc9202_test_info.biovalue_gap[1]);
	//DEBUG_LOG("[FAT]-ret(%d,%d)", vc9202_test_info.step1_ret, vc9202_test_info.step2_ret);
}

void vc9202_usr_interrupt_handler( AlgoSportMode hr_alg_mode, unsigned char spo2_alg_mode )
{
	unsigned char wear_status = WEAR_STA_NONE;
	SamplingDataTypeDef data = {0};
	GsensorTypeDef gsensor_data = {0};
	unsigned char gsensor_size = 0;
	unsigned char ppg_size = 0;
	vc9202_drv_get_sampling_data(&data, &ppg_size);
	switch ( work_mode )
	{
		case WORK_MODE_HR:
			if( vc9202_usr_get_int_reason()&(VC9202_EVENT_PS_MARK|VC9202_EVENT_FIFO_MARK) )
			{
				wear_status = vc9202_usr_get_wear_status();
				get_gsensor_data(&gsensor_data, &gsensor_size, ppg_size);
				if( gsensor_size>0 )
				{
					vc9202_usr_gsensor_actuating_quantity( gsensor_data.gsensor_x_axis[0], gsensor_data.gsensor_y_axis[0], gsensor_data.gsensor_z_axis[0] );
				}
				if(wear_status)
				{
					vc9202_alg_results.hr_rate = alg_calculate_hr_handler(vc9202_alg_results.hr_rate, &data, ppg_size, &gsensor_data, hr_alg_mode );
					printk("vc9202 hr_rate:%d\n",vc9202_alg_results.hr_rate);
					hr_os_api.hb_handler(vc9202_alg_results.hr_rate, 0, 0);
					
					//vc9202_debug_send_heart_rate_data( &data, &gsensor_data, vc9202_alg_results.hr_rate, hr_alg_mode, ppg_size );
				}
				else
				{
					vc9202_alg_results.hr_rate = 0;
					hr_os_api.hb_handler(0, 0, 0);
					Algo_Init();
				}
			}
			break;

		case WORK_MODE_SPO2:
			if( vc9202_usr_get_int_reason()&(VC9202_EVENT_PS_MARK|VC9202_EVENT_FIFO_MARK) )
			{
				wear_status = vc9202_usr_get_wear_status();
				get_gsensor_data(&gsensor_data, &gsensor_size, ppg_size);
				if(wear_status)
				{
					vc9202_alg_results.spo2_val = alg_calculate_spo2_handler( vc9202_alg_results.spo2_val, &data, ppg_size/2, &gsensor_data, spo2_alg_mode );
					//vc9202_debug_send_spo2_data( &data, &gsensor_data, vc9202_alg_results.spo2_val, ppg_size/2 );
					printk("vc9202 spo2_val: %d\n",vc9202_alg_results.spo2_val);
				}
				else
				{
					vc9202_alg_results.spo2_val = 0;
					spo2AlgoInit();
					vcSportMotionAlgoInit();
				}
			}
			break;

		case WORK_MODE_HRV:
			if( vc9202_usr_get_int_reason()&(VC9202_EVENT_PS_MARK|VC9202_EVENT_FIFO_MARK) )
			{
				wear_status = vc9202_usr_get_wear_status();
				get_gsensor_data(&gsensor_data, &gsensor_size, ppg_size);
				if(wear_status)
				{
					vc9202_alg_results.hr_rate = alg_calculate_hr_handler( vc9202_alg_results.hr_rate, &data, ppg_size, &gsensor_data, hr_alg_mode );
					/* 校验+算法HRV */
				}
				else
				{
					vc9202_alg_results.hr_rate = 0;
					Algo_Init();
					/* 清空数据+初始化算法 */
				}
			}
			break;

		case WORK_MODE_STRESS:
			if( vc9202_usr_get_int_reason()&(VC9202_EVENT_PS_MARK|VC9202_EVENT_FIFO_MARK) )
			{
				wear_status = vc9202_usr_get_wear_status();
				get_gsensor_data(&gsensor_data, &gsensor_size, ppg_size);
				if(wear_status)
				{
					/* 校验+算法STRESS */
				}
				else
				{
					/* 清空数据+初始化算法 */
				}
			}
			break;

		case WORK_MODE_WEAR:
			if( vc9202_usr_get_int_reason() & VC9202_EVENT_PS_MARK )
			{
				wear_status = vc9202_usr_get_wear_status();
				get_gsensor_data(&gsensor_data, &gsensor_size, ppg_size);
				if( gsensor_size>0 )
				{
					vc9202_usr_gsensor_actuating_quantity( gsensor_data.gsensor_x_axis[0], gsensor_data.gsensor_y_axis[0], gsensor_data.gsensor_z_axis[0] );
				}
				/* 根据佩戴状态添加对应的处理代码 */
				if(wear_status)
				{
				}
				else
				{
				}
			}
			break;
		case WORK_MODE_FACTORY:
			factory_test_function( &data );
			break;
		case WORK_MODE_FPC_TEST:
			fpc_test_function(&data);
		break;

		default:
			break;
	}
}

#if 0
int main(int argc, char *argv[])
{
    board_driver_init();	//MCU外设初始化
    InitParamTypeDef config={400,WORK_MODE_HR};	//修改fifo间隔时间（关系到fifolength）, 设定工作模式
    vc9102_usr_init( &config );
    while(1)
    {
        if( irq_msg )
        {
						//调用时传入心率算法模式（日常、运动...）和血氧算法参数。
            vc9102_usr_interrupt_handler( algo_mode, 1 );
        }
    }
    return 0
}
#endif
