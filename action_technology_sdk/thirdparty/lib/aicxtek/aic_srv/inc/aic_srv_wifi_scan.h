/******************************************************************************/
/*                                                                            */
/*    Copyright 2022 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_wifi_scan.h
 *
 */

#ifndef __AIC_SRV_WIFI_SCAN_H__
#define __AIC_SRV_WIFI_SCAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "aic_portable.h"
#include "aic_srv_bus.h"

/*********************
 *      DEFINES
 *********************/
#define WIFI_SCAN_MAC_MAX_LEN               (18)
#define WIFI_SCAN_SSID_MAX_LEN              (32)
#define WIFI_SCAN_CHANNEL_MAX_NUM           (13) /* 2.4G wifi channel*/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * struct wfs_scan_ap_t - the ap info of scan result
 * @channel: the channel of the wifi ap, value:1~13.
 * @mac: the mac address string of the wifi ap.
 * @ssid: the ssid string of the wifi ap.
 * @rssi: the signal strenth of the wifi ap.
 * @security: the security type of the wifi ap.
 *
 * wifi ap info.
 */
typedef struct {
    uint8_t  channel;
    uint8_t  mac[WIFI_SCAN_MAC_MAX_LEN];
    uint8_t  ssid[WIFI_SCAN_SSID_MAX_LEN];
    int32_t  rssi;
    uint32_t security;
}wfs_scan_ap_t;

/**
 * struct wfs_scan_result_t - the info of scan result
 * @p_ap_arrayp: the array pointer of the wifi ap.
 * @total_ap_num: the scanned total number of wifi ap.
 * Note: the p_ap_array is already malloced accoding with the total_ap_num,
 *       ande it will be released until next time scan.
 *
 * wifi scan result.
 */
typedef struct {
    /* Currently only 0 and -1, 0: success, -1: fail. */
    int32_t         ret_value;
    uint32_t        total_ap_num;
    uint32_t        current_round;
    wfs_scan_ap_t   *p_ap_array;
}wfs_scan_result_t;

/**
 * struct wfs_scan_result_t - the info of scan result
 * @p_ap_arrayp: the array pointer of the wifi ap.
 * @total_ap_num: the scanned total number of wifi ap.
 * Note: the p_ap_array is already malloced accoding with the total_ap_num,
 *       ande it will be released until next time scan.
 *
 * wifi scan result.
 */
typedef struct {
    /* Currently only 0 and -1, 0: success, -1: fail. */
    int32_t ret_value;
}wfs_scan_stop_resp_t;

/**
 * struct wfs_scan_config_t - wifi scan config info.
 * @scan_channel: the channel index array which need scan.
 *               scan_channel[0]=0: scan all channel
 *               scan_channel[0]=6: scan channel 6
 * @mac: filter ap which match with this mac address.if none, don't filter.
 * @ssid: filter ap which match with this ssid.if none, don't filter.
 * @time_out: scan timeout. range: 10~60 default:10 unit: second
 * @max_ap_num: the max ap number of scan.
 * @round: the scan round, range: 1~3 default:1.
 *
 * wifi scan config info.
 */
typedef struct {
    uint8_t  scan_channel[WIFI_SCAN_CHANNEL_MAX_NUM];    /* range: 1~13 default:scan all*/
    uint8_t  mac[WIFI_SCAN_MAC_MAX_LEN];     /* default: don't filter mac*/
    uint8_t  ssid[WIFI_SCAN_SSID_MAX_LEN];   /* default: don't filter mac*/
    uint32_t time_out;       /* range: 10~60 default:10 unit: second */
    uint32_t max_ap_num;     /* range: 0~100 default:0, report all */
    uint32_t round;          /* range: 1~3 default:1 */
}wfs_scan_config_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*************** The APIs which customer used **********************/

/**
* aic_srv_wifi_scan_start - start wifi scan.
* @p_wifi_scan_cfg: [IN] wifi scan config info.
* @_response_callback: [IN] wifi scan call back to get scan result.
* Return: 0 if success, otherwise -1
* Description: This function is used to start wifi scan.
*/
int32_t aic_srv_wifi_scan_start(wfs_scan_config_t *p_wifi_scan_cfg, srv_func_callback _response_callback);

/**
* aic_srv_wifi_scan_stop - stop wifi scan.
* @wifi_scan_handle: [IN] wifi scan handle when start create.
* @_response_callback: [IN] wifi scan call back to get option result.
* Return: 0 if success, otherwise -1
* Description: This function is used to stop wifi scan.
*/
int32_t aic_srv_wifi_scan_stop(srv_func_callback _response_callback);

/**
* aic_srv_wifi_scan_get_config - get wifi scan config info.
* @p_wifi_scan_cfg: [OUT] out put wifi scan config info.
* Return: 0 if success, otherwise -1
* Description: This function is used to get wifi scan config info.
*/
int32_t aic_srv_wifi_scan_get_config(wfs_scan_config_t *p_wifi_scan_cfg);

/**
* aic_srv_wifi_scan_get_result - return latest wifi scan result.
* @p_wifi_scan_result: [OUT] out put latest wifi scan result.
* Return: 0 if success, otherwise -1
* Description: This function is used to get latest wifi scan result.
*/
int32_t aic_srv_wifi_scan_get_result(wfs_scan_result_t *p_wifi_scan_result);


/*************** The APIs which aic srv module used **********************/

/**
* aic_srv_wifi_scan_init - init wifi scan module.
* Return: 0 if success, otherwise -1
* Description: This function is used init wifi scan.
*/
int32_t aic_srv_wifi_scan_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_WIFI_SCAN_H__ */
