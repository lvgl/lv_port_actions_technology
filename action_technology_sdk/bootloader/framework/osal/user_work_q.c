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
#include <kernel.h>
#include <init.h>
#include "os_common_api.h"

#ifdef CONFIG_USER_WORK_Q

static struct k_work_q user_workq;
static K_THREAD_STACK_DEFINE(user_workq_stack, CONFIG_USER_WORK_Q_STACK_SIZE);

static int user_work_q_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_queue_start(&user_workq, user_workq_stack,
		       K_THREAD_STACK_SIZEOF(user_workq_stack),
		       CONFIG_USER_WORK_Q_PRIORITY, NULL);
	k_thread_name_set(&user_workq.thread, "userworkq");

	return 0;
}

SYS_INIT(user_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

os_work_q *os_get_user_work_queue(void)
{
#ifdef CONFIG_USER_WORK_Q
	return &user_workq;
#else
	return NULL;
#endif
}