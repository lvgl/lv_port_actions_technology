#ifndef TUYA_BLE_APP_DEMO_H_
#define TUYA_BLE_APP_DEMO_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TUYA_NET_CONFIG_READY = 1,
    TUYA_ALEXA_AUTH_PASS,
    TUYA_VOS_TIMER_SET,
    TUYA_VOS_ALARM_SET,
	TUYA_VOS_ALEXA_TTS,
	TUYA_VOS_INCOMING_NOTIFY,
} tuya_vos_tone_play_t;

#define APP_PRODUCT_ID          "4fiybcor"
// #define APP_PRODUCT_ID          "ziodqisw" //OTA test
// #define APP_PRODUCT_ID          "mouhvzie" // VOS test

#define APP_BUILD_FIRMNAME      "tuya_ble_sdk_app_demo_ats301x"  

// firmware version
#define TY_APP_VER_NUM       0x0200
#define TY_APP_VER_STR	     "2.0" 	

//hardware version
#define TY_HARD_VER_NUM      0x0200
#define TY_HARD_VER_STR	     "2.0" 	

void tuya_ble_app_init(void);

void  tuya_nv_sync(u8_t* buf, u16_t len);

void  tuya_role_switch_access(u8_t* buf, u32_t len);
void tuya_ota_proc(uint16_t cmd,uint8_t*recv_data,uint32_t recv_len);
void tuya_ota_resource_release(void);
void tuya_ble_unbond(void);
void tuya_start_tone_play(u32_t type);
void tuya_find_device_start_notify(void);
void tuya_find_device_stop_notify(void);
void tuya_resource_alloc(void);
void tuya_resource_release(void);
void tuya_dev_record_stop(void);
void tuya_dev_record_start(void);

#ifdef __cplusplus
}
#endif

#endif // 







