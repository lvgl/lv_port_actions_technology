/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/atomic.h>

#include "de_common.h"

struct de_command_pool {
	sys_slist_t list;
	struct k_sem sem;
	struct k_spinlock lock;
};

struct de_instance {
	uint16_t flags;
	uint16_t sequence;

	bool waiting;
	atomic_t alloc_num;
	struct k_sem sem;
	struct k_spinlock lock;

	struct de_command_pool *pool;

	display_engine_instance_callback_t callback;
	void *user_data;
};

static struct de_command_entry *de_command_pool_alloc(struct de_command_pool *pool);
static void de_command_pool_free(struct de_command_pool *pool, struct de_command_entry *entry);

static struct de_command_entry __in_section_unique(ram.noinit.drv_de.cmd)
	de_entries[CONFIG_DISPLAY_ENGINE_DRAW_COMMAND_NUM + CONFIG_DISPLAY_ENGINE_POST_COMMAND_NUM];

static struct de_command_pool de_draw_pool;
#if CONFIG_DISPLAY_ENGINE_POST_COMMAND_NUM > 0
static struct de_command_pool de_post_pool;
#endif

static struct de_instance de_instances[CONFIG_DISPLAY_ENGINE_INSTANCE_NUM];

int de_alloc_instance(uint32_t flags)
{
	for (int i = 0; i < ARRAY_SIZE(de_instances); i++) {
		if (de_instances[i].pool == NULL) {
			de_instances[i].flags = (uint16_t)flags;
#if CONFIG_DISPLAY_ENGINE_POST_COMMAND_NUM > 0
			if (flags & DISPLAY_ENGINE_FLAG_POST) {
				de_instances[i].pool = &de_post_pool;
			} else
#endif
			{
				de_instances[i].pool = &de_draw_pool;
			}

			k_sem_init(&de_instances[i].sem, 0, 1);
			memset(&de_instances[i].lock, 0, sizeof(de_instances[i].lock));
			return i;
		}
	}

	return -ENOBUFS;
}

int de_free_instance(int inst)
{
	assert(inst >= 0 && inst < ARRAY_SIZE(de_instances));
	assert(de_instances[inst].pool != NULL);

	memset(&de_instances[inst], 0, sizeof(de_instances[inst]));
	return 0;
}

bool de_instance_has_flag(int inst, uint32_t flag)
{
	assert(inst >= 0 && inst < ARRAY_SIZE(de_instances));
	assert(de_instances[inst].pool != NULL);

	return (de_instances[inst].flags & flag);
}

__de_func struct de_command_entry *de_instance_alloc_entry(int inst)
{
	struct de_command_entry *entry;

	assert(inst >= 0 && inst < ARRAY_SIZE(de_instances));
	assert(de_instances[inst].pool != NULL);

	if (inst < 0 ||  inst >= ARRAY_SIZE(de_instances) ||
		de_instances[inst].pool == NULL) {
		return NULL;
	}

	entry = de_command_pool_alloc(de_instances[inst].pool);
	assert (entry != NULL);

	if (entry) {
		entry->inst = (uint8_t)inst;
		entry->seq = de_instances[inst].sequence++;

		atomic_inc(&de_instances[inst].alloc_num);
	}

	return entry;
}

__de_func int de_instance_free_entry(struct de_command_entry *entry)
{
	int inst = entry->inst;
	bool waiting;

	assert(inst >= 0 && inst < ARRAY_SIZE(de_instances));
	assert(de_instances[inst].pool != NULL);

	atomic_dec(&de_instances[inst].alloc_num);

	k_spinlock_key_t key = k_spin_lock(&de_instances[inst].lock);
	waiting = de_instances[inst].waiting;
	k_spin_unlock(&de_instances[inst].lock, key);

	if (waiting) {
		k_sem_give(&de_instances[inst].sem);
	}

	de_command_pool_free(de_instances[inst].pool, entry);
	return 0;
}

__de_func int de_instance_poll(int inst, int timeout)
{
	k_spinlock_key_t key;

	assert(inst >= 0 && inst < ARRAY_SIZE(de_instances));
	assert(de_instances[inst].pool != NULL);

wait_finish:
	key = k_spin_lock(&de_instances[inst].lock);
	de_instances[inst].waiting = (atomic_get(&de_instances[inst].alloc_num) > 0);
	k_spin_unlock(&de_instances[inst].lock, key);

	if (de_instances[inst].waiting) {
		k_sem_take(&de_instances[inst].sem, (timeout >= 0) ? K_MSEC(timeout) : K_FOREVER);
		de_instances[inst].waiting = 0;
		goto wait_finish;
	}

	return atomic_get(&de_instances[inst].alloc_num) ? -ETIMEDOUT : 0;
}

int de_instance_register_callback(int inst, display_engine_instance_callback_t callback, void *user_data)
{
	assert(inst >= 0 && inst < ARRAY_SIZE(de_instances));
	assert(de_instances[inst].pool != NULL);

	de_instances[inst].user_data = user_data;
	de_instances[inst].callback = callback;
	return 0;
}

__de_func int de_instance_notify(struct de_command_entry *entry, int status)
{
	int inst = entry->inst;

	assert(inst >= 0 && inst < ARRAY_SIZE(de_instances));
	assert(de_instances[inst].pool != NULL);

	if (de_instances[inst].callback) {
		de_instances[inst].callback(status, entry->seq, de_instances[inst].user_data);
	}

	return 0;
}

static void de_command_pool_init(struct de_command_pool *pool,
		struct de_command_entry *entries, int num_entries)
{
	sys_slist_init(&pool->list);

	for (int i = num_entries - 1; i >= 0; i--) {
		sys_slist_append(&pool->list, &entries[i].node);
	}

	k_sem_init(&pool->sem, num_entries, num_entries);
}

void de_command_pools_init(void)
{
	de_command_pool_init(&de_draw_pool, de_entries, CONFIG_DISPLAY_ENGINE_DRAW_COMMAND_NUM);
#if CONFIG_DISPLAY_ENGINE_POST_COMMAND_NUM > 0
	de_command_pool_init(&de_post_pool, &de_entries[CONFIG_DISPLAY_ENGINE_DRAW_COMMAND_NUM], CONFIG_DISPLAY_ENGINE_POST_COMMAND_NUM);
#endif
}

__de_func static struct de_command_entry *de_command_pool_alloc(struct de_command_pool *pool)
{
	sys_snode_t *node = NULL;
	k_spinlock_key_t key;

get_free_entry:
	key = k_spin_lock(&pool->lock);
	node = sys_slist_peek_head(&pool->list);
	if (node)
		sys_slist_remove(&pool->list, NULL, node);
	k_spin_unlock(&pool->lock, key);

	if (node == NULL && !k_is_in_isr()) {
		k_sem_take(&pool->sem, K_FOREVER);
		goto get_free_entry;
	}

	assert (node != NULL);

	return CONTAINER_OF(node, struct de_command_entry, node);
}

__de_func static void de_command_pool_free(struct de_command_pool *pool, struct de_command_entry *entry)
{
	bool empty;

	k_spinlock_key_t key = k_spin_lock(&pool->lock);
	empty = sys_slist_is_empty(&pool->list);
	sys_slist_append(&pool->list, &entry->node);
	k_spin_unlock(&pool->lock, key);

	if (empty) {
		k_sem_give(&pool->sem);
	}
}
