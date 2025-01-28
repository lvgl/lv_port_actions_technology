/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSP_HAL_H__
#define __DSP_HAL_H__

#include <zephyr.h>
#include <mem_manager.h>
#include <string.h>
#include <errno.h>
#include <soc_dsp.h>
#include <dsp_hal_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*dsp_session_buf_read_fn)(void *, void *, unsigned int);
typedef int (*dsp_session_buf_write_fn)(void *, const void *, unsigned int);

/* session information to initialize a session */
struct dsp_session_info {
	/* session id */
	unsigned int type;
	/* allowed functions (bitfield) */
	unsigned int func_allowed;
	/* dsp image name */
	const char *main_dsp;
	const char *sub_dsp;
};

struct dsp_session;
struct dsp_session_buf;

/* dsp session ops */
/**
 * @brief open the global session
 *
 * session can be open multiple times, thus, the parameter "info" will be
 * ignored if this is not the first open.
 *
 * @param info session information
 *
 * @return Address of global session
 */
struct dsp_session *dsp_open_global_session(struct dsp_session_info *info);

/**
 * @brief close the global session
 *
 * @return N/A
 */
void dsp_close_global_session(struct dsp_session *session);

/**
 * @brief get session (task) state
 *
 * @param session Address of session
 *
 * @return the state (DSP_TASK_x) of session
 */
int dsp_session_get_state(struct dsp_session *session);

/**
 * @brief get session run error
 *
 * @param session Address of session
 *
 * @return the error code of session
 */
int dsp_session_get_error(struct dsp_session *session);

/**
 * @brief dump session information
 *
 * @param session Address of session
 *
 * @return N/A
 */
void dsp_session_dump(struct dsp_session *session);

/**
 * @brief submit session command
 *
 * @param session Address of session
 * @param command Command to submit
 *
 * @return 0 if succeed, the others if failed
 */
int dsp_session_submit_command(struct dsp_session *session, struct dsp_command *command);

/**
 * @brief submit session command without data
 *
 * @param session Address of session
 * @param command Command to submit
 *
 * @return 0 if succeed, the others if failed
 */
static inline int dsp_session_submit_simple_command(struct dsp_session *session, unsigned int id, struct k_sem *sem)
{
	struct dsp_command command = {
		.id = id,
		.sem = (uint32_t)sem,
		.size = 0,
	};

	return dsp_session_submit_command(session, &command);;
}

/**
 * @brief query session command finished
 *
 * @param session Address of session
 * @param command Command to query
 *
 * @return the query result
 */
bool dsp_session_command_finished(struct dsp_session *session, struct dsp_command *command);

/**
 * @brief kick dsp, since data ready
 *
 * @param session Address of session
 *
 * @return 0 if succeed, the others if failed
 */
int dsp_session_kick(struct dsp_session *session);

/**
 * @brief wait dsp for data output
 *
 * @param session Address of session
 * @param timout Waiting period to take the semaphore (in milliseconds),
 *                or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 Wait succeed.
 * @retval -ENOTSUP Waiting not supported, since wait_sem of sessoin_info
 *                  not configured when opening session.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
int dsp_session_wait(struct dsp_session *session, int timout);

/**
 * @brief Define and initialize a dsp command without data.
 *
 * The command can be accessed outside the module where it is defined using:
 *
 * @code extern struct dsp_command <name>; @endcode
 *
 * @param name Name of the command.
 * @param cmd_id Command id.
 * @param sem_obj Semaphore object for synchronization.
 */
#define DSP_COMMAND_DEFINE(name, cmd_id, sem_obj) \
	struct dsp_command name = {                   \
		.id = cmd_id,                             \
		.sem = (uint32_t)sem_obj,                 \
		.size = 0,                                \
	}

/**
 * @brief Initialize allocate and initialize a dsp command.
 *
 * This routine allocate and initializes a dsp command.
 *
 * @param id Command id.
 * @param sem Address of semaphore object for synchronization.
 * @param size Size of command data.
 * @param data Address of command data.
 *
 * @return Address of command.
 */
static inline struct dsp_command *dsp_command_alloc(unsigned int id,
				struct k_sem *sem, size_t size, const void *data)
{
	struct dsp_command *command = mem_malloc(sizeof(*command) + size);
	if (command) {
		command->id = (uint16_t)id;
		command->sem = (uint32_t)sem;
		command->size = size;
		if (data)
			memcpy(command->data, data, size);
	}

	return command;
}

/**
 * @brief Free a dsp command.
 *
 * This routine free a dsp command.
 *
 * @param command Address of command.
 *
 * @return N/A
 */
static inline void dsp_command_free(struct dsp_command *command)
{
	mem_free((void *)command);
}

/**
 * @brief submit command DSP_CMD_NULL.
 *
 * This routine submit command DSP_CMD_NULL.
 *
 * @param session Address of the session.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_submit_null(struct dsp_session *session, struct k_sem *sem)
{
	DSP_COMMAND_DEFINE(command, DSP_CMD_NULL, sem);
	return dsp_session_submit_command(session, &command);
}

/**
 * @brief Initialize a session with command DSP_CMD_SESSION_INIT
 *
 * This routine initialize a session using command DSP_CMD_SESSION_INIT.
 *
 * @param session Address of the session.
 * @param size Size of prarameters.
 * @param params Address of prarameters.
 *
 * @return Command result.
 */
static inline int dsp_session_init(struct dsp_session *session, size_t size, const void *params)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_SESSION_INIT, NULL, size, params);
	int res = -ENOMEM;
	if (command) {
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Begin a session with command DSP_CMD_SESSION_BEGIN
 *
 * This routine begin a session using command DSP_CMD_SESSION_BEGIN without params.
 *
 * @param session Address of the session.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_begin(struct dsp_session *session, struct k_sem *sem)
{
	DSP_COMMAND_DEFINE(command, DSP_CMD_SESSION_BEGIN, sem);
	return dsp_session_submit_command(session, &command);
}

/**
 * @brief End a session with command DSP_CMD_SESSION_END.
 *
 * This routine end a session using command DSP_CMD_SESSION_END without params.
 *
 * @param session Address of the session.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_end(struct dsp_session *session, struct k_sem *sem)
{
	DSP_COMMAND_DEFINE(command, DSP_CMD_SESSION_END, sem);
	return dsp_session_submit_command(session, &command);
}

/**
 * @brief Configure a session function with command DSP_CMD_FUNCTION_CONFIG.
 *
 * This routine configure a function using command DSP_CMD_FUNCTION_CONFIG.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 * @param conf Function specific configure ID.
 * @param size Size of prarameters.
 * @param params Address of prarameters.
 *
 * @return Command result.
 */
static inline int dsp_session_config_func(struct dsp_session *session,
		unsigned int func, unsigned int conf, size_t size, const void *params)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_CONFIG, NULL, size + 8, NULL);
	int res = -ENOMEM;
	if (command) {
		command->data[0] = func;
		command->data[1] = conf;
		memcpy(&command->data[2], params, size);
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Enable a session function with command DSP_CMD_FUNCTION_ENABLE.
 *
 * This routine enable a function using command DSP_CMD_FUNCTION_ENABLE without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_enable_func_sync(struct dsp_session *session, unsigned int func, struct k_sem *sem)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_ENABLE, sem, 4, &func);
	int res = -ENOMEM;
	if (command) {
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Enable a session function with command DSP_CMD_FUNCTION_ENABLE.
 *
 * This routine enable a function using command DSP_CMD_FUNCTION_ENABLE without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 *
 * @return Command result.
 */
static inline int dsp_session_enable_func(struct dsp_session *session, unsigned int func)
{
	return dsp_session_enable_func_sync(session, func, NULL);
}

/**
 * @brief Disable a session function with command DSP_CMD_FUNCTION_DISABLE.
 *
 * This routine disable a function using command DSP_CMD_FUNCTION_DISABLE without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_disable_func_sync(struct dsp_session *session, unsigned int func, struct k_sem *sem)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_DISABLE, sem, 4, &func);
	int res = -ENOMEM;
	if (command) {
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Disable a session function with command DSP_CMD_FUNCTION_DISABLE.
 *
 * This routine disable a function using command DSP_CMD_FUNCTION_DISABLE without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 *
 * @return Command result.
 */
static inline int dsp_session_disable_func(struct dsp_session *session, unsigned int func)
{
	return dsp_session_disable_func_sync(session, func, NULL);
}

/**
 * @brief Pause a session function with command DSP_CMD_FUNCTION_PAUSE.
 *
 * This routine pause a function using command DSP_CMD_FUNCTION_PAUSE without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_pause_func_sync(struct dsp_session *session, unsigned int func, struct k_sem *sem)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_PAUSE, sem, 4, &func);
	int res = -ENOMEM;
	if (command) {
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Pause a session function with command DSP_CMD_FUNCTION_PAUSE.
 *
 * This routine pause a function using command DSP_CMD_FUNCTION_PAUSE without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 *
 * @return Command result.
 */
static inline int dsp_session_pause_func(struct dsp_session *session, unsigned int func)
{
	return dsp_session_pause_func_sync(session, func, NULL);
}

/**
 * @brief Resume a session function with command DSP_CMD_FUNCTION_RESUME.
 *
 * This routine resume a function using command DSP_CMD_FUNCTION_RESUME without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 * @param sem Address of semaphore object for synchronization.
 *
 * @return Command result.
 */
static inline int dsp_session_resume_func_sync(struct dsp_session *session, unsigned int func, struct k_sem *sem)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_RESUME, sem, 4, &func);
	int res = -ENOMEM;
	if (command) {
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief Resume a session function with command DSP_CMD_FUNCTION_RESUME.
 *
 * This routine resume a function using command DSP_CMD_FUNCTION_RESUME without params.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 *
 * @return Command result.
 */
static inline int dsp_session_resume_func(struct dsp_session *session, unsigned int func)
{
	return dsp_session_resume_func_sync(session, func, NULL);
}

/**
 * @brief Debug a session function with command DSP_CMD_FUNCTION_DEBUG.
 *
 * This routine debug a function using command DSP_CMD_FUNCTION_DEBUG.
 *
 * @param session Address of the session.
 * @param func Session function ID.
 * @param size Size of debug options.
 * @param options Address of debug options.
 *
 * @return Command result.
 */
static inline int dsp_session_debug_func(struct dsp_session *session,
		unsigned int func, size_t size, const void *options)
{
	struct dsp_command *command = dsp_command_alloc(DSP_CMD_FUNCTION_DEBUG, NULL, size + 4, NULL);
	int res = -ENOMEM;
	if (command) {
		command->data[0] = func;
		memcpy(&command->data[1], options, size);
		res = dsp_session_submit_command(session, command);
		dsp_command_free(command);
	}

	return res;
}

/**
 * @brief get samples count of specific function
 *
 * This function must export samples_count information to cpu side.
 *
 * @param session Address of session
 * @param func Session function ID
 *
 * @return samples count
 */
unsigned int dsp_session_get_samples_count(struct dsp_session *session, unsigned int func);

/**
 * @brief get lost count of specific function
 *
 * This function must export datalost_count information to cpu side.
 *
 * @param session Address of session
 * @param func Session function ID
 *
 * @return lost count
 */
unsigned int dsp_session_get_datalost_count(struct dsp_session *session, unsigned int func);

/**
 * @brief get raw stream count of specific function
 *
 * This function must export raw_count information to cpu side.
 * For encoder, this is the encoded stream amount.
 * For decoder, this is the decoded stream amount.
 *
 * @param session Address of session
 * @param func Session function ID
 *
 * @return lost count
 */
unsigned int dsp_session_get_raw_count(struct dsp_session *session, unsigned int func);

/**
 * @brief get dsp session recoder param
 *
 * This function must export recoder param to cpu side.
 *
 * @param session Address of session
 * @param param_type encoder sub module type
 * @param param      param address
 *
 * @return lost count
 */
int dsp_session_get_recoder_param(struct dsp_session *session, int param_type, void *param);

/**
 * @brief get dsp session recoder param
 *
 * This function must export recoder param to cpu side.
 *
 * @param session Address of session
 * @param function_type function type
 *
 * @return function is runable or not
 */
int dsp_session_get_function_runable(struct dsp_session *session, int function_type);

/**
 * @brief get dsp session recoder param
 *
 * This function must export recoder param to cpu side.
 *
 * @param session Address of session
 * @param function_type function type
 *
 * @return function is enabled or not
 */
int dsp_session_get_function_enable(struct dsp_session *session, int function_type);

/* dsp session buffer ops */
/**
 * @brief Initialiize a dsp session buffer.
 *
 * This routine initialize a dsp session buffer with external buffer backstore.
 *
 * @param session Address of session.
 * @param data Address of external buffer backstore.
 * @param size Size of session buffer, must be mutiple of 2.
 *
 * @return Address of session buffer
 */
struct dsp_session_buf *dsp_session_buf_init(struct dsp_session *session, void *data, unsigned int size);

/* dsp session buffer ops */
/**
 * @brief clone a dsp session buffer.
 *
 * This routine clone a dsp session buffer from external buffer backstore.
 *
 * @param session Address of session.
 * @param buf external dsp session buf
 *
 * @return Address of new session buffer
 */
struct dsp_session_buf *dsp_session_buf_clone(struct dsp_session *session, struct dsp_session_buf *buf);

/**
 * @brief Destroy a dsp session buffer
 *
 * This routine dstroy a dsp session buffer, which has external buffer backstore.
 *
 * @param session Address of session.
 *
 * @return Address of session buffer
 */
void dsp_session_buf_destroy(struct dsp_session_buf *buf);

/* dsp session buffer ops */
/**
 * @brief Allocate a dsp session buffer.
 *
 * This routine allocate a dsp session buffer.
 *
 * @param session Address of session.
 * @param size Size of session buffer, must be mutiple of 2.
 *
 * @return Address of session buffer
 */
struct dsp_session_buf *dsp_session_buf_alloc(struct dsp_session *session, unsigned int size);

/**
 * @brief Free a dsp session buffer.
 *
 * This routine free a dsp session buffer.
 *
 * @param buf Address of session buffer.
 *
 * @return N/A
 */
void dsp_session_buf_free(struct dsp_session_buf *buf);

/**
 * @brief Reset a dsp session buffer.
 *
 * This routine reset a dsp session buffer.
 *
 * @param buf Address of session buffer.
 *
 * @return N/A.
 */
void dsp_session_buf_reset(struct dsp_session_buf *buf);

/**
 * @brief Read a dsp session buffer.
 *
 * This routine read a dsp session buffer.
 *
 * @param buf Address of session buffer.
 * @param data Address of data buffer.
 * @param size Size of data.
 *
 * @return number of bytes successfully read.
 */
int dsp_session_buf_read(struct dsp_session_buf *buf, void *data, unsigned int size);

/**
 * @brief Write a dsp session buffer.
 *
 * This routine write a dsp session buffer.
 *
 * @param buf Address of session buffer.
 * @param data Address of data buffer.
 * @param size Size of data..
 *
 * @return number of bytes successfully written.
 */
int dsp_session_buf_write(struct dsp_session_buf *buf, const void *data, unsigned int size);

/**
 * @brief Read a dsp session buffer to stream.
 *
 * This routine read a dsp session buffer.
 *
 * @param buf Address of session buffer.
 * @param stream Handle of stream.
 * @param size Size of data.
 * @param stream_write Stream write ops.
 *
 * @return number of bytes successfully read.
 */
int dsp_session_buf_read_to_stream(struct dsp_session_buf *buf,
		void *stream, unsigned int size,
		dsp_session_buf_write_fn stream_write);

/**
 * @brief Write a dsp session buffer from stream.
 *
 * This routine read a dsp session buffer.
 *
 * @param buf Address of session buffer.
 * @param stream Handle of stream.
 * @param size Size of data.
 * @param stream_write Stream read ops.
 *
 * @return number of bytes successfully written.
 */
int dsp_session_buf_write_from_stream(struct dsp_session_buf *buf,
		void *stream, unsigned int size,
		dsp_session_buf_read_fn stream_read);

/**
 * @brief Determine size of a dsp session buffer.
 *
 * @param buf Address of session buffer.
 *
 * @return Session buffer size.
 */
unsigned int dsp_session_buf_size(struct dsp_session_buf *buf);

/**
 * @brief Determine free space a dsp session buffer.
 *
 * @param buf Address of session buffer.
 *
 * @return Session buffer free space.
 */
unsigned int dsp_session_buf_space(struct dsp_session_buf *buf);

/**
 * @brief Determine length of a dsp session buffer.
 *
 * @param buf Address of session buffer.
 *
 * @return Session buffer data length.
 */
unsigned int dsp_session_buf_length(struct dsp_session_buf *buf);

/**
 * @brief Drop all data of a dsp session buffer
 *
 * @param buf Address of ring buffer.
 *
 * @return number of elements dropped in elements.
 */
size_t dsp_session_buf_drop_all(struct dsp_session_buf *buf);


int dsp_session_buf_read_to_buffer(struct dsp_session_buf *buf, void *buffer, unsigned int size);

int dsp_session_buf_write_from_buffer(struct dsp_session_buf *buf, void *buffer, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif /* __DSP_HAL_H__ */
