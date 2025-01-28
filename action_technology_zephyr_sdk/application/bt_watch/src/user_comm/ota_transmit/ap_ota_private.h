/* Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 */

#ifndef __AP_OTA_PRIVATE__
#define __AP_OTA_PRIVATE__
#include <ota_manifest.h>
#include <ota_backend.h>
#include <ota_breakpoint.h>
//#include <ota_image.h>
#include <ota_upgrade.h>

#define OTA_BIN_VALID_NUM_MAX 5
#define OTA_BIN_VALID_NUM_MIN 3
#define TWS_OTA_BREAKPOINT_LEN	256
#define TWS_OTA_BREAKPOINT_MAGIC (0x43215678)


enum TWS_OTA_STATUS
{
	OTA_STATUS_NOUSE	= 0x00,
	OTA_STATUS_READY	= 0x01,
	OTA_STATUS_HALT 	= 0x04,
	OTA_STATUS_FW_CHECK   = 0x05,
	OTA_STATUS_REBOOT	= 0x07,
	OTA_STATUS_INQUIRE = 0x08,
	
	OTA_STATUS_BK_SYNC = 0x0E,
	OTA_STATUS_VALUE_MAX = 0x0F,
};

#define OTA_FW_HDR_MAGIC	0x41544f41 //'AOTA'

struct ota_fw_hdr {
	uint32_t	magic;
	uint32_t	header_checksum;
	uint16_t	header_version;
	uint16_t	header_size;
	uint16_t	file_cnt;
	uint16_t	flag;
	uint16_t	dir_offset;
	uint16_t	data_offset;
	uint32_t	data_size;
	uint32_t	data_checksum;
	uint8_t	reserved[4];
} __attribute__((packed));

/* size 0x60 */
struct ota_fw_ver {
	uint8_t version_name[32];
	uint8_t board_name[32];
	uint8_t hardware_ver[4];
	uint32_t version_code;
	uint8_t reserved[8];
	uint8_t build_time[16];
} __attribute__((packed));

struct ota_dir_entry {
	uint8_t filename[12];
	uint8_t reserved1[4];
	uint32_t offset;
	uint32_t length;
	uint8_t reserved2[4];
	uint32_t checksum;
} __attribute__((packed));

struct ota_fw_dir {
	struct ota_dir_entry	entrys[16];
};

struct ota_fw_head {
	/* offset: 0x0 */
	struct ota_fw_hdr	hdr;

	/* offset: 0x20 */
	uint8_t			reserved1[32];

	/* offset: 0x40, new OTA firmware version */
	struct ota_fw_ver	new_ver;

	/* offset: 0xa0, old ota firmware version
	 * only used for ota patch firmware
	 */
	struct ota_fw_ver	old_ver;

	/* offset: 0x100 */
	uint8_t			reserved2[256];

	/* offset: 0x200, OTA file dir entries */
	struct ota_fw_dir	dir;
} __attribute__((packed));

struct ota_progress {
#define OTA_PROGRESS_UP_LIMIT (99)
	uint32_t total_size;
	uint32_t cur_size;
	uint8_t cursor;
	uint8_t on;
};

struct ota_image {
	struct ota_backend *backend;
	struct ota_fw_hdr hdr;
	struct ota_fw_dir dir;
	struct ota_dir_entry *cur_dir;
	struct ota_fw_head *fw_head;
	struct ota_progress progress;
};

struct ota_upgrade_info {
	int state;
	int backend_type;
	unsigned int flags;

	ota_notify_t notify;

	struct ota_image *img;
	struct ota_storage *storage;

	int data_buf_size;
	uint8_t *data_buf;
	uint32_t xml_offset;
	struct ota_manifest manifest;
	struct ota_breakpoint bp;
};


struct ota_backend_apota_init_param {
	int upload_type;
};

struct apota_stream_init_param {
	u32_t read_timeout;
	u32_t write_timeout;
	void *connect_cb;
};

typedef struct
{
	// 勿调换结构体成员位置，增加结构体成员需谨慎
	//u8_t resume[TWS_OTA_BREAKPOINT_LEN];
	u32_t fw_size;
	u32_t fw_ver;
	u32_t cur_fn;
	u32_t bk_checksum;
	u32_t unique_id;
	u8_t pn;
	u8_t status;
	u8_t ota_rst;
	
} __attribute__((packed)) tws_ota_sync_info_t;

typedef struct {
	uint8_t filename[12];
	uint32_t offset;
	uint32_t length;
	uint32_t checksum;
} __attribute__((packed)) ota_offset_entry;

typedef struct
{
	u32_t		   wflag; // MAGIC FALG, 不要更换位置
	u32_t		   unique_id;
	//ota_offset_entry entry[OTA_BIN_VALID_NUM_MAX];
	struct ota_breakpoint image_bk;
	//u32_t		new_version;
	u32_t		checksum;
} __attribute__((packed)) ota_rs_breakponit_t;


struct ota_backend *ota_backend_apota_init(ota_backend_notify_cb_t cb,
		struct ota_backend_apota_init_param *param);

int ota_backend_apota_write_stream(struct ota_backend *backend, uint8_t *buf, int length);

//int ota_backend_apota_start(struct ota_backend *backend);
//int ota_backend_apota_stop(struct ota_backend *backend);
int ota_backend_apota_reply(struct ota_backend *backend);
io_stream_t apota_stream_create(void *param);
int ota_backend_apota_get_offset(void);
void apota_stream_connected(u32_t len, u32_t offset);
void apota_stream_disconnected(void);
u16_t ota_ap_head_xml_get(ota_rs_breakponit_t *breakpoint, u8_t *ptr);
extern int ota_app_init(void);
extern struct ota_upgrade_info *ota_app_info_get(void);

#endif /* __AP_OTA_PRIVATE__*/

