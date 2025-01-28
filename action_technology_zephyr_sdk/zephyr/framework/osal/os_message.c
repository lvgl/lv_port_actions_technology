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
 *	2023/5/11: Created by wh.
 */

#include "os_common_api.h"
#include "string.h"
#include <zephyr.h>
#include <sys/atomic.h>
#include <assert.h>
/****************************************************************************
 * Private micro Prototypes
 ****************************************************************************/
#define HIGH_PRIORITY_MSG 1
#define NORMAL_PRIORITY_MSG 0

#define HIGH_PRIORITY_MSG_NUM 5
/****************************************************************************
 * Private Types
 ****************************************************************************/

/** message pool */
struct msg_info
{
	os_sem msg_sem;
#ifdef CONFIG_MESSAGE_DEBUG
	os_tid_t sender;
	os_tid_t receiver;
#endif
	char msg[MSG_MEM_SIZE];
};

struct msg_cache_item {
	sys_snode_t node;     /* used for delay_msg list*/
	uint8_t busy_flag;
	uint8_t high_priority_flag;
	os_tid_t sender;
	os_tid_t receiver;
	char msg[MSG_MEM_SIZE];
};

struct msg_pool
{
	atomic_t free_size;
	uint8_t pool_size;
	uint8_t free_high_msg_cache_num;
	sys_slist_t	high_priority_msg_list;
	struct msg_info pool[CONFIG_NUM_MBOX_ASYNC_MSGS];
	struct msg_cache_item high_msg_cache[HIGH_PRIORITY_MSG_NUM];
};

#define MSG_INFO(_node) CONTAINER_OF(_node, struct msg_cache_item, node)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/** message function*/
K_MBOX_DEFINE(global_mailbox);

/** mutex for msg pool */
OS_MUTEX_DEFINE(msg_pool_mutex);

#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(osal.noinit.msg_pool)
#endif
static struct msg_pool globle_msg_pool;

static int high_priority_msg_cnt = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static struct msg_cache_item *_msg_pool_get_free_msg_cache(void)
{
	struct msg_pool *pool = &globle_msg_pool;
	for (uint8_t i = 0 ; i < HIGH_PRIORITY_MSG_NUM; i++) {
		struct msg_cache_item *msg_cache = &pool->high_msg_cache[i];
		if (!msg_cache->busy_flag) {
			msg_cache->busy_flag = 1;
			msg_cache->high_priority_flag = 0;
			pool->free_high_msg_cache_num--;
			//SYS_LOG_INF("get free_high_msg_cache_num %d \n", pool->free_high_msg_cache_num);
			return msg_cache;
		}
	}
	//SYS_LOG_INF("get free_high_msg_cache_num %d \n", pool->free_high_msg_cache_num);
	return NULL;
}

static void _msg_pool_release_msg_cache(struct msg_cache_item *msg_cache)
{
	struct msg_pool *pool = &globle_msg_pool;
	if (msg_cache) {
		msg_cache->busy_flag = 0;
		pool->free_high_msg_cache_num++;
		//SYS_LOG_INF("release free_high_msg_cache_num %d \n", pool->free_high_msg_cache_num);	
	}
}

static struct msg_info *_msg_pool_get_free_msg_info(void)
{
	struct msg_pool *pool = &globle_msg_pool;
	struct msg_info *result = NULL;

	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info * msg_content = &pool->pool[i];
		if (k_sem_take(&msg_content->msg_sem, SYS_TIMEOUT_MS(OS_NO_WAIT)) == 0) {
			memset(&msg_content->msg, 0, MSG_MEM_SIZE);
			result = msg_content;
			break;
		} else 	{
			//SYS_LOG_WRN("msg %d is busy\n", i);
		}
	}

	return result;
}

static int _msg_pool_send_async_msg_inner(void *receiver, void *msg, int msg_size, int high_priority)
{
	os_mbox_msg send_msg;
	struct msg_info *msg_content;

	__ASSERT(!k_is_in_isr(),"send messag in isr");

	msg_content = _msg_pool_get_free_msg_info();
	if (!msg_content) {
		SYS_LOG_ERR("msg_content is NULL ... ");
		return -ENOMEM;
	}

	atomic_dec(&globle_msg_pool.free_size);

	memcpy(&msg_content->msg, msg, msg_size);

#ifdef CONFIG_MESSAGE_DEBUG
	msg_content->receiver = (os_tid_t)receiver;
	msg_content->sender = os_current_get();
#endif

	/* prepare to send message */
	send_msg.info = high_priority;
	send_msg.size = msg_size;
	send_msg.tx_data = &msg_content->msg;
	send_msg.tx_target_thread = (os_tid_t)receiver;

	/* send message containing most current data and loop around */
	os_mbox_async_put(&global_mailbox, &send_msg, &msg_content->msg_sem);

	return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int msg_pool_get_free_msg_num(void)
{
	return atomic_get(&globle_msg_pool.free_size);
}

void msg_pool_dump(void(*dump_fn)(os_tid_t sender, os_tid_t receiver,
		const char *content, int max_size))
{
	struct msg_pool *pool = &globle_msg_pool;

	printk("mbox free msg cnt %d/%d\n", msg_pool_get_free_msg_num(), pool->pool_size);
	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info * msg_content = &pool->pool[i];

		if (os_sem_take(&msg_content->msg_sem, OS_NO_WAIT) != 0) {
			printk("busy msg %d:\n", i);
#ifdef CONFIG_MESSAGE_DEBUG
			dump_fn(msg_content->sender, msg_content->receiver, msg_content->msg, MSG_MEM_SIZE);
#else
			dump_fn(OS_ANY, OS_ANY, msg_content->msg, MSG_MEM_SIZE);
#endif
		} else {
			os_sem_give(&msg_content->msg_sem);
		}
	}

	printk("high priority msg free %d used %d total %d \n",
		pool->free_high_msg_cache_num,
		high_priority_msg_cnt,
		HIGH_PRIORITY_MSG_NUM);

	for (uint8_t i = 0 ; i < HIGH_PRIORITY_MSG_NUM; i++) {
		struct msg_cache_item *msg_content = &pool->high_msg_cache[i];
		if (msg_content->busy_flag) {
#ifdef CONFIG_MESSAGE_DEBUG
			dump_fn(msg_content->sender, msg_content->receiver, msg_content->msg, MSG_MEM_SIZE);
#else
			dump_fn(OS_ANY, OS_ANY, msg_content->msg, MSG_MEM_SIZE);
#endif
		}
	}
}

int os_send_async_msg_discardable(void *receiver, void *msg, int msg_size)
{
	sys_snode_t *node, *tmp;
	struct msg_cache_item *msg_info;
	int ret;

	os_mutex_lock(&msg_pool_mutex, OS_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&globle_msg_pool.high_priority_msg_list, node, tmp) {
		msg_info = MSG_INFO(node);
		if (!memcmp(msg_info->msg, msg, msg_size))	{
			//SYS_LOG_INF("drop high priority msg \n");
			os_mutex_unlock(&msg_pool_mutex);
			return 0;
		}
	}

	ret = _msg_pool_send_async_msg_inner(receiver, msg, msg_size, HIGH_PRIORITY_MSG);
	if (!ret) {
		msg_info = _msg_pool_get_free_msg_cache();
		if (msg_info){
			memcpy(&msg_info->msg, msg, msg_size);
			msg_info->receiver = (os_tid_t)receiver;
			msg_info->sender = os_current_get();
			msg_info->high_priority_flag = 1;
			sys_slist_append(&globle_msg_pool.high_priority_msg_list, (sys_snode_t *)msg_info);
			//SYS_LOG_INF("add high_priority_msg_cnt %d \n",++high_priority_msg_cnt);
		} else {
			SYS_LOG_WRN("no memory for immediately msg record \n");
		}
	}
	os_mutex_unlock(&msg_pool_mutex);

	return ret;
}

int os_send_async_msg(void *receiver, void *msg, int msg_size)
{
	int ret = 0;
	int try_cnt = 500;

try_again:
	os_mutex_lock(&msg_pool_mutex, OS_FOREVER);
	ret = _msg_pool_send_async_msg_inner(receiver, msg, msg_size, NORMAL_PRIORITY_MSG);
	os_mutex_unlock(&msg_pool_mutex);
	if (ret) {
		SYS_LOG_INF("wait msg_content again %d",try_cnt);
		os_sleep(2);
		if(try_cnt-- <= 0 || receiver == os_current_get()) {
			SYS_LOG_ERR("msg_content is NULL try_cnt %d  receiver %p current %p\n",try_cnt,receiver,os_current_get());
			return ret;
		} else {
			goto try_again;
		}
	}

	return ret;
}

int os_receive_msg(void *msg, int msg_size,int timeout)
{
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

	atomic_inc(&globle_msg_pool.free_size);

	/* copy msg from recvied buffer */
	memcpy(msg, buffer, msg_size);

	os_mutex_lock(&msg_pool_mutex, OS_FOREVER);

	/* remove high priority msg */
	if (recv_msg.info == HIGH_PRIORITY_MSG) {
		sys_snode_t *node, *tmp;
		int cnt = 0;
		SYS_SLIST_FOR_EACH_NODE_SAFE(&globle_msg_pool.high_priority_msg_list, node, tmp) {
			struct msg_cache_item *msg_info = MSG_INFO(node);
			if (!memcmp(msg_info->msg, msg, msg_size))	{
				sys_slist_find_and_remove(&globle_msg_pool.high_priority_msg_list, (sys_snode_t *)node);
				_msg_pool_release_msg_cache(msg_info);
				//SYS_LOG_INF("release high_priority_msg_cnt %d \n",--high_priority_msg_cnt);
				break;
			} else {
			}
			cnt++;
		}
		//SYS_LOG_INF("high_priority cnt %d\n",cnt);
	}

	os_mutex_unlock(&msg_pool_mutex);
	return 0;
}

int os_poll_msg(void *msg, int msg_size, os_sem *sem, int timeout)
{
#ifdef CONFIG_POLL
	struct k_poll_event events[2];
	int ret = OS_POLL_TIMEOUT;

	if (sem == NULL) {
		if (!os_receive_msg(msg, msg_size, timeout)) {
			ret = OS_POLL_MSG;
		}
		return ret;
	}

	k_poll_event_init(&events[0], K_POLL_TYPE_MBOX_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY, &global_mailbox);
	k_poll_event_init(&events[1], K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY, sem);

	int rc = k_poll(events, ARRAY_SIZE(events), SYS_TIMEOUT_MS(timeout));
	if (rc == 0) {
		if (events[0].state == K_POLL_STATE_MBOX_DATA_AVAILABLE) {
			if (!os_receive_msg(msg, msg_size, 0)) {
				ret = OS_POLL_MSG;
			} else {
				SYS_LOG_WRN("mbox (0x%x) poll err", (uint32_t)&global_mailbox);
			}
		} else if (events[1].state == K_POLL_STATE_SEM_AVAILABLE) {
			if (!k_sem_take(events[1].sem, K_NO_WAIT)) {
				ret = OS_POLL_SEM;
			} else {
				SYS_LOG_WRN("sem (0x%x) poll err", (uint32_t)sem);
			}
		}
	}
	return ret;
#else
	return os_receive_msg(msg, msg_size, timeout);
#endif
}

void os_msg_clean(void)
{
	unsigned int key = irq_lock();

	os_mbox_clear_msg(&global_mailbox);

	atomic_set(&globle_msg_pool.free_size, CONFIG_NUM_MBOX_ASYNC_MSGS);

	irq_unlock(key);
}

int os_get_pending_msg_cnt(void)
{
	return k_mbox_get_pending_msg_cnt(&global_mailbox, os_current_get());
}

void os_msg_init(void)
{
	struct msg_pool *pool = &globle_msg_pool;

	pool->free_size = CONFIG_NUM_MBOX_ASYNC_MSGS;
	pool->pool_size = CONFIG_NUM_MBOX_ASYNC_MSGS;
	pool->free_high_msg_cache_num = HIGH_PRIORITY_MSG_NUM;

	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info *msg_content = &pool->pool[i];
		os_sem_init(&msg_content->msg_sem, 1, 1);
	}

	for (uint8_t i = 0 ; i < HIGH_PRIORITY_MSG_NUM; i++) {
		struct msg_cache_item *msg_cache = &pool->high_msg_cache[i];
		memset(msg_cache, 0, sizeof(struct msg_cache_item));
	}

	sys_slist_init(&globle_msg_pool.high_priority_msg_list);
}

