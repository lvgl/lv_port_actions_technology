/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <devicetree.h>
#include <string.h>
#include <mem_manager.h>
#include "dsp_inner.h"
#include <sdfs.h>
extern const struct dsp_imageinfo *dsp_create_image(const char *name);
extern void dsp_free_image(const struct dsp_imageinfo *image);

static struct dsp_session *global_session = NULL;
static unsigned int global_uuid = 0;

int dsp_session_get_state(struct dsp_session *session)
{
	int state = 0;
	dsp_request_userinfo(session->dev, DSP_REQUEST_TASK_STATE, &state);
	return state;
}

int dsp_session_get_error(struct dsp_session *session)
{
	int error = 0;
	dsp_request_userinfo(session->dev, DSP_REQUEST_ERROR_CODE, &error);
	return error;
}

uint32_t dsp_session_get_debug_info(struct dsp_session *session, uint32_t index)
{
	dsp_request_userinfo(session->dev, DSP_REQUEST_USER_DEFINED, &index);
	return index;
}

void dsp_session_dump(struct dsp_session *session)
{
	struct dsp_request_session request;

	if (!session) {
		/* implicitly point to global session */
		session = global_session;
		if (!session)
			return;
	}

	dsp_request_userinfo(session->dev, DSP_REQUEST_SESSION_INFO, &request);

	SYS_LOG_ERR("\n---------------------------dsp dump---------------------------\n");
	SYS_LOG_ERR("session (id=%u, uuid=%u):\n", session->id, session->uuid);
	SYS_LOG_ERR("\tstate=%d\n", dsp_session_get_state(session));
	SYS_LOG_ERR("\terror=0x%x\n", dsp_session_get_error(session));
	SYS_LOG_ERR("\tfunc_allowed=0x%x\n", session->func_allowed);
	SYS_LOG_ERR("\tfunc_enabled=0x%x\n", request.func_enabled);
	SYS_LOG_ERR("\tfunc_runnable=0x%x\n", request.func_runnable);
	SYS_LOG_ERR("\tfunc_counter=0x%x\n", request.func_counter);

	if (request.info)
		dsp_session_dump_info(session, request.info);
	if (request.func_enabled) {
		for (int i = 0; i < DSP_NUM_FUNCTIONS; i++) {
			if (request.func_enabled & DSP_FUNC_BIT(i))
				dsp_session_dump_function(session, i);
		}
	}
	SYS_LOG_ERR("--------------------------------------------------------------\n\n");
}

static int dsp_session_set_id(struct dsp_session *session)
{
	struct dsp_session_id_set set = {
		.id = session->id,
		.uuid = session->uuid,
		.func_allowed = session->func_allowed,
	};

	struct dsp_command *command =
		dsp_command_alloc(DSP_CMD_SET_SESSION, NULL, sizeof(set), &set);
	if (command == NULL)
		return -ENOMEM;

	dsp_session_submit_command(session, command);
	dsp_command_free(command);
	return 0;
}

static inline int dsp_session_get(struct dsp_session *session)
{
	return atomic_inc(&session->open_refcnt);
}

static inline int dsp_session_put(struct dsp_session *session)
{
	return atomic_dec(&session->open_refcnt);
}

static void dsp_session_destroy(struct dsp_session *session);

static struct acts_ringbuf cmd_buff  __in_section_unique(DSP_SHARE_RAM);
static u8_t dsp_cmd_buff[DSP_SESSION_COMMAND_BUFFER_SIZE] __in_section_unique(DSP_SHARE_RAM);
static struct dsp_session static_globle_session __in_section_unique(DSP_SHARE_RAM);
static struct dsp_session *dsp_session_create(struct dsp_session_info *info)
{
	struct dsp_session *session = NULL;

	if (info == NULL || info->main_dsp == NULL)
		return NULL;

	session = &static_globle_session;
	memset(session, 0, sizeof(struct dsp_session));
	if (session == NULL) {
		SYS_LOG_ERR("malloc failed %d", sizeof(*session));
		return NULL;
	}

	memset(session, 0, sizeof(*session));

	/* find dsp device */
	session->dev = (struct device *)device_get_binding(CONFIG_DSP_NAME);
	if (session->dev == NULL) {
		SYS_LOG_ERR("cannot find device \"%s\"", CONFIG_DSP_ACTS_DEV_NAME);
		goto fail_exit;
	}

	/* find images */
	session->images[DSP_IMAGE_MAIN] = dsp_create_image(info->main_dsp);
	if (!session->images[DSP_IMAGE_MAIN]) {
		goto fail_exit;
	}

	if (info->sub_dsp) {
		session->images[DSP_IMAGE_SUB] = dsp_create_image(info->sub_dsp);
		if (!session->images[DSP_IMAGE_SUB]) {
			//goto fail_exit;
		}
	}

	/* allocate command buffer */
	acts_ringbuf_init(&cmd_buff, dsp_cmd_buff, DSP_SESSION_COMMAND_BUFFER_SIZE);

	cmd_buff.dsp_ptr = mcu_to_dsp_address(cmd_buff.cpu_ptr, 0);

	session->cmdbuf.cur_seq = 0;
	session->cmdbuf.alloc_seq = 1;
	session->cmdbuf.cpu_bufptr = (uint32_t)&cmd_buff;
	session->cmdbuf.dsp_bufptr = mcu_to_dsp_address(session->cmdbuf.cpu_bufptr, 0);

	k_sem_init(&session->sem, 0, 1);
	k_mutex_init(&session->mutex);
	atomic_set(&session->open_refcnt, 0);
#ifdef ENABLE_SESSION_BEGIN_END_REFCNT
	atomic_set(&session->run_refcnt, 0);
#endif
	session->id = info->type;
	session->uuid = ++global_uuid;
	SYS_LOG_INF("session %u created (uuid=%u)", session->id, session->uuid);
	return session;
fail_exit:
	dsp_session_destroy(session);
	return NULL;
}

static void dsp_session_destroy(struct dsp_session *session)
{
	if (atomic_get(&session->open_refcnt) != 0)
		SYS_LOG_ERR("session not closed yet (ref=%d)", atomic_get(&session->open_refcnt));

	for (int i = 0; i < ARRAY_SIZE(session->images); i++) {
		if (session->images[i])
			dsp_free_image(session->images[i]);
	}

    //static malloc, should no be free
	//if (session->cmdbuf.cpu_bufptr)
	//	acts_ringbuf_free((struct acts_ringbuf *)(session->cmdbuf.cpu_bufptr));

	SYS_LOG_INF("session %u destroyed (uuid=%u)", session->id, session->uuid);
	//mem_free(session);
}

static int dsp_session_message_handler(struct dsp_message *message)
{
	if (!global_session || message->id != DSP_MSG_KICK)
		return -ENOTTY;

	switch (message->param1) {
	case DSP_EVENT_FENCE_SYNC:
		if (message->param2){
			if(k_sem_count_get((struct k_sem *)message->param2) > 1){
				SYS_LOG_ERR("invalid ksem %p", (struct k_sem *)message->param2);
			}else{
				k_sem_give((struct k_sem *)message->param2);
			}
		}
		break;
	case DSP_EVENT_NEW_CMD:
	case DSP_EVENT_NEW_DATA:
		k_sem_give(&global_session->sem);
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}
#ifdef CONFIG_DSP_LOAD_ROM

#define DSP_PROM_ADDR 0x30010000
#define DSP_SDKPROM_ADDR (0x30010000 + 0x12E00)
#define DSP_DROM_ADDR 0x30000000
#define DSP_PROM_NAME "DPROM.bin"
#define DSP_DROM_NAME "DDROM.bin"
#define DSP_SDKDROM_NAME "sdk_rom.bin"
static int rom_image_loaded = 0;

static int dsp_load_rom_image(const char *name, unsigned int addr)
{
    void *ptr;
    int size;

 	SYS_LOG_ERR("dsp image \"%s\"\n", name);

    if (sd_fmap(name, (void **)&ptr, (int *)&size)) {
        SYS_LOG_ERR("cannot find dsp image \"%s\"\n", name);
        return -1;
    }

    memcpy((void *)addr, ptr, size);

    return 0;
}
#endif

static int dsp_session_open(struct dsp_session *session, struct dsp_session_info *info)
{
	int res = 0;

	if (dsp_session_get(session) != 0) {
		//SYS_LOG_ERR("session is opened already!  uuid: 0x%x\n", session->uuid);
		return -EALREADY;
	}

	if (info == NULL) {
		res = -EINVAL;
		goto fail_out;
	}

    /* load dsp rom image */
#ifdef CONFIG_DSP_LOAD_ROM
    if( !rom_image_loaded ) {
        if(dsp_load_rom_image(DSP_PROM_NAME, DSP_PROM_ADDR) < 0) {
            res = -1;
            goto fail_out;
        }
        if(dsp_load_rom_image(DSP_DROM_NAME, DSP_DROM_ADDR) < 0) {
            res = -1;
            goto fail_out;
        }
#if 0
        if(dsp_load_rom_image(DSP_SDKDROM_NAME, DSP_SDKPROM_ADDR) < 0) {
            res = -1;
            goto fail_out;
        }
#endif
        rom_image_loaded = 1;
    }
#endif
	/* load main dsp image */
	res = dsp_request_image(session->dev, session->images[DSP_IMAGE_MAIN], DSP_IMAGE_MAIN);
	if (res) {
		SYS_LOG_ERR("failed to load main image \"%s\"", session->images[DSP_IMAGE_MAIN]->name);
		goto fail_out;
	}

	/* load sub dsp image */
	if (session->images[DSP_IMAGE_SUB]) {
		res = dsp_request_image(session->dev, session->images[DSP_IMAGE_SUB], DSP_IMAGE_SUB);
		if (res) {
			SYS_LOG_ERR("failed to load sub image \"%s\"", session->images[DSP_IMAGE_SUB]->name);
			goto fail_release_main;
		}
	}

	/* reset and register command buffer */
	session->cmdbuf.cur_seq = 0;
	session->cmdbuf.alloc_seq = 1;
	acts_ringbuf_reset((struct acts_ringbuf *)(session->cmdbuf.cpu_bufptr));

	/* power on */
	res = dsp_poweron(session->dev, &session->cmdbuf);
	if (res) {
		SYS_LOG_ERR("dsp_poweron failed");
		goto fail_release_sub;
	}

	/* configure session id to dsp */
	session->func_allowed = info->func_allowed;
	res = dsp_session_set_id(session);
	if (res) {
		SYS_LOG_ERR("dsp_session_set_id failed");
		dsp_poweroff(session->dev);
		goto fail_release_sub;
	}

	/* register message handler only after everything OK */
	k_sem_reset(&session->sem);
	dsp_register_message_handler(session->dev, dsp_session_message_handler);
	SYS_LOG_INF("session %u opened (uuid=%u, allowed=0x%x)",
			session->id, session->uuid, info->func_allowed);
	return 0;
fail_release_sub:
	if (session->images[DSP_IMAGE_SUB])
		dsp_release_image(session->dev, DSP_IMAGE_SUB);
fail_release_main:
	dsp_release_image(session->dev, DSP_IMAGE_MAIN);
fail_out:
	dsp_session_put(session);
	return res;
}

static int dsp_session_close(struct dsp_session *session)
{
	if (dsp_session_put(session) != 1)
		return -EBUSY;

	if (session->cmdbuf.cur_seq + 1 != session->cmdbuf.alloc_seq)
		SYS_LOG_WRN("session %u command not finished %d %d ", session->id,session->cmdbuf.cur_seq,session->cmdbuf.alloc_seq);

	dsp_unregister_message_handler(session->dev);
	dsp_poweroff(session->dev);

	for (int i = 0; i < ARRAY_SIZE(session->images); i++) {
		if (session->images[i])
			dsp_release_image(session->dev, i);
	}

	dsp_release_mem(session->dev, 0);
	SYS_LOG_INF("session %u closed (uuid=%u)", session->id, session->uuid);
	return 0;
}

struct dsp_session *dsp_open_global_session(struct dsp_session_info *info)
{
	int res;

    if (global_session == NULL) {
        if(info){
            global_session = dsp_session_create(info);
            if (global_session == NULL) {
                SYS_LOG_ERR("failed to create global session");
                return NULL;
            }
        }else{
            return NULL;
        }
    }

    if(info){
        res = dsp_session_open(global_session, info);
        if (res < 0 && res != -EALREADY) {
            SYS_LOG_ERR("failed to open global session");
            dsp_session_destroy(global_session);
            global_session = NULL;
        }
    }else{
        res = dsp_session_open(global_session, info);
        if (res < 0 && res != -EALREADY) {
            SYS_LOG_ERR("failed to open global session");
            return NULL;
        }
    }

	return global_session;
}

void dsp_close_global_session(struct dsp_session *session)
{
	//assert(session == global_session);

	if (global_session) {
		if (!dsp_session_close(global_session)) {
			dsp_session_destroy(global_session);
			global_session = NULL;
		}
	}
}

int dsp_session_wait(struct dsp_session *session, int timeout)
{
	return k_sem_take(&session->sem, SYS_TIMEOUT_MS(timeout));
}

int dsp_session_kick(struct dsp_session *session)
{
	return dsp_kick(session->dev, session->uuid, DSP_EVENT_NEW_DATA, 0);
}

int dsp_session_submit_command(struct dsp_session *session, struct dsp_command *command)
{
	struct acts_ringbuf *buf = (struct acts_ringbuf *)(session->cmdbuf.cpu_bufptr);
	unsigned int size = sizeof_dsp_command(command);
	unsigned int space;
	unsigned int wait_time = 500;
	int res = -ENOMEM;

	if (!global_session){
        return -ENODEV;
	}

	k_mutex_lock(&session->mutex, K_FOREVER);

#ifdef ENABLE_SESSION_BEGIN_END_REFCNT
	if ((command->id == DSP_CMD_SESSION_BEGIN && atomic_inc(&session->run_refcnt) != 0) ||
		(command->id == DSP_CMD_SESSION_END && atomic_dec(&session->run_refcnt) != 1)) {
		if (command->sem) {
			SYS_LOG_DBG("ignore command %d", command->id);
			k_sem_give((struct k_sem *)command->sem);
		}

		res = 0;
		goto out_unlock;
	}
#endif

    while(wait_time){
        space = acts_ringbuf_space(buf);
        if (space < ACTS_RINGBUF_NELEM(size)) {
            os_sleep(10);
            wait_time -= 10;
            SYS_LOG_WRN("wait cmd space %d %d %d", size, acts_ringbuf_head_offset(buf), acts_ringbuf_tail_offset(buf));
        }else{
            break;
        }
    }

    if (wait_time == 0){
        SYS_LOG_ERR("No enough space (%u) for command (id=0x%04x, size=%u)",
                space, command->id, size);

#ifdef ENABLE_SESSION_BEGIN_END_REFCNT
		switch (command->id) {
		case DSP_CMD_SESSION_BEGIN:
			atomic_dec(&session->run_refcnt);
			break;
		case DSP_CMD_SESSION_END:
			atomic_inc(&session->run_refcnt);
			break;
		default:
			break;
		}
#endif
        goto out_unlock;
    }

	/* alloc sequence number */
	command->seq = session->cmdbuf.alloc_seq++;
	/* insert command */
	res = acts_ringbuf_put(buf, command, ACTS_RINGBUF_NELEM(size));

	if(res == 0){
        SYS_LOG_ERR("send cmd failed\n");
	}
	SYS_LOG_INF("submit cmd %d %d %d %d seq %d totalsize %d buffer  %d \n", command->id, command->data[0], command->data[1],command->size,command->seq,ACTS_RINGBUF_NELEM(size),acts_ringbuf_length(buf));
	/* kick dsp to process command */
	res = dsp_kick(session->dev, session->uuid, DSP_EVENT_NEW_CMD, 0);
	res = 0;
out_unlock:
	k_mutex_unlock(&session->mutex);
	return res;
}

int dsp_console_set_cmd(uint16_t id, uint32_t param1, uint32_t param2)
{
	struct dsp_command *command;
	int ret = 0;

	command = mem_malloc(sizeof(*command) + 8);
	if (command) {
		command->id = id;
		command->sem = 0;
		command->size = 8;
		command->data[0] = param1;
		command->data[1] = param2;
	} else {
		return -ENOMEM;
	}

	if (!global_session) {
		printk("[WRN] dsp global session is null\n");
		ret = -ENODEV;
	} else {
		dsp_session_submit_command(global_session, command);
	}

	mem_free((void *)command);

	return ret;
}
