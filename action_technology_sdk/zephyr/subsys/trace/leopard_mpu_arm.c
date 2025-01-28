/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file arm mpu
 */

#include <kernel.h>
#include <string.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <act_arm_mpu.h>
#include <linker/linker-defs.h>
#include <soc.h>

/* Global MAIR configurations */
#define MPU_MAIR_INDEX_NC        0

#define MPU_RBAR_AP_Pos                     1U
/* Privileged Read Write, Unprivileged Read Write */
#define P_RW_U_RW       0x1
#define P_RW_U_RW_Msk  (P_RW_U_RW << MPU_RBAR_AP_Pos)
/* Privileged Read Only, Unprivileged Read Only */
#define P_RO_U_RO       0x3
#define P_RO_U_RO_Msk   (P_RO_U_RO << MPU_RBAR_AP_Pos)

#define MPU_RBAR_SH_Pos                     3U
#define INNER_SHAREABLE 					0x3
#define INNER_SHAREABLE_Msk 	(INNER_SHAREABLE << MPU_RBAR_SH_Pos)

/* Attribute flag for not-allowing execution (eXecute Never) */
#define NOT_EXEC  0x01


static const char *mpu_attr_str[3] = { "NO", "RW", "RO"};


/*MPU set, mem_base= must align to size ,  n >= 5 (32-2GB),*/
void act_mpu_set(uint32_t chan, uint32_t mem_base, uint32_t size, uint32_t attr)
{
	uint32_t mpu_base, mpu_end, mpu_attr;
	uint32_t num_region ;

	num_region = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
	if(chan >= num_region){
		printk("mpu set over max region:%d >= %d\n", chan, num_region);
		return;
	}
	
	if(!(MPU->CTRL & MPU_CTRL_ENABLE_Msk)){
		// Memory Attribute Indirection Register 0
		MPU->MAIR0 = ((0x44 &  MPU_MAIR0_Attr0_Msk)   |  // Normal memory, Inner Non-cacheable, Outer Non-cacheable
					  (0x44 << MPU_MAIR0_Attr1_Pos)   |  // Normal memory, Inner Non-cacheable, Outer Non-cacheable
					  (0x44 << MPU_MAIR0_Attr2_Pos)   |  // Normal memory, Inner Non-cacheable, Outer Non-cacheable
					  (0x44 << MPU_MAIR0_Attr3_Pos));	 // Normal memory, Inner Non-cacheable, Outer Non-cacheable

		MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;
	}

	mpu_base = (mem_base+0x1f) & (~0x1f); // 32B align
	mpu_end =  ((mem_base+size) & (~0x1f))-1;

	if(attr == MPU_ATTR_NO){
		mpu_attr = P_RO_U_RO_Msk | INNER_SHAREABLE_Msk | NOT_EXEC; //no exe, ro
	}else  if(attr == MPU_ATTR_RW){
		mpu_attr = P_RW_U_RW_Msk | INNER_SHAREABLE_Msk ;  // exe, rw
	} else {
		mpu_attr = P_RO_U_RO_Msk | INNER_SHAREABLE_Msk ; // exe,  ro
	}	
	printk("mpu %d(%d): 0x%x-0x%x, set=0x%x-0x%x,attr=%s\n", chan , num_region, mpu_base, mpu_end,
			mem_base, mem_base+size, mpu_attr_str[attr]);

	MPU->RNR = chan;
    //  Base address
    MPU->RBAR = (mpu_base | mpu_attr);
	 //  Limit register
	 MPU->RLAR = ((mpu_end & MPU_RLAR_LIMIT_Msk) |  // Size
				  (MPU_MAIR_INDEX_NC << MPU_RLAR_AttrIndx_Pos)	|  // AttrIndex - 0
				  (0x1 << MPU_RLAR_EN_Pos));		   // Enable	- On


}

void act_mpu_unset(uint32_t chan)
{
	uint32_t num_region ;
	num_region = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
	if(chan >= num_region){
		printk("mpu unset over max region:%d >= %d\n", chan, num_region);
		return;
	}
	MPU->RNR = chan;
	MPU->RBAR = 0;
	MPU->RLAR = 0;
	printk("mpu %d unset\n", chan);
}

void arm_mpu_protect_init(void)
{
	/* protect rom section */
	act_mpu_set(MPU_CHAN_ROM, 0x0, 0x10000, MPU_ATTR_RO);
	/* protect flash cache all address section */
	act_mpu_set(MPU_CHAN_CODE, (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET), 0x1000000, MPU_ATTR_RO);
#if defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
	act_mpu_set(MPU_CHAN_RAMFUC, (uint32_t)&__ramfunc_start, (uint32_t)&__ramfunc_size, MPU_ATTR_RO);
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */

}

#define CPU_TRACE_RAM_ADDR 		0x31000000
#define CPU_TRACE_RAM_MAXLEN 	1024

static void trace_dump_str(unsigned int *buf, unsigned int len)
{
	int i;	
	for(i = 0; i < len/4; i++) {
		if((i&0x7)==0){
			printk("%08x:",CPU_TRACE_RAM_ADDR+i*4);
		}
		printk("%08x ", buf[i]);
		if( (i&0x7)==7 )
			printk("\n");
	}
	printk("\n");
}

void cpu_trace_enable(int enable)
{
	if(enable){
		sys_write32(0 , 0xe0043004);
		memset((void*)CPU_TRACE_RAM_ADDR, 0, CPU_TRACE_RAM_MAXLEN);
		sys_write32(0 , 0xe0043000);
		sys_write32((1<<31)|0xf , 0xe0043004);
	}else{
		sys_write32(0 , 0xe0043004);
	}
}


static int act_mpu_init(const struct device *arg)
{
	//unsigned int val = 0;
	ARG_UNUSED(arg);
	printk("arm mpu init\n");
	arm_mpu_protect_init();
	//soc_pstore_get(SOC_PSTORE_TAG_SYS_PANIC,&val);
	//if(val){
	printk("arm trace dump:\n");
	trace_dump_str((unsigned int *)CPU_TRACE_RAM_ADDR,  CPU_TRACE_RAM_MAXLEN);
	//}
	cpu_trace_enable(1);
	return 0;
}

SYS_INIT(act_mpu_init, APPLICATION, 20);


