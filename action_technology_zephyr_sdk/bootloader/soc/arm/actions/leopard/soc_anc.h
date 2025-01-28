/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_ANC_H_
#define SOC_ANC_H_

#include <os_common_api.h>
#include <stdint.h>
#include <soc_regs.h>
#include <soc_irq.h>
#include <rbuf/rbuf_mem.h>

/* if set, the user can externally shut down the ANC root clock (gated by CMU) */
#define PSU_ANC_IDLE BIT(25)
/* if set, only the ANC Internal core clock is gated */
#define PSU_ANC_CORE_IDLE BIT(24)

#define ANC_STATUS_EXT_CTL_ANC_WAIT_EN          	  (1 << 0)
#define ANC_STATUS_EXT_CTL_ANC_EXTERNAL_WAIT          (1 << 1)

#define DTCM_AND_PTCM 0
#define ALL_EXT_MEM   1

#define CODE_ADDR 1
#define DATA_ADDR 0

#define ANC_DTCM_SIZE		0x8000
#define CPU_ANC_DTCM_BASE	0x01070000
#define ANC_ANC_DTCM_BASE	0

#define ANC_PTCM_SIZE		0x4000
#define CPU_ANC_PTCM_BASE	0x01078000
#define ANC_ANC_PTCM_BASE	0

#define ANC_EXTMEM_SIZE		0x2400
#define CPU_ANC_EXTMEM_BASE	0x0107c000
#define ANC_ANC_EXTMEM_BASE	0x00010000

#define ANC_SHARE_MEM_SIZE	0x7FFF
#define CPU_SHARE_BASE		0x1068000
#define ANC_SHARE_BASE		0x20000


static inline unsigned int mcu_to_anc_address(unsigned int mcu_addr, uint8_t addr_type)
{
	int anc_addr = 0;

	/** ptcm */
	if (mcu_addr >= CPU_ANC_PTCM_BASE && mcu_addr <= CPU_ANC_PTCM_BASE + ANC_PTCM_SIZE && addr_type == CODE_ADDR) {
		anc_addr = ANC_ANC_PTCM_BASE + (mcu_addr - CPU_ANC_PTCM_BASE) / 2;
	} else
	/** dtcm */
	if(mcu_addr >= CPU_ANC_DTCM_BASE && mcu_addr <= CPU_ANC_DTCM_BASE + ANC_DTCM_SIZE && addr_type == DATA_ADDR) {
		anc_addr = ANC_ANC_DTCM_BASE + (mcu_addr - CPU_ANC_DTCM_BASE) / 2;
	} else
	/** ext mem  */
	if(mcu_addr >= CPU_ANC_EXTMEM_BASE && mcu_addr <= CPU_ANC_EXTMEM_BASE + ANC_EXTMEM_SIZE) {
		anc_addr = ANC_ANC_EXTMEM_BASE + (mcu_addr - CPU_ANC_EXTMEM_BASE) / 2;
	}
	/*share mem*/
	if(mcu_addr >= CPU_SHARE_BASE && mcu_addr <= CPU_SHARE_BASE + ANC_SHARE_MEM_SIZE){
		anc_addr = ANC_SHARE_BASE + (mcu_addr - CPU_SHARE_BASE) / 2;
	}
	//printk("mcu_addr %x anc_addr %x addr_type %d \n",mcu_addr,ANC_addr,addr_type);
	return anc_addr;
}

static inline unsigned int anc_to_mcu_address(unsigned int anc_addr, uint8_t addr_type)
{
	int mcu_addr = -1;

	/** ptcm */
	if (anc_addr >= ANC_ANC_PTCM_BASE && anc_addr <= ANC_ANC_PTCM_BASE + ANC_PTCM_SIZE && addr_type == CODE_ADDR) {
		mcu_addr = CPU_ANC_PTCM_BASE + (anc_addr - ANC_ANC_PTCM_BASE) * 2;
	} else
	/** dtcm */
	if(anc_addr >= ANC_ANC_DTCM_BASE && anc_addr <= ANC_ANC_DTCM_BASE + ANC_DTCM_SIZE && addr_type == DATA_ADDR) {
		mcu_addr = CPU_ANC_DTCM_BASE + (anc_addr - ANC_ANC_DTCM_BASE) * 2;
	} else
	/** ext mem  */
	if(anc_addr >= ANC_ANC_EXTMEM_BASE && anc_addr <= ANC_ANC_EXTMEM_BASE + ANC_EXTMEM_SIZE) {
		mcu_addr = CPU_ANC_EXTMEM_BASE + (anc_addr - ANC_ANC_EXTMEM_BASE) * 2;
	}
	//printk("anc_addr %x mcu_addr %x addr_type %d \n",anc_addr,mcu_addr,addr_type);
	return mcu_addr;
}

static inline unsigned int mcu_to_anc_data_address(unsigned int mcu_addr)
{
	return mcu_to_anc_address(mcu_addr, DATA_ADDR);
}

static inline int get_hw_idle(void)
{
	return sys_read32(ANC_STATUS_EXT_CTL) & PSU_ANC_IDLE;
}


static inline void anc_do_wait(void)
{
    sys_write32(sys_read32(ANC_STATUS_EXT_CTL)
					| BIT(ANC_STATUS_EXT_CTL_ANC_WAIT_EN)
					| BIT(ANC_STATUS_EXT_CTL_ANC_EXTERNAL_WAIT),
					ANC_STATUS_EXT_CTL);
}

static inline void anc_undo_wait(void)
{
    sys_write32(sys_read32(ANC_STATUS_EXT_CTL)
					& ~(BIT(ANC_STATUS_EXT_CTL_ANC_WAIT_EN)
					| BIT(ANC_STATUS_EXT_CTL_ANC_EXTERNAL_WAIT)),
					ANC_STATUS_EXT_CTL);
}

static inline void anc_init_clk(void)
{
	/*
	bit20:ANCDSPDMACLK
	bit14:ANCDSPTIMERCLKEN
	bit13:ANCDSPCLKEN
	bit12:ANCFFIIRCLKEN
	bit11:ANCDSCLKEN
	bit10:ANCUSCLKEN
	*/
	sys_write32(sys_read32(CMU_DEVCLKEN1) | ((1<<20) | (0x1f<<10)), CMU_DEVCLKEN1);

	//enable anc clk, pll0
	sys_write32(0xf0000, CMU_ANCCLK);

	//enable anc clk, pll1
	//sys_write32(0xf0100, CMU_ANCCLK);

	//enable ancdsp clk
	sys_write32(0x32, CMU_ANCDSPCLK);

	sys_write32(sys_read32(AUDIO_PLL0_CTL) | 0x10, AUDIO_PLL0_CTL);
}

static int anc_check_hw_idle(void)
{
	return sys_read32(ANC_STATUS_EXT_CTL) & PSU_ANC_IDLE;
}


static inline int anc_soc_request_mem(void)
{
	/*ptcm dtcm ram switch to anc*/
	sys_write32(sys_read32(CMU_MEMCLKSRC1) | 0x00000001, CMU_MEMCLKSRC1);

	return 0;
}

static inline int anc_soc_release_mem(void)
{
	/*ptcm dtcm ram switch to mcu*/
	sys_write32(sys_read32(CMU_MEMCLKSRC1) & (~0x00000001), CMU_MEMCLKSRC1);
	return 0;
}

static inline void anc_soc_request_reg(void)
{
	sys_write32(0x01, ALL_REG_ACCESS_SEL);
}

static inline void anc_soc_release_reg(void)
{
	sys_write32(0x00, ALL_REG_ACCESS_SEL);
}

static void inline set_anc_vector_addr(unsigned int anc_addr)
{
    sys_write32(anc_addr, ANC_VCT_ADDR);
}

static inline void clear_anc_irq_pending(unsigned int irq)
{
	sys_write32(0x3, PENDING_FROM_ANC_DSP);
}

static inline void clear_anc_all_irq_pending(void)
{
	sys_write32(0x3, PENDING_FROM_ANC_DSP);
}

static inline int do_request_anc(int in_user)
{
    //clear info
    sys_write32(in_user, INFO_TO_ANC_DSP);

    // info
    sys_write32(1, INT_TO_ANC_DSP);

    return 0;
}

static inline void mcu_trigger_irq_to_anc(void)
{
	// info
 	sys_write32(0, INT_TO_ANC_DSP);

	for (int i = 0; i < 10; i ++) {
		;
	}
   	 sys_write32(1, INT_TO_ANC_DSP);
}

static inline void mcu_untrigger_irq_to_anc(void)
{
    	// info
	// sys_write32(0, INT_TO_ANC_DSP);
}



static inline void anc_reset_enable(void)
{
	int i;
	sys_write32(sys_read32(RMU_MRCR1) & ~((1<<30) | (1<<13) | (1<<11) | (1<<10)),RMU_MRCR1);
	for(i=0;i<20;i++);
	sys_write32(sys_read32(RMU_MRCR1) | ((1<<30) | (1<<13) | (1<<11) | (1<<10)),RMU_MRCR1);
	for(i=0;i<20;i++);
}

static inline int anc_dump_info(void)
{
	printk("0x%x: ANC_VCT_ADDR 		0x%x \n",ANC_VCT_ADDR, sys_read32(ANC_VCT_ADDR));
	printk("0x%x: PENDING_FROM_ANC 		0x%x \n",PENDING_FROM_ANC_DSP, sys_read32(PENDING_FROM_ANC_DSP));
	printk("0x%x: INFO_TO_ANC 		0x%x \n",INFO_TO_ANC_DSP, sys_read32(INFO_TO_ANC_DSP));
	printk("0x%x: ANC_VCT_ADDR 		0x%x \n",ANC_VCT_ADDR, sys_read32(ANC_VCT_ADDR));

	printk("0x%x: RMU_MRCR1 		0x%x \n",RMU_MRCR1, sys_read32(RMU_MRCR1));
	printk("0x%x: CMU_DEVCLKEN1 		0x%x \n",CMU_DEVCLKEN1, sys_read32(CMU_DEVCLKEN1));
	printk("0x%x: CMU_MEMCLKSRC0 		0x%x \n",CMU_MEMCLKSRC0, sys_read32(CMU_MEMCLKSRC0));
	printk("0x%x: CMU_MEMCLKSRC1 		0x%x \n",CMU_MEMCLKSRC1, sys_read32(CMU_MEMCLKSRC1));
	printk("0x%x: CMU_MEMCLKSRC2 		0x%x \n",CMU_MEMCLKSRC2, sys_read32(CMU_MEMCLKSRC2));
	printk("0x%x: MEMORYCTL 		0x%x \n",MEMORY_CTL, sys_read32(MEMORY_CTL));

	printk("0x%x: PWRGATE_DIG 		0x%x \n",PWRGATE_DIG, sys_read32(PWRGATE_DIG));
	printk("0x%x: CMU_ANCDSPCLK 		0x%x \n",CMU_ANCDSPCLK, sys_read32(CMU_ANCDSPCLK));
	printk("0x%x: COREPLL_CTL 		0x%x \n",COREPLL_CTL, sys_read32(COREPLL_CTL));
	printk("0x%x: CMU_SYSCLK 		0x%x \n",CMU_SYSCLK, sys_read32(CMU_SYSCLK));
	printk("0x%x: CMU_ADCCLK 		0x%x \n",CMU_ADCCLK, sys_read32(CMU_ADCCLK));
	printk("0x%x: AUDIOLDO_CTL 		0x%x \n",AUDIOLDO_CTL, sys_read32(AUDIOLDO_CTL));
	printk("0x%x: AUDIO_PLL0_CTL 		0x%x \n",AUDIO_PLL0_CTL, sys_read32(AUDIO_PLL0_CTL));
	printk("0x%x: AUDIO_PLL1_CTL 		0x%x \n",AUDIO_PLL1_CTL, sys_read32(AUDIO_PLL1_CTL));
	printk("0x%x: CMU_ANCCLK 		0x%x \n",CMU_ANCCLK, sys_read32(CMU_ANCCLK));
	printk("0x%x: ALL_REG_ACCESS_SEL 	0x%x \n",ALL_REG_ACCESS_SEL, sys_read32(ALL_REG_ACCESS_SEL));
	return 0;
}
#endif /* SOC_ANC_H_ */
