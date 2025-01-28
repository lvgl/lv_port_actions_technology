/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSP_INNER_H__
#define __DSP_INNER_H__

#include <zephyr.h>
#include <dsp_hal.h>
#include <soc.h>
#include <acts_ringbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DSP_SESSION_COMMAND_BUFFER_SIZE		(256)
#define ENABLE_SESSION_BEGIN_END_REFCNT

struct dsp_session_buf {
	struct acts_ringbuf buf;
#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	struct dsp_session *session;
#endif
};

struct dsp_session {
	unsigned int id;	/* session id (session type id) */
	unsigned int uuid;	/* session unique id */
	unsigned int func_allowed; /* session functions allowed mask */

	/* dsp device */
	struct device *dev;
	/* dsp command buffer */
	struct dsp_command_buffer cmdbuf;
	/* dsp images to run */
	const struct dsp_imageinfo *images[DSP_NUM_IMAGE_TYPES];

	/* semaphore to wait for dsp output */
	struct k_sem sem;
	/* mutex in case of multi-thread */
	struct k_mutex mutex;

	/* reference count */
	atomic_t open_refcnt;
#ifdef ENABLE_SESSION_BEGIN_END_REFCNT
	atomic_t run_refcnt;
#endif
};

void dsp_session_dump_info(struct dsp_session *session, void *info);
void dsp_session_dump_function(struct dsp_session *session, unsigned int func);

#ifdef __cplusplus
}
#endif

#endif /* __DSP_INNER_H__ */
