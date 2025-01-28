/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for DSP drivers and applications
 */

#ifndef __DSP_H__
#define __DSP_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <zephyr/types.h>
#include <device.h>
#include <drivers/cfg_drv/dev_config.h>

#ifdef CONFIG_LOAD_IMAGE_FROM_FS
#include <fs/fs.h>
#endif /* LOAD_IMAGE_FROM_FS */

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************/
/* dsp image definition                                                           */
/**********************************************************************************/

/* image type, for multiple images support */
enum {
	DSP_IMAGE_MAIN = 0,	/* provide program entry point */
	DSP_IMAGE_SUB,

	DSP_NUM_IMAGE_TYPES,
};

#define DSP_IMAGE_NMLEN (12)
typedef int (*dsp_acts_irq_callback)(void *cb_data, u32_t cmd, void *param);

struct dsp_imageinfo {
	const char *name;
#ifdef CONFIG_LOAD_IMAGE_FROM_FS
	struct fs_file_t filp;
#else
	const void *ptr;
#endif
	size_t size;
	uint32_t entry_point;
};

/* boot arguments of dsp image */
struct dsp_bootargs {
	int32_t power_latency;  /* us */
	uint32_t command_buffer; /* address of command buffer */
	uint32_t debug_buf;
	uint32_t sub_entry_point;
};


/**********************************************************************************/
/* DSP/CPU communication mechanism (3 kinds)                                      */
/**********************************************************************************/
/**********************************************************************************/
/* 1. message bidirectional communication                                         */
/*    - Based on mailbox registers and interrupts                                 */
/**********************************************************************************/

/* message id */
enum dsp_msg_id {
	DSP_MSG_NULL = 0, /* empty message */

	/* from dsp */
	DSP_MSG_REQUEST_BOOTARGS,
	/* notify important state changing to cpu, parameter is enum dsp_task_state */
	DSP_MSG_STATE_CHANGED,
	DSP_MSG_PAGE_MISS,	/* parameter is epc */
    DSP_MSG_PAGE_FLUSH,  /* parameter is epc */

	/* to dsp */
	DSP_MSG_SUSPEND,	 	/* no parameter */
	DSP_MSG_RESUME,	 		/* no parameter */

	/* bidirection */
	DSP_MSG_KICK, /* parameter is enum dsp_msg_event */

	/* user-defined message id started from this */
	DSP_MSG_USER_DEFINED,
};


/* synchronize m4f and dsp clocks */
#define DSP_NEED_SYNC_CLOCK     0x434C4B /* 'CLK' */
#define DSP_SYNC_CLOCK_NULL     0
#define DSP_SYNC_CLOCK_START    1
#define DSP_SYNC_CLOCK_REPLY    2
#define DSP_SYNC_CLOCK_OFFSET   3


/* parameter of DSP_MSG_KICK */
enum dsp_msg_event {
	DSP_EVENT_NEW_CMD = 0,	/* new command in session command buffer */
	DSP_EVENT_NEW_DATA,		/* new data in session data buffer */
	DSP_EVENT_FENCE_SYNC,	/* sync signaled */

	DSP_EVENT_USER_DEFINED,
};

/* message handling result */
enum dsp_msg_result {
	DSP_NOACK = -2,		/* ack not received  */
	DSP_FAIL = -1,		/* ack received but failed to handled */
	DSP_NOTOUCH = 0,	/* not yet touched */
	DSP_INPROGRESS, 	/* not yet completed */
	DSP_DONE,			/* all requests done */
	DSP_REPLY,			/* all requests done, with reply present */
};

/* messsage struct */
struct dsp_message {
	/* message id */
	uint16_t id;
	/* message result */
	int16_t result;
	/* message owner */
	uint32_t owner;

	/* user-defined parameter */
	uint32_t param1;
	uint32_t param2;
};


/**********************************************************************************/
/* 2. command buffer unidirectional communication (bound to specific session)     */
/*    - Based on message mechanism and buffer pooling                             */
/**********************************************************************************/

/* command id */
enum dsp_cmd_id {
	/* generic commands */
	DSP_CMD_NULL = 0,	/* empty message, dsp will go idle */
	DSP_CMD_SET_SESSION = 1,	/* set session to run */

	DSP_CMD_CONSOLE_DSPLOAD = 0x10,	/* dsp load 	    : param1 1=start, 0=stop; param2 interval N ms */
	DSP_CMD_CONSOLE_MDW,			/* mem dump words   : param1 dsp address; param2 words count */
	DSP_CMD_CONSOLE_MDH,			/* mem dump halfs   : param1 dsp address; param2 halfs count */
	DSP_CMD_CONSOLE_MWW,			/* mem write 1 word : param1 dsp address; param2 word value */
	DSP_CMD_CONSOLE_MWH,			/* mem write 1 half : param1 dsp address; param2 half value */
	DSP_CMD_CONSOLE_RIN,			/* register in      : param1 dsp register address */
	DSP_CMD_CONSOLE_ROUT,			/* register out     : param1 dsp register address; param2 out value */
	DSP_CMD_CONSOLE_MAX,
};

/* parameters of DSP_CMD_SET_SESSION */
struct dsp_session_id_set {
	uint32_t id;		/* session type */
	uint32_t uuid;		/* session uuid */
	uint32_t func_allowed;	/* function allowed */
};

struct dsp_command_buffer {
	/* sequence of current command (counted from 1) */
	uint16_t cur_seq;
	/* sequence to be allocated */
	uint16_t alloc_seq;
	/* buffer object address (struct dsp_ringbuf) to store commands */
	uint32_t cpu_bufptr;
	uint32_t dsp_bufptr;
};

struct dsp_command {
	/* command id */
	uint16_t id;
	/* sequence in command buffer */
	uint16_t seq;
	/* semaphore handle (dsp will not touch) if synchronization needed */
	uint32_t sem;
	/* size of data in bytes */
	uint32_t size;
	uint32_t data[0];
};

/* total size should be multiple of 16-bit words */
static inline size_t sizeof_dsp_command(struct dsp_command *command)
{
	return sizeof(*command) + command->size;
}


/**********************************************************************************/
/* 3. directly shared bidirectional communication                                 */
/*    - Based on mailbox registers, for frequently requested information          */
/**********************************************************************************/

/* request type */
enum dsp_request_type {
	DSP_REQUEST_TASK_STATE,		/* task state */
	DSP_REQUEST_ERROR_CODE,		/* error code */
	DSP_REQUEST_SESSION_INFO,	/* session information */
	DSP_REQUEST_FUNCTION_INFO,	/* function information */
	DSP_REQUEST_USER_DEFINED,	/* user defined information */
};

/* task state */
enum dsp_task_state {
	DSP_TASK_PRESTART = -1, /* has not yet started */
	DSP_TASK_STARTED  = 0,  /* started, prepare running */
	DSP_TASK_PENDING,		/* waiting for new command or data */
	DSP_TASK_RUNNING,		/* running */
	DSP_TASK_SUSPENDED,		/* suspended */
	DSP_TASK_DEAD,			/* terminated */

	DSP_TASK_IDLE = DSP_TASK_PENDING,
	DSP_TASK_RESUMED = DSP_TASK_RUNNING,
};

/* error code */
enum dsp_error {
	DSP_NO_ERROR = 0,
	DSP_BAD_BOOTARGS,
	DSP_BAD_COMMAND,
	DSP_BAD_SESSION,
	DSP_BAD_FUNCTION,
	DSP_BAD_PARAMETER,
	DSP_ERR_USER_DEFINED,
};

/* DSP_REQUEST_SESSION_INFO */
struct dsp_request_session {
	uint32_t func_enabled;	/* enabled function */
	uint32_t func_counter;  /* function counter */
	void *info;				/* private information */
	uint32_t func_runnable;	/* runnable function */
};

/* DSP_REQUEST_FUNCTION_INFO */
struct dsp_request_function {
	unsigned int id;	/* function id */
	void *info;			/* function information address */
};


/**********************************************************************************/
/* dsp device driver api                                                          */
/**********************************************************************************/

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
 */

/**
 * @typedef dsp_api_poweron
 * @brief Callback API for power on
 */
typedef int (*dsp_api_poweron)(struct device *dev, void *cmdbuf);

/**
 * @typedef dsp_api_poweroff
 * @brief Callback API for power off
 */
typedef int (*dsp_api_poweroff)(struct device *dev);

/**
 * @typedef dsp_api_suspend
 * @brief Callback API for power suspend
 */
typedef int (*dsp_api_suspend)(struct device *dev);

/**
 * @typedef dsp_api_resume
 * @brief Callback API for power resume
 */
typedef int (*dsp_api_resume)(struct device *dev);

/**
 * @typedef dsp_api_kick
 * @brief Callback API for kicking dsp
 */
typedef int (*dsp_api_kick)(struct device *dev, uint32_t owner, uint32_t event, uint32_t params);

/**
 * @typedef dsp_api_request_image
 * @brief Callback API for loading image
 */
typedef int (*dsp_api_request_image)(struct device *dev, const struct dsp_imageinfo *image, int type);

/**
 * @typedef dsp_api_release_image
 * @brief Callback API for releasing image
 */
typedef int (*dsp_api_release_image)(struct device *dev, int type);

/**
 * @typedef dsp_api_release_image
 * @brief Callback API for releasing image
 */
typedef int (*dsp_api_request_mem)(struct device *dev, int type);

/**
 * @typedef dsp_api_release_image
 * @brief Callback API for releasing image
 */
typedef int (*dsp_api_release_mem)(struct device *dev, int type);

/**
 * @typedef dsp_message_handler
 * @brief Prototype definition for message handler
 */
typedef int (*dsp_message_handler)(struct dsp_message *msg);

/**
 * @typedef dsp_api_register_message_handler
 * @brief Callback API for registering message handler
 */
typedef int (*dsp_api_register_message_handler)(struct device *dev, dsp_message_handler handler);

/**
 * @typedef dsp_api_unregister_message_handler
 * @brief Callback API for unregistering message handler
 */
typedef int (*dsp_api_unregister_message_handler)(struct device *dev);

/**
 * @typedef dsp_api_send_message
 * @brief Callback API for releasing image
 */
typedef int (*dsp_api_send_message)(struct device *dev, struct dsp_message *msg);

/**
 * @typedef dsp_acts_request_userinfo
 * @brief Callback API for requesting user interested information
 */
typedef int (*dsp_acts_request_userinfo)(struct device *dev, int request, void *info);

struct dsp_driver_api {
	dsp_api_poweron poweron;
	dsp_api_poweroff poweroff;
	dsp_api_suspend suspend;
	dsp_api_resume resume;
	dsp_api_kick kick;
	dsp_api_register_message_handler register_message_handler;
	dsp_api_unregister_message_handler unregister_message_handler;
	dsp_api_request_image request_image;
	dsp_api_release_image release_image;
	dsp_api_request_mem request_mem;
	dsp_api_release_mem release_mem;
	dsp_api_send_message send_message;
	dsp_acts_request_userinfo request_userinfo;
};

/**
 * @endcond
 */

/**
 * @brief start the dsp device
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param cmdbuf  Address of session command buffer.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_poweron(struct device *dev, void *cmdbuf)
{
	const struct dsp_driver_api *api = dev->api;

	return api->poweron(dev, cmdbuf);
}

/**
 * @brief stop the dsp device
 *
 * @param dev     Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_poweroff(struct device *dev)
{
	const struct dsp_driver_api *api = dev->api;

	return api->poweroff(dev);
}

/**
 * @brief suspend the dsp device
 *
 * @param dev     Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_suspend(struct device *dev)
{
	const struct dsp_driver_api *api = dev->api;

	return api->suspend(dev);
}

/**
 * @brief resume the dsp device
 *
 * @param dev     Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_resume(struct device *dev)
{
	const struct dsp_driver_api *api = dev->api;

	return api->resume(dev);
}

/**
 * @brief resume the dsp device
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param owner   Owner who send the event.
 * @param event   Event id.
 * @param params  Event params.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_kick(struct device *dev, unsigned int owner, unsigned int event, unsigned int params)
{
	const struct dsp_driver_api *api = dev->api;

	return api->kick(dev, owner, event, params);
}

/**
 * @brief request the dsp image
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param image dsp image information
 * @param type dsp image filetype
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_request_image(struct device *dev, const struct dsp_imageinfo *image, int type)
{
    const struct dsp_driver_api *api = dev->api;

    return api->request_image(dev, image, type);
}

/**
 * @brief release the dsp image
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param type dsp image filetype
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_release_image(struct device *dev, int type)
{
    const struct dsp_driver_api *api = dev->api;

    return api->release_image(dev, type);
}

/**
 * @brief request the dsp mem
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param image dsp image information
 * @param type dsp image filetype
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_request_mem(struct device *dev,int type)
{
    const struct dsp_driver_api *api = dev->api;

    return api->request_mem(dev, type);
}

/**
 * @brief release the dsp image
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param type dsp image filetype
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_release_mem(struct device *dev, int type)
{
    const struct dsp_driver_api *api = dev->api;

    return api->release_mem(dev, type);
}


/**
 * @brief register dsp message handler
 *
 * @param dev           Pointer to the device structure for the driver instance.
 * @param handler  message handler function
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_register_message_handler(struct device *dev, dsp_message_handler handler)
{
    const struct dsp_driver_api *api = dev->api;

	return api->register_message_handler(dev, handler);
}

/**
 * @brief unregister dsp callback
 *
 * @param dev           Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_unregister_message_handler(struct device *dev)
{
    const struct dsp_driver_api *api = dev->api;

	return api->unregister_message_handler(dev);
}

/**
 * @brief send message to dsp
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param msg    message to send
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_send_message(struct device *dev, struct dsp_message *msg)
{
    const struct dsp_driver_api *api = dev->api;

	return api->send_message(dev, msg);
}

/**
 * @brief request user information writing by dsp
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param info    information to store
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dsp_request_userinfo(struct device *dev, int request, void *info)
{
	const struct dsp_driver_api *api = dev->api;

	return api->request_userinfo(dev, request, info);
}


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __DSP_H__ */
