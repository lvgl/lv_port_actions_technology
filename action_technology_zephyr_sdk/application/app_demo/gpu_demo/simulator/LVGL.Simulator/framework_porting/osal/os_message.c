/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Author: wh<wanghui@actions-semi.com>
 *
 * Change log:
 *	2021/12/9: Created by wh.
 */

#include "os_common_api.h"
#include "stdbool.h"
#include "stdint.h"
#include "string.h"

#ifdef CONFIG_SIMULATOR
#include "sys/slist.h"
#include <Windows.h>
#endif

/**message function*/

K_MBOX_DEFINE(global_mailbox);
#ifdef CONFIG_SIMULATOR
//static sys_slist_t	global_msg_list;
//os_mutex global_msg_mutex;
#endif

/** message pool */
struct msg_info
{
#ifdef CONFIG_SIMULATOR
	sys_snode_t node;     /* used for app list*/
	bool is_busy;
#endif

	os_sem msg_sem;
	os_tid_t sender;
	os_tid_t receiver;
	char msg[MSG_MEM_SIZE];
};

#define MSG_INFO(_node) CONTAINER_OF(_node, struct msg_info, node)

struct msg_pool
{
	int pool_size;
	struct msg_info *pool;
};

static struct msg_info msg_pool_buff[CONFIG_NUM_MBOX_ASYNC_MSGS];

static struct msg_pool globle_msg_pool= {
	.pool_size = CONFIG_NUM_MBOX_ASYNC_MSGS,
	.pool = (struct msg_info *)&msg_pool_buff,
};

static struct msg_info *msg_pool_get_free_msg_info(void)
{
	struct msg_pool *pool = &globle_msg_pool;
	struct msg_info *result = NULL;

	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info * msg_content = &pool->pool[i];
#ifdef CONFIG_SIMULATOR
		if (!msg_content->is_busy) {
			memset(&msg_content->msg, 0, MSG_MEM_SIZE);
			result = msg_content;
		}
#else
		if (os_sem_take(&msg_content->msg_sem, SYS_TIMEOUT_MS(OS_NO_WAIT)) == 0) {
			memset(&msg_content->msg, 0, MSG_MEM_SIZE);
			result = msg_content;
			break;
		} else 	{
			//SYS_LOG_WRN("msg %d is busy\n", i);
		}
#endif
	}

	return result;
}

int msg_pool_get_free_msg_num(void)
{
	struct msg_pool *pool = &globle_msg_pool;
	int used_num = 0;
	unsigned int key = os_irq_lock();

	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info * msg_content = &pool->pool[i];
#ifdef CONFIG_SIMULATOR
		if (msg_content->is_busy) {
			used_num++;
		}
#else
		if (os_sem_take(&msg_content->msg_sem, OS_NO_WAIT) != 0) {
			used_num++;
		} else {
			os_sem_give(&msg_content->msg_sem);
		}
#endif
	}

	os_irq_unlock(key);

	return pool->pool_size - used_num;
}

void msg_pool_dump(void(*dump_fn)(os_tid_t sender, os_tid_t receiver,
		const char *content, int max_size))
{
	struct msg_pool *pool = &globle_msg_pool;

	os_printk("free msg cnt %d/%d\n", msg_pool_get_free_msg_num(), pool->pool_size);

	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info * msg_content = &pool->pool[i];

		if (os_sem_take(&msg_content->msg_sem, OS_NO_WAIT) != 0) {
			os_printk("busy msg %d:\n", i);
#ifdef CONFIG_MESSAGE_DEBUG
			dump_fn(msg_content->sender, msg_content->receiver, msg_content->msg, MSG_MEM_SIZE);
#else
			dump_fn(OS_ANY, OS_ANY, msg_content->msg, MSG_MEM_SIZE);
#endif
		} else {
			os_sem_give(&msg_content->msg_sem);
		}
	}
}
#ifdef CONFIG_SIMULATOR
#define UM_MSG_RECIEVE_LIST WM_USER+1
#endif

int os_send_async_msg(void *receiver, void *msg, int msg_size)
{
#ifdef CONFIG_SIMULATOR
	struct msg_info *msg_content;
	msg_content = msg_pool_get_free_msg_info();
	if (!msg_content) {
		SYS_LOG_ERR("msg_content is NULL ... ");
		return -ENOMEM;
	}
	msg_content->receiver = (os_tid_t)receiver;
	msg_content->sender = os_current_get();
	memcpy(&msg_content->msg, msg, msg_size);
	msg_content->is_busy = true;

    while (!PostThreadMessage((int)receiver, UM_MSG_RECIEVE_LIST, (WPARAM)msg_content, 0))
    {
        // Wait for receiver thread start.
        // Otherwise, the message will be missed.
        printf("os_send_async_msg: receiver thread not start, please wait...\n");
        Sleep(30);
    }


   // os_mutex_unlock(&global_msg_mutex);

#else
	os_mbox_msg send_msg;
	struct msg_info *msg_content;

	__ASSERT(!k_is_in_isr(),"send messag in isr");

	msg_content = msg_pool_get_free_msg_info();
	if (!msg_content) {
		SYS_LOG_ERR("msg_content is NULL ... ");
		return -ENOMEM;
	}

	memcpy(&msg_content->msg, msg, msg_size);
#ifdef CONFIG_MESSAGE_DEBUG
	msg_content->receiver = (os_tid_t)receiver;
	msg_content->sender = os_current_get();
#endif
	/* prepare to send message */
	send_msg.info = msg_size;
	send_msg.size = msg_size;
	send_msg.tx_data = &msg_content->msg;
	send_msg.tx_target_thread = (os_tid_t)receiver;

	/* send message containing most current data and loop around */
	os_mbox_async_put(&global_mailbox, &send_msg, &msg_content->msg_sem);
#endif
	return 0;
}

int os_send_async_msg_discardable(void *receiver, void *msg, int msg_size)
{
#ifdef CONFIG_SIMULATOR
	struct msg_info *msg_content;
	msg_content = msg_pool_get_free_msg_info();
	if (!msg_content) {
		//SYS_LOG_ERR("msg_content is NULL ... ");
		return -ENOMEM;
	}
	msg_content->receiver = (os_tid_t)receiver;
	msg_content->sender = os_current_get();
	memcpy(&msg_content->msg, msg, msg_size);
	msg_content->is_busy = true;

    while (!PostThreadMessage((int)receiver, UM_MSG_RECIEVE_LIST, (WPARAM)msg_content, 0))
    {
        // Wait for receiver thread start.
        // Otherwise, the message will be missed.
        printf("os_send_async_msg: receiver thread not start, please wait...\n");
        Sleep(30);
    }
#else
	os_mbox_msg send_msg;
	struct msg_info *msg_content;

	__ASSERT(!k_is_in_isr(),"send messag in isr");

	msg_content = msg_pool_get_free_msg_info();
	if (!msg_content) {
		SYS_LOG_ERR("msg_content is NULL ... ");
		return -ENOMEM;
	}

	memcpy(&msg_content->msg, msg, msg_size);
#ifdef CONFIG_MESSAGE_DEBUG
	msg_content->receiver = (os_tid_t)receiver;
	msg_content->sender = os_current_get();
#endif
	/* prepare to send message */
	send_msg.info = msg_size;
	send_msg.size = msg_size;
	send_msg.tx_data = &msg_content->msg;
	send_msg.tx_target_thread = (os_tid_t)receiver;

	/* send message containing most current data and loop around */
	os_mbox_async_put(&global_mailbox, &send_msg, &msg_content->msg_sem);
#endif
	return 0;
}

unsigned long count = 0;
int os_receive_msg(void *msg, int msg_size,int timeout)
{
#ifdef CONFIG_SIMULATOR
    MSG win_msg;
    PeekMessage(&win_msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
    while (GetMessage(&win_msg, NULL, 0, 0))
    {
        if (win_msg.message == UM_MSG_RECIEVE_LIST)
        {
            struct msg_info* rev_msg = (struct msg_info*)win_msg.wParam;
            if (!rev_msg)
            {
                Sleep(10);
                return -ETIMEDOUT;
            }
            memcpy(msg, rev_msg->msg, msg_size);
            rev_msg->is_busy = false;
            break;
        }
    }

#if 0
	os_mutex_lock(&global_msg_mutex,OS_FOREVER);
	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_msg_list, node, tmp) {
		struct msg_info *rev_msg = MSG_INFO(node);
		if (rev_msg->receiver == os_current_get())	{
			memcpy(msg, rev_msg->msg, msg_size);
			rev_msg->is_busy = false;
			sys_slist_find_and_remove(&global_msg_list, (sys_snode_t *)rev_msg);
			received_msg = true;
			break;
		}
	}
	os_mutex_unlock(&global_msg_mutex);
#endif


#else
	os_mbox_msg recv_msg;
	char buffer[MSG_MEM_SIZE];

	memset(buffer, 0, msg_size);

	/* prepare to receive message */
	recv_msg.info =  msg_size;
	recv_msg.size =  msg_size;
	recv_msg.rx_source_thread = OS_ANY;

    /* get a data item, waiting as long as needed */
	if (os_mbox_get(&global_mailbox, &recv_msg, buffer, SYS_TIMEOUT_MS(timeout))) {
		//SYS_LOG_INF("no message");
		return -ETIMEDOUT;
	}

	/* verify that message data was fully received */
	if (recv_msg.info != recv_msg.size) {
	    SYS_LOG_ERR("some message data dropped during transfer! \n ");
	    SYS_LOG_ERR("sender tried to send %d bytes"
					"only received %zu bytes receiver %p \n",
					recv_msg.info ,recv_msg.size,os_current_get());
	    return -EMSGSIZE;
	}

	/* copy msg from recvied buffer */
	memcpy(msg, buffer, msg_size);
#endif
	return 0;
}

void os_msg_clean(void)
{
#ifndef CONFIG_SIMULATOR
	os_mbox_clear_msg(&global_mailbox);
#endif
}

int os_get_pending_msg_cnt(void)
{
#ifndef CONFIG_SIMULATOR
	return k_mbox_get_pending_msg_cnt(&global_mailbox, os_current_get());
#endif
}

void os_msg_init(void)
{
	struct msg_pool *pool = &globle_msg_pool;

	//sys_slist_init(&global_msg_list);

    //global_msg_mutex = os_mutex_create();

	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info *msg_content = &pool->pool[i];
		os_sem_init(&msg_content->msg_sem, 1, 1);
		msg_content->is_busy = false;
	}
}
