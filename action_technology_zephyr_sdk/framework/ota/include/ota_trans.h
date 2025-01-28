/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA backend interface
 */

#ifndef __OTA_TRANS_H__
#define __OTA_TRANS_H__

#define OTA_BACKEND_TYPE_UNKNOWN		(0)
#define OTA_TRANS_TYPE_BLUETOOTH		(1)

#define OTA_TRANS_IOCTL_REQUEST_UPGRADE 	(0x10000)
#define OTA_TRANS_IOCTL_CONNECT_NEGOTIATION 	(0x10001)
#define OTA_TRANS_IOCTL_NEGOTIATION_RESULT 	(0x10002)
#define OTA_TRANS_IOCTL_SEND_IMAGE 	(0x10003)

#define OTA_TRANS_IOCTL_UNITSIZE_GET (0x20000)

enum OTA_TRANS_UPGRADE_STATUS
{
    OTA_TRANS_STATUS_NULL         = 0x0000,
    OTA_TRANS_INIT,
    OTA_TRANS_SDAP_RESULT,    
    OTA_TRANS_CONNECTED,   
    OTA_TRANS_DISCONNECT,    
    OTA_TRANS_REQUEST_UPGRADE_ACK,
    OTA_TRANS_CONNECT_NEGOTIATION_ACK,    
    OTA_TRANS_NEGOTIATION_RESULT_ACK,   
    OTA_TRANS_REQUEST_IMAGE_DATA,     
    OTA_TRANS_VALIDATE_REPORT,   
    OTA_TRANS_UPGRADE_STATUS_NOTIFY,       
};

struct ota_trans;

typedef void (*ota_trans_notify_cb_t)(struct ota_trans *trans, int state, void* param);

/**
 * @brief Logger backend API.
 */
struct ota_trans_api
{
	struct ota_trans * (*init)(ota_trans_notify_cb_t cb, void *init_param);
	void (*exit)(struct ota_trans *trans);
	int (*open)(struct ota_trans *trans);
	int (*read)(struct ota_trans *trans, int offset, void *buf, int size);
	int (*ioctl)(struct ota_trans *trans, int cmd, void *param);
	int (*close)(struct ota_trans *trans);
};

struct ota_trans {
	struct ota_trans_api *api;
	int type;
	ota_trans_notify_cb_t cb;
};

static inline int ota_trans_get_type(struct ota_trans *trans)
{
	 __ASSERT_NO_MSG(trans);

	return trans->type;
}

static inline int ota_trans_ioctl(struct ota_trans *trans, int cmd,
				    void* param)
{
	 __ASSERT_NO_MSG(trans);

	 if (trans->api->ioctl) {
		 return trans->api->ioctl(trans, cmd, param);
	 }

	return 0;
}

static inline int ota_trans_read(struct ota_trans *trans, int offset,
				   void *buf, int size)
{
	 __ASSERT_NO_MSG(trans);

	return trans->api->read(trans, offset, buf, size);
}

static inline int ota_trans_open(struct ota_trans *trans)
{
	 __ASSERT_NO_MSG(trans);

	return trans->api->open(trans);
}

static inline int ota_trans_close(struct ota_trans *trans)
{
	 __ASSERT_NO_MSG(trans);

	return trans->api->close(trans);
}

static inline void ota_trans_exit(struct ota_trans *trans)
{
	 __ASSERT_NO_MSG(trans);
	 
	 return trans->api->exit(trans);
}

static inline int ota_trans_init(struct ota_trans *trans, int type,
				   struct ota_trans_api *api,
				   ota_trans_notify_cb_t cb)
{
	 __ASSERT_NO_MSG(trans);

	trans->type = type;
	trans->api = api;
	trans->cb = cb;

	return 0;
}

#endif /* __OTA_TRANS_H__ */
