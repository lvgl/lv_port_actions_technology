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
#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#endif

#define MPU_ATTR_NO  		0
#define MPU_ATTR_RW  		1
#define MPU_ATTR_RO  		2

static const char *mpu_attr_str[3] = { "NO", "RW", "RO"};


/*MPU set, mem_base= must align to size , size = 2^n, n >= 5 (32-2GB),*/
void act_mpu_set(uint32_t chan, uint32_t mem_base, uint32_t size, uint32_t attr)
{
	uint32_t i, mpu_base, mpu_size, mpu_attr;
	uint32_t num_region ;

	num_region = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
	if(chan >= num_region){
		printk("mpu set over max region:%d >= %d\n", chan, num_region);
		return;
	}
	if(!(MPU->CTRL & MPU_CTRL_ENABLE_Msk))
		MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;

	if(attr > MPU_ATTR_RO)
		attr = MPU_ATTR_RO;

	for(i = 31; i >= 0; i--){
		if(size & (1 << i))
			break;
	}
	if(i < 5){
		mpu_size = 0x20;
		i = 5;
	}else{
		mpu_size = size & (1 << i);
	}

	mpu_base = mem_base & (~(mpu_size-1));
	mpu_attr = (i-1) << MPU_RASR_SIZE_Pos;
	if(attr == MPU_ATTR_NO){
		mpu_attr |= MPU_RASR_XN_Msk| (0 << MPU_RASR_AP_Pos); //no exe, no access
	}else  if(attr == MPU_ATTR_RW){
		mpu_attr |= (3 << MPU_RASR_AP_Pos); // exe, rw
	} else {
		mpu_attr |= (7 << MPU_RASR_AP_Pos); // exe, ro
	}
	
	printk("mpu %d: 0x%x-0x%x, set=0x%x-0x%x,attr=%s\n", chan , mpu_base, mpu_base+mpu_size,
			mem_base, mem_base+size, mpu_attr_str[attr]);

	MPU->RNR = chan;
	MPU->RBAR = (mpu_base&MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | chan;
	MPU->RASR = mpu_attr | MPU_RASR_ENABLE_Msk;

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
	MPU->RBAR = chan;
	MPU->RASR = 0;
	printk("mpu %d unset\n", chan);
}


