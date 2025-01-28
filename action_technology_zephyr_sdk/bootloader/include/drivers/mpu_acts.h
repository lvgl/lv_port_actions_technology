
/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief actions mpu interface
 */

#ifndef MPU_ACTS_H_
#define MPU_ACTS_H_

/**
 * @brief Declare a minimum MPU guard alignment and size
 *
 * This specifies the minimum MPU guard alignment/size for the MPU.  This
 * will be used to denote the guard section of the stack, if it exists.
 *
 * One key note is that this guard results in extra bytes being added to
 * the stack.  APIs which give the stack ptr and stack size will take this
 * guard size into account.
 *
 * Stack is allocated, but initial stack pointer is at the end
 * (highest address).  Stack grows down to the actual allocation
 * address (lowest address).  Stack guard, if present, will comprise
 * the lowest MPU_GUARD_ALIGN_AND_SIZE bytes of the stack.
 *
 * As the stack grows down, it will reach the end of the stack when it
 * encounters either the stack guard region, or the stack allocation
 * address.
 *
 * ----------------------- <---- Stack allocation address + stack size +
 * |                     |            MPU_GUARD_ALIGN_AND_SIZE
 * |  Some thread data   | <---- Defined when thread is created
 * |        ...          |
 * |---------------------| <---- Actual initial stack ptr
 * |  Initial Stack Ptr  |       aligned to STACK_ALIGN_SIZE
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |  Stack Ends         |
 * |---------------------- <---- Stack Buffer Ptr from API
 * |  MPU Guard,         |
 * |     if present      |
 * ----------------------- <---- Stack Allocation address
 *
 */
#if defined(CONFIG_MPU_STACK_GUARD)
#define MPU_GUARD_ALIGN_AND_SIZE	32
#else
#define MPU_GUARD_ALIGN_AND_SIZE	0
#endif


#define     MPU_DMA_WRITE       0x20
#define     MPU_CPU_READ        0x10
#define     MPU_CPU_WRITE       0x08
#define     MPU_DSP_P_READ      0x04
#define     MPU_DSP_D_READ     	0x02
#define     MPU_DSP_D_WRITE    	0x01


void mpu_set_address(uint32_t start, uint32_t end, uint32_t flag, uint32_t index);

void mpu_protect_enable(void);

void mpu_protect_disable(void);

void mpu_enable_region(unsigned int index);

void mpu_disable_region(unsigned int index);

int mpu_region_is_enable(unsigned int index);

#endif /* MPU_ACTS_H_ */
