/*!
 * \file     nv_protocol.h
 * \brief
 * \details
 * \author
 * \date
 * \copyright Actions
 */

#ifndef _NV_PROTOCOL_H
#define _NV_PROTOCOL_H
#include <stdint.h>

enum BLE_NV_CHARACTERISTICS
{
    BLE_NV_SV = 0,
    BLE_NV_REQ,
    BLE_NV_LWP,
	BLE_NV_LWO,
	BLE_NV_DID,
	BLE_NV_CID,
	BLE_NV_DP,
	BLE_NV_ADDR,
	BLE_NV_AUTH,

    BLE_NV_CHARACTERISTICS_NUM,
};


typedef struct
{
	// VAServiceVersion UUID: 38FA8BF1-A473-4438-B307-48C647BF5AC0
	// Permissions: Read, Payload size: 4 bytes
	int (*nv_vaserviceversion_cb)(u8_t *, u16_t);
	// NVDeviceID UUID: 38FA8BF5-A473-4438-B307-48C647BF5AC0
	// Permissions: Read, Payload size: 16 bytes
	int (*nv_deviceid_cb)(u8_t *, u16_t);
	// NVClientID UUID: 38FA8BF6-A473-4438-B307-48C647BF5AC0
	// Permissions: Write, Payload size: 4 bytes
	int (*nv_clientid_cb)(u8_t *, u16_t);

	int (*nv_key_cb)(u8_t *, u16_t);
} nv_user_cb_st;

void nv_user_cb_init(nv_user_cb_st *p_cb_st);

void nv_resource_alloc(void);
void nv_resource_release(void);
u8_t ble_nv_status(void);
u8_t nv_uuid_num_get(void *attr);
void nv_recv_proc(u8_t *buf, u16_t len);
void nv_va_stop_session(void);
u16_t nv_sv_get(u8_t *buf);
u16_t nv_did_get(u8_t *buf);
u16_t nv_cid_set(u8_t *buf, u16_t len);

#endif  // _NV_PROTOCOL_H


