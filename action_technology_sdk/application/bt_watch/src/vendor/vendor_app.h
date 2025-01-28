#ifndef __VENDOR_APP_H__
#define __VENDOR_APP_H__

#include <errno.h>
#include <drivers/rtc.h>
#include <os_common_api.h>

#define BLE_READ_CONTINUE 1
#define BLE_READ_STOP	  0
#define SVC_WATCH 0x01

#define TLV_PARAMS_LENGTH 0x80
#define TLV_TYPE_COMPLETE 0x81

#define	VS_CMD_FAILED  0x00
#define VS_CMD_SUCCEED 0x01

#define VS_NAME_LEN 24
#define VS_DATE_LEN 24
#define VS_TEXT_LEN 24

struct ble_stream_init_param {
	const uint8_t *spp_uuid;
	void *gatt_attr;
	uint8_t attr_size;
	void *tx_chrc_attr;
	void *tx_attr;
	void *tx_ccc_attr;
	void *rx_attr;
    void *connect_cb;
	int32_t read_timeout;
	int32_t write_timeout;
};

enum command_id {
	CMD_H2D_SET_TIME = 0x01,
	CMD_H2D_SEND_FILE,
	CMD_H2D_SEND_MSG,
	CMD_D2H_RESULT,
};

enum {
	TLV_DATE_TIME = 0x01,
};

enum {
	TLV_FILE_NAME = 0x01,
	TLV_FILE_TYPE,
	TLV_FILE_DATE,
	TLV_FILE_LENGTH,
};

enum {
	TLV_MSG_APP_NAME = 0x01,
	TLV_MSG_TITLE,
	TLV_MSG_TEXT,
	TLV_MSG_DATE,
};

enum {
	TLV_RESULT_CMD = 0x01,
	TLV_RESULT_CODE,
};

struct vs_date_time {
	/* valid range 1582 to 9999 */
	uint16_t year;
	/* valid range 1 to 12 */
	uint8_t  month;
	/* valid range 1 to 31 */
	uint8_t  day;
	/* valid range 0 to 23 */
	uint8_t  hours;
	/* valid range 0 to 59 */
	uint8_t  minutes;
	/* valid range 0 to 59 */
	uint8_t  seconds;
	/* valid range 1 to 7 */
	uint8_t  day_of_week;
} __packed;

struct vs_file_info {
	char 	 name[24];
	char	 path[48];
	uint8_t  type;
	char 	 date[24];	/* yyyyMMdd HHmmSS */
	uint32_t length;
	uint8_t  buf[256];
};

struct vs_msg_info {
	char app_name[24];
	char title[24];
	char text[24];
	char date[24];
};

struct vs_cmd_result {
	uint8_t cmd_id;
	uint8_t result;
};

typedef struct {
	os_tid_t read_tid;
    uint16_t stream_ready : 1;
	uint16_t file_recv : 1;
	uint16_t msg_recv : 1;
	const struct device *rtc_dev;
	struct rtc_time rtc_time;
    io_stream_t ble_stream;
	io_stream_t file_stream;
	struct vs_date_time time;
	struct vs_file_info *file;
	struct vs_msg_info *msg;
	uint8_t buf[128];
} vendor_app_t;

#endif
