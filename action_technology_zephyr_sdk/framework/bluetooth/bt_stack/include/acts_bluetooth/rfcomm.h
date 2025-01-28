/** @file
 *  @brief Bluetooth RFCOMM handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_RFCOMM_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_RFCOMM_H_

/**
 * @brief RFCOMM
 * @defgroup bt_rfcomm RFCOMM
 * @ingroup bluetooth
 * @{
 */

#include <acts_bluetooth/buf.h>
#include <acts_bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RFCOMM channels (1-30): pre-allocated for profiles to avoid conflicts
 * Just server register need channel, client connect server, use server rfcomm channel.
 * and the same channel can use in diffrent acl connection.
 */
enum {
	BT_RFCOMM_CHAN_USED_START = 1,
	BT_RFCOMM_CHAN_HFP_HF = 1,
	BT_RFCOMM_CHAN_HFP_AG,
	BT_RFCOMM_CHAN_HSP_AG,
	BT_RFCOMM_CHAN_HSP_HS,
	BT_RFCOMM_CHAN_SPP,					/* Zephyr bt shell used */
										/* 6~9 reserve for fixed channel used */
	BT_RFCOMM_CHAN_REG_START = 10,		/* For malloc register channel */
	BT_RFCOMM_CHAN_REG_END = 30,		/* 0x1E, for rfcomm address 5bit, must < 0x1f */
};

struct bt_rfcomm_dlc;

/** @brief RFCOMM DLC operations structure. */
struct bt_rfcomm_dlc_ops {
	/** DLC connected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param dlc The dlc that has been connected
	 */
	void (*connected)(struct bt_rfcomm_dlc *dlc);

	/** DLC disconnected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  dlc is disconnected, including when a connection gets
	 *  rejected or cancelled (both incoming and outgoing)
	 *
	 *  @param dlc The dlc that has been Disconnected
	 */
	void (*disconnected)(struct bt_rfcomm_dlc *dlc);

	/** DLC recv callback
	 *
	 *  @param dlc The dlc receiving data.
	 *  @param buf Buffer containing incoming data.
	 */
	void (*recv)(struct bt_rfcomm_dlc *dlc, struct net_buf *buf);
};

/** @brief Role of RFCOMM session and dlc. Used only by internal APIs
 */
typedef enum bt_rfcomm_role {
	BT_RFCOMM_ROLE_ACCEPTOR,
	BT_RFCOMM_ROLE_INITIATOR
} __packed bt_rfcomm_role_t;

/** @brief RFCOMM DLC structure. */
struct bt_rfcomm_dlc {
	/* Response Timeout eXpired (RTX) timer */
	struct k_delayed_work      rtx_work;

	struct bt_rfcomm_session  *session;
	struct bt_rfcomm_dlc_ops  *ops;
	struct bt_rfcomm_dlc      *_next;

	bt_security_t              required_sec_level;
	bt_rfcomm_role_t           role;

	uint16_t                      mtu;
	uint8_t                       dlci;
	uint8_t                       state;
	uint8_t                       rx_credit;
	uint8_t                       tx_credit;
	uint8_t                       msc_state;
};

struct bt_rfcomm_server {
	/** Server Channel */
	uint8_t channel;

	/** Server accept callback
	 *
	 *  This callback is called whenever a new incoming connection requires
	 *  authorization.
	 *
	 *  @param conn The connection that is requesting authorization
	 *  @param dlc Pointer to received the allocated dlc
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_rfcomm_dlc **dlc, uint8_t channel);

	struct bt_rfcomm_server	*_next;
};

/** @brief Register RFCOMM server
 *
 *  Register RFCOMM server for a channel, each new connection is authorized
 *  using the accept() callback which in case of success shall allocate the dlc
 *  structure to be used by the new connection.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_server_register(struct bt_rfcomm_server *server);

/** @brief Connect RFCOMM channel
 *
 *  Connect RFCOMM dlc by channel, once the connection is completed dlc
 *  connected() callback will be called. If the connection is rejected
 *  disconnected() callback is called instead.
 *
 *  @param conn Connection object.
 *  @param dlc Dlc object.
 *  @param channel Server channel to connect to.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_connect(struct bt_conn *conn, struct bt_rfcomm_dlc *dlc,
			  uint8_t channel);

/** @brief Send data to RFCOMM
 *
 *  Send data from buffer to the dlc. Length should be less than or equal to
 *  mtu.
 *
 *  @param dlc Dlc object.
 *  @param buf Data buffer.
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_send(struct bt_rfcomm_dlc *dlc, struct net_buf *buf);

/** @brief Disconnect RFCOMM dlc
 *
 *  Disconnect RFCOMM dlc, if the connection is pending it will be
 *  canceled and as a result the dlc disconnected() callback is called.
 *
 *  @param dlc Dlc object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_disconnect(struct bt_rfcomm_dlc *dlc);

/** @brief Allocate the buffer from pool after reserving head room for RFCOMM,
 *  L2CAP and ACL headers.
 *
 *  @param pool Which pool to take the buffer from.
 *
 *  @return New buffer.
 */
struct net_buf *bt_rfcomm_create_pdu(struct net_buf_pool *pool);

/* Actions add start */

/** @brief Malloc register rfcomm server channel,
 *
 *  @return channel, 0 failed, other success.
 */
uint8_t bt_rfcomm_malloc_reg_channel(void);

/** @brief free register rfcomm server channel,
 *
 *  @param channel.
 */
void bt_rfcomm_free_reg_channel(uint8_t channel);

/* Actions add end */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_RFCOMM_H_ */
