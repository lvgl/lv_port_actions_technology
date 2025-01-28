/** @file
 *  @brief Handsfree Profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_

/**
 * @brief Hands Free Profile (HFP)
 * @defgroup bt_hfp Hands Free Profile (HFP)
 * @ingroup bluetooth
 * @{
 */

#include <acts_bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Command complete types for the application
 */
#define HFP_HF_CMD_OK             0
#define HFP_HF_CMD_ERROR          1
#define HFP_HF_CMD_CME_ERROR      2
#define HFP_HF_CMD_UNKNOWN_ERROR  4
#define HFP_HF_CMD_TIMEOUT        8

/** @brief HFP HF Command completion field */
struct bt_hfp_hf_cmd_complete {
	/* Command complete status */
	uint8_t type;
	/* CME error number to be added */
	uint8_t cme;
	/* Error command */
	char *sending_cmd;
};

/** @brief HFP profile application callback */
struct bt_hfp_hf_cb {
	void (*connect_failed)(struct bt_conn *conn);
	/** HF connected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param conn Connection object.
	 */
	void (*connected)(struct bt_conn *conn);
	/** HF disconnected callback to application
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection gets disconnected, including when a connection gets
	 *  rejected or cancelled or any error in SLC establisment.
	 *
	 *  @param conn Connection object.
	 */
	void (*disconnected)(struct bt_conn *conn);
	/** HF indicator Callback
	 *
	 *  This callback provides service indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value service indicator value received from the AG.
	 */
	void (*service)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides call indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value call indicator value received from the AG.
	 */
	void (*call)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides call setup indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value call setup indicator value received from the AG.
	 */
	void (*call_setup)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides call held indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value call held indicator value received from the AG.
	 */
	void (*call_held)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides signal indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value signal indicator value received from the AG.
	 */
	void (*signal)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback provides roaming indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value roaming indicator value received from the AG.
	 */
	void (*roam)(struct bt_conn *conn, uint32_t value);
	/** HF indicator Callback
	 *
	 *  This callback battery service indicator value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value battery indicator value received from the AG.
	 */
	void (*battery)(struct bt_conn *conn, uint32_t value);
	/** HF incoming call Ring indication callback to application
	 *
	 *  If this callback is provided it will be called whenever there
	 *  is an incoming call.
	 *
	 *  @param conn Connection object.
	 */
	void (*ring_indication)(struct bt_conn *conn);
	/** HF notify command completed callback to application
	 *
	 *  The command sent from the application is notified about its status
	 *
	 *  @param conn Connection object.
	 *  @param cmd structure contains status of the command including cme.
	 */
	void (*cmd_complete_cb)(struct bt_conn *conn,
			      struct bt_hfp_hf_cmd_complete *cmd);

	/** HF Bluetooth Set In-band Ring tone
	 *
	 *  This callback bluetooth set in-band ring tone value to the application
	 *
	 *  @param conn Connection object.
	 *  @param value 0: not provide in-band ring tones, 1: provide in-band ring tones.
	 */
	void (*bsir)(struct bt_conn *conn, uint32_t value);

	/** HF Call Waiting notification
	 *
	 *  This callback notify call waiting
	 *
	 *  @param conn Connection object.
	 *  @param buf: Phone number.
	 *  @param len: Phone number length.
	 *  @param type: Phone number type.
	 */
	void (*ccwa)(struct bt_conn *conn, uint8_t *buf, uint32_t type);

	/** HF Calling Line Identification notification
	 *
	 *  This callback notify calling line indentification
	 *
	 *  @param conn Connection object.
	 *  @param buf: Phone number.
	 *  @param len: Phone number length.
	 *  @param type: Phone number type.
	 */
	void (*clip)(struct bt_conn *conn, uint8_t *buf, uint32_t type, uint8_t *name);

	/** HF Bluetooth codec selection
	 *
	 *  This callback notify calling line indentification
	 *
	 *  @param conn Connection object.
	 *  @param codec_id: codec id.
	 */
	void (*bcs)(struct bt_conn *conn, uint32_t codec_id);

	/** HF Bluetooth Voice Recognition Activation.
	 *
	 *  This callback notify Voice Recognition Activation.
	 *
	 *  @param conn Connection object.
	 *  @param active: 0 Disable Voice recognition, 1 Enable Voice recognition.
	 */
	void (*bvra)(struct bt_conn *conn, uint32_t active);

	/** HF Gain of Speaker.
	 *
	 *  This callback notify Gain of Speaker.
	 *
	 *  @param conn Connection object.
	 *  @param value: gain
	 */
	void (*vgs)(struct bt_conn *conn, uint32_t value);

	/** HF Gain of Microphone.
	 *
	 *  This callback notify Gain of Microphone.
	 *
	 *  @param conn Connection object.
	 *  @param value: gain
	 */
	void (*vgm)(struct bt_conn *conn, uint32_t value);

	/** HF Bluetooth Response and Hold Feature.
	 *
	 *  This callback notify Bluetooth Response and Hold.
	 *
	 *  @param conn Connection object.
	 *  @param value:  Response and Hold
	 */
	void (*btrh)(struct bt_conn *conn, uint32_t value);

	/** HF network operator.
	 *
	 *  This callback notify network operator..
	 *
	 *  @param conn Connection object.
	 *  @param mode
	 *  @param format
	 *  @param opt
	 */
	void (*cops)(struct bt_conn *conn, uint32_t mode, uint32_t format, uint8_t *opt);

	/** HF Subscriber Number Information.
	 *
	 *  This callback notify Subscriber Number Information.
	 *
	 *  @param conn Connection object.
	 *  @param buf: Phone number.
	 *  @param len: Phone number length.
	 *  @param type: Phone number type.
	 */
	void (*cnum)(struct bt_conn *conn, uint8_t *buf, uint32_t type);

	/** HF List current calls result code.
	 *
	 *	This callback notify list current calls result code.
	 *
	 *	@param conn Connection object.
	 *	@param idx: The numbering (starting with 1) of the call given by the sequence.
	 *	@param dir: outgoing or incoming.
	 *	@param status: refer HFP_v1.7.2.pdf.
	 *	@param mode: refer HFP_v1.7.2.pdf.
	 *	@param mpty: refer HFP_v1.7.2.pdf.
	 */
	void (*clcc)(struct bt_conn *conn, uint32_t idx, uint32_t dir, uint32_t status, uint32_t mode, uint32_t mpty,uint8_t *number, uint32_t type, uint8_t *name);

	/** HF Subscriber Number Information.
	 *
	 *  This callback notify Subscriber Number Information.
	 *
	 *  @param conn Connection object.
	 *  @param time: like "18/03/03,21:00:02-32".
	 */
	void (*cclk)(struct bt_conn *conn, uint8_t *buf);

	/** HF Manufacturer Information.
	 *
	 *  This callback notify Manufacturer Information.
	 *
	 *  @param conn Connection object.
	 *  @param time: like "Apple Inc.".
	 */
	void (*cgmi)(struct bt_conn *conn, uint8_t *buf);

	/** HF Battery Indicator.
	 *
	 *  This callback battery indicator.
	 *
	 *  @param conn Connection object.
	 *  @param status: enable or disable.
	 */
	void (*battery_indicator)(struct bt_conn *conn, uint32_t status);

	/** HF AT commands receive.
	 *
	 *  This callback to receive all AT commands.
	 *
	 *  @param conn Connection object.
	 *  @param data Data content.
	 *  @param len Data length.
	 */
	void (*recv)(struct bt_conn *conn, uint8_t *data, uint16_t len);

};

/** @brief Register HFP HF call back function
 *
 *  Register Handsfree callbacks to monitor the state and get the
 *  required HFP details to display.
 *
 *  @param cb callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_register_cb(struct bt_hfp_hf_cb *cb);

/** @brief Handsfree client Send AT
 *
 *  Send specific AT commands to handsfree client profile.
 *
 *  @param conn Connection object.
 *  @param user_cmd user command
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_send_cmd(struct bt_conn *conn, char *user_cmd);

/** @brief Handsfree connect AG.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_connect(struct bt_conn *conn);

/** @brief Handsfree disconnect AG.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hfp_hf_disconnect(struct bt_conn *conn);

/** @brief Handsfree request codec connection.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of req connection,
 *                 other hf not connected,
 */
int bt_hfp_hf_req_codec_connection(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HFP_HF_H_ */
