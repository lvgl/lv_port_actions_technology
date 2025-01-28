/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSP_INNER_H__
#define __DSP_INNER_H__

#include <kernel.h>
#include <stdint.h>
#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	DSP_STATUS_POWEROFF,
	DSP_STATUS_POWERON,
	DSP_STATUS_SUSPENDED,
};

/* mailbox protocol status flags */
enum mailbox_status_bits {
	/* Flags set by the message sender: */
	/* indicate this is a new message, should clear by the receiver */
	MSG_FLAG_BUSY = BIT(0),
	/* indicate this is synchronized transaction. ignored so far */
	MSG_FLAG_SYNC = BIT(1),

	/* Flags set by the message receiver: */
	/* indicate message received, should clear by the sender */
	MSG_FLAG_ACK = BIT(8),
	/* indicate message handled completely */
	MSG_FLAG_DONE = BIT(9),
	/* indicate message handled completely, with reply present */
	MSG_FLAG_RPLY = BIT(10),
	/* fail to handle the message */
	MSG_FLAG_FAIL = BIT(11),
};

struct dsp_protocol_mailbox {
	/* message: low 16its is message id, high 16bits is message status */
	volatile uint32_t msg;
	/* message owner */
	volatile uint32_t owner;
	/* user-defined parameter list */
	volatile uint32_t param1;
	volatile uint32_t param2;
};

#define MSG_ID(msg) ((msg) & 0xffff)
#define MSG_STATUS(msg) ((msg) >> 16)
#define MAILBOX_MSG(id, status) (((uint32_t)(status) << 16) | ((id) & 0xffff))

struct dsp_protocol_userinfo {
	/* global state */
	volatile uint32_t task_state;
	volatile uint32_t error_code;
	/* session information */
	volatile uint32_t func_enabled;
	volatile uint32_t func_counter;
	volatile uint32_t ssn_info;
	/* function information table */
	volatile uint32_t func_table;
	volatile uint32_t func_size;
	/* function paused/resumed state */
	volatile uint32_t func_runnable;
};

struct dsp_acts_data {
	/* power status */
	int8_t pm_status;
	/* need sync clock flag */
	int8_t need_sync_clock;

	/* message semaphore */
	struct k_sem msg_sem;
	struct k_mutex msg_mutex;

	/* user-defined message handler */
	dsp_message_handler msg_handler;

	/* one is decoder, the other is digital audio effect */
	struct dsp_imageinfo images[DSP_NUM_IMAGE_TYPES];
	/* image bootargs */
	struct dsp_bootargs bootargs;
	dsp_acts_irq_callback cb;
	void *cb_data;
};

struct dsp_acts_config {
	/* message box (from cpu to dsp) through mailbox registers */
	struct dsp_protocol_mailbox *dsp_mailbox;
	/* message box (from dsp to cpu) through mailbox registers */
	struct dsp_protocol_mailbox *cpu_mailbox;
	/* dsp exported user information through mailbox registers */
	struct dsp_protocol_userinfo *dsp_userinfo;
};

uint32_t addr_cpu2dsp(uint32_t addr, bool is_code __unused);
uint32_t addr_dsp2cpu(uint32_t addr, bool is_code __unused);

int wait_hw_idle_timeout(int usec_to_wait);

int dsp_acts_request_image(struct device *dev, const struct dsp_imageinfo *image, int type);
int dsp_acts_release_image(struct device *dev, int type);
int dsp_acts_handle_image_pagemiss(struct device *dev, uint32_t epc);
int dsp_acts_handle_image_pageflush(struct device *dev, uint32_t epc);

int dsp_acts_register_message_handler(struct device *dev, dsp_message_handler handler);
int dsp_acts_unregister_message_handler(struct device *dev);

int dsp_acts_send_message(struct device *dev, struct dsp_message *msg);
int dsp_acts_recv_message(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* __DSP_INNER_H__ */
