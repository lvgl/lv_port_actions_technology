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
 *	2017/7/7: Created by wh.
 */

#include "os_common_api.h"
#include "string.h"
#include <zephyr.h>

#define MAX_THREAD_TERMINAL_NUM 3

struct thread_terminal_info_t{
	os_thread *wait_terminal_thread;
	os_sem terminal_sem;
};

static struct thread_terminal_info_t thread_terminal_info[MAX_THREAD_TERMINAL_NUM] = {0};

/**thread function */
int os_thread_create(char *stack, size_t stack_size,
					 void (*entry)(void *, void *, void*),
					 void *p1, void *p2, void *p3,
					 int prio, u32_t options, int delay) {
	k_tid_t tid = NULL;

	os_thread *thread = NULL;

	thread = (os_thread *)stack;

	tid = k_thread_create(thread, (os_thread_stack_t *)&stack[sizeof(os_thread)],
							stack_size - sizeof(os_thread),
							entry,
							p1, p2, p3,
							prio,
							options,
							SYS_TIMEOUT_MS(delay));

	return (int)tid;
}

int os_thread_prepare_terminal(int tid)
{
	int ret = 0;
	struct thread_terminal_info_t *terminal_info = NULL;

	os_sched_lock();

	for (int i = 0; i < MAX_THREAD_TERMINAL_NUM; i++){
		if(!thread_terminal_info[i].wait_terminal_thread) {
			terminal_info = &thread_terminal_info[i];
			break;
		}
	}

	if (!terminal_info) {
		SYS_LOG_ERR("%d busy\n", tid);
		ret = -EBUSY;
		goto exit;
	}

	terminal_info->wait_terminal_thread = (os_thread *)tid;
	os_sem_init(&terminal_info->terminal_sem, 0, 1);

	SYS_LOG_INF(" 0x%x ok\n",tid);
exit:
	os_sched_unlock();
	return ret;
}

int os_thread_wait_terminal(int tid)
{
	int ret = 0;
	struct thread_terminal_info_t *terminal_info = NULL;

	os_sched_lock();
	for (int i = 0; i < MAX_THREAD_TERMINAL_NUM; i++){

		if((uintptr_t)thread_terminal_info[i].wait_terminal_thread == tid) {
			terminal_info = &thread_terminal_info[i];
		}
	}
	os_sched_unlock();

	if (!terminal_info) {
		SYS_LOG_ERR("terminal tid %d not found\n",tid);
		ret = -EBUSY;
	}

	if (k_thread_join(terminal_info->wait_terminal_thread, SYS_TIMEOUT_MS(5000))) {
		SYS_LOG_ERR("timeout \n");
		ret = -EBUSY;
	}

	os_sched_lock();
	terminal_info->wait_terminal_thread = NULL;
	os_sched_unlock();

	SYS_LOG_INF(" 0x%x ok\n",tid);
	return ret;
}

const char *os_thread_get_name_by_prio(int prio)
{
	struct k_thread *thread_list = (struct k_thread *)(_kernel.threads);
	unsigned int key = irq_lock();

	while (thread_list != NULL) {
		int thread_prio = k_thread_priority_get(thread_list);
		if (prio == thread_prio) {
			break;
		}

		thread_list = (struct k_thread *)thread_list->next_thread;
	}
	irq_unlock(key);

	if (thread_list) {
		return k_thread_name_get(thread_list);
	}
	return "NULL";
}

/**message function*/

K_MBOX_DEFINE(global_mailbox);

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

int msg_pool_get_free_msg_num(void)
{
	struct msg_pool *pool = &globle_msg_pool;
	int used_num = 0;
	unsigned int key = irq_lock();

	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info * msg_content = &pool->pool[i];

		if (os_sem_take(&msg_content->msg_sem, OS_NO_WAIT) != 0) {
			used_num++;
		} else {
			os_sem_give(&msg_content->msg_sem);
		}
	}

	irq_unlock(key);

	return pool->pool_size - used_num;
}

void msg_pool_dump(void(*dump_fn)(os_tid_t sender, os_tid_t receiver,
		const char *content, int max_size))
{
	struct msg_pool *pool = &globle_msg_pool;

	printk("free msg cnt %d/%d\n", msg_pool_get_free_msg_num(), pool->pool_size);

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
}

#if 0
static void os_sync_msg_callback(struct os_app_msg* msg, int result, void* not_used)
{

	if (msg->sync_sem) {
#ifdef CONFIG_MESSAGE_DEBUG
		printk("--(%s->%s)-- %s %d: type_id %d, msg_id %d, e_time %u\n",
				"",
				msg_manager_get_current(),
				__func__, __LINE__,
				msg->type,
				msg->cmd,
				k_uptime_get_32());
#endif

		os_sem_give(msg->sync_sem);
	}

}

int os_send_sync_msg(void *receiver, void *msg, int msg_size)
{

	os_mbox_msg send_msg;
	char msg_content[MSG_MEM_SIZE];
	struct k_sem sync_sem;
	struct os_app_msg *tmp_msg = NULL;

	__ASSERT(!_is_in_isr(),"send messag in isr");

	memcpy(&msg_content, msg, msg_size);

	tmp_msg = &msg_content;
    tmp_msg->callback = os_sync_msg_callback;
    tmp_msg->sync_sem = &sync_sem;
    k_sem_init(&sync_sem, 0, UINT_MAX);

    /* prepare to send message */
    send_msg.info = msg_size;
    send_msg.size = msg_size;
    send_msg.tx_data = &msg_content;
    send_msg.tx_target_thread = (os_tid_t)receiver;

    /* send message containing most current data and loop around */
    os_mbox_put(&global_mailbox, &send_msg, SYS_TIMEOUT_MS(OS_FOREVER));

    os_sem_take(&sync_sem, OS_FOREVER);


	return 0;
}
#endif

int os_send_async_msg(void *receiver, void *msg, int msg_size)
{
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

	return 0;
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

	return 0;
}

void os_msg_clean(void)
{
	os_mbox_clear_msg(&global_mailbox);
}

int os_get_pending_msg_cnt(void)
{
	return k_mbox_get_pending_msg_cnt(&global_mailbox, os_current_get());
}

void os_msg_init(void)
{
	struct msg_pool *pool = &globle_msg_pool;

	for (uint8_t i = 0 ; i < pool->pool_size; i++) {
		struct msg_info *msg_content = &pool->pool[i];
		os_sem_init(&msg_content->msg_sem, 1, 1);
	}
}

static bool low_latency_mode = true;

int system_check_low_latencey_mode(void)
{
#ifdef CONFIG_OS_LOW_LATENCY_MODE
	return low_latency_mode ? 1 : 0;
#else
	return 0;
#endif
}

void system_set_low_latencey_mode(bool low_latencey)
{
	low_latency_mode = low_latencey;
}

s32_t os_sleep(int timeout)
{
	return k_sleep(SYS_TIMEOUT_MS(timeout));
}

int os_sem_take(os_sem *sem, s32_t timeout)
{
	return k_sem_take(sem, SYS_TIMEOUT_MS(timeout));
}

int os_mutex_lock(os_mutex * mutex, s32_t timeout)
{
	return k_mutex_lock(mutex, SYS_TIMEOUT_MS(timeout));
}

int os_delayed_work_submit(os_delayed_work *work, s32_t delay)
{
	return k_delayed_work_submit(work, SYS_TIMEOUT_MS(delay));
}

int os_delayed_work_submit_to_queue(os_work_q *work_q, os_delayed_work *work, s32_t delay)
{
	return k_delayed_work_submit_to_queue(work_q, work, SYS_TIMEOUT_MS(delay));
}

void *os_fifo_get(os_fifo *fifo, int32_t timeout)
{
	return k_fifo_get(fifo, SYS_TIMEOUT_MS(timeout));
}

int os_msgq_put(os_msgq *msgq, const void *data, int32_t timeout)
{
	return k_msgq_put(msgq, data, SYS_TIMEOUT_MS(timeout));
}

int os_msgq_get(os_msgq *msgq, void *data, int32_t timeout)
{
	return k_msgq_get(msgq, data, SYS_TIMEOUT_MS(timeout));
}

bool os_is_in_isr(void)
{
	return k_is_in_isr();
}

void os_printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (IS_ENABLED(CONFIG_LOG_PRINTK)) {
		log_printk(fmt, ap);
	} else {
		vprintk(fmt, ap);
	}

	va_end(ap);
}