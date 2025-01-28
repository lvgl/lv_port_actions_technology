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
#include "string.h"

#ifdef CONFIG_SIMULATOR
#include <Windows.h>
typedef void (*callback_temp)(void*, void*, void*);
#endif

#ifdef CONFIG_SIMULATOR // work queue thread call back function
void os_work_queue_thread_start_callback(LPVOID lpParam)
{
    if (!lpParam)
    {
        printf("os_work_queue_thread_callback: work queue is not valied!");
        return;
    }
    os_work_q* queue = (os_work_q*)lpParam;

    while (1)
    {
        os_mutex_lock(&queue->mutex, OS_FOREVER);
        sys_snode_t* node = sys_slist_get(&queue->pending);

        if (!node)
        {
            os_mutex_unlock(&queue->mutex);
            continue;
        }

        os_mutex_unlock(&queue->mutex);

        os_work* work = (os_work*)node;
        os_mutex_lock(&work->mutex, OS_FOREVER);

        if (work->handler && work->state == 0)
            work->handler(work);

        os_mutex_unlock(&work->mutex);
    }
}
#endif

void os_work_queue_start(os_work_q* queue, os_thread_stack_t* stack,
    size_t stack_size, int prio, const os_work_queue_config* cfg)
{
#ifdef CONFIG_SIMULATOR
    if (!queue)
    {
        printf("os_work_queue_start: work queue start failed!");
        return;
    }

    sys_slist_init(&queue->pending);
    queue->mutex = os_mutex_create();
    queue->tread_id = os_thread_create((char*)stack, stack_size,
                        (callback_temp)os_work_queue_thread_start_callback, (void*)queue,
                        NULL, NULL, prio, 0, 0);
#else
    k_work_queue_start(queue, stack, stack_size, prio, cfg);
#endif
}

void os_work_init(os_work *work, os_work_handler_t handler)
{
#ifdef CONFIG_SIMULATOR
    if (!work || !handler)
    {
        printf("os_work_init: work init failed!");
        return;
    }

    work->node.next = NULL;
    work->queue = NULL;
    work->handler = handler;

    work->state = 0;
    work->mutex = os_mutex_create();
#else
	return  k_work_init(work, handler);
#endif
}

void os_work_submit_to_queue(os_work_q *work_q, os_work *work)
{
#ifdef CONFIG_SIMULATOR

    if (!work_q || !work)
    {
        printf("os_work_submit_to_queue: work submit to queue failed!");
        return;
    }

    work->queue = work_q;
    work->node = *(sys_snode_t*)work;

    os_mutex_lock(&work_q->mutex, OS_FOREVER);
    sys_slist_append(&work_q->pending, (sys_snode_t*)work);
    os_mutex_unlock(&work_q->mutex);

#else
	return  k_work_submit_to_queue(work_q, work);
#endif
}

void os_work_submit(os_work *work)
{
#ifdef CONFIG_SIMULATOR
    os_work_submit_to_queue(work->queue, work);
#else
	return  k_work_submit(work);
#endif
}

void os_delayed_work_init(os_delayed_work *dwork, os_work_handler_t handler)
{
#ifdef CONFIG_SIMULATOR

    if (!dwork || !handler)
    {
        printf("os_delayed_work_init: dwork init failed!");
        return;
    }

    os_work_init(&dwork->work, handler);
    dwork->timeout = 0;

#else
    return  k_delayed_work_init(work, handler);
#endif
}

int os_delayed_work_submit(os_delayed_work *dwork, int32_t delay)
{
#ifdef CONFIG_SIMULATOR
    if (!dwork)
        return -1;

    return os_delayed_work_submit_to_queue(dwork->work.queue, dwork, delay);
#else
	return  k_delayed_work_submit(dwork,SYS_TIMEOUT_MS(delay));
#endif
}

#ifdef CONFIG_SIMULATOR
typedef struct
{
    os_work_q*       work_q;
    os_delayed_work* dwork ;
    int32_t          delay ;

}os_delayed_work_info;

void delayed_work_submit_callback(LPVOID lpParam)
{
    if (!lpParam)
    {
        printf("delayed_work_submit_callback: dwork submit to queue failed!");
        return;
    }
    os_delayed_work_info* info = (os_delayed_work_info*)lpParam;

    Sleep(info->delay);

    os_mutex_lock(&info->work_q->mutex, OS_FOREVER);
    os_mutex_lock(&info->dwork->work.mutex, OS_FOREVER);

    if (info->dwork->work.state == 0)
        sys_slist_append(&info->work_q->pending, (sys_snode_t*)info->dwork);

    os_mutex_unlock(&info->work_q->mutex);
    os_mutex_unlock(&info->dwork->work.mutex);

    free(info);
}
#endif

int os_delayed_work_cancel(os_delayed_work *dwork)
{
#ifdef CONFIG_SIMULATOR

    if (!dwork)
        return -1;

    os_mutex_lock(&dwork->work.mutex, OS_FOREVER);
    dwork->work.state = 1;
    os_mutex_unlock(&dwork->work.mutex);

    return 1;
#else
	return  k_delayed_work_cancel(dwork);
#endif
}

int os_delayed_work_submit_to_queue(os_work_q* work_q, os_delayed_work* dwork, int32_t delay)
{
#ifdef CONFIG_SIMULATOR
    if (!work_q || !dwork)
    {
        printf("os_delayed_work_submit_to_queue: dwork submit to queue failed!");
        return -1;
    }

    dwork->work.queue = work_q;
    dwork->work.node = *(sys_snode_t*)dwork;
    dwork->timeout = delay;

    int prio = os_thread_priority_get(work_q->tread_id);

    os_delayed_work_info* info = (os_delayed_work_info*)malloc(sizeof(os_delayed_work_info));

    info->work_q = work_q;
    info->dwork = dwork;
    info->delay = delay;

    int thread_id = os_thread_create(NULL, 0,
        (callback_temp)delayed_work_submit_callback, (void*)info,
        NULL, NULL, prio, 0, 0);

    HANDLE handle = OpenThread(THREAD_ALL_ACCESS, TRUE, thread_id);

    CloseHandle(handle);
    return 0;
#else
    return  k_delayed_work_submit_to_queue(work_q, dwork, SYS_TIMEOUT_MS(delay));
#endif
}
