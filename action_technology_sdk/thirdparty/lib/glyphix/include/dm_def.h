/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __DM_DEF_H__
#define __DM_DEF_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "gx_cdef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DM_SW_VERSION           "0.0.1"
#define DM_SW_VERSION_NUM       0x000001L

#define DM_MAGIC_NUM            0x44434d30L

struct dm_pool;
typedef void (*dm_load_cb_t)(struct dm_pool *pool, void *user_data);
typedef void (*dm_notify_fn_t)(void *user_data);

typedef enum {
    DM_OK = 0,
    DM_FAILED,
    DM_NO_MEM,
    DM_NAME_ERR,
    DM_INIT_ERR,
    DM_PARAM_ERR,
} dm_err_t;

typedef enum {
    DM_CB_TYPE_CHANGE = 0,
    DM_CB_TYPE_GET,
    DM_CB_TYPE_DELETE,
    DM_CB_TYPE_RELEASE             /* notice of release */
} dm_cb_type_t;

typedef enum {
    DM_HOOK_POOL_DEINIT = 0,
    DM_HOOK_POOL_RESET,
    DM_HOOK_CACHE_BIND,
    DM_HOOK_CACHE_UNBIND,
    DM_HOOK_CACHE_PUT,
    DM_HOOK_CACHE_GET,
    DM_HOOK_CACHE_CLONE,
    DM_HOOK_CACHE_DELETE,
    DM_HOOK_CACHE_NODE_FIND,
    DM_HOOK_CACHE_BLOB_UPDATE,
    DM_HOOK_CACHE_FLAG_SET,
    DM_HOOK_CACHE_FLAG_CLEAN
} dm_hook_type_t;

struct dm_blob
{
    void *buf;                              /* blob data buffer */
    size_t len;                             /* blob data length */
};
typedef struct dm_blob dm_blob_t;

struct dm_notify
{
    uint8_t type;                           /* callback type */
    int8_t ref;                             /* reference count */
    uint8_t reserve;
    dm_notify_fn_t callback;                /* value change callback function */
    void *user_data;                        /* callback user-data */
    gx_slist_t list;                        /* callback list */
};

#define DM_CACHE_FLAG_AUTO_STORE    0x1 << 0

struct dm_cache_node
{
    char *name;                             /* cache node name */
    struct dm_blob *blob;                   /* cache blob data */
    gx_slist_t list;                        /* cache node list */
    gx_slist_t cb_list;                     /* cache node callback list */
    long flags;
};
typedef struct dm_cache_node dm_cache_node_t;

struct dm_pool;
typedef void (*dm_hook_t)(struct dm_pool* pool, dm_hook_type_t type, const char* key);

struct dm_pool
{
    uint32_t magic;                         /* magic word(`D`, `C`, `M`, `0`) */
    char *name;                             /* cache pool name */
    uint16_t max_num;                       /* the maximum number of cache node */
    bool init_ok;                           /* initialized successfully */
    dm_hook_t hook;                         /* hook function */
    void *lock;                             /* cache pool operation lock */
    void *tpool;                            /* value change callback thread pool */
    gx_slist_t *clist;                      /* cache node list array */
    gx_slist_t list;                        /* cache pool list */
    void *db_handle;                        /* db handle */
    const char *db_name;                    /* db name */
    uint32_t db_size;                       /* db size */
    uint32_t sec_size;                      /* db sector size */
    uint32_t db_mode;                       /* db mode 0: file mode,1: fal mode */
};
typedef struct dm_pool dm_pool_t;

#ifdef __cplusplus
}
#endif

#endif /* __DM_DEF_H__ */
