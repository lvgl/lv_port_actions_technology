#ifndef CUSTOM_APP_UART_COMMON_HANDLER_H_
#define CUSTOM_APP_UART_COMMON_HANDLER_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>


#define TUYA_BLE_UART_COMMON_HEART_MSG_TYPE       	        0x00
#define TUYA_BLE_UART_COMMON_SEARCH_PID_TYPE      	        0x01
#define TUYA_BLE_UART_COMMON_CK_MCU_TYPE          	        0x02
#define TUYA_BLE_UART_COMMON_REPORT_WORK_STATE_TYPE  	    0x03
#define TUYA_BLE_UART_COMMON_RESET_TYPE             	    0x04
#define TUYA_BLE_UART_COMMON_SEND_CMD_TYPE           	    0x06
#define TUYA_BLE_UART_COMMON_SEND_STATUS_TYPE        	    0x07
#define TUYA_BLE_UART_COMMON_QUERY_STATUS            	    0x08
#define TUYA_BLE_UART_COMMON_MODULE_UNBOUND           	    0x09
#define TUYA_BLE_UART_COMMON_RF_TEST               	        0x0E
#define TUYA_BLE_UART_COMMON_SEND_STORAGE_TYPE              0xE0
#define TUYA_BLE_UART_COMMON_SEND_TIME_SYNC_TYPE            0xE1
#define TUYA_BLE_UART_COMMON_MODIFY_ADV_INTERVAL		    0xE2
#define TUYA_BLE_UART_COMMON_TURNOFF_SYSTEM_TIME		    0xE4
#define TUYA_BLE_UART_COMMON_ENANBLE_LOWER_POWER		    0xE5
#define TUYA_BLE_UART_COMMON_SEND_ONE_TIME_PASSWORD_TOKEN   0xE6
#define TUYA_BLE_UART_COMMON_ACTIVE_DISCONNECT			    0xE7

#define TUYA_BLE_UART_COMMON_QUERY_MCU_VERSION			    0xE8
#define TUYA_BLE_UART_COMMON_MCU_SEND_VERSION			    0xE9

//#define TUYA_BLE_UART_COMMON_MODIFY_BLE_CONN_INTERVER       
#define TUYA_BLE_UART_COMMON_BLE_OTA_STATUS            	    0xF0



void tuya_ble_custom_app_uart_common_process(uint8_t *p_in_data,uint16_t in_len);


#ifdef __cplusplus
}
#endif

#endif // 

