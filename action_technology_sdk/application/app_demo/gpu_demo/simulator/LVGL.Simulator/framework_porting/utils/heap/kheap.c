/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <kheap.h>
#ifdef CONFIG_SIMULATOR
#include <Windows.h>
#endif
void k_heap_init(struct k_heap *h, void *mem, size_t bytes)
{
	//z_waitq_init(&h->wait_q);
    h->mutex = CreateMutex(NULL, FALSE, NULL);
	sys_heap_init(&h->heap, mem, bytes);

	//SYS_PORT_TRACING_OBJ_INIT(k_heap, h);
}

void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
			int timeout)
{
	//int64_t now, end = sys_clock_timeout_end_calc(timeout);
	void *ret = NULL;
	//k_spinlock_key_t key = k_spin_lock(&h->lock);

	//SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap, aligned_alloc, h, timeout);

	//__ASSERT(!arch_is_in_isr() || K_TIMEOUT_EQ(timeout, K_NO_WAIT), "");
    WaitForSingleObject(h->mutex, timeout);
	bool blocked_alloc = false;


	ret = sys_heap_aligned_alloc(&h->heap, align, bytes);

	//now = sys_clock_tick_get();

	if (!blocked_alloc) {
		blocked_alloc = true;
		//SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_heap, aligned_alloc, h, timeout);
	} else {
		/**
			* @todo	Trace attempt to avoid empty trace segments
			*/
	}

	//(void) z_pend_curr(&h->lock, key, &h->wait_q,
	//		   K_TICKS(end - now));
	//key = k_spin_lock(&h->lock);


	//SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap, aligned_alloc, h, timeout, ret);

	//k_spin_unlock(&h->lock, key);
    ReleaseMutex(h->mutex);
	if(ret == NULL){
		printf("kalloc %d fail\n", bytes);
		sys_heap_dump(&h->heap);
	}
		
	return ret;
}

void *k_heap_alloc(struct k_heap *h, size_t bytes, int timeout)
{
	//SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap, alloc, h, timeout);

	void *ret = k_heap_aligned_alloc(h, sizeof(void *), bytes, timeout);

	//SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap, alloc, h, timeout, ret);

	return ret;
}

void k_heap_free(struct k_heap *h, void *mem)
{
	//k_spinlock_key_t key = k_spin_lock(&h->lock);
    WaitForSingleObject(h->mutex, INFINITE);
	sys_heap_free(&h->heap, mem);
    ReleaseMutex(h->mutex);
	//SYS_PORT_TRACING_OBJ_FUNC(k_heap, free, h);
	//if (IS_ENABLED(CONFIG_MULTITHREADING) && z_unpend_all(&h->wait_q) != 0) {
	//	z_reschedule(&h->lock, key);
	//} else {
	//	k_spin_unlock(&h->lock, key);
	//}
}
