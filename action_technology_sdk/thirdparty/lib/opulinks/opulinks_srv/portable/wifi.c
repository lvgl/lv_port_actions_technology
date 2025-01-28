#include <init.h>
#include <stdio.h>
#include <kernel.h>
#include <sys/ring_buffer.h>
#include <board_cfg.h>
#include <file_stream.h>
#include <property_manager.h>
#include <os_common_api.h>
#include "at_command.h"
#include "serial.h"
#include "spi_slave.h"
#include "wifi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define DOWNLOAD_FILE "/NAND:/download.bin"

#define WIFI_MAX_SSID_LENGTH             (33)
#define WIFI_MAC_ADDRESS_LENGTH          (18)
#define WIFI_PASSPHRASE_LENGTH           (65)
#define WIFI_CONNECTED_INFO              "WIFI_INFO"
/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int wifi_data_indication_callback(uint8_t *pu8Payload,
											uint16_t u16PayloadLen);
static int wifi_data_status_change_callback(uint16_t status, uint32_t len);
static int wifi_scan_response_callback(uint8_t *msg, uint16_t msg_len);
static int wifi_connect_status_change_callback(uint16_t status);
/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct wifi_connect_info
{
	uint8_t u8aBssid[WIFI_MAC_ADDRESS_LENGTH];  // The MAC address of the target AP.
	uint8_t u8aSsid[WIFI_MAX_SSID_LENGTH];   // The SSID of the target AP.
	uint8_t u8SsidLen;
	uint8_t u8aPwd[WIFI_PASSPHRASE_LENGTH];     // The password of the target AP.
	uint8_t u8PwdLen;                           // The length of the password. If the length is 64, the password is regarded as PMK.
} wifi_connect_info_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static uint32_t g_wifi_status = WIFI_UNKOWN_STATE;
static uint32_t g_download_status = DOWNLOAD_NOINIT;
static uint32_t g_download_req_size = 0;
static uint32_t g_download_received_size = 0;
static uint32_t g_start_time = 0;
static uint32_t g_finished_time = 0;
static io_stream_t g_download_stream = NULL;

const wifi_ops_callback_t ops_callback = {
	.data_indication = wifi_data_indication_callback,
	.data_status = wifi_data_status_change_callback,
	.scan_response = wifi_scan_response_callback,
	.connect_status = wifi_connect_status_change_callback,
};

static wifi_scan_response_callback_t user_scan_callback;
static wifi_connect_status_change_callback_t user_status_change_callback;
static wifi_connect_info_t g_wifi_connect_info;

extern io_stream_t download_stream_create(const char *param);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void wifi_create_download_file(void)
{
	int ret = 0;

	if (g_download_stream) {
		stream_close(g_download_stream);
		stream_destroy(g_download_stream);
		g_download_stream = NULL;
	}

	io_stream_t file_stream = download_stream_create(DOWNLOAD_FILE);
	if (!file_stream) {
		LOG_ERR("Failed to create file!\n");
		return;
	}

	ret = stream_open(file_stream, MODE_OUT);

	if (ret) {
		LOG_ERR("stream open failed %d\n", ret);
		stream_destroy(file_stream);
		file_stream = NULL;
	}
	g_download_stream = file_stream;
	return;
}

static void wifi_close_download_file(void)
{
	if (g_download_stream) {
		stream_close(g_download_stream);
		stream_destroy(g_download_stream);
		g_download_stream = NULL;
	}
}
static int write_download_size = 0;
static int wifi_data_indication_callback(uint8_t *pu8Payload,
											uint16_t u16PayloadLen)
{
	int try_cnt = 0;
	if (!g_download_received_size) {
		g_start_time = k_uptime_get_32();
		wifi_create_download_file();
	}
retry:
	if (g_download_stream) {
		int ret = stream_write(g_download_stream, pu8Payload, u16PayloadLen);
		if (u16PayloadLen != ret) {
			if (try_cnt ++ < 5){
				os_sleep(2);
				goto retry;
			}
			SYS_LOG_ERR("need write %d bytes but only write %d bytes \n",u16PayloadLen,ret);
		}
		write_download_size += ret;
	}

	g_download_received_size += u16PayloadLen;

	return g_download_received_size;
}

static int wifi_data_status_change_callback(uint16_t status, uint32_t len)
{

	if(status == DOWNLOAD_BEGIN) {
		g_wifi_status = WIFI_DOWNLOADING_STATE;
		if(g_download_status == DOWNLOAD_NOINIT
			|| g_download_status == DOWNLOAD_END) {
			g_download_req_size = len;
			g_download_status = DOWNLOAD_BEGIN;
		}
	} else if(status == DOWNLOAD_END) {
		if(g_download_req_size == g_download_received_size) {
			g_wifi_status = WIFI_DOWNLOAD_FINISHED_STATE;
			g_download_status = DOWNLOAD_END; 
			g_finished_time = os_uptime_get_32();
		}
		wifi_close_download_file();
	} else if(status == DOWNLOAD_FAILED){
		g_wifi_status = WIFI_DOWNLOAD_FAILED_STATE;
		g_finished_time = os_uptime_get_32();
		wifi_close_download_file();
	}

	return 0;
}

static int wifi_scan_response_callback(uint8_t *msg, uint16_t msg_len)
{
	if (strlen(g_wifi_connect_info.u8aBssid) == WIFI_MAC_ADDRESS_LENGTH - 1 
			&& strstr(msg, g_wifi_connect_info.u8aBssid) != NULL) {

		at_command_wifi_connect(g_wifi_connect_info.u8aBssid,
							 g_wifi_connect_info.u8aPwd);
		LOG_INF("wifi auto reconnect to: u8aBssid %s u8aPwd %s \n",
								g_wifi_connect_info.u8aBssid,
								g_wifi_connect_info.u8aPwd);
	}

	if (user_scan_callback) {
		user_scan_callback(msg, msg_len);
	}
	return 0;
}

static int wifi_connect_status_change_callback(uint16_t status)
{
	g_wifi_status = status;

	if(status == WIFI_READY_STATE){
		at_command_wifi_scan();
	}

	if (status == WIFI_CONNECTED_STATE) {
		/* save WiFi info to nvram */
		property_set_factory(WIFI_CONNECTED_INFO, (char *)&g_wifi_connect_info, sizeof(wifi_connect_info_t));
		property_flush(WIFI_CONNECTED_INFO);
	}

	if (user_status_change_callback) {
		user_status_change_callback(status);
	}
	return 0;
}

static int openlink_wifi_init(const struct device *arg)
{
	at_command_init(act_uart_open(1),(wifi_ops_callback_t *)&ops_callback);

	spi_slave_init(&wifi_data_indication_callback);

	memset(&g_wifi_connect_info, 0, sizeof(wifi_connect_info_t));

	property_get(WIFI_CONNECTED_INFO, (char *)&g_wifi_connect_info, sizeof(wifi_connect_info_t));

	return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wifi_get_status(void)
{
	return g_wifi_status;
}

int wifi_get_download_size(void)
{
	return g_download_received_size;
}

int wifi_get_download_status(void)
{
	return g_download_status;
}

int wifi_get_req_size(void)
{
	return g_download_req_size;
}

int wifi_get_download_process(void)
{
	if(wifi_get_status() == WIFI_DOWNLOAD_FINISHED_STATE) {
		return 100;
	}
	if(g_download_req_size != 0) {
		return g_download_received_size * 100 / g_download_req_size;
	}
	return 0;
}

int wifi_get_download_speed(void)
{
	if(g_start_time) {
		if(g_finished_time) {
			return (uint32_t)(((uint64_t)g_download_req_size) * 1000 
				/ (uint64_t)(1 + g_finished_time - g_start_time));
		} else {
			return (uint32_t)(((uint64_t)g_download_received_size * 1000)
				/ (uint64_t)(os_uptime_get_32() - g_start_time));
		}
	} else {
		return 0;
	}
}

int wifi_req_scan(wifi_scan_response_callback_t callback)
{
	user_scan_callback = callback;
	return at_command_wifi_scan();
}

int wifi_req_reconnect(void)
{
	if (strlen(g_wifi_connect_info.u8aBssid) == WIFI_MAC_ADDRESS_LENGTH - 1) {
		return at_command_wifi_connect(g_wifi_connect_info.u8aBssid,
							 g_wifi_connect_info.u8aPwd);
	}
	return 0;
}

int wifi_req_connect(const uint8_t *wifi, const uint8_t *pwd, 
						wifi_connect_status_change_callback_t callback)
{
	user_status_change_callback = callback;

	/*save bssid info*/
	snprintf(g_wifi_connect_info.u8aBssid, sizeof(g_wifi_connect_info.u8aBssid), "%s", wifi);

	/*save pwd info*/
	g_wifi_connect_info.u8PwdLen = strlen(pwd);
	snprintf(g_wifi_connect_info.u8aPwd, sizeof(g_wifi_connect_info.u8aPwd), "%s", pwd);

	LOG_INF("wifi connect to: u8aBssid %s u8aPwd %s \n",
				g_wifi_connect_info.u8aBssid,g_wifi_connect_info.u8aPwd);

	return at_command_wifi_connect(wifi, pwd);
}

int wifi_req_disconnect(void)
{
	return at_command_wifi_disconnect();
}

int wifi_req_http_get(const uint8_t *url)
{
	g_download_received_size = 0;
	g_download_req_size = 0;
	g_download_status = DOWNLOAD_NOINIT;
	fs_unlink(DOWNLOAD_FILE);
	return at_command_http_get(url);
}

int wifi_req_http_stop(void)
{
	return at_command_http_stop();
}

int wifi_req_http_get_resume(const uint8_t *url, int offset, int length)
{
	return at_command_http_get_resume(url, offset, length);
}


int wifi_req_http_post(const uint8_t *url, uint8_t *playload,
										 uint32_t playload_size)
{
	return at_command_http_post(url, playload, playload_size);
}

SYS_INIT(openlink_wifi_init, APPLICATION, 5);
