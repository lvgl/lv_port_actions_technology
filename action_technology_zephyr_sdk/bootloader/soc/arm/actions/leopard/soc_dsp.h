/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_DSP_H_
#define SOC_DSP_H_

#include <os_common_api.h>
#include <stdint.h>
#include <soc_regs.h>
#include <soc_irq.h>
#include <rbuf/rbuf_mem.h>

/* if set, the user can externally shut down the DSP root clock (gated by CMU) */
#define PSU_DSP_IDLE BIT(25)
/* if set, only the DSP Internal core clock is gated */
#define PSU_DSP_CORE_IDLE BIT(24)

#define DSP_STATUS_EXT_CTL_DSP_WAIT_EN          	  (1 << 0)
#define DSP_STATUS_EXT_CTL_DSP_EXTERNAL_WAIT          (1 << 1)

#define DSP_MAILBOX_REG_BASE   0x106A000
#define DSP_M2D_MAILBOX_REGISTER_BASE       (DSP_MAILBOX_REG_BASE + 0x00)
#define DSP_D2M_MAILBOX_REGISTER_BASE       (DSP_MAILBOX_REG_BASE + 0x10)
#define DSP_USER_REGION_REGISTER_BASE       (DSP_MAILBOX_REG_BASE + 0x20)
#define DSP_DEBUG_REGION_REGISTER_BASE      (DSP_MAILBOX_REG_BASE + 0x40)

#define DTCM_AND_PTCM 0
#define ALL_EXT_MEM   1

#define CODE_ADDR 1
#define DATA_ADDR 0

#define DTCM_SIZE      0xBFFF
#define CPU_DTCM_BASE  0x1100000
#define DSP_DTCM_BASE  0

#define PTCM_SIZE	   0x1FFF
#define CPU_PTCM_BASE  0x1140000
#define DSP_PTCM_BASE  0

#define DSP_SHARE_MEM_SIZE  0x7FFF
#define CPU_SHARE_BASE  0x1068000
#define DSP_SHARE_BASE  0x40044000

#define EXT_MEM_SIZE  0x17FFF
#define CPU_EXT_BASE  0x1040000
#define DSP_EXT_BASE  0x40000000
#define IMG_BANK_INNER_ADDR(addr) (addr & 0x4003ffff)

static inline unsigned int mcu_to_dsp_address(unsigned int mcu_addr, uint8_t addr_type)
{
	int dsp_addr = 0;

	/** ptcm */
	if (mcu_addr >= CPU_PTCM_BASE && mcu_addr <= CPU_PTCM_BASE + PTCM_SIZE && addr_type == CODE_ADDR) {
		dsp_addr = DSP_PTCM_BASE + (mcu_addr - CPU_PTCM_BASE) / 2;
	} else 
	/** dtcm */
	if(mcu_addr >= CPU_DTCM_BASE && mcu_addr <= CPU_DTCM_BASE + DTCM_SIZE && addr_type == DATA_ADDR) {
		dsp_addr = DSP_DTCM_BASE + (mcu_addr - CPU_DTCM_BASE) / 2;
	} else
	/** ext mem  */
	if(mcu_addr >= CPU_EXT_BASE && mcu_addr <= CPU_EXT_BASE + EXT_MEM_SIZE) {
		dsp_addr = DSP_EXT_BASE + (mcu_addr - CPU_EXT_BASE) / 2;
	} else 
	/** share ram */
	if (mcu_addr >= CPU_SHARE_BASE && mcu_addr <= CPU_SHARE_BASE + DSP_SHARE_MEM_SIZE) {
		dsp_addr = DSP_SHARE_BASE + (mcu_addr - CPU_SHARE_BASE) / 2;
	}

	//printk("mcu_addr %x dsp_addr %x addr_type %d \n",mcu_addr,dsp_addr,addr_type);
	return dsp_addr;
}

static inline unsigned int dsp_to_mcu_address(unsigned int dsp_addr, uint8_t addr_type)
{
	int mcu_addr = -1;

	/** ptcm */
	if (dsp_addr >= DSP_PTCM_BASE && dsp_addr <= DSP_PTCM_BASE + PTCM_SIZE && addr_type == CODE_ADDR) {
		mcu_addr = CPU_PTCM_BASE + (dsp_addr - DSP_PTCM_BASE) * 2;
	} else 
	/** dtcm */
	if(dsp_addr >= DSP_DTCM_BASE && dsp_addr <= DSP_DTCM_BASE + DTCM_SIZE && addr_type == DATA_ADDR) {
		mcu_addr = CPU_DTCM_BASE + (dsp_addr - DSP_DTCM_BASE) * 2;
	} else
	/** ext mem  */
	if(dsp_addr >= DSP_EXT_BASE && dsp_addr <= DSP_EXT_BASE + EXT_MEM_SIZE) {
		mcu_addr = CPU_EXT_BASE + (dsp_addr - DSP_EXT_BASE) * 2;
	} else 
	/** share ram */
	if (dsp_addr >= DSP_SHARE_BASE && dsp_addr <= DSP_SHARE_BASE + DSP_SHARE_MEM_SIZE / 2) {
		mcu_addr = CPU_SHARE_BASE + (dsp_addr - DSP_SHARE_BASE) * 2;
	} else {
		dsp_addr = IMG_BANK_INNER_ADDR(dsp_addr);
		/** bank ram */
		if (dsp_addr >= 0x4000 && dsp_addr <= 0x4000 + EXT_MEM_SIZE / 2) {
			mcu_addr = CPU_EXT_BASE + dsp_addr * 2;
		}
	}
	//printk("dsp_addr %x mcu_addr %x addr_type %d \n",dsp_addr,mcu_addr,addr_type);
	return mcu_addr;
}

static inline unsigned int mcu_to_dsp_data_address(unsigned int mcu_addr)
{
	return mcu_to_dsp_address(mcu_addr, DATA_ADDR);
}

static inline int get_hw_idle(void)
{
	return sys_read32(DSP_STATUS_EXT_CTL) & PSU_DSP_IDLE;
}


static inline void dsp_do_wait(void)
{
    sys_write32(sys_read32(DSP_STATUS_EXT_CTL) 
					| BIT(DSP_STATUS_EXT_CTL_DSP_WAIT_EN)
					| BIT(DSP_STATUS_EXT_CTL_DSP_EXTERNAL_WAIT),
					DSP_STATUS_EXT_CTL);
}

static inline void dsp_undo_wait(void)
{
    sys_write32(sys_read32(DSP_STATUS_EXT_CTL) 
					& ~(BIT(DSP_STATUS_EXT_CTL_DSP_WAIT_EN)
					| BIT(DSP_STATUS_EXT_CTL_DSP_EXTERNAL_WAIT)),
					DSP_STATUS_EXT_CTL);
}

static inline void clear_dsp_pageaddr(void)
{
    sys_write32(0, DSP_PAGE_ADDR0);
    sys_write32(0, DSP_PAGE_ADDR0 + 4);
    sys_write32(0, DSP_PAGE_ADDR0 + 8);
    sys_write32(0, DSP_PAGE_ADDR0 + 12);
}

static inline void dsp_init_clk(void)
{
	sys_write32(sys_read32(CMU_DEVCLKEN1) | 0x01, CMU_DEVCLKEN1);
#ifndef CONFIG_SOC_EP_MODE
	sys_write32(0x12, CMU_DSPCLK);
#else
	sys_write32(0xe3, CMU_DSPCLK);
#endif
}

static inline int dsp_check_hw_idle(void)
{
	return sys_read32(DSP_STATUS_EXT_CTL) & PSU_DSP_IDLE;
}

static inline int dsp_soc_request_addr(int cpu_addr)
{
	if (cpu_addr >= CPU_EXT_BASE && cpu_addr < CPU_EXT_BASE + EXT_MEM_SIZE) {
		sys_write32(sys_read32(CMU_MEMCLKSRC0) | (1 << 24) | (1 << 22) | (1 << 20), CMU_MEMCLKSRC0);		
	}
	return 0;
}

static inline int dsp_soc_release_addr(int cpu_addr)
{
	if (cpu_addr >= CPU_EXT_BASE && cpu_addr < CPU_EXT_BASE + EXT_MEM_SIZE) {
		sys_write32(sys_read32(CMU_MEMCLKSRC0) & (~((1 << 24) | (1 << 22) | (1 << 20))), CMU_MEMCLKSRC0);	
	} 
	return 0;
}
static inline int dsp_soc_request_mem(int type)
{
	sys_write32((sys_read32(CMU_MEMCLKSRC0) | (1 << 24) | (1 << 22) | (1 << 20) | (1 << 12)),CMU_MEMCLKSRC0);

	return 0;
}

static inline int dsp_soc_release_mem(int type)
{
	sys_write32((sys_read32(CMU_MEMCLKSRC0) & (~((1 << 24) | (1 << 22) | (1 << 20) | (1 << 12)))), CMU_MEMCLKSRC0);

	return 0;
}

static inline void mem_controller_dsp_pageaddr_set(uint32_t index, uint32_t value)
{
    sys_write32(value, DSP_PAGE_ADDR0 + index * 4);
}

static void inline set_dsp_vector_addr(unsigned int dsp_addr)
{
    sys_write32(dsp_addr, DSP_VCT_ADDR);
}

static inline void clear_dsp_irq_pending(unsigned int irq)
{
	sys_write32(0x1, PENDING_FROM_DSP);

	while(sys_read32(PENDING_FROM_DSP));

}

static inline void clear_dsp_all_irq_pending(void)
{
    sys_write32(0x1, PENDING_FROM_DSP);
}

static inline int do_request_dsp(int in_user)
{
    //clear info
    sys_write32(in_user, INFO_TO_DSP);

    // info
 	sys_write32(0, INT_TO_DSP);

	for (int i = 0; i < 10; i ++) {
		;
	}

    sys_write32(1, INT_TO_DSP);

    return 0;
}

static inline void mcu_trigger_irq_to_dsp(void)
{
    // info
 	sys_write32(0, INT_TO_DSP);

	for (int i = 0; i < 10; i ++) {
		;
	}
    sys_write32(1, INT_TO_DSP);
}

static inline void mcu_untrigger_irq_to_dsp(void)
{
    // info
    //sys_write32(0, INT_TO_DSP);
}

static inline int dsp_dump_info(void)
{
	printk("DSP_PAGE_ADDR0 0x%x \n",sys_read32(DSP_PAGE_ADDR0));
	printk("DSP_PAGE_ADDR1 0x%x \n",sys_read32(DSP_PAGE_ADDR0 + 4));
	printk("DSP_PAGE_ADDR2 0x%x \n",sys_read32(DSP_PAGE_ADDR0 + 8));
	printk("DSP_PAGE_ADDR3 0x%x \n",sys_read32(DSP_PAGE_ADDR0 + 12));
	printk("DSP_VCT_ADDR 0x%x \n",sys_read32(DSP_VCT_ADDR));
	printk("PENDING_FROM_DSP 0x%x \n",sys_read32(PENDING_FROM_DSP));
	printk("INFO_TO_DSP 0x%x \n",sys_read32(INFO_TO_DSP));
	printk("DSP_VCT_ADDR 0x%x \n",sys_read32(DSP_VCT_ADDR));

	printk("MRCR1 0x%x \n",sys_read32(0x40000004));
	printk("CMU_DEVCLKEN1 0x%x \n",sys_read32(CMU_DEVCLKEN1));
	printk("CMU_MEMCLKSRC0 0x%x \n",sys_read32(CMU_MEMCLKSRC0));
	printk("CMU_MEMCLKSRC1 0x%x \n",sys_read32(CMU_MEMCLKSRC1));
	printk("CMU_MEMCLKSRC2 0x%x \n",sys_read32(CMU_MEMCLKSRC2));
	printk("MEMORYCTL 0x%x \n",sys_read32(MEMORY_CTL));

	printk("PWRGATE_DIG 0x%x \n",sys_read32(0x40004000 + 0x30));
	printk("CMU_DSPCLK 0x%x \n",sys_read32(0x40001000 +0x90));
	printk("COREPLL_CTL 0x%x \n",sys_read32(COREPLL_CTL));
	printk("CMU_SYSCLK 0x%x \n",sys_read32(CMU_SYSCLK));
	return 0;
}
#endif /* SOC_DSP_H_ */
