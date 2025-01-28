
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include <board_cfg.h>
#include "spi_flash.h"

#ifdef CONFIG_SOC_LEOPARD
#define DELAY_CHAIN_NUM		64
#else
#define DELAY_CHAIN_NUM		16
#endif

//static inline void test_setup_delaychain(struct spinor_info *sni, u8_t ns)
//{
	//struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	//spi->ctrl = (spi->ctrl & ~(0xF << 16)) | (ns << 16);

	//volatile int i = 100000;
	//while (i--);

//}
#define TEST_READ_WRITE

#ifdef  TEST_READ_WRITE

#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)
static u32_t TEST_ADDR = 0x200000;
static u32_t TEST_ADDR_END = 0x400000;
#else
#define TEST_ADDR 0x200000
#define TEST_ADDR_END  0x400000
#endif

#define TEST_SIZE	(1024*4)
static u32_t nor_test_buf[TEST_SIZE/4];
static u32_t nor_test_start= 0x200000;
__ramfunc int test_read_write_try(const struct device *dev, u8_t delay_chain)
{
	int ret, i;
	struct spinor_info *sni = (struct spinor_info *)(dev)->data;

	sni->spi.delay_chain = delay_chain;

	if(nor_test_start < TEST_ADDR || nor_test_start >= TEST_ADDR_END)
		nor_test_start = TEST_ADDR;

	ret = flash_erase(dev, nor_test_start, TEST_SIZE);
	for(i = 0; i < TEST_SIZE/4; i++)
		nor_test_buf[i] = nor_test_start + i;

	ret = flash_write(dev, nor_test_start, nor_test_buf, TEST_SIZE);
	memset(nor_test_buf, 0 , TEST_SIZE);
	ret = flash_read(dev, nor_test_start, nor_test_buf, TEST_SIZE);
	if(ret){
		//test_setup_delaychain(sni, CONFIG_SPI_FLASH_DELAY_CHAIN);
		printk("read write fail =0x%x\n", nor_test_start);
		return -1;
	}
	for(i = 0; i < TEST_SIZE/4; i++){
		if( nor_test_buf[i] != 	nor_test_start + i){
			//test_setup_delaychain(sni, CONFIG_SPI_FLASH_DELAY_CHAIN);
			printk("read 0x%x != 0x%x\n", nor_test_start + i, nor_test_buf[i]);
			nor_test_start += TEST_SIZE;
			return -1;
		}
	}
	nor_test_start += TEST_SIZE;

	return 0;
}

#endif

__ramfunc u32_t test_delaychain_read_id(const struct device *dev, u8_t delay_chain)
{
	u32_t nor_id, mid;
	struct spinor_info *sni = (struct spinor_info *)(dev)->data;

	sni->spi.delay_chain = delay_chain;

	/* configure delay chain */
	//test_setup_delaychain(sni, sni->spi.delay_chain);
	//k_busy_wait(50);
	nor_id = p_spinor_api->read_chipid(sni) & 0xffffff;

	mid = nor_id & 0xff;
	if ((mid == 0xff) || (mid == 0x00))
		return 0;

	return nor_id;
}

__ramfunc s32_t test_delaychain_try(const struct device *dev, u8_t *ret_delaychain, u32_t chipid_ref)
{
	u32_t i, try_delaychain;
	bool match_flag = 0;
	//u32_t nor_id_value_check;
	u32_t local_irq_save;

	local_irq_save = irq_lock();

	ret_delaychain[0] = 0;

	for (try_delaychain = 1; try_delaychain < DELAY_CHAIN_NUM; try_delaychain++) {
		match_flag = 1;
		printk("try_delaychain :%d\n", try_delaychain);
		soc_udelay(5000);
		#ifdef  TEST_READ_WRITE
		for (i = 0; i < 2; i++) {
			if(test_read_write_try(dev, try_delaychain)){
				match_flag = 0;
				break;
			}
		}
		#else
		for (i = 0; i < 64; i++) {
			nor_id_value_check = test_delaychain_read_id(dev, try_delaychain);
			if (nor_id_value_check != chipid_ref) {
				printk("read:0x%x @ %d\n", nor_id_value_check, try_delaychain);
				match_flag = 0;
				break;
			}

		}
		#endif

		ret_delaychain[try_delaychain] = match_flag;
	}

	//test_delaychain_read_id(dev, CONFIG_SPI_FLASH_DELAY_CHAIN);

	irq_unlock(local_irq_save);

	return 0;
}

__ramfunc u32_t nor_read_chipid(const struct device *dev)
{
	u32_t nor_id;
	u32_t local_irq_save;
	struct spinor_info *sni = (struct spinor_info *)(dev)->data;
	local_irq_save = irq_lock();
	nor_id = p_spinor_api->read_chipid(sni) & 0xffffff;
	irq_unlock(local_irq_save);
	return nor_id;
}

__ramfunc u8_t spinor_test_delaychain(const struct device *test_nor_dev)
{
	u8_t ret_delaychain = 0;
	u8_t delaychain_flag[DELAY_CHAIN_NUM];
	u8_t delaychain_total[DELAY_CHAIN_NUM];
	u8_t start = 0, end, middle, i;
	u8_t expect_max_count_delay_chain = 0, max_count_delay_chain;
	//u32_t freq;
	u32_t chipid_ref;
	u8_t bak_delaytran;
	//struct device *test_nor_dev = device_get_binding(CONFIG_SPI_FLASH_NAME);
	struct spinor_info *sni = (struct spinor_info *)(test_nor_dev)->data;


	bak_delaytran = sni->spi.delay_chain;
	printk("spinor test delaychain start\n");

	chipid_ref = nor_read_chipid(test_nor_dev);

	printk("delaytran=0x%x, chipid = 0x%x\n", bak_delaytran, chipid_ref);

	memset(delaychain_total, 0x0, DELAY_CHAIN_NUM);

	//for (freq = 6; freq <= 222; freq += 6) {
		expect_max_count_delay_chain++;

	//	soc_freq_set_cpu_clk(0, freq);

	//	printk("set cpu freq : %d\n", freq);

		if (test_delaychain_try(test_nor_dev, delaychain_flag, chipid_ref) == 0) {
			for (i = 0; i < DELAY_CHAIN_NUM; i++)
				delaychain_total[i] += delaychain_flag[i];
		} else {
			printk("test_delaychain_try error!!\n");
			goto delay_chain_exit;
		}

	//}
	sni->spi.delay_chain = bak_delaytran;

	printk("delaychain_total : ");
	for (i = 0; i < DELAY_CHAIN_NUM; i++)
		printk("%d,", delaychain_total[i]);
	printk("\n");

	max_count_delay_chain = 0;
	for (i = 0; i < DELAY_CHAIN_NUM; i++) {
		if (delaychain_total[i] > max_count_delay_chain)
			max_count_delay_chain = delaychain_total[i];
	}

	for (i = 0; i < DELAY_CHAIN_NUM; i++) {
		if (delaychain_total[i] == max_count_delay_chain) {
			start = i;
			break;
		}
	}
	end = start;
	for (i = start + 1; i < DELAY_CHAIN_NUM; i++) {
		if (delaychain_total[i] != max_count_delay_chain)
			break;
		end = i;
	}

	if (max_count_delay_chain < expect_max_count_delay_chain) {
		printk("test delaychain max count is %d, less then expect %d!!\n",
			max_count_delay_chain, expect_max_count_delay_chain);
		goto delay_chain_exit;
	}

	if ((end - start + 1) < 3) {
		printk("test delaychain only %d ok!! too less!!\n", end - start + 1);
		goto delay_chain_exit;
	}

	middle = (start + end) / 2;
	printk("test delaychain pass, best delaychain is : %d\n\n", middle);

	ret_delaychain = middle;

	delay_chain_exit:

	return ret_delaychain;
}

/*
leopard: vdd < 1000 spi clk 64MHZ  else 93MHZ

*/
#define NUM_VDD_ITEM  4
const uint16_t g_vdd_volt[NUM_VDD_ITEM] = {1200, 1100, 1000, 950};
//const uint16_t g_vdd_volt[NUM_VDD_ITEM] = {1000, 950};
__ramfunc int nor_test_delaychain(const struct device *dev)
{
	int i;
	uint16_t vdd;
	//printk_dma_switch(0);
	printk("-----nor_test_delaychain =0x%x----\n", sys_read32(SPI1_DELAYCHAIN));
	sys_write32((sys_read32(SPI1_DELAYCHAIN)&(~0xf))|0x06, SPI1_DELAYCHAIN);
	soc_freq_set_cpu_clk(70,70);

	sys_write32(0, WD_CTL);
	#if 1
	for(i = 0; i < NUM_VDD_ITEM; i++){
		if(g_vdd_volt[i] == 950){
			printk("set spi0/spi1 clk 0.95V\n");
			sys_write32((sys_read32(SPI1_DELAYCHAIN)&(~0xf))|0x03, SPI1_DELAYCHAIN);
			clk_set_rate(CLOCK_ID_SPI1, MHZ(70) * 2);
			clk_set_rate(CLOCK_ID_SPI0, MHZ(64));
		}

		soc_pmu_set_vdd_voltage(g_vdd_volt[i]);
		soc_udelay(1000);
		printk("-----%d vdd set %d mv, try delaytran----\n", i, g_vdd_volt[i]);
		printk("%d, vdd=%d mv, delaychain : %d\n", i, g_vdd_volt[i], spinor_test_delaychain(dev));
	}
	#endif

#if defined(CONFIG_SPI_FLASH_GPIO_2CS) && (CONFIG_SPI_FLASH_GPIO_2CS == 1)

	for(i = NUM_VDD_ITEM-1; i >= 0; i--){
		vdd = g_vdd_volt[i];
		soc_pmu_set_vdd_voltage(vdd);
		soc_udelay(1000);
		if(g_vdd_volt[i] == 1000){
			printk("set spi0/spi1 clk 1.0V\n");
			sys_write32((sys_read32(SPI1_DELAYCHAIN)&(~0xf))|0x06, SPI1_DELAYCHAIN);
			clk_set_rate(CLOCK_ID_SPI1, MHZ(140) * 2);
			clk_set_rate(CLOCK_ID_SPI0, MHZ(100));
		}
	}

	printk("\n\n---------------try cs1 nor-----------------\n\n");
	TEST_ADDR = 0x200000+0x1000000;
	TEST_ADDR_END = 0x400000+0x1000000;
	nor_test_start= TEST_ADDR;

	for(i = 0; i < NUM_VDD_ITEM; i++){
		if(g_vdd_volt[i] == 950){
			printk("set spi0/spi1 clk 0.95V\n");
			sys_write32((sys_read32(SPI1_DELAYCHAIN)&(~0xf))|0x03, SPI1_DELAYCHAIN);
			clk_set_rate(CLOCK_ID_SPI1, MHZ(70) * 2);
			clk_set_rate(CLOCK_ID_SPI0, MHZ(64));
		}
		soc_udelay(1000);
		printk("--start cs1---%d vdd set %d mv, try delaytran----\n", i, g_vdd_volt[i]);
		printk("--end cs1 %d, vdd=%d mv, best delaychain : %d\n\n", i, g_vdd_volt[i], spinor_test_delaychain(dev));
	}
#endif

	printk("\n ----nor_test_delaychain-- finshed-----\n");
	while(1);
	return 0;
}