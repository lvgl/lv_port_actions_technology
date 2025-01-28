#if 1//def CFG_TUYA_BLE_APP_SUPPORT

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tuya_ble_port.h"
#include "tuya_ble_type.h"
//#include "nrf_delay.h"
//#include "nrf_gpio.h"
//#include "ble_advertising.h"
//#include "app_timer.h"
//#include "nrf_drv_pwm.h"
#include "tuya_ble_internal_config.h"
//#include "app_uart.h"
//#include "flash.h"
//#include "app_util_platform.h"
//#include "elog.h"
#include "custom_app_uart_common_handler.h"


void tuya_ble_custom_app_uart_common_process(uint8_t *p_in_data,uint16_t in_len)
{
    uint8_t cmd = p_in_data[3];
    //uint16_t data_len = (p_in_data[4]<<8) + p_in_data[5];
    //uint8_t *data_buffer = p_in_data+6;

    switch(cmd)
    {
    case TUYA_BLE_UART_COMMON_HEART_MSG_TYPE:

        break;

    case TUYA_BLE_UART_COMMON_SEARCH_PID_TYPE:

        break;

    case TUYA_BLE_UART_COMMON_CK_MCU_TYPE:
        
        break;

    case TUYA_BLE_UART_COMMON_QUERY_MCU_VERSION:

        break;

    case TUYA_BLE_UART_COMMON_MCU_SEND_VERSION:

        break;

    case TUYA_BLE_UART_COMMON_REPORT_WORK_STATE_TYPE:

        break;
    case TUYA_BLE_UART_COMMON_RESET_TYPE:

        break;
    case TUYA_BLE_UART_COMMON_SEND_CMD_TYPE:

        break;
    case TUYA_BLE_UART_COMMON_SEND_STATUS_TYPE:

        break;
    case TUYA_BLE_UART_COMMON_QUERY_STATUS:

        break;
    case TUYA_BLE_UART_COMMON_SEND_STORAGE_TYPE:

        break;
    case TUYA_BLE_UART_COMMON_SEND_TIME_SYNC_TYPE:

        break;
    case TUYA_BLE_UART_COMMON_MODIFY_ADV_INTERVAL:

        break;
    case TUYA_BLE_UART_COMMON_TURNOFF_SYSTEM_TIME:

        break;
    case TUYA_BLE_UART_COMMON_ENANBLE_LOWER_POWER:

        break;
    case TUYA_BLE_UART_COMMON_SEND_ONE_TIME_PASSWORD_TOKEN:

        break;
    case TUYA_BLE_UART_COMMON_ACTIVE_DISCONNECT:

        break;
    case TUYA_BLE_UART_COMMON_BLE_OTA_STATUS:

        break;
    default:
        break;
    };

}
#endif
