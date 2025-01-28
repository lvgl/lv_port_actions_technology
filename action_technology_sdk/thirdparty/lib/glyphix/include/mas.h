/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __MAS_H
#define __MAS_H

#include "mas_low_lvl.h"
#include "mas_rpc.h"

#ifndef MAS_SERVER_DID
#define MAS_SERVER_DID 0
#endif

#define MAS_PKT_MAX_SIZE 20480

int mas_init();
int mas_deinit();
void mas_set_status_callback(mas_status_cb *cb, mas_status_callback_fn status_callback);
void mas_set_network_status_callback(mas_status_cb *cb, mas_status_callback_fn status_callback);
mas_status_t mas_get_status(void);
void mas_set_network_status(mas_network_status_type status);
mas_network_status_type mas_get_network_status(void);
int mas_get_server_ver_num(void);
void mas_log_enable(mas_log_t level, bool enabled);

#endif /* __MAS_H__ */
