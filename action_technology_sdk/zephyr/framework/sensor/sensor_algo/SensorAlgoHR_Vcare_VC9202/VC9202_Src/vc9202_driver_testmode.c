/*********************************************************************************************************
 *               Copyright(c) 2022, vcare Corporation. All rights reserved.
 **********************************************************************************************************
 * @file     vc9202_driver_testmode.c
 * @brief    
 * @details
 * @author
 * @date     
 * @version  
 *********************************************************************************************************/
//!!!注意：本文文件包含step1(id,频率，温度的读取判断)，step2(bio是否满足变化/ps是否能够达到阈值)两步进行FPC测试，用于那些不使用测试板的测试的用户，排查基础问题
//!!!注意：本文文件包含step1(id,频率，温度的读取判断)，step2(bio是否满足变化/ps是否能够达到阈值)两步进行FPC测试，用于那些不使用测试板的测试的用户，排查基础问题
//如果不需要，删除本文件和其他报错的地方即可
#include "vc9202_common.h"
#include "vc9202_driver.h"


VC9202TestInfoTypeDef vc9202_test_info = {0};

const static unsigned char disp_green[17] = {0xC5, 0x30, 0x8A, 0x40, 0x03, 0x1F, 0x00,
											 0x30, 0x40, 0x5A,
											 0x57, 0x37, 0x47,
											 0x16, 0x56, 0x16, 0x00};
const static unsigned char disp_red[17] = {0xC6, 0x30, 0x8A, 0x40, 0x03, 0x1F, 0x00,
										   0x30, 0x40, 0x5A,
										   0x57, 0x37, 0x47,
										   0x16, 0x56, 0x16, 0x00};
static unsigned int mcu_cnt[2] = {0}, chip_cnt[2] = {0};
void vc9202_test_init(VC9202TestInfoTypeDef *ptest_info)
{
	mcu_cnt[0] = 0;
	mcu_cnt[1] = 0;
	chip_cnt[0] = 0;
	chip_cnt[1] = 0;
	memset(ptest_info, 0, sizeof(VC9202TestInfoTypeDef));
}

static unsigned int vc9202_test_real_frequency(void)
{
	unsigned char cnt_reg[4] = {0};
	unsigned int mcu_cnt_dif = 0;
	unsigned int chp_cnt_dif = 0;
	unsigned int calc_freq = 0;

	mcu_cnt[1] = mcu_cnt[0];
	chip_cnt[1] = chip_cnt[0];
	mcu_cnt[0] = usr_get_clock_ticks();
	usr_iic_read_bytes(0x07, cnt_reg, 4);
	chip_cnt[0] = (cnt_reg[0] << 8) + cnt_reg[1];
	unsigned short cnt1 = (cnt_reg[0] << 8) + cnt_reg[1];
	unsigned short cnt2 = (cnt_reg[2] << 8) + cnt_reg[3];
	if (abs(cnt1 - cnt2) >= 10)
	{
		return 0xffff;
	}
	if ((0 == mcu_cnt[1]) || (0 == mcu_cnt[0]))
		return 20 * 1000;
	if ((0 == chip_cnt[1]) || (0 == chip_cnt[0]))
		return 20 * 1000;
	mcu_cnt_dif = (mcu_cnt[0] > mcu_cnt[1]) ? (mcu_cnt[0] - mcu_cnt[1]) : (VC9202_CFG_CLK_CHECK_MAX - mcu_cnt[1] + mcu_cnt[0]);
	chp_cnt_dif = (chip_cnt[0] > chip_cnt[1]) ? (chip_cnt[0] - chip_cnt[1]) : (0xffff - chip_cnt[1] + chip_cnt[0]);
	float timeDiff = (float)mcu_cnt_dif / (float)VC9202_CFG_CLK_CHECK_FREQUENCY;
	calc_freq = (float)chp_cnt_dif / timeDiff;
	if (calc_freq >= 0xffff)
		calc_freq = 0xffff;
	if (calc_freq <= 0)
		calc_freq = 0;
	return calc_freq;
}

static int vc9202_test_led_glint(VC9202TestInfoTypeDef *ptest_info)
{
	static unsigned char glint_count = 0;
	if (ptest_info->step1_ret && ptest_info->step2_ret)
	{
		glint_count++;
		if (glint_count == 25)
		{
			usr_iic_write_bytes(0x10, (unsigned char *)disp_green, 17);
		}
		else if (glint_count == 50)
		{
			glint_count = 0;
			usr_iic_write_bytes(0x10, (unsigned char *)disp_red, 17);
		}
		return 1;
	}
	return 0;
}
/*记录bio变化值的最大的差值，用于判断bio变化是否满足阈值判断*/
static void vc9202_test_bio_variation(VC9202TestInfoTypeDef *ptest_info, unsigned short bio)
{
	unsigned short now_gap = 0;
	ptest_info->biovalue_now[1] = ptest_info->biovalue_now[0];
	ptest_info->biovalue_now[0] = bio;
	if (0 == ptest_info->biovalue_now[1] || 0 == ptest_info->biovalue_now[0])
	{
		return;
	}
	now_gap = (ptest_info->biovalue_now[0] > ptest_info->biovalue_now[1]) ? (ptest_info->biovalue_now[0] - ptest_info->biovalue_now[1]) : (ptest_info->biovalue_now[1] - ptest_info->biovalue_now[0]);
	if (now_gap > ptest_info->biovalue_max)
	{
		ptest_info->biovalue_max = now_gap;
		ptest_info->biovalue_gap[0] = ptest_info->biovalue_now[0];
		ptest_info->biovalue_gap[1] = ptest_info->biovalue_now[1];
	}
	return;
}
void vc9202_test_handler(SamplingDataTypeDef *pResult, VC9202TestInfoTypeDef *ptest_info)
{
	// 判断是否测试完成，只执行交替闪
	if (vc9202_test_led_glint(ptest_info))
	{
		return;
	}
	// step1 计算频率 + 读取ID及中断原因
	if (0 == ptest_info->step1_ret)
	{
		ptest_info->max_current = pResult->led_cur[0];
		ptest_info->leak_pre_value = pResult->apre_val[0];
		ptest_info->frequency = vc9202_test_real_frequency();
		usr_iic_read_bytes(0x00, &ptest_info->icid, 1);
		usr_iic_read_bytes(0x02, &ptest_info->reason, 1);
		return;
	}
	// step2 bio.ps
	if (0 != ptest_info->step1_ret && 0 == ptest_info->step2_ret)
	{
		ptest_info->ps_value = pResult->aps_val;
		vc9202_test_bio_variation(ptest_info, pResult->bio_val_o);
	}
	return;
}

// 在xx_user.c中调用   case WORK_MODE_FACTORY: factory_test_function( &data ); break;
#if 0
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
			vc9202_test_info.frequency >= 16000 && vc9202_test_info.frequency <= 24000)
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
		if (vc9202_test_info.biovalue_max >= 400 && vc9202_test_info.ps_value >= 200)
		{
			vc9202_test_info.step2_ret = 1;
		}
	}
	/*注意：上述方式会一直等待，直到检测通过，也可以使用人为触发记录，进行比较两次触发时的记录值差距是否满足。
		记录两次不同状态（模拟佩戴和摘下）下时的pdata->bio_val_o，比较差值
		记录模拟佩戴下的pdata->ps_val，判断是否满足PS范围
	*/
	/* 其中step1_ret和step2_ret，分别表示测试是否通过，由于step2_ret需要检测变化量，所以需要一直等待操作，如果测试通过后，驱动会控制红绿灯交替闪烁 */
	DEBUG_LOG("[FAT]icid=%#x,reason=%#x,frequency=%d,temperature=%d", vc9202_test_info.icid, vc9202_test_info.reason, vc9202_test_info.frequency, vc9202_test_info.temperature);
	DEBUG_LOG("[FAT]biovalue_max=%d,ps=%d,gap[%d,%d]", vc9202_test_info.biovalue_max, vc9202_test_info.ps_value, vc9202_test_info.biovalue_gap[0], vc9202_test_info.biovalue_gap[1]);
	DEBUG_LOG("[FAT]-ret(%d,%d)", vc9202_test_info.step1_ret, vc9202_test_info.step2_ret);
}

#endif
