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
 *	2017/1/20: Created by wh.
 */
#ifndef __SERVICE_MANAGER_H__
#define __SERVICE_MANAGER_H__

extern struct service_entry_t __service_entry_table[];
extern struct service_entry_t __service_entry_end[];

struct service_info {
	sys_snode_t node;     /* used for app list*/
	char *name;
	os_tid_t tid;          /* app thread id*/
	char *stackptr;
	struct service_entry_t *entry;
};

#define SRV_INFO(_node) CONTAINER_OF(_node, struct service_info, node)

#endif

