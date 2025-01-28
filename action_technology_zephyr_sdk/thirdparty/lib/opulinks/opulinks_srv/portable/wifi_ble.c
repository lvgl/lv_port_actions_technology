#include <init.h>
#include <stdio.h>
#include <kernel.h>
#include <sys/ring_buffer.h>
#include <board_cfg.h>
#include "at_command.h"
#include "serial.h"
#include "spi_slave.h"
#include "wifi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static io_stream_t g_ble_stream = NULL;
static uint8_t scan_result[1280];
/****************************************************************************
 * Private Functions
 ****************************************************************************/
/*
+WFSCAN:64:6e:97:31:9d:79,Happy Work_2.4G,1,-46,4
+WFSCAN:14:75:90:28:6c:cc,Actions-Hall2,1,-54,3
+WFSCAN:02:79:ca:50:9f:47,ACTZH-SRD-PRD3,6,-38,3
+WFSCAN:ec:26:ca:e2:66:0a,ACTZH-PAD-GMS1,6,-23,4
+WFSCAN:22:f4:4e:2d:e7:ca,ACTZH-PRD3-TB,6,-27,3
OK
*/
static int ble_send_wifi_scan_result(uint8_t *buf, uint16_t len)
{
	char *apinfo = buf;
	uint8_t bssid[6];
	uint8_t ssid_len = 0;
	uint8_t *ssid = NULL;
	uint16_t ap_num = 0;
	int auth_mode;
	int rssi;
	int channel;
	/*resever 2 byts for total length */
	int scan_offset = 0;

	do {
		apinfo = strstr(apinfo, "+WFSCAN:");
		if (!apinfo) {
			break;
		}
		ap_num ++;
		apinfo += strlen("+WFSCAN:");
		sscanf(apinfo, "%x:%x:%x:%x:%x:%x",
				(uint32_t*)&bssid[0],(uint32_t*)&bssid[1],
				(uint32_t*)&bssid[2],(uint32_t*)&bssid[3],
				(uint32_t*)&bssid[4],(uint32_t*)&bssid[5]);

		/*ssid */
		ssid = strchr(apinfo, ',') + 1;
		apinfo = strchr(ssid, ',');
		ssid_len =  (uint32_t)apinfo - (uint32_t)ssid;

		if (scan_offset + ssid_len + 12 >= sizeof(scan_result)) {
			printk("scan buffer overflow  %d max %d \n", scan_offset + ssid_len + 12, sizeof(scan_result));
			break;
		}
		scan_result[scan_offset++] = ((ssid_len + 10) >> 8) & 0xff;
		scan_result[scan_offset++] = (ssid_len + 10) & 0xff;
		scan_result[scan_offset++] = 0x40;
		scan_result[scan_offset++] = 0x00;

		/*ssid length */
		scan_result[scan_offset++] = ssid_len;
		memcpy(&scan_result[scan_offset], ssid, ssid_len);
		scan_offset += ssid_len;

		/*bssid */
		memcpy(&scan_result[scan_offset], bssid, 6);
		scan_offset += 6;

		/*auth mode and rssi */
		sscanf(apinfo, ",%d,%d,%d", &auth_mode,&rssi,&channel);		
		scan_result[scan_offset++] = auth_mode;
		scan_result[scan_offset++] = rssi;
	} while (scan_offset < sizeof(scan_result));
	printk("g_ble_stream  %p scan_offset %d \n",g_ble_stream, scan_offset);
	if (g_ble_stream && scan_offset > 0) {
		static const uint8_t scan_finished[] = {0x40, 0x01, 0x00, 0x04, 0x00, 0x00};
		stream_write(g_ble_stream, scan_result, scan_offset);
		stream_write(g_ble_stream, scan_finished, sizeof(scan_finished));
	}

	g_ble_stream = NULL;
	return 0;
}

static int ble_process_wifi_scan(io_stream_t ble_stream, uint16_t param_size)
{
	int ret = 0;
	uint8_t param[6];
	int rd = stream_read(ble_stream, param, param_size);
	if (rd <= 0) {
		ret = -EIO;
	}
	wifi_req_scan(ble_send_wifi_scan_result);
	g_ble_stream = ble_stream;
	return 0;
}

static int ble_send_wifi_connect_result(uint16_t status)
{
	if(g_ble_stream) {
		if (status == WIFI_CONNECTED_STATE) {
			static const uint8_t connected_finished[] = {0x40, 0x02, 0x00};
			stream_write(g_ble_stream, connected_finished, sizeof(connected_finished));
		} else {
			static const uint8_t connected_finished[] = {0x40, 0x02, 0x01};
			stream_write(g_ble_stream, connected_finished, sizeof(connected_finished));
		}
		g_ble_stream = NULL;
	}
	return 0;
}

static int ble_process_wifi_connect(io_stream_t ble_stream, uint16_t param_size)
{
	int ret = 0;
	uint8_t param[80];
	uint8_t bssid[18] = {0};
	uint8_t psw[64]= {0};

	if (param_size > sizeof(param))	{
		return -EINVAL;
	}

	int rd = stream_read(ble_stream, param, param_size);
	if (rd <= 0) {
		ret = -EIO;
	}

	/* convert bssid */
	snprintf(bssid, sizeof(bssid),"%x:%x:%x:%x:%x:%x", param[0], param[1], param[2], param[3], param[4], param[5]);	

	/* copy psw */
	memcpy(psw, &param[8], param[7]);

	wifi_req_connect(bssid, psw, ble_send_wifi_connect_result);
	g_ble_stream = ble_stream;
	return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int ble_process_wifi_command(uint8_t command_id, io_stream_t ble_stream, uint16_t command_size)
{
	int ret = 0;
	switch (command_id) {
	case 0x00:
		ret = ble_process_wifi_scan(ble_stream, command_size);
		break;
	case 0x01:
		ret = ble_process_wifi_connect(ble_stream, command_size);
		break;
	}
	return ret;	
}