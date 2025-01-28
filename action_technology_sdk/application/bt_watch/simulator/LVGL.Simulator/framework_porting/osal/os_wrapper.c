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
#ifdef CONFIG_SIMULATOR
#include <Windows.h>
#endif

uint32_t os_cycle_get_32(void)
{
#ifdef CONFIG_SIMULATOR
	return GetTickCount();
#endif
}

uint32_t os_current_get(void)
{
#ifdef CONFIG_SIMULATOR
	return GetCurrentThreadId();
#endif
}

uint32_t os_cyc_to_ms_near32(uint32_t t)
{
	return t;
}

uint32_t os_uptime_get_32(void)
{
#ifdef CONFIG_SIMULATOR
	return GetTickCount();
#endif
}

int64_t os_uptime_get(void)
{
#ifdef CONFIG_SIMULATOR
	return GetTickCount64();
#endif
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

int32_t os_sleep(int timeout)
{
#ifdef CONFIG_SIMULATOR
	Sleep(timeout);
	return 0;
#else
	return k_sleep(SYS_TIMEOUT_MS(timeout));
#endif
}

void *os_fifo_get(os_fifo *fifo, int32_t timeout)
{
#ifndef CONFIG_SIMULATOR
	return k_fifo_get(fifo, SYS_TIMEOUT_MS(timeout));
#endif
}

int os_msgq_put(os_msgq *msgq, const void *data, int32_t timeout)
{
#ifndef CONFIG_SIMULATOR
	return k_msgq_put(msgq, data, SYS_TIMEOUT_MS(timeout));
#endif
}

int os_msgq_get(os_msgq *msgq, void *data, int32_t timeout)
{
#ifndef CONFIG_SIMULATOR
	return k_msgq_get(msgq, data, SYS_TIMEOUT_MS(timeout));
#endif
}

bool os_is_in_isr(void)
{
#ifndef CONFIG_SIMULATOR
	return k_is_in_isr();
#endif
}

void os_sched_lock(void)
{
#ifndef CONFIG_SIMULATOR
	return k_is_in_isr();
#endif
}

void os_sched_unlock(void)
{
#ifndef CONFIG_SIMULATOR
	return k_is_in_isr();
#endif
}


int os_irq_lock(void)
{
#ifndef CONFIG_SIMULATOR
	return irq_lock();
#endif
}

void os_irq_unlock(int key)
{
#ifndef CONFIG_SIMULATOR
	return irq_unlock(key);
#endif
}

void os_printk(const char *fmt, ...)
{
#ifdef CONFIG_SIMULATOR
	printf(fmt);
#else
	va_list ap;

	va_start(ap, fmt);

	if (IS_ENABLED(CONFIG_LOG_os_printk)) {
		log_os_printk(fmt, ap);
	} else {
		vk_printk(fmt, ap);
	}

	va_end(ap);
#endif
}

void os_yield(void)
{

}

int atomic_inc(int *target)
{
#if 0
	int key;
	int ret;

	key = os_irq_lock();

	ret = *target;
	*target += 1;

	os_irq_unlock(key);
	return ret;
#else
	int val = *target;

	InterlockedIncrement(target);
	return val;
#endif
}

int atomic_dec(int *target)
{
#if 0
	int key;
	int ret;

	key = os_irq_lock();

	ret = *target;
	*target -= 1;

	os_irq_unlock(key);
	return ret;
#else
	int val = *target;

	InterlockedDecrement(target);
	return val;
#endif
}

int atomic_get(int *target)
{
	int key;
	int ret;

	key = os_irq_lock();

	ret = *target;

	os_irq_unlock(key);
	return ret;
}

int atomic_set(int *target, int value)
{
	int key;
	int ret;

	key = os_irq_lock();

	ret = *target;
	*target = value;

	os_irq_unlock(key);
	return ret;
}
