/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_DEVICE_H_
#define ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_DEVICE_H_

#include <kernel.h>
#include <drivers/display/display_engine.h>

struct de_data {
	int open_count;

	/* For display */
	uint32_t display_format; /* enum display_pixel_format */
	uint8_t display_bytes_per_pixel;
	uint8_t display_sync_en;
	uint8_t op_mode; /* enum display_engine_mode_type */
	display_engine_prepare_display_t prepare_fn;
	void *prepare_fn_arg;
	display_engine_start_display_t start_fn;
	void *start_fn_arg;

	int cmd_num;           /* number of commands not completed yet */
	int cmd_status;
	sys_snode_t *cmd_node; /* current command */
	sys_slist_t cmd_list;  /* normal priority command list */
#ifdef CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE
	sys_slist_t high_cmd_list; /* high priority command list */
#endif

	uint8_t preconfiged : 1; /* regs are pre-configured ? */
	uint8_t waiting : 1; /* waiting on wait_sem ? */
	struct k_sem wait_sem;
	struct k_mutex mutex;
	struct k_work_delayable timeout_work;
};

extern const struct display_engine_driver_api de_drv_api;
extern struct de_data de_drv_data;

#ifdef CONFIG_PM_DEVICE
int de_pm_control(const struct device *dev, enum pm_device_action action);
#endif

int de_init(const struct device *dev);

void de_isr(const void *arg);

void de_dump(void);

void nor_xip_set_locked(bool locked);

#endif /* ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_DEVICE_H_ */
