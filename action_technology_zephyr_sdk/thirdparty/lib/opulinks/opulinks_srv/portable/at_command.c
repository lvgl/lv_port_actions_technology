#include <init.h>
#include <stdio.h>
#include <kernel.h>
#include <sys/ring_buffer.h>
#include <board_cfg.h>
#include "serial.h"
#include "at_command.h"
#include "wifi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define AT_COMMAND(cmd) 	"at+"cmd"\r\n"
#define DEMOSTART   		"demostart"
#define WIFISCAN    		"wifiscan"
#define WIFICONNECT 		"wificonnect=%s,%s"
#define WIFIDISCONNECT 		"wifidisconnect"
#define WIFIINFO 			"wifiapinfo"
#define WIFIDEVMAC 			"wifidevmac"
#define WIFISTATUS 			"wifistatus"
#define WIFIAPPROFILE		"wifiapprofile"
#define WIFIHTTPPOST		"httppost=%d,%s"
#define WIFIHTTPGET			"httpget=%d"
#define WIFIHTTPSTOP  		"httpstop"
#define WIFIHTTPGETRESUME	"httpget=%d,%d,%d"
#define SLEEPMODE			"sleepmode"
#define HOSTREADY			"hostready"
#define APPFWVER  			"appfwver"
#define INTNLPREIO  		"intnlprio"


#define RES_MODULE_READY    	"MODULE READY"
#define RES_WIFI_SCAN    		"+WFSCAN"
#define RES_WIFI_CONNECTED  	"WIFI UP"
#define RES_WIFI_DISCONNECTED  	"WIFI DOWN"
#define RES_WIFI_APINFO  		"+WFAPINFO"
#define RES_WIFI_DEVMAC  		"+WFDEVMAC"
#define RES_WIFI_STATUS  		"+WFSTATUS"
#define RES_WIFI_APPPROFILE  	"+WFAPPPROFILE"
#define RES_WIFI_CHN  			"+WIFICHN"
#define RES_WIFI_HTTPPOST	  	"+HTTPPOST"
#define RES_WIFI_HTTPGET	  	"+HTTPGET:"
#define RES_WIFI_APPROFILE  	"+WFAPPPROFILE"
#define RES_WIFI_SWAKEUP  		"SMART SLEEP WAKEUP"
#define RES_WIFI_TWAKEUP  		"TIMER SLEEP WAKEUP"
#define RES_WIFI_FWVER		  	"+FWVER"
#define RES_WIFI_INTNELPRIO  	"+INTNLPRIO"


/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

typedef int (*at_cmd_res_handler_t)(char *buf, int len);

static int AT_CmdRes_ModuleReady(char *buf, int len);
static int AT_CmdRes_WifiScan(char *buf, int len);
static int AT_CmdRes_WifiConnected(char *buf, int len);
static int AT_CmdRes_WifiDisConnected(char *buf, int len);
static int AT_CmdRes_ApInfo(char *buf, int len);
static int AT_CmdRes_DevMac(char *buf, int len);
static int AT_CmdRes_Status(char *buf, int len);
static int AT_CmdRes_AppProfile(char *buf, int len);
static int AT_CmdRes_Channel(char *buf, int len);
static int AT_CmdRes_HttpPost(char *buf, int len);
static int AT_CmdRes_HttpGet(char *buf, int len);
static int AT_CmdRes_SmartWakeUp(char *buf, int len);
static int AT_CmdRes_TimerWakeUp(char *buf, int len);
static int AT_CmdRes_FwVer(char *buf, int len);
static int AT_CmdRes_IntnelPrio(char *buf, int len);

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct at_command_response {
    const char *cmd;
    at_cmd_res_handler_t cmd_handle;
}at_command_response_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static void* g_uart_handle;

static uint8_t at_command_buffer[1536];
static wifi_ops_callback_t *g_ops_callback;

const at_command_response_t g_AtCmdResponse[] =
{
    {RES_MODULE_READY,    	AT_CmdRes_ModuleReady},
    {RES_WIFI_SCAN,    		AT_CmdRes_WifiScan},
    {RES_WIFI_CONNECTED,    AT_CmdRes_WifiConnected},
    {RES_WIFI_DISCONNECTED, AT_CmdRes_WifiDisConnected},
    {RES_WIFI_APINFO,    	AT_CmdRes_ApInfo},
    {RES_WIFI_DEVMAC,    	AT_CmdRes_DevMac},
    {RES_WIFI_STATUS,    	AT_CmdRes_Status},
    {RES_WIFI_APPPROFILE,   AT_CmdRes_AppProfile},
    {RES_WIFI_CHN,    		AT_CmdRes_Channel},
    {RES_WIFI_HTTPPOST,    	AT_CmdRes_HttpPost},
    {RES_WIFI_HTTPGET,    	AT_CmdRes_HttpGet},
    {RES_WIFI_SWAKEUP,    	AT_CmdRes_SmartWakeUp},
    {RES_WIFI_TWAKEUP,    	AT_CmdRes_TimerWakeUp},
    {RES_WIFI_FWVER,    	AT_CmdRes_FwVer},
    {RES_WIFI_INTNELPRIO,   AT_CmdRes_IntnelPrio},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int AT_Cmd_Send(const uint8_t *command)
{
	int len = snprintf(at_command_buffer, sizeof(at_command_buffer), AT_COMMAND("%s"), command);
	printk("%s \n",at_command_buffer);
	return act_uart_write(g_uart_handle, at_command_buffer, len);
}

static int AT_CmdRes_ModuleReady(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	if (g_ops_callback && g_ops_callback->connect_status) {
		g_ops_callback->connect_status(WIFI_READY_STATE);
	}

	return 0;
}
#if 0
uint8_t cont_buffer[] = "+WFSCAN:a0:36:bc:5e:23:c0,ASUS_RT_AX56U,1,-46,0"\
"+WFSCAN:d0:17:c2:61:cf:e0,Internal-meeting-room,1,-58,3"\
"+WFSCAN:78:8c:b5:94:62:66,SVT_TP-Link_Deco_X50,2,-16,0"\
"+WFSCAN:7e:8c:b5:94:62:66,,2,-23,3"\
"+WFSCAN:74:05:a5:86:58:05,SVT_TP-LINK_5805,3,-26,4"\
"+WFSCAN:a4:2b:b0:c7:16:a5,AE_TP1,3,-39,0"\
"+WFSCAN:48:7d:2e:fa:f3:2e,Netlink-B,3,-38,4"\
"+WFSCAN:30:ae:a4:80:05:35,Opulinks-ESP32,4,-43,4"\
"+WFSCAN:f0:a7:31:a2:c5:3c,TP-Link_WR840N,6,-46,0"\
"+WFSCAN:24:2f:d0:50:be:41,TP-LINK_Power Strip_BE41,6,-53,0"\
"+WFSCAN:24:4b:fe:18:e7:34,Netlink-IT,6,-45,3"\
"+WFSCAN:62:22:32:1f:fe:4e,,6,-85,3"\
"+WFSCAN:b0:6e:bf:78:f1:18,ASUS_18,7,-26,3"\
"+WFSCAN:f4:8c:eb:8e:5f:e0,SVT_dlink-5FDE,8,-33,6"\
"+WFSCAN:62:bb:d5:56:6a:8d,AE_HUAWEI_WS7100,9,-21,0"\
"+WFSCAN:30:b4:9e:71:a7:a4,TP-LINK_71A7A4,9,-67,4"\
"+WFSCAN:2c:56:dc:8b:b3:80,ASUS_Pico,10,-44,3"\
"+WFSCAN:64:70:02:5d:93:05,dd-wrt,10,-38,0"\
"+WFSCAN:28:6c:07:5e:ea:2a,AE_XIAOMI_MIR3,10,-12,0"\
"+WFSCAN:fa:d0:27:3d:70:73,DIRECT-273DF073,10,-64,3"\
"+WFSCAN:88:1f:a1:33:7a:20,AE_APPLE_AIRPORT,11,-47,0"\
"+WFSCAN:7c:8b:ca:61:86:88,SVT_TL_WR841N,11,-39,4"\
"+WFSCAN:7c:10:c9:38:66:a8,H GROUP_2G,11,-56,3"\
"+WFSCAN:02:16:bc:1a:7a:e1,SVT_Huawei_WS8700,11,-49,3"\
"+WFSCAN:1c:49:7b:9a:14:06,CHT_I040GW1,11,-51,3"\
"OK";
#endif
static int AT_CmdRes_WifiScan(char *buf, int len)
{
	printk("%s \n",buf);
	if (g_ops_callback && g_ops_callback->scan_response) {
		g_ops_callback->scan_response(buf, len);
	}
	return 0;	
}

static int AT_CmdRes_WifiConnected(char *buf, int len)
{
	if (g_ops_callback && g_ops_callback->connect_status) {
		g_ops_callback->connect_status(WIFI_CONNECTED_STATE);
	}

	return 0;
}

static int AT_CmdRes_WifiDisConnected(char *buf, int len)
{
	if (g_ops_callback && g_ops_callback->connect_status) {
		g_ops_callback->connect_status(WIFI_DISCONNECTED_STATE);
	}

	return 0;
}

static int AT_CmdRes_ApInfo(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}
static int AT_CmdRes_DevMac(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}
static int AT_CmdRes_Status(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}
static int AT_CmdRes_AppProfile(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}
static int AT_CmdRes_Channel(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}
static int AT_CmdRes_HttpPost(char *buf, int len)
{
	return 0;
}

static int AT_CmdRes_HttpGet(char *buf, int len)
{
	int download_state;
	int high_speed;
	int file_len;
	int playload_len;

	buf += strlen(RES_WIFI_HTTPGET);

	sscanf(buf, "%d,%d,%d,%d",
				(uint32_t*)&download_state,(uint32_t*)&high_speed,
				(uint32_t*)&file_len,(uint32_t*)&playload_len);

	if (strstr(buf, "ERROR") != NULL) {
		download_state = DOWNLOAD_FAILED;
	}

	if (g_ops_callback && g_ops_callback->data_status) {
		g_ops_callback->data_status(download_state, file_len);
	}

	return 0;
}

static int AT_CmdRes_SmartWakeUp(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}

static int AT_CmdRes_TimerWakeUp(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}

static int AT_CmdRes_FwVer(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}

static int AT_CmdRes_IntnelPrio(char *buf, int len)
{
	printk("%s : %s \n",__FUNCTION__, buf);
	return 0;
}

int at_command_response_process(uint8_t *command_data, uint16_t command_size)
{
	int ret = 0;

	for (int i = 0; i < ARRAY_SIZE(g_AtCmdResponse); i++){
		if (strncmp(command_data, g_AtCmdResponse[i].cmd, strlen(g_AtCmdResponse[i].cmd)) == 0) {
			command_data[command_size] = 0;
			ret = g_AtCmdResponse[i].cmd_handle(command_data, command_size);
			break;
		}
	}

	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int at_command_wifi_scan(void)
{
	return AT_Cmd_Send(WIFISCAN);
}

int at_command_wifi_connect(const uint8_t *wifi, const uint8_t *pwd)
{
	uint8_t command[58];
	snprintf(command, sizeof(command), WIFICONNECT, wifi, pwd);
	return AT_Cmd_Send(command);
}

int at_command_wifi_disconnect(void)
{
	return AT_Cmd_Send(WIFIDISCONNECT);
}

int at_command_http_get(const uint8_t *url)
{
	int ret = 0;
	uint8_t command[16];
	snprintf(command, sizeof(command), WIFIHTTPGET, strlen(url));
	ret = AT_Cmd_Send(command);
	if (ret) {
		k_sleep(K_MSEC(4));
		ret = snprintf(at_command_buffer, sizeof(at_command_buffer), "%s\r\n", url);
		act_uart_write(g_uart_handle, at_command_buffer, ret);
	}
	return ret;
}

int at_command_http_stop(void)
{
	return AT_Cmd_Send(WIFIHTTPSTOP);
}
int at_command_http_get_resume(const uint8_t *url, int offset, int length)
{
	int ret = 0;
	uint8_t command[32];
	snprintf(command, sizeof(command), WIFIHTTPGETRESUME, strlen(url), offset, length);
	ret = AT_Cmd_Send(command);
	if (ret) {
		k_sleep(K_MSEC(4));
		ret = snprintf(at_command_buffer, sizeof(at_command_buffer), "%s\r\n", url);
		act_uart_write(g_uart_handle, at_command_buffer, ret);
	}
	return ret;
}


int at_command_http_post(const uint8_t *url, uint8_t *playload, uint32_t playload_size)
{
	int ret = 0;
	uint8_t command[16];
	snprintf(command, sizeof(command), WIFIHTTPPOST, playload_size, url);
	ret = AT_Cmd_Send(command);
	if (ret) {
		k_sleep(K_MSEC(4));
		ret = snprintf(at_command_buffer, sizeof(at_command_buffer), "%s\r\n", playload);
		act_uart_write(g_uart_handle, at_command_buffer, ret);
	}
	return ret;
}

int at_command_init(void* uart_handle, wifi_ops_callback_t *ops_callback)
{
	int ret = 0;

	if (!uart_handle) {
		return -ENODEV;
	}

	g_uart_handle = uart_handle;

	g_ops_callback = ops_callback;

	act_uart_register_data_callback(at_command_response_process);

	return ret;
}


