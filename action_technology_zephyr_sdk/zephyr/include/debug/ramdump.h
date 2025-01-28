/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file Ram dump interface
 *
 * NOTE: All Ram dump functions cannot be called in interrupt context.
 */

#ifndef _RAMDUMP__H_
#define _RAMDUMP__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <toolchain.h>
#include <arch/cpu.h>

#ifdef CONFIG_DEBUG_RAMDUMP

/**
 * @brief Save ramdump.
 *
 * This routine saves ramdump to flash.
 *
 * @param esf mcpu exception context
 * @param dump_bt btcpu exception flag
 *
 * @return true if success, otherwise return false
 */
extern int ramdump_save(const z_arch_esf_t *esf, int btcpu_info);

/**
 * @brief Dump ramdump.
 *
 * This routine dump ramdump region from flash.
 *
 * @param N/A
 *
 * @return true if success, otherwise return false
 */
extern int ramdump_dump(void);

/**
 * @brief calling traverse_cb to transfer ramdump.
 *
 * This routine transfer ramdump region from flash.
 *
 * @param N/A
 *
 * @return transfer length.
 */
extern int ramdump_transfer(int (*traverse_cb)(uint8_t *data, uint32_t max_len));

/**
 * @brief get flash offset of ramdump.
 *
 * This routine get ramdump offset from flash.
 *
 * @param N/A
 *
 * @return ramdump offset.
 */
extern uint32_t ramdump_get_offset(void);

/**
 * @brief get flash size of ramdump.
 *
 * This routine get ramdump size from flash.
 *
 * @param N/A
 *
 * @return ramdump size.
 */
extern uint32_t ramdump_get_size(void);

/**
 * @brief Reset ramdump.
 *
 * This routine resets ramdump to flash.
 *
 * @param N/A
 *
 * @return true if success, otherwise return false
 */
extern int ramdump_reset(void);

#else

#define ramdump_save(x,y)	do{}while(0)
#define ramdump_dump()	do{}while(0)

#endif

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEBUG_RAMDUMP */
