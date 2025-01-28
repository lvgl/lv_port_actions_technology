/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock interface for Actions SoC
 */

#include <kernel.h>
#include <device.h>
#include <soc.h>
extern void cmu_dev_clk_enable(uint32_t id);
extern void cmu_dev_clk_disable(uint32_t id);

static void acts_clock_peripheral_control(int clock_id, int enable)
{
	unsigned int key;

	if (clock_id > CLOCK_ID_MAX_ID)
		return;

	key = irq_lock();

	if (enable) {
		if (clock_id < 32) {
			sys_set_bit(CMU_DEVCLKEN0, clock_id);
		} else {
			sys_set_bit(CMU_DEVCLKEN1, clock_id - 32);
		}
	} else {
		if (clock_id < 32) {
			sys_clear_bit(CMU_DEVCLKEN0, clock_id);
		} else {
			sys_clear_bit(CMU_DEVCLKEN1, clock_id - 32);
		}
	}
	irq_unlock(key);
}

void acts_clock_peripheral_enable(int clock_id)
{
	acts_clock_peripheral_control(clock_id, 1);
}

void acts_clock_peripheral_disable(int clock_id)
{
	acts_clock_peripheral_control(clock_id, 0);
}

uint32_t clk_rate_get_corepll(void)
{
	return MHZ(((sys_read32(COREPLL_CTL)&0x3F)*8));
}

uint32_t soc_freq_calculate(uint8_t divisor, uint8_t clk_src)
{
	uint32_t freq = 0;

	if (clk_src == 0) { /* RC4M */
		freq = 4000000UL;
	} else if (clk_src == 1) { /* HOSC */
		freq = 32000000UL;
	} else if (clk_src == 3) { /* RC64M */
		freq = 64000000UL;
	} else if (clk_src == 4) { /* RC96M */
		freq = 96000000UL;
	} else if (clk_src == 5) { /* RC128M */
		freq = 128000000UL;
	} else if (clk_src == 6) { /* RC32K */
		freq = 32000UL;
	} else if (clk_src == 2) { /* COREPLL */
		freq = clk_rate_get_corepll();
	}

	if (divisor == 14) {
		/* 1.5 divisor */
		freq = freq * 10 / 15;
	} else if (divisor == 15) {
		/* 2.5 divisor */
		freq = freq * 10 / 25;
	} else {
		freq = freq / (divisor + 1);
	}

	return freq;
}

uint8_t soc_freq_divisor_calculate(uint32_t freq_mhz, uint32_t max_mhz)
{
	uint8_t divisor;

	if (max_mhz > freq_mhz && max_mhz <= (freq_mhz * 3 / 2)) {
		/* /1.5 */
		divisor = 14;
	} else if ((max_mhz > 2 * freq_mhz) && max_mhz <= (freq_mhz * 5 / 2)) {
		/* /2.5 */
		divisor = 15;
	} else {
		/* /n */
		divisor = (max_mhz + freq_mhz - 1) / freq_mhz - 1;
		if (divisor > 13) {
			divisor = 13;
		}
	}

	return divisor;
}

uint32_t soc_freq_get_dsp_freq(void)
{
	uint8_t dspclk_div = (sys_read32(CMU_DSPCLK) & (0xF << 4)) >> 4;
	uint8_t dspclk_src = sys_read32(CMU_DSPCLK) & 0x7;

	return soc_freq_calculate(dspclk_div, dspclk_src);
}

uint32_t soc_freq_get_cpu_freq(void)
{
	uint8_t cpuclk_div = (sys_read32(CMU_SYSCLK) & (0xF << 4)) >> 4;
	uint8_t cpuclk_src = sys_read32(CMU_SYSCLK) & 0x7;

	return soc_freq_calculate(cpuclk_div, cpuclk_src);
}

void soc_freq_set_gpu_clk(uint32_t gpu_mhz, uint32_t de_mhz, uint32_t jpeg_mhz)
{
	uint32_t val, flags, divisor;
	uint32_t corepll_mhz;

	flags = irq_lock();

	corepll_mhz = clk_rate_get_corepll() / 1000000UL;

	if ((gpu_mhz > corepll_mhz)
		|| (de_mhz > corepll_mhz)
		|| (jpeg_mhz > corepll_mhz)) {
		irq_unlock(flags);
		return ;
	}

	/* set gpu clk */
	if (gpu_mhz) {
		divisor = soc_freq_divisor_calculate(gpu_mhz, corepll_mhz);

		val = sys_read32(CMU_GPUCLK);
		val &= ~((0xf << 0) | (0x3 << 8));
		val |= (0x1 << 8); /* select gpu clock source from corepll */
		val |= (divisor << 0);
		sys_write32(val, CMU_GPUCLK);
	}

	/* set de clk */
	if (de_mhz) {
		divisor = soc_freq_divisor_calculate(de_mhz, corepll_mhz);

		val = sys_read32(CMU_DECLK);
		val &= ~((0xf << 0) | (0x3 << 8));
		val |= (0x1 << 8); /* select de clock source from corepll */
		val |= (divisor << 0);
		sys_write32(val, CMU_DECLK);
	}

	/* set jpeg clk */
	if (jpeg_mhz) {
		divisor = soc_freq_divisor_calculate(jpeg_mhz, corepll_mhz);

		val = sys_read32(CMU_JPEGCLK);
		val &= ~((0xf << 0) | (0x3 << 8));
		val |= (0x1 << 8); /* select jpeg clock source from corepll */
		val |= (divisor << 0);
		sys_write32(val, CMU_JPEGCLK);
	}

	irq_unlock(flags);
}

void soc_freq_set_cpu_clk(uint32_t dsp_mhz, uint32_t cpu_mhz)
{
	uint32_t val, flags, divisor;
	uint32_t corepll_mhz;

	flags = irq_lock();

	corepll_mhz = clk_rate_get_corepll() / 1000000UL;

	if ((cpu_mhz > corepll_mhz) || (!cpu_mhz))
		return;

	if ((dsp_mhz > corepll_mhz) || (!dsp_mhz))
		return;

	divisor = soc_freq_divisor_calculate(cpu_mhz, corepll_mhz);

	/* set cpu clock */
	val = sys_read32(CMU_SYSCLK);
	val &= ~((0xf << 4) | (0x7 << 0) | (3<<8));
	val |= (0x2 << 0); /* select cpu clock source from corepll */
	val |= divisor << 4;
	if (soc_dvfs_opt())
		val |= (0x1 << 8); /* ahb /4 */
	else
		val |= (0x2 << 8); /* ahb /1 */
	sys_write32(val, CMU_SYSCLK);

	divisor = soc_freq_divisor_calculate(dsp_mhz, corepll_mhz);

	/* set dsp clock */
	val = sys_read32(CMU_DSPCLK);
	val &= ~((0x3f << 4) | (0x7 << 0));
	val |= (0x2 << 0); /* select dsp clock source from corepll */
	val |= divisor << 4;
	val |= (0x2 << 8); /* apb / 2 */
	sys_write32(val, CMU_DSPCLK);

	irq_unlock(flags);
}

#ifdef CONFIG_MMC_ACTS
static void  acts_clk_set_rate_sd(int sd, unsigned int rate_hz)
{
	unsigned int core_pll, hosc_hz, val, real_rate, div;
	core_pll = clk_rate_get_corepll();
	hosc_hz = CONFIG_HOSC_CLK_MHZ*1000000;

	/*
	* Set the RDELAY and WDELAY based on the sd clk.
	*/
	if (rate_hz < hosc_hz/16) {
		/* clock source: HOSC, 1/128, real freq: 188KHz */
		div = (hosc_hz+(128*rate_hz)-1)/(128*rate_hz);
		val = 0x40 + div-1 ;
		real_rate = hosc_hz/(128*div);

	} else if (rate_hz <= hosc_hz) {
		/* clock source: HOSC, real freq: 1.5M~24M */
		div = (hosc_hz+rate_hz-1)/rate_hz;
		val = div - 1;
		real_rate = hosc_hz/div;
	} else {
		/* clock source: core_pll, real freq: 200MHz */
		div = (core_pll+rate_hz-1)/rate_hz;
		real_rate = core_pll/div;
		val = (div-1)|(1<<8);
	}
	sys_write32(val, CMU_SD0CLK+sd*4);

	printk("mmc%d: set rate %d Hz, real rate %d Hz, core pll=%d HZ\n",
		sd, rate_hz, real_rate, core_pll);

}
#else
static void  acts_clk_set_rate_sd(int sd, unsigned int rate_hz)
{

}
#endif


#ifdef CONFIG_SOC_SPI0_USE_CK64M

static unsigned int calc_spi_clk_div(unsigned int max_freq, unsigned int spi_freq)
{
	unsigned int div;

	if (max_freq > spi_freq && max_freq <= (spi_freq * 3 / 2)) {
		/* /1.5 */
		div = 14;
	} else if ((max_freq > 2 * spi_freq) && max_freq <= (spi_freq * 5 / 2)) {
		/* /2.5 */
		div = 15;
	} else {
		/* /n */
		div = (max_freq + spi_freq - 1) / spi_freq - 1;
		if (div > 13) {
			div = 13;
		}
	}

	return div;
}

static void __acts_clk_set_rate_spi_ck64m(int clock_id, unsigned int rate_hz)
{
	unsigned int div, reg_val, real_rate;

	div = calc_spi_clk_div(MHZ(64), rate_hz) & 0xf;

	/* check CK64M has been enabled or not */
	if (!(sys_read32(CMU_S1CLKCTL) & (1 << 2))) {
		/* enable S1 CK64M */
		sys_write32(sys_read32(CMU_S1CLKCTL) | (1 << 2), CMU_S1CLKCTL);
		/* enable S1BT CK64M */
		sys_write32(sys_read32(CMU_S1BTCLKCTL) | (1 << 2), CMU_S1BTCLKCTL);
		k_busy_wait(10);
		/* calibrate CK64M clock */
		sys_write32(0xe, CK64M_CTL);
		/* wait calibration done */
		while(!(sys_read32(CK64M_CTL) & (1 << 8))) {
			;
		}
	}

	/* set SPIx clock source and divison */
	reg_val = sys_read32(CMU_SPI0CLK + ((clock_id - CLOCK_ID_SPI0) * 4));
	reg_val &= ~0x30f;
	reg_val |= (0x3 << 8) | (div << 0);

	if (div == 14)
		real_rate = MHZ(64) * 2 / 3;
	else if (div == 15)
		real_rate = MHZ(64) * 2 / 5;
	else
		real_rate = MHZ(64) / (div + 1);

	sys_write32(reg_val, CMU_SPI0CLK + ((clock_id - CLOCK_ID_SPI0) * 4));

	printk("SPI%d: set rate %d Hz real rate %d Hz\n",
			clock_id - CLOCK_ID_SPI0, rate_hz, real_rate);
}

#endif
static void  acts_clk_set_rate_spi(int clock_id, unsigned int rate_hz)
{
	unsigned int core_pll, val, real_rate, div;

#ifdef CONFIG_SOC_SPI0_USE_CK64M
	if (CLOCK_ID_SPI0 == clock_id)
		return __acts_clk_set_rate_spi_ck64m(CLOCK_ID_SPI0, rate_hz);
#endif

	if(rate_hz < CONFIG_HOSC_CLK_MHZ*1000000/2){
		div = (CONFIG_HOSC_CLK_MHZ*1000000+rate_hz-1)/rate_hz;
		real_rate = CONFIG_HOSC_CLK_MHZ*1000000/div;
		val = (div-1);
	}else{	
		core_pll = clk_rate_get_corepll();
		div = (core_pll+rate_hz-1)/rate_hz;
		real_rate = core_pll/div;
		val = (div-1)|(1<<8);
	}

	sys_write32(val, CMU_SPI0CLK + (clock_id - CLOCK_ID_SPI0)*4);

	printk("SPI%d: set rate %d Hz, real rate %d Hz,CMU_SPICLK=0x%x\n",
		clock_id - CLOCK_ID_SPI0, rate_hz, real_rate, val);
}


#ifdef CONFIG_SPIMT_ACTS

static void  acts_clk_set_rate_spimt(int clock_id, unsigned int rate_hz)
{
	unsigned int val, div, real_rate;

	if (rate_hz > 4000000) {
		rate_hz = 4000000;
	}
	div = 4000000 / rate_hz;
	real_rate = 4000000 / div;
	val = (0 << 8) | (div - 1);
	sys_write32(val, CMU_SPIMT0CLK + (clock_id - CLOCK_ID_SPIMT0)*4);

	printk("SPIMT%d: set rate %d Hz, real rate %d Hz\n",
		clock_id - CLOCK_ID_SPIMT0, rate_hz, real_rate);
}
#else
static void  acts_clk_set_rate_spimt(int clock_id, unsigned int rate_hz)
{

}
#endif

#ifdef CONFIG_DISPLAY_LCDC
static void acts_clk_set_rate_lcd(unsigned int rate_hz)
{
	uint32_t real_rate, div, pre_div;
	uint32_t src_sel, parent_clk;

	parent_clk = clk_rate_get_corepll();
	if (rate_hz < parent_clk / 72) {
		parent_clk = CONFIG_HOSC_CLK_MHZ * 1000000u;
		src_sel = 0;
	} else {
		src_sel = 1 << 8;
	}

	pre_div = (rate_hz < parent_clk / 12) ? 6 : 1;

	div = (parent_clk + rate_hz * pre_div - 1) / (rate_hz * pre_div);
	if (div < 1) div = 1;
	else if (div > 12) div = 12;

	real_rate = parent_clk / (div * pre_div);
	sys_write32(src_sel | (div - 1) | ((pre_div > 1) ? (1 << 4) : 0), CMU_LCDCLK);

	printk("LCD: set rate %d Hz, real rate %d Hz\n", rate_hz, real_rate);
}
#endif /* CONFIG_DISPLAY_LCDC */

#ifdef CONFIG_DISPLAY_ENGINE
static void acts_clk_set_rate_de(unsigned int rate_hz)
{
	uint32_t core_pll, real_rate, div2, div;

	core_pll = clk_rate_get_corepll();
	div2 = (core_pll * 2 + rate_hz - 1) / rate_hz;

	switch (div2) {
	case 3:
		div = 15; /* +1 */
		break;
	case 5:
		div = 16; /* +1 */
		break;
	default:
		div = div2 / 2;
		if (div < 1) div = 1;
		else if (div > 14) div = 14;

		div2 = div * 2;
		break;
	}

	real_rate = core_pll * 2 / div2;

	sys_write32((1 << 8) | (div - 1), CMU_DECLK);

	printk("DE: set rate %d Hz, real rate %d Hz\n", rate_hz, real_rate);
}
#endif /* CONFIG_DISPLAY_ENGINE */

#ifdef CONFIG_VG_LITE
static void acts_clk_set_rate_gpu(unsigned int rate_hz)
{
	uint32_t core_pll, real_rate, div2, div;

	core_pll = clk_rate_get_corepll();
	div2 = (core_pll * 2 + rate_hz - 1) / rate_hz;

	switch (div2) {
	case 3:
		div = 15; /* +1 */
		break;
	case 5:
		div = 16; /* +1 */
		break;
	default:
		div = div2 / 2;
		if (div < 1) div = 1;
		else if (div > 14) div = 14;

		div2 = div * 2;
		break;
	}

	real_rate = core_pll * 2 / div2;

	sys_write32((1 << 12) | (1 << 8) | (div - 1), CMU_GPUCLK);

	printk("GPU: set rate %d Hz, real rate %d Hz\n", rate_hz, real_rate);
}
#endif /* CONFIG_VG_LITE */

#ifdef CONFIG_JPEG_HW
static void acts_clk_set_rate_jpeg(unsigned int rate_hz)
{
	uint32_t core_pll, real_rate, div2, div;

	core_pll = clk_rate_get_corepll();
	div2 = (core_pll * 2 + rate_hz - 1) / rate_hz;

	switch (div2) {
	case 3:
		div = 15; /* +1 */
		break;
	case 5:
		div = 16; /* +1 */
		break;
	default:
		div = div2 / 2;
		if (div < 1) div = 1;
		else if (div > 14) div = 14;

		div2 = div * 2;
		break;
	}

	real_rate = core_pll * 2 / div2;

	sys_write32((1 << 8) | (div - 1), CMU_JPEGCLK);

	//printk("jpeg : set rate %d Hz, real rate %d Hz\n", rate_hz, real_rate);
}
#endif /* CONFIG_JPEG_HW */

int clk_set_rate(int clock_id,  uint32_t rate_hz)
{
	int ret = 0;
	switch(clock_id) {

	case CLOCK_ID_SD0:
	case CLOCK_ID_SD1:
		acts_clk_set_rate_sd(clock_id-CLOCK_ID_SD0, rate_hz);
		break;
	case CLOCK_ID_DMA:
		break;
	case CLOCK_ID_SPI0:
	case CLOCK_ID_SPI1:
	case CLOCK_ID_SPI2:
	case CLOCK_ID_SPI3:
		acts_clk_set_rate_spi(clock_id, rate_hz);
		break;
	case CLOCK_ID_SPIMT0:
	case CLOCK_ID_SPIMT1:
		acts_clk_set_rate_spimt(clock_id, rate_hz);
		break;

#ifdef CONFIG_DISPLAY_LCDC
	case CLOCK_ID_LCD:
		acts_clk_set_rate_lcd(rate_hz);
		break;
#endif

#ifdef CONFIG_DISPLAY_ENGINE
	case CLOCK_ID_DE:
		acts_clk_set_rate_de(rate_hz);
		break;
#endif

#ifdef CONFIG_VG_LITE
	case CLOCK_ID_GPU:
		acts_clk_set_rate_gpu(rate_hz);
		break;
#endif
#ifdef CONFIG_JPEG_HW
	case CLOCK_ID_JPEG:
		acts_clk_set_rate_jpeg(rate_hz);
		break;	
#endif

	case CLOCK_ID_I2CMT0:
	case CLOCK_ID_I2CMT1:
	case CLOCK_ID_SPI0CACHE:
	case CLOCK_ID_SPI1CACHE:
	case CLOCK_ID_USB:
	case CLOCK_ID_USB2:
	case CLOCK_ID_SE:
	case CLOCK_ID_LRADC:
	case CLOCK_ID_UART0:
	case CLOCK_ID_UART1:
	case CLOCK_ID_UART2:
	case CLOCK_ID_I2C0:
	case CLOCK_ID_I2C1:
	case CLOCK_ID_DSP:
	case CLOCK_ID_ASRC:
	case CLOCK_ID_DAC:
	case CLOCK_ID_ADC:
	case CLOCK_ID_I2STX:
	case CLOCK_ID_I2SRX:
		printk("clkid=%d not support clk set\n",clock_id);
		ret = -1;
		break;

	}

	return 0;
}

uint32_t clk_get_rate(int clock_id)
{

	uint32_t rate = 0;
	switch(clock_id) {
	case CLOCK_ID_SD0:
	case CLOCK_ID_SD1:
	case CLOCK_ID_DMA:
	case CLOCK_ID_SPI0:
	case CLOCK_ID_SPI1:
	case CLOCK_ID_SPI2:
	case CLOCK_ID_SPI3:
	case CLOCK_ID_SPI0CACHE:
	case CLOCK_ID_SPI1CACHE:
	case CLOCK_ID_USB:
	case CLOCK_ID_USB2:
	case CLOCK_ID_DE:
	case CLOCK_ID_LCD:
	case CLOCK_ID_SE:
	case CLOCK_ID_LRADC:
	case CLOCK_ID_UART0:
	case CLOCK_ID_UART1:
	case CLOCK_ID_UART2:
	case CLOCK_ID_I2C0:
	case CLOCK_ID_I2C1:
	case CLOCK_ID_DSP:
	case CLOCK_ID_ASRC:
	case CLOCK_ID_DAC:
	case CLOCK_ID_ADC:
	case CLOCK_ID_I2STX:
	case CLOCK_ID_I2SRX:
		printk("clkid=%d not support clk get\n",clock_id);
		break;

	}

	return rate;

}


/*set cpu clk, return old clk*/
#if 0
uint32_t clk_cpu_set(uint32_t mhz)
{

	unsigned int core_pll, div, tmp, real_rate;
	core_pll = (sys_read32(COREPLL_CTL)&0x3F)*80;
	tmp = core_pll/mhz;
	if(tmp > 13){// div 1.5
		real_rate = core_pll/15;
		div = 14;
	} else if ( (tmp < 28) && (tmp > 22) ) { // div 2.5
		real_rate = core_pll/25;
		div = 15;

	}else if( tmp > 140){
		real_rate = core_pll/14;
		div = 13;
	}else{
		div = (tmp/10)-1;
		real_rate = core_pll/10;
	}
	tmp = sys_read32(CMU_SYSCLK);
	sys_write32((old & (~(0x3<<8))) | val , CMU_SYSCLK);
	printk("cpu: set rate %d MHz, real rate %d MHz, core pll=%d MHZ\n", mhz, real_rate, core_pll);

}
#endif
/*set ahb div, return old div*/
uint32_t clk_ahb_set(uint32_t div)
{
	uint32_t val, old;
	old = sys_read32(CMU_SYSCLK);
	if(div == 1)
		val = 1<<8;
	else if (div == 4)
		val = 3<<8;
	else
		val = 0;
	div = (old >>8) & 0x03;
	sys_write32((old & (~(0x3<<8))) | val , CMU_SYSCLK);
	return div;
}




