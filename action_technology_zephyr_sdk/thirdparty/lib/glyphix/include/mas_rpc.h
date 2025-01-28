/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __MAS_RPC_H__
#define __MAS_RPC_H__

#include "intrusive/gx_list.h"
#include "var_export.h"

#include <gx_cdef.h>
#include <stdint.h>

#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAS_RPC_SW_VER            "0.1.0"
#define MAS_RPC_SW_VER_NUM        0x000100

#define MAS_RPC_SVC_VER_NUM       0x1

#define MAS_RPC_SVC_FLAG_NEED_RSP 0x01
#define MAS_RPC_SVC_FLAG_NEED_ACK 0x02

/* masRpc data object */
struct mas_rpc_blob {
    void *buf;   /* raw buffer */
    size_t len;  /* current data length */
    size_t size; /* maximum data size */
};

/* masRpc service execute function */
typedef struct mas_rpc_blob *(*mas_rpc_svc_fn)(struct mas_rpc_blob *input);

/* masRpc service object */
struct mas_rpc_svc {
    uint8_t ver;       /* version */
    const char *name;  /* service name */
    mas_rpc_svc_fn fn; /* service function*/
    gx_slist_t list;   /* service list */
};

/* masRpc initialize and start */
int mas_rpc_init(uint8_t id);
void mas_rpc_proc();
void mas_rpc_run();

#define MOBILE_ABILITY_VAR_EXPORT(func)                                                            \
    extern const VarExport _var_export_##func;                                                     \
    const VarExport _var_export_##func = {#func, (void *)&func};                                   \
    GX_VAR_EXPORT(mobility_ability_var_export, _var_export_##func)

/* masRpc service static register */
#define MAS_RPC_SVC_STATIC_REGISTER_ALIAS(func, name)                                              \
    static const char mas_rpc_svc_##func##_name[] = #name;                                         \
    static const struct mas_rpc_svc mas_rpc_##name = {                                             \
        MAS_RPC_SVC_VER_NUM, mas_rpc_svc_##func##_name, (mas_rpc_svc_fn) & func, nullptr};         \
    MOBILE_ABILITY_VAR_EXPORT(mas_rpc_##name)

#define MAS_RPC_FFI_FUNC_STATIC_REGISTER_ALIAS(func, name)                                         \
    static const char mas_ffi_svc_##func##_name[] = "ffi." #name;                                  \
    static const struct mas_rpc_svc mas_ffi_##name = {                                             \
        MAS_RPC_SVC_VER_NUM, mas_ffi_svc_##func##_name, (mas_rpc_svc_fn)(void *)&func, nullptr};   \
    MOBILE_ABILITY_VAR_EXPORT(mas_ffi_##name)

#define MAS_RPC_SVC_STATIC_REGISTER(func)      MAS_RPC_SVC_STATIC_REGISTER_ALIAS(func, func)
#define MAS_RPC_FFI_FUNC_STATIC_REGISTER(func) MAS_RPC_FFI_FUNC_STATIC_REGISTER_ALIAS(func, func)

/* masRpc service dynamic register */
int mas_rpc_svc_register(const char *name, mas_rpc_svc_fn svc_fn);
int mas_rpc_ffi_func_register(const char *func_name, void *func_addr);

/* masRpc service execute */
int mas_rpc_svc_exec(uint8_t dst_id, const char *name, uint16_t flag, struct mas_rpc_blob *input,
                     struct mas_rpc_blob *output, int32_t timeout);

/* masRpc blob object operates */
struct mas_rpc_blob *mas_rpc_blob_make(struct mas_rpc_blob *blob, void *buf, size_t len,
                                       size_t size);
struct mas_rpc_blob *mas_rpc_blob_alloc(size_t size);
void mas_rpc_blob_free(struct mas_rpc_blob *blob);

/* masRpc create error code blob object */
struct mas_rpc_blob *mas_rpc_create_result_blob(uint8_t result_code);
uint8_t mas_rpc_result_code_get(struct mas_rpc_blob *blob);

/* cJSON object and blob object change */
struct mas_rpc_blob *mas_rcp_json_to_blob(cJSON *json, struct mas_rpc_blob *blob);
cJSON *mas_rpc_blob_to_json(struct mas_rpc_blob *blob, cJSON *json);

#ifdef __cplusplus
}
#endif

#endif /* __MAS_RPC_H__ */
