#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>

#define RTC_REMAIN2_BIT_JTAG_FLAG		0
#define RTC_REMAIN2_BIT_CAP_FLAG		1
#define RTC_REMAIN2_BIT_OTA_UPGRADE		2
#define RTC_REMAIN2_BIT_WD_RESET_CNT	3   /*bit3-bit6*/
#define RTC_REMAIN2_BIT_SLEEP_DBG_STAGE	8   /*bit8-bit15*/
#define RTC_REMAIN2_BIT_RTC_RC32K_CAL	26   /*bit26*/
#define RTC_REMAIN2_BIT_HR_RESET		27   /*bit27*/
#define RTC_REMAIN2_BIT_SYS_PANIC		28   /*bit28*/




static void soc_ps_setbit_val(mem_addr_t reg_addr, int bit, int bit_width, u32_t value)
{
	u32_t tmp,bit_mask;
	bit_mask = ~(((1<<bit_width)-1) << bit);
	tmp = sys_read32(reg_addr) & bit_mask;
	tmp |= (value << bit);
	sys_write32(tmp, reg_addr);
}

int soc_pstore_set(u32_t tag, u32_t value)
{
	switch(tag){
		case SOC_PSTORE_TAG_CAPACITY:
			sys_write32(value, RTC_REMAIN1);
			break;
		case SOC_PSTORE_TAG_FLAG_CAP:
			soc_ps_setbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_CAP_FLAG, 1,  !!value);
			break;
		case SOC_PSTORE_TAG_OTA_UPGRADE:
			soc_ps_setbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_OTA_UPGRADE, 1, !!value);
			break;
		case SOC_PSTORE_TAG_FLAG_JTAG:
			soc_ps_setbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_JTAG_FLAG, 1, !!value);
			break;
		case SOC_PSTORE_TAG_WD_RESET_CNT:
			soc_ps_setbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_WD_RESET_CNT, 4, value);
			break;
		case SOC_PSTORE_TAG_SLEEP_DBG_STAGE:
			soc_ps_setbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_SLEEP_DBG_STAGE, 8, value);
			break;
		case SOC_PSTORE_TAG_HR_RESET:
			soc_ps_setbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_HR_RESET, 1, value);
			break;
		case SOC_PSTORE_TAG_SYS_PANIC:
			soc_ps_setbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_SYS_PANIC, 1, value);
			break;
		case SOC_PSTORE_TAG_RTC_RC32K_CAL:
			soc_ps_setbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_RTC_RC32K_CAL, 1, value);
			break;
		default:
			return -1;
			break;
	}

	return 0;
}
static void soc_ps_getbit_val(mem_addr_t reg_addr, int bit, int bit_width, u32_t *p_value)
{
	u32_t bit_mask;
	bit_mask = (1<<bit_width)-1;
	*p_value = (sys_read32(reg_addr)>> bit) & bit_mask;
}

int soc_pstore_get(u32_t tag, u32_t *p_value)
{
	switch(tag){
		case SOC_PSTORE_TAG_CAPACITY:
			*p_value = sys_read32(RTC_REMAIN1);
			break;
		case SOC_PSTORE_TAG_FLAG_CAP:
			soc_ps_getbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_CAP_FLAG, 1, p_value);
			break;
		case SOC_PSTORE_TAG_OTA_UPGRADE:
			soc_ps_getbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_OTA_UPGRADE, 1, p_value);
			break;
		case SOC_PSTORE_TAG_FLAG_JTAG:
			soc_ps_getbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_JTAG_FLAG, 1, p_value);
			break;
		case SOC_PSTORE_TAG_WD_RESET_CNT:
			soc_ps_getbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_WD_RESET_CNT, 4, p_value);
			break;
		case SOC_PSTORE_TAG_SLEEP_DBG_STAGE:
			soc_ps_getbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_SLEEP_DBG_STAGE, 8, p_value);
			break;
		case SOC_PSTORE_TAG_HR_RESET:
			soc_ps_getbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_HR_RESET, 1, p_value);
			break;
		case SOC_PSTORE_TAG_SYS_PANIC:
			soc_ps_getbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_SYS_PANIC, 1, p_value);
			break;
		case SOC_PSTORE_TAG_RTC_RC32K_CAL:
			soc_ps_getbit_val(RTC_REMAIN2, RTC_REMAIN2_BIT_RTC_RC32K_CAL, 1, p_value);
			break;
		default:
			return -1;
			break;

	}
	return 0;
}


int soc_pstore_reset_all(void)
{
	int i;
	for(i = 0; i <6; i++ )
		sys_write32(0x0, RTC_REMAIN0+i*4); 
	return 0;
}

