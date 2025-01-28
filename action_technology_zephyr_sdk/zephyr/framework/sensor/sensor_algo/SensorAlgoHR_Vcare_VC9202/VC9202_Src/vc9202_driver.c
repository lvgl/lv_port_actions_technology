/*********************************************************************************************************
 *               Copyright(c) 2022, vcare Corporation. All rights reserved.
 **********************************************************************************************************
 * @file     vc9202_driver.c
 * @brief    vc9202 driver layer interface implementation.
 * @details
 * @author
 * @date
 * @version
 *********************************************************************************************************/
#include "vc9202_common.h"
#include "vc9202_driver.h"
#include "BIODetect_V14_1.h"

#include <sys/printk.h>

/* static wear status declaration */
static int bio_result = VC9202_BIO_STA_NONE;
WearStatusTypeDef alg_wear_status = WEAR_STA_HOLD;
static unsigned int gsnesor_axis_diff = 0;
static VC9202WearInfoTypeDef dev9202_wear = {0};
/* static adjust data declaration */
static VC9202AdjustSlotTypeDef dev9202_adjust[3] = {0};
static unsigned char ppg_mark[128 / 2] = {0};
/* static device data declaration */
static VC9202DrvInfoTypeDef dev9202_info = {0};
/* static oscclk data declaration */
static VC9202OscClkTypeDef dev9202_clk = {0};

static int Bio_Max_Value = 0;
static int Bio_Max_Diff_Value = 0;
static int Bio_Min_Diff_Value = 0;
static int Bio_Old_Value = 0;
static uint8_t Bio_cnt = 0;
static uint8_t Bio_HOLD_cnt = 0;
static uint8_t BIO_HOLD_DROP_FLAG = 2;
/* global test infomation declaration  */
extern VC9202TestInfoTypeDef vc9202_test_info;
extern void vc9202_test_handler(SamplingDataTypeDef *pResult, VC9202TestInfoTypeDef *ptest_info);
extern void vc9202_test_init(VC9202TestInfoTypeDef *ptest_info);
/* res-level tab */
const static VC9202ResTableTypeDef res_table[22] = {
	/* level: 0 */ {0, 0, 1},
	/* level: 1 */ {1, 0, 1},
	/* level: 2 */ {2, 0, 1},
	/* level: 3 */ {3, 0, 1},
	/* level: 4 */ {4, 0, 1},
	/* level: 5 */ {5, 0, 1},
	/* level: 6 */ {0, 0, 0},
	/* level: 7 */ {1, 0, 0},
	/* level: 8 */ {2, 0, 0},
	/* level: 9 */ {3, 0, 0},
	/* level:10 */ {4, 0, 0},
	/* level:11 */ {5, 0, 0},
	/* level:12 */ {6, 0, 0},
	/* level:13 */ {7, 0, 0},
	/* level:14 */ {6, 1, 0}, /* +2 */
	/* level:15 */ {7, 1, 0},
	/* level:16 */ {6, 2, 0}, /* +4 */
	/* level:17 */ {7, 2, 0},
	/* level:18 */ {6, 3, 0}, /* +6 */
	/* level:19 */ {7, 3, 0},
	/* level:20 */ {6, 4, 0}, /* +8 */
	/* level:21 */ {7, 4, 0},
}; // 21-5 = 16

void vc9202_drv_set_gsensor_actuating_quantity(unsigned int axis_diff)
{
	gsnesor_axis_diff = axis_diff;
}

void vc9202_drv_set_algo_wear_status(WearStatusTypeDef status)
{
	alg_wear_status = status;
}

/********************************************************************************************************
 * 时钟校准部分
 ********************************************************************************************************/
static void vc9202_drv_clear_clk(void)
{
	dev9202_clk.osc_gap_cnt = 0;
	dev9202_clk.count_index = 0;
	dev9202_clk.last_clk_count = 0;
	dev9202_info.osc_clk_status = 0;

	dev9202_clk.last_clk_check = 0;
}
/**
  * @brief  时钟校验,与原来的VC9201系列一致
  * @param 无
  * @retval
		  0 校验正常通过
		  1 vc9202计数当次就溢出
		  2 mcu tick或vc9202后两次中断的时候有计数溢出
		  3 频率计算没有通过
  */
static unsigned char vc9202_drv_calibration_osc_clk(void)
{
	unsigned char count_vaild = 1;
	unsigned char i = 0;
	unsigned char osc_buf[4] = {0};
	unsigned int osc_tick[2] = {0};
	unsigned int clock_tick[2] = {0};
	unsigned short int calc_div[2] = {0};
	unsigned short int mean_div = 0;
	unsigned char reg_div1[2] = {0};

	
	/* 判断是否需要校准时钟，如不需则直接返回 */
	unsigned int sec_gap_cnt = VC9202_CFG_CLK_CHECK_GAPTIME * 1000 / dev9202_info.fifo_time;
	if (dev9202_info.osc_clk_status && dev9202_clk.osc_gap_cnt++ <= sec_gap_cnt)
	{	
		return 0;
	}
	/* 获取时钟计数 */
	clock_tick[0] = usr_get_clock_ticks();
	usr_iic_read_bytes(0x07, osc_buf, 4);
	clock_tick[1] = usr_get_clock_ticks();
	osc_tick[0] = (osc_buf[0] << 8) + osc_buf[1];
	osc_tick[1] = (osc_buf[2] << 8) + osc_buf[3];
	if ((osc_tick[0] > osc_tick[1]) || (clock_tick[0] > clock_tick[1]))
	{
		dev9202_clk.count_index = 0;
		printk("cc3 clk check current over\n");
		return 1;
	}
	/* 存入校准缓存 */
	dev9202_clk.osc_count[dev9202_clk.count_index * 2] = osc_tick[0];
	dev9202_clk.osc_count[dev9202_clk.count_index * 2 + 1] = osc_tick[1];
	dev9202_clk.clk_count[dev9202_clk.count_index * 2] = clock_tick[0];
	dev9202_clk.clk_count[dev9202_clk.count_index * 2 + 1] = clock_tick[1];
	dev9202_clk.count_index++;
	if (dev9202_clk.count_index >= 3)
	{
		dev9202_clk.count_index = 0;
		for (i = 0; i < 2; i++)
		{
			count_vaild = count_vaild && (ABS((dev9202_clk.clk_count[2 * i + 2] - dev9202_clk.clk_count[2 * i]), (dev9202_clk.clk_count[2 * i + 3] - dev9202_clk.clk_count[2 * i + 1])) < VC9202_CFG_CLK_CHECK_DEVIATION);
			count_vaild = count_vaild && (ABS((dev9202_clk.osc_count[2 * i + 2] - dev9202_clk.osc_count[2 * i]), (dev9202_clk.osc_count[2 * i + 3] - dev9202_clk.osc_count[2 * i + 1])) < VC9202_CFG_CLK_CHECK_DEVIATION);
			count_vaild = count_vaild && (!(dev9202_clk.clk_count[2 * i + 2] == dev9202_clk.clk_count[2 * i])); /* avoid divisor==0, diffCountIsOk set false */
		}
		if (count_vaild)
		{
			for (i = 0; i < 2; i++)
			{
				calc_div[i] = ((unsigned int)dev9202_clk.osc_count[2 * i + 2] - dev9202_clk.osc_count[2 * i]) * VC9202_CFG_CLK_CHECK_FREQUENCY / dev9202_info.work_frequency / ((unsigned int)dev9202_clk.clk_count[2 * i + 2] - dev9202_clk.clk_count[2 * i]) - 1;
				if (calc_div[i] < (20000 / dev9202_info.work_frequency * 0.75f) || calc_div[i] > (20000 / dev9202_info.work_frequency * 1.25f))
				{
					return 3;
				}
			}
			/* 比较两次计算偏差是否合理 */
			if ((calc_div[1] - calc_div[0] <= 20) || (calc_div[0] - calc_div[1] <= 20))
			{
				mean_div = (calc_div[1] + calc_div[0]) / 2;
				reg_div1[0] = mean_div >> 8;
				reg_div1[1] = mean_div;
				usr_iic_write_bytes(0x14, reg_div1, 2);
				dev9202_clk.count_index = 0;
				dev9202_clk.osc_gap_cnt = 0;
				dev9202_info.osc_clk_status = 1;
				return 0;
			}
		}
	}
	return 2;
}
/********************************************************************************************************
 * 摘配带判断部分
 ********************************************************************************************************/
/**
  * @brief  佩戴检测,bio检测计算(内部阅读：f_BIOdetect，在BIO+PS均由佩戴产生的边缘时输出2佩戴；摘下时只依赖PS边缘)
  * @param
		  bio_value bio采样值
		  ps_value  ps采样值
  * @retval bio结果
  */
static int vc9202_drv_calc_biology_sense(unsigned short int bio_value, unsigned char ps_value, unsigned char env)
{
	static unsigned char calc_ps = 0;
	static unsigned short int calc_bio = 0;
	static int Bio_Diff_Value = 0;
	int ret = 0;
	if (bio_value)
	{
		calc_bio = bio_value;
		Bio_Diff_Value = abs(Bio_Old_Value - calc_bio);
		Bio_Old_Value = calc_bio;
	}
	if (ps_value)
	{
		calc_ps = ps_value;
	}
	if ((calc_ps < 100) && (env > 4))
	{
		if (Bio_Diff_Value < VC9202_CFG_WEAR_PARAM_BIOIN * 0.2)
		{
			Bio_Max_Value = calc_bio;
			Bio_Max_Diff_Value = Bio_Max_Value - (VC9202_CFG_WEAR_PARAM_BIOIN * 0.8);
			Bio_Min_Diff_Value = Bio_Max_Value - (VC9202_CFG_WEAR_PARAM_BIOIN * 1.5);
		}
	}
	if (Bio_Max_Diff_Value)
	{
		if ((calc_bio > Bio_Max_Diff_Value) && (Bio_Diff_Value < VC9202_CFG_WEAR_PARAM_BIOIN * 0.2))
		{
			Bio_HOLD_cnt++;
			if (Bio_HOLD_cnt > 4)
			{
				BIO_HOLD_DROP_FLAG = 1;
			}
		}
		else if (calc_bio < Bio_Min_Diff_Value)
		{
			Bio_cnt++;
			if (Bio_cnt > 2)
			{
				BIO_HOLD_DROP_FLAG = 2;
			}
		}
		else
		{
			Bio_cnt = 0;
			Bio_HOLD_cnt = 0;
			BIO_HOLD_DROP_FLAG = 0;
		}
	}
	ret = f_BIOdetect(calc_bio, calc_ps, VC9202_CFG_WEAR_PARAM_PS);
	if (VC9202_BIO_STA_HOLD == ret)
	{
		vc9202_drv_set_algo_wear_status(WEAR_STA_HOLD); /* bio */
		bio_result = VC9202_BIO_STA_HOLD;
	}
	else if (VC9202_BIO_STA_DROP == ret)
	{
		bio_result = VC9202_BIO_STA_DROP;
	}
	return bio_result;
}
/**
 * @brief  佩戴检测过程及状态
 * @param  *pwear-佩戴结构体，env-环境光，ps-ps值，bio-活体值
 * @retval 识别改变结果
 */
static VC9202WearChangeTypeDef vc9202_drv_wear_status_identify(VC9202WearInfoTypeDef *pwear, unsigned char env, unsigned char ps, unsigned short int bio)
{
	VC9202WearChangeTypeDef ret_change = WEAR_NO_CHANGE;
	int ret_bio_algo = VC9202_BIO_STA_NONE;
	/* bio算法运算 */
	if (dev9202_info.enable_bio)
	{
		ret_bio_algo = vc9202_drv_calc_biology_sense(bio, ps, env);
	}
	else
	{
		ret_bio_algo = VC9202_BIO_STA_HOLD; /* 20230614 因为下面的条件把(ret_bio_algo==VC9202_BIO_STA_NONE)佩戴的条件删了,所以此处必须添加，不然在未开bio的模式下无法佩戴*/
	}
#if (FUNC_DISABLE == VC9202_CFG_WEAR_THRESHOLD_EN)
	BIO_HOLD_DROP_FLAG = 2;
#endif
#if (FUNC_DISABLE == VC9202_CFG_WEAR_BIO_EN)
	ret_bio_algo = VC9202_BIO_STA_NONE;
	BIO_HOLD_DROP_FLAG = 2;
#endif
#if (FUNC_DISABLE == VC9202_CFG_WEAR_ALG_EN)
	alg_wear_status = WEAR_STA_HOLD;
#endif
#if (FUNC_DISABLE == VC9202_CFG_WEAR_ACT_EN)
	gsnesor_axis_diff = 1;
#endif
	switch (pwear->wear_status)
	{
	case WEAR_STA_DROP:
		if ((ps >= VC9202_CFG_WEAR_PARAM_PS) && (env <= VC9202_CFG_WEAR_PARAM_ENVMIN) && (VC9202_BIO_STA_HOLD == ret_bio_algo) &&
			(WEAR_STA_HOLD == alg_wear_status) && (1 == gsnesor_axis_diff) && (BIO_HOLD_DROP_FLAG == 2))
		{
			if (--pwear->wear_hold_cnt <= 0)
			{
				pwear->wear_hold_cnt = VC9202_CFG_WEAR_HOLD_CNT;
				pwear->wear_drop_cnt = VC9202_CFG_WEAR_DROP_CNT;
				ret_change = DROP_TO_HOLD;
			}
		}
		else
		{
			pwear->wear_hold_cnt = VC9202_CFG_WEAR_HOLD_CNT;
		}
		break;

	case WEAR_STA_HOLD:
		if ((ps < VC9202_CFG_WEAR_PARAM_PS) || (env > VC9202_CFG_WEAR_PARAM_ENVMIN) || (VC9202_BIO_STA_DROP == ret_bio_algo) || (WEAR_STA_DROP == alg_wear_status) || (BIO_HOLD_DROP_FLAG == 1))
		{
			if ((WEAR_STA_DROP == alg_wear_status) || (ret_bio_algo == VC9202_BIO_STA_DROP))
			{
				pwear->wear_drop_cnt = 1;
			}
			if (--pwear->wear_drop_cnt <= 0)
			{
				pwear->wear_hold_cnt = VC9202_CFG_WEAR_HOLD_CNT;
				pwear->wear_drop_cnt = VC9202_CFG_WEAR_DROP_CNT;
				ret_change = HOLD_TO_DROP;
			}
		}
		else
		{
			pwear->wear_drop_cnt = VC9202_CFG_WEAR_DROP_CNT;
		}
		break;
	default:
		break;
	}
	return ret_change;
}

/**
 * @brief  佩戴检测及通道的开关
 * @param 无
 * @retval 佩戴检测状态
 */
WearStatusTypeDef vc9202_drv_get_wear_status(void)
{
	WearStatusTypeDef wear_status = dev9202_wear.last_status; /* last status */
	VC9202WearChangeTypeDef calc_status = WEAR_NO_CHANGE;
	unsigned char event = 0;
	unsigned char en_int = 0;
	unsigned char env_value = 0, ps_value = 0;
	//unsigned char bio_buffer[4] = {0};
	unsigned short int bio_value = 0;
	unsigned char wear_interrupt = 0;
	unsigned char slot_index = 0;
#if (VC9202_CFG_WEAR_DETECTION != FUNC_ENABLE)
	return WEAR_IS_HOLD;
#endif
	usr_iic_read_bytes(0x11, &en_int, 1);
#if 0
	usr_iic_read_bytes(0x02, &event, 1);
#else
	event = dev9202_info.int_event;
#endif
#if (FUNC_ENABLE == VC9202_CFG_DRV_BY_TIMER) /* 定时触发时，不看中断原因，可能被清掉 */
	wear_interrupt = 1;
#else
	/* 使能了PsInt，且使能PS */
	if ((event & VC9202_EVENT_PS_MARK) && (en_int & (1 << 4)))
	{
		wear_interrupt = 1;
	}
	/* 未使能PsInt下,ps_ppg同频通过fifo中断去识别摘配带 */
	if (event & VC9202_EVENT_FIFO_MARK)
	{
		wear_interrupt = 1;
	}
#endif
	/* 非心率模式，就算使能了运动量，也没有去做处理，直接置1忽略 */
	if (dev9202_info.work_mode != WORK_MODE_HR && dev9202_info.work_mode != WORK_MODE_WEAR)
	{
		gsnesor_axis_diff = 1;
	}
	if (wear_interrupt)
	{
/* 读取佩戴检测所需的数据 */
#if 0
		usr_iic_read_bytes(0x43, &env_value, 1);
		usr_iic_read_bytes(0x46, &ps_value, 1);
		usr_iic_read_bytes(0x4B, bio_buffer, 4);
		bio_value = (bio_buffer[2] << 8) + bio_buffer[3];
#else
		env_value = dev9202_wear.wear_env;
		ps_value = dev9202_wear.wear_ps;
		bio_value = dev9202_wear.wear_bio;
#endif
		/* 根据数据得出佩戴状态 */
		calc_status = vc9202_drv_wear_status_identify(&dev9202_wear, env_value, ps_value, bio_value);
		if (DROP_TO_HOLD == calc_status)
		{
			wear_status = WEAR_STA_HOLD;
			dev9202_wear.wear_status = wear_status;
			dev9202_wear.last_status = wear_status;
			InitParamTypeDef init_param = {dev9202_info.fifo_time, dev9202_info.work_mode};
			vc9202_drv_reg_init(&init_param);
		}
		else if (HOLD_TO_DROP == calc_status)
		{
			wear_status = WEAR_STA_DROP;
			dev9202_wear.wear_status = wear_status;
			dev9202_wear.last_status = wear_status;
			/* 关灯操作：获取那些ppg通道使能/记录当前使能的通道，然后关闭通道 */
			for (slot_index = 0; slot_index <= 1; slot_index++)
			{
				if (dev9202_info.enslot[slot_index])
				{
					vc9202_drv_channel_switch(slot_index, 0);
				}
			}
		} /* end of(HOLD_TO_DROP == calc_status)  */
	}
	return wear_status;
}

/********************************************************************************************************
 * IC状态读取部分，包括IC异常状态处理，常规数据的读取，ENV、PPG调整部分
 ********************************************************************************************************/
/**
 * @brief 等效电阻等级转化为寄存器参数
 * @param level 等效电阻等级
 * @retval 寄存器参数
 */
static VC9202ResLevelTypeDef vc9202_drv_res_level_to_register(unsigned char level)
{
	VC9202ResLevelTypeDef reg;
	reg.res_val = res_table[level].res;
	reg.low_res = res_table[level].low;
	if (res_table[level].offset >= 4)
	{
		reg.offset = 0;
		reg.offset3 = 1;
	}
	else
	{
		reg.offset = res_table[level].offset;
		reg.offset3 = 0;
	}
	return reg;
}

/**
 * @brief  寄存器参数转化为等效电阻等级
 * @param  寄存器参数
 * @retval 等效电阻等级
 */
static unsigned char vc9202_drv_res_register_to_level(VC9202ResLevelTypeDef *preg)
{
	unsigned char index = 0;
	unsigned char level = 0;
	VC9202ResTableTypeDef table = {0};
	if (preg->offset3 == 1)
	{
		table.offset = 4;
	}
	else
	{
		table.offset = preg->offset;
	}
	table.res = preg->res_val;
	table.low = preg->low_res;
	for (index = 0; index < sizeof(res_table) / sizeof(res_table[0]); index++)
	{
		if (res_table[index].res == table.res && res_table[index].offset == table.offset && res_table[index].low == table.low)
		{
			level = index;
			break;
		}
	}
	return level;
}

/**
 * @brief  设置通道电流参数
 * @param 无
 * @retval 无
 */
static void vc9202_drv_set_ppgSlot_current(unsigned char slot_index, unsigned char cur)
{
	unsigned char reg = 0;
	unsigned char addr = 0x17 + slot_index;
	cur = (cur > 0x7f) ? 0x7f : cur;
	usr_iic_read_bytes(addr, &reg, 1);
	reg &= (1 << 7);
	reg |= cur;
	usr_iic_write_bytes(addr, &reg, 1);
}

/**
 * @brief  设置通道电阻参数
 * @param  设置的通道号，设定的值
 * @retval 无
 */
static void vc9202_drv_set_ppgSlot_resistance(unsigned char slot_index, unsigned char level)
{
	unsigned char reg_addr = 0x1A + slot_index;
	unsigned char res_reg = 0;
	unsigned char res_ext[2] = {0};
	VC9202ResLevelTypeDef res = {0};
	level = (level > VC9202_MAX_RES_LEVEL) ? VC9202_MAX_RES_LEVEL : level;
	res = vc9202_drv_res_level_to_register(level);
	usr_iic_read_bytes(0x22, res_ext, 2);
	switch (slot_index)
	{
	case 0:
		res_ext[0] &= ~(3 << 4); /* offset */
		res_ext[0] |= (res.offset << 4);
		res_ext[0] &= ~(1 << 0); /* lowres */
		res_ext[0] |= (res.low_res << 0);
		res_ext[1] &= ~(1 << 0);
		res_ext[1] |= (res.offset3 << 0);
		break;
	case 1:
		res_ext[0] &= ~(3 << 6); /* offset */
		res_ext[0] |= (res.offset << 6);
		res_ext[0] &= ~(1 << 1); /* lowres */
		res_ext[0] |= (res.low_res << 1);
		res_ext[1] &= ~(1 << 2);
		res_ext[1] |= (res.offset3 << 2);
		break;
	default:
		break;
	}
	usr_iic_read_bytes(reg_addr, &res_reg, 1);
	res_reg &= (0x8f);
	res_reg |= (res.res_val << 4);
	usr_iic_write_bytes(reg_addr, &res_reg, 1);
	usr_iic_write_bytes(0x22, res_ext, 2);
}
/**
 * @brief  读取PPG饱和中断时的采样值
 * @param  enfifo-是否使能fifo，slot_num-通道数，slot_index-通道号， *pdata 数据指针
 * @retval 返回当前读取地址
 */
static unsigned char vc9202_drv_adjust_ppg_read(unsigned char enfifo, unsigned char slot_num, unsigned char slot_index, unsigned int *pdata)
{
	unsigned char write_index = 0, fifo_addr = 0;
	unsigned char fifo_buff[2] = {0};
	/* PPG IRQ addr */
	usr_iic_read_bytes(0x03, &write_index, 1);
	fifo_addr = write_index - slot_num * 2 + slot_index * 2;
	fifo_addr = (fifo_addr < 0x80) ? (fifo_addr + 0x80) : (fifo_addr);
	fifo_addr = enfifo ? fifo_addr : 0x80 + slot_index * 2;
	/* read PPG data */
	usr_iic_read_bytes(fifo_addr, fifo_buff, 2);
	*pdata = (fifo_buff[0] << 8) + fifo_buff[1];
	return write_index;
}

/**
 * @brief  参数调整时，添加调整标志位
 * @param 无
 * @retval 无
 */
static void vc9202_drv_adjust_set_fifo_mark(unsigned char slot_index) // SamplingDataTypeDef **pp_result, unsigned char rindex, unsigned char windex, unsigned char slot_num )
{
	unsigned char mark_ppg_index = 0;
	unsigned char mark_offset = 0;
	unsigned char rindex = dev9202_info.read_fifo_address;
	unsigned char windex = 0;
	usr_iic_read_bytes(0x03, &windex, 1);
	/* calc PPGOffset */
	if (windex > rindex)
	{
		mark_offset = (windex - rindex);
	}
	else
	{
		mark_offset = windex + 128 - rindex;
	}
	if ((mark_offset < 2) || (mark_offset > 128))
	{
		return;
	}
	mark_ppg_index = mark_offset / 2 - dev9202_info.slot_number + slot_index; /* 如果是双通道，要判断是那个通道产生的调整 */
	ppg_mark[mark_ppg_index] = 1;
}

/**
 * @brief  检测因电阻过大，电流的最小步长调整都会反复饱和的情况
 * @param  dir 调整方向， slot_index 当前通道， res_level 当前电阻等级， cur 当前电流值
 * @retval 1:错误，0：正常
 */
static unsigned char vc9202_drv_adjust_res_error(VC9202AgcDirTypeDef dir, unsigned char slot_index, uint8_t res_level, uint8_t cur)
{
	static VC9202AgcDirTypeDef lastDir = AGC_DIR_NULL;
	static uint8_t last_level = 0;
	static uint8_t last_cur[2][3] = {0};
	static uint8_t log_cnt = 0;
	if (lastDir != dir && last_level == res_level)
	{
		last_cur[slot_index][0] = last_cur[slot_index][1];
		last_cur[slot_index][1] = last_cur[slot_index][2];
		last_cur[slot_index][2] = cur;
		if (last_cur[slot_index][0] == last_cur[slot_index][2])
			log_cnt++;
	}
	else
	{
		log_cnt = 0;
	}
	lastDir = dir;
	last_level = res_level;
	if (log_cnt >= 3)
	{
		log_cnt = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}

/**
 * @brief  PPG饱和时，上/下调整电流电阻配置参数
 * @param  *padjust-调整结构体，thh_res-电阻限值，thh_cur-电流限值，slot_index-通道编号
 * @retval 执行结果
 */
static VC9202DrvRetTypeDef vc9202_drv_adjust_ppg_slot_current_res_config(VC9202AdjustSlotTypeDef *padjust, unsigned char thh_res, unsigned char thh_cur, unsigned char slot_index)
{
	unsigned char old_res_lvl = 0;
	unsigned char old_current = 0;
	unsigned short int old_current_step = 0;
	old_res_lvl = padjust->res_level;
	old_current = padjust->current;
	old_current_step = padjust->info.current_step;
	/* 电流已经最大，调电阻 */
	if (padjust->current == thh_cur)
	{
		if (old_res_lvl == thh_res)
		{
			return RET_PPG_ERR;
		}
		else
		{
			if (padjust->info.dir == AGC_DIR_UP)
			{
				padjust->res_level = (old_res_lvl >= VC9202_MAX_RES_LEVEL) ? (VC9202_MAX_RES_LEVEL) : (old_res_lvl + 1);
			}
			else
			{
				padjust->res_level = (old_res_lvl < 1) ? (0) : (old_res_lvl - 1);
			}
			vc9202_drv_adjust_set_fifo_mark(slot_index);
		}
	}
	else /* 电流可调 */
	{
		/* step calc */
		if (padjust->info.last_dir == AGC_DIR_NULL)
		{
			old_current_step *= 16;
		}
		else if (padjust->info.dir == padjust->info.last_dir)
		{
			if ((old_current_step == 1) || (old_current_step == 2))
			{
				old_current_step = (old_current_step + 1) * 16;
			}
			else
			{
				old_current_step *= VC9202_ADJUST_INCREASE;
			}
		}
		else
		{
			old_current_step *= VC9202_ADJUST_DECREASE;
		}
		padjust->info.current_step = old_current_step >> 4;
		padjust->info.current_step = (padjust->info.current_step <= VC9202_ADJUST_STEP_MIN) ? VC9202_ADJUST_STEP_MIN : padjust->info.current_step;
		padjust->info.current_step = (padjust->info.current_step >= VC9202_ADJUST_STEP_MAX) ? VC9202_ADJUST_STEP_MAX : padjust->info.current_step;
		/* 调整方向 */
		if (padjust->info.dir == AGC_DIR_UP)
		{
			padjust->current = ((old_current + padjust->info.current_step) > thh_cur) ? thh_cur : (old_current + padjust->info.current_step);
		}
		else
		{
			padjust->current = (old_current < padjust->info.current_step) ? thh_cur : old_current - padjust->info.current_step;
		}
		vc9202_drv_adjust_set_fifo_mark(slot_index);
		padjust->info.last_dir = padjust->info.dir;
	}
	if (vc9202_drv_adjust_res_error(padjust->info.dir, slot_index, padjust->res_level, padjust->current))
	{
		if (padjust->res_level >= 13)
		{
			padjust->res_level--;
		}
	}
	/* 调整后的电流电阻写入寄存器 */
	vc9202_drv_set_ppgSlot_resistance(slot_index, padjust->res_level);
	vc9202_drv_set_ppgSlot_current(slot_index, padjust->current);
	return RET_OK;
}

/**
 * @brief  环境光饱和中断调整电阻
 * @param  *padjust-调整结构体，slot_index-通道编号
 * @retval 执行结果
 */
static VC9202DrvRetTypeDef vc9202_drv_adjust_env_configure(VC9202AdjustSlotTypeDef *padjust, unsigned char slot_index)
{
	if (padjust->slot_env > VC9202_ENV_MAX_TH)
	{
		padjust->record_env = padjust->slot_env;
		if (padjust->res_level != 0)
		{
			padjust->res_level--;
			padjust->max_res_level = padjust->res_level;
			vc9202_drv_adjust_set_fifo_mark(slot_index);
			vc9202_drv_set_ppgSlot_resistance(slot_index, padjust->res_level);
		}
		else
		{
			return RET_ENV_ERR;
		}
	}
	return RET_OK;
}

/**
 * @brief  如果产生overload,降低电流，和电流的最大限制，过载电流-1
 * @param  *padjust-调整结构体，slot_index-通道编号，status-状态值
 * @retval 执行结果
 */
static VC9202DrvRetTypeDef vc9202_drv_adjust_over_load_configure(VC9202AdjustSlotTypeDef *padjust, unsigned char slot_index, unsigned char status)
{
	unsigned char overload[3] = {status & VC9202_STATUS_OVERLOAD0, status & VC9202_STATUS_OVERLOAD1, status & VC9202_STATUS_OVERLOAD2};
	if (overload[slot_index])
	{
		if (padjust->current != 0)
		{
			padjust->current--;
			padjust->max_current = padjust->current;
			vc9202_drv_adjust_set_fifo_mark(slot_index);
			vc9202_drv_set_ppgSlot_current(slot_index, padjust->current);
		}
		else
		{
			return RET_OVERLOAD_ERR;
		}
	}
	return RET_OK;
}

/**
 * @brief  获取PPG进行电流电阻的调整
 * @param  *padjust-调整结构体，slot_index-通道编号，enfifo-是否使能fifo， slot_num-通道数
 * @retval 执行结果
 */
static VC9202DrvRetTypeDef vc9202_drv_adjust_ppg_configure(VC9202AdjustSlotTypeDef *padjust, unsigned char slot_index, unsigned char enfifo, unsigned char slot_num)
{
	static unsigned char last_index = 0, cnt = 0;
	VC9202DrvRetTypeDef vc_ret = RET_OK;
	unsigned char windex = 0;
	unsigned char index_gap = 0;
	unsigned int data = 0;
	// 根据通道号和是否enfifo获取产生ppg中断的ppg值。
	windex = vc9202_drv_adjust_ppg_read(enfifo, slot_num, slot_index, &data);
	/* 修改last dir */
	if (enfifo)
	{
		index_gap = (windex > last_index) ? (windex - last_index) : (windex - last_index + 0x80);
		if (index_gap != slot_num * 2)
		{
			padjust->info.last_dir = AGC_DIR_NULL;
		}
		cnt++;
		if (cnt >= slot_num)
		{
			cnt = 0;
			last_index = windex;
		}
	}
	/* 根据ppg值进行电流、等效电阻上下调节 */
	if (data >= 3770)
	{
		padjust->info.dir = AGC_DIR_UP;
		vc_ret |= vc9202_drv_adjust_ppg_slot_current_res_config(padjust, padjust->max_res_level, padjust->max_current, slot_index);
	}
	else if (data < 320)
	{
		padjust->info.dir = AGC_DIR_DOWN;
		vc_ret |= vc9202_drv_adjust_ppg_slot_current_res_config(padjust, 0, 0, slot_index);
	}
	return vc_ret;
}

/**
 * @brief  最大电阻的调整，通过判断环境光，来恢复电阻的最大限制
 * @param  *padjust-调整结构体，slot_index-通道编号
 * @retval 无
 */
static void vc9202_drv_res_max_resize(VC9202AdjustSlotTypeDef *padjust, unsigned char slot_index)
{
	static unsigned short int count_qares[2] = {0};
	static unsigned short int count_pdres[2] = {0};
	unsigned char min_env = 0;
	// 该通道得最大电阻已经是最大的等级了，无需恢复退出
	if (padjust->max_res_level >= VC9202_MAX_RES_LEVEL)
	{
		return;
	}
	/* 获取一个环境光比较值，取上一次环境光调整时的一半或者环境光中断阈值 */
	min_env = (padjust->record_env / 2 < VC9202_ENV_MAX_TH) ? (padjust->record_env / 2) : VC9202_ENV_MAX_TH;
	if (padjust->slot_env < min_env)
	{
		/* 环境光一直小于1，持续超过30次，那么直接将电阻限制设置为最大 */
		if (padjust->slot_env <= 1)
		{
			count_pdres[slot_index] = 0;
			count_qares[slot_index] = (count_qares[slot_index] == 30) ? 0 : count_qares[slot_index] + 1;
			padjust->max_res_level = (count_qares[slot_index] == 30) ? VC9202_MAX_RES_LEVEL : padjust->max_res_level;
		}
		else
		{
			/* 环境光一直小于比较值，持续超过120次，那么直接将电阻限制+1 */
			count_qares[slot_index] = 0;
			count_pdres[slot_index] = count_pdres[slot_index] + 1;
			padjust->max_res_level = (count_pdres[slot_index] == 120) ? padjust->max_res_level + 1 : padjust->max_res_level;
		}
	}
	else
	{
		count_pdres[slot_index] = 0;
		count_qares[slot_index] = 0;
	}
}

/* 判断PS通道的过载和环境光是否异常 */
VC9202DrvRetTypeDef vc9202_drv_ps_slot_status(VC9202AdjustSlotTypeDef *p_slot_ps, unsigned char event, unsigned char status)
{
	VC9202DrvRetTypeDef vc_ret = RET_OK;
#if (FUNC_ENABLE == VC9202_CFG_DRV_BY_TIMER) /* 定时触发时，不看中断原因，因为可能被清掉 */
	if (status & VC9202A_STATUS_OVERLOAD2)
	{
		vc_ret |= RET_OVERLOAD_ERR;
	}
	if (p_slot_ps->slot_env > ENV_MAX_TH)
	{
		vc_ret |= RET_ENV_ERR;
	}
#else
	if ((event & VC9202_EVENT_OV_MARK) && (status & VC9202_STATUS_OVERLOAD2))
	{
		vc_ret |= RET_OVERLOAD_ERR;
	}
	if ((event & VC9202_EVENT_ENV_MARK) && (p_slot_ps->slot_env > VC9202_ENV_MAX_TH))
	{
		vc_ret |= RET_ENV_ERR;
	}
#endif
	return vc_ret;
}
/**
 * @brief  识别IC的中断状态及中断事件，并对不同的情况进行调整处理等
 * @param  status-工作状态， event-中断事件
 * @retval 执行结果
 */
static VC9202DrvRetTypeDef vc9202_drv_status_event_verify(unsigned char status, unsigned char event)
{
	VC9202DrvRetTypeDef vc_ret = RET_OK;
	unsigned char index = 0;
	unsigned char index_slot = 0;
	if (status & (VC9202_STATUS_CONFLICT | VC9202_STATUS_INSAMPLE))
	{
		return RET_STATUS_ERR;
	}
	/* PS通道异常判断 */
	vc_ret |= vc9202_drv_ps_slot_status(&dev9202_adjust[2], event, status);
	/* PPG通道的环境光读取，异常中断处理 */
	for (index = 0; index < dev9202_info.slot_number; index++)
	{
		index_slot = (dev9202_info.slot_number == 2) ? index : (dev9202_info.enslot[0] ? 0 : 1);
		/* 恢复最大电阻的限制，根据当前 */
		vc9202_drv_res_max_resize(&dev9202_adjust[index_slot], index_slot);
		/* over load, max current */
		if (event & VC9202_EVENT_OV_MARK)
		{
			vc_ret |= vc9202_drv_adjust_over_load_configure(&dev9202_adjust[index_slot], index_slot, status);
		}
		/* env, max res */
		if (event & VC9202_EVENT_ENV_MARK)
		{
			vc_ret |= vc9202_drv_adjust_env_configure(&dev9202_adjust[index_slot], index_slot);
		}
		/* ppg interruput */
		if (event & VC9202_EVENT_PPG_MARK)
		{
			vc_ret |= vc9202_drv_adjust_ppg_configure(&dev9202_adjust[index_slot], index_slot, dev9202_info.enfifo_div, dev9202_info.slot_number);
		}
		else
		{
			dev9202_adjust[index].info.dir = AGC_DIR_NULL;
			dev9202_adjust[index].info.last_dir = AGC_DIR_NULL;
		}
	}
	return vc_ret;
}

/********************************************************************************************************
 * fifo 读取部分
 ********************************************************************************************************/
/**
 * @brief  校验fifo读写的偏移的可用性，及fifo是否翻转等错误。
 * @param  rindex-读地址，windex-写地址，slot_num-通道数量，fifo_div-fifo深度
 * @retval 无
 */
static VC9202DrvRetTypeDef vc9202_drv_fifo_infomation_check(unsigned char *rindex, unsigned char *windex, unsigned char slot_num, unsigned char fifo_div)
{
	unsigned int clk_diffs = 0;
	unsigned int clk_count = usr_get_clock_ticks();
	/* 间隔时间的校验,后续看是否需要补充 */
	clk_diffs = (dev9202_clk.last_clk_count < clk_count) ? (clk_count - dev9202_clk.last_clk_count) : (clk_count - dev9202_clk.last_clk_count + VC9202_CFG_CLK_CHECK_MAX);
	if (clk_diffs > (unsigned int)((64.0f / dev9202_info.work_frequency) * 0.8f * VC9202_CFG_CLK_CHECK_FREQUENCY / dev9202_info.slot_number))
	{
		*windex = 0;
		*rindex = 0x80;
		dev9202_clk.last_clk_count = clk_count;
		return RET_FIFO_ERR;
	}
	/* 读写地址的校验 */
	if (*windex < 0x80)
	{
		*rindex = *rindex + (fifo_div + 1) * 2;
		if (*rindex < 0x80)
		{
			*rindex += 0x80;
		}
		dev9202_clk.last_clk_count = clk_count;
		return RET_FIFO_ERR;
	}
	if (*rindex < 0x80)
	{
		*rindex = *windex;
		dev9202_clk.last_clk_count = clk_count;
		return RET_FIFO_ERR;
	}
	if (*windex == *rindex)
	{
		dev9202_clk.last_clk_count = clk_count;
		return RET_FIFO_ERR;
	}
	dev9202_clk.last_clk_count = clk_count;
	return RET_OK;
}

/**
 * @brief  获取fifos数据
 * @param  enfifo-是否使能fifo，p_result-数据地址，psize-数据大小，rindex-读地址，windex-写地址，slot_num-通道数量
 * @retval 无
 */
static void vc9202_drv_read_fifo_data(unsigned char enfifo, SamplingDataTypeDef *p_result, unsigned char *psize, unsigned char *rindex, unsigned char *windex, unsigned char slot_num)
{
	unsigned char i = 0;
	unsigned char fifo_length = 0;
	unsigned char fifoData[128] = {0};
	unsigned char fifo_index_offset = 0; /*正常情况下：r=w, 异常情况下，r=w-fifo_index_offset */
	/*------------- disable fifo -------------*/
	if (!enfifo)
	{
		*psize = slot_num;
		usr_iic_read_bytes(0x80, fifoData, slot_num * 2);
		for (i = 0; i < *psize; i++)
		{
			p_result->ppg_data[i] = (fifoData[i * 2] << 8) + fifoData[i * 2 + 1];
			if (ppg_mark[i])
			{
				ppg_mark[i] = 0;
				p_result->ppg_data[i] |= 0x1000; // 0x01000000;
			}
		}
		return;
	}
	/*------------- enable fifo -------------*/
	/* 读取解析fifo数据 */
	if (*windex > *rindex)
	{
		fifo_length = *windex - *rindex;
		if ((fifo_length % (dev9202_info.slot_number * 2)) && (dev9202_info.slot_number >= 2)) /* 当是多通道，但是通道数与字节数对应不上的异常处理 */
		{
			fifo_index_offset = fifo_length % (dev9202_info.slot_number * 2);
			fifo_length = fifo_length - fifo_index_offset; /* 只读取满足通道数的fifo数量，如果未出现异常，fifo_index_offset=0，跟之前处理一样 */
		}
		usr_iic_read_bytes(*rindex, fifoData, fifo_length);
		*rindex = *windex - fifo_index_offset;
	}
	else
	{
		fifo_length = *windex + 0x80 - *rindex;
		if ((fifo_length % (dev9202_info.slot_number * 2)) && (dev9202_info.slot_number >= 2)) /* 当是多通道，但是通道数与字节数对应不上的异常处理 */
		{
			fifo_index_offset = fifo_length % (dev9202_info.slot_number * 2);
			fifo_length = fifo_length - fifo_index_offset; /* 只读取满足通道数的fifo数量 */
		}
		if (*windex == 0x80) /* 数据刚好在0xfe-0xff截至 */
		{
			usr_iic_read_bytes(*rindex, fifoData, fifo_length);
			if (fifo_index_offset)
			{
				*rindex = *windex + 0x80 - fifo_index_offset;
			}
			else
			{
				*rindex = *windex - fifo_index_offset; /* fifo_index_offset=0 */
			}
		}
		else /* (*windex != 0x80): 0x80及以后还有数据 */
		{
			usr_iic_read_bytes(*rindex, fifoData, (256 - *rindex));									  /* 获取rindex-0xff的数据 */
			usr_iic_read_bytes(0x80, &fifoData[256 - *rindex], (*windex - 0x80) - fifo_index_offset); /* 剩下的数据部分如果存在 */
			*rindex = *windex - fifo_index_offset;
		}
	}

	*psize = fifo_length / 2;
	for (i = 0; i < *psize; i++)
	{
		p_result->ppg_data[i] = (fifoData[i * 2] << 8) + fifoData[i * 2 + 1];
		if (ppg_mark[i])
		{
			ppg_mark[i] = 0;
			p_result->ppg_data[i] |= 0x1000; // 0x01000000;
		}
	}
}

/**
 * @brief  校验fifo并获取结果
 * @param  p_result-数据地址，psize-数据大小
 * @retval 执行结果
 */
static VC9202DrvRetTypeDef vc9202_drv_read_fifo_result(SamplingDataTypeDef *p_result, unsigned char *psize)
{
	VC9202DrvRetTypeDef vc_ret = RET_OK;
	unsigned char write_index = 0;
	unsigned char event = 0;
	event = dev9202_info.int_event;
#if (VC9202_CFG_DRV_BY_TIMER == FUNC_ENABLE)
	evnet = EVENT_FIFO_MARK;
#endif
	if (event & VC9202_EVENT_FIFO_MARK)
	{
		usr_iic_read_bytes(0x03, &write_index, 1);
		vc_ret |= vc9202_drv_fifo_infomation_check(&dev9202_info.read_fifo_address, &write_index, dev9202_info.slot_number, dev9202_info.enfifo_div);
		if (RET_OK == vc_ret && dev9202_info.osc_clk_status)
		{
			vc9202_drv_read_fifo_data(dev9202_info.enfifo_div, p_result, psize, &dev9202_info.read_fifo_address, &write_index, dev9202_info.slot_number);
		}
	}
	return vc_ret;
}

/*********************************************************************************************************/
/**
 * @brief  读取相关采样值和寄存器值
 * @param 无
 * @retval 无
 */
static unsigned char vc9202_drv_read_reg(SamplingDataTypeDef *read_reg)
{
	unsigned char reg_val[6] = {0};
	unsigned char res_ext[2] = {0};
	VC9202ResLevelTypeDef res = {0};

	/* 4bit数据读取 */
	usr_iic_read_bytes(0x04, reg_val, 3);
	read_reg->pre_val[0] = reg_val[0] & 0x0F;
	read_reg->pre_val[1] = reg_val[1] & 0x0F;
	read_reg->env_val[0] = (reg_val[0] >> 4) & 0x0F;
	read_reg->env_val[1] = (reg_val[1] >> 4) & 0x0F;
	read_reg->env_val[2] = (reg_val[2] >> 4) & 0x0F;
	read_reg->ps_val = reg_val[2] & 0x0F;

	/* old param */
	usr_iic_read_bytes(0x17, reg_val, 6);
	read_reg->led_cur[0] = reg_val[0] & 0x7F;
	read_reg->led_cur[1] = reg_val[1] & 0x7F;
	read_reg->led_cur[2] = reg_val[2] & 0x7F;
	read_reg->pd_res[0] = (reg_val[3] >> 4) & 0x07;
	read_reg->pd_res[1] = (reg_val[4] >> 4) & 0x07;
	read_reg->pd_res[2] = (reg_val[5] >> 4) & 0x07;
	/* 8bit数据读取 */
	usr_iic_read_bytes(0x41, reg_val, 6);
	read_reg->aenv_val[0] = reg_val[0];
	read_reg->aenv_val[1] = reg_val[1];
	read_reg->aenv_val[2] = reg_val[2];
	read_reg->apre_val[0] = reg_val[3];
	read_reg->apre_val[1] = reg_val[4];
	read_reg->aps_val = reg_val[5];
	/* new param */
	usr_iic_read_bytes(0x22, res_ext, 2);
	/* slot0 */
	dev9202_adjust[0].slot_env = read_reg->aenv_val[0];
	dev9202_adjust[0].current = read_reg->led_cur[0];
	res.res_val = read_reg->pd_res[0];
	res.low_res = (res_ext[0] >> 0) & 0x01;
	res.offset = (res_ext[0] >> 4) & 0x03;
	res.offset3 = 0;
	dev9202_adjust[0].res_level = vc9202_drv_res_register_to_level(&res);
	/* slot1 */
	dev9202_adjust[1].slot_env = read_reg->aenv_val[1];
	dev9202_adjust[1].current = read_reg->led_cur[1];
	res.res_val = read_reg->pd_res[1];
	res.low_res = (res_ext[0] >> 1) & 0x01;
	res.offset = (res_ext[0] >> 6) & 0x03;
	res.offset3 = 0;
	dev9202_adjust[1].res_level = vc9202_drv_res_register_to_level(&res);
	/* slot2 */
	dev9202_adjust[2].slot_env = read_reg->aenv_val[2];
	dev9202_adjust[2].current = read_reg->led_cur[2];
	dev9202_adjust[2].res_level = read_reg->pd_res[2];
	/* 如果有等效电阻的等级，那么传出等效等级 */
	read_reg->pd_res[0] = dev9202_adjust[0].res_level;
	read_reg->pd_res[1] = dev9202_adjust[1].res_level;
	read_reg->pd_res[2] = dev9202_adjust[2].res_level;
	/* pdbias */
	usr_iic_read_bytes(0x47, reg_val, 4);
	read_reg->apdbias_val[0] = reg_val[0];
	read_reg->apdbias_val[1] = reg_val[1];
	read_reg->apdbias_val[2] = reg_val[2];
	read_reg->apdbias_val[3] = reg_val[3] & 0xf;
	read_reg->pdbias = read_reg->apdbias_val[2] * 16 + read_reg->apdbias_val[3];

	usr_iic_read_bytes(0x4B, reg_val, 4);
	read_reg->bio_val_i = (reg_val[0] << 8) + reg_val[1];
	read_reg->bio_val_o = (reg_val[2] << 8) + reg_val[3];

	if ((read_reg->aps_val >> 4) == read_reg->ps_val) /* 防止数据错误，4bit与8bit不匹配，则不对佩戴数据进行更新，防止因为异常摘下 */
	{
		dev9202_wear.wear_ps = read_reg->aps_val;
		dev9202_wear.wear_env = read_reg->pdbias;
		dev9202_wear.wear_bio = read_reg->bio_val_o;
	}
	return 0;
}

// 注意，该部分需要分为两部分，一部分为中断调用，一部分为定时调用，保证低端客户的可用性较高
/**
 * @brief  通过宏定义#if (FUNC_ENABLE==VC9202_CFG_DRV_BY_TIMER) 已经对定时器调用方式做了对应的处理，现在通用一个函数接口即可
 * @param 无
 * @retval 无
 */
static VC9202DrvRetTypeDef vc9202_drv_gpio_event_handler(SamplingDataTypeDef *pResult, unsigned char *psize)
{
	VC9202DrvRetTypeDef vc_ret = RET_OK;
	switch (dev9202_info.work_mode)
	{
	case WORK_MODE_HR:
		vc_ret |= vc9202_drv_status_event_verify(dev9202_info.work_status, dev9202_info.int_event);
		vc9202_drv_calibration_osc_clk();
		vc_ret |= vc9202_drv_read_fifo_result(pResult, psize);
		break;
	case WORK_MODE_SPO2:
		vc_ret |= vc9202_drv_status_event_verify(dev9202_info.work_status, dev9202_info.int_event);
		vc9202_drv_calibration_osc_clk();
		vc_ret |= vc9202_drv_read_fifo_result(pResult, psize);
		break;
	case WORK_MODE_HRV:
		vc_ret |= vc9202_drv_status_event_verify(dev9202_info.work_status, dev9202_info.int_event);
		vc9202_drv_calibration_osc_clk();
		vc_ret |= vc9202_drv_read_fifo_result(pResult, psize);
		break;
	case WORK_MODE_STRESS:
		vc_ret |= vc9202_drv_status_event_verify(dev9202_info.work_status, dev9202_info.int_event);
		vc9202_drv_calibration_osc_clk();
		vc_ret |= vc9202_drv_read_fifo_result(pResult, psize);
		break;
	case WORK_MODE_WEAR:
		vc_ret |= vc9202_drv_status_event_verify(dev9202_info.work_status, dev9202_info.int_event);
		vc9202_drv_calibration_osc_clk();
		break;
	case WORK_MODE_FACTORY:
		if (dev9202_info.int_event & VC9202_EVENT_OV_MARK)
		{
			vc_ret |= vc9202_drv_adjust_over_load_configure(&dev9202_adjust[0], 0, dev9202_info.work_status);
		}
		pResult->led_cur[0] = dev9202_adjust[0].max_current;
		break;
	case WORK_MODE_FPC_TEST:
		vc9202_test_handler(pResult, &vc9202_test_info);
		break;
	default:
		break;
	}
	return vc_ret;
}

/**
  * @brief  初始化驱动参数
  * @param
		 @pInit fifo_div fifo分频深度
		 @pInit enslot 通道使能配置
  * @retval 无
  */
void vc9202_drv_system_param_init(unsigned char fifo_div, unsigned char enslot)
{
	unsigned char i = 0;
	/* dev info param init */
	dev9202_info.enfifo_div = fifo_div;
	dev9202_info.enslot[0] = (enslot & 0x01);
	dev9202_info.enslot[1] = (enslot & 0x02) >> 1;
	dev9202_info.slot_number = dev9202_info.enslot[0] + dev9202_info.enslot[1];

	dev9202_info.osc_clk_status = 0;
	dev9202_info.read_fifo_address = 0x80;
	/* dev9202_adjust info param init */
	for (i = 0; i < 2; i++)
	{
		dev9202_adjust[i].max_current = VC9202_MAX_CUR_LEVEL;
		dev9202_adjust[i].max_res_level = VC9202_MAX_RES_LEVEL;
		dev9202_adjust[i].record_env = 0;
		dev9202_adjust[i].info.dir = AGC_DIR_NULL;
		dev9202_adjust[i].info.last_dir = AGC_DIR_NULL;
		dev9202_adjust[i].info.current_step = 4;
	}
	dev9202_adjust[2].max_res_level = VC9202_MAX_RES_LEVEL;
	/* dev9202_wear param init */
#if (VC9202_CFG_WEAR_DEFAULT_STATUS == WEAR_IS_HOLD)
	dev9202_info.wear_status = WEAR_STA_HOLD;
	dev9202_wear.wear_status = WEAR_STA_HOLD;
	dev9202_wear.last_status = (WEAR_STA_NONE == dev9202_wear.last_status) ? WEAR_STA_HOLD : dev9202_wear.last_status;
#else
	dev9202_wear.wear_status = WEAR_STA_DROP;
	dev9202_wear.last_status = (WEAR_STA_NONE == dev9202_wear.last_status) ? WEAR_STA_DROP : dev9202_wear.last_status;
#endif
#if (VC9202_CFG_WEAR_DETECTION != FUNC_ENABLE)
	dev9202_info.wear_status = WEAR_STA_HOLD;
	dev9202_wear.wear_status = WEAR_STA_HOLD;
	dev9202_wear.last_status = WEAR_STA_HOLD;
#endif
	if (dev9202_wear.last_status == WEAR_STA_DROP && dev9202_info.work_mode != WORK_MODE_FACTORY && dev9202_info.work_mode != WORK_MODE_FPC_TEST)
	{
		if (dev9202_info.enslot[0])
		{
			vc9202_drv_channel_switch(0, 0);
		}
		if (dev9202_info.enslot[1])
		{
			vc9202_drv_channel_switch(1, 0);
		}
	}
	dev9202_wear.wear_status = dev9202_wear.last_status;
	dev9202_info.wear_status = dev9202_wear.wear_status;
	alg_wear_status = WEAR_STA_HOLD;
	dev9202_wear.wear_drop_cnt = VC9202_CFG_WEAR_DROP_CNT;
	dev9202_wear.wear_hold_cnt = VC9202_CFG_WEAR_HOLD_CNT;
	vc9202_drv_clear_clk();
#if 0
	f_BIOInit(VC9202_CFG_WEAR_PARAM_BIOIN, VC9202_CFG_WEAR_PARAM_PSIN);
	bio_result = VC9202_BIO_STA_NONE;
	if( dev9202_wear.last_status==WEAR_STA_HOLD )
	{
		bio_result = VC9202_BIO_STA_HOLD;
	}
	else
	{
		bio_result = VC9202_BIO_STA_NONE;
	}
#else
	if (VC9202_BIO_STA_NONE == bio_result)
	{
		f_BIOInit(VC9202_CFG_WEAR_PARAM_BIOIN, VC9202_CFG_WEAR_PARAM_PSIN);
	}
#endif
}

/*********************************************************************************************************
 * 对外功能函数
 ********************************************************************************************************/

/**
 * @brief  获取chip_id
 * @param  无
 * @retval chip_id值
 */
unsigned char vc9202_drv_get_chip_id(void)
{
	unsigned char chip_id = 0;
	usr_iic_read_bytes(0x00, &chip_id, 1);
	return chip_id;
}

/**
 * @brief  开始采样
 * @param  无
 * @retval 无
 */
void vc9202_drv_start_sampling(void)
{
	unsigned char temp = 0x00;
	usr_iic_read_bytes(0x10, &temp, 1);
	temp |= (1 << 7);
	usr_iic_write_bytes(0x10, &temp, 1);
}

/**
 * @brief  停止采样
 * @param  无
 * @retval 无
 */
void vc9202_drv_stop_sampling(void)
{
	unsigned char temp = 0x00;
	usr_iic_read_bytes(0x10, &temp, 1);
	temp &= ~(1 << 7);
	usr_iic_write_bytes(0x10, &temp, 1);
	bio_result = VC9202_BIO_STA_NONE;
}

/**
 * @brief  软复位
 * @param  无
 * @retval 无
 */
void vc9202_drv_soft_reset(void)
{
	unsigned char reset_reg_cfg = 0x5A;
	usr_iic_write_bytes(0x3B, &reset_reg_cfg, 1);
}

/**
  * @brief  独立使/失能某一物理通道
  * @param
		 @num_channel 通道编号(0,1,2)
		 @enable_flag 使能或失能标志位
  * @retval 0执行成功
  */
unsigned char vc9202_drv_channel_switch(unsigned char num_channel, unsigned char enable_flag)
{
	unsigned char temp = 0x00;
	usr_iic_read_bytes(0x10, &temp, 1);
	if (num_channel > 2)
	{
		return 1;
	}
	if (enable_flag == 1)
	{
		temp |= (1 << num_channel);
		usr_iic_write_bytes(0x10, &temp, 1);
		/* 同频处理 */
		if (WORK_MODE_SPO2 == dev9202_info.work_mode)
		{
			usr_iic_read_bytes(0x11, &temp, 1);
			temp &= ~(1 << 4);
			usr_iic_write_bytes(0x11, &temp, 1);
			temp = 0;
			usr_iic_write_bytes(0x16, &temp, 1);
		}
	}
	else if (enable_flag == 0)
	{
		temp &= ~(1 << num_channel);
		usr_iic_write_bytes(0x10, &temp, 1);
		/* 同频处理 */
		usr_iic_read_bytes(0x11, &temp, 1);
		temp |= (1 << 4);
		usr_iic_write_bytes(0x11, &temp, 1);
		temp = dev9202_info.enfifo_div;
		usr_iic_write_bytes(0x16, &temp, 1);
	}
	else
	{
		return 2;
	}
	return 0;
}

/**
 * @brief  获取中断原因
 * @param 无
 * @retval 中断原因
 */
unsigned char vc9202_drv_get_int_reason(void)
{
	unsigned char int_reason = 0;
	unsigned char int_enable = 0;
	usr_iic_read_bytes(0x02, &int_reason, 1);
	usr_iic_read_bytes(0x11, &int_enable, 1);
	/* 在同频时，获取到的ps中断，并非ps所需要的中断 */
	if (!(int_enable & (1 << 4)))
	{
		int_reason &= ~(1 << 4);
	}
#if (VC9202_CFG_DRV_BY_TIMER == FUNC_ENABLE)
	int_reason = (EVENT_PS_MARK | EVENT_FIFO_MARK);
#endif
	return int_reason;
}

/**
 * @brief  防止静电打死，自动恢复,定时去调用检查即可
 * @param 无
 * @retval
 */
void vc9202_drv_electrostatic_death(void)
{
	unsigned char work_reg = 1;
	unsigned char bio_reg[2] = {1, 1};
	unsigned short bio_val = 0;
	usr_iic_read_bytes(0x10, &work_reg, 1);
	if(dev9202_info.enable_bio)
	{
	  usr_iic_read_bytes(0x4D, bio_reg, 2);
	}
	bio_val = (bio_reg[1] << 8) + bio_reg[0];
	/* dev9202 death */
	if (0 == work_reg || 0 == bio_val)
	{
		InitParamTypeDef init_param = {dev9202_info.fifo_time, dev9202_info.work_mode};
		vc9202_drv_soft_reset();
		vc9202_drv_reg_init(&init_param);
	}
}

/**
 * @brief  防止任务队列堆积处理，导致短时间连续执行
 * @param 无
 * @retval 0-pass,-1-fail
 */
#define CHECK_GAPTIME_RATIO 3/4 /* 最短中断时间的3/4,30ms，算上频率在[16K,24K]波动，限制的时间为[25~37]ms以内会被过滤掉 */
static int vc9202_drv_check_running_gaptime(void)
{
	unsigned int last_count = dev9202_clk.last_clk_check;
	unsigned int now_count = 0;

	unsigned int count1 = 0, count2 = 0;
	unsigned int count_diff = 0;
	unsigned char cnt_reg[4] = {0};
	int ret = 0;
	unsigned char check_gap_time = (1000/dev9202_info.work_frequency)*CHECK_GAPTIME_RATIO;
	usr_iic_read_bytes(0x07, cnt_reg, 4);
	count1 = (cnt_reg[0] << 8) + cnt_reg[1];
	count2 = (cnt_reg[2] << 8) + cnt_reg[3];
	count_diff = (count1 <= count2) ? (count2 - count1) : (count2 - count1 + 0xffff);
	/* count值获取正确有效 */
	if (count_diff <= 5)
	{
		now_count = count1;
	}
	count_diff = 8000;
	if ((0 != now_count) && (0 != last_count))
	{
		count_diff = (last_count <= now_count) ? (now_count - last_count) : (now_count - last_count + 0xffff);
	}
	/* 间隔时间过小,算上频率在[16K,24K]波动，限制的时间为[25~37]ms以内会被过滤掉 */
	if (count_diff <= 20 * check_gap_time)
	{
		ret = -1;
	}
	dev9202_clk.last_clk_check = now_count;
	return ret;
}

/**
  * @brief  寄存器配置
  * @param
		 @pInit_Param->work_mode 工作模式
		 @pInit_Param->sampling_gap_time FIFO中断间隔时间,FIFO深度
  * @retval 无
  */
unsigned char vc9202_drv_reg_init(InitParamTypeDef *pInit_Param)
{
	unsigned short int clk_div = 0x0000;
	unsigned char fifo_div = 0;
	unsigned char reg_cfg[17] = {0x00, 0x1F, 0x8A, 0x00, 0x03, 0x1F, 0x00, 0x00, 0x80, 0x00, 0x57, 0x57, 0x07, 0x16, 0x56, 0x16, 0x00};
	unsigned char reg_new = (1 << 3);
	unsigned char reg_bio_cfg[2] = {0x00, 0xA2};
/* 设置中断触发方式 0上升沿 1下降沿 */
#if VC9202_CFG_INTERRUPT_MODE
	reg_cfg[1] |= (1 << 7);
#endif

/* 设置中断电平保持时间 */
#ifdef VC9202_CFG_INTERRUPT_TIME
	reg_cfg[1] |= (VC9202_CFG_INTERRUPT_TIME << 5);
#endif

	/* 设置FIFO深度,FIFO/PS中断间隔时间,同时设置采样分频 */
	dev9202_info.work_frequency = VC9202_CFG_SAMPLE_RATE;
	dev9202_info.work_mode = (WorkModeTypeDef)pInit_Param->work_mode;
	dev9202_info.fifo_time = pInit_Param->sampling_gap_time;
	if (WORK_MODE_HRV == dev9202_info.work_mode)
	{
		dev9202_info.work_frequency = 100;
		dev9202_info.fifo_time = 200;
	}
	fifo_div = dev9202_info.fifo_time / (1000 / dev9202_info.work_frequency) - 1;
	clk_div = (20000 / dev9202_info.work_frequency) - 1;
	reg_cfg[3] = fifo_div;
	reg_cfg[4] = (clk_div >> 8) & 0xFF;
	reg_cfg[5] = (clk_div & 0xFF);
	reg_cfg[6] = fifo_div;

	/* 设置PS通道PD电阻,LED电流 */
	reg_cfg[9] = VC9202_CFG_PS_SLOT_CUR;
	reg_cfg[12] = VC9202_CFG_PS_SLOT_RES;
	switch (dev9202_info.work_mode)
	{
	case WORK_MODE_HR:
		reg_cfg[0] = 0x45;
#if VC9202_CFG_IS_HALVE_CAP
		reg_cfg[10] = 0x54; /* 根据发光时间设置滤波电阻 */
		reg_cfg[13] = 0x03; // 0x03; /* 绿灯(物理通道0)映射到逻辑通道0,并设置采样时长为50us */
		reg_cfg[16] = 0x01; /* 发光时间减半,开1倍过采 */
#else
		reg_cfg[10] = 0x57; /* 根据发光时间设置滤波电阻 0x57 */
		reg_cfg[13] = 0x16; /* 绿灯(物理通道0)映射到逻辑通道0,并设置采样时长为50us */
		reg_cfg[16] = 0x00; /* 发光时间减半,开1倍过采 */
#endif
		break;

	case WORK_MODE_SPO2:
		reg_cfg[0] = 0x47;
		reg_cfg[1] &= ~(1 << 4); /* 关闭PS中断 */
		reg_cfg[6] = 0x00;		 /* PS开同频,避免引入同频干扰 */
		reg_cfg[7] = 0x80;		 /* slot0(红光)开32倍增益 */
		reg_cfg[13] = 0xB6;		 /*B6-3 红外(物理通道2)映射到逻辑通道0,并设置采样时长为200us */
		reg_cfg[14] = 0x76;		 /*76-3 红光(物理通道1)映射到逻辑通道0,并设置采样时长为200us */
		reg_cfg[16] = 0x04;		 /* 开16倍过采 */
		break;

	case WORK_MODE_HRV:
		reg_cfg[0] = 0x45;
#if VC9202_CFG_IS_HALVE_CAP
		reg_cfg[10] = 0x54; /* 根据发光时间设置滤波电阻 */
		reg_cfg[13] = 0x03; /* 绿灯(物理通道0)映射到逻辑通道0,并设置采样时长为50us */
		reg_cfg[16] = 0x01; /* 发光时间减半,开1倍过采 */
#else
		reg_cfg[10] = 0x57; /* 根据发光时间设置滤波电阻 */
		reg_cfg[13] = 0x16; /* 绿灯(物理通道0)映射到逻辑通道0,并设置采样时长为50us */
		reg_cfg[16] = 0x00; /* 发光时间减半,开1倍过采 */
#endif
		reg_bio_cfg[1] = 0x00;
		break;

	case WORK_MODE_STRESS:
		reg_cfg[0] = 0x45;
#if VC9202_CFG_IS_HALVE_CAP
		reg_cfg[10] = 0x54; /* 根据发光时间设置滤波电阻 */
		reg_cfg[13] = 0x03; /* 绿灯(物理通道0)映射到逻辑通道0,并设置采样时长为50us */
		reg_cfg[16] = 0x01; /* 发光时间减半,开1倍过采 */
#else
		reg_cfg[10] = 0x57; /* 根据发光时间设置滤波电阻 */
		reg_cfg[13] = 0x16; /* 绿灯(物理通道0)映射到逻辑通道0,并设置采样时长为50us */
		reg_cfg[16] = 0x00; /* 发光时间减半,开1倍过采 */
#endif
		break;

	case WORK_MODE_WEAR:
		reg_cfg[0] = 0x44;
		reg_cfg[1] = (reg_cfg[1] | (1 << 4)) & 0xF0;
		break;

	case WORK_MODE_FACTORY:
		reg_cfg[0] = 0x47;
		reg_cfg[7] = 0xEF;
		reg_cfg[8] = 0xEF;
		reg_cfg[10] = 0x77;
		reg_cfg[11] = 0x77;
		break;
	case WORK_MODE_FPC_TEST:
		reg_cfg[0] = 0x47;
		reg_cfg[7] = 0xEF;
		reg_cfg[8] = 0xEF;
		reg_cfg[10] = 0x77;
		reg_cfg[11] = 0x77;
		vc9202_test_init(&vc9202_test_info);
		break;
	default:
		break;
	}
	usr_iic_write_bytes(0x10, reg_cfg, 17); /* 心率血氧相关寄存器配置 */
	usr_iic_write_bytes(0x22, &reg_new, 1);
	usr_iic_write_bytes(0x23, reg_bio_cfg, 2); /*BIO相关寄存器配置 */
	if (reg_bio_cfg[1] & 0x80)
	{
		dev9202_info.enable_bio = 1;
	}
	else
	{
		dev9202_info.enable_bio = 0;
	}
	vc9202_drv_system_param_init(fifo_div, reg_cfg[0]);
	/* mode位置1,芯片开始工作 */
	vc9202_drv_start_sampling();
	return 0;
}

/**
 * @brief  传递采样数据
 * @param  采样数据保存地址，ppg数量
 * @retval 执行结果
 */
unsigned char vc9202_drv_get_sampling_data(SamplingDataTypeDef *pSamplingData, unsigned char *psize)
{
	unsigned char ret = 0;
	/* 读取状态及中断事件 */
	usr_iic_read_bytes(0x01, &dev9202_info.work_status, 1);
	usr_iic_read_bytes(0x02, (unsigned char *)&dev9202_info.int_event, 1);
	if (dev9202_info.work_status & 0xf8) /* 防止insample和conflict时数据错误，则不对设备进行处理 */
	{
		dev9202_info.int_event = 0; /* 异常时，避免执行佩戴检测 */
		return 1;
	}
	if (-1 == vc9202_drv_check_running_gaptime())
	{
		dev9202_info.int_event = 0; /* 异常时，避免执行佩戴检测 */
		return 1;
	}
	ret = vc9202_drv_read_reg(pSamplingData);
	ret = (unsigned char)vc9202_drv_gpio_event_handler(pSamplingData, psize);
	return ret;
}
