/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file property manager interface
 */

#ifndef __PROPERTY_MANAGER_H__
#define __PROPERTY_MANAGER_H__
#define CFG_SN_INFO 			  	"SN_NUM"
#define CFG_MQTT_SERVER_ADDR_INFO 	"MQTT_SRV_ADDR"
#define CFG_MQTT_SERVER_PORT		"MQTT_SRV_PORT"
#define CFG_MQTT_TOPIC_HEAD		    "MQTT_TOPIC_HEAD"
#define CFG_WECHAT_SERVER_ADDR_INFO "WECHAT_SRV_ADDR"
#define CFG_WECHAT_SRV_URL          "WECHAT_SRV_URL"
#define CFG_SOFTAP_INFO   			"SOFTAP_INFO"
#define CFG_FW_VERSION 				"FW_VERION"
#define CFG_AUTO_POWERDOWN 			"AUTO_POWERDOWN"
#define CFG_AUTO_RECONNECT 			"AUTO_RECONNECT"
#define CFG_AUTOCONN_INFO			"BT_AUTOCONN_INFO"
#define CFG_OTA_FLAG 				"OTA_FLAG"
#define CFG_SOFTAP_INFO   		  	"SOFTAP_INFO"
#define CFG_FW_VERSION 			  	"FW_VERION"
#define CFG_MAX_SILENT_TIME  	    "MAX_SILIENT_TIME"
#define CFG_SHUTDOWN_POWER_LEVEL 	"NOPOWER_LEVEL"
#define CFG_LOW_POWER_WARNING_LEVEL	"LOWPOWER_LEVEL"
#define CFG_BT_MAC                  "BT_MAC"
#define CFG_BT_NAME                 "BT_NAME"
#define CFG_BLE_NAME                "BT_LE_NAME"
#define CFG_BLE_AUDIO               "BT_LE_AUDIO"
#define CFG_BT_PRE_NAME             "BT_PRE_NAME"
#define CFG_BT_ID                   "BT_ID"
#define CFG_BT_IRK                  "BT_IRK"
#define CFG_BT_KEYS                 "BT_KEYS"
#define CFG_BT_HASH                 "BT_HASH"
#define CFG_BT_CCC                  "BT_CCC"
#define CFG_BT_SC                   "BT_SC"
#define CFG_BT_CF                   "BT_CF"

#define CFG_BT_CFO_VAL              "BT_CFO_VAL"
#define CFG_BT_FREQ_CAL             "BT_FREQ_CAL"
#define CFG_BT_CLBR_PARAL           "BT_CLBR_PARAL"
#define CFG_BT_RX_TXPOWER           "BT_RF_TXPOWER"
#define CFG_BT_TEST_MODE            "BT_TEST_MODE"
#define CFG_AUDIO_OUT_VOLUME        "AUDIOUT_VOLUME"
#define CFG_AUDIO_OUT_CHANNEL       "AUDIOUT_CHANNEL"
#define CFG_HOLDABLE_KEY            "HOLDABLE_KEY"

#define CFG_SAIR_SERVER 		  	"SAIR_SERVER"
#define CFG_SAIR_URL 			  	"SAIR_URL"
#define CFG_SAIR_TOKEN_URL 		  	"SAIR_TOKEN_URL"
#define CFG_SAIR_AGENT_ID		  	"SAIR_AGENT_ID"
#define CFG_SAIR_CLIENT_ID		  	"SAIR_CLIENT_ID"
#define CFG_SAIR_TOKEN		  	    "SAIR_TOKEN"
#define CFG_AI_CUSTOMER_ID 			"ai_customer"

#define CFG_ALARM_INFO   			"ALARM_INFO"

#define CFG_MIC_GAIN               	"MIC_GAIN"
#define CFG_MIC_GAIN_BOOST         	"MIC_GAIN_BOOST"
#define CFG_LINEIN_GAIN            	"LINEIN_GAIN"
#define CFG_ADC_GAIN               	"ADC_GAIN"
#define CFG_DEC_KEY		  		   	"DEC_KEY"

#define CFG_TONE_VOLUME            	"TONE_VOLUME"
#define CFG_TTS_MIN_VOL          	"TTS_MIN_VOLUME"
#define CFG_TTS_MAX_VOL          	"TTS_MAX_VOLUME"
#define CFG_BTPLAY_VOLUME          	"BTPLAY_VOLUME"
#define CFG_BTCALL_VOLUME          	"BTCALL_VOLUME"
#define CFG_LINEIN_VOLUME          	"LINEIN_VOLUME"
#define CFG_MIC_IN_VOLUME          	"MIC_IN_VOLUME"
#define CFG_SPDIF_IN_VOLUME          	"SPDIF_IN_VOLUME"
#define CFG_FM_VOLUME          		"FM_VOLUME"
#define CFG_I2SRX_IN_VOLUME         "I2SRX_IN_VOLUME"
#define CFG_LOCALPLAY_VOLUME       	"LOCALPLAY_VOLUME"
#define CFG_USOUND_VOLUME           "USOUND_VOLUME"
#define CFG_LE_VOLUME               "LE_VOLUME"
#define CFG_BT_PINCODE             	"BT_PINCODE"
#define CFG_SPECTRUM_FREQ		  	"SPECTRUM_FREQ"

#define CFG_AGING_PLAYBACK_INFO     "AGING_PLAYBACK_INFO"
#define CFG_ATT_BAT_INFO            "ATT_BAT_INFO"


/**
 * @defgroup property_manager_apis App property Manager APIs
 * @ingroup system_apis
 * @{
 */

/**
 * @brief Obtaining value of target key
 *
 * @details get value by the target key, value format is string .
 *
 * @param key string point of key
 * @param value point of return value
 * @param value_len  max length of value
 *
 * @return <= 0  get failed
 * @return >0    string length of value
 */
int property_get(const char *key, char *value, int value_len);


/**
 * @brief set value of target key
 *
 * @details set value by the target key, value format is string .
 *
 * @param key string point of key
 * @param value point of return value
 * @param value_len  max length of value
 *
 * @return != 0  set failed
 * @return == 0  set success
 */
int property_set(const char *key, char *value, int value_len);

/**
 * @brief set value of target key
 *
 * @details set value by the target key, value format is string, this property store in factory area .
 *  will not earase when brun fimrware.
 *
 * @param key string point of key
 * @param value point of return value
 * @param value_len  max length of value
 *
 * @return != 0  set failed
 * @return == 0  set success
 */
int property_set_factory(const char *key, char *value, int value_len);

/**
 * @brief Obtaining value of target key
 *
 * @details get value by the target key, value format is int .
 *
 * @param key string point of key
 * @param default_value default value if not found the key of target value
 *
 * @return < 0  get failed
 * @return others int format value of target key
 */

int property_get_int(const char *key, int default_value);

/**
 * @brief set value of target key
 *
 * @details set value by the target key, value format is int .
 *
 * @param key string point of key
 * @param value int type value want to set
 *
 * @return != 0  set failed
 * @return == 0  set success
 */

int property_set_int(const char *key, int value);

/**
 * @brief set value of target key
 *
 * @details set value by the target key, value format is int .
 *
 * @param key string point of key
 * @param value int type value want to set
 *
 * @return != 0  set failed
 * @return == 0  set success
 */

int property_get_byte_array(const char *key, char *value, int value_len, const char *default_value);

/**
 * @brief set value of target key
 *
 * @details set value by the target key, value format is int .
 *
 * @param key string point of key
 * @param value int type value want to set
 *
 * @return != 0  set failed
 * @return == 0  set success
 */

int property_set_byte_array(const char *key, char *value, int value_len);

/**
 * @brief get value of target key
 *
 * @details get value by the target key, value format is int array.
 *
 * @param key string point of key
 * @param value int type value want to set
 * @param value_len length of int array
 * @param default_value default value if not found the key of target value
 *
 * @return != 0  set failed
 * @return == 0  set success
 */

int property_get_int_array(const char *key, int *value, int value_len,  const short *default_value);

/**
 * @brief set value of target key
 *
 * @details set value by the target key, value format is int array .
 *
 * @param key string point of key
 * @param value int array type value want to set
 *
 * @return != 0  set failed
 * @return == 0  set success
 */

int property_set_int_array(const char *key, int *value, int value_len);

/**
 * @brief flush all property cache
 *
 * @details  This routine flush property cache if property cahce enabled
 * if key is NULL means flush all property in cache, otherwise flush target
 * key property
 *
 * @param key target proptery want to flush
 *
 * @return != 0  set failed
 * @return == 0  set success
 */

int property_flush(const char *key);

/**
 * @brief requstion flush all property cache
 *
 * @details  This routine requstion flush property cache if property cahce enabled
 * if key is NULL means flush all property in cache, otherwise flush target
 * key property, not flush property immediately, system will Come to flush at the right time
 *
 * @param key target proptery want to flush
 *
 * @return != 0  set failed
 * @return == 0  set success
 */

int property_flush_req(const char *key);

/**
 * @brief init property manager
 *
 * @details This routine init property cache if enable property cache
 *
 * @return != 0  init failed
 * @return == 0  init success
 */

int property_manager_init(void);

/**
 * @} end defgroup property_manager_apis
 */

int property_flush_req_deal(void);

#endif
