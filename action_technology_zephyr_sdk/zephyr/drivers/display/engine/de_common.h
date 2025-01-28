/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_COMMON_H_
#define ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_COMMON_H_

#include <kernel.h>
#include <sys/slist.h>
#include <drivers/display/display_engine.h>
#include <linker/linker-defs.h>

enum de_command_id {
	/* overlay commands */
	DE_CMD_COMPOSE = 0,
	DE_CMD_COMPOSE_WB,    /* compose to local buffer */
	DE_CMD_FILL,
	DE_CMD_BLIT,
	DE_CMD_BLEND,
	DE_CMD_BLEND_FG,

	DE_CMD_SET_CLUT,

	/* rotate commands */
	DE_CMD_ROTATE_FILL,
	DE_CMD_ROTATE_RECT,
	DE_CMD_ROTATE_CIRCLE,
};

struct de_command_entry {
	uint8_t inst;
	uint8_t cmd;
	uint16_t seq; /* large enough ? */

#if defined(CONFIG_DISPLAY_ENGINE_LARK)
	uint32_t cfg[36];
#elif defined(CONFIG_DISPLAY_ENGINE_LEOPARD)
	uint32_t cfg[38];
#endif

	sys_snode_t node;
};

/**
 * @brief Allocate new instance id
 *
 * @param flags Instance flags
 *
 * @retval Instance id
 */
int de_alloc_instance(uint32_t flags);

/**
 * @brief Free new instance id
 *
 * @param inst Instance id
 * @retval N/A
 */
int de_free_instance(int inst);

/**
 * @brief Query instance flag
 *
 * @param inst Instance id
 * @param flag
 *
 * @retval query result
 */
bool de_instance_has_flag(int inst, uint32_t flag);

/**
 * @brief Alloc a command entry of specific instance
 *
 * @param inst Instance id
 * @retval Pointer to a command entry
 */
struct de_command_entry *de_instance_alloc_entry(int inst);

/**
 * @brief Free a command entry of specific instance
 *
 * @param entry Pointer to a command entry
 * @retval 0 on success else negative errno code.
 */
int de_instance_free_entry(struct de_command_entry *entry);

/**
 * @brief Poll for all commands of specific instance
 *
 * @param inst Instance id
 * @param timeout Time out in milliseconds
 * @retval 0 on success else negative errno code.
 */
int de_instance_poll(int inst, int timeout);

/**
 * @brief Regiter callback of specific instance
 *
 * @param inst Instance id
 * @param callback Callback function
 * @param User defined parameter of callback function
 * @retval 0 on success else negative errno code.
 */
int de_instance_register_callback(int inst, display_engine_instance_callback_t callback, void *user_data);

/**
 * @brief Notify command execution status of specific instance
 *
 * @param entry Pointer to a command entry
 * @param status Command execution status
 * @retval 0 on success else negative errno code.
 */
int de_instance_notify(struct de_command_entry *entry, int status);

/**
 * @brief Initialize the command pool
 *
 * @retval N/A.
 */
void de_command_pools_init(void);

#endif /* ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_COMMON_H_ */
