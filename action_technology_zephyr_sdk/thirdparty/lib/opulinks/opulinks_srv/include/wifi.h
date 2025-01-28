#ifndef __PORTABLE_INCLUDE_WIFI_H__
#define __PORTABLE_INCLUDE_WIFI_H__
#include <stream.h>

typedef enum wifi_type
{
	WIFI_UNKOWN_STATE,
	WIFI_READY_STATE,
	WIFI_CONNECTED_STATE,
	WIFI_DOWNLOADING_STATE,
	WIFI_DOWNLOAD_FINISHED_STATE,
	WIFI_DOWNLOAD_FAILED_STATE,
	WIFI_DISCONNECTED_STATE,
} wifi_status_t;

typedef enum data_status
{
	DOWNLOAD_BEGIN,
	DOWNLOAD_PROCESS,
	DOWNLOAD_END,
	DOWNLOAD_FAILED,
	DOWNLOAD_NOINIT,
} data_status_t;


typedef int (*wifi_data_indication_callback_t)(uint8_t *pu8Payload, uint16_t u16PayloadLen);
typedef int (*wifi_data_status_change_callback_t)(uint16_t status, uint32_t len);
typedef int (*wifi_scan_response_callback_t)(uint8_t *buf, uint16_t len);
typedef int (*wifi_connect_status_change_callback_t)(uint16_t status);

typedef struct wifi_ops_callback {
	wifi_data_indication_callback_t data_indication;
	wifi_data_status_change_callback_t data_status;
	wifi_scan_response_callback_t scan_response;
	wifi_connect_status_change_callback_t connect_status;
} wifi_ops_callback_t;

int wifi_get_status(void);
int wifi_set_reveived_data(int size);
int wifi_get_download_process(void);
int wifi_get_download_speed(void);
int wifi_get_download_status(void);
int wifi_req_download(void);
int wifi_get_download_size(void);
int wifi_get_req_size(void);
int wifi_req_scan(wifi_scan_response_callback_t callback);
int wifi_req_disconnect(void);
int wifi_req_connect(const uint8_t *wifi, const uint8_t *pwd, wifi_connect_status_change_callback_t callback);
int wifi_req_reconnect(void);

int wifi_req_http_get(const uint8_t *url);
int wifi_req_http_stop(void);
int wifi_req_http_get_resume(const uint8_t *url, int offset, int length);
int wifi_req_http_post(const uint8_t *url, uint8_t *playload, uint32_t playload_size);


int ble_process_wifi_command(uint8_t command_id, io_stream_t ble_stream, uint16_t command_size);

#endif /* __PORTABLE_INCLUDE_WIFI_H__ */