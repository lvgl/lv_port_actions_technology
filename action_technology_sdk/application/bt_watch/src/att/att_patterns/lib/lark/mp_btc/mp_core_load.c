#include "att_pattern_test.h"
#include "code_in_btram0.h"
#include "mp_btc_inner.h"

#define MP_BTC_NAME  "mp_btc.bin"

static int mp_btc_load_code(void)
{
#if 0
	unsigned int i, *p;
	p = (unsigned int *)0x01200000;
	for(i=0;i<sizeof(code_in_btram0)/4;i++)
	{
		*(p+i) = code_in_btram0[i];
	}

	udelay(5);

	return 0;
#else
	atf_dir_t dir;
	int ret_val;
	ret_val = read_atf_sub_file(NULL, 0x10000, (const u8_t *)MP_BTC_NAME, 0, -1, &dir);
	if (ret_val <= 0) {
		printk("read_file %s fail, quit!\n", MP_BTC_NAME);
		return -EIO;
	}

	return 0;
#endif	
}

void mp_btc_mem_init(void)
{
	printk("memsrc:%x\n", *((REG32)(CMU_MEMCLKSRC1)));
	*((REG32)(CMU_MEMCLKEN0)) = 0xffffffff;
	*((REG32)(CMU_MEMCLKEN1)) = 0xffffffff;
	//change bt ram to cpu
	*((REG32)(CMU_MEMCLKSRC1)) &= (~0x3f0000);

	printk("memsrc:%x\n", *((REG32)(CMU_MEMCLKSRC1)));

	//clear btc memory
	memset((uint8_t *)0x1200000, 0, 64 * 1024);

}

int mp_btc_core_load(void)
{
	int ret_val;

	*((REG32)(MEMORYCTL)) |= 0x1<<4;   // bt cpu boot from btram0

	ret_val = mp_btc_load_code();

	*((REG32)(CMU_MEMCLKSRC1))	|=	0x1<<16;												// change BT ram clk source
	//*((REG32)(CMU_MEMCLKSRC0))  |=  (0x2<<20)|(0x2<<22)|(0x2<<24);	// change AUDIO DSP ram clk source
	udelay(5);
	*((REG32)(CMU_DEVCLKEN1))  |= 0x1f000000;

	return ret_val;
}

void mp_btc_hardware_init(void)
{
	   /******************for BT*********************************************/
	*((REG32)(CMU_DEVCLKEN1)) |= (0x1F<<24);

	*((REG32)(CMU_S1CLKCTL)) |=(0x7<<0);
	*((REG32)(CMU_S1BTCLKCTL)) |=(0x7<<0);

	*((REG32)(LOSC_CTL)) |=(1<<LOSC_CTL_LOSC_EN);
//	*((REG32)(LOSC_CTL)) &=~(1<<LOSC_CTL_LOSC_EN);	//disable

	//*((REG32)(BDG_CTL_SVCC)) |=(0xC<<16);

	*((REG32)(CMU_MEMCLKSRC0)) |= (2<<CMU_MEMCLKSRC0_AUDDSPRAM0CLKSRC_SHIFT)\
														 | (2<<CMU_MEMCLKSRC0_AUDDSPRAM1CLKSRC_SHIFT)\
														 | (2<<CMU_MEMCLKSRC0_AUDDSPRAM2CLKSRC_SHIFT);
	*((REG32)(CMU_MEMCLKSRC1)) |= (1<<CMU_MEMCLKSRC1_BTRAMCLKSRC);

	*((REG32)(MRCR1)) |= (1<<24);					// release BT reset
}

void mp_btc_hardware_deinit(void)
{
	*((REG32)(MRCR1)) &= (~(1<<24));					//  BT reset
}

void mp_btc_init(void)
{
	mp_btc_deinit();

	mp_btc_mem_init();

	ipmsg_btc_update_bt_table((void *)BT_RAM_TABLE_ADDR);

	//init code
	mp_btc_core_load();

	//init hardware
	mp_btc_hardware_init();

	//init ipmsg communicate
	mp_btc_ipmsg_init();
}

void mp_btc_deinit(void)
{
	mp_btc_hardware_deinit();
}




