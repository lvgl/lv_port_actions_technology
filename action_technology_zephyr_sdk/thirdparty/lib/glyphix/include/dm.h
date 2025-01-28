/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef DM_H_
#define DM_H_

#include <dm_def.h>
#include <dm_cfg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* data manager pool operation */
int dm_pool_init(struct dm_pool *pool, dm_load_cb_t load_cb, void *user_data);
void dm_pool_deinit(struct dm_pool *pool);
int dm_pool_reset(struct dm_pool* pool);
struct dm_pool *dm_pool_find(const char *name);
dm_hook_t dm_pool_sethook(struct dm_pool* pool, dm_hook_t hook);

/* data manager cache node operation */
struct dm_notify* dm_cache_bind(struct dm_pool* pool, const char* key, dm_cb_type_t type, dm_notify_fn_t callback, void* user_data);
int dm_cache_unbind(struct dm_pool *pool, const char* key, struct dm_notify* notify);
int dm_cache_put(struct dm_pool *pool, const char *key, struct dm_blob *blob);
const dm_blob *dm_cache_get(struct dm_pool *pool, const char *key);
int dm_cache_clone(struct dm_pool *pool, const char *key, struct dm_blob *blob);
int dm_cache_delete(struct dm_pool *pool, const char *key);
struct dm_cache_node *dm_cache_node_find(struct dm_pool *pool, const char * key);
int dm_cache_blob_update(struct dm_pool *pool, const char *key, struct dm_blob *blob /* It must be alloc blob */);
int dm_cache_flag_set(struct dm_pool *pool, const char *key, long flag);
int dm_cache_flag_clean(struct dm_pool *pool, const char *key, long flag);

/* blob format data operation */
struct dm_blob *dm_blob_make(struct dm_blob *blob, void *buf, size_t len);
struct dm_blob *dm_blob_alloc(size_t len);
void dm_blob_free(struct dm_blob *blob);

struct dm_blob *dm_blob_duplicate(const struct dm_blob *blob);
struct dm_blob *dm_blob_copy(struct dm_blob *des, const struct dm_blob *src);
int dm_blob_compare(const struct dm_blob *blob1, const struct dm_blob *blob2);

int dm_pool_lock(struct dm_pool* pool);
void dm_pool_unlock(struct dm_pool* pool);

/* port */
void* dm_buff_alloc(const size_t len);
void* dm_buff_realloc(void* ptr, const size_t len);
void dm_buff_free(void* ptr);

#ifdef DM_USING_STORAGE
/* data manager data storage */
int dm_cache_reset(struct dm_pool* pool);
int dm_cache_save(struct dm_pool *pool, const char *key);
int dm_cache_save_sync(struct dm_pool* pool, const char* key);
int dm_cache_reload(struct dm_pool* pool, const char* key);
int dm_cache_set_and_save(struct dm_pool *pool, const char *key, struct dm_blob *blob);
int dm_cache_del_and_save(struct dm_pool *pool, const char *key);
int dm_cache_set_and_save_sync(struct dm_pool *pool, const char *key, struct dm_blob *blob);
int dm_cache_del_and_save_sync(struct dm_pool *pool, const char *key);
#endif /* DM_USING_STORAGE */

#ifdef __cplusplus
}
#endif

#endif /* DM_H_ */
