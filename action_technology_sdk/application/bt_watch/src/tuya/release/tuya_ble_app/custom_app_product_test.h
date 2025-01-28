#ifndef CUSTOM_APP_PRODUCT_TEST_H_
#define CUSTOM_APP_PRODUCT_TEST_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_ble_type.h"

tuya_ble_status_t tuya_ble_prod_beacon_scan_start(void);

tuya_ble_status_t tuya_ble_prod_beacon_scan_stop(void);

tuya_ble_status_t tuya_ble_prod_beacon_get_rssi_avg(int8_t *rssi);

tuya_ble_status_t tuya_ble_prod_gpio_test(void);

void tuya_ble_custom_app_production_test_process(uint8_t channel,uint8_t *p_in_data,uint16_t in_len);


#ifdef __cplusplus
}
#endif

#endif // 

