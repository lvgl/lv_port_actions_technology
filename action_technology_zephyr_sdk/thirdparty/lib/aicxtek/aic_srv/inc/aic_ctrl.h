/**************************************************************************/
/*                                                                        */
/* Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                        */
/**************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file defines aic ctrl module apis.
 */

#ifndef AIC_CTRL_H
#define AIC_CTRL_H

#include <aic_srv_bus.h>

/*
 * aic boot reason define:
 * AIC_CTRL_NORMAL_BOOT: It is booted by call the poweron function
 *                       aic_ctrl_poweron.
 * AIC_CTRL_EXCEPTION_BOOT: any other boot, such as aic watch dog reboot.
 *
 * Don't warry about the boot reason, as long as the state of aic is alived.
 * It can work well.
 */
typedef enum aic_ctrl_boot_reason {
    AIC_CTRL_NORMAL_BOOT = 0,
    AIC_CTRL_EXCEPTION_BOOT
} aic_ctrl_boot_reason_t;

/*
 * aic state defeine:
 * AIC_CTRL_STATE_SHUTDOWN:  The default state or after poweroff aic succ.
 * AIC_CTRL_STATE_BOOTING:   Once call the aic poweron function.
 * AIC_CTRL_STATE_BOOTED:    After power on it, aic has been booted succ.
 * AIC_CTRL_STATE_AILVED     The modem alived, can use modem funtion.
 * AIC_CTRL_STATE_CALI:      Aic enter to calibration mode.
 * AIC_CTRL_STATE_FOTA:      Aic enter to fota update mode.
 * AIC_CTRL_STATE_EXCEPTION: Aic occus exception.
 * AIC_CTRL_STATE_SHUTTING:  Once call the aic poweroff function.
 * AIC_CTRL_STATE_POWERON_TIMEOUT:  poweron aic timeout state.
 * AIC_CTRL_STATE_SHUTDOWN_TIMEOUT: shutdown aic timeout state.
 */
typedef enum aic_state {
    AIC_CTRL_STATE_SHUTDOWN = 0,
    AIC_CTRL_STATE_BOOTING,
    AIC_CTRL_STATE_BOOTED,
    AIC_CTRL_STATE_AILVE,
    AIC_CTRL_STATE_CALI,
    AIC_CTRL_STATE_FOTA,
    AIC_CTRL_STATE_EXCEPTION,
    AIC_CTRL_STATE_SHUTTING,
    AIC_CTRL_STATE_POWERON_TIMEOUT,
    AIC_CTRL_STATE_SHUTDOWN_TIMEOUT
} aic_ctrl_state_t;

typedef struct aic_ctrl_cb {
    aic_ctrl_state_t            state;
    aic_ctrl_boot_reason_t      reason;
} aic_ctrl_cb_t;

typedef struct {
    char*       p_module;
    void*       p_cfg_data;
    uint32_t    cfg_data_len;
} aic_ctrl_cfg_info_t;

/**
 * aic_ctrl_state_register - aic ctrl server register function.
 * @p_bus_info: [IN] bus info.
 * @_srv_callback: [IN] callback function client passed to servic.
 *                 the callback function param:
 *                 p_param for the struc aic_ctrl_cb_t pointer.
 *                 size: for the struct aic_ctrl_cb_t size.
 * Return: > 0 means handle of aic ctrl service, client，< 0 means error.
 */
int aic_ctrl_state_register(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);

/**
 * aic_ctrl_state_unregister - deregister inferface for aic ctrl service.
 * @client_handle: [IN] handle of aic ctrl service client.
 * Return: 0 means success, other means error.
 */
int aic_ctrl_state_unregister(int client_handle);

/**
 * aic_ctrl_poweronoff - poweon or power off the aic.
 *
 * @on: TRUE - power on, FALSE - power off
 * This func is executed asynchronously, the real state of aic can get by
 * register a aic state server callback function.
 *
 */
int aic_ctrl_power_on(bool on);
#define aic_ctrl_poweron()       aic_ctrl_power_on(TRUE)
#define aic_ctrl_poweroff()      aic_ctrl_power_on(FALSE)

/**
 * aic_ctrl_set_poweron_retry_times
 *
 * @retry_times: Set the poweron retry times.
 *               the default retry time is 0xffff.
 *
 * After finished the setting times retry(about 12s at one time), still haven't
 *  waited aic alive, will stop retry and report AIC_CTRL_STATE_POWERON_TIMOUT
 * state by state callback function.
 */
void aic_ctrl_set_poweron_retry_times(unsigned short retry_times);

/**
 * aic_ctrl_set_shutdown_retry_times
 *
 * @retry_times: Set the shutdown retry times.
 *         the default retry time is 0xffff
 *
 * After finished the setting times retry(about 18s at one time), still haven't
 * waited aic shutdown, will stop retry and report AIC_CTRL_STATE_POWERON_TIMOUT
 * state by state callback function.
 */
void aic_ctrl_set_shutdown_retry_times(unsigned short retry_times);

/**
 * aic_ctrl_sync_power_onoff - poweon or power off the aic and will wait
 * .power on or poweroff complete.
 *
 * @on: TRUE - power on, FALSE - power off
 * This func is executed synchronously, it will be blocked until the
 * power on or poweroff action complete.
 * Return: 0 means success, other means error.
 *
 * Attention: It can't be called in init, irq and timer context,
 * It is task ontext safe.
 */
int aic_ctrl_sync_power_onoff(bool on);
#define aic_ctrl_sync_poweron()     aic_ctrl_sync_power_onoff(TRUE)
#define aic_ctrl_sync_poweroff()    aic_ctrl_sync_power_onoff(FALSE)

/**
 * aic_ctrl_reset
 *
 * This func will poweoff and wait aic power off complete,
 * And after aic power off complete will power on the aic,
 * This func will return until power on succ or timeout.

 * @timeout: how long will wait this api return,
 * the unit of the timeou is second, the minium timeout is 30S.
 * Return: 0 means success, other means error.
 */
int aic_ctrl_reset(void);

/**
 * aic_ctrl_assert - assert the aic, only can used for debug.
 */
int aic_ctrl_assert(void);

/**
 * aic_ctrl_srv_set_fota_mode
 * tell the aic ctrl module that will now enter or exit fota mode.
 * In fota mode, will not report the aic alive state to app.
 * The default state is not fota mode.
 *
 * @fota: TRUE - enter fota mode, FALSE: exit  fota mode,
 */
void aic_ctrl_srv_set_fota_mode(bool fota);

/**
 * aic_ctrl_srv_is_fota_mode
 *
 * Return: TURE OR FALSE.
 */
bool aic_ctrl_srv_is_fota_mode(void);

/**
 * aic_ctrl_srv_get_state
 * get the state of aic.
 */
aic_ctrl_state_t aic_ctrl_srv_get_state(void);

/**
 * aic_ctrl_set_config
 * set config info to aic.
 */
int aic_ctrl_set_config(aic_ctrl_cfg_info_t *p_cfg_info);

/**
 * aic_ctrl_is_version_unmatch.
 */
bool aic_ctrl_is_version_unmatch(void);

#endif /* AIC_CTRL_H */
