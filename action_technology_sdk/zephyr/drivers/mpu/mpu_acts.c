/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief actions mpu
 */

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(mpu_acts);


#include <kernel.h>
#include <init.h>
#include <device.h>
#include <linker/linker-defs.h>
#include <irq.h>
#include <drivers/mpu_acts.h>
#include <soc.h>
#include <string.h>
#include <board_cfg.h>


static void _mpu_protect_disable(void)
{
	sys_write32(0, MPUIE);
}

static void _mpu_clear_all_pending(void)
{
	sys_write32(0xffffffff, MPUIP);
}

static s8_t mpu_enable_count[4];

void mpu_set_address(u32_t start, u32_t end, u32_t flag, u32_t index)
{
	mpu_base_register_t *base_register = (mpu_base_register_t *)MPUCTL0;
	if(index >= 4){
		LOG_ERR("index =%d >=4\n", index);
		return;
	}
	base_register += index;
	base_register->MPUBASE = start;
	base_register->MPUEND = end;
	base_register->MPUCTL = ((flag) << 1);
	sys_write32((sys_read32(MPUIE) & ~(0x1f << 8*index)) | (flag << 8*index), MPUIE);

}

static void _mpu_protect_init(void)
{
#ifdef CONFIG_MPU_MONITOR_CACHECODE_WRITE
	/* protect rom section */
	mpu_set_address((unsigned int)_image_rom_start, (unsigned int)_image_rom_end - 1,
		(MPU_CPU_WRITE), CONFIG_MPU_MONITOR_CACHECODE_WRITE_INDEX);
	mpu_enable_region(CONFIG_MPU_MONITOR_CACHECODE_WRITE_INDEX);
#endif

#ifdef CONFIG_MPU_MONITOR_FLASH_AREA_WRITE
	/* protect flash cache all address section */
	mpu_set_address(CONFIG_MPU_MONITOR_FLASH_AREA_BASE, CONFIG_MPU_MONITOR_FLASH_AREA_END - 1,
		(MPU_CPU_WRITE), CONFIG_MPU_MONITOR_CACHECODE_WRITE_INDEX);
	mpu_enable_region(CONFIG_MPU_MONITOR_CACHECODE_WRITE_INDEX);
#endif


#ifdef CONFIG_MPU_MONITOR_RAMFUNC_WRITE
	/*  protect ram function  section*/
	mpu_set_address((unsigned int)__ramfunc_start, (unsigned int)__ramfunc_end - 1,
	(MPU_CPU_WRITE | MPU_DMA_WRITE), CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX);
	mpu_enable_region(CONFIG_MPU_MONITOR_RAMFUNC_WRITE_INDEX);
#endif

#ifdef CONFIG_ACTIONS_ARM_MPU
	extern void act_mpu_set(uint32_t chan, uint32_t mem_base, uint32_t size, uint32_t attr);
	act_mpu_set(6, 0x0, 0x10000, 2); //exe, ro
#else
	#ifdef CONFIG_MPU_MONITOR_ROMFUNC_WRITE
		/* protect rom addr section */
		mpu_set_address(0x0, 0x10000 - 1,
			(MPU_CPU_WRITE), CONFIG_MPU_MONITOR_ROMFUNC_WRITE_INDEX);
		mpu_enable_region(CONFIG_MPU_MONITOR_ROMFUNC_WRITE_INDEX);
	#endif
#endif
	_mpu_clear_all_pending();
}


void dma_dump_info(void);
static int mpu_analyse(void)
{
	unsigned int mpux_pending,pending, addr;
	int  i; //, dma, len;
	mpu_base_register_t *base_register;
	//struct dma_regs *dma_base_regs;

	pending = sys_read32(MPUIP);
	if(pending == 0)
		return 0;

	LOG_ERR("mpu pending:0x%x, IE=0x%x\n", pending, sys_read32(MPUIE));
	_mpu_protect_disable();	
	base_register = (mpu_base_register_t *)MPUCTL0;
	for (i = 0; i < CONFIG_MPU_ACTS_MAX_INDEX; i++) {
		mpux_pending = (sys_read32(MPUIP) >> (i*8)) & 0x3f;
		if (!mpux_pending)
			continue;
		
		addr = base_register[i].MPUERRADDR;
		if (mpux_pending & MPU_CPU_WRITE) {
			LOG_ERR("Warning:%d invalid cpu wirte addr=0x%x!\n", i, addr);
		}else if (mpux_pending & MPU_DMA_WRITE) {
			LOG_ERR("Warning:%d invalid dma wirte addr=0x%x!\n", i, addr);
			#if defined(CONFIG_DMA_DBG_DUMP)
				dma_dump_info();
			#endif
		} else /*if(mpux_pending & MPU_DMA_WRITE)*/ {
			LOG_ERR("Warning:invalid access  %d  pd=0x%x addr=0x%x!\n", i, mpux_pending, addr);
		}

	}

	k_panic();

	return 0;
}

void mpu_protect_clear_pending(int mpu_no)
{
	if (mpu_no < 4) {
		sys_write32(0x1f << (8 * mpu_no), MPUIP);
	} 
}

void mpu_enable_region(unsigned int index)
{
	mpu_base_register_t *base_register = (mpu_base_register_t *)MPUCTL0;
	u32_t flags;

	flags = irq_lock();
	if (mpu_enable_count[index] == 0) {
		base_register += index;

		base_register->MPUCTL |= (0x01);
	}
	mpu_enable_count[index]++;
	irq_unlock(flags);
}

void mpu_disable_region(unsigned int index)
{
	mpu_base_register_t *base_register = (mpu_base_register_t *)MPUCTL0;
	u32_t flags;

	flags = irq_lock();
	mpu_enable_count[index]--;
	if (mpu_enable_count[index] == 0) {
		base_register += index;

		base_register->MPUCTL &= (~(0x01));
	}
	irq_unlock(flags);
}

int mpu_region_is_enable(unsigned int index)
{
	mpu_base_register_t *base_register = (mpu_base_register_t *)MPUCTL0;
	base_register += index;
	return ((base_register->MPUCTL & 0x01) == 1);
}

#ifdef CONFIG_MPU_MONITOR_USER_DATA
int mpu_user_data_monitor(unsigned int start_addr, unsigned int end_addr, int mpu_user_no)
{
    /* protect text section*/
	mpu_set_address((unsigned int)start_addr, (unsigned int)end_addr - 1,
		(MPU_CPU_WRITE | MPU_DMA_WRITE), CONFIG_MPU_MONITOR_USER_DATA_INDEX + mpu_user_no);
	mpu_enable_region(CONFIG_MPU_MONITOR_USER_DATA_INDEX + mpu_user_no);
	return 0;
}

int mpu_user_data_monitor_stop(int mpu_user_no)
{
	mpu_disable_region(CONFIG_MPU_MONITOR_USER_DATA_INDEX + mpu_user_no);
	return 0;
}
#endif


void mpu_handler(void *arg)
{
	mpu_analyse();
	_mpu_clear_all_pending();
}

static int mpu_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	_mpu_protect_init();
	_mpu_clear_all_pending();
	LOG_INF("mpu init\n");

#ifdef CONFIG_MPU_IRQ_DRIVEN
	IRQ_CONNECT(IRQ_ID_MPU, CONFIG_MPU_IRQ_PRI, mpu_handler, 0, 0);
	irq_enable(IRQ_ID_MPU);
#endif

#ifdef CONFIG_MPU_EXCEPTION_DRIVEN
	sys_write32(sys_read32(MEMORYCTL) | MEMORYCTL_BUSERROR_BIT, MEMORYCTL);
#endif
	LOG_INF("mpu init end\n");
	return 0;
}

//SYS_INIT(mpu_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
SYS_INIT(mpu_init, APPLICATION, 20);






