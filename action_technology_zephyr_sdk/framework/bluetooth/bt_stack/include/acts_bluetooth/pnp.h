/** @file
 *  @brief Pnp infomation Profile handling.
 */

/*
 * Copyright (c) 2015-2099 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_PNP_H
#define __BT_PNP_H

#ifdef __cplusplus
extern "C" {
#endif

struct bt_pnp_info_cb {
	int (*info_get)(struct bt_conn *conn, uint16_t type, void *param);
};

int bt_pnp_info_register_cb(struct bt_pnp_info_cb *cb);
int bt_pnp_info_search(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* __BT_PNP_H */
