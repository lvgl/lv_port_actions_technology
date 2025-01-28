/**************************************************************************/
/*                                                                        */
/* Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                        */
/**************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file defines aic fota module apis.
 */
#ifndef AIC_FOTA_H
#define AIC_FOTA_H

typedef enum {
    AIC_FOTA_SUCC = 0,
    AIC_FOTA_STATE_ERR,
    AIC_FOTA_SIZE_ERR,
    AIC_FOTA_NO_MEM
} aic_fota_err_t;

typedef enum {
    AIC_FOTA_UPDATE_COMPLETE = 0,
    AIC_FOTA_PAC_TRNS_TIMEOUT,
    AIC_FOTA_PAC_VER_NOT_MATCH,
    AIC_FOTA_PAC_CHECK_FAIL,
    AIC_FOTA_UPDATE_TIMEOUT,
    AIC_FOTA_UPDATE_STATE_ERR,
    AIC_FOTA_UPDATE_INIT
} aic_fota_update_result_t;

/**
 * aic_fota_start - start the fota update action.
 *
 * @pac_size: the total size of the fota update package.
 *
 * Return: 0 means success, otherwise means error.
 */
int aic_fota_start(u32 pac_size);

/**
 * aic_fota_start_update - start the fota update action.
 *
 * @data: the one of the diff package data buffer pointer.
 * @size: the size of the data.
 *
 * Return: 0 means success, otherwise means error.
 */
int aic_fota_trans_data(void *data, u32 size);

/**
 * aic_fota_wait_update_result - wait the fota update result.
 *
 * @timeout: the max time of wait fota update result, the unit is second.
 *
 * Return: return the update result.
 */
aic_fota_update_result_t aic_fota_wait_update_result(int timout);

/**
 * aic_fota_stop - top the fota update action.
 *
 * will stop the fota update and release all the resource of fota update.
 */
void aic_fota_stop(void);

/**
 * aic_fota_get_aic_remote_ver_num - get the remote aic version number.
 *
 * return:
 *    >= 0, get a version number of the remote aic software.
 *    < 0,  can't get the version number of the remote aic software,
 *          may has some error.
 */
int aic_fota_get_aic_remote_ver_num(void);

/**
 * aic_fota_get_aic_local_ver_num - get the local aic version number.
 *
 * Attention: Can used it is to check if the aic software need update.
 * if the local version number is bigger than the remote version number,
 * It means that need to update the aic version.
 */
int aic_fota_get_aic_local_ver_num(void);

#endif /* AIC_FOTA_H */
