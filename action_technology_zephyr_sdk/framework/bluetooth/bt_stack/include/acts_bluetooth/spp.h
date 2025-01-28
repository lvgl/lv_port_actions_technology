/** @file
 *  @brief Bluetooth SPP handling
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_SPP_H
#define __BT_SPP_H

/**
 * @brief SPP
 * @defgroup SPP
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <acts_bluetooth/buf.h>
#include <acts_bluetooth/conn.h>

/** @brief spp app call back structure. */
struct bt_spp_app_cb {
	void (*connect_failed)(struct bt_conn *conn, uint8_t spp_id);
	void (*connected)(struct bt_conn *conn, uint8_t spp_id);
	void (*disconnected)(struct bt_conn *conn, uint8_t spp_id);
	void (*recv)(struct bt_conn *conn, uint8_t spp_id, uint8_t *data, uint16_t len);
};

int bt_spp_send_data(uint8_t channel, uint8_t *data, uint16_t len);
uint8_t bt_spp_connect(struct bt_conn *conn, uint8_t *uuid);
int bt_spp_disconnect(uint8_t spp_id);
uint8_t bt_spp_register_service(uint8_t *uuid);
int bt_spp_register_cb(struct bt_spp_app_cb *cb);
void bt_spp_service_restore(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* __BT_SPP_H */
