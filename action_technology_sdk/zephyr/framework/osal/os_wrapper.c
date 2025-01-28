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
#include <sys/atomic.h>

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
		goto exit;
	}

	if (k_thread_join(terminal_info->wait_terminal_thread, SYS_TIMEOUT_MS(5000))) {
		SYS_LOG_ERR("timeout \n");
		ret = -EBUSY;
		goto exit;
	}

	os_sched_lock();
	terminal_info->wait_terminal_thread = NULL;
	os_sched_unlock();

	SYS_LOG_INF(" 0x%x ok\n",tid);
exit:
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
#ifndef CONFIG_ACTLOG_USE_NANOLOG
void actlog_printk_nano(uint32_t pack_data, const char *fmt, ...)
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
#endif