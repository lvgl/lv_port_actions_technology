/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_RTT_H__
#define SHELL_RTT_H__

#include <shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_rtt_transport_api;

typedef void (* shell_rtt_callback_t)(const uint8_t *data, uint32_t len, void *user_data);

struct shell_rtt {
	shell_transport_handler_t handler;
	struct k_timer timer;
	void *context;
	shell_rtt_callback_t callback;
	void *user_data;
};

#define SHELL_RTT_DEFINE(_name)					\
	static struct shell_rtt _name##_shell_rtt;			\
	struct shell_transport _name = {				\
		.api = &shell_rtt_transport_api,			\
		.ctx = (struct shell_rtt *)&_name##_shell_rtt		\
	}

/**
 * @brief Function provides pointer to shell rtt backend instance.
 *
 * Function returns pointer to the shell rtt instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_rtt_get_ptr(void);

/**
 * @brief register output backend
 *
 * @param callback callback function for output backend
 * @param user_data user-defined data
 *
 * @retval 0 on success else negative code.
 */
int shell_rtt_register_output_callback(shell_rtt_callback_t callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_RTT_H__ */
