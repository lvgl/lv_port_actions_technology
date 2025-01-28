/******************************************************************************/
/*                                                                            */
/*    Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file wifi_scan.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "aic_srv_wifi_scan.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int32_t  wfs_scan_result_handler(void *p_param, uint32_t size)
{
    u32 i = 0;
    wfs_scan_result_t *p_wfs_result = NULL;
    uint32_t total_ap_num = 0;
    printf("[wifi_scan sample]wfs_scan_result_handler \r\n");

    p_wfs_result = (wfs_scan_result_t *)p_param;
    total_ap_num = p_wfs_result->total_ap_num;

    if (-1 == p_wfs_result->ret_value) {
        printf("[wifi_scan sample]wfs_scan fail \r\n");
        return -1;
    }

    if ((NULL == p_wfs_result->p_ap_array) || (0 == total_ap_num)) {
        printf("[wifi_scan sample]wfs_scan success but scan 0 ap info! \r\n");
        return -1;
    }

    for (i = 0; i < total_ap_num; i++ ) {
        /* package scan result at string */
        /* urc eg. +WIFISCAN:1,1,1A:2B:3C:4D:5E:6F,-30,-,-*/
        printf("[wifi_scan sample] channel (%d), mac (%s), rssi (%d) \r\n",p_wfs_result->p_ap_array[i].channel,p_wfs_result->p_ap_array[i].mac,p_wfs_result->p_ap_array[i].rssi);
    }

    return 0;
}

static int32_t  wfs_scan_stop_handler(void *p_param, uint32_t size)
{
    u32 i = 0;
    int32_t ret_value = 0;
    wfs_scan_stop_resp_t *p_wfs_stop = NULL;

    p_wfs_stop = (wfs_scan_stop_resp_t *)p_param;
    ret_value = p_wfs_stop->ret_value;

    printf("[wifi_scan sample]wfs_scan_stop_handler ret_value (%d) \r\n", ret_value);

    return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void start_wifi_scan_default(void)
{
    wfs_scan_config_t wfs_cfg = {0};
    printf("[wifi_scan sample]start_wifi_scan_default enter \r\n");
    aic_srv_wifi_scan_start(&wfs_cfg, (srv_func_callback)wfs_scan_result_handler);
}

void start_wifi_scan(void)
{
    wfs_scan_config_t wfs_cfg = {0};
    printf("[wifi_scan sample]start_wifi_scan enter \r\n");
    wfs_cfg.round = 3;
    wfs_cfg.max_ap_num = 5;
    wfs_cfg.scan_channel[0] = 6;
    aic_srv_wifi_scan_start(&wfs_cfg, (srv_func_callback)wfs_scan_result_handler);
}

void stop_wifi_scan(void)
{
    aic_srv_wifi_scan_stop((srv_func_callback)wfs_scan_stop_handler);
}

