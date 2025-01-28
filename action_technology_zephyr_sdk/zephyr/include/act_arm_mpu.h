/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ACT_ARM_MPU_H_
#define ZEPHYR_INCLUDE_ACT_ARM_MPU_H_

#include <kernel.h>
#include <kernel_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*mpu attr */
#define MPU_ATTR_NO  		0    /*read and no exe*/
#define MPU_ATTR_RW  		1    /*read/write and exe*/
#define MPU_ATTR_RO  		2   /*read and exe*/

/*mpu chan */
#define MPU_CHAN_ROM  			0  /*system use*/
#define MPU_CHAN_CODE  			1  /*system use*/
#define MPU_CHAN_RAMFUC  		2  /*system use*/
#define MPU_CHAN_SHELL			3  /*shell cmd use*/
#define MPU_CHAN_USER0			4  /*common use*/
#define MPU_CHAN_USER1			5  /*common use*/
#define MPU_CHAN_USER2			6  /*common use*/
#define MPU_CHAN_USER3			7  /*commonuse*/


/**
 *
 * @brief mpu set  mem_base= must align to size(32B) ,  n >= 5 (32-2GB)
 *
 * chan: MPU chan
 * mem_base: mpu base addr
 * size: mpu size 
 * attr:  *mpu attr
 * @return size of the d-cache line or 0 if the d-cache is not enabled.
 */
void act_mpu_set(uint32_t chan, uint32_t mem_base, uint32_t size, uint32_t attr);
void act_mpu_unset(uint32_t chan);
void arm_mpu_protect_init(void);

/*arm trace enable*/
void cpu_trace_enable(int enable);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ACT_ARM_MPU_H_ */
